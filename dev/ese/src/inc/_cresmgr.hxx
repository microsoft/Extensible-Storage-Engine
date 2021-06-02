// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  if the following is defined it will generate rmstat.txt file with the
//      performance information related to the resource manager
//      might not work if you kill the app.
//  #define RM_STATISTICS

#ifdef DEBUG
//  if the following is defined it will make it so the lookaside of the resource 
//  manager is deactivated and the free list in a chunk is queued up to its
//  maximum size (the max number of blocks in a chunk) before they are given
//  out in allocations. Even then, instead of a LIFO behavior, allocations are given
//  out from the free list in FIFO. 
//
//  Obviously, the goal of this define is to maximize as much as possible the time by
//  which we hold free blocks between reallocations, defering the actual free'ing. This then
//  also maximizes the time window where we will catch a double free red-handed.
//
//  #define RM_DEFERRED_FREE
#endif // DEBUG

//  Description of the columns in rmstat.txt
//  RESID - resource tag
//  Alloc - how many allocations were performed
//  (RFOLWait) - how many times we've waited on the RFOL critical section
//  Free - how many frees were performed
//  WaitChnk - how many time we've waited chunk to be allocated
//  WaitLoop - how many not full chunks have been checked while waiting to alloc
//              the chunk
//  WaitSucc - how many of the wait loops ended with successfull allocation of
//              an object from the not free list instead of waiting to allocate
//              a new chunk
//  LAGACX   - Total AtomicCompareExchanges/AtomicExchanges in the lookaside
//              array in order to get an object
//  LAGFst   - How many first hist we've got %
//  LAGHit   - How many not first hit tries has been made %
//  LARACX   - Total AtomicCompareExchanges/AtomicExchanges in the lookaside
//              array in order to free an object
//  LARFst   - How many first hist we've got %
//  LARHit   - How many not first hit tries has been made %

extern DWORD_PTR cbSectionSize;
#define maskSection DWORD_PTR( 0 - cbSectionSize )

//  Classes declared in this header file
class CLookaside;
class CResourceFreeObjectList;
class CResourceChunkInfo;

template <class T> T AlignDownMask( T const x, DWORD_PTR dwAlign )
//  x - (x mod dwAlign)
//  Example: AlignDownMask( 17, 4 ) = 17 - ( 17 mod 4 ) = 17 - 1 = 16
    { return (T)( DWORD_PTR( x ) & DWORD_PTR( 0 - dwAlign ) ); }

template <class T> T AlignUpMask( T const x, DWORD_PTR dwAlign )
//  (x + dwAlign + 1) - ((x + dwAlign - 1) mod dwAlign)
//  Example: AlignUpMask( 17, 4 ) = 20 - ( 20 mod 4 ) = 20 - 0 = 20
    { return (T)( ( DWORD_PTR( x ) + dwAlign - 1 ) & DWORD_PTR( 0 - dwAlign ) ); }

//==============================================================================
class CLookaside
//  Static array. Used to cache freed object in order to speed up alloc and
//  free operations. Based on external hints it will first look at the hinted
//  index and if it is empty it will check the array cache line containing this
//  element
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
#endif // RM_STATISTICS

public:
    INLINE          CLookaside();
                    ~CLookaside();
            ERR     ErrInit();
            ERR     ErrSetSize( INT cSize );
    INLINE  BOOL    FInit() const;
    INLINE  INT     CSize() const;
            VOID    Term();
            VOID    *PvGet( DWORD_PTR dwHint );
                    //  looks for the object in the area provided by the hint
            BOOL    FReturn( DWORD_PTR dwHint, VOID * const pv );
                    //  looks for free space in the area provided by the hint
            VOID    *PvFlush();
                    //  looks fot the object in the whole array

#ifdef DEBUGGER_EXTENSION
            VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const;
#endif // DEBUGGER_EXTENSION

private:
    //  Forbidden constructors
            CLookaside( CLookaside const & );
            CLookaside &operator=( CLookaside const & );
};

//==============================================================================
class CResourceFreeObjectList
//  defines structure of the linked list of free objects in each chunk
//  acronym RFOL
{
public:
    CResourceFreeObjectList * volatile m_pRFOLNext;

public:
    INLINE          CResourceFreeObjectList();

    INLINE static   VOID    RFOLAddObject(
#ifdef RM_DEFERRED_FREE
            CResourceFreeObjectList * volatile *ppRFOLHead,
            CResourceFreeObjectList * volatile *ppRFOLTail,
#else // RM_DEFERRED_FREE
            CResourceFreeObjectList * volatile *ppRFOL,
#endif // !RM_DEFERRED_FREE
            VOID * const pv
            );

    INLINE static   CResourceFreeObjectList *   PRFOLRemoveObject(
#ifdef RM_DEFERRED_FREE
            CResourceFreeObjectList * volatile *ppRFOLHead,
            CResourceFreeObjectList * volatile *ppRFOLTail
#else // RM_DEFERRED_FREE
            CResourceFreeObjectList * volatile *ppRFOL
#endif // !RM_DEFERRED_FREE
            );
    INLINE static   VOID    RFOLLoopCheck( CResourceFreeObjectList * volatile *ppRFOL );

private:
    //  Forbidden constructors
            CResourceFreeObjectList( CResourceFreeObjectList const & );
            CResourceFreeObjectList &operator=( CResourceFreeObjectList const & );
};

//==============================================================================
class CResourceChunkInfo
//  contains chunks metadata and pointer to the chunk itself
//  acronym RCI
//
//  Once allocated it never gets freed. Thus we guarrantee that chunk metadata
//  will always be there. Once the chunk associated with particular RCI gets
//  freed and later on the RCI goes to the
//  RCI free list this RCI becomes reusable.
{
public:
#ifdef RM_DEFERRED_FREE
    INT                     m_cDeferredFrees;       //  count of deferred frees
    CResourceFreeObjectList * volatile m_pRFOLHead; //  list of free objects in the chunk
    CResourceFreeObjectList * volatile m_pRFOLTail; //  list of free objects in the chunk
#else // RM_DEFERRED_FREE
    CResourceFreeObjectList * volatile m_pRFOL; //  list of free objects in the chunk
#endif // !RM_DEFERRED_FREE
    CCriticalSection        m_critAlloc;        //  Allocation critical section
    volatile LONG           m_cUsed;    //  amount of allocated objects in the chunk
    volatile LONG           m_cNextAlloc;   //  first unused object in the chunk
    CResourceChunkInfo      * volatile m_pRCINextNotFull;   //  next not completely full chunk
    VOID                    *m_pvData;      //  pointer to the raw chunk data
    CResourceChunkInfo      *m_pRCINext;    //  next RCI in the RM RCI list
    CResourceManager        *m_pRMOwner;
    CResourceChunkInfo      * volatile m_pRCINextFree;  //  next free RCI
                            //  NextNotFull & NextFree are mutex but we keep
                            //  them separated for the sake of easier debugging

public:
    INLINE      CResourceChunkInfo( CResourceManager *pRMOwner );

private:
    //  Forbidden constructors
            CResourceChunkInfo( CResourceChunkInfo const & );
            CResourceChunkInfo &operator=( CResourceChunkInfo const & );
};

//==============================================================================
class CResourceSection
//  metadata stored at the beginning of each section.
//  acronym RS
//
//  contains pointer to the RCI of the owner chunk + RMtag.
//  In debug also contains object's allocation information.
{
public:
    CResourceChunkInfo *m_pRCI; //  pointer to RCI owning this section
    LONG                m_cbSectionHeader; // size of section header
    CHAR                m_rgchTag[JET_resTagSize];  //  tag
#ifdef MEM_CHECK
    class FileLineData
    {
        public:
            CHAR * volatile szFile;
            LONG volatile lLine;
    };
    FileLineData        m_rgFLD[0];
#endif // MEM_CHECK

public:
    INLINE          CResourceSection();

private:
    //  Forbidden constructors
            CResourceSection( CResourceSection const & );
            CResourceSection &operator=( CResourceSection const & );
};



//==============================================================================
class CResourceManager
//  the real work is done here. Managing all the RCIs and object allocations
//  and frees.
//  acronym RM
//
//  contains three lists of RCI
//  m_pRCIList - a global list of the all RCIs allocated by this RM ever, we
//      only append to this list when we need to allocate chunk and there is no
//      free RCI
//  m_pRCINotFullList - a subset of m_pRCIList containing all the RCIs pointing
//      ot semifull chunks and some free RCIs. Chunks get into the list
//      when the first object of the full chunk is deleted.
//  m_pRCIFreeList - a subset of m_pRCIList containing the free RCIs. Chunks
//      get there when we traverse the m_pRCINotFullList and find a free RCI.
//
//  m_pRCIToAlloc - current chunk to allocate data from
//
//  How it works:
// ===============
//  When we allocate we first try default chunk if we cannot find free objects
//  there we traverse m_pRCINotFullList for not free RCI. All the free ones
//  are moved in the m_pRCIFreeList. If there is no free RCI then have to
//  allocate a new chunk. First we check the m_pRCIFreeList if we can avoid
//  allocating the RCI at least.
//
//  When we free if this is the first object freed in the chunk and this is not
//  the default chunk then we add the chunk to the m_pRCINotFullList. If this
//  is the last object in the chunk and this is not the default chunk then we
//  free the chunk
{
    //  RM working properties
    CLookaside          m_lookaside;
    CResourceChunkInfo  * volatile m_pRCIToAlloc;   //  RCI from which to alloc
    INT                 m_cObjectsPerChunk;     //  objects_per_section * sections_per_chunk
    CResourceChunkInfo  * volatile m_pRCINotFullList; // list of these RCIs that chunk is not full
    CResourceChunkInfo  * volatile m_pRCIFreeList;  //  list of these RCIs that chunk is deleted (free RCI)
    volatile LONG       m_cFreeRCI;             //  number of allocate RCI that have no chunks attached to them
    volatile LONG       m_cAllocatedRCI;        //  the total number of allocated RCI
    volatile ULONG_PTR  m_cUsedObjects;         //  the total number of used objects in the allocated RCIs
    CResourceChunkInfo  * volatile m_pRCIList;  //  list of all RCI
    volatile LONG       m_cCResourceLinks;      //  How many CResource objects are linked to that RM
    CCriticalSection    m_critLARefiller;       //  Protects LA refill allocation and RCI reorg
    DWORD               m_cAvgCount;

    //  RM calculated parameters
    INT                 m_cbSectionHeader;      //  section overhead (pRCI, tag, header size) DEBUG info
    INT                 m_cbAlignedObject;      //  sizeof the object including alignment
    INT                 m_cObjectsPerSection;   //  amount of the objects that can fit in single section
    INT                 m_cAllocatedRCIMax;     //  this limit the number of chunks as well
    INT                 m_cAllocatedChunksMin;  //  the minimal amount of allocated chunks
    INT                 m_1_cbAlignedObject;    // used to simulate fast division by m_cbAlignedObject, see CResource::FCallingProgramPassedValidJetHandle()
public:
    enum { shfAlignedObject = 16 };
private:
    //  RM initial parameters
    JET_RESID           m_resid;                //  resource id (set in constructor)
    CHAR                m_rgchTag[JET_resTagSize];  //  tag
    LONG                m_cbObjectAlign;        //  object alignment
    LONG                m_cbObjectSize;         //  obejct size (unaligned)
    DWORD_PTR           m_cbChunkSize;          //  size of each chunk
    LONG                m_cbRFOLOffset;         //  where to store RFOL information in the freed objects
    LONG                m_cObjectsMin;          //  min number of preallocated objects
    LONG                m_cObjectsMax;          //  max number of allocated objects

    union
    {
        ULONG           m_ulFlags;
        struct
        {
            BOOL        m_fGuarded:1;           //  if there is a guard page at the end of each section
            BOOL        m_fAllocTopDown:1;      //  chunks are allocated using MEM_TOP_DOWN
            BOOL        m_fPreserveFreed:1;     //  freed chunks are not decommitted/released back to the OS
            BOOL        m_fAllocFromHeap:1;     //  objects are allocated from the heap, bypassing the resource manager altogether
        };
    };

    CCriticalSection    m_critInitTerm;         //  Init/Term critical section

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
#endif  //  DEBUG
    friend CResource::FCallingProgramPassedValidJetHandle( const JET_RESID, const VOID * const );

#ifdef MEM_CHECK
    enum    { RCI_Free = 0x0, RCI_InLA = 0x1, RCI_Allocated = 0x2 };
    typedef union
    {
        LONG    lLine;
        struct
        {
            ULONG uLine   : 16; //  16 bits for file line, hope it will be enough
            ULONG uVersion: 8;  //  how many thimes the object was reused
            ULONG uFlags  : 8;  //  where exatly is the object look above
        };
    } OBJECTLINEDBGINFO;
#endif  //  MEM_CHECK

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
#else  //  MEM_CHECK
            VOID    *PvAlloc_();
#endif  //  MEM_CHECK
    VOID    Free( VOID * const );

    static  CResourceManager *PRMFromResid( JET_RESID );

    static  BOOL FMemoryLeak();

#ifdef DEBUGGER_EXTENSION
    VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const;
#endif // DEBUGGER_EXTENSION

    INLINE ULONG_PTR CbAllocated() const;
    INLINE ULONG_PTR CbUsed() const;
    INLINE ULONG_PTR CbQuota() const;

private:
    //  Forbidden constructors
            CResourceManager( CResourceManager const & );
            CResourceManager &operator=( CResourceManager const & );

    enum    { cChunkProtect = 0x40000000 }; //   protect chunk to be freed
    //  Alloc INLINE functions
            VOID    *PvRFOLAlloc_( CResourceChunkInfo * const pRCI );   //  allocates from RFOL list of the default chunk
            VOID    *PvNewAlloc_( CResourceChunkInfo * const pRCI );    //  allocates unused object from default chunk
            BOOL    FReleaseAllocChunk_();                              //  releases default chunk if it is full
                                                                        //  otherwise reserves an object in it
            VOID    *PvAllocFromChunk_( CResourceChunkInfo * const pRCI );  //  updates chunk info and calls PvNewAlloc_
                                                                        //  if it fails it calls PvRFOLAlloc_
            BOOL    FGetNotFullChunk_();                                //  search for not empty chunk in the list of not full ones
            BOOL    FAllocateNewChunk_();                               //  allocates a new chunk

#ifdef MEM_CHECK
            #define MarkAsAllocated_( pv, option, szFile, lLine ) MarkAsAllocated__( pv, option, szFile, (LONG)lLine )
            VOID    MarkAsAllocated__( VOID * const pv, LONG option, __in PCSTR szFile, LONG lLine );
            #define MarkAsFreed_( pv, option ) MarkAsFreed__( pv, option )
            VOID    MarkAsFreed__( VOID * const pv, LONG option );
#else  //  !MEM_CHECK
            #define MarkAsAllocated_( pv, option, szFile, lLine ) MarkAsAllocated__( pv )
            VOID    MarkAsAllocated__( VOID * const pv );
            #define MarkAsFreed_( pv, option ) MarkAsFreed__( pv )
            VOID    MarkAsFreed__( VOID * const pv );
#endif  //  MEM_CHECK

            INT     CalcObjectsPerSection(INT *pcbSectionHeader = NULL) const;

    static  BOOL        fMemoryLeak;
    static  DWORD_PTR   AllocatedResource;
    static  DWORD_PTR   FreedResource;
    static  LONG        cAllocFromHeap;
};

//==============================================================================
class CRMContainer
//  Contains all the Resource Managers. Serves as a linker between RM and CResource
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
#endif // DEBUGGER_EXTENSION

};

extern CRMContainer *g_pRMContainer;


//=============================================================================
//  INLINE funcions
//=============================================================================

//======================================
//  Class CLookaside

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
#endif // RM_STATISTICS
}


INLINE BOOL CLookaside::FInit() const
    { return NULL != m_ppvData? fTrue: fFalse; }

INLINE INT CLookaside::CSize() const { return m_cItems; }


//======================================
//  Class CResourceFreeObjectList

INLINE CResourceFreeObjectList::CResourceFreeObjectList() :
        m_pRFOLNext( NULL )
    {}


//======================================
INLINE VOID CResourceFreeObjectList::RFOLAddObject(
#ifdef RM_DEFERRED_FREE
    CResourceFreeObjectList * volatile *ppRFOLHead,
    CResourceFreeObjectList * volatile *ppRFOLTail,
#else // RM_DEFERRED_FREE
    CResourceFreeObjectList * volatile *ppRFOL,
#endif // !RM_DEFERRED_FREE
    VOID * const pv
    )
//  atomically adds an object to the list
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
#else // RM_DEFERRED_FREE
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
#endif // !RM_DEFERRED_FREE
}

//======================================
INLINE CResourceFreeObjectList * CResourceFreeObjectList::PRFOLRemoveObject(
#ifdef RM_DEFERRED_FREE
    CResourceFreeObjectList * volatile *ppRFOLHead,
    CResourceFreeObjectList * volatile *ppRFOLTail
#else // RM_DEFERRED_FREE
    CResourceFreeObjectList * volatile *ppRFOL
#endif // !RM_DEFERRED_FREE
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
#else // RM_DEFERRED_FREE
    OSSYNC_FOREVER
    {
        Assert( NULL != *ppRFOL );
        pRFOL = *ppRFOL;
        
        if ( AtomicCompareExchangePointer( (void **)ppRFOL, pRFOL, pRFOL->m_pRFOLNext ) == pRFOL )
        {
            break;
        }
    }
#endif // !RM_DEFERRED_FREE
    
    return (CResourceFreeObjectList *) pRFOL;
}


//======================================
INLINE VOID CResourceFreeObjectList::RFOLLoopCheck(
    CResourceFreeObjectList * volatile *ppRFOL )
//  checks if the single linked list contains loop
//
//  How? use to different pointers to go with two steps and one step ahead
//  if they meet each other it means the list contains loop.
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


//======================================
//  Class CResourceChunkInfo

//======================================
INLINE CResourceChunkInfo::CResourceChunkInfo( CResourceManager *pRMOwner ) :
#ifdef RM_DEFERRED_FREE
        m_pRFOLHead( NULL ),
        m_pRFOLTail( NULL ),
        m_cDeferredFrees( 0 ),
#else // RM_DEFERRED_FREE
        m_pRFOL( NULL ),
#endif // !RM_DEFERRED_FREE
        m_pvData( NULL ),
        m_cUsed( -1 ),
        m_cNextAlloc( 0 ),
//      m_pRCINext( NULL ), Don't ever change m_pRCI next. every alive RCI should be in the list of RCIs
        m_pRCINextNotFull( NULL ),
        m_pRCINextFree( NULL ),
        m_critAlloc( CLockBasicInfo( CSyncBasicInfo( "CResourceChunkInfo::m_critAlloc" ), 0, 0 ) ),
        m_pRMOwner( pRMOwner )
{
        Assert( NULL != pRMOwner );
}


//======================================
//  Class CResourceSection

//======================================
INLINE CResourceSection::CResourceSection() :
        m_pRCI( NULL ),
        m_cbSectionHeader( 0 )
{
    memset( m_rgchTag, 0, sizeof( m_rgchTag ) );
}

