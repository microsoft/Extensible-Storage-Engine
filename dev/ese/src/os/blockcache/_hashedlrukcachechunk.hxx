// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once


//  Cached Block Chunk

#pragma push_macro( "new" )
#undef new

#include <pshpack1.h>

//PERSISTED
class CCachedBlockChunk : CBlockCacheHeaderHelpers  // cbc
{
    public:

        static ERR ErrLoad( _In_    IFileSystemConfiguration* const pfsconfig, 
                            _In_    IFileFilter* const              pff,
                            _In_    const QWORD                     ib,
                            _In_    const CachedBlockWriteCount     cbwc,
                            _Out_   CCachedBlockChunk** const       ppcbc );
        static ERR ErrDump( _In_ IFileFilter* const pff,
                            _In_    const QWORD     ib,
                            _In_ CPRINTF* const     pcprintf );

        static DWORD Ccbl() { return _countof( m_rgcbl ); }

        ERR ErrVerify( _In_ const QWORD ib, _In_ const CachedBlockWriteCount cbwc );

        void Reset( _In_ const QWORD ib, _In_ const CachedBlockWriteCount cbwc );

        ERR ErrFinalize();

        void operator delete( _In_opt_ void* const pv );

        CachedBlockWriteCount Cbwc() const { return m_le_cbwc; }

        CCachedBlock* Pcbl( _In_ const DWORD icbl )
        {
            return icbl >= Ccbl() ? NULL : &m_rgcbl[ icbl ];
        }

        ERR ErrDump( _In_ CPRINTF* const pcprintf );

    private:

        friend class CBlockCacheHeaderHelpers;

        enum { cbChunk = cbBlock };

        CCachedBlockChunk();

        void* operator new( _In_ const size_t cb );
        void* operator new( _In_ const size_t cb, _In_ const void* const pv );
        
        ERR ErrValidate( _In_ const QWORD ib, _In_ const CachedBlockWriteCount cbwc );

    private:

        LittleEndian<ULONG>                 m_le_ulChecksum;        //  offset 0:  checksum
        BYTE                                m_rgbZero[ 4 ];         //  unused because it is not protected by the ECC
        LittleEndian<ClusterNumber>         m_le_clno;              //  location of this chunk
        LittleEndian<CachedBlockWriteCount> m_le_cbwc;              //  write state
        BYTE                                m_rgbReserved[ 24 ];    //  reserved; always zero

        static const size_t s_cbCachedBlock =   cbChunk 
                                                - sizeof( m_le_ulChecksum )
                                                - sizeof( m_rgbZero )
                                                - sizeof( m_le_clno )
                                                - sizeof( m_le_cbwc )
                                                - sizeof( m_rgbReserved );

        CCachedBlock                        m_rgcbl[ s_cbCachedBlock / sizeof( CCachedBlock ) ];
};

#include <poppack.h>

template<>
struct CBlockCacheHeaderTraits<CCachedBlockChunk>
{
    static const CBlockCacheHeaderHelpers::ChecksumType checksumType            = CBlockCacheHeaderHelpers::ChecksumType::checksumECC;
    static const IFileFilter::IOMode                    ioMode                  = iomEngine;
};

INLINE CCachedBlockChunk::CCachedBlockChunk()
{
    C_ASSERT( 0 == offsetof( CCachedBlockChunk, m_le_ulChecksum ) );
    C_ASSERT( cbChunk == sizeof( CCachedBlockChunk ) );

    const int ccbl = 104;
    C_ASSERT( ccbl == _countof( m_rgcbl ) );
    C_ASSERT( ccbl % 8 == 0 );  //  we want to cache 8 block items (i.e. 32kb pages) without wasting space
}

INLINE ERR CCachedBlockChunk::ErrLoad(  _In_    IFileSystemConfiguration* const pfsconfig,
                                        _In_    IFileFilter* const              pff,
                                        _In_    const QWORD                     ib,
                                        _In_    const CachedBlockWriteCount     cbwc,
                                        _Out_   CCachedBlockChunk** const       ppcbc )
{
    ERR                 err     = JET_errSuccess;
    CCachedBlockChunk*  pcbc    = NULL;

    *ppcbc = NULL;

    Call( ErrLoadHeader( pff, ib, &pcbc ) );

    Call( pcbc->ErrValidate( ib, cbwc ) );

    *ppcbc = pcbc;
    pcbc = NULL;

HandleError:
    delete pcbc;
    if ( err < JET_errSuccess )
    {
        delete *ppcbc;
        *ppcbc = NULL;
    }
    return err;
}

INLINE ERR CCachedBlockChunk::ErrDump(  _In_ IFileFilter* const pff,
                                        _In_ const QWORD        ib,
                                        _In_ CPRINTF* const     pcprintf )
{
    ERR                 err     = JET_errSuccess;
    CCachedBlockChunk*  pcbc    = NULL;

    Call( ErrLoadHeader( pff, ib, &pcbc ) );

    Call( pcbc->ErrValidate( ib, pcbc->Cbwc() ) );

    Call( pcbc->ErrDump( pcprintf ) );

HandleError:
    delete pcbc;
    return err;
}

INLINE ERR CCachedBlockChunk::ErrVerify( _In_ const QWORD ib, _In_ const CachedBlockWriteCount cbwc )
{
    ERR err = JET_errSuccess;

    //  do the base header validation / repair

    Call( ErrValidateHeader( this ) );

    Call( ErrValidate( ib, cbwc ) );

HandleError:
    return err;
}

INLINE void CCachedBlockChunk::Reset( _In_ const QWORD ib, _In_ const CachedBlockWriteCount cbwc )
{
    memset( this, 0, sizeof( CCachedBlockChunk ) );
    m_le_clno = ClusterNumber( ib / cbCachedBlock );
    m_le_cbwc = cbwc;
}

INLINE ERR CCachedBlockChunk::ErrFinalize()
{
    memset( m_rgbZero, 0, _cbrg( m_rgbZero ) );
    memset( m_rgbReserved, 0, _cbrg( m_rgbReserved ) );

    do { m_le_cbwc++; } while ( Cbwc() == cbwcUnknown || Cbwc() == cbwcNeverWritten );

    m_le_ulChecksum = GenerateChecksum( this );

    return JET_errSuccess;
}

INLINE void CCachedBlockChunk::operator delete( _In_opt_ void* const pv )
{
    OSMemoryPageFree( pv );
}

INLINE ERR CCachedBlockChunk::ErrDump( _In_ CPRINTF* const pcprintf )
{
    (*pcprintf)(    "Cached Block Chunk:\n" );
    (*pcprintf)(    "\n" );
    (*pcprintf)(    "Fields:\n" );
    (*pcprintf)(    "                  ulChecksum:  0x%08lx\n", LONG( m_le_ulChecksum ) );
    (*pcprintf)(    "                        clno:  0x%08lx\n", LONG( ClusterNumber( m_le_clno ) ) );
    (*pcprintf)(    "    Cached Block Write Count:  %d\n", LONG( CachedBlockWriteCount( m_le_cbwc ) ) );
    (*pcprintf)(    "          Cached Block Count:  %d\n", LONG( Ccbl() ) );
    (*pcprintf)(    "\n" );

    return JET_errSuccess;
}

INLINE void* CCachedBlockChunk::operator new( _In_ const size_t cb )
{
#ifdef MEM_CHECK
    return PvOSMemoryPageAlloc_( cb, NULL, fFalse, SzNewFile(), UlNewLine() );
#else  //  !MEM_CHECK
    return PvOSMemoryPageAlloc( cb, NULL );
#endif  //  MEM_CHECK
}

INLINE void* CCachedBlockChunk::operator new( _In_ const size_t cb, _In_ const void* const pv )
{
    return (void*)pv;
}

#pragma pop_macro( "new" )

INLINE ERR CCachedBlockChunk::ErrValidate( _In_ const QWORD ib, _In_ const CachedBlockWriteCount cbwc )
{
    ERR     err     = JET_errSuccess;
    BOOL    fUninit = fFalse;

    //  determine if the cached block chunk is uninitialized

    fUninit = m_le_ulChecksum == 0 && FUtilZeroed( (const BYTE*)this, sizeof( *this ) );

    //  if the write state doesn't match our expectations then declare a lost write
    //
    //  NOTE:  the lost write may be in the flush map.  in that case its write count will be behind

    if ( cbwc != cbwcUnknown && ( !fUninit && m_le_cbwc != cbwc ) || ( fUninit && cbwc != cbwcNeverWritten ) )
    {
        Error( ErrERRCheck( JET_errReadLostFlushVerifyFailure ) );
    }

    //  if the cached block chunk is uninitialized then indicate this so we can handle it

    if ( fUninit )
    {
        m_le_clno = ClusterNumber( ib / cbCachedBlock );

        Error( ErrERRCheck( JET_errPageNotInitialized ) );
    }

    //  verify that we read the cached block chunk at the correct offset

    if ( m_le_clno != ClusterNumber( ib / cbCachedBlock ) )
    {
        Error( ErrERRCheck( JET_errReadVerifyFailure ) );
    }

HandleError:
    return err;
}
