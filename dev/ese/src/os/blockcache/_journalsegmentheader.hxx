// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  Journal Segment Header

#pragma push_macro( "new" )
#undef new

#include <pshpack1.h>

//PERSISTED
class CJournalSegmentHeader : CBlockCacheHeaderHelpers  // jsh
{
    public:

        static ERR ErrCreate(   _In_    const SegmentPosition           spos,
                                _In_    const DWORD                     dwUniqueIdPrev,
                                _In_    const SegmentPosition           sposReplay,
                                _In_    const SegmentPosition           sposDurable,
                                _Out_   CJournalSegmentHeader** const   ppjsh );
        static ERR ErrLoad( _In_    IFileFilter* const              pff,
                            _In_    const QWORD                     ib,
                            _Out_   CJournalSegmentHeader** const   ppjsh );
        static ERR ErrDump( _In_ IFileFilter* const pff,
                            _In_ const QWORD        ib,
                            _In_ CPRINTF* const     pcprintf );

        ERR ErrFinalize();

        void operator delete( _In_ void* const pv );

        RegionPosition RposFirst() const { return m_le_rposFirst; }
        SegmentPosition Spos() const
        {
            return (SegmentPosition)rounddn( (QWORD)RposFirst(), cbSegment );
        }
        DWORD DwUniqueId() const { return m_le_dwUniqueId; }
        DWORD DwUniqueIdPrev() const { return m_le_dwUniqueIdPrev; }
        SegmentPosition SposReplay() const { return m_le_sposReplay; }
        SegmentPosition SposDurable() const { return m_le_sposDurable; }

        size_t CbRegions() const { return sizeof( m_rgbRegions ); }
        BYTE* RgbRegions() { return &m_rgbRegions[ 0 ]; }

        const CJournalRegion* const Pjreg( _In_ const RegionPosition rpos )
        {
            const QWORD ib = rpos - RposFirst();
            return ib >= CbRegions() - sizeof( CJournalRegion )
                ? NULL
                : (const CJournalRegion*)( RgbRegions() + ib );
        }

        ERR ErrDump( _In_ CPRINTF* const pcprintf );

    private:

        friend class CBlockCacheHeaderHelpers;

        enum { cbSegment = cbBlock };

        CJournalSegmentHeader();

        void* operator new( _In_ const size_t cb );
        void* operator new( _In_ const size_t cb, _In_ const void* const pv );
        
        ERR ErrValidate();

    private:

        LittleEndian<ULONG>             m_le_ulChecksum;        //  offset 0:  checksum
        BYTE                            m_rgbZero[ 4 ];         //  unused because it is not protected by the ECC
        LittleEndian<RegionPosition>    m_le_rposFirst;         //  region position of the first region in this segment
        LittleEndian<DWORD>             m_le_dwUniqueId;        //  unique id for this segment
        LittleEndian<DWORD>             m_le_dwUniqueIdPrev;    //  unique id for the previous segment
        LittleEndian<SegmentPosition>   m_le_sposReplay;        //  segment position of the last known replay segment
        LittleEndian<SegmentPosition>   m_le_sposDurable;       //  segment position of the last known durable segment

        BYTE                            m_rgbRegions[   cbSegment 
                                                        - sizeof( m_le_ulChecksum )
                                                        - sizeof( m_rgbZero )
                                                        - sizeof( m_le_rposFirst )
                                                        - sizeof( m_le_dwUniqueId )
                                                        - sizeof( m_le_dwUniqueIdPrev )
                                                        - sizeof( m_le_sposReplay )
                                                        - sizeof( m_le_sposDurable ) ];
};

#include <poppack.h>

template<>
struct CBlockCacheHeaderTraits<CJournalSegmentHeader>
{
    static const CBlockCacheHeaderHelpers::ChecksumType checksumType            = CBlockCacheHeaderHelpers::ChecksumType::checksumECC;
    static const IFileFilter::IOMode                    ioMode                  = iomEngine;
};

INLINE CJournalSegmentHeader::CJournalSegmentHeader()
{
    C_ASSERT( 0 == offsetof( CJournalSegmentHeader, m_le_ulChecksum ) );
    C_ASSERT( cbSegment == sizeof( CJournalSegmentHeader ) );
}

INLINE ERR CJournalSegmentHeader::ErrCreate(    _In_    const SegmentPosition           spos,
                                                _In_    const DWORD                     dwUniqueIdPrev,
                                                _In_    const SegmentPosition           sposReplay,
                                                _In_    const SegmentPosition           sposDurable,
                                                _Out_   CJournalSegmentHeader** const   ppjsh)
{
    ERR                     err         = JET_errSuccess;
    CJournalSegmentHeader*  pjsh        = NULL;
    UINT                    uiUniqueId  = 0;

    *ppjsh = NULL;

    Alloc( pjsh = new CJournalSegmentHeader() );

    pjsh->m_le_rposFirst = (RegionPosition)spos + offsetof( CJournalSegmentHeader, m_rgbRegions );
    rand_s( &uiUniqueId );
    pjsh->m_le_dwUniqueId = uiUniqueId;
    pjsh->m_le_dwUniqueIdPrev = dwUniqueIdPrev;
    pjsh->m_le_sposReplay = sposReplay;
    pjsh->m_le_sposDurable = sposDurable;

    *ppjsh = pjsh;
    pjsh = NULL;

HandleError:
    delete pjsh;
    if ( err < JET_errSuccess )
    {
        delete * ppjsh;
        *ppjsh = NULL;
    }
    return err;
}

INLINE ERR CJournalSegmentHeader::ErrLoad(  _In_    IFileFilter* const              pff,
                                            _In_    const QWORD                     ib,
                                            _Out_   CJournalSegmentHeader** const   ppjsh )
{
    ERR                     err     = JET_errSuccess;
    CJournalSegmentHeader*  pjsh    = NULL;

    *ppjsh = NULL;

    Call( ErrLoadHeader( pff, ib, &pjsh ) );

    Call( pjsh->ErrValidate() );

    *ppjsh = pjsh;
    pjsh = NULL;

HandleError:
    delete pjsh;
    if ( err < JET_errSuccess )
    {
        delete* ppjsh;
        *ppjsh = NULL;
    }
    return err;
}

INLINE ERR CJournalSegmentHeader::ErrDump(  _In_ IFileFilter* const pff,
                                            _In_ const QWORD        ib,
                                            _In_ CPRINTF* const     pcprintf )
{
    ERR                     err     = JET_errSuccess;
    CJournalSegmentHeader*  pjsh    = NULL;

    Call( ErrLoadHeader( pff, ib, &pjsh ) );

    Call( pjsh->ErrValidate() );

    Call( pjsh->ErrDump( pcprintf ) );

HandleError:
    delete pjsh;
    return err;
}

INLINE ERR CJournalSegmentHeader::ErrFinalize()
{
    m_le_ulChecksum = GenerateChecksum( this );

    return JET_errSuccess;
}

INLINE void CJournalSegmentHeader::operator delete( _In_ void* const pv )
{
    OSMemoryPageFree( pv );
}

INLINE ERR CJournalSegmentHeader::ErrDump( _In_ CPRINTF* const pcprintf )
{
    (*pcprintf)(    "JOURNAL SEGMENT 0x%016I64x:\n", Spos() );
    (*pcprintf)(    "\n" );
    (*pcprintf)(    "Fields:\n" );
    (*pcprintf)(    "        ulChecksum:  0x%08lx\n", LONG( m_le_ulChecksum ) );
    (*pcprintf)(    "         rposFirst:  0x%016I64x\n", RposFirst() );
    (*pcprintf)(    "        dwUniqueId:  0x%08lx\n", DwUniqueId() );
    (*pcprintf)(    "    dwUniqueIdPrev:  0x%08lx\n", DwUniqueIdPrev() );
    (*pcprintf)(    "        sposReplay:  0x%016I64x\n", SposReplay());
    (*pcprintf)(    "       sposDurable:  0x%016I64x\n", SposDurable() );
    (*pcprintf)(    "\n" );

    return JET_errSuccess;
}

INLINE void* CJournalSegmentHeader::operator new( _In_ const size_t cb )
{
#ifdef MEM_CHECK
    return PvOSMemoryPageAlloc_( cb, NULL, fFalse, SzNewFile(), UlNewLine() );
#else  //  !MEM_CHECK
    return PvOSMemoryPageAlloc( cb, NULL );
#endif  //  MEM_CHECK
}

INLINE void* CJournalSegmentHeader::operator new( _In_ const size_t cb, _In_ const void* const pv )
{
    return (void*)pv;
}

#pragma pop_macro( "new" )

INLINE ERR CJournalSegmentHeader::ErrValidate()
{
    ERR err = JET_errSuccess;

    if ( m_le_ulChecksum == 0 && FUtilZeroed( (const BYTE*)this, sizeof( *this ) ) )
    {
        Error( ErrERRCheck( JET_errPageNotInitialized ) );
    }

HandleError:
    return err;
}
