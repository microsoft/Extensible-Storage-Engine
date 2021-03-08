// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#include "osstd.hxx"

#include <winioctl.h>



#ifndef OS_LAYER_VIOLATIONS
HANDLE HOSLayerTestGetFileHandle( IFileAPI * pfapi )
{
    const HANDLE hFile = ((COSFile*)pfapi)->Handle();
    return hFile;
}
#endif


C_ASSERT( NO_ERROR == ERROR_SUCCESS );

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

        case ERROR_VC_DISCONNECTED:
        case ERROR_IO_DEVICE:
        case ERROR_DEVICE_NOT_CONNECTED:
        case ERROR_NOT_READY:
            return ErrERRCheck_( JET_errDiskIO, szFile, lLine );

        case ERROR_FILE_CORRUPT:
        case ERROR_DISK_CORRUPT:
            return ErrERRCheck( JET_errFileSystemCorruption );

        case ERROR_NO_MORE_FILES:
        case ERROR_FILE_NOT_FOUND:
            return ErrERRCheck_( JET_errFileNotFound, szFile, lLine );

        case ERROR_PATH_NOT_FOUND:
        case ERROR_BAD_PATHNAME:
            return ErrERRCheck_( JET_errInvalidPath, szFile, lLine );

        case ERROR_ACCESS_DENIED:
        case ERROR_SHARING_VIOLATION:
        case ERROR_LOCK_VIOLATION:
        case ERROR_WRITE_PROTECT:
            return ErrERRCheck_( JET_errFileAccessDenied, szFile, lLine );

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

    overlapped.hEvent = HANDLE( DWORD_PTR( overlapped.hEvent ) | hNoCPEvent );

    fSucceeded = DeviceIoControl( hfile,
                                  dwIoControlCode,
                                  pvInBuffer,
                                  cbInBufferSize,
                                  NULL,
                                  0,
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
        Assert( ERROR_IO_PENDING != *perrorSystem );

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



void OSFilePostterm()
{
}


BOOL FOSFilePreinit()
{

    return fTrue;
}



QWORD           g_cbZero = 1024 * 1024;
BYTE*           g_rgbZero;
COSMemoryMap*   g_posmmZero;

void COSLayerPreInit::SetZeroExtend( QWORD cbZeroExtend )
{
    if ( cbZeroExtend < OSMemoryPageCommitGranularity() )
    {
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


QWORD           g_cbMMSize;

POSTRACEREFLOG  g_pFfbTraceLog = NULL;


void OSFileTerm()
{

    if ( g_cbZero < OSMemoryPageCommitGranularity() )
    {

        OSMemoryHeapFree( g_rgbZero );
        g_rgbZero = NULL;
    }
    else if ( !COSMemoryMap::FCanMultiMap() ||
        g_cbZero < OSMemoryPageReserveGranularity() )
    {

        OSMemoryPageFree( g_rgbZero );
        g_rgbZero = NULL;
    }
    else if ( g_posmmZero )
    {

        g_posmmZero->OSMMPatternFree();
        g_rgbZero = NULL;


        g_posmmZero->OSMMTerm();
        delete g_posmmZero;
        g_posmmZero = NULL;
    }

    OSTraceDestroyRefLog( g_pFfbTraceLog );
    g_pFfbTraceLog = NULL;
}



ERR ErrOSFileInit()
{
    ERR     err = JET_errSuccess;
    size_t  cbFill = 0;

#ifdef DEBUG
    (VOID)ErrOSTraceCreateRefLog( 1000, 0, &g_pFfbTraceLog );
#endif


    g_rgbZero = NULL;
    g_posmmZero = NULL;


    SYSTEM_INFO sinf;
    GetSystemInfo( &sinf );
    g_cbMMSize = sinf.dwAllocationGranularity;


    if ( g_cbZero < OSMemoryPageCommitGranularity() )
    {

        if ( !( g_rgbZero = (BYTE*)PvOSMemoryHeapAlloc( size_t( g_cbZero ) ) ) )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }

        cbFill = size_t( g_cbZero );
    }
    else if ( !COSMemoryMap::FCanMultiMap() ||
        g_cbZero < OSMemoryPageReserveGranularity() )
    {

        if ( !( g_rgbZero = (BYTE*)PvOSMemoryPageAlloc( size_t( g_cbZero ), NULL ) ) )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }

        cbFill = size_t( g_cbZero );
    }
    else
    {


        g_posmmZero = new COSMemoryMap();
        if ( !g_posmmZero )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }


        COSMemoryMap::ERR errOSMM;
        errOSMM = g_posmmZero->ErrOSMMInit();
        if ( COSMemoryMap::ERR::errSuccess != errOSMM )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }


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


    m_rgcbFileSize[ 0 ] = m_cbFileSize;
    m_semChangeFileSize.Release();


    m_posv = posv;
    ASSERT_VALID( m_posv );


    if ( !( m_p_osf = new _OSFILE ) )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }


    CallS( m_posv->ErrGetDisk( &(m_p_osf->m_posd) ) );
    Assert( m_p_osf->m_posd );
    ASSERT_VALID( m_p_osf->m_posd );

    m_p_osf->m_posf             = this;
    m_p_osf->hFile              = m_hFile;
    m_p_osf->pfnFileIOComplete  = _OSFILE::PfnFileIOComplete( IOComplete_ );
    m_p_osf->keyFileIOComplete  = DWORD_PTR( this );


    m_p_osf->pfsconfig          = Pfsconfig();


    m_p_osf->pfpapi             = &g_cosfileperfDefault;


    m_p_osf->iFile = QWORD( DWORD_PTR( m_p_osf->hFile ) );


    m_p_osf->iomethodMost = IOREQ::iomethodScatterGather;


    m_p_osf->fRegistered = fFalse;


    m_p_osf->fmfTracing = m_fmf;


    return JET_errSuccess;

HandleError:
    m_hFile = INVALID_HANDLE_VALUE;
    m_posv = NULL;
    return err;
}

COSFile::~COSFile()
{
    TraceStationId( tsidrCloseTerm );

#ifdef OS_LAYER_VIOLATIONS

    if ( m_errFlushFailed >= JET_errSuccess && !( m_fmf & IFileAPI::fmfTemporary ) )
    {
        Assert( m_cioUnflushed == 0 || FNegTest( fLeakingUnflushedIos )  );
        Assert( m_cioFlushing == 0 );
    }
#endif

    if ( CioNonFlushed() > 0 && m_errFlushFailed >= JET_errSuccess && !( m_fmf & IFileAPI::fmfTemporary ) )
    {
#ifdef OS_LAYER_VIOLATIONS
        AssertSz( fFalse, "All ESE-level files should be completely flushed by file close." );
#endif
        (void)ErrFlushFileBuffers( (IOFLUSHREASON) 0x00800000  );
    }

    
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


    m_posfs = NULL;
}

void COSFile::TraceStationId( const TraceStationIdentificationReason tsidr )
{

    if ( m_p_osf == NULL )
    {
        return;
    }
    Assert( m_p_osf );
    Assert( m_p_osf->pfpapi );
    Assert( m_p_osf->m_posd );

    if ( tsidr == tsidrPulseInfo )
    {

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

    const QWORD hFile = m_p_osf->iFile;
    const DWORD dwEngineFileType = (DWORD)m_p_osf->pfpapi->DwEngineFileType();
    const QWORD qwEngineFileId = m_p_osf->pfpapi->QwEngineFileId();
    const DWORD dwDiskNumber = m_p_osf->m_posd->DwDiskNumber();
    
    ETFileStationId( tsidr, hFile, dwDiskNumber, dwEngineFileType, qwEngineFileId, m_fmf, m_cbFileSize, m_wszAbsPath );
}


ERR COSFile::ErrPath( _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) WCHAR* const wszAbsPath )
{
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


    if ( m_errFlushFailed != JET_errSuccess )
    {
        return ErrERRCheck( m_errFlushFailed );
    }

    if ( !( m_fmf & fmfStorageWriteBack ) &&
         !( m_fmf & fmfLossyWriteBack ) )
    {
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

    Assert( m_cioUnflushed >= 0 );
    Assert( m_cioFlushing >= ciosDelta );

    m_critDefer.Leave();

    m_p_osf->m_posd->TrackOsFfbBegin( iofr, (QWORD)m_hFile );
    if ( g_pFfbTraceLog )
    {
        OSTraceWriteRefLog( g_pFfbTraceLog, (LONG)m_cioUnflushed, this );
    }

    HRT hrtStart = HrtHRTCount();

    BOOL fFaultedFlushSucceeded = RFSAlloc( OSFileFlush );
    if ( !fFaultedFlushSucceeded )
    {
        UtilSleep( 80 );
        SetLastError( ERROR_BROKEN_PIPE  );
    }

    DWORD error = ERROR_SUCCESS;

    if ( !fFaultedFlushSucceeded || !FlushFileBuffers( m_hFile ) )
    {
        error = GetLastError();
        err = ErrOSFileIFromWinError( error );
        Assert( ERROR_IO_PENDING != error );
        Assert( err != wrnIOPending );
        if ( error == ERROR_SUCCESS || err >= JET_errSuccess )
        {
            FireWall( "FfbFailedWithSuccess" );
            error = ERROR_BAD_PIPE;
            err = ErrERRCheck( errCodeInconsistency );
        }
    }

    const HRT dhrtFfb = HrtHRTCount() - hrtStart;
    const QWORD usFfb = CusecHRTFromDhrt( dhrtFfb );

    m_p_osf->m_posd->TrackOsFfbComplete( iofr, error, hrtStart, usFfb, ciosDelta, m_posfs->WszPathFileName( WszFile() ) );

    if ( err >= JET_errSuccess )
    {
        m_p_osf->pfpapi->IncrementFlushFileBuffer( dhrtFfb );
        
        m_critDefer.Enter();
        AtomicAdd( (QWORD*)&m_cioFlushing, -ciosDelta );
        Assert( m_cioFlushing >= 0 );
        m_critDefer.Leave();

    }
    else
    {
        m_critDefer.Enter();
        (void)AtomicCompareExchange( (LONG*)&m_errFlushFailed, (LONG)JET_errSuccess, (LONG)err );
        AtomicAdd( (QWORD*)&m_cioUnflushed, ciosDelta );
        AtomicAdd( (QWORD*)&m_cioFlushing, -ciosDelta );
        Assert( m_cioFlushing >= 0 );
        m_critDefer.Leave();

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
    Assert( qosIODispatchMask & grbitQOS );
    Assert( !( m_fmf & IFileAPI::fmfReadOnlyClient ) );
    Expected( !( m_fmf & IFileAPI::fmfReadOnly ) );


    CExtendingWriteRequest* const pewreq = new CExtendingWriteRequest();
    if ( !pewreq )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }


    IOREQ* pioreq = NULL;

    Assert( qosIODispatchImmediate == ( qosIODispatchMask & grbitQOS ) );
    CallS( m_p_osf->m_posd->ErrAllocIOREQ( grbitQOS, NULL, fTrue, 0, 0, &pioreq ) );

    Assert( NULL != pioreq );

    pioreq->p_osf           = m_p_osf;
    
    pioreq->SetIOREQType( IOREQ::ioreqSetSize );
    pioreq->grbitQOS = grbitQOS;

    Assert( tc.iorReason.Iorp() != iorpNone );
    pioreq->m_tc.DeepCopy( getutc.Utc(), tc );


    m_semChangeFileSize.Acquire();
    const INT group = m_msFileSize.GroupActive();

    if ( m_rgcbFileSize[ group ] >= cbSize )
    {
        pioreq->ibOffset        = cbSize;
    }
    else if ( ErrFaultInjection( 47100 ) >= JET_errSuccess &&
              ErrIOSetFileSize( cbSize, fFalse  ) >= JET_errSuccess &&
              !( m_fmf & fmfSparse ) )
    {
        pioreq->ibOffset        = cbSize;

        if ( !fZeroFill || ( m_fmf & fmfTemporary ) )
        {
            m_p_osf->semFilePointer.Acquire();
            (void)SetFileValidData( m_hFile, cbSize );
            m_p_osf->semFilePointer.Release();
        }
    }
    else
    {
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
    pewreq->m_ibOffset          = cbSize;
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
    Expected( !( m_fmf & IFileAPI::fmfReadOnly ) );

    if ( m_posfs->FPathIsRelative( wszAbsPathDest ) )
    {
        Assert( !m_posfs->FPathIsRelative( wszAbsPathDest ) || FNegTest( fInvalidUsage ) );
        return ErrERRCheck( JET_errInvalidParameter );
    }



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
    Expected( !( m_fmf & IFileAPI::fmfReadOnly ) );

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

USHORT UsEvenRand( const USHORT usRangeMax )
{
    Expected( usRangeMax <= 4100 );
    unsigned int uiT;
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

    Assert( cbData );

    Assert( qosIODispatchMask & grbitQOS );
    Assert( ( ( qosIODispatchMask & grbitQOS ) == qosIODispatchImmediate ) ||
            ( ( qosIODispatchMask & grbitQOS ) == qosIODispatchBackground ) );
    Assert( !( ( grbitQOS & qosIODispatchBackground ) && ( grbitQOS & qosIODispatchImmediate ) ) );

    Assert( !( qosIOSignalSlowSyncIO & grbitQOS ) || pfnIOComplete == NULL || pfnIOComplete == PfnIOComplete( IOSyncComplete_ ) );

    Assert( !( qosIOReadCopyMask & grbitQOS ) || pfnIOComplete == NULL || pfnIOComplete == PfnIOComplete( IOSyncComplete_ ) );

    Assert( 0 == ( qosIOCompleteMask & grbitQOS ) );

    if ( Pfsconfig()->PermillageSmoothIo() != 0 && !( grbitQOS & qosIOReadCopyMask ) )
    {
        if ( FOSFileTestUrgentIo() )
        {
            grbitQOS = ( grbitQOS & ~qosIODispatchBackground ) | qosIODispatchImmediate;
        }
        else
        {
            grbitQOS = ( grbitQOS & ~qosIODispatchImmediate ) | qosIODispatchBackground;
        }
    }


    if ( pfnIOComplete )
    {

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


    else
    {
        CIOComplete iocomplete;
        iocomplete.m_keyIOComplete = keyIOComplete;
        iocomplete.m_pfnIOHandoff = pfnIOHandoff;

        Assert( !( grbitQOS & qosIOOptimizeCombinable ) );
        Assert( NULL == pvioreq );


        OSFILEQOS   qosEffective = grbitQOS;
        if ( ErrFaultInjection( 59364 ) )
        {
            qosEffective = ( qosEffective & ~qosIODispatchBackground ) | qosIODispatchImmediate | qosIOReadCopyTestExclusiveIo;
        }


        Assert( qosIODispatchImmediate == ( grbitQOS & qosIODispatchMask ) || qosIODispatchBackground == ( grbitQOS & qosIODispatchMask ) );
        CallS( ErrIORead(   tc,
                            ibOffset,
                            cbData,
                            pbData,
                            qosEffective,
                            PfnIOComplete( IOSyncComplete_ ),
                            DWORD_PTR( &iocomplete ),
                            ( pfnIOHandoff != NULL ) ? PfnIOHandoff( IOSyncHandoff_ ) : NULL ) );


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

        Assert( ( errDiskTilt == err ) || ( JET_errFileIOBeyondEOF == err ) );
        Expected( ( errDiskTilt != err ) ||
                    ( ( qosIODispatchImmediate != ( grbitQOS & qosIODispatchMask ) ) ||
                        ( grbitQOS & qosIOOptimizeCombinable ) ) );
    }
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
    Expected( !( m_fmf & IFileAPI::fmfReadOnly ) );

    AssertRTL( tc.iorReason.FValid() );
    Assert( tc == *PetcTLSGetEngineContext() );

    Assert( cbData );

    Assert( qosIODispatchMask & grbitQOS );
    Assert( !( ( grbitQOS & qosIODispatchBackground ) && ( grbitQOS & qosIODispatchImmediate ) ) );

    Assert( !( qosIOSignalSlowSyncIO & grbitQOS ) || pfnIOComplete == NULL || pfnIOComplete == PfnIOComplete( IOSyncComplete_ ) );

    Assert( 0 == ( qosIOCompleteMask & grbitQOS ) );


    if ( pfnIOComplete )
    {

        IOREQ* pioreq = NULL;
        Call( m_p_osf->m_posd->ErrAllocIOREQ( grbitQOS, m_p_osf, fTrue, ibOffset, cbData, &pioreq ) );


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


    else
    {
        CIOComplete iocomplete;
        iocomplete.m_keyIOComplete = keyIOComplete;
        iocomplete.m_pfnIOHandoff = pfnIOHandoff;

        Assert( !( grbitQOS & qosIOOptimizeCombinable ) );


        Assert( qosIODispatchImmediate == ( grbitQOS & qosIODispatchMask ) );
        CallS( ErrIOWrite(  tc,
                            ibOffset,
                            cbData,
                            pbData,
                            grbitQOS,
                            PfnIOComplete( IOSyncComplete_ ),
                            DWORD_PTR( &iocomplete ),
                            ( pfnIOHandoff != NULL ) ? PfnIOHandoff( IOSyncHandoff_ ) : NULL ) );


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

        Assert( errDiskTilt == err );
    }
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

    Assert( qosIODispatchMask & grbitQOS );
    Assert( !( pfapi->Fmf() & IFileAPI::fmfReadOnlyClient ) );
    Expected( !( pfapi->Fmf() & IFileAPI::fmfReadOnly ) );

    Expected( ( grbitQOS & qosIOOptimizeCombinable ) );

    Assert( 0 == ( qosIOCompleteMask & grbitQOS ) );

    COSFile::CIOComplete rgStackIoComplete[2];
    COSFile::CIOComplete *rgIoComplete  = rgStackIoComplete;

    if ( cData > _countof(rgStackIoComplete) )
    {
        rgIoComplete = new COSFile::CIOComplete[cData];
        if ( rgIoComplete == NULL )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }
        fAllocatedFromHeap = fTrue;
    }


    QWORD ibOffsetCurrent = ibOffset;
    size_t iData = 0;
    for ( iData = 0; iData < cData; ++iData )
    {
        Assert( rgcbData[iData] );
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
            break;
        }
        ibOffsetCurrent += rgcbData[iData];
    }
    if ( iData > 0 )
    {
        CallS( pfapi->ErrIOIssue() );
    }
    for ( size_t i = 0; i < iData; ++i )
    {
        
        rgIoComplete[i].Wait();
    }
    if ( iData == cData )
    {
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
            Assert( ibAlloc == ib );
            Assert( cbAlloc != 0 );
        }

        ib = ( cbAlloc != 0 ) ? ( ibAlloc + cbAlloc ) : ( ibLast + 1 );
    }

HandleError:

#if DEBUG
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
        MarkHandleInfo.UsnSourceInfo = iCopyNumber;
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


    m_p_osf->m_posd->EnqueueDeferredIORun( m_p_osf );


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


    if ( !RFSAlloc( OSMemoryPageAddressSpace ) )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }


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
            Assert( ERROR_IO_PENDING != GetLastError() );
            Call( ErrOSFileIGetLastError() );
        }
        if ( AtomicCompareExchangePointer( (void**)&m_rghFileMap[ group ], NULL, hFileMap ) == NULL )
        {
            hFileMap = NULL;
        }
    }


    const QWORD ibOffsetAlign   = ( ibOffset / g_cbMMSize ) * g_cbMMSize;
    const QWORD ibOffsetBias    = ibOffset - ibOffsetAlign;
    const QWORD cbViewSize      = min( m_rgcbFileSize[ group ] - ibOffsetAlign, ( ( ibOffsetBias + cbSize + g_cbMMSize - 1 ) / g_cbMMSize ) * g_cbMMSize );
    void* const pvHint          = (void*)( ( DWORD_PTR( *ppvMap ) / g_cbMMSize ) * g_cbMMSize );


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
        Assert( ERROR_IO_PENDING != GetLastError() );
        Assert( pvMap == NULL );
        Call( ErrOSFileIGetLastError() );
    }
    Assert( ( DWORD_PTR( pvMap ) / g_cbMMSize ) * g_cbMMSize == DWORD_PTR( pvMap ) - ibOffsetBias );


    Assert( err == JET_errSuccess );
    if ( !RFSAlloc( OSFileRead ) || ErrFaultInjection( 34060 ) )
    {
        DWORD flOldProtect;
        (void)VirtualProtect( pvMap, size_t( cbSize ), PAGE_NOACCESS, &flOldProtect );
        err = ErrERRCheck( wrnLossy );
    }


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

    Assert( QWORD( size_t( cbSize ) ) == cbSize );
    const size_t cbSizeT = size_t( cbSize );


    Call( ErrMMRead( ibOffset, cbSize, ppvMap ) );

    Assert( FOSMemoryFileMapped( *ppvMap, cbSizeT ) );
    Assert( !FOSMemoryFileMappedCowed( *ppvMap, cbSizeT ) );
    Assert( !FOSMemoryPageAllocated( *ppvMap, cbSizeT ) );


    if ( err != wrnLossy )
    {

        DWORD flOldProtect;
        if ( !VirtualProtect( *ppvMap, cbSizeT, PAGE_WRITECOPY, &flOldProtect ) )
        {
            Call( ErrOSFileIGetLastError() );
        }
    }
    err = JET_errSuccess;

    Assert( FOSMemoryFileMapped( *ppvMap, cbSizeT ) );
    Assert( !FOSMemoryFileMappedCowed( *ppvMap, cbSizeT ) );
    Assert( !FOSMemoryPageAllocated( *ppvMap, cbSizeT ) );

HandleError:

    Expected( err != JET_errFileAccessDenied );

    if ( err < JET_errSuccess )
    {
        Expected( *ppvMap == NULL );
        *ppvMap = NULL;
    }

    return err;
}


ERR COSFile::ErrMMIORead(
    _In_                    const QWORD             ibOffset,
    _Out_writes_bytes_(cb)  BYTE * const            pb,
    _In_                    ULONG                   cb,
    _In_                    const FileMmIoReadFlag  fmmiorf )
{
    Assert( FOSMemoryFileMapped( pb, cb ) );
    Assert( !FOSMemoryPageAllocated( pb, cb ) );
    Expected( ( fmmiorf & fmmiorfKeepCleanMapped ) || ( fmmiorf & fmmiorfCopyWritePage ) );
    Expected( ibOffset != 0 );

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


    if ( size_t( cbSize ) != cbSize )
    {
        ExpectedSz( fFalse, "QWORD size truncated on 32-bit. %I64d != %I64d", cbSize, (QWORD)size_t( cbSize ) );
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }
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
        Error( ErrERRCheck( JET_errFeatureNotAvailable ) );
    }


    DWORD flOldProtect;
    if ( !VirtualProtect( pvMap, size_t( cbSize ), PAGE_REVERT_TO_FILE_MAP | PAGE_WRITECOPY, &flOldProtect ) )
    {
        const DWORD error = GetLastError();
        err = ErrOSFileIGetLastError();
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
            Assert( ERROR_IO_PENDING != GetLastError() );
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

    if ( pioreq->m_tc.etc.FEmpty() )
    {
        GetCurrUserTraceContext getutc;
        const TraceContext* petc = PetcTLSGetEngineContext();
        Assert( petc->iorReason.FValid() );
        Assert( petc->iorReason.Iorp() != iorpNone );
        pioreq->m_tc.DeepCopy( getutc.Utc(), *petc );
    }

    pioreq->hrtIOStart = HrtHRTCount();


    if (    pioreq->fWrite &&
            pioreq->ibOffset + pioreq->cbData > m_rgcbFileSize[ pioreq->group ] )
    {
        pioreq->m_fCanCombineIO = fFalse;

        if ( !pioreq->m_fHasHeapReservation )
        {

            Call( m_p_osf->m_posd->ErrReserveQueueSpace( grbitQOS, pioreq ) );

            Assert( pioreq->m_fHasHeapReservation || ( pioreq->grbitQOS & qosIOPoolReserved ) );
            Assert( pioreq->FEnqueueable() );
        }


        if ( pfnIOHandoff )
        {
            Assert( ( (void*)IOSyncComplete_ != (void*)pfnIOComplete ) || ( (void*)IOSyncHandoff_ == (void*)pfnIOHandoff ) );
            pfnIOHandoff( JET_errSuccess, this, pioreq->m_tc, grbitQOS, ibOffset, cbData, pbData, keyIOComplete, pioreq );
        }


        CExtendingWriteRequest* const pewreq = new CExtendingWriteRequest();

        pioreq->SetIOREQType( IOREQ::ioreqExtendingWriteIssued );


        if ( !pewreq )
        {

            OSDiskIIOThreadCompleteWithErr( ERROR_NOT_ENOUGH_MEMORY,
                                            pioreq->cbData,
                                            pioreq );


            Assert( err == JET_errSuccess );
            goto HandleError;
        }



        pewreq->m_posf              = this;
        pewreq->m_pioreq            = pioreq;
        pewreq->m_group             = group;
        pewreq->m_tc.DeepCopy( pioreq->m_tc.utc, pioreq->m_tc.etc );
        pewreq->m_ibOffset          = ibOffset;
        pewreq->m_cbData            = cbData;
        pewreq->m_pbData            = pbData;
        pewreq->m_grbitQOS          = grbitQOS;
        pewreq->m_pfnIOComplete     = pfnIOComplete;
        pewreq->m_keyIOComplete     = keyIOComplete;
        pewreq->m_tickReqStart      = TickOSTimeCurrent();
        pewreq->m_tickReqStep       = 0;
        pewreq->m_tickReqComplete   = 0;


        if ( m_semChangeFileSize.FTryAcquire() )
        {

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


        else
        {

            const INT groupT = pioreq->group;

            m_critDefer.Enter();
            m_ilDefer.InsertAsNextMost( pewreq );
            m_critDefer.Leave();

            m_msFileSize.Leave( groupT );
        }

    }


    else if (   COSFile::FOSFileSyncComplete( pioreq ) &&
                !Postls()->fIOThread )
    {


        if ( pfnIOHandoff )
        {
            Assert( ( (void*)IOSyncComplete_ != (void*)pfnIOComplete ) || ( (void*)IOSyncHandoff_ == (void*)pfnIOHandoff ) );
            pfnIOHandoff( JET_errSuccess, this, pioreq->m_tc, grbitQOS, ibOffset, cbData, pbData, keyIOComplete, pioreq );
        }


        if ( pioreq->cbData == 0 )
        {

            pioreq->m_posdCurrentIO = pioreq->p_osf->m_posd;
            pioreq->SetIOREQType( IOREQ::ioreqIssuedSyncIO );
            IOMgrCompleteOp( pioreq );
        }
        
        
        else
        {
            IOMgrIssueSyncIO( pioreq );
        }
    }


    else
    {

        if ( pfnIOHandoff )
        {
            Assert( ( (void*)IOSyncComplete_ != (void*)pfnIOComplete ) || ( (void*)IOSyncHandoff_ == (void*)pfnIOHandoff ) );
            pfnIOHandoff( JET_errSuccess, this, pioreq->m_tc, grbitQOS, ibOffset, cbData, pbData, keyIOComplete, pioreq );
        }


        m_p_osf->m_posd->EnqueueIOREQ( pioreq );
    }

HandleError:

    Assert( err == JET_errSuccess ||
            err == errDiskTilt );

    if ( err == errDiskTilt )
    {

        Assert( qosIODispatchImmediate != ( grbitQOS & qosIODispatchMask ) );
        Assert( qosIOPoolReserved != ( grbitQOS & qosIODispatchMask ) );


        m_msFileSize.Leave( pioreq->group );

        pioreq->m_fCanCombineIO = fFalse;
        pioreq->m_tc.Clear();

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

    pewreq->m_err = err;
    pewreq->m_tickReqStep = TickOSTimeCurrent();
    Assert( ( pewreq->m_tickReqStep - pewreq->m_tickReqStart ) < ( ( 10 * (TICK)cmsecDeadlock ) ) );
    Assert( pewreq->m_tickReqComplete == 0 );


    DWORD cbZero = (DWORD)g_cbZero;
    const BYTE* const rgbZero = g_rgbZero;


    if ( pewreq->m_err >= JET_errSuccess &&
        ibOffset + cbData == m_rgcbFileSize[ pewreq->m_group ] &&
        pewreq->m_ibOffset > m_rgcbFileSize[ pewreq->m_group ] &&
        ( pewreq->m_cbData > 0 || pewreq->m_ibOffset - m_rgcbFileSize[ pewreq->m_group ] > cbZero ) &&
        m_rghFileMap[ pewreq->m_group ] == NULL )
    {

        const QWORD cbSize      = pewreq->m_ibOffset + pewreq->m_cbData;

        const ERR errSetFileSize = ErrIOSetFileSize( cbSize, fTrue  );

        pewreq->m_err = (   pewreq->m_err < JET_errSuccess ?
                            pewreq->m_err :
                            errSetFileSize );
    }


    if ( pewreq->m_err >= JET_errSuccess )
    {


        Assert( pewreq->m_pioreq );

        Assert( pewreq->m_tc.etc.iorReason.FValid() );

        OSFILEQOS qosT = pewreq->m_grbitQOS;
        qosT = ( qosT & ~qosIODispatchMask ) | qosIODispatchImmediate;
        
        const QWORD ibWriteRemaining = ibOffset + cbData;
        if ( ibWriteRemaining < pewreq->m_ibOffset )
        {

            const QWORD ibWrite = ( ( ( cbZero < 64 * 1024 ) && ( ( ibWriteRemaining + cbZero ) <= pewreq->m_ibOffset )  ) ?
                                        pewreq->m_ibOffset - cbZero :
                                        ibWriteRemaining );



            const QWORD cbWrite = min(  cbZero,
                                        pewreq->m_ibOffset - ibWrite );
            Assert( DWORD( cbWrite ) == cbWrite );


            m_rgcbFileSize[ 1 - pewreq->m_group ] = ibWrite + cbWrite;


            const P_OSFILE p_osf = pewreq->m_posf->m_p_osf;
            Assert( p_osf == m_p_osf );

            IOREQ* pioreq = NULL;
            Assert( qosIODispatchImmediate == ( qosIODispatchMask & qosT ) );

            CallS( p_osf->m_posd->ErrAllocIOREQ( qosT, NULL, fTrue, 0, 0, &pioreq ) );
            Assert( pioreq->FEnqueueable() || Postls()->pioreqCache == NULL );

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


        else
        {

            m_rgcbFileSize[ 1 - pewreq->m_group ] = pewreq->m_ibOffset + pewreq->m_cbData;


            const P_OSFILE p_osf = pewreq->m_posf->m_p_osf;
            Assert( p_osf == m_p_osf );

            IOREQ* pioreq = NULL;
            Assert( qosIODispatchImmediate == ( qosIODispatchMask & qosT ) );
            CallS( p_osf->m_posd->ErrAllocIOREQ( qosT, NULL, fTrue, 0, 0, &pioreq ) );
            Assert( pioreq->FEnqueueable() || Postls()->pioreqCache == NULL );

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


    else
    {
        pewreq->m_tickReqStep = TickOSTimeCurrent();
        Assert( ( pewreq->m_tickReqStep - pewreq->m_tickReqStart ) < ( 10 * (TICK)cmsecDeadlock ) );
        Assert( pewreq->m_tickReqComplete == 0 );
        pewreq->m_tickReqComplete = pewreq->m_tickReqStep;
        

        m_rgcbFileSize[ 1 - pewreq->m_group ] = m_rgcbFileSize[ pewreq->m_group ];


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

    pewreq->m_err = err;
    Assert( pewreq->m_tickReqComplete == 0 );
    pewreq->m_tickReqComplete = TickOSTimeCurrent();
    Assert( ( pewreq->m_tickReqStep - pewreq->m_tickReqStart ) < ( 10 * (TICK)cmsecDeadlock ) );
    Assert( ( pewreq->m_tickReqComplete - pewreq->m_tickReqStep ) < ( 10 * (TICK)cmsecDeadlock ) );


    if ( err >= JET_errSuccess )
    {

        m_msFileSize.Partition( CMeteredSection::PFNPARTITIONCOMPLETE( IOChangeFileSizeComplete_ ),
                                DWORD_PTR( pewreq ) );
    }


    else
    {

        m_rgcbFileSize[ 1 - pewreq->m_group ] = m_rgcbFileSize[ pewreq->m_group ];


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


    if ( m_rghFileMap[ pewreq->m_group ] )
    {
        CloseHandle( m_rghFileMap[ pewreq->m_group ] );
        m_rghFileMap[ pewreq->m_group ] = NULL;
    }


    if ( cbSize < m_rgcbFileSize[ pewreq->m_group ] ||
        pewreq->m_err < JET_errSuccess )
    {

        const ERR errSetFileSize = ErrIOSetFileSize( cbSize, fTrue  );

        pewreq->m_err = (   pewreq->m_err < JET_errSuccess ?
                            pewreq->m_err :
                            errSetFileSize );
    }


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


    m_semChangeFileSize.Release();


    m_critDefer.Enter();
    CDeferList ilDefer = m_ilDefer;
    m_ilDefer.Empty();
    m_critDefer.Leave();


    Assert( pewreq->m_pioreq );

    Assert( pewreq->m_tc.etc.iorReason.FValid() );


    pewreq->m_pfnIOComplete(    pewreq->m_err,
                                this,
                                pewreq->m_tc,
                                pewreq->m_grbitQOS,
                                pewreq->m_ibOffset,
                                pewreq->m_cbData,
                                pewreq->m_pbData,
                                pewreq->m_keyIOComplete );

    delete pewreq;


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
        Assert( this == pewreqEOF->m_posf );


        pewreqEOF->m_pioreq->SetIOREQType( IOREQ::ioreqAllocFromEwreqLookaside );

        Assert( pewreqEOF->m_pioreq->FEnqueueable() || Postls()->pioreqCache == NULL );
        Expected( pewreqEOF->m_pioreq->FEnqueueable() || ( Postls()->pioreqCache == NULL && FIOThread() ) );
        Expected( pewreqEOF->m_pioreq->FEnqueueable() );
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
        Assert( this == pewreqDefer->m_posf );


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

    *pcbSize = 0;

    CallS( ErrPath( wszAttrList ) );
    OSStrCbAppendW( wszAttrList, sizeof(wszAttrList), L"::$ATTRIBUTE_LIST" );

    hFile = CreateFileW( wszAttrList, FILE_READ_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    if ( hFile == INVALID_HANDLE_VALUE )
    {
        err = ErrERRCheck( JET_errFileNotFound );
        goto HandleError;
    }

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
        Assert( ERROR_IO_PENDING != GetLastError() );
        Error( ErrOSFileIFromWinError( errorSystem = GetLastError() ) );
    }

    if ( !SetEndOfFile( m_hFile ) )
    {

        Assert( ERROR_IO_PENDING != GetLastError() );
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
                fTrue ,
                cbSize,
                0 ,
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
            err != JET_errUnloadableOSFunctionality &&
            errorSystem != ERROR_SUCCESS )
    {
        OSFileIIOReportError(
            m_p_osf->m_posf,
            fTrue,
            ibOffset,
            (DWORD) cbZeroes,
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

    overlapped.hEvent = HANDLE( DWORD_PTR( overlapped.hEvent ) | DWORD_PTR( hNoCPEvent ) );

    fSucceeded = DeviceIoControl( hfile,
                                  FSCTL_QUERY_ALLOCATED_RANGES,
                                  &farbQuery,
                                  sizeof( farbQuery ),
                                  &farbResult,
                                  sizeof( farbResult ),
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

                Assert( ERROR_IO_PENDING != errorSystem );
            }
        }

        if ( ERROR_MORE_DATA == errorSystem )
        {
            errorSystem = ERROR_SUCCESS;
        }

        if ( ERROR_SUCCESS != errorSystem )
        {
            goto HandleError;
        }
    }

    Assert( sizeof( farbResult ) == cbReturned || 0 == cbReturned );

    *pibStartAllocatedRegion = farbResult.FileOffset.QuadPart;
    *pcbAllocate = farbResult.Length.QuadPart;

    Assert( farbResult.FileOffset.QuadPart >= farbQuery.FileOffset.QuadPart ||
            ( farbResult.FileOffset.QuadPart == 0 ) );

HandleError:

    if ( errorSystem != ERROR_SUCCESS )
    {
        Assert( ERROR_IO_PENDING != errorSystem );
        Assert( ERROR_MORE_DATA != errorSystem );

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
