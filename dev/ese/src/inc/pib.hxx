// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

const DWORD_PTR dwPIBSessionContextNull     = 0;
const DWORD_PTR dwPIBSessionContextUnusable = DWORD_PTR( ~0 );

const DWORD cchContextStringSize = 32;


//  ================================================================
class TrxidStack
//  ================================================================
//
//  Tracks a stack of transaction IDs and the time the transaction
//  was started. This is used for logging/reporting so a method
//  to dump the current stack is provided.
//
//-
{
public:
    TrxidStack();
    ~TrxidStack();

    // Add an entry to the top of the stack (begin a transaction)
    void Push( const TRXID trxid );

    // Pop an entry off the top of the stack (end a transaction)
    void Pop();

    // Return the trxid on the top of the stack ( doesn't begin or end a transaction )
    TRXID Peek() const;

    // Return the trxid on the bottom of the stack ( doesn't begin or end a transaction )
    TRXID Peek0() const;

    // Clear all entries on the stack
    void Clear();

    // Dump the entries on the stack. If the buffer is too small
    // then the buffer is left as empty.
    ERR ErrDump( __out_ecount_z( cchBuf ) PWSTR const szBuf, const size_t cchBuf, const WCHAR * const szLineBreak = L"\r\n" ) const;

private:
    // Maximum transaction nesting level
    static const INT m_ctrxMax = levelMax+1;
    
    // A stack of transaction IDs, showing the places
    // that BeginTransaction was called from.
    TRXID m_rgtrxid[m_ctrxMax];
    
    // A stack of DateTimes, showing the times BeginTransaction
    // was called.
    __int64 m_rgtrxtime[m_ctrxMax];

    // Current number of transactions in the stack
    INT m_ctrxCurr;

private:    // not implemented
    TrxidStack( const TrxidStack& );
    TrxidStack& operator=( const TrxidStack& );
};

class PIB;

// A functor that gets the engine TraceContext from the cached TLS pointer in PPIB::_tls
// To be used with PIBTraceContextScope (to reduce calls to GetTlsValue())
class PIBGetEtcFunctor
{
    const PIB* m_ppib;

public:
    PIBGetEtcFunctor( const PIB* ppib ) : m_ppib( ppib )
    {}

    TraceContext* operator()() const;
};

// TraceContextScope that uses the TLS pointer cached on the PIB to get the current engine TraceContext.
// Improves perf by removing calls to TlsGetValue()
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

//
// Process Information Block
//
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

        // Default user ID, if the user doesn't set one.
        // Helps us in asserting/trapping invalid trace contexts deep down in the bowels of the engine.
        m_utc.context.dwUserID = OCUSER_UNINIT;
    }

    ~PIB()
    {
        if ( &m_utc == PutcTLSGetUserContext() )
        {
            // A dangling pointer to this PIB's utc on the TLS was detected !
            // We messed up JetEndSession() !
            FireWall( "PibUtcCleanup" );
            TLSSetUserTraceContext( NULL );
        }
    }

    //  It would make a lot sense to make this static, but there are several callers, and that would balloon this 
    //  change.  For now these are (or should be) effectively friends.
    friend ERR ErrPIBBeginSession( INST *pinst, _Outptr_ PIB **pppib, PROCID procidTarget, BOOL fForCreateTempDatabase );
    friend VOID PIBEndSession( PIB *ppib );
    void PIBGetSessionInfoV1( _Out_ JET_SESSIONINFO * psessinfo ) const
    {
        psessinfo->ulTrxBegin0 = trxBegin0;
        psessinfo->ulTrxLevel = Level();
        psessinfo->ulProcid = procid;
        psessinfo->ulTrxContext = dwTrxContext;
        psessinfo->ulFlags = 0;     //  currently unused
    }

    friend VOID PIBReportSessionSharingViolation( const PIB * const ppib );

#pragma push_macro( "new" )
#undef new
private:
    void* operator new( size_t );           //  meaningless without INST*
    void* operator new[]( size_t );         //  meaningless without INST*
    void operator delete[]( void* );        //  not supported
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

    /*  most used field has offset 0
    /**/
    TRX                 trxBegin0;              // f  trx id
    TRX                 trxCommit0;

    INST                *m_pinst;

private:
    DWORD_PTR           dwTrxContext;           //  default is thread id.

    friend VOID PIBSetTrxBegin0( PIB * const ppib );
    LGPOS               lgposTrxBegin0;         //  temporary, only for debugging

    //  flags
    union
    {
        BOOL            m_fFlags;
        struct
        {
            FLAG32      m_fUserSession:1;                   // f  user session
            FLAG32      m_fAfterFirstBT:1;                  // f  for redo only
            FLAG32      m_fRecoveringEndAllSessions:1;      // f  session used for cleanup at end of recovery
            FLAG32      m_fOLD:1;                           // f  session for online defrag
            FLAG32      m_fSystemCallback:1;                // f  this is internal session being used for a callback
            FLAG32      m_fCIMCommitted:1;
            FLAG32      m_fCIMDirty:1;
            FLAG32      m_fSetAttachDB:1;                   //    set up attachdb.
            FLAG32      m_fUseSessionContextForTrxContext:1;
            FLAG32      m_fBegin0Logged:1;                  //    begin transaction has logged
            FLAG32      m_fLGWaiting:1;                     //    waiting for log to write
            FLAG32      m_fReadOnlyTrx:1;                   //    session is in a read-only transaction
            FLAG32      m_fBatchIndexCreation:1;            //    session is in multi-threaded batch index creation
            FLAG32      m_fDistributedTrx:1;                //    session is in a distributed transaction
            FLAG32      m_fPreparedToCommitTrx:1;           //    session has issued a prepare-to-commit of a distributed transaction
            FLAG32      m_fOLD2:1;                          //    session is for OLD2
            FLAG32      m_fMustRollbackToLevel0:1;          //    session must rollback to level 0 before being able to commit
            FLAG32      m_fDBScan:1;                        //    session is for DBSCAN
#ifdef DEBUG
            FLAG32      m_fUpdatingExtentPageCountCache:1;  //    session is currently updating the cached CPG values in the catalog
#endif
        };
    };

public:

    LONG                m_cInJetAPI;            //  indicates if the session is in use by JET
//  32 / 40 bytes

    RCE                 *prceNewest;            // f  newest RCE of session

    FUCB                *pfucbOfSession;        // l  list of active fucb of this thread

    PROCID              procid;                 // f  thread id
//  42 / 58 bytes

    USHORT              rgcdbOpen[dbidMax];             // f  counter for open databases
//  56 / 72 bytes

    TLS                 *ptlsApi;

    PIB                 *ppibNext;              // l  PIB list

private:
    LEVEL               m_level;                // id (!=levelNil) transaction level of this session

public:
    LEVEL               levelBegin;             // f  transaction level when first begin transaction operation
    LEVEL               clevelsDeferBegin;      // f  count of deferred open transactions
    LEVEL               levelRollback;          // f  transaction level which must be rolled back

    LONG                grbitCommitDefault;

    UPDATEID            updateid;               //  the update that this PIB is currently on

    ULONG               cCursors;               // number of cursors currently opened by this session
//  80 / 104 bytes

    //  a doubly-linked list of PIBs in trxBegin0 order partitioned by processor
    //  through this instance's PLS
    CInvasiveList< PIB, OffsetOfTrxOldestILE >::CElement
                        m_ileTrxOldest;

    LGPOS               lgposStart;             // f  log time
    LGPOS               lgposCommit0;           // f  level 0 precommit record position.
//  104 / 136 bytes

    //  this is the instance PLS containing the head of the list ordered by
    //  trxBegin0 of which we may be a member (see m_ileTrxOldest above)
    INST::PLS*          m_pplsTrxOldest;

private:
    DWORD_PTR           dwSessionContext;
    DWORD_PTR           dwSessionContextThreadId;

private:
    VOID                *m_pvRecordFormatConversionBuffer;
//  120 / 168 bytes

public:
    CCriticalSection    critCursors;            // protects session's FUCB list
    CCriticalSection    critConcurrentDDL;
    CCriticalSection    critLogBeginTrx;
    CAutoResetSignal    asigWaitLogWrite;
//  136 / 200 bytes 

    PIB                 *ppibNextWaitWrite;
    PIB                 *ppibPrevWaitWrite;
    LGPOS               lgposWaitLogWrite;

    PIB                 *ppibHashOverflow;

    TLS                 *ptlsTrxBeginLast;
//  160 / 232 bytes

private:
    // these structures are used during recovery

    // a list of RCEIDs that we haven't created yet
    CRedBlackTree<RCEID,PGNO>   m_redblacktreeRceidDeferred;

    // during recovery we need to insert RCEs into the session
    // list in an out-of-order fashion. this red-black
    // tree tracks the already inserted RCEs so we can insert
    // into the list quickly
    CRedBlackTree<RCEID,RCE*>   m_redblacktreePrceOfSession;

private:
    MACRO               *m_pMacroNext;

    ERR                 m_errRollbackFailure;
//  172 / 264 bytes

    TrxidStack          m_trxidstack;
//  376 / 472 bytes

    BYTE                m_cbClientCommitContextGeneric;
    BYTE                m_rgbClientCommitContextGeneric[70];

    BYTE                m_fCommitContextContainsCustomerData;
//  448 / 544 bytes

    UserTraceContext    m_utc;              // Packs together OPERATION_CONTEXT, correlation ID, and exchange specific tracing information
    TICK                m_tickLevel0Begin;
//  472 / 572 bytes

    volatile BOOL       m_fLoggedCheckpointGettingDeep;

    CCriticalSection    m_critCachePriority;            // Protects m_pctCachePriority and m_rgpctCachePriority
    WORD                m_pctCachePriority;             // Default cache priority for this PIB.
    WORD                m_rgpctCachePriority[dbidMax];  // Resolved (i.e., taking instance, FMP and PIB cache priorities into account) cache priority for each open database.
                                                        // Consumers of cache priority get cache priority preferrably from this.

    ULONG               m_grbitUserIoPriority;          // The user specified JET_sesparamIOPriority flags.
    ULONG               m_qosIoPriority;                // The converted QOS flags to pass to IO functions.

//  504 / 608 bytes

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

    // Basic trx context, and session context setting, checking, etc.
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

#ifdef DEBUG
    // Debug only marker that we're currently updating the cache.  Used only in asserts.
    BOOL                FUpdatingExtentPageCountCache() const               { return m_fUpdatingExtentPageCountCache; }
    VOID                SetFUpdatingExtentPageCountCache()                  { m_fUpdatingExtentPageCountCache = fTrue;  }
    VOID                ResetFUpdatingExtentPageCountCache()                { m_fUpdatingExtentPageCountCache = fFalse; }
#endif

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
#endif  //  DEBUGGER_EXTENSION
};

//  Be conscious of the size if you're changing it ...
#ifdef _WIN64
C_ASSERT( sizeof(PIB) == 608 );
#else  //  !_WIN64
C_ASSERT( sizeof(PIB) == 504 );
#endif  //  _WIN64

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

    //  allocate another MACRO
    //
    pMacro = (MACRO *) PvOSMemoryHeapAlloc( sizeof( MACRO ) );
    if ( NULL == pMacro )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    //  set dbtime
    //
    memset( pMacro, 0, sizeof( MACRO ) );
    pMacro->SetDbtime( dbtime );

    //  link macro to list of macros in session
    //
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
                //  only macro in session -- leave for later use
                //
                pMacro->SetDbtime( dbtimeNil );
                pMacro->ResetBuffer();
            }
            else
            {
                //  remove from list and release
                //
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

        // to don't overflow (we are talking unsigned here)
        // the result should be bigger then any of the 2 values
        //
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


//  ================================================================
INLINE ERR PIB::ErrAllocPvRecordFormatConversionBuffer()
//  ================================================================
{
    Assert( NULL == m_pvRecordFormatConversionBuffer );
    if( NULL == ( m_pvRecordFormatConversionBuffer = PvOSMemoryPageAlloc( g_cbPage, NULL ) ) )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }
    return JET_errSuccess;
}

//  ================================================================
INLINE VOID PIB::FreePvRecordFormatConversionBuffer()
//  ================================================================
{
    if ( NULL != m_pvRecordFormatConversionBuffer )
    {
        OSMemoryPageFree( m_pvRecordFormatConversionBuffer );
    }
    m_pvRecordFormatConversionBuffer = NULL;
}

//  ================================================================
INLINE ERR PIB::ErrSetClientCommitContextGeneric( const void * const pvCtx, const INT cbCtx )
//  ================================================================
{
    if ( cbCtx > sizeof(m_rgbClientCommitContextGeneric) )
    {
        return ErrERRCheck( JET_errInvalidBufferSize );
    }

    Assert( cbCtx < 256 );  // size of ppib->m_cbClientCommitContextGeneric
    m_cbClientCommitContextGeneric = (BYTE)cbCtx;

    if ( cbCtx )
    {
        memcpy( m_rgbClientCommitContextGeneric, pvCtx, cbCtx );
    }

    return JET_errSuccess;
}

//  ================================================================
INLINE ERR PIB::ErrSetCommitContextContainsCustomerData( const void * const pvCtx, const INT cbCtx )
//  ================================================================
{
    if ( cbCtx != sizeof( DWORD ) )
    {
        return ErrERRCheck( JET_errInvalidBufferSize );
    }

    m_fCommitContextContainsCustomerData = !!*(DWORD *)pvCtx;
    return JET_errSuccess;
}

//  =======================================================================

//  ================================================================
INLINE ERR PIB::ErrSetOperationContext( const void * const pvCtx, const INT cbCtx )
//  ================================================================
{
    static_assert( sizeof( JET_OPERATIONCONTEXT ) == sizeof( OPERATION_CONTEXT ), "JET_OPERATIONCONTEXT and OPERATION_CONTEXT should be the same size" );

    if ( cbCtx != sizeof( JET_OPERATIONCONTEXT ) )
    {
        return ErrERRCheck( JET_errInvalidBufferSize );
    }

    memcpy( &m_utc.context, pvCtx, cbCtx );
    return JET_errSuccess;
}

//  ================================================================
INLINE ERR PIB::ErrSetCorrelationID( const void *pvID, const INT cbID )
//  ================================================================
{
    if ( cbID != sizeof( m_utc.dwCorrelationID ) )
    {
        return ErrERRCheck( JET_errInvalidBufferSize );
    }

    static_assert( sizeof( m_utc.dwCorrelationID ) == sizeof( DWORD ), "Correlation ID should be a DWORD" );
    m_utc.dwCorrelationID = *( (DWORD *) pvID );
    return JET_errSuccess;
}

//  ================================================================
INLINE ERR PIB::ErrSetIOSESSTraceFlags( const void *pvFlags, const INT cbFlags )
//  ================================================================
{
    if ( cbFlags != sizeof( m_utc.dwIOSessTraceFlags ) )
    {
        return ErrERRCheck( JET_errInvalidBufferSize );
    }

    static_assert( sizeof( m_utc.dwIOSessTraceFlags ) == sizeof( DWORD ), "dwIOSessTraceFlags should be a DWORD" );
    m_utc.dwIOSessTraceFlags = *( (DWORD *) pvFlags );
    return JET_errSuccess;
}

//  ================================================================
INLINE ERR PIB::ErrSetClientActivityID( const void *pvID, const INT cbID )
//  ================================================================
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

//  ================================================================
INLINE ERR PIB::ErrSetClientComponentDesc( const void *pvClientComponent, const INT cb )
//  ================================================================
{
    ERR err = JET_errSuccess;
    CallR( m_utc.ErrLazyInitUserContextDesc() );
    return USER_CONTEXT_DESC::ErrCopyDescString( m_utc.pUserContextDesc->szClientComponent, (const char*) pvClientComponent, cb );
}

//  ================================================================
INLINE ERR PIB::ErrSetClientActionDesc( const void *pvClientAction, const INT cb )
//  ================================================================
{
    ERR err = JET_errSuccess;
    CallR( m_utc.ErrLazyInitUserContextDesc() );
    return USER_CONTEXT_DESC::ErrCopyDescString( m_utc.pUserContextDesc->szClientAction, (const char*) pvClientAction, cb );
}

//  ================================================================
INLINE ERR PIB::ErrSetClientActionContextDesc( const void *pvClientActionContext, const INT cb )
//  ================================================================
{
    ERR err = JET_errSuccess;
    CallR( m_utc.ErrLazyInitUserContextDesc() );
    return USER_CONTEXT_DESC::ErrCopyDescString( m_utc.pUserContextDesc->szClientActionContext, (const char*) pvClientActionContext, cb );
}

//  ================================================================
INLINE ULONG_PTR PIB::PctCachePriorityPibDbid( const DBID dbid ) const
//  ================================================================
{
    Assert( dbid < _countof( m_rgpctCachePriority ) );

    const ULONG_PTR pctCachePriority = m_rgpctCachePriority[ dbid ];
    Assert( FIsCachePriorityValid( pctCachePriority ) );

    return pctCachePriority;
}

//  ================================================================
INLINE ERR PIB::ErrSetUserIoPriority( const void* const pvUserFlags, const INT cbFlags )
//  ================================================================
{
    ERR err = JET_errSuccess;

    if ( cbFlags != sizeof( JET_GRBIT ) )
    {
        CallR( ErrERRCheck( JET_errInvalidBufferSize ) );
    }

    JET_GRBIT grbitUserIoPriFlagsUnprocessed = *( (JET_GRBIT *) pvUserFlags );
    ULONG qosIo = 0x0;

    //  Double check the mapping of JET API bits match the BF-API pass through masks, and
    //  match the OS-level IO QOS bits.
    //
    // Note that JET_IOPriorityLow != qosIOOSLowPriority so that needs to be translated.
    

    //  First translate (any non-pass-thru bits)
    
    if ( grbitUserIoPriFlagsUnprocessed & JET_IOPriorityLow )
    {
        grbitUserIoPriFlagsUnprocessed &= ~JET_IOPriorityLow;
        qosIo |= qosIOOSLowPriority;
    }

    qosIo |= ( grbitUserIoPriFlagsUnprocessed & ( JET_IOPriorityUserClassIdMask | JET_IOPriorityMarkAsMaintenance ) );
    grbitUserIoPriFlagsUnprocessed &= ~JET_IOPriorityUserClassIdMask;
    grbitUserIoPriFlagsUnprocessed &= ~JET_IOPriorityMarkAsMaintenance;

    //  We allow pass through for a limited number of flags as we flight this feature experimentally.
    qosIo |= ( grbitUserIoPriFlagsUnprocessed & qosIODispatchBackground );
    grbitUserIoPriFlagsUnprocessed &= ~qosIODispatchBackground;

    if ( grbitUserIoPriFlagsUnprocessed != 0 )
    {
        AssertSz( fFalse, "Client passed these flags 0x%x in for User IoPriority flags that ESE does not understand.", grbitUserIoPriFlagsUnprocessed );
        CallR( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    // we've successfully converted all User IoPriority flags into IO QOS flags, so update pib members ...

    m_grbitUserIoPriority = *( (JET_GRBIT *) pvUserFlags );
    m_qosIoPriority |= qosIo;

    return JET_errSuccess;
}

//  ================================================================
INLINE BFPriority PIB::BfpriPriority( const IFMP ifmpOverride ) const
//  ================================================================
{
    Assert( ifmpOverride > 0 );
    Assert( ifmpOverride != ifmpNil );

    //  If this function / line shows up in performance traces, we could pre-compute the 
    //  entire BFPriority and store it on PIB, so it just gets passed back.
    return BfpriBFMake( PctCachePriorityPibDbid( DbidFromIfmp( ifmpOverride ) ), QosIoPriority() );
}


//  ================================================================
INLINE PIBTraceContextScope PIB::InitTraceContextScope() const
//  ================================================================
{
    // For any threads entering the JET API through APICALL_SESID, the current user context in the TLS should point to the session's context
    Assert( Putc() == PutcTLSGetUserContext() || m_cInJetAPI == 0 );

    return PIBTraceContextScope( this );
}

//  ================================================================
INLINE void PIB::SetUserTraceContextInTls() const
//  ================================================================
{
    const UserTraceContext* ptcPrev = TLSSetUserTraceContext( Putc() );
    Assert( NULL == ptcPrev );
}

//  ================================================================
INLINE void PIB::ClearUserTraceContextInTls() const
//  ================================================================
{
    const UserTraceContext* ptcPrev = TLSSetUserTraceContext( NULL );
    Assert( ptcPrev == Putc() );
}


//  =======================================================================

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

/*  PIB validation
/**/
#define CheckPIB( ppib )    Assert( (ppib)->Level() < levelMax )

#define ErrPIBCheck( ppib ) JET_errSuccess

#define ErrPIBCheckIfmp( ppib, ifmp )                               \
    ( ( FInRangeIFMP( ifmp ) && ( ppib->FSystemCallback() || FPIBUserOpenedDatabase( ppib, g_rgfmp[ifmp].Dbid() ) ) ) \
        ? JET_errSuccess : ErrERRCheck( JET_errInvalidDatabaseId ) )

#ifdef DEBUG


//  ================================================================
INLINE VOID PIB::AssertValid( ) const
//  ================================================================
{
    CResource::AssertValid( JET_residPIB, this );
    Assert( m_level < levelMax );
}


#endif  //  DEBUG

#define FPIBActive( ppib )                      ( (ppib)->level != levelNil )

/*  prototypes
/**/
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

// increment trxid by 4 each time - this is because no valid trxid can be trxMax (special value) and we need space
// between normal trxids for the special read-from-passive trxids
#define TRXID_INCR  4

//  ================================================================
INLINE VOID PIBSetTrxBegin0( PIB * const ppib )
//  ================================================================
//
//  Used when a transaction starts from level 0 or refreshes
//
//-
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
    // collect lgpos for debugging purpose
    ppib->lgposTrxBegin0 = pinst->m_plog->LgposLGLogTipNoLock();

    ppib->m_pplsTrxOldest = ppls;
#ifdef DEBUG
    // This trxBegin0 better not be older than the trxBegin0 of the first session on the invasive list or
    // TrxOldest calculation will be busted
    PIB* const ppibTrxOldest = ppls->m_ilTrxOldest.PrevMost();
    Assert ( !ppibTrxOldest || ( INT( ppibTrxOldest->trxBegin0 - ppib->trxBegin0 ) <= 0 ) );
#endif
    // Oldest transaction can only change if this is the first transaction
    if ( ppls->m_ilTrxOldest.PrevMost() == NULL )
    {
        pinst->SetTrxOldestCachedMayBeStale();
    }
    ppls->m_ilTrxOldest.InsertAsNextMost( ppib );
    ppls->m_rwlPIBTrxOldest.LeaveAsWriter();

    ppib->trxCommit0 = trxMax;
}

//  ================================================================
INLINE VOID PIBResetTrxBegin0( PIB * const ppib )
//  ================================================================
//
//  Used when a transaction commits or rollsback to level 0
//
//-
{
    Assert( 0 == ppib->Level() );
    Assert( NULL == ppib->prceNewest );

    INST::PLS* const ppls = ppib->m_pplsTrxOldest;
    if ( ppls != NULL )
    {
        ppls->m_rwlPIBTrxOldest.EnterAsWriter();
        // Oldest transaction can only change if this is the first transaction
        if ( ppls->m_ilTrxOldest.PrevMost() == ppib )
        {
            PinstFromPpib( ppib )->SetTrxOldestCachedMayBeStale();
        }
        ppls->m_ilTrxOldest.Remove( ppib );
        ppls->m_rwlPIBTrxOldest.LeaveAsWriter();
    }

    ppib->trxBegin0     = trxMax;
    ppib->lgposStart    = lgposMax;
    ppib->ResetFReadOnlyTrx();
    ppib->ResetLoggedCheckpointGettingDeep();

//  trxBegin0 is no longer unique, due to unversioned AddColumn/CreateIndex
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

INLINE DWORD_PTR /* Technically a DWORD */ PIB::TidActive() const
{
    //  Normally we use the TID for the trx context, except when there is an explicit user session context, in
    //  which case the TID gets displaced to dwSessionContextThreadId.  Yes, it is confusing.
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
        //  TrxContext is not properly maintained during recovery (not all
        //  places that could begin a transaction set the TrxContext)
        //
        //  TrxContext also not properly maintained during batch index creations
        //  as multiple worker threads may be using the same PIB
        //  WARNING: may even make callbacks (eg. jcb.dll) and have multiple
        //  threads begin transactions and call back into Jet using the same
        //  session, but I believe it is safe because they are all calling
        //  back into Jet under the context of batch index creation
        //
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

//  We use this to prepare for rollback from end session which might be happening from 
//  term on another thread.

INLINE VOID PIB::PrepareTrxContextForEnd()
{
    if ( m_fUseSessionContextForTrxContext )
    {
        Assert( !m_pinst->FRecovering() );

        //  fake out TrxContext to allow us to rollback on behalf of another thread
        dwTrxContext = dwSessionContext;
    }
    else
    {
        //  not using SessionContext model, so if the transaction wasn't started by
        //  this thread, the rollback call below will err out with SessionSharingViolation
    }
}

INLINE BOOL PIB::FPIBCheckTrxContext() const
{
    if ( m_pinst->FRecovering() || this->FBatchIndexCreation() )
    {
        //  TrxContext is not properly maintained during recovery (not all
        //  places that could begin a transaction set the TrxContext)
        //
        //  TrxContext also not properly maintained during batch index creations
        //  as multiple worker threads may be using the same PIB
        //  WARNING: may even make callbacks (eg. jcb.dll) and have multiple
        //  threads begin transactions and call back into Jet using the same
        //  session, but I believe it is safe because they are all calling
        //  back into Jet under the context of batch index creation
        //
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
    //  Not done from regular PIB thread, only in checkpoint update, so make sure this PIB won't go away.
    Assert( m_pinst->m_critPIB.FOwner() );
    Expected( critLogBeginTrx.FOwner() );
    BOOL fCheck = AtomicCompareExchange( (LONG*)&m_fLoggedCheckpointGettingDeep, fFalse, fTrue );
    if ( fCheck == fFalse )
    {
        //  we won ...
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
        //  can go off from PIBReset / Commit / Rollback IF checkpoint update calls this func because it lost a
        //  super narrow race (in a RARE error condition).  In practice, nearly impossible.
        Expected( fCheck == fTrue );
    }
}

//  ================================================================
INLINE ERR ErrPIBCheckUpdatable( const PIB * ppib )
//  ================================================================
{
    ERR     err;

    if ( ppib->FReadOnlyTrx() )
        err = ErrERRCheck( JET_errTransReadOnly );
    else
        err = JET_errSuccess;

    return err;
}


/*  JET API flags
/**/
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

        enum    { csessionsMax = 0xFFFF };      //  max size of hash table

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
    // Get the TraceContext from the TLS pointer cached on the PIB
    // Eliminates the call to GetTlsValue() which is significant CPU on AD cpu traces
    return m_ppib->ptlsApi != NULL ?
        const_cast<TraceContext*>( PetcTLSGetEngineContextCached( m_ppib->ptlsApi ) ) :
        const_cast<TraceContext*>( PetcTLSGetEngineContext() );
}

