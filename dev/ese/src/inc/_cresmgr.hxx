// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#ifdef DEBUG
#endif


extern DWORD_PTR cbSectionSize;
#define maskSection DWORD_PTR( 0 - cbSectionSize )

class CLookaside;
class CResourceFreeObjectList;
class CResourceChunkInfo;

template <class T> T AlignDownMask( T const x, DWORD_PTR dwAlign )
    { return (T)( DWORD_PTR( x ) & DWORD_PTR( 0 - dwAlign ) ); }

template <class T> T AlignUpMask( T const x, DWORD_PTR dwAlign )
    { return (T)( ( DWORD_PTR( x ) + dwAlign - 1 ) & DWORD_PTR( 0 - dwAlign ) ); }

class CLookaside
{
    VOID        **m_ppvData;
    INT         m_cItems;
#ifdef RM_STATISTICS
    LONG        m_cGetHitFirst;
    LONG        m_cGetHit;
    LONG        m_cGetACX;
    LONG        m_cReturnHitFirst;
    LONG        m_cReturnHit;
    LONG        m_cReturnACX;
#endif

public:
    INLINE          CLookaside();
                    ~CLookaside();
            ERR     ErrInit();
            ERR     ErrSetSize( INT cSize );
    INLINE  BOOL    FInit() const;
    INLINE  INT     CSize() const;
            VOID    Term();
            VOID    *PvGet( DWORD_PTR dwHint );
            BOOL    FReturn( DWORD_PTR dwHint, VOID * const pv );
            VOID    *PvFlush();

#ifdef DEBUGGER_EXTENSION
            VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const;
#endif

private:
            CLookaside( CLookaside const & );
            CLookaside &operator=( CLookaside const & );
};

class CResourceFreeObjectList
{
public:
    CResourceFreeObjectList * volatile m_pRFOLNext;

public:
    INLINE          CResourceFreeObjectList();

    INLINE static   VOID    RFOLAddObject(
#ifdef RM_DEFERRED_FREE
            CResourceFreeObjectList * volatile *ppRFOLHead,
            CResourceFreeObjectList * volatile *ppRFOLTail,
#else
            CResourceFreeObjectList * volatile *ppRFOL,
#endif
            VOID * const pv
            );

    INLINE static   CResourceFreeObjectList *   PRFOLRemoveObject(
#ifdef RM_DEFERRED_FREE
            CResourceFreeObjectList * volatile *ppRFOLHead,
            CResourceFreeObjectList * volatile *ppRFOLTail
#else
            CResourceFreeObjectList * volatile *ppRFOL
#endif
            );
    INLINE static   VOID    RFOLLoopCheck( CResourceFreeObjectList * volatile *ppRFOL );

private:
            CResourceFreeObjectList( CResourceFreeObjectList const & );
            CResourceFreeObjectList &operator=( CResourceFreeObjectList const & );
};

class CResourceChunkInfo
{
public:
#ifdef RM_DEFERRED_FREE
    INT                     m_cDeferredFrees;
    CResourceFreeObjectList * volatile m_pRFOLHead;
    CResourceFreeObjectList * volatile m_pRFOLTail;
#else
    CResourceFreeObjectList * volatile m_pRFOL;
#endif
    CCriticalSection        m_critAlloc;
    volatile LONG           m_cUsed;
    volatile LONG           m_cNextAlloc;
    CResourceChunkInfo      * volatile m_pRCINextNotFull;
    VOID                    *m_pvData;
    CResourceChunkInfo      *m_pRCINext;
    CResourceManager        *m_pRMOwner;
    CResourceChunkInfo      * volatile m_pRCINextFree;

public:
    INLINE      CResourceChunkInfo( CResourceManager *pRMOwner );

private:
            CResourceChunkInfo( CResourceChunkInfo const & );
            CResourceChunkInfo &operator=( CResourceChunkInfo const & );
};

class CResourceSection
{
public:
    CResourceChunkInfo *m_pRCI;
    LONG                m_cbSectionHeader;
    CHAR                m_rgchTag[JET_resTagSize];
#ifdef MEM_CHECK
    class FileLineData
    {
        public:
            CHAR * volatile szFile;
            LONG volatile lLine;
    };
    FileLineData        m_rgFLD[0];
#endif

public:
    INLINE          CResourceSection();

private:
            CResourceSection( CResourceSection const & );
            CResourceSection &operator=( CResourceSection const & );
};



class CResourceManager
{
    CLookaside          m_lookaside;
    CResourceChunkInfo  * volatile m_pRCIToAlloc;
    INT                 m_cObjectsPerChunk;
    CResourceChunkInfo  * volatile m_pRCINotFullList;
    CResourceChunkInfo  * volatile m_pRCIFreeList;
    volatile LONG       m_cFreeRCI;
    volatile LONG       m_cAllocatedRCI;
    volatile ULONG_PTR  m_cUsedObjects;
    CResourceChunkInfo  * volatile m_pRCIList;
    volatile LONG       m_cCResourceLinks;
    CCriticalSection    m_critLARefiller;
    DWORD               m_cAvgCount;

    INT                 m_cbSectionHeader;
    INT                 m_cbAlignedObject;
    INT                 m_cObjectsPerSection;
    INT                 m_cAllocatedRCIMax;
    INT                 m_cAllocatedChunksMin;
    INT                 m_1_cbAlignedObject;
public:
    enum { shfAlignedObject = 16 };
private:
    JET_RESID           m_resid;
    CHAR                m_rgchTag[JET_resTagSize];
    LONG                m_cbObjectAlign;
    LONG                m_cbObjectSize;
    DWORD_PTR           m_cbChunkSize;
    LONG                m_cbRFOLOffset;
    LONG                m_cObjectsMin;
    LONG                m_cObjectsMax;

    union
    {
        ULONG           m_ulFlags;
        struct
        {
            BOOL        m_fGuarded:1;
            BOOL        m_fAllocTopDown:1;
            BOOL        m_fPreserveFreed:1;
            BOOL        m_fAllocFromHeap:1;
        };
    };

    CCriticalSection    m_critInitTerm;

#ifdef RM_STATISTICS
    LONG                m_cAllocCalls;
    LONG                m_cRFOLWait;
    LONG                m_cFreeCalls;
    LONG                m_cWaitAllocTries;
    LONG                m_cWaitAllocSuccess;
    LONG                m_cWaitAllocLoops;
#endif RM_STATISTICS

public:
#ifdef DEBUG
    friend VOID ::AssertValid( JET_RESID, const VOID * const );
#endif
    friend CResource::FCallingProgramPassedValidJetHandle( const JET_RESID, const VOID * const );

#ifdef MEM_CHECK
    enum    { RCI_Free = 0x0, RCI_InLA = 0x1, RCI_Allocated = 0x2 };
    typedef union
    {
        LONG    lLine;
        struct
        {
            ULONG uLine   : 16;
            ULONG uVersion: 8;
            ULONG uFlags  : 8;
        };
    } OBJECTLINEDBGINFO;
#endif

                    CResourceManager( JET_RESID resid );
                    ~CResourceManager();
            BOOL    FLink( CResource * const );
            VOID    Unlink( CResource * const );
            ERR     ErrSetParam( JET_RESOPER resop, DWORD_PTR dwParam );
            ERR     ErrGetParam( JET_RESOPER resop, DWORD_PTR * const pdwParam ) const;
            JET_RESID   ResID() const { return m_resid; }
            ERR     ErrInit();
            VOID    Term( BOOL fDuringInit = fFalse );
#ifdef MEM_CHECK
            VOID    *PvAlloc_( __in PCSTR szFile, LONG lLine );
            VOID    IDumpAlloc( const WCHAR* szDumpFile );
#else
            VOID    *PvAlloc_();
#endif
    VOID    Free( VOID * const );

    static  CResourceManager *PRMFromResid( JET_RESID );

    static  BOOL FMemoryLeak();

#ifdef DEBUGGER_EXTENSION
    VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const;
#endif

    INLINE ULONG_PTR CbAllocated() const;
    INLINE ULONG_PTR CbUsed() const;
    INLINE ULONG_PTR CbQuota() const;

private:
            CResourceManager( CResourceManager const & );
            CResourceManager &operator=( CResourceManager const & );

    enum    { cChunkProtect = 0x40000000 };
            VOID    *PvRFOLAlloc_( CResourceChunkInfo * const pRCI );
            VOID    *PvNewAlloc_( CResourceChunkInfo * const pRCI );
            BOOL    FReleaseAllocChunk_();
            VOID    *PvAllocFromChunk_( CResourceChunkInfo * const pRCI );
            BOOL    FGetNotFullChunk_();
            BOOL    FAllocateNewChunk_();

#ifdef MEM_CHECK
            #define MarkAsAllocated_( pv, option, szFile, lLine ) MarkAsAllocated__( pv, option, szFile, (LONG)lLine )
            VOID    MarkAsAllocated__( VOID * const pv, LONG option, __in PCSTR szFile, LONG lLine );
            #define MarkAsFreed_( pv, option ) MarkAsFreed__( pv, option )
            VOID    MarkAsFreed__( VOID * const pv, LONG option );
#else
            #define MarkAsAllocated_( pv, option, szFile, lLine ) MarkAsAllocated__( pv )
            VOID    MarkAsAllocated__( VOID * const pv );
            #define MarkAsFreed_( pv, option ) MarkAsFreed__( pv )
            VOID    MarkAsFreed__( VOID * const pv );
#endif

            INT     CalcObjectsPerSection(INT *pcbSectionHeader = NULL) const;

    static  BOOL        fMemoryLeak;
    static  DWORD_PTR   AllocatedResource;
    static  DWORD_PTR   FreedResource;
    static  LONG        cAllocFromHeap;
};

class CRMContainer
{
private:
    CRMContainer    *m_pNext;
    CResourceManager m_RM;
    static CCriticalSection s_critAddDelete;

public:
    INLINE      CRMContainer( JET_RESID resid );

    static CResourceManager *PRMFind( JET_RESID resid );
    static BOOL FAdd( JET_RESID resid );
    static VOID Delete( JET_RESID resid );

    INLINE static VOID CalcAllocatedObjects( JET_RESID resid, void* pvBuf );
    INLINE static VOID CalcUsedObjects( JET_RESID resid, void* pvBuf );
    INLINE static VOID CalcQuotaObjects( JET_RESID resid, void* pvBuf );

#ifdef DEBUGGER_EXTENSION
    static CResourceManager *EDBGPRMFind( JET_RESID resid );
#endif

};

extern CRMContainer *g_pRMContainer;




INLINE CLookaside::CLookaside() :
    m_ppvData( NULL ),
    m_cItems( cbCacheLine/sizeof( VOID * ) )
{
#ifdef RM_STATISTICS
    m_cGetHitFirst = 0;
    m_cGetHit = 0;
    m_cGetACX = 0;
    m_cReturnHitFirst = 0;
    m_cReturnHit = 0;
    m_cReturnACX = 0;
#endif
}


INLINE BOOL CLookaside::FInit() const
    { return NULL != m_ppvData? fTrue: fFalse; }

INLINE INT CLookaside::CSize() const { return m_cItems; }



INLINE CResourceFreeObjectList::CResourceFreeObjectList() :
        m_pRFOLNext( NULL )
    {}


INLINE VOID CResourceFreeObjectList::RFOLAddObject(
#ifdef RM_DEFERRED_FREE
    CResourceFreeObjectList * volatile *ppRFOLHead,
    CResourceFreeObjectList * volatile *ppRFOLTail,
#else
    CResourceFreeObjectList * volatile *ppRFOL,
#endif
    VOID * const pv
    )
{
    CResourceFreeObjectList *pRFOLToAdd = ( CResourceFreeObjectList * )pv;
    
#ifdef RM_DEFERRED_FREE
    if ( NULL == *ppRFOLHead )
    {
        Assert( NULL == *ppRFOLTail );
        *ppRFOLTail = *ppRFOLHead = pRFOLToAdd;
    }
    else
    {
        Assert( NULL != *ppRFOLTail );

        (*ppRFOLTail)->m_pRFOLNext = pRFOLToAdd;
        *ppRFOLTail = pRFOLToAdd;
    }

    pRFOLToAdd->m_pRFOLNext = NULL;
#else
    CResourceFreeObjectList *pRFOL      = NULL;

    OSSYNC_FOREVER
    {
        pRFOL = *ppRFOL;
        pRFOLToAdd->m_pRFOLNext = pRFOL;
        if ( AtomicCompareExchangePointer( (void**)ppRFOL, pRFOL, pRFOLToAdd ) == pRFOL )
        {
            break;
        }
    }
#endif
}

INLINE CResourceFreeObjectList * CResourceFreeObjectList::PRFOLRemoveObject(
#ifdef RM_DEFERRED_FREE
    CResourceFreeObjectList * volatile *ppRFOLHead,
    CResourceFreeObjectList * volatile *ppRFOLTail
#else
    CResourceFreeObjectList * volatile *ppRFOL
#endif
    )
{
    CResourceFreeObjectList *pRFOL      = NULL;

#ifdef RM_DEFERRED_FREE
    Assert( NULL != *ppRFOLTail );
    Assert( NULL != *ppRFOLHead );
    
    pRFOL = *ppRFOLHead;

    *ppRFOLHead = (*ppRFOLHead)->m_pRFOLNext;

    if ( NULL == *ppRFOLHead )
    {
        *ppRFOLTail = NULL;
    }
#else
    OSSYNC_FOREVER
    {
        Assert( NULL != *ppRFOL );
        pRFOL = *ppRFOL;
        
        if ( AtomicCompareExchangePointer( (void **)ppRFOL, pRFOL, pRFOL->m_pRFOLNext ) == pRFOL )
        {
            break;
        }
    }
#endif
    
    return (CResourceFreeObjectList *) pRFOL;
}


INLINE VOID CResourceFreeObjectList::RFOLLoopCheck(
    CResourceFreeObjectList * volatile *ppRFOL )
{
    CResourceFreeObjectList *pRFOL  = NULL;
    CResourceFreeObjectList *pRFOL2 = NULL;
    pRFOL = *ppRFOL;
    pRFOL2 = pRFOL;
    while ( NULL != pRFOL && NULL != pRFOL2 )
    {
        pRFOL = pRFOL->m_pRFOLNext;
        pRFOL2 = pRFOL2->m_pRFOLNext;
        if ( NULL != pRFOL2 )
        {
            pRFOL2 = pRFOL2->m_pRFOLNext;
        }
        else
        {
            return;
        }
        Assert( pRFOL != pRFOL2 );
    }
}



INLINE CResourceChunkInfo::CResourceChunkInfo( CResourceManager *pRMOwner ) :
#ifdef RM_DEFERRED_FREE
        m_pRFOLHead( NULL ),
        m_pRFOLTail( NULL ),
        m_cDeferredFrees( 0 ),
#else
        m_pRFOL( NULL ),
#endif
        m_pvData( NULL ),
        m_cUsed( -1 ),
        m_cNextAlloc( 0 ),
        m_pRCINextNotFull( NULL ),
        m_pRCINextFree( NULL ),
        m_critAlloc( CLockBasicInfo( CSyncBasicInfo( "CResourceChunkInfo::m_critAlloc" ), 0, 0 ) ),
        m_pRMOwner( pRMOwner )
{
        Assert( NULL != pRMOwner );
}



INLINE CResourceSection::CResourceSection() :
        m_pRCI( NULL ),
        m_cbSectionHeader( 0 )
{
    memset( m_rgchTag, 0, sizeof( m_rgchTag ) );
}

