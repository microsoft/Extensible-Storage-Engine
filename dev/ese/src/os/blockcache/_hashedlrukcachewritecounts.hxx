// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  Cached Block Write Counts
//
//  This part of the cache format provides lost write protection for the core cache state provided by the
//  CCachedBlockSlab's chunks (CCachedBlockChunk).  An array of CCachedBlockWriteCounts are managed by the
//  CCachedBlockWriteCountsManager to provide lost write protection for the cache.
//
//  Whenever a CCachedBlockChunk is loaded, its integrity is verified.  We then verify that this image is the last
//  known written image of that chunk using this write count table.  If the write count matches then it is safe to use
//  the cache state.  If there is a mismatch then we can tell which information experienced the lost write.

#pragma push_macro( "new" )
#undef new

#include <pshpack1.h>

//PERSISTED
class CCachedBlockWriteCounts : CBlockCacheHeaderHelpers  // cbwcs
{
    public:

        static ERR ErrCreate( _Out_ CCachedBlockWriteCounts** const ppcbwcs );
        static ERR ErrLoad( _In_    IFileSystemConfiguration* const pfsconfig, 
                            _In_    IFileFilter* const              pff,
                            _In_    const QWORD                     ib,
                            _Out_   CCachedBlockWriteCounts** const ppcbwcs );
        static ERR ErrDump( _In_ IFileFilter* const pff,
                            _In_    const QWORD     ib,
                            _In_ CPRINTF* const     pcprintf );

        static DWORD Ccbwc() { return _countof( m_rgcbwc ); }

        CachedBlockWriteCount Cbwc() const { return m_le_cbwc; }
        DWORD DwUniqueId() const { return m_le_dwUniqueId; }

        volatile CachedBlockWriteCount* Pcbwc( _In_ const DWORD icbwc )
        {
            return icbwc >= Ccbwc() ? NULL : &m_rgcbwc[ icbwc ];
        }

        ERR ErrVerify( _In_ const QWORD ib );

        ERR ErrFinalize(    _In_ const QWORD                    ib,
                            _In_ const CachedBlockWriteCount    cbwc,
                            _In_ const DWORD                    dwUniqueId );

        void operator delete( _In_opt_ void* const pv );

        ERR ErrDump( _In_ CPRINTF* const pcprintf );

    private:

        friend class CBlockCacheHeaderHelpers;

        enum { cbWriteCounts = cbBlock };

        CCachedBlockWriteCounts();

        void* operator new( _In_ const size_t cb );
        void* operator new( _In_ const size_t cb, _In_ const void* const pv );
        
        ERR ErrValidate( _In_ const QWORD ib );

    private:

        using CWriteCountsPool = TPool<BYTE[ cbWriteCounts ], fFalse>;

    private:

        LittleEndian<ULONG>                 m_le_ulChecksum;    //  offset 0:  checksum
        BYTE                                m_rgbZero[ 4 ];     //  unused because it is not protected by the ECC
        LittleEndian<ClusterNumber>         m_le_clno;          //  location of these states
        LittleEndian<CachedBlockWriteCount> m_le_cbwc;          //  write set write count
        LittleEndian<DWORD>                 m_le_dwUniqueId;    //  write set unique id

        static const size_t                 s_cbCachedBlockWriteCounts  =   cbWriteCounts
                                                                            - sizeof( m_le_ulChecksum )
                                                                            - sizeof( m_rgbZero )
                                                                            - sizeof( m_le_clno )
                                                                            - sizeof( m_le_cbwc )
                                                                            - sizeof( m_le_dwUniqueId );

        volatile CachedBlockWriteCount      m_rgcbwc[ s_cbCachedBlockWriteCounts / sizeof( CachedBlockWriteCount ) ];
};

#include <poppack.h>

template<>
struct CBlockCacheHeaderTraits<CCachedBlockWriteCounts>
{
    static const CBlockCacheHeaderHelpers::ChecksumType checksumType            = CBlockCacheHeaderHelpers::ChecksumType::checksumECC;
    static const IFileFilter::IOMode                    ioMode                  = iomEngine;
};

INLINE CCachedBlockWriteCounts::CCachedBlockWriteCounts()
{
    C_ASSERT( 0 == offsetof( CCachedBlockWriteCounts, m_le_ulChecksum ) );
    C_ASSERT( cbWriteCounts == sizeof( CCachedBlockWriteCounts ) );
    C_ASSERT( 1019 == _countof( m_rgcbwc ) );
}

INLINE ERR CCachedBlockWriteCounts::ErrCreate( _Out_ CCachedBlockWriteCounts** const ppcbwcs )
{
    ERR                         err     = JET_errSuccess;
    CCachedBlockWriteCounts*    pcbwcs  = NULL;

    *ppcbwcs = NULL;

    Alloc( pcbwcs = new CCachedBlockWriteCounts() );

    *ppcbwcs = pcbwcs;
    pcbwcs = NULL;

HandleError:
    delete pcbwcs;
    if ( err < JET_errSuccess )
    {
        delete *ppcbwcs;
        *ppcbwcs = NULL;
    }
    return err;
}

INLINE ERR CCachedBlockWriteCounts::ErrLoad(    _In_    IFileSystemConfiguration* const pfsconfig,
                                                _In_    IFileFilter* const              pff,
                                                _In_    const QWORD                     ib,
                                                _Out_   CCachedBlockWriteCounts** const ppcbwcs )
{
    ERR                         err     = JET_errSuccess;
    CCachedBlockWriteCounts*    pcbwcs  = NULL;

    *ppcbwcs = NULL;

    Call( ErrLoadHeader( pff, ib, &pcbwcs ) );

    Call( pcbwcs->ErrValidate( ib ) );

    *ppcbwcs = pcbwcs;
    pcbwcs = NULL;

HandleError:
    delete pcbwcs;
    if ( err < JET_errSuccess )
    {
        delete *ppcbwcs;
        *ppcbwcs = NULL;
    }
    return err;
}

INLINE ERR CCachedBlockWriteCounts::ErrDump(    _In_ IFileFilter* const pff,
                                                _In_ const QWORD        ib,
                                                _In_ CPRINTF* const     pcprintf )
{
    ERR                         err     = JET_errSuccess;
    CCachedBlockWriteCounts*    pcbwcs  = NULL;

    Call( ErrLoadHeader( pff, ib, &pcbwcs ) );

    Call( pcbwcs->ErrValidate( ib ) );

    Call( pcbwcs->ErrDump( pcprintf ) );

HandleError:
    delete pcbwcs;
    return err;
}

INLINE ERR CCachedBlockWriteCounts::ErrVerify( _In_ const QWORD ib )
{
    ERR err = JET_errSuccess;

    //  do the base header validation / repair

    Call( ErrValidateHeader( this ) );

    //  perform uninitialized page checks

    Call( ErrValidate( ib ) );

HandleError:
    return err;
}

INLINE ERR CCachedBlockWriteCounts::ErrFinalize(    _In_ const QWORD                    ib,
                                                    _In_ const CachedBlockWriteCount    cbwc,
                                                    _In_ const DWORD                    dwUniqueId )
{
    //  ensure that the bytes that aren't used because they aren't covered by the ECC are set to zero

    memset( m_rgbZero, 0, _cbrg( m_rgbZero ) );

    //  set our cluster number, write set count, and unique id

    m_le_clno = ClusterNumber( ib / cbCachedBlock );
    m_le_cbwc = cbwc;
    m_le_dwUniqueId = dwUniqueId;

    //  compute our checksum

    m_le_ulChecksum = GenerateChecksum( this );

    return JET_errSuccess;
}

INLINE void CCachedBlockWriteCounts::operator delete( _In_opt_ void* const pv )
{
    void* pvT = pv;
    CWriteCountsPool::Free( &pvT );
}

INLINE ERR CCachedBlockWriteCounts::ErrDump( _In_ CPRINTF* const pcprintf )
{
    (*pcprintf)(    "Cached Block Write Counts:\n" );
    (*pcprintf)(    "\n" );
    (*pcprintf)(    "Fields:\n" );
    (*pcprintf)(    "                         ulChecksum:  0x%08lx\n", LONG( m_le_ulChecksum ) );
    (*pcprintf)(    "                               clno:  0x%08lx\n", LONG( ClusterNumber( m_le_clno ) ) );
    (*pcprintf)(    "                               cbwc:  0x%08lx\n", LONG( CachedBlockWriteCount( m_le_cbwc ) ) );
    (*pcprintf)(    "                         dwUniqueId:  0x%08lx\n", LONG( m_le_dwUniqueId ) );
    (*pcprintf)(    "    Cached Block Write Counts Count:  %d\n", LONG( Ccbwc() ) );
    (*pcprintf)(    "\n" );

    return JET_errSuccess;
}

INLINE void* CCachedBlockWriteCounts::operator new( _In_ const size_t cb )
{
    return CWriteCountsPool::PvAllocate();
}

INLINE void* CCachedBlockWriteCounts::operator new( _In_ const size_t cb, _In_ const void* const pv )
{
    return (void*)pv;
}

#pragma pop_macro( "new" )

INLINE ERR CCachedBlockWriteCounts::ErrValidate( _In_ const QWORD ib )
{
    ERR     err = JET_errSuccess;
    BOOL    fUninit = fFalse;

    //  determine if the cached block chunk is uninitialized

    fUninit = m_le_ulChecksum == 0 && FUtilZeroed( (const BYTE*)this, sizeof( *this ) );

    //  uninitialized pages are expected and allowed.  if this is due to a lost write then we will discover that when
    //  checking against the write count in a cached block chunk later

    if ( fUninit )
    {
        m_le_clno = ClusterNumber( ib / cbCachedBlock );

        Error( JET_errSuccess );
    }

    //  verify that we read the status at the correct offset

    if ( m_le_clno != ClusterNumber( ib / cbCachedBlock ) )
    {
        Error( ErrERRCheck( JET_errReadVerifyFailure ) );
    }

HandleError:
    return err;
}
