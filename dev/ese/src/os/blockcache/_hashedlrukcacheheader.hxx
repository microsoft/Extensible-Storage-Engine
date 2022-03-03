// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  Hashed LRUK Caching File Header

//  8a33bc21-ccb0-40ae-bb1c-d231d485eda9
const BYTE c_rgbHashedLRUKCacheHeaderV1[ CBlockCacheHeaderHelpers::cbGuid ] = { 0x21, 0xBC, 0x33, 0x8A, 0xB0, 0xCC, 0xAE, 0x40, 0xBB, 0x1C, 0xD2, 0x31, 0xD4, 0x85, 0xED, 0xA9 };

#pragma push_macro( "new" )
#undef new

#include <pshpack1.h>

//PERSISTED
class CHashedLRUKCacheHeader : CBlockCacheHeaderHelpers  // ch
{
    public:

        static ERR ErrCreate(   _In_    const QWORD                     ibJournal,
                                _In_    const QWORD                     cbJournal,
                                _In_    const QWORD                     ibChunkHash,
                                _In_    const QWORD                     cbChunkHash,
                                _In_    const QWORD                     ibChunkJournal,
                                _In_    const QWORD                     cbChunkJournal,
                                _In_    const QWORD                     ibWriteCounts,
                                _In_    const QWORD                     cbWriteCounts,
                                _In_    const QWORD                     ibClustersHash,
                                _In_    const QWORD                     cbClustersHash,
                                _In_    const QWORD                     ibClustersJournal,
                                _In_    const QWORD                     cbClustersJournal,
                                _In_    const QWORD                     cbCachingFilePerSlab,
                                _In_    const QWORD                     cbCachedFilePerSlab,
                                _In_    const QWORD                     icbwcHash,
                                _In_    const QWORD                     icbwcJournal,
                                _Out_   CHashedLRUKCacheHeader** const  ppch );
        static ERR ErrLoad( _In_    IFileSystemConfiguration* const pfsconfig, 
                            _In_    IFileFilter* const              pff,
                            _In_    const QWORD                     ib,
                            _Out_   CHashedLRUKCacheHeader** const  ppch );
        static ERR ErrDump( _In_ IFileFilter* const pff,
                            _In_ const QWORD        ib,
                            _In_ CPRINTF* const     pcprintf );

        void operator delete( _In_opt_ void* const pv );

        QWORD IbJournal() const { return m_le_ibJournal; }
        QWORD CbJournal() const { return m_le_cbJournal; }
        QWORD IbChunkHash() const { return m_le_ibChunkHash; }
        QWORD CbChunkHash() const { return m_le_cbChunkHash; }
        QWORD IbChunkJournal() const { return m_le_ibChunkJournal; }
        QWORD CbChunkJournal() const { return m_le_cbChunkJournal; }
        QWORD IbWriteCounts() const { return m_le_ibWriteCounts; }
        QWORD CbWriteCounts() const { return m_le_cbWriteCounts; }
        QWORD IbClustersHash() const { return m_le_ibClustersHash; }
        QWORD CbClustersHash() const { return m_le_cbClustersHash; }
        QWORD IbClustersJournal() const { return m_le_ibClustersJournal; }
        QWORD CbClustersJournal() const { return m_le_cbClustersJournal; }
        QWORD CbCachingFilePerSlab() const { return m_le_cbCachingFilePerSlab; }
        QWORD CbCachedFilePerSlab() const { return m_le_cbCachedFilePerSlab; }
        QWORD IcbwcHash() const { return m_le_icbwcHash; }
        QWORD IcbwcJournal() const { return m_le_icbwcJournal; }

        ERR ErrDump( _In_ CPRINTF* const pcprintf );

    private:

        friend class CBlockCacheHeaderHelpers;

        enum { cbCacheHeader = cbBlock };

        CHashedLRUKCacheHeader();

        void* operator new( _In_ const size_t cb );
        void* operator new( _In_ const size_t cb, _In_ const void* const pv );
        
        ERR ErrValidate( _In_ const ERR errInvalidType );

    private:

        LittleEndian<ULONG> m_le_ulChecksum;            //  offset 0:  checksum
        BYTE                m_rgbHeaderType[ cbGuid ];  //  header type
        LittleEndian<QWORD> m_le_ibJournal;             //  CJournalSegmentHeader array offset
        LittleEndian<QWORD> m_le_cbJournal;             //  CJournalSegmentHeader array size
        LittleEndian<QWORD> m_le_ibChunkHash;           //  Hash table CCachedBlockChunk array offset
        LittleEndian<QWORD> m_le_cbChunkHash;           //  Hash table CCachedBlockChunk array size
        LittleEndian<QWORD> m_le_cbChunkJournal;        //  Journal clusters CCachedBlockChunk array size
        LittleEndian<QWORD> m_le_ibChunkJournal;        //  Journal clusters CCachedBlockChunk array offset
        LittleEndian<QWORD> m_le_ibWriteCounts;         //  CCachedBlockWriteCounts array offset
        LittleEndian<QWORD> m_le_cbWriteCounts;         //  CCachedBlockWriteCounts array size
        LittleEndian<QWORD> m_le_ibClustersHash;        //  Hash table Cluster array offset
        LittleEndian<QWORD> m_le_cbClustersHash;        //  Hash table Cluster array size
        LittleEndian<QWORD> m_le_ibClustersJournal;     //  Journal Cluster array offset
        LittleEndian<QWORD> m_le_cbClustersJournal;     //  Journal Cluster array size
        LittleEndian<QWORD> m_le_cbCachingFilePerSlab;  //  The amount of caching file managed per cached block slab
        LittleEndian<QWORD> m_le_cbCachedFilePerSlab;   //  The consecutive bytes of a cached file per cached block slab
        LittleEndian<QWORD> m_le_icbwcHash;             //  Hash table write count base index
        LittleEndian<QWORD> m_le_icbwcJournal;          //  Journal clusters write count base index

        BYTE                m_rgbPadding[   cbCacheHeader 
                                            - sizeof( m_le_ulChecksum )
                                            - sizeof( m_rgbHeaderType )
                                            - sizeof( m_le_ibJournal )
                                            - sizeof( m_le_cbJournal )
                                            - sizeof( m_le_ibChunkHash )
                                            - sizeof( m_le_cbChunkHash )
                                            - sizeof( m_le_ibChunkJournal )
                                            - sizeof( m_le_cbChunkJournal )
                                            - sizeof( m_le_ibWriteCounts )
                                            - sizeof( m_le_cbWriteCounts )
                                            - sizeof( m_le_ibClustersHash )
                                            - sizeof( m_le_cbClustersHash )
                                            - sizeof( m_le_ibClustersJournal )
                                            - sizeof( m_le_cbClustersJournal )
                                            - sizeof( m_le_cbCachingFilePerSlab )
                                            - sizeof( m_le_cbCachedFilePerSlab )
                                            - sizeof( m_le_icbwcHash )
                                            - sizeof( m_le_icbwcJournal ) ];
};

#include <poppack.h>

INLINE CHashedLRUKCacheHeader::CHashedLRUKCacheHeader()
{
    C_ASSERT( 0 == offsetof( CHashedLRUKCacheHeader, m_le_ulChecksum ) );
    C_ASSERT( cbCacheHeader == sizeof( CHashedLRUKCacheHeader ) );
}

INLINE ERR CHashedLRUKCacheHeader::ErrCreate(   _In_    const QWORD                     ibJournal,
                                                _In_    const QWORD                     cbJournal,
                                                _In_    const QWORD                     ibChunkHash,
                                                _In_    const QWORD                     cbChunkHash,
                                                _In_    const QWORD                     ibChunkJournal,
                                                _In_    const QWORD                     cbChunkJournal,
                                                _In_    const QWORD                     ibWriteCounts,
                                                _In_    const QWORD                     cbWriteCounts,
                                                _In_    const QWORD                     ibClustersHash,
                                                _In_    const QWORD                     cbClustersHash,
                                                _In_    const QWORD                     ibClustersJournal,
                                                _In_    const QWORD                     cbClustersJournal,
                                                _In_    const QWORD                     cbCachingFilePerSlab,
                                                _In_    const QWORD                     cbCachedFilePerSlab,
                                                _In_    const QWORD                     icbwcHash,
                                                _In_    const QWORD                     icbwcJournal,
                                                _Out_   CHashedLRUKCacheHeader** const  ppch )
{
    ERR                     err = JET_errSuccess;
    CHashedLRUKCacheHeader* pch = NULL;

    *ppch = NULL;

    Alloc( pch = new CHashedLRUKCacheHeader() );

    UtilMemCpy( pch->m_rgbHeaderType, c_rgbHashedLRUKCacheHeaderV1, cbGuid );

    pch->m_le_ibJournal = ibJournal;
    pch->m_le_cbJournal = cbJournal;
    pch->m_le_ibChunkHash = ibChunkHash;
    pch->m_le_cbChunkHash = cbChunkHash;
    pch->m_le_ibChunkJournal = ibChunkJournal;
    pch->m_le_cbChunkJournal = cbChunkJournal;
    pch->m_le_ibWriteCounts = ibWriteCounts;
    pch->m_le_cbWriteCounts = cbWriteCounts;
    pch->m_le_ibClustersHash = ibClustersHash;
    pch->m_le_cbClustersHash = cbClustersHash;
    pch->m_le_ibClustersJournal = ibClustersJournal;
    pch->m_le_cbClustersJournal = cbClustersJournal;
    pch->m_le_cbCachingFilePerSlab = cbCachingFilePerSlab;
    pch->m_le_cbCachedFilePerSlab = cbCachedFilePerSlab;
    pch->m_le_icbwcHash = icbwcHash;
    pch->m_le_icbwcJournal = icbwcJournal;

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

INLINE ERR CHashedLRUKCacheHeader::ErrLoad( _In_    IFileSystemConfiguration* const pfsconfig,
                                            _In_    IFileFilter* const              pff,
                                            _In_    const QWORD                     ib,
                                            _Out_   CHashedLRUKCacheHeader** const  ppch )
{
    ERR                     err = JET_errSuccess;
    CHashedLRUKCacheHeader* pch = NULL;

    *ppch = NULL;

    Call( ErrLoadHeader( pff, ib, &pch ) );

    Call( pch->ErrValidate( JET_errReadVerifyFailure ) );

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

INLINE ERR CHashedLRUKCacheHeader::ErrDump( _In_ IFileFilter* const pff,
                                            _In_ const QWORD        ib,
                                            _In_ CPRINTF* const     pcprintf )
{
    ERR                     err = JET_errSuccess;
    CHashedLRUKCacheHeader* pch = NULL;

    Call( ErrLoadHeader( pff, ib, &pch ) );

    Call( pch->ErrValidate( JET_errFileInvalidType ) );

    Call( pch->ErrDump( pcprintf ) );

HandleError:
    delete pch;
    return err;
}

INLINE void CHashedLRUKCacheHeader::operator delete( _In_opt_ void* const pv )
{
    OSMemoryPageFree( pv );
}

INLINE ERR CHashedLRUKCacheHeader::ErrDump( _In_ CPRINTF* const pcprintf )
{
    (*pcprintf)(    "Hashed LRUK Cache Header:\n" );
    (*pcprintf)(    "\n" );
    (*pcprintf)(    "                                            Checksum:  0x%08lx\n", LONG( m_le_ulChecksum ) );
    (*pcprintf)(    "                                                Type:  %08lx-%04hx-%04hx-%02hx%02hx-%02hx%02hx%02hx%02hx%02hx%02hx\n",
                    *((DWORD*)&m_rgbHeaderType[ 0 ]),
                    *((WORD*)&m_rgbHeaderType[ 4 ]),
                    *((WORD*)&m_rgbHeaderType[ 6 ]),
                    *((BYTE*)&m_rgbHeaderType[ 8 ]),
                    *((BYTE*)&m_rgbHeaderType[ 9 ]),
                    *((BYTE*)&m_rgbHeaderType[ 10 ]),
                    *((BYTE*)&m_rgbHeaderType[ 11 ]),
                    *((BYTE*)&m_rgbHeaderType[ 12 ]),
                    *((BYTE*)&m_rgbHeaderType[ 13 ]),
                    *((BYTE*)&m_rgbHeaderType[ 14 ]),
                    *((BYTE*)&m_rgbHeaderType[ 15 ]) );
    (*pcprintf)(    "                 Journal Segment Header Array Offset:  0x%016I64x\n", QWORD( m_le_ibJournal ) );
    (*pcprintf)(    "                   Journal Segment Header Array Size:  0x%016I64x\n", QWORD( m_le_cbJournal ) );
    (*pcprintf)(    "          Hash Table Cached Block Chunk Array Offset:  0x%016I64x\n", QWORD( m_le_ibChunkHash ) );
    (*pcprintf)(    "            Hash Table Cached Block Chunk Array Size:  0x%016I64x\n", QWORD( m_le_cbChunkHash ) );
    (*pcprintf)(    "    Journal Clusters Cached Block Chunk Array Offset:  0x%016I64x\n", QWORD( m_le_ibChunkJournal ) );
    (*pcprintf)(    "      Journal Clusters Cached Block Chunk Array Size:  0x%016I64x\n", QWORD( m_le_cbChunkJournal ) );
    (*pcprintf)(    "              Cached Block Write Counts Array Offset:  0x%016I64x\n", QWORD( m_le_ibWriteCounts ) );
    (*pcprintf)(    "                Cached Block Write Counts Array Size:  0x%016I64x\n", QWORD( m_le_cbWriteCounts ) );
    (*pcprintf)(    "                     Hash Table Cluster Array Offset:  0x%016I64x\n", QWORD( m_le_ibClustersHash ) );
    (*pcprintf)(    "                       Hash Table Cluster Array Size:  0x%016I64x\n", QWORD( m_le_cbClustersHash ) );
    (*pcprintf)(    "                        Journal Cluster Array Offset:  0x%016I64x\n", QWORD( m_le_ibClustersJournal ) );
    (*pcprintf)(    "                          Journal Cluster Array Size:  0x%016I64x\n", QWORD( m_le_cbClustersJournal ) );
    (*pcprintf)(    "                         Caching File Bytes per Slab:  0x%016I64x\n", QWORD( m_le_cbCachingFilePerSlab ) );
    (*pcprintf)(    "              Consecutive Cached File Bytes per Slab:  0x%016I64x\n", QWORD( m_le_cbCachedFilePerSlab ) );
    (*pcprintf)(    "                   Hash Table Write Count Base Index:  0x%016I64x\n", QWORD( m_le_icbwcHash) );
    (*pcprintf)(    "             Journal Clusters Write Count Base Index:  0x%016I64x\n", QWORD( m_le_icbwcJournal ) );
    (*pcprintf)(    "\n" );

    return JET_errSuccess;
}

INLINE void* CHashedLRUKCacheHeader::operator new( _In_ const size_t cb )
{
#ifdef MEM_CHECK
    return PvOSMemoryPageAlloc_( cb, NULL, fFalse, SzNewFile(), UlNewLine() );
#else  //  !MEM_CHECK
    return PvOSMemoryPageAlloc( cb, NULL );
#endif  //  MEM_CHECK
}

INLINE void* CHashedLRUKCacheHeader::operator new( _In_ const size_t cb, _In_ const void* const pv )
{
    return (void*)pv;
}

#pragma pop_macro( "new" )

INLINE ERR CHashedLRUKCacheHeader::ErrValidate( _In_ const ERR errInvalidType )
{
    ERR err = JET_errSuccess;

    if ( memcmp( m_rgbHeaderType, c_rgbHashedLRUKCacheHeaderV1, cbGuid ) )
    {
        Error( ErrERRCheck( errInvalidType ) );
    }

HandleError:
    return err;
}
