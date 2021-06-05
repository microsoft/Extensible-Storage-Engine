// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

/*******************************************************************

A physical page consists of the page header, a
data array and, at the end of the page, a TAG array
Each TAG in the TAG array holds the length and location
of one data blob. The upper bits of the size and location
are used to hold the flags associated with the data
blob.

One TAG (TAG 0) is reserved for use as an external
header. The external view of the page does not include
this TAG so we must add an offset to each iLine that
we receive as an argument.

Insertions and deletions from the page may cause space
on the page to become fragmented. When there is enough
space on the page for an insertion or replacement but
not enough contigous space the data blobs on the page
are compacted, moving all the free space to the end of
the data array.

Insert/Replace routines expect there to be enough free
space in the page for their operation.

It is possible to assign one CPAGE to another. e.g.
    CPAGE foo;
    CPAGE bar;
    ...
    foo = bar;

There are two caveats:
*  The destination page (foo) must not be currently attached
   to a page, i.e. It must be new or have had Release*Latch()
   called on it.
*  The source page (bar) is destroyed in the process of copying.
   This is done to keep the semantics simple by ensuring that
   there is only ever one copy of a page, and that every CPAGE
   maps to a unique resource and should be released.
i.e Assignment changes ownership -- like the unique_ptr<T> template

*******************************************************************/

#include "std.hxx"

#include <malloc.h> // required for _alloca()

//  We have moved all globals / statics involving g_cbPage out of
//  cpage, and new usage is variabla non-grata.
//
#undef g_cbPage
#define g_cbPage g_cbPage_CPAGE_NOT_ALLOWED_TO_USE_THIS_VARIABLE


CLimitedEventSuppressor  g_lesCorruptPages;


#ifdef PERFMON_SUPPORT

// Perf counters for doing a page reorganization

PERFInstanceDelayedTotal<> cCPAGEReorganizeData[reorgMax];

LONG LCPAGEOtherReorganizeDataCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        cCPAGEReorganizeData[reorgOther].PassTo( iInstance, pvBuf );
    }
    return 0;
}

LONG LCPAGEFreeSpaceRequestReorganizeDataCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        cCPAGEReorganizeData[reorgFreeSpaceRequest].PassTo( iInstance, pvBuf );
    }
    return 0;
}

LONG LCPAGEPageMoveLoggingReorganizeDataCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        cCPAGEReorganizeData[reorgPageMoveLogging].PassTo( iInstance, pvBuf );
    }
    return 0;
}

LONG LCPAGEDehydrateBufferReorganizeDataCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        cCPAGEReorganizeData[reorgDehydrateBuffer].PassTo( iInstance, pvBuf );
    }
    return 0;
}

PERFInstanceLiveTotal<> cCPAGENewPages;
LONG LCPAGENewPagesCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cCPAGENewPages.PassTo( iInstance, pvBuf );
    return 0;
}

#endif // PERFMON_SUPPORT


//  ================================================================
class ILatchManager
//  ================================================================
{
public:
    virtual ~ILatchManager();

    // asserts
    virtual void AssertPageIsDirty( const BFLatch& ) const = 0;

    // properties
    virtual bool FLoadedPage() const = 0;

    // latch properties
    virtual bool FDirty( const BFLatch& ) const = 0;
#ifdef DEBUG
    virtual bool FReadLatch( const BFLatch& ) const = 0;
    virtual bool FRDWLatch( const BFLatch& ) const = 0;
    virtual bool FWriteLatch( const BFLatch& ) const = 0;
    virtual bool FWARLatch( const BFLatch& ) const = 0;
#endif  //  DEBUG

    //
    virtual BFLatch* PBFLatch( BFLatch * const ) const = 0;

    // updates
    virtual void SetLgposModify( BFLatch * const, const LGPOS, const LGPOS, const TraceContext& tc ) const = 0;
    virtual void Dirty( BFLatch * const, const BFDirtyFlags, const TraceContext& tc ) const = 0;

    // buffer management
    virtual void SetBuffer( BFLatch * const, void * const pvBuffer, const ULONG cbBuffer ) const = 0;
    virtual void AllocateBuffer( BFLatch * const, const ULONG cbBuffer ) const = 0;
    virtual void FreeBuffer( BFLatch * const ) const = 0;
    virtual ULONG CbBuffer( const BFLatch& ) const = 0;
    virtual ULONG CbPage( const BFLatch& ) const = 0;
    virtual void SetBufferSize( BFLatch * const, __in ULONG cbNewSize ) const = 0;
    virtual void MarkAsSuperCold( BFLatch * const ) const = 0;

    // latching functions
    virtual void ReleaseWriteLatch( BFLatch * const, const bool ) const = 0;
    virtual void ReleaseRDWLatch( BFLatch * const, const bool ) const = 0;
    virtual void ReleaseReadLatch( BFLatch * const, const bool ) const = 0;
    virtual ERR ErrUpgradeReadLatchToWriteLatch( BFLatch * const ) const = 0;
    virtual void UpgradeRDWLatchToWriteLatch( BFLatch * const ) const = 0;
    virtual ERR ErrTryUpgradeReadLatchToWARLatch( BFLatch * const ) const = 0;
    virtual void UpgradeRDWLatchToWARLatch( BFLatch * const ) const = 0;
    virtual void DowngradeWriteLatchToRDWLatch( BFLatch * const ) const = 0;
    virtual void DowngradeWriteLatchToReadLatch( BFLatch * const ) const = 0;
    virtual void DowngradeWARLatchToRDWLatch( BFLatch * const ) const = 0;
    virtual void DowngradeWARLatchToReadLatch( BFLatch * const ) const = 0;
    virtual void DowngradeRDWLatchToReadLatch( BFLatch * const ) const = 0;

protected:
    ILatchManager();
};

//  ================================================================
ILatchManager::ILatchManager()
//  ================================================================
{
}

//  ================================================================
ILatchManager::~ILatchManager()
//  ================================================================
{
}

//  ================================================================
class CNullLatchManager : public ILatchManager
//  ================================================================
{
public:
    CNullLatchManager() {}
    virtual ~CNullLatchManager() {}

    // asserts
    virtual void AssertPageIsDirty( const BFLatch& ) const { Fail_(); }

    // properties
    virtual bool FLoadedPage() const
    {
        return false;
    }

    // latch properties
    virtual bool FDirty( const BFLatch& ) const
    {
        Fail_();
        return false;
    }

#ifdef DEBUG
    virtual bool FReadLatch( const BFLatch& ) const
    {
        Fail_();
        return false;
    }
    virtual bool FRDWLatch( const BFLatch& ) const
    {
        Fail_();
        return false;
    }
    virtual bool FWriteLatch( const BFLatch& ) const
    {
        Fail_();
        return false;
    }
    virtual bool FWARLatch( const BFLatch& ) const
    {
        Fail_();
        return false;
    }
#endif // DEBUG

    //
    virtual BFLatch* PBFLatch( BFLatch * const ) const
    {
        Fail_();
        return NULL;
    }

    // updates
    virtual void SetLgposModify( BFLatch * const, const LGPOS, const LGPOS, const TraceContext& tc ) const { Fail_(); }
    virtual void Dirty( BFLatch * const, const BFDirtyFlags, const TraceContext& tc ) const { Fail_(); }

    // buffer management
    virtual void SetBuffer( BFLatch * const pbfl, void * const pvBuffer, const ULONG cbBuffer ) const { Fail_(); }
    virtual void AllocateBuffer( BFLatch * const, const ULONG cbBuffer ) const { Fail_(); }
    virtual void FreeBuffer( BFLatch * const ) const { Fail_(); }
    virtual ULONG CbBuffer( const BFLatch& ) const { Fail_(); return 0; }
    virtual ULONG CbPage( const BFLatch& ) const { Fail_(); return 0; }
    virtual void SetBufferSize( BFLatch * const, __in ULONG cbNewSize ) const { Fail_(); }
    virtual void MarkAsSuperCold( BFLatch * const ) const { Fail_(); }

    // latching functions
    virtual void ReleaseWriteLatch( BFLatch * const, const bool ) const { Fail_(); }
    virtual void ReleaseRDWLatch( BFLatch * const, const bool ) const { Fail_(); }
    virtual void ReleaseReadLatch( BFLatch * const, const bool ) const { Fail_(); }
    virtual ERR ErrUpgradeReadLatchToWriteLatch( BFLatch * const ) const
    {
        Fail_();
        return ErrERRCheck( JET_errInternalError );
    }
    virtual void UpgradeRDWLatchToWriteLatch( BFLatch * const ) const { Fail_(); }
    virtual ERR ErrTryUpgradeReadLatchToWARLatch( BFLatch * const ) const
    {
        Fail_();
        return ErrERRCheck( JET_errInternalError );
    }
    virtual void UpgradeRDWLatchToWARLatch( BFLatch * const ) const { Fail_(); }
    virtual void DowngradeWriteLatchToRDWLatch( BFLatch * const ) const { Fail_(); }
    virtual void DowngradeWriteLatchToReadLatch( BFLatch * const ) const { Fail_(); }
    virtual void DowngradeWARLatchToRDWLatch( BFLatch * const ) const { Fail_(); }
    virtual void DowngradeWARLatchToReadLatch( BFLatch * const ) const { Fail_(); }
    virtual void DowngradeRDWLatchToReadLatch( BFLatch * const ) const { Fail_(); }

private:
    void Fail_() const
    {
        AssertSz( fFalse, "CNullLatchManager" );
    }
};
CNullLatchManager g_nullPageLatchManager;

//  ================================================================
class CBFLatchManager : public ILatchManager
//  ================================================================
{
public:
    CBFLatchManager() {}
    virtual ~CBFLatchManager() {}

    // asserts
    virtual void AssertPageIsDirty( const BFLatch& bfl ) const
    {
        Assert( FBFDirty( &bfl ) != bfdfClean );
    }

    // properties
    virtual bool FLoadedPage() const
    {
        return false;
    }

    // latch properties
    virtual bool FDirty( const BFLatch& bfl ) const
    {
        return bfl.pv && FBFDirty( &bfl );
    }

#ifdef DEBUG
    virtual bool FReadLatch( const BFLatch& bfl ) const
    {
        return !!FBFReadLatched( &bfl );
    }
    virtual bool FRDWLatch( const BFLatch& bfl ) const
    {
        return !!FBFRDWLatched( &bfl );
    }
    virtual bool FWriteLatch( const BFLatch& bfl ) const
    {
        return !!FBFWriteLatched( &bfl );
    }
    virtual bool FWARLatch( const BFLatch& bfl ) const
    {
        return !!FBFWARLatched( &bfl );
    }
#endif  //  DEBUG

    //
    virtual BFLatch* PBFLatch( BFLatch * const pbfl ) const
    {
        return pbfl;
    }

    // updates
    virtual void SetLgposModify( BFLatch * const pbfl, const LGPOS lgposModify, const LGPOS lgposBegin0, const TraceContext& tc ) const
    {
        //  We do not set lgposBegin0 until we update lgposModify.
        //  The reason is that we do not log deferred Begin0 until we issue
        //  the first log operation. Currently the sequence is
        //      Dirty -> Update (first) Op -> Log update Op -> update lgposModify and lgposStart
        //  Since we may not have lgposStart until the deferred begin0 is logged
        //  when the first Log Update Op is issued for this transaction.
        //
        //  During redo, since we do not log update op, so the lgposStart will not
        //  be logged, so we have to do it here (dirty).
        BFSetLgposModify( pbfl, lgposModify );
        BFSetLgposBegin0( pbfl, lgposBegin0, tc );
    }
    virtual void Dirty( BFLatch * const pbfl, const BFDirtyFlags bfdf, const TraceContext& tc ) const
    {
        BFDirty( pbfl, bfdf, tc );
    }

    // buffer management
    virtual void SetBuffer( BFLatch * const, void * const pvBuffer, const ULONG cbBuffer ) const
    {
        AssertSz( fFalse, "Unexpected call to ILatchManager::SetBuffer()" );
    }
    virtual void AllocateBuffer( BFLatch * const, const ULONG cbBuffer ) const
    {
        AssertSz( fFalse, "Unexpected call to ILatchManager::AllocateBuffer()" );
    }
    virtual void FreeBuffer( BFLatch * const ) const {}
    virtual ULONG CbBuffer( const BFLatch& bfl ) const
    {
        const ULONG cbBuffer = CbBFBufferSize( &bfl );
#ifdef DEBUG
        const ULONG cbPage = CbBFPageSize( &bfl );
        Assert( cbBuffer <= cbPage );
#endif

        return cbBuffer;
    }
    virtual ULONG CbPage( const BFLatch& bfl ) const
    {
        const ULONG cbPage = CbBFPageSize( &bfl );
#ifdef DEBUG
        const ULONG cbBuffer = CbBFBufferSize( &bfl );
        Assert( cbBuffer <= cbPage );
#endif

        return cbPage;
    }
    virtual void SetBufferSize( BFLatch * const pbfl, __in ULONG cbNewSize ) const
    {
        BFSetBufferSize( pbfl, cbNewSize );

        Assert( (ULONG)CbBFBufferSize( pbfl ) == cbNewSize );
    }

    virtual void MarkAsSuperCold( BFLatch * const pbfl ) const
    {
        BFMarkAsSuperCold( pbfl );
    }


    // latching functions
    virtual void ReleaseWriteLatch( BFLatch * const pbfl, const bool fTossImmediate ) const
    {
        if ( !fTossImmediate )
        {
            BFWriteUnlatch( pbfl );
        }
        else
        {
            BFDowngradeWriteLatchToReadLatch( pbfl );
            BFRenouncePage( pbfl );
        }
    }
    virtual void ReleaseRDWLatch( BFLatch * const pbfl, const bool fTossImmediate ) const
    {
        if ( !fTossImmediate )
        {
            BFRDWUnlatch( pbfl );
        }
        else
        {
            BFDowngradeRDWLatchToReadLatch( pbfl );
            BFRenouncePage( pbfl );
        }
    }
    virtual void ReleaseReadLatch( BFLatch * const pbfl, const bool fTossImmediate ) const
    {
        if ( !fTossImmediate )
        {
            BFReadUnlatch( pbfl );
        }
        else
        {
            BFRenouncePage( pbfl );
        }
    }
    virtual ERR ErrUpgradeReadLatchToWriteLatch( BFLatch * const pbfl ) const
    {
        return ErrBFUpgradeReadLatchToWriteLatch( pbfl );
    }
    virtual void UpgradeRDWLatchToWriteLatch( BFLatch * const pbfl ) const
    {
        CallS( ErrBFUpgradeRDWLatchToWriteLatch( pbfl ) );
    }
    virtual ERR ErrTryUpgradeReadLatchToWARLatch( BFLatch * const pbfl ) const
    {
        return ErrBFUpgradeReadLatchToWARLatch( pbfl );
    }
    virtual void UpgradeRDWLatchToWARLatch( BFLatch * const pbfl ) const
    {
        CallS( ErrBFUpgradeRDWLatchToWARLatch( pbfl ) );
    }
    virtual void DowngradeWriteLatchToRDWLatch( BFLatch * const pbfl ) const
    {
        BFDowngradeWriteLatchToRDWLatch( pbfl );
    }
    virtual void DowngradeWriteLatchToReadLatch( BFLatch * const pbfl ) const
    {
        BFDowngradeWriteLatchToReadLatch( pbfl );
    }
    virtual void DowngradeWARLatchToRDWLatch( BFLatch * const pbfl ) const
    {
        BFDowngradeWARLatchToRDWLatch( pbfl );
    }
    virtual void DowngradeWARLatchToReadLatch( BFLatch * const pbfl ) const
    {
        BFDowngradeWARLatchToReadLatch( pbfl );
    }
    virtual void DowngradeRDWLatchToReadLatch( BFLatch * const pbfl ) const
    {
        BFDowngradeRDWLatchToReadLatch( pbfl );
    }
};
CBFLatchManager g_bfLatchManager;

//  Normally we use the latch manager (aka buffer manager) to take care of allocation, but
//  occasionally we either want to allocate our own buffer and/or just happen to have the
//  buffer already allocated

//  ================================================================
class CLoadedPageLatchManager : public ILatchManager
//  ================================================================
{
public:
    CLoadedPageLatchManager() {}
    virtual ~CLoadedPageLatchManager() {}

    // asserts
    virtual void AssertPageIsDirty( const BFLatch& ) const {}

    // properties
    virtual bool FLoadedPage() const { return true; }

    // latch properties
    virtual bool FDirty( const BFLatch& ) const { return true; }

#ifdef DEBUG
    virtual bool FReadLatch( const BFLatch& ) const { return true; }
    virtual bool FRDWLatch( const BFLatch& ) const { return true; }
    virtual bool FWriteLatch( const BFLatch& ) const { return true; }
    virtual bool FWARLatch( const BFLatch& ) const { return true; }
#endif  //  DEBUG

    //
    virtual BFLatch* PBFLatch( BFLatch * const ) const
    {
        AssertSz( fFalse, "Don't get the latch on a loaded page" );
        return NULL;
    }

    // updates
    virtual void SetLgposModify( BFLatch * const, const LGPOS, const LGPOS, const TraceContext& tc ) const {}
    virtual void Dirty( BFLatch * const, const BFDirtyFlags, const TraceContext& tc ) const {}

    // buffer management
protected:
    void SetBufferSizeInfo( BFLatch * const pbfl, __in ULONG cbNewSize ) const
    {
#ifdef DEBUG
        ULONG cbPageOrig = CbPage( *pbfl );
#endif

        //  Constraints required to use the dwContext the way we do.

        Assert( cbNewSize <= 0xFFFF );

        pbfl->dwContext = ( DWORD_PTR( cbNewSize ) << 16 ) | ( DWORD_PTR( 0x0000FFFF ) & pbfl->dwContext );

        Assert( CbPage( *pbfl ) == cbPageOrig );
        Assert( CbBuffer( *pbfl ) == cbNewSize );
    }
#pragma warning ( disable : 4481 )
    virtual void SetBuffer( BFLatch * const pbfl, void * const pvBuffer, const ULONG cbBuffer ) const sealed
    {
        //  Two constraints required to use the dwContext the way we do.

        Assert( cbBuffer <= 0xFFFF );
        Assert( 0 == ( cbBuffer & 0x1 ) );

        pbfl->pv = pvBuffer;

        //  The buffer size in the high short, the true page size in the low short and a 0x1 as
        //  pointers from the buffer manager are always at least 4 byte aligned and we can then
        //  differentiate

        pbfl->dwContext = cbBuffer | 0x1;

        SetBufferSizeInfo( pbfl, cbBuffer );

        Assert( CbPage( *pbfl ) == cbBuffer );
        Assert( CbBuffer( *pbfl ) == cbBuffer );
    }
#pragma warning ( default : 4481 )
    virtual void AllocateBuffer( BFLatch * const, const ULONG cbBuffer ) const {}
    virtual void FreeBuffer( BFLatch * const ) const {}
private:
    INLINE ULONG CbBuffer_( const BFLatch& bfl ) const
    {
        //  high short is buffer size, see SetBuffer().

        return ( 0xFFFF0000 & bfl.dwContext ) >> 16;
    }
public:
    virtual ULONG CbBuffer( const BFLatch& bfl ) const
    {
        Assert( CbBuffer_( bfl ) <= CbPage( bfl ) );

        return CbBuffer_( bfl );
    }
#pragma warning ( disable : 4481 )
    virtual ULONG CbPage( const BFLatch& bfl ) const sealed
    {
        //  low short is buffer size, see SetBuffer().

        ULONG cbPage = 0xFFFE & bfl.dwContext;

        Assert( CbBuffer_( bfl ) <= cbPage );

        return cbPage;
    }
#pragma warning ( default : 4481 )
public:
    virtual void SetBufferSize( BFLatch * const pbfl, __in ULONG cbNewSize ) const
    {
#ifdef DEBUG
        ULONG cbPageOrig = CbPage( *pbfl );
#endif

        SetBufferSizeInfo( pbfl, cbNewSize );

        Assert( CbPage( *pbfl ) == cbPageOrig );
        Assert( CbBuffer( *pbfl ) == cbNewSize );
    }

    virtual void MarkAsSuperCold( BFLatch * const pbfl ) const
    {
        // nothing to do
    }

    // latching functions
    virtual void ReleaseWriteLatch( BFLatch * const, const bool ) const {}
    virtual void ReleaseRDWLatch( BFLatch * const, const bool ) const {}
    virtual void ReleaseReadLatch( BFLatch * const, const bool ) const {}
    virtual ERR ErrUpgradeReadLatchToWriteLatch( BFLatch * const ) const
    {
        return JET_errSuccess;
    }
    virtual void UpgradeRDWLatchToWriteLatch( BFLatch * const ) const {}
    virtual ERR ErrTryUpgradeReadLatchToWARLatch( BFLatch * const ) const
    {
        return JET_errSuccess;
    }
    virtual void UpgradeRDWLatchToWARLatch( BFLatch * const ) const {}
    virtual void DowngradeWriteLatchToRDWLatch( BFLatch * const ) const {}
    virtual void DowngradeWriteLatchToReadLatch( BFLatch * const ) const {}
    virtual void DowngradeWARLatchToRDWLatch( BFLatch * const ) const {}
    virtual void DowngradeWARLatchToReadLatch( BFLatch * const ) const {}
    virtual void DowngradeRDWLatchToReadLatch( BFLatch * const ) const {}

    bool FIsLoadedPage( BFLatch * const pbfl ) const
    {
        //  the low bit is set for loaded pages, see SetBuffer().

        return ( pbfl->dwContext & 0x1 );
    }

};
CLoadedPageLatchManager g_loadedPageLatchManager;

//  ================================================================
class CAllocatedLoadedPageLatchManager : public CLoadedPageLatchManager
//  ================================================================
{
public:
    CAllocatedLoadedPageLatchManager() {}
    virtual ~CAllocatedLoadedPageLatchManager() {}

    virtual void AllocateBuffer( BFLatch * const pbfl, const ULONG cbBuffer ) const
    {
        void * pvBuffer;
        BFAlloc( bfasForDisk, &pvBuffer, cbBuffer );
        SetBuffer( pbfl, pvBuffer, cbBuffer );
    }
    virtual void FreeBuffer( BFLatch * const pbfl ) const
    {
        BFFree( pbfl->pv );
        pbfl->pv        = NULL;
        pbfl->dwContext = NULL;
    }
};
CAllocatedLoadedPageLatchManager g_allocatedPageLatchManager;


//  ================================================================
class CTestLatchManager : public CLoadedPageLatchManager
//  ================================================================
{
public:
    CTestLatchManager() {}
    virtual ~CTestLatchManager() {}

    const static ULONG s_cbProtectedArea = 64 * 1024;

    virtual void AllocateBuffer( BFLatch * const pbfl, const ULONG cbBuffer ) const
    {
        void * pvBuffer = NULL;

        Expected( s_cbProtectedArea == OSMemoryPageReserveGranularity() );
        Assert( s_cbProtectedArea % OSMemoryPageCommitGranularity() == 0 );
        Assert( cbBuffer < OSMemoryPageReserveGranularity() );

        //  This function allocates a "test page" buffer that has a guard page on both the front
        //  and back of the buffer ... this allows us to AV on buffer over/under runs.

        const ULONG cpgBufferCommit = roundup( cbBuffer, OSMemoryPageCommitGranularity() ) / OSMemoryPageCommitGranularity();
        const ULONG cbOverSizedBuffer = 2 * s_cbProtectedArea + cpgBufferCommit * OSMemoryPageCommitGranularity();
        const ULONG cbOverOverSizedBuffer = roundup( cbOverSizedBuffer, OSMemoryPageReserveGranularity() );

        do {
            void * pvT = (BYTE*)PvOSMemoryPageReserve( cbOverOverSizedBuffer, NULL );

            if ( pvT )
            {
                if( FOSMemoryPageCommit( (BYTE*)pvT + s_cbProtectedArea,
                                            cpgBufferCommit * OSMemoryPageCommitGranularity() ) )
                {
                    pvBuffer = (BYTE*)pvT + s_cbProtectedArea;
                }
                else
                {
                    OSMemoryPageFree( pvT );
                }
            }

        }
        while( NULL == pvBuffer );

        SetBuffer( pbfl, pvBuffer, cbBuffer );

        Assert( cbBuffer == CbPage( *pbfl ) );
        Assert( cbBuffer == CbBuffer( *pbfl ) );
    }
    virtual void FreeBuffer( BFLatch * const pbfl ) const
    {
        OSMemoryPageDecommit( pbfl->pv, CbBuffer( *pbfl ) );
        OSMemoryPageFree( (BYTE*)(pbfl->pv) - s_cbProtectedArea );
        pbfl->pv        = NULL;
        pbfl->dwContext = NULL;
    }

    virtual void SetBufferSize( BFLatch * const pbfl, __in ULONG cbNewSize ) const
    {
#ifdef DEBUG
        ULONG cbPageOrig = CbPage( *pbfl );
#endif
        Assert( cbNewSize <= CbPage( *pbfl ) );

        if ( cbNewSize > CbBuffer( *pbfl ) )
        {
            //  Grow the buffer.

            const ULONG cbGrowth = cbNewSize - CbBuffer( *pbfl );

            Assert( cbGrowth >= OSMemoryPageCommitGranularity() );
            Assert( 0 == ( cbGrowth % OSMemoryPageCommitGranularity() ) );

            //  When we set the grow, we have to commit the new space

            while( !FOSMemoryPageCommit( (BYTE*)(pbfl->pv) + CbBuffer( *pbfl ), cbGrowth ) )
            {
            }
        }
        else if ( cbNewSize < CbBuffer( *pbfl ) )
        {
            //  Shrink the buffer.

            const ULONG cbShrinkage = CbBuffer( *pbfl ) - cbNewSize;

            Assert( cbShrinkage >= OSMemoryPageCommitGranularity() );
            Assert( 0 == ( cbShrinkage % OSMemoryPageCommitGranularity() ) );

            //  When we shrink, we decommit to force AVs

            OSMemoryPageDecommit( (BYTE*)(pbfl->pv) + cbNewSize, cbShrinkage );
        }

        SetBufferSizeInfo( pbfl, cbNewSize );

        Assert( CbPage( *pbfl ) == cbPageOrig );
        Assert( CbBuffer( *pbfl ) == cbNewSize );
    }

};
CTestLatchManager g_testPageLatchManager;

//
//  TAG Comparator interfaces
//

//  =================================================================
BOOL CmpPtagIbSmall( __in const CPAGE::TAG * const ptag1, __in const CPAGE::TAG * const ptag2 )
//  =================================================================
{
    return ptag1->Ib( fTrue /* fSmallFormat */ ) < ptag2->Ib( fTrue /* fSmallFormat */ );
}

//  =================================================================
BOOL CmpPtagIbLarge( __in const CPAGE::TAG * const ptag1, __in const CPAGE::TAG * const ptag2 )
//  =================================================================
{
    return ptag1->Ib( fFalse /* fSmallFormat */ ) < ptag2->Ib( fFalse /* fSmallFormat */ );
}

//  =================================================================
CPAGE::PfnCmpTAG CPAGE::PfnCmpPtagIb( const BOOL fSmallFormat )
//  =================================================================
{
    return ( fSmallFormat ? CmpPtagIbSmall : CmpPtagIbLarge );
}

//  =================================================================
CPAGE::PfnCmpTAG CPAGE::PfnCmpPtagIb( __in ULONG cbPage )
//  =================================================================
{
    return CPAGE::PfnCmpPtagIb( FIsSmallPage( cbPage ) );
}

//  =================================================================
CPAGE::PfnCmpTAG CPAGE::PfnCmpPtagIb() const
//  =================================================================
{
    return CPAGE::PfnCmpPtagIb( m_platchManager->CbPage( m_bfl ) );
}


//
//  Private diagnostic helpers
//

//  ================================================================
CPageValidationLogEvent::CPageValidationLogEvent( const IFMP ifmp, const INT logflags, const CategoryId category ) :
//  ================================================================
    m_ifmp( ifmp ),
    m_logflags( logflags ),
    m_category( category )
{
}

//  ================================================================
CPageValidationLogEvent::~CPageValidationLogEvent()
//  ================================================================
{
}

//  ================================================================
void CPageValidationLogEvent::BadChecksum( const PGNO pgno, const ERR err, const PAGECHECKSUM checksumStoredInHeader, const PAGECHECKSUM checksumComputedOffData )
//  ================================================================
{
    if( m_logflags & LOG_BAD_CHECKSUM )
    {
        ReportBadChecksum_( pgno, err, checksumStoredInHeader, checksumComputedOffData );

        if( !g_rgfmp[m_ifmp].FReadOnlyAttach() )
        {
            PdbfilehdrReadWrite pdbfilehdr = g_rgfmp[m_ifmp].PdbfilehdrUpdateable();

            pdbfilehdr->le_ulBadChecksum++;
            LGIGetDateTime( &(pdbfilehdr->logtimeBadChecksum) );
        }
    }
}

//  ================================================================
void CPageValidationLogEvent::UninitPage( const PGNO pgno, const ERR err )
//  ================================================================
{
    if( m_logflags & LOG_UNINIT_PAGE )
    {
        ReportUninitializedPage_( pgno, err );
    }
}

//  ================================================================
void CPageValidationLogEvent::BadPgno( const PGNO pgnoExpected, const PGNO pgnoStoredInHeader, const ERR err )
//  ================================================================
{
    if( m_logflags & LOG_BAD_PGNO )
    {
        ReportPageNumberFailed_( pgnoExpected, err, pgnoStoredInHeader );
    }
}

//  ================================================================
void CPageValidationLogEvent::BitCorrection( const PGNO pgno, const INT ibitCorrupted )
//  ================================================================
{
    if( m_logflags & LOG_CORRECTION )
    {
        ReportPageCorrection_( pgno, ibitCorrupted );

        if( !g_rgfmp[m_ifmp].FReadOnlyAttach() )
        {
            PdbfilehdrReadWrite pdbfilehdr = g_rgfmp[m_ifmp].PdbfilehdrUpdateable();

            pdbfilehdr->le_ulECCFixSuccess++;
            LGIGetDateTime( &(pdbfilehdr->logtimeECCFixSuccess) );
        }
    }
}

//  ================================================================
void CPageValidationLogEvent::BitCorrectionFailed( const PGNO pgno, const ERR err, const INT ibitCorrupted )
//  ================================================================
{
    if( m_logflags & LOG_CHECKFAIL )
    {
        //  the error was corrected, but page checks failed
        ReportPageCorrectionFailed_( pgno, err, ibitCorrupted );

        if( !g_rgfmp[m_ifmp].FReadOnlyAttach() )
        {
            PdbfilehdrReadWrite pdbfilehdr = g_rgfmp[m_ifmp].PdbfilehdrUpdateable();

            pdbfilehdr->le_ulECCFixFail++;
            LGIGetDateTime( &(pdbfilehdr->logtimeECCFixFail) );
        }
    }
}

#define ENABLE_LOST_FLUSH_INSTRUMENTATION

//  ================================================================
void CPageValidationLogEvent::LostFlush(
    const PGNO pgno,
    const FMPGNO fmpgno,
    const INT pgftExpected,
    const INT pgftActual,
    const ERR err,
    const BOOL fRuntime,
    const BOOL fFailOnRuntimeOnly )
//  ================================================================
{
    if ( m_logflags & LOG_LOST_FLUSH )
    {
        BOOL fInCache           = fFalse;
        ERR errBF               = JET_errSuccess;
        BFDirtyFlags bfdf       = bfdfMin;

        BFCacheStatus( m_ifmp, pgno, &fInCache, &errBF, &bfdf );

        WCHAR wszLostFlushContext[64];
        if ( fInCache )
        {
            OSStrCbFormatW(
                wszLostFlushContext,
                sizeof( wszLostFlushContext ),
                L"InCache:%I32u:%d:%d",
                m_category,
                errBF,
                (INT)bfdf );
        }
        else
        {
            OSStrCbFormatW(
                wszLostFlushContext,
                sizeof( wszLostFlushContext ),
                L"NotInCache:%I32u",
                m_category );
        }

        ReportLostFlush_( pgno, fmpgno, pgftExpected, pgftActual, err, fRuntime, fFailOnRuntimeOnly, wszLostFlushContext );
    }

#ifdef ENABLE_LOST_FLUSH_INSTRUMENTATION
    // Do not assert at this point if we're bypassing the cache because there is extra instrumentation there
    // to help us diagnose unexplained lost flushes during the test pass.
    Assert( FNegTest( fCorruptingWithLostFlush ) || ( m_category != BUFFER_MANAGER_CATEGORY ) );
#else
    Assert( FNegTest( fCorruptingWithLostFlush ) );
#endif
}


#define REPORT_PROLOG   \
    IFileAPI * const    pfapi       = g_rgfmp[ m_ifmp ].Pfapi();  \
    const WCHAR*    rgpsz[ 9 ] = { NULL }; \
    DWORD           irgpsz      = 0;    \
    WCHAR           szAbsPath[ IFileSystemAPI::cchPathMax ];    \
    WCHAR           szOffset[ 64 ]; \
    WCHAR           szLength[ 64 ]; \
    WCHAR           szError[ 64 ];  \
    CallS( pfapi->ErrPath( szAbsPath ) );   \
    OSStrCbFormatW( szOffset, sizeof( szOffset ), L"%I64i (0x%016I64x)", OffsetOfPgno( pgno ), OffsetOfPgno( pgno ) );  \
    OSStrCbFormatW( szLength, sizeof( szLength ), L"%u (0x%08x)", g_rgfmp[m_ifmp].CbPage(), g_rgfmp[m_ifmp].CbPage() ); \
    OSStrCbFormatW( szError, sizeof( szError ), L"%i (0x%08x)", err, err ); \
    rgpsz[ irgpsz++ ]   = szAbsPath;    \
    rgpsz[ irgpsz++ ]   = szOffset; \
    rgpsz[ irgpsz++ ]   = szLength; \
    rgpsz[ irgpsz++ ]   = szError;

//  ================================================================
VOID CPageValidationLogEvent::ReportBadChecksum_(
    const PGNO pgno,
    const ERR err,
    const PAGECHECKSUM checksumStoredInHeader,
    const PAGECHECKSUM checksumComputedOffData ) const
//  ================================================================
{
    REPORT_PROLOG;

    Assert( 4 == cxeChecksumPerPage );
    const WCHAR* const wszFormat = FIsSmallPage() ? L"[%016I64x]" : L"[%016I64x:%016I64x:%016I64x:%016I64x]";

    const XECHECKSUM* const pExp = checksumStoredInHeader.rgChecksum;
    WCHAR   szChecksumStoredInHeader[ 80 ];
    OSStrCbFormatW( szChecksumStoredInHeader, sizeof( szChecksumStoredInHeader ), wszFormat, pExp[ 0 ], pExp[ 1 ], pExp[ 2 ], pExp[ 3 ] );

    const XECHECKSUM* const pAct = checksumComputedOffData.rgChecksum;
    WCHAR   szChecksumComputedOffData[ 80 ];
    OSStrCbFormatW( szChecksumComputedOffData, sizeof( szChecksumComputedOffData ), wszFormat, pAct[ 0 ], pAct[ 1 ], pAct[ 2 ], pAct[ 3 ] );

    WCHAR   szPageNumber[ 32 ];
    OSStrCbFormatW( szPageNumber, sizeof( szPageNumber ), L"%d (0x%X)", pgno, pgno );

    rgpsz[ irgpsz++ ]   = szChecksumStoredInHeader;
    rgpsz[ irgpsz++ ]   = szChecksumComputedOffData;
    rgpsz[ irgpsz++ ]   = szPageNumber;

    UtilReportEvent(    eventError,
                        m_category,
                        DATABASE_PAGE_CHECKSUM_MISMATCH_ID,
                        irgpsz,
                        rgpsz,
                        0,
                        NULL,
                        PinstFromIfmp( m_ifmp ) );

    OSUHAPublishEvent(
        HaDbFailureTagRepairable, PinstFromIfmp( m_ifmp ), Ese2HaId( m_category ),
        HaDbIoErrorNone, szAbsPath, OffsetOfPgno( pgno ), g_rgfmp[m_ifmp].CbPage(),
        HA_DATABASE_PAGE_CHECKSUM_MISMATCH_ID, irgpsz, rgpsz );
}

//  ================================================================
VOID CPageValidationLogEvent::ReportUninitializedPage_( const PGNO pgno, const ERR err ) const
//  ================================================================
{
    REPORT_PROLOG;

    WCHAR   szPageNumber[ 64 ];

    OSStrCbFormatW( szPageNumber, sizeof(szPageNumber), L"%d (0x%X)", pgno, pgno );
    rgpsz[ irgpsz++ ]   = szPageNumber;

    UtilReportEvent(    eventError,
                        m_category,
                        DATABASE_PAGE_DATA_MISSING_ID,
                        irgpsz,
                        rgpsz,
                        0,
                        NULL,
                        PinstFromIfmp( m_ifmp ) );

    OSUHAPublishEvent(
        HaDbFailureTagCorruption, PinstFromIfmp( m_ifmp ), Ese2HaId( m_category ),
        HaDbIoErrorNone, szAbsPath, OffsetOfPgno( pgno ), g_rgfmp[m_ifmp].CbPage(),
        HA_DATABASE_PAGE_DATA_MISSING_ID, irgpsz, rgpsz );
}

//  ================================================================
VOID CPageValidationLogEvent::ReportPageCorrectionFailed_( const PGNO pgno, const ERR err, const INT ibitCorrupted ) const
//  ================================================================
{
    REPORT_PROLOG;

    WCHAR           szIbitCorrupted[ 64 ];
    WCHAR           szPageNumber[ 64 ];

    OSStrCbFormatW( szIbitCorrupted, sizeof(szIbitCorrupted), L"%d", ibitCorrupted );
    OSStrCbFormatW( szPageNumber, sizeof(szPageNumber), L"%d (0x%X)", pgno, pgno );

    rgpsz[ irgpsz++ ]   = szIbitCorrupted;
    rgpsz[ irgpsz++ ]   = szPageNumber;


    UtilReportEvent(    eventError,
                        m_category,
                        DATABASE_PAGE_CHECK_FAILED_ID,
                        irgpsz,
                        rgpsz,
                        0,
                        NULL,
                        PinstFromIfmp( m_ifmp ) );

    OSUHAPublishEvent(
        HaDbFailureTagRepairable, PinstFromIfmp( m_ifmp ), Ese2HaId( m_category ),
        HaDbIoErrorNone, szAbsPath, OffsetOfPgno( pgno ), g_rgfmp[m_ifmp].CbPage(),
        HA_DATABASE_PAGE_CHECK_FAILED_ID, irgpsz, rgpsz );
}

//  ================================================================
VOID CPageValidationLogEvent::ReportPageNumberFailed_( const PGNO pgno, const ERR err, const PGNO pgnoStoredInHeader ) const
//  ================================================================
{
    REPORT_PROLOG;

    WCHAR wszPgnoExpected[ 64 ];
    WCHAR wszPgnoStoredInHeader[ 64 ];

    OSStrCbFormatW( wszPgnoExpected, sizeof( wszPgnoExpected ), L"%u (0x%08x)", pgno, pgno );
    OSStrCbFormatW( wszPgnoStoredInHeader, sizeof( wszPgnoStoredInHeader ), L"%d (0x%X)", pgnoStoredInHeader, pgnoStoredInHeader );

    rgpsz[ irgpsz++ ]   = wszPgnoExpected;
    rgpsz[ irgpsz++ ]   = wszPgnoStoredInHeader;

    UtilReportEvent(    eventError,
                        m_category,
                        DATABASE_PAGE_NUMBER_MISMATCH_ID,
                        irgpsz,
                        rgpsz,
                        0,
                        NULL,
                        PinstFromIfmp( m_ifmp ) );

    OSUHAPublishEvent(
        HaDbFailureTagRepairable, PinstFromIfmp( m_ifmp ), Ese2HaId( m_category ),
        HaDbIoErrorNone, szAbsPath, OffsetOfPgno( pgno ), g_rgfmp[m_ifmp].CbPage(),
        HA_DATABASE_PAGE_NUMBER_MISMATCH_ID, irgpsz, rgpsz );
}

//  ================================================================
VOID CPageValidationLogEvent::ReportPageCorrection_( const PGNO pgno, const INT ibitCorrupted ) const
//  ================================================================
{
    //  we don't log an error, so don't use the REPORT_PROLOG macro

    IFileAPI*       pfapi       = g_rgfmp[ m_ifmp ].Pfapi();
    const WCHAR*    rgpsz[ 5 ];
    DWORD           irgpsz      = 0;
    WCHAR           szAbsPath[ IFileSystemAPI::cchPathMax ];
    WCHAR           szOffset[ 64 ];
    WCHAR           szLength[ 64 ];
    WCHAR           szIbitCorrupted[ 64 ];
    WCHAR           szPageNumber[ 64 ];

    CallS( pfapi->ErrPath( szAbsPath ) );
    OSStrCbFormatW( szOffset, sizeof(szOffset), L"%I64i (0x%016I64x)", OffsetOfPgno( pgno ), OffsetOfPgno( pgno ) );
    OSStrCbFormatW( szLength, sizeof(szLength), L"%u (0x%08x)", g_rgfmp[m_ifmp].CbPage(), g_rgfmp[m_ifmp].CbPage() );
    OSStrCbFormatW( szIbitCorrupted, sizeof(szIbitCorrupted), L"%d", ibitCorrupted );
    OSStrCbFormatW( szPageNumber, sizeof(szPageNumber), L"%d (0x%X)", pgno, pgno );

    rgpsz[ irgpsz++ ]   = szAbsPath;
    rgpsz[ irgpsz++ ]   = szOffset;
    rgpsz[ irgpsz++ ]   = szLength;
    rgpsz[ irgpsz++ ]   = szIbitCorrupted;
    rgpsz[ irgpsz++ ]   = szPageNumber;


    UtilReportEvent(    eventWarning,
                        m_category,
                        DATABASE_PAGE_CHECKSUM_CORRECTED_ID,
                        irgpsz,
                        rgpsz,
                        0,
                        NULL,
                        PinstFromIfmp( m_ifmp ) );

    OSUHAPublishEvent(
        HaDbFailureTagRepairable, PinstFromIfmp( m_ifmp ), Ese2HaId( m_category ),
        HaDbIoErrorNone, szAbsPath, OffsetOfPgno( pgno ), g_rgfmp[m_ifmp].CbPage(),
        HA_DATABASE_PAGE_CHECKSUM_CORRECTED_ID, irgpsz, rgpsz );
}

//  ================================================================
VOID CPageValidationLogEvent::ReportLostFlush_(
    const PGNO pgno,
    const FMPGNO fmpgno,
    const INT pgftExpected,
    const INT pgftActual,
    const ERR err,
    const BOOL fRuntime,
    const BOOL fFailOnRuntimeOnly,
    const WCHAR* const wszContext ) const
//  ================================================================
{
    REPORT_PROLOG;
    WCHAR wszDbPageNumber[ 64 ];
    WCHAR wszDbFlushState[ 8 ];
    WCHAR wszFmPageNumber[ 64 ];
    WCHAR wszFmFlushState[ 8 ];

    OSStrCbFormatW( wszDbPageNumber, sizeof(wszDbPageNumber), L"%u (0x%X)", pgno, pgno );
    rgpsz[ irgpsz++ ] = wszDbPageNumber;
    OSStrCbFormatW( wszDbFlushState, sizeof(wszDbFlushState), L"%d", pgftActual );
    rgpsz[ irgpsz++ ] = wszDbFlushState;
    OSStrCbFormatW( wszFmPageNumber, sizeof(wszFmPageNumber), L"%d (0x%X)", fmpgno, fmpgno );
    rgpsz[ irgpsz++ ] = wszFmPageNumber;
    OSStrCbFormatW( wszFmFlushState, sizeof(wszFmFlushState), L"%d", pgftExpected );
    rgpsz[ irgpsz++ ] = wszFmFlushState;
    Expected( wszContext != NULL );
    rgpsz[ irgpsz++ ] = wszContext ? wszContext : L"Undefined";

    UtilReportEvent(    eventError,
                        m_category,
                        fRuntime ? DATABASE_PAGE_LOST_FLUSH_ID : DATABASE_PAGE_PERSISTED_LOST_FLUSH_ID,
                        irgpsz,
                        rgpsz,
                        0,
                        NULL,
                        PinstFromIfmp( m_ifmp ) );

    if ( fRuntime || !fFailOnRuntimeOnly )
    {
        OSUHAPublishEvent(
            HaDbFailureTagLostFlushDetected, PinstFromIfmp( m_ifmp ), Ese2HaId( m_category ),
            HaDbIoErrorNone, szAbsPath, OffsetOfPgno( pgno ), g_rgfmp[m_ifmp].CbPage(),
            HA_DATABASE_PAGE_LOST_FLUSH_ID, irgpsz, rgpsz );
    }
}


//  ****************************************************************
//  TAG FUNCTIONS
//  ****************************************************************

//
//  Return the cb of a TAG, masking out the flags stored in the high
//  order bits.
//
INLINE USHORT CPAGE::TAG::Cb( __in const BOOL fSmallFormat ) const
{
    if ( fSmallFormat )
    {
        return ( USHORT )(cb_) & mskSmallTagIbCb;
    }
    else
    {
        return ( USHORT )(cb_) & mskLargeTagIbCb;
    }
}

//
//  Return the ib of a TAG, masking out the flags stored in the high
//  order bits.
//
INLINE USHORT CPAGE::TAG::Ib( __in const BOOL fSmallFormat ) const
{
    if ( fSmallFormat )
    {
        return ( USHORT )(ib_) & mskSmallTagIbCb;
    }
    else
    {
        return ( USHORT )(ib_) & mskLargeTagIbCb;
    }
}

//
//  Gets the flags stored in a TAG. They are extracted and put in the
//  lower bits of a USHORT
//
INLINE USHORT CPAGE::TAG::FFlags( __in const CPAGE * const pPage, __in const BOOL fSmallFormat ) const
{
    Assert( fSmallFormat == pPage->FSmallPageFormat() );

    if ( fSmallFormat )
    {
        const USHORT us = USHORT( ib_ );
        return ( us & mskTagFlags ) >> shfTagFlags;
    }
    else
    {
        USHORT usFlags = 0;

        if ( 2 <= Cb( fSmallFormat ) )
        {
            // small pages (<=8kiB), flags are in the tag, large pages (16/32kiB), flags are in the node.
            UnalignedLittleEndian< USHORT >* pule_us = (UnalignedLittleEndian<USHORT>*)pPage->PbFromIb_( Ib( fSmallFormat ) );
            const USHORT us = USHORT( *pule_us );
            usFlags = ( us & mskTagFlags ) >> shfTagFlags;
        }

        return usFlags;
    }
}

//
//  This sets the ib in a tag. The flags are left unchanged
//
INLINE VOID CPAGE::TAG::SetIb( __in CPAGE * const pPage, __in USHORT ib )
{
    const BOOL fSmallFormat = pPage->FSmallPageFormat();

    if ( fSmallFormat )
    {
        Assert( ( ib & ~mskSmallTagIbCb ) == 0 );
        OnDebug( USHORT usFlags = FFlags( pPage, fSmallFormat) );

        USHORT ibT = ib_;           // endian conversion
        ibT &= ~mskSmallTagIbCb;    // clear current ib
        ibT |= ib;                      // set new ib
        ib_ = ibT;

        Assert( Ib( fSmallFormat ) == ib );
        Assert( FFlags( pPage, fSmallFormat ) == usFlags );
    }
    else
    {
        Assert( ( ib & ~mskLargeTagIbCb ) == 0 );
        //  no assertion for no change to flags since flags not in tag
        //
        USHORT ibT = ib_;           // endian conversion
        ibT &= ~mskLargeTagIbCb;    // clear current ib
        ibT |= ib;                      // set new ib
        ib_ = ibT;

        Assert( Ib( fSmallFormat ) == ib );
    }
}

//
//  Sets the cb in a tag. The flags are left unchanged
//
INLINE VOID CPAGE::TAG::SetCb( __in CPAGE * const pPage, __in USHORT cb )
{
    const BOOL fSmallFormat = pPage->FSmallPageFormat();

    if ( fSmallFormat )
    {
        Assert( ( cb & ~mskSmallTagIbCb ) == 0 );
        OnDebug( USHORT usFlags = FFlags( pPage, fSmallFormat ) );

        USHORT  cbT = cb_;              // endian conversion
        cbT &= ~mskSmallTagIbCb;    // clear current cb
        cbT |= cb;                      // set new cb
        cb_ = cbT;

        Assert( Cb( fSmallFormat ) == cb );
        Assert( FFlags( pPage, fSmallFormat ) == usFlags );
    }
    else
    {
        Assert( ( cb & ~mskLargeTagIbCb ) == 0 );
        //  no assertion for no change to flags since flags not in tag
        //

        USHORT  cbT = cb_;              // endian conversion
        cbT &= ~mskLargeTagIbCb;    // clear current cb
        cbT |= cb;                      // set new cb
        cb_ = cbT;

        Assert( Cb( fSmallFormat ) == cb );
    }
}

//
//  Sets the flags in a TAG. The cb and ib are not changed.
//
INLINE VOID CPAGE::TAG::SetFlags( __in CPAGE * const pPage, __in USHORT fFlags )
{
    Assert( 0 == ( fFlags & ~0x7 ) );   // only 3 valid flags

    const BOOL fSmallFormat = pPage->FSmallPageFormat();
    OnDebug( const INT cbOld = Cb( fSmallFormat ) );
    OnDebug( const INT ibOld = Ib( fSmallFormat ) );

    if ( fSmallFormat )
    {
        //  small pages (<=8kiB), flags are in the tag, large pages (16/32kiB), flags are in the node.
        //
        //  it is essential that this value is marked as volatile as otherwise, the
        //  compiler may apply & and | operation directly on the puus. This causes
        //  a "flicker" on the flags. this function is called "hot" i.e. the version flag is
        //  turned off while other threads are reading. A flickering "compressed" or "deleted" flag
        //  is a disaster.

        typedef UnalignedLittleEndian< volatile USHORT >* PULEUS;
        PULEUS puus =  ( PULEUS )&(ib_);

        USHORT ibT = *puus;         // endian conversion
        ibT &= ~mskTagFlags;            // clear current flag
        ibT |= fFlags << shfTagFlags;   // set new flag
        *puus = ibT;                    // write back

        Assert( FFlags( pPage, fSmallFormat ) == fFlags );
        Assert( Cb( fSmallFormat ) == cbOld );
        Assert( Ib( fSmallFormat ) == ibOld );
    }
    else
    {
        // set flags
        const bool fTag0 = pPage->PtagFromItag_( 0 ) == this;
        if ( !fTag0 && 2 <= Cb( fSmallFormat ) )
        {
            //  small pages (<=8kiB), flags are in the tag, large pages (16/32kiB), flags are in the node.
            //
            //  it is essential that this value is marked as volatile as otherwise, the
            //  compiler may apply & and | operation directly on the puus. This causes
            //  a "flicker" on the flags. this function is called "hot" i.e. the version flag is
            //  turned off while other threads are reading. A flickering "compressed" or "deleted" flag
            //  is a disaster.

            typedef UnalignedLittleEndian< volatile USHORT >* PULEUS;
            PULEUS puus = ( PULEUS )pPage->PbFromIb_( Ib( fSmallFormat ) );

            USHORT ibT = *puus;         // endian conversion
            ibT &= ~mskTagFlags;            // clear current flag
            ibT |= fFlags << shfTagFlags;   // set new flag
            *puus = ibT;                    // write back

            Assert( FFlags( pPage, fSmallFormat ) == fFlags );
            Assert( Cb( fSmallFormat ) == cbOld );
            Assert( Ib( fSmallFormat ) == ibOld );
        }
        else
        {
            Assert( ( fTag0 || !Cb( fSmallFormat ) ) && !fFlags );
        }
    }
}


//  ****************************************************************
//  CLASS STATICS
//  ****************************************************************

SIZE_T          CPAGE::cbHintCache              = 0;
SIZE_T          CPAGE::maskHintCache            = 0;
DWORD_PTR*      CPAGE::rgdwHintCache            = NULL;




//  ================================================================
INLINE ULONG CPAGE::CbPage() const
//  ================================================================
{
//  try this ... for now this should work same as CbPage() as there is no differences yet
//  return m_platchManager->CbBuffer( m_bfl );
    return m_platchManager->CbPage( m_bfl );
}

//  ================================================================
INLINE ULONG CPAGE::CbBuffer() const
//  ================================================================
{
    return m_platchManager->CbBuffer( m_bfl );
}

//  ****************************************************************
//  INTERNAL INLINE ROUTINES
//  ****************************************************************

//  ================================================================
template< PageNodeBoundsChecking pgnbc >
INLINE VOID CPAGE::GetPtr_( INT itag, LINE * pline, _Out_opt_ ERR * perrNoEnforce ) const
//  ================================================================
//
//  Returns a pointer to the line on the page. Calling a method that
//  reorganizes the page (Insert or Replace) may invalidate the pointer.
//  The error will be set if the page seems corrupted, e.g.:
//    - TAG entry is way too low in page, overlapping PGHDR, perhaps itagMicFree corrupt.
//    - The line returned is overlapping PGHDR, perhaps corrupt TAG::ib.
//    - The line returned is overlapping the TAG array, perhaps corrupt TAG::cb or itagMicFree.
//
//-
{
    Assert( itag >= 0 );
    Assert( itag < ((PGHDR *)m_bfl.pv)->itagMicFree || FNegTest( fCorruptingPageLogically ) );
    Assert( pline );
    Expected( ( pgnbc == pgnbcChecked ) == ( perrNoEnforce != NULL ) );
    Assert( ( pgnbc == pgnbcChecked ) || ( perrNoEnforce == NULL ) ); // this wouldn't make sense.
    Assert( perrNoEnforce == NULL || *perrNoEnforce == JET_errSuccess ); // only set for errors.

    const bool fSmallFormat = FSmallPageFormat();
#ifdef DEBUG
    const ULONG cbBufferDebug = m_platchManager->CbBuffer( m_bfl );

    //  this grabs the bounds of data portion of the page, that is available to hold
    //  the actual lines / nodes of data.

    const ULONG_PTR pbPageBufferStartDebug = (ULONG_PTR)m_bfl.pv;
    const ULONG_PTR pbPageBufferEndDebug = pbPageBufferStartDebug + cbBufferDebug;

    const ULONG_PTR pbPageDataStartDebug = pbPageBufferStartDebug + 
                                      ( fSmallFormat ? sizeof( PGHDR ) : sizeof( PGHDR2 ) );   // m_bfl.pv + header size
    const ULONG_PTR pbPageDataEndDebug = pbPageBufferEndDebug - CbTagArray_() - 1;             // m_bfl.pv + cbBuffer - tag array size (off itagMicFree)

    Assert( pbPageDataStartDebug == PbDataStart_( fSmallFormat ) );
    Assert( pbPageDataEndDebug == PbDataEnd_( cbBufferDebug ) );
#endif

    #pragma warning(suppress:4101) //  unreferenced local variable - compiler is not smart enough to notice that's only true in one templated version.
    bool fFlagsOnPage;

    //  now get TAG * and compare check it against data start / end

    const TAG * const ptag = PtagFromItag_( itag );

    //  if the itagMicFree / and thus passed in itag are corrupted, the ptag 
    //  can end up off the page.

    Assert( ( (ULONG_PTR)ptag + sizeof( TAG ) - 1 ) <= pbPageBufferEndDebug );
    if constexpr( pgnbc == pgnbcChecked )
    {
        fFlagsOnPage = fTrue;

        const ULONG_PTR pbPageBufferStart = (ULONG_PTR)m_bfl.pv;
        const ULONG_PTR pbPageDataStart = pbPageBufferStart + 
                                      ( fSmallFormat ? sizeof( PGHDR ) : sizeof( PGHDR2 ) );                           // m_bfl.pv + header size
        const ULONG_PTR pbPageBufferEnd = pbPageBufferStart + m_platchManager->CbBuffer( m_bfl );

        if ( (ULONG_PTR)ptag < pbPageDataStart || 
               //  OPTIMIZE: If perf is a problem, consider dropping this clause ... it is probably impossible to get this to 
               //  fire because we can only have 32k to maybe 64k itags ... _though_ itag is coming in as INT / 32-bit but most 
               //  would be based off itagMicFree which is a USHORT / 16 bit ... so 64,000 * sizeof(TAG) ... 256 KB ... so we 
               //  would have to have a page pointer within the first 256 KB of VA.  Probably can't happen.
               ( (ULONG_PTR)ptag + sizeof( TAG ) - 1 ) > pbPageBufferEnd )
        {
            //  this corruption case (probably of itagMicFree) is so severe the ptag is off page and so 
            //  we can't even compute the line.xx members without potentially AV'ing (or looking at 
            //  another page's data).

            pline->pv = NULL;
            pline->cb = 0;
            pline->fFlags = 0;

            if ( perrNoEnforce != NULL )
            {
                PageAssertTrack( *this, FNegTest( fCorruptingPageLogically ), "TagPtrOffPage%hs", (ULONG_PTR)ptag < pbPageDataStart ? "" : "HighImpossible" );
                *perrNoEnforce = ErrERRCheck( JET_errPageTagCorrupted );
            }
            else
            {
                PageAssertTrack( *this, fFalse, "TagPtrOffPage%hs", (ULONG_PTR)ptag < pbPageDataStart ? "NoErrRet" : "HighImpossibleNoErrRet" );
            }
            return;
        }
    }

    pline->cb = ptag->Cb( fSmallFormat );

    if ( pline->cb == 0 )
    {
        // the ib for zero-length tag zero can be bad; it has never been validated
        // that it is even on the page; even then there are cases where it points
        // past the end of the dehydrated buffer.

        if constexpr( pgnbc == pgnbcChecked )
        {
            if ( itag != 0 )
            {
                if ( perrNoEnforce != NULL )
                {
                    PageAssertTrack( *this, FNegTest( fCorruptingPageLogically ), "NoExtHdrTagHasZeroCb" );
                    *perrNoEnforce = ErrERRCheck( JET_errPageTagCorrupted );
                }
                else
                {
                    PageAssertTrack( *this, fFalse, "NoExtHdrTagHasZeroCbNoErrRet" );
                }
            }
        }

        pline->pv = (VOID *)-29;

        // Note: pline->cb == 0 already (the if clause we're in).

        // Next Assert must be true (even in corrupt condtions) due TAG::FFlags() impl (for large pages) which approx is: 
        //     if ( ptag->Cb() < 2 ) { return 0 } else { return ( *(USHORT*)PbFromIb_( Ib( fSmallFormat ) ) & mskTagFlags ) >> shfTagFlags; }
        // So there is no danger in "deref'ing" ptag->FFlags() at end of the function due also to the cb == 0 condition.
        Assert( fSmallFormat || ptag->FFlags( this, fSmallFormat ) == 0x0 );
        // Do not know if either of these two holds though, nor if required.  Odd to have flags on a half created tag though,
        // and think it may only be embeddedunittest created the bum page in fSmallFormat.  Might be fixable.
#ifndef ENABLE_JET_UNIT_TEST
        Assert( ptag->FFlags( this, fSmallFormat ) == 0x0 );
#else
        Assert( ptag->FFlags( this, fSmallFormat ) == 0x0 || FNegTest( fCorruptingPageLogically ) ); // the unit test puts pages in bad states.
#endif
    }
    else
    {
        const ULONG_PTR pbLine = (ULONG_PTR)PbFromIb_( ptag->Ib( fSmallFormat ) );

        pline->pv = (void*)pbLine;

        Assert( pline->cb <= (ULONG)usMax );

        const ULONG_PTR pbLineLastByte = pbLine + pline->cb - 1;
        PageAssertTrack( *this, pbLine <= pbLineLastByte, "NonEuclideanRamGeometryMaybeImpossible" ); // should be computationally impossible by adding USHORT to wrap a ptr.

        Assert( FOnData( pline->pv, pline->cb ) || FNegTest( fCorruptingPageLogically ) );

        if constexpr( pgnbc == pgnbcChecked )
        {
            //  this grabs the bounds of data portion of the page, that is available to hold
            //  the actual lines / nodes of data.

            const ULONG_PTR pbPageBufferStart = (ULONG_PTR)m_bfl.pv;

            const ULONG_PTR pbPageDataStart = pbPageBufferStart + 
                                      ( fSmallFormat ? sizeof( PGHDR ) : sizeof( PGHDR2 ) );                               // m_bfl.pv + header size
            const ULONG_PTR pbPageDataEnd = (ULONG_PTR)m_bfl.pv + m_platchManager->CbBuffer( m_bfl ) - CbTagArray_() - 1;  // m_bfl.pv + cbBuffer - tag array size (off itagMicFree)

            //  This if, is an inlined FOnData() without the nested recomputation of pbPageDataStart, End, etc
            if ( pbLineLastByte > pbPageDataEnd ||
                   //  OPTIMIZE: I (SOMEONE) do not think that any of these remaining conditions could happen (except 
                   //  last - but that's ALSO covered by first condition), due to the math of PbFromIb_().  For now we 
                   //  will check all conditions.  Note; We can't just trade removing these conditions with an 
                   //  AssertTrack outside of this if, as that does not actually optimize / remove the checks.  We'd 
                   //  have to just completely remove them - and have faith (well debug asserts).
                   pbLine < pbPageDataStart ||
                   pbLine >= pbPageDataEnd ||  // someday > maybe if a 1-byte node ever shows up.
                   pbLineLastByte < pbPageDataStart )
            {
                //  Some portion of the line is off page, or overlapping PGHDR or overlapping
                //  the tag array!
                PageAssertTrack( *this, pbLine >= pbPageDataStart, "LineBeforeDataStartImpossible" ); // adding tag.ib / USHORT shouldn't allow us to go down in value
                PageAssertTrack( *this, pbLineLastByte >= pbPageDataStart, "LineBeforeDataStartImpossible" ); // adding tag.ib & tag.cb (USHORTs) shouldn't allow us to go down in value
                if ( pbLine >= pbPageDataEnd && pbLineLastByte <= pbPageDataEnd )
                {
                    // We will see "LineStartsOffDataEnd, but also always a matching "LineTrailsOffDataEnd".  If they match
                    // it tells us we really only need the last check above to catch corruptions just by nature of the math.
                    // This track will tell us if we are wrong.
                    PageAssertTrack( *this, fFalse, "LineStartsOffButNotEndsOffPageImpossible" );
                }
                if ( perrNoEnforce != NULL )
                {
                    PageAssertTrack( *this, pbLine < pbPageDataEnd || FNegTest( fCorruptingPageLogically ), "LineStartsOffDataEnd" );
                    PageAssertTrack( *this, pbLineLastByte <= pbPageDataEnd || FNegTest( fCorruptingPageLogically ), "LineStartsOff%hs", pbLine >= pbPageDataEnd ? "DataEnd" : "TrailsOffDataEnd" );
                    *perrNoEnforce = ErrERRCheck( JET_errPageTagCorrupted );
                }
                else
                {
                    if( pbLine >= pbPageDataEnd )
                    {
                        PageAssertTrack( *this, fFalse, "LineStartsOffDataEndNoErrRet" );
                    }
                    else
                    {
                        PageAssertTrack( *this, fFalse, "LineTrailsOffDataEndNoErrRet" );
                    }
                }

                fFlagsOnPage = fSmallFormat || // small page flags are part of ptag - which was proven to be on page above.
                               ULONG_PTR( pbLine + 1 ) < pbPageDataEnd; // the +1, even though in _first_ byte, deref'd as USHORT
            }
        }
    }

    if constexpr( pgnbc == pgnbcNoChecks )
    {
        pline->fFlags =  ptag->FFlags( this, fSmallFormat );
    }
    else
    {
        pline->fFlags =  fFlagsOnPage ? ptag->FFlags( this, fSmallFormat ) : 0;
    }
}


//  ================================================================
INLINE INT CPAGE::CbDataTotal_( const DATA * rgdata, INT cdata ) const
//  ================================================================
//
//  Returns the total size of all elements in a DATA array
//
//-
{
    Assert( rgdata );
    Assert( cdata >= 0 );

    INT cbTotal = 0;
    INT iline   = 0;
    for ( ; iline < cdata; iline++ )
    {
        cbTotal += rgdata[iline].Cb();
    }
    return cbTotal;
}


//  ================================================================
INLINE USHORT CPAGE::CbAdjustForPage() const
//  ================================================================
//
//  returns count of bytes that are "missing" from the buffer to be
//  considered a full page.  Used to adjust counts for other functions
//  that normally produce a buffer-based sizing calculation.
//
//-
{
    Assert( CbPage() < 0x10000 );
    Assert( m_platchManager->CbBuffer( m_bfl ) < 0x10000 );
    Assert( CbPage() >= m_platchManager->CbBuffer( m_bfl ) );
    return (USHORT)CbPage() - (USHORT)m_platchManager->CbBuffer( m_bfl );
}

//  ================================================================
INLINE INT CPAGE::CbBufferData() const
//  ================================================================
//
//  Returns the size of the data space of the current buffer.  Also
//  see CbPageData().
//
//-
{
    return m_platchManager->CbBuffer( m_bfl ) - CbPageHeader();
}

//  ================================================================
INLINE INT CPAGE::CbPageData() const
//  ================================================================
//
//  Returns the size of the data space of the naturally sized page.
//  Also see CbBufferData().
//
//-
{
    return CbBufferData() + CbAdjustForPage();
}

//  ================================================================
INLINE INT CPAGE::CbContiguousBufferFree_( ) const
//  ================================================================
//
//  Returns the number of bytes available at the end of the last line
//  on the buffer (not the page). This gives the size of the largest
//  item that can be inserted without reorganizing or growing the
//  buffer.  Also see CbContiguousFree_().
//
//-
{
    INT cb = CbBufferData() - ((PGHDR *)m_bfl.pv)->ibMicFree;
    cb -= CbTagArray_();
    return cb;
}

//  ================================================================
INLINE INT CPAGE::CbContiguousFree_( ) const
//  ================================================================
//
//  Returns the number of bytes available at the end of the last line
//  on the page. This gives the size of the largest item that can be
//  inserted without reorganizing.  Also see CbContiguousBufferFree_().
//
//-
{
    return CbContiguousBufferFree_() + CbAdjustForPage();
}

//  ================================================================
INLINE USHORT CPAGE::CbFree_ ( ) const
//  ================================================================
{
    return ((PGHDR*)m_bfl.pv)->cbFree;
}


//  ================================================================
USHORT CPAGE::CbPageFree ( ) const
//  ================================================================
{
    AssertRTL( (ULONG)((PGHDR*)m_bfl.pv)->cbFree + (ULONG)CbAdjustForPage() < 0x10000 );
    return ((PGHDR*)m_bfl.pv)->cbFree + CbAdjustForPage();
}


//  ================================================================
INLINE VOID CPAGE::FreeSpace_( __in const INT cb )
//  ================================================================
//
//  Creates the amount of contigous free space passed to it,
//  reorganizing if necessary.
//  If not enough free space can be created we Assert
//
//-
{
    Assert ( cb <= ((PGHDR*)m_bfl.pv)->cbFree && cb > 0 );

    if ( CbContiguousFree_( ) < cb )
    {
        ReorganizeData_( reorgFreeSpaceRequest );
    }

    PageEnforce( (*this), CbContiguousFree_( ) >= cb );
}


//  ================================================================
INLINE VOID CPAGE::CopyData_( TAG * ptag, const DATA * rgdata, INT cdata )
//  ================================================================
//
//  Copies the data array into the page location pointed to by the TAG
//
//-
{
    Assert( ptag && rgdata );
    Assert( cdata > 0 );

    const BOOL fSmallFormat = this->FSmallPageFormat();

    // make sure we don't trash flags on big (16/32kiB) pages
    USHORT usFlags = ptag->FFlags( this, fSmallFormat );

    BYTE * pb = PbFromIb_( ptag->Ib( fSmallFormat ) );
    INT ilineCopy = 0;
    for ( ; ilineCopy < cdata; ilineCopy++ )
    {
        if ( rgdata[ilineCopy].Cb() < 0 )
        {
            (void)ErrCaptureCorruptedPageInfo( OnErrorEnforce, L"DataLengthNegative" );
        }

        memmove( pb, rgdata[ilineCopy].Pv(), rgdata[ilineCopy].Cb() );
        pb += rgdata[ilineCopy].Cb();
    }

    if ( PtagFromItag_( 0 ) != ptag )
    {
        ptag->SetFlags( this, usFlags );
    }

    Assert( PbFromIb_( ptag->Ib( fSmallFormat ) + ptag->Cb( fSmallFormat ) ) == pb );
}


//  ================================================================
INLINE CPAGE::TAG * CPAGE::PtagFromItag_( INT itag ) const
//  ================================================================
//
//  Turn an itag into a pointer to a tag.
//  Remember that the TAG array grows backwards from the end of the
//  page.
//
//-
{
    Assert( itag >= 0 );
    Assert( itag <= ((PGHDR*)m_bfl.pv)->itagMicFree ); // The <= is for Insert_() and Delete_() which are expanding/shrinking array.

    TAG * ptag = (TAG *)( (BYTE*)m_bfl.pv + m_platchManager->CbBuffer( m_bfl ) );
    ptag -= itag + 1;
#if !defined(ARM) && !defined(ARM64)
    _mm_prefetch( (char*)ptag, _MM_HINT_T0 );   //  almost always this will be immediately useful ...
#endif
    Assert( NULL != ptag );
    Assert( !FAssertNotOnPage_( ptag ) || FNegTest( fCorruptingPageLogically ) );
    Assert( (BYTE*)ptag > ((BYTE*)m_bfl.pv + CbPageHeader()) || FNegTest( fCorruptingPageLogically ) );
    //  We chose >= and +1 because sometimes the itagMicFree is updated after computing a PtagFromItag( itag )
    //  for a newly inserted node / see Insert_().  It would be stricter if we did not add 1 here.
    Assert( (BYTE*)ptag >= ( ((BYTE*)m_bfl.pv + m_platchManager->CbBuffer( m_bfl )) - ((((PGHDR*)m_bfl.pv)->itagMicFree + 1)  * sizeof(TAG)) ) );
#ifdef DEBUG
    //  If however the ptag matches 1 past the end of the itag array (i.e. insert, THEN we know we must have
    //  at least 2 bytes free with which to insert a tag.
    if( (BYTE*)ptag == ( ((BYTE*)m_bfl.pv + m_platchManager->CbBuffer( m_bfl )) - ((((PGHDR*)m_bfl.pv)->itagMicFree + 1)  * sizeof(TAG)) ) )
    {
        Assert( ((PGHDR*)m_bfl.pv)->cbFree > sizeof(TAG) );
    }
#endif

    return ptag;
}


#ifdef DEBUG
//  ================================================================
INLINE CPAGE::TAG * CPAGE::PtagFromRgbCbItag_( BYTE *rgbPage, INT cbPage, INT itag ) const
//  ================================================================
//
//  Turn an rgbPage, cbPage and itag into a pointer to a tag.
//  Remember that the TAG array grows backwards from the end of the
//  page.
//
//-
{
    Assert( itag >= 0 );
    Assert( itag <= ((PGHDR*)rgbPage)->itagMicFree );

    TAG * ptag = (TAG *)( (BYTE*)rgbPage + cbPage );
    ptag -= itag + 1;

    Assert( NULL != ptag );
    //  We chose >= and +1 because sometimes the itagMicFree is updated after computing a PtagFromItag( itag )
    //  for a newly inserted node / see Insert_().  It would be stricter if we did not add 1 here.
    Assert( (BYTE*)ptag >= ( ((BYTE*)rgbPage + cbPage) - ((((PGHDR*)rgbPage)->itagMicFree+1)  * sizeof(TAG)) ) );
#ifdef DEBUG
    //  If however the ptag matches 1 past the end of the itag array (i.e. insert, THEN we know we must have
    //  at least 2 bytes free with which to insert a tag.
    if( (BYTE*)ptag == ( ((BYTE*)rgbPage + cbPage) - ((((PGHDR*)rgbPage)->itagMicFree+1)  * sizeof(TAG)) ) )
    {
        Assert( ((PGHDR*)rgbPage)->cbFree > sizeof(TAG) );
    }
#endif

    return ptag;
}

#ifdef DEBUG_PAGE
#define FDoPageSnap()   (fTrue)
#else
#define FDoPageSnap()   ( rand() % 100 == 3 )   /* 1% chance */
#endif

//  Keeps a copy of the original page for later validation.
//
#define DEBUG_SNAP_PAGE()       \
    INT cbSnapPage = m_platchManager->CbBuffer( m_bfl );                                \
    INT cbSnapPageHdr = CbPageHeader();                                                 \
    CPAGE cpageSnapOriginal;                                                            \
    if ( FDoPageSnap() )                                                                \
    {                                                                                   \
        BFAlloc( bfasTemporary, &cpageSnapOriginal.m_bfl.pv, g_cbPageMax );             \
        UtilMemCpy( cpageSnapOriginal.m_bfl.pv, m_bfl.pv, cbSnapPage );                 \
        cpageSnapOriginal.m_ppib    = m_ppib;                                           \
        cpageSnapOriginal.m_ifmp    = m_ifmp;                                           \
        cpageSnapOriginal.m_pgno    = m_pgno;                                           \
        cpageSnapOriginal.m_fSmallFormat = m_fSmallFormat;                              \
        cpageSnapOriginal.m_dbtimePreInit = m_dbtimePreInit;                            \
        cpageSnapOriginal.m_objidPreInit = m_objidPreInit;                              \
    }                                                                                   \
    Assert( (BOOL)FIsSmallPage( m_platchManager->CbPage( m_bfl ) ) == m_fSmallFormat );

//  Check for no data change.
//
#define DEBUG_SNAP_COMPARE()    \
    if ( cpageSnapOriginal.m_bfl.pv )                                                   \
    {                                                                                   \
        Assert( ((PGHDR*)cpageSnapOriginal.m_bfl.pv)->itagMicFree == ((PGHDR*)m_bfl.pv)->itagMicFree );     \
        for ( INT itagT = 0; itagT < ((PGHDR*)m_bfl.pv)->itagMicFree; ++itagT )         \
        {                                                                               \
            const TAG * const ptagOld = cpageSnapOriginal.PtagFromRgbCbItag_( ((BYTE*)cpageSnapOriginal.m_bfl.pv), cbSnapPage, itagT );     \
            const TAG * const ptagNew = PtagFromItag_( itagT );                         \
            Assert( ptagOld != ptagNew );                                               \
            Assert( ptagOld->Cb( cpageSnapOriginal.m_fSmallFormat ) == ptagNew->Cb( cpageSnapOriginal.m_fSmallFormat ) );   \
            Assert( ((BYTE*)cpageSnapOriginal.m_bfl.pv) + cbSnapPageHdr + ptagOld->Ib( cpageSnapOriginal.m_fSmallFormat ) != PbFromIb_( ptagNew->Ib( cpageSnapOriginal.m_fSmallFormat ) ) );    \
            Assert( memcmp( ((BYTE*)cpageSnapOriginal.m_bfl.pv) + cbSnapPageHdr + ptagOld->Ib( cpageSnapOriginal.m_fSmallFormat ), PbFromIb_( ptagNew->Ib( cpageSnapOriginal.m_fSmallFormat ) ), ptagNew->Cb( cpageSnapOriginal.m_fSmallFormat ) ) == 0 );  \
        }                                                                               \
        cpageSnapOriginal.m_ppib    = ppibNil;                                          \
        cpageSnapOriginal.m_ifmp    = 0;                                                \
        cpageSnapOriginal.m_pgno    = pgnoNull;                                         \
        cpageSnapOriginal.m_dbtimePreInit = dbtimeNil;                                  \
        cpageSnapOriginal.m_objidPreInit = objidNil;                                    \
        BFFree( cpageSnapOriginal.m_bfl.pv );                                           \
        cpageSnapOriginal.m_bfl.pv  = NULL;                                             \
    }

#else
#define DEBUG_SNAP_PAGE()
#define DEBUG_SNAP_COMPARE()
#endif  //  DEBUG


//  ================================================================
INLINE ULONG CPAGE::CbTagArray_() const
//  ================================================================
{
    return ((PGHDR *)m_bfl.pv)->itagMicFree * sizeof(TAG);
}


//  ================================================================
INLINE BYTE * CPAGE::PbFromIb_( USHORT ib ) const
//  ================================================================
//
//  Turns an index into the data array on the page into a pointer
//
//-
{
    Assert( ib >= 0 );
    Assert( (ULONG)ib <= ( m_platchManager->CbBuffer( m_bfl ) - CbPageHeader() - CbTagArray_() ) || FNegTest( fCorruptingPageLogically ) );

    return (BYTE*)m_bfl.pv + CbPageHeader() + ib;
}


//  ****************************************************************
//  PUBLIC MEMBER FUNCTIONS
//  ****************************************************************


//  ================================================================
CPAGE::CPAGE( ) :
//  ================================================================
//
//  Sets the member variables to NULL values. In DEBUG we see if we
//  need to run the one-time checks
//
//-
        m_ppib( ppibNil ),
        m_ifmp( 0 ),
        m_dbtimePreInit( dbtimeNil ),
        m_pgno( pgnoNull ),
        m_objidPreInit( objidNil ),
        m_platchManager( &g_nullPageLatchManager ),
        m_fSmallFormat( fFormatInvalid ),
        m_iRuntimeScrubbingEnabled( -1 )
{
    C_ASSERT( sizeof( CPAGE::TAG ) == 4 );
    C_ASSERT( ctagReserved > 0 );
    m_bfl.pv        = NULL;
    m_bfl.dwContext = NULL;
}

//  ================================================================
CPAGE::~CPAGE( )
//  ================================================================
//
//  Empty destructor
//
//  OPTIMIZATION:  consider inlining this method
//
//-
{
    //  the page should have been released
    Assert( FAssertUnused_( ) );
}


//  ================================================================
VOID CPAGE::PreInitializeNewPage_(  PIB * const ppib,
                                    const IFMP ifmp,
                                    const PGNO pgno,
                                    const OBJID objidFDP,
                                    const DBTIME dbtime )
//  ================================================================
//
//  Get and latch a new page buffer from the buffer manager. Initialize the new page.
//
//-
{
    PGHDR *ppghdr;

    //  set CPAGE member variables
    m_ppib = ppib;
    m_ifmp = ifmp;
    m_pgno = pgno;

#ifndef ENABLE_JET_UNIT_TEST
    Assert( ( ifmpNil != ifmp ) || FNegTest( fInvalidUsage ) || dbtime == dbtimeRevert );
#endif  //  !ENABLE_JET_UNIT_TEST

    Assert( m_platchManager != &g_nullPageLatchManager );
    Assert( m_fSmallFormat != fFormatInvalid );
    Assert( m_platchManager->CbBuffer( m_bfl ) );

    //  initialize the page to 0
    memset( m_bfl.pv, 0, m_platchManager->CbBuffer( m_bfl ) );

    //  set the page header variables
    ppghdr = (PGHDR*)m_bfl.pv;
    ppghdr->fFlags              = CPAGE::fPagePreInit;
    ppghdr->objidFDP            = objidFDP;
    ppghdr->cbFree              = (USHORT)CbPageData();
    ppghdr->cbUncommittedFree   = 0;
    ppghdr->ibMicFree           = 0;
    ppghdr->itagMicFree         = 0;
    ppghdr->pgnoNext            = pgnoNull;
    ppghdr->pgnoPrev            = pgnoNull;
    ppghdr->dbtimeDirtied       = dbtime;

    if ( !FSmallPageFormat() )
    {
        // for new page, we can just set it w/o size test, but it is not elegant.
        PGHDR2* ppghdr2 = ( PGHDR2* )ppghdr;
        ppghdr2->pgno = pgno;

        ppghdr2->pghdr.checksum = 0xdeadbeefbaadf00d;
        ppghdr2->rgChecksum[ 0 ] = 0x6570755320455345;
        ppghdr2->rgChecksum[ 1 ] = 0x524f584343452072;
        ppghdr2->rgChecksum[ 2 ] = 0x6d75736b63656843;
    }

    Assert( ( size_t )ppghdr->cbFree + CbPageHeader() == ( size_t ) CbPage() );

    SetFNewRecordFormat();

    ASSERT_VALID( this );
    DebugCheckAll();
}

//  ================================================================
ERR CPAGE::ErrGetNewPreInitPage(    PIB * ppib,
                                    IFMP ifmp,
                                    PGNO pgno,
                                    OBJID objidFDP,
                                    DBTIME dbtimeNoLog,
                                    BOOL fLogNewPage )
//  ================================================================
//
//  Get and latch a new page buffer from the buffer manager. Initialize the new page.
//
//-
{
    ASSERT_VALID( ppib );
    Assert( FAssertUnused_( ) );
    Assert( ( m_dbtimePreInit == dbtimeNil ) && ( m_objidPreInit == objidNil ) );
    Assert( fLogNewPage || ( dbtimeNoLog >= 0 ) );
    Expected( fLogNewPage || ( dbtimeNoLog != 0 ) || g_rgfmp[ifmp].FCreatingDB() );

    ERR err = JET_errSuccess;
    BOOL fLatched = fFalse;
    BOOL fCachedNewPage = fFalse;
    BOOL fLogged = fFalse;
    OnDebug( BOOL fPointOfNoReturn = fFalse );
    GetCurrUserTraceContext tcUser;
    auto tc = TcCurr();
    LOG* const plog = PinstFromIfmp( ifmp )->m_plog;
    LGPOS lgpos = lgposMin;

    // This code presumes we set fCachedNewPage only for a cache fault.
    Assert( !( bflfPreInitLatchFlags & bflfNew ) ); // this code presumes we set fCachedNewPage only for a cache fault.
    Call( ErrBFWriteLatchPage( &m_bfl, ifmp, pgno, bflfPreInitLatchFlags, ppib->BfpriPriority( ifmp ), tc, &fCachedNewPage ) );
    fLatched = fTrue;

    Assert( ifmpNil != ifmp );
    Assert( pgno <= g_rgfmp[ifmp].PgnoLast() ||
            CmpLogFormatVersion( FmtlgverLGCurrent( plog ), fmtlgverInitialDbSizeLoggedMain ) < 0 );

    m_ppib = ppib;
    m_ifmp = ifmp;
    m_pgno = pgno;
    m_platchManager = &g_bfLatchManager;
    m_fSmallFormat  = FIsSmallPage( g_rgfmp[ifmp].CbPage() );

    Assert( !fLogNewPage ||
            ( ( plog->FRecoveringMode() != fRecoveringRedo ) && g_rgfmp[ifmp].FLogOn() ) );

    m_dbtimePreInit = fLogNewPage ? g_rgfmp[ifmp].DbtimeIncrementAndGet() : dbtimeNoLog;
    m_objidPreInit = objidFDP;

    // Log operation if applicable.
    if ( fLogNewPage )
    {
        PERFOpt( cCPAGENewPages.Inc( PinstFromIfmp( ifmp ) ) );

        if ( ( g_rgfmp[ifmp].ErrDBFormatFeatureEnabled( JET_efvLogNewPage ) == JET_errSuccess ) &&
             ( plog->ErrLGFormatFeatureEnabled( JET_efvLogNewPage ) == JET_errSuccess ) )
        {
            Call( ErrLGNewPage( ppib, ifmp, pgno, m_objidPreInit, m_dbtimePreInit, &lgpos ) );
            fLogged = fTrue;
        }
    }

    OnDebug( fPointOfNoReturn = fTrue );

    BFInitialize( &m_bfl, tc );
    PreInitializeNewPage_( ppib, ifmp, pgno, objidFDP, m_dbtimePreInit );

    Dirty( bfdfDirty );

    if ( fLogged )
    {
        SetLgposModify( lgpos );
    }

    Ptls()->threadstats.cPageReferenced++;
    OSTraceFMP(
        ifmp,
        JET_tracetagCursorPageRefs,
        OSFormat(
            "Session=[0x%p:0x%x] referenced new page=[0x%x:0x%x] of objid=[0x%x:0x%x]",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            (ULONG)ifmp,
            pgno,
            (ULONG)ifmp,
            objidFDP ) );

    Assert( err >= JET_errSuccess );
    err = JET_errSuccess;

HandleError:
    Assert( !fLatched || FAssertWriteLatch() );
    Assert( ( m_dbtimePreInit == dbtimeNil ) == ( m_objidPreInit == objidNil ) );

    if ( err < JET_errSuccess )
    {
        Assert( !fPointOfNoReturn );
        if ( fLatched )
        {
            if ( fCachedNewPage )
            {
                BFAbandonNewPage( &m_bfl, tc );
            }
            else
            {
                BFWriteUnlatch( &m_bfl );
            }
            fLatched = fFalse;

            Abandon_();
        }

        Assert( FAssertUnused_( ) );
        Assert( !fLatched );
        Assert( ( m_dbtimePreInit == dbtimeNil ) && ( m_objidPreInit == objidNil ) );
    }
    else
    {
        Assert( ErrBFLatchStatus( &m_bfl ) == JET_errSuccess );
        Assert( FFlags() == ( CPAGE::fPageNewRecordFormat | CPAGE::fPagePreInit ) );
    }

    Assert( ( err != JET_errSuccess ) || FAssertWriteLatch() );

    return err;
}

//  ================================================================
VOID CPAGE::ConsumePreInitPage( const ULONG fPageFlags )
//  ================================================================
{
    // Check consistency of current state and new flags.
    Assert( FFlags() == ( CPAGE::fPageNewRecordFormat | CPAGE::fPagePreInit ) );
    Assert( ( fPageFlags & CPAGE::fPagePreInit ) == 0 );
    Assert( ((PGHDR*)m_bfl.pv)->itagMicFree == 0 );

    // Insert the line for the external header.
    PGHDR* const ppghdr = (PGHDR*)m_bfl.pv;
    ppghdr->itagMicFree = ctagReserved;

    USHORT cbFree = ppghdr->cbFree;     // endian conversion
    cbFree -= (USHORT)sizeof( CPAGE::TAG );
    ppghdr->cbFree = cbFree;

    TAG* const ptag = PtagFromItag_( 0 );
    ptag->SetIb( this, 0 );
    ptag->SetCb( this, 0 );
    ptag->SetFlags( this, 0 );

    // Set flags.
    SetFlags( ( fPageFlags | CPAGE::fPageNewRecordFormat ) & ~CPAGE::fPagePreInit );
    Assert( FNewRecordFormat() );
    Assert( !FPreInitPage() );
}

//  ================================================================
VOID CPAGE::FinalizePreInitPage( OnDebug( const BOOL fCheckPreInitPage ) )
//  ================================================================
{
    Assert( ( m_dbtimePreInit == dbtimeNil ) == ( m_objidPreInit == objidNil ) );
    Expected( ( m_dbtimePreInit != dbtimeNil ) && ( m_objidPreInit != objidNil ) );
    Assert( FFlags() != 0 );
    Assert( !!fCheckPreInitPage == !!FPreInitPage() );
    Assert( !fCheckPreInitPage == ((PGHDR*)m_bfl.pv)->itagMicFree >= ctagReserved );
    m_dbtimePreInit = dbtimeNil;
    m_objidPreInit = objidNil;

    //  since this trace is stolen out of bf.cxx, see comments involving etguidCacheBlockNewPage in bf.cxx, then
    //  we preserve the logical iorpBFLatch here - which is practically speaking true, we are doing a write latch.
    GetCurrUserTraceContext tcUser;
    auto tc = TcCurr();
    PGHDR* const ppghdr = (PGHDR*)m_bfl.pv;
    ETCacheNewPage(
        m_ifmp,
        m_pgno,
        bflfPreInitLatchFlags,
        ppghdr->objidFDP,
        ppghdr->fFlags,
        tcUser->context.dwUserID,
        tcUser->context.nOperationID,
        tcUser->context.nOperationType,
        tcUser->context.nClientType,
        tcUser->context.fFlags,
        tcUser->dwCorrelationID,
        iorpBFLatch,
        tc.iorReason.Iors(),
        tc.iorReason.Iort(),
        tc.iorReason.Ioru(),
        tc.iorReason.Iorf(),
        tc.nParentObjectClass,
        ppghdr->dbtimeDirtied,
        ppghdr->itagMicFree, // kind of pointless for this trace, but shares template with read page.
        ppghdr->cbFree  );
}

//  ================================================================
ERR CPAGE::ErrGetReadPage ( PIB * ppib,
                            IFMP ifmp,
                            PGNO pgno,
                            BFLatchFlags bflf,
                            BFLatch* pbflHint )
//  ================================================================
{
    ERR err;

    ASSERT_VALID( ppib );
    Assert( FAssertUnused_( ) );
    Assert( ifmpNil != ifmp );
    Assert( ( m_dbtimePreInit == dbtimeNil ) && ( m_objidPreInit == objidNil ) );
    //  if we were given a hint for this page or it is the same page that
    //  we latched last time, we will attempt to use the hint to latch the
    //  page more quickly
    m_bfl.dwContext = m_pgno == pgno ? m_bfl.dwContext : NULL;
    m_bfl.dwContext = pbflHint ? pbflHint->dwContext : m_bfl.dwContext;
    m_platchManager = &g_bfLatchManager;
    m_fSmallFormat  = FIsSmallPage( g_rgfmp[ifmp].CbPage() );

#ifdef MINIMAL_FUNCTIONALITY
#else
LatchPage:
#endif

    Assert( FAssertUnused_( ) );

    //  get the page
    Call( ErrBFReadLatchPage( &m_bfl, ifmp, pgno, BFLatchFlags( bflf | bflfHint ), ppib->BfpriPriority( ifmp ), TcCurr() ) );

    //  we may need to refresh the latch hint
    if ( err != JET_errSuccess && pbflHint )
    {
        //  Note: we're refreshing the dwContext hint, but the error for the latch
        //  conflict will be flowing out of this function.

        Assert( wrnBFBadLatchHint == err ||
                    wrnBFPageFlushPending == err );
        Assert( FBFCurrentLatch( &m_bfl ) );

        //  get the latch hint for the latched BF
        DWORD_PTR dwHint = BFGetLatchHint( &m_bfl );

        //  refresh the hint if it has changed
        if ( pbflHint->dwContext != dwHint )
        {
            pbflHint->dwContext = dwHint;
        }
    }

#ifdef DEBUG
    if ( !FSmallPageFormat() )
    {
        PGHDR2* ppghdr2 = ( PGHDR2* )m_bfl.pv;
        AssertSz( ppghdr2->pgno == pgno,
                  "Inconsistent PGNO found between CPAGE (%d) and PGHDR2! (%d)",
                  pgno,
                  (PGNO)ppghdr2->pgno );
    }
#endif

    //  set CPAGE member variables
    m_ppib          = ppib;
    m_ifmp          = ifmp;
    m_pgno          = pgno;

#ifdef MINIMAL_FUNCTIONALITY
    Enforce( FNewRecordFormat() );
#else
    //  check to see if we need to perform a record format upgrade
    if( !FNewRecordFormat() )
    {
        if( NULL == ppib->PvRecordFormatConversionBuffer() )
        {
            const ERR   errT = ppib->ErrAllocPvRecordFormatConversionBuffer();
            if ( errT < 0 )
            {
                BFReadUnlatch( &m_bfl );
                Abandon_();
                //  only cannibalise error code from ErrBFReadLatchPage() if we hit an error
                Call( errT );
            }
            CallS( errT );
        }

        const ERR   errT    = ErrBFUpgradeReadLatchToWriteLatch( &m_bfl );
        if( errBFLatchConflict != errT )
        {
            if ( errT < 0 )
            {
                BFReadUnlatch( &m_bfl );
                Abandon_();

                //  only cannibalise error code from ErrBFReadLatchPage() if we hit an error
                Call( errT );
            }
        }
        else
        {
            BFReadUnlatch( &m_bfl );
            Abandon_();

            //  someone else beat us to it (ie. latched the page and
            //  probably did the format upgrade while it was at it),
            //  so just go back and wait for the other guy to release
            //  his latch.
            UtilSleep( cmsecWaitWriteLatch );
            goto LatchPage;
        }

        CallS( ErrUPGRADEPossiblyConvertPage( this, m_pgno, ppib->PvRecordFormatConversionBuffer() ) );
        BFDowngradeWriteLatchToReadLatch( &m_bfl );
        Assert( FNewRecordFormat() );
    }
#endif  //  MINIMAL_FUNCTIONALITY

    ASSERT_VALID( this );
#ifdef DEBUG_PAGE
    DebugCheckAll();
#endif  // DEBUG_PAGE

    Ptls()->threadstats.cPageReferenced++;
    OSTraceFMP(
        ifmp,
        JET_tracetagCursorPageRefs,
        OSFormat(
            "Session=[0x%p:0x%x] referenced read page=[0x%x:0x%x] of objid=[0x%x:0x%x] [pgflags=0x%x]",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            (ULONG)ifmp,
            pgno,
            (ULONG)ifmp,
            ObjidFDP(),
            FFlags() ) );

HandleError:
    Expected( 0 == ( bflf & ( bflfNoFaultFail ) ) );  //  since this flag is not currently used, we can assert that on err, we never have the latch on the page
    if ( err < 0 )
    {
        Assert( FBFNotLatched( ifmp, pgno ) );
        Assert( FAssertUnused_( ) );
    }
    else
    {
        Assert( FAssertReadLatch() );
    }
    return err;
}

//  ================================================================
ERR CPAGE::ErrGetRDWPage(   PIB * ppib,
                            IFMP ifmp,
                            PGNO pgno,
                            BFLatchFlags bflf,
                            BFLatch* pbflHint )
//  ================================================================
{
    ERR err;

    ASSERT_VALID( ppib );
    Assert( FAssertUnused_( ) );
    Assert( ifmpNil != ifmp );
    Assert( ( m_dbtimePreInit == dbtimeNil ) && ( m_objidPreInit == objidNil ) );

    //  DbScan is special because it scans past the logical EOF (i.e. what RootOE claims) to 
    //  the true physical end of the file / EOF errors (actually past EOF a little because of
    //  the read size).
    //  Also, we can't maintain this during REDO because we may be replaying early logs for a
    //  database that is going to be shrunk in the future, we try to access the page first to
    //  then add it to the list of pages that are expected to be shrunk later.

    Expected( PinstFromIfmp( ifmp )->m_plog->FRecoveringMode() == fRecoveringRedo ||
                pgno <= g_rgfmp[ifmp].PgnoLast() ||
                bflf & bflfDBScan ||    // my how quickly this leaks past intended usage!
                g_rgfmp[ifmp].FOlderDemandExtendDb() ||
                g_rgfmp[ifmp].FShrinkDatabaseEofOnAttach() );

    //  if we were given a hint for this page or it is the same page that
    //  we latched last time, we will attempt to use the hint to latch the
    //  page more quickly
    m_bfl.dwContext = m_pgno == pgno ? m_bfl.dwContext : NULL;
    m_bfl.dwContext = pbflHint ? pbflHint->dwContext : m_bfl.dwContext;
    m_platchManager = &g_bfLatchManager;
    m_fSmallFormat  = FIsSmallPage( g_rgfmp[ifmp].CbPage() );

    //  get the page
    Call( ErrBFRDWLatchPage( &m_bfl, ifmp, pgno, BFLatchFlags( bflf | bflfHint ), ppib->BfpriPriority( ifmp ), TcCurr() ) );

    //  we may need to refresh the latch hint
    if ( err != JET_errSuccess && pbflHint )
    {
        //  get the latch hint for the latched BF
        DWORD_PTR dwHint = BFGetLatchHint( &m_bfl );

        //  refresh the hint if it has changed
        if ( pbflHint->dwContext != dwHint )
        {
            pbflHint->dwContext = dwHint;
        }
    }

#ifdef DEBUG
    if ( !FSmallPageFormat() )
    {
        PGHDR2* ppghdr2 = ( PGHDR2* )m_bfl.pv;
        AssertSz( ppghdr2->pgno == pgno,
                  "Inconsistent PGNO found between CPAGE (%d) and PGHDR2! (%d)",
                  pgno,
                  (PGNO)ppghdr2->pgno );
    }
#endif

    //  set CPAGE member variables
    m_ppib          = ppib;
    m_ifmp          = ifmp;
    m_pgno          = pgno;

#ifdef MINIMAL_FUNCTIONALITY
    Enforce( FNewRecordFormat() );
#else
    //  check to see if we need to perform a record format upgrade
    if( !FNewRecordFormat() )
    {
        CallS( ErrBFUpgradeRDWLatchToWriteLatch( &m_bfl ) );

        //  UNDONE: Any reason why we don't allocate the conversion
        //  buffer BEFORE upgrading the page latch?
        if( NULL == ppib->PvRecordFormatConversionBuffer() )
        {
            const ERR   errT    = ppib->ErrAllocPvRecordFormatConversionBuffer();
            if ( errT < 0 )
            {
                BFWriteUnlatch( &m_bfl );
                Abandon_();
                //  only cannibalise error code from ErrBFRDWLatchPage() if we hit an error
                Call( errT );
            }
            CallS( errT );
        }
        CallS( ErrUPGRADEPossiblyConvertPage( this, m_pgno, ppib->PvRecordFormatConversionBuffer() ) );
        BFDowngradeWriteLatchToRDWLatch( &m_bfl );
        Assert( FNewRecordFormat() );
    }
#endif  //  MINIMAL_FUNCTIONALITY

    ASSERT_VALID( this );
    DebugCheckAll();

    Ptls()->threadstats.cPageReferenced++;
    OSTraceFMP(
        ifmp,
        JET_tracetagCursorPageRefs,
        OSFormat(
            "Session=[0x%p:0x%x] referenced RDW page=[0x%x:0x%x] of objid=[0x%x:0x%x] [pgflags=0x%x]",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            (ULONG)ifmp,
            pgno,
            (ULONG)ifmp,
            ObjidFDP(),
            FFlags() ) );

HandleError:
    Expected( 0 == ( bflf & bflfNoFaultFail ) );  //  since this flag is not currently used, we can assert that on err, we never have the latch on the page
    if ( err < 0 )
    {
        Assert( FBFNotLatched( ifmp, pgno ) );
        Assert( FAssertUnused_( ) );
    }
    else
    {
        Assert( FAssertRDWLatch() );
    }
    return err;
}


//  ================================================================
ERR CPAGE::ErrLoadPage(
    PIB * ppib,
    IFMP ifmp,
    PGNO pgno,
    const VOID * pv,
    const ULONG cb )
//  ================================================================
//
//  Loads a CPAGE from an arbitrary chunk of memory
//
//-
{
    Assert( ifmpNil != ifmp );  // either we need an ifmp or must start passing a cb.
    Assert( ( m_dbtimePreInit == dbtimeNil ) && ( m_objidPreInit == objidNil ) );

    m_platchManager = &g_allocatedPageLatchManager;
    m_ppib          = ppib;
    m_ifmp          = ifmp;
    m_pgno          = pgno;
    m_fSmallFormat  = FIsSmallPage( g_rgfmp[ifmp].CbPage() );

    Assert( (ULONG)g_rgfmp[ifmp].CbPage() == cb );

    m_platchManager->AllocateBuffer( &m_bfl, cb );

    Assert( g_loadedPageLatchManager.FIsLoadedPage( &m_bfl ) );

    UtilMemCpy( m_bfl.pv, pv, cb );

    return JET_errSuccess;
}

//  ================================================================
VOID CPAGE::LoadNewPage(
    const IFMP ifmp,
    const PGNO pgno,
    const OBJID objidFDP,
    const ULONG fFlags,
    VOID * const pv,
    const ULONG cb )
//  ================================================================
{
    Assert( 0 != cb );
    Assert( ifmpNil != ifmp );
    Assert( ( m_dbtimePreInit == dbtimeNil ) && ( m_objidPreInit == objidNil ) );

    m_platchManager = &g_loadedPageLatchManager;
    m_fSmallFormat  = FIsSmallPage( cb );

    m_platchManager->SetBuffer( &m_bfl, pv, cb );

    Assert( g_loadedPageLatchManager.FIsLoadedPage( &m_bfl ) );

    PreInitializeNewPage_( ppibNil, ifmp, pgno, objidFDP, 0 );
    ConsumePreInitPage( fFlags );
}

#ifdef ENABLE_JET_UNIT_TEST
//  ================================================================
VOID CPAGE::LoadNewTestPage( __in const ULONG cb, __in const IFMP ifmp )
//  ================================================================
{
    Assert( 0 != cb );

    //  Use the test buffer/latch manager so we can catch overruns with AV
    //

    m_platchManager = &g_testPageLatchManager;
    m_platchManager->AllocateBuffer( &m_bfl, cb );
    Assert( g_loadedPageLatchManager.FIsLoadedPage( &m_bfl ) );

    // Should we init the buffer to garbage?
    //memset( pvPage, chPAGETestPageFill, cbPage );

    //  Set element for tag management
    //

    m_fSmallFormat = FIsSmallPage( cb );

    //  Initialize the Page so it is usable for testing
    //

    PreInitializeNewPage_( ppibNil, ifmp, 2, 3, 0 );
    ConsumePreInitPage( 0x0 );

    //  Avoid Uninitialized Page issues
    //

    PGHDR * ppghdr = (PGHDR*)PvBuffer();
    ppghdr->checksum = 0xf7e97daa;

}
#endif // ENABLE_JET_UNIT_TEST

//  ================================================================
VOID CPAGE::LoadPage( const IFMP ifmp, const PGNO pgno, VOID * const pv, const ULONG cb )
//  ================================================================
//
//  Loads a CPAGE from an arbitrary chunk of memory
//
//-
{
    Assert( ( m_dbtimePreInit == dbtimeNil ) && ( m_objidPreInit == objidNil ) );

    m_ppib          = ppibNil;
    m_ifmp          = ifmp;
    m_pgno          = pgno;

    Assert( cb != 0 );

    m_platchManager = &g_loadedPageLatchManager;
    m_fSmallFormat  = FIsSmallPage( cb );

    m_platchManager->SetBuffer( &m_bfl, pv, cb );

    Assert( g_loadedPageLatchManager.FIsLoadedPage( &m_bfl ) );
}

//  ================================================================
VOID CPAGE::LoadDehydratedPage( const IFMP ifmp, const PGNO pgno, VOID * const pv, const ULONG cb, const ULONG cbPage )
//  ================================================================
//
//  Loads a CPAGE from an arbitrary chunk of memory
//
//-
{
    LoadPage( ifmp, pgno, pv, cbPage );

    m_platchManager->SetBufferSize( &m_bfl, cb );

    Assert( cb != 0 );
    Assert( cb == m_platchManager->CbBuffer( m_bfl ) );
    Assert( cbPage != 0 );
    Assert( cbPage == m_platchManager->CbPage( m_bfl ) );

    Assert( g_loadedPageLatchManager.FIsLoadedPage( &m_bfl ) );
}

//  ================================================================
VOID CPAGE::LoadPage( VOID * const pv, const ULONG cb )
//  ================================================================
//
//  Loads a CPAGE from an arbitrary chunk of memory
//
//-
{
    Assert( cb != 0 );
    LoadPage( ifmpNil, pgnoNull, pv, cb );
}

//  ================================================================
VOID CPAGE::GetShrunkPage(
    const IFMP ifmp,
    const PGNO pgno,
    VOID * const pv,
    const ULONG cb )
//  ================================================================
{
    Assert( 0 != cb );
    Assert( ( ifmpNil != ifmp ) || FNegTest( fInvalidUsage ) );

    m_platchManager = &g_loadedPageLatchManager;
    m_fSmallFormat  = FIsSmallPage( cb );

    m_platchManager->SetBuffer( &m_bfl, pv, cb );

    Assert( g_loadedPageLatchManager.FIsLoadedPage( &m_bfl ) );

    PreInitializeNewPage_( ppibNil, ifmp, pgno, objidNil, dbtimeShrunk );
    ConsumePreInitPage( 0 );

    SetPageChecksum( (BYTE *)m_bfl.pv, CbPage(), databasePage, m_pgno );
}

//  ================================================================
VOID CPAGE::GetRevertedNewPage(
    const PGNO pgno,
    VOID * const pv,
    const ULONG cb )
    //  ================================================================
{
    Assert( 0 != cb );

    m_platchManager = &g_loadedPageLatchManager;
    m_fSmallFormat  = FIsSmallPage( cb );

    m_platchManager->SetBuffer( &m_bfl, pv, cb );

    Assert( g_loadedPageLatchManager.FIsLoadedPage( &m_bfl ) );

    PreInitializeNewPage_( ppibNil, ifmpNil, pgno, objidNil, dbtimeRevert );
    ConsumePreInitPage( 0 );

    SetPageChecksum( (BYTE *)m_bfl.pv, CbPage(), databasePage, m_pgno );
}

//  ================================================================
VOID CPAGE::ReBufferPage( __in const BFLatch& bfl, const IFMP ifmp, const PGNO pgno, VOID * const pv, const ULONG cb )
//  ================================================================
//
//  Loads a CPAGE from an arbitrary chunk of memory
//
//-
{
    LoadPage( ifmp, pgno, pv, cb );

    Assert( (BOOL)FIsSmallPage( cb ) == m_fSmallFormat );

    //  We technically have a loaded page manager right now, but what
    //  we want is the regular buffer manager.  And we have a BFLatch
    //  provided by the buffer manager with which to load our CPAGE.
    Assert( g_loadedPageLatchManager.FIsLoadedPage( &m_bfl ) );

    m_platchManager = &g_bfLatchManager;
    m_bfl = bfl;

    Assert( !g_loadedPageLatchManager.FIsLoadedPage( &m_bfl ) );
}

//  ================================================================
VOID CPAGE::UnloadPage()
//  ================================================================
//
//  Unloads a CPAGE from an arbitrary chunk of memory
//
//-
{
    m_platchManager->FreeBuffer( &m_bfl );
    m_ppib              = ppibNil;
    m_ifmp              = 0;
    m_dbtimePreInit     = dbtimeNil;
    m_pgno              = pgnoNull;
    m_objidPreInit      = objidNil;
    m_platchManager     = &g_nullPageLatchManager;
    m_fSmallFormat      = fFormatInvalid;
}


//  ================================================================
CPAGE& CPAGE::operator=( CPAGE& rhs )
//  ================================================================
//
//  Copies another CPAGE into this one. Ownership of the page is
//  transferred -- the CPAGE being assigned to must not reference
//  a page. This makes sure that there is only ever one copy of a
//  page being used.
//  !! The source CPAGE will no longer reference a page after assignment
//  We do not take a const CPAGE& argument as we modify the argument.
//
//-
{
    ASSERT_VALID( &rhs );
    Assert( &rhs != this );         //  we cannot assign to ourselves
    Assert( FAssertUnused_( ) );

    //  copy the data
    m_ppib          = rhs.m_ppib;
    m_ifmp          = rhs.m_ifmp;
    m_pgno          = rhs.m_pgno;
    m_dbtimePreInit = rhs.m_dbtimePreInit;
    m_objidPreInit  = rhs.m_objidPreInit;
    m_bfl.pv        = rhs.m_bfl.pv;
    m_bfl.dwContext = rhs.m_bfl.dwContext;
    m_platchManager = rhs.m_platchManager;
    m_fSmallFormat  = rhs.m_fSmallFormat;

    //  the source CPAGE loses ownership
    rhs.Abandon_( );
    return *this;
}

//  ================================================================
ERR CPAGE::ErrResetHeader( PIB *ppib, const IFMP ifmp, const PGNO pgno )
//  ================================================================
{
    ERR     err;
    BFLatch bfl;
    auto tc = TcCurr();

    CallR( ErrBFWriteLatchPage( &bfl, ifmp, pgno, bflfNew, ppib->BfpriPriority( ifmp ), tc ) );
    BFDirty( &bfl, bfdfDirty, tc );
    memset( bfl.pv, 0, g_bfLatchManager.CbBuffer( bfl ) ); // s should be just CbBFBufferSize( bfl )???
    BFWriteUnlatch( &bfl );

    return JET_errSuccess;
}


//  ================================================================
VOID CPAGE::SetFEmpty( )
//  ================================================================
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    ULONG ulFlags = ((PGHDR*)m_bfl.pv)->fFlags;
    ulFlags |= fPageEmpty;
    ((PGHDR*)m_bfl.pv)->fFlags = ulFlags;
}


//  ================================================================
VOID CPAGE::SetFNewRecordFormat ( )
//  ================================================================
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    ULONG ulFlags = ((PGHDR*)m_bfl.pv)->fFlags;
    ulFlags |= fPageNewRecordFormat;
    ((PGHDR*)m_bfl.pv)->fFlags = ulFlags;
}


//  ================================================================
VOID CPAGE::SetPgno ( PGNO pgno )
//  ================================================================
{
    if ( !FSmallPageFormat() )
    {
        PGHDR2 * const ppghdr2 = ( PGHDR2* )(m_bfl.pv);
        ppghdr2->pgno = pgno;
    }
}


//  ================================================================
VOID CPAGE::SetPgnoNext ( PGNO pgnoNext )
//  ================================================================
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    ((PGHDR*)m_bfl.pv)->pgnoNext = pgnoNext;
}


//  ================================================================
VOID CPAGE::SetPgnoPrev ( PGNO pgnoPrev )
//  ================================================================
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    ((PGHDR*)m_bfl.pv)->pgnoPrev = pgnoPrev;
}


//  ================================================================
VOID CPAGE::SetDbtime( const DBTIME dbtime )
//  ================================================================
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif

    Assert( ((PGHDR*)m_bfl.pv)->dbtimeDirtied <= dbtime ||
        ( FNegTest( fCorruptingWithLostFlush ) && FNegTest( fInvalidUsage ) ) );
    m_platchManager->AssertPageIsDirty( m_bfl );
    Assert( FAssertWriteLatch( ) );

    ((PGHDR*)m_bfl.pv)->dbtimeDirtied = dbtime;
}


//  ================================================================
VOID CPAGE::RevertDbtime( const DBTIME dbtime, const ULONG fFlags )
//  ================================================================
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    Assert( ((PGHDR*)m_bfl.pv)->dbtimeDirtied >= dbtime );
    Expected( fFlags != 0 );
    m_platchManager->AssertPageIsDirty( m_bfl );
    Assert( FAssertWriteLatch( ) );

    ((PGHDR*)m_bfl.pv)->dbtimeDirtied = dbtime;

    // We also revert the scrubbed flag, as this is party of the "dirtied"/dbtime
    // state with the way scrubbed was implemented embedded in ::Dirty() and ::DirtyForScrub().
    // Still, we don't expect any other flags to change other than fPageScrubbed.
    // If the FireWall() below goes off, it doesn't necessarily mean we have
    // a corruption problem, but it means there will be a divergence between
    // copies in a replicated system that may triger a DB divergence error.
#ifndef ENABLE_JET_UNIT_TEST
    if ( ( FFlags() | fPageScrubbed ) != ( fFlags | fPageScrubbed ) )
    {
        FireWall( OSFormat( "RevertDbtime:0x%I32x:0x%I32x", fFlags, FFlags() ) );
    }
#endif
    const BOOL fScrubbedBefore = ( fFlags & fPageScrubbed );
    if ( !FScrubbed() != !fScrubbedBefore )
    {
        SetFScrubbedValue_( fScrubbedBefore );
    }
}


//  ================================================================
VOID CPAGE::SetCbUncommittedFree ( INT cbUncommittedFreeNew )
//  ================================================================
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
#ifdef DEBUG
    const INT   cbUncommittedFreeOld = ((PGHDR*)m_bfl.pv)->cbUncommittedFree;

    Unused( cbUncommittedFreeOld );
#endif  //  DEBUG
    ((PGHDR*)m_bfl.pv)->cbUncommittedFree = USHORT( cbUncommittedFreeNew );
    Assert( CbUncommittedFree() <= CbPageFree() );
}


//  ================================================================
VOID CPAGE::AddUncommittedFreed ( INT cbToAdd )
//  ================================================================
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    Assert( ((PGHDR*)m_bfl.pv)->cbUncommittedFree + cbToAdd > ((PGHDR*)m_bfl.pv)->cbUncommittedFree );
    ((PGHDR*)m_bfl.pv)->cbUncommittedFree = USHORT( ((PGHDR*)m_bfl.pv)->cbUncommittedFree + cbToAdd );
    Assert( CbUncommittedFree() <= CbPageFree() );
}


//  ================================================================
VOID CPAGE::ReclaimUncommittedFreed ( INT cbToReclaim )
//  ================================================================
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    Assert( ((PGHDR*)m_bfl.pv)->cbUncommittedFree - cbToReclaim < ((PGHDR*)m_bfl.pv)->cbUncommittedFree );
    Assert( ((PGHDR*)m_bfl.pv)->cbUncommittedFree - cbToReclaim >= 0 );
    ((PGHDR*)m_bfl.pv)->cbUncommittedFree = USHORT( ((PGHDR*)m_bfl.pv)->cbUncommittedFree - cbToReclaim );
    Assert( CbUncommittedFree() <= CbPageFree() );
}

//  ================================================================
VOID CPAGE::MarkAsSuperCold( )
//  ================================================================
//
//  Mark the page as super cold. The page is required to be latched.
//
//-
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif  //  DEBUG_PAGE

    m_platchManager->MarkAsSuperCold( &m_bfl );

    return;
}

//  ================================================================
BOOL CPAGE::FLoadedPage ( ) const
//  ================================================================
{
    return m_platchManager->FLoadedPage();
}

//  ================================================================
BOOL CPAGE::FIsNormalSized  ( ) const
//  ================================================================
{
    return CbPage() == m_platchManager->CbBuffer( m_bfl );
}

//  ================================================================
VOID CPAGE::Dirty( const BFDirtyFlags bfdf )
//  ================================================================
{
    Dirty_( bfdf );
    UpdateDBTime_();
    ResetFScrubbed_();
    m_platchManager->AssertPageIsDirty( m_bfl );
}


//  ================================================================
VOID CPAGE::DirtyForScrub( )
//  ================================================================
{
    PageAssertTrack( *this, !FScrubbed(), "DoubleScrubbing" );
    Assert( TcCurr().iorReason.Iort() == iortScrubbing );
    Dirty_( bfdfDirty );
    UpdateDBTime_();
    SetFScrubbed_();
    m_platchManager->AssertPageIsDirty( m_bfl );
}


//  ================================================================
VOID CPAGE::CoordinatedDirty( const DBTIME dbtime, const BFDirtyFlags bfdf )
//  ================================================================
//
//  called for multi-page operations to coordinate the dbtime of
//  all the pages involved in the operation with one dbtime
//
//-
{
    Dirty_( bfdf );
    CoordinateDBTime_( dbtime );
    ResetFScrubbed_();
    m_platchManager->AssertPageIsDirty( m_bfl );
}


//  ================================================================
BFLatch* CPAGE::PBFLatch ( )
//  ================================================================
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    return m_platchManager->PBFLatch( &m_bfl );
}

//  ================================================================
VOID CPAGE::GetLatchHint( INT iline, BFLatch** ppbfl ) const
//  ================================================================
//
//  returns the pointer to a BFLatch that may contain a latch hint for
//  the page stored in the specified line or NULL if no hint is available.
//  the pointer can only be used by the CPAGE::ErrGetReadPage() and
//  CPAGE::ErrGetRDWPage()
//
//-
{
    Assert( ppbfl );
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    Assert( fPageLeaf == 2 );

#ifdef MINIMAL_FUNCTIONALITY

    *ppbfl = NULL;

#else  //  !MINIMAL_FUNCTIONALITY

    if( FLoadedPage() )
    {
        *ppbfl = NULL;
    }
    else
    {
        *ppbfl = (BFLatch*)( (BYTE*)( rgdwHintCache + ( ( m_pgno * 128 + iline ) & maskHintCache ) ) - OffsetOf( BFLatch, dwContext ) );
        *ppbfl = (BFLatch*)( DWORD_PTR( *ppbfl ) & ~( LONG_PTR( (DWORD_PTR)(((PGHDR*)m_bfl.pv)->fFlags) << ( sizeof( DWORD_PTR ) * 8 - 2 ) ) >> ( sizeof( DWORD_PTR ) * 8 - 1 ) ) );
    }

#endif  //  MINIMAL_FUNCTIONALITY
}

//  ================================================================
VOID CPAGE::UpdateDBTime_( )
//  ================================================================
{
    PGHDR *ppghdr = (PGHDR*)m_bfl.pv;

    if ( PinstFromIfmp( m_ifmp )->m_plog->FRecoveringMode() != fRecoveringRedo ||
        g_rgfmp[ m_ifmp ].FCreatingDB() )
    {
        ppghdr->dbtimeDirtied = g_rgfmp[ m_ifmp ].DbtimeIncrementAndGet();
        Assert( 2 == m_pgno || ppghdr->dbtimeDirtied > 1 );
    }
    else
    {
        //  Do not set the dbtime for redo time
        Assert( FAssertWriteLatch() );
    }
}

//  ================================================================
VOID CPAGE::CoordinateDBTime_( const DBTIME dbtime )
//  ================================================================
//
//  called for multi-page operations to coordinate the dbtime of
//  all the pages involved in the operation with one dbtime
//
//-
{
    PGHDR   * const ppghdr  = (PGHDR*)m_bfl.pv;

    Assert( ppghdr->dbtimeDirtied <= dbtime );
    ppghdr->dbtimeDirtied = dbtime;
}


//  ================================================================
VOID CPAGE::SetLgposModify( LGPOS lgpos )
//  ================================================================
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif  //  DEBUG_PAGE
    Assert( FAssertWriteLatch( ) );

    if ( g_rgfmp[ m_ifmp ].FLogOn() )
    {
        Assert( !PinstFromIfmp( m_ifmp )->m_plog->FLogDisabled() );
        m_platchManager->SetLgposModify( &m_bfl, lgpos, m_ppib->lgposStart, TcCurr() );
    }
}


//  ================================================================
VOID CPAGE::Replace( INT iline, const DATA * rgdata, INT cdata, INT fFlags )
//  ================================================================
//
//  Replaces the specified line. Replace() may reorganize the page.
//
//-
{
    Assert( iline >= 0 );
    Assert( rgdata );
    Assert( cdata > 0 );
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif  //  DEBUG_PAGE

    Replace_( iline + ctagReserved, rgdata, cdata, fFlags );
}


//  ================================================================
VOID CPAGE::ReplaceFlags( INT iline, INT fFlags )
//  ================================================================
//
//  Sets the flags on the specified line.
//
//-
{
    Assert( iline >= 0 );
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif  //  DEBUG_PAGE

    ReplaceFlags_( iline + ctagReserved, fFlags );
}


//  ================================================================
VOID CPAGE::Insert( INT iline, const DATA * rgdata, INT cdata, INT fFlags )
//  ================================================================
//
//  Insert a new line into the page at the given location. This may
//  cause the page to be reorganized.
//
//-
{
    Assert( iline >= 0 );
    Assert( rgdata );
    Assert( cdata > 0 );
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif  //  DEBUG_PAGE

#ifdef DEBUG
    if( FExpensiveDebugCodeEnabled( Debug_Page_Shake ) )
    {
        //  force a reorganization
        DebugMoveMemory_();
    }
#endif

    Insert_( iline + ctagReserved, rgdata, cdata, fFlags );
}


//  ================================================================
VOID CPAGE::Delete( INT iline )
//  ================================================================
//
//  Deletes a line, freeing its space on the page
//
//-
{
    Assert( iline >= 0 );
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif  //  DEBUG_PAGE

    Delete_( iline + ctagReserved );
}


//  ================================================================
VOID CPAGE::SetExternalHeader( const DATA * rgdata, INT cdata, INT fFlags )
//  ================================================================
//
//  Sets the external header.
//
//-
{
    Assert( rgdata );
    Assert( cdata > 0 );
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif  //  DEBUG_PAGE

    Replace_( 0, rgdata, cdata, fFlags );
}


//  ================================================================
VOID CPAGE::ReleaseWriteLatch( BOOL fTossImmediate )
//  ================================================================
//
//  Calls the buffer manager to write unlatch the page. Abandons ownership of the
//  page.
//
//-
{
    ASSERT_VALID( this );
    DebugCheckAll();
    Assert( ( m_dbtimePreInit == dbtimeNil ) == ( m_objidPreInit == objidNil ) );

    // Revert to pre-init state if we haven't made all the way through initialization.
    if ( m_dbtimePreInit != dbtimeNil )
    {
        m_platchManager->AssertPageIsDirty( m_bfl );

        Assert( m_ppib != ppibNil );
        Assert( m_ifmp != ifmpNil );
        Assert( m_pgno != pgnoNull );
        PreInitializeNewPage_( m_ppib, m_ifmp, m_pgno, m_objidPreInit, m_dbtimePreInit );
        Assert( FPreInitPage() );

        m_dbtimePreInit = dbtimeNil;
        m_objidPreInit = objidNil;
    }

    m_platchManager->ReleaseWriteLatch( &m_bfl, !!fTossImmediate );
    Abandon_();
    Assert( FAssertUnused_( ) );
}

//  ================================================================
VOID CPAGE::ReleaseRDWLatch( BOOL fTossImmediate )
//  ================================================================
//
//  Calls the buffer manager to RDW unlatch the page. Abandons ownership of the
//  page.
//
//-
{
    ASSERT_VALID( this );
    DebugCheckAll();

    m_platchManager->ReleaseRDWLatch( &m_bfl, !!fTossImmediate );
    Abandon_();
    Assert( FAssertUnused_( ) );
}


//  ================================================================
VOID CPAGE::ReleaseReadLatch( BOOL fTossImmediate )
//  ================================================================
//
//  Calls the buffer manager to read unlatch the page. Abandons ownership of the
//  page.
//
//-
{
    ASSERT_VALID( this );
#ifdef DEBUG_PAGE
    DebugCheckAll();
#endif  // DEBUG_PAGE

    m_platchManager->ReleaseReadLatch( &m_bfl, !!fTossImmediate );
    Abandon_();
    Assert( FAssertUnused_( ) );
}


//  ================================================================
ERR CPAGE::ErrUpgradeReadLatchToWriteLatch( )
//  ================================================================
//
//  Upgrades a read latch to a write latch. If this fails the page is abandoned.
//
//-
{
    ASSERT_VALID( this );
    Assert( FAssertReadLatch() );
    Assert( m_ifmp != ifmpNil );
    Assert( PgnoThis() <= g_rgfmp[m_ifmp].PgnoLast() );

    ERR err = JET_errSuccess;
    CallJ( m_platchManager->ErrUpgradeReadLatchToWriteLatch( &m_bfl ), Abandon );
    Assert( JET_errSuccess != err || FAssertWriteLatch( ) );

    goto HandleError;

Abandon:
    BFReadUnlatch( &m_bfl );
    Abandon_( );

HandleError:
    return err;
}


//  ================================================================
VOID CPAGE::UpgradeRDWLatchToWriteLatch( )
//  ================================================================
//
//  Upgrades a RDW latch to a write latch. this upgrade cannot fail.
//
//-
{
    ASSERT_VALID( this );
    Assert( FAssertRDWLatch() );

    // During redo, there are cases in which there is page dependency (e.g. source/destination in ErrLGRIIRedoPageMove),
    // so if we need to redo the destination but not the source, we'll load the source in memory outside of BF
    // to consume it while redoing the operation. Because of shrink that loaded page might be beyond EOF, so ignore
    // those cases because we aren't actually write-latching a page in the cache and therefore aren't going to extend
    // the file unexpectedly.
    Expected( PgnoThis() <= g_rgfmp[m_ifmp].PgnoLast() || g_rgfmp[m_ifmp].FOlderDemandExtendDb() || FLoadedPage() );

    m_platchManager->UpgradeRDWLatchToWriteLatch( &m_bfl );
    Assert( FAssertWriteLatch( ) );
}

//  ================================================================
ERR CPAGE::ErrTryUpgradeReadLatchToWARLatch()
//  ================================================================
{
    ASSERT_VALID( this );
    Assert( FAssertReadLatch() );
    Assert( PgnoThis() <= g_rgfmp[m_ifmp].PgnoLast() );

    const ERR err = m_platchManager->ErrTryUpgradeReadLatchToWARLatch( &m_bfl );
    Assert( JET_errSuccess != err || FAssertWARLatch( ) );
    return err;
}

//  ================================================================
VOID CPAGE::UpgradeRDWLatchToWARLatch()
//  ================================================================
{
    ASSERT_VALID( this );
    Assert( FAssertRDWLatch() );
    Assert( PgnoThis() <= g_rgfmp[m_ifmp].PgnoLast() );

    m_platchManager->UpgradeRDWLatchToWARLatch( &m_bfl );
    Assert( FAssertWARLatch( ) );
}

//  ================================================================
VOID CPAGE::DowngradeWriteLatchToRDWLatch( )
//  ================================================================
//
//  Calls the buffer manager to downgrade Write latch to RDW latch.
//
//-
{
    ASSERT_VALID( this );
    DebugCheckAll();
    Assert( ( m_dbtimePreInit == dbtimeNil ) && ( m_objidPreInit == objidNil ) );

    Assert( FAssertWriteLatch( ) );
    m_platchManager->DowngradeWriteLatchToRDWLatch( &m_bfl );
    Assert( FAssertRDWLatch( ) );
}


//  ================================================================
VOID CPAGE::DowngradeWriteLatchToReadLatch( )
//  ================================================================
//
//  Calls the buffer manager to downgrade Write latch to Read latch.
//
//-
{
    ASSERT_VALID( this );
    DebugCheckAll();
    Assert( ( m_dbtimePreInit == dbtimeNil ) && ( m_objidPreInit == objidNil ) );

    Assert( FAssertWriteLatch() );
    m_platchManager->DowngradeWriteLatchToReadLatch( &m_bfl );
    Assert( FAssertReadLatch( ) );
}

//  ================================================================
VOID CPAGE::DowngradeWARLatchToRDWLatch()
//  ================================================================
{
    ASSERT_VALID( this );
    DebugCheckAll();

    Assert( FAssertWARLatch( ) );
    m_platchManager->DowngradeWARLatchToRDWLatch( &m_bfl );
    Assert( FAssertRDWLatch( ) );
}

//  ================================================================
VOID CPAGE::DowngradeWARLatchToReadLatch()
//  ================================================================
{
    ASSERT_VALID( this );
    DebugCheckAll();

    Assert( FAssertWARLatch() );
    m_platchManager->DowngradeWARLatchToReadLatch( &m_bfl );
    Assert( FAssertReadLatch( ) );
}

//  ================================================================
VOID CPAGE::DowngradeRDWLatchToReadLatch( )
//  ================================================================
//
//  Calls the buffer manager to downgrade RDW latch to Read latch.
//
//-
{
    ASSERT_VALID( this );
    DebugCheckAll();

    Assert( FAssertRDWLatch() );
    m_platchManager->DowngradeRDWLatchToReadLatch( &m_bfl );
    Assert( FAssertReadLatch( ) );
}

//  ================================================================
VOID CPAGE::SetFScrubbedValue_( const BOOL fValue )
//  ================================================================
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    const ULONG ulFlags = ((PGHDR*)m_bfl.pv)->fFlags;
    if ( fValue )
    {
        ((PGHDR*)m_bfl.pv)->fFlags = ulFlags | fPageScrubbed;
    }
    else
    {
        ((PGHDR*)m_bfl.pv)->fFlags = ulFlags & ~fPageScrubbed;
    }
}


//  ================================================================
VOID CPAGE::SetFScrubbed_( )
//  ================================================================
{
    SetFScrubbedValue_( fTrue );
}


//  ================================================================
VOID CPAGE::ResetFScrubbed_( )
//  ================================================================
{
    SetFScrubbedValue_( fFalse );
}


//  ================================================================
BOOL CPAGE::FLastNodeHasNullKey() const
//  ================================================================
{
    const PGHDR * const ppghdr  = (PGHDR*)m_bfl.pv;
    LINE                line;

    //  should only be called for internal pages
    Assert( FInvisibleSons() );

    //  must be at least one node on the page
    Assert( ppghdr->itagMicFree > ctagReserved );
    GetPtr( ppghdr->itagMicFree - ctagReserved - 1, &line );

    //  internal nodes can't be marked versioned or deleted
    Assert( !( line.fFlags & (fNDVersion|fNDDeleted) ) );

    return ( !( line.fFlags & fNDCompressed )
            && cbKeyCount + sizeof(PGNO) == line.cb
            && 0 == *(UnalignedLittleEndian<SHORT> *)line.pv );
}


//  ================================================================
BOOL CPAGE::FPageIsInitialized( __in const void* const pv, __in ULONG cb )
//  ================================================================
{
    Assert ( pv != NULL );

    //  Compare pv to zero memory repeatedly
    const BYTE rgbZeroMem[256] = { 0 };
    const BYTE* pvAddrToCompare = (BYTE*)pv;
    //  Assert here for safe, so we'll know in case pvLastAddrToCompare can possibly get minus...
    Assert ( cb > sizeof ( rgbZeroMem ) );
    const BYTE* const pvPageEnd = reinterpret_cast<const BYTE*>(pv) + cb;
    const BYTE* const pvLastAddrToCompare = (BYTE*)( pvPageEnd - sizeof ( rgbZeroMem ) );
    for (; pvAddrToCompare <= pvLastAddrToCompare; pvAddrToCompare += sizeof ( rgbZeroMem ) )
    {
        if ( memcmp ( pvAddrToCompare, rgbZeroMem, sizeof ( rgbZeroMem ) ) != 0 )
        {
            return fTrue;
        }
    }

    //  There might be some bytes less than sizeof(rgbZeroMem) not compared in the loop
    if ( pvAddrToCompare < pvPageEnd )
    {
        return ( memcmp ( pvAddrToCompare, rgbZeroMem, pvPageEnd - pvAddrToCompare ) != 0 );
    }

    return fFalse;
}

//  ================================================================
BOOL CPAGE::FPageIsInitialized() const
//  ================================================================
{
    //  If checksum is zero, the checksum can be wrong, let's check
    //  other fields and even the whole page content and report
    //  initialized if any fields or content is non-zero,
    //  then later we'll know the checksum is not correct
    //  and get the page patched.
    //  We are actually raising the proportion of page patching compared to
    //  solely rely on checksum check.
    return CPAGE::FPageIsInitialized( m_bfl.pv, CbPage() );
}

//  ================================================================
BOOL CPAGE::FShrunkPage ( ) const
//  ================================================================
{
    return dbtimeShrunk == Dbtime();
}


//  ================================================================
VOID CPAGE::ResetAllFlags( INT fFlags )
//  ================================================================
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif  //  DEBUG_PAGE
    Assert( FAssertWriteLatch() || FAssertWARLatch() );

    const BOOL fSmallFormat = FSmallPageFormat();

    for ( INT itag = ((PGHDR*)m_bfl.pv)->itagMicFree - 1; itag >= ctagReserved; itag-- )
    {
        TAG * const ptag = PtagFromItag_( itag );
        ptag->SetFlags( this, USHORT( ptag->FFlags( this, fSmallFormat ) & ~fFlags ) );
    }

    DebugCheckAll();
}


//  ================================================================
bool CPAGE::FAnyLineHasFlagSet( INT fFlags ) const
//  ================================================================
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif  //  DEBUG_PAGE

    const BOOL fSmallFormat = FSmallPageFormat();

    for ( INT itag = ( ( PGHDR* )m_bfl.pv )->itagMicFree - 1; itag >= ctagReserved; itag-- )
    {
        const TAG* const ptag = PtagFromItag_( itag );

        if ( ptag->FFlags( this, fSmallFormat ) & fFlags )
        {
            return true;
        }
    }

    return false;
}


//  ================================================================
VOID CPAGE::Dirty_( const BFDirtyFlags bfdf )
//  ================================================================
//
//  Tells the buffer manager that the page has been modified.
//  Checking the flags is cheaper than setting them (don't need a
//  critical section) so we check the flags to avoid setting them
//  redundantly.
//
//-
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif  //  DEBUG_PAGE
    Assert( FAssertWriteLatch() ||
            FAssertWARLatch() );
    //  for now, but someday in the future we may allow dirty small pages
    Assert( FIsNormalSized() );

    if( FLoadedPage() )
    {
    }
    else
    {
        //  dirty the buffer.  if the page is in MSOShadow, make it filthy to force
        //  it to disk ASAP

        BFDirtyFlags bfdfT = bfdf;
        TraceContext tc = TcCurr();

        //  for the shadow catalog override changes and send them directly to disk

        if ( objidFDPMSOShadow == ((PGHDR*)m_bfl.pv)->objidFDP
            && !PinstFromIfmp( m_ifmp )->FRecovering()
            && !FFMPIsTempDB( m_ifmp )
            && FLeafPage()
            && !FSpaceTree()
            && !g_fEseutil
            && bfdfDirty == bfdf )
        {
            bfdfT = bfdfFilthy;
        }

        m_platchManager->Dirty( &m_bfl, bfdfT, tc );

        //  We do not set pbf->lgposBegin0 until we update lgposModify.
        //  The reason is that we do not log deferred Begin0 until we issue
        //  the first log operation. Currently the sequence is
        //      Dirty -> Update (first) Op -> Log update Op -> update lgposModify and lgposStart
        //  Since we may not have lgposStart until the deferred begin0 is logged
        //  when the first Log Update Op is issued for this transaction.
        //
        //  During redo, since we do not log update op, so the lgposStart will not
        //  be logged, so we have to do it here (dirty).

        if ( g_rgfmp[ m_ifmp ].FLogOn() && PinstFromIfmp( m_ifmp )->m_plog->FRecoveringMode() == fRecoveringRedo )
        {
            Assert( !PinstFromIfmp( m_ifmp )->m_plog->FLogDisabled() );
            Assert( CmpLgpos( &m_ppib->lgposStart, &lgposMax ) != 0 );
            Assert( CmpLgpos( &m_ppib->lgposStart, &lgposMin ) != 0 );
            BFSetLgposModify( &m_bfl, PinstFromIfmp( m_ifmp )->m_plog->LgposLGLogTipNoLock() );
            BFSetLgposBegin0( &m_bfl, m_ppib->lgposStart, tc );
        }
    }
}

//  ================================================================
VOID CPAGE::OverwriteUnusedSpace( const CHAR chZero )
//  ================================================================
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    DEBUG_SNAP_PAGE();

    const PGHDR * const ppghdr = (PGHDR*)m_bfl.pv;

    //  fully compact the page if necessary
    if ( CbContiguousFree_( ) != ppghdr->cbFree )
    {
        ZeroOutGaps_( chZero );
    }

    BYTE * const pbFree = PbFromIb_( ppghdr->ibMicFree );
    memset( pbFree, chZero, CbContiguousFree_() );

    DEBUG_SNAP_COMPARE();
    DebugCheckAll( );
}


//  ================================================================
VOID CPAGE::ReorganizePage(
    __out const VOID ** pvHeader,
    __out size_t * const  pcbHeader,
    __out const VOID ** pvTrailer,
    __out size_t * const pcbTrailer)
//  ================================================================
{
    ASSERT_VALID( this );
    Assert( pvHeader );
    Assert( pcbHeader );
    Assert( pvTrailer );
    Assert( pcbTrailer );
    Assert( pvHeader != pvTrailer );
    Assert( pcbHeader != pcbTrailer );
    Assert( ctagReserved > 0 );
    Assert( !FPreInitPage() );

    *pvHeader = NULL;
    *pcbHeader = 0;
    *pvTrailer = NULL;
    *pcbTrailer = 0;

    const PGHDR * const ppghdr = (PGHDR*)m_bfl.pv;

    //  fully compact the page if necessary
    if ( CbContiguousFree_( ) != ppghdr->cbFree )
    {
        ReorganizeData_( reorgPageMoveLogging );
    }

    // The page now looks like this:
    //  HEADER | LINES | FREE SPACE | TAGS
    //
    const BYTE * const pbMin    = (BYTE *)m_bfl.pv;
    const BYTE * const pbFree   = PbFromIb_( ppghdr->ibMicFree );
    const BYTE * const pbTags   = (BYTE *)PtagFromItag_( ppghdr->itagMicFree-1 );
    const BYTE * const pbMax    = pbMin + m_platchManager->CbBuffer( m_bfl );

    Assert( FOnPage( pbFree, 0 ) );
    Assert( FOnPage( pbTags, CbTagArray_() ) );
    Assert( pbTags >= pbFree );

    Assert( pbFree >= pbMin + CbPageHeader() ); //  stronger and newer (2013/09/11) ... does it hold?
    Assert( CbContiguousFree_() == pbTags - pbFree );

    *pvHeader   = pbMin;
    *pcbHeader  = ( pbFree - pbMin );
    *pvTrailer  = pbTags;
    *pcbTrailer = ( pbMax - pbTags );

    Assert( 0 == ( *pcbTrailer % sizeof(CPAGE::TAG) ) );
    Assert( CbFree_() == CbContiguousFree_() );
    Assert( (size_t)m_platchManager->CbBuffer( m_bfl ) == *pcbHeader + *pcbTrailer + CbContiguousFree_() );

    Assert( NULL != *pvHeader );
    Assert( 0 != *pcbHeader );
    Assert( NULL != pvTrailer );
    Assert( 0 != *pcbTrailer );
    ASSERT_VALID( this );
}

//  ================================================================
XECHECKSUM CPAGE::UpdateLoggedDataChecksum_(
                    const XECHECKSUM checksumLoggedData,
    __in_bcount(cb) const BYTE * const pb,
                    const INT cb )
//  ================================================================
//
//  An optimized version of the hash function from "Algorithms in C++" by Sedgewick
//
//-
{
    XECHECKSUM checksumLoggedDataNew = checksumLoggedData;
    INT ib = 0;
    switch( cb % 8 )
    {
        case 0:
        while ( ib < cb )
        {
            AssumePREFAST( ib < cb );
            checksumLoggedDataNew = ( _rotl64( checksumLoggedDataNew, 6 ) + pb[ib++] );
        case 7:
            AssumePREFAST( ib < cb );
            checksumLoggedDataNew = ( _rotl64( checksumLoggedDataNew, 6 ) + pb[ib++] );
        case 6:
            AssumePREFAST( ib < cb );
            checksumLoggedDataNew = ( _rotl64( checksumLoggedDataNew, 6 ) + pb[ib++] );
        case 5:
            AssumePREFAST( ib < cb );
            checksumLoggedDataNew = ( _rotl64( checksumLoggedDataNew, 6 ) + pb[ib++] );
        case 4:
            AssumePREFAST( ib < cb );
            checksumLoggedDataNew = ( _rotl64( checksumLoggedDataNew, 6 ) + pb[ib++] );
        case 3:
            AssumePREFAST( ib < cb );
            checksumLoggedDataNew = ( _rotl64( checksumLoggedDataNew, 6 ) + pb[ib++] );
        case 2:
            AssumePREFAST( ib < cb );
            checksumLoggedDataNew = ( _rotl64( checksumLoggedDataNew, 6 ) + pb[ib++] );
        case 1:
            AssumePREFAST( ib < cb );
            checksumLoggedDataNew = ( _rotl64( checksumLoggedDataNew, 6 ) + pb[ib++] );
        }
    }

    return checksumLoggedDataNew;
}


//  ================================================================
PAGECHECKSUM CPAGE::LoggedDataChecksum() const
//  ================================================================
//
//  To compare two databases for equivalence we can't just compare the
//  bits. Recovery doesn't set the version bits and scrubbing changes the
//  unused data so the pages will be different. This function generates
//  a checksum which can be used to determine if (for recovery purposes)
//  two pages are the same.
//
//  This means the following data has to be ignored:
//
//      checksum
//      cbUncommittedFree
//      cbFree
//      ibMicFree
//      lost flush detection bit
//      all version flags
//      unused space on the page
//      the data in flag deleted nodes
//
//  In addition we should allow the nodes to be organized in any fashion,
//  currently this isn't strictly necessary but may be desirable in the future
//  (i.e. we don't want this blowing up down the road)
//
//  To do this we will checksum the header, excluding cbUncomittedFree and
//  ibMicFree, then cycle through the nodes on the page and checksum them.
//
//-
{
    const PGHDR2* const ppghdr2 = ( const PGHDR2* )m_bfl.pv;

    XECHECKSUM checksum = 0;

    //  it is easiest to simply copy the header to a temporary header and then checksum it
    //  of course, we MUST use a PGHDR2, and copy in CbPageHeader() bytes
    PGHDR2 pghdr2T;
    const USHORT cbPageHeader = CbPageHeader();
    AssertPREFIX( cbPageHeader <= sizeof( pghdr2T ) );
    memcpy( &pghdr2T, ppghdr2, cbPageHeader );
    pghdr2T.pghdr.checksum          = 0;
    pghdr2T.pghdr.cbUncommittedFree = 0;
    pghdr2T.pghdr.cbFree            = 0;
    pghdr2T.pghdr.ibMicFree         = 0;
    //  flush bit can flip-flop regardless of logging / replay
    //  checksum is not summed in, neither needs bit (which changes/upgrades from dehydration)
    pghdr2T.pghdr.fFlags            &= ~( maskFlushType | fPageNewChecksumFormat );
    memset( pghdr2T.rgChecksum, 0, sizeof( pghdr2T.rgChecksum ) );
    memset( pghdr2T.rgbReserved, 0, sizeof( pghdr2T.rgbReserved ) );
    checksum = UpdateLoggedDataChecksum_( checksum, ( const BYTE* )&pghdr2T, CbPageHeader() );

    const BOOL fSmallFormat = FSmallPageFormat();

    BOOL fCaughtTagNotOnPage = fFalse;
    //  loop through the nodes on the page. checksum the data and then mix in the flags
    for ( INT itagT = 0; itagT < ppghdr2->pghdr.itagMicFree; ++itagT )
    {
        const TAG * const   ptag    = PtagFromItag_( itagT );

        if ( !FOnPage( ptag, sizeof(TAG) ) )
        {
            if ( !fCaughtTagNotOnPage )
            {
                FireWall( "LogicalDataSumTagNotOnPage" );
                fCaughtTagNotOnPage = fTrue;
            }
            continue;
        }

        const INT           cb      = ptag->Cb( fSmallFormat );
        //  An external header can get a zero'd out cb ("essentially deleted"), while leaving 
        //  the ib too high.  So we can't run PbFromIb_() automatically, we must ensure we're
        //  not dealing with one of these zero'd out TAGs.  Dunno if it can happen on any tag
        //  other than the external header.  Once that's fixed, we can move to this simplier
        //  line if we don't care about backwards compat.
        //const BYTE * const    pb      = PbFromIb_( ptag->Ib( fSmallFormat ) );
        const BYTE * const  pb      = cb < cbKeyCount ? NULL : PbFromIb_( ptag->Ib( fSmallFormat ) );
        if( pb && !FOnData( pb, cb ) )
        {
            if ( !FNegTest( fCorruptingPageLogically ) )
            {
                FireWall( "LogicalDataSumPbNotOnPage" );
            }
            continue;
        }

        //  this actually accessed data at pb / PbFromIb( ptag->Ib() )
        const BYTE          bFlags  = (BYTE)(ptag->FFlags( this, fSmallFormat ) & (~fNDVersion));   //  checksum the flags, ignoring the version bit

        Assert( 0 == cb || cbKeyCount <= cb );
        if ( !(ptag->FFlags( this, fSmallFormat ) & fNDDeleted ) && cbKeyCount <= cb )
        {
            AssertRTL( pb == PbFromIb_( ptag->Ib( fSmallFormat ) ) );   //   moved pb = up, checking I didn't alter anything.
            USHORT              cbKey   = ptag->Cb( fSmallFormat );
            Assert( sizeof( cbKey ) == cbKeyCount );

            checksum = UpdateLoggedDataChecksum_( checksum, ( const BYTE* )&cbKey, sizeof( cbKey ) );
            checksum = UpdateLoggedDataChecksum_( checksum, pb + cbKeyCount, cb - cbKeyCount );
        }
        checksum = ( _rotl64( checksum, 6 ) + bFlags );
    }

    return checksum;
}

//  ================================================================
ERR CPAGE::ErrCheckForUninitializedPage_( IPageValidationAction * const pvalidationaction ) const
//  ================================================================
//
//  check if the pgno of the page is pgnoNull
//
//-
{
    Assert( pvalidationaction );
    if ( !FPageIsInitialized() )
    {
        pvalidationaction->UninitPage( m_pgno, JET_errPageNotInitialized );
        return ErrERRCheck( JET_errPageNotInitialized );
    }

    return JET_errSuccess;
}

//  ================================================================
ERR CPAGE::ErrReadVerifyFailure_() const
//  ================================================================
//
//  It is possible to turn off checksum verification. This method
//  should be called when page checksum verification fails. It
//  will return the appropriate error.
//
//-
{
    ERR err = JET_errSuccess;

    if ( !BoolParam( JET_paramDisableBlockVerification ) )
    {
        Error( ErrERRCheck( JET_errReadVerifyFailure ) );
    }
    else if ( m_ifmp != ifmpNil && PinstFromIfmp( m_ifmp )->FRecovering() )
    {
        PAGECHECKSUM    checksumStoredInHeader  = 0;
        PAGECHECKSUM    checksumComputedOffData = 0;

        ChecksumPage(
                    m_bfl.pv,
                    CbPage(),
                    databaseHeader,
                    0,
                    &checksumStoredInHeader,
                    &checksumComputedOffData );
        if ( checksumStoredInHeader == checksumComputedOffData )
        {
            //  We found a database header in the database during recovery.
            //  that means that we found the patch page. We must return
            //  JET_errReadVerifyFailure even if JET_paramDisableBlockVerification
            //  is enabled in this case so that recovery will work.
            //
            Error( ErrERRCheck( JET_errReadVerifyFailure ) );
        }
    }
HandleError:
    return err;
}

//  ================================================================
VOID CPAGE::ReportReadLostFlushVerifyFailure_(
    IPageValidationAction * const pvalidationaction,
    const FMPGNO fmpgno,
    const PageFlushType pgftExpected,
    const BOOL fUninitializedPage,
    const BOOL fRuntime,
    const BOOL fFailOnRuntimeOnly ) const
//  ================================================================
{
    const PageFlushType pgftActual = Pgft();
    pvalidationaction->LostFlush(
        m_pgno,
        fmpgno,
        (INT)pgftExpected,
        (INT)pgftActual,
        JET_errReadLostFlushVerifyFailure,
        fRuntime,
        fFailOnRuntimeOnly );

    if ( g_rgfmp != NULL )
    {
        WCHAR wszLostFlushMessage[200];
        if ( fUninitializedPage )
        {
            OSStrCbFormatW( wszLostFlushMessage, sizeof(wszLostFlushMessage), L"DB Pgno: %u (empty page), Flush Map Pgno: %d, Flush State: %d (expected), Runtime: %d\n", m_pgno, fmpgno, pgftExpected, (INT)fRuntime );
        }
        else
        {
            OSStrCbFormatW( wszLostFlushMessage, sizeof(wszLostFlushMessage), L"DB Pgno: %u, Flush Map Pgno: %d, Flush State: %d (expected) != %d (actual), Runtime: %d\n", m_pgno, fmpgno, pgftExpected, pgftActual, (INT)fRuntime );
        }
        (void)ErrDumpToIrsRaw( L"LostFlush", wszLostFlushMessage ); // event in pvalidationaction->LostFlush().
    }
}

//  ================================================================
VOID CPAGE::SetFlushType_( __in const CPAGE::PageFlushType pgft )
//  ================================================================
{
    ((PGHDR*)m_bfl.pv)->fFlags = ( ((PGHDR*)m_bfl.pv)->fFlags & ~maskFlushType ) | ( (ULONG)pgft << 15 );
}

//  ================================================================
ERR CPAGE::ErrValidatePage(
    __in const PAGEValidationFlags pgvf,
    __in IPageValidationAction * const pvalidationaction,
    __in CFlushMap* pflushmap )
//  ================================================================
{
    Assert( pgvf < pgvfMax );
    Assert( pvalidationaction );
    Assert( FIsNormalSized() );

    ERR err = JET_errSuccess;
    PAGECHECKSUM checksumStoredInHeader = 0;
    PAGECHECKSUM checksumComputedOffData = 0;
    const BOOL fFixErrors = ( pgvf & pgvfFixErrors ) != 0;
    const BOOL fExtensiveChecks = ( pgvf & pgvfExtensiveChecks ) != 0;
    const BOOL fCheckForLostFlush = ( pgvf & pgvfDoNotCheckForLostFlush ) == 0;
    const BOOL fFailOnRuntimeLostFlushOnly = ( pgvf & pgvfFailOnRuntimeLostFlushOnly ) != 0;
    const BOOL fCheckForLostFlushIfNotRuntime = ( pgvf & pgvfDoNotCheckForLostFlushIfNotRuntime ) == 0;
    PageFlushType pgftExpected = pgftUnknown;
    BOOL fRuntimeFlushState = fFalse;

#ifndef ENABLE_JET_UNIT_TEST
    // An externally provided flush map is used exclusively in unit testing, when we may not have a valid
    // FMP.
    Assert( pflushmap == NULL );
#endif // !ENABLE_JET_UNIT_TEST

    // Set the correct flush map if necessary.
    if ( fCheckForLostFlush )
    {
        if ( ( pflushmap == NULL ) && ( g_rgfmp != NULL ) && ( m_ifmp != ifmpNil ) )
        {
            pflushmap = g_rgfmp[ m_ifmp ].PFlushMap();
        }

        AssertSz( pflushmap != NULL, "We must have a flush map available." );
    }

    err = ErrCheckForUninitializedPage_( pvalidationaction );

    //  if we expect a specific flush state, fail out with that error.

    if ( fCheckForLostFlush &&
        ( err == JET_errPageNotInitialized ) &&
        ( ( pgftExpected = pflushmap->PgftGetPgnoFlushType( m_pgno, dbtimeNil, &fRuntimeFlushState ) ) != pgftUnknown ) &&
        ( fRuntimeFlushState || fCheckForLostFlushIfNotRuntime ) )
    {
        ReportReadLostFlushVerifyFailure_(
            pvalidationaction,
            pflushmap->FmpgnoGetFmPgnoFromDbPgno( m_pgno ),
            pgftExpected,
            fTrue,  // fUninitializedPage
            fRuntimeFlushState,
            fFailOnRuntimeLostFlushOnly );

        if ( fRuntimeFlushState || !fFailOnRuntimeLostFlushOnly )
        {
            Error( ErrERRCheck( JET_errReadLostFlushVerifyFailure ) );
        }
    }

    Call( err );

    BOOL            fCorrectableError   = fFalse;
    INT             ibitCorrupted       = -1;

    ChecksumAndPossiblyFixPage(
                m_bfl.pv,
                CbPage(),
                databasePage,
                m_pgno,
                ( fFixErrors ? fTrue : fFalse ),
                &checksumStoredInHeader,
                &checksumComputedOffData,
                &fCorrectableError,
                &ibitCorrupted );

    if( checksumStoredInHeader != checksumComputedOffData )
    {
        pvalidationaction->BadChecksum( m_pgno, JET_errReadVerifyFailure, checksumStoredInHeader, checksumComputedOffData );
        Call( ErrReadVerifyFailure_() );
    }
    else if( fCorrectableError )
    {
        //  the error was corrected. verify the page

        err = ErrCheckForUninitializedPage_( pvalidationaction );

        if ( fCheckForLostFlush &&
            ( err == JET_errPageNotInitialized ) &&
            ( ( pgftExpected = pflushmap->PgftGetPgnoFlushType( m_pgno, dbtimeNil, &fRuntimeFlushState ) ) != pgftUnknown ) &&
            ( fRuntimeFlushState || fCheckForLostFlushIfNotRuntime ) )
        {
            ReportReadLostFlushVerifyFailure_(
                pvalidationaction,
                pflushmap->FmpgnoGetFmPgnoFromDbPgno( m_pgno ),
                pgftExpected,
                fTrue,  // fUninitializedPage
                fRuntimeFlushState,
                fFailOnRuntimeLostFlushOnly );

            if ( fRuntimeFlushState || !fFailOnRuntimeLostFlushOnly )
            {
                Error( ErrERRCheck( JET_errReadLostFlushVerifyFailure ) );
            }
        }

        Call( err );
        
        err = ErrCheckPage( CPRINTFDBGOUT::PcprintfInstance(), OnErrorReturnError, CheckAll );

        //  the page is fine

        if( JET_errSuccess == err )
        {
            pvalidationaction->BitCorrection( m_pgno, ibitCorrupted );
        }
        else
        {
            pvalidationaction->BitCorrectionFailed( m_pgno, err, ibitCorrupted );
            Call( ErrReadVerifyFailure_() );
        }
    }
    else
    {
    }

    // check for pgno (only possible on large pages)
    if ( !FSmallPageFormat() )
    {
        const PGHDR2* const ppghdr2 = ( const PGHDR2* )m_bfl.pv;
        const PGNO pgnoStoredInHeader = ppghdr2->pgno;

        if ( m_pgno != pgnoStoredInHeader )
        {
            pvalidationaction->BadPgno( m_pgno, pgnoStoredInHeader, JET_errReadPgnoVerifyFailure );
            Error( ErrERRCheck( JET_errReadPgnoVerifyFailure ) );
        }

#ifndef RTM
        //  Ensure that the reserved is in fact zero, not stack trash or something.
        for ( ULONG ibReserved = 0; ibReserved < _countof(ppghdr2->rgbReserved); ibReserved++ )
        {
            AssertRTL( ppghdr2->rgbReserved[ibReserved] == 0 );
        }
#endif
    }

    //  Finally if all that passes, check for a lost flush

    if ( fCheckForLostFlush &&
        ( ( pgftExpected = pflushmap->PgftGetPgnoFlushType( m_pgno, fCheckForLostFlushIfNotRuntime ? Dbtime() : dbtimeNil, &fRuntimeFlushState ) ) != pgftUnknown ) &&
        ( fRuntimeFlushState || fCheckForLostFlushIfNotRuntime ) &&
        ( pgftUnknown != Pgft() ) &&
        ( pgftExpected != Pgft() ) )
    {
        ReportReadLostFlushVerifyFailure_(
            pvalidationaction,
            pflushmap->FmpgnoGetFmPgnoFromDbPgno( m_pgno ),
            pgftExpected,
            fFalse, // fUninitializedPage.
            fRuntimeFlushState,
            fFailOnRuntimeLostFlushOnly );

        if ( fRuntimeFlushState || !fFailOnRuntimeLostFlushOnly )
        {
            Error( ErrERRCheck( JET_errReadLostFlushVerifyFailure ) );
        }
    }

    //  outside of repair, we really expect our pages to be good at this point i.e. we have passed
    //  checksums. for non-RTM builds, we are happy to get a remote / crash-dump if we find we are about
    //  to read/write a page that satisfy basic page/tag invariants. getting these wrong will likely cause
    //  a crash later.
    //
    //  for RTM-builds: there are a large number of crashes in the field. returning an error about a
    //  corrupted database should lead to a better user experience than an enforce here. performance
    //  tests indicated that we can't do the extensive checks here.

    if ( !g_fRepair )
    {
        CPAGE::CheckPageExtensiveness cpe;
        if ( fExtensiveChecks )
        {
            cpe = CPAGE::CheckAll;
        }
        else
        {
            cpe = CPAGE::CheckDefault;
        }
        Call( ErrCheckPage( CPRINTFDBGOUT::PcprintfInstance(),
                            OnErrorFireWall,
                            cpe ) );
    }

#ifdef DEBUG
    //  NULL g_rgfmp happens during unit tests and page patching, so no assert for now - backup seems to
    //  set the ifmp, and operating when there is a valid g_rgfmp
    if ( ( g_rgfmp != NULL ) && ( m_ifmp != ifmpNil ) && g_rgfmp[m_ifmp].FAttached() )
    {
        Assert( PgnoThis() != pgnoNull );

        if ( ( PgnoThis() > g_rgfmp[m_ifmp].PgnoLast() ) &&
             !g_rgfmp[m_ifmp].FOlderDemandExtendDb() )
        {
            PGNO pgnoLastFs = pgnoNull;
            Assert( ( g_rgfmp[m_ifmp].ErrPgnoLastFileSystem( &pgnoLastFs ) < JET_errSuccess ) ||
                    ( PgnoThis() <= pgnoLastFs ) );
        }
    }
#endif  // DEBUG

HandleError:
    return err;
}

//  ================================================================
NOINLINE ERR CPAGE::ErrCaptureCorruptedPageInfo_( const CheckPageMode mode, const PCSTR szCorruptionType, const PCWSTR wszCorruptionDetails, const CHAR * const szFile, const LONG lLine, const BOOL fLogEvent ) const
//  ================================================================
{
    WCHAR wszType[100];
    OSStrCbFormatW( wszType, sizeof( wszType ), L"%hs", szCorruptionType );
    return ErrCaptureCorruptedPageInfo_( mode, wszType, wszCorruptionDetails, szFile, lLine, fLogEvent );
}

//  ================================================================
NOINLINE ERR CPAGE::ErrCaptureCorruptedPageInfo_( const CheckPageMode mode, const PCWSTR szCorruptionType, const PCWSTR szCorruptionDetails, const CHAR * const szFile, const LONG lLine, const BOOL fLogEvent ) const
//  ================================================================
{
    OnRTM( BOOL fFriendyEvent = fFalse );

    //  Log an event about the corruption issue AND spit the details of the in-memory page
    //  out to our .RAW data file for further analysis.

    if ( g_rgfmp != NULL && m_ifmp != ifmpNil )
    {
        if ( fLogEvent )
        {
            const WCHAR *rgpsz[ 5 ];
            rgpsz[0] = g_rgfmp[ m_ifmp ].WszDatabaseName();
            WCHAR szPageNumber[ 32 ];
            OSStrCbFormatW( szPageNumber, sizeof( szPageNumber ), L"%d (0x%X)", PgnoThis(), PgnoThis() );
            rgpsz[1] = szPageNumber;
            WCHAR szObjid[ 16 ];
            OSStrCbFormatW( szObjid, sizeof( szObjid ), L"%u", ObjidFDP() );
            rgpsz[2] = szObjid;
            rgpsz[3] = szCorruptionType;
            rgpsz[4] = szCorruptionDetails ? szCorruptionDetails : L"";

            UtilReportEvent( eventError,
                         DATABASE_CORRUPTION_CATEGORY,
                         DATABASE_PAGE_LOGICALLY_CORRUPT_ID,
                         _countof( rgpsz ),
                         rgpsz,
                         0,
                         NULL,
                         PinstFromIfmp( m_ifmp ) );

            if ( mode == OnErrorFireWall )
            {
                OSUHAEmitFailureTag( PinstFromIfmp( m_ifmp ), HaDbFailureTagCorruption, L"a7334cdf-3710-4341-a887-e7a18a8068db" );
            }

            //  Since we printed a "more specific FireWall" type event, use that for diagnostics and
            //  skip the FireWall( which logs an event ), except in DEBUG or non-RTM, where we have the additional
            //  behavior of crashing / debug breaking.
            OnRTM( fFriendyEvent = fTrue );
        }

        //  To avoid overloading the .IRS.RAW file we'll only track first 4 instances of each pgno.

        //  Note: This might occasionally miss a page in a process w/ many DBs and unlucky two pgnos corrupt,
        //  but that'd be _really_ hard to hit[1].  More likely this will be missing a page dump because after
        //  four pages, this just shuts off dumping to .RAW file.
        //  [1] How hard:
        //       w/ < 8 FMPs: @ 32 KB aliasing at 16 TB, or @ 4KB aliasing at 2 TB
        //       w/ > 8 FMPs: @ 32 KB aliasing at 500 GBs, or @ 4KB aliasing at 62 GBs.
        //  ... fortunately if you have alot of DBs, likely they aren't super large and vice versa.
        if ( g_lesCorruptPages.FNeedToLog( ( ( m_ifmp <= 7 ) ? ( m_ifmp << 29 ) : ( m_ifmp << 24 ) ) | PgnoThis() ) )
        {
            // Have not gotten this page yet, capture it for posterity
            (void)ErrDumpToIrsRaw( szCorruptionType, szCorruptionDetails ? szCorruptionDetails : L"" );
        }
    }

    //  Just return the failures where we are not actually going to force a crash via 
    //  AssertRTL or Enforce.

    if ( mode == OnErrorReturnError )
    {
        return ErrERRCheck_( JET_errDatabaseCorrupted, szFile, lLine );
    }

    CHAR szCorr[ 100 ];
    (void)ErrOSStrCbFormatA( szCorr, sizeof( szCorr ), "LogicalPageCorruption:%ws", szCorruptionType );

#ifdef RTM
    if ( mode == OnErrorFireWall )
    {
        if ( !fFriendyEvent )
        {
            FireWallAt( szCorr, szFile, lLine );
        }

        return ErrERRCheck_( JET_errDatabaseCorrupted, szFile, lLine );
    }
#endif

    //  for some crash dumps (mini dumps or view-cache), we need to see the page that has
    //  the logical corruption. so we copy it here. we use a static variable to make it
    //  easier to debug in retail; it is not thread-safe; so do not use.

    static volatile BYTE *s_rgbBufDebugDoNotUse;
    BYTE *rgbBuf = (BYTE *)_alloca( CbBuffer() );
    s_rgbBufDebugDoNotUse = rgbBuf;

    memcpy( rgbBuf, (BYTE *)PvBuffer(), CbBuffer() );

    //  We'll provide a little more info in case of non-RTM.

    if ( mode == OnErrorFireWall )
    {
        AssertFail( "Page-level Logical corruption detected when page was read in. Pgno = %d (%#x): %ws - %ws", szFile, lLine, PgnoThis(), PgnoThis(), szCorruptionType, szCorruptionDetails );
    }

    if ( mode == OnErrorEnforce )
    {
        EnforceAtSz( fFalse, szCorr, szFile, lLine );
    }

    return ErrERRCheck_( JET_errDatabaseCorrupted, szFile, lLine );
}

//  ================================================================
ERR CPAGE::ErrCheckPage(
    CPRINTF * const pcprintf,
    const CheckPageMode mode, // = OnErrorReturnError
    const CheckPageExtensiveness grbitExtensiveCheck // = CheckDefault
     ) const
//  ================================================================
{
    ERR                 err     = JET_errSuccess;
    const PGHDR * const ppghdr  = (PGHDR*)m_bfl.pv;
    ULONG              *rgdw    = NULL;
    KEY                 keyLast;

    //  that this function is called on every read/write IO operation. However, it plays
    //  a substantial percentage of our CPU cost for deletion scenarios and for blocking the scavenging
    //  thread. additionally, with view cache, read IOs may be very fast because the page may still be
    //  cached by the OS (even though they have been kicked out by BF).

    //  The cbCheckSize, is the size of the buffer if we want to validate the page against a smaller
    //  size, or the size of the page if we want to validate against true page format.

    if ( ppghdr->cbFree > CbBufferData() )
    {
        (*pcprintf)( "page corruption (%d): cbFree too large (%d bytes, expected no more than %d bytes)\r\n",
                        m_pgno, (USHORT)ppghdr->cbFree, CbBufferData() );
        Error( ErrCaptureCorruptedPageInfo( mode, L"CbFreeTooLarge" ) );
    }

    if ( (USHORT) ppghdr->cbUncommittedFree > (USHORT) CbPageFree() )
    {
        (*pcprintf)( "page corruption (%d): cbUncommittedFree too large (%d bytes, CbPageFree() is %d bytes / cbFree is %d bytes)\r\n",
                        m_pgno, (USHORT)ppghdr->cbUncommittedFree, CbPageFree(), (USHORT)ppghdr->cbFree );
        Error( ErrCaptureCorruptedPageInfo( mode, L"CbUncommittedFreeTooLarge" ) );
    }

    if ( ppghdr->ibMicFree > CbBufferData() )
    {
        (*pcprintf)( "page corruption (%d): ibMicFree too large (%d bytes, expected no more than %d bytes)\r\n",
                        m_pgno, (USHORT)ppghdr->ibMicFree, CbBufferData() );
        Error( ErrCaptureCorruptedPageInfo( mode, L"ibMicFreeTooLarge" ) );
    }

    if ( ppghdr->itagMicFree >= ( CbBufferData() / sizeof( TAG ) ) )
    {
        (*pcprintf)( "page corruption (%d): itagMicFree too large (%d bytes)\r\n",
                        m_pgno, (USHORT)ppghdr->itagMicFree );
        Error( ErrCaptureCorruptedPageInfo( mode, L"itagMicFreeTooLarge" ) );
    }

    //  make sure number of lines / itagMicFree variable is consistent

    if ( ( ppghdr->itagMicFree == 0 ) && !FEmptyPage() && !FPreInitPage() )
    {
        (*pcprintf)( "page corruption (%d): Empty page without fPageEmpty or fPagePreInit flags set\r\n", m_pgno );
        Error( ErrCaptureCorruptedPageInfo( mode, L"EmptyAndPreInitFlagsNotSet" ) );
    }

    const __int64 ctags = (__int64)((PGHDR *)m_bfl.pv)->itagMicFree;
    const ULONG_PTR pbPageDataStart = PbDataStart_();  // m_bfl.pv + header size
    const ULONG_PTR pbPageDataEnd = PbDataEnd_();      // m_bfl.pv + CbBuffer() - tag array size (off itagMicFree)
    if ( pbPageDataEnd <= ( pbPageDataStart + 1 /* generous 1 byte in data section, tighter check next */ ) )
    {
        (*pcprintf)( "page corruption (%d): itagMicFree / tag array too large, overlapping PGHDR (%p,%p,%I64d / %d).\r\n", m_pgno, pbPageDataEnd, pbPageDataStart, ctags, CbTagArray_() );
        PageAssertTrack( *this, fFalse, "TagArrayWalkingOntoPghdr" );
#ifdef DEBUG
        Error( ErrCaptureCorruptedPageInfo( mode, L"TagArrayWalkingOntoPghdr" ) );
#endif
    }

    if ( ctags > 0 )
    {
        const __int64 cbMinLine = 4; // Except ext hdr, every node has cbKey + 1 byte of key? + 1 byte of data ... i.e. 4 bytes _min_min_.
        // Note: There is a early phase of page init, and as well some wonky pages created in the CPAGE and Node unit tests that do
        // not have proper external headers > cbMinLine, so we subtract one from ctags to remove ext hdr from consideration.
        if ( __int64( __int64( pbPageDataEnd - pbPageDataStart ) - ( ( ctags - 1 ) * cbMinLine ) ) < 0 )
        {
            (*pcprintf)( "page corruption (%d): itagMicFree / tag array too large for real data left over (%I64d / %d,%p,%p).\r\n", m_pgno, ctags, CbTagArray_(), pbPageDataEnd, pbPageDataStart );
            PageAssertTrack( *this, FNegTest( fCorruptingPageLogically ), "TagArrayTooBigToLeaveDataRoom" );
#ifdef DEBUG
            Error( ErrCaptureCorruptedPageInfo( mode, L"TagArrayTooBigToLeaveDataRoom" ) );
#endif
        }
    }

    //  make sure the page flags are coherent

    if ( FEmptyPage() && FPreInitPage() )
    {
        (*pcprintf)( "page corruption (%d): fPageEmpty and fPagePreInit flags must not be set at the same time\r\n", m_pgno );
        Error( ErrCaptureCorruptedPageInfo( mode, L"PageEmptyAndPagePreInitFlagsBothSet" ) );
    }

    if ( FPreInitPage() )
    {
        if ( ( ppghdr->itagMicFree != 0 ) ||
            !FNewRecordFormat() ||
             ( ( FFlags() & ~fPagePreInit & ~fPageNewRecordFormat & ~fPageNewChecksumFormat & ~maskFlushType ) != 0 ) ||
             ( PgnoPrev() != pgnoNull ) ||
             ( PgnoNext() != pgnoNull ) )
        {
            (*pcprintf)( "page corruption (%d): writing a preinit page with non-0 itag, or non-allowed flags. Page should be largely zeros.\r\n", m_pgno );
            Error( ErrCaptureCorruptedPageInfo( mode, L"PreInitPageFlagSetWithInvalidFlags" ) );
        }
    }

    if ( FIndexPage() && FPrimaryPage() )
    {
        (*pcprintf)( "page corruption (%d): corrupt flags (0x%x)\r\n", m_pgno, FFlags() );
        Error( ErrCaptureCorruptedPageInfo( mode, L"CorruptFlagsMixedIndexPrimary" ) );
    }

    if ( FLongValuePage() && FIndexPage() )
    {
        (*pcprintf)( "page corruption (%d): corrupt flags (0x%x)\r\n", m_pgno, FFlags() );
        Error( ErrCaptureCorruptedPageInfo( mode, L"CorruptFlagsMixedIndexLv" ) );
    }

    if ( FRepairedPage() && !g_fRepair )
    {
        (*pcprintf)( "page corruption (%d): repaired page (0x%x)\r\n", m_pgno, FFlags() );
        Error( ErrCaptureCorruptedPageInfo( mode, L"RepairedPage" ) );
    }

    INT cbTotal = 0;
    ULONG dwOverlaps = 0;

    const BOOL fSmallFormat = FSmallPageFormat();

    if ( grbitExtensiveCheck & CheckTagsNonOverlapping )
    {
        ULONG cdw = ( CbBufferData() + 31 ) / 32;
        const BOOL fCleanUpStateSavedSavedSaved = FOSSetCleanupState( fFalse );

        rgdw     = new ULONG[ cdw ];
        if ( rgdw )
        {
            memset( rgdw, 0, cdw * sizeof( ULONG ) );
        }

        // Restore cleanup checking
        FOSSetCleanupState( fCleanUpStateSavedSavedSaved );
    }

    const INT cbPrefixMax = ( FSpaceTree() || ppghdr->objidFDP == 0 /* zero'd page */ ) ? 0 : PtagFromItag_( 0 )->Cb( fSmallFormat );

    for ( INT itag  = 0; itag < ppghdr->itagMicFree; ++itag )
    {
        TAG * const ptag = PtagFromItag_( itag );

        USHORT ib = ptag->Ib( fSmallFormat );
        USHORT cb = ptag->Cb( fSmallFormat );

        cbTotal += cb;

        //  only tag-zero is allowed to be zero-length. however, the IB portion of
        //  tag-zero is unfortunately allowed to be invalid i.e. greater than ibMicFree.

        if ( cb == 0 )
        {
            if ( itag == 0 )
            {
                continue;
            }
            (*pcprintf)( "page corruption (%d): Non TAG-0 - TAG %d has zero cb(cb = %d, ib = %d)\r\n", m_pgno, itag, cb, ib );
            Error( ErrCaptureCorruptedPageInfo( mode, L"ZeroLengthIline" ) );
        }

        //  Cb() and Ib() are USHORTs; so only need to look for overflows

        //  ibMicFree may not be correct, but it should only be too large
        //  ibMicFree becomes wrong if you have three nodes ABC and you delete
        //  B (doesn't change ibMicFree) and then you delete C (ibMicFree should
        //  point to the end of A, but points to the end of B)

        if ( (DWORD)ib + (DWORD)cb > (DWORD)ppghdr->ibMicFree )
        {
            (*pcprintf)( "page corruption (%d): TAG %d ends in free space (cb = %d, ib = %d, ibMicFree = %d)\r\n",
                            m_pgno, itag, cb, ib, (USHORT)ppghdr->ibMicFree );
            Error( ErrCaptureCorruptedPageInfo( mode, L"TagInFreeSpace" ) );
        }

        if ( grbitExtensiveCheck )  //  all hidden under this grbit to avoid cost to CheckBasic ...
        {
            //  check that the offset, size, and end of buffer all fit on the page

            const DWORD ibEnd = (DWORD)ib + (DWORD)cb;
            const DWORD cbPageAvail = CbBuffer();   //  start w/ tighter constraint of CbBuffer() ...
            if ( ib >= cbPageAvail || cb >= cbPageAvail || ibEnd >= cbPageAvail )
            {
                (*pcprintf)( "page corruption (%d): TAG %d starts, sizes, or ends off end of page (ib = %d, cb = %d, cbPageAvail = %d)\r\n",
                                m_pgno, itag, ib, cb, cbPageAvail );
                Error( ErrCaptureCorruptedPageInfo( mode, L"TagBeyondEndOfPage" ) );
            }

            const INT iline = itag - ctagReserved;
            LINE line;
            const ERR errGetLine = itag < ctagReserved ?
                                       ErrGetPtrExternalHeader( &line ) :
                                       ErrGetPtr( iline, &line );

            // Note: ErrNDIGetkeydataflags() down lower also does a ErrGetPtr(), but we do it separately, so we can know
            // whether it is the TAG ib / offset or cb that is corrupted or later the flags / prefix.cbKey / suffix.cbKey
            // in the beginning of the actual node data.

            //  ErrGetPtr() has three return modes:
            //     JET_errSuccess & line.pv != NULL - Everything is fine.
            //     err & line.pv != NULL - If the node offset or cb is bad, but the itag / tag array entry is at least on page.
            //     err & line.pv == NULL - ONLY if the itag / tag array entry is off page ... this however should be handled
            //                             by the checks on the itagMicFree above for the header.
            Expected( line.pv != NULL ); // We're asserting just so we know, but we'll handle line.pv == NULL below.
            Expected( line.cb != 0 || FNegTest( fCorruptingPageLogically ) );

            if ( line.pv == NULL )
            {
                AssertTrack( errGetLine < JET_errSuccess, "GotNullLineWithoutErrValue" ); // ErrGetPtr should not return this case.
                (*pcprintf)( "page corruption (%d): TAG %d offset / line could not be retrieve from tag array (ib=%d, cb=%d).\r\n", m_pgno, itag, ib, cb );
                PageAssertTrack( *this, fFalse, "GetLineGotNullPtr" );
#ifdef DEBUG
                Error( ErrCaptureCorruptedPageInfo( mode, L"GetLineGotNullPtr" ) );
#endif
            }

            const ULONG_PTR pbLineLastByte = (ULONG_PTR)line.pv + (ULONG_PTR)cb - (ULONG_PTR)1;

            //  following four if's / analysis assumes this - but don't think pbLineLastByte _can_ be below line.pv as 
            //  the cb is 16-bits and not large enough to wrap negative.
            PageAssertTrack( *this, pbLineLastByte >= (ULONG_PTR)line.pv, "LastByteOverflowedToBeforeLineStart" );

            //  similarly (and noted in GetPtr_()) that at least the first two of these probably can't happen as cb is
            //  too small to wrap negative.
            if ( pbLineLastByte < pbPageDataStart )
            {
                //  The whole line is starting below the data start, i.e. off the data section, and possibly even off page.
                (*pcprintf)( "page corruption (%d): TAG %d computed whole line is too low (ib=%d, cb=%d, %p < %p).\r\n", m_pgno, itag, ib, cb, pbLineLastByte, pbPageDataStart );
                PageAssertTrack( *this, fFalse, "LineEntirelyBelowDataSection" );
#ifdef DEBUG
                Error( ErrCaptureCorruptedPageInfo( mode, L"LineEntirelyBelowDataSection" ) );
#endif
            }
            if ( (ULONG_PTR)line.pv < pbPageDataStart )
            {
                //  The line starts below the data start, i.e. off the data section, but then overlaps into valid data section.
                (*pcprintf)( "page corruption (%d): TAG %d computed start offset starts too low (ib=%d, cb=%d, %p < %p).\r\n", m_pgno, itag, ib, cb, line.pv, pbPageDataStart );
                PageAssertTrack( *this, fFalse, "LineStartsBelowDataSection" );
#ifdef DEBUG
                Error( ErrCaptureCorruptedPageInfo( mode, L"LineStartsBelowDataSection" ) );
#endif
            }
            if ( (ULONG_PTR)line.pv > pbPageDataEnd )
            {
                //  The whole line is starting above the data start, i.e. off the data section, and possibly even off page.
                (*pcprintf)( "page corruption (%d): TAG %d computed offset starts too high (ib=%d, cb=%d, %p > %p).\r\n", m_pgno, itag, ib, cb, line.pv, pbPageDataEnd );

                PageAssertTrack( *this, fFalse, "LineEntirelyAboveDataSection" );
#ifdef DEBUG
                Error( ErrCaptureCorruptedPageInfo( mode, L"LineEntirelyAboveDataSection" ) );
#endif
            }
            if ( pbLineLastByte > pbPageDataEnd )
            {
                //  The line ends above the data start, i.e. off the data section, but does start / overlaping in valid data section.
                (*pcprintf)( "page corruption (%d): TAG %d computed offset starts too high (ib=%d, cb=%d, %p > %p).\r\n", m_pgno, itag, ib, cb, pbLineLastByte, pbPageDataEnd );
                PageAssertTrack( *this, FNegTest( fCorruptingPageLogically ), "LineEndsAboveDataSection" );
#ifdef DEBUG
                Error( ErrCaptureCorruptedPageInfo( mode, L"LineEndsAboveDataSection" ) );
#endif
            }

            if ( errGetLine < JET_errSuccess || !FOnData( line.pv, line.cb ) )
            {
                // catch all
                CHAR szGetLineErr[40];
                OSStrCbFormatA( szGetLineErr, sizeof( szGetLineErr ), "GetLineFailed%d\n", errGetLine );
                (*pcprintf)( "UNCAUGHT: page corruption (%d): TAG %d ErrGetPtr() failed or got line off page (ib=%d, cb=%d, err=%d,f=%d).\r\n", m_pgno, itag, ib, cb, errGetLine, FOnData( line.pv, line.cb ) );
                //  there should not be too many errors coming from ErrGetLine() that we can't embed the err in the corruption type.
                PageAssertTrack( *this, FNegTest( fCorruptingPageLogically ), "GetLineFailed%d", errGetLine );
#ifdef DEBUG
                Error( ErrCaptureCorruptedPageInfo( mode, szGetLineErr ) );
#endif
            }

            //  do some simple KEYDATAFLAGS checks

            if ( ( grbitExtensiveCheck & CheckLineBoundedByTag ) && !FSpaceTree() && itag >= ctagReserved  )
            {
                KEYDATAFLAGS kdf;
                kdf.Nullify();

                const ERR errGetKdf = ErrNDIGetKeydataflags( *this, iline, &kdf );

                const CHAR * szGetKdfSourceFile = ( errGetKdf < JET_errSuccess && PefLastThrow() != NULL ) ? PefLastThrow()->SzFile() : NULL;
                const ULONG ulGetKdfSourceLine = ( errGetKdf < JET_errSuccess && PefLastThrow() != NULL ) ? PefLastThrow()->UlLine() : 0;
                Expected( errGetKdf >= JET_errSuccess || ( szGetKdfSourceFile != NULL && ulGetKdfSourceLine > 0 ) );
                Expected( szGetKdfSourceFile == NULL || 0 == _stricmp( SzSourceFileName( szGetKdfSourceFile ), "node.cxx" ) );

                //  if the compressed key size of this node is larger than the prefix node is big
                //  (according to the tag array size), then we have a problem ...

                if ( FNDCompressed( kdf ) && ( kdf.key.prefix.Cb() > cbPrefixMax || kdf.key.prefix.Cb() < 0 ) )
                {
                    (*pcprintf)( "page corruption (%d): TAG %d prefix usage is larger than prefix node (prefix.Cb() = %d, cbPrefixMax = %d)\r\n",
                                    m_pgno, itag, kdf.key.prefix.Cb(), cbPrefixMax );
                    Error( ErrCaptureCorruptedPageInfo( mode, L"TagcbPrefixTooLarge" ) );
                }
                //  if the key portion of this node is larger than the node (according to the tag 
                //  array size), then we have a problem ...

#ifdef DEBUG
                //  Not used anywhere.
                if ( ErrFaultInjection( 60348 ) )
                {
                    ErrCaptureCorruptedPageInfo( CPAGE::OnErrorReturnError, L"TestFakeCheckPageCorruption" );
                    err = JET_errSuccess;   //  carry on like nothing happened ...
                }
#endif
                if ( kdf.key.suffix.Cb() > cb || kdf.key.suffix.Cb() < 0 )
                {
                    (*pcprintf)( "page corruption (%d): TAG %d suffix size is larger than the actual tag size (suffix.Cb() = %d, cb = %d)\r\n",
                                    m_pgno, itag, kdf.key.suffix.Cb(), cb );
                    Error( ErrCaptureCorruptedPageInfo( mode, L"TagcbSuffixTooLarge" ) );
                }

                //  if the data portion of this node is larger than the node (according to the tag 
                //  array size), then we have a problem ...

                //  might not even be possible
                if ( kdf.data.Cb() > cb || kdf.data.Cb() < 0 )
                {
                    (*pcprintf)( "page corruption (%d): TAG %d data size is larger than actual tag size (data.Cb() = %d, cb = %d)\r\n",
                                    m_pgno, itag, kdf.data.Cb(), cb );
                    Error( ErrCaptureCorruptedPageInfo( mode, L"TagcbDataTooLarge" ) );
                }

                //  This is trying to pre-cover the node.cxx g_fNodeMiscMemoryTrashedDefenseInDepthTemp PageEnforce() but
                //  to be honest, not 100% sure what constraints that check is on ... it might be internal only pages, or
                //  some other subset of pages.  So it is possible that this check needs to be qualified with some page
                //  flags or something.
                if ( kdf.data.Cb() == 0 )
                {
                    (*pcprintf)( "page corruption (%d): TAG %d data size is zero ... we don't have non-data nodes ... yet at least. (data.Cb() = %d, cb = %d)\r\n",
                                    m_pgno, itag, kdf.data.Cb(), cb );

                    PageAssertTrack( *this, FNegTest( fCorruptingPageLogically ), "TagCbDataZero" );
#ifdef DEBUG
                    Error( ErrCaptureCorruptedPageInfo( mode, L"TagCbDataZero" ) );
#endif
                }

                //  if the overhead + key(suffix) + data is larger than the node (according to the tag 
                //  array size), then we have a problem ...

                //  might not even be possible
                const INT cbInternalNode = ( ( FNDCompressed( kdf ) ? 2 : 0 ) /* prefix used cb */ +
                                                sizeof(USHORT) /* suffix cb */ +
                                                kdf.key.suffix.Cb() +
                                                kdf.data.Cb() );
                if ( cbInternalNode > cb || cbInternalNode < 0 )
                {
                    (*pcprintf)( "page corruption (%d): TAG %d aggregate key suffix/data size is larger than actual tag size (cbInternalNode=( 2 + %d + %d + %d ), cb = %d)\r\n",
                                    m_pgno, itag, ( FNDCompressed( kdf ) ? 2 : 0 ), kdf.key.suffix.Cb(), kdf.data.Cb(), cb );
                    Error( ErrCaptureCorruptedPageInfo( mode, L"TagDataTooLarge" ) );
                }

                //  We check the error later because the above checks all things (prefix size, suffix size, etc) that probably
                //  cause the wonky err from ND.

                if ( errGetKdf < JET_errSuccess )
                {
                    CHAR szGetKdfErr [40];
                    //  there should not be too many errors coming from ErrNDIGetKeydataflags() that we can't embed the err in the corruption type.
                    OSStrCbFormatA( szGetKdfErr, sizeof( szGetKdfErr ), "NdiGetKdfFailed%d\n", errGetKdf );
                    (*pcprintf)( "page corruption (%d): TAG %d failed to load NDIGetKeydataFlags with %d\r\n", errGetKdf );
                    PageAssertTrack( *this, FNegTest( fCorruptingPageLogically ), "NdiGetKdfFailed%d\n", errGetKdf );
#ifdef DEBUG
                    Error( ErrCaptureCorruptedPageInfo_( mode, szGetKdfErr, NULL, szGetKdfSourceFile, ulGetKdfSourceLine, fTrue ) );
#endif
                }

                if ( grbitExtensiveCheck & CheckLinesInOrder )
                {
                    if ( itag > ctagReserved &&
                         ( FLeafPage() || itag < ppghdr->itagMicFree - 1 ) && // last key in non-leaf pages is empty
                         CmpKey( kdf.key, keyLast ) < 0 )
                    {
                        (*pcprintf)( "page corruption (%d): TAG %d out of order on the page compared to the previous tag\r\n", m_pgno, itag );
                        Error( ErrCaptureCorruptedPageInfo( mode, L"LinesOutOfOrder" ) );
                    }

                    keyLast = kdf.key;
                }
            }

            //  short circuit if necessary

            if ( rgdw )
            {
                //  we set bits in an array to keep track of which bytes are in use. if a byte is used twice
                //  then we will OR in a non-zero value into dwOverlaps

                //  this loop handles cases where we are setting bits all the way to the end of
                //  current byte. it iterates until we're no longer writing to the end of the byte


                while ( cb >= 32 - ( ib & 31 ) )
                {
                    USHORT iBit          = ib;
                    ULONG  bit           = (BYTE)( 1 << ( iBit & 31 ) );
                    DWORD *pdw           = rgdw + iBit / 32;
                    ULONG  set           = bit;
                    USHORT cbitRightSkip = iBit & 31;

                    set -= 1;
                    set = ~set;
                    dwOverlaps |= *pdw & set;
                    *pdw       |= set;

                    ib += sizeof( BYTE ) * (32 - cbitRightSkip );
                    cb -= sizeof( BYTE ) * (32 - cbitRightSkip );
                }

                //  next we start at an arbitrary bit and stop at an arbitrary bit
                if ( cb )
                {
                    USHORT iBit          = ib;
                    ULONG  bit           = (BYTE)( 1 << ( iBit & 31 ) );
                    DWORD *pdw           = rgdw + iBit / 32;
                    ULONG  set           = bit;
                    USHORT cbitRightSkip = iBit & 31;

                    set -= 1;
                    set = ~set;

                    // zero out the bit positions that go past what we want
                    USHORT cbitLeftSkip = 32 - cb - cbitRightSkip;
                    set <<= cbitLeftSkip;
                    set >>= cbitLeftSkip;

                    dwOverlaps |= *pdw & set;
                    *pdw       |= set;
                }
            }   //  if ( rgdw )
        }   //  if ( grbitExtensiveCheck )
    }

    if ( dwOverlaps != 0 )
    {
        (*pcprintf)( "page corruption (%d): TAG overlap \r\n",
                    m_pgno );
        Error( ErrCaptureCorruptedPageInfo( mode, L"TagsOverlap" ) );
    }

    //  all space on the page should be accounted for

    const INT cbAccountedFor = cbTotal + ppghdr->cbFree + CbTagArray_();
    if ( cbAccountedFor != CbBufferData() )
    {
        (*pcprintf)( "page corruption (%d): space mismatch (%d bytes accounted for, %d bytes expected)\r\n",
                        m_pgno, cbAccountedFor, CbBufferData() );
        Error( ErrCaptureCorruptedPageInfo( mode, L"SpaceSizeMismatch" ) );
    }

    err = JET_errSuccess;

HandleError:
    delete [] rgdw;

    return err;
}

//  ================================================================
VOID CPAGE::PreparePageForWrite( 
    __in const CPAGE::PageFlushType pgft,
    __in const BOOL fSkipSetFlushType,  // default = fFalse
    __in const BOOL fSkipFMPCheck )     // default = fFalse
//  ================================================================
{
    Assert( FIsNormalSized() );

    //  set the flush type
    if ( !fSkipSetFlushType )
    {
        SetFlushType_( pgft );
    }

#ifndef ENABLE_JET_UNIT_TEST
    Expected( fSkipFMPCheck || ifmpNil != m_ifmp || FNegTest( fInvalidUsage ) /* corruption test hook uses this */ );
#endif // ENABLE_JET_UNIT_TEST
    Assert( CbPage() == m_platchManager->CbBuffer( m_bfl ) );

    //  compute and update page checksum

    SetPageChecksum( (BYTE *)m_bfl.pv, CbPage(), databasePage, m_pgno );

    //  the following enforce will prevent us from writing out a page to disk that
    //  doesn't obey basic page invariants; in particular zero'ed and overlapping tags.

    (VOID)ErrCheckPage( CPRINTFDBGOUT::PcprintfInstance(), OnErrorEnforce );

}

//------------------------------------------------------------------
//  PRIVATE MEMBER FUNCTIONS
//------------------------------------------------------------------

//  ================================================================
VOID CPAGE::Abandon_( )
//  ================================================================
//
//  Abandons ownership of the page that we currently reference. The
//  page is not returned to the buffer manager. Use Release*Latch()
//  to unlatch the page and then abandon it
//
//-
{
    if( FLoadedPage() )
    {
        UnloadPage();
    }
    else
    {
#ifdef DEBUG
        Assert( !FAssertUnused_( ) );   //  don't release twice

        m_ppib = ppibNil;
        m_dbtimePreInit = dbtimeNil;
        m_objidPreInit = objidNil;

        Assert( FAssertUnused_( ) );
#endif
    }
    m_platchManager = &g_nullPageLatchManager;
    m_fSmallFormat = fFormatInvalid;
}



//  ================================================================
VOID CPAGE::Replace_( INT itag, const DATA * rgdata, INT cdata, INT fFlags )
//  ================================================================
//
//  Replace the specified line. Do not try to replace a line with data elsewhere on
//  the page -- a reorganization will destroy the pointers.
//
//-
{
    PGHDR *ppghdr = (PGHDR*)m_bfl.pv;

    Assert( itag >= 0 && itag < ppghdr->itagMicFree );
    Assert( rgdata );
    Assert( cdata > 0 );
    Assert( FAssertWriteLatch( ) );
    Assert( FIsNormalSized() );

    const USHORT    cbTotal = (USHORT)CbDataTotal_( rgdata, cdata );
    Assert( cbTotal >= 0 );

    TAG * const ptag    = PtagFromItag_( itag );
    const BOOL fSmallFormat = FSmallPageFormat();

    //  WARNING: some users of this api pass pointers to existing parts of the page

    const SHORT cbDiff  = (SHORT)((INT)ptag->Cb( fSmallFormat ) - (INT)cbTotal);    //  shrinking if cbDiff > 0

#ifdef DEBUG
    if ( cbDiff < 0 )
    {
        //  if we are growing, then the caller is responsible for not passing pointers
        //  to data already in the page because reorganization will move data on the page
        for ( INT i = 0; i < cdata; i++ )
        {
            Assert( FAssertNotOnPage_ ( rgdata[ i ].Pv() ) );
        }
    }
#endif // DEBUG

    //  we should have enough space
    Assert( cbDiff > 0 || -cbDiff <= ppghdr->cbFree );

    if (    cbDiff >= 0 ||
            (   ptag->Ib( fSmallFormat ) + ptag->Cb( fSmallFormat ) == ppghdr->ibMicFree &&
                CbContiguousFree_( ) >= -cbDiff ) )
    {

        //  we are either shrinking or we are the last node on the page and there is enough space at the end
        //  of the page for us to grow. the node stays where it is
        if ( ptag->Ib( fSmallFormat ) + ptag->Cb( fSmallFormat ) == ppghdr->ibMicFree )
        {
            ppghdr->ibMicFree = USHORT( ppghdr->ibMicFree - cbDiff );
        }
        ppghdr->cbFree = USHORT( ppghdr->cbFree + cbDiff );
    }
    else
    {
        //  GROWING. we will be moving the line

        if ( FRuntimeScrubbingEnabled_() )
        {
            //  scrub the old data
            memset( PbFromIb_( ptag->Ib( fSmallFormat ) ), chPAGEReplaceFill, ptag->Cb( fSmallFormat ) );
        }

        //  delete the current line in preparation
        USHORT cbFree = (USHORT)( ppghdr->cbFree + ptag->Cb( fSmallFormat ) );
        ppghdr->cbFree = cbFree;
        ptag->SetIb( this, 0 );
        ptag->SetCb( this, 0 );
        ptag->SetFlags( this, 0 );

#ifdef DEBUG
        //  we cannot have a pointer to anything on the page because we may reorganize the page
        if( FExpensiveDebugCodeEnabled( Debug_Page_Shake ) )
        {
            //  force a reorganization
            DebugMoveMemory_();
        }
#endif  //  DEBUG

        FreeSpace_( cbTotal );

        ptag->SetIb( this, ppghdr->ibMicFree );
        ppghdr->ibMicFree = USHORT( ppghdr->ibMicFree + cbTotal );
        ppghdr->cbFree = USHORT( ppghdr->cbFree - cbTotal );
    }

    ptag->SetCb( this, cbTotal );
    ptag->SetFlags( this, (USHORT)fFlags );

    CopyData_ ( ptag, rgdata, cdata );

    if ( cbDiff > 0 && FRuntimeScrubbingEnabled_() )
    {
        //  for shrinking, we need to scrub the end of the record after
        //  the copydata because an overlapping shrink may be happening.

        //  scrub cbDiff bytes after the end of the record
        memset( PbFromIb_( ptag->Ib( fSmallFormat ) + ptag->Cb( fSmallFormat ) ), chPAGEReplaceFill, cbDiff );
    }

    DebugCheckAll();
}


//  ================================================================
VOID CPAGE::ReplaceFlags_( INT itag, INT fFlags )
//  ================================================================
{
    Assert( itag >= 0 && itag < ((PGHDR*)m_bfl.pv)->itagMicFree );
    Assert( FAssertWriteLatch() || FAssertWARLatch() );

    TAG * const ptag = PtagFromItag_( itag );
    ptag->SetFlags( this, (USHORT)fFlags );

#ifdef DEBUG_PAGE
    DebugCheckAll();
#endif
}


//  ================================================================
VOID CPAGE::Insert_( INT itag, const DATA * rgdata, INT cdata, INT fFlags )
//  ================================================================
//
//  Insert a new line into the page, reorganizing if necessary. If we are
//  inserting at a location where a line exists it will be moved up.
//
//-
{
    PGHDR *ppghdr = (PGHDR*)m_bfl.pv;

    Assert( itag >= 0 && itag <= ppghdr->itagMicFree );
    Assert( rgdata );
    Assert( cdata > 0 );
    Assert( FAssertWriteLatch( ) );
    Assert( FIsNormalSized() );

    const BOOL fSmallFormat = FSmallPageFormat();

#ifdef DEBUG
    if ( ppghdr->itagMicFree > 1 )
    {
        //  check the last itag (often the last record) is consistent with ibMicFree

        TAG * const ptagLast = PtagFromItag_( ppghdr->itagMicFree - 1);
        USHORT ibAfterLastTag = ptagLast->Ib( fSmallFormat ) + ptagLast->Cb( fSmallFormat );
        Assert( ibAfterLastTag <= ppghdr->ibMicFree );
    }
#endif

    const USHORT cbTotal = (USHORT)CbDataTotal_( rgdata, cdata );
    Assert( cbTotal >= 0 );

#ifdef DEBUG_PAGE
    //  we cannot have a pointer to anything on the page because we may reorganize the page
    INT idata = 0;
    for ( ; idata < cdata; ++idata )
    {
        Assert( FAssertNotOnPage_( rgdata[idata].Pv() ) );
    }
#endif  //  DEBUG

    FreeSpace_( cbTotal + sizeof( CPAGE::TAG ) );

    if( itag != ppghdr->itagMicFree )
    {
        //  expand the tag array and make room

        VOID * const pvTagSrc   = PtagFromItag_( ppghdr->itagMicFree-1 );
        VOID * const pvTagDest  = PtagFromItag_( ppghdr->itagMicFree );
        const LONG  cTagsToMove = ppghdr->itagMicFree - itag;

        //  tags grow from the end of the page (i.e. from high memory to low) so the destination
        //  will be less than the source

        Assert( pvTagDest < pvTagSrc );
        Assert( sizeof( CPAGE::TAG ) == ( (LONG_PTR)pvTagSrc - (LONG_PTR)pvTagDest ) );
        Assert( cTagsToMove > 0 );

        //

        C_ASSERT( sizeof( CPAGE::TAG ) == sizeof( DWORD ) );

        DWORD *             pdw         = (DWORD *)pvTagDest;
        const DWORD * const pdwLast     = pdw + cTagsToMove;

        switch( cTagsToMove % 8 )
        {
            do
            {
                case 0:
                    pdw[0] = pdw[1];
                    pdw++;
                case 7:
                    pdw[0] = pdw[1];
                    pdw++;
                case 6:
                    pdw[0] = pdw[1];
                    pdw++;
                case 5:
                    pdw[0] = pdw[1];
                    pdw++;
                case 4:
                    pdw[0] = pdw[1];
                    pdw++;
                case 3:
                    pdw[0] = pdw[1];
                    pdw++;
                case 2:
                    pdw[0] = pdw[1];
                    pdw++;
                case 1:
                    pdw[0] = pdw[1];
                    pdw++;
                    Assert( pdw <= pdwLast );
            } while( pdw < pdwLast );
        }

        //  if the move was done correctly the contents of the tag at itag should
        //  have been moved to itag+1, leaving a (temporarily) duplicate tag

        Assert( PtagFromItag_( itag )->Ib( fSmallFormat ) == PtagFromItag_( itag + 1 )->Ib( fSmallFormat ) );
    }

    ppghdr->itagMicFree = USHORT( ppghdr->itagMicFree + 1 );

    TAG * const ptag = PtagFromItag_( itag );
    ptag->SetCb( this, 0 );
    ptag->SetIb( this, 0 );
    ptag->SetFlags( this, 0 );

    ptag->SetIb( this, ppghdr->ibMicFree );
    ppghdr->ibMicFree = USHORT( ppghdr->ibMicFree + cbTotal );
    ptag->SetCb( this, cbTotal );
    const USHORT cbFree =   (USHORT)(ppghdr->cbFree - ( cbTotal + sizeof( CPAGE::TAG ) ) );
    ppghdr->cbFree = cbFree;
    ptag->SetFlags( this, (USHORT)fFlags );

    CopyData_ ( ptag, rgdata, cdata );

#ifdef DEBUG_PAGE
    DebugCheckAll();
#endif
}


//  ================================================================
VOID CPAGE::Delete_( INT itag )
//  ================================================================
//
//  Delete a line, freeing its space on the page
//
//-
{
    PGHDR *ppghdr = (PGHDR*)m_bfl.pv;

    Assert( itag >= ctagReserved && itag < ppghdr->itagMicFree );   // never delete the external header
    Assert( FAssertWriteLatch( ) );
    Assert( FIsNormalSized() );

    const TAG * const ptag = PtagFromItag_( itag );
    const BOOL fSmallFormat = FSmallPageFormat();

    if ( FRuntimeScrubbingEnabled_() )
    {
        BYTE * pb = PbFromIb_(ptag->Ib( fSmallFormat ));
        memset(pb, chPAGEDeleteFill, ptag->Cb( fSmallFormat ));
    }

    //  reclaim the free space if it is at the end
    if ( (ptag->Cb( fSmallFormat ) + ptag->Ib( fSmallFormat )) == ppghdr->ibMicFree )
    {
        const USHORT ibMicFree = (USHORT)( ppghdr->ibMicFree - ptag->Cb( fSmallFormat ) );
        ppghdr->ibMicFree = ibMicFree;
    }

    //  do this before we trash the tag

    const USHORT cbFree =   (USHORT)( ppghdr->cbFree + ptag->Cb( fSmallFormat ) + sizeof( CPAGE::TAG ) );
    ppghdr->cbFree = cbFree;

    ppghdr->itagMicFree = USHORT( ppghdr->itagMicFree - 1 );
    copy_backward(
        PtagFromItag_( ppghdr->itagMicFree ),
        PtagFromItag_( itag ),
        PtagFromItag_( itag-1 ) );

    //  update ibMicFree if page is now empty
    if ( ppghdr->cbFree + sizeof( CPAGE::TAG ) + CbPageHeader() == m_platchManager->CbBuffer( m_bfl ) )
    {
        AssertRTL( PtagFromItag_( 0 )->Cb( fSmallFormat ) == 0 );
        AssertRTL( ppghdr->itagMicFree == 1 );
        ppghdr->ibMicFree = 0;
    }

    DebugCheckAll();
}


//  ================================================================
INLINE VOID CPAGE::ReorganizeData_( __in_range( reorgOther, reorgMax - 1 ) const CPAGEREORG reorgReason )
//  ================================================================
//
//  Compact the data on the page to be contigous on the lower end of
//  the page. Sort the tags ( actually an array of pointers to TAGS -
//  we cannot reorder the tags on the page) by ib and move the lines
//  down, from first to last.
//
//  OPTIMIZATION:  searching the sorted array of tags for a gap of the right size
//  OPTIMIZATION:  sort an array of itags (2-bytes), not TAG* (4 bytes)
//  OPTIMIZATION:  try to fill gaps with a tag of the appropriate size from the end
//
//-
{
    PGHDR * const ppghdr = (PGHDR*)m_bfl.pv;

    DEBUG_SNAP_PAGE();

    Assert( !FPreInitPage() );
    Assert( ppghdr->itagMicFree > 0 );
    Assert( 0 != ppghdr->cbFree );  // we should have space if we are to reorganize

    const BOOL fSmallFormat = FSmallPageFormat();

    //  create a temporary array for the tags every tag except the external header
    //  must have at least two bytes of data and there must be at least one empty byte

    BYTE *rgbBuf;
    BFAlloc( bfasTemporary, (VOID **)&rgbBuf );
    TAG ** rgptag = (TAG **)rgbBuf;

//  TAG * rgptagBuf[ ( g_cbPageMax / ( cbNDNullKeyData ) ) + 1 ];
//  TAG ** rgptag = rgptagBuf;

    //  find all non-zero length tags and put them in the temporary array
    //  only the external header can be zero-length so we check for that
    //  case separately (for speed)
    INT iptag   = 0;
    INT itag    = 0;
    for ( ; itag < ppghdr->itagMicFree; ++itag )
    {
        TAG * const ptag = PtagFromItag_( itag );
        rgptag[iptag++] = ptag;
    }

    const INT cptag = iptag;
    Assert( iptag <= ppghdr->itagMicFree );

    //  sort the array
    sort( rgptag, rgptag + cptag, PfnCmpPtagIb() );

    //  work through the array, from smallest to largest, moving the tags down
    USHORT ibDest       = 0;
    BYTE * pbDest   = PbFromIb_( ibDest );
    for ( iptag = 0; iptag < cptag; ++iptag )
    {
        TAG * const ptag            = rgptag[iptag];

        if ( ptag->Cb( fSmallFormat ) != 0 )
        {
            const BYTE * const pbSrc = PbFromIb_( ptag->Ib( fSmallFormat ) );

            if ( pbSrc < pbDest )
            {
                // Enforce if we detect overlapping tags
                (void)ErrCaptureCorruptedPageInfo( OnErrorEnforce, L"TagsOverlapOnReorg" );
            }

            memmove( pbDest, pbSrc, ptag->Cb( fSmallFormat ) );
        }
        ptag->SetIb( this, ibDest );

        ibDest = (USHORT)( ibDest + ptag->Cb( fSmallFormat ) );
        pbDest += ptag->Cb( fSmallFormat );
    }

    ppghdr->ibMicFree = ibDest;

    BFFree( rgbBuf );

    if ( FRuntimeScrubbingEnabled_() )
    {
        BYTE * const pbFree = PbFromIb_( ppghdr->ibMicFree );
        memset( pbFree, chPAGEReorgContiguousFreeFill, CbContiguousBufferFree_() );
    }

    if ( !m_platchManager->FLoadedPage() )
    {
        //  ifmp not valid for loaded page, so can only inc counter when using
        //  real CPAGE + latch manager.

        PERFOpt( cCPAGEReorganizeData[reorgReason].Inc( PinstFromIfmp( m_ifmp ) ) );
    }

    DEBUG_SNAP_COMPARE();
    DebugCheckAll( );
}


//  ================================================================
ULONG CPAGE::CbContiguousDehydrateRequired_() const
//  ================================================================
//
//  Determines the size of the page assuming a non-re-org based
//  dehydration is performed.
//
//-
{
    PGHDR * ppghdr = (PGHDR *)m_bfl.pv;
    //  we round to the size of USHORT for the ib/cb of TAG
    return roundup( ( CbPageHeader() + ppghdr->ibMicFree + CbTagArray_() ), sizeof(USHORT) );
}

//  ================================================================
ULONG CPAGE::CbReorganizedDehydrateRequired_() const
//  ================================================================
//
//  Determines the size of the page assuming a re-org based
//  dehydration is performed.
//
//-
{
    //  we round to the size of USHORT for the ib/cb of TAG
    return roundup( ( m_platchManager->CbBuffer( m_bfl ) - CbFree_() ), sizeof(USHORT) );
}


//  ================================================================
BOOL CPAGE::FPageIsDehydratable( __out ULONG * pcbMinimumSize, __in const BOOL fAllowReorg ) const
//  ================================================================
//
//  returns if it is worth it to compress / dehydrate this page, and
//  if so, what is the minimum amount of buffer required.
//
//-
{
    PGHDR * ppghdr = (PGHDR *)m_bfl.pv;

    Assert( FAssertWARLatch() || FAssertRDWLatch() || FAssertWriteLatch() );

    *pcbMinimumSize = CbPage();

    if ( ppghdr->checksum == 0 &&
            ppghdr->dbtimeDirtied == 0 &&
            ppghdr->objidFDP == 0 &&
            ppghdr->cbFree == 0 )
    {
        return fFalse;
    }

    const INT cbCommitGranularity = OSMemoryPageCommitGranularity();

    if ( ( CbContiguousBufferFree_() >= cbCommitGranularity ) &&                    //  compresses to less than 16 KB free
            //( ( CbPageHeader() + ppghdr->ibMicFree + CbTagArray_() ) <= 4096 )    //  compresses to less than 4 KB
            ( ( CbPageHeader() + ppghdr->ibMicFree + CbTagArray_() ) <
                    ( m_platchManager->CbBuffer( m_bfl ) - ( 4 * 1024 ) ) )         //  at least 4 KB of progress against current buffer
            //( ppghdr->itagMicFree < ( ( 4 * 1024 ) / sizeof(TAG) ) ) &&           //  only have to move 4 KB or less tags
            )
    {
        *pcbMinimumSize = CbContiguousDehydrateRequired_();
    }

#ifdef FDTC_0_REORG_DEHYDRATE
    if ( fAllowReorg &&
         ( CbReorganizedDehydrateRequired_() < *pcbMinimumSize )
         )
    {
        *pcbMinimumSize = CbReorganizedDehydrateRequired_();
    }
#endif

    // Note, even though the TAG array must be aligned to 2-byte offset, we do not need to add +1 due to the buffer manager
    // always picking a power of 2 equal or larger in size than the *pcbMinimumSize.

    return ( *pcbMinimumSize < ( m_platchManager->CbBuffer( m_bfl ) - cbCommitGranularity ) );
}

//  ================================================================
VOID CPAGE::OptimizeInternalFragmentation()
//  ================================================================
//
//  This function optimizes / reorganizes the page if doing so would
//  yield an improvement in memory fragmentation (due to hyper-cache
//  being able to dehydrate the buffer / shrink the memory usage). If
//  no improvement could be achieved the function does nothing unless
//  Debug_Page_Shake is on.
//
//-
{
#ifdef FDTC_0_REORG_DEHYDRATE_PLAN_B
    Assert( m_platchManager->FWriteLatch( m_bfl ) );

    const ULONG cbMinCurrSize = CbContiguousDehydrateRequired_();
    const ULONG cbMinReorgSize = CbReorganizedDehydrateRequired_();

    Assert( cbMinReorgSize <= cbMinCurrSize );

    //  if the native buffer size for a reorg page is less than native buffer
    //  size as the page is currently laid out, then we can get some improvement
    //  in Hyper-Cache / dehydration by re-organizing ...

    if( cbMinReorgSize != cbMinCurrSize &&
        CbBFGetBufferSize( cbMinReorgSize ) < CbBFGetBufferSize( cbMinCurrSize ) )
    {
        ReorganizeData_( reorgDehydrateBuffer );
        Assert( CbContiguousFree_( ) == reinterpret_cast<PGHDR *>( m_bfl.pv )->cbFree );
    }
    else
    {
#ifdef DEBUG
        if( FExpensiveDebugCodeEnabled( Debug_Page_Shake ) )
        {
            //  force a reorganization
            DebugMoveMemory_();
        }
#endif  //  DEBUG
    }
#endif
}


//  ================================================================
VOID CPAGE::DehydratePageUnadjusted_( __in const ULONG cbNewSize )
//  ================================================================
//
//  Takes a new size (which should be at least as large as the value
//  returned by FPageIsDehydratable()) and shrinks the page to this
//  smaller size, setting the buffer along the way.
//
//-
{
    PGHDR * ppghdr = (PGHDR *)m_bfl.pv;

    Assert( FAssertWriteLatch() );
    Assert( cbNewSize < m_platchManager->CbBuffer( m_bfl ) || FNegTest( fInvalidUsage ) );

    OnDebug( ULONG cbMinReq );
    Assert( FPageIsDehydratable( &cbMinReq, fFalse ) && cbMinReq <= cbNewSize );

    OnNonRTM( PAGECHECKSUM pgchk = LoggedDataChecksum() );

    const ULONG cbShrinkage = m_platchManager->CbBuffer( m_bfl ) - cbNewSize;

    //  Validate no underflow, and that this is a safe shrink permutation
    //

    Assert( cbShrinkage > 0 || FNegTest( fInvalidUsage ) );
    Enforce( cbShrinkage < CbPage() );
    Enforce( cbShrinkage < 0x10000 );

    Enforce( ppghdr->cbFree >= cbShrinkage );
    Enforce( ppghdr->ibMicFree < ( m_platchManager->CbBuffer( m_bfl ) - cbShrinkage ) );

    Assert( ppghdr->ibMicFree < m_platchManager->CbBuffer( m_bfl ) - CbTagArray_() );


    //  Shrink the page image
    //

    void * pvTarget = (BYTE*)m_bfl.pv + cbNewSize - CbTagArray_();
    AssertRTL( ( (DWORD_PTR)pvTarget % 2 ) == 0 );      // TAG array expected to be 2-byte/SHORT aligned ...

    memmove( pvTarget,
                (BYTE*)m_bfl.pv + m_platchManager->CbBuffer( m_bfl ) - CbTagArray_(),
                CbTagArray_() );

    //  fix up cbFree

    ppghdr->cbFree -= (USHORT)cbShrinkage;

    //  Adjust the actual buffer size

    m_platchManager->SetBufferSize( &m_bfl, cbNewSize );

    //  validate expected new size

    Assert( cbNewSize == m_platchManager->CbBuffer( m_bfl ) );
    Assert( !FIsNormalSized() );

    //  validate page is good before we set the checksum

    Assert( JET_errSuccess == ErrCheckPage( CPRINTFDBGOUT::PcprintfInstance(), OnErrorFireWall, CheckTagsNonOverlapping ) || FEmptyPage() || FPreInitPage() );
    Assert( pgchk == LoggedDataChecksum() );

    //  Set the checksum to maintain the clean page contract

    if ( !m_platchManager->FDirty( m_bfl ) )
    {
        SetPageChecksum( (BYTE *)m_bfl.pv, CbBuffer(), databasePage, m_pgno );
    }

    //  Validate that the page is valid and in-tact with it's previous data.
    //

    Assert( JET_errSuccess == ErrCheckPage( CPRINTFDBGOUT::PcprintfInstance(), OnErrorFireWall, CheckTagsNonOverlapping ) || FEmptyPage() || FPreInitPage() );

    AssertRTL( pgchk == LoggedDataChecksum() );

    ASSERT_VALID(this);
    DebugCheckAll();
}


//  ================================================================
VOID CPAGE::DehydratePage( __in const ULONG cbNewSize, __in const BOOL fAllowReorg )
//  ================================================================
//
//  Takes a new size (which should be at least as large as the value
//  returned by FPageIsDehydratable()) and shrinks the page to this
//  smaller size, reorganizing the page if necessary.
//
//-
{
    //  These formats should never essentially change ... have to stick
    //  them in a random member function instead of globally b/c they are
    //  private.
    C_ASSERT( sizeof(PGHDR) == 40 );
    C_ASSERT( sizeof(PGHDR2) == 80 );

    Assert( FAssertWriteLatch() );
    Assert( cbNewSize < m_platchManager->CbBuffer( m_bfl ) || FNegTest( fInvalidUsage ) );

    OnNonRTM( PAGECHECKSUM pgchk = LoggedDataChecksum() );

    //  Calculate if need to do a re-org to achieve the new size.
    //
    ULONG cbMinUnadjustedReq;
    if ( !FPageIsDehydratable( &cbMinUnadjustedReq, fFalse ) )
    {
        cbMinUnadjustedReq = CbPage();
    }
#ifdef FDTC_0_REORG_DEHYDRATE
    ULONG cbMinReorganizeReq;
    if ( !FPageIsDehydratable( &cbMinReorganizeReq, fTrue ) )
    {
        cbMinReorganizeReq = CbPage();
    }
    Assert( cbMinReorganizeReq <= cbNewSize );
    Assert( cbMinUnadjustedReq <= cbNewSize || cbMinReorganizeReq <= cbNewSize );

    //  Reorganize page if necessary for maximum shrinkage
    //
    if ( cbNewSize < cbMinUnadjustedReq )
    {
        //  Just a unadjusted dehydrated is not enough, must re-org / adjust the page to fit
        //

        if ( !fAllowReorg )
        {
            //  Just in case the code has gotten confused, defend ourselves.
            //  Someone tried to truncate the buffer to lower than an unadjusted dehydrate's size.
            FireWall( "DehydrateTooSmall" );

            return;
        }

        //  Re-organize the page to be compacted into the low offsets (except
        //  for the tags, which will be taken care of below)
        //
        ReorganizeData_( reorgDehydrateBuffer );
    }

    //  nothing should've been changed by re-org
    //
    AssertRTL( pgchk == LoggedDataChecksum() );
#endif

    //  now the page should be dehydratable without adjustment / re-organization ...
    //
    if ( !FPageIsDehydratable( &cbMinUnadjustedReq, fFalse ) )
    {
        //  Rather than try to shrink and lose data, we'll just not dehydrate ...
        //

        if ( !FNegTest( fInvalidUsage ) )
        {
            FireWall( "MustBeDehydrateble" );
        }

        return;
    }

#ifdef FDTC_0_REORG_DEHYDRATE
    //  the min unadjusted required should be equal to re-org required now, OR we
    //  didn't need to re-org to achieve the desired size, so we didn't.
    Assert( cbMinUnadjustedReq <= cbMinReorganizeReq || cbNewSize >= cbMinUnadjustedReq );
#endif
    //  And definitely the unadjusted required should be small enough now!
    Assert( cbMinUnadjustedReq <= cbNewSize );

    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );

    //  Perform regular unadjusted dehydrate
    //
    DehydratePageUnadjusted_( cbNewSize );

    //  validate expected new size

    Assert( cbNewSize == m_platchManager->CbBuffer( m_bfl ) );
    Assert( !FIsNormalSized() );

    //  Validate that the page is valid and in-tact with it's previous data.
    //

    Assert( JET_errSuccess == ErrCheckPage( CPRINTFDBGOUT::PcprintfInstance(), OnErrorFireWall, CheckTagsNonOverlapping ) || FEmptyPage() || FPreInitPage() );

    //  Restore state check setting
    //
    FOSSetCleanupState( fCleanUpStateSaved );

    AssertRTL( pgchk == LoggedDataChecksum() );

    DebugCheckAll();
}

//  ================================================================
VOID CPAGE::RehydratePage()
//  ================================================================
//
//  Takes a page, and restores it to regular DB page size.
//
//-
{
    Assert( FAssertWriteLatch() );

    BYTE* const pvPage = (BYTE*)m_bfl.pv;
    PGHDR* ppghdr = (PGHDR*)pvPage;

    const ULONG cbBufferOrigSize = m_platchManager->CbBuffer( m_bfl );
    Assert( cbBufferOrigSize > 0 ); // CPAGE doesn't know the concept of a zeroed page.
    Enforce( cbBufferOrigSize <= g_cbPageMax );

    const ULONG cbBufferNewSizeTarget = CbPage();
    Enforce( cbBufferNewSizeTarget <= g_cbPageMax );
    Assert( cbBufferNewSizeTarget >= cbBufferOrigSize );
    Expected( cbBufferNewSizeTarget > cbBufferOrigSize );   // otherwise, there's no point in re-hydrating it (i.e., there would be no growth).

    OnNonRTM( PAGECHECKSUM pgchk = LoggedDataChecksum() );

    //  Adjust the actual buffer size
    //

    m_platchManager->SetBufferSize( &m_bfl, cbBufferNewSizeTarget );
    const ULONG cbBufferNewSize = m_platchManager->CbBuffer( m_bfl );
    Enforce( cbBufferNewSize == cbBufferNewSizeTarget );
    Assert( FIsNormalSized() );

    Enforce( cbBufferNewSize > cbBufferOrigSize );
    const ULONG cbGrowth = cbBufferNewSize - cbBufferOrigSize;

    //  Grow the page image (into the expanded buffer)
    //

    const ULONG cbTagArray = CbTagArray_();
    const BYTE* const pvSource = pvPage + cbBufferOrigSize - cbTagArray;
    BYTE* const pvTarget = pvPage + cbBufferNewSize - cbTagArray;
    AssertRTL( ( (ULONG_PTR)pvTarget % 2 ) == 0 );      // TAG array expected to be 2-byte/SHORT aligned ...
    Enforce( pvSource >= pvPage );  // source not ouf-of-bounds
    Enforce( ( pvTarget + cbTagArray ) <= ( pvPage + cbBufferNewSize ) );   // destination not ouf-of-bounds

    memmove( pvTarget, pvSource, cbTagArray );

    Enforce( ( (ULONG)ppghdr->cbFree + cbGrowth ) <= g_cbPageMax );

    ppghdr->cbFree += (USHORT)cbGrowth;

    if ( FRuntimeScrubbingEnabled_() )
    {
        //  Simplified OverwriteUnusedSpace( 'H' );
        BYTE * const pbFree = PbFromIb_( ppghdr->ibMicFree );
        memset( pbFree, chPAGEReHydrateFill, CbContiguousBufferFree_() );
    }

    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );

    //  validate page is good before we set the checksum

    Assert( JET_errSuccess == ErrCheckPage( CPRINTFDBGOUT::PcprintfInstance(), OnErrorFireWall, CheckTagsNonOverlapping ) || FEmptyPage() || FPreInitPage() );
    Assert( pgchk == LoggedDataChecksum() );

    //  Set the checksum to maintain the clean page contract

    if ( !m_platchManager->FDirty( m_bfl ) )
    {
        SetPageChecksum( pvPage, CbPage(), databasePage, m_pgno );
    }

    //  Validate that the page is valid and intact with its previous data.
    //

    Assert( JET_errSuccess == ErrCheckPage( CPRINTFDBGOUT::PcprintfInstance(), OnErrorFireWall, CheckTagsNonOverlapping ) || FEmptyPage() || FPreInitPage() );

    Assert( CbPage() == m_platchManager->CbBuffer( m_bfl ) );

    //  Restore cleanup checking
    //
    FOSSetCleanupState( fCleanUpStateSaved );

    AssertRTL( pgchk == LoggedDataChecksum() );

    ASSERT_VALID(this);
    DebugCheckAll();
}


//  ================================================================
INLINE VOID CPAGE::ZeroOutGaps_( const CHAR chZero )
//  ================================================================
//
//  Code has been cut-and-pasted from ReorganizeData_() above, then
//  modified slightly to zero out gaps instead of filling them in
//
//-
{
    PGHDR * const ppghdr = (PGHDR*)m_bfl.pv;

    Assert( !FPreInitPage() );
    Assert( ppghdr->itagMicFree > 0 );
    Assert( 0 != ppghdr->cbFree );  // we should have space if we are to reorganize

    const BOOL fSmallFormat = FSmallPageFormat();

    //  create a temporary array for the tags every tag except the external header
    //  must have at least two bytes of data and there must be at least one empty byte

    BYTE *rgbBuf;
    BFAlloc( bfasTemporary, (VOID **)&rgbBuf );
    TAG ** rgptag = (TAG **)rgbBuf;

//  TAG * rgptagBuf[ ( g_cbPageMax / ( cbNDNullKeyData ) ) + 1 ];
//  TAG ** rgptag = rgptagBuf;

    //  find all non-zero length tags and put them in the temporary array
    //  note: only the external header can be zero-length
    INT iptag   = 0;
    INT itag    = 0;
    for ( ; itag < ppghdr->itagMicFree; ++itag )
    {
        TAG * const ptag = PtagFromItag_( itag );

        if ( ptag->Cb( fSmallFormat ) > 0 )
        {
            rgptag[ iptag++ ] = ptag;
        }
        else
        {

            Assert( iptag == 0 );
        }
    }

    const INT cptag = iptag;
    Assert( iptag <= ppghdr->itagMicFree );

    //  sort the array
    sort( rgptag, rgptag + cptag, PfnCmpPtagIb() );

    //  fix ibMicFree and scrub the gap from the end of the header to the first node
    if ( cptag == 0 )
    {

        ppghdr->ibMicFree = 0;
    }
    else
    {

        const TAG * const ptagFirst = rgptag[ 0 ];
        BYTE * const pbStartZeroing = PbFromIb_( 0 );
        const SIZE_T cbToZero       = PbFromIb_( ptagFirst->Ib( fSmallFormat ) ) - pbStartZeroing;
        memset( pbStartZeroing, chZero, cbToZero );

        const TAG * const ptagLast = rgptag[ cptag - 1 ];
        ppghdr->ibMicFree = ptagLast->Ib( fSmallFormat ) + ptagLast->Cb( fSmallFormat );
    }

    //  work through the array, from smallest to largest, filling in gaps
    for ( iptag = 0; iptag < cptag - 1; ++iptag )
    {
        const TAG   * const ptag            = rgptag[iptag];
        const TAG   * const ptagNext        = rgptag[iptag+1];
        BYTE        * const pbStartZeroing  = PbFromIb_( ptag->Ib( fSmallFormat ) ) + ptag->Cb( fSmallFormat );
        const LONG_PTR cbToZero             = PbFromIb_( ptagNext->Ib( fSmallFormat ) ) - pbStartZeroing;

        Assert( ptag->Ib( fSmallFormat ) < ptagNext->Ib( fSmallFormat ) );
        Assert( ptag->Ib( fSmallFormat ) + ptag->Cb( fSmallFormat )         <= ppghdr->ibMicFree );
        Assert( ptagNext->Ib( fSmallFormat ) + ptagNext->Cb( fSmallFormat ) <= ppghdr->ibMicFree );

        if ( cbToZero > 0 )
        {
            memset( pbStartZeroing, chZero, cbToZero );
        }
        else if ( cbToZero != 0 )
        {
            //  a negative value implies overlapping tags and that we're in trouble
            //  better to enforce that leave the page in a bad state.
            (void) ErrCaptureCorruptedPageInfo( OnErrorEnforce, L"TagsOverlapOnZero" );
        }
    }

    BFFree( rgbBuf );
}

//  ================================================================
INLINE BOOL CPAGE::FRuntimeScrubbingEnabled_ ( ) const
//  ================================================================
{
#ifdef ENABLE_JET_UNIT_TEST
    if ( m_iRuntimeScrubbingEnabled != -1 )
    {
        return (BOOL)m_iRuntimeScrubbingEnabled;
    }
#endif // ENABLE_JET_UNIT_TEST

    return ( g_rgfmp && ( m_ifmp != ifmpNil ) && BoolParam( PinstFromIfmp( m_ifmp ), JET_paramZeroDatabaseUnusedSpace ) );
}

#ifdef ENABLE_JET_UNIT_TEST
//  ================================================================
VOID CPAGE::SetRuntimeScrubbingEnabled_( const BOOL fEnabled )
//  ================================================================
{
    m_iRuntimeScrubbingEnabled = fEnabled ? 1 : 0;
}
#endif // ENABLE_JET_UNIT_TEST

//  ================================================================
CPAGE::TAG::TAG() : cb_( 0 ), ib_( 0 )
//  ================================================================
{
}

//------------------------------------------------------------------
//  TEST ROUTINE
//------------------------------------------------------------------

//  ================================================================
VOID CPAGE::TAG::ErrTest( __in VOID * const pvBuffer, ULONG cbPageSize )
//  ================================================================
{
    PGHDR* ppgHdr = ( PGHDR* )pvBuffer;
    memset( ppgHdr, 0, sizeof( *ppgHdr ) );

    ppgHdr->itagMicFree = 2;
    ppgHdr->fFlags = 0;

    CPAGE cpage;
    cpage.LoadPage( pvBuffer, cbPageSize );
    ((CPAGE::PGHDR*)pvBuffer)->cbFree = sizeof(TAG)+1;  // more than a tag must be free
    AssertRTL( ppgHdr == cpage.m_bfl.pv );

    TAG& tag0 = *cpage.PtagFromItag_( 0 );
    TAG& tag1 = *cpage.PtagFromItag_( 1 );
    TAG& tag2 = *cpage.PtagFromItag_( 2 );

    tag0.SetIb( &cpage, 0x0 );
    tag0.SetCb( &cpage, 0x0 );
    tag0.SetFlags( &cpage, 0x0 );
    AssertRTL( tag0.Cb( cpage.FSmallPageFormat() ) == 0x0 );
    AssertRTL( tag0.Ib( cpage.FSmallPageFormat() ) == 0x0 );
    AssertRTL( tag0.FFlags( &cpage, cpage.FSmallPageFormat() ) == 0x0 );

    tag1.SetIb( &cpage, 0x0 );
    tag1.SetCb( &cpage, 0x2 );
    tag1.SetFlags( &cpage, 0x0 );
    AssertRTL( tag1.Cb( cpage.FSmallPageFormat() ) == 0x2 );
    AssertRTL( tag1.Ib( cpage.FSmallPageFormat() ) == 0x0 );
    AssertRTL( tag1.FFlags( &cpage, cpage.FSmallPageFormat() ) == 0x0 );

    tag2.SetIb( &cpage, 0x10 );
    tag2.SetCb( &cpage, 0x2 );
    tag2.SetFlags( &cpage, 0x0 );
    AssertRTL( tag2.Cb( cpage.FSmallPageFormat() ) == 0x2 );
    AssertRTL( tag2.Ib( cpage.FSmallPageFormat() ) == 0x10 );
    AssertRTL( tag2.FFlags( &cpage, cpage.FSmallPageFormat() ) == 0x0 );

    AssertRTL( CPAGE::PfnCmpPtagIb( cbPageSize )( &tag1, &tag2 ) );

    tag1.SetIb( &cpage, 0x100 );
    tag1.SetCb( &cpage, 0x100 );
    tag1.SetFlags( &cpage, 0x0007 );
    AssertRTL( tag1.Cb( cpage.FSmallPageFormat() ) == 0x100 );
    AssertRTL( tag1.Ib( cpage.FSmallPageFormat() ) == 0x100 );
    AssertRTL( tag1.FFlags( &cpage, cpage.FSmallPageFormat() ) == 0x007 );

    AssertRTL( !CPAGE::PfnCmpPtagIb( cbPageSize )( &tag1, &tag2 ) );
    AssertRTL( CPAGE::PfnCmpPtagIb( cbPageSize )( &tag2, &tag1 ) );

    tag2.SetIb( &cpage, 0x50 );
    tag2.SetCb( &cpage, 0x2 );
    tag2.SetFlags( &cpage, 0x0003 );
    AssertRTL( tag2.Cb( cpage.FSmallPageFormat() ) == 0x2 );
    AssertRTL( tag2.Ib( cpage.FSmallPageFormat() ) == 0x50 );
    AssertRTL( tag2.FFlags( &cpage, cpage.FSmallPageFormat() ) == 0x0003 );

    AssertRTL( !CPAGE::PfnCmpPtagIb( cbPageSize )( &tag1, &tag2 ) );
    AssertRTL( CPAGE::PfnCmpPtagIb( cbPageSize )( &tag2, &tag1 ) );

    tag2.SetIb( &cpage, 0xFF );
    tag2.SetCb( &cpage, 0xFF );
    tag2.SetFlags( &cpage, 0x0007 );
    AssertRTL( tag2.Cb( cpage.FSmallPageFormat() ) == 0xFF );
    AssertRTL( tag2.Ib( cpage.FSmallPageFormat() ) == 0xFF );
    AssertRTL( tag2.FFlags( &cpage, cpage.FSmallPageFormat() ) == 0x0007 ); // max 3 flags

    tag2.SetIb( &cpage, 0x123 );
    tag2.SetCb( &cpage, 0x456 );
    tag2.SetFlags( &cpage, 0x0002 );
    AssertRTL( tag2.Cb( cpage.FSmallPageFormat() ) == 0x456 );
    AssertRTL( tag2.Ib( cpage.FSmallPageFormat() ) == 0x123 );
    AssertRTL( tag2.FFlags( &cpage, cpage.FSmallPageFormat() ) == 0x0002 );
}


//  ================================================================
ERR CPAGE::ErrTest( __in const ULONG cbPageSize )
//  ================================================================
{
    //  Check the constants
    AssertRTL( 0 == CbPage() % cchDumpAllocRow );

    VOID * const pvBuffer = PvOSMemoryHeapAlloc( cbPageSize );
    if( NULL == pvBuffer )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }
    // tag tests
    TAG::ErrTest( pvBuffer, cbPageSize );
    OSMemoryHeapFree( pvBuffer );
    return JET_errSuccess;
}


//  Page Hint Cache

//  ================================================================
ERR CPAGE::ErrGetPageHintCacheSize( ULONG_PTR* const pcbPageHintCache )
//  ================================================================
{
#ifdef MINIMAL_FUNCTIONALITY
    *pcbPageHintCache = 0;
#else  //  !MINIMAL_FUNCTIONALITY
    *pcbPageHintCache = ( maskHintCache + 1 ) * sizeof( DWORD_PTR );
#endif  //  MINIMAL_FUNCTIONALITY
    return JET_errSuccess;
}

//  ================================================================
ERR CPAGE::ErrSetPageHintCacheSize( const ULONG_PTR cbPageHintCache )
//  ================================================================
{
#ifdef MINIMAL_FUNCTIONALITY
#else  //  !MINIMAL_FUNCTIONALITY

    //  clip the requested size to the valid range

    const SIZE_T cbPageHintCacheMin = 128;
    const SIZE_T cbPageHintCacheMax = cbHintCache;

    SIZE_T  cbPageHintCacheVal  = cbPageHintCache;
            cbPageHintCacheVal  = max( cbPageHintCacheVal, cbPageHintCacheMin );
            cbPageHintCacheVal  = min( cbPageHintCacheVal, cbPageHintCacheMax );

    //  round the validated size up to the next power of two

    SIZE_T cbPageHintCacheSet;
    for (   cbPageHintCacheSet = 1;
            cbPageHintCacheSet < cbPageHintCacheVal;
            cbPageHintCacheSet *= 2 );

    //  set the new utilized size of the page hint cache to the new size if it
    //  has grown by a power of two or shrunk by two powers of two.  we try to
    //  minimize size changes as they invalidate the contents of the cache

    if (    maskHintCache < cbPageHintCacheSet / sizeof( DWORD_PTR ) - 1 ||
            maskHintCache > cbPageHintCacheSet / sizeof( DWORD_PTR ) * 2 - 1 )
    {
        maskHintCache = cbPageHintCacheSet / sizeof( DWORD_PTR ) - 1;
    }

#endif  //  MINIMAL_FUNCTIONALITY

    return JET_errSuccess;
}


//  ================================================================
ERR CPAGE::ErrInit()
//  ================================================================
{
    ERR err = JET_errSuccess;

#ifdef MINIMAL_FUNCTIONALITY
#else  //  !MINIMAL_FUNCTIONALITY

    //  allocate the hint cache.  the hint cache is a direct map cache
    //  used to store BFLatch hints for the page pointer of each node
    //  in each internal page
    //
    //  NOTE:  due to the fact that this is a direct map cache, it is not
    //  possible to guarantee the consistency of an entire BFLatch structure.
    //  as a result, we only store the dwContext portion of the BFLatch.  the
    //  buffer manager recomputes the other parts of the BFLatch on a hit.
    //  this helps the cache as it allows it have a higher hint density
    //
    //  The maximum cache size can change, but we'll assume the amount
    //  of physical memory won't change
    //

    //  By picking a small page size as divisor here, we are allowing our max
    //  page hint cache size to exceed by upto (depending upon engine's real
    //  page size) 8x the usable cache size (for now).
    const ULONG_PTR cbfCacheAbsoluteMax = ULONG_PTR( QWORD( min( OSMemoryPageReserveTotal(), OSMemoryTotal() ) ) / g_cbPageMin );

    const SIZE_T cbPageHintCacheMin = sizeof( DWORD_PTR );
    const SIZE_T cbPageHintCacheMax = cbfCacheAbsoluteMax * sizeof( DWORD_PTR );

    SIZE_T  cbPageHintCache = UlParam( JET_paramPageHintCacheSize );
            cbPageHintCache = max( cbPageHintCache, cbPageHintCacheMin );
            cbPageHintCache = min( cbPageHintCache, cbPageHintCacheMax );

    for ( cbHintCache = 1; cbHintCache < cbPageHintCache; cbHintCache *= 2 );
    maskHintCache = cbPageHintCacheMin / sizeof( DWORD_PTR ) - 1;

    if ( !( rgdwHintCache = (DWORD_PTR*)PvOSMemoryHeapAlloc( cbHintCache ) ) )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }
    memset( rgdwHintCache, 0, cbHintCache );

HandleError:
    if ( err < JET_errSuccess )
    {
        Term();
    }

#endif  //  MINIMAL_FUNCTIONALITY

    return err;
}

//  ================================================================
VOID CPAGE::Term()
//  ================================================================
{
#ifdef MINIMAL_FUNCTIONALITY
#else  //  !MINIMAL_FUNCTIONALITY

    //  free the hint cache

    if ( rgdwHintCache )
    {
        OSMemoryHeapFree( (void*) rgdwHintCache );
        rgdwHintCache = NULL;
    }

#endif  //  MINIMAL_FUNCTIONALITY
}


//------------------------------------------------------------------
//  DEBUG/DEBUGGER_EXTENSION ROUTINES
//------------------------------------------------------------------


//  ================================================================
ERR CPAGE::ErrEnumTags( CPAGE::PFNVISITNODE pfnEval, void * pvCtx ) const
//  ================================================================
{
    ERR err = JET_errSuccess;
    const PGHDR * const ppghdr      = (PGHDR*)m_bfl.pv;

    Assert( Clines()+1 == ppghdr->itagMicFree );

    for ( INT itag = 0; itag < ppghdr->itagMicFree; ++itag )
    {
        const TAG * const ptag = PtagFromItag_( itag );
        if ( itag < ctagReserved )
        {
            CallR( pfnEval( ppghdr, itag, ptag->FFlags( this, FSmallPageFormat() ), NULL, pvCtx ) );
        }
        else
        {
            KEYDATAFLAGS kdf;
            kdf.Nullify();
            NDIGetKeydataflags( *this, itag - ctagReserved, &kdf );
            CallR( pfnEval( ppghdr, itag, ptag->FFlags( this, FSmallPageFormat() ), &kdf, pvCtx ) );
        }
    }

    Assert( err == JET_errSuccess );
    return JET_errSuccess;
}

ERR ErrAccumulatePageStats(
    const CPAGE::PGHDR * const  ppghdr,
    INT                         itag,
    DWORD                       fNodeFlags,
    const KEYDATAFLAGS * const  pkdf,
    void *                      pvCtx
    )
{
    ERR err = JET_errSuccess;
    BTREE_STATS_PAGE_SPACE * pbtsPageSpace = (BTREE_STATS_PAGE_SPACE *) pvCtx;

    Assert( pbtsPageSpace );
    Assert( pbtsPageSpace->phistoDataSizes );   // maybe check the others?

    if ( NULL == pkdf )
    {
        Assert( itag == 0 );
        //  Certain stats we only rack up once per page.
        Call( ErrFromCStatsErr( CStatsFromPv(pbtsPageSpace->phistoFreeBytes)->ErrAddSample( ppghdr->cbFree ) ) );
        Call( ErrFromCStatsErr( CStatsFromPv(pbtsPageSpace->phistoNodeCounts)->ErrAddSample( max( ppghdr->itagMicFree, 1 ) - 1 ) ) );
        err = JET_errSuccess;
        goto HandleError;
    }

    Assert( pkdf->fFlags == fNodeFlags );

    //
    //  regular nodes
    //
    Call( ErrFromCStatsErr( CStatsFromPv(pbtsPageSpace->phistoKeySizes)->ErrAddSample( pkdf->key.Cb() ) ) );
    Call( ErrFromCStatsErr( CStatsFromPv(pbtsPageSpace->phistoDataSizes)->ErrAddSample( pkdf->data.Cb() ) ) );

    if ( fNodeFlags & fNDCompressed )
    {
        Call( ErrFromCStatsErr( CStatsFromPv(pbtsPageSpace->phistoKeyCompression)->ErrAddSample( pkdf->key.prefix.Cb() ) ) );
    }

    if ( fNodeFlags & fNDDeleted )
    {
        Call( ErrFromCStatsErr( CStatsFromPv(pbtsPageSpace->phistoUnreclaimedBytes)->ErrAddSample( CPAGE::cbInsertionOverhead + pkdf->key.Cb() + pkdf->data.Cb() ) ) );
    }

    if ( fNodeFlags & fNDVersion )
    {
        pbtsPageSpace->cVersionedNodes++;
    }

HandleError:
#pragma prefast( push )
#pragma prefast( disable:6262, "These DumpAllocMap_ functions use a lot of stack (32k-65k)" )

    return err;
}


//  ================================================================
VOID CPAGE::DumpAllocMap_( _TCHAR * rgchBuf, CPRINTF * pcprintf ) const
//  ================================================================
//
//  Prints a 'map' of the page, showing how it is used.
//      H -- header
//      E -- external header
//      * -- data
//      T -- tag
//      . -- unused
//
//-
{
    INT     ich;
    INT     ichBase     = 0;
    INT     itag;
    INT     iptag       = 0;
    PGHDR * ppghdr      = (PGHDR*)m_bfl.pv;

    TAG * rgptagBuf[g_cbPageMax/sizeof(TAG)];
    TAG ** rgptag = rgptagBuf;

    //  header
    for ( ich = 0; ich < CbPageHeader(); ++ich )
    {
        rgchBuf[ich+ichBase] = _T( 'H' );
    }
    ichBase = ich;

    //  no need to process the TAG array if the page has 0 TAG.
    //
    if ( ppghdr->itagMicFree > 0 )
    {
        const TAG * const ptag = PtagFromItag_( 0 );
        Assert( ptag->Cb( FSmallPageFormat() ) < min( CbPage(), m_platchManager->CbBuffer( m_bfl ) ) );
        Assert( ptag->Ib( FSmallPageFormat() ) < min( CbPage(), m_platchManager->CbBuffer( m_bfl ) ) );
        for ( ich = ptag->Ib( FSmallPageFormat() ); ich < (ptag->Cb( FSmallPageFormat() ) + ptag->Ib( FSmallPageFormat() )); ++ich )
        {
            rgchBuf[ich+ichBase] = _T( 'E' );
        }

        for ( itag = 1; itag < ppghdr->itagMicFree; ++itag )
        {
            TAG * const ptagT = PtagFromItag_( itag );
            if ( itag >= (sizeof(rgptagBuf)/sizeof(rgptagBuf[0])) )
            {
                AssertSz( false, "More tags (%d) on this page than can fit on a single page!", itag);
                break;
            }
            rgptag[iptag++] = ptagT;
        }
    }

    const INT cptag = iptag;
    Assert( iptag <= ppghdr->itagMicFree );

    //  sort the array
    sort( rgptag, rgptag + cptag, PfnCmpPtagIb() );

    //  nodes
    for ( iptag = 0; iptag < (INT)ppghdr->itagMicFree - 1; ++iptag )
    {
        const TAG * const ptagT = rgptag[iptag];
        Assert( ptagT->Cb( FSmallPageFormat() ) < min( CbPage(), m_platchManager->CbBuffer( m_bfl ) ) );
        Assert( ptagT->Ib( FSmallPageFormat() ) < min( CbPage(), m_platchManager->CbBuffer( m_bfl ) ) );
        ich = ptagT->Ib( FSmallPageFormat() );
        for ( ; ich < ((ptagT->Cb( FSmallPageFormat() )) + (ptagT->Ib( FSmallPageFormat() ))); ++ich )
        {
            rgchBuf[ich+ichBase] = ( iptag % 2 ) ? _T( '%' ) : _T( '#' );
        }
    }

    //  tags
    ichBase = m_platchManager->CbBuffer( m_bfl );
    ichBase -= sizeof( CPAGE::TAG ) * ppghdr->itagMicFree;

    for ( ich = 0; ich < (INT)(sizeof( CPAGE::TAG ) * ppghdr->itagMicFree); ++ich )
    {
        rgchBuf[ich+ichBase] = _T( 'T' );
    }

    // print the map
    for ( INT iRow = 0; iRow < (INT)m_platchManager->CbBuffer( m_bfl )/cchDumpAllocRow; ++iRow )
    {
        _TCHAR rgchLineBuf[cchDumpAllocRow+1+1];
        UtilMemCpy( rgchLineBuf, &(rgchBuf[iRow*cchDumpAllocRow]), cchDumpAllocRow * sizeof( _TCHAR ) );
        rgchLineBuf[cchDumpAllocRow] = _T( '\n' );
        rgchLineBuf[cchDumpAllocRow+1] = 0;
        (*pcprintf)( "%s", rgchLineBuf );
    }
    (*pcprintf)( _T( "\n" ) );
}

//  ================================================================
ERR CPAGE::DumpAllocMap( CPRINTF * pcprintf ) const
//  ================================================================
{
    const INT   cchBuf      = CbPage();
    _TCHAR      rgchBuf[g_cbPageMax];

    for ( INT ich = 0; ich < cchBuf && ich < (sizeof(rgchBuf)/sizeof(rgchBuf[0])); ++ich )
    {
        //  we have to use a loop, not memset, so this will work with unicode
        rgchBuf[ich] = _T( '.' );
    }

#pragma prefast( pop )
    //  NOTE: use a separate function to do the actual dumping in order to
    //  avoid a complaint by prefix that we're allocating too much stack space
    //
    DumpAllocMap_( rgchBuf, pcprintf );

    return JET_errSuccess;
}

//  ================================================================
ERR CPAGE::DumpTags( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const
//  ================================================================
{
    const PGHDR * const ppghdr      = (PGHDR*)m_bfl.pv;

    Assert( Clines()+1 == ppghdr->itagMicFree );

    CMinMaxTotStats rgStats[6];
    BTREE_STATS_PAGE_SPACE btsPageSpace = {
            sizeof(BTREE_STATS_PAGE_SPACE),
            (JET_HISTO*)&rgStats[0],    //  phistoFreeBytes
            (JET_HISTO*)&rgStats[1],    //  phistoNodeCounts
            (JET_HISTO*)&rgStats[2],    //  phistoKeySizes
            (JET_HISTO*)&rgStats[3],    //  phistoDataSizes
            (JET_HISTO*)&rgStats[4],    //  phistoKeyCompression
            (JET_HISTO*)&rgStats[5]     //  phistoUnreclaimedBytes
        };

    if ( ErrEnumTags( ErrAccumulatePageStats, (void*)&btsPageSpace ) < JET_errSuccess )
    {
        (*pcprintf)( _T( "Failed to accumulate page stats!\n" ) );
    }

    for ( INT itag = 0; itag < ppghdr->itagMicFree; ++itag )
    {
        const TAG * const ptag = PtagFromItag_( itag );
        KEYDATAFLAGS    kdf;
        if ( itag >= ctagReserved )
        {
            NDIGetKeydataflags( *this, itag - ctagReserved, &kdf );
        }

        CHAR szTagFlags[7] = "";
        OSStrCbFormatA( szTagFlags, sizeof(szTagFlags),
                        "(%c%c%c)",
                        ptag->FFlags( this, FSmallPageFormat() ) & fNDVersion    ? 'v' : ' ',
                        ptag->FFlags( this, FSmallPageFormat() ) & fNDDeleted    ? 'd' : ' ',
                        ptag->FFlags( this, FSmallPageFormat() ) & fNDCompressed ? 'c' : ' ' );

        if( 0 == dwOffset )
        {
            if ( itag < ctagReserved )
            {
                (*pcprintf)( _T( "TAG %3d: cb:0x%04x,ib:0x%04x                                                  offset:0x%04x-0x%04x flags:0x%04x %s" ),
                     itag,
                     ptag->Cb( FSmallPageFormat() ),
                     ptag->Ib( FSmallPageFormat() ),
                     ptag->Ib( FSmallPageFormat() ) + CbPageHeader(),
                     ptag->Ib( FSmallPageFormat() ) + CbPageHeader()  + ptag->Cb( FSmallPageFormat() ),
                     ptag->FFlags( this, FSmallPageFormat() ),
                     szTagFlags );
            }
            else
            {
                BOOL fHandledSpecialCase = fFalse;
                if ( FLeafPage() && FSpaceTree() )
                {
                    PGNO pgnoLast = 0xFFFFFFFF;
                    CPG cpgExtent = 0xFFFFFFFF;
                    PCWSTR cwszPoolName = NULL;

                    if( ErrSPREPAIRValidateSpaceNode( &kdf, &pgnoLast, &cpgExtent, &cwszPoolName ) >= JET_errSuccess )
                    {
                        (*pcprintf)(
                                _T( "TAG %3d: cb=0x%04x,ib=0x%04x SP: %ws: %d,%d-%d flags=0x%04x %s" ),
                                itag,
                                ptag->Cb( FSmallPageFormat() ),
                                ptag->Ib( FSmallPageFormat() ),
                                cwszPoolName,
                                cpgExtent,
                                pgnoLast - cpgExtent + 1,
                                pgnoLast,
                                ptag->FFlags( this, FSmallPageFormat() ),
                                szTagFlags );
                        fHandledSpecialCase = fTrue;
                    }
                }

                if ( !fHandledSpecialCase )
                {
                    (*pcprintf)( _T( "TAG %3d: cb:0x%04x,ib:0x%04x prefix:cb=0x%04x suffix:cb=0x%04x data:cb=0x%04x offset:0x%04x-0x%04x flags:0x%04x %s" ),
                         itag,
                         ptag->Cb( FSmallPageFormat() ),
                         ptag->Ib( FSmallPageFormat() ),
                         kdf.key.prefix.Cb(),
                         kdf.key.suffix.Cb(),
                         kdf.data.Cb(),
                         ptag->Ib( FSmallPageFormat() ) + CbPageHeader(),
                         ptag->Ib( FSmallPageFormat() ) + CbPageHeader()  + ptag->Cb( FSmallPageFormat() ),
                         ptag->FFlags( this, FSmallPageFormat() ),
                         szTagFlags );
                }
            }
        }
        else
        {
            const DWORD_PTR     dwAddress   = reinterpret_cast<DWORD_PTR>( PbFromIb_( 0 ) ) +  ptag->Ib( FSmallPageFormat() ) + dwOffset;

            if ( itag < ctagReserved )
            {
                (*pcprintf)(
                        _T( "TAG %3d:  pb=0x%I64x,cb=0x%04x,ib=0x%04x  flags=0x%04x %s" ),
                        itag,
                        __int64( dwAddress ),
                        ptag->Cb( FSmallPageFormat() ),
                        ptag->Ib( FSmallPageFormat() ),
                        ptag->FFlags( this, FSmallPageFormat() ),
                        szTagFlags );
            }
            else
            {
                BOOL fHandledSpecialCase = fFalse;
                if ( FLeafPage() && FSpaceTree() )
                {
                    PGNO pgnoLast       = 0xFFFFFFFF;
                    CPG cpgExtent       = 0xFFFFFFFF;
                    PCWSTR wsczPoolName = NULL;

                    if( ErrSPREPAIRValidateSpaceNode( &kdf, &pgnoLast, &cpgExtent, &wsczPoolName ) >= JET_errSuccess )
                    {
                        (*pcprintf)(
                                _T( "TAG %3d:  pb=0x%I64x,cb=0x%04x,ib=0x%04x SP: %ws: %d,%d-%d flags=0x%04x %s" ),
                                itag,
                                __int64( dwAddress ),
                                ptag->Cb( FSmallPageFormat() ),
                                ptag->Ib( FSmallPageFormat() ),
                                wsczPoolName,
                                cpgExtent,
                                pgnoLast - cpgExtent + 1,
                                pgnoLast,
                                ptag->FFlags( this, FSmallPageFormat() ),
                                szTagFlags );
                        fHandledSpecialCase = fTrue;
                    }
                }

                if ( !fHandledSpecialCase )
                {
                    //  If nothing could recognize and handle the special case, then print out a regular line.
                    (*pcprintf)(
                            _T( "TAG %3d:  pb=0x%I64x,cb=0x%04x,ib=0x%04x  prefix:cb=0x%04x  suffix:pb=0x%I64x,cb=0x%04x  data:pb=0x%I64x,cb=0x%04x  flags=0x%04x %s" ),
                            itag,
                            __int64( dwAddress ),
                            ptag->Cb( FSmallPageFormat() ),
                            ptag->Ib( FSmallPageFormat() ),
                            kdf.key.prefix.Cb(),
                            __int64( (BYTE *)kdf.key.suffix.Pv() + dwOffset ),
                            kdf.key.suffix.Cb(),
                            __int64( (BYTE *)kdf.data.Pv() + dwOffset ),
                            kdf.data.Cb(),
                            ptag->FFlags( this, FSmallPageFormat() ),
                            szTagFlags );
                }
            }

        }

        if ( itag >= ctagReserved )
        {
            kdf;
            NDIGetKeydataflags( *this, itag - ctagReserved, &kdf );
            if ( FInvisibleSons() )
            {
                (*pcprintf)( "    pgno: %d (0x%x)",
                    (ULONG)*((LittleEndian<ULONG>*)kdf.data.Pv()), (ULONG)*((LittleEndian<ULONG>*)kdf.data.Pv()) );
            }
        }

        (*pcprintf)( "\n" );
    }

    if ( 0 == ppghdr->itagMicFree )
    {
        (*pcprintf)( _T( "[No tags found]\n" ) );
    }
    else
    {
        (*pcprintf)( _T( "\n" ) );

        if ( CStatsFromPv(btsPageSpace.phistoNodeCounts)->C() )
        {
            Assert( CStatsFromPv(btsPageSpace.phistoNodeCounts)->C() == 1 );
            Assert( CStatsFromPv(btsPageSpace.phistoNodeCounts)->Min() == CStatsFromPv(btsPageSpace.phistoNodeCounts)->Ave() );
            Assert( CStatsFromPv(btsPageSpace.phistoNodeCounts)->Max() == CStatsFromPv(btsPageSpace.phistoNodeCounts)->Ave() );
            (*pcprintf)( _T( "Nodes: %I64d\n" ),
                        CStatsFromPv(btsPageSpace.phistoNodeCounts)->Ave(),
                        CStatsFromPv(btsPageSpace.phistoKeyCompression)->C(),
                        CStatsFromPv(btsPageSpace.phistoUnreclaimedBytes)->C() );
            (*pcprintf)( _T( "                      min,    ave,   max, total\n" ) );
            (*pcprintf)( _T( " Logical Key Sizes: %5I64d, %6.1f, %5I64d, %5I64d\n" ),
                        CStatsFromPv(btsPageSpace.phistoKeySizes)->Min(),
                        CStatsFromPv(btsPageSpace.phistoKeySizes)->DblAve(),
                        CStatsFromPv(btsPageSpace.phistoKeySizes)->Max(),
                        CStatsFromPv(btsPageSpace.phistoKeySizes)->Total() );
            if ( CStatsFromPv(btsPageSpace.phistoKeyCompression)->C() )
            {
                (*pcprintf)( _T( "   Key Compression: %5I64d, %6.1f, %5I64d, %5I64d (nodes=%I64d)\n" ),
                            CStatsFromPv(btsPageSpace.phistoKeyCompression)->Min(),
                            CStatsFromPv(btsPageSpace.phistoKeyCompression)->DblAve(),
                            CStatsFromPv(btsPageSpace.phistoKeyCompression)->Max(),
                            CStatsFromPv(btsPageSpace.phistoKeyCompression)->Total(),
                            CStatsFromPv(btsPageSpace.phistoKeyCompression)->C() );
            }
            (*pcprintf)( _T( "   Node Data Sizes: %5I64d, %6.1f, %5I64d, %5I64d\n" ),
                        CStatsFromPv(btsPageSpace.phistoDataSizes)->Min(),
                        CStatsFromPv(btsPageSpace.phistoDataSizes)->DblAve(),
                        CStatsFromPv(btsPageSpace.phistoDataSizes)->Max(),
                        CStatsFromPv(btsPageSpace.phistoDataSizes)->Total() );
            if ( CStatsFromPv(btsPageSpace.phistoUnreclaimedBytes)->C() )
            {
                (*pcprintf)( _T( " Unreclaimed Space: %5I64d, %6.1f, %5I64d, %5I64d (nodes=%I64d)\n" ),
                            CStatsFromPv(btsPageSpace.phistoUnreclaimedBytes)->Min(),
                            CStatsFromPv(btsPageSpace.phistoUnreclaimedBytes)->DblAve(),
                            CStatsFromPv(btsPageSpace.phistoUnreclaimedBytes)->Max(),
                            CStatsFromPv(btsPageSpace.phistoUnreclaimedBytes)->Total(),
                            CStatsFromPv(btsPageSpace.phistoUnreclaimedBytes)->C() );
            }
        }
        else
        {
            (*pcprintf)( _T( " No nodes, except maybe external header.  No stats.\n" ) );
        }

    }

    (*pcprintf)( _T( "\n" ) );

    return 0;
}


//  =====================================================================================
VOID CPAGE::DumpTag( CPRINTF * pcprintf, const INT iline, const DWORD_PTR dwOffset ) const
//  =====================================================================================
{
    const INT           itag            = iline + ctagReserved;
    const TAG * const   ptag            = PtagFromItag_( itag );
    CHAR                szTagFlags[7]   = "";
    KEYDATAFLAGS        kdf;

    NDIGetKeydataflags( *this, iline, &kdf );

    OSStrCbFormatA(
        szTagFlags,
        sizeof(szTagFlags),
        "(%c%c%c)",
        ptag->FFlags( this, FSmallPageFormat() ) & fNDVersion    ? 'v' : ' ',
        ptag->FFlags( this, FSmallPageFormat() ) & fNDDeleted    ? 'd' : ' ',
        ptag->FFlags( this, FSmallPageFormat() ) & fNDCompressed ? 'c' : ' ' );

    (*pcprintf)(
            _T( "TAG %d:  pb=0x%I64x,cb=0x%04x,ib=0x%04x  prefix:cb=0x%04x  suffix:pb=0x%I64x,cb=0x%04x  data:pb=0x%I64x,cb=0x%04x  flags=0x%04x %s\n" ),
            itag,
            __int64( PbFromIb_( 0 ) +  ptag->Ib( FSmallPageFormat() ) + dwOffset ),
            ptag->Cb( FSmallPageFormat() ),
            ptag->Ib( FSmallPageFormat() ),
            kdf.key.prefix.Cb(),
            __int64( (BYTE *)kdf.key.suffix.Pv() + dwOffset ),
            kdf.key.suffix.Cb(),
            __int64( (BYTE *)kdf.data.Pv() + dwOffset ),
            kdf.data.Cb(),
            ptag->FFlags( this, FSmallPageFormat() ),
            szTagFlags );
}

// external header size array for different type of data stored in external header.
extern USHORT g_rgcbExternalHeaderSize[];

//  Get persisted flag out of nodeRootField enum
extern INLINE BYTE BNDIGetPersistedNrfFlag( _In_range_(noderfSpaceHeader, noderfIsamAutoInc) NodeRootField noderf );

// Not worth changing the FORMAT_xxx macros for bit shift warnings.
#pragma warning(disable:4293)
//  ================================================================
ERR CPAGE::DumpHeader( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const
//  ================================================================
//
//  Print the header of the page.
//
//-
{
    DumpPageChecksumInfo( m_bfl.pv, m_platchManager->CbBuffer( m_bfl ), databasePage, m_pgno, pcprintf );
    const __int64 chksumLogPage = LoggedDataChecksum().rgChecksum[0];
    (*pcprintf)( "                       logged data checksum = %16I64x\n", chksumLogPage );
    (*pcprintf)( _T( "\n" ) );

    (*pcprintf)( FORMAT_INT( CPAGE::PGHDR, (PGHDR*)m_bfl.pv, checksum, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CPAGE::PGHDR, (PGHDR*)m_bfl.pv, dbtimeDirtied, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CPAGE::PGHDR, (PGHDR*)m_bfl.pv, pgnoPrev, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CPAGE::PGHDR, (PGHDR*)m_bfl.pv, pgnoNext, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CPAGE::PGHDR, (PGHDR*)m_bfl.pv, objidFDP, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CPAGE::PGHDR, (PGHDR*)m_bfl.pv, cbFree, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CPAGE::PGHDR, (PGHDR*)m_bfl.pv, cbUncommittedFree, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CPAGE::PGHDR, (PGHDR*)m_bfl.pv, ibMicFree, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CPAGE::PGHDR, (PGHDR*)m_bfl.pv, itagMicFree, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CPAGE::PGHDR, (PGHDR*)m_bfl.pv, fFlags, dwOffset ) );

    if ( !FSmallPageFormat() )
    {
        (*pcprintf)( FORMAT_INT( CPAGE::PGHDR2, ( PGHDR2* )m_bfl.pv, rgChecksum[0], dwOffset ) );
        (*pcprintf)( FORMAT_INT( CPAGE::PGHDR2, ( PGHDR2* )m_bfl.pv, rgChecksum[1], dwOffset ) );
        (*pcprintf)( FORMAT_INT( CPAGE::PGHDR2, ( PGHDR2* )m_bfl.pv, rgChecksum[2], dwOffset ) );
        (*pcprintf)( FORMAT_UINT( CPAGE::PGHDR2,  (PGHDR2* )m_bfl.pv, pgno, dwOffset ) );
    }

    if( FLeafPage() )
    {
        (*pcprintf)( _T( "\t\tLeaf page\n" ) );
    }

    if( FParentOfLeaf() )
    {
        (*pcprintf)( _T( "\t\tParent of leaf\n" ) );
    }

    if( FInvisibleSons() )
    {
        (*pcprintf)( _T( "\t\tInternal page\n" ) );
    }

    if( FRootPage() )
    {
        (*pcprintf)( _T( "\t\tRoot page\n" ) );
    }

    BOOL fNewExtHdrFormat = fFalse;
    BYTE fNodeFlag = 0;
    if( FFDPPage() )
    {
        (*pcprintf)( _T( "\t\tFDP page\n" ) );

        const TAG * const ptag = PtagFromItag_( 0 );

        if ( ptag->Cb( FSmallPageFormat() ) > sizeof(SPACE_HEADER) )
        {
            fNewExtHdrFormat = true;
        }

        if ( sizeof(SPACE_HEADER) != ptag->Cb( FSmallPageFormat() ) && !fNewExtHdrFormat
            || ptag->Ib( FSmallPageFormat() ) < 0
            || ptag->Ib( FSmallPageFormat() ) > m_platchManager->CbBuffer( m_bfl ) - CbPageHeader() - sizeof(TAG) )
        {
            (*pcprintf)( _T( "\t\tCorrupted External Header\n" ) );
        }
        else
        {
            BOOL fNeedPrintAutoInc = fFalse;
            QWORD qwAutoInc = 0;
            const SPACE_HEADER* psph = NULL;

            if ( fNewExtHdrFormat )
            {
                BYTE* pb = PbFromIb_( ptag->Ib( FSmallPageFormat() ) );
                fNodeFlag = *pb;
                ++pb;

                (*pcprintf)( _T( "\t\tNew external header format\n" ) );
                if ( fNodeFlag & BNDIGetPersistedNrfFlag( noderfSpaceHeader ) )
                {
                    (*pcprintf)( _T( "\t\t\tSpace header flag presents\n" ) );
                }
                if ( fNodeFlag & BNDIGetPersistedNrfFlag( noderfIsamAutoInc ) )
                {
                    (*pcprintf)( _T( "\t\t\tAutoInc flag presents\n" ) );
                }

                const USHORT usTagSize = ptag->Cb( FSmallPageFormat() );
                // calculate the expected tag size
                USHORT usExpectedTagSize = 1; //for the flag byte
                BYTE fFlagT = 0x1;
                for ( INT i = 0; i < noderfMax; ++i, fFlagT <<= 1 )
                {
                    if ( fFlagT & fNodeFlag )
                    {
                        usExpectedTagSize += g_rgcbExternalHeaderSize[i+1];
                    }
                }
                // check the expected tag size is consistent with the flag
                if ( usExpectedTagSize != usTagSize )
                {
                    (*pcprintf)( _T( "\t\tCorrupted Extended External Header. External header flag %d, Expected external header size %d, actual size %d.\n" ),
                                fNodeFlag,
                                usExpectedTagSize,
                                usTagSize );
                }
                else
                {
                    if ( fNodeFlag & BNDIGetPersistedNrfFlag( noderfSpaceHeader ) )
                    {
                        psph = (SPACE_HEADER*)pb;
                        pb += sizeof(SPACE_HEADER);
                    }
                    else
                    {
                        (*pcprintf)( _T( "Corruption, on a FDP page with no space header!" ) );
                        AssertSz( fFalse, "Corruption, on an FDP page with no space header!" );

                    }
                    if ( fNodeFlag & BNDIGetPersistedNrfFlag( noderfIsamAutoInc ) )
                    {
                        fNeedPrintAutoInc = fTrue;
                        qwAutoInc = *(QWORD*)pb;
                    }
                }
            }
            else
            {
                psph = (SPACE_HEADER *)PbFromIb_( ptag->Ib( FSmallPageFormat() ) );
            }
            if ( psph != NULL )
            {
                if ( psph->FMultipleExtent() )
                {
                    (*pcprintf)(
                        _T( "\t\t\tMultiple Extent Space (ParentFDP: %d, pgnoOE: %d)\n" ),
                        psph->PgnoParent(),
                        psph->PgnoOE() );
                }
                else
                {
                    (*pcprintf)( _T( "\t\t\tSingle Extent Space (ParentFDP: %d, CpgPri: %d, AvailBitmap: 0x%08X)\n" ), psph->PgnoParent(), psph->CpgPrimary(), psph->RgbitAvail() );
                }
            }
            if ( fNeedPrintAutoInc )
            {
                (*pcprintf)( _T( "\t\t\tAuto increment maximum: %d\n" ), qwAutoInc );
            }
        }
    }

    if( FEmptyPage() )
    {
        (*pcprintf)( _T( "\t\tEmpty page\n" ) );
    }

    if( FPreInitPage() )
    {
        (*pcprintf)( _T( "\t\tPre-init page\n" ) );
    }

    if( FSpaceTree() )
    {
        (*pcprintf)( _T( "\t\tSpace tree page" ) );

        if ( FRootPage() )
        {
            const TAG * const ptag = PtagFromItag_( 0 );
            if ( sizeof(SPLIT_BUFFER) != ptag->Cb( FSmallPageFormat() )
                || ptag->Ib( FSmallPageFormat() ) < 0
                || ptag->Ib( FSmallPageFormat() ) > m_platchManager->CbBuffer( m_bfl ) - CbPageHeader() - sizeof(TAG) )
            {
                (*pcprintf)( _T( "\tCorrupted Split Buffer!\n" ) );
            }
            else
            {
                const SPLIT_BUFFER  * const pslitbuf    = (SPLIT_BUFFER *)PbFromIb_( ptag->Ib( FSmallPageFormat() ) );
                if ( 0 == pslitbuf->CpgBuffer1() &&
                        0 == pslitbuf->CpgBuffer2() )

                {
                    (*pcprintf)( _T( " (spbuf: none)\n" ) );
                }
                else
                {
                    (*pcprintf)( _T( " (spbuf:" ) );
                    if ( pslitbuf->CpgBuffer1() )
                    {
                        (*pcprintf)( _T( " buf1: %d-%d (%d)" ),
                                            pslitbuf->PgnoLastBuffer1() - pslitbuf->CpgBuffer1() + 1,
                                            pslitbuf->PgnoLastBuffer1(), pslitbuf->CpgBuffer1() );
                    }
                    if ( pslitbuf->CpgBuffer2() )
                    {
                        (*pcprintf)( _T( " buf2: %d-%d (%d)" ),
                                            pslitbuf->PgnoLastBuffer2() - pslitbuf->CpgBuffer2() + 1,
                                            pslitbuf->PgnoLastBuffer2(), pslitbuf->CpgBuffer2() );
                    }
                    (*pcprintf)( _T( ") \n" ) );
                }
            }
        }
        else
        {
            (*pcprintf)( _T( "\n" ) );
        }

    }

    if( FRepairedPage() )
    {
        (*pcprintf)( _T( "\t\tRepaired page\n" ) );
    }

    if( FPrimaryPage() )
    {
        (*pcprintf)( _T( "\t\tPrimary page\n" ) );
        Assert( !FIndexPage() );
    }

    if( FIndexPage() )
    {
        (*pcprintf)( _T( "\t\tIndex page " ) );

        if ( FNonUniqueKeys() )
        {
            (*pcprintf)( _T( "(non-unique keys)\n" ) );
        }
        else
        {
            (*pcprintf)( _T( "(unique keys)\n" ) );
        }
    }
    else
    {
        Assert( !FNonUniqueKeys() );
    }

    if( FLongValuePage() )
    {
        (*pcprintf)( _T( "\t\tLong Value page\n" ) );
    }

    if( FNewRecordFormat() )
    {
        (*pcprintf)( _T( "\t\tNew record format\n" ) );
    }

    if( FNewChecksumFormat() )
    {
        (*pcprintf)( _T( "\t\tNew checksum format\n" ) );
    }

    if( FScrubbed() )
    {
        (*pcprintf)( _T( "\t\tScrubbed\n" ) );
    }

    (*pcprintf)( _T( "\t\tPageFlushType = %d\n" ), Pgft() );

    (*pcprintf)( _T( "\n" ) );

    return JET_errSuccess;
}

#pragma warning(default:4293)

//  ================================================================
ERR CPAGE::ErrDumpToIrsRaw( _In_ PCWSTR wszReason, _In_ PCWSTR wszDetails ) const
//  ================================================================
//
//  Dump the page contents to irs.raw file.
//
{
    ERR      err;
    INST    *pinst = PinstFromIfmp( m_ifmp );
    __int64  fileTime;
    WCHAR    szDate[32];
    WCHAR    szTime[32];
    size_t   cchRequired;
    CPRINTF * pcprintfPageTrace = NULL;

    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );   //  just for diagnostics.

    //  start tracing (before anything else)
    //
    Call( ErrBeginDatabaseIncReseedTracing( pinst->m_pfsapi, g_rgfmp[ m_ifmp ].WszDatabaseName(), &pcprintfPageTrace ) );

    fileTime = UtilGetCurrentFileTime();
    ErrUtilFormatFileTimeAsTimeWithSeconds(
            fileTime,
            szTime,
            _countof(szTime),
            &cchRequired);
    ErrUtilFormatFileTimeAsDate(
            fileTime,
            szDate,
            _countof(szDate),
            &cchRequired);
    (*pcprintfPageTrace)( "[%d.%d] Begin " __FUNCTION__ "( %ws - %ws ).  Time %ws %ws\n", DwUtilProcessId(), DwUtilThreadId(), wszReason, wszDetails, szTime, szDate );

    (*pcprintfPageTrace)( "Begin dump of pgno %d (0x%X)\n", PgnoThis(), PgnoThis() );
    DumpHeader( pcprintfPageTrace );
    (*pcprintfPageTrace)( "\n" );

    (*pcprintfPageTrace)( "Raw dump (cbPage = %d / cbBuffer = %d) in big-endian quartets of bytes (not little endian DWORDs):\n", CbPage(), CbBuffer() );
    const ULONG cbBuffer = CbBuffer();
    if ( ( cbBuffer % 256 ) == 0 )
    {
        CHAR szBuf[1024];
        for ( ULONG i=0; i < cbBuffer/256; i++ )
        {
            DBUTLSprintHex( szBuf, sizeof(szBuf), reinterpret_cast<BYTE *>( PvBuffer() ) + 256*i, 256, 32, 4, 8, 256*i );
            (*pcprintfPageTrace)( "%s", szBuf );
        }
        (*pcprintfPageTrace)( "\n" );
    }
    else
    {
        (*pcprintfPageTrace)( "Mis-sized buffer of %d bytes, won't allow us to dump properly.\n", cbBuffer );
    }

    fileTime = UtilGetCurrentFileTime();
    ErrUtilFormatFileTimeAsTimeWithSeconds(
            fileTime,
            szTime,
            _countof(szTime),
            &cchRequired);
    ErrUtilFormatFileTimeAsDate(
            fileTime,
            szDate,
            _countof(szDate),
            &cchRequired);
    (*pcprintfPageTrace)( "[%d.%d] End " __FUNCTION__ "().  Time %ws %ws\n", DwUtilProcessId(), DwUtilThreadId(), szTime, szDate );

HandleError:

    EndDatabaseIncReseedTracing( &pcprintfPageTrace );

    // Restore cleanup checking
    FOSSetCleanupState( fCleanUpStateSaved );

    return err;
}


#if defined( DEBUG ) || defined( ENABLE_JET_UNIT_TEST )

//  ================================================================
VOID CPAGE::CorruptHdr( _In_ const ULONG ipgfld, const QWORD qwToAdd )
//  ================================================================
//
//  Corrupts the a specified page header field.
//
//-
{
    PGHDR *ppghdr = (PGHDR*)m_bfl.pv;
    switch( ipgfld )
    {
    case ipgfldCorruptItagMicFree:
        Assert( qwToAdd <= 0xFFFF ); // no point otherwise
        ppghdr->itagMicFree = ppghdr->itagMicFree + (USHORT)qwToAdd;
        break;
    default:
        AssertSz( fFalse, "NYI" );
    }
}
//  ================================================================
VOID CPAGE::CorruptTag( _In_ const ULONG itag, _In_ BOOL fCorruptCb /* vs. tag's Ib */, _In_ const USHORT usToAdd )
//  ================================================================
//
//  Corrupts the properly constructed tag's .ib or .cb data by adding the value specified.
//
//-
{
    Assert( itag < ((PGHDR*)m_bfl.pv)->itagMicFree );

    TAG * const ptag = PtagFromItag_( itag );
    if ( !fCorruptCb )
    {
        // Corrupt the IB
        // Note: That on small pages the flags are stored here!
        ptag->CorruptIb( usToAdd );
    }
    else
    {
        // Corrupt the CB
        ptag->CorruptCb( usToAdd );
    }
}

#endif // DEBUG || ENABLE_JET_UNIT_TEST

#ifdef DEBUG

//  ================================================================
VOID CPAGE::AssertValid() const
//  ================================================================
//
//  Do basic sanity checking on the object. Do not call a public method
//  from this, as it will call ASSERT_VALID( this ) again, causing an infinite
//  loop.
//
//-
{
    PGHDR *ppghdr = (PGHDR*)m_bfl.pv;

    //  can only verify much on non-empty page
    if( 0 != ppghdr->checksum )
    {
        Assert( ppghdr->cbFree <= CbPageData() );
        Assert( ppghdr->cbUncommittedFree <= CbPageFree() );
        Assert( ppghdr->ibMicFree <= CbPageData() );
        Assert( FPreInitPage() || ( (USHORT) ppghdr->itagMicFree >= ctagReserved ) );
        //  we must use a static_cast to do the unsigned/signed conversion
        Assert( (USHORT) ppghdr->itagMicFree <= ( CbPageData() - (USHORT) ppghdr->cbFree ) / static_cast<INT>( sizeof( CPAGE::TAG ) ) ); // external header tag
        Assert( CbContiguousFree_() <= CbPageFree() );
        Assert( CbContiguousBufferFree_() <= ppghdr->cbFree );
        Assert( CbContiguousFree_() >= 0 );
        Assert( FPreInitPage() || ( static_cast<VOID *>( PbFromIb_( ppghdr->ibMicFree ) ) <= static_cast<VOID *>( PtagFromItag_( ppghdr->itagMicFree - 1 ) ) ) );
    }
}


//  ================================================================
VOID CPAGE::DebugCheckAll_( ) const
//  ================================================================
//
//  Extensive checking on the page.
//
//  On regular DEBUG
//      1 in 100 chance of calling ASSERT_VALID( this ) ...
//               NOTE: this is handled in the inlined outer wrapper DebugCheckAll(), later probabilities
//               are assuming this.
//      1 in 10k chance of doing a full tag walk (validating size total) and a single line x N lines overlap walk.
//      1 in  1M chance of doing a full NxN line overlap walk.  [.000001 * O(n^2)]
//  On DEBUG_PAGE
//      We do the ASSERT_VALID( this ) every time.
//      We do the full tag walk (validating size total) every time.
//      1 in 10k chance of doing the full NxN line overlap walk.
//  On DEBUG_PAGE_EXTENSIVE
//      We do the full NxN line overlap walk. [O(n^2)]
//
//-
{

    //  Note: Already at a 1% chance of being here in regular DEBUG (100% chance in DEBUG_PAGE case) ...
    //
    ASSERT_VALID( this );

    PGHDR *ppghdr = (PGHDR*)m_bfl.pv;
    INT cbTotal = 0;

    //  determine if we're going to do a full tag walk and try to account for all space
    //

#ifdef DEBUG_PAGE
#ifdef DEBUG_PAGE_EXTENSIVE         // most debugging
    const BOOL fFullTagWalk = fTrue;
#else                               // less debugging
    const BOOL fFullTagWalk = fTrue;
#endif
#else                               // least debugging
    const BOOL fFullTagWalk = ( ( rand() % 100 ) == 42 );       // 1% chance of continuing ...
#endif
    if ( !fFullTagWalk )
    {
        return;
    }

    //  determine if we're going to do a full NxN walk to ensure there are no overlapping lines
    //

#ifdef DEBUG_PAGE
#ifdef DEBUG_PAGE_EXTENSIVE         // most debugging
    const BOOL fFullNxNWalk = fTrue;
#else                               // less debugging
    const BOOL fFullNxNWalk = ( ( rand() % 10000 ) == 42 );     // 1 in 10k chance of doing full NxN ...
#endif
#else                               // least debugging
    const BOOL fFullNxNWalk = ( ( rand() % 100 ) == 42 );       // 1% of 1% of 1% chance of doing full NxN ...
#endif

    const BOOL fSmallFormat = FSmallPageFormat();
    for ( INT itag = 0; itag < ppghdr->itagMicFree; ++itag )
    {
        const TAG * const ptag = PtagFromItag_( itag );
        Assert( ptag );
        const USHORT cbTag = ptag->Cb( fSmallFormat );
        Assert( cbTag >= 0 );
        if ( 0 == cbTag )
        {
            continue;
        }
        const USHORT ibTag = ptag->Ib( fSmallFormat );
        Assert( ibTag >= 0 );
        Assert( ibTag + cbTag <= ppghdr->ibMicFree );
        cbTotal += cbTag;

        //  determine if we can do a overlap check for just this single line
        //

        BOOL fSingleOverlapWalk = ( ( rand() % ( 1 + ppghdr->itagMicFree ) ) == 0 );
#ifdef DEBUG_PAGE
        if ( !fSingleOverlapWalk )
        {
            //  By doing overlap walk by itagMicFree it amortizes us to do it about "once per page", BUT
            //  we'll do it less as we have more lines on the page, which is sort of opposite of what you
            //  want. SO another clever way to do this is to modulo on cbFree, as that we we do it more
            //  frequently as free space gets smaller, but probably still pretty infrequently.
            fSingleOverlapWalk = ( ( rand() % ( 1 + ppghdr->cbFree ) ) == 0 );
        }
#endif
        if ( !fSingleOverlapWalk && !fFullNxNWalk )
        {
            continue;
        }

        //  check to see that we do not overlap with other tags

        INT itagOther = 0;
        for ( itagOther = 0; itagOther < ppghdr->itagMicFree; ++itagOther )
        {
            if ( cbTag == 0 )
            {
                continue;
            }
            if ( itagOther != itag )
            {
                const TAG * const ptagOther = PtagFromItag_( itagOther );
                Assert( ptagOther != ptag );
                const USHORT cbOtherTag = ptagOther->Cb( fSmallFormat );
                if ( cbOtherTag == 0 )
                {
                    continue;
                }
                const USHORT ibOtherTag = ptagOther->Ib( fSmallFormat );
                Assert( ibOtherTag != ibTag );
                if ( ibOtherTag < ibTag )
                {
                    Assert( ibOtherTag + cbOtherTag <= ibTag );
                }
                else
                {
                    Assert( ibTag + cbTag <= ibOtherTag );
                }
            }
        }
    }
#pragma prefast( push )
#pragma prefast( disable:6262, "These DebugMoveMemory_ functions use a lot of stack (33k)" )

    //  all space on the page should be accounted for
    //
    Assert( cbTotal + CbPageFree() + CbTagArray_() == (size_t)CbPageData() );
}


//  ================================================================
VOID CPAGE::DebugMoveMemory_( )
//  ================================================================
//
//  This forces a reorganization of the page by removing the smallest
//  tag, reorganizing the page and then re-intserting it. Watch out
//  for infinite loops with Replace_, which calls this function.
//
//-
{
    BYTE    rgbBuf[g_cbPageMax];
    PGHDR *ppghdr = (PGHDR*)m_bfl.pv;
    INT     cbTag = 0;
    INT     fFlagsTag;

    //  there may not be enough tags to reorganize
    //  we need one to delete and one to move
    if ( ppghdr->itagMicFree < ctagReserved + 2 )
    {
        return;
    }

    //  the page may be fully compacted
    if ( CbContiguousFree_( ) == ppghdr->cbFree )
    {
        return;
    }

    const BOOL fSmallFormat = FSmallPageFormat();

    //  save the smallest tag with a non-zero size
    //  we only loop to itagMicFree-1 as deleting the last tag is useless
    TAG * ptag  = NULL;
    INT itag    = ctagReserved;
    for ( ; itag < ppghdr->itagMicFree - 1; ++itag )
    {
        ptag    = PtagFromItag_( itag );
        cbTag   = ptag->Cb( fSmallFormat );
        if ( cbTag > 0 )
        {
            break;
        }
    }
    Assert( ptag );

    if ( 0 == cbTag )
    {
        //  nothing to reorganize
        return;
    }
    Assert( itag >= ctagReserved && itag < (ppghdr->itagMicFree - 1) );
    Assert( cbTag > 0 );

    fFlagsTag = ptag->FFlags( this, fSmallFormat );
    AssertPREFIX( cbTag <= sizeof( rgbBuf ) );
    UtilMemCpy( rgbBuf, PbFromIb_( ptag->Ib( fSmallFormat ) ), cbTag );

    //  reorganize the page
    Delete_( itag );
    ReorganizeData_( reorgOther );
#pragma prefast( pop )

    //  reinsert the tag
    DATA data;
    data.SetPv( rgbBuf );
    data.SetCb( cbTag );
    Insert_( itag, &data, 1, fFlagsTag );
}


//  ================================================================
BOOL CPAGE::FAssertDirty( ) const
//  ================================================================
{
    return m_platchManager->FDirty( m_bfl );
}


//  ================================================================
BOOL CPAGE::FAssertReadLatch( ) const
//  ================================================================
{
    return m_platchManager->FReadLatch( m_bfl );
}


//  ================================================================
BOOL CPAGE::FAssertRDWLatch( ) const
//  ================================================================
{
    return m_platchManager->FRDWLatch( m_bfl );
}


//  ================================================================
BOOL CPAGE::FAssertWriteLatch( ) const
//  ================================================================
{
    return m_platchManager->FWriteLatch( m_bfl );
}


//  ================================================================
BOOL CPAGE::FAssertWARLatch( ) const
//  ================================================================
{
    return m_platchManager->FWARLatch( m_bfl );
}


//  ================================================================
BOOL CPAGE::FAssertNotOnPage_( const VOID * pv ) const
//  ================================================================
//
//  Tests to see if the pointer given is on the page or not. Returns
//  fTrue if the pointer is not on the page.
//
//-
{
    const BYTE * const pb = static_cast<const BYTE *>( pv );
    BOOL fGood =
        pb < reinterpret_cast<const BYTE * >( m_bfl.pv )
        || pb >= reinterpret_cast<const BYTE * >( m_bfl.pv ) + m_platchManager->CbBuffer( m_bfl )
        ;
    return fGood;
}

#endif // DEBUG

