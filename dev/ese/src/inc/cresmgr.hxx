// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef CRESMGR_H_INCLUDED
#define CRESMGR_H_INCLUDED

//  Classed declared in this header
class CQuota;
class CResource;
#ifdef DEBUG
class CProfileStorage;
class CProfileCounter;
#endif // DEBUG

//=============================================================================
//  NEW_CRESMGR NEW_CRESMGR NEW_CRESMGR NEW_CRESMGR NEW_CRESMGR NEW_CRESMGR
//=============================================================================

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

//  global resource managers
//
extern CResource    RESINST;
extern CResource    RESLOG;
extern CResource    RESSPLIT;
extern CResource    RESSPLITPATH;
extern CResource    RESMERGE;
extern CResource    RESMERGEPATH;
extern CResource    RESKEY;
extern CResource    RESBOOKMARK;
extern CResource    RESLRUKHIST;


//==============================================================================
class CQuota
//  sets qouta limit object. Simplified version of the semaphore which is easier
//  to be reset at the end. (Does not need to aquire all the releases).
{
private:
    volatile LONG   m_cQuotaFree;   //  semaphore like to control the quotas
                                    //  has negative value when is not initialized or is terminated
    volatile LONG   m_cQuota;       //  quota max
                                    //  has negative value when the quota is disabled

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
#endif // DEBUGGER_EXTENSION

private:
    //  Forbidden constructors
            CQuota( CQuota const & );
            CQuota &operator=( CQuota const & );
};


class CResourceManager;
const INT cbRESHeader = 0x20;   //  Section overhead on each section boundary.
                                //  Caps the max size of single object allocated
                                //  by the Resource Manager.
                                //  Must be cbCacheLine aligned

template < class TData, BOOL fHashPerProc > class PERFInstance;

//==============================================================================
class CResource
//  The interface to the resource manager
{
    CResourceManager    *m_pRM;
    INST                *m_pinst;
    CQuota              m_quota;

public:
    INLINE          CResource( INST * pinst = NULL );
    INLINE          ~CResource();
            ERR     ErrSetParam( JET_RESOPER resop, DWORD_PTR dwParam );
                //  some of the parameter will be passed to the resource manager
            ERR     ErrGetParam( JET_RESOPER resop, DWORD_PTR * const pdwParam ) const;
                //  some of the parameter will be passed to the resource manager
            ERR     ErrInit( JET_RESID resid);
            VOID    Term();
#ifdef MEM_CHECK
            #define PvRESAlloc() PvAlloc_( __FILE__, __LINE__ )
            #define PvRESAlloc_( szFile, lLine ) PvAlloc_( szFile, lLine )
            VOID    *PvAlloc_( __in PCSTR szFile, LONG lLine );
#else  //  !MEM_CHECK
            #define PvRESAlloc() PvAlloc_()
            #define PvRESAlloc_( szFile, lLine ) PvAlloc_()
            VOID    *PvAlloc_();
#endif  //  MEM_CHECK
#ifdef DEBUG
            VOID    AssertValid( const VOID * const pv );
    static  VOID    AssertValid( const JET_RESID resid, const VOID * const pv );
#else // DEBUG
            VOID    AssertValid( const VOID * const pv ) {}
    static  VOID    AssertValid( const JET_RESID resid, const VOID * const pv ) {}
#endif // DEBUG
            VOID    Free( VOID * const );

            BOOL    FCloseToQuota();
    static  BOOL    FCallingProgramPassedValidJetHandle( _In_ const JET_RESID resid, _In_ const VOID * const pv );

#ifdef DEBUGGER_EXTENSION
    VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const;
#endif // DEBUGGER_EXTENSION

#ifdef RTM
#else // RTM
    static VOID UnitTest();
#endif // RTM

private:
    //  Forbidden constructors
            CResource( CResource const & );
            CResource &operator=( CResource const & );

};

#ifdef DEBUG
    VOID AssertValid( JET_RESID, const VOID * const pv );
#else // DEBUG
INLINE VOID AssertValid( JET_RESID, const VOID * const pv ) {}
#endif // DEBUG

//=============================================================================
//  INLINE funcions
//=============================================================================

//======================================
//  Class CQuota

//======================================
INLINE CQuota::CQuota()
    :   m_cQuotaFree( -1 ),
        m_cQuota( QUOTA_MAX )
{
}


//======================================
INLINE CQuota::~CQuota()
{
    Assert( m_cQuotaFree < 0 );
}


//======================================
INLINE LONG CQuota::GetQuota() const
//  Returns the quota max value
//
//  If the quota is disabled then QUOTA_MAX is returned
{
    return m_cQuota < 0 ? QUOTA_MAX : m_cQuota;
}


//======================================
INLINE LONG CQuota::GetQuotaFree() const
//  Returns the quota free value
//
//  If the quota is disabled then QUOTA_MAX is returned
{
    return m_cQuota < 0 ? QUOTA_MAX : m_cQuotaFree;
}


//======================================
INLINE BOOL CQuota::FAcquire()
//  Acquires a single unit of quota
//
//  If the quota is QUOTA_MAX or is disabled then we automatically succeed
{
    //  if the current quota is QUOTA_MAX or disabled (negative) then we
    //  automatically succeed
    //
    //  NOTE:  this only works if QUOTA_MAX is lMax / LONG-MAX
    //
    Assert( QUOTA_MAX == lMax );
    if ( (ULONG)m_cQuota >= QUOTA_MAX )
    {
        return fTrue;
    }

    //  atomically decrement the available quota only if the result will not be
    //  negative
    //
    OSSYNC_FOREVER
    {
        //  get the current available quota
        //
        const LONG cQuotaFree           = m_cQuotaFree;

        //  this function has no effect on 0x80000000, so this MUST be an illegal
        //  value!
        //
        Assert( cQuotaFree != 0x80000000 );

        //  munge end value such that the transaction will only work if we are in
        //  mode 0 and we have at least one available count (we do this to save a
        //  branch)
        //
        const LONG cQuotaFreeAI         = ( cQuotaFree - 1 ) & 0x7FFFFFFF;

        //  compute start value relative to munged end value
        //
        const LONG cQuotaFreeBIExpected = cQuotaFreeAI + 1;

        //  validate transaction
        //
        Assert( cQuotaFree <= 0 || ( cQuotaFreeBIExpected > 0 && cQuotaFreeAI >= 0 && cQuotaFreeAI == cQuotaFree - 1 ) );

        //  attempt the transaction
        //
        const LONG cQuotaFreeBI = AtomicCompareExchange( (LONG *)&m_cQuotaFree, cQuotaFreeBIExpected, cQuotaFreeAI );

        //  the transaction succeeded
        //
        if ( cQuotaFreeBI == cQuotaFreeBIExpected )
        {
            return fTrue;
        }

        //  the transaction failed
        //
        else
        {
            //  the transaction failed because of a collision with another context
            //
            if ( cQuotaFreeBIExpected > 0 )
            {
                continue;
            }

            //  the transaction failed because there are no available counts
            //
            else
            {
                return fFalse;
            }
        }
    }
}


//======================================
INLINE VOID CQuota::Release()
//  Releases a single unit of quota
//
//  If the quota is QUOTA_MAX or is disabled then this is a no op
{
    //  if the current quota is QUOTA_MAX or disabled (negative) then we
    //  don't need to do anything
    //
    //  NOTE:  this only works if QUOTA_MAX is lMax / LONG-MAX
    //
    Assert( QUOTA_MAX == lMax );
    if ( (ULONG)m_cQuota >= QUOTA_MAX )
    {
        return;
    }

    const LONG cQuotaFreeAI = AtomicIncrement( (LONG *)&m_cQuotaFree );
    Assert( cQuotaFreeAI > 0 );
    Assert( cQuotaFreeAI <= m_cQuota );
}

//======================================
//  Class CResource

INLINE CResource::CResource( INST * pinst ) :
    m_pRM( NULL ),
    m_pinst( pinst )
    {}

INLINE CResource::~CResource() { Term(); }


#ifdef DEBUG
//  profiling stuff used for debugging purposes

enum { PROFILEMODE_NONE         = 0,
        PROFILEMODE_COUNT       = 0x1,  //  log each call
        PROFILEMODE_TIME        = 0x2,  //  log the time of the call
        PROFILEMODE_TIMING      = 0x4,  //  log the lenght of the operation
        PROFILEMODE_SUMMARY     = 0x8,  //  print summary at the end
        PROFILEMODE_LOG_JET_OP  = 0x10 };   //  profile ESE APIs
                //  also you have to define PROFILE_JET_API in order to work
                //  with ESE APIs. Does not log failures at JET API level

//==============================================================================
class CProfileStorage
//  contains total information for single profile object
//  must be a global value in order to work properly. Automatically is added to
//  the profile storage objects list
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
            //  sets the log name of the profile object
    VOID    Update( const HRT hrtStart, const HRT dhrtExecute );
            //  updates and possibly prints data for the profile object
    VOID    Summary();
            //  prints summary at the end.

    static ERR ErrInit( const char * const szFileName, const INT iMode );
    static VOID Term();
            //  prints all the summaries if neccessary
    static INT Mode() { return iMode; }
    static VOID Print( const char * const sz );
            //  prints arbitrary text into the profile file

private:
    static FILE *pFile;
    static INT iMode;
    static LONG volatile g_iPSIndex;
    static QWORD csecTimeOffset;
    static CProfileStorage *pPSFirst;
    static CProfileStorage * volatile pPSLast;
};

//==============================================================================
class CProfileCounter
//  Local class used for specific profile measurement. The interface to the
//  profile storage
{
    HRT     m_dhrtExecute;
    HRT     m_hrtStart;
    CProfileStorage *m_pPS;

public:
            CProfileCounter( CProfileStorage *pPS, BOOL fAutoStart = fTrue );
            CProfileCounter( CProfileStorage *pPS, const char * const szName );
            ~CProfileCounter();
    VOID    Start();    //  starts the measurement. Usually autostarted by the
                        //      constructor
    VOID    Pause();    //  does not lose collected data for this run
                        //      and does not update the storage
    VOID    Continue(); //  continues after pause
    VOID    Stop();     //  completes the measurement and updates the storage
};
#endif // DEBUG

#endif // CRESMGR_H_INCLUDED

