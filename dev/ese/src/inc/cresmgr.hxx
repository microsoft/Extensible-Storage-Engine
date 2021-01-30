// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef CRESMGR_H_INCLUDED
#define CRESMGR_H_INCLUDED

class CQuota;
class CResource;
#ifdef DEBUG
class CProfileStorage;
class CProfileCounter;
#endif


ERR ErrRESSetResourceParam(
        INST * const pinst,
        JET_RESID resid,
        JET_RESOPER resoper,
        DWORD_PTR ulParam );

ERR ErrRESGetResourceParam(
        INST * const pinst,
        JET_RESID resid,
        JET_RESOPER resoper,
        DWORD_PTR * const pdwParam );

extern CResource    RESINST;
extern CResource    RESLOG;
extern CResource    RESSPLIT;
extern CResource    RESSPLITPATH;
extern CResource    RESMERGE;
extern CResource    RESMERGEPATH;
extern CResource    RESKEY;
extern CResource    RESBOOKMARK;
extern CResource    RESLRUKHIST;


class CQuota
{
private:
    volatile LONG   m_cQuotaFree;
    volatile LONG   m_cQuota;

public:
    enum { QUOTA_MAX = lMax };

public:
    INLINE          CQuota();
    INLINE          ~CQuota();

            ERR     ErrSetQuota( const LONG cQuota );
            ERR     ErrEnableQuota( const BOOL fEnable );

            LONG    GetQuota() const;
            LONG    GetQuotaFree() const;

            ERR     ErrInit();
            VOID    Term();

    INLINE  BOOL    FAcquire();
    INLINE  VOID    Release();

#ifdef DEBUGGER_EXTENSION
    VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const;
#endif

private:
            CQuota( CQuota const & );
            CQuota &operator=( CQuota const & );
};


class CResourceManager;
const INT cbRESHeader = 0x20;

template < class TData, BOOL fHashPerProc > class PERFInstance;

class CResource
{
    CResourceManager    *m_pRM;
    INST                *m_pinst;
    CQuota              m_quota;

public:
    INLINE          CResource( INST * pinst = NULL );
    INLINE          ~CResource();
            ERR     ErrSetParam( JET_RESOPER resop, DWORD_PTR dwParam );
            ERR     ErrGetParam( JET_RESOPER resop, DWORD_PTR * const pdwParam ) const;
            ERR     ErrInit( JET_RESID resid);
            VOID    Term();
#ifdef MEM_CHECK
            #define PvRESAlloc() PvAlloc_( __FILE__, __LINE__ )
            #define PvRESAlloc_( szFile, lLine ) PvAlloc_( szFile, lLine )
            VOID    *PvAlloc_( __in PCSTR szFile, LONG lLine );
#else
            #define PvRESAlloc() PvAlloc_()
            #define PvRESAlloc_( szFile, lLine ) PvAlloc_()
            VOID    *PvAlloc_();
#endif
#ifdef DEBUG
            VOID    AssertValid( const VOID * const pv );
    static  VOID    AssertValid( const JET_RESID resid, const VOID * const pv );
#else
            VOID    AssertValid( const VOID * const pv ) {}
    static  VOID    AssertValid( const JET_RESID resid, const VOID * const pv ) {}
#endif
            VOID    Free( VOID * const );

            BOOL    FCloseToQuota();
    static  BOOL    FCallingProgramPassedValidJetHandle( _In_ const JET_RESID resid, _In_ const VOID * const pv );

#ifdef DEBUGGER_EXTENSION
    VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const;
#endif

#ifdef RTM
#else
    static VOID UnitTest();
#endif

private:
            CResource( CResource const & );
            CResource &operator=( CResource const & );

};

#ifdef DEBUG
    VOID AssertValid( JET_RESID, const VOID * const pv );
#else
INLINE VOID AssertValid( JET_RESID, const VOID * const pv ) {}
#endif



INLINE CQuota::CQuota()
    :   m_cQuotaFree( -1 ),
        m_cQuota( QUOTA_MAX )
{
}


INLINE CQuota::~CQuota()
{
    Assert( m_cQuotaFree < 0 );
}


INLINE LONG CQuota::GetQuota() const
{
    return m_cQuota < 0 ? QUOTA_MAX : m_cQuota;
}


INLINE LONG CQuota::GetQuotaFree() const
{
    return m_cQuota < 0 ? QUOTA_MAX : m_cQuotaFree;
}


INLINE BOOL CQuota::FAcquire()
{
    Assert( QUOTA_MAX == lMax );
    if ( (ULONG)m_cQuota >= QUOTA_MAX )
    {
        return fTrue;
    }

    OSSYNC_FOREVER
    {
        const LONG cQuotaFree           = m_cQuotaFree;

        Assert( cQuotaFree != 0x80000000 );

        const LONG cQuotaFreeAI         = ( cQuotaFree - 1 ) & 0x7FFFFFFF;

        const LONG cQuotaFreeBIExpected = cQuotaFreeAI + 1;

        Assert( cQuotaFree <= 0 || ( cQuotaFreeBIExpected > 0 && cQuotaFreeAI >= 0 && cQuotaFreeAI == cQuotaFree - 1 ) );

        const LONG cQuotaFreeBI = AtomicCompareExchange( (LONG *)&m_cQuotaFree, cQuotaFreeBIExpected, cQuotaFreeAI );

        if ( cQuotaFreeBI == cQuotaFreeBIExpected )
        {
            return fTrue;
        }

        else
        {
            if ( cQuotaFreeBIExpected > 0 )
            {
                continue;
            }

            else
            {
                return fFalse;
            }
        }
    }
}


INLINE VOID CQuota::Release()
{
    Assert( QUOTA_MAX == lMax );
    if ( (ULONG)m_cQuota >= QUOTA_MAX )
    {
        return;
    }

    const LONG cQuotaFreeAI = AtomicIncrement( (LONG *)&m_cQuotaFree );
    Assert( cQuotaFreeAI > 0 );
    Assert( cQuotaFreeAI <= m_cQuota );
}


INLINE CResource::CResource( INST * pinst ) :
    m_pRM( NULL ),
    m_pinst( pinst )
    {}

INLINE CResource::~CResource() { Term(); }


#ifdef DEBUG

enum { PROFILEMODE_NONE         = 0,
        PROFILEMODE_COUNT       = 0x1,
        PROFILEMODE_TIME        = 0x2,
        PROFILEMODE_TIMING      = 0x4,
        PROFILEMODE_SUMMARY     = 0x8,
        PROFILEMODE_LOG_JET_OP  = 0x10 };

class CProfileStorage
{
    const char          *m_szName;
    INT                 m_iIndex;
    HRT                 m_dhrtTotal;
    HRT                 m_dhrtMin;
    HRT                 m_dhrtMax;
    INT                 m_iCalls;
    CCriticalSection    m_critSummary;
    CProfileStorage     *m_pPSNext;

public:
            CProfileStorage( const char * const szName = NULL );
            ~CProfileStorage();
    VOID    SetName(const char * const szName ) { if ( NULL == m_szName ) m_szName = szName; }
    VOID    Update( const HRT hrtStart, const HRT dhrtExecute );
    VOID    Summary();

    static ERR ErrInit( const char * const szFileName, const INT iMode );
    static VOID Term();
    static INT Mode() { return iMode; }
    static VOID Print( const char * const sz );

private:
    static FILE *pFile;
    static INT iMode;
    static LONG volatile g_iPSIndex;
    static QWORD csecTimeOffset;
    static CProfileStorage *pPSFirst;
    static CProfileStorage * volatile pPSLast;
};

class CProfileCounter
{
    HRT     m_dhrtExecute;
    HRT     m_hrtStart;
    CProfileStorage *m_pPS;

public:
            CProfileCounter( CProfileStorage *pPS, BOOL fAutoStart = fTrue );
            CProfileCounter( CProfileStorage *pPS, const char * const szName );
            ~CProfileCounter();
    VOID    Start();
    VOID    Pause();
    VOID    Continue();
    VOID    Stop();
};
#endif

#endif

