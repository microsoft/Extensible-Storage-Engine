// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  Block Cache Header Helpers

class CBlockCacheHeaderHelpers
{
    public:

        enum { ibFileType = 667 };
        enum { cbGuid = sizeof(GUID) };
        enum { cbBlock = cbCachedBlock };

        enum class ChecksumType
        {
            checksumLegacy,
            checksumECC,
            checksumDefault = checksumLegacy,
        };

    protected:

        static ERR ErrGetFileId(    _In_    IFileFilter* const              pff, 
                                    _Out_   LittleEndian<VolumeId>* const   pvolumeid, 
                                    _Out_   LittleEndian<FileId>* const     pfileid );
        static ERR ErrVerifyFileId( _In_    IFileSystemConfiguration* const pfsconfig, 
                                    _In_    IFileFilter* const              pff, 
                                    _In_    const VolumeId                  volumeid, 
                                    _In_    const FileId                    fileid );
        static VOID ReportFileIdMismatch(   _In_    IFileSystemConfiguration* const pfsconfig,
                                            _In_    IFileFilter* const              pff, 
                                            _In_    const VolumeId                  volumeidExpected,
                                            _In_    const FileId                    fileidExpected,
                                            _In_    const VolumeId                  volumeidActual,
                                            _In_    const FileId                    fileidActual );

        static VOID GenerateUniqueId( _Out_writes_( cbGuid ) BYTE* const rgb );
        static VOID GenerateUniqueId(   _Out_writes_( cbGuid )  BYTE* const                     rgb, 
                                        _Out_                   LittleEndian<FileSerial>* const pserialNumber );

        template< class T >
        static ULONG GenerateChecksum( _In_ const T* const pt );

        template< class T >
        static ERR ErrLoadHeader( _In_ IFileFilter* const pff, _In_ const QWORD ib, _Out_ T** const ppt );

        template< class T >
        static ERR ErrLoadHeader(   _In_reads_( cbHeader )  const BYTE* const   pbHeader,
                                    _In_                    const DWORD         cbHeader,
                                    _Out_                   T** const           ppt );

        template< class T >
        static ERR ErrValidateHeader( _Inout_ T* const pt );

    private:

        template< ChecksumType CT >
        static ULONG GenerateChecksum(  _In_reads_bytes_( cbData )  const VOID* const   pvData,
                                        _In_                        const DWORD         cbData );

        template< class T >
        static ERR ErrRepairHeader( _Inout_ T* const    pt,
                                    _In_    const ULONG ulChecksumExpected,
                                    _In_    const ULONG ulChecksumActual );

        template< ChecksumType CT >
        static ERR ErrRepairHeader( _Inout_updates_bytes_( cbData ) VOID* const pvData,
                                    _In_                            const DWORD cbData,
                                    _In_                            const ULONG ulChecksumExpected,
                                    _In_                            const ULONG ulChecksumActual );

        template< class T >
        static ERR ErrReadHeader( _In_ IFileFilter* const pff, _In_ const QWORD ib, _In_ const T* const pt );

        template< IFileFilter::IOMode IOM >
        static ERR ErrReadHeader(   _In_                            IFileFilter* const  pff, 
                                    _In_                            const QWORD         ib,
                                    _Out_writes_bytes_( cbData )    VOID* const         pvData,
                                    _In_                            const DWORD         cbData );
};

template<typename T>
struct CBlockCacheHeaderTraits
{
    static const CBlockCacheHeaderHelpers::ChecksumType checksumType = CBlockCacheHeaderHelpers::ChecksumType::checksumDefault;
    static const IFileFilter::IOMode ioMode = iomEngine;
};

INLINE ERR CBlockCacheHeaderHelpers::ErrGetFileId(  _In_    IFileFilter* const              pff,
                                                    _Out_   LittleEndian<VolumeId>* const   pvolumeid, 
                                                    _Out_   LittleEndian<FileId>* const     pfileid )
{
    ERR         err         = JET_errSuccess;
    VolumeId    volumeid    = volumeidInvalid;
    FileId      fileid      = fileidInvalid;
    FileSerial  fileserial  = fileserialInvalid;

    *pvolumeid = volumeidInvalid;
    *pfileid = fileidInvalid;

    Call( pff->ErrGetPhysicalId( &volumeid, &fileid, &fileserial ) );

    *pvolumeid = volumeid;
    *pfileid = fileid;

HandleError:
    if ( err < JET_errSuccess )
    {
        *pvolumeid = volumeidInvalid;
        *pfileid = fileidInvalid;
    }
    return err;
}

INLINE ERR CBlockCacheHeaderHelpers::ErrVerifyFileId(   _In_    IFileSystemConfiguration* const pfsconfig, 
                                                        _In_    IFileFilter* const              pff,
                                                        _In_    const VolumeId                  volumeidExpected, 
                                                        _In_    const FileId                    fileidExpected )
{
    ERR         err                 = JET_errSuccess;
    VolumeId    volumeidActual      = volumeidInvalid;
    FileId      fileidActual        = fileidInvalid;
    FileSerial  fileserialActual    = fileserialInvalid;

    Call( pff->ErrGetPhysicalId( &volumeidActual, &fileidActual, &fileserialActual ) );

    if ( volumeidActual != volumeidExpected )
    {
        ReportFileIdMismatch( pfsconfig, pff, volumeidExpected, fileidExpected, volumeidActual, fileidActual );
        Error( ErrERRCheck( JET_errDiskIO ) );
    }
    if ( fileidActual != fileidExpected )
    {
        ReportFileIdMismatch( pfsconfig, pff, volumeidExpected, fileidExpected, volumeidActual, fileidActual );
        Error( ErrERRCheck( JET_errDiskIO ) );
    }

HandleError:
    return err;
}

INLINE VOID CBlockCacheHeaderHelpers::ReportFileIdMismatch( _In_    IFileSystemConfiguration* const pfsconfig, 
                                                            _In_    IFileFilter* const              pff,
                                                            _In_    const VolumeId                  volumeidExpected, 
                                                            _In_    const FileId                    fileidExpected,
                                                            _In_    const VolumeId                  volumeidActual,
                                                            _In_    const FileId                    fileidActual )
{
    const ULONG     cwsz                            = 5;
    const WCHAR*    rgpwsz[ cwsz ]                  = { 0 };
    DWORD           irgpwsz                         = 0;
    WCHAR           wszAbsPath[ OSFSAPI_MAX_PATH ]  = { 0 };
    WCHAR           wszVolumeIdExpected[ 64 ]       = { 0 };
    WCHAR           wszFileIdExpected[ 64 ]         = { 0 };
    WCHAR           wszVolumeIdActual[ 64 ]         = { 0 };
    WCHAR           wszFileIdActual[ 64 ]           = { 0 };

    CallS( pff->ErrPath( wszAbsPath ) );
    OSStrCbFormatW( wszVolumeIdExpected, sizeof( wszVolumeIdExpected ), L"0x%08x", volumeidExpected );
    OSStrCbFormatW( wszFileIdExpected, sizeof( wszFileIdExpected ), L"0x%016I64x", fileidExpected );
    OSStrCbFormatW( wszVolumeIdActual, sizeof( wszVolumeIdActual ), L"0x%08x", volumeidActual );
    OSStrCbFormatW( wszFileIdActual, sizeof( wszFileIdActual ), L"0x%016I64x", fileidActual );

    rgpwsz[ irgpwsz++ ] = wszAbsPath;
    rgpwsz[ irgpwsz++ ] = wszVolumeIdExpected;
    rgpwsz[ irgpwsz++ ] = wszFileIdExpected;
    rgpwsz[ irgpwsz++ ] = wszVolumeIdActual;
    rgpwsz[ irgpwsz++ ] = wszFileIdActual;

    pfsconfig->EmitEvent(   eventError,
                            BLOCK_CACHE_CATEGORY,
                            BLOCK_CACHE_FILE_ID_MISMATCH_ID,
                            irgpwsz,
                            rgpwsz,
                            JET_EventLoggingLevelMin );
}

INLINE VOID CBlockCacheHeaderHelpers::GenerateUniqueId( _Out_writes_( cbGuid ) BYTE* const rgb )
{
    LittleEndian<FileSerial> serialNumber;
    GenerateUniqueId( rgb, &serialNumber );
}

INLINE VOID CBlockCacheHeaderHelpers::GenerateUniqueId( _Out_writes_( cbGuid )  BYTE* const                     rgb,
                                                        _Out_                   LittleEndian<FileSerial>* const pserialNumber )
{
    for ( int ib = 0; ib < CBlockCacheHeaderHelpers::cbGuid; ib += sizeof( unsigned int ) )
    {
        rand_s( (unsigned int*)&rgb[ ib ] );
    }

    for (   *pserialNumber = fileserialInvalid;
            *pserialNumber == fileserialInvalid;
            rand_s( (unsigned int*)pserialNumber ) )
    {
    }
}

template< class T >
INLINE ERR CBlockCacheHeaderHelpers::ErrLoadHeader( _In_    IFileFilter* const  pff,
                                                    _In_    const QWORD         ib,
                                                    _Out_   T** const           ppt )
{
    ERR     err     = JET_errSuccess;
    QWORD   cbSize  = 0;
    void*   pvData  = NULL;
    T*      pt      = NULL;

    *ppt = NULL;

    Call( pff->ErrSize( &cbSize, IFileAPI::filesizeLogical ) );
    if ( ib + cbSize < sizeof( T ) )
    {
        Error( ErrERRCheck( JET_errReadVerifyFailure ) );
    }

    Alloc( pvData = PvOSMemoryPageAlloc( sizeof( T ), NULL ) );
    Call( ErrReadHeader( pff, ib, (T*)pvData ) );
    Call( ErrValidateHeader( (T*)pvData  ) );

    pt = new( pvData ) T();
    pvData = NULL;

    *ppt = pt;
    pt = NULL;

HandleError:
    OSMemoryPageFree( pvData );
    delete pt;
    if ( err < JET_errSuccess )
    {
        delete *ppt;
        *ppt = NULL;
    }
    return err;
}

template< class T >
INLINE ULONG CBlockCacheHeaderHelpers::GenerateChecksum( _In_ const T* const pt )
{
    return GenerateChecksum<CBlockCacheHeaderTraits<T>::checksumType>( (const BYTE*)pt, sizeof( T ) );
}

template< class T >
INLINE ERR CBlockCacheHeaderHelpers::ErrLoadHeader( _In_reads_( cbHeader )  const BYTE* const   pbHeader,
                                                    _In_                    const DWORD         cbHeader, 
                                                    _Out_                   T** const           ppt )
{
    ERR     err     = JET_errSuccess;
    void*   pvData  = NULL;
    T*      pt      = NULL;

    *ppt = NULL;

    Alloc( pvData = PvOSMemoryPageAlloc( sizeof( T ), NULL ) );

    UtilMemCpy( pvData, pbHeader, min( sizeof( T ), cbHeader ) );

    Call( ErrValidateHeader( (T*)pvData ) );

    pt = new( pvData ) T();
    pvData = NULL;

    *ppt = pt;
    pt = NULL;

HandleError:
    OSMemoryPageFree( pvData );
    delete pt;
    if ( err < JET_errSuccess )
    {
        delete *ppt;
        *ppt = NULL;
    }
    return err;
}

template< CBlockCacheHeaderHelpers::ChecksumType CT >
INLINE ULONG CBlockCacheHeaderHelpers::GenerateChecksum(    _In_reads_bytes_( cbData )  const VOID* const   pvData,
                                                            _In_                        const DWORD         cbData )
{
    return ChecksumOldFormat( (const BYTE*)pvData, cbData );
}

template<>
INLINE ULONG CBlockCacheHeaderHelpers::GenerateChecksum<CBlockCacheHeaderHelpers::ChecksumType::checksumECC>(
    _In_reads_bytes_( cbData )  const VOID* const   pvData,
    _In_                        const DWORD         cbData )
{
    return DwECCChecksumFromXEChecksum( ChecksumNewFormat( (const BYTE*)pvData, cbData, 0, fTrue ) );
}

template< class T >
INLINE ERR CBlockCacheHeaderHelpers::ErrValidateHeader( _Inout_ T* const pt )
{
    ERR err = JET_errSuccess;

    ULONG ulChecksumExpected = *((ULONG*)pt);
    ULONG ulChecksumActual = GenerateChecksum( pt );

    if ( ulChecksumActual != ulChecksumExpected )
    {
        Call( ErrRepairHeader( pt, ulChecksumExpected, ulChecksumActual ) );
    }

HandleError:
    return err;
}

template< class T >
INLINE ERR CBlockCacheHeaderHelpers::ErrRepairHeader(   _Inout_ T* const    pt,
                                                        _In_    const ULONG ulChecksumExpected,
                                                        _In_    const ULONG ulChecksumActual )
{
    return ErrRepairHeader<CBlockCacheHeaderTraits<T>::checksumType>(   (BYTE*)pt,
                                                                        sizeof( T ),
                                                                        ulChecksumExpected, 
                                                                        ulChecksumActual);
}

template< CBlockCacheHeaderHelpers::ChecksumType CT >
INLINE ERR CBlockCacheHeaderHelpers::ErrRepairHeader(   _Inout_updates_bytes_( cbData ) VOID* const pvData,
                                                        _In_                            const DWORD cbData,
                                                        _In_                            const ULONG ulChecksumExpected,
                                                        _In_                            const ULONG ulChecksumActual )
{
    ERR err = JET_errSuccess;

    if ( ulChecksumExpected != ulChecksumActual )
    {
        Error( ErrERRCheck( JET_errReadVerifyFailure ) );
    }

HandleError:
    return err;
}

template<>
INLINE ERR CBlockCacheHeaderHelpers::ErrRepairHeader<CBlockCacheHeaderHelpers::ChecksumType::checksumECC>(
    _Inout_updates_bytes_( cbData ) VOID* const pvData,
    _In_                            const DWORD cbData,
    _In_                            const ULONG ulChecksumExpected,
    _In_                            const ULONG ulChecksumActual )
{
    ERR err = JET_errSuccess;

    //  determine if we have a correctable error

    if ( FECCErrorIsCorrectable( cbData, ulChecksumExpected, ulChecksumActual ) )
    {
        //  try to use the ECC to determine the location of a single bit error

        const UINT ibitCorrupted = IbitCorrupted( cbData, ulChecksumExpected, ulChecksumActual );

        //  if we think we found a single bit error then flip it back

        const UINT ib = ibitCorrupted / 8;

        if ( ib < cbData )
        {
            ( (BYTE*)pvData )[ ib ] ^= (BYTE)( 1 << ( ibitCorrupted % 8 ) );
        }
    }

    //  recompute the ECC and fail if we haven't succeeded in fixing the error

    const DWORD ulChecksumCurrent = GenerateChecksum<CBlockCacheHeaderHelpers::ChecksumType::checksumECC>( pvData, cbData );
    Call( ErrRepairHeader<CBlockCacheHeaderHelpers::ChecksumType::checksumDefault>( pvData,
                                                                                    cbData,
                                                                                    ulChecksumExpected,
                                                                                    ulChecksumCurrent ) );

HandleError:
    return err;
}

template< class T >
INLINE ERR CBlockCacheHeaderHelpers::ErrReadHeader( _In_ IFileFilter* const pff,
                                                    _In_ const QWORD        ib, 
                                                    _In_ const T* const     pt )
{
    return ErrReadHeader<CBlockCacheHeaderTraits<T>::ioMode>( pff, ib, (VOID*)pt, sizeof( T ) );
}

template< IFileFilter::IOMode IOM >
INLINE ERR CBlockCacheHeaderHelpers::ErrReadHeader( _In_                            IFileFilter* const  pff, 
                                                    _In_                            const QWORD         ib,
                                                    _Out_writes_bytes_( cbData )    VOID* const         pvData,
                                                    _In_                            const DWORD         cbData )
{
    TraceContextScope tcScope( iorpBlockCache );

    return pff->ErrRead(    *tcScope,
                            ib,
                            cbData,
                            (BYTE*)pvData,
                            qosIONormal,        
                            IOM,
                            NULL,
                            NULL,
                            NULL,
                            NULL );
}
