// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#include "PageSizeClean.hxx"

/***********************************************************

The Patch Request List keeps track of outstanding patch requests for a
specific FMP.  A PRL entry is created when a page page record is logged
and is cleared when a page is dirtied.

Patch requests are synchronized through the buffer latch. When creating
a page the entries are synchronized in this way:

Latch Page (non-exclusive)
    Create PRL entry

When a page is dirtied the entry is cleared in this way:

Latch Page (exclusive)
    Dirty Page
        Clear PRL entry

Patching a page works like this:

Latch Page (exclusive)
    If patch request exists:
        If request is active:
            Patch Page
            Dirty Page
                Clear PRL entry

***********************************************************/

//  ================================================================
namespace PagePatching
//  ================================================================
{
    static const JET_TRACETAG g_tracetag = JET_tracetagOLD;
}

//  ================================================================
struct PatchRequestId
//  ================================================================
{
    IFMP    ifmp;
    PGNO    pgno;
    DBTIME  dbtime;
};

//  ================================================================
class PatchRequest
//  ================================================================
//
//  Describes an outstanding page patch request.
//
//-
{
public:
    // Create a new patch request, filling in the output parameter if the request is logged successfully
    static ERR ErrCreateRequest( const IFMP ifmp, const PGNO pgno, _Out_ PatchRequest * const prequest );

    // True if the request is currently active
    bool FRequestIsActive() const { return m_fRequestIsActive; }

    // True if the request is for a page associated with the instance
    bool FRequestIsForThisInst( const INST * const pinst ) const;
    
    // True if the request is for a page in the specified FMP
    bool FRequestIsForIfmp( const IFMP ifmp) const;

    // True if the request is for the specified ifmp:pgno
    bool FRequestIsForIfmpPgno( const IFMP ifmp, const PGNO pgno ) const;

    // How long (in seconds) this request has been pending for
    ULONG_PTR UlSecAgeOfRequest() const;

    // Patch the page if the request is active and the PatchRequestId matches
    ERR ErrPatchPage(
        const PatchRequestId id,
        BFLatch * const pbfl,
        __in_bcount(cb) const void * const pv,
        const INT cb );

    // Cancel this request. Subsequent calls to ErrPatchPage will fail.
    void CancelRequest();

    PatchRequest();
    ~PatchRequest();

protected:
    PatchRequest( const IFMP ifmp, const PGNO pgnoRequest );
    PatchRequest& operator=( PatchRequest& );
    
protected:
    IFMP Ifmp_() const { return m_ifmp; }
    INST * Pinst_() const { return PinstFromIfmp( Ifmp_() ); }
    LOG * Plog_() const { return Pinst_()->m_plog; }

    PGNO PgnoRequest_() const { return m_pgnoRequest; }
    DBTIME DbtimeOfPatchRequest_() const { return m_dbtimeOfPatchRequest; }
    PatchRequestId Id_() const;
    ULONG_PTR UlTimeWhenRequestWasLogged_() const { return m_ulTimeWhenRequestWasLogged; }
#if defined( USE_HAPUBLISH_API )
    #define LogFailureItem_( haTag, haCategory )    LogFailureItemI_( haTag, haCategory);
    void LogFailureItemI_(const HaDbFailureTag haTag, const HaDbIoErrorCategory haCategory) const;
#else
    #define LogFailureItem_( haTag, haCategory )
#endif

    ERR ErrStartPatch_();
    ERR ErrValidatePatchArguments_(
        const PatchRequestId id,
        const BFLatch * const pbfl,
        __in_bcount(cb) const void * const pv,
        const INT cb ) const;

    virtual DBTIME DbtimeGet_() { return g_rgfmp[ Ifmp_() ].DbtimeIncrementAndGet(); }
    virtual ERR ErrLogPatchRequest_();
    
private:
    static const JET_TRACETAG m_tracetag = PagePatching::g_tracetag;
    
    void Trace_( const char * const szFunction, const char * const szTrace ) const;
    void TraceErr_( const char * const szFunction, const ERR err ) const;

    void AssertValid() const;
    
    IFMP        m_ifmp;
    PGNO        m_pgnoRequest;
    DBTIME      m_dbtimeOfPatchRequest;
    ULONG_PTR   m_ulTimeWhenRequestWasLogged;
    bool        m_fRequestIsActive;

private:    // not implemented
    PatchRequest( const PatchRequest& );
};

//  ================================================================
namespace PagePatching
//  ================================================================
//
//  Functions that aren't exposed in the header
//
//-
{
    void IssueCorruptPageCallback( const IFMP ifmp, const PGNO pgno );
    ERR ErrRequestPatch( const IFMP ifmp, const PGNO pgno );
    ERR ErrCreateRequest( const INT iprl, const IFMP ifmp, const PGNO pgno );

    ERR ErrValidatePatchArguments(
        const IFMP                          ifmp,
        const ULONG                 pgno,
        __in_bcount(cbData) const void *    pvToken,
        const ULONG                 cbToken,
        __in_bcount(cbData) const void *    pvData,
        const ULONG                 cbData);

    PatchRequestId IdFromToken( const IFMP ifmp, const PGNO pgno, const PAGE_PATCH_TOKEN * const ptoken );

    void LogPatchEvent( const IFMP ifmp, const PGNO pgno );
    void LogFullPatchListEvent( const IFMP ifmp, const PGNO pgno );

    CCriticalSection g_critPRL( CLockBasicInfo( CSyncBasicInfo( szPRL ), rankPRL, 0 ) );
    
    const size_t g_crequests = 256;
    PatchRequest* g_rgrequests = NULL;
    bool g_fHasAtLeastOneRequest = false;
    volatile INT g_iprlMac = -1;

    INT IndexOfRequest( const IFMP ifmp, const PGNO pgno );
    bool FHasRequest( const IFMP ifmp, const PGNO pgno );
    bool FHasRequest_( const IFMP ifmp, const PGNO pgno );
    PatchRequest& RequestFor( const IFMP ifmp, const PGNO pgno );

    INT IndexOfFreeRequest();
    bool FPRLIsFull();

    void RecalcPatchRequestListState();

    DWORD
    RequestPagePatch_(
        VOID *      pParameter
        );

    void
    RequestPagePatchOnNewThread(
        IFMP        ifmp,
        LONG        pgno
        );

    // The g_rgrequests object is fairly large (8k) and should  be allocated
    // dynamically, to avoid allocation for clients that don't need it. This class
    // manages the lifetime of the array:
    // -ErrPRLInit() allocates the array, if necessary.
    // -The destructor frees the array, if necessary.
    class PagePatchingArrayLifetimeManager
    {
    public:
        ~PagePatchingArrayLifetimeManager()
        {
            AssertSz( !g_fDllUp, "This destructor is intended to be called only when the DLL is being unloaded." );

            delete[] g_rgrequests;
            g_rgrequests = nullptr;
        }

        ERR ErrAllocateGlobalArray()
        {
            Assert( g_critPRL.FOwner() );

            JET_ERR err = JET_errSuccess;
            if ( !g_rgrequests )
            {
                Alloc( g_rgrequests = new PatchRequest[ g_crequests ] );
            }
HandleError:
            return err;
        }

    private:
    } g_pagepatchingarraycleanup;

    //  ================================================================
    // It is legal to call ErrPRLInit() multiple times. The internal
    // page-patching array is allocated the first time an instance is
    // started with JET_paramEnableExternalAutoHealing already set.
    ERR ErrPRLInit(
        _In_ INST* pinst )
    //  ================================================================
    {
        ERR err = JET_errSuccess;

        ENTERCRITICALSECTION crit( &g_critPRL );
        if ( BoolParam( pinst, JET_paramEnableExternalAutoHealing ) )
        {
            Call( g_pagepatchingarraycleanup.ErrAllocateGlobalArray() );
        }
HandleError:
        return err;
    }

}


//  ================================================================
bool operator== ( const PatchRequestId& id1, const PatchRequestId& id2 )
//  ================================================================
{
    return (
        id1.ifmp == id2.ifmp
        && id1.pgno == id2.pgno
        && id1.dbtime == id2.dbtime );
}

//  ================================================================
bool operator!= ( const PatchRequestId& id1, const PatchRequestId& id2 )
//  ================================================================
{
    return !(id1 == id2);
}

//  ================================================================
ERR PatchRequest::ErrCreateRequest( const IFMP ifmp, const PGNO pgno, _Out_ PatchRequest * const prequest )
//  ================================================================
//
//  Create a new patch request. If the request creation succeeds then
//  *prequest will be filled in with the request.
//
//-
{
    ERR err;

    // Set the output to a null patch request
    *prequest = PatchRequest();
    
    PatchRequest request( ifmp, pgno );
    Call( request.ErrStartPatch_() );

    // The request has been logged, return it
    *prequest = request;

HandleError:
    return err;
}

//  ================================================================
PatchRequest::PatchRequest() :
//  ================================================================
    m_ifmp( ifmpNil ),
    m_pgnoRequest( pgnoNull ),
    m_dbtimeOfPatchRequest( dbtimeNil ),
    m_ulTimeWhenRequestWasLogged( 0 ),
    m_fRequestIsActive( false )
{
    ASSERT_VALID_RTL( this );
}

//  ================================================================
PatchRequest::PatchRequest( const IFMP ifmp, const PGNO pgnoRequest ) :
//  ================================================================
    m_ifmp( ifmp ),
    m_pgnoRequest( pgnoRequest ),
    m_dbtimeOfPatchRequest( dbtimeNil ),
    m_ulTimeWhenRequestWasLogged( 0 ),
    m_fRequestIsActive( false )
{
    ASSERT_VALID_RTL( this );
}

//  ================================================================
PatchRequest::~PatchRequest()
//  ================================================================
{
    CancelRequest();
}

//  ================================================================
PatchRequest& PatchRequest::operator=( PatchRequest& rhs)
//  ================================================================
{
    ASSERT_VALID_RTL( this );
    ASSERT_VALID_RTL( &rhs );
    if (this != &rhs)
    {
        m_ifmp                          = rhs.Ifmp_();
        m_pgnoRequest                   = rhs.PgnoRequest_();
        m_dbtimeOfPatchRequest          = rhs.DbtimeOfPatchRequest_();
        m_ulTimeWhenRequestWasLogged    = rhs.UlTimeWhenRequestWasLogged_();
        m_fRequestIsActive              = rhs.FRequestIsActive();

        rhs.m_fRequestIsActive = false;
    }
    ASSERT_VALID_RTL( this );
    return *this;
}

//  ================================================================
bool PatchRequest::FRequestIsForThisInst( const INST * const pinst ) const
//  ================================================================
{
    ASSERT_VALID_RTL( this );
    return FRequestIsActive() && Pinst_() == pinst;
}

//  ================================================================
bool PatchRequest::FRequestIsForIfmp( const IFMP ifmp) const
//  ================================================================
{
    ASSERT_VALID_RTL( this );
    return FRequestIsActive() && Ifmp_() == ifmp;
}

//  ================================================================
bool PatchRequest::FRequestIsForIfmpPgno( const IFMP ifmp, const PGNO pgno ) const
//  ================================================================
{
    ASSERT_VALID_RTL( this );
    return FRequestIsActive() && FRequestIsForIfmp( ifmp ) && PgnoRequest_() == pgno;
}

//  ================================================================
ULONG_PTR PatchRequest::UlSecAgeOfRequest() const
//  ================================================================
{
    ASSERT_VALID_RTL( this );
    const ULONG_PTR ulsec = UlUtilGetSeconds() - UlTimeWhenRequestWasLogged_();
    Assert( ulsec >= 0 );
    return ulsec;
}

//  ================================================================
ERR PatchRequest::ErrPatchPage(
    const PatchRequestId id,
    BFLatch * const pbfl,
    __in_bcount(cb) const void * const pv,
    const INT cb )
//  ================================================================
{
    ASSERT_VALID_RTL( this );
    ERR err;

    Assert( FBFWriteLatched( pbfl ) );

    Call( ErrValidatePatchArguments_( id, pbfl, pv, cb ) );

    if ( FRequestIsActive() )
    {
        Trace_( __FUNCTION__, "patching" );
        memcpy( pbfl->pv, pv, cb );
    }
    else
    {
        Trace_( __FUNCTION__, "request is not active ignoring patch" );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    m_fRequestIsActive = false;
    
    LogFailureItem_(HaDbFailureTagActivePagePatchCompleted, HaDbIoErrorNone);
    
HandleError:
    TraceErr_( __FUNCTION__, err );
    ASSERT_VALID_RTL( this );
    Assert( FBFWriteLatched( pbfl ) );
    return err;
}

//  ================================================================
void PatchRequest::CancelRequest()
//  ================================================================
{
    ASSERT_VALID_RTL( this );
    if ( FRequestIsActive() )
    {
        Trace_( __FUNCTION__, "cancelling" );
        m_fRequestIsActive = false;
    }
    ASSERT_VALID_RTL( this );
}

//  ================================================================
ERR PatchRequest::ErrStartPatch_()
//  ================================================================
//
//  To start the patch we log a patch request. Rolling the log after
//  logging the request means it can be answered faster.
//
//-
{
    ERR err;
    Trace_( __FUNCTION__, "starting" );
    
    m_dbtimeOfPatchRequest = DbtimeGet_();
    Call( ErrLogPatchRequest_() );

    m_ulTimeWhenRequestWasLogged = UlUtilGetSeconds();
    m_fRequestIsActive = true;

HandleError:
    TraceErr_( __FUNCTION__, err );
    return err;
}

#if defined( USE_HAPUBLISH_API )

//  ================================================================
void PatchRequest::LogFailureItemI_(const HaDbFailureTag haTag, const HaDbIoErrorCategory haCategory) const
//  ================================================================
{
    IFileAPI * const pfapi = g_rgfmp[ Ifmp_() ].Pfapi();
    WCHAR wszAbsPath[ IFileSystemAPI::cchPathMax ];
    CallS( pfapi->ErrPath( wszAbsPath ) );

    OSUHAPublishEvent(
        haTag,                                  // haTag
        Pinst_(),                               // pinst
        HA_GENERAL_CATEGORY,                    // dwComponent
        haCategory,                             // eseCategory
        wszAbsPath,                             // wszFilename
        OffsetOfPgno( PgnoRequest_() ),         // qwOffset
        g_rgfmp[Ifmp_()].CbPage(),              // cbSize
        0,                                      // dwEventId
        0,                                      // cParameter
        NULL );                                 // rgwszParameter
}

#endif

//  ================================================================
ERR PatchRequest::ErrLogPatchRequest_()
//  ================================================================
//
//  To start the patch we log a patch request. Rolling the log after
//  logging the request means it can be answered faster.
//
//-
{
    ERR err;
    Trace_( __FUNCTION__, "logging patch request" );
    
    Call( ErrLGPagePatchRequest( Plog_(), Ifmp_(), PgnoRequest_(), m_dbtimeOfPatchRequest ) );

    Call( ErrLGForceLogRollover( Pinst_(), __FUNCTION__ ) );

    LogFailureItem_(HaDbFailureTagActivePagePatchRequested, HaDbIoErrorRead);

HandleError:
    TraceErr_( __FUNCTION__, err );
    return err;
}

//  ================================================================
ERR PatchRequest::ErrValidatePatchArguments_(
        const PatchRequestId id,
        const BFLatch * const pbfl,
        __in_bcount(cb) const void * const pv,
        const INT cb ) const
//  ================================================================
{
    ERR err;
    CPAGE cpage;
    CPageValidationNullAction nullaction( pgvr::ActivePagePatch );

    if ( Id_() != id )
    {
        Trace_( __FUNCTION__, "incorrect id!" );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    
    if ( cb != g_rgfmp[ Ifmp_() ].CbPage() )
    {
        Trace_( __FUNCTION__, "buffer size is not page size!" );
        Error( ErrERRCheck( JET_errInvalidBufferSize ) );
    }

    cpage.LoadPage( Ifmp_(), PgnoRequest_(), const_cast<void *>(pv), cb );
    err = cpage.ErrValidatePage( pgvfExtensiveChecks | pgvfDoNotCheckForLostFlush, &nullaction );
    cpage.UnloadPage();
    if ( err < JET_errSuccess )
    {
        Trace_( __FUNCTION__, "CPAGE::ErrValidatePage failed!" );
    }
    Call( err );

HandleError:
    TraceErr_( __FUNCTION__, err );
    return err;
}

//  ================================================================
PatchRequestId PatchRequest::Id_() const
//  ================================================================
{
    const PatchRequestId id = { Ifmp_(), PgnoRequest_(), DbtimeOfPatchRequest_() };
    return id;
}

//  ================================================================
void PatchRequest::Trace_( const char * const szFunction, const char * const szTrace ) const
//  ================================================================
{
    OSTraceFMP( Ifmp_(), m_tracetag,
        OSFormat( "%s [%d:%d:%I64d]: %s", szFunction, (ULONG)Ifmp_(), PgnoRequest_(), DbtimeOfPatchRequest_(), szTrace ) );
}

//  ================================================================
void PatchRequest::TraceErr_( const char * const szFunction, const ERR err ) const
//  ================================================================
{
    if (err < JET_errSuccess )
    {
        OSTraceFMP( Ifmp_(), m_tracetag,
            OSFormat( "%s [%d:%d:%I64d]: error %d", szFunction, (ULONG)Ifmp_(), PgnoRequest_(), DbtimeOfPatchRequest_(), err ) );
    }
}

//  ================================================================
void PatchRequest::AssertValid() const
//  ================================================================
{
    AssertRTL( JET_tracetagNull != m_tracetag );
    AssertRTL( JET_tracetagMax != m_tracetag );

    if ( ifmpNil == m_ifmp )
    {
        // a null patch request
        AssertRTL( pgnoNull == m_pgnoRequest );
        AssertRTL( dbtimeNil == m_dbtimeOfPatchRequest );
        AssertRTL( 0 == m_ulTimeWhenRequestWasLogged );
        AssertRTL( false == m_fRequestIsActive );
    }
    else
    {
        Assert( pgnoNull != m_pgnoRequest );
        if ( m_fRequestIsActive )
        {
            AssertRTL( dbtimeNil != m_dbtimeOfPatchRequest );
            AssertRTL( 0 != m_ulTimeWhenRequestWasLogged );
            AssertRTL( m_ulTimeWhenRequestWasLogged <= UlUtilGetSeconds() );
        }
    }
}

//  ================================================================
INT PagePatching::IndexOfRequest( const IFMP ifmp, const PGNO pgno )
//  ================================================================
{
    Assert( g_critPRL.FOwner() );
    for (INT i = 0; i < g_crequests; ++i)
    {
        AssertSz( !g_rgrequests[i].FRequestIsActive() || i <= g_iprlMac, "There should be no active requests past g_iprlMac." );
        if ( g_rgrequests[i].FRequestIsForIfmpPgno( ifmp, pgno ) && g_rgrequests[i].FRequestIsActive() )
        {
            return i;
        }
    }
    return -1;
}

//  ================================================================
bool PagePatching::FHasRequest( const IFMP ifmp, const PGNO pgno )
//  ================================================================
{
    ENTERCRITICALSECTION crit( &g_critPRL );
    Assert( g_rgrequests );
    return FHasRequest_( ifmp, pgno );
}

//  ================================================================
bool PagePatching::FHasRequest_( const IFMP ifmp, const PGNO pgno )
//  ================================================================
{
    Assert( g_critPRL.FOwner() );
    return -1 != IndexOfRequest( ifmp, pgno );
}

//  ================================================================
PatchRequest& PagePatching::RequestFor( const IFMP ifmp, const PGNO pgno )
//  ================================================================
{
    Assert( g_critPRL.FOwner() );
    Assert( FHasRequest_( ifmp, pgno ) );
    return g_rgrequests[IndexOfRequest(ifmp, pgno)];
}

//  ================================================================
INT PagePatching::IndexOfFreeRequest()
//  ================================================================
{
    Assert( g_critPRL.FOwner() );
    for (INT i = 0; i < g_crequests; ++i)
    {
        AssertSz( !g_rgrequests[i].FRequestIsActive() || i <= g_iprlMac, "There should be no active requests past g_iprlMac." );
        if ( !g_rgrequests[i].FRequestIsActive() )
        {
            return i;
        }
    }
    return -1;
}

//  ================================================================
bool PagePatching::FPRLIsFull()
//  ================================================================
{
    Assert( g_critPRL.FOwner() );
    return -1 == IndexOfFreeRequest();
}

//  ================================================================
void PagePatching::RecalcPatchRequestListState()
//  ================================================================
{
    BOOL fFoundActiveRequest = fFalse;
    
    Assert( g_critPRL.FOwner() );

    const INT iprlBeforeExpected = g_iprlMac;
    
    for (INT i = g_crequests - 1; i >= 0; --i)
    {
        if ( g_rgrequests[i].FRequestIsActive() )
        {
            const INT iprlBefore = AtomicCompareExchange( (LONG*)&g_iprlMac, iprlBeforeExpected, i );
            AssertSz( iprlBeforeExpected == iprlBefore, "Unexpected concurrency detected on g_iprlMac." );
            fFoundActiveRequest = fTrue;
            break;
        }
    }

    if ( !fFoundActiveRequest )
    {
        const INT iprlBefore = AtomicCompareExchange( (LONG*)&g_iprlMac, iprlBeforeExpected, -1 );
        AssertSz( iprlBeforeExpected == iprlBefore, "Unexpected coucurrency detected on g_iprlMac." );
    }
    
    g_fHasAtLeastOneRequest = (g_iprlMac >= 0);
}

//  ================================================================
INLINE BOOL PagePatching::FIsPatchableError( const ERR err )
//  ================================================================
// 
// This determines the policy for correctable / patchable errors.
//
//-
{
    return FErrIsDbCorruption( err ) ||
            JET_errDiskIO == err;
}

//  ================================================================
void PagePatching::TryToRequestPatch( const IFMP ifmp, const PGNO pgno )
//  ================================================================
// 
// This can either log a patch request or call the corrupt page
// callback
//
//-
{
    AssertSz( !g_fRepair, "Page patching should never happen during repair/integrity check." );

    ERR err = JET_errSuccess;
    INST * const pinst = PinstFromIfmp( ifmp );
    LOG * const plog = pinst->m_plog;

    Assert( BoolParam( pinst, JET_paramEnableExternalAutoHealing ) );

    bool fRequestedPatch = false;

    // During runtime we can try logging a patch request
    if ( !plog->FLogDisabled()
        && !plog->FRecovering() )
    {
        err = PagePatching::ErrRequestPatch( ifmp, pgno );
        fRequestedPatch = (JET_errSuccess == err);
    }
    // During recovery we can try using the corrupt page callback
    else if ( plog->FRecovering() )
    {
        PagePatching::IssueCorruptPageCallback( ifmp, pgno );
        fRequestedPatch = true;
    }
    else
    {

        Assert( plog->FLogDisabled() );
        err = ErrERRCheck( JET_errLoggingDisabled );
    }

    if (!fRequestedPatch)
    {
        // We couldn't issue a patch request. Publish a failure event so that
        // HA can take action.
        Assert( JET_errSuccess != err );
        
        IFileAPI * const pfapi = g_rgfmp[ ifmp ].Pfapi();
        WCHAR wszAbsPath[ IFileSystemAPI::cchPathMax ];
        CallS( pfapi->ErrPath( wszAbsPath ) );

        WCHAR wszPgno[16];
        OSStrCbFormatW( wszPgno, sizeof( wszPgno ), L"%ld", pgno );
        WCHAR wszErr[16];
        OSStrCbFormatW( wszErr, sizeof( wszErr ), L"%ld", err );
        const WCHAR * rgcwszT[] = { wszAbsPath, wszPgno, wszErr } ;
        
        UtilReportEvent(
            eventError,
            DATABASE_CORRUPTION_CATEGORY,
            PATCH_REQUEST_ISSUE_FAILED,
            _countof( rgcwszT ),
            rgcwszT,
            0,
            NULL,
            pinst );

        OSUHAPublishEvent(
            HaDbFailureTagFailedToRepairActivePagePatch, // haTag
            pinst,                                  // pinst
            HA_GENERAL_CATEGORY,                    // dwComponent
            HaDbIoErrorMeta,                        // eseCategory
            wszAbsPath,                             // wszFilename
            OffsetOfPgno( pgno ),                   // qwOffset
            g_rgfmp[ifmp].CbPage(),                 // cbSize
            HA_PATCH_REQUEST_ISSUE_FAILED,          // dwEventId
            _countof( rgcwszT ),                    // cParameter
            rgcwszT );                              // rgwszParameter
    }
}

//  ================================================================
VOID PagePatching::TryPatchFromCopy( const IFMP ifmp, const PGNO pgno, VOID *pv, SHORT *perr )
//  ================================================================
//
//  Read from all storage space logical copies until we find a good page to patch with.
//
//-
{
    ERR err = JET_errSuccess;
    BYTE *pvPage = NULL;

    // Note: This pv,perr is actually a pointer to pbf->pv,pbf->err in the BF itself!
    // Ensure we have the page write latched.
    Assert( FBFWriteLatched( ifmp, pgno ) );

    ENTERCRITICALSECTION crit( &g_critPRL ); // Only do one patch at a time

    FMP *pfmp = &g_rgfmp[ ifmp ];
    INST *pinst = pfmp->Pinst();
    LOG *plog = pinst->m_plog;
    IFileAPI *pfapi = pfmp->Pfapi();
    TraceContextScope tcPatchPage( iortPagePatching );
    tcPatchPage->nParentObjectClass = tceNone;

    const BOOL fInRecoveryRedo = ( !plog->FLogDisabled() && ( fRecoveringRedo == plog->FRecoveringMode() ) );
    const BOOL fReplayingRequiredRange = fInRecoveryRedo && pfmp->FContainsDataFromFutureLogs();
    CPAGE cpage;
    const BOOL fFailOnRuntimeLostFlushOnly = ( ( UlParam( pinst, JET_paramPersistedLostFlushDetection ) & JET_bitPersistedLostFlushDetectionFailOnRuntimeOnly ) != 0 );
    const PAGEValidationFlags pgvf =
                pgvfFixErrors | pgvfExtensiveChecks |
                ( fFailOnRuntimeLostFlushOnly ? pgvfFailOnRuntimeLostFlushOnly : pgvfDefault ) |
                ( fReplayingRequiredRange ? pgvfDoNotCheckForLostFlush : pgvfDefault );
    CPageValidationLogEvent validationaction(
                pgvr::RaidCopySelfPatch,
                ifmp,
                CPageValidationLogEvent::LOG_NONE,
                LOGGING_RECOVERY_CATEGORY );

    LONG cCopies = pfapi->CLogicalCopies();

    BFAlloc( bfasTemporary, (VOID **)&pvPage );

    // Need to read and validate page with under latch, so a page from another copy does not get read successfully
    // and updated underneath us.
    for ( LONG iCopy = 0; iCopy < cCopies; iCopy++ )
    {
        err = pfapi->ErrIORead(
                    *tcPatchPage,
                    OffsetOfPgno( pgno ),
                    g_rgfmp[ ifmp ].CbPage(),
                    pvPage,
                    QosSyncDefault( pinst ) | QosOSFileReadCopy( iCopy ) );
        if ( err < JET_errSuccess )
        {
            continue;
        }
        cpage.LoadPage( ifmp, pgno, pvPage, g_rgfmp[ ifmp ].CbPage() );
        err = cpage.ErrValidatePage( pgvf, &validationaction );
        cpage.UnloadPage();
        if ( err >= JET_errSuccess )
        {
            break;
        }
    }

    // Patch BF's pv and err
    if ( err >= JET_errSuccess )
    {
        UtilMemCpy( pv, pvPage, g_rgfmp[ ifmp ].CbPage() );
        *perr = SHORT( err );
        Assert( *perr == err );

        LogPatchEvent( ifmp, pgno );
    }

    BFFree( pvPage );
}

//  ================================================================
void PagePatching::CancelPatchRequest( const IFMP ifmp, const PGNO pgno )
//  ================================================================
{
    AssertSz( !g_fRepair, "Page patching should never happen during repair/integrity check." );
    Assert( BoolParam( PinstFromIfmp( ifmp ), JET_paramEnableExternalAutoHealing ) );
    Assert( g_rgrequests );

    // Concurrency improvement: if there are no patch requests then don't enter the critical section
    // as all. If a patch request is being created it won't be for this page because the ifmp:pgno combination
    // is currently latched (asserted below).
    if( !g_fHasAtLeastOneRequest )
    {
        return;
    }

    INT prlMac = g_iprlMac;

#ifdef DEBUG
    for (INT i = 0; i < g_crequests; ++i)
#else // DEBUG
    for (INT i = 0; i <= prlMac; ++i)
#endif // !DEBUG
    {
        AssertSz( !g_rgrequests[i].FRequestIsActive() || i <= prlMac, "There should be no active requests past g_iprlMac." );
        if ( g_rgrequests[i].FRequestIsForIfmpPgno( ifmp, pgno ) )
        {
            // Since we are already page latched (verified with the assert above for
            // WAR or Write latch), we only need to lock when we are actually going
            // to cancel the request.
            ENTERCRITICALSECTION crit( &g_critPRL );
            g_rgrequests[i].CancelRequest();
            
            RecalcPatchRequestListState();
        }
    }
}

//  ================================================================
void PagePatching::TermFmp( const IFMP ifmp )
//  ================================================================
{
    if ( BoolParam( PinstFromIfmp( ifmp ), JET_paramEnableExternalAutoHealing ) )
    {
        ENTERCRITICALSECTION crit( &g_critPRL );

        Assert( g_rgrequests );

        for (INT i = 0; i < g_crequests; ++i)
        {
            AssertSz( !g_rgrequests[i].FRequestIsActive() || i <= g_iprlMac, "There should be no active requests past g_iprlMac." );
            if ( g_rgrequests[i].FRequestIsForIfmp( ifmp ) )
            {
                g_rgrequests[i].CancelRequest();
            }
        }
        RecalcPatchRequestListState();
    }
}

//  ================================================================
void PagePatching::TermInst( INST * const pinst )
//  ================================================================
{
    if ( BoolParam( pinst, JET_paramEnableExternalAutoHealing ) )
    {
        ENTERCRITICALSECTION crit( &g_critPRL );

        Assert( g_rgrequests );

        for (INT i = 0; i < g_crequests; ++i)
        {
            AssertSz( !g_rgrequests[i].FRequestIsActive() || i <= g_iprlMac, "There should be no active requests past g_iprlMac." );
            if ( g_rgrequests[i].FRequestIsForThisInst( pinst ) )
            {
                g_rgrequests[i].CancelRequest();
            }
        }
        RecalcPatchRequestListState();
    }
}

//  ================================================================
ERR PagePatching::ErrDoPatch(
    _In_ const IFMP                     ifmp,
    _In_ const PGNO                     pgno,
    __inout BFLatch * const             pbfl,
    __in_bcount(cbToken) const void *   pvToken,
    _In_ ULONG                          cbToken,
    __in_bcount(cbData) const void *    pvData,
    _In_ ULONG                          cbData,
    _Out_ BOOL *                        pfPatched )
//  ================================================================
{
    ERR err = JET_errSuccess;

    AssertSz( !g_fRepair, "Page patching should never happen during repair/integrity check." );
    Assert( BoolParam( PinstFromIfmp( ifmp ), JET_paramEnableExternalAutoHealing ) );
    Assert( FBFWriteLatched( pbfl ) );
    Assert( FBFDirty( pbfl ) == bfdfClean );
    Assert( g_rgrequests );

    *pfPatched = fFalse;

    Call( ErrValidatePatchArguments( ifmp, pgno, pvToken, cbToken, pvData, cbData ) );

    const PAGE_PATCH_TOKEN * const ptoken = (PAGE_PATCH_TOKEN *)pvToken;
    const INST * const pinst = PinstFromIfmp( ifmp );
    const LOG * const plog = pinst->m_plog;
    if ( 0 != memcmp( &(ptoken->signLog), &plog->SignLog(), sizeof(SIGNATURE) ) )
    {
        Error( ErrERRCheck( JET_errBadLogSignature ) );
    }

    err = ErrBFLatchStatus( pbfl );
    if( FIsPatchableError( err ) )
    {
        // We actually expect these errors, that is why we are patching the page in the first place.
        // If the failure is transient then it is also possible to get back JET_errSuccess. The page
        // will still be patched in that case.
        err = JET_errSuccess;
    }
    Call( err );

{
    ENTERCRITICALSECTION crit( &g_critPRL );
    if ( FHasRequest_( ifmp, pgno ) )
    {
        Call( RequestFor( ifmp, pgno ).ErrPatchPage( IdFromToken( ifmp, pgno, ptoken ), pbfl, pvData, cbData ) );

        Assert( !FHasRequest_( ifmp, pgno ) );
        *pfPatched = true;

        RecalcPatchRequestListState();
    }
}

    if ( *pfPatched )
    {
        BFDirty( pbfl, bfdfFilthy, *TraceContextScope( iorpNone, iorsNone, iortPagePatching ) );
        // status should have been reset here
        Assert( ErrBFLatchStatus( pbfl ) >= JET_errSuccess );   // the status should be OK now, so the page can be used.
        LogPatchEvent( ifmp, pgno );
    }

HandleError:
    return err;
}

//  ================================================================
ERR PagePatching::ErrCreateRequest( const INT iprl, const IFMP ifmp, const PGNO pgno )
//  ================================================================
{
    ERR err;
    
    Assert( g_critPRL.FOwner() );
    Assert( g_fHasAtLeastOneRequest );

    // We're either creating a new request or we re-issueing an existing request.
    Assert( !g_rgrequests[iprl].FRequestIsActive() || g_rgrequests[iprl].FRequestIsForIfmpPgno( ifmp, pgno ) );

    const INT iprlBeforeExpected = g_iprlMac;

    // If we're past g_iprlMac, we have to update it. We may not be over
    // since there can be gaps (patch requests not active) before.
    if ( iprl > iprlBeforeExpected )
    {
        const INT iprlBefore = AtomicCompareExchange( (LONG*)&g_iprlMac, iprlBeforeExpected, iprl );
        AssertSz( iprlBeforeExpected == iprlBefore, "Unexpected concurrency detected on g_iprlMac." );
    }

    err = PatchRequest::ErrCreateRequest( ifmp, pgno, &g_rgrequests[iprl] );
    AssertSz( err <= JET_errSuccess, "We should only fail or succeeed, no warnings should happen." );
    
    // If we failed to create the request, we need to put g_iprlMac and g_fHasAtLeastOneRequest back to
    // what it was.
    if ( err < JET_errSuccess )
    {
        RecalcPatchRequestListState();
    }

    return err;
}

//  ================================================================
ERR PagePatching::ErrRequestPatch(const IFMP ifmp, const PGNO pgno)
//  ================================================================
{
    if( BoolParam( PinstFromIfmp( ifmp ), JET_paramEnableExternalAutoHealing ) )
    {
        Assert( g_rgrequests );

        ENTERCRITICALSECTION crit( &g_critPRL );
        g_fHasAtLeastOneRequest = true;
        
        if ( FHasRequest_( ifmp, pgno ) )
        {
            const ULONG_PTR ulTimeout = UlParam( PinstFromIfmp( ifmp ), JET_paramPatchRequestTimeout );
            if ( RequestFor( ifmp, pgno ).UlSecAgeOfRequest() <= ulTimeout )
            {
                OSTraceFMP( ifmp, g_tracetag,
                    OSFormat(
                        "Not issuing patch request for [%d:%d]. The existing request is %d seconds old.",
                        (ULONG)ifmp,
                        pgno,
                        (ULONG)RequestFor( ifmp, pgno ).UlSecAgeOfRequest() ) );
                return JET_errSuccess;
            }

            OSTraceFMP( ifmp, g_tracetag,
                OSFormat(
                    "Reissuing patch request for [%d:%d]. The existing request is %d seconds old.",
                    (ULONG)ifmp,
                    pgno,
                    (ULONG)RequestFor( ifmp, pgno ).UlSecAgeOfRequest() ) );

            // May reset g_fHasAtLeastOneRequest if we fail to add a request.
            return ErrCreateRequest( IndexOfRequest(ifmp, pgno), ifmp, pgno );
        }
        else if ( FPRLIsFull() )
        {
            LogFullPatchListEvent( ifmp, pgno );
            return ErrERRCheck( errTooManyPatchRequests );
        }
        else
        {
            OSTraceFMP( ifmp, g_tracetag,
                OSFormat(
                    "Creating new patch request for [%d:%d].",
                    (ULONG)ifmp,
                    pgno ) );

            // May reset g_fHasAtLeastOneRequest if we fail to add a request.
            return ErrCreateRequest( IndexOfFreeRequest(), ifmp, pgno );
        }
    }
    return JET_errSuccess;
}

//  ================================================================
void PagePatching::IssueCorruptPageCallback( const IFMP ifmp, const PGNO pgno )
//  ================================================================
{
    OSTrace(
        g_tracetag,
        OSFormat( "Issuing corrupt page callback for ifmp:pgno %d:%d", (ULONG)ifmp, pgno ) );

    FMP * const pfmp = g_rgfmp + ifmp;
    INST * const pinst = PinstFromIfmp( ifmp );

    Assert( pinst->FRecovering() );

    // The corrupt page callback feature is only enabled if auto-healing is on
    // (other client might not be prepared for this new callback)
    if ( pinst->m_pfnInitCallback &&
        BoolParam( pinst, JET_paramEnableExternalAutoHealing ) )
    {
        JET_SNCORRUPTEDPAGE     snCorruptedPage;

        Assert( g_rgrequests );

        //  initialize base structs
        //
        memset( &snCorruptedPage, 0, sizeof(snCorruptedPage) );
        snCorruptedPage.cbStruct = sizeof(snCorruptedPage);

        //  setup the database identification for the corrupted page
        //
        snCorruptedPage.dbid = (JET_DBID) ifmp;
        // UNDONE: would it be better to have a buffer to pass
        // rather then the FMP member?
        snCorruptedPage.wszDatabase = pfmp->WszDatabaseName();
        FMP::EnterFMPPoolAsWriter();
        C_ASSERT( sizeof(snCorruptedPage.dbinfomisc) == sizeof(JET_DBINFOMISC7) );
        CallS( ErrDBSetUserDbHeaderInfo( pfmp, sizeof(snCorruptedPage.dbinfomisc), &snCorruptedPage.dbinfomisc ) );
        FMP::LeaveFMPPoolAsWriter();

        //  identify the problematic page
        //
        snCorruptedPage.pageNumber = pgno;

        const JET_SNP   snp = JET_snpExternalAutoHealing;
        const JET_SNT   snt = JET_sntCorruptedPage;
        void * const    pv = &snCorruptedPage;

        // The error from this is ignored because the error can be hit on
        // a non-recovery thread so we have no easy way to error out.
        (void)pinst->m_pfnInitCallback( snp, snt, pv, pinst->m_pvInitCallbackContext );
    }
}

//  ================================================================
ERR PagePatching::ErrValidatePatchArguments(
    const IFMP                          ifmp,
    const ULONG                 pgno,
    __in_bcount(cbData) const void *    pvToken,
    const ULONG                 cbToken,
    __in_bcount(cbData) const void *    pvData,
    const ULONG                 cbData)
//  ================================================================
{
    ERR err = JET_errSuccess;

    if ( ifmpNil == ifmp )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    
    if ( pgnoNull == pgno )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( NULL == pvData )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( NULL == pvToken )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( sizeof(PAGE_PATCH_TOKEN) != cbToken )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    const PAGE_PATCH_TOKEN * const ptoken = (PAGE_PATCH_TOKEN *)pvToken;
    if ( sizeof(PAGE_PATCH_TOKEN) != ptoken->cbStruct )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

HandleError:
    return err;
}

//  ================================================================
PatchRequestId PagePatching::IdFromToken( const IFMP ifmp, const PGNO pgno, const PAGE_PATCH_TOKEN * const ptoken )
//  ================================================================
{
    Assert( ptoken );
    Assert( sizeof(PAGE_PATCH_TOKEN) == ptoken->cbStruct );

    PatchRequestId id;
    id.ifmp = ifmp;
    id.pgno = pgno;
    id.dbtime = ptoken->dbtime;

    return id;
}

//  ================================================================
void PagePatching::LogPatchEvent( const IFMP ifmp, const PGNO pgno )
//  ================================================================
{
    INST * const pinst = PinstFromIfmp( ifmp );
    
    const WCHAR *   rgcwszT[2];
    INT             isz = 0;

    WCHAR wszPgno[64];
    OSStrCbFormatW( wszPgno, sizeof( wszPgno ), L"%u (0x%08x)", pgno, pgno );
    
    rgcwszT[isz++] = wszPgno;
    rgcwszT[isz++] = g_rgfmp[ifmp].WszDatabaseName();
    
    Assert( isz == _countof(rgcwszT) );
    
    UtilReportEvent(
        eventWarning,
        DATABASE_CORRUPTION_CATEGORY,
        PAGE_PATCHED_ID,
        isz,
        rgcwszT,
        0,
        NULL,
        pinst );
}

//  ================================================================
void PagePatching::LogFullPatchListEvent( const IFMP ifmp, const PGNO pgno )
//  ================================================================
{
    OSTraceFMP( ifmp, g_tracetag,
        OSFormat(
            "Unable to log patch request for [%d:%d] because the patch request list has hit its maximum size of %d entries",
            (ULONG)ifmp,
            pgno,
            (ULONG)g_crequests ) );
}

struct PATCH_DATA
{
    IFMP        ifmp;
    LONG        pgno;
};

DWORD
PagePatching::RequestPagePatch_(
    VOID *      pParameter
    )
{
    PATCH_DATA *pPatchData = (PATCH_DATA *)pParameter;
    TraceContextScope tcPatchPage( iortPagePatching );
    tcPatchPage->nParentObjectClass = tceNone;  //  cannot know the TCE of the page

    Assert( BoolParam( PinstFromIfmp( pPatchData->ifmp ), JET_paramEnableExternalAutoHealing ) );

    //
    // Note that it is possible that BF has a clean page in memory
    // from before the corruption (and that page may be modified),
    // in that case, we need to get that page rather than issue a
    // patch request
    //
    BFLatch bfl;
    ERR errLatch = ErrBFWriteLatchPage( &bfl, pPatchData->ifmp, pPatchData->pgno, BFLatchFlags( bflfUninitPageOk | bflfNoFaultFail ), BfpriBackgroundRead( pPatchData->ifmp, ppibNil ), *tcPatchPage );
    if ( errLatch >= JET_errSuccess )
    {
        //
        // If the in-memory page was corrupted, BF already issued
        // patch request, else make sure that the clean page
        // gets written to disk ASAP
        //
        if ( ErrBFLatchStatus( &bfl ) >= JET_errSuccess )
        {
            BFDirty( &bfl, bfdfFilthy, *tcPatchPage );
        }
        else
        {
            Assert( !PagePatching::FIsPatchableError( ErrBFLatchStatus( &bfl ) ) ||
                    PinstFromIfmp( pPatchData->ifmp )->FRecovering() ||
                    PagePatching::FHasRequest( pPatchData->ifmp, pPatchData->pgno ) );
        }

        BFWriteUnlatch( &bfl );
    }

    delete pPatchData;
    return 0;
}

void
PagePatching::RequestPagePatchOnNewThread(
    IFMP        ifmp,
    LONG        pgno
    )
{
    ERR err;
    PATCH_DATA *pPatchData;

    Alloc( pPatchData = new PATCH_DATA );
    pPatchData->ifmp = ifmp;
    pPatchData->pgno = pgno;

    Call( PinstFromIfmp( ifmp )->Taskmgr().ErrTMPost(
                    RequestPagePatch_,
                    pPatchData ) );
    // pPatchData now owned by the task
    pPatchData = NULL;

HandleError:
    delete pPatchData;
}

#ifdef ENABLE_JET_UNIT_TEST

//  ================================================================
JETUNITTEST( PatchRequestId, OperatorEqualsReturnsTrueWhenAllMembersAreEqual )
//  ================================================================
{
    const PatchRequestId id1 = { 1, 2, 3 };
    const PatchRequestId id2 = { 1, 2, 3 };
    CHECK(id1 == id2);
}

//  ================================================================
JETUNITTEST( PatchRequestId, OperatorEqualsReturnsFalseWhenIfmpIsNotEqual )
//  ================================================================
{
    const PatchRequestId id1 = { 1, 2, 3 };
    const PatchRequestId id2 = { 0, 2, 3 };
    CHECK(id1 != id2);
}

//  ================================================================
JETUNITTEST( PatchRequestId, OperatorEqualsReturnsFalseWhenPgnoIsNotEqual )
//  ================================================================
{
    const PatchRequestId id1 = { 1, 2, 3 };
    const PatchRequestId id2 = { 1, 0, 3 };
    CHECK(id1 != id2);
}

//  ================================================================
JETUNITTEST( PatchRequestId, OperatorEqualsReturnsFalseWhenDbtimeIsNotEqual )
//  ================================================================
{
    const PatchRequestId id1 = { 1, 2, 3 };
    const PatchRequestId id2 = { 1, 2, 0 };
    CHECK(id1 != id2);
}

//  ================================================================
JETUNITTEST( PagePatching, IdFromToken )
//  ================================================================
{
    PAGE_PATCH_TOKEN token;
    token.cbStruct = sizeof( token );
    token.dbtime = 1234;

    PatchRequestId idExpected;
    idExpected.ifmp = 1;
    idExpected.pgno = 2;
    idExpected.dbtime = token.dbtime;
    
    const PatchRequestId idActual = PagePatching::IdFromToken( idExpected.ifmp, idExpected.pgno, &token );

    CHECK(idExpected == idActual);
}

//  ================================================================
class PatchArgumentsTestFixture : public JetTestFixture
//  ================================================================
{
    private:
        IFMP m_ifmp;
        PGNO m_pgno;
        PAGE_PATCH_TOKEN m_token;
        BYTE m_rgb[4096];

    public:
        PatchArgumentsTestFixture() {}
        ~PatchArgumentsTestFixture() {}
        
    protected:
    
        bool SetUp_()
        {
            m_ifmp = 1;
            m_pgno = 99;
            m_token.cbStruct = sizeof( m_token );
            m_token.dbtime = 1234;
            SIGGetSignature( &m_token.signLog );
            return true;
        }

        void TearDown_() {}
        
    public:

        void ValidatePassesWhenArgumentsAreCorrect()
        {
            CHECK(JET_errSuccess ==
                PagePatching::ErrValidatePatchArguments(
                    m_ifmp,
                    m_pgno,
                    &m_token,
                    sizeof( m_token ),
                    m_rgb,
                    sizeof( m_rgb ) ) );
        }

        void ValidateFailsWhenIfmpIsNil()
        {
            CHECK(JET_errInvalidParameter ==
                PagePatching::ErrValidatePatchArguments(
                    ifmpNil,
                    m_pgno,
                    &m_token,
                    sizeof( m_token ),
                    m_rgb,
                    sizeof( m_rgb ) ) );
        }

        void ValidateFailsWhenPgnoIsNull()
        {
            CHECK(JET_errInvalidParameter ==
                PagePatching::ErrValidatePatchArguments(
                    m_ifmp,
                    pgnoNull,
                    &m_token,
                    sizeof( m_token ),
                    m_rgb,
                    sizeof( m_rgb ) ) );
        }

        void ValidateFailsWhenTokenIsNull()
        {
            CHECK(JET_errInvalidParameter ==
                PagePatching::ErrValidatePatchArguments(
                    m_ifmp,
                    m_pgno,
                    NULL,
                    sizeof( m_token ),
                    m_rgb,
                    sizeof( m_rgb ) ) );
        }

        void ValidateFailsWhenTokenSizeIsWrong()
        {
            CHECK(JET_errInvalidParameter ==
                PagePatching::ErrValidatePatchArguments(
                    m_ifmp,
                    m_pgno,
                    &m_token,
                    sizeof( m_token ) + 1,
                    m_rgb,
                    sizeof( m_rgb ) ) );
        }

        void ValidateFailsWhenTokenCbStructIsWrong()
        {
            m_token.cbStruct--;
            CHECK(JET_errInvalidParameter ==
                PagePatching::ErrValidatePatchArguments(
                    m_ifmp,
                    m_pgno,
                    &m_token,
                    sizeof( m_token ),
                    m_rgb,
                    sizeof( m_rgb ) ) );
        }

        void ValidateFailsWhenDataIsNull()
        {
            CHECK(JET_errInvalidParameter ==
                PagePatching::ErrValidatePatchArguments(
                    m_ifmp,
                    m_pgno,
                    &m_token,
                    sizeof( m_token ),
                    NULL,
                    sizeof( m_rgb ) ) );
        }
};

static const JetTestCaller<PatchArgumentsTestFixture> patf1(
    "PagePatching.ValidatePassesWhenArgumentsAreCorrect",
    &PatchArgumentsTestFixture::ValidatePassesWhenArgumentsAreCorrect);

static const JetTestCaller<PatchArgumentsTestFixture> patf2(
    "PagePatching.ValidateFailsWhenIfmpIsNil",
    &PatchArgumentsTestFixture::ValidateFailsWhenIfmpIsNil);

static const JetTestCaller<PatchArgumentsTestFixture> patf3(
    "PagePatching.ValidateFailsWhenPgnoIsNull",
    &PatchArgumentsTestFixture::ValidateFailsWhenPgnoIsNull);

static const JetTestCaller<PatchArgumentsTestFixture> patf4(
    "PagePatching.ValidateFailsWhenTokenIsNull",
    &PatchArgumentsTestFixture::ValidateFailsWhenTokenIsNull);

static const JetTestCaller<PatchArgumentsTestFixture> patf5(
    "PagePatching.ValidateFailsWhenTokenSizeIsWrong",
    &PatchArgumentsTestFixture::ValidateFailsWhenTokenSizeIsWrong);

static const JetTestCaller<PatchArgumentsTestFixture> patf6(
    "PagePatching.ValidateFailsWhenTokenCbStructIsWrong",
    &PatchArgumentsTestFixture::ValidateFailsWhenTokenCbStructIsWrong);

static const JetTestCaller<PatchArgumentsTestFixture> patf7(
    "PagePatching.ValidateFailsWhenDataIsNull",
    &PatchArgumentsTestFixture::ValidateFailsWhenDataIsNull);

#endif // ENABLE_JET_UNIT_TEST

//  ================================================================
class PatchRequest_FTO : public PatchRequest
//  ================================================================
{
public:
    static void CreateAndStart_FTO( const IFMP ifmp, const PGNO pgno, PatchRequest_FTO * const prequest )
    {
        PatchRequest_FTO request( ifmp, pgno );
        CallS( request.ErrStartPatch_() );
        *prequest = request;
    }

    PatchRequest_FTO() : PatchRequest() {}
    PatchRequest_FTO( const IFMP ifmp, const PGNO pgnoRequest ) : PatchRequest(ifmp, pgnoRequest) {}
    PatchRequestId Id_FTO() const { return Id_(); }

    DBTIME DbtimeOfRequest() const { return 0x23456; }

    PatchRequest_FTO& operator= (PatchRequest_FTO& rhs) { PatchRequest::operator=( rhs ); return *this; }

protected:
    virtual DBTIME DbtimeGet_() { return DbtimeOfRequest(); }
    virtual ERR ErrLogPatchRequest_() { return JET_errSuccess; }
    virtual ERR ErrPatchPage_( __in_bcount(cb) const void * const pv, const INT cb ) { return JET_errSuccess; }
};


#ifdef ENABLE_JET_UNIT_TEST

//  ================================================================
JETUNITTEST( PatchRequest, ConstructorCreatesInactiveRequest )
//  ================================================================
{
    PatchRequest_FTO request(1, 567);
    CHECK(false == request.FRequestIsActive());
}

//  ================================================================
JETUNITTEST( PatchRequest, CreateAndStartReturnsActiveRequest )
//  ================================================================
{
    PatchRequest_FTO request;
    PatchRequest_FTO::CreateAndStart_FTO(1, 32, &request);
    CHECK(true == request.FRequestIsActive());
}

//  ================================================================
JETUNITTEST( PatchRequest, FRequestIsForInstReturnsFalseWhenInactive )
//  ================================================================
{
    PatchRequest_FTO request(1, 567);
    CHECK(false == request.FRequestIsForThisInst((INST *)0xBADF00D));
}

//  ================================================================
JETUNITTEST( PatchRequest, FRequestIsForIfmpReturnsTrueWhenIfmpMatches )
//  ================================================================
{
    PatchRequest_FTO request;
    PatchRequest_FTO::CreateAndStart_FTO(1, 32, &request);
    CHECK(true == request.FRequestIsForIfmp(1));
}

//  ================================================================
JETUNITTEST( PatchRequest, FRequestIsForIfmpReturnsFalseWhenIfmpDoesNotMatch )
//  ================================================================
{
    PatchRequest_FTO request;
    PatchRequest_FTO::CreateAndStart_FTO(1, 32, &request);
    CHECK(false == request.FRequestIsForIfmp(2));
}

//  ================================================================
JETUNITTEST( PatchRequest, FRequestIsForIfmpReturnsFalseWhenInactive )
//  ================================================================
{
    PatchRequest_FTO request(1, 567);
    CHECK(false == request.FRequestIsForIfmp(1));
}

//  ================================================================
JETUNITTEST( PatchRequest, FRequestIsForIfmpPgnoReturnsTrueWhenIfmpAndPgnoBothMatch )
//  ================================================================
{
    PatchRequest_FTO request;
    PatchRequest_FTO::CreateAndStart_FTO(1, 567, &request);
    CHECK(true == request.FRequestIsForIfmpPgno(1, 567));
}

//  ================================================================
JETUNITTEST( PatchRequest, FRequestIsForIfmpPgnoReturnsFalseWhenIfmpDoesNotMatch )
//  ================================================================
{
    PatchRequest_FTO request;
    PatchRequest_FTO::CreateAndStart_FTO(1, 567, &request);
    CHECK(false == request.FRequestIsForIfmpPgno(2, 567));
}

//  ================================================================
JETUNITTEST( PatchRequest, FRequestIsForIfmpPgnoReturnsFalseWhenPgnoDoesNotMatch )
//  ================================================================
{
    PatchRequest_FTO request;
    PatchRequest_FTO::CreateAndStart_FTO(1, 567, &request);
    CHECK(false == request.FRequestIsForIfmpPgno(1, 566));
}

//  ================================================================
JETUNITTEST( PatchRequest, FRequestIsForIfmpPgnoReturnsFalseWhenInactive )
//  ================================================================
{
    PatchRequest_FTO request(1, 567);
    CHECK(false == request.FRequestIsForIfmpPgno(1, 567));
}

//  ================================================================
JETUNITTEST( PatchRequest, CancelDeactivatesRequest )
//  ================================================================
{
    PatchRequest_FTO request;
    PatchRequest_FTO::CreateAndStart_FTO(1, 32, &request);
    request.CancelRequest();
    CHECK(false == request.FRequestIsActive());
}

//  ================================================================
JETUNITTEST( PatchRequest, AssignmentDuplicatesRequest )
//  ================================================================
{
    PatchRequest_FTO request;
    PatchRequest_FTO request2;
    PatchRequest_FTO::CreateAndStart_FTO(1, 32, &request);
    request2 = request;
    CHECK(request.Id_FTO() == request2.Id_FTO());
}

//  ================================================================
JETUNITTEST( PatchRequest, CopyConstructorDeactivatesOriginalRequest )
//  ================================================================
{
    PatchRequest_FTO request;
    PatchRequest_FTO request2;
    PatchRequest_FTO::CreateAndStart_FTO(1, 32, &request);
    request2 = request;
    CHECK(false == request.FRequestIsActive());
    CHECK(true == request2.FRequestIsActive());
}

//  ================================================================
JETUNITTEST( PatchRequest, CancelTwice )
//  ================================================================
{
    PatchRequest_FTO request;
    PatchRequest_FTO::CreateAndStart_FTO(1, 32, &request);
    request.CancelRequest();
    request.CancelRequest();
    CHECK(false == request.FRequestIsActive());
}

//  ================================================================
JETUNITTEST( PatchRequest, AgeOfRequestIsReasonable )
//  ================================================================
{
    PatchRequest_FTO request;
    PatchRequest_FTO::CreateAndStart_FTO(1, 32, &request);
    CHECK(request.UlSecAgeOfRequest() < 10);
}

//  ================================================================
JETUNITTEST( PatchRequest, IdReturnsRequestId )
//  ================================================================
{
    PatchRequest_FTO request;
    PatchRequest_FTO::CreateAndStart_FTO(1, 32, &request);

    PatchRequestId idExpected;
    idExpected.ifmp = 1;
    idExpected.pgno = 32;
    idExpected.dbtime = request.DbtimeOfRequest();

    PatchRequestId idActual = request.Id_FTO();
    CHECK(idActual == idExpected);
}

#endif // ENABLE_JET_UNIT_TEST

