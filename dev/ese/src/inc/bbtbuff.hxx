// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

/************************************************************************************************************
A BBT buffer is a data structure used to store tree operations on the BBT at intermediate levels of the tree.
Each non-leaf (root & internal) page in a BBT has a BBT buffer. It has the following strucutre:
1. Each BBT buffer comprises of multiple physically contiguous ESE pages.
2. The first page of the BBT buffer is called the BBT root (or BBT base page). A portion of it is allocated
   to hold the BBT buffer using a reserved tag. The rest is available for use by classic BT nodes for storing
   key separators.
3. The rest of the pages comprising the BBT buffer each contain 1 reserved tag that takes up all of the
   available space on the page.
3. All of this space is considered one logical buffer indexed by an offset starting from the BBT root.
3. The BBT header is stored at offset 0 (i.e. at the begining of the BBT root).
4. The rest of the space is used to store nodes in a SkipList structure.
5. Each node in the skiplist stores a BBTBuffOpcode, KEY & DATA for the operation.
6. The buffer is designed to be append-only. Each new operation on the tree results in adding a new node to
   the buffer with the right opcode. E.g., a delete on a key will result in an insertion of bbtOpDelete with
   the right key (and no data).

The BBT buffer provides the following:
1. It is sorted, allowing avg log(n) seek/insert/delete operations.
2. It allows adding duplicate nodes, and provides a stable sort over the nodes. That is, the BBT buffer
   guarantees that the insertion order of duplicate nodes is maintained by the sort. This means that
   operations can be enumerated from oldest to latest.
3. Provides facilities for bulk merge and deletion of nodes. This is used to 'evict' operations from the
   top-level to lower levels of the BBT, as buffers get full.

Layering and Responsibilities:
1. Owns the physical format of the BBT buffer. Uses CPAGE to manage per-page format. Comparable to NODE.
2. Also owns runtime interface to the buffer:
   - Read APIs and Write APIs (non-transacted).
   - The companion class BBTBuffWriter provides transacted APIs for writing, allowing redo through a trx log.
   - Manages latches. Built on top of CSR latch APIs.
   - Also handles currency for the buffer. This is comparable to the CSR interface.

BBTBuff Latching policy:
1. Base page must always be kept latched (caller must own this latch).
2. Base page latch state when the BBTBuff is loaded is considered 'ground state.'
3. Any latches obtained during any BBTBuff operation are kept latched at 'ground state'
   for the lifetime of BBTBuff or until Release() is called.
4. For modify operations, latches will be upgraded to write for the duration of the operation,
   then downgraded to ground state when the operation is complete.
5. ErrUpgradeToWriteMode() means that the ground state is upgraded to latchWrite alongwith the base page latch,
   so after that, any latches acquired will be kept at latchWrite state.
6. Downgrade() can be called to bring the ground state back to read or RIW. Base page latch will be
   downgraded accordingly.
7. Latch order: Base page must be latched/upgraded first. The rest of the pages can be latched in any order.

Above policy is used in three different ways:
1. Read-only: Simplest form. All latches stay at latchRead.
2. RIW: BBTBuff is loaded with base page at latchRIW. Any modify operations will temporarily
        upgrade to latchWrite, and then downgrades latches back to latchRIW. Theoretically, this
        can also be done starting with latchRead, but latch conflicts could cause us to lose
        all latches and fail (BBTBuff handles this case properly, and marks itself unusable).
        However, we always start with RIW for a write operation.
3. Write: ErrUpgradeToWriteMode() is used to keep BBTBuff in latchWrite state.

************************************************************************************************************/

// Maximum levels supported by the skiplist
// 16 allows 64k nodes to be stored in the list with 
// list operations achieving average case O(logn) performance.
constexpr int MAX_LEVELS = 16;

// This is the reserved itag on every BBTBuff root page. Must be the second reserved itag.
constexpr int itagReservedBBTBuffRoot = 1;

struct BBTBuffFormat
{
    int cbCPAGE;        // size of the underlying physical page (CPAGE).
    int cbBBTRoot;      // size of the BBT root, amount of space allocated to the BBT buffer on the BBT base page.
    int cpgInBBTBuff;   // total number of physical pages that comprise a BBT superpage.
};

// UA_TODO: magic numbers, tune later
extern constexpr __declspec( selectany ) BBTBuffFormat BBTBUFF_FORMAT_CONSTANTS[] =
{
    { 4 * 1024,     1 * 1024,       32 },   // 128k BBTBuff
    { 8 * 1024,     4 * 1024,       16 },   // 128k
    { 16 * 1024,    8 * 1024,       16 },   // 256k
    { 32 * 1024,    24 * 1024,      16 },   // 512k
};

///////////////////////////////////////////////////////////////////////////////
// BEGIN persisted structures
#include <pshpack1.h>

PERSISTED
enum BBTBuffVersion : BYTE
{
    bbtvInvalid     = 0,
    bbtvInitial     = 1,

    bbtvMax         = 255
};

PERSISTED
enum BBTBuffOpcode : BYTE
{
    bbtOpNone           = 0,
    bbtOpInsert         = 1,
    bbtOpUpdate         = 2,
    bbtOpDelete         = 3,
    bbtOpEscrowUpdate   = 4,
    bbtOpUpsert         = 5,

    bbtOpMax            = 15,
};

// SkipListNode structure:
//        | Hdr | m_cbKey+Flags  | m_cbData | rgLinks[ m_iLevel + 2 ] |        Key        |            Data            |
// Bytes: 0     2            DVC-4          6                       11->51          IbKey()+m_CbKey            IbData()+m_cbData
// D = fDeleted
// V = fVersioned
// C = Compressed
// - = Reserved
// iLevel+2 because level0 node stores prev0 and next0 link

// Stored on the high bits of SkipListNode::m_cbKey
PERSISTED
enum SkipListNodeFlags : USHORT
{
    fSLNNone        = 0,
    fSLNDeleted     = 0x01,
    fSLNVersioned   = 0x02,
    fSLNCompressed  = 0x04,
};

inline SkipListNodeFlags operator|( SkipListNodeFlags a, SkipListNodeFlags b )
{
    return static_cast<SkipListNodeFlags>( static_cast<USHORT>( a ) | static_cast<USHORT>( b ) );
}

inline SkipListNodeFlags operator&( SkipListNodeFlags a, SkipListNodeFlags b )
{
    return static_cast<SkipListNodeFlags>( static_cast<USHORT>( a ) & static_cast<USHORT>( b ) );
}

inline SkipListNodeFlags operator~( SkipListNodeFlags a )
{
    return static_cast<SkipListNodeFlags>( ~static_cast<USHORT>( a ) );
}

PERSISTED
class SkipListLink
{
    friend class SkipListLinkArray;
    static const int ALIGNMENT = 4; // must be power-of-2
    int32_t ib;

public:
    SkipListLink()                                      = default;
    SkipListLink( const SkipListLink& rhs )             = default;
    SkipListLink( nullptr_t unused )                    { ib = 0; } // nullptr_t used to only allow nullptr or 0 as an argument,
                                                                    // other ints or ptr types aren't convertible to nullptr_t
    bool FNull() const                                  { return ib == 0; }
    void Nullify()                                      { ib = 0; }
    int ToInt() const                                   { return ib; }

    bool operator==( const SkipListLink& rhs ) const    { return ( ib == rhs.ib ); }
    bool operator!=( const SkipListLink& rhs ) const    { return (ib != rhs.ib ); }
    SkipListLink& operator=( const SkipListLink& rhs )  = default;

    SkipListLink& Inc( int _ib )
    {
        Assert( FRounded( _ib ) );
        ib += _ib;
        return *this;
    }

    SkipListLink& Dec( int _ib )
    {
        Assert( FRounded( _ib ) );
        ib -= _ib;
        return *this;
    }

public:
    static SkipListLink FromInt( int _ib )
    {
        Assert( FRounded( _ib ) );
        SkipListLink link;
        link.ib = _ib;
        return link;
    }

    static_assert( FPowerOf2( ALIGNMENT ), "The following functions require ALIGNMENT to be a power-of-2" );

    static SkipListLink Null()              { return SkipListLink( 0 ); }
    static int Roundup( int _ib )           { return ( _ib + ALIGNMENT - 1 ) & -ALIGNMENT; }
    static bool FRounded( int _ib )         { return !( _ib & ( ALIGNMENT - 1 ) ); }
};

#include <poppack.h>

// SkipListLinkArray structure:
//        | Preamble | rgLinks[ cLinks ] |
// Bytes: 0         1->x              x->x+c*2
// Preamable = A sequence of bytes with each 4-bits in it storing the lower order 4 bits of the link.
//             So each link is 16-bits in rgLinks[i] + 4-bits in preamble = 20-bits.
// The 20-bits are further left-shifted to yield a 22-bit link (that can address 4-byte aligned upto 4mb).

// Manipulates persisted representation of skiplist links of a given node.
// The class layout doesn't define the persisted layout of the array.
// Instead, its implementation defines the persisted layout.
PERSISTED
class SkipListLinkArray
{
public:
    static const int    REMOTE_BITS = 4;    // bits per link stored in the preamble bytes
    static const int    LOCAL_BITS = 16;    // bits stored with each link
    static const int    REMOTE_BITMASK = ( 1 << REMOTE_BITS ) - 1;
    static const int    LINK_MAX = ( 1 << ( LOCAL_BITS + REMOTE_BITS ) ) * SkipListLink::ALIGNMENT - 1; // 4mb

private:
    BYTE*       m_rgb = nullptr;
    int         m_cLinks = 0;
    int         m_iOffset = 0;

    SkipListLink GetLink_( int i ) const
    {
        Assert( i < m_cLinks );
        const UnalignedLittleEndianPtr<USHORT> prgLinks = (USHORT*) &m_rgb[ CbPreamble( m_cLinks ) ];
        int ilink = prgLinks[ i ] << REMOTE_BITS;
        int remoteBits = m_rgb[ ( i * REMOTE_BITS ) / 8 ];
        remoteBits = ( remoteBits >> ( ( i * REMOTE_BITS ) % 8 ) ) & REMOTE_BITMASK;
        ilink |= remoteBits;
        return SkipListLink::FromInt( ilink * SkipListLink::ALIGNMENT );
    }

    void SetLink_( int i, SkipListLink link )
    {
        UnalignedLittleEndianPtr<USHORT> prgLinks = (USHORT*) &m_rgb[ CbPreamble( m_cLinks ) ];

        Assert( i < m_cLinks );
        Assert( link.ib <= LINK_MAX );
        int ilink = link.ib / SkipListLink::ALIGNMENT;
        int remoteBits = ilink & REMOTE_BITMASK;
        prgLinks[ i ] = (USHORT)( ilink >> REMOTE_BITS );
        remoteBits <<= ( i * REMOTE_BITS ) % 8;
        int remoteMask = ~( REMOTE_BITMASK << ( ( i * REMOTE_BITS ) % 8 ) );
        m_rgb[ ( i * REMOTE_BITS ) / 8 ] &= remoteMask; // zero out old bits
        m_rgb[ ( i * REMOTE_BITS ) / 8 ] |= remoteBits; // write new bits

        Assert( GetLink_( i ) == link );
    }

public:
    SkipListLinkArray() = default;
    SkipListLinkArray( const SkipListLinkArray& ) = default;
    SkipListLinkArray( BYTE* rgb, int cLinks, int iOffset )
        : m_rgb( rgb ), m_cLinks( cLinks ), m_iOffset( iOffset ) {};

    SkipListLinkArray& operator=( const SkipListLinkArray& ) = default;

    SkipListLink operator[]( int i ) const          { return GetLink_( m_iOffset + i ); }
    void SetLink( int i, SkipListLink link )        { SetLink_( m_iOffset + i, link ); }
    static constexpr int CbPreamble( int cLinks )   { return ( cLinks * REMOTE_BITS + 7 ) / 8; }
    static constexpr int Cb( int cLinks )           { return CbPreamble( cLinks ) + cLinks * ( LOCAL_BITS / 8 ); }
};

#include <pshpack1.h>

PERSISTED
struct SkipListNodeHdr
{
    static_assert( FHostIsLittleEndian(), "The union below needs to be fixed for big-endian architectures." );
    union
    {
        struct
        {
            BYTE            fDuplicate          : 1;    // indicates that the next node (at level 0) is a duplicate of this one.
            BYTE            reserved            : 7;    // reserved for future expansion
            BYTE            iLevel              : 4;    // skiplist level of the node
            BBTBuffOpcode   opcode              : 4;
        };
        UnalignedLittleEndian<USHORT>           le_usHeader;
    };

    SkipListNodeHdr()                           = default;
    SkipListNodeHdr( const SkipListNodeHdr& rhs )
                                                { le_usHeader = rhs.le_usHeader; }
    bool IsValid() const                        { return ( reserved == 0 ); }

    const SkipListNodeHdr& operator=( const SkipListNodeHdr& rhs )
    {
        le_usHeader = rhs.le_usHeader;
        return *this;
    }
};

PERSISTED
class SkipListNode
{
    friend class SkipListAsserts;
    friend class BBTBuff;
    template <typename T> friend class BBTBuffWriter;

    static const USHORT     CBKEY_MASK          = 0x0fff;
    static const USHORT     CBKEY_FLAGS_SHIFT   = 12;

    static constexpr USHORT LShiftFlag( SkipListNodeFlags flags ) { return static_cast<USHORT>( flags ) << CBKEY_FLAGS_SHIFT; }

    SkipListNodeHdr                 m_header;
    UnalignedLittleEndian<USHORT>   m_cbKey;
    UnalignedLittleEndian<USHORT>   m_cbData;
    BYTE                            m_rgbNode[ 0 ]; // Variable sized strucures as shown above in the SkipListNode structure comment

    SkipListNode( int level, BBTBuffOpcode opcode, int cbKey, int cbData )
    {
        Assert( level >= 0 && level < MAX_LEVELS );
        Assert( cbKey <= CBKEY_MASK );
        Assert( cbData <= USHRT_MAX );

        m_header.le_usHeader = 0;
        m_header.iLevel = level;
        m_header.opcode = opcode;

        m_cbKey = (USHORT) cbKey;
        m_cbData = (USHORT) cbData;
        memset( m_rgbNode, 0, CbLinks( level ) );
    }

    int IbLinks() const                     { return offsetof( SkipListNode, m_rgbNode ); }
    int IbKey() const                       { return IbLinks() + CbLinks( m_header.iLevel ); }
    int IbData() const                      { return IbKey() + CbKey(); }

    BYTE* PbLinks() const                   { return ( ( (BYTE*) this ) + IbLinks() ); }
    BYTE* PbKey() const                     { return ( ( (BYTE*) this ) + IbKey() ); }
    BYTE* PbData() const                    { return ( ( (BYTE*) this ) + IbData() ); }

    void SetDeleted( bool fDeleted )        { m_cbKey = ( fDeleted ? m_cbKey | LShiftFlag( fSLNDeleted ) : m_cbKey & ( ~LShiftFlag( fSLNDeleted ) ) ); }
    void SetVersioned( bool fVersioned )    { m_cbKey = ( fVersioned ? m_cbKey | LShiftFlag( fSLNVersioned ) : m_cbKey & ( ~LShiftFlag( fSLNVersioned ) ) ); }
    void SetCompressed( bool fCompressed )  { m_cbKey = ( fCompressed ? m_cbKey | LShiftFlag( fSLNCompressed ) : m_cbKey & ( ~LShiftFlag( fSLNCompressed ) ) ); }

public:
    // Node Basics

    bool IsValid() const                    { return ( m_header.IsValid() && CbKey() > 0 && CbKey() <= cbKeyMostMost ); }
    SkipListNodeHdr Header() const          { return m_header; }


    // Skiplist Navigation

    int Level() const                       { return m_header.iLevel; }
    SkipListLink LinkPrev0() const          { return SkipListLinkArray{ PbLinks(), Level() + 2, 0 }[ 0 ]; }
    SkipListLink LinkNext0() const          { return SkipListLinkArray{ PbLinks(), Level() + 2, 0 }[ 1 ]; }
    SkipListLinkArray RgLinksNext() const   { return SkipListLinkArray{ PbLinks(), Level() + 2, 1 }; }  // returns array of next links (LinkPrev0 isn't part of this array)
    bool FDuplicateNext0() const            { return m_header.fDuplicate; }
    void SetLinkPrev0( SkipListLink link )  { SkipListLinkArray{ PbLinks(), Level() + 2, 0 }.SetLink( 0, link ); }
    void SetDuplicateNext0( bool fDup )     { m_header.fDuplicate = fDup; }


    // Node key/data/flags manipulation

    BBTBuffOpcode Opcode() const            { return m_header.opcode; }
    int CbKey() const                       { return ( m_cbKey & CBKEY_MASK ); }
    int CbData() const                      { return m_cbData; }
    int Cb() const                          { return sizeof( SkipListNode ) + CbLinks( m_header.iLevel ) + CbKey() + CbData(); }
    bool FDeleted() const                   { return !!( m_cbKey & LShiftFlag( fSLNDeleted ) ); }
    bool FVersioned() const                 { return !!( m_cbKey & LShiftFlag( fSLNVersioned ) ); }
    bool FCompressed() const                { return !!( m_cbKey & LShiftFlag( fSLNCompressed ) ); }
    SkipListNodeFlags FFlags() const        { return static_cast<SkipListNodeFlags>( m_cbKey >> CBKEY_FLAGS_SHIFT ); }
    KEY Key() const;
    DATA Data() const;

    void CopyKeyDataIntoBuffer( void* pvBuffer, int cbMost ) const;
    void SetOpcode( BBTBuffOpcode opcode );
    void SetNodeKey( const KEY& key );
    void SetNodeData( const DATA& data );
    void SetNodeFlags( SkipListNodeFlags flags );
    int CmpKey( const KEY& keyRhs ) const;


    // Creation/Sizing helpers

    static constexpr int CbLinks( int level )
    {
        // Level 0 nodes contain 2 links: linkPrev0, linkNext0.
        return SkipListLinkArray::Cb( level + 2 );
    }

    static constexpr int Cb( int level, int cbKey, int cbData )
    {
        return sizeof( SkipListNode ) + CbLinks( level ) + cbKey + cbData;
    }

    static SkipListNode* Create( void* pv, int level, BBTBuffOpcode opcode, int cbKey, int cbData )
    {
        return new ( pv ) SkipListNode( level, opcode, cbKey, cbData );
    }
};

class SkipListAsserts
{
    static_assert( std::is_standard_layout<SkipListLink>::value, "SkipListLink should be a standard layout type." );
    static_assert( std::is_standard_layout<SkipListNodeHdr>::value, "SkipListNodeHdr should be a standard layout type." );
    static_assert( std::is_standard_layout<SkipListNode>::value, "SkipListNode should be a standard layout type." );
    static_assert( sizeof( SkipListLink ) == 4, "SkipListLink must be 4 bytes." );
    static_assert( SkipListNode::CBKEY_MASK >= cbKeyMostMost, "m_cbKey is too small for cbKeyMostMost." );
    static_assert( offsetof( SkipListNode, m_cbKey ) == 2, "SkipListNode header must be 2 bytes." );
    static_assert( SkipListNode::Cb( 0, 0, 0 ) == 11, "Minimum SkipListNode overhead must be 11 bytes." );
};

PERSISTED
struct BBTBuffHeader
{
    BYTE                                nVersion;           // bbtbuff format version
    BYTE                                cMaxPages;          // number of physical pages that make up the bbt buff
    UnalignedLittleEndian<SkipListLink> le_ibMicFree;       // offset where the next node is to be inserted
    UnalignedLittleEndian<INT>          le_cbFree;          // total number of free bytes in the buff (can be different from ibMicFree because of deletes)
    UnalignedLittleEndian<INT>          le_cNodes;          // total number of nodes in the bbt buff
    BYTE                                rgbLinks[ SkipListLinkArray::Cb( MAX_LEVELS ) ];    // links to the first node in the bbt buff
    BYTE                                reserved[ 2 ];      // align to 4-bytes
};

// Changing header size is a format breaking change!
// Make sure to handle format compatibility requirements.
static_assert( sizeof( BBTBuffHeader ) == 56, "BBTBuffHeader size changed." );

INLINE SkipListLinkArray RgSkipListLinksHead( BBTBuffHeader* pHeader )
{
    return SkipListLinkArray{ pHeader->rgbLinks, MAX_LEVELS, 0 };
}


#include <poppack.h>
// END persisted structures
///////////////////////////////////////////////////////////////////////////////

struct PageOffsetTuple
{
    USHORT ipg;
    USHORT ibOnPage;
};

struct BBTBuffChangeContext
{
    DBTIME              dbtimeBefore;
    DBTIME              dbtimeCurr;
    SkipListLink        rgLinksPrev[ MAX_LEVELS ];
    INT                 level;
    SkipListLink        linkCurr;
};

class BBTBuff
{
    // Companion classes for transacted writing.
    template <typename T> friend class BBTBuffWriter;
    template <typename T> friend class IBBTBuffTrxLog;
    friend class BBTBuffRedo;
    friend class DbTimeGuard;

    // Debug extensions that need to inspect private state.
    friend std::string DumpBBTBuff( const BBTBuff& bbtBuff, INT level, INT ib = 0 );
    friend LOCAL ERR ErrEDBGDumpBBTBuff_( BBTBuff* pBBTBuffDebuggee, INT level );

private:
    BBTBuffHeader*  m_pHeader           = NULL;
    CSR*            m_pcsrBase          = NULL;         // Base page must stay latched for the lifetime of BBTBuff
    CSR*            m_rgcsrLatched      = NULL;
    SkipListNode*   m_pnodeCurr         = NULL;
    INT             m_ipgCurr           = -1;
    INT             m_cbPageDataMax     = 0;
    BYTE            m_cMaxPages         = 0;            // m_rgcsrLatched is sized based on this
    LATCH           m_latchType         = latchNone;
    BYTE            m_ifmt              = 0xff;

    INT             m_iLevelSeedTestOnly= -1;           // only used in unit testing

    // Needed for latching pages
    PIB*            m_ppib              = NULL;
    IFMP            m_ifmp              = ifmpNil;

public:
    BBTBuff() = default;
    ~BBTBuff();

    // Initialization

    void Load( _In_ PIB* ppib, IFMP ifmp, _In_ CSR* pcsr, _In_ CSRHeapArray& rgcsrBBT, _In_ BBTBuffHeader* pbbtbHeader, LATCH latchType );
    void Unload();
    bool FLoaded() const;


    // Page latch manipulation

    ERR ErrLatchAll();
    ERR ErrWriteLatchAll();
    void Downgrade( LATCH latchType );
    LATCH LatchType() const                 { return m_latchType; }


    // BBT buffer navigation (seeks and moves)

    enum SeekFlags : BYTE
    {
        sfSkipDuplicates = 0,
        sfAllowDuplicates = 1
    };

    ERR ErrSeekLEQ( const KEY& key );
    ERR ErrSeekGEQOldest( const KEY& key );
    ERR ErrMoveFirst( SeekFlags fFlags );
    ERR ErrMoveLast();
    ERR ErrMoveNext( SeekFlags fFlags );
    ERR ErrMovePrev( SeekFlags fFlags );
    ERR ErrMoveToLatestInDuplicateSequence();

    // Gets the ith previous duplicate node of the current (without moving the current).
    // piDup (in/out): ith duplicate node to get. 0 = current node.
    //                 Returns the index of the returned node in the duplicate chain
    //                 e.g. can return a smaller piDup, if the BBTBuff didn't have enough duplicate versions.
    ERR ErrGetDuplicate( _Out_ SkipListNode** ppnodeDup, _Inout_ int* piDup );


    // Cursor manipulation

    void ResetCurr();
    SkipListNode* PnodeCurr()               { return m_pnodeCurr; }
    const SkipListNode* PnodeCurr() const   { return m_pnodeCurr; }
    SkipListLink GetLinkToCurrNode() const;
    ERR ErrSetCurrNodeFromLink( SkipListLink link );


    // Invariant Format

    int CbMaxNodeSize() const               { return m_cbPageDataMax; };
    int Cpg() const                         { Assert( FLoaded() ); return m_cMaxPages; }
    static int IBBTBuffFormatForPage( int cbPage );
    static const BBTBuffFormat* PBBTBuffFormatForPage( int cbPage );
    static int CbMax(); // returns the total size of the BBT buffer


    // Buffer space

    int CNodes() const                      { return m_pHeader->le_cNodes; }
    int CbFree() const                      { return m_pHeader->le_cbFree; }
    int IbFree() const                      { return m_pHeader->le_ibMicFree->ToInt(); }


    // Misc

    bool FCanInsert( const KEY& key, const DATA& data, _Out_ int* pcbReq );
    SkipListNode* PnodeFromLink( SkipListLink link ) const;


    // These operations do unlogged modifications to the BBTBuff.
    // They must be logged externally.

    template <typename TMergeIt, typename TDelIt>
    void MergeAndDelNodes(
        TMergeIt        itNodesMergeBegin,
        const TMergeIt  itNodesMergeEnd,
        TDelIt          itNodesDelBegin,
        const TDelIt    itNodesDelEnd,
        SkipListLink    linkIbMergeStart,
        _Out_ int*      pcNodesMerged,
        _Out_ int*      pcNodesDel );


    // Validation

    bool FIsHeaderValid() const;
    ERR ErrAssertIsLatestInDuplicateSequence( const SkipListNode* pnode );
    ERR ErrAssertIsOldestInDuplicateSequence( const SkipListNode* pnode );
    ERR ErrValidate();


    // BBTBuff page initialization functions

public:
    static void InitBBTBuffRoot( FUCB* pfucb, CSR* pcsr );
    static void InitBBTBuffNonRoot( CSR* pcsr );
    static void GetBBTBuffRoot( const CSR& csr, LINE* pline );
    static BBTBuffHeader* PBBTHeader( const LINE& line );
private:
    static void InitBBTBuffHeader( _Out_ BBTBuffHeader* pbbtbHeader, _In_ int cMaxPages );


private:
    // Format primitives

    const BBTBuffFormat* PFormat() const;
    BYTE* PbPage( int ipg ) const { return (BYTE*) Pcsr( ipg )->Cpage().PvBuffer(); }
    int IbHeader() const;
    int IbPageDataBegin( int ipg ) const { return ( ipg > 0 ? m_pcsrBase->Cpage().CbPageHeader() : IbHeader() + sizeof( BBTBuffHeader ) ); }
    int IbPageDataEnd( int ipg ) const { return ( ipg > 0 ? g_cbPage - CPAGE::CbTagReservedLegacy() : IbHeader() + PFormat()->cbBBTRoot ); }


    // Latch manipulation

    CSR* Pcsr( int ipg ) const { return ( ipg > 0 ? &m_rgcsrLatched[ ipg - 1 ] : m_pcsrBase ); }
    ERR ErrEnsurePageLatched( SkipListLink link, LATCH latchType );
    ERR ErrEnsurePageLatched( int ipgOffset, LATCH latchType );
    void DowngradeLatches();
    void AssertLatchedAll( LATCH latchType );
    void AssertReadyForWrite();


    // SkiplinstLink Translation

    PageOffsetTuple IpgOffsetFromLink( SkipListLink link ) const;
    SkipListLink LinkFromIpgOffset( int ipg, int ibOnPage ) const;
    SkipListNode* PnodeFromIpgOffset( PageOffsetTuple pgOffset ) const;
    ERR ErrPnodeFromLink_AcqLatch( SkipListLink link, SkipListNode** ppnode );


    // Misc

    void ChangeCurr( int ipg, SkipListNode* pnodeCurr );
    ERR ErrGetLatestInDuplicateSequence( SkipListNode* pnodeCurr, _Out_ SkipListLink* plinkLatestDup );
    static int GenLevel( int iSeed );
    int GenLevel();


    // Internal implementation of certain operations

#ifdef ENABLE_JET_UNIT_TEST
    // ErrSeek_() needs unit testing directly
public:
#endif
    enum class SeekMode { LT, LEQ };
    enum class SeekPos  { Prev, Curr };
    ERR ErrSeek_(
        const KEY& key,
        SeekMode seekMode,
        SeekPos seekPos,
        _Out_ int* piResult,
        _Out_ SkipListLink rgLinks[ MAX_LEVELS ],
        _Out_ SkipListNode* rgNodes[ MAX_LEVELS ] );

private:
    SkipListNode* PnodeInsert_(
        const BBTBuffChangeContext& changeCtx,
        SkipListNode* rgNodes[ MAX_LEVELS ],
        BBTBuffOpcode opcode,
        const KEY& key,
        const DATA& data,
        SkipListNodeFlags flags,
        bool fDuplicate );

    void Delete_(
        BBTBuffChangeContext changeCtx,
        SkipListNode* rgNodes[ MAX_LEVELS ],
        const SkipListNode* pnodeToDelete );

    void RangeDelete_( SkipListLink linkFirst, int cNodes );

    template <typename TMergeIt, typename TDelIt>
    void CopyMergeAndDelNodes_(
        BYTE**          rgpbPage,
        TMergeIt        itNodesMergeBegin,
        const TMergeIt  itNodesMergeEnd,
        TDelIt          itNodesDelBegin,
        const TDelIt    itNodesDelEnd,
        SkipListLink    linkIbMergeStart,
        _Out_ int*      pcNodesMerged,
        _Out_ int*      pcNodesDel );

    // Unit test hooks
#ifdef ENABLE_JET_UNIT_TEST
public:
    void SetLevelSeed( int iSeed )                  { m_iLevelSeedTestOnly = iSeed; }
#endif
};

enum CursorLocation : BYTE
{
    clocInvalid = 0,
    clocFailed,
    clocOnTarget,
    clocOnTargetOlderVersion,   // on an older version of the target node
    clocBeforeTarget,
    clocAfterTarget,
    clocBeforeFirst,            // before first node on the page (not on the tree)
    clocAfterLast,              // after last node on the page (not on the tree)
};

INLINE bool FCLocOnNode( CursorLocation cloc )
{
    return ( cloc == clocOnTarget || cloc == clocBeforeTarget || cloc == clocAfterTarget );
}

INLINE bool FCLocValid( CursorLocation cloc )
{
    return ( cloc != clocInvalid && cloc != clocFailed );
}

// BBTBuff Currency Stack Register and associated state.
struct BBTBuffCSR
{
    CSR             csr;
    CSRHeapArray    rgcsrBBT;
    BBTBuff         bbtbuff;
    SkipListLink    link;
    CursorLocation  cursorLoc;

    BBTBuffCSR() : cursorLoc( clocInvalid )
    {
        link.Nullify();
    }

    // Release all latches except base page latch.
    // Caller is responsible for managing base page latch.
    void Release( bool fTossImmediate  = false )
    {
        // Releasing latches, bbtbuff is invalid.
        // But currency is still valid, i.e. link and cursorLoc are still valid.
        bbtbuff.Unload();

        if ( rgcsrBBT != NULL )
        {
            for ( int i = 0; i < rgcsrBBT.CItems(); i++ )
            {
                // All operations on BBTBuff pages must be kept at the same dbtime.
                Assert( !rgcsrBBT[ i ].FLatched() || csr.Dbtime() == rgcsrBBT[ i ].Dbtime() );
                rgcsrBBT[ i ].ReleasePage( fTossImmediate );
            }
        }

        csr.ReleasePage( fTossImmediate );
    }

    void Reset()
    {
        Assert( !bbtbuff.FLoaded() );
        cursorLoc = clocInvalid;
        link.Nullify();
        rgcsrBBT.ForEach( []( CSR& csr ) { csr.Reset(); } );
        csr.Reset();
    }

    bool FValid()
    {
        // UA_TODO: fix validity criteria
        return true;
    }
};


///////////////////////////////////////////////////////////////////////////////
// SkipListNode INLINE methods

INLINE KEY SkipListNode::Key() const
{
    Assert( IsValid() );
    KEY key;
    key.prefix.Nullify();
    key.suffix.SetPv( PbKey() );
    key.suffix.SetCb( CbKey() );
    return key;
}

INLINE DATA SkipListNode::Data() const
{
    Assert( IsValid() );
    DATA data;
    data.SetPv( PbData() );
    data.SetCb( CbData() );
    return data;
}

INLINE void SkipListNode::CopyKeyDataIntoBuffer( void* pvBuffer, int cbMost ) const
{
    Assert( PbData() == PbKey() + CbKey() );    // data must immediately follow the key
    INT cbCopy = min( cbMost, CbKey() + CbData() );
    UtilMemCpy( pvBuffer, PbKey(), cbCopy );
}

INLINE void SkipListNode::SetOpcode( BBTBuffOpcode opcode )
{
    m_header.opcode = opcode;
}

INLINE void SkipListNode::SetNodeKey( const KEY& key )
{
    Assert( key.Cb() == CbKey() );
    key.CopyIntoBuffer( PbKey(), CbKey() );
}

INLINE void SkipListNode::SetNodeData( const DATA& data )
{
    Assert( data.Cb() == CbData() );
    UtilMemCpy( PbData(), data.Pv(), CbData() );
}

INLINE void SkipListNode::SetNodeFlags( SkipListNodeFlags flags )
{
    USHORT usFlags = static_cast<USHORT>( flags ) << CBKEY_FLAGS_SHIFT;
    Assert( 0 == ( usFlags & CBKEY_MASK ) );
    m_cbKey |= usFlags;
}

INLINE int SkipListNode::CmpKey( const KEY& keyRhs ) const
{
    KEY keyLhs;
    keyLhs.prefix.Nullify();
    keyLhs.suffix.SetPv( PbKey() );
    keyLhs.suffix.SetCb( CbKey() );
    return ::CmpKey( keyLhs, keyRhs );
}


///////////////////////////////////////////////////////////////////////////////
// BBTBuff INLINE methods

INLINE const BBTBuffFormat* BBTBuff::PFormat() const
{
    Assert( m_ifmt < _countof( BBTBUFF_FORMAT_CONSTANTS ) );
    return &BBTBUFF_FORMAT_CONSTANTS[ m_ifmt ];
}

INLINE int BBTBuff::IbHeader() const
{
    auto ib = ( (BYTE*) m_pHeader ) - PbPage( 0 );
    Assert( ib < g_cbPage );
    return (int) ib;
}

INLINE PageOffsetTuple BBTBuff::IpgOffsetFromLink( SkipListLink link ) const
{
    Assert( !link.FNull() );
    PageOffsetTuple tuple;
    int ib = link.ToInt();
    ib -= PFormat()->cbBBTRoot;
    if ( ib >= 0 )
    {
        tuple.ipg = (USHORT) ( 1 + ib / m_cbPageDataMax );
        tuple.ibOnPage = (USHORT) ( m_pcsrBase->Cpage().CbPageHeader() + ( ib % m_cbPageDataMax ) ); // SkipListLink::ib = 0 points to BBTBuffHeader
    }
    else
    {
        tuple.ipg = 0;
        tuple.ibOnPage = (USHORT) ( link.ToInt() + IbHeader() );
    }

    Assert( tuple.ibOnPage != 0 );
    return tuple;
}

INLINE SkipListLink BBTBuff::LinkFromIpgOffset( int ipg, int ibOnPage ) const
{
    Assert( ipg < Cpg() );
    Assert( ibOnPage != 0 );

    int ib;
    if ( ipg > 0 )
    {
        ib = PFormat()->cbBBTRoot;
        ib += ( ipg - 1 ) * m_cbPageDataMax;
        ib += ( ibOnPage - m_pcsrBase->Cpage().CbPageHeader() ); // SkipListLink::ib = 0 points to BTBuffHeader
    }
    else
    {
        ib = ibOnPage - IbHeader();
    }

    Assert( !( ib & 0x03 ) );
    return SkipListLink::FromInt( ib );
}

INLINE SkipListNode* BBTBuff::PnodeFromIpgOffset( PageOffsetTuple pgOffset ) const
{
    Assert( Pcsr( pgOffset.ipg )->FLatched() );
    auto pnode = (SkipListNode*) ( PbPage( pgOffset.ipg ) + pgOffset.ibOnPage );
    Assert( pnode->IsValid() );
    return pnode;
}

INLINE SkipListNode* BBTBuff::PnodeFromLink( SkipListLink link ) const
{
    if ( link.FNull() )
    {
        return NULL;
    }

    auto pgOffset = IpgOffsetFromLink( link );
    Assert( Pcsr( pgOffset.ipg )->FLatched() );
    return PnodeFromIpgOffset( pgOffset );
}

INLINE ERR BBTBuff::ErrPnodeFromLink_AcqLatch( SkipListLink link, SkipListNode** ppnode )
{
    ERR err = JET_errSuccess;

    if ( link.FNull() )
    {
        *ppnode = NULL;
        return err;
    }

    auto pgOffset = IpgOffsetFromLink( link );
    if ( !Pcsr( pgOffset.ipg )->FLatched() )
    {
        CallR( ErrEnsurePageLatched( pgOffset.ipg, m_latchType ) );
    }

    *ppnode = PnodeFromIpgOffset( pgOffset );
    return err;
}

INLINE SkipListLink BBTBuff::GetLinkToCurrNode() const
{
    if ( m_pnodeCurr == NULL )
    {
        return SkipListLink::Null();
    }

    BYTE* pbPage = PbPage( m_ipgCurr );
    Assert( m_pnodeCurr->IsValid() );
    Assert( m_ipgCurr >= 0 );
    Assert( Pcsr( m_ipgCurr )->FLatched() && pbPage != NULL );

    BYTE* pb = (BYTE*) m_pnodeCurr;
    EnforceSz( pb >= pbPage, "BBTBuff corruption !" );
    EnforceSz( pb < ( pbPage + g_cbPage ), "BBTBuff corruption !" );
    EnforceSz( pb + m_pnodeCurr->Cb() < ( pbPage + g_cbPage ), "BBTBuff corruption !" );

    auto ibOnPage = pb - pbPage;
    Assert( ibOnPage + m_pnodeCurr->Cb() <= IbPageDataEnd( m_ipgCurr ) );
    SkipListLink link = LinkFromIpgOffset( m_ipgCurr, (int) ibOnPage );
    Assert( link.ToInt() < m_pHeader->le_ibMicFree->ToInt() );
    return link;
}

INLINE void BBTBuff::ResetCurr()
{
    Assert( ( m_ipgCurr < 0 ) == ( m_pnodeCurr == NULL ) );
    m_ipgCurr = -1;
    m_pnodeCurr = NULL;
}

INLINE void BBTBuff::ChangeCurr( int ipg, SkipListNode* pnodeCurr )
{
    Assert( ipg >= 0 && ipg < Cpg() );
    Assert( Pcsr( ipg )->FLatched() );

    BYTE* pb = (BYTE*) pnodeCurr;
    BYTE* pbPage = PbPage( ipg );
    Assert( pb >= pbPage );
    Assert( pb < ( pbPage + g_cbPage ) );
    Assert( pb + pnodeCurr->Cb() < ( pbPage + g_cbPage ) );

    m_ipgCurr = ipg;
    m_pnodeCurr = pnodeCurr;
}

INLINE int BBTBuff::GenLevel( int iSeed )
{
    int level = 0;
    const int SEED_MAX = 1 << ( MAX_LEVELS - 1 );// a max value of 0x7fff will generate a level of 15
    for ( int r = iSeed % SEED_MAX; ( r & 1 ); r >>= 1 )
    {
        level++;
    }

    return level;
}

INLINE int BBTBuff::GenLevel()
{
#ifdef ENABLE_JET_UNIT_TEST
    return GenLevel( m_iLevelSeedTestOnly >= 0 ? m_iLevelSeedTestOnly : rand() );
#else
    return GenLevel( rand() );
#endif
}

INLINE int BBTBuff::IBBTBuffFormatForPage( int cbPage )
{
    Assert( FPowerOf2( cbPage ) );
    unsigned long ulFmt = Log2OfPowerOf2( (unsigned long) cbPage ) - 12;   // 2^12 = 4k, first entry in the format constants table
    const BBTBuffFormat* pFormat = &BBTBUFF_FORMAT_CONSTANTS[ ulFmt ];
    Assert( pFormat->cbCPAGE == cbPage );
    return (int) ulFmt;
}

INLINE const BBTBuffFormat* BBTBuff::PBBTBuffFormatForPage( int cbPage )
{
    return &BBTBUFF_FORMAT_CONSTANTS[ IBBTBuffFormatForPage( cbPage ) ];
}

INLINE int BBTBuff::CbMax()
{
    // BBTBuff doesn't insert tags/ilines. It uses the page data space as one big buffer.
    // But to keep CPAGE checks happy, we can't reclaim the reserved tag.
    const BBTBuffFormat* pFormat = PBBTBuffFormatForPage( g_cbPage );
    return pFormat->cbBBTRoot + ( pFormat->cpgInBBTBuff - 1 ) * ( CPAGE::CbPageDataMaxNoInsert( g_cbPage ) );
}

// Performs an evict operation, deleting required local nodes, while merging external nodes.
// This operation must be externally logged.
template <typename TMergeIt, typename TDelIt>
void BBTBuff::MergeAndDelNodes(
    TMergeIt        itNodesMergeBegin,
    const TMergeIt  itNodesMergeEnd,
    TDelIt          itNodesDelBegin,
    const TDelIt    itNodesDelEnd,
    SkipListLink    linkIbMergeStart,
    _Out_ int*      pcNodesMerged,
    _Out_ int*      pcNodesDel )
{
    AssertReadyForWrite();  // must already be dirtied

    BYTE** rgpbPage = (BYTE**) _alloca( sizeof( BYTE* ) * Cpg() );
    Assert( rgpbPage );

    for ( int i = 0; i < Cpg(); i++ )
    {
        BFAlloc( bfasTemporary, (void**) &rgpbPage[ i ], m_pcsrBase->Cpage().CbPage() );    // can't fail
        Assert( rgpbPage[ i ] );
    }

    CopyMergeAndDelNodes_(
            rgpbPage,
            itNodesMergeBegin,
            itNodesMergeEnd,
            itNodesDelBegin,
            itNodesDelEnd,
            linkIbMergeStart,
            pcNodesMerged,
            pcNodesDel );

    // Copy root page (BBTBuffHeader + any data)
    memcpy(
            PbPage( 0 ) + IbHeader(),
            rgpbPage[ 0 ] + IbHeader(),
            PFormat()->cbRoot );

    // if the last node fits perfectly at the end of the last page, ibMicFree can point to the next page
    int ipgLast = min( Cpg() - 1, IpgOffsetFromLink( m_pHeader->le_ibMicFree ).ipg );
    for ( int i = 1; i <= ipgLast; i++ )
    {
        // Copy back the page at the appropriate offset
        int cbCopy = IbPageDataEnd( i ) - IbPageDataBegin( i );
        memcpy(
                PbPage( i ) + IbPageDataBegin( i ),
                rgpbPage[ i ] + IbPageDataBegin( i ),
                cbCopy );
    }

    // UA_TODO: pattern-fill leftover space in pages

    // Cleanup allocated memory
    for ( int i = 0; i < Cpg(); i++ )
    {
        BFFree( rgpbPage[ i ] );
    }
}

// Makes a copy of the current BBTBuff while deleting specified nodes and merging in as many nodes as can fit in.
// Inputs:
// 1. An array of BYTE* pointers pointing to allocated memory for each page in the BBTBuff.
// 2. A LegacyForwadrIterator specifying merged nodes. The iterator should return pointers to objects that can pass for a SkipListNode.
// 3. A LegacyForwardIterator specifying deleted nodes. The iterator should return on-page addresses of nodes that should be deleted.
// 4. Offset in BBTBuff where the merged nodes are placed.
//
// Outputs:
// 1. Count of nodes merged (can be less than the input).
// 2. Count of nodes deleted.
template <typename TMergeIt, typename TDelIt>
void BBTBuff::CopyMergeAndDelNodes_(
    BYTE**          rgpbPage,
    TMergeIt        itNodesMergeBegin,
    const TMergeIt  itNodesMergeEnd,
    TDelIt          itNodesDelBegin,
    const TDelIt    itNodesDelEnd,
    SkipListLink    linkIbMergeStart,
    _Out_ int*      pcNodesMerged,
    _Out_ int*      pcNodesDel )
{
    using TMergeNode    = std::iterator_traits<TMergeIt>::value_type;
    using TDelItValue   = std::iterator_traits<TDelIt>::value_type;
    enum NodeSource { nsInvalid = 0, nsLocal, nsMerge };
    static_assert( std::is_same<TDelItValue, SkipListLink>::value == true, "TDelIt must return SkipListLink" );

    // The new pages being constructed have to be mem-copyable into the actual cpage. So we need
    // to obey the current strucutre of the pages comprising the BBTBuff.
    // This means that we need to construct the skip list to make sure that it begins at offsets
    // dictated by the current root (for the root page). For the other pages, it needs to obey 
    // the current structure of the page, which for now is just leaving space for the page header.

    // Non-standard, should use std::distance() but requires RandomAccessIterator,
    // that we don't want to support.
    auto               cNodesToMerge = itNodesMergeEnd - itNodesMergeBegin;
    Assert( cNodesToMerge <= INT32_MAX );

    const SkipListLinkArray rgLinksHead = RgSkipListLinksHead( m_pHeader );
    SkipListNode*       rgpNodePrevAtLevel[ MAX_LEVELS ] = { 0 };
    TMergeNode          pnodeMerge = ( itNodesMergeBegin != itNodesMergeEnd ? *itNodesMergeBegin : nullptr );
    SkipListLink        linkDel = ( itNodesDelBegin != itNodesDelEnd ? *itNodesDelBegin : SkipListLink::Null() );
    SkipListLink        linkLocal = rgLinksHead[ 0 ];
    SkipListNode*       pnodeLocal = PnodeFromLink( linkLocal );
    SkipListNode*       pnodePrevNew = NULL;
    int                 cNodes = 0;
    int                 iNodesMerge = 0;
    int                 iNodesDel = 0;
    int                 cbFree = CbMax() - sizeof( BBTBuffHeader );
    SkipListLink        ibCurrLocal = SkipListLink::FromInt( sizeof( BBTBuffHeader ) );  // leave space for the header
    SkipListLink        ibCurrMerge = linkIbMergeStart;
    SkipListLink        linkPrev0( 0 );
    BBTBuffHeader*      pHeader = (BBTBuffHeader*) ( rgpbPage[ 0 ] + IbHeader() );
    SkipListLinkArray   rgLinksHeadNew = RgSkipListLinksHead( pHeader );

    while( pnodeLocal != NULL || pnodeMerge != NULL )
    {
        NodeSource nodeSrcCurr = nsInvalid;
        SkipListLink*       pibCurr = NULL;
        if ( pnodeLocal != NULL && pnodeMerge != NULL )
        {
            int result = pnodeLocal->CmpKey( pnodeMerge->Key() );
            if ( result <= 0 )
            {
                nodeSrcCurr = nsLocal;
                pibCurr = &ibCurrLocal;
            }
            else
            {
                // Evicted nodes from ancestors are more recent, must be inserted later than equal local nodes.
               nodeSrcCurr = nsMerge;
                pibCurr = &ibCurrMerge;
            }
        }
        else if ( pnodeLocal != NULL )
        {
            nodeSrcCurr = nsLocal;
            pibCurr = &ibCurrLocal;
        }
        else
        {
            Assert( pnodeMerge != NULL );
            nodeSrcCurr = nsMerge;
            pibCurr = &ibCurrMerge;
        }

        // Skip if the current node matches a node in delete sequence
        if ( nodeSrcCurr == nsMerge || linkLocal != linkDel )
        {
            // Compute space needed for the current node
            //

            // Re-level deterministically, we can because we are appending sequentially.
            // Remove the effect of merged nodes on local node levels. This is required to generate local nodes
            // with the same size as we calculated earlier and reserved ( ErrReorg_CalcIbMerge() ).
            // Not doing so can result in overrunning our estimate, causing local nodes not to fit.
            // UA_TODO: The skip list will not be optimally laid out. But I don't think it is a problem.
            int level = GenLevel( nodeSrcCurr == nsLocal ? cNodes - iNodesMerge : cNodes );
            BBTBuffOpcode opcodeCurr = ( nodeSrcCurr == nsLocal ? pnodeLocal->Opcode() : pnodeMerge->Opcode() );
            KEY keyCurr = ( nodeSrcCurr == nsLocal ? pnodeLocal->Key() : pnodeMerge->Key() );
            DATA dataCurr = ( nodeSrcCurr == nsLocal ? pnodeLocal->Data() : pnodeMerge->Data() );
            int cbUsed = SkipListLink::Roundup( SkipListNode::Cb( level, keyCurr.Cb(), dataCurr.Cb() ) );   // count wasted space too

            if ( cbUsed > CbMaxNodeSize() )
            {
                // Generate level 0 node to eliminate overhead as much as we can.
                level = 0;
                cbUsed = SkipListLink::Roundup( SkipListNode::Cb( level, keyCurr.Cb(), dataCurr.Cb() ) );
                EnforceSz( cbUsed <= CbMaxNodeSize(), "BBTBuff MergeNodeTooBig" );   // how was this node able to fit earlier?
            }

            SkipListLink ibPrev = *pibCurr; // needed to calcualte wasted space at the end of a page
            PageOffsetTuple pgOffsetCurr = IpgOffsetFromLink( *pibCurr );
            const int cbLeft = ( pgOffsetCurr.ipg < Cpg() ? IbPageDataEnd( pgOffsetCurr.ipg ) - pgOffsetCurr.ibOnPage : 0 );
            if ( cbUsed > cbLeft )
            {
                // Move to new page if current page is full
                pgOffsetCurr.ipg++;
                if ( pgOffsetCurr.ipg >= Cpg() )
                {
                    if ( nodeSrcCurr == nsMerge )
                    {
                        // Ran out of space, can't merge external nodes anymore.
                        // But we still have to merge all the local nodes.
                        // This will leave pseqNodesMerge positioned on the last node to fail merge,
                        // and then will proceed to merge in all local nodes.
                        pnodeMerge = NULL;
                        continue;   // will try merging current pnodeLocal next
                    }
                    else
                    {
                        // This should never hit. We reserve the space needed to copy over local nodes,
                        // and only merge in as many external nodes as can fit.
                        EnforceSz( false, "BBTBuff MergeEvictedOverflow" );
                    }
                }

                Assert( rgpbPage[ pgOffsetCurr.ipg ] != NULL );
                pgOffsetCurr.ibOnPage = (USHORT) IbPageDataBegin( pgOffsetCurr.ipg );
                *pibCurr = LinkFromIpgOffset( pgOffsetCurr.ipg, pgOffsetCurr.ibOnPage );
            }

            // Alloc and copy to new node
            SkipListLink ibNew = *pibCurr;
            pibCurr->Inc( cbUsed );
            cbFree -= ( pibCurr->ToInt() - ibPrev.ToInt() );  // count wasted space because of a page switch, too

            EnforceSz( ibCurrLocal.ToInt() <= linkIbMergeStart.ToInt(), "BBTBuff ReorgLocalNodeOverflow" );

            SkipListNode* pnodeNew = SkipListNode::Create( rgpbPage[ pgOffsetCurr.ipg ] + pgOffsetCurr.ibOnPage, level, opcodeCurr, keyCurr.Cb(), dataCurr.Cb() );
            pnodeNew->SetNodeKey( keyCurr );
            pnodeNew->SetNodeData( dataCurr );
            pnodeNew->SetNodeFlags( nodeSrcCurr == nsLocal ? pnodeLocal->FFlags() : pnodeMerge->FFlags() );

            // Recompute duplicate flag
            Assert( linkPrev0.FNull() == ( pnodePrevNew == NULL ) );
            if ( pnodePrevNew != NULL )
            {
                // Since we are only adding nodes, technically we can only recompute when FDuplicate() is false.
                // But this can be broken if a caller combines duplicate nodes into 1, invalidating duplicate flags of the merge sequence.
                // So always recompute.
                Assert( !pnodePrevNew->FDuplicateNext0() );
                if ( pnodePrevNew->CmpKey( pnodeNew->Key() ) == 0 )
                {
                    pnodePrevNew->SetDuplicateNext0( true );
                }
            }

            // Adjust links
            pnodeNew->SetLinkPrev0( linkPrev0 );
            SkipListLinkArray rgLinksNew = pnodeNew->RgLinksNext();
            for ( int i = 0; i <= level; i++ )
            {
                ( rgpNodePrevAtLevel[ i ] != NULL ? rgpNodePrevAtLevel[ i ]->RgLinksNext() : rgLinksHeadNew )
                    .SetLink( i, ibNew );
                rgpNodePrevAtLevel[ i ] = pnodeNew;
            }

            linkPrev0 = ibNew;
            pnodePrevNew = pnodeNew;
            cNodes++;
        }
        else
        {
            // Node should be deleted.
            // Deleted node was identified by a link comparison. This works because nodes can't move around during evict-merge.
            // Local links/pointers only become invalid after the merge copies back data to BBTBuff.
            Assert( nodeSrcCurr == nsLocal );  // Only local nodes allowed in del sequence.
            ++iNodesDel;
            ++itNodesDelBegin;
            linkDel = ( itNodesDelBegin != itNodesDelEnd ? *itNodesDelBegin : SkipListLink::Null() );
        }

        // MoveNext
        if ( nodeSrcCurr == nsLocal )
        {
            linkLocal = pnodeLocal->LinkNext0();
            pnodeLocal = PnodeFromLink( pnodeLocal->LinkNext0() );
        }
        else
        {
            Assert( nodeSrcCurr == nsMerge );
            ++iNodesMerge;
            ++itNodesMergeBegin;
            pnodeMerge = ( itNodesMergeBegin != itNodesMergeEnd ? *itNodesMergeBegin : nullptr );
        }
    }

    Assert( linkDel.FNull() ); // all deleted nodes should've been matched
    EnforceSz( cNodes == m_pHeader->le_cNodes + iNodesMerge - iNodesDel, "BBTBuffReorg MissingNodes" );
    EnforceSz( ibCurrMerge.ToInt() == CbMax() - cbFree, "BBTBuffReorg BadSpace" );

    // Null-terminate all levels of the new skip list.
    SkipListLink linkNull( 0 );
    for ( int i = 0; i < MAX_LEVELS; i++ )
    {
        ( rgpNodePrevAtLevel[ i ] != NULL ? rgpNodePrevAtLevel[ i ]->RgLinksNext() : rgLinksHeadNew )
            .SetLink( i, linkNull );
    }

    // Fix the new header
    pHeader->nVersion = m_pHeader->nVersion;
    pHeader->cMaxPages = m_pHeader->cMaxPages;
    pHeader->le_cNodes = cNodes;
    pHeader->le_cbFree = cbFree;
    pHeader->le_ibMicFree = ibCurrMerge;

    Assert( !rgLinksHeadNew[ 0 ].FNull() || cNodes == 0 );

    // pHeader->rgSkipListLinksHead has already been fixed up
    // We are done. New re-organized pages are in rgpbPage.
    if ( pcNodesMerged != NULL )
    {
        *pcNodesMerged = iNodesMerge;
    }
    if ( pcNodesDel != NULL )
    {
        *pcNodesDel = iNodesDel;
    }
}
