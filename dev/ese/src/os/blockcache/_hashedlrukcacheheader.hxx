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

        static ERR ErrCreate( _Out_ CHashedLRUKCacheHeader** const ppch );
        static ERR ErrLoad( _In_    IFileSystemConfiguration* const pfsconfig,
                            _In_    IFileFilter* const              pff,
                            _In_    const QWORD                     ib,
                            _Out_   CHashedLRUKCacheHeader** const  ppch );
        static ERR ErrDump( _In_ IFileFilter* const pff,
                            _In_ const QWORD        ib,
                            _In_ CPRINTF* const     pcprintf );

        void operator delete( _In_ void* const pv );

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

        BYTE                m_rgbPadding[   cbCacheHeader 
                                            - sizeof( m_le_ulChecksum )
                                            - sizeof( m_rgbHeaderType ) ];
};

#include <poppack.h>

INLINE CHashedLRUKCacheHeader::CHashedLRUKCacheHeader()
{
    C_ASSERT( 0 == offsetof( CHashedLRUKCacheHeader, m_le_ulChecksum ) );
    C_ASSERT( cbCacheHeader == sizeof( CHashedLRUKCacheHeader ) );
}

INLINE ERR CHashedLRUKCacheHeader::ErrCreate( _Out_ CHashedLRUKCacheHeader** const ppch )
{
    ERR                     err = JET_errSuccess;
    CHashedLRUKCacheHeader* pch = NULL;

    *ppch = NULL;

    Alloc( pch = new CHashedLRUKCacheHeader() );

    UtilMemCpy( pch->m_rgbHeaderType, c_rgbCacheHeaderV1, cbGuid );

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

INLINE void CHashedLRUKCacheHeader::operator delete( _In_ void* const pv )
{
    OSMemoryPageFree( pv );
}

INLINE ERR CHashedLRUKCacheHeader::ErrDump( _In_ CPRINTF* const pcprintf )
{
    (*pcprintf)(    "HASHED LRUK CACHE HEADER:\n" );
    (*pcprintf)(    "\n" );
    (*pcprintf)(    "Fields:\n" );
    (*pcprintf)(    "                                      Checksum:  0x%08lx\n", LONG( m_le_ulChecksum ) );
    (*pcprintf)(    "                                          Type:  %08lx-%04hx-%04hx-%02hx%02hx-%02hx%02hx%02hx%02hx%02hx%02hx\n",
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
        Call( ErrERRCheck( errInvalidType ) );
    }

HandleError:
    return err;
}
