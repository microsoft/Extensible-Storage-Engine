// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.



#include "std.hxx"

#include <malloc.h>

#undef g_cbPage
#define g_cbPage g_cbPage_CPAGE_NOT_ALLOWED_TO_USE_THIS_VARIABLE


CLimitedEventSuppressor  g_lesCorruptPages;


#ifdef PERFMON_SUPPORT


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

#endif


class ILatchManager
{
public:
    virtual ~ILatchManager();

    virtual void AssertPageIsDirty( const BFLatch& ) const = 0;

    virtual bool FLoadedPage() const = 0;

    virtual bool FDirty( const BFLatch& ) const = 0;
#ifdef DEBUG
    virtual bool FReadLatch( const BFLatch& ) const = 0;
    virtual bool FRDWLatch( const BFLatch& ) const = 0;
    virtual bool FWriteLatch( const BFLatch& ) const = 0;
    virtual bool FWARLatch( const BFLatch& ) const = 0;
#endif

    virtual BFLatch* PBFLatch( BFLatch * const ) const = 0;

    virtual void SetLgposModify( BFLatch * const, const LGPOS, const LGPOS, const TraceContext& tc ) const = 0;
    virtual void Dirty( BFLatch * const, const BFDirtyFlags, const TraceContext& tc ) const = 0;

    virtual void SetBuffer( BFLatch * const, void * const pvBuffer, const ULONG cbBuffer ) const = 0;
    virtual void AllocateBuffer( BFLatch * const, const ULONG cbBuffer ) const = 0;
    virtual void FreeBuffer( BFLatch * const ) const = 0;
    virtual ULONG CbBuffer( const BFLatch& ) const = 0;
    virtual ULONG CbPage( const BFLatch& ) const = 0;
    virtual void SetBufferSize( BFLatch * const, __in ULONG cbNewSize ) const = 0;
    virtual void MarkAsSuperCold( BFLatch * const ) const = 0;

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

ILatchManager::ILatchManager()
{
}

ILatchManager::~ILatchManager()
{
}

class CNullLatchManager : public ILatchManager
{
public:
    CNullLatchManager() {}
    virtual ~CNullLatchManager() {}

    virtual void AssertPageIsDirty( const BFLatch& ) const { Fail_(); }

    virtual bool FLoadedPage() const
    {
        return false;
    }

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
#endif

    virtual BFLatch* PBFLatch( BFLatch * const ) const
    {
        Fail_();
        return NULL;
    }

    virtual void SetLgposModify( BFLatch * const, const LGPOS, const LGPOS, const TraceContext& tc ) const { Fail_(); }
    virtual void Dirty( BFLatch * const, const BFDirtyFlags, const TraceContext& tc ) const { Fail_(); }

    virtual void SetBuffer( BFLatch * const pbfl, void * const pvBuffer, const ULONG cbBuffer ) const { Fail_(); }
    virtual void AllocateBuffer( BFLatch * const, const ULONG cbBuffer ) const { Fail_(); }
    virtual void FreeBuffer( BFLatch * const ) const { Fail_(); }
    virtual ULONG CbBuffer( const BFLatch& ) const { Fail_(); return 0; }
    virtual ULONG CbPage( const BFLatch& ) const { Fail_(); return 0; }
    virtual void SetBufferSize( BFLatch * const, __in ULONG cbNewSize ) const { Fail_(); }
    virtual void MarkAsSuperCold( BFLatch * const ) const { Fail_(); }

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

class CBFLatchManager : public ILatchManager
{
public:
    CBFLatchManager() {}
    virtual ~CBFLatchManager() {}

    virtual void AssertPageIsDirty( const BFLatch& bfl ) const
    {
        Assert( FBFDirty( &bfl ) != bfdfClean );
    }

    virtual bool FLoadedPage() const
    {
        return false;
    }

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
#endif

    virtual BFLatch* PBFLatch( BFLatch * const pbfl ) const
    {
        return pbfl;
    }

    virtual void SetLgposModify( BFLatch * const pbfl, const LGPOS lgposModify, const LGPOS lgposBegin0, const TraceContext& tc ) const
    {
        BFSetLgposModify( pbfl, lgposModify );
        BFSetLgposBegin0( pbfl, lgposBegin0, tc );
    }
    virtual void Dirty( BFLatch * const pbfl, const BFDirtyFlags bfdf, const TraceContext& tc ) const
    {
        BFDirty( pbfl, bfdf, tc );
    }

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


class CLoadedPageLatchManager : public ILatchManager
{
public:
    CLoadedPageLatchManager() {}
    virtual ~CLoadedPageLatchManager() {}

    virtual void AssertPageIsDirty( const BFLatch& ) const {}

    virtual bool FLoadedPage() const { return true; }

    virtual bool FDirty( const BFLatch& ) const { return true; }

#ifdef DEBUG
    virtual bool FReadLatch( const BFLatch& ) const { return true; }
    virtual bool FRDWLatch( const BFLatch& ) const { return true; }
    virtual bool FWriteLatch( const BFLatch& ) const { return true; }
    virtual bool FWARLatch( const BFLatch& ) const { return true; }
#endif

    virtual BFLatch* PBFLatch( BFLatch * const ) const
    {
        AssertSz( fFalse, "Don't get the latch on a loaded page" );
        return NULL;
    }

    virtual void SetLgposModify( BFLatch * const, const LGPOS, const LGPOS, const TraceContext& tc ) const {}
    virtual void Dirty( BFLatch * const, const BFDirtyFlags, const TraceContext& tc ) const {}

protected:
    void SetBufferSizeInfo( BFLatch * const pbfl, __in ULONG cbNewSize ) const
    {
#ifdef DEBUG
        ULONG cbPageOrig = CbPage( *pbfl );
#endif


        Assert( cbNewSize <= 0xFFFF );

        pbfl->dwContext = ( DWORD_PTR( cbNewSize ) << 16 ) | ( DWORD_PTR( 0x0000FFFF ) & pbfl->dwContext );

        Assert( CbPage( *pbfl ) == cbPageOrig );
        Assert( CbBuffer( *pbfl ) == cbNewSize );
    }
#pragma warning ( disable : 4481 )
    virtual void SetBuffer( BFLatch * const pbfl, void * const pvBuffer, const ULONG cbBuffer ) const sealed
    {

        Assert( cbBuffer <= 0xFFFF );
        Assert( 0 == ( cbBuffer & 0x1 ) );

        pbfl->pv = pvBuffer;


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
    }

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

        return ( pbfl->dwContext & 0x1 );
    }

};
CLoadedPageLatchManager g_loadedPageLatchManager;

class CAllocatedLoadedPageLatchManager : public CLoadedPageLatchManager
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


class CTestLatchManager : public CLoadedPageLatchManager
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

            const ULONG cbGrowth = cbNewSize - CbBuffer( *pbfl );

            Assert( cbGrowth >= OSMemoryPageCommitGranularity() );
            Assert( 0 == ( cbGrowth % OSMemoryPageCommitGranularity() ) );


            while( !FOSMemoryPageCommit( (BYTE*)(pbfl->pv) + CbBuffer( *pbfl ), cbGrowth ) )
            {
            }
        }
        else if ( cbNewSize < CbBuffer( *pbfl ) )
        {

            const ULONG cbShrinkage = CbBuffer( *pbfl ) - cbNewSize;

            Assert( cbShrinkage >= OSMemoryPageCommitGranularity() );
            Assert( 0 == ( cbShrinkage % OSMemoryPageCommitGranularity() ) );


            OSMemoryPageDecommit( (BYTE*)(pbfl->pv) + cbNewSize, cbShrinkage );
        }

        SetBufferSizeInfo( pbfl, cbNewSize );

        Assert( CbPage( *pbfl ) == cbPageOrig );
        Assert( CbBuffer( *pbfl ) == cbNewSize );
    }

};
CTestLatchManager g_testPageLatchManager;


BOOL CmpPtagIbSmall( __in const CPAGE::TAG * const ptag1, __in const CPAGE::TAG * const ptag2 )
{
    return ptag1->Ib( fTrue  ) < ptag2->Ib( fTrue  );
}

BOOL CmpPtagIbLarge( __in const CPAGE::TAG * const ptag1, __in const CPAGE::TAG * const ptag2 )
{
    return ptag1->Ib( fFalse  ) < ptag2->Ib( fFalse  );
}

CPAGE::PfnCmpTAG CPAGE::PfnCmpPtagIb( const BOOL fSmallFormat )
{
    return ( fSmallFormat ? CmpPtagIbSmall : CmpPtagIbLarge );
}

CPAGE::PfnCmpTAG CPAGE::PfnCmpPtagIb( __in ULONG cbPage )
{
    return CPAGE::PfnCmpPtagIb( FIsSmallPage( cbPage ) );
}

CPAGE::PfnCmpTAG CPAGE::PfnCmpPtagIb() const
{
    return CPAGE::PfnCmpPtagIb( m_platchManager->CbPage( m_bfl ) );
}



CPageValidationLogEvent::CPageValidationLogEvent( const IFMP ifmp, const INT logflags, const CategoryId category ) :
    m_ifmp( ifmp ),
    m_logflags( logflags ),
    m_category( category )
{
}

CPageValidationLogEvent::~CPageValidationLogEvent()
{
}

void CPageValidationLogEvent::BadChecksum( const PGNO pgno, const ERR err, const PAGECHECKSUM checksumStoredInHeader, const PAGECHECKSUM checksumComputedOffData )
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

void CPageValidationLogEvent::UninitPage( const PGNO pgno, const ERR err )
{
    if( m_logflags & LOG_UNINIT_PAGE )
    {
        ReportUninitializedPage_( pgno, err );
    }
}

void CPageValidationLogEvent::BadPgno( const PGNO pgnoExpected, const PGNO pgnoStoredInHeader, const ERR err )
{
    if( m_logflags & LOG_BAD_PGNO )
    {
        ReportPageNumberFailed_( pgnoExpected, err, pgnoStoredInHeader );
    }
}

void CPageValidationLogEvent::BitCorrection( const PGNO pgno, const INT ibitCorrupted )
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

void CPageValidationLogEvent::BitCorrectionFailed( const PGNO pgno, const ERR err, const INT ibitCorrupted )
{
    if( m_logflags & LOG_CHECKFAIL )
    {
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

void CPageValidationLogEvent::LostFlush(
    const PGNO pgno,
    const FMPGNO fmpgno,
    const INT pgftExpected,
    const INT pgftActual,
    const ERR err,
    const BOOL fRuntime,
    const BOOL fFailOnRuntimeOnly )
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

VOID CPageValidationLogEvent::ReportBadChecksum_(
    const PGNO pgno,
    const ERR err,
    const PAGECHECKSUM checksumStoredInHeader,
    const PAGECHECKSUM checksumComputedOffData ) const
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

VOID CPageValidationLogEvent::ReportUninitializedPage_( const PGNO pgno, const ERR err ) const
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

VOID CPageValidationLogEvent::ReportPageCorrectionFailed_( const PGNO pgno, const ERR err, const INT ibitCorrupted ) const
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

VOID CPageValidationLogEvent::ReportPageNumberFailed_( const PGNO pgno, const ERR err, const PGNO pgnoStoredInHeader ) const
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

VOID CPageValidationLogEvent::ReportPageCorrection_( const PGNO pgno, const INT ibitCorrupted ) const
{

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

VOID CPageValidationLogEvent::ReportLostFlush_(
    const PGNO pgno,
    const FMPGNO fmpgno,
    const INT pgftExpected,
    const INT pgftActual,
    const ERR err,
    const BOOL fRuntime,
    const BOOL fFailOnRuntimeOnly,
    const WCHAR* const wszContext ) const
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
            UnalignedLittleEndian< USHORT >* pule_us = (UnalignedLittleEndian<USHORT>*)pPage->PbFromIb_( Ib( fSmallFormat ) );
            const USHORT us = USHORT( *pule_us );
            usFlags = ( us & mskTagFlags ) >> shfTagFlags;
        }

        return usFlags;
    }
}

INLINE VOID CPAGE::TAG::SetIb( __in CPAGE * const pPage, __in USHORT ib )
{
    const BOOL fSmallFormat = pPage->FSmallPageFormat();

    if ( fSmallFormat )
    {
        Assert( ( ib & ~mskSmallTagIbCb ) == 0 );
        OnDebug( USHORT usFlags = FFlags( pPage, fSmallFormat) );

        USHORT ibT = ib_;
        ibT &= ~mskSmallTagIbCb;
        ibT |= ib;
        ib_ = ibT;

        Assert( Ib( fSmallFormat ) == ib );
        Assert( FFlags( pPage, fSmallFormat ) == usFlags );
    }
    else
    {
        Assert( ( ib & ~mskLargeTagIbCb ) == 0 );
        USHORT ibT = ib_;
        ibT &= ~mskLargeTagIbCb;
        ibT |= ib;
        ib_ = ibT;

        Assert( Ib( fSmallFormat ) == ib );
    }
}

INLINE VOID CPAGE::TAG::SetCb( __in CPAGE * const pPage, __in USHORT cb )
{
    const BOOL fSmallFormat = pPage->FSmallPageFormat();

    if ( fSmallFormat )
    {
        Assert( ( cb & ~mskSmallTagIbCb ) == 0 );
        OnDebug( USHORT usFlags = FFlags( pPage, fSmallFormat ) );

        USHORT  cbT = cb_;
        cbT &= ~mskSmallTagIbCb;
        cbT |= cb;
        cb_ = cbT;

        Assert( Cb( fSmallFormat ) == cb );
        Assert( FFlags( pPage, fSmallFormat ) == usFlags );
    }
    else
    {
        Assert( ( cb & ~mskLargeTagIbCb ) == 0 );

        USHORT  cbT = cb_;
        cbT &= ~mskLargeTagIbCb;
        cbT |= cb;
        cb_ = cbT;

        Assert( Cb( fSmallFormat ) == cb );
    }
}

INLINE VOID CPAGE::TAG::SetFlags( __in CPAGE * const pPage, __in USHORT fFlags )
{
    Assert( 0 == ( fFlags & ~0x7 ) );

    const BOOL fSmallFormat = pPage->FSmallPageFormat();
    OnDebug( const INT cbOld = Cb( fSmallFormat ) );
    OnDebug( const INT ibOld = Ib( fSmallFormat ) );

    if ( fSmallFormat )
    {

        typedef UnalignedLittleEndian< volatile USHORT >* PULEUS;
        PULEUS puus =  ( PULEUS )&(ib_);

        USHORT ibT = *puus;
        ibT &= ~mskTagFlags;
        ibT |= fFlags << shfTagFlags;
        *puus = ibT;

        Assert( FFlags( pPage, fSmallFormat ) == fFlags );
        Assert( Cb( fSmallFormat ) == cbOld );
        Assert( Ib( fSmallFormat ) == ibOld );
    }
    else
    {
        const bool fTag0 = pPage->PtagFromItag_( 0 ) == this;
        if ( !fTag0 && 2 <= Cb( fSmallFormat ) )
        {

            typedef UnalignedLittleEndian< volatile USHORT >* PULEUS;
            PULEUS puus = ( PULEUS )pPage->PbFromIb_( Ib( fSmallFormat ) );

            USHORT ibT = *puus;
            ibT &= ~mskTagFlags;
            ibT |= fFlags << shfTagFlags;
            *puus = ibT;

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



SIZE_T          CPAGE::cbHintCache              = 0;
SIZE_T          CPAGE::maskHintCache            = 0;
DWORD_PTR*      CPAGE::rgdwHintCache            = NULL;




INLINE ULONG CPAGE::CbPage() const
{
    return m_platchManager->CbPage( m_bfl );
}

INLINE ULONG CPAGE::CbBuffer() const
{
    return m_platchManager->CbBuffer( m_bfl );
}


template< PageNodeBoundsChecking pgnbc >
INLINE VOID CPAGE::GetPtr_( INT itag, LINE * pline, _Out_opt_ ERR * perrNoEnforce ) const
{
    Assert( itag >= 0 );
    Assert( itag < ((PGHDR *)m_bfl.pv)->itagMicFree || FNegTest( fCorruptingPageLogically ) );
    Assert( pline );
    Expected( ( pgnbc == pgnbcChecked ) == ( perrNoEnforce != NULL ) );
    Assert( ( pgnbc == pgnbcChecked ) || ( perrNoEnforce == NULL ) );
    Assert( perrNoEnforce == NULL || *perrNoEnforce == JET_errSuccess );

    const bool fSmallFormat = FSmallPageFormat();
#ifdef DEBUG
    const ULONG cbBufferDebug = m_platchManager->CbBuffer( m_bfl );


    const ULONG_PTR pbPageBufferStartDebug = (ULONG_PTR)m_bfl.pv;
    const ULONG_PTR pbPageBufferEndDebug = pbPageBufferStartDebug + cbBufferDebug;

    const ULONG_PTR pbPageDataStartDebug = pbPageBufferStartDebug + 
                                      ( fSmallFormat ? sizeof( PGHDR ) : sizeof( PGHDR2 ) );
    const ULONG_PTR pbPageDataEndDebug = pbPageBufferEndDebug - CbTagArray_() - 1;

    Assert( pbPageDataStartDebug == PbDataStart_( fSmallFormat ) );
    Assert( pbPageDataEndDebug == PbDataEnd_( cbBufferDebug ) );
#endif

    #pragma warning(suppress:4101)
    bool fFlagsOnPage;


    const TAG * const ptag = PtagFromItag_( itag );


    Assert( ( (ULONG_PTR)ptag + sizeof( TAG ) - 1 ) <= pbPageBufferEndDebug );
    if constexpr( pgnbc == pgnbcChecked )
    {
        fFlagsOnPage = fTrue;

        const ULONG_PTR pbPageBufferStart = (ULONG_PTR)m_bfl.pv;
        const ULONG_PTR pbPageDataStart = pbPageBufferStart + 
                                      ( fSmallFormat ? sizeof( PGHDR ) : sizeof( PGHDR2 ) );
        const ULONG_PTR pbPageBufferEnd = pbPageBufferStart + m_platchManager->CbBuffer( m_bfl );

        if ( (ULONG_PTR)ptag < pbPageDataStart || 
               ( (ULONG_PTR)ptag + sizeof( TAG ) - 1 ) > pbPageBufferEnd )
        {

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


        Assert( fSmallFormat || ptag->FFlags( this, fSmallFormat ) == 0x0 );
#ifndef ENABLE_JET_UNIT_TEST
        Assert( ptag->FFlags( this, fSmallFormat ) == 0x0 );
#else
        Assert( ptag->FFlags( this, fSmallFormat ) == 0x0 || FNegTest( fCorruptingPageLogically ) );
#endif
    }
    else
    {
        const ULONG_PTR pbLine = (ULONG_PTR)PbFromIb_( ptag->Ib( fSmallFormat ) );

        pline->pv = (void*)pbLine;

        Assert( pline->cb <= (ULONG)usMax );

        const ULONG_PTR pbLineLastByte = pbLine + pline->cb - 1;
        PageAssertTrack( *this, pbLine <= pbLineLastByte, "NonEuclideanRamGeometryMaybeImpossible" );

        Assert( FOnData( pline->pv, pline->cb ) || FNegTest( fCorruptingPageLogically ) );

        if constexpr( pgnbc == pgnbcChecked )
        {

            const ULONG_PTR pbPageBufferStart = (ULONG_PTR)m_bfl.pv;

            const ULONG_PTR pbPageDataStart = pbPageBufferStart + 
                                      ( fSmallFormat ? sizeof( PGHDR ) : sizeof( PGHDR2 ) );
            const ULONG_PTR pbPageDataEnd = (ULONG_PTR)m_bfl.pv + m_platchManager->CbBuffer( m_bfl ) - CbTagArray_() - 1;

            if ( pbLineLastByte > pbPageDataEnd ||
                   pbLine < pbPageDataStart ||
                   pbLine >= pbPageDataEnd ||
                   pbLineLastByte < pbPageDataStart )
            {
                PageAssertTrack( *this, pbLine >= pbPageDataStart, "LineBeforeDataStartImpossible" );
                PageAssertTrack( *this, pbLineLastByte >= pbPageDataStart, "LineBeforeDataStartImpossible" );
                if ( pbLine >= pbPageDataEnd && pbLineLastByte <= pbPageDataEnd )
                {
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

                fFlagsOnPage = fSmallFormat ||
                               ULONG_PTR( pbLine + 1 ) < pbPageDataEnd;
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


INLINE INT CPAGE::CbDataTotal_( const DATA * rgdata, INT cdata ) const
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


INLINE USHORT CPAGE::CbAdjustForPage() const
{
    Assert( CbPage() < 0x10000 );
    Assert( m_platchManager->CbBuffer( m_bfl ) < 0x10000 );
    Assert( CbPage() >= m_platchManager->CbBuffer( m_bfl ) );
    return (USHORT)CbPage() - (USHORT)m_platchManager->CbBuffer( m_bfl );
}

INLINE INT CPAGE::CbBufferData() const
{
    return m_platchManager->CbBuffer( m_bfl ) - CbPageHeader();
}

INLINE INT CPAGE::CbPageData() const
{
    return CbBufferData() + CbAdjustForPage();
}

INLINE INT CPAGE::CbContiguousBufferFree_( ) const
{
    INT cb = CbBufferData() - ((PGHDR *)m_bfl.pv)->ibMicFree;
    cb -= CbTagArray_();
    return cb;
}

INLINE INT CPAGE::CbContiguousFree_( ) const
{
    return CbContiguousBufferFree_() + CbAdjustForPage();
}

INLINE USHORT CPAGE::CbFree_ ( ) const
{
    return ((PGHDR*)m_bfl.pv)->cbFree;
}


USHORT CPAGE::CbPageFree ( ) const
{
    AssertRTL( (ULONG)((PGHDR*)m_bfl.pv)->cbFree + (ULONG)CbAdjustForPage() < 0x10000 );
    return ((PGHDR*)m_bfl.pv)->cbFree + CbAdjustForPage();
}


INLINE VOID CPAGE::FreeSpace_( __in const INT cb )
{
    Assert ( cb <= ((PGHDR*)m_bfl.pv)->cbFree && cb > 0 );

    if ( CbContiguousFree_( ) < cb )
    {
        ReorganizeData_( reorgFreeSpaceRequest );
    }

    PageEnforce( (*this), CbContiguousFree_( ) >= cb );
}


INLINE VOID CPAGE::CopyData_( TAG * ptag, const DATA * rgdata, INT cdata )
{
    Assert( ptag && rgdata );
    Assert( cdata > 0 );

    const BOOL fSmallFormat = this->FSmallPageFormat();

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


INLINE CPAGE::TAG * CPAGE::PtagFromItag_( INT itag ) const
{
    Assert( itag >= 0 );
    Assert( itag <= ((PGHDR*)m_bfl.pv)->itagMicFree );

    TAG * ptag = (TAG *)( (BYTE*)m_bfl.pv + m_platchManager->CbBuffer( m_bfl ) );
    ptag -= itag + 1;
#if !defined(ARM) && !defined(ARM64)
    _mm_prefetch( (char*)ptag, _MM_HINT_T0 );
#endif
    Assert( NULL != ptag );
    Assert( !FAssertNotOnPage_( ptag ) || FNegTest( fCorruptingPageLogically ) );
    Assert( (BYTE*)ptag > ((BYTE*)m_bfl.pv + CbPageHeader()) || FNegTest( fCorruptingPageLogically ) );
    Assert( (BYTE*)ptag >= ( ((BYTE*)m_bfl.pv + m_platchManager->CbBuffer( m_bfl )) - ((((PGHDR*)m_bfl.pv)->itagMicFree + 1)  * sizeof(TAG)) ) );
#ifdef DEBUG
    if( (BYTE*)ptag == ( ((BYTE*)m_bfl.pv + m_platchManager->CbBuffer( m_bfl )) - ((((PGHDR*)m_bfl.pv)->itagMicFree + 1)  * sizeof(TAG)) ) )
    {
        Assert( ((PGHDR*)m_bfl.pv)->cbFree > sizeof(TAG) );
    }
#endif

    return ptag;
}


#ifdef DEBUG
INLINE CPAGE::TAG * CPAGE::PtagFromRgbCbItag_( BYTE *rgbPage, INT cbPage, INT itag ) const
{
    Assert( itag >= 0 );
    Assert( itag <= ((PGHDR*)rgbPage)->itagMicFree );

    TAG * ptag = (TAG *)( (BYTE*)rgbPage + cbPage );
    ptag -= itag + 1;

    Assert( NULL != ptag );
    Assert( (BYTE*)ptag >= ( ((BYTE*)rgbPage + cbPage) - ((((PGHDR*)rgbPage)->itagMicFree+1)  * sizeof(TAG)) ) );
#ifdef DEBUG
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
#define FDoPageSnap()   ( rand() % 100 == 3 )   
#endif

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
#endif


INLINE ULONG CPAGE::CbTagArray_() const
{
    return ((PGHDR *)m_bfl.pv)->itagMicFree * sizeof(TAG);
}


INLINE BYTE * CPAGE::PbFromIb_( USHORT ib ) const
{
    Assert( ib >= 0 );
    Assert( (ULONG)ib <= ( m_platchManager->CbBuffer( m_bfl ) - CbPageHeader() - CbTagArray_() ) || FNegTest( fCorruptingPageLogically ) );

    return (BYTE*)m_bfl.pv + CbPageHeader() + ib;
}




CPAGE::CPAGE( ) :
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

CPAGE::~CPAGE( )
{
    Assert( FAssertUnused_( ) );
}


VOID CPAGE::PreInitializeNewPage_(  PIB * const ppib,
                                    const IFMP ifmp,
                                    const PGNO pgno,
                                    const OBJID objidFDP,
                                    const DBTIME dbtime )
{
    PGHDR *ppghdr;

    m_ppib = ppib;
    m_ifmp = ifmp;
    m_pgno = pgno;

#ifndef ENABLE_JET_UNIT_TEST
    Assert( ( ifmpNil != ifmp ) || FNegTest( fInvalidUsage ) || dbtime == dbtimeRevert );
#endif

    Assert( m_platchManager != &g_nullPageLatchManager );
    Assert( m_fSmallFormat != fFormatInvalid );
    Assert( m_platchManager->CbBuffer( m_bfl ) );

    memset( m_bfl.pv, 0, m_platchManager->CbBuffer( m_bfl ) );

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

ERR CPAGE::ErrGetNewPreInitPage(    PIB * ppib,
                                    IFMP ifmp,
                                    PGNO pgno,
                                    OBJID objidFDP,
                                    DBTIME dbtimeNoLog,
                                    BOOL fLogNewPage )
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

    Assert( !( bflfPreInitLatchFlags & bflfNew ) );
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

VOID CPAGE::ConsumePreInitPage( const ULONG fPageFlags )
{
    Assert( FFlags() == ( CPAGE::fPageNewRecordFormat | CPAGE::fPagePreInit ) );
    Assert( ( fPageFlags & CPAGE::fPagePreInit ) == 0 );
    Assert( ((PGHDR*)m_bfl.pv)->itagMicFree == 0 );

    PGHDR* const ppghdr = (PGHDR*)m_bfl.pv;
    ppghdr->itagMicFree = ctagReserved;

    USHORT cbFree = ppghdr->cbFree;
    cbFree -= (USHORT)sizeof( CPAGE::TAG );
    ppghdr->cbFree = cbFree;

    TAG* const ptag = PtagFromItag_( 0 );
    ptag->SetIb( this, 0 );
    ptag->SetCb( this, 0 );
    ptag->SetFlags( this, 0 );

    SetFlags( ( fPageFlags | CPAGE::fPageNewRecordFormat ) & ~CPAGE::fPagePreInit );
    Assert( FNewRecordFormat() );
    Assert( !FPreInitPage() );
}

VOID CPAGE::FinalizePreInitPage( OnDebug( const BOOL fCheckPreInitPage ) )
{
    Assert( ( m_dbtimePreInit == dbtimeNil ) == ( m_objidPreInit == objidNil ) );
    Expected( ( m_dbtimePreInit != dbtimeNil ) && ( m_objidPreInit != objidNil ) );
    Assert( FFlags() != 0 );
    Assert( !!fCheckPreInitPage == !!FPreInitPage() );
    Assert( !fCheckPreInitPage == ((PGHDR*)m_bfl.pv)->itagMicFree >= ctagReserved );
    m_dbtimePreInit = dbtimeNil;
    m_objidPreInit = objidNil;

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
        ppghdr->itagMicFree,
        ppghdr->cbFree  );
}

ERR CPAGE::ErrGetReadPage ( PIB * ppib,
                            IFMP ifmp,
                            PGNO pgno,
                            BFLatchFlags bflf,
                            BFLatch* pbflHint )
{
    ERR err;

    ASSERT_VALID( ppib );
    Assert( FAssertUnused_( ) );
    Assert( ifmpNil != ifmp );
    Assert( ( m_dbtimePreInit == dbtimeNil ) && ( m_objidPreInit == objidNil ) );
    m_bfl.dwContext = m_pgno == pgno ? m_bfl.dwContext : NULL;
    m_bfl.dwContext = pbflHint ? pbflHint->dwContext : m_bfl.dwContext;
    m_platchManager = &g_bfLatchManager;
    m_fSmallFormat  = FIsSmallPage( g_rgfmp[ifmp].CbPage() );

#ifdef MINIMAL_FUNCTIONALITY
#else
LatchPage:
#endif

    Assert( FAssertUnused_( ) );

    Call( ErrBFReadLatchPage( &m_bfl, ifmp, pgno, BFLatchFlags( bflf | bflfHint ), ppib->BfpriPriority( ifmp ), TcCurr() ) );

    if ( err != JET_errSuccess && pbflHint )
    {

        Assert( wrnBFBadLatchHint == err ||
                    wrnBFPageFlushPending == err );
        Assert( FBFCurrentLatch( &m_bfl ) );

        DWORD_PTR dwHint = BFGetLatchHint( &m_bfl );

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

    m_ppib          = ppib;
    m_ifmp          = ifmp;
    m_pgno          = pgno;

#ifdef MINIMAL_FUNCTIONALITY
    Enforce( FNewRecordFormat() );
#else
    if( !FNewRecordFormat() )
    {
        if( NULL == ppib->PvRecordFormatConversionBuffer() )
        {
            const ERR   errT = ppib->ErrAllocPvRecordFormatConversionBuffer();
            if ( errT < 0 )
            {
                BFReadUnlatch( &m_bfl );
                Abandon_();
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

                Call( errT );
            }
        }
        else
        {
            BFReadUnlatch( &m_bfl );
            Abandon_();

            UtilSleep( cmsecWaitWriteLatch );
            goto LatchPage;
        }

        CallS( ErrUPGRADEPossiblyConvertPage( this, m_pgno, ppib->PvRecordFormatConversionBuffer() ) );
        BFDowngradeWriteLatchToReadLatch( &m_bfl );
        Assert( FNewRecordFormat() );
    }
#endif

    ASSERT_VALID( this );
#ifdef DEBUG_PAGE
    DebugCheckAll();
#endif

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
    Expected( 0 == ( bflf & ( bflfNoFaultFail ) ) );
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

ERR CPAGE::ErrGetRDWPage(   PIB * ppib,
                            IFMP ifmp,
                            PGNO pgno,
                            BFLatchFlags bflf,
                            BFLatch* pbflHint )
{
    ERR err;

    ASSERT_VALID( ppib );
    Assert( FAssertUnused_( ) );
    Assert( ifmpNil != ifmp );
    Assert( ( m_dbtimePreInit == dbtimeNil ) && ( m_objidPreInit == objidNil ) );


    Expected( PinstFromIfmp( ifmp )->m_plog->FRecoveringMode() == fRecoveringRedo ||
                pgno <= g_rgfmp[ifmp].PgnoLast() ||
                bflf & bflfDBScan ||
                g_rgfmp[ifmp].FOlderDemandExtendDb() ||
                g_rgfmp[ifmp].FShrinkDatabaseEofOnAttach() );

    m_bfl.dwContext = m_pgno == pgno ? m_bfl.dwContext : NULL;
    m_bfl.dwContext = pbflHint ? pbflHint->dwContext : m_bfl.dwContext;
    m_platchManager = &g_bfLatchManager;
    m_fSmallFormat  = FIsSmallPage( g_rgfmp[ifmp].CbPage() );

    Call( ErrBFRDWLatchPage( &m_bfl, ifmp, pgno, BFLatchFlags( bflf | bflfHint ), ppib->BfpriPriority( ifmp ), TcCurr() ) );

    if ( err != JET_errSuccess && pbflHint )
    {
        DWORD_PTR dwHint = BFGetLatchHint( &m_bfl );

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

    m_ppib          = ppib;
    m_ifmp          = ifmp;
    m_pgno          = pgno;

#ifdef MINIMAL_FUNCTIONALITY
    Enforce( FNewRecordFormat() );
#else
    if( !FNewRecordFormat() )
    {
        CallS( ErrBFUpgradeRDWLatchToWriteLatch( &m_bfl ) );

        if( NULL == ppib->PvRecordFormatConversionBuffer() )
        {
            const ERR   errT    = ppib->ErrAllocPvRecordFormatConversionBuffer();
            if ( errT < 0 )
            {
                BFWriteUnlatch( &m_bfl );
                Abandon_();
                Call( errT );
            }
            CallS( errT );
        }
        CallS( ErrUPGRADEPossiblyConvertPage( this, m_pgno, ppib->PvRecordFormatConversionBuffer() ) );
        BFDowngradeWriteLatchToRDWLatch( &m_bfl );
        Assert( FNewRecordFormat() );
    }
#endif

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
    Expected( 0 == ( bflf & bflfNoFaultFail ) );
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


ERR CPAGE::ErrLoadPage(
    PIB * ppib,
    IFMP ifmp,
    PGNO pgno,
    const VOID * pv,
    const ULONG cb )
{
    Assert( ifmpNil != ifmp );
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

VOID CPAGE::LoadNewPage(
    const IFMP ifmp,
    const PGNO pgno,
    const OBJID objidFDP,
    const ULONG fFlags,
    VOID * const pv,
    const ULONG cb )
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
VOID CPAGE::LoadNewTestPage( __in const ULONG cb, __in const IFMP ifmp )
{
    Assert( 0 != cb );


    m_platchManager = &g_testPageLatchManager;
    m_platchManager->AllocateBuffer( &m_bfl, cb );
    Assert( g_loadedPageLatchManager.FIsLoadedPage( &m_bfl ) );



    m_fSmallFormat = FIsSmallPage( cb );


    PreInitializeNewPage_( ppibNil, ifmp, 2, 3, 0 );
    ConsumePreInitPage( 0x0 );


    PGHDR * ppghdr = (PGHDR*)PvBuffer();
    ppghdr->checksum = 0xf7e97daa;

}
#endif

VOID CPAGE::LoadPage( const IFMP ifmp, const PGNO pgno, VOID * const pv, const ULONG cb )
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

VOID CPAGE::LoadDehydratedPage( const IFMP ifmp, const PGNO pgno, VOID * const pv, const ULONG cb, const ULONG cbPage )
{
    LoadPage( ifmp, pgno, pv, cbPage );

    m_platchManager->SetBufferSize( &m_bfl, cb );

    Assert( cb != 0 );
    Assert( cb == m_platchManager->CbBuffer( m_bfl ) );
    Assert( cbPage != 0 );
    Assert( cbPage == m_platchManager->CbPage( m_bfl ) );

    Assert( g_loadedPageLatchManager.FIsLoadedPage( &m_bfl ) );
}

VOID CPAGE::LoadPage( VOID * const pv, const ULONG cb )
{
    Assert( cb != 0 );
    LoadPage( ifmpNil, pgnoNull, pv, cb );
}

VOID CPAGE::GetShrunkPage(
    const IFMP ifmp,
    const PGNO pgno,
    VOID * const pv,
    const ULONG cb )
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

VOID CPAGE::GetRevertedNewPage(
    const PGNO pgno,
    VOID * const pv,
    const ULONG cb )
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

VOID CPAGE::ReBufferPage( __in const BFLatch& bfl, const IFMP ifmp, const PGNO pgno, VOID * const pv, const ULONG cb )
{
    LoadPage( ifmp, pgno, pv, cb );

    Assert( (BOOL)FIsSmallPage( cb ) == m_fSmallFormat );

    Assert( g_loadedPageLatchManager.FIsLoadedPage( &m_bfl ) );

    m_platchManager = &g_bfLatchManager;
    m_bfl = bfl;

    Assert( !g_loadedPageLatchManager.FIsLoadedPage( &m_bfl ) );
}

VOID CPAGE::UnloadPage()
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


CPAGE& CPAGE::operator=( CPAGE& rhs )
{
    ASSERT_VALID( &rhs );
    Assert( &rhs != this );
    Assert( FAssertUnused_( ) );

    m_ppib          = rhs.m_ppib;
    m_ifmp          = rhs.m_ifmp;
    m_pgno          = rhs.m_pgno;
    m_dbtimePreInit = rhs.m_dbtimePreInit;
    m_objidPreInit  = rhs.m_objidPreInit;
    m_bfl.pv        = rhs.m_bfl.pv;
    m_bfl.dwContext = rhs.m_bfl.dwContext;
    m_platchManager = rhs.m_platchManager;
    m_fSmallFormat  = rhs.m_fSmallFormat;

    rhs.Abandon_( );
    return *this;
}

ERR CPAGE::ErrResetHeader( PIB *ppib, const IFMP ifmp, const PGNO pgno )
{
    ERR     err;
    BFLatch bfl;
    auto tc = TcCurr();

    CallR( ErrBFWriteLatchPage( &bfl, ifmp, pgno, bflfNew, ppib->BfpriPriority( ifmp ), tc ) );
    BFDirty( &bfl, bfdfDirty, tc );
    memset( bfl.pv, 0, g_bfLatchManager.CbBuffer( bfl ) );
    BFWriteUnlatch( &bfl );

    return JET_errSuccess;
}


VOID CPAGE::SetFEmpty( )
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    ULONG ulFlags = ((PGHDR*)m_bfl.pv)->fFlags;
    ulFlags |= fPageEmpty;
    ((PGHDR*)m_bfl.pv)->fFlags = ulFlags;
}


VOID CPAGE::SetFNewRecordFormat ( )
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    ULONG ulFlags = ((PGHDR*)m_bfl.pv)->fFlags;
    ulFlags |= fPageNewRecordFormat;
    ((PGHDR*)m_bfl.pv)->fFlags = ulFlags;
}


VOID CPAGE::SetPgno ( PGNO pgno )
{
    if ( !FSmallPageFormat() )
    {
        PGHDR2 * const ppghdr2 = ( PGHDR2* )(m_bfl.pv);
        ppghdr2->pgno = pgno;
    }
}


VOID CPAGE::SetPgnoNext ( PGNO pgnoNext )
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    ((PGHDR*)m_bfl.pv)->pgnoNext = pgnoNext;
}


VOID CPAGE::SetPgnoPrev ( PGNO pgnoPrev )
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    ((PGHDR*)m_bfl.pv)->pgnoPrev = pgnoPrev;
}


VOID CPAGE::SetDbtime( const DBTIME dbtime )
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


VOID CPAGE::RevertDbtime( const DBTIME dbtime, const ULONG fFlags )
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    Assert( ((PGHDR*)m_bfl.pv)->dbtimeDirtied >= dbtime );
    Expected( fFlags != 0 );
    m_platchManager->AssertPageIsDirty( m_bfl );
    Assert( FAssertWriteLatch( ) );

    ((PGHDR*)m_bfl.pv)->dbtimeDirtied = dbtime;

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


VOID CPAGE::SetCbUncommittedFree ( INT cbUncommittedFreeNew )
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
#ifdef DEBUG
    const INT   cbUncommittedFreeOld = ((PGHDR*)m_bfl.pv)->cbUncommittedFree;

    Unused( cbUncommittedFreeOld );
#endif
    ((PGHDR*)m_bfl.pv)->cbUncommittedFree = USHORT( cbUncommittedFreeNew );
    Assert( CbUncommittedFree() <= CbPageFree() );
}


VOID CPAGE::AddUncommittedFreed ( INT cbToAdd )
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    Assert( ((PGHDR*)m_bfl.pv)->cbUncommittedFree + cbToAdd > ((PGHDR*)m_bfl.pv)->cbUncommittedFree );
    ((PGHDR*)m_bfl.pv)->cbUncommittedFree = USHORT( ((PGHDR*)m_bfl.pv)->cbUncommittedFree + cbToAdd );
    Assert( CbUncommittedFree() <= CbPageFree() );
}


VOID CPAGE::ReclaimUncommittedFreed ( INT cbToReclaim )
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    Assert( ((PGHDR*)m_bfl.pv)->cbUncommittedFree - cbToReclaim < ((PGHDR*)m_bfl.pv)->cbUncommittedFree );
    Assert( ((PGHDR*)m_bfl.pv)->cbUncommittedFree - cbToReclaim >= 0 );
    ((PGHDR*)m_bfl.pv)->cbUncommittedFree = USHORT( ((PGHDR*)m_bfl.pv)->cbUncommittedFree - cbToReclaim );
    Assert( CbUncommittedFree() <= CbPageFree() );
}

VOID CPAGE::MarkAsSuperCold( )
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif

    m_platchManager->MarkAsSuperCold( &m_bfl );

    return;
}

BOOL CPAGE::FLoadedPage ( ) const
{
    return m_platchManager->FLoadedPage();
}

BOOL CPAGE::FIsNormalSized  ( ) const
{
    return CbPage() == m_platchManager->CbBuffer( m_bfl );
}

VOID CPAGE::Dirty( const BFDirtyFlags bfdf )
{
    Dirty_( bfdf );
    UpdateDBTime_();
    ResetFScrubbed_();
    m_platchManager->AssertPageIsDirty( m_bfl );
}


VOID CPAGE::DirtyForScrub( )
{
    PageAssertTrack( *this, !FScrubbed(), "DoubleScrubbing" );
    Assert( TcCurr().iorReason.Iort() == iortScrubbing );
    Dirty_( bfdfDirty );
    UpdateDBTime_();
    SetFScrubbed_();
    m_platchManager->AssertPageIsDirty( m_bfl );
}


VOID CPAGE::CoordinatedDirty( const DBTIME dbtime, const BFDirtyFlags bfdf )
{
    Dirty_( bfdf );
    CoordinateDBTime_( dbtime );
    ResetFScrubbed_();
    m_platchManager->AssertPageIsDirty( m_bfl );
}


BFLatch* CPAGE::PBFLatch ( )
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    return m_platchManager->PBFLatch( &m_bfl );
}

VOID CPAGE::GetLatchHint( INT iline, BFLatch** ppbfl ) const
{
    Assert( ppbfl );
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    Assert( fPageLeaf == 2 );

#ifdef MINIMAL_FUNCTIONALITY

    *ppbfl = NULL;

#else

    if( FLoadedPage() )
    {
        *ppbfl = NULL;
    }
    else
    {
        *ppbfl = (BFLatch*)( (BYTE*)( rgdwHintCache + ( ( m_pgno * 128 + iline ) & maskHintCache ) ) - OffsetOf( BFLatch, dwContext ) );
        *ppbfl = (BFLatch*)( DWORD_PTR( *ppbfl ) & ~( LONG_PTR( (DWORD_PTR)(((PGHDR*)m_bfl.pv)->fFlags) << ( sizeof( DWORD_PTR ) * 8 - 2 ) ) >> ( sizeof( DWORD_PTR ) * 8 - 1 ) ) );
    }

#endif
}

VOID CPAGE::UpdateDBTime_( )
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
        Assert( FAssertWriteLatch() );
    }
}

VOID CPAGE::CoordinateDBTime_( const DBTIME dbtime )
{
    PGHDR   * const ppghdr  = (PGHDR*)m_bfl.pv;

    Assert( ppghdr->dbtimeDirtied <= dbtime );
    ppghdr->dbtimeDirtied = dbtime;
}


VOID CPAGE::SetLgposModify( LGPOS lgpos )
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    Assert( FAssertWriteLatch( ) );

    if ( g_rgfmp[ m_ifmp ].FLogOn() )
    {
        Assert( !PinstFromIfmp( m_ifmp )->m_plog->FLogDisabled() );
        m_platchManager->SetLgposModify( &m_bfl, lgpos, m_ppib->lgposStart, TcCurr() );
    }
}


VOID CPAGE::Replace( INT iline, const DATA * rgdata, INT cdata, INT fFlags )
{
    Assert( iline >= 0 );
    Assert( rgdata );
    Assert( cdata > 0 );
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif

    Replace_( iline + ctagReserved, rgdata, cdata, fFlags );
}


VOID CPAGE::ReplaceFlags( INT iline, INT fFlags )
{
    Assert( iline >= 0 );
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif

    ReplaceFlags_( iline + ctagReserved, fFlags );
}


VOID CPAGE::Insert( INT iline, const DATA * rgdata, INT cdata, INT fFlags )
{
    Assert( iline >= 0 );
    Assert( rgdata );
    Assert( cdata > 0 );
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif

#ifdef DEBUG
    if( FExpensiveDebugCodeEnabled( Debug_Page_Shake ) )
    {
        DebugMoveMemory_();
    }
#endif

    Insert_( iline + ctagReserved, rgdata, cdata, fFlags );
}


VOID CPAGE::Delete( INT iline )
{
    Assert( iline >= 0 );
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif

    Delete_( iline + ctagReserved );
}


VOID CPAGE::SetExternalHeader( const DATA * rgdata, INT cdata, INT fFlags )
{
    Assert( rgdata );
    Assert( cdata > 0 );
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif

    Replace_( 0, rgdata, cdata, fFlags );
}


VOID CPAGE::ReleaseWriteLatch( BOOL fTossImmediate )
{
    ASSERT_VALID( this );
    DebugCheckAll();
    Assert( ( m_dbtimePreInit == dbtimeNil ) == ( m_objidPreInit == objidNil ) );

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

VOID CPAGE::ReleaseRDWLatch( BOOL fTossImmediate )
{
    ASSERT_VALID( this );
    DebugCheckAll();

    m_platchManager->ReleaseRDWLatch( &m_bfl, !!fTossImmediate );
    Abandon_();
    Assert( FAssertUnused_( ) );
}


VOID CPAGE::ReleaseReadLatch( BOOL fTossImmediate )
{
    ASSERT_VALID( this );
#ifdef DEBUG_PAGE
    DebugCheckAll();
#endif

    m_platchManager->ReleaseReadLatch( &m_bfl, !!fTossImmediate );
    Abandon_();
    Assert( FAssertUnused_( ) );
}


ERR CPAGE::ErrUpgradeReadLatchToWriteLatch( )
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


VOID CPAGE::UpgradeRDWLatchToWriteLatch( )
{
    ASSERT_VALID( this );
    Assert( FAssertRDWLatch() );

    Expected( PgnoThis() <= g_rgfmp[m_ifmp].PgnoLast() || g_rgfmp[m_ifmp].FOlderDemandExtendDb() || FLoadedPage() );

    m_platchManager->UpgradeRDWLatchToWriteLatch( &m_bfl );
    Assert( FAssertWriteLatch( ) );
}

ERR CPAGE::ErrTryUpgradeReadLatchToWARLatch()
{
    ASSERT_VALID( this );
    Assert( FAssertReadLatch() );
    Assert( PgnoThis() <= g_rgfmp[m_ifmp].PgnoLast() );

    const ERR err = m_platchManager->ErrTryUpgradeReadLatchToWARLatch( &m_bfl );
    Assert( JET_errSuccess != err || FAssertWARLatch( ) );
    return err;
}

VOID CPAGE::UpgradeRDWLatchToWARLatch()
{
    ASSERT_VALID( this );
    Assert( FAssertRDWLatch() );
    Assert( PgnoThis() <= g_rgfmp[m_ifmp].PgnoLast() );

    m_platchManager->UpgradeRDWLatchToWARLatch( &m_bfl );
    Assert( FAssertWARLatch( ) );
}

VOID CPAGE::DowngradeWriteLatchToRDWLatch( )
{
    ASSERT_VALID( this );
    DebugCheckAll();
    Assert( ( m_dbtimePreInit == dbtimeNil ) && ( m_objidPreInit == objidNil ) );

    Assert( FAssertWriteLatch( ) );
    m_platchManager->DowngradeWriteLatchToRDWLatch( &m_bfl );
    Assert( FAssertRDWLatch( ) );
}


VOID CPAGE::DowngradeWriteLatchToReadLatch( )
{
    ASSERT_VALID( this );
    DebugCheckAll();
    Assert( ( m_dbtimePreInit == dbtimeNil ) && ( m_objidPreInit == objidNil ) );

    Assert( FAssertWriteLatch() );
    m_platchManager->DowngradeWriteLatchToReadLatch( &m_bfl );
    Assert( FAssertReadLatch( ) );
}

VOID CPAGE::DowngradeWARLatchToRDWLatch()
{
    ASSERT_VALID( this );
    DebugCheckAll();

    Assert( FAssertWARLatch( ) );
    m_platchManager->DowngradeWARLatchToRDWLatch( &m_bfl );
    Assert( FAssertRDWLatch( ) );
}

VOID CPAGE::DowngradeWARLatchToReadLatch()
{
    ASSERT_VALID( this );
    DebugCheckAll();

    Assert( FAssertWARLatch() );
    m_platchManager->DowngradeWARLatchToReadLatch( &m_bfl );
    Assert( FAssertReadLatch( ) );
}

VOID CPAGE::DowngradeRDWLatchToReadLatch( )
{
    ASSERT_VALID( this );
    DebugCheckAll();

    Assert( FAssertRDWLatch() );
    m_platchManager->DowngradeRDWLatchToReadLatch( &m_bfl );
    Assert( FAssertReadLatch( ) );
}

VOID CPAGE::SetFScrubbedValue_( const BOOL fValue )
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


VOID CPAGE::SetFScrubbed_( )
{
    SetFScrubbedValue_( fTrue );
}


VOID CPAGE::ResetFScrubbed_( )
{
    SetFScrubbedValue_( fFalse );
}


BOOL CPAGE::FLastNodeHasNullKey() const
{
    const PGHDR * const ppghdr  = (PGHDR*)m_bfl.pv;
    LINE                line;

    Assert( FInvisibleSons() );

    Assert( ppghdr->itagMicFree > ctagReserved );
    GetPtr( ppghdr->itagMicFree - ctagReserved - 1, &line );

    Assert( !( line.fFlags & (fNDVersion|fNDDeleted) ) );

    return ( !( line.fFlags & fNDCompressed )
            && cbKeyCount + sizeof(PGNO) == line.cb
            && 0 == *(UnalignedLittleEndian<SHORT> *)line.pv );
}


BOOL CPAGE::FPageIsInitialized( __in const void* const pv, __in ULONG cb )
{
    Assert ( pv != NULL );

    const BYTE rgbZeroMem[256] = { 0 };
    const BYTE* pvAddrToCompare = (BYTE*)pv;
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

    if ( pvAddrToCompare < pvPageEnd )
    {
        return ( memcmp ( pvAddrToCompare, rgbZeroMem, pvPageEnd - pvAddrToCompare ) != 0 );
    }

    return fFalse;
}

BOOL CPAGE::FPageIsInitialized() const
{
    return CPAGE::FPageIsInitialized( m_bfl.pv, CbPage() );
}

BOOL CPAGE::FShrunkPage ( ) const
{
    return dbtimeShrunk == Dbtime();
}


VOID CPAGE::ResetAllFlags( INT fFlags )
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    Assert( FAssertWriteLatch() || FAssertWARLatch() );

    const BOOL fSmallFormat = FSmallPageFormat();

    for ( INT itag = ((PGHDR*)m_bfl.pv)->itagMicFree - 1; itag >= ctagReserved; itag-- )
    {
        TAG * const ptag = PtagFromItag_( itag );
        ptag->SetFlags( this, USHORT( ptag->FFlags( this, fSmallFormat ) & ~fFlags ) );
    }

    DebugCheckAll();
}


bool CPAGE::FAnyLineHasFlagSet( INT fFlags ) const
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif

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


VOID CPAGE::Dirty_( const BFDirtyFlags bfdf )
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    Assert( FAssertWriteLatch() ||
            FAssertWARLatch() );
    Assert( FIsNormalSized() );

    if( FLoadedPage() )
    {
    }
    else
    {

        BFDirtyFlags bfdfT = bfdf;
        TraceContext tc = TcCurr();


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

VOID CPAGE::OverwriteUnusedSpace( const CHAR chZero )
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif
    DEBUG_SNAP_PAGE();

    const PGHDR * const ppghdr = (PGHDR*)m_bfl.pv;

    if ( CbContiguousFree_( ) != ppghdr->cbFree )
    {
        ZeroOutGaps_( chZero );
    }

    BYTE * const pbFree = PbFromIb_( ppghdr->ibMicFree );
    memset( pbFree, chZero, CbContiguousFree_() );

    DEBUG_SNAP_COMPARE();
    DebugCheckAll( );
}


VOID CPAGE::ReorganizePage(
    __out const VOID ** pvHeader,
    __out size_t * const  pcbHeader,
    __out const VOID ** pvTrailer,
    __out size_t * const pcbTrailer)
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

    if ( CbContiguousFree_( ) != ppghdr->cbFree )
    {
        ReorganizeData_( reorgPageMoveLogging );
    }

    const BYTE * const pbMin    = (BYTE *)m_bfl.pv;
    const BYTE * const pbFree   = PbFromIb_( ppghdr->ibMicFree );
    const BYTE * const pbTags   = (BYTE *)PtagFromItag_( ppghdr->itagMicFree-1 );
    const BYTE * const pbMax    = pbMin + m_platchManager->CbBuffer( m_bfl );

    Assert( FOnPage( pbFree, 0 ) );
    Assert( FOnPage( pbTags, CbTagArray_() ) );
    Assert( pbTags >= pbFree );

    Assert( pbFree >= pbMin + CbPageHeader() );
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

XECHECKSUM CPAGE::UpdateLoggedDataChecksum_(
                    const XECHECKSUM checksumLoggedData,
    __in_bcount(cb) const BYTE * const pb,
                    const INT cb )
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


PAGECHECKSUM CPAGE::LoggedDataChecksum() const
{
    const PGHDR2* const ppghdr2 = ( const PGHDR2* )m_bfl.pv;

    XECHECKSUM checksum = 0;

    PGHDR2 pghdr2T;
    const USHORT cbPageHeader = CbPageHeader();
    AssertPREFIX( cbPageHeader <= sizeof( pghdr2T ) );
    memcpy( &pghdr2T, ppghdr2, cbPageHeader );
    pghdr2T.pghdr.checksum          = 0;
    pghdr2T.pghdr.cbUncommittedFree = 0;
    pghdr2T.pghdr.cbFree            = 0;
    pghdr2T.pghdr.ibMicFree         = 0;
    pghdr2T.pghdr.fFlags            &= ~( maskFlushType | fPageNewChecksumFormat );
    memset( pghdr2T.rgChecksum, 0, sizeof( pghdr2T.rgChecksum ) );
    memset( pghdr2T.rgbReserved, 0, sizeof( pghdr2T.rgbReserved ) );
    checksum = UpdateLoggedDataChecksum_( checksum, ( const BYTE* )&pghdr2T, CbPageHeader() );

    const BOOL fSmallFormat = FSmallPageFormat();

    BOOL fCaughtTagNotOnPage = fFalse;
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
        const BYTE * const  pb      = cb < cbKeyCount ? NULL : PbFromIb_( ptag->Ib( fSmallFormat ) );
        if( pb && !FOnData( pb, cb ) )
        {
            if ( !FNegTest( fCorruptingPageLogically ) )
            {
                FireWall( "LogicalDataSumPbNotOnPage" );
            }
            continue;
        }

        const BYTE          bFlags  = (BYTE)(ptag->FFlags( this, fSmallFormat ) & (~fNDVersion));

        Assert( 0 == cb || cbKeyCount <= cb );
        if ( !(ptag->FFlags( this, fSmallFormat ) & fNDDeleted ) && cbKeyCount <= cb )
        {
            AssertRTL( pb == PbFromIb_( ptag->Ib( fSmallFormat ) ) );
            USHORT              cbKey   = ptag->Cb( fSmallFormat );
            Assert( sizeof( cbKey ) == cbKeyCount );

            checksum = UpdateLoggedDataChecksum_( checksum, ( const BYTE* )&cbKey, sizeof( cbKey ) );
            checksum = UpdateLoggedDataChecksum_( checksum, pb + cbKeyCount, cb - cbKeyCount );
        }
        checksum = ( _rotl64( checksum, 6 ) + bFlags );
    }

    return checksum;
}

ERR CPAGE::ErrCheckForUninitializedPage_( IPageValidationAction * const pvalidationaction ) const
{
    Assert( pvalidationaction );
    if ( !FPageIsInitialized() )
    {
        pvalidationaction->UninitPage( m_pgno, JET_errPageNotInitialized );
        return ErrERRCheck( JET_errPageNotInitialized );
    }

    return JET_errSuccess;
}

ERR CPAGE::ErrReadVerifyFailure_() const
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
            Error( ErrERRCheck( JET_errReadVerifyFailure ) );
        }
    }
HandleError:
    return err;
}

VOID CPAGE::ReportReadLostFlushVerifyFailure_(
    IPageValidationAction * const pvalidationaction,
    const FMPGNO fmpgno,
    const PageFlushType pgftExpected,
    const BOOL fUninitializedPage,
    const BOOL fRuntime,
    const BOOL fFailOnRuntimeOnly ) const
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
        (void)ErrDumpToIrsRaw( L"LostFlush", wszLostFlushMessage );
    }
}

VOID CPAGE::SetFlushType_( __in const CPAGE::PageFlushType pgft )
{
    ((PGHDR*)m_bfl.pv)->fFlags = ( ((PGHDR*)m_bfl.pv)->fFlags & ~maskFlushType ) | ( (ULONG)pgft << 15 );
}

ERR CPAGE::ErrValidatePage(
    __in const PAGEValidationFlags pgvf,
    __in IPageValidationAction * const pvalidationaction,
    __in CFlushMap* pflushmap )
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
    Assert( pflushmap == NULL );
#endif

    if ( fCheckForLostFlush )
    {
        if ( ( pflushmap == NULL ) && ( g_rgfmp != NULL ) && ( m_ifmp != ifmpNil ) )
        {
            pflushmap = g_rgfmp[ m_ifmp ].PFlushMap();
        }

        AssertSz( pflushmap != NULL, "We must have a flush map available." );
    }

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
            fTrue,
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
                fTrue,
                fRuntimeFlushState,
                fFailOnRuntimeLostFlushOnly );

            if ( fRuntimeFlushState || !fFailOnRuntimeLostFlushOnly )
            {
                Error( ErrERRCheck( JET_errReadLostFlushVerifyFailure ) );
            }
        }

        Call( err );
        
        err = ErrCheckPage( CPRINTFDBGOUT::PcprintfInstance(), OnErrorReturnError, CheckAll );


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
        for ( ULONG ibReserved = 0; ibReserved < _countof(ppghdr2->rgbReserved); ibReserved++ )
        {
            AssertRTL( ppghdr2->rgbReserved[ibReserved] == 0 );
        }
#endif
    }


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
            fFalse,
            fRuntimeFlushState,
            fFailOnRuntimeLostFlushOnly );

        if ( fRuntimeFlushState || !fFailOnRuntimeLostFlushOnly )
        {
            Error( ErrERRCheck( JET_errReadLostFlushVerifyFailure ) );
        }
    }


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
#endif

HandleError:
    return err;
}

NOINLINE ERR CPAGE::ErrCaptureCorruptedPageInfo_( const CheckPageMode mode, const PCSTR szCorruptionType, const PCWSTR wszCorruptionDetails, const CHAR * const szFile, const LONG lLine, const BOOL fLogEvent ) const
{
    WCHAR wszType[100];
    OSStrCbFormatW( wszType, sizeof( wszType ), L"%hs", szCorruptionType );
    return ErrCaptureCorruptedPageInfo_( mode, wszType, wszCorruptionDetails, szFile, lLine, fLogEvent );
}

NOINLINE ERR CPAGE::ErrCaptureCorruptedPageInfo_( const CheckPageMode mode, const PCWSTR szCorruptionType, const PCWSTR szCorruptionDetails, const CHAR * const szFile, const LONG lLine, const BOOL fLogEvent ) const
{
    OnRTM( BOOL fFriendyEvent = fFalse );


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

            OnRTM( fFriendyEvent = fTrue );
        }


        if ( g_lesCorruptPages.FNeedToLog( ( ( m_ifmp <= 7 ) ? ( m_ifmp << 29 ) : ( m_ifmp << 24 ) ) | PgnoThis() ) )
        {
            (void)ErrDumpToIrsRaw( szCorruptionType, szCorruptionDetails ? szCorruptionDetails : L"" );
        }
    }


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


    static volatile BYTE *s_rgbBufDebugDoNotUse;
    BYTE *rgbBuf = (BYTE *)_alloca( CbBuffer() );
    s_rgbBufDebugDoNotUse = rgbBuf;

    memcpy( rgbBuf, (BYTE *)PvBuffer(), CbBuffer() );


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

ERR CPAGE::ErrCheckPage(
    CPRINTF * const pcprintf,
    const CheckPageMode mode,
    const CheckPageExtensiveness grbitExtensiveCheck
     ) const
{
    ERR                 err     = JET_errSuccess;
    const PGHDR * const ppghdr  = (PGHDR*)m_bfl.pv;
    ULONG              *rgdw    = NULL;
    KEY                 keyLast;



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


    if ( ( ppghdr->itagMicFree == 0 ) && !FEmptyPage() && !FPreInitPage() )
    {
        (*pcprintf)( "page corruption (%d): Empty page without fPageEmpty or fPagePreInit flags set\r\n", m_pgno );
        Error( ErrCaptureCorruptedPageInfo( mode, L"EmptyAndPreInitFlagsNotSet" ) );
    }

    const __int64 ctags = (__int64)((PGHDR *)m_bfl.pv)->itagMicFree;
    const ULONG_PTR pbPageDataStart = PbDataStart_();
    const ULONG_PTR pbPageDataEnd = PbDataEnd_();
    if ( pbPageDataEnd <= ( pbPageDataStart + 1  ) )
    {
        (*pcprintf)( "page corruption (%d): itagMicFree / tag array too large, overlapping PGHDR (%p,%p,%I64d / %d).\r\n", m_pgno, pbPageDataEnd, pbPageDataStart, ctags, CbTagArray_() );
        PageAssertTrack( *this, fFalse, "TagArrayWalkingOntoPghdr" );
#ifdef DEBUG
        Error( ErrCaptureCorruptedPageInfo( mode, L"TagArrayWalkingOntoPghdr" ) );
#endif
    }

    if ( ctags > 0 )
    {
        const __int64 cbMinLine = 4;
        if ( __int64( __int64( pbPageDataEnd - pbPageDataStart ) - ( ( ctags - 1 ) * cbMinLine ) ) < 0 )
        {
            (*pcprintf)( "page corruption (%d): itagMicFree / tag array too large for real data left over (%I64d / %d,%p,%p).\r\n", m_pgno, ctags, CbTagArray_(), pbPageDataEnd, pbPageDataStart );
            PageAssertTrack( *this, FNegTest( fCorruptingPageLogically ), "TagArrayTooBigToLeaveDataRoom" );
#ifdef DEBUG
            Error( ErrCaptureCorruptedPageInfo( mode, L"TagArrayTooBigToLeaveDataRoom" ) );
#endif
        }
    }


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

        FOSSetCleanupState( fCleanUpStateSavedSavedSaved );
    }

    const INT cbPrefixMax = ( FSpaceTree() || ppghdr->objidFDP == 0  ) ? 0 : PtagFromItag_( 0 )->Cb( fSmallFormat );

    for ( INT itag  = 0; itag < ppghdr->itagMicFree; ++itag )
    {
        TAG * const ptag = PtagFromItag_( itag );

        USHORT ib = ptag->Ib( fSmallFormat );
        USHORT cb = ptag->Cb( fSmallFormat );

        cbTotal += cb;


        if ( cb == 0 )
        {
            if ( itag == 0 )
            {
                continue;
            }
            (*pcprintf)( "page corruption (%d): Non TAG-0 - TAG %d has zero cb(cb = %d, ib = %d)\r\n", m_pgno, itag, cb, ib );
            Error( ErrCaptureCorruptedPageInfo( mode, L"ZeroLengthIline" ) );
        }



        if ( (DWORD)ib + (DWORD)cb > (DWORD)ppghdr->ibMicFree )
        {
            (*pcprintf)( "page corruption (%d): TAG %d ends in free space (cb = %d, ib = %d, ibMicFree = %d)\r\n",
                            m_pgno, itag, cb, ib, (USHORT)ppghdr->ibMicFree );
            Error( ErrCaptureCorruptedPageInfo( mode, L"TagInFreeSpace" ) );
        }

        if ( grbitExtensiveCheck )
        {

            const DWORD ibEnd = (DWORD)ib + (DWORD)cb;
            const DWORD cbPageAvail = CbBuffer();
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


            Expected( line.pv != NULL );
            Expected( line.cb != 0 || FNegTest( fCorruptingPageLogically ) );

            if ( line.pv == NULL )
            {
                AssertTrack( errGetLine < JET_errSuccess, "GotNullLineWithoutErrValue" );
                (*pcprintf)( "page corruption (%d): TAG %d offset / line could not be retrieve from tag array (ib=%d, cb=%d).\r\n", m_pgno, itag, ib, cb );
                PageAssertTrack( *this, fFalse, "GetLineGotNullPtr" );
#ifdef DEBUG
                Error( ErrCaptureCorruptedPageInfo( mode, L"GetLineGotNullPtr" ) );
#endif
            }

            const ULONG_PTR pbLineLastByte = (ULONG_PTR)line.pv + (ULONG_PTR)cb - (ULONG_PTR)1;

            PageAssertTrack( *this, pbLineLastByte >= (ULONG_PTR)line.pv, "LastByteOverflowedToBeforeLineStart" );

            if ( pbLineLastByte < pbPageDataStart )
            {
                (*pcprintf)( "page corruption (%d): TAG %d computed whole line is too low (ib=%d, cb=%d, %p < %p).\r\n", m_pgno, itag, ib, cb, pbLineLastByte, pbPageDataStart );
                PageAssertTrack( *this, fFalse, "LineEntirelyBelowDataSection" );
#ifdef DEBUG
                Error( ErrCaptureCorruptedPageInfo( mode, L"LineEntirelyBelowDataSection" ) );
#endif
            }
            if ( (ULONG_PTR)line.pv < pbPageDataStart )
            {
                (*pcprintf)( "page corruption (%d): TAG %d computed start offset starts too low (ib=%d, cb=%d, %p < %p).\r\n", m_pgno, itag, ib, cb, line.pv, pbPageDataStart );
                PageAssertTrack( *this, fFalse, "LineStartsBelowDataSection" );
#ifdef DEBUG
                Error( ErrCaptureCorruptedPageInfo( mode, L"LineStartsBelowDataSection" ) );
#endif
            }
            if ( (ULONG_PTR)line.pv > pbPageDataEnd )
            {
                (*pcprintf)( "page corruption (%d): TAG %d computed offset starts too high (ib=%d, cb=%d, %p > %p).\r\n", m_pgno, itag, ib, cb, line.pv, pbPageDataEnd );

                PageAssertTrack( *this, fFalse, "LineEntirelyAboveDataSection" );
#ifdef DEBUG
                Error( ErrCaptureCorruptedPageInfo( mode, L"LineEntirelyAboveDataSection" ) );
#endif
            }
            if ( pbLineLastByte > pbPageDataEnd )
            {
                (*pcprintf)( "page corruption (%d): TAG %d computed offset starts too high (ib=%d, cb=%d, %p > %p).\r\n", m_pgno, itag, ib, cb, pbLineLastByte, pbPageDataEnd );
                PageAssertTrack( *this, FNegTest( fCorruptingPageLogically ), "LineEndsAboveDataSection" );
#ifdef DEBUG
                Error( ErrCaptureCorruptedPageInfo( mode, L"LineEndsAboveDataSection" ) );
#endif
            }

            if ( errGetLine < JET_errSuccess || !FOnData( line.pv, line.cb ) )
            {
                CHAR szGetLineErr[40];
                OSStrCbFormatA( szGetLineErr, sizeof( szGetLineErr ), "GetLineFailed%d\n", errGetLine );
                (*pcprintf)( "UNCAUGHT: page corruption (%d): TAG %d ErrGetPtr() failed or got line off page (ib=%d, cb=%d, err=%d,f=%d).\r\n", m_pgno, itag, ib, cb, errGetLine, FOnData( line.pv, line.cb ) );
                PageAssertTrack( *this, FNegTest( fCorruptingPageLogically ), "GetLineFailed%d", errGetLine );
#ifdef DEBUG
                Error( ErrCaptureCorruptedPageInfo( mode, szGetLineErr ) );
#endif
            }


            if ( ( grbitExtensiveCheck & CheckLineBoundedByTag ) && !FSpaceTree() && itag >= ctagReserved  )
            {
                KEYDATAFLAGS kdf;
                kdf.Nullify();

                const ERR errGetKdf = ErrNDIGetKeydataflags( *this, iline, &kdf );

                const CHAR * szGetKdfSourceFile = ( errGetKdf < JET_errSuccess && PefLastThrow() != NULL ) ? PefLastThrow()->SzFile() : NULL;
                const ULONG ulGetKdfSourceLine = ( errGetKdf < JET_errSuccess && PefLastThrow() != NULL ) ? PefLastThrow()->UlLine() : 0;
                Expected( errGetKdf >= JET_errSuccess || ( szGetKdfSourceFile != NULL && ulGetKdfSourceLine > 0 ) );
                Expected( szGetKdfSourceFile == NULL || 0 == _stricmp( SzSourceFileName( szGetKdfSourceFile ), "node.cxx" ) );


                if ( FNDCompressed( kdf ) && ( kdf.key.prefix.Cb() > cbPrefixMax || kdf.key.prefix.Cb() < 0 ) )
                {
                    (*pcprintf)( "page corruption (%d): TAG %d prefix usage is larger than prefix node (prefix.Cb() = %d, cbPrefixMax = %d)\r\n",
                                    m_pgno, itag, kdf.key.prefix.Cb(), cbPrefixMax );
                    Error( ErrCaptureCorruptedPageInfo( mode, L"TagcbPrefixTooLarge" ) );
                }

#ifdef DEBUG
                if ( ErrFaultInjection( 60348 ) )
                {
                    ErrCaptureCorruptedPageInfo( CPAGE::OnErrorReturnError, L"TestFakeCheckPageCorruption" );
                    err = JET_errSuccess;
                }
#endif
                if ( kdf.key.suffix.Cb() > cb || kdf.key.suffix.Cb() < 0 )
                {
                    (*pcprintf)( "page corruption (%d): TAG %d suffix size is larger than the actual tag size (suffix.Cb() = %d, cb = %d)\r\n",
                                    m_pgno, itag, kdf.key.suffix.Cb(), cb );
                    Error( ErrCaptureCorruptedPageInfo( mode, L"TagcbSuffixTooLarge" ) );
                }


                if ( kdf.data.Cb() > cb || kdf.data.Cb() < 0 )
                {
                    (*pcprintf)( "page corruption (%d): TAG %d data size is larger than actual tag size (data.Cb() = %d, cb = %d)\r\n",
                                    m_pgno, itag, kdf.data.Cb(), cb );
                    Error( ErrCaptureCorruptedPageInfo( mode, L"TagcbDataTooLarge" ) );
                }

                if ( kdf.data.Cb() == 0 )
                {
                    (*pcprintf)( "page corruption (%d): TAG %d data size is zero ... we don't have non-data nodes ... yet at least. (data.Cb() = %d, cb = %d)\r\n",
                                    m_pgno, itag, kdf.data.Cb(), cb );

                    PageAssertTrack( *this, FNegTest( fCorruptingPageLogically ), "TagCbDataZero" );
#ifdef DEBUG
                    Error( ErrCaptureCorruptedPageInfo( mode, L"TagCbDataZero" ) );
#endif
                }


                const INT cbInternalNode = ( ( FNDCompressed( kdf ) ? 2 : 0 )  +
                                                sizeof(USHORT)  +
                                                kdf.key.suffix.Cb() +
                                                kdf.data.Cb() );
                if ( cbInternalNode > cb || cbInternalNode < 0 )
                {
                    (*pcprintf)( "page corruption (%d): TAG %d aggregate key suffix/data size is larger than actual tag size (cbInternalNode=( 2 + %d + %d + %d ), cb = %d)\r\n",
                                    m_pgno, itag, ( FNDCompressed( kdf ) ? 2 : 0 ), kdf.key.suffix.Cb(), kdf.data.Cb(), cb );
                    Error( ErrCaptureCorruptedPageInfo( mode, L"TagDataTooLarge" ) );
                }


                if ( errGetKdf < JET_errSuccess )
                {
                    CHAR szGetKdfErr [40];
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
                         ( FLeafPage() || itag < ppghdr->itagMicFree - 1 ) &&
                         CmpKey( kdf.key, keyLast ) < 0 )
                    {
                        (*pcprintf)( "page corruption (%d): TAG %d out of order on the page compared to the previous tag\r\n", m_pgno, itag );
                        Error( ErrCaptureCorruptedPageInfo( mode, L"LinesOutOfOrder" ) );
                    }

                    keyLast = kdf.key;
                }
            }


            if ( rgdw )
            {



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

                if ( cb )
                {
                    USHORT iBit          = ib;
                    ULONG  bit           = (BYTE)( 1 << ( iBit & 31 ) );
                    DWORD *pdw           = rgdw + iBit / 32;
                    ULONG  set           = bit;
                    USHORT cbitRightSkip = iBit & 31;

                    set -= 1;
                    set = ~set;

                    USHORT cbitLeftSkip = 32 - cb - cbitRightSkip;
                    set <<= cbitLeftSkip;
                    set >>= cbitLeftSkip;

                    dwOverlaps |= *pdw & set;
                    *pdw       |= set;
                }
            }
        }
    }

    if ( dwOverlaps != 0 )
    {
        (*pcprintf)( "page corruption (%d): TAG overlap \r\n",
                    m_pgno );
        Error( ErrCaptureCorruptedPageInfo( mode, L"TagsOverlap" ) );
    }


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

VOID CPAGE::PreparePageForWrite( 
    __in const CPAGE::PageFlushType pgft,
    __in const BOOL fSkipSetFlushType,
    __in const BOOL fSkipFMPCheck )
{
    Assert( FIsNormalSized() );

    if ( !fSkipSetFlushType )
    {
        SetFlushType_( pgft );
    }

#ifndef ENABLE_JET_UNIT_TEST
    Expected( fSkipFMPCheck || ifmpNil != m_ifmp || FNegTest( fInvalidUsage )  );
#endif
    Assert( CbPage() == m_platchManager->CbBuffer( m_bfl ) );


    SetPageChecksum( (BYTE *)m_bfl.pv, CbPage(), databasePage, m_pgno );


    (VOID)ErrCheckPage( CPRINTFDBGOUT::PcprintfInstance(), OnErrorEnforce );

}


VOID CPAGE::Abandon_( )
{
    if( FLoadedPage() )
    {
        UnloadPage();
    }
    else
    {
#ifdef DEBUG
        Assert( !FAssertUnused_( ) );

        m_ppib = ppibNil;
        m_dbtimePreInit = dbtimeNil;
        m_objidPreInit = objidNil;

        Assert( FAssertUnused_( ) );
#endif
    }
    m_platchManager = &g_nullPageLatchManager;
    m_fSmallFormat = fFormatInvalid;
}



VOID CPAGE::Replace_( INT itag, const DATA * rgdata, INT cdata, INT fFlags )
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


    const SHORT cbDiff  = (SHORT)((INT)ptag->Cb( fSmallFormat ) - (INT)cbTotal);

#ifdef DEBUG
    if ( cbDiff < 0 )
    {
        for ( INT i = 0; i < cdata; i++ )
        {
            Assert( FAssertNotOnPage_ ( rgdata[ i ].Pv() ) );
        }
    }
#endif

    Assert( cbDiff > 0 || -cbDiff <= ppghdr->cbFree );

    if (    cbDiff >= 0 ||
            (   ptag->Ib( fSmallFormat ) + ptag->Cb( fSmallFormat ) == ppghdr->ibMicFree &&
                CbContiguousFree_( ) >= -cbDiff ) )
    {

        if ( ptag->Ib( fSmallFormat ) + ptag->Cb( fSmallFormat ) == ppghdr->ibMicFree )
        {
            ppghdr->ibMicFree = USHORT( ppghdr->ibMicFree - cbDiff );
        }
        ppghdr->cbFree = USHORT( ppghdr->cbFree + cbDiff );
    }
    else
    {

        if ( FRuntimeScrubbingEnabled_() )
        {
            memset( PbFromIb_( ptag->Ib( fSmallFormat ) ), chPAGEReplaceFill, ptag->Cb( fSmallFormat ) );
        }

        USHORT cbFree = (USHORT)( ppghdr->cbFree + ptag->Cb( fSmallFormat ) );
        ppghdr->cbFree = cbFree;
        ptag->SetIb( this, 0 );
        ptag->SetCb( this, 0 );
        ptag->SetFlags( this, 0 );

#ifdef DEBUG
        if( FExpensiveDebugCodeEnabled( Debug_Page_Shake ) )
        {
            DebugMoveMemory_();
        }
#endif

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

        memset( PbFromIb_( ptag->Ib( fSmallFormat ) + ptag->Cb( fSmallFormat ) ), chPAGEReplaceFill, cbDiff );
    }

    DebugCheckAll();
}


VOID CPAGE::ReplaceFlags_( INT itag, INT fFlags )
{
    Assert( itag >= 0 && itag < ((PGHDR*)m_bfl.pv)->itagMicFree );
    Assert( FAssertWriteLatch() || FAssertWARLatch() );

    TAG * const ptag = PtagFromItag_( itag );
    ptag->SetFlags( this, (USHORT)fFlags );

#ifdef DEBUG_PAGE
    DebugCheckAll();
#endif
}


VOID CPAGE::Insert_( INT itag, const DATA * rgdata, INT cdata, INT fFlags )
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

        TAG * const ptagLast = PtagFromItag_( ppghdr->itagMicFree - 1);
        USHORT ibAfterLastTag = ptagLast->Ib( fSmallFormat ) + ptagLast->Cb( fSmallFormat );
        Assert( ibAfterLastTag <= ppghdr->ibMicFree );
    }
#endif

    const USHORT cbTotal = (USHORT)CbDataTotal_( rgdata, cdata );
    Assert( cbTotal >= 0 );

#ifdef DEBUG_PAGE
    INT idata = 0;
    for ( ; idata < cdata; ++idata )
    {
        Assert( FAssertNotOnPage_( rgdata[idata].Pv() ) );
    }
#endif

    FreeSpace_( cbTotal + sizeof( CPAGE::TAG ) );

    if( itag != ppghdr->itagMicFree )
    {

        VOID * const pvTagSrc   = PtagFromItag_( ppghdr->itagMicFree-1 );
        VOID * const pvTagDest  = PtagFromItag_( ppghdr->itagMicFree );
        const LONG  cTagsToMove = ppghdr->itagMicFree - itag;


        Assert( pvTagDest < pvTagSrc );
        Assert( sizeof( CPAGE::TAG ) == ( (LONG_PTR)pvTagSrc - (LONG_PTR)pvTagDest ) );
        Assert( cTagsToMove > 0 );


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


VOID CPAGE::Delete_( INT itag )
{
    PGHDR *ppghdr = (PGHDR*)m_bfl.pv;

    Assert( itag >= ctagReserved && itag < ppghdr->itagMicFree );
    Assert( FAssertWriteLatch( ) );
    Assert( FIsNormalSized() );

    const TAG * const ptag = PtagFromItag_( itag );
    const BOOL fSmallFormat = FSmallPageFormat();

    if ( FRuntimeScrubbingEnabled_() )
    {
        BYTE * pb = PbFromIb_(ptag->Ib( fSmallFormat ));
        memset(pb, chPAGEDeleteFill, ptag->Cb( fSmallFormat ));
    }

    if ( (ptag->Cb( fSmallFormat ) + ptag->Ib( fSmallFormat )) == ppghdr->ibMicFree )
    {
        const USHORT ibMicFree = (USHORT)( ppghdr->ibMicFree - ptag->Cb( fSmallFormat ) );
        ppghdr->ibMicFree = ibMicFree;
    }


    const USHORT cbFree =   (USHORT)( ppghdr->cbFree + ptag->Cb( fSmallFormat ) + sizeof( CPAGE::TAG ) );
    ppghdr->cbFree = cbFree;

    ppghdr->itagMicFree = USHORT( ppghdr->itagMicFree - 1 );
    copy_backward(
        PtagFromItag_( ppghdr->itagMicFree ),
        PtagFromItag_( itag ),
        PtagFromItag_( itag-1 ) );

    if ( ppghdr->cbFree + sizeof( CPAGE::TAG ) + CbPageHeader() == m_platchManager->CbBuffer( m_bfl ) )
    {
        AssertRTL( PtagFromItag_( 0 )->Cb( fSmallFormat ) == 0 );
        AssertRTL( ppghdr->itagMicFree == 1 );
        ppghdr->ibMicFree = 0;
    }

    DebugCheckAll();
}


INLINE VOID CPAGE::ReorganizeData_( __in_range( reorgOther, reorgMax - 1 ) const CPAGEREORG reorgReason )
{
    PGHDR * const ppghdr = (PGHDR*)m_bfl.pv;

    DEBUG_SNAP_PAGE();

    Assert( !FPreInitPage() );
    Assert( ppghdr->itagMicFree > 0 );
    Assert( 0 != ppghdr->cbFree );

    const BOOL fSmallFormat = FSmallPageFormat();


    BYTE *rgbBuf;
    BFAlloc( bfasTemporary, (VOID **)&rgbBuf );
    TAG ** rgptag = (TAG **)rgbBuf;


    INT iptag   = 0;
    INT itag    = 0;
    for ( ; itag < ppghdr->itagMicFree; ++itag )
    {
        TAG * const ptag = PtagFromItag_( itag );
        rgptag[iptag++] = ptag;
    }

    const INT cptag = iptag;
    Assert( iptag <= ppghdr->itagMicFree );

    sort( rgptag, rgptag + cptag, PfnCmpPtagIb() );

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

        PERFOpt( cCPAGEReorganizeData[reorgReason].Inc( PinstFromIfmp( m_ifmp ) ) );
    }

    DEBUG_SNAP_COMPARE();
    DebugCheckAll( );
}


ULONG CPAGE::CbContiguousDehydrateRequired_() const
{
    PGHDR * ppghdr = (PGHDR *)m_bfl.pv;
    return roundup( ( CbPageHeader() + ppghdr->ibMicFree + CbTagArray_() ), sizeof(USHORT) );
}

ULONG CPAGE::CbReorganizedDehydrateRequired_() const
{
    return roundup( ( m_platchManager->CbBuffer( m_bfl ) - CbFree_() ), sizeof(USHORT) );
}


BOOL CPAGE::FPageIsDehydratable( __out ULONG * pcbMinimumSize, __in const BOOL fAllowReorg ) const
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

    if ( ( CbContiguousBufferFree_() >= cbCommitGranularity ) &&
            ( ( CbPageHeader() + ppghdr->ibMicFree + CbTagArray_() ) <
                    ( m_platchManager->CbBuffer( m_bfl ) - ( 4 * 1024 ) ) )
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


    return ( *pcbMinimumSize < ( m_platchManager->CbBuffer( m_bfl ) - cbCommitGranularity ) );
}

VOID CPAGE::OptimizeInternalFragmentation()
{
#ifdef FDTC_0_REORG_DEHYDRATE_PLAN_B
    Assert( m_platchManager->FWriteLatch( m_bfl ) );

    const ULONG cbMinCurrSize = CbContiguousDehydrateRequired_();
    const ULONG cbMinReorgSize = CbReorganizedDehydrateRequired_();

    Assert( cbMinReorgSize <= cbMinCurrSize );


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
            DebugMoveMemory_();
        }
#endif
    }
#endif
}


VOID CPAGE::DehydratePageUnadjusted_( __in const ULONG cbNewSize )
{
    PGHDR * ppghdr = (PGHDR *)m_bfl.pv;

    Assert( FAssertWriteLatch() );
    Assert( cbNewSize < m_platchManager->CbBuffer( m_bfl ) || FNegTest( fInvalidUsage ) );

    OnDebug( ULONG cbMinReq );
    Assert( FPageIsDehydratable( &cbMinReq, fFalse ) && cbMinReq <= cbNewSize );

    OnNonRTM( PAGECHECKSUM pgchk = LoggedDataChecksum() );

    const ULONG cbShrinkage = m_platchManager->CbBuffer( m_bfl ) - cbNewSize;


    Assert( cbShrinkage > 0 || FNegTest( fInvalidUsage ) );
    Enforce( cbShrinkage < CbPage() );
    Enforce( cbShrinkage < 0x10000 );

    Enforce( ppghdr->cbFree >= cbShrinkage );
    Enforce( ppghdr->ibMicFree < ( m_platchManager->CbBuffer( m_bfl ) - cbShrinkage ) );

    Assert( ppghdr->ibMicFree < m_platchManager->CbBuffer( m_bfl ) - CbTagArray_() );



    void * pvTarget = (BYTE*)m_bfl.pv + cbNewSize - CbTagArray_();
    AssertRTL( ( (DWORD_PTR)pvTarget % 2 ) == 0 );

    memmove( pvTarget,
                (BYTE*)m_bfl.pv + m_platchManager->CbBuffer( m_bfl ) - CbTagArray_(),
                CbTagArray_() );


    ppghdr->cbFree -= (USHORT)cbShrinkage;


    m_platchManager->SetBufferSize( &m_bfl, cbNewSize );


    Assert( cbNewSize == m_platchManager->CbBuffer( m_bfl ) );
    Assert( !FIsNormalSized() );


    Assert( JET_errSuccess == ErrCheckPage( CPRINTFDBGOUT::PcprintfInstance(), OnErrorFireWall, CheckTagsNonOverlapping ) || FEmptyPage() || FPreInitPage() );
    Assert( pgchk == LoggedDataChecksum() );


    if ( !m_platchManager->FDirty( m_bfl ) )
    {
        SetPageChecksum( (BYTE *)m_bfl.pv, CbBuffer(), databasePage, m_pgno );
    }


    Assert( JET_errSuccess == ErrCheckPage( CPRINTFDBGOUT::PcprintfInstance(), OnErrorFireWall, CheckTagsNonOverlapping ) || FEmptyPage() || FPreInitPage() );

    AssertRTL( pgchk == LoggedDataChecksum() );

    ASSERT_VALID(this);
    DebugCheckAll();
}


VOID CPAGE::DehydratePage( __in const ULONG cbNewSize, __in const BOOL fAllowReorg )
{
    C_ASSERT( sizeof(PGHDR) == 40 );
    C_ASSERT( sizeof(PGHDR2) == 80 );

    Assert( FAssertWriteLatch() );
    Assert( cbNewSize < m_platchManager->CbBuffer( m_bfl ) || FNegTest( fInvalidUsage ) );

    OnNonRTM( PAGECHECKSUM pgchk = LoggedDataChecksum() );

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

    if ( cbNewSize < cbMinUnadjustedReq )
    {

        if ( !fAllowReorg )
        {
            FireWall( "DehydrateTooSmall" );

            return;
        }

        ReorganizeData_( reorgDehydrateBuffer );
    }

    AssertRTL( pgchk == LoggedDataChecksum() );
#endif

    if ( !FPageIsDehydratable( &cbMinUnadjustedReq, fFalse ) )
    {

        if ( !FNegTest( fInvalidUsage ) )
        {
            FireWall( "MustBeDehydrateble" );
        }

        return;
    }

#ifdef FDTC_0_REORG_DEHYDRATE
    Assert( cbMinUnadjustedReq <= cbMinReorganizeReq || cbNewSize >= cbMinUnadjustedReq );
#endif
    Assert( cbMinUnadjustedReq <= cbNewSize );

    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );

    DehydratePageUnadjusted_( cbNewSize );


    Assert( cbNewSize == m_platchManager->CbBuffer( m_bfl ) );
    Assert( !FIsNormalSized() );


    Assert( JET_errSuccess == ErrCheckPage( CPRINTFDBGOUT::PcprintfInstance(), OnErrorFireWall, CheckTagsNonOverlapping ) || FEmptyPage() || FPreInitPage() );

    FOSSetCleanupState( fCleanUpStateSaved );

    AssertRTL( pgchk == LoggedDataChecksum() );

    DebugCheckAll();
}

VOID CPAGE::RehydratePage()
{
    Assert( FAssertWriteLatch() );

    BYTE* const pvPage = (BYTE*)m_bfl.pv;
    PGHDR* ppghdr = (PGHDR*)pvPage;

    const ULONG cbBufferOrigSize = m_platchManager->CbBuffer( m_bfl );
    Assert( cbBufferOrigSize > 0 );
    Enforce( cbBufferOrigSize <= g_cbPageMax );

    const ULONG cbBufferNewSizeTarget = CbPage();
    Enforce( cbBufferNewSizeTarget <= g_cbPageMax );
    Assert( cbBufferNewSizeTarget >= cbBufferOrigSize );
    Expected( cbBufferNewSizeTarget > cbBufferOrigSize );

    OnNonRTM( PAGECHECKSUM pgchk = LoggedDataChecksum() );


    m_platchManager->SetBufferSize( &m_bfl, cbBufferNewSizeTarget );
    const ULONG cbBufferNewSize = m_platchManager->CbBuffer( m_bfl );
    Enforce( cbBufferNewSize == cbBufferNewSizeTarget );
    Assert( FIsNormalSized() );

    Enforce( cbBufferNewSize > cbBufferOrigSize );
    const ULONG cbGrowth = cbBufferNewSize - cbBufferOrigSize;


    const ULONG cbTagArray = CbTagArray_();
    const BYTE* const pvSource = pvPage + cbBufferOrigSize - cbTagArray;
    BYTE* const pvTarget = pvPage + cbBufferNewSize - cbTagArray;
    AssertRTL( ( (ULONG_PTR)pvTarget % 2 ) == 0 );
    Enforce( pvSource >= pvPage );
    Enforce( ( pvTarget + cbTagArray ) <= ( pvPage + cbBufferNewSize ) );

    memmove( pvTarget, pvSource, cbTagArray );

    Enforce( ( (ULONG)ppghdr->cbFree + cbGrowth ) <= g_cbPageMax );

    ppghdr->cbFree += (USHORT)cbGrowth;

    if ( FRuntimeScrubbingEnabled_() )
    {
        BYTE * const pbFree = PbFromIb_( ppghdr->ibMicFree );
        memset( pbFree, chPAGEReHydrateFill, CbContiguousBufferFree_() );
    }

    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );


    Assert( JET_errSuccess == ErrCheckPage( CPRINTFDBGOUT::PcprintfInstance(), OnErrorFireWall, CheckTagsNonOverlapping ) || FEmptyPage() || FPreInitPage() );
    Assert( pgchk == LoggedDataChecksum() );


    if ( !m_platchManager->FDirty( m_bfl ) )
    {
        SetPageChecksum( pvPage, CbPage(), databasePage, m_pgno );
    }


    Assert( JET_errSuccess == ErrCheckPage( CPRINTFDBGOUT::PcprintfInstance(), OnErrorFireWall, CheckTagsNonOverlapping ) || FEmptyPage() || FPreInitPage() );

    Assert( CbPage() == m_platchManager->CbBuffer( m_bfl ) );

    FOSSetCleanupState( fCleanUpStateSaved );

    AssertRTL( pgchk == LoggedDataChecksum() );

    ASSERT_VALID(this);
    DebugCheckAll();
}


INLINE VOID CPAGE::ZeroOutGaps_( const CHAR chZero )
{
    PGHDR * const ppghdr = (PGHDR*)m_bfl.pv;

    Assert( !FPreInitPage() );
    Assert( ppghdr->itagMicFree > 0 );
    Assert( 0 != ppghdr->cbFree );

    const BOOL fSmallFormat = FSmallPageFormat();


    BYTE *rgbBuf;
    BFAlloc( bfasTemporary, (VOID **)&rgbBuf );
    TAG ** rgptag = (TAG **)rgbBuf;


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

    sort( rgptag, rgptag + cptag, PfnCmpPtagIb() );

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
            (void) ErrCaptureCorruptedPageInfo( OnErrorEnforce, L"TagsOverlapOnZero" );
        }
    }

    BFFree( rgbBuf );
}

INLINE BOOL CPAGE::FRuntimeScrubbingEnabled_ ( ) const
{
#ifdef ENABLE_JET_UNIT_TEST
    if ( m_iRuntimeScrubbingEnabled != -1 )
    {
        return (BOOL)m_iRuntimeScrubbingEnabled;
    }
#endif

    return ( g_rgfmp && ( m_ifmp != ifmpNil ) && BoolParam( PinstFromIfmp( m_ifmp ), JET_paramZeroDatabaseUnusedSpace ) );
}

#ifdef ENABLE_JET_UNIT_TEST
VOID CPAGE::SetRuntimeScrubbingEnabled_( const BOOL fEnabled )
{
    m_iRuntimeScrubbingEnabled = fEnabled ? 1 : 0;
}
#endif

CPAGE::TAG::TAG() : cb_( 0 ), ib_( 0 )
{
}


VOID CPAGE::TAG::ErrTest( __in VOID * const pvBuffer, ULONG cbPageSize )
{
    PGHDR* ppgHdr = ( PGHDR* )pvBuffer;
    memset( ppgHdr, 0, sizeof( *ppgHdr ) );

    ppgHdr->itagMicFree = 2;
    ppgHdr->fFlags = 0;

    CPAGE cpage;
    cpage.LoadPage( pvBuffer, cbPageSize );
    ((CPAGE::PGHDR*)pvBuffer)->cbFree = sizeof(TAG)+1;
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
    AssertRTL( tag2.FFlags( &cpage, cpage.FSmallPageFormat() ) == 0x0007 );

    tag2.SetIb( &cpage, 0x123 );
    tag2.SetCb( &cpage, 0x456 );
    tag2.SetFlags( &cpage, 0x0002 );
    AssertRTL( tag2.Cb( cpage.FSmallPageFormat() ) == 0x456 );
    AssertRTL( tag2.Ib( cpage.FSmallPageFormat() ) == 0x123 );
    AssertRTL( tag2.FFlags( &cpage, cpage.FSmallPageFormat() ) == 0x0002 );
}


ERR CPAGE::ErrTest( __in const ULONG cbPageSize )
{
    AssertRTL( 0 == CbPage() % cchDumpAllocRow );

    VOID * const pvBuffer = PvOSMemoryHeapAlloc( cbPageSize );
    if( NULL == pvBuffer )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }
    TAG::ErrTest( pvBuffer, cbPageSize );
    OSMemoryHeapFree( pvBuffer );
    return JET_errSuccess;
}



ERR CPAGE::ErrGetPageHintCacheSize( ULONG_PTR* const pcbPageHintCache )
{
#ifdef MINIMAL_FUNCTIONALITY
    *pcbPageHintCache = 0;
#else
    *pcbPageHintCache = ( maskHintCache + 1 ) * sizeof( DWORD_PTR );
#endif
    return JET_errSuccess;
}

ERR CPAGE::ErrSetPageHintCacheSize( const ULONG_PTR cbPageHintCache )
{
#ifdef MINIMAL_FUNCTIONALITY
#else


    const SIZE_T cbPageHintCacheMin = 128;
    const SIZE_T cbPageHintCacheMax = cbHintCache;

    SIZE_T  cbPageHintCacheVal  = cbPageHintCache;
            cbPageHintCacheVal  = max( cbPageHintCacheVal, cbPageHintCacheMin );
            cbPageHintCacheVal  = min( cbPageHintCacheVal, cbPageHintCacheMax );


    SIZE_T cbPageHintCacheSet;
    for (   cbPageHintCacheSet = 1;
            cbPageHintCacheSet < cbPageHintCacheVal;
            cbPageHintCacheSet *= 2 );


    if (    maskHintCache < cbPageHintCacheSet / sizeof( DWORD_PTR ) - 1 ||
            maskHintCache > cbPageHintCacheSet / sizeof( DWORD_PTR ) * 2 - 1 )
    {
        maskHintCache = cbPageHintCacheSet / sizeof( DWORD_PTR ) - 1;
    }

#endif

    return JET_errSuccess;
}


ERR CPAGE::ErrInit()
{
    ERR err = JET_errSuccess;

#ifdef MINIMAL_FUNCTIONALITY
#else


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

#endif

    return err;
}

VOID CPAGE::Term()
{
#ifdef MINIMAL_FUNCTIONALITY
#else


    if ( rgdwHintCache )
    {
        OSMemoryHeapFree( (void*) rgdwHintCache );
        rgdwHintCache = NULL;
    }

#endif
}




ERR CPAGE::ErrEnumTags( CPAGE::PFNVISITNODE pfnEval, void * pvCtx ) const
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
    Assert( pbtsPageSpace->phistoDataSizes );

    if ( NULL == pkdf )
    {
        Assert( itag == 0 );
        Call( ErrFromCStatsErr( CStatsFromPv(pbtsPageSpace->phistoFreeBytes)->ErrAddSample( ppghdr->cbFree ) ) );
        Call( ErrFromCStatsErr( CStatsFromPv(pbtsPageSpace->phistoNodeCounts)->ErrAddSample( max( ppghdr->itagMicFree, 1 ) - 1 ) ) );
        err = JET_errSuccess;
        goto HandleError;
    }

    Assert( pkdf->fFlags == fNodeFlags );

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


VOID CPAGE::DumpAllocMap_( _TCHAR * rgchBuf, CPRINTF * pcprintf ) const
{
    INT     ich;
    INT     ichBase     = 0;
    INT     itag;
    INT     iptag       = 0;
    PGHDR * ppghdr      = (PGHDR*)m_bfl.pv;

    TAG * rgptagBuf[g_cbPageMax/sizeof(TAG)];
    TAG ** rgptag = rgptagBuf;

    for ( ich = 0; ich < CbPageHeader(); ++ich )
    {
        rgchBuf[ich+ichBase] = _T( 'H' );
    }
    ichBase = ich;

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

    sort( rgptag, rgptag + cptag, PfnCmpPtagIb() );

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

    ichBase = m_platchManager->CbBuffer( m_bfl );
    ichBase -= sizeof( CPAGE::TAG ) * ppghdr->itagMicFree;

    for ( ich = 0; ich < (INT)(sizeof( CPAGE::TAG ) * ppghdr->itagMicFree); ++ich )
    {
        rgchBuf[ich+ichBase] = _T( 'T' );
    }

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

ERR CPAGE::DumpAllocMap( CPRINTF * pcprintf ) const
{
    const INT   cchBuf      = CbPage();
    _TCHAR      rgchBuf[g_cbPageMax];

    for ( INT ich = 0; ich < cchBuf && ich < (sizeof(rgchBuf)/sizeof(rgchBuf[0])); ++ich )
    {
        rgchBuf[ich] = _T( '.' );
    }

#pragma prefast( pop )
    DumpAllocMap_( rgchBuf, pcprintf );

    return JET_errSuccess;
}

ERR CPAGE::DumpTags( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const
{
    const PGHDR * const ppghdr      = (PGHDR*)m_bfl.pv;

    Assert( Clines()+1 == ppghdr->itagMicFree );

    CMinMaxTotStats rgStats[6];
    BTREE_STATS_PAGE_SPACE btsPageSpace = {
            sizeof(BTREE_STATS_PAGE_SPACE),
            (JET_HISTO*)&rgStats[0],
            (JET_HISTO*)&rgStats[1],
            (JET_HISTO*)&rgStats[2],
            (JET_HISTO*)&rgStats[3],
            (JET_HISTO*)&rgStats[4],
            (JET_HISTO*)&rgStats[5]
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


VOID CPAGE::DumpTag( CPRINTF * pcprintf, const INT iline, const DWORD_PTR dwOffset ) const
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

extern USHORT g_rgcbExternalHeaderSize[];

extern INLINE BYTE BNDIGetPersistedNrfFlag( _In_range_(noderfSpaceHeader, noderfIsamAutoInc) NodeRootField noderf );

#pragma warning(disable:4293)
ERR CPAGE::DumpHeader( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const
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
                USHORT usExpectedTagSize = 1;
                BYTE fFlagT = 0x1;
                for ( INT i = 0; i < noderfMax; ++i, fFlagT <<= 1 )
                {
                    if ( fFlagT & fNodeFlag )
                    {
                        usExpectedTagSize += g_rgcbExternalHeaderSize[i+1];
                    }
                }
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

ERR CPAGE::ErrDumpToIrsRaw( _In_ PCWSTR wszReason, _In_ PCWSTR wszDetails ) const
{
    ERR      err;
    INST    *pinst = PinstFromIfmp( m_ifmp );
    __int64  fileTime;
    WCHAR    szDate[32];
    WCHAR    szTime[32];
    size_t   cchRequired;
    CPRINTF * pcprintfPageTrace = NULL;

    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );

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

    FOSSetCleanupState( fCleanUpStateSaved );

    return err;
}


#if defined( DEBUG ) || defined( ENABLE_JET_UNIT_TEST )

VOID CPAGE::CorruptHdr( _In_ const ULONG ipgfld, const QWORD qwToAdd )
{
    PGHDR *ppghdr = (PGHDR*)m_bfl.pv;
    switch( ipgfld )
    {
    case ipgfldCorruptItagMicFree:
        Assert( qwToAdd <= 0xFFFF );
        ppghdr->itagMicFree = ppghdr->itagMicFree + (USHORT)qwToAdd;
        break;
    default:
        AssertSz( fFalse, "NYI" );
    }
}
VOID CPAGE::CorruptTag( _In_ const ULONG itag, _In_ BOOL fCorruptCb , _In_ const USHORT usToAdd )
{
    Assert( itag < ((PGHDR*)m_bfl.pv)->itagMicFree );

    TAG * const ptag = PtagFromItag_( itag );
    if ( !fCorruptCb )
    {
        ptag->CorruptIb( usToAdd );
    }
    else
    {
        ptag->CorruptCb( usToAdd );
    }
}

#endif

#ifdef DEBUG

VOID CPAGE::AssertValid() const
{
    PGHDR *ppghdr = (PGHDR*)m_bfl.pv;

    if( 0 != ppghdr->checksum )
    {
        Assert( ppghdr->cbFree <= CbPageData() );
        Assert( ppghdr->cbUncommittedFree <= CbPageFree() );
        Assert( ppghdr->ibMicFree <= CbPageData() );
        Assert( FPreInitPage() || ( (USHORT) ppghdr->itagMicFree >= ctagReserved ) );
        Assert( (USHORT) ppghdr->itagMicFree <= ( CbPageData() - (USHORT) ppghdr->cbFree ) / static_cast<INT>( sizeof( CPAGE::TAG ) ) );
        Assert( CbContiguousFree_() <= CbPageFree() );
        Assert( CbContiguousBufferFree_() <= ppghdr->cbFree );
        Assert( CbContiguousFree_() >= 0 );
        Assert( FPreInitPage() || ( static_cast<VOID *>( PbFromIb_( ppghdr->ibMicFree ) ) <= static_cast<VOID *>( PtagFromItag_( ppghdr->itagMicFree - 1 ) ) ) );
    }
}


VOID CPAGE::DebugCheckAll_( ) const
{

    ASSERT_VALID( this );

    PGHDR *ppghdr = (PGHDR*)m_bfl.pv;
    INT cbTotal = 0;


#ifdef DEBUG_PAGE
#ifdef DEBUG_PAGE_EXTENSIVE
    const BOOL fFullTagWalk = fTrue;
#else
    const BOOL fFullTagWalk = fTrue;
#endif
#else
    const BOOL fFullTagWalk = ( ( rand() % 100 ) == 42 );
#endif
    if ( !fFullTagWalk )
    {
        return;
    }


#ifdef DEBUG_PAGE
#ifdef DEBUG_PAGE_EXTENSIVE
    const BOOL fFullNxNWalk = fTrue;
#else
    const BOOL fFullNxNWalk = ( ( rand() % 10000 ) == 42 );
#endif
#else
    const BOOL fFullNxNWalk = ( ( rand() % 100 ) == 42 );
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


        BOOL fSingleOverlapWalk = ( ( rand() % ( 1 + ppghdr->itagMicFree ) ) == 0 );
#ifdef DEBUG_PAGE
        if ( !fSingleOverlapWalk )
        {
            fSingleOverlapWalk = ( ( rand() % ( 1 + ppghdr->cbFree ) ) == 0 );
        }
#endif
        if ( !fSingleOverlapWalk && !fFullNxNWalk )
        {
            continue;
        }


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

    Assert( cbTotal + CbPageFree() + CbTagArray_() == (size_t)CbPageData() );
}


VOID CPAGE::DebugMoveMemory_( )
{
    BYTE    rgbBuf[g_cbPageMax];
    PGHDR *ppghdr = (PGHDR*)m_bfl.pv;
    INT     cbTag = 0;
    INT     fFlagsTag;

    if ( ppghdr->itagMicFree < ctagReserved + 2 )
    {
        return;
    }

    if ( CbContiguousFree_( ) == ppghdr->cbFree )
    {
        return;
    }

    const BOOL fSmallFormat = FSmallPageFormat();

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
        return;
    }
    Assert( itag >= ctagReserved && itag < (ppghdr->itagMicFree - 1) );
    Assert( cbTag > 0 );

    fFlagsTag = ptag->FFlags( this, fSmallFormat );
    AssertPREFIX( cbTag <= sizeof( rgbBuf ) );
    UtilMemCpy( rgbBuf, PbFromIb_( ptag->Ib( fSmallFormat ) ), cbTag );

    Delete_( itag );
    ReorganizeData_( reorgOther );
#pragma prefast( pop )

    DATA data;
    data.SetPv( rgbBuf );
    data.SetCb( cbTag );
    Insert_( itag, &data, 1, fFlagsTag );
}


BOOL CPAGE::FAssertDirty( ) const
{
    return m_platchManager->FDirty( m_bfl );
}


BOOL CPAGE::FAssertReadLatch( ) const
{
    return m_platchManager->FReadLatch( m_bfl );
}


BOOL CPAGE::FAssertRDWLatch( ) const
{
    return m_platchManager->FRDWLatch( m_bfl );
}


BOOL CPAGE::FAssertWriteLatch( ) const
{
    return m_platchManager->FWriteLatch( m_bfl );
}


BOOL CPAGE::FAssertWARLatch( ) const
{
    return m_platchManager->FWARLatch( m_bfl );
}


BOOL CPAGE::FAssertNotOnPage_( const VOID * pv ) const
{
    const BYTE * const pb = static_cast<const BYTE *>( pv );
    BOOL fGood =
        pb < reinterpret_cast<const BYTE * >( m_bfl.pv )
        || pb >= reinterpret_cast<const BYTE * >( m_bfl.pv ) + m_platchManager->CbBuffer( m_bfl )
        ;
    return fGood;
}

#endif

