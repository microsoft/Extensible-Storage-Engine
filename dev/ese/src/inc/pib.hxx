// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

const DWORD_PTR dwPIBSessionContextNull     = 0;
const DWORD_PTR dwPIBSessionContextUnusable = DWORD_PTR( ~0 );

const DWORD cchContextStringSize = 32;


class TrxidStack
{
public:
    TrxidStack();
    ~TrxidStack();

    void Push( const TRXID trxid );

    void Pop();

    void Clear();

    ERR ErrDump( __out_ecount_z( cchBuf ) PWSTR const szBuf, const size_t cchBuf, const WCHAR * const szLineBreak = L"\r\n" ) const;

private:
    static const INT m_ctrxMax = levelMax+1;
    
    TRXID m_rgtrxid[m_ctrxMax];
    
    __int64 m_rgtrxtime[m_ctrxMax];

    INT m_ctrxCurr;

private:
    TrxidStack( const TrxidStack& );
    TrxidStack& operator=( const TrxidStack& );
};

class PIB;

class PIBGetEtcFunctor
{
    const PIB* m_ppib;

public:
    PIBGetEtcFunctor( const PIB* ppib ) : m_ppib( ppib )
    {}

    TraceContext* operator()() const;
};

class PIBTraceContextScope : public _TraceContextScope< PIBGetEtcFunctor >
{
public:
    PIBTraceContextScope( const PIB* ppib ) : _TraceContextScope( PIBGetEtcFunctor{ ppib } )
    {}

    PIBTraceContextScope( PIB* ppib, IOREASONPRIMARY iorp, IOREASONSECONDARY iors = iorsNone, IOREASONTERTIARY iort = iortNone, IOREASONFLAGS iorf = iorfNone )
        : _TraceContextScope( PIBGetEtcFunctor{ ppib }, iorp, iors, iort, iorf )
    {}

    PIBTraceContextScope( const PIB* ppib, IOREASONSECONDARY iors, IOREASONTERTIARY iort = iortNone, IOREASONFLAGS iorf = iorfNone )
        : _TraceContextScope( PIBGetEtcFunctor{ ppib }, iors, iort, iorf )
    {}

    PIBTraceContextScope( const PIB* ppib, IOREASONTERTIARY iort, IOREASONFLAGS iorf = iorfNone )
        : _TraceContextScope( PIBGetEtcFunctor{ ppib }, iort, iorf )
    {}

    PIBTraceContextScope( PIBTraceContextScope&& rvalue ) : _TraceContextScope( std::move( rvalue ) )
    {}
};

class PIB
    :   public CZeroInit
{
private:
    class MACRO
    {
        private:
            DBTIME      m_dbtime;
            BYTE        *m_rgbLogrec;
            size_t      m_cbLogrecMac;
            size_t      m_ibLogrecAvail;
            MACRO       *m_pMacroNext;

        public:
            DBTIME      Dbtime()        const           { return m_dbtime; }
            VOID        SetDbtime( DBTIME dbtime )      { m_dbtime = dbtime; }

            MACRO       *PMacroNext()   const           { return m_pMacroNext; }
            VOID        SetMacroNext( MACRO *pMacro )   { m_pMacroNext = pMacro; }

            ERR         ErrInsertLogrec( const VOID * pv, const ULONG cb );
            VOID        ResetBuffer();
            VOID        ReleaseBuffer();
            VOID        *PvLogrec()     const           { return m_rgbLogrec; }
            size_t      CbSizeLogrec()  const           { return m_ibLogrecAvail; }
    };

public:
    PIB( void )
        :   CZeroInit( sizeof( PIB ) ),
            critCursors( CLockBasicInfo( CSyncBasicInfo( szPIBCursors ), rankPIBCursors, 0 ) ),
            critConcurrentDDL( CLockBasicInfo( CSyncBasicInfo( szPIBConcurrentDDL ), rankPIBConcurrentDDL, 0 ) ),
            critLogBeginTrx( CLockBasicInfo( CSyncBasicInfo( szPIBLogBeginTrx ), rankPIBLogBeginTrx, 0 ) ),
            ptlsApi( NULL ),
            ptlsTrxBeginLast( Ptls() ),
            asigWaitLogWrite( CSyncBasicInfo( _T( "PIB::asigWaitLogWrite" ) ) ),
            m_level( 0 ),
            m_critCachePriority( CLockBasicInfo( CSyncBasicInfo( szPIBCachePriority ), rankPIBCachePriority, 0 ) ),
            m_pctCachePriority( g_pctCachePriorityUnassigned ),
            m_fCommitContextContainsCustomerData( fTrue )
    {
        for ( DBID dbid = 0; dbid < _countof( m_rgpctCachePriority ); dbid++ )
        {
            m_rgpctCachePriority[ dbid ] = g_pctCachePriorityUnassigned;
        }

        m_utc.context.dwUserID = OCUSER_UNINIT;
    }

    ~PIB()
    {
        if ( &m_utc == PutcTLSGetUserContext() )
        {
            FireWall( "PibUtcCleanup" );
            TLSSetUserTraceContext( NULL );
        }
    }

    friend ERR ErrPIBBeginSession( INST *pinst, _Outptr_ PIB **pppib, PROCID procidTarget, BOOL fForCreateTempDatabase );
    friend VOID PIBEndSession( PIB *ppib );
    void PIBGetSessionInfoV1( _Out_ JET_SESSIONINFO * psessinfo ) const
    {
        psessinfo->ulTrxBegin0 = trxBegin0;
        psessinfo->ulTrxLevel = Level();
        psessinfo->ulProcid = procid;
        psessinfo->ulTrxContext = dwTrxContext;
        psessinfo->ulFlags = 0;
    }

    friend VOID PIBReportSessionSharingViolation( const PIB * const ppib );

#pragma push_macro( "new" )
#undef new
private:
    void* operator new( size_t );
    void* operator new[]( size_t );
    void operator delete[]( void* );
public:
    void* operator new( size_t cbAlloc, INST* pinst )
    {
        return pinst->m_cresPIB.PvRESAlloc_( SzNewFile(), UlNewLine() );
    }
    void operator delete( void* pv )
    {
        if ( pv )
        {
            PIB* ppib = reinterpret_cast< PIB* >( pv );
            ppib->m_pinst->m_cresPIB.Free( pv );
        }
    }
#pragma pop_macro( "new" )

#ifdef DEBUG
    VOID    AssertValid() const;
#endif

    
    TRX                 trxBegin0;
    TRX                 trxCommit0;

    INST                *m_pinst;

private:
    DWORD_PTR           dwTrxContext;

    friend VOID PIBSetTrxBegin0( PIB * const ppib );
    LGPOS               lgposTrxBegin0;

    union
    {
        BOOL            m_fFlags;
        struct
        {
            FLAG32      m_fUserSession:1;
            FLAG32      m_fAfterFirstBT:1;
            FLAG32      m_fRecoveringEndAllSessions:1;
            FLAG32      m_fOLD:1;
            FLAG32      m_fSystemCallback:1;
            FLAG32      m_fCIMCommitted:1;
            FLAG32      m_fCIMDirty:1;
            FLAG32      m_fSetAttachDB:1;
            FLAG32      m_fUseSessionContextForTrxContext:1;
            FLAG32      m_fBegin0Logged:1;
            FLAG32      m_fLGWaiting:1;
            FLAG32      m_fReadOnlyTrx:1;
            FLAG32      m_fBatchIndexCreation:1;
            FLAG32      m_fDistributedTrx:1;
            FLAG32      m_fPreparedToCommitTrx:1;
            FLAG32      m_fOLD2:1;
            FLAG32      m_fMustRollbackToLevel0:1;
            FLAG32      m_fDBScan:1;
        };
    };

public:

    LONG                m_cInJetAPI;

    RCE                 *prceNewest;

    FUCB                *pfucbOfSession;

    PROCID              procid;

    USHORT              rgcdbOpen[dbidMax];

    TLS                 *ptlsApi;

    PIB                 *ppibNext;

private:
    LEVEL               m_level;

public:
    LEVEL               levelBegin;
    LEVEL               clevelsDeferBegin;
    LEVEL               levelRollback;

    LONG                grbitCommitDefault;

    UPDATEID            updateid;

    ULONG               cCursors;

    CInvasiveList< PIB, OffsetOfTrxOldestILE >::CElement
                        m_ileTrxOldest;

    LGPOS               lgposStart;
    LGPOS               lgposCommit0;

    INST::PLS*          m_pplsTrxOldest;

private:
    DWORD_PTR           dwSessionContext;
    DWORD_PTR           dwSessionContextThreadId;

private:
    VOID                *m_pvRecordFormatConversionBuffer;

public:
    CCriticalSection    critCursors;
    CCriticalSection    critConcurrentDDL;
    CCriticalSection    critLogBeginTrx;
    CAutoResetSignal    asigWaitLogWrite;

    PIB                 *ppibNextWaitWrite;
    PIB                 *ppibPrevWaitWrite;
    LGPOS               lgposWaitLogWrite;

    PIB                 *ppibHashOverflow;

    TLS                 *ptlsTrxBeginLast;

private:

    CRedBlackTree<RCEID,PGNO>   m_redblacktreeRceidDeferred;

    CRedBlackTree<RCEID,RCE*>   m_redblacktreePrceOfSession;

private:
    MACRO               *m_pMacroNext;

    ERR                 m_errRollbackFailure;

    TrxidStack          m_trxidstack;

    BYTE                m_cbClientCommitContextGeneric;
    BYTE                m_rgbClientCommitContextGeneric[70];

    BYTE                m_fCommitContextContainsCustomerData;

    UserTraceContext    m_utc;
    TICK                m_tickLevel0Begin;

    volatile BOOL       m_fLoggedCheckpointGettingDeep;

    CCriticalSection    m_critCachePriority;
    WORD                m_pctCachePriority;
    WORD                m_rgpctCachePriority[dbidMax];

    ULONG               m_grbitUserIoPriority;
    ULONG               m_qosIoPriority;


public:
    LEVEL               Level() const { return m_level; }
    void                SetLevel( const LEVEL level );
    void                IncrementLevel( const TRXID trxid );
    void                DecrementLevel();

    const TrxidStack&   TrxidStack() const { return m_trxidstack; }
    
    BOOL                FMacroGoing()   const;
    BOOL                FMacroGoing( DBTIME dbtime )    const;
    ERR                 ErrSetMacroGoing( DBTIME dbtime );
    VOID                ResetMacroGoing( DBTIME dbtime );
    ERR                 ErrAbortAllMacros( BOOL fLogEndMacro );
    ERR                 ErrInsertLogrec( DBTIME dbtime, const VOID * pv, const ULONG cb );
    VOID                *PvLogrec( DBTIME dbtime )      const;
    SIZE_T              CbSizeLogrec( DBTIME dbtime )   const;

    RCE                 * PrceOldest();

    VOID                PIBSetTrxContext();
    VOID                PIBResetTrxContext();
    ERR                 ErrPIBCheckCorrectSessionContext() const;
    void                PIBGetSessContextInfo( _Out_cap_c_(cchContextStringSize) PWSTR szSesContextThreadID, _Out_cap_c_(cchContextStringSize) PWSTR szSesContext ) const;
    ERR                 ErrPIBSetSessionContext( const DWORD_PTR dwContext );
    VOID                PIBResetSessionContext( _In_ const BOOL fExpectedReset );
    BOOL                FPIBCheckTrxContext() const;
    VOID                PrepareTrxContextForEnd();
    DWORD_PTR           TidActive() const;

    VOID                ResetFlags()                                        { m_fFlags = 0; }

    BOOL                FUserSession() const                                { return m_fUserSession; }
    VOID                SetFUserSession()                                   { m_fUserSession = fTrue; }

    BOOL                FAfterFirstBT() const                               { return m_fAfterFirstBT; }
    VOID                SetFAfterFirstBT()                                  { m_fAfterFirstBT = fTrue; }

    BOOL                FRecoveringEndAllSessions() const                   { return m_fRecoveringEndAllSessions; }
    VOID                SetFRecoveringEndAllSessions()                      { m_fRecoveringEndAllSessions = fTrue; }

    BOOL                FSessionOLD() const                                 { return m_fOLD; }
    VOID                SetFSessionOLD()                                    { m_fOLD = fTrue; }

    BOOL                FSessionOLD2() const                                { return m_fOLD2; }
    VOID                SetFSessionOLD2()                                   { m_fOLD2 = fTrue; }

    BOOL                FSystemCallback() const                             { return m_fSystemCallback; }
    VOID                SetFSystemCallback()                                { m_fSystemCallback = fTrue; }

    BOOL                FSessionDBScan() const                              { return m_fDBScan; }
    VOID                SetFSessionDBScan()                                 { m_fDBScan = fTrue; }

    BOOL                FSessionSystemTask() const                          { return ( FSessionOLD() || FSessionOLD2() || FSessionDBScan() ); }

    BOOL                FCIMCommitted() const                               { return m_fCIMCommitted; }

    BOOL                FCIMDirty() const                                   { return m_fCIMDirty; }
    VOID                SetFCIMDirty()                                      { m_fCIMDirty = fTrue; }
    VOID                ResetFCIMDirty()                                    { m_fCIMDirty = fFalse; }

    BOOL                FSetAttachDB() const                                { return m_fSetAttachDB; }
    VOID                SetFSetAttachDB()                                   { m_fSetAttachDB = fTrue; }
    VOID                ResetFSetAttachDB()                                 { m_fSetAttachDB = fFalse; }

    BOOL                FBegin0Logged() const                               { return m_fBegin0Logged; }
    VOID                SetFBegin0Logged()                                  { m_fBegin0Logged = fTrue; }
    VOID                ResetFBegin0Logged()                                { m_fBegin0Logged = fFalse; }

    BOOL                FLGWaiting() const                                  { return m_fLGWaiting; }
    VOID                SetFLGWaiting()                                     { m_fLGWaiting = fTrue; }
    VOID                ResetFLGWaiting()                                   { m_fLGWaiting = fFalse; }

    BOOL                FReadOnlyTrx() const                                { return m_fReadOnlyTrx; }
    VOID                SetFReadOnlyTrx()                                   { m_fReadOnlyTrx = fTrue; }
    VOID                ResetFReadOnlyTrx()                                 { m_fReadOnlyTrx = fFalse; }

    BOOL                FBatchIndexCreation() const                         { return m_fBatchIndexCreation; }
    VOID                SetFBatchIndexCreation()                            { m_fBatchIndexCreation = fTrue; }
    VOID                ResetFBatchIndexCreation()                          { m_fBatchIndexCreation = fFalse; }

    VOID                * PvRecordFormatConversionBuffer() const            { return m_pvRecordFormatConversionBuffer; }
    ERR                 ErrAllocPvRecordFormatConversionBuffer();
    VOID                FreePvRecordFormatConversionBuffer();

    ERR                 ErrRollbackFailure() const                          { return m_errRollbackFailure; }
    VOID                SetErrRollbackFailure( const ERR err );

    ERR                 ErrRegisterDeferredRceid( const RCEID&, PGNO pgno );
    ERR                 ErrDeregisterDeferredRceid( const RCEID& );
    VOID                RemoveAllDeferredRceid();
    VOID                AssertNoDeferredRceid() const;

    ERR                 ErrRegisterRceid( const RCEID rceid, RCE * const prce);
    ERR                 ErrDeregisterRceid( const RCEID rceid );
    VOID                RemoveAllRceid();
    RCE *               PrceNearestRegistered( const RCEID rceid ) const;

    VOID                SetMustRollbackToLevel0()                       { m_fMustRollbackToLevel0 = fTrue; }
    VOID                ResetMustRollbackToLevel0()                     { m_fMustRollbackToLevel0 = fFalse; }
    BOOL                FMustRollbackToLevel0() const                   { return m_fMustRollbackToLevel0; }

    ERR                 ErrSetClientCommitContextGeneric( const void * const pvCtx, const INT cbCtx );
    INT                 CbClientCommitContextGeneric() const            { return m_cbClientCommitContextGeneric; }
    const VOID *        PvClientCommitContextGeneric() const            { return m_rgbClientCommitContextGeneric; }

    ERR                 ErrSetCommitContextContainsCustomerData( const void * const pvCtx, const INT cbCtx );
    BOOL                FCommitContextContainsCustomerData() const      { return m_fCommitContextContainsCustomerData; }

    ERR                 ErrSetOperationContext( const void* const pvOperationContext, const INT cbCtx );
    const VOID *        PvOperationContext() const                      { return &m_utc.context; }

    ERR                 ErrSetCorrelationID( const void* const pvID, const INT cbID );
    const VOID *        PvCorrelationID() const                         { return &m_utc.dwCorrelationID; }

    ERR                 ErrSetIOSESSTraceFlags( const void* const pvFlags, const INT cbFlags );
    const VOID *        PvIOSessTraceFlags() const                      { return &m_utc.dwIOSessTraceFlags; }

    ERR                 ErrSetClientActivityID( const void* const pvID, const INT cbID );
    const GUID *        PguidClientActivityID() const                   { return &m_utc.pUserContextDesc->guidActivityId; }

    ERR                 ErrSetClientComponentDesc( const void* const pvClientComponent, const INT cb );
    const char *        PszClientComponentDesc() const                  { return m_utc.pUserContextDesc->szClientComponent; }

    ERR                 ErrSetClientActionDesc( const void* const pvClientAction, const INT cb );
    const char *        PszClientActionDesc() const                     { return m_utc.pUserContextDesc->szClientAction; }

    ERR                 ErrSetClientActionContextDesc( const void* const pvClientActionContext, const INT cb );
    const char *        PszClientActionContextDesc() const              { return m_utc.pUserContextDesc->szClientActionContext; }
    TICK                TickLevel0Begin() const { return m_tickLevel0Begin; }

#ifdef DEBUG
    BOOL                FLoggedCheckpointGettingDeep() const                { return m_fLoggedCheckpointGettingDeep; }
#endif
    BOOL                FCheckSetLoggedCheckpointGettingDeep();
    VOID                ResetLoggedCheckpointGettingDeep();

private:
    VOID                ResolveCachePriorityForDb_( const DBID dbid );

public:
    ERR                 ErrSetCachePriority( const void* const pvCachePriority, const INT cbCachePriority );
    VOID                ResolveCachePriorityForDb( const DBID dbid );
    WORD                PctCachePriorityPib() const;
    ULONG_PTR           PctCachePriorityPibDbid( const DBID dbid ) const;

    ERR                 ErrSetUserIoPriority( const void* const pvUserFlags, const INT cbFlags );
    ULONG               GrbitUserIoPriority() const                     { return m_grbitUserIoPriority; }
    BFTEMPOSFILEQOS     QosIoPriority() const                           { return (BFTEMPOSFILEQOS)m_qosIoPriority; }

    BFPriority          BfpriPriority( const IFMP ifmp ) const;

    PIBTraceContextScope InitTraceContextScope() const;
    void SetUserTraceContextInTls() const;
    void ClearUserTraceContextInTls() const;
    UserTraceContext*   Putc()                                          { return &m_utc; }
    const UserTraceContext* Putc() const                                { return &m_utc; }

#ifdef DEBUGGER_EXTENSION
    VOID DumpOpenTrxUserDetails( _In_ CPRINTF * const pcprintf, _In_ const DWORD_PTR dwOffset = 0, _In_ LONG lgenTip = 0 ) const;
    VOID DumpBasic( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
    VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif
};

#ifdef _WIN64
C_ASSERT( sizeof(PIB) == 608 );
#else
C_ASSERT( sizeof(PIB) == 504 );
#endif

INLINE SIZE_T OffsetOfTrxOldestILE()    { return OffsetOf( PIB, m_ileTrxOldest ); }

#define PinstFromPpib( ppib )   ((ppib)->m_pinst)

INLINE JET_SESID SesidFromPpib( const PIB * const ppib ) { Assert( sizeof( JET_SESID ) == sizeof( PIB * ) ); return reinterpret_cast<JET_SESID>( ppib ); }
INLINE PIB *PpibFromSesid( const JET_SESID sesid) { Assert( sizeof( JET_SESID ) == sizeof( PIB * ) ); return reinterpret_cast<PIB *>( sesid ); }

INLINE BOOL PIB::FMacroGoing()  const
{
    ASSERT_VALID( this );

    MACRO   *pMacro;
    for ( pMacro = m_pMacroNext; pMacro != NULL; pMacro = pMacro->PMacroNext() )
    {
        if ( pMacro->Dbtime() != dbtimeNil )
        {
            return fTrue;
        }
    }

    return fFalse;
}


INLINE BOOL PIB::FMacroGoing( DBTIME dbtime )   const
{
    ASSERT_VALID( this );

    MACRO   *pMacro;
    for ( pMacro = m_pMacroNext; pMacro != NULL; pMacro = pMacro->PMacroNext() )
    {
        if ( pMacro->Dbtime() == dbtime )
        {
            return fTrue;
        }
    }

    return fFalse;
}


INLINE ERR PIB::ErrSetMacroGoing( DBTIME dbtime )
{
    ASSERT_VALID( this );

    MACRO   *pMacro = m_pMacroNext;

    #ifdef DEBUG
    for ( pMacro = m_pMacroNext; pMacro != NULL; pMacro = pMacro->PMacroNext() )
    {
        Assert( pMacro->Dbtime() != dbtime );
    }
    #endif

    for ( pMacro = m_pMacroNext; pMacro != NULL; pMacro = pMacro->PMacroNext() )
    {
        if ( pMacro->Dbtime() == dbtimeNil )
        {
            pMacro->SetDbtime( dbtime );
            return JET_errSuccess;
        }
    }

    pMacro = (MACRO *) PvOSMemoryHeapAlloc( sizeof( MACRO ) );
    if ( NULL == pMacro )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    memset( pMacro, 0, sizeof( MACRO ) );
    pMacro->SetDbtime( dbtime );

    pMacro->SetMacroNext ( m_pMacroNext );
    m_pMacroNext = pMacro;

    return JET_errSuccess;
}


INLINE VOID PIB::ResetMacroGoing( DBTIME dbtime )
{
    ASSERT_VALID( this );

    MACRO *     pMacro;
    MACRO *     pMacroPrev  = NULL;

    for ( pMacro = m_pMacroNext; pMacro != NULL; pMacroPrev = pMacro, pMacro = pMacro->PMacroNext() )
    {
        if ( pMacro->Dbtime() == dbtime )
        {
            Assert( m_pMacroNext != NULL );
            if ( m_pMacroNext->PMacroNext() == NULL )
            {
                pMacro->SetDbtime( dbtimeNil );
                pMacro->ResetBuffer();
            }
            else
            {
                if ( NULL == pMacroPrev )
                {
                    m_pMacroNext = pMacro->PMacroNext();
                }
                else
                {
                    pMacroPrev->SetMacroNext( pMacro->PMacroNext() );
                }

                pMacro->ReleaseBuffer();
                OSMemoryHeapFree( pMacro );
            }
            break;
        }
    }

    Assert( NULL != pMacro || !FAfterFirstBT() );
}


INLINE VOID PIB::MACRO::ResetBuffer()
{
    m_ibLogrecAvail = 0;
}

INLINE VOID PIB::MACRO::ReleaseBuffer()
{
    if ( NULL != m_rgbLogrec )
    {
        OSMemoryHeapFree( m_rgbLogrec );
        m_rgbLogrec = NULL;
        m_ibLogrecAvail = 0;
        m_cbLogrecMac = 0;
    }
}

INLINE ERR PIB::MACRO::ErrInsertLogrec( const VOID * pv, const ULONG cb )
{
    const ULONG     cbStep  = 512;
    const ULONG     cbAlloc = cbStep * ( ( cb / cbStep ) + 1 );

    while ( m_ibLogrecAvail + cb >= m_cbLogrecMac )
    {
        BYTE *  rgbLogrecOld    = m_rgbLogrec;
        size_t  cbLogrecMacOld  = m_cbLogrecMac;

        if (  ( cbLogrecMacOld + cbAlloc < cbLogrecMacOld ) || ( cbLogrecMacOld + cbAlloc < cbAlloc ) )
        {
            return ErrERRCheck( JET_errOutOfMemory );
        }
        
        BYTE *  rgbLogrec       = reinterpret_cast<BYTE *>( PvOSMemoryHeapAlloc( cbLogrecMacOld + cbAlloc ) );

        if ( NULL == rgbLogrec )
        {
            return ErrERRCheck( JET_errOutOfMemory );
        }

        UtilMemCpy( rgbLogrec, rgbLogrecOld, cbLogrecMacOld );

        m_rgbLogrec = rgbLogrec;
        m_cbLogrecMac = cbLogrecMacOld + cbAlloc;

        if ( NULL != rgbLogrecOld )
        {
            OSMemoryHeapFree( rgbLogrecOld );
        }
    }

    UtilMemCpy( m_rgbLogrec + m_ibLogrecAvail, pv, cb );
    m_ibLogrecAvail += cb;

    return JET_errSuccess;
}


INLINE VOID * PIB::PvLogrec( DBTIME dbtime )    const
{
    ASSERT_VALID( this );

    MACRO   *pMacro;
    for ( pMacro = m_pMacroNext; pMacro != NULL; pMacro = pMacro->PMacroNext() )
    {
        if ( pMacro->Dbtime() == dbtime )
        {
            return pMacro->PvLogrec();
        }
    }

    Assert( fFalse );
    return NULL;
}


INLINE SIZE_T PIB::CbSizeLogrec( DBTIME dbtime )    const
{
    ASSERT_VALID( this );

    MACRO   *pMacro;
    for ( pMacro = m_pMacroNext; pMacro != NULL; pMacro = pMacro->PMacroNext() )
    {
        if ( pMacro->Dbtime() == dbtime )
        {
            return pMacro->CbSizeLogrec();
        }
    }

    EnforceSz( fFalse, "UnknownCbSizeLogrec" );
    return 0;
}


INLINE ERR PIB::ErrInsertLogrec( DBTIME dbtime, const VOID * pv, const ULONG cb )
{
    ASSERT_VALID( this );

    for ( MACRO * pMacro = m_pMacroNext; pMacro != NULL; pMacro = pMacro->PMacroNext() )
    {
        if ( pMacro->Dbtime() == dbtime )
        {
            return pMacro->ErrInsertLogrec( pv, cb );
        }
    }

    Assert( fFalse );
    return ErrERRCheck( JET_errInvalidSesid );
}


INLINE VOID PIB::SetErrRollbackFailure( const ERR err )
{
    Assert( err < JET_errSuccess );
    Assert( JET_errSuccess == ErrRollbackFailure() );
    m_errRollbackFailure = err;
}


INLINE ERR PIB::ErrAllocPvRecordFormatConversionBuffer()
{
    Assert( NULL == m_pvRecordFormatConversionBuffer );
    if( NULL == ( m_pvRecordFormatConversionBuffer = PvOSMemoryPageAlloc( g_cbPage, NULL ) ) )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }
    return JET_errSuccess;
}

INLINE VOID PIB::FreePvRecordFormatConversionBuffer()
{
    if ( NULL != m_pvRecordFormatConversionBuffer )
    {
        OSMemoryPageFree( m_pvRecordFormatConversionBuffer );
    }
    m_pvRecordFormatConversionBuffer = NULL;
}

INLINE ERR PIB::ErrSetClientCommitContextGeneric( const void * const pvCtx, const INT cbCtx )
{
    if ( cbCtx > sizeof(m_rgbClientCommitContextGeneric) )
    {
        return ErrERRCheck( JET_errInvalidBufferSize );
    }

    Assert( cbCtx < 256 );
    m_cbClientCommitContextGeneric = (BYTE)cbCtx;

    if ( cbCtx )
    {
        memcpy( m_rgbClientCommitContextGeneric, pvCtx, cbCtx );
    }

    return JET_errSuccess;
}

INLINE ERR PIB::ErrSetCommitContextContainsCustomerData( const void * const pvCtx, const INT cbCtx )
{
    if ( cbCtx != sizeof( DWORD ) )
    {
        return ErrERRCheck( JET_errInvalidBufferSize );
    }

    m_fCommitContextContainsCustomerData = !!*(DWORD *)pvCtx;
    return JET_errSuccess;
}


INLINE ERR PIB::ErrSetOperationContext( const void * const pvCtx, const INT cbCtx )
{
    static_assert( sizeof( JET_OPERATIONCONTEXT ) == sizeof( OPERATION_CONTEXT ), "JET_OPERATIONCONTEXT and OPERATION_CONTEXT should be the same size" );

    if ( cbCtx != sizeof( JET_OPERATIONCONTEXT ) )
    {
        return ErrERRCheck( JET_errInvalidBufferSize );
    }

    memcpy( &m_utc.context, pvCtx, cbCtx );
    return JET_errSuccess;
}

INLINE ERR PIB::ErrSetCorrelationID( const void *pvID, const INT cbID )
{
    if ( cbID != sizeof( m_utc.dwCorrelationID ) )
    {
        return ErrERRCheck( JET_errInvalidBufferSize );
    }

    static_assert( sizeof( m_utc.dwCorrelationID ) == sizeof( DWORD ), "Correlation ID should be a DWORD" );
    m_utc.dwCorrelationID = *( (DWORD *) pvID );
    return JET_errSuccess;
}

INLINE ERR PIB::ErrSetIOSESSTraceFlags( const void *pvFlags, const INT cbFlags )
{
    if ( cbFlags != sizeof( m_utc.dwIOSessTraceFlags ) )
    {
        return ErrERRCheck( JET_errInvalidBufferSize );
    }

    static_assert( sizeof( m_utc.dwIOSessTraceFlags ) == sizeof( DWORD ), "dwIOSessTraceFlags should be a DWORD" );
    m_utc.dwIOSessTraceFlags = *( (DWORD *) pvFlags );
    return JET_errSuccess;
}

INLINE ERR PIB::ErrSetClientActivityID( const void *pvID, const INT cbID )
{
    ERR err = JET_errSuccess;
    if ( cbID != sizeof( m_utc.pUserContextDesc->guidActivityId ) )
    {
        return ErrERRCheck( JET_errInvalidBufferSize );
    }

    CallR( m_utc.ErrLazyInitUserContextDesc() );
    memcpy( &m_utc.pUserContextDesc->guidActivityId, pvID, cbID );
    return JET_errSuccess;
}

INLINE ERR PIB::ErrSetClientComponentDesc( const void *pvClientComponent, const INT cb )
{
    ERR err = JET_errSuccess;
    CallR( m_utc.ErrLazyInitUserContextDesc() );
    return USER_CONTEXT_DESC::ErrCopyDescString( m_utc.pUserContextDesc->szClientComponent, (const char*) pvClientComponent, cb );
}

INLINE ERR PIB::ErrSetClientActionDesc( const void *pvClientAction, const INT cb )
{
    ERR err = JET_errSuccess;
    CallR( m_utc.ErrLazyInitUserContextDesc() );
    return USER_CONTEXT_DESC::ErrCopyDescString( m_utc.pUserContextDesc->szClientAction, (const char*) pvClientAction, cb );
}

INLINE ERR PIB::ErrSetClientActionContextDesc( const void *pvClientActionContext, const INT cb )
{
    ERR err = JET_errSuccess;
    CallR( m_utc.ErrLazyInitUserContextDesc() );
    return USER_CONTEXT_DESC::ErrCopyDescString( m_utc.pUserContextDesc->szClientActionContext, (const char*) pvClientActionContext, cb );
}

INLINE ULONG_PTR PIB::PctCachePriorityPibDbid( const DBID dbid ) const
{
    Assert( dbid < _countof( m_rgpctCachePriority ) );

    const ULONG_PTR pctCachePriority = m_rgpctCachePriority[ dbid ];
    Assert( FIsCachePriorityValid( pctCachePriority ) );

    return pctCachePriority;
}

INLINE ERR PIB::ErrSetUserIoPriority( const void* const pvUserFlags, const INT cbFlags )
{
    ERR err = JET_errSuccess;

    if ( cbFlags != sizeof( JET_GRBIT ) )
    {
        CallR( ErrERRCheck( JET_errInvalidBufferSize ) );
    }

    JET_GRBIT grbitUserIoPriFlagsUnprocessed = *( (JET_GRBIT *) pvUserFlags );
    ULONG qosIo = 0x0;

    

    
    if ( grbitUserIoPriFlagsUnprocessed & JET_IOPriorityLow )
    {
        grbitUserIoPriFlagsUnprocessed &= ~JET_IOPriorityLow;
        qosIo |= qosIOOSLowPriority;
    }

    qosIo |= ( grbitUserIoPriFlagsUnprocessed & ( JET_IOPriorityUserClassIdMask | JET_IOPriorityMarkAsMaintenance ) );
    grbitUserIoPriFlagsUnprocessed &= ~JET_IOPriorityUserClassIdMask;
    grbitUserIoPriFlagsUnprocessed &= ~JET_IOPriorityMarkAsMaintenance;

    qosIo |= ( grbitUserIoPriFlagsUnprocessed & qosIODispatchBackground );
    grbitUserIoPriFlagsUnprocessed &= ~qosIODispatchBackground;

    if ( grbitUserIoPriFlagsUnprocessed != 0 )
    {
        AssertSz( fFalse, "Client passed these flags 0x%x in for User IoPriority flags that ESE does not understand.", grbitUserIoPriFlagsUnprocessed );
        CallR( ErrERRCheck( JET_errInvalidGrbit ) );
    }


    m_grbitUserIoPriority = *( (JET_GRBIT *) pvUserFlags );
    m_qosIoPriority |= qosIo;

    return JET_errSuccess;
}

INLINE BFPriority PIB::BfpriPriority( const IFMP ifmpOverride ) const
{
    Assert( ifmpOverride > 0 );
    Assert( ifmpOverride != ifmpNil );

    return BfpriBFMake( PctCachePriorityPibDbid( DbidFromIfmp( ifmpOverride ) ), QosIoPriority() );
}


INLINE PIBTraceContextScope PIB::InitTraceContextScope() const
{
    Assert( Putc() == PutcTLSGetUserContext() || m_cInJetAPI == 0 );

    return PIBTraceContextScope( this );
}

INLINE void PIB::SetUserTraceContextInTls() const
{
    const UserTraceContext* ptcPrev = TLSSetUserTraceContext( Putc() );
    Assert( NULL == ptcPrev );
}

INLINE void PIB::ClearUserTraceContextInTls() const
{
    const UserTraceContext* ptcPrev = TLSSetUserTraceContext( NULL );
    Assert( ptcPrev == Putc() );
}



TRX TrxOldest( INST *pinst );
TRX TrxOldestCached( const INST * const pinst );
void UpdateCachedTrxOldest( INST * const pinst );

ERR ErrPIBInit( INST *pinst );
VOID PIBTerm( INST *pinst );

INLINE VOID PIBSetUpdateid( PIB * ppib, UPDATEID updateid )
{
    ppib->updateid = updateid;
}

INLINE UPDATEID UpdateidOfPpib( const PIB * ppib )
{
    return ppib->updateid;
}

INLINE BOOL FPIBUserOpenedDatabase( _In_ const PIB * const ppib, _In_ const DBID dbid )
{
    return ppib->rgcdbOpen[ dbid ] > 0;
}

BOOL FPIBSessionLV( PIB *ppib );
BOOL FPIBSessionRCEClean( PIB *ppib );

INLINE BOOL FPIBSessionSystemCleanup( PIB *ppib )
{
    return ( FPIBSessionRCEClean( ppib )
        || ppib->FSystemCallback()
        || ppib->FSessionOLD()
        || ppib->FSessionOLD2()
        || ppib->FSessionDBScan() );
}


#define CheckPIB( ppib )    Assert( (ppib)->Level() < levelMax )

#define ErrPIBCheck( ppib ) JET_errSuccess

#define ErrPIBCheckIfmp( ppib, ifmp )                               \
    ( ( FInRangeIFMP( ifmp ) && ( ppib->FSystemCallback() || FPIBUserOpenedDatabase( ppib, g_rgfmp[ifmp].Dbid() ) ) ) \
        ? JET_errSuccess : ErrERRCheck( JET_errInvalidDatabaseId ) )

#ifdef DEBUG


INLINE VOID PIB::AssertValid( ) const
{
    CResource::AssertValid( JET_residPIB, this );
    Assert( m_level < levelMax );
}


#endif

#define FPIBActive( ppib )                      ( (ppib)->level != levelNil )


ERR ErrPIBBeginSession( INST *pinst, _Outptr_ PIB ** pppib, PROCID procid, BOOL fForCreateTempDatabase );
VOID PIBEndSession( PIB *ppib );
ERR ErrPIBOpenTempDatabase( PIB *ppib );
ERR ErrEnsureTempDatabaseOpened( _In_ const INST* const pinst, _In_ const PIB * const ppib );
#ifdef DEBUG
VOID PIBPurge( VOID );
#else
#define PIBPurge()
#endif

VOID PIBReportSessionSharingViolation( const PIB * const ppib );

INLINE VOID PIBSetPrceNewest( PIB *ppib, RCE *prce )
{
    ppib->prceNewest = prce;
}


INLINE VOID PIBSetLevelRollback( PIB *ppib, LEVEL levelT )
{
    Assert( levelT > levelMin && levelT < levelMax );
    Assert( ppib->levelRollback >= levelMin && ppib->levelRollback < levelMax );
    if ( levelT < ppib->levelRollback )
        ppib->levelRollback = levelT;
}

#define TRXID_INCR  4

INLINE VOID PIBSetTrxBegin0( PIB * const ppib )
{
    INST* const pinst = PinstFromPpib( ppib );
    INST::PLS* const ppls = pinst->Ppls();

    ppls->m_rwlPIBTrxOldest.EnterAsWriter();
    if ( ppib->FReadOnlyTrx() )
    {
        ppib->trxBegin0 = pinst->m_trxNewest + TRXID_INCR/2;
    }
    else
    {
        ppib->trxBegin0 = TRX( AtomicExchangeAdd( (LONG *)&pinst->m_trxNewest, TRXID_INCR ) ) + TRXID_INCR;
    }
    ppib->lgposTrxBegin0 = pinst->m_plog->LgposLGLogTipNoLock();

    ppib->m_pplsTrxOldest = ppls;
#ifdef DEBUG
    PIB* const ppibTrxOldest = ppls->m_ilTrxOldest.PrevMost();
    Assert ( !ppibTrxOldest || ( INT( ppibTrxOldest->trxBegin0 - ppib->trxBegin0 ) <= 0 ) );
#endif
    ppls->m_ilTrxOldest.InsertAsNextMost( ppib );
    ppls->m_rwlPIBTrxOldest.LeaveAsWriter();

    ppib->trxCommit0 = trxMax;
}

INLINE VOID PIBResetTrxBegin0( PIB * const ppib )
{
    Assert( 0 == ppib->Level() );
    Assert( NULL == ppib->prceNewest );

    INST::PLS* const ppls = ppib->m_pplsTrxOldest;
    if ( ppls != NULL )
    {
        ppls->m_rwlPIBTrxOldest.EnterAsWriter();
        ppls->m_ilTrxOldest.Remove( ppib );
        ppls->m_rwlPIBTrxOldest.LeaveAsWriter();
    }

    ppib->trxBegin0     = trxMax;
    ppib->lgposStart    = lgposMax;
    ppib->ResetFReadOnlyTrx();
    ppib->ResetLoggedCheckpointGettingDeep();

}


INLINE ERR PIB::ErrPIBSetSessionContext( const DWORD_PTR dwContext )
{
    Assert( dwPIBSessionContextNull != dwContext );
    if ( dwPIBSessionContextNull != AtomicCompareExchangePointer( (void**)&dwSessionContext, (void*)dwPIBSessionContextNull, (void*)dwContext ) )
        return ErrERRCheck( JET_errSessionContextAlreadySet );

    Assert( 0 == dwSessionContextThreadId );
    dwSessionContextThreadId = DwUtilThreadId();
    Assert( 0 != dwSessionContextThreadId );

    return JET_errSuccess;
}

INLINE VOID PIB::PIBResetSessionContext( _In_ const BOOL fExpectedReset )
{
    if ( fExpectedReset )
    {
        Assert( dwPIBSessionContextUnusable != dwSessionContext );
    }
    else
    {
        Assert( dwPIBSessionContextUnusable == dwSessionContext );
    }
    Assert( DwUtilThreadId() == dwSessionContextThreadId );
    dwSessionContextThreadId = 0;
    dwSessionContext = dwPIBSessionContextNull;
}

INLINE ERR PIB::ErrPIBCheckCorrectSessionContext() const
{
    if ( dwSessionContextThreadId != DwUtilThreadId()
        || dwPIBSessionContextNull == dwSessionContext )
    {
        return ErrERRCheck( JET_errSessionContextNotSetByThisThread );
    }
    return JET_errSuccess;
}

INLINE DWORD_PTR  PIB::TidActive() const
{
    return m_fUseSessionContextForTrxContext ? dwSessionContextThreadId : dwTrxContext;
}

INLINE void PIB::PIBGetSessContextInfo( _Out_cap_c_(cchContextStringSize) PWSTR szSesContextThreadID, _Out_cap_c_(cchContextStringSize) PWSTR szSesContext ) const
{
    const SIZE_T cbContextStringSize = cchContextStringSize * sizeof(WCHAR);
    if ( m_fUseSessionContextForTrxContext )
    {
        OSStrCbFormatW( szSesContextThreadID, cbContextStringSize, L"0x%0*I64X", (ULONG)sizeof(DWORD_PTR)*2, QWORD( dwSessionContextThreadId ) );
        OSStrCbFormatW( szSesContext, cbContextStringSize, L"0x%0*I64X", (ULONG)sizeof(DWORD_PTR)*2, QWORD( dwSessionContext ) );
    }
    else
    {
        OSStrCbFormatW( szSesContextThreadID, cbContextStringSize, L"0x%0*I64X", (ULONG)sizeof(DWORD_PTR)*2, QWORD( dwTrxContext ) );
        OSStrCbFormatW( szSesContext, cbContextStringSize, L"0x%08X", 0 );
    }
}


INLINE VOID PIB::PIBSetTrxContext()
{
    m_tickLevel0Begin = TickOSTimeCurrent();
    if ( m_pinst->FRecovering() || this->FBatchIndexCreation() )
    {
    }
    else if ( dwPIBSessionContextNull == dwSessionContext )
    {
        m_fUseSessionContextForTrxContext = fFalse;
        dwTrxContext = DwUtilThreadId();
    }
    else
    {
        m_fUseSessionContextForTrxContext = fTrue;
        dwTrxContext = dwSessionContext;
    }
}

INLINE VOID PIB::PIBResetTrxContext()
{
    if ( !m_fUseSessionContextForTrxContext )
    {
        dwTrxContext = 0;
    }
}


INLINE VOID PIB::PrepareTrxContextForEnd()
{
    if ( m_fUseSessionContextForTrxContext )
    {
        Assert( !m_pinst->FRecovering() );

        dwTrxContext = dwSessionContext;
    }
    else
    {
    }
}

INLINE BOOL PIB::FPIBCheckTrxContext() const
{
    if ( m_pinst->FRecovering() || this->FBatchIndexCreation() )
    {
        return fTrue;
    }
    else if ( m_fUseSessionContextForTrxContext )
    {
        return ( dwTrxContext == dwSessionContext
                && dwSessionContextThreadId == DwUtilThreadId() );
    }
    else
    {
        return ( dwTrxContext == DwUtilThreadId() );
    }
}

inline BOOL PIB::FCheckSetLoggedCheckpointGettingDeep()
{
    Assert( m_pinst->m_critPIB.FOwner() );
    Expected( critLogBeginTrx.FOwner() );
    BOOL fCheck = AtomicCompareExchange( (LONG*)&m_fLoggedCheckpointGettingDeep, fFalse, fTrue );
    if ( fCheck == fFalse )
    {
        return fTrue;
    }
    return fFalse;
}

INLINE VOID PIB::ResetLoggedCheckpointGettingDeep()                  
{
    Expected( lgposStart.lGeneration == lGenerationInvalid );
    if ( m_fLoggedCheckpointGettingDeep )
    {
        BOOL fCheck = AtomicCompareExchange( (LONG*)&m_fLoggedCheckpointGettingDeep, fTrue, fFalse );
        Expected( fCheck == fTrue );
    }
}

INLINE ERR ErrPIBCheckUpdatable( const PIB * ppib )
{
    ERR     err;

    if ( ppib->FReadOnlyTrx() )
        err = ErrERRCheck( JET_errTransReadOnly );
    else
        err = JET_errSuccess;

    return err;
}



INLINE BOOL FPIBVersion( const PIB * const ppib )
{
    const BOOL  fCIMEnabled = ( ppib->FCIMCommitted() || ppib->FCIMDirty() );
    return !fCIMEnabled;
}

INLINE BOOL FPIBCommitted( const PIB * const ppib )
{
    return ppib->FCIMCommitted();
}

INLINE BOOL FPIBDirty( const PIB * const ppib )
{
    return ppib->FCIMDirty();
}
INLINE VOID PIBSetCIMDirty( PIB * const ppib )
{
    ppib->SetFCIMDirty();
}
INLINE VOID PIBResetCIMDirty( PIB * const ppib )
{
    ppib->ResetFCIMDirty();
}


class CSessionHash : public CSimpleHashTable<PIB>
{
    public:
        CSessionHash( const ULONG csessions ) :
            CSimpleHashTable<PIB>( min( csessions, csessionsMax ), OffsetOf( PIB, ppibHashOverflow ) )      {}
        ~CSessionHash()     {}

        enum    { csessionsMax = 0xFFFF };

        ULONG UlHash( const PROCID procid ) const
        {
            return ULONG( procid % CEntries() );
        }

        VOID InsertPib( PIB * ppib )
        {
            const ULONG ulHash  = UlHash( ppib->procid );
            InsertEntry( ppib, ulHash );
        }

        VOID RemovePib( PIB * ppib )
        {
            const ULONG ulHash  = UlHash( ppib->procid );
            RemoveEntry( ppib, ulHash );
        }

        PIB * PpibHashFind( const PROCID procid ) const
        {
            const ULONG ulHash=UlHash( procid );

            for ( PIB * ppib = PentryOfHash( ulHash );
                ppibNil != ppib;
                ppib = ppib->ppibHashOverflow )
            {
                if ( procid == ppib->procid )
                {
                    return ppib;
                }
            }

            return ppibNil;
        }
};

INLINE TraceContext* PIBGetEtcFunctor::operator()() const
{
    return m_ppib->ptlsApi != NULL ?
        const_cast<TraceContext*>( PetcTLSGetEngineContextCached( m_ppib->ptlsApi ) ) :
        const_cast<TraceContext*>( PetcTLSGetEngineContext() );
}

