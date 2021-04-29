// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  Cached File Header

//  bd07ebca-290c-46bb-8bc8-99041dfe21c6
const BYTE c_rgbCachedFileHeaderV1[ CBlockCacheHeaderHelpers::cbGuid ] = { 0xCA, 0xEB, 0x07, 0xBD, 0x0C, 0x29, 0xBB, 0x46, 0x8B, 0xC8, 0x99, 0x04, 0x1D, 0xFE, 0x21, 0xC6 };

#pragma push_macro( "new" )
#undef new

#include <pshpack1.h>

//PERSISTED
class CCachedFileHeader : CBlockCacheHeaderHelpers  //  cfh
{
    public:

        static ERR ErrCreate(   _In_    IFileFilter* const          pffCachedFile,
                                _In_    ICache* const               pc,
                                _Out_   CCachedFileHeader** const   ppcfh );
        static ERR ErrLoad( _In_    IFileSystemConfiguration* const pfsconfig, 
                            _In_    IFileFilter* const              pff,
                            _Out_   CCachedFileHeader** const       ppcfh );
        static ERR ErrLoad( _In_                    IFileSystemConfiguration* const pfsconfig, 
                            _In_reads_( cbHeader )  const BYTE* const               pbHeader, 
                            _In_                    const DWORD                     cbHeader,
                            _Out_                   CCachedFileHeader** const       ppcfh );
        static BOOL FValid( _In_                    IFileSystemConfiguration* const pfsconfig, 
                            _In_reads_( cbHeader )  const BYTE* const               pbHeader,
                            _In_                    const DWORD                     cbHeader );
        static ERR ErrDump( _In_ IFileSystemConfiguration* const    pfsconfig,
                            _In_ IFileIdentification* const         pfident, 
                            _In_ IFileFilter* const                 pff,
                            _In_ CPRINTF* const                     pcprintf );

        void operator delete( _In_ void* const pv );

        FileSerial Fileserial() const { return m_le_serialNumber; }

        VolumeId VolumeidCache() const { return m_le_volumeidCache; }
        FileId FileidCache() const { return m_le_fileidCache; }
        const BYTE* RgbUniqueIdCache() const { return m_rgbUniqueIdCache; }

    private:

        friend class CBlockCacheHeaderHelpers;

        enum { cbCachedFileHeader = cbBlock };

        CCachedFileHeader();

        void* operator new( _In_ const size_t cb );
        void* operator new( _In_ const size_t cb, _In_ const void* const pv );

        ERR ErrValidate(    _In_ const ERR                          errInvalidType,
                            _In_ IFileSystemConfiguration* const    pfsconfig, 
                            _In_ IFileFilter* const                 pff             = NULL );

    private:

        LittleEndian<ULONG>             m_le_ulChecksum;                //  offset 0:  checksum
        BYTE                            m_rgbHeaderType[ cbGuid ];      //  header type
        LittleEndian<VolumeId>          m_le_volumeid;                  //  volume id when last attached to the caching file
        LittleEndian<FileId>            m_le_fileid;                    //  file id when last attached to the caching file
        BYTE                            m_rgbUniqueId[ cbGuid ];        //  unique id
        LittleEndian<FileSerial>        m_le_serialNumber;              //  serial number
        LittleEndian<VolumeId>          m_le_volumeidCache;             //  caching file's volume id
        LittleEndian<FileId>            m_le_fileidCache;               //  caching file's file id
        BYTE                            m_rgbUniqueIdCache[ cbGuid ];   //  caching file's unique id

        BYTE                            m_rgbPadding1[  ibFileType
                                                        - sizeof( m_le_ulChecksum )
                                                        - sizeof( m_rgbHeaderType )
                                                        - sizeof( m_le_volumeid )
                                                        - sizeof( m_le_fileid )
                                                        - sizeof( m_rgbUniqueId )
                                                        - sizeof( m_le_serialNumber )
                                                        - sizeof( m_le_volumeidCache )
                                                        - sizeof( m_le_fileidCache )
                                                        - sizeof( m_rgbUniqueIdCache ) ];

        UnalignedLittleEndian<ULONG>    m_le_filetype;                                      //  offset 667:  file type = JET_filetypeCachedFile

        BYTE                            m_rgbPadding2[  cbCachedFileHeader
                                                        - ibFileType
                                                        - sizeof( m_le_filetype ) ];
};

#include <poppack.h>

template<>
struct CBlockCacheHeaderTraits<CCachedFileHeader>
{
    static const CBlockCacheHeaderHelpers::ChecksumType checksumType = CBlockCacheHeaderHelpers::ChecksumType::checksumDefault;
    static const IFileFilter::IOMode ioMode = iomRaw;
};

INLINE CCachedFileHeader::CCachedFileHeader()
{
    C_ASSERT( 0 == offsetof( CCachedFileHeader, m_le_ulChecksum ) );
    C_ASSERT( ibFileType == offsetof( CCachedFileHeader, m_le_filetype ) );
    C_ASSERT( cbCachedFileHeader == sizeof( CCachedFileHeader ) );
}

INLINE ERR CCachedFileHeader::ErrCreate(    _In_    IFileFilter* const          pffCachedFile,
                                            _In_    ICache* const               pc,
                                            _Out_   CCachedFileHeader** const   ppcfh )
{
    ERR                 err                     = JET_errSuccess;
    CCachedFileHeader*  pcfh                    = NULL;
    VolumeId            volumeid                = volumeidInvalid;
    FileId              fileid                  = fileidInvalid;
    BYTE                rgbUniqueId[ cbGuid ]   = { 0 };

    *ppcfh = NULL;

    Alloc( pcfh = new CCachedFileHeader() );

    UtilMemCpy( pcfh->m_rgbHeaderType, c_rgbCachedFileHeaderV1, cbGuid );
    Call( ErrGetFileId( pffCachedFile, &pcfh->m_le_volumeid, &pcfh->m_le_fileid ) );
    GenerateUniqueId( pcfh->m_rgbUniqueId, &pcfh->m_le_serialNumber );
    Call( pc->ErrGetPhysicalId( &volumeid, &fileid, rgbUniqueId ) );
    pcfh->m_le_volumeidCache = volumeid;
    pcfh->m_le_fileidCache = fileid;
    UtilMemCpy( pcfh->m_rgbUniqueIdCache, rgbUniqueId, cbGuid );
    pcfh->m_le_filetype = JET_filetypeCachedFile;

    pcfh->m_le_ulChecksum = GenerateChecksum( pcfh );

    Assert( pcfh->m_le_volumeid != pcfh->m_le_volumeidCache || pcfh->m_le_fileid != pcfh->m_le_fileidCache );

    *ppcfh = pcfh;
    pcfh = NULL;

HandleError:
    delete pcfh;
    if ( err < JET_errSuccess )
    {
        delete *ppcfh;
        *ppcfh = NULL;
    }
    return err;
}

INLINE ERR CCachedFileHeader::ErrLoad(  _In_    IFileSystemConfiguration* const pfsconfig, 
                                        _In_    IFileFilter* const              pff,
                                        _Out_   CCachedFileHeader** const       ppcfh )
{
    ERR                 err     = JET_errSuccess;
    CCachedFileHeader*  pcfh    = NULL;

    *ppcfh = NULL;

    Call( ErrLoadHeader( pff, 0, &pcfh ) );

    Call( pcfh->ErrValidate( JET_errReadVerifyFailure, pfsconfig, pff ) );

    *ppcfh = pcfh;
    pcfh = NULL;

HandleError:
    delete pcfh;
    if ( err < JET_errSuccess )
    {
        delete *ppcfh;
        *ppcfh = NULL;
    }
    return err;
}

INLINE ERR CCachedFileHeader::ErrLoad(  _In_                    IFileSystemConfiguration* const pfsconfig, 
                                        _In_reads_( cbHeader )  const BYTE* const               pbHeader, 
                                        _In_                    const DWORD                     cbHeader,
                                        _Out_                   CCachedFileHeader** const       ppcfh )
{
    ERR                 err     = JET_errSuccess;
    CCachedFileHeader*  pcfh    = NULL;

    *ppcfh = NULL;

    Call( ErrLoadHeader( pbHeader, cbHeader, &pcfh ) );

    Call( pcfh->ErrValidate( JET_errReadVerifyFailure, pfsconfig ) );

    *ppcfh = pcfh;
    pcfh = NULL;

HandleError:
    delete pcfh;
    if ( err < JET_errSuccess )
    {
        delete *ppcfh;
        *ppcfh = NULL;
    }
    return err;
}

INLINE BOOL CCachedFileHeader::FValid(  _In_                    IFileSystemConfiguration* const pfsconfig, 
                                        _In_reads_( cbHeader )  const BYTE* const               pbHeader,
                                        _In_                    const DWORD                     cbHeader )
{
    ERR                 err                     = JET_errSuccess;
    BOOL                fCleanUpStateSaved      = fFalse;
    BOOL                fRestoreCleanupState    = fFalse;
    CCachedFileHeader*  pcfh                    = NULL;
    BOOL                fValid                  = fFalse;

    fCleanUpStateSaved = FOSSetCleanupState( fFalse );
    fRestoreCleanupState = fTrue;

    Call( CCachedFileHeader::ErrLoad( pfsconfig, pbHeader, cbHeader, &pcfh ) );

    fValid = fTrue;

HandleError:
    if ( fRestoreCleanupState )
    {
        FOSSetCleanupState( fCleanUpStateSaved );
    }
    delete pcfh;
    return fValid;
}

INLINE ERR CCachedFileHeader::ErrDump(  _In_ IFileSystemConfiguration* const    pfsconfig,
                                        _In_ IFileIdentification* const         pfident, 
                                        _In_ IFileFilter* const                 pff,
                                        _In_ CPRINTF* const                     pcprintf )
{
    ERR                 err                                 = JET_errSuccess;
    CCachedFileHeader*  pcfh                                = NULL;
    const DWORD         cwchAnyAbsPathMax                   = OSFSAPI_MAX_PATH;
    WCHAR               wszAnyAbsPath[ cwchAnyAbsPathMax ]  = { 0 };
    const DWORD         cwchKeyPathMax                      = OSFSAPI_MAX_PATH;
    WCHAR               wszKeyPath[ cwchKeyPathMax ]        = { 0 };
    WCHAR               wszAbsPath[ OSFSAPI_MAX_PATH ]      = { 0 };
    VolumeId            volumeid                            = volumeidInvalid;
    FileId              fileid                              = fileidInvalid;
    FileSerial          fileserial                          = fileserialInvalid;

    Call( ErrLoadHeader( pff, 0, &pcfh ) );

    Call( pcfh->ErrValidate( JET_errFileInvalidType, pfsconfig ) );

    (void)pfident->ErrGetFilePathById( pcfh->m_le_volumeidCache, pcfh->m_le_fileidCache, wszAnyAbsPath, wszKeyPath );

    CallS( pff->ErrPath( wszAbsPath ) );
    CallS( pff->ErrGetPhysicalId( &volumeid, &fileid, &fileserial ) );

    (*pcprintf)(    "CACHED FILE HEADER:\n" );
    (*pcprintf)(    "\n" );
    (*pcprintf)(    "Fields:\n" );
    (*pcprintf)(    "          Checksum:  0x%08lx\n", LONG( pcfh->m_le_ulChecksum ) );
    (*pcprintf)(    "              Type:  %08lx-%04hx-%04hx-%02hx%02hx-%02hx%02hx%02hx%02hx%02hx%02hx\n",
                    *((DWORD*)&pcfh->m_rgbHeaderType[ 0 ]),
                    *((WORD*)&pcfh->m_rgbHeaderType[ 4 ]),
                    *((WORD*)&pcfh->m_rgbHeaderType[ 6 ]),
                    *((BYTE*)&pcfh->m_rgbHeaderType[ 8 ]),
                    *((BYTE*)&pcfh->m_rgbHeaderType[ 9 ]),
                    *((BYTE*)&pcfh->m_rgbHeaderType[ 10 ]),
                    *((BYTE*)&pcfh->m_rgbHeaderType[ 11 ]),
                    *((BYTE*)&pcfh->m_rgbHeaderType[ 12 ]),
                    *((BYTE*)&pcfh->m_rgbHeaderType[ 13 ]),
                    *((BYTE*)&pcfh->m_rgbHeaderType[ 14 ]),
                    *((BYTE*)&pcfh->m_rgbHeaderType[ 15 ]) );
    (*pcprintf)(    "          VolumeId:  0x%08x\n", DWORD( VolumeId( pcfh->m_le_volumeid ) ) );
    (*pcprintf)(    "            FileId:  0x%016I64x\n", QWORD( FileId( pcfh->m_le_fileid ) ) );
    (*pcprintf)(    "          UniqueId:  %08lx-%04hx-%04hx-%02hx%02hx-%02hx%02hx%02hx%02hx%02hx%02hx\n",
                    *((DWORD*)&pcfh->m_rgbUniqueId[ 0 ]),
                    *((WORD*)&pcfh->m_rgbUniqueId[ 4 ]),
                    *((WORD*)&pcfh->m_rgbUniqueId[ 6 ]),
                    *((BYTE*)&pcfh->m_rgbUniqueId[ 8 ]),
                    *((BYTE*)&pcfh->m_rgbUniqueId[ 9 ]),
                    *((BYTE*)&pcfh->m_rgbUniqueId[ 10 ]),
                    *((BYTE*)&pcfh->m_rgbUniqueId[ 11 ]),
                    *((BYTE*)&pcfh->m_rgbUniqueId[ 12 ]),
                    *((BYTE*)&pcfh->m_rgbUniqueId[ 13 ]),
                    *((BYTE*)&pcfh->m_rgbUniqueId[ 14 ]),
                    *((BYTE*)&pcfh->m_rgbUniqueId[ 15 ]) );
    (*pcprintf)(    "        FileSerial:  0x%08x\n", DWORD( FileSerial( pcfh->m_le_serialNumber ) ) );
    (*pcprintf)(    "    Cache VolumeId:  0x%08x\n", DWORD( VolumeId( pcfh->m_le_volumeidCache ) ) );
    (*pcprintf)(    "      Cache FileId:  0x%016I64x\n", QWORD( FileId( pcfh->m_le_fileidCache ) ) );
    (*pcprintf)(    "        Cache Path:  %S\n", wszAnyAbsPath[0] ? wszAnyAbsPath : L"???" );
    (*pcprintf)(    "    Cache UniqueId:  %08lx-%04hx-%04hx-%02hx%02hx-%02hx%02hx%02hx%02hx%02hx%02hx\n",
                    *((DWORD*)&pcfh->m_rgbUniqueIdCache[ 0 ]),
                    *((WORD*)&pcfh->m_rgbUniqueIdCache[ 4 ]),
                    *((WORD*)&pcfh->m_rgbUniqueIdCache[ 6 ]),
                    *((BYTE*)&pcfh->m_rgbUniqueIdCache[ 8 ]),
                    *((BYTE*)&pcfh->m_rgbUniqueIdCache[ 9 ]),
                    *((BYTE*)&pcfh->m_rgbUniqueIdCache[ 10 ]),
                    *((BYTE*)&pcfh->m_rgbUniqueIdCache[ 11 ]),
                    *((BYTE*)&pcfh->m_rgbUniqueIdCache[ 12 ]),
                    *((BYTE*)&pcfh->m_rgbUniqueIdCache[ 13 ]),
                    *((BYTE*)&pcfh->m_rgbUniqueIdCache[ 14 ]),
                    *((BYTE*)&pcfh->m_rgbUniqueIdCache[ 15 ]) );
    (*pcprintf)(    "           File Type:  %d\n", LONG( pcfh->m_le_filetype ) );
    (*pcprintf)(    "\n" );
    (*pcprintf)(    "Current File Properties:\n" );
    (*pcprintf)(    "              Path:  %S\n", wszAbsPath );
    (*pcprintf)(    "          VolumeId:  0x%08x\n", DWORD( volumeid ) );
    (*pcprintf)(    "            FileId:  0x%016I64x\n", QWORD( fileid ) );
    (*pcprintf)(    "\n" );

HandleError:
    delete pcfh;
    return err;
}

INLINE void* CCachedFileHeader::operator new( _In_ const size_t cb )
{
#ifdef MEM_CHECK
    return PvOSMemoryPageAlloc_( cb, NULL, fFalse, SzNewFile(), UlNewLine() );
#else  //  !MEM_CHECK
    return PvOSMemoryPageAlloc( cb, NULL );
#endif  //  MEM_CHECK
}

INLINE void* CCachedFileHeader::operator new( _In_ const size_t cb, _In_ const void* const pv )
{
    return (void*)pv;
}

INLINE void CCachedFileHeader::operator delete( _In_ void* const pv )
{
    OSMemoryPageFree( pv );
}

INLINE ERR CCachedFileHeader::ErrValidate(  _In_ const ERR                          errInvalidType,
                                            _In_ IFileSystemConfiguration* const    pfsconfig, 
                                            _In_ IFileFilter* const                 pff )
{
    ERR err = JET_errSuccess;

    if ( m_le_filetype != JET_filetypeCachedFile )
    {
        Call( ErrERRCheck( errInvalidType ) );
    }

    if ( memcmp( m_rgbHeaderType, c_rgbCachedFileHeaderV1, cbGuid ) )
    {
        Call( ErrERRCheck( errInvalidType ) );
    }

    if ( pff )
    {
        Call( ErrVerifyFileId( pfsconfig, pff, m_le_volumeid, m_le_fileid ) );
    }

HandleError:
    return err;
}

#pragma pop_macro( "new" )
