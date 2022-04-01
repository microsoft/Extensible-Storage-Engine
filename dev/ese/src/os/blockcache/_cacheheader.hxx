// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  Caching File Header

//  a6ea21e4-7ee7-4abe-aa30-f8ce919253d1
const BYTE c_rgbCacheHeaderV1[ CBlockCacheHeaderHelpers::cbGuid ] = { 0xE4, 0x21, 0xEA, 0xA6, 0xE7, 0x7E, 0xBE, 0x4A, 0xAA, 0x30, 0xF8, 0xCE, 0x91, 0x92, 0x53, 0xD1 };

#pragma push_macro( "new" )
#undef new

#include <pshpack1.h>

//PERSISTED
class CCacheHeader : CBlockCacheHeaderHelpers  // ch
{
    public:

        static ERR ErrCreate(   _In_reads_( cbGuid )    const BYTE* const       rgbCacheType,
                                _In_                    IFileFilter* const      pff,
                                _Out_                   CCacheHeader** const    ppch );
        static ERR ErrLoad( _In_    IFileSystemConfiguration* const pfsconfig, 
                            _In_    IFileFilter* const              pff,
                            _Out_   CCacheHeader** const            ppch );
        static ERR ErrDump( _In_ IFileSystemConfiguration* const    pfsconfig,
                            _In_ IFileIdentification* const         pfident,
                            _In_ IFileFilter* const                 pff,
                            _In_ CPRINTF* const                     pcprintf );

        void operator delete( _In_opt_ void* const pv );

        VolumeId Volumeid() const { return m_le_volumeid; }
        FileId Fileid() const { return m_le_fileid; }
        const BYTE* const RgbUniqueId() const { return m_rgbUniqueId; }
        const BYTE* const RgbCacheType() const { return m_rgbCacheType; }

    private:

        friend class CBlockCacheHeaderHelpers;

        enum { cbCacheHeader = cbBlock };

        CCacheHeader();

        void* operator new( _In_ const size_t cb );
        void* operator new( _In_ const size_t cb, _In_ const void* const pv );
        
        ERR ErrValidate(    _In_        const ERR                          errInvalidType,
                            _In_        IFileSystemConfiguration* const    pfsconfig, 
                            _In_opt_    IFileFilter* const                 pff             = NULL );

    private:

        LittleEndian<ULONG>             m_le_ulChecksum;            //  offset 0:  checksum
        BYTE                            m_rgbHeaderType[ cbGuid ];  //  header type
        LittleEndian<VolumeId>          m_le_volumeid;              //  volume id when created
        LittleEndian<FileId>            m_le_fileid;                //  file id when created
        BYTE                            m_rgbUniqueId[ cbGuid ];    //  unique id
        BYTE                            m_rgbCacheType[ cbGuid ];   //  cache type

        BYTE                            m_rgbPadding1[  ibFileType
                                                        - sizeof( m_le_ulChecksum )
                                                        - sizeof( m_rgbHeaderType )
                                                        - sizeof( m_le_volumeid )
                                                        - sizeof( m_le_fileid )
                                                        - sizeof( m_rgbUniqueId )
                                                        - sizeof( m_rgbCacheType ) ];

        UnalignedLittleEndian<ULONG>    m_le_filetype;                                  //  offset 667:  file type = JET_filetypeCachingFile

        BYTE                            m_rgbPadding2[  cbCacheHeader 
                                                        - ibFileType
                                                        - sizeof( m_le_filetype ) ];
};

#include <poppack.h>

INLINE CCacheHeader::CCacheHeader()
{
    C_ASSERT( 0 == offsetof( CCacheHeader, m_le_ulChecksum ) );
    C_ASSERT( ibFileType == offsetof( CCacheHeader, m_le_filetype ) );
    C_ASSERT( cbCacheHeader == sizeof( CCacheHeader ) );
}

INLINE ERR CCacheHeader::ErrCreate( _In_reads_( cbGuid )    const BYTE* const       rgbCacheType,
                                    _In_                    IFileFilter* const      pff,
                                    _Out_                   CCacheHeader** const    ppch )
{
    ERR             err = JET_errSuccess;
    CCacheHeader*   pch = NULL;

    *ppch = NULL;

    Alloc( pch = new CCacheHeader() );

    UtilMemCpy( pch->m_rgbHeaderType, c_rgbCacheHeaderV1, cbGuid );
    Call( ErrGetFileId( pff, &pch->m_le_volumeid, &pch->m_le_fileid ) );
    GenerateUniqueId( pch->m_rgbUniqueId );
    pch->m_le_filetype = JET_filetypeCachingFile;
    UtilMemCpy( pch->m_rgbCacheType, rgbCacheType, cbGuid );

    pch->m_le_ulChecksum = GenerateChecksum( pch );

    *ppch = pch;
    pch = NULL;

HandleError:
    delete pch;
    if ( err < JET_errSuccess )
    {
        delete *ppch;
        *ppch = NULL;
    }
    return err;
}

INLINE ERR CCacheHeader::ErrLoad(   _In_    IFileSystemConfiguration* const pfsconfig, 
                                    _In_    IFileFilter* const              pff,
                                    _Out_   CCacheHeader** const            ppch )
{
    ERR             err = JET_errSuccess;
    CCacheHeader*   pch = NULL;

    *ppch = NULL;

    Call( ErrLoadHeader( pff, 0, &pch ) );

    Call( pch->ErrValidate( JET_errReadVerifyFailure, pfsconfig, pff ) );

    *ppch = pch;
    pch = NULL;

HandleError:
    delete pch;
    if ( err < JET_errSuccess )
    {
        delete *ppch;
        *ppch = NULL;
    }
    return err;
}

INLINE ERR CCacheHeader::ErrDump(   _In_ IFileSystemConfiguration* const    pfsconfig,
                                    _In_ IFileIdentification* const         pfident, 
                                    _In_ IFileFilter* const                 pff,
                                    _In_ CPRINTF* const                     pcprintf )
{
    ERR             err                             = JET_errSuccess;
    CCacheHeader*   pch                             = NULL;
    const DWORD     cwchAbsPathMax                  = IFileSystemAPI::cchPathMax;
    WCHAR           wszAbsPath[ cwchAbsPathMax ]    = { 0 };
    VolumeId        volumeid                        = volumeidInvalid;
    FileId          fileid                          = fileidInvalid;
    FileSerial      fileserial                      = fileserialInvalid;

    Call( ErrLoadHeader( pff, 0, &pch ) );

    Call( pch->ErrValidate( JET_errFileInvalidType, pfsconfig ) );

    CallS( pff->ErrPath( wszAbsPath ) );
    CallS( pff->ErrGetPhysicalId( &volumeid, &fileid, &fileserial ) );

    (*pcprintf)(    "Cache Header:\n" );
    (*pcprintf)(    "\n" );
    (*pcprintf)(    "             Checksum:  0x%08lx\n", LONG( pch->m_le_ulChecksum ) );
    (*pcprintf)(    "                 Type:  %08lx-%04hx-%04hx-%02hx%02hx-%02hx%02hx%02hx%02hx%02hx%02hx\n",
                    *((DWORD*)&pch->m_rgbHeaderType[ 0 ]),
                    *((WORD*)&pch->m_rgbHeaderType[ 4 ]),
                    *((WORD*)&pch->m_rgbHeaderType[ 6 ]),
                    *((BYTE*)&pch->m_rgbHeaderType[ 8 ]),
                    *((BYTE*)&pch->m_rgbHeaderType[ 9 ]),
                    *((BYTE*)&pch->m_rgbHeaderType[ 10 ]),
                    *((BYTE*)&pch->m_rgbHeaderType[ 11 ]),
                    *((BYTE*)&pch->m_rgbHeaderType[ 12 ]),
                    *((BYTE*)&pch->m_rgbHeaderType[ 13 ]),
                    *((BYTE*)&pch->m_rgbHeaderType[ 14 ]),
                    *((BYTE*)&pch->m_rgbHeaderType[ 15 ]) );
    (*pcprintf)(    "             VolumeId:  0x%08x\n", DWORD( VolumeId( pch->m_le_volumeid ) ) );
    (*pcprintf)(    "               FileId:  0x%016I64x\n", QWORD( FileId( pch->m_le_fileid ) ) );
    (*pcprintf)(    "             UniqueId:  %08lx-%04hx-%04hx-%02hx%02hx-%02hx%02hx%02hx%02hx%02hx%02hx\n",
                    *((DWORD*)&pch->m_rgbUniqueId[ 0 ]),
                    *((WORD*)&pch->m_rgbUniqueId[ 4 ]),
                    *((WORD*)&pch->m_rgbUniqueId[ 6 ]),
                    *((BYTE*)&pch->m_rgbUniqueId[ 8 ]),
                    *((BYTE*)&pch->m_rgbUniqueId[ 9 ]),
                    *((BYTE*)&pch->m_rgbUniqueId[ 10 ]),
                    *((BYTE*)&pch->m_rgbUniqueId[ 11 ]),
                    *((BYTE*)&pch->m_rgbUniqueId[ 12 ]),
                    *((BYTE*)&pch->m_rgbUniqueId[ 13 ]),
                    *((BYTE*)&pch->m_rgbUniqueId[ 14 ]),
                    *((BYTE*)&pch->m_rgbUniqueId[ 15 ]) );
    (*pcprintf)(    "           Cache Type:  %08lx-%04hx-%04hx-%02hx%02hx-%02hx%02hx%02hx%02hx%02hx%02hx\n",
                    *((DWORD*)&pch->m_rgbCacheType[ 0 ]),
                    *((WORD*)&pch->m_rgbCacheType[ 4 ]),
                    *((WORD*)&pch->m_rgbCacheType[ 6 ]),
                    *((BYTE*)&pch->m_rgbCacheType[ 8 ]),
                    *((BYTE*)&pch->m_rgbCacheType[ 9 ]),
                    *((BYTE*)&pch->m_rgbCacheType[ 10 ]),
                    *((BYTE*)&pch->m_rgbCacheType[ 11 ]),
                    *((BYTE*)&pch->m_rgbCacheType[ 12 ]),
                    *((BYTE*)&pch->m_rgbCacheType[ 13 ]),
                    *((BYTE*)&pch->m_rgbCacheType[ 14 ]),
                    *((BYTE*)&pch->m_rgbCacheType[ 15 ]) );
    (*pcprintf)(    "            File Type:  %d\n", LONG( pch->m_le_filetype ) );
    (*pcprintf)(    "\n" );
    (*pcprintf)(    "Current File Properties:\n" );
    (*pcprintf)(    "                 Path:  %ws\n", wszAbsPath );
    (*pcprintf)(    "             VolumeId:  0x%08x\n", DWORD( volumeid ) );
    (*pcprintf)(    "               FileId:  0x%016I64x\n", QWORD( fileid ) );
    (*pcprintf)(    "\n" );


HandleError:
    delete pch;
    return err;
}

INLINE void* CCacheHeader::operator new( _In_ const size_t cb )
{
#ifdef MEM_CHECK
    return PvOSMemoryPageAlloc_( cb, NULL, fFalse, SzNewFile(), UlNewLine() );
#else  //  !MEM_CHECK
    return PvOSMemoryPageAlloc( cb, NULL );
#endif  //  MEM_CHECK
}

INLINE void* CCacheHeader::operator new( _In_ const size_t cb, _In_ const void* const pv )
{
    return (void*)pv;
}

INLINE void CCacheHeader::operator delete( _In_opt_ void* const pv )
{
    OSMemoryPageFree( pv );
}

#pragma pop_macro( "new" )

INLINE ERR CCacheHeader::ErrValidate(   _In_        const ERR                          errInvalidType,
                                        _In_        IFileSystemConfiguration* const    pfsconfig, 
                                        _In_opt_    IFileFilter* const                 pff )
{
    ERR err = JET_errSuccess;


    if ( m_le_filetype != JET_filetypeCachingFile )
    {
        Error( ErrERRCheck( errInvalidType ) );
    }

    if ( memcmp( m_rgbHeaderType, c_rgbCacheHeaderV1, cbGuid ) )
    {
        Error( ErrERRCheck( errInvalidType ) );
    }

    if ( pff )
    {
        Call( ErrVerifyFileId( pfsconfig, pff, m_le_volumeid, m_le_fileid ) );
    }

HandleError:
    return err;
}
