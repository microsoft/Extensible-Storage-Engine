// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//
//  OS File Layer
//
//

#include "osstd.hxx"

#include <winioctl.h>


////////////////////////////////////////
//  Support Functions

#ifndef OS_LAYER_VIOLATIONS
HANDLE HOSLayerTestGetFileHandle( IFileAPI * pfapi )
{
    const HANDLE hFile = ((COSFile*)pfapi)->Handle();
    return hFile;
}
#endif

//  converts the last Win32 error code into an OSFile error code for return via
//  the OSFile API

C_ASSERT( NO_ERROR == ERROR_SUCCESS );  //   deprecated NO_ERROR as there are only 16 hits of it, and 449 hits of ERROR_SUCCESS (clearly the code base has voted) ...

ERR ErrOSFileIFromWinError_( _In_ const DWORD error, _In_z_ PCSTR szFile, _In_ const LONG lLine )
{
    switch ( error )
    {
        case ERROR_SUCCESS:
            return JET_errSuccess;

        case ERROR_IO_PENDING:
            return wrnIOPending;

        case ERROR_INVALID_PARAMETER:
        case ERROR_CALL_NOT_IMPLEMENTED:
        case ERROR_INVALID_ADDRESS:
        case ERROR_NOT_SUPPORTED:
            return ErrERRCheck_( JET_errInvalidParameter, szFile, lLine );

        case ERROR_DISK_FULL:
            return ErrERRCheck_( JET_errDiskFull, szFile, lLine );

        case ERROR_HANDLE_EOF:
            return ErrERRCheck_( JET_errFileIOBeyondEOF, szFile, lLine );

        //  CONSIDER:  allow these to hit the default case because they are
        //  general I/O errors
        //
        case ERROR_VC_DISCONNECTED:
        case ERROR_IO_DEVICE:
        case ERROR_DEVICE_NOT_CONNECTED:
        case ERROR_NOT_READY:
            return ErrERRCheck_( JET_errDiskIO, szFile, lLine );

        case ERROR_FILE_CORRUPT:
        case ERROR_DISK_CORRUPT:
            return ErrERRCheck( JET_errFileSystemCorruption );

        //  CONSIDER:  allow these to hit the default case because they are not
        //  typical errors for ordinary I/O against an open file
        //
        case ERROR_NO_MORE_FILES:
        case ERROR_FILE_NOT_FOUND:
            return ErrERRCheck_( JET_errFileNotFound, szFile, lLine );

        //  CONSIDER:  allow these to hit the default case because they are not
        //  typical errors for ordinary I/O against an open file
        //
        case ERROR_PATH_NOT_FOUND:
        case ERROR_BAD_PATHNAME:
            return ErrERRCheck_( JET_errInvalidPath, szFile, lLine );

        //  CONSIDER:  allow these to hit the default case because they are not
        //  typical errors for ordinary I/O against an open file
        //
        case ERROR_ACCESS_DENIED:
        case ERROR_SHARING_VIOLATION:
        case ERROR_LOCK_VIOLATION:
        case ERROR_WRITE_PROTECT:
            return ErrERRCheck_( JET_errFileAccessDenied, szFile, lLine );

        //  CONSIDER:  map to JET_errOutOfMemory
        //
        case ERROR_TOO_MANY_OPEN_FILES:
            return ErrERRCheck_( JET_errOutOfFileHandles, szFile, lLine );

        case ERROR_INVALID_USER_BUFFER:
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_WORKING_SET_QUOTA:
        case ERROR_NO_SYSTEM_RESOURCES:
        case ERROR_COMMITMENT_LIMIT:
            return ErrERRCheck_( JET_errOutOfMemory, szFile, lLine );

        case ERROR_CRC:
            return ErrERRCheck_( JET_errDiskReadVerificationFailure, szFile, lLine );


        default:
            return ErrERRCheck_( JET_errDiskIO, szFile, lLine );
    }
}

ERR ErrOSFileIIoControlReadOnly(
    _In_ const HANDLE hfile,
    _In_ IFileSystemConfiguration * const pfsconfig,
    _In_ DWORD dwIoControlCode,
    _In_reads_bytes_opt_(cbInBufferSize) PVOID pvInBuffer,
    _In_ DWORD cbInBufferSize,
    _Out_ DWORD* perrorSystem )
{
    ERR err = JET_errSuccess;
    BOOL fSucceeded;
    DWORD cbReturned = 0;
    OVERLAPPED overlapped;

    *perrorSystem = ERROR_SUCCESS;

    memset( &overlapped, 0, sizeof( overlapped ) );

    overlapped.hEvent = CreateEventW( NULL, TRUE, FALSE, NULL );
    if ( NULL == overlapped.hEvent )
    {
        *perrorSystem = GetLastError();
        goto HandleError;
    }

    // Forces the IO to be synchronous, even if the file HANDLE is opened with IO Completion ports.
    overlapped.hEvent = HANDLE( DWORD_PTR( overlapped.hEvent ) | hNoCPEvent );

    fSucceeded = DeviceIoControl( hfile,
                                  dwIoControlCode,
                                  pvInBuffer,
                                  cbInBufferSize,
                                  NULL, // pOutBuffer
                                  0, // cbOutButffer
                                  &cbReturned,
                                  &overlapped );

    if ( !fSucceeded )
    {
        *perrorSystem = GetLastError();
        if ( ERROR_IO_PENDING == *perrorSystem )
        {
            *perrorSystem = ERROR_SUCCESS;
            fSucceeded = GetOverlappedResult_( hfile, &overlapped, &cbReturned, TRUE );
            if ( !fSucceeded )
            {
                *perrorSystem = GetLastError();
                goto HandleError;
            }
        }
        else
        {
            goto HandleError;
        }
    }

HandleError:

    if ( *perrorSystem != ERROR_SUCCESS )
    {
        Assert( ERROR_IO_PENDING != *perrorSystem );    // not bad, just unexpected

        // ERROR_INVALID_FUNCTION is returned on FAT systems.
        if ( *perrorSystem == ERROR_USER_MAPPED_FILE || *perrorSystem == ERROR_INVALID_FUNCTION )
        {
            err = ErrERRCheck( JET_errUnloadableOSFunctionality );
        }
        else
        {
            err = ErrOSFileIFromWinError( *perrorSystem );
        }

        OSTrace( JET_tracetagIOProblems,
                 OSFormat( "%s:  pinst %s failed to do operation (ioctl=%d) with error %s (0x%x).",
                           __FUNCTION__,
                           OSFormatPointer( pfsconfig->PvTraceContext() ),
                           dwIoControlCode,
                           OSFormatSigned( err ) ,
                           *perrorSystem ) );
    }

    if ( overlapped.hEvent != NULL )
    {
        overlapped.hEvent = HANDLE( DWORD_PTR( overlapped.hEvent ) & ~hNoCPEvent );
        CloseHandle( overlapped.hEvent );
    }

    return err;
}


//  post-terminate file subsystem

void OSFilePostterm()
{
    //  nop
}

//  pre-init file subsystem

BOOL FOSFilePreinit()
{
    //  nop

    return fTrue;
}


//  pre-zeroed buffer used to extend files

QWORD           g_cbZero = 1024 * 1024;   // 1 MB default extension buffer size
BYTE*           g_rgbZero;
COSMemoryMap*   g_posmmZero;

void COSLayerPreInit::SetZeroExtend( QWORD cbZeroExtend )
{
    if ( cbZeroExtend < OSMemoryPageCommitGranularity() )
    {
        //  leave at existing alignment
    }
    else if ( cbZeroExtend < OSMemoryPageReserveGranularity() )
    {
        cbZeroExtend -= cbZeroExtend % OSMemoryPageCommitGranularity();
    }
    else
    {
        cbZeroExtend -= cbZeroExtend % OSMemoryPageReserveGranularity();
    }
    g_cbZero = cbZeroExtend;
}

//  parameters

QWORD           g_cbMMSize;

POSTRACEREFLOG  g_pFfbTraceLog = NULL;

//  terminate file subsystem

void OSFileTerm()
{

    if ( g_cbZero < OSMemoryPageCommitGranularity() )
    {
        //  free file extension buffer

        OSMemoryHeapFree( g_rgbZero );
        g_rgbZero = NULL;
    }
    else if ( !COSMemoryMap::FCanMultiMap() ||
        g_cbZero < OSMemoryPageReserveGranularity() )
    {
        //  free file extension buffer

        OSMemoryPageFree( g_rgbZero );
        g_rgbZero = NULL;
    }
    else if ( g_posmmZero )
    {
        //  free file extension buffer

        g_posmmZero->OSMMPatternFree();
        g_rgbZero = NULL;

        //  term the memory map

        g_posmmZero->OSMMTerm();
        delete g_posmmZero;
        g_posmmZero = NULL;
    }

    OSTraceDestroyRefLog( g_pFfbTraceLog );
    g_pFfbTraceLog = NULL;
}


//  init file subsystem

ERR ErrOSFileInit()
{
    ERR     err = JET_errSuccess;
    size_t  cbFill = 0;

#ifdef DEBUG
    (VOID)ErrOSTraceCreateRefLog( 1000, 0, &g_pFfbTraceLog );
#endif

    //  reset all pointers

    g_rgbZero = NULL;
    g_posmmZero = NULL;

    //  fetch our MM block size

    SYSTEM_INFO sinf;
    GetSystemInfo( &sinf );
    g_cbMMSize = sinf.dwAllocationGranularity;

    //  init the file extension buffer

    if ( g_cbZero < OSMemoryPageCommitGranularity() )
    {
        //  allocate the file extension buffer

        if ( !( g_rgbZero = (BYTE*)PvOSMemoryHeapAlloc( size_t( g_cbZero ) ) ) )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }

        cbFill = size_t( g_cbZero );
    }
    else if ( !COSMemoryMap::FCanMultiMap() ||
        g_cbZero < OSMemoryPageReserveGranularity() )
    {
        //  allocate the file extension buffer

        if ( !( g_rgbZero = (BYTE*)PvOSMemoryPageAlloc( size_t( g_cbZero ), NULL ) ) )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }

        cbFill = size_t( g_cbZero );
    }
    else
    {
        //  allocate file extension buffer by allocating the smallest chunk of page
        //  store possible and remapping it consecutively in memory until we hit the
        //  desired chunk size

        //  allocate the memory map object

        g_posmmZero = new COSMemoryMap();
        if ( !g_posmmZero )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }

        //  init the memory map

        COSMemoryMap::ERR errOSMM;
        errOSMM = g_posmmZero->ErrOSMMInit();
        if ( COSMemoryMap::ERR::errSuccess != errOSMM )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }

        //  allocate the pattern

        errOSMM = g_posmmZero->ErrOSMMPatternAlloc( size_t( OSMemoryPageReserveGranularity() ),
            size_t( g_cbZero ),
            (void**)&g_rgbZero );
        if ( COSMemoryMap::ERR::errSuccess != errOSMM )
        {
            AssertSz(   COSMemoryMap::ERR::errOutOfBackingStore == errOSMM ||
                        COSMemoryMap::ERR::errOutOfAddressSpace == errOSMM ||
                        COSMemoryMap::ERR::errOutOfMemory == errOSMM,
                        "unexpected error while allocating memory pattern" );
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }

        cbFill = OSMemoryPageReserveGranularity();
    }

    //  protect the file extension buffer from modification

    if ( g_cbZero >= OSMemoryPageCommitGranularity() )
    {
        for ( size_t ib = 0; ib < g_cbZero; ib += cbFill )
        {
            OSMemoryPageProtect( g_rgbZero + ib, cbFill );
        }
    }

    return JET_errSuccess;

HandleError:
    OSFileTerm();
    return err;
}


////////////////////////////////////////
//  API Implementation

COSFile::COSFile() :
    m_posfs( NULL ),
    m_posv( NULL ),
    m_hFile( INVALID_HANDLE_VALUE ),
    m_p_osf( NULL ),
    m_semChangeFileSize( CSyncBasicInfo( _T( "COSFile::m_semChangeFileSize" ) ) ),
    m_critDefer( CLockBasicInfo( CSyncBasicInfo( _T( "COSFile::m_critDefer" ) ), 0, 0 ) ),
    m_fmf( fmfNone ),
    m_cioUnflushed( 0 ),
    m_cioFlushing( 0 ),
    m_errFlushFailed( JET_errSuccess ),
    m_traceidcheckFile(),
    m_cLogicalCopies( 0 )
{
    //  init our file mapping handles

    for ( INT i = 0; i < 2; i++ )
    {
        m_rghFileMap[ i ] = NULL;
    }
}

ERR COSFile::ErrInitFile(   COSFileSystem* const                posfs,
                            COSVolume * const                   posv,
                            __in PCWSTR const                   wszAbsPath,
                            const HANDLE                        hFile,
                            const QWORD                         cbFileSize,
                            const FileModeFlags                 fmf,
                            const DWORD                         cbIOSize,
                            const DWORD                         cbSectorSize )
{
    ERR err = JET_errSuccess;

    Assert( posfs );
    Assert( posv );
    Assert( wszAbsPath );
    Assert( hFile && INVALID_HANDLE_VALUE != hFile );

#ifndef FORCE_STORAGE_WRITEBACK
    AssertSz( !( fmf & IFileAPI::fmfCached ) || !( fmf & IFileAPI::fmfStorageWriteBack ), "Can't use storage write back with caching together %#x", fmf );
    ExpectedSz( !( fmf & IFileAPI::fmfLossyWriteBack ) || !( fmf & IFileAPI::fmfStorageWriteBack ), "Can't use storage write back with lossy write back together %#x", fmf );
    ExpectedSz( !( fmf & IFileAPI::fmfReadOnly ) || !( fmf & IFileAPI::fmfStorageWriteBack ), "Can't use read only and storage write back together %#x", fmf );
#endif

    //  copy our arguments
    //
    //  callers should have already prevalidated the path,
    //  but be defensive just in case
    //
    m_posfs         = posfs;

    Assert( LOSStrLengthW( wszAbsPath ) < IFileSystemAPI::cchPathMax );
    OSStrCbCopyW( m_wszAbsPath, IFileSystemAPI::cchPathMax * sizeof( WCHAR ), wszAbsPath );

    m_wszAbsPath[ IFileSystemAPI::cchPathMax - 1 ] = L'\0';

    m_hFile         = hFile;
    m_cbFileSize    = cbFileSize;
    m_fmf           = fmf;
    m_cbIOSize      = cbIOSize;
    m_cbSectorSize  = cbSectorSize;

    Assert( m_cbIOSize != 0 );
    Assert( m_cbSectorSize != 0 );
    Assert( ( m_cbSectorSize >= m_cbIOSize ) && ( ( m_cbSectorSize % m_cbIOSize ) == 0 ) );

    //  set our initial file size

    m_rgcbFileSize[ 0 ] = m_cbFileSize;
    m_semChangeFileSize.Release();

    //  set our volume

    m_posv = posv;
    ASSERT_VALID( m_posv );

    //  init our I/O context

    if ( !( m_p_osf = new _OSFILE ) )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    //  set our disk (available to async/enqueued IO)
    //  we should not fail here because the disk is supposed to have been created at this point, when the volume
    //  got connected

    CallS( m_posv->ErrGetDisk( &(m_p_osf->m_posd) ) );
    Assert( m_p_osf->m_posd );
    ASSERT_VALID( m_p_osf->m_posd );

    m_p_osf->m_posf             = this;
    m_p_osf->hFile              = m_hFile;
    m_p_osf->pfnFileIOComplete  = _OSFILE::PfnFileIOComplete( IOComplete_ );
    m_p_osf->keyFileIOComplete  = DWORD_PTR( this );

    //  save the IFileSystemConfiguration of this file

    m_p_osf->pfsconfig          = Pfsconfig();

    //  configure a default IFilePerfAPI

    m_p_osf->pfpapi             = &g_cosfileperfDefault;

    //  get File Index for I/O Heap

    m_p_osf->iFile = QWORD( DWORD_PTR( m_p_osf->hFile ) );

    //  initially enable Scatter/Gather I/O.  we will disable it later if it
    //  is not permitted on this file

    m_p_osf->iomethodMost = IOREQ::iomethodScatterGather;

    //  defer registering with the I/O thread until our first async I/O

    m_p_osf->fRegistered = fFalse;

    //  copy FMF flags (for tracing purposes only)

    m_p_osf->fmfTracing = m_fmf;

    //  it would be natural to do the main posd->TraceStationId( tsidrOpenInit ) station 
    //  identification call here, but it is not because the COSFilePerfAPI which registered 
    //  all the proper Engine FileType/ID goo happens after COSFileSystem::ErrFileOpen with 
    //  the RegisterIFilePerfAPI call.  Consider passing COSFilePerfAPI as a required 
    //  argument to ErrFileOpen() to force ID up front.  In the mean time, we will put the
    //  in main tsidrOpenInit TraceStationId() call in RegisterIFilePerfAPI() so full detail
    //  is registered.  This means if someone opens direct OS file, bypassing CIOFilePerf
    //  will not be correctly identified.  But this is discouraged.

    return JET_errSuccess;

HandleError:
    m_hFile = INVALID_HANDLE_VALUE;
    m_posv = NULL;
    return err;
}

COSFile::~COSFile()
{
    TraceStationId( tsidrCloseTerm );

    //  Only for ese[nt].dll at the moment, as OS layer has a ton of file tests.
#ifdef OS_LAYER_VIOLATIONS
    //  Check that we've flushed all IO ...

    if ( m_errFlushFailed >= JET_errSuccess && !( m_fmf & IFileAPI::fmfTemporary ) )
    {
        Assert( m_cioUnflushed == 0 || FNegTest( fLeakingUnflushedIos ) /* for embeddedunittest */ );
        Assert( m_cioFlushing == 0 );
    }
#endif

    //  SOMEONE requested as defense in depth if we forgot any IOs to do a FFB on the way to 
    //  delete (this should generally not go off).
    if ( CioNonFlushed() > 0 && m_errFlushFailed >= JET_errSuccess && !( m_fmf & IFileAPI::fmfTemporary ) )
    {
#ifdef OS_LAYER_VIOLATIONS
        AssertSz( fFalse, "All ESE-level files should be completely flushed by file close." );
#endif
        (void)ErrFlushFileBuffers( (IOFLUSHREASON) 0x00800000 /* iofrDefensiveCloseFlush not available */ );
    }

    //  tear down our volume 
    
    if ( m_posv )
    {
        OSVolumeDisconnect( m_posv );
        m_posv = NULL;
    }

    if ( m_hFile != INVALID_HANDLE_VALUE )
    {
        SetHandleInformation( m_hFile, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
        CloseHandle( m_hFile );
        m_hFile = INVALID_HANDLE_VALUE;
    }

    if ( m_p_osf )
    {
        if ( m_p_osf->m_posd )
        {
            OSDiskDisconnect( m_p_osf->m_posd, m_p_osf );
            m_p_osf->m_posd = NULL;
        }
    }
    delete m_p_osf;
    m_p_osf = NULL;

    for ( INT i = 0; i < 2; i++ )
    {
        if ( m_rghFileMap[ i ] )
        {
            CloseHandle( m_rghFileMap[ i ] );
            m_rghFileMap[ i ] = NULL;
        }
    }

    //  reset this last because some calls made by this dtor use it

    m_posfs = NULL;
}

void COSFile::TraceStationId( const TraceStationIdentificationReason tsidr )
{
    //  Called (with tsidrPulseInfo) for every IOs.

    if ( m_p_osf == NULL )
    {
        //  COSFileSystem::ErrFileOpen() failure path has a not-fully formed object, but still
        //  needs to call the .dtor.
        return;
    }
    Assert( m_p_osf );
    Assert( m_p_osf->pfpapi );
    Assert( m_p_osf->m_posd );
    //  Until we consider passing COSFilePerfAPI as a required argument to ErrFileOpen() to force 
    //  ID up front we can't defend against this.
    //Assert( m_p_osf->pfpapi != &g_cosfileperfDefault );

    if ( tsidr == tsidrPulseInfo )
    {
        //  vector off to OSDisk to give it a chance to announce as well    

        m_p_osf->m_posd->TraceStationId( tsidr );
    }

    if ( !m_traceidcheckFile.FAnnounceTime< _etguidFileStationId >( tsidr ) )
    {
        return;
    }

    Expected( tsidr == tsidrPulseInfo ||
                tsidr == tsidrOpenInit ||
                tsidr == tsidrCloseTerm ||
                tsidr == tsidrFileRenameFile ||
                tsidr == tsidrFileChangeEngineFileType );
    //  today we log everyting on all these events, but optionally we could suppress like the path on 
    //  say tsidrCloseTerm and tsidrFileChangeEngineFileType case to reduce logging sizes.

    const QWORD hFile = m_p_osf->iFile;
    const DWORD dwEngineFileType = (DWORD)m_p_osf->pfpapi->DwEngineFileType();
    const QWORD qwEngineFileId = m_p_osf->pfpapi->QwEngineFileId();
    const DWORD dwDiskNumber = m_p_osf->m_posd->DwDiskNumber();
    
    ETFileStationId( tsidr, hFile, dwDiskNumber, dwEngineFileType, qwEngineFileId, m_fmf, m_cbFileSize, m_wszAbsPath );
}


ERR COSFile::ErrPath( _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) WCHAR* const wszAbsPath )
{
    // UNDONE_BANAPI:
    OSStrCbCopyW( wszAbsPath, OSFSAPI_MAX_PATH*sizeof(WCHAR), m_wszAbsPath );
    return JET_errSuccess;
}

template< typename Ret, typename Arg1, typename Arg2 >
INLINE Ret CbFailedWithGLE( Arg1, Arg2 )
{
    SetLastError( ErrorThunkNotSupported() );
    return Ret( INVALID_FILE_SIZE );
}

NTOSFuncCustom( g_pfnGetCompressedFileSizeW, g_mwszzFileLibs, GetCompressedFileSizeW, CbFailedWithGLE, oslfExpectedOnWin5x );

LOCAL ERR ErrOSFileIOnDiskSize(
    _In_ JET_PCWSTR wszAbsPath,
    _Out_ QWORD* const pcbSize )
{
    ERR err = JET_errSuccess;
    ULARGE_INTEGER liFileSize = { 0 };
    DWORD errorSystem;

    liFileSize.LowPart = g_pfnGetCompressedFileSizeW( wszAbsPath, &liFileSize.HighPart );
    errorSystem = GetLastError();

    if ( INVALID_FILE_SIZE == liFileSize.LowPart && ERROR_SUCCESS != errorSystem )
    {
        err = ErrOSFileIFromWinError( errorSystem );
        goto HandleError;
    }

    *pcbSize = liFileSize.QuadPart;

HandleError:
    return err;
}

IFileAPI::FileModeFlags COSFile::Fmf() const
{
    return m_fmf;
}

ERR COSFile::ErrSize(
    _Out_ QWORD* const pcbSize,
    _In_ const FILESIZE filesize )
{
    switch ( filesize )
    {
    case IFileAPI::filesizeLogical:
    {
        const INT group = m_msFileSize.Enter();
        *pcbSize = m_rgcbFileSize[ group ];
        m_msFileSize.Leave( group );
        return JET_errSuccess;
    }

    case IFileAPI::filesizeOnDisk:
        return ErrOSFileIOnDiskSize( m_wszAbsPath, pcbSize );

    default:
        AssertSz( fFalse, "Invalid value passed for filesize enum: %d.", filesize );
        return ErrERRCheck( JET_errInternalError );
    }
}

ERR COSFile::ErrIsReadOnly( BOOL* const pfReadOnly )
{
    *pfReadOnly = ( m_fmf & fmfReadOnly );
    return JET_errSuccess;
}

#if defined( USE_HAPUBLISH_API )
extern HaDbFailureTag OSDiskIIOHaTagOfErr( const ERR err, const BOOL fWrite );
#endif

ERR COSFile::ErrFlushFileBuffers( const IOFLUSHREASON iofr )
{
    ERR err = JET_errSuccess;

#ifdef OS_LAYER_VIOLATIONS
    Assert( iofr != 0 );
#endif

    //  For consistency it is better if the error is sticky once thrown (note this is 
    //  on a per file basis).

    if ( m_errFlushFailed != JET_errSuccess )
    {
        return ErrERRCheck( m_errFlushFailed );
    }

    if ( !( m_fmf & fmfStorageWriteBack ) &&
         !( m_fmf & fmfLossyWriteBack ) )
    {
        //  Since we are not in either write back mode, we do not need to actually
        //  trigger the FFB call, but we will zero out the m_cioFlushed counts so
        //  that we can test both modes simultaneously as far as the engine is
        //  concerned.
        m_critDefer.Enter();
        Assert( m_cioFlushing == 0 );
        m_cioUnflushed = 0;
        m_critDefer.Leave();

        m_p_osf->m_posd->TrackOsFfbBegin( IOFLUSHREASON( iofr | iofrOsFakeFlushSkipped ), (QWORD)m_hFile );

        return JET_errSuccess;
    }


    m_critDefer.Enter();

    const LONG64 ciosDelta = AtomicRead( &m_cioUnflushed );
    AtomicAdd( (QWORD*)&m_cioFlushing, ciosDelta );
    AtomicAdd( (QWORD*)&m_cioUnflushed, -ciosDelta );

    Assert( m_cioUnflushed >= 0 );      // note: NOT guaranteed this is zero, due to new IO completing concurrently incrementing count.
    Assert( m_cioFlushing >= ciosDelta );

    m_critDefer.Leave();

    m_p_osf->m_posd->TrackOsFfbBegin( iofr, (QWORD)m_hFile );
    if ( g_pFfbTraceLog )
    {
        OSTraceWriteRefLog( g_pFfbTraceLog, (LONG)m_cioUnflushed, this );
    }

    HRT hrtStart = HrtHRTCount();

    //  RFS IO
    //
    BOOL fFaultedFlushSucceeded = RFSAlloc( OSFileFlush );
    if ( !fFaultedFlushSucceeded )
    {
        // Be nice to have something like RFSAlloc()-like wrapper that works on NT calls.
        UtilSleep( 80 );
        SetLastError( ERROR_BROKEN_PIPE /* hopefully odd enough to cause people to look at code */ );
    }

    DWORD error = ERROR_SUCCESS;

    //  CONSIDER: Should we have some sort of locking at COSFile or COSDisk on running 
    //  concurrent FFB calls?  I can't find any documentation that it is not supported,
    //  so it is only a potentially inefficiency (pointless)
    if ( !fFaultedFlushSucceeded || !FlushFileBuffers( m_hFile ) )
    {
        error = GetLastError();
        err = ErrOSFileIFromWinError( error );
        Assert( ERROR_IO_PENDING != error );    // not bad, just unexpected
        Assert( err != wrnIOPending );
        if ( error == ERROR_SUCCESS || err >= JET_errSuccess )
        {
            FireWall( "FfbFailedWithSuccess" );
            error = ERROR_BAD_PIPE;  // following above convention
            err = ErrERRCheck( errCodeInconsistency );
        }
    }

    const HRT dhrtFfb = HrtHRTCount() - hrtStart;
    const QWORD usFfb = CusecHRTFromDhrt( dhrtFfb );

    //  We do this at the OSDisk layer because we want to track distance between different FFB
    //  calls that may come from different files but be on the same disk.
    //m_p_osf->m_posd->TrackOsFfbOperation( iofr, error, hrtStart, usFfb, ciosDelta, m_posfs->WszPathFileName( WszFile() ) );
    m_p_osf->m_posd->TrackOsFfbComplete( iofr, error, hrtStart, usFfb, ciosDelta, m_posfs->WszPathFileName( WszFile() ) );

    if ( err >= JET_errSuccess )
    {
        m_p_osf->pfpapi->IncrementFlushFileBuffer( dhrtFfb );
        
        m_critDefer.Enter();
        AtomicAdd( (QWORD*)&m_cioFlushing, -ciosDelta );
        Assert( m_cioFlushing >= 0 );       //  note: Again due to concurrent flushes, this may not be zero.
        m_critDefer.Leave();

    }
    else // err!
    {
        //  Reset Tracking
        //
        m_critDefer.Enter();
        (void)AtomicCompareExchange( (LONG*)&m_errFlushFailed, (LONG)JET_errSuccess, (LONG)err );
        AtomicAdd( (QWORD*)&m_cioUnflushed, ciosDelta );
        AtomicAdd( (QWORD*)&m_cioFlushing, -ciosDelta );
        Assert( m_cioFlushing >= 0 );       //  note: Again due to concurrent flushes, this may not be zero.
        m_critDefer.Leave();

        //  Log issue ... 
        //  
        WCHAR   wszAbsPath[ IFileSystemAPI::cchPathMax ];
        WCHAR   wszLatency[ 64 ];
        WCHAR   wszSystemError[ 64 ];
        WCHAR   wszErr[ 64 ];
        WCHAR*  wszSystemErrorDescription = NULL;
    
        CallS( ErrPath( wszAbsPath ) );
        OSStrCbFormatW( wszLatency, sizeof( wszLatency ), L"%0.3f", (float)CusecHRTFromDhrt( HrtHRTCount() - hrtStart ) / 1000000.0 );
        OSStrCbFormatW( wszErr, sizeof( wszErr ), L"%i (0x%08x)", err, err );
        OSStrCbFormatW( wszSystemError, sizeof( wszSystemError ), L"%u (0x%08x)", error, error );
        FormatMessageW( ( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK ),
                        NULL,
                        error,
                        MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ),
                        LPWSTR( &wszSystemErrorDescription ),
                        0,
                        NULL );
    
        const WCHAR *   rgpwsz[] = { wszAbsPath, wszLatency, wszSystemError, wszSystemErrorDescription, wszErr };
        
        OSTrace( JET_tracetagFlushFileBuffers, OSFormat( "COSFile::Flush Failed[%8x] err %d (error %d) - %ws", (DWORD)iofr, err, error, wszAbsPath ) );
        m_p_osf->Pfsconfig()->EmitEvent(    eventError,
                                            GENERAL_CATEGORY,
                                            OSFILE_FLUSH_ERROR_ID,
                                            _countof(rgpwsz),
                                            rgpwsz,
                                            JET_EventLoggingLevelMin );

#if defined( USE_HAPUBLISH_API )
        m_p_osf->Pfsconfig()->EmitEvent(    OSDiskIIOHaTagOfErr( err, fTrue ),
                                            Ese2HaId( GENERAL_CATEGORY ),
                                            Ese2HaId( OSFILE_FLUSH_ERROR_ID ),
                                            _countof(rgpwsz),
                                            rgpwsz,
                                            HaDbIoErrorWrite,
                                            wszAbsPath );
#endif

        AssertTrack( FRFSFailureDetected( OSFileFlush ), OSFormat( "FFbError:%I32u", error ) );
    
        LocalFree( wszSystemErrorDescription );
    }

    return err;
}

LONG64 COSFile::CioNonFlushed() const
{
    return m_cioUnflushed + m_cioFlushing;
}

void COSFile::SetNoFlushNeeded()
{
    m_cioUnflushed = 0;
}

ERR COSFile::ErrSetSize(
    const TraceContext& tc,
    const QWORD         cbSize,
    const BOOL          fZeroFill,
    const OSFILEQOS     grbitQOS
    )
{
    CIOComplete iocomplete;
    GetCurrUserTraceContext getutc;

    Assert( tc == *PetcTLSGetEngineContext() );
    Assert( qosIODispatchMask & grbitQOS ); // at least one dispatch bit should be set ...
    Assert( !( m_fmf & IFileAPI::fmfReadOnlyClient ) );
    Expected( !( m_fmf & IFileAPI::fmfReadOnly ) ); // will fail at OS op, but unexpected.

    //  allocate an extending write request

    CExtendingWriteRequest* const pewreq = new CExtendingWriteRequest();
    if ( !pewreq )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    //  allocate an I/O request structure

    IOREQ* pioreq = NULL;

    Assert( qosIODispatchImmediate == ( qosIODispatchMask & grbitQOS ) );
    CallS( m_p_osf->m_posd->ErrAllocIOREQ( grbitQOS, NULL, fTrue, 0, 0, &pioreq ) );

    Assert( NULL != pioreq );

    //  Set early, so we can validate things properly.
    pioreq->p_osf           = m_p_osf;
    
    //  mark IOREQ as being used to set size
    //
    pioreq->SetIOREQType( IOREQ::ioreqSetSize );
    pioreq->grbitQOS = grbitQOS;

    Assert( tc.iorReason.Iorp() != iorpNone );
    pioreq->m_tc.DeepCopy( getutc.Utc(), tc );

    //  wait until we can change the file size

    m_semChangeFileSize.Acquire();
    const INT group = m_msFileSize.GroupActive();

    if ( m_rgcbFileSize[ group ] >= cbSize )
    {
        // setup a null extending write request at the desired file size
        pioreq->ibOffset        = cbSize;
    }
    else if ( ErrFaultInjection( 47100 ) >= JET_errSuccess &&
              ErrIOSetFileSize( cbSize, fFalse /* fReportError */ ) >= JET_errSuccess &&
              // for sparse files, we cannot depend on deferred NTFS zeroing
              // of the extended region as NTFS will sparsify the region under
              // us at a later time and we will end up with dirty buffers not
              // backed by allocated regions on disk and hence possible disk
              // full error on flushing those buffers
              !( m_fmf & fmfSparse ) )
    {
        // File is now extended, filesystem will zero fill on demand
        pioreq->ibOffset        = cbSize;

        // If zero fill not requested (or needed), try to extend file valid
        // offset so that NTFS does not have to do IO for zero filling
        if ( !fZeroFill || ( m_fmf & fmfTemporary ) )
        {
            m_p_osf->semFilePointer.Acquire();
            (void)SetFileValidData( m_hFile, cbSize );
            m_p_osf->semFilePointer.Release();
        }
    }
    else
    {
        // If SetFilePointerEx fails (or sparse file), extend the file using
        // extending writes
        pioreq->ibOffset        = m_rgcbFileSize[ group ];
    }

    pioreq->p_osf               = m_p_osf;
    pioreq->fWrite              = fTrue;
    pioreq->group               = group;

    pioreq->cbData              = 0;
    pioreq->pbData              = NULL;
    pioreq->dwCompletionKey     = DWORD_PTR( pewreq );
    pioreq->pfnCompletion       = PFN( IOZeroingWriteComplete_ );

    pioreq->ovlp.Offset         = (ULONG) ( pioreq->ibOffset );
    pioreq->ovlp.OffsetHigh     = (ULONG) ( pioreq->ibOffset >> 32 );
    pioreq->pioreqIorunNext     = NULL;
    pioreq->pioreqVipList       = NULL;

    pewreq->m_posf              = this;
    pewreq->m_pioreq            = pioreq;
    pewreq->m_group             = group;
    pewreq->m_tc.DeepCopy( getutc.Utc(), tc );
    pewreq->m_ibOffset          = cbSize;       // this is the final size requested.
    pewreq->m_cbData            = 0;
    pewreq->m_pbData            = NULL;
    pewreq->m_grbitQOS          = grbitQOS;
    pewreq->m_pfnIOComplete     = PfnIOComplete( IOSyncComplete_ );
    pewreq->m_keyIOComplete     = DWORD_PTR( &iocomplete );
    pewreq->m_tickReqStart      = TickOSTimeCurrent();
    pewreq->m_tickReqStep       = 0;
    pewreq->m_tickReqComplete   = 0;
    
    OSDiskIIOThreadCompleteWithErr( ERROR_SUCCESS,
                                    pioreq->cbData,
                                    pioreq );

    //  wait for the I/O completion and return its result

    if ( !iocomplete.FComplete() )
    {
        CallS( ErrIOIssue() );
        iocomplete.Wait();
    }
    return iocomplete.m_err;
}

ERR COSFile::ErrRename( const WCHAR* const  wszAbsPathDest,
                        const BOOL          fOverwriteExisting )
{
    DWORD error = ERROR_SUCCESS;
    ERR err = JET_errSuccess;
    OSFSRETRY osfsRetry( Pfsconfig() );

    Assert( !( m_fmf & IFileAPI::fmfReadOnlyClient ) );
    Expected( !( m_fmf & IFileAPI::fmfReadOnly ) ); // will fail at OS op, but unexpected.

    if ( m_posfs->FPathIsRelative( wszAbsPathDest ) )
    {
        Assert( !m_posfs->FPathIsRelative( wszAbsPathDest ) || FNegTest( fInvalidUsage ) );
        return ErrERRCheck( JET_errInvalidParameter );
    }

/*
 *  // SOMEONE_2020-10-14 - This assert fires incorrectly with LogsOnSSD symbolic links. [ O36Core.1735244 ]
    //
#ifdef DEBUG
    // Catch usage errors where the file is being moved to a different volume. No one should be doing that using this function.
    // SetFileInformationByHandle returns ERROR_NOT_SAME_DEVICE in this case, which is mapped to JET_errDiskIO.

    IVolumeAPI * pvolapi = NULL;
    if ( JET_errSuccess <= ErrOSVolumeConnect( m_posfs, wszAbsPathDest, &pvolapi ) )
    {
        Assert( pvolapi );
        Assert( m_posv == PosvFromVolAPI( pvolapi ) );
        OSVolumeDisconnect( pvolapi );
    }
#endif
 *
 */

    // No one calls this on memory-mapped files today. It should work but we didn't test this.
    Expected( m_rghFileMap[ 0 ] == NULL && m_rghFileMap[ 1 ] == NULL );
    
    const size_t cchPathDest = wcslen( wszAbsPathDest );
    const size_t cbPathDest = ( cchPathDest + 1 ) * sizeof( WCHAR );
    const DWORD cbBuffer = sizeof( FILE_RENAME_INFO ) + cbPathDest;
    FILE_RENAME_INFO* const pRenameInfo = (FILE_RENAME_INFO*) alloca( cbBuffer );
    
    memset( pRenameInfo, 0, sizeof( FILE_RENAME_INFO ) );

    OSStrCbCopyW( pRenameInfo->FileName, cbPathDest, wszAbsPathDest );
    pRenameInfo->FileNameLength = cchPathDest;
    pRenameInfo->ReplaceIfExists = fOverwriteExisting == fTrue;
    pRenameInfo->RootDirectory = NULL;

    do
    {
        if ( !SetFileInformationByHandle( m_hFile, FileRenameInfo, pRenameInfo, cbBuffer ) )
        {
            error = GetLastError();
            Assert( error != ERROR_NOT_SAME_DEVICE );
            err = ErrOSErrFromWin32Err( error, JET_errDiskIO );
        }
    }
    while ( osfsRetry.FRetry( err ) );
    Call( err );

    OSTrace( JET_tracetagFile, OSFormat( "COSFile::ErrRename() %ws --> %ws", m_wszAbsPath, wszAbsPathDest ) );

    OSStrCbCopyW( m_wszAbsPath, IFileSystemAPI::cchPathMax * sizeof( WCHAR ), wszAbsPathDest );

    TraceStationId( tsidrFileRenameFile );

    return JET_errSuccess;

HandleError:
    m_posfs->ReportFileError( OSFS_MOVE_FILE_ERROR_ID, m_wszAbsPath, wszAbsPathDest, err, error );
    return err;
}

ERR COSFile::ErrSetSparseness()
{
    // Set FSCTL_SET_SPARSE.
    return ErrIOSetFileSparse( m_hFile, fTrue );
}

ERR COSFile::ErrIOTrim( const TraceContext& tc,
                        const QWORD         ibOffset,
                        const QWORD         cbToFree
                        )
{
    ERR err;
    AssertRTL( tc.iorReason.FValid() );
    Assert( !( m_fmf & IFileAPI::fmfReadOnlyClient ) );
    Expected( !( m_fmf & IFileAPI::fmfReadOnly ) ); // will fail at OS op, but unexpected.

    Call( ErrIOSetFileRegionSparse( m_hFile, ibOffset, cbToFree, fTrue ) );

HandleError:
    return err;
}

ERR COSFile::ErrRetrieveAllocatedRegion( const QWORD            ibOffsetToQuery,
                                      _Out_ QWORD* const    pibStartAllocatedRegion,
                                      _Out_ QWORD* const    pcbAllocated )
{
    ERR err;

    Call( ErrIOGetAllocatedRange( m_hFile, ibOffsetToQuery, pibStartAllocatedRegion, pcbAllocated ) );

HandleError:
    return err;
}

ERR COSFile::ErrIOSize( DWORD* const pcbSize )
{
    Assert( m_cbIOSize != 0 );
    *pcbSize = m_cbIOSize;
    return JET_errSuccess;
}

ERR COSFile::ErrSectorSize( DWORD* const pcbSize )
{
    Assert( m_cbSectorSize != 0 );
    *pcbSize = m_cbSectorSize;
    return JET_errSuccess;
}

ERR COSFile::ErrReserveIOREQ(
    const QWORD         ibOffset,
    const DWORD         cbData,
    const OSFILEQOS     grbitQOS,
    VOID **             ppioreq
    )
{
    return m_p_osf->m_posd->ErrAllocIOREQ( grbitQOS, m_p_osf, fFalse, ibOffset, cbData, (IOREQ **)ppioreq );
}

VOID COSFile::ReleaseUnusedIOREQ(
    VOID *              pioreq
    )
{
    m_p_osf->m_posd->FreeIOREQ( (IOREQ *)pioreq );
}

//  Gives value between 0 and ( <usRangeMax> - 1 ), with a (nearly) evenly distributed probability.
USHORT UsEvenRand( const USHORT usRangeMax )
{
    Expected( usRangeMax <= 4100 ); // if you go higher, you lower the probability of even distribution to > 1 in 1M.
    unsigned int uiT;
    //  Use rand_s, b/c it's 32-bit and so only a 1 in ~4.3 M chance of getting the last bit
    //  of the range and an inaccurate probability.
    //  Note: rand_s() tested to be ~2x as slow as rand() ... about .088 us (very consistent)
    //  for rand() to a less consistent 0.14 - 0.17 us for rand_s(), so still very fast.  
    //  Hopefully there are no systems where this is problematically slow ... not sure how 
    //  they are achieving crypt-secure rands, but we do not care about security here.
    //  Note: The exact string "errno" is kind of like a CRT quasi-global value, so trying
    //  to use it as a local: 
    //       errno_t errno = rand_s( &uiT );
    //  ...does not work!  Variable must be named like "errnoT" instead of plain "errno".
    errno_t errnoT = rand_s( &uiT );
    AssertSz( errnoT == 0, "rand_s() returned %d\n", errnoT );
    return (USHORT)( uiT % (ULONG)usRangeMax );
}

BOOL COSFile::FOSFileTestUrgentIo()
{
    return UsEvenRand( 1000 ) >= Pfsconfig()->PermillageSmoothIo();
}

ERR COSFile::ErrIORead( const TraceContext& tc,
                        const QWORD         ibOffset,
                        const DWORD         cbData,
__out_bcount( cbData )  BYTE* const         pbData,
                        const OSFILEQOS     grbitQOSIn,
                        const PfnIOComplete pfnIOComplete,
                        const DWORD_PTR     keyIOComplete,
                        const PfnIOHandoff  pfnIOHandoff,
                        const VOID *        pvioreq )
{
    ERR err = JET_errSuccess;
    OSFILEQOS grbitQOS = grbitQOSIn;

    AssertRTL( tc.iorReason.FValid() );
    Assert( tc == *PetcTLSGetEngineContext() );

    Assert( cbData );   // real disk ops have a size

    Assert( qosIODispatchMask & grbitQOS ); // at least one dispatch bit should be set ...
    Assert( ( ( qosIODispatchMask & grbitQOS ) == qosIODispatchImmediate ) ||
            ( ( qosIODispatchMask & grbitQOS ) == qosIODispatchBackground ) );
    Assert( !( ( grbitQOS & qosIODispatchBackground ) && ( grbitQOS & qosIODispatchImmediate ) ) );

    Assert( !( qosIOSignalSlowSyncIO & grbitQOS ) || pfnIOComplete == NULL || pfnIOComplete == PfnIOComplete( IOSyncComplete_ ) );

    Assert( !( qosIOReadCopyMask & grbitQOS ) || pfnIOComplete == NULL || pfnIOComplete == PfnIOComplete( IOSyncComplete_ ) ); // ReadCopy only allowed on SyncIO.

    Assert( 0 == ( qosIOCompleteMask & grbitQOS ) ); // no completion signals should be set

    if ( Pfsconfig()->PermillageSmoothIo() != 0 && !( grbitQOS & qosIOReadCopyMask ) )
    {
        if ( FOSFileTestUrgentIo() )
        {
            //  This one is lucky, mark an urgent IO.
            grbitQOS = ( grbitQOS & ~qosIODispatchBackground ) | qosIODispatchImmediate;
        }
        else
        {
            //  This one not so much, house usually wins - mark an background IO.
            //  Must both strip the qosIODispatchImmediate and add qosIODispatchBackground, as these 
            //  two flags can't be used together.
            grbitQOS = ( grbitQOS & ~qosIODispatchImmediate ) | qosIODispatchBackground;
        }
    }

    //  a completion routine was specified

    if ( pfnIOComplete )
    {
        //  allocate an I/O request structure

        IOREQ* pioreq = NULL;
        if ( pvioreq != NULL )
        {
            pioreq = (IOREQ *)pvioreq;

            Assert( !pioreq->m_fDequeueCombineIO );
            Assert( !pioreq->m_fSplitIO );
            Assert( !pioreq->m_fReMergeSplitIO );
            Assert( !pioreq->m_fOutOfMemory );
            Assert( pioreq->m_cTooComplex == 0 );
            Assert( pioreq->m_cRetries == 0 );

            Assert( pioreq->FEnqueueable() || pioreq->m_fCanCombineIO );
        }
        else
        {
            Call( m_p_osf->m_posd->ErrAllocIOREQ( grbitQOS, m_p_osf, fFalse, ibOffset, cbData, &pioreq ) );
        }

        //  use it to perform the I/O asynchronously

        Call( ErrIOAsync(   pioreq,
                            fFalse,
                            grbitQOS,
                            0,
                            ibOffset,
                            cbData,
                            pbData,
                            pfnIOComplete,
                            keyIOComplete,
                            pfnIOHandoff ) );
    }

    //  a completion routine was not specified

    else
    {
        CIOComplete iocomplete;
        iocomplete.m_keyIOComplete = keyIOComplete;
        iocomplete.m_pfnIOHandoff = pfnIOHandoff;

        Assert( !( grbitQOS & qosIOOptimizeCombinable ) );
        Assert( NULL == pvioreq );

        //  testing exclusive IO requests ...

        OSFILEQOS   qosEffective = grbitQOS;
        if ( ErrFaultInjection( 59364 ) )
        {
            //  To ensure the read copy code works we'll use the Nth+1 copy we support for
            //  testing the exclusive IO path.  We must also force the Dispatch level up to
            //  immediate because you can't read exclusively from background thread.
            qosEffective = ( qosEffective & ~qosIODispatchBackground ) | qosIODispatchImmediate | qosIOReadCopyTestExclusiveIo;
        }

        //  perform the I/O asynchronously

        // we have to think carefully about what sync IO would mean if it wasn't immediate dispatch
        Assert( qosIODispatchImmediate == ( grbitQOS & qosIODispatchMask ) || qosIODispatchBackground == ( grbitQOS & qosIODispatchMask ) );
        CallS( ErrIORead(   tc,
                            ibOffset,
                            cbData,
                            pbData,
                            qosEffective,
                            PfnIOComplete( IOSyncComplete_ ),
                            DWORD_PTR( &iocomplete ),
                            ( pfnIOHandoff != NULL ) ? PfnIOHandoff( IOSyncHandoff_ ) : NULL ) );

        //  wait for the I/O completion and return its result

        if ( !iocomplete.FComplete() )
        {
            CallS( ErrIOIssue() );
            iocomplete.Wait();
        }
        Call( iocomplete.m_err );
    }

    CallSx( err, wrnIOSlow );

    return ( pfnIOComplete == NULL && ( qosIOSignalSlowSyncIO & grbitQOS ) ) ? err : JET_errSuccess;

HandleError:

#ifdef DEBUG
    if ( pfnIOComplete )
    {
        //  for async IO we have a strong contract, success (above) or quota limit 
        //  reached or beyond EOF ...

        Assert( ( errDiskTilt == err ) || ( JET_errFileIOBeyondEOF == err ) );
        Expected( ( errDiskTilt != err ) ||
                    ( ( qosIODispatchImmediate != ( grbitQOS & qosIODispatchMask ) ) ||
                        ( grbitQOS & qosIOOptimizeCombinable ) ) );
    }
    // else a whole bunch of errors, see ErrOSFileIFromWinError() ...
#endif

    return err;
}

ERR COSFile::ErrIOWrite(    const TraceContext& tc,
                            const QWORD         ibOffset,
                            const DWORD         cbData,
                            const BYTE* const   pbData,
                            const OSFILEQOS     grbitQOS,
                            const PfnIOComplete pfnIOComplete,
                            const DWORD_PTR     keyIOComplete,
                            const PfnIOHandoff  pfnIOHandoff  )
{
    ERR err = JET_errSuccess;

    Assert( !( m_fmf & IFileAPI::fmfReadOnlyClient ) );
    Expected( !( m_fmf & IFileAPI::fmfReadOnly ) ); // will fail at OS op, but unexpected.

    AssertRTL( tc.iorReason.FValid() );
    Assert( tc == *PetcTLSGetEngineContext() );

    Assert( cbData );   // real disk ops have a size

    Assert( qosIODispatchMask & grbitQOS ); // at least one dispatch bit should be set ...
    Assert( !( ( grbitQOS & qosIODispatchBackground ) && ( grbitQOS & qosIODispatchImmediate ) ) );

    Assert( !( qosIOSignalSlowSyncIO & grbitQOS ) || pfnIOComplete == NULL || pfnIOComplete == PfnIOComplete( IOSyncComplete_ ) );

    Assert( 0 == ( qosIOCompleteMask & grbitQOS ) ); // no completion signals should be set

    //  a completion routine was specified

    if ( pfnIOComplete )
    {
        //  allocate an I/O request structure

        IOREQ* pioreq = NULL;
        Call( m_p_osf->m_posd->ErrAllocIOREQ( grbitQOS, m_p_osf, fTrue, ibOffset, cbData, &pioreq ) );

        //  use it to perform the I/O asynchronously

        Call( ErrIOAsync(   pioreq,
                            fTrue,
                            grbitQOS,
                            m_msFileSize.Enter(),
                            ibOffset,
                            cbData,
                            (BYTE* const)pbData,
                            pfnIOComplete,
                            keyIOComplete,
                            pfnIOHandoff ) );

    }

    //  a completion routine was not specified

    else
    {
        CIOComplete iocomplete;
        iocomplete.m_keyIOComplete = keyIOComplete;
        iocomplete.m_pfnIOHandoff = pfnIOHandoff;

        Assert( !( grbitQOS & qosIOOptimizeCombinable ) );

        //  perform the I/O asynchronously

        // we have to think carefully about what sync IO would mean if it wasn't immediate dispatch
        Assert( qosIODispatchImmediate == ( grbitQOS & qosIODispatchMask ) );
        CallS( ErrIOWrite(  tc,
                            ibOffset,
                            cbData,
                            pbData,
                            grbitQOS,
                            PfnIOComplete( IOSyncComplete_ ),
                            DWORD_PTR( &iocomplete ),
                            ( pfnIOHandoff != NULL ) ? PfnIOHandoff( IOSyncHandoff_ ) : NULL ) );

        //  wait for the I/O completion and return its result

        if ( !iocomplete.FComplete() )
        {
            CallS( ErrIOIssue() );
            iocomplete.Wait();
        }
        Call( iocomplete.m_err );
    }

    CallSx( err, wrnIOSlow );

    return ( pfnIOComplete == NULL && ( qosIOSignalSlowSyncIO & grbitQOS ) ) ? err : JET_errSuccess;

HandleError:

#ifdef DEBUG
    if ( pfnIOComplete )
    {
        //  for async IO we have a strong contract, success (above) or quota limit reached

        Assert( errDiskTilt == err );
        // it should be impossible to get JET_errFileIOBeyondEOF here, we extend the file to
        // make offsets valid first.
    }
    // else a whole bunch of errors, see ErrOSFileIGetLastError() ...
#endif

    return err;
}

void IOWriteContiguousComplete_(    const ERR                   err,
                                    COSFile* const              posf,
                                    const FullTraceContext&     tc,
                                    const OSFILEQOS             grbitQOS,
                                    const QWORD                 ibOffset,
                                    const DWORD                 cbData,
                                    BYTE* const                 pbData,
                                    COSFile::CIOComplete* const piocomplete )
{
    if ( piocomplete )
    {
        piocomplete->Complete( err );
    }
    else
    {
        AssertSz( fFalse, "Async IO CIOComplete shoud not be NULL!" );
    }
}

ERR ErrIOWriteContiguous(   IFileAPI* const                         pfapi,
                            __in_ecount( cData ) const TraceContext rgtc[],
                            const QWORD                             ibOffset,
                            __in_ecount( cData ) const DWORD        rgcbData[],
                            __in_ecount( cData ) const BYTE* const  rgpbData[],
                            const size_t                            cData,
                            const OSFILEQOS                         grbitQOS )
{
    ERR err = JET_errSuccess;
    BOOL fAllocatedFromHeap = fFalse;

    Assert( qosIODispatchMask & grbitQOS ); // at least one dispatch bit should be set ...
    Assert( !( pfapi->Fmf() & IFileAPI::fmfReadOnlyClient ) );
    Expected( !( pfapi->Fmf() & IFileAPI::fmfReadOnly ) ); // will fail at OS op, but unexpected.

    Expected( ( grbitQOS & qosIOOptimizeCombinable ) );

    Assert( 0 == ( qosIOCompleteMask & grbitQOS ) ); // no completion signals should be set

    // pre-allocated from stack and allocate from heap if there are more than 2 IOs.
    COSFile::CIOComplete rgStackIoComplete[2];
    COSFile::CIOComplete *rgIoComplete  = rgStackIoComplete;

    if ( cData > _countof(rgStackIoComplete) )
    {
        // allocate from heap
        rgIoComplete = new COSFile::CIOComplete[cData];
        if ( rgIoComplete == NULL )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }
        fAllocatedFromHeap = fTrue;
    }

    //  perform the I/O asynchronously

    QWORD ibOffsetCurrent = ibOffset;
    size_t iData = 0;
    for ( iData = 0; iData < cData; ++iData )
    {
        Assert( rgcbData[iData] );  // real disk ops have a size
        TraceContextScope tcScope;
        *tcScope = rgtc[ iData ];
        err = pfapi->ErrIOWrite(    *tcScope,
                                    ibOffsetCurrent,
                                    rgcbData[iData],
                                    rgpbData[iData],
                                    grbitQOS,
                                    IFileAPI::PfnIOComplete( IOWriteContiguousComplete_ ),
                                    DWORD_PTR( &rgIoComplete[iData] ) );
        Assert( err != errDiskTilt );
        if ( err != JET_errSuccess )
        {
            // no need to make further io requests.
            break;
        }
        ibOffsetCurrent += rgcbData[iData];
    }
    //  wait for the I/O completion and return its result
    if ( iData > 0 )
    {
        CallS( pfapi->ErrIOIssue() );
    }
    for ( size_t i = 0; i < iData; ++i )
    {
        
        // we still need to wait for successfully scheduled requests.
        rgIoComplete[i].Wait();
    }
    if ( iData == cData )
    {
        // check error code only when all I/O requests are completed successfully.
        for ( size_t j = 0; j < cData; ++j )
        {
            Call( rgIoComplete[j].m_err );
        }
    }
    CallSx( err, wrnIOSlow );

HandleError:
    if ( fAllocatedFromHeap )
    {
        Assert( rgIoComplete != rgStackIoComplete );
        Assert( rgIoComplete != NULL );
        delete [] rgIoComplete;
        rgIoComplete = NULL;
    }
    if ( err == wrnIOSlow || err ==  JET_errSuccess )
    {
        return ( ( qosIOSignalSlowSyncIO & grbitQOS ) ) ? err : JET_errSuccess;
    }
    else
    {
        Assert( err < 0 );
        return err;
    }
}

// Given the range [ibFirst, ibLast], finds all segments which are sparse and fully
// contained within.

ERR ErrIORetrieveSparseSegmentsInRegion(    IFileAPI* const                             pfapi,
                                            _In_ QWORD                                  ibFirst,
                                            _In_ QWORD                                  ibLast,
                                            _Inout_ CArray<SparseFileSegment>* const    parrsparseseg )
{
    ERR err = JET_errSuccess;

    Expected( ibLast >= ibFirst );
    Assert( parrsparseseg != NULL );
    Expected( parrsparseseg->Size() == 0 );

    for ( QWORD ib = ibFirst; ib <= ibLast; )
    {
        QWORD ibAlloc = 0, cbAlloc = 0;
        Call( pfapi->ErrRetrieveAllocatedRegion( ib, &ibAlloc, &cbAlloc ) );

        if ( ( ibAlloc > ib ) || ( cbAlloc == 0 ) )
        {
            // ib is a sparse byte.
            SparseFileSegment sparseseg;
            sparseseg.ibFirst = ib;
            if ( cbAlloc == 0 )
            {
                Assert( ibAlloc == 0 );
                sparseseg.ibLast = ibLast;
            }
            else
            {
                Assert( ibAlloc > ib );
                sparseseg.ibLast = min( ibAlloc - 1, ibLast );
            }

            Call( ( parrsparseseg->ErrSetEntry( parrsparseseg->Size(), sparseseg ) == CArray<SparseFileSegment>::ERR::errSuccess ) ?
                                                                                      JET_errSuccess :
                                                                                      ErrERRCheck( JET_errOutOfMemory ) );
        }
        else
        {
            // ib is an allocated (i.e., non-sparse) byte.
            Assert( ibAlloc == ib );
            Assert( cbAlloc != 0 );
        }

        ib = ( cbAlloc != 0 ) ? ( ibAlloc + cbAlloc ) : ( ibLast + 1 );
    }

HandleError:

#ifdef DEBUG
    for ( size_t isparseseg = 0; isparseseg < parrsparseseg->Size(); isparseseg++ )
    {
        const SparseFileSegment& sparseseg = (*parrsparseseg)[isparseseg];
        Assert( ibFirst <= ibLast );
        Assert( sparseseg.ibFirst <= sparseseg.ibLast );
        Assert( sparseseg.ibFirst >= ibFirst );
        Assert( sparseseg.ibLast <= ibLast );
        if ( isparseseg > 0 )
        {
            Assert( sparseseg.ibFirst > (*parrsparseseg)[isparseseg - 1].ibLast );
        }
    }
#endif

    return err;
}

// Exchange build environment has the right headers but is building with 0x0601
#if (_WIN32_WINNT < 0x0602 )
#define MARK_HANDLE_READ_COPY               (0x00000080)
#define MARK_HANDLE_NOT_READ_COPY           (0x00000100)
#endif

ERR _OSFILE::ErrSetReadCopyNumber( LONG iCopyNumber )
{
    MARK_HANDLE_INFO MarkHandleInfo = { 0 };
    ULONG dwRet;

    if ( iCopyNumber < -1 )
    {
        Assert( fFalse );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( iCopyNumber == -1 )
    {
        MarkHandleInfo.HandleInfo = MARK_HANDLE_NOT_READ_COPY;
    }
    else
    {
        MarkHandleInfo.HandleInfo = MARK_HANDLE_READ_COPY;
#if (_WIN32_WINNT < 0x0602 )
        MarkHandleInfo.UsnSourceInfo = iCopyNumber; // Same as CopyNumber (union)
#else
        MarkHandleInfo.CopyNumber = iCopyNumber;
#endif
    }

    if ( !DeviceIoControl( hFile,
                           FSCTL_MARK_HANDLE,
                           &MarkHandleInfo,
                           sizeof(MarkHandleInfo),
                           NULL,
                           0,
                           &dwRet,
                           NULL ) )
    {
        return ErrOSFileIFromWinError( GetLastError() );
    }

    return JET_errSuccess;
}

IFileSystemConfiguration* const _OSFILE::Pfsconfig() const { return pfsconfig; }


LONG COSFile::CLogicalCopies()
{
    if ( m_cLogicalCopies != 0 )
    {
        return m_cLogicalCopies;
    }

    m_p_osf->m_posd->BeginExclusiveIo();

    // ioctl for StorageDeviceResiliencyProperty does not work over SMB, so figure out
    // number of logical copies by trying to set current copy until we fail
    LONG cCopies = 1;
    if ( m_p_osf->ErrSetReadCopyNumber( 0 ) >= JET_errSuccess )
    {
        for ( ; cCopies<osfileMaxReadCopy; cCopies++ )
        {
            if ( m_p_osf->ErrSetReadCopyNumber( cCopies ) < JET_errSuccess )
            {
                break;
            }
        }

        m_p_osf->ErrSetReadCopyNumber( -1 );
    }

    m_p_osf->m_posd->EndExclusiveIo();

    AtomicExchange( &m_cLogicalCopies, cCopies );
    return cCopies;
}

ERR COSFile::ErrIOIssue()
{
    // would go off if someone forgot to call ErrIOIssue() after enqueuing IOREQs ...
    // Update: Can't assert this because checkpoint pushes IO to multiple files, and you
    // don't know which file will get ErrIOIssue() called first.  Might be interesting
    // to make EnqueueDeferredIORun() OSFile specific, and only issue if the Postls()->IORun
    // matches the file we called ErrIOIssue() on. Just random thoughts.
    //Assert( Postls()->IORun.PioreqHead()->p_osf->m_posd == m_p_osf->m_posd );

    // drain the thread local I/O combining list into I/O heap

    m_p_osf->m_posd->EnqueueDeferredIORun( m_p_osf );

    // tell I/O thread to issue from I/O heap to FS

    OSDiskIOThreadStartIssue( m_p_osf );
    
    return JET_errSuccess;
}

ERR COSFile::ErrMMRead( const QWORD     ibOffset,
                        const QWORD     cbSize,
                        void** const    ppvMap )
{
    ERR         err         = JET_errSuccess;
    HANDLE      hFileMap    = NULL;
    void*       pvMap       = NULL;
    const INT   group       = m_msFileSize.Enter();

    //  validate parameters

    if ( size_t( cbSize ) != cbSize )
    {
        ExpectedSz( fFalse, "QWORD size truncated on 32-bit. %I64d != %I64d", cbSize, (QWORD)size_t( cbSize ) );
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( ibOffset >= m_rgcbFileSize[ group ] )
    {
        Call( ErrERRCheck( JET_errFileIOBeyondEOF ) );
    }
    if ( ibOffset + cbSize > m_rgcbFileSize[ group ] )
    {
        Call( ErrERRCheck( JET_errFileIOBeyondEOF ) );
    }

    //  RFS:  out of address space

    if ( !RFSAlloc( OSMemoryPageAddressSpace ) )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    //  defer create the file mapping

    if ( !m_rghFileMap[ group ] )
    {
        if ( !( hFileMap = CreateFileMappingW(  m_hFile,
                                                NULL,
                                                (   ( m_fmf & fmfReadOnly ) ?
                                                        ( PAGE_READONLY | SEC_COMMIT ) :
                                                        ( PAGE_READWRITE | SEC_COMMIT ) ),
                                                0,
                                                0,
                                                NULL ) ) )
        {
            Assert( ERROR_IO_PENDING != GetLastError() );   // not bad, just unexpected
            Call( ErrOSFileIGetLastError() );
        }
        if ( AtomicCompareExchangePointer( (void**)&m_rghFileMap[ group ], NULL, hFileMap ) == NULL )
        {
            hFileMap = NULL;
        }
    }

    //  create the requested view of the file
    //
    //  NOTE:  we will hack around the fact that you can only create views that
    //  start at offsets that are multiples of g_cbMMSize
    //
    //  To compensate somewhat for this inefficency, we'll only map the minimum.
    //
    //  +---+---+---+---+---+---+---+---+
    //  |   |   |   |   |   |   |   |   |
    //  +---+---+---+---+---+---+---+---+
    //  ^           ^
    //  |           |
    //  |           \-- ibOffset (some page in the middle).
    //  |
    //  \----- ibOffsetAlign, on an alignment boundary.
    //
    //  Results in:
    //  +---+---+---+---+---+---+---+---+
    //  | X | X | X |***| X | X | X | X |
    //  +---+---+---+---+---+---+---+---+
    //  ^           ^
    //  |           |
    //  |           \-- pvMapping
    //  |
    //  \-- pvMap, the value returned by the OS, on an alignment boundary. This
    //      is also the value that needs to be freed.
    //
    //  Where:
    //  'X' is mapped, but not referenced.
    //  *** is mapped, and the only page we care about. This is also
    //      the only page that ErrBFIValidatePageSlowly will touch,
    //  'o' is unmapped.
    //
    //  Layering violation notes (regarding ErrBFIValidatePageSlowly)
    //  'X' will be mapped read only, so will not affect private bytes or private working set.
    //  *** may be mapped copy-on-write (if ErrMMRead() is being called from within ErrMMCopy()), 
    //      so will affect private bytes on mapping and private working set when dirtied.
    //
    //  IMPORTANT: if another request comes in for *any* other page, then an
    //  entirely new 64k mapping will be made!

    const QWORD ibOffsetAlign   = ( ibOffset / g_cbMMSize ) * g_cbMMSize;
    const QWORD ibOffsetBias    = ibOffset - ibOffsetAlign;
    const QWORD cbViewSize      = min( m_rgcbFileSize[ group ] - ibOffsetAlign, ( ( ibOffsetBias + cbSize + g_cbMMSize - 1 ) / g_cbMMSize ) * g_cbMMSize );
    void* const pvHint          = (void*)( ( DWORD_PTR( *ppvMap ) / g_cbMMSize ) * g_cbMMSize );

    //  first map read-only

    if (    ( pvMap = (BYTE*)MapViewOfFileEx(   m_rghFileMap[ group ],
                                                FILE_MAP_READ,
                                                DWORD( ibOffsetAlign >> 32 ),
                                                DWORD( ibOffsetAlign ),
                                                size_t( cbViewSize ),
                                                pvHint ) + ibOffsetBias ) == (void*)DWORD_PTR( ibOffsetBias ) &&
            ( pvMap = (BYTE*)MapViewOfFileEx(   m_rghFileMap[ group ],
                                                FILE_MAP_READ,
                                                DWORD( ibOffsetAlign >> 32 ),
                                                DWORD( ibOffsetAlign ),
                                                size_t( cbViewSize ),
                                                NULL ) + ibOffsetBias ) == (void*)DWORD_PTR( ibOffsetBias ) )
    {
        Assert( ERROR_IO_PENDING != GetLastError() );   // not bad, just unexpected
        Assert( pvMap == NULL );
        Call( ErrOSFileIGetLastError() );
    }
    Assert( ( DWORD_PTR( pvMap ) / g_cbMMSize ) * g_cbMMSize == DWORD_PTR( pvMap ) - ibOffsetBias );

    //  RFS:  in-page error

    Assert( err == JET_errSuccess );    //  otherwise we can't signal ErrMMCopy() with the warning for IO RFS ...
    if ( !RFSAlloc( OSFileRead ) || ErrFaultInjection( 34060 ) )
    {
        DWORD flOldProtect;
        (void)VirtualProtect( pvMap, size_t( cbSize ), PAGE_NOACCESS, &flOldProtect );
        err = ErrERRCheck( wrnLossy );
    }

    //  return the mapping

    Assert( FOSMemoryFileMapped( pvMap, size_t( cbSize ) ) );
    Assert( !FOSMemoryFileMappedCowed( pvMap, size_t( cbSize ) ) );

    *ppvMap = pvMap;
    pvMap = NULL;

HandleError:

    if ( err == JET_errFileAccessDenied )
    {
        err = ErrERRCheck( JET_errFileIOBeyondEOF );
    }
    m_msFileSize.Leave( group );
    if ( hFileMap )
    {
        CloseHandle( hFileMap );
    }
    if ( pvMap )
    {
        (void)ErrMMFree( pvMap );
    }
    if ( err < JET_errSuccess )
    {
        *ppvMap = NULL;
    }
    return err;
}

ERR COSFile::ErrMMCopy( const QWORD     ibOffset,
                        const QWORD     cbSize,
                        void** const    ppvMap )
{
    ERR         err         = JET_errSuccess;

    Assert( QWORD( size_t( cbSize ) ) == cbSize );  //  for FOSMemoryFileMapped() below, but ErrMMRead() will error on this.
    const size_t cbSizeT = size_t( cbSize );

    //  first map read-only

    Call( ErrMMRead( ibOffset, cbSize, ppvMap ) );

    Assert( FOSMemoryFileMapped( *ppvMap, cbSizeT ) );
    Assert( !FOSMemoryFileMappedCowed( *ppvMap, cbSizeT ) );
    Assert( !FOSMemoryPageAllocated( *ppvMap, cbSizeT ) );

    //  ErrMMRead() signals a fault injection via returning this warning, if they did
    //  not (i.e. the success case) we can convert this page to _WRITECOPY so the page
    //  gets duplicated into our memory if it is modified.

    if ( err != wrnLossy )
    {
        // convert the page(s) we are interested in to copy-on-write

        DWORD flOldProtect;
        if ( !VirtualProtect( *ppvMap, cbSizeT, PAGE_WRITECOPY, &flOldProtect ) )
        {
            Call( ErrOSFileIGetLastError() );
        }
    }
    err = JET_errSuccess;

    Assert( FOSMemoryFileMapped( *ppvMap, cbSizeT ) );
    //  While we declared it _WRITECOPY, this won't return true UNTIL we've actually copied (well forced
    //  the copy by writing to it).
    Assert( !FOSMemoryFileMappedCowed( *ppvMap, cbSizeT ) );
    Assert( !FOSMemoryPageAllocated( *ppvMap, cbSizeT ) );

HandleError:

    Expected( err != JET_errFileAccessDenied ); //  before ErrMMRead() merge, this err is converted to JET_errFileIOBeyondEOF.

    if ( err < JET_errSuccess )
    {
        Expected( *ppvMap == NULL );
        *ppvMap = NULL;     //  just in case, no harm to an error path
    }

    return err;
}


ERR COSFile::ErrMMIORead(
    _In_                    const QWORD             ibOffset,   //  note: Only needed for debugging
    _Out_writes_bytes_(cb)  BYTE * const            pb,
    _In_                    ULONG                   cb,
    _In_                    const FileMmIoReadFlag  fmmiorf )
{
    Assert( FOSMemoryFileMapped( pb, cb ) );
    Assert( !FOSMemoryPageAllocated( pb, cb ) );
    Expected( ( fmmiorf & fmmiorfKeepCleanMapped ) || ( fmmiorf & fmmiorfCopyWritePage ) );
    Expected( ibOffset != 0 );  //  Zero is a valid file offset!  But ESE probably will never use it (b/c we don't map our headers ;), and the unit tests can just avoid it.

    ERR err = JET_errSuccess;

    const BOOL fFixed = FDiskFixed();

    volatile LONG forceread;

    TRY
    {
        const size_t cbChunk = min( (size_t)cb, OSMemoryPageCommitGranularity() );
        for ( size_t ib = 0; ib < (size_t)cb; ib += cbChunk )
        {
            volatile LONG * const plAddress = &(((volatile LONG*)(pb + ib))[0]);

            if ( fmmiorf & fmmiorfCopyWritePage || !fFixed )
            {
                AtomicExchangeAdd( (LONG * const)plAddress, 0 );
            }
            else
            {
                forceread = *plAddress;
            }
        }
    }
    EXCEPT( efaExecuteHandler )
    {
        err = ErrERRCheck( JET_errDiskIO );
    }

    if ( err < JET_errSuccess )
    {
#if defined( USE_HAPUBLISH_API )
        //  indicate a hard I/O error
        //
        m_p_osf->Pfsconfig()->EmitFailureTag( HaDbFailureTagIoHard, L"7123782f-848a-4d19-abab-f7f6533491df" );
#endif
    }

    return err;
}

ERR COSFile::ErrMMRevert(
    _In_                        const QWORD             ibOffset,
    _In_reads_bytes_(cbSize)    void* const             pvMap,
    _In_                        const QWORD             cbSize )
{
    ERR err = JET_errSuccess;

    Assert( !FOSMemoryPageAllocated( pvMap, size_t( cbSize ) ) );

    const INT   group       = m_msFileSize.Enter();

    //  validate parameters

    if ( size_t( cbSize ) != cbSize )
    {
        ExpectedSz( fFalse, "QWORD size truncated on 32-bit. %I64d != %I64d", cbSize, (QWORD)size_t( cbSize ) );
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }
    //  why even bother with the group stuff?  Well I don't know what will happen if we are past
    //  EOF, so lets defend like the original mapping code to be safe.
    if ( ibOffset >= m_rgcbFileSize[ group ] )
    {
        AssertSz( fFalse, "Should not be able to Reset a page we haven't already mapped in." );
        Call( ErrERRCheck( JET_errFileIOBeyondEOF ) );
    }
    if ( ibOffset + cbSize > m_rgcbFileSize[ group ] )
    {
        AssertSz( fFalse, "Should not be able to Reset a page + cbSize we haven't already mapped in." );
        Call( ErrERRCheck( JET_errFileIOBeyondEOF ) );
    }

    if ( !FDiskFixed() )
    {
        //  We do not allow full / proper clean viewcache mode on non-local (really non-system
        //  in initial incarnation) files / drives.
        Error( ErrERRCheck( JET_errFeatureNotAvailable ) );
    }

    //  convert the page(s) we are interested BACK in to a mapped COWable page

    DWORD flOldProtect;
    if ( !VirtualProtect( pvMap, size_t( cbSize ), PAGE_REVERT_TO_FILE_MAP | PAGE_WRITECOPY, &flOldProtect ) )
    {
        const DWORD error = GetLastError();
        err = ErrOSFileIGetLastError();
        //  On Win7 - this PAGE_REVERT_TO_FILE_MAP feature is not present and the OS fails out
        //  with ERROR_INVALID_PARAMETER ... we expect this, and pass the error up (so caller
        //  knows the remap failed).
        AssertSz( error == ERROR_INVALID_PARAMETER, "This is promised not to fail due to OOM via our favorite OS MM dev, so what else could be wrong?!?  error: %u (0x%x) / %d", error, error, err );
        Call( err );
    }
    Assert( FOSMemoryFileMapped( pvMap, size_t( cbSize ) ) );
    Assert( !FOSMemoryFileMappedCowed( pvMap, size_t( cbSize ) ) );

HandleError:

    m_msFileSize.Leave( group );
    return err;
}

ERR COSFile::ErrMMFree( void* const pvMap )
{
    ERR err = JET_errSuccess;

    if ( pvMap )
    {
        if ( !UnmapViewOfFile( (void*)( ( DWORD_PTR( pvMap ) / g_cbMMSize ) * g_cbMMSize ) ) )
        {
            Assert( ERROR_IO_PENDING != GetLastError() );   // not bad, just unexpected
            Call( ErrOSFileIGetLastError() );
        }
    }

HandleError:
    return err;
}

void COSFile::IOComplete_(  IOREQ* const    pioreq,
                            const ERR       err,
                            COSFile* const  posf )
{
    posf->IOComplete( pioreq, err );
}

void COSFile::IOComplete( IOREQ* const pioreq, const ERR err )
{
    //  if this is a normal write then release our lock on the file size

    if (    pioreq->fWrite &&
            pioreq->pfnCompletion != PFN( IOZeroingWriteComplete_ ) &&
            pioreq->pfnCompletion != PFN( IOExtendingWriteComplete_ ) )
    {
        m_msFileSize.Leave( pioreq->group );
    }

    if ( pioreq->fWrite )
    {
        m_cioUnflushed++;
    }
    
    TraceStationId( tsidrPulseInfo );

    //  perform I/O completion callback

    const PfnIOComplete pfnIOComplete   = PfnIOComplete( pioreq->pfnCompletion );

    pfnIOComplete(  err,
                    this,
                    pioreq->m_tc,
                    pioreq->grbitQOS,
                    pioreq->ibOffset,
                    pioreq->cbData,
                    pioreq->pbData,
                    pioreq->dwCompletionKey );
}

void COSFile::IOSyncComplete_(  const ERR           err,
                                COSFile* const      posf,
                                const FullTraceContext& tc,
                                const OSFILEQOS     grbitQOS,
                                const QWORD         ibOffset,
                                const DWORD         cbData,
                                BYTE* const         pbData,
                                CIOComplete* const  piocomplete )
{
    posf->IOSyncComplete(   err,
                            tc,
                            grbitQOS,
                            ibOffset,
                            cbData,
                            pbData,
                            piocomplete );
}

void COSFile::IOSyncHandoff_(   const ERR           err,
                                COSFile* const      posf,
                                const FullTraceContext& tc,
                                const OSFILEQOS     grbitQOS,
                                const QWORD         ibOffset,
                                const DWORD         cbData,
                                const BYTE* const   pbData,
                                CIOComplete* const  piocomplete,
                                void* const         pioreq )
{
    Assert( ( piocomplete != NULL ) && ( piocomplete->m_pfnIOHandoff != NULL ) );
    piocomplete->m_pfnIOHandoff(    err,
                                    posf,
                                    tc,
                                    grbitQOS,
                                    ibOffset,
                                    cbData,
                                    pbData,
                                    piocomplete->m_keyIOComplete,
                                    pioreq );
}

void COSFile::IOSyncComplete(   const ERR           err,
                                const FullTraceContext& tc,
                                const OSFILEQOS     grbitQOS,
                                const QWORD         ibOffset,
                                const DWORD         cbData,
                                BYTE* const         pbData,
                                CIOComplete* const  piocomplete )
{
    piocomplete->Complete( err );
}

BOOL COSFile::FOSFileISyncIOREQ( const IOREQ * const pioreq )
{
    return pioreq->pfnCompletion == PFN( IOSyncComplete_ );
}

BOOL COSFile::FOSFileIZeroingSyncIOREQ( const IOREQ * const pioreq )
{
    return (    pioreq->pfnCompletion == PFN( IOZeroingWriteComplete_ ) &&
                ((CExtendingWriteRequest*)(pioreq->dwCompletionKey))->m_pfnIOComplete == PfnIOComplete( IOSyncComplete_ ) &&
                ( pioreq->IbBlockMax() ) == ((CExtendingWriteRequest*)(pioreq->dwCompletionKey))->m_ibOffset );
}

BOOL COSFile::FOSFileIExtendingSyncIOREQ( const IOREQ * const pioreq )
{
    return (    pioreq->pfnCompletion == PFN( IOExtendingWriteComplete_ ) &&
                ((CExtendingWriteRequest*)(pioreq->dwCompletionKey))->m_pfnIOComplete == PfnIOComplete( IOSyncComplete_ ) );
}

BOOL COSFile::FOSFileSyncComplete( const IOREQ * const pioreq )
{
    return ( qosIODispatchImmediate == ( pioreq->grbitQOS & qosIODispatchMask ) ) &&
           ( FOSFileISyncIOREQ( pioreq ) ||
             FOSFileIZeroingSyncIOREQ( pioreq ) ||
             FOSFileIExtendingSyncIOREQ( pioreq ) );
}



INLINE void OSDiskIIOREQFree( IOREQ* pioreq );
void IOMgrCompleteOp( __inout IOREQ * const pioreq );
VOID IOMgrIssueSyncIO( IOREQ * pioreqSingle );

ERR COSFile::ErrIOAsync(    IOREQ* const        pioreq,
                            const BOOL          fWrite,
                            const OSFILEQOS     grbitQOS,
                            const INT           group,
                            const QWORD         ibOffset,
                            const DWORD         cbData,
                            BYTE* const         pbData,
                            const PfnIOComplete pfnIOComplete,
                            const DWORD_PTR     keyIOComplete,
                            const PfnIOHandoff  pfnIOHandoff )
{
    ERR err = JET_errSuccess;

    //  setup the I/O request

    pioreq->p_osf           = m_p_osf;
    pioreq->fWrite          = fWrite;
    pioreq->grbitQOS        = grbitQOS;
    pioreq->group           = group;
    pioreq->fCoalesced      = fFalse;
    pioreq->ibOffset        = ibOffset;
    pioreq->cbData          = cbData;
    pioreq->pbData          = pbData;
    pioreq->dwCompletionKey = keyIOComplete;
    pioreq->pfnCompletion   = PFN( pfnIOComplete );

    pioreq->ovlp.Offset     = (ULONG) ( pioreq->ibOffset );
    pioreq->ovlp.OffsetHigh = (ULONG) ( pioreq->ibOffset >> 32 );
    pioreq->pioreqIorunNext = NULL;
    pioreq->pioreqVipList   = NULL;

    // if the trace context hasn't been captured already, then capture it
    // ErrIOAsync() can be called for IO that was deferred (e.g. by an extending write completion)
    // In that case, tc was already captured when the io was originally issued.
    if ( pioreq->m_tc.etc.FEmpty() )
    {
        GetCurrUserTraceContext getutc;
        const TraceContext* petc = PetcTLSGetEngineContext();
        Assert( petc->iorReason.FValid() );
        Assert( petc->iorReason.Iorp() != iorpNone );   // atleast iorp should be set
        pioreq->m_tc.DeepCopy( getutc.Utc(), *petc );
    }

    pioreq->hrtIOStart = HrtHRTCount();

    //  this is an extending write

    if (    pioreq->fWrite &&
            pioreq->ibOffset + pioreq->cbData > m_rgcbFileSize[ pioreq->group ] )
    {
        //  We can't be sure we will be able to combine this on the back side of the extending
        //  write completion, so we can't claim it.  We will get a second chance to combine it
        //  as we do ErrIOAsync() calls on the back side (i.e. IO Thread completion).
        pioreq->m_fCanCombineIO = fFalse;

        if ( !pioreq->m_fHasHeapReservation )
        {
            //  Probably we thought this IO was combinable, but since it is a file extension it
            //  is not, so reserve the queue space so this can be executed / enqueued properly
            //  from within IOChangeFileSizeComplete.

            Call( m_p_osf->m_posd->ErrReserveQueueSpace( grbitQOS, pioreq ) );

            Assert( pioreq->m_fHasHeapReservation || ( pioreq->grbitQOS & qosIOPoolReserved ) );
            Assert( pioreq->FEnqueueable() );   // same thing
        }

        //  We are accepting this IO, signal handoff

        if ( pfnIOHandoff )
        {
            Assert( ( (void*)IOSyncComplete_ != (void*)pfnIOComplete ) || ( (void*)IOSyncHandoff_ == (void*)pfnIOHandoff ) );   // sync IOs can only use the sync-thunk handoff.
            pfnIOHandoff( JET_errSuccess, this, pioreq->m_tc, grbitQOS, ibOffset, cbData, pbData, keyIOComplete, pioreq );
        }

        //  try to allocate an extending write request

        CExtendingWriteRequest* const pewreq = new CExtendingWriteRequest();

        pioreq->SetIOREQType( IOREQ::ioreqExtendingWriteIssued );

        //  we failed to allocate an extending write request

        if ( !pewreq )
        {
            //  fail the I/O with out of memory

            OSDiskIIOThreadCompleteWithErr( ERROR_NOT_ENOUGH_MEMORY,
                                            pioreq->cbData,
                                            pioreq );

            //  we're done here

            Assert( err == JET_errSuccess );    // error handled with OSDiskIIOThreadCompleteWithErr
            goto HandleError;
        }

        //  we got an extending write request

        //  save the parameters for this write

        pewreq->m_posf              = this;
        pewreq->m_pioreq            = pioreq;
        pewreq->m_group             = group;
        pewreq->m_tc.DeepCopy( pioreq->m_tc.utc, pioreq->m_tc.etc );
        pewreq->m_ibOffset          = ibOffset;     // this is the final size requested.
        pewreq->m_cbData            = cbData;
        pewreq->m_pbData            = pbData;
        pewreq->m_grbitQOS          = grbitQOS;
        pewreq->m_pfnIOComplete     = pfnIOComplete;
        pewreq->m_keyIOComplete     = keyIOComplete;
        pewreq->m_tickReqStart      = TickOSTimeCurrent();
        pewreq->m_tickReqStep       = 0;
        pewreq->m_tickReqComplete   = 0;

        //  we can initiate a change in the file size

        if ( m_semChangeFileSize.FTryAcquire() )
        {
            //  start file extension by completing a fake extension I/O up to
            //  the current file size

            pioreq->ibOffset        = m_rgcbFileSize[ pioreq->group ];
            pioreq->cbData          = 0;
            pioreq->pbData          = NULL;
            pioreq->dwCompletionKey = DWORD_PTR( pewreq );
            pioreq->pfnCompletion   = PFN( IOZeroingWriteComplete_ );

            pioreq->ovlp.Offset     = (ULONG) ( pioreq->ibOffset );
            pioreq->ovlp.OffsetHigh = (ULONG) ( pioreq->ibOffset >> 32 );

            m_msFileSize.Leave( pioreq->group );

            OSDiskIIOThreadCompleteWithErr( ERROR_SUCCESS,
                                            pioreq->cbData,
                                            pioreq );
        }

        //  we cannot initiate a change in the file size

        else
        {
            //  save our group number before we defer this write so that we
            //  can leave the proper group number after posting it to the
            //  deferred extending write queue

            const INT groupT = pioreq->group;

            //  defer this extending write
            m_critDefer.Enter();
            m_ilDefer.InsertAsNextMost( pewreq );
            m_critDefer.Leave();

            // leave metered section after appending to list to ensure
            // that IOChangeFileSizeComplete() will see the deferral.
            m_msFileSize.Leave( groupT );
        }

    }

    //  SPECIAL CASE:  this is a sync I/O and we can directly issue it on this
    //  thread
    //  SPECIAL CASE:  this is a single sync zeroing write and we can directly
    //  issue it on this thread
    //  SPECIAL CASE:  this is a sync extending write and we can directly issue
    //  it on this thread

    else if (   COSFile::FOSFileSyncComplete( pioreq ) &&
                !Postls()->fIOThread )
    {

        //  We are accepting this IO, signal handoff

        if ( pfnIOHandoff )
        {
            Assert( ( (void*)IOSyncComplete_ != (void*)pfnIOComplete ) || ( (void*)IOSyncHandoff_ == (void*)pfnIOHandoff ) );   // sync IOs can only use the sync-thunk handoff.
            pfnIOHandoff( JET_errSuccess, this, pioreq->m_tc, grbitQOS, ibOffset, cbData, pbData, keyIOComplete, pioreq );
        }

        //  if this is a zero sized I/O then complete it immediately w/o calling the
        //  OS to avoid the overhead and ruining our I/O stats from the OS perspective

        if ( pioreq->cbData == 0 )
        {
            //  Issue the Special Op ...

            pioreq->m_posdCurrentIO = pioreq->p_osf->m_posd;    // faking this to make valid IOREQ state.
            pioreq->SetIOREQType( IOREQ::ioreqIssuedSyncIO );
            IOMgrCompleteOp( pioreq );
        }
        
        //
        //  Issue the actual I/O
        //
        
        else
        {
            IOMgrIssueSyncIO( pioreq );
            // note: a difference with this method than the explicit code we used to have here, is we'll 
            // be passing the response to GetOverlappedResult_() through the below logic for JET_errOutOfMemory, 
            // so we'll re-issue it in that case.
        }
    }

    //  this is not an extending write and is not a special case sync I/O

    else
    {
        //  We are accepting this IO, signal handoff

        if ( pfnIOHandoff )
        {
            Assert( ( (void*)IOSyncComplete_ != (void*)pfnIOComplete ) || ( (void*)IOSyncHandoff_ == (void*)pfnIOHandoff ) );   // sync IOs can only use the sync-thunk handoff.
            pfnIOHandoff( JET_errSuccess, this, pioreq->m_tc, grbitQOS, ibOffset, cbData, pbData, keyIOComplete, pioreq );
        }

        //  Enqueue IOREQ (may enqueue only in TLS, or may insert in IO heap/queue)

        m_p_osf->m_posd->EnqueueIOREQ( pioreq );
    }

HandleError:

    Assert( err == JET_errSuccess ||
            err == errDiskTilt );

    if ( err == errDiskTilt )
    {
        //  This IO operation has been rejected due to not enough quota ...

        Assert( qosIODispatchImmediate != ( grbitQOS & qosIODispatchMask ) );
        Assert( qosIOPoolReserved != ( grbitQOS & qosIODispatchMask ) );

        //  Release the resources we've gotten

        m_msFileSize.Leave( pioreq->group );

        pioreq->m_fCanCombineIO = fFalse;
        pioreq->m_tc.Clear();

        //  The callback will not be called, we must free the pioreq
        OSDiskIIOREQFree( pioreq );
    }

    return err;
}

void COSFile::IOZeroingWriteComplete_(  const ERR                       err,
                                        COSFile* const                  posf,
                                        const FullTraceContext&         tc,
                                        const OSFILEQOS                 grbitQOS,
                                        const QWORD                     ibOffset,
                                        const DWORD                     cbData,
                                        BYTE* const                     pbData,
                                        CExtendingWriteRequest* const   pewreq )
{
    posf->IOZeroingWriteComplete(   err,
                                    ibOffset,
                                    cbData,
                                    pbData,
                                    pewreq );
}

void COSFile::IOZeroingWriteComplete(   const ERR                       err,
                                        const QWORD                     ibOffset,
                                        const DWORD                     cbData,
                                        BYTE* const                     pbData,
                                        CExtendingWriteRequest* const   pewreq )
{
    //  save the current error and timestamp

    pewreq->m_err = err;
    pewreq->m_tickReqStep = TickOSTimeCurrent();
    Assert( ( pewreq->m_tickReqStep - pewreq->m_tickReqStart ) < ( ( 10 * (TICK)cmsecDeadlock ) ) );
    Assert( pewreq->m_tickReqComplete == 0 );

    //  get our zeroing buffer

    DWORD cbZero = (DWORD)g_cbZero;
    const BYTE* const rgbZero = g_rgbZero;

    //  start file extension process of zeroing from current filesystem EOF
    //  (using as many zeroing I/Os as we need) up to any extending write
    //  that we need to complete. if we're going to be using more than 1 I/O
    //  to extend the file to its new size, we should call SetEndOfFile() ahead
    //  of time to give the filesystem more information up front. (and we
    //  don't want to always call SetEndOfFile() [even though it would be
    //  "correct"] because NTFS counterintuitively behaves worse then.)

    if ( pewreq->m_err >= JET_errSuccess &&
        //  start of extension process (also case of file size staying the same)
        ibOffset + cbData == m_rgcbFileSize[ pewreq->m_group ] &&
        //  if extending write is past EOF, this means that at least 1
        //  zeroing I/O will need to be done
        pewreq->m_ibOffset > m_rgcbFileSize[ pewreq->m_group ] &&
        //  if extending write has actual data to be written, that is an additional I/O,
        //  or if there will need to be more than 1 zeroing I/O.
        ( pewreq->m_cbData > 0 || pewreq->m_ibOffset - m_rgcbFileSize[ pewreq->m_group ] > cbZero ) &&
        //  if we are not mapping the file (SetEndOfFile() is not allowed in this case)
        m_rghFileMap[ pewreq->m_group ] == NULL )
    {
        //  give the filesystem early notification of how much storage we require
        //  for this entire multi-I/O extension process

        const QWORD cbSize      = pewreq->m_ibOffset + pewreq->m_cbData;

        const ERR errSetFileSize = ErrIOSetFileSize( cbSize, fTrue /* fReportError */ );

        pewreq->m_err = (   pewreq->m_err < JET_errSuccess ?
                            pewreq->m_err :
                            errSetFileSize );
    }

    //  this zeroing write succeeded

    if ( pewreq->m_err >= JET_errSuccess )
    {
        //  there is still more file to be zeroed between the original file size
        //  and the extending write

        //  retrieve the reason and QOS for this IO from the completed IOREQ ...

        //  ok, this is really vugly ... this m_pioreq is not actually valid right now, of course
        //  it exists because we never deallocate these things ... but we've free'd it, and it 
        //  could already be reused for other purposes.  The IOREQ that is actually completing 
        //  this IOZeroingWriteComplete_ is a stack based IOREQ allocated in OSDiskIIOThreadCompleteWithErr()
        //  with this line:
        //        IOREQ ioreq( pioreq );
        Assert( pewreq->m_pioreq );

        Assert( pewreq->m_tc.etc.iorReason.FValid() );

        OSFILEQOS qosT = pewreq->m_grbitQOS;
        //  We are going to simplify our lives here, and just insist this is qosIODispatchImmediate,
        //  thus guaranteeing we go to the old behavior of looking for a regular IOREQ and
        //  hanging/waiting until one shows up...
        qosT = ( qosT & ~qosIODispatchMask ) | qosIODispatchImmediate;
        
        const QWORD ibWriteRemaining = ibOffset + cbData;
        if ( ibWriteRemaining < pewreq->m_ibOffset )
        {
            //  compute the offset of the current chunk to be zeroed
            //
            //  NOTE:  if the zeroing buffer is very small, just let the OS
            //  do most of the work by writing a single chunk to the very end
            //  of the region we wish to extend

            const QWORD ibWrite = ( ( ( cbZero < 64 * 1024 ) && ( ( ibWriteRemaining + cbZero ) <= pewreq->m_ibOffset )  ) ?
                                        pewreq->m_ibOffset - cbZero :
                                        ibWriteRemaining );


            //  We used to chunk align our zeroing writes (for unknown
            //  reasons) by using "cbZero - ibWrite % cbZero" instead of
            //  "cbZero" in the min() below. Chunk aligning broke NTFS's
            //  secret automagic pre-allocation algorithms which increased
            //  file fragmentation.

            const QWORD cbWrite = min(  cbZero,
                                        pewreq->m_ibOffset - ibWrite );
            Assert( DWORD( cbWrite ) == cbWrite );

            //  set the new file size to the file size after extension

            m_rgcbFileSize[ 1 - pewreq->m_group ] = ibWrite + cbWrite;

            //  zero the next aligned chunk of the file

            const P_OSFILE p_osf = pewreq->m_posf->m_p_osf;
            Assert( p_osf == m_p_osf ); // is this ever not true?

            IOREQ* pioreq = NULL;
            Assert( qosIODispatchImmediate == ( qosIODispatchMask & qosT ) );

            CallS( p_osf->m_posd->ErrAllocIOREQ( qosT, NULL, fTrue, 0, 0, &pioreq ) );
            Assert( pioreq->FEnqueueable() || Postls()->pioreqCache == NULL );

            // stamp the tc coming in from the the thread that started the zeroing write
            Assert( pewreq->m_tc.etc.iorReason.Iorp() != iorpNone );
            pioreq->m_tc.DeepCopy( pewreq->m_tc.utc, pewreq->m_tc.etc );

            CallS( ErrIOAsync(  pioreq,
                                fTrue,
                                qosT,
                                1 - pewreq->m_group,
                                ibWrite,
                                DWORD( cbWrite ),
                                (BYTE* const)rgbZero,
                                PfnIOComplete( IOZeroingWriteComplete_ ),
                                DWORD_PTR( pewreq ) ) );

            CallS( ErrIOIssue() );
        }

        //  there is no more file to be zeroed

        else
        {
            //  set the new file size to the file size after extension

            m_rgcbFileSize[ 1 - pewreq->m_group ] = pewreq->m_ibOffset + pewreq->m_cbData;

            //  perform the original extending write

            const P_OSFILE p_osf = pewreq->m_posf->m_p_osf;
            Assert( p_osf == m_p_osf ); // is this ever not true?

            IOREQ* pioreq = NULL;
            Assert( qosIODispatchImmediate == ( qosIODispatchMask & qosT ) );
            CallS( p_osf->m_posd->ErrAllocIOREQ( qosT, NULL, fTrue, 0, 0, &pioreq ) );
            Assert( pioreq->FEnqueueable() || Postls()->pioreqCache == NULL );

            // stamp the tc coming in from the the thread that started the zeroing write
            Assert( pewreq->m_tc.etc.iorReason.Iorp() != iorpNone );
            pioreq->m_tc.DeepCopy( pewreq->m_tc.utc, pewreq->m_tc.etc );

            CallS( ErrIOAsync( pioreq,
                                fTrue,
                                qosT,
                                1 - pewreq->m_group,
                                pewreq->m_ibOffset,
                                pewreq->m_cbData,
                                pewreq->m_pbData,
                                PfnIOComplete( IOExtendingWriteComplete_ ),
                                DWORD_PTR( pewreq ) ) );

            CallS( ErrIOIssue() );
        }
    }

    //  this zeroing write failed

    else
    {
        pewreq->m_tickReqStep = TickOSTimeCurrent();
        Assert( ( pewreq->m_tickReqStep - pewreq->m_tickReqStart ) < ( 10 * (TICK)cmsecDeadlock ) );
        Assert( pewreq->m_tickReqComplete == 0 );
        pewreq->m_tickReqComplete = pewreq->m_tickReqStep;
        
        //  set the file size back to the original file size

        m_rgcbFileSize[ 1 - pewreq->m_group ] = m_rgcbFileSize[ pewreq->m_group ];

        //  change over to the new file size

        m_msFileSize.Partition( CMeteredSection::PFNPARTITIONCOMPLETE( IOChangeFileSizeComplete_ ),
                                DWORD_PTR( pewreq ) );
    }
}

void COSFile::IOExtendingWriteComplete_(    const ERR                       err,
                                            COSFile* const                  posf,
                                            const FullTraceContext&         tc,
                                            const OSFILEQOS                 grbitQOS,
                                            const QWORD                     ibOffset,
                                            const DWORD                     cbData,
                                            BYTE* const                     pbData,
                                            CExtendingWriteRequest* const   pewreq )
{
    posf->IOExtendingWriteComplete( err,
                                    ibOffset,
                                    cbData,
                                    pbData,
                                    pewreq );
}

void COSFile::IOExtendingWriteComplete( const ERR                       err,
                                        const QWORD                     ibOffset,
                                        const DWORD                     cbData,
                                        BYTE* const                     pbData,
                                        CExtendingWriteRequest* const   pewreq )
{
    //  save the current error and timestamp

    pewreq->m_err = err;
    Assert( pewreq->m_tickReqComplete == 0 );
    pewreq->m_tickReqComplete = TickOSTimeCurrent();
    Assert( ( pewreq->m_tickReqStep - pewreq->m_tickReqStart ) < ( 10 * (TICK)cmsecDeadlock ) );
    Assert( ( pewreq->m_tickReqComplete - pewreq->m_tickReqStep ) < ( 10 * (TICK)cmsecDeadlock ) );

    //  this extending write succeeded

    if ( err >= JET_errSuccess )
    {
        //  change over to the new file size

        m_msFileSize.Partition( CMeteredSection::PFNPARTITIONCOMPLETE( IOChangeFileSizeComplete_ ),
                                DWORD_PTR( pewreq ) );
    }

    //  this extending write failed

    else
    {
        //  set the file size back to the original file size

        m_rgcbFileSize[ 1 - pewreq->m_group ] = m_rgcbFileSize[ pewreq->m_group ];

        //  change over to the new file size

        m_msFileSize.Partition( CMeteredSection::PFNPARTITIONCOMPLETE( IOChangeFileSizeComplete_ ),
                                DWORD_PTR( pewreq ) );
    }
}

void COSFile::IOChangeFileSizeComplete_( CExtendingWriteRequest* const pewreq )
{
    pewreq->m_posf->IOChangeFileSizeComplete( pewreq );
}

void COSFile::IOChangeFileSizeComplete( CExtendingWriteRequest* const pewreq )
{
    const QWORD cbSize      = m_rgcbFileSize[ 1 - pewreq->m_group ];

    //  close the file mapping for the obsolete file size, if any

    if ( m_rghFileMap[ pewreq->m_group ] )
    {
        CloseHandle( m_rghFileMap[ pewreq->m_group ] );
        m_rghFileMap[ pewreq->m_group ] = NULL;
    }

    //  Shrinking file (user requested, or because we enlarged it, but
    //  subsequently encountered an I/O error and now want to shrink
    //  it back), so we need to explicitly set the file size.
    //
    //  NOTE:  this will fail if the file has any active file mappings or views

    if ( cbSize < m_rgcbFileSize[ pewreq->m_group ] ||
        pewreq->m_err < JET_errSuccess )
    {
        //  set the end of file pointer to the new file size

        const ERR errSetFileSize = ErrIOSetFileSize( cbSize, fTrue /* fReportError */ );

        //  BUGBUG:  if this fails it is too late because the file state
        //  has already changed!  should we just ignore the error?
        pewreq->m_err = (   pewreq->m_err < JET_errSuccess ?
                            pewreq->m_err :
                            errSetFileSize );
    }

    //  When enlarging the file, we wrote into the newly allocated portion
    //  of the file so we should already be set

    else
    {
#ifdef DEBUG
        BY_HANDLE_FILE_INFORMATION  bhfi;

        const DWORD cbSizeLow   = DWORD( cbSize );
        const DWORD cbSizeHigh  = DWORD( cbSize >> 32 );

        if ( GetFileInformationByHandle( m_hFile, &bhfi ) )
        {
            Assert( cbSizeLow == bhfi.nFileSizeLow );
            Assert( cbSizeHigh == bhfi.nFileSizeHigh );
        }
#endif
    }

    //  we have completed changing the file size so allow others to change it

    m_semChangeFileSize.Release();

    //  grab the list of deferred extending writes.  we must do this before we
    //  complete this extending write because the completion of the write might
    //  delete this file object if the list is empty!

    m_critDefer.Enter();
    CDeferList ilDefer = m_ilDefer;
    m_ilDefer.Empty();
    m_critDefer.Leave();

    //  retrieve the reason and QOS for this IO from the completed IOREQ ...

    //  How do we know this IOREQ is valid??? ... not 100% sure, see the
    //  IOZeroingWriteComplete_ issue above ...
    Assert( pewreq->m_pioreq );

    Assert( pewreq->m_tc.etc.iorReason.FValid() );

    //  fire the completion for the extending write

    pewreq->m_pfnIOComplete(    pewreq->m_err,
                                this,
                                pewreq->m_tc,
                                pewreq->m_grbitQOS,
                                pewreq->m_ibOffset,
                                pewreq->m_cbData,
                                pewreq->m_pbData,
                                pewreq->m_keyIOComplete );

    delete pewreq;

    //  reissue all deferred extending writes
    //
    //  NOTE:  we start with the deferred extending write with the highest
    //  offset to minimize the number of times we extend the file.  this little
    //  trick makes a HUGE difference when appending to the file

    P_OSFILE p_osf = NULL;

    CExtendingWriteRequest* pewreqT;
    CExtendingWriteRequest* pewreqEOF;
    for ( pewreqT = pewreqEOF = ilDefer.PrevMost(); pewreqT; pewreqT = ilDefer.Next( pewreqT ) )
    {
        Expected( pewreqEOF->m_pioreq->FExtendingWriteIssued() );
        if ( pewreqT->m_ibOffset > pewreqEOF->m_ibOffset )
        {
            pewreqEOF = pewreqT;
        }
    }

    if ( pewreqEOF )
    {
        ilDefer.Remove( pewreqEOF );

        p_osf = pewreqEOF->m_posf->m_p_osf;
        Assert( this == pewreqEOF->m_posf ); // the above line seems silly if this holds?

        //  Repurpose the lookaside IOREQ to be issued.

        pewreqEOF->m_pioreq->SetIOREQType( IOREQ::ioreqAllocFromEwreqLookaside );

        Assert( pewreqEOF->m_pioreq->FEnqueueable() || Postls()->pioreqCache == NULL );
        Expected( pewreqEOF->m_pioreq->FEnqueueable() || ( Postls()->pioreqCache == NULL && FIOThread() ) );    //  seeing if this holds
        Expected( pewreqEOF->m_pioreq->FEnqueueable() );    //  seeing if this holds
        Assert( pewreqEOF->m_pioreq->m_tc.etc.iorReason.Iorp() != iorpNone );

        CallS( ErrIOAsync(  pewreqEOF->m_pioreq,
                            fTrue,
                            pewreqEOF->m_pioreq->grbitQOS,
                            m_msFileSize.Enter(),
                            pewreqEOF->m_ibOffset,
                            pewreqEOF->m_cbData,
                            pewreqEOF->m_pbData,
                            pewreqEOF->m_pfnIOComplete,
                            pewreqEOF->m_keyIOComplete ) );

        delete pewreqEOF;
        pewreqEOF = NULL;
    }

    while ( ilDefer.PrevMost() )
    {
        CExtendingWriteRequest* const pewreqDefer = ilDefer.PrevMost();
        ilDefer.Remove( pewreqDefer );

        p_osf = pewreqDefer->m_posf->m_p_osf;
        Assert( this == pewreqDefer->m_posf ); // the above line seems silly if this holds?

        //  Repurpose the lookaside IOREQ to be issued.

        pewreqDefer->m_pioreq->SetIOREQType( IOREQ::ioreqAllocFromEwreqLookaside );

        Expected( pewreqDefer->m_pioreq->FEnqueueable() );
        Assert( pewreqDefer->m_pioreq->FEnqueueable() || Postls()->pioreqCache == NULL );
        Expected( pewreqDefer->m_pioreq->FEnqueueable() || ( Postls()->pioreqCache == NULL && FIOThread() ) );
        Assert( pewreqDefer->m_pioreq->m_tc.etc.iorReason.Iorp() != iorpNone );

        CallS( ErrIOAsync(  pewreqDefer->m_pioreq,
                            fTrue,
                            pewreqDefer->m_pioreq->grbitQOS,
                            m_msFileSize.Enter(),
                            pewreqDefer->m_ibOffset,
                            pewreqDefer->m_cbData,
                            pewreqDefer->m_pbData,
                            pewreqDefer->m_pfnIOComplete,
                            pewreqDefer->m_keyIOComplete ) );

        delete pewreqDefer;
    }

    if ( p_osf )
    {
        CallS( ErrIOIssue() );
    }
}


ERR COSFile::ErrNTFSAttributeListSize( QWORD* const pcbSize )
{

    ERR err = JET_errSuccess;
    WCHAR wszAttrList[ OSFSAPI_MAX_PATH + 100 ];
    HANDLE hFile = INVALID_HANDLE_VALUE;
    LARGE_INTEGER li;
    li.QuadPart = -1;

    // Zero output
    *pcbSize = 0;

    // Get the path
    CallS( ErrPath( wszAttrList ) );
    OSStrCbAppendW( wszAttrList, sizeof(wszAttrList), L"::$ATTRIBUTE_LIST" );

    // Open the file
    hFile = CreateFileW( wszAttrList, FILE_READ_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    if ( hFile == INVALID_HANDLE_VALUE )
    {
        err = ErrERRCheck( JET_errFileNotFound );
        goto HandleError;
    }

    // Get the size
    if ( !GetFileSizeEx( hFile, &li ) )
    {
        ExpectedSz( fFalse, "This is an unexpected failure. GetLastError = %u (0x%x), internal mapping = %d", GetLastError(), GetLastError(), ErrOSFileIGetLastError() );
        err = ErrERRCheck( JET_errFileNotFound );
        goto HandleError;
    }

    *pcbSize = li.QuadPart;

HandleError:
    
    if ( hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFile );
    }

    return err;
}


ERR COSFile::ErrDiskId( ULONG_PTR* const pulDiskId ) const
{
    ERR err = JET_errSuccess;
    
    Assert( pulDiskId != NULL );

    Call( m_posv->ErrDiskId( pulDiskId ) );

HandleError:

    return err;
}

ERR COSFile::ErrIOSetFileSize(
    const QWORD cbSize,
    const BOOL fReportError
    )
{
    ERR err = JET_errSuccess;
    DWORD errorSystem = ERROR_SUCCESS;

    Assert( NULL != m_p_osf );
    Assert( m_p_osf->semFilePointer.CAvail() != 0 );

    m_p_osf->semFilePointer.Acquire();

    const TICK tickStart = TickOSTimeCurrent();

    Assert( cbSize <= llMax );
    LARGE_INTEGER ibOffset;
    ibOffset.QuadPart = (LONGLONG)cbSize;
    if ( !SetFilePointerEx( m_hFile, ibOffset, NULL, FILE_BEGIN ) )
    {
        Assert( ERROR_IO_PENDING != GetLastError() );   // not bad, just unexpected
        Error( ErrOSFileIFromWinError( errorSystem = GetLastError() ) );
    }

    if ( !SetEndOfFile( m_hFile ) )
    {

        Assert( ERROR_IO_PENDING != GetLastError() );   // not bad, just unexpected
        Error( ErrOSFileIFromWinError( errorSystem = GetLastError() ) );
    }
    
HandleError:

    const TICK tickEnd = TickOSTimeCurrent();

    m_p_osf->semFilePointer.Release();

    if ( err != JET_errSuccess )
    {
        OSTrace(    JET_tracetagIOProblems,
                    OSFormat(   "%s:  pinst %s failed to set file size with error %s (0x%x) at offset %I64d.",
                                __FUNCTION__,
                                OSFormatPointer( m_p_osf->Pfsconfig()->PvTraceContext() ),
                                OSFormatSigned( err ) ,
                                errorSystem,
                                cbSize ) );

        if ( fReportError )
        {
            OSFileIIOReportError(
                m_p_osf->m_posf,
                fTrue /* fWrite */,
                cbSize,
                0 /* cbLength */,
                err,
                errorSystem,
                tickEnd - tickStart );
        }
    }

    return err;
}

ERR COSFile::ErrIOSetFileSparse(
    _In_ const HANDLE hfile,
    _In_ const BOOL fReportError )
{
    ERR err = JET_errSuccess;
    FILE_SET_SPARSE_BUFFER filesetsparsebuffer;
    DWORD errorSystem = ERROR_SUCCESS;

    filesetsparsebuffer.SetSparse = fTrue;

    Call( ErrOSFileIIoControlReadOnly(
        hfile,
        m_p_osf->Pfsconfig(),
        FSCTL_SET_SPARSE,
        &filesetsparsebuffer,
        sizeof( filesetsparsebuffer ),
        &errorSystem ) );

    const FileModeFlags fmfNew = *(FileModeFlags*)&m_fmf | fmfSparse;
    m_fmf = fmfNew;

HandleError:
    return err;
}

ERR COSFile::ErrIOSetFileRegionSparse(
    _In_ const HANDLE hfile,
    _In_ const QWORD ibOffset,
    _In_ const QWORD cbZeroes,
    _In_ const BOOL fReportError )
{
    ERR err = JET_errSuccess;
    FILE_ZERO_DATA_INFORMATION filezerodatainformation;
    DWORD errorSystem = ERROR_SUCCESS;

    // file should already be marked sparse
    Assert( ( m_fmf & fmfSparse ) || FNegTest( fInvalidUsage ) );

    Assert( 0 == ibOffset % cbSparseFileGranularity );
    Assert( 0 == cbZeroes % cbSparseFileGranularity );

    filezerodatainformation.FileOffset.QuadPart = ibOffset;
    filezerodatainformation.BeyondFinalZero.QuadPart = ibOffset + cbZeroes;

    const TICK tickStart = TickOSTimeCurrent();

    Call( ErrOSFileIIoControlReadOnly(
        hfile,
        m_p_osf->Pfsconfig(),
        FSCTL_SET_ZERO_DATA,
        &filezerodatainformation,
        sizeof( filezerodatainformation ),
        &errorSystem ) );

HandleError:

    const TICK tickEnd = TickOSTimeCurrent();

    if ( fReportError &&
            // suppress this for now.
            err != JET_errUnloadableOSFunctionality &&
            errorSystem != ERROR_SUCCESS )
    {
        OSFileIIOReportError(
            m_p_osf->m_posf,
            fTrue, // fWrite
            ibOffset,
            (DWORD) cbZeroes, // Possible truncation
            err,
            errorSystem,
            tickEnd - tickStart );
    }

    return err;
}

ERR COSFile::ErrIOGetAllocatedRange(
    _In_ const HANDLE hfile,
    _In_ const QWORD        ibOffsetToQuery,
    _Out_ QWORD* const  pibStartAllocatedRegion,
    _Out_ QWORD* const  pcbAllocate )
{
    ERR err = JET_errSuccess;
    BOOL fSucceeded;
    FILE_ALLOCATED_RANGE_BUFFER farbQuery;
    FILE_ALLOCATED_RANGE_BUFFER farbResult = { 0 };
    DWORD cbReturned = 0;
    OVERLAPPED overlapped;
    DWORD errorSystem = ERROR_SUCCESS;

    *pibStartAllocatedRegion = 0;
    *pcbAllocate = 0;

    farbQuery.FileOffset.QuadPart = ibOffsetToQuery;
    farbQuery.Length.QuadPart = 1024 * 1024 * 1024;

    memset( &overlapped, 0, sizeof( overlapped ) );
    overlapped.hEvent = CreateEventW( NULL, TRUE, FALSE, NULL );
    if ( NULL == overlapped.hEvent )
    {
        errorSystem = GetLastError();
        goto HandleError;
    }

    // Forces the IO to be synchronous, even if the file HANDLE is opened with IO Completion ports.
    overlapped.hEvent = HANDLE( DWORD_PTR( overlapped.hEvent ) | DWORD_PTR( hNoCPEvent ) );

    fSucceeded = DeviceIoControl( hfile,
                                  FSCTL_QUERY_ALLOCATED_RANGES,
                                  &farbQuery,
                                  sizeof( farbQuery ),
                                  &farbResult, // pOutBuffer
                                  sizeof( farbResult ), // cbOutButffer
                                  &cbReturned,
                                  &overlapped );

    if ( !fSucceeded )
    {
        errorSystem = GetLastError();
        if ( ERROR_IO_PENDING == errorSystem )
        {
            errorSystem = ERROR_SUCCESS;
            fSucceeded = GetOverlappedResult_( hfile, &overlapped, &cbReturned, TRUE );

            if ( !fSucceeded )
            {
                errorSystem = GetLastError();

                // It shouldn't still be pending!
                Assert( ERROR_IO_PENDING != errorSystem );
            }
        }

        if ( ERROR_MORE_DATA == errorSystem )
        {
            // This just indicates that there is more complexity than can be returned in
            // a single FILE_ALLOCATED_RANGE_BUFFER element (it wants an array).
            errorSystem = ERROR_SUCCESS;
        }

        if ( ERROR_SUCCESS != errorSystem )
        {
            // Unknown error. Time to bail.
            goto HandleError;
        }
    }

    // If the query was in a sparse region, then the output buffer is not touched.
    Assert( sizeof( farbResult ) == cbReturned || 0 == cbReturned );

    // Populate the output variables.
    *pibStartAllocatedRegion = farbResult.FileOffset.QuadPart;
    *pcbAllocate = farbResult.Length.QuadPart;

    // MSDN claims that this does not always hold (sometimes the FS rounds down?)
    Assert( farbResult.FileOffset.QuadPart >= farbQuery.FileOffset.QuadPart ||
            ( farbResult.FileOffset.QuadPart == 0 ) );

HandleError:

    if ( errorSystem != ERROR_SUCCESS )
    {
        Assert( ERROR_IO_PENDING != errorSystem );  // not bad, just unexpected
        Assert( ERROR_MORE_DATA != errorSystem );   // This should have been caught up above!

        // ERROR_INVALID_FUNCTION is returned on FAT systems.
        if ( errorSystem == ERROR_INVALID_FUNCTION )
        {
            err = ErrERRCheck( JET_errUnloadableOSFunctionality );
        }
        else
        {
            err = ErrOSFileIFromWinError( errorSystem );
        }

        OSTrace( JET_tracetagIOProblems,
                 OSFormat( "%s:  pinst %s failed to query the allocated region from %#I64x with error %s (0x%x).",
                           __FUNCTION__,
                           OSFormatPointer( m_p_osf->Pfsconfig()->PvTraceContext() ),
                           ibOffsetToQuery,
                           OSFormatSigned( err ) ,
                           errorSystem ) );
    }

    if ( overlapped.hEvent != NULL )
    {
        CloseHandle( overlapped.hEvent );
        overlapped.hEvent = NULL;
    }

    if ( err >= JET_errSuccess )
    {
        Expected( ( *pcbAllocate != 0 ) || ( *pibStartAllocatedRegion == 0 ) );
        Assert( ( *pibStartAllocatedRegion == 0 ) || ( *pibStartAllocatedRegion >= ibOffsetToQuery ) );
    }

    return err;
}

TICK COSFile::DtickIOElapsed( void* const pvIOContext ) const
{
    IOREQ* const    pioreq = (IOREQ*)pvIOContext;
    const QWORD     cmsecIOElapsed = CmsecLatencyOfOSOperation( pioreq );

    return (TICK)min( cmsecIOElapsed, dwMax );
}
