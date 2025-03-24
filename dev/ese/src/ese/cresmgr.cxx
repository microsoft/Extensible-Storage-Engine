// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#ifdef DEBUG
#define TERMINATION_CHECK
#endif // DEBUG

#ifndef DEBUG
///#define SILENT_FCB_LEAK
#endif // DEBUG

#include "_cresmgr.hxx"

#ifdef MEM_CHECK
C_ASSERT( sizeof( CResourceSection ) + sizeof( CResourceSection::FileLineData ) <= cbRESHeader );
#else // MEM_CHECK
C_ASSERT( sizeof( CResourceSection ) <= cbRESHeader );
#endif // MEM_CHECK

static const INT cbAlignDefault         = 32;
static const DWORD_PTR cbChunkDefault   = 64 * 1024;
static const INT cpresLookasideDefault  = 32;
static const INT cbRFOLOffsetDefault    = sizeof( DWORD_PTR );
static const INT cbChunkMax             = 16 * 1024 * 1024;
                                                //  lowest usage should have with 8k pages and 64k sections
                                                //  and we have one page lost for the section header
static const INT percentSectionUsageMin = (64 - 8) * 100 / 64;

static BOOL fOSRMPreinitPostTerm = fTrue;   //  We initalizing basic RM during memory preinit mode
                                            //  At this time we do not have properly initialized critical sections, so avoid them


#ifdef RM_STATISTICS
static CPRINTFFILE *g_pcprintf;
#endif // RM_STATISTICS

DWORD_PTR cbSectionSize = 0;

const LONG cRCIIsFree = -1;


//  performance counters

LONG LFResMgrFCBAllocCEFLPv( LONG iInstance, void* pvBuf )
{
    CRMContainer::CalcAllocatedObjects( JET_residFCB, pvBuf );
    return 0;
}

LONG LFResMgrFCBAllocUsedCEFLPv( LONG iInstance, void* pvBuf )
{
    CRMContainer::CalcUsedObjects( JET_residFCB, pvBuf );
    return 0;
}

LONG LFResMgrFCBQuotaCEFLPv( LONG iInstance, void* pvBuf )
{
    CRMContainer::CalcQuotaObjects( JET_residFCB, pvBuf );
    return 0;
}

LONG LFResMgrFUCBAllocCEFLPv( LONG iInstance, void* pvBuf )
{
    CRMContainer::CalcAllocatedObjects( JET_residFUCB, pvBuf );
    return 0;
}

LONG LFResMgrFUCBAllocUsedCEFLPv( LONG iInstance, void* pvBuf )
{
    CRMContainer::CalcUsedObjects( JET_residFUCB, pvBuf );
    return 0;
}

LONG LFResMgrFUCBQuotaCEFLPv( LONG iInstance, void* pvBuf )
{
    CRMContainer::CalcQuotaObjects( JET_residFUCB, pvBuf );
    return 0;
}

LONG LFResMgrTDBAllocCEFLPv( LONG iInstance, void* pvBuf )
{
    CRMContainer::CalcAllocatedObjects( JET_residTDB, pvBuf );
    return 0;
}

LONG LFResMgrTDBAllocUsedCEFLPv( LONG iInstance, void* pvBuf )
{
    CRMContainer::CalcUsedObjects( JET_residTDB, pvBuf );
    return 0;
}

LONG LFResMgrTDBQuotaCEFLPv( LONG iInstance, void* pvBuf )
{
    CRMContainer::CalcQuotaObjects( JET_residTDB, pvBuf );
    return 0;
}

LONG LFResMgrIDBAllocCEFLPv( LONG iInstance, void* pvBuf )
{
    CRMContainer::CalcAllocatedObjects( JET_residIDB, pvBuf );
    return 0;
}

LONG LFResMgrIDBAllocUsedCEFLPv( LONG iInstance, void* pvBuf )
{
    CRMContainer::CalcUsedObjects( JET_residIDB, pvBuf );
    return 0;
}

LONG LFResMgrIDBQuotaCEFLPv( LONG iInstance, void* pvBuf )
{
    CRMContainer::CalcQuotaObjects( JET_residIDB, pvBuf );
    return 0;
}

//=============================================================================
//  IMPLEMENTATION
//=============================================================================

CRMContainer *g_pRMContainer = NULL;
CCriticalSection CRMContainer::s_critAddDelete( CLockBasicInfo( CSyncBasicInfo( "CRMContainer::s_critAddDelete" ), 0, 100 ) );
//======================================
//  Class CQuota

//======================================
ERR CQuota::ErrSetQuota( const LONG cQuota )
//  sets quota
//
//  works only if the object is not initialized.
{
    if ( m_cQuotaFree >= 0 )
    {
        return ErrERRCheck( JET_errAlreadyInitialized );
    }

    if ( 0 > cQuota || QUOTA_MAX < cQuota )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    m_cQuota = m_cQuota < 0 ? -cQuota : cQuota;

    return JET_errSuccess;
}

//======================================
ERR CQuota::ErrEnableQuota( const BOOL fEnable )
//  enables or disables the quota
//
//  works only if the object is not initialized.
//  you cannot multiply enable or disable a quota.
{
    if ( m_cQuotaFree >= 0 )
    {
        return ErrERRCheck( JET_errAlreadyInitialized );
    }

    if ( fEnable && m_cQuota > 0 )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    if ( !fEnable && m_cQuota < 0 )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    m_cQuota = -m_cQuota;

    return JET_errSuccess;
}

//======================================
ERR CQuota::ErrInit()
//  initializes the quota object
//
//  works only if the object is not initialized.
{
    if ( m_cQuotaFree >= 0 )
    {
        return ErrERRCheck( JET_errAlreadyInitialized );
    }

    m_cQuotaFree = m_cQuota < 0 ? -m_cQuota : m_cQuota;

    return JET_errSuccess;
}


//======================================
VOID CQuota::Term()
//  Terminates quota object
{
    if ( m_cQuotaFree < 0 )
    {
        return;
    }

#ifdef TERMINATION_CHECK
    //  check that all the resources are freed
    AssertSzRTL( FRFSKnownResourceLeak() || FUtilProcessAbort() || m_cQuotaFree == m_cQuota || m_cQuotaFree == -m_cQuota,
            "Not all resource objects are freed before CResource termination" );
#endif // TERMINATION_CHECK

    m_cQuotaFree    = -1;
}


//======================================
//  Class CLookaside

//======================================
CLookaside::~CLookaside()
{
    Term();
#ifdef RM_STATISTICS
        (*g_pcprintf)( "(%8i %6.2f %6.2f) (%8i %6.2f %6.2f)",
            m_cGetACX,
            ((0!=m_cGetACX)?m_cGetHitFirst*100.0/m_cGetACX:0.0),
            ((0!=m_cGetACX)?m_cGetHit*100.0/m_cGetACX:0.0),
            m_cReturnACX,
            ((0!=m_cReturnACX)?m_cReturnHitFirst*100.0/m_cReturnACX:0.0),
            ((0!=m_cReturnACX)?m_cReturnHit*100.0/m_cReturnACX:0.0) );
#endif // RM_STATISTICS
}


//======================================
ERR CLookaside::ErrSetSize( INT cSize )
//  sets size of the lookaside array
//
//  arbitrary limitation on the size of the array of 0x10000 is imposed
{
    Assert( NULL == m_ppvData );
    if ( 0 > cSize || 0x10000 < cSize )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    m_cItems = cSize;
    return JET_errSuccess;
}


//======================================
ERR CLookaside::ErrInit()
//  initializes the lookaside array
//
//  allocates the array to be alligned on the cache line boundary
{
    Assert( 0 <= m_cItems );
    m_ppvData = (VOID **)PvOSMemoryHeapAllocAlign( sizeof( VOID* ) * m_cItems, cbCacheLine );
    if ( NULL == m_ppvData )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }
    memset( (VOID *)m_ppvData, 0, sizeof( VOID*) * m_cItems );
    return JET_errSuccess;
}


//======================================
VOID CLookaside::Term()
{
    if ( FInit() )
    {
        Assert( 0 <= m_cItems );
#ifdef TERMINATION_CHECK
        INT i;
        if ( !FUtilProcessAbort() )
        {
            for ( i = m_cItems; i-- > 0; )
            {
                AssertRTL( NULL == m_ppvData[i] );
            }
        }
#endif // TERMINATION_CHECK
        OSMemoryHeapFreeAlign( m_ppvData );
        m_ppvData = NULL;
    }
    Assert( NULL == m_ppvData );
}


//======================================
VOID *CLookaside::PvGet(
    DWORD_PTR dwHint )  //  [IN] the eventual index with object
//  Gets an object from the lookaside subarray
//
//  first checks the hinted element if it is empty then checks the whole
//  cache line containing this element.
//  returns NULL if there is no object in the search area
{
    Assert( 0 <= m_cItems );
    Assert( NULL != m_ppvData );
    const INT cItems                    = m_cItems;
    if ( 0 == cItems )
    {
        return NULL;
    }

    VOID    ** const ppvBase            = m_ppvData;
    INT     iThreadId   = (INT)( ( dwHint ) % cItems );
    VOID    * volatile *ppvData = &ppvBase[iThreadId];
    VOID    *pvResult   = NULL;

    //  try to grab exact object
    if ( NULL != *ppvData )
    {
#ifdef RM_STATISTICS
        m_cGetACX++;
#endif // RM_STATISTICS
        // try to grab it
        pvResult = AtomicExchangePointer( (void **)ppvData, NULL );
        if ( NULL != pvResult )
        {
#ifdef RM_STATISTICS
            m_cGetHitFirst++;
#endif // RM_STATISTICS
            return pvResult;
        }
    }

    //  check the whole cache line
    VOID    * volatile * const ppvDataStart = AlignDownMask( ppvData, cbCacheLine );
    VOID    * volatile * ppvDataLast        = ppvDataStart + cbCacheLine/sizeof(VOID *);
    if ( ppvDataLast > &ppvBase[cItems] )
    {
        ppvDataLast = &ppvBase[cItems];
    }
    ppvData = ppvDataStart;
    while ( ppvDataLast != ppvData )
    {
        //  if current item is a true object pointer
        if ( NULL != *ppvData )
        {
#ifdef RM_STATISTICS
            m_cGetACX++;
#endif // RM_STATISTICS
            // try to grab it
            pvResult = AtomicExchangePointer( (void **)ppvData, NULL );
            if ( NULL != pvResult )
            {
#ifdef RM_STATISTICS
                m_cGetHit++;
#endif // RM_STATISTICS
                return pvResult;
            }
        }
        ppvData+=1;
    }
    return NULL;
}


//======================================
BOOL CLookaside::FReturn( DWORD_PTR dwHint, VOID * const pv )
//  Tries to add an object to the lookaside array
//  returns fFalse if there is no free space in the search area
{
    Assert( NULL != pv );
    Assert( 0 <= m_cItems );
    Assert( NULL != m_ppvData );
    const INT cItems = m_cItems;
    if ( 0 == cItems )
    {
        return fFalse;
    }

    VOID    ** const ppvBase    = m_ppvData;
    INT     iThreadId   = (INT)( ( dwHint ) % cItems );
    VOID    * volatile *ppvData = &ppvBase[iThreadId];

    //  try to return at exact place
    if ( NULL == *ppvData )
    {
#ifdef RM_STATISTICS
        m_cReturnACX++;
#endif // RM_STATISTICS
        // try to put our object pointer here
        if ( NULL == AtomicCompareExchangePointer( (void **)ppvData, NULL, pv ) )
        {
#ifdef RM_STATISTICS
            m_cReturnHitFirst++;
#endif // RM_STATISTICS
            return fTrue;
        }
    }

    //  check the whole cache line
    VOID    * volatile * const ppvDataStart = AlignDownMask( ppvData, cbCacheLine );
    VOID    * volatile * ppvDataLast        = ppvDataStart + cbCacheLine/sizeof(VOID *);
    if ( ppvDataLast > &ppvBase[cItems] )
    {
        ppvDataLast = &ppvBase[cItems];
    }
    ppvData = ppvDataStart;
    while ( ppvDataLast != ppvData )
    {
        //  if current item is a NULL pointer
        if ( NULL == *ppvData )
        {
#ifdef RM_STATISTICS
            m_cReturnACX++;
#endif // RM_STATISTICS
            // try to put our object pointer here
            if ( NULL == AtomicCompareExchangePointer( (void **)ppvData, NULL, pv ) )
            {
#ifdef RM_STATISTICS
                m_cReturnHit++;
#endif // RM_STATISTICS
                return fTrue;
            }
        }
        ppvData+=1;
    }
    return fFalse;
}


//======================================
VOID *CLookaside::PvFlush()
//  Search the whole lookaside array for object
//  not just one cache line.
{
    Assert( 0 <= m_cItems );
    Assert( NULL != m_ppvData );
    if ( 0 == m_cItems )
    {
        return NULL;
    }
    
    //  % can be avoided if m_cItems is power of two
    VOID    *pvResult   = NULL;
    VOID    * volatile *ppvData             = m_ppvData;
    VOID    * volatile * const ppvDataLast  = &m_ppvData[m_cItems];
    while ( ppvDataLast != ppvData )
    {
        //  if current item is a true object pointer
        if ( NULL != *ppvData )
        {
            // try to grab it
            pvResult = AtomicExchangePointer( (void **)ppvData, NULL );
            if ( NULL != pvResult )
            {
                return pvResult;
            }
        }
        ppvData++;
    }
    return NULL;
}

//======================================
//  Class CResourceManager

BOOL        CResourceManager::fMemoryLeak       = fFalse;
DWORD_PTR   CResourceManager::AllocatedResource = 0;
DWORD_PTR   CResourceManager::FreedResource     = 0;
LONG        CResourceManager::cAllocFromHeap    = 0;

CResourceManager::CResourceManager( JET_RESID resid ) :
        m_cbObjectAlign( cbCacheLine ),
        m_cbObjectSize( 0 ),
        m_cbChunkSize( cbChunkDefault ),
        m_cbRFOLOffset( cbRFOLOffsetDefault ),
        m_pRCIList( NULL ),
        m_pRCINotFullList( NULL ),
        m_pRCIFreeList( NULL ),
        m_cFreeRCI( 0 ),
        m_cAllocatedRCI( 0 ),
        m_pRCIToAlloc( NULL ),
        m_cCResourceLinks( 0 ),
        m_cObjectsMin( 0 ),
        m_cObjectsMax( 0 ),
        m_cUsedObjects( 0 ),
        m_resid( resid ),
        m_ulFlags( 0 ),
        m_critInitTerm( CLockBasicInfo( CSyncBasicInfo( "CResourceManager::m_critInitTerm" ), 0, 10 ) ),
        m_critLARefiller( CLockBasicInfo( CSyncBasicInfo( "CResourceManager::m_critLARefiller" ), 0, 5 ) )
{
    Assert( 0 < cbSectionSize );
    Assert( cbChunkDefault >= cbSectionSize );
    Assert( cbCacheLine >= sizeof( DWORD_PTR ) );
    memset( m_rgchTag, 0, sizeof( m_rgchTag ) );
#ifdef RM_STATISTICS
    m_cAllocCalls = 0;
    m_cRFOLWait = 0;
    m_cFreeCalls = 0;
    m_cWaitAllocTries = 0;
    m_cWaitAllocSuccess = 0;
    m_cWaitAllocLoops = 0;
#endif // RM_STATISTICS
}


//======================================
CResourceManager::~CResourceManager()
{
    Term();
#ifdef RM_STATISTICS
    (*g_pcprintf)( "\r\n%*.*s: %8i (%8i) %8i (%8i %8i %8i) ",
        sizeof( m_rgchTag ),
        sizeof( m_rgchTag ),
        m_rgchTag,
        m_cAllocCalls,
        m_cRFOLWait,
        m_cFreeCalls,
        m_cWaitAllocTries,
        m_cWaitAllocLoops,
        m_cWaitAllocSuccess );
#endif // RM_STATISTICS
}


//======================================
ERR CResourceManager::ErrSetParam(
    JET_RESOPER resop,  //  [IN]
    DWORD_PTR dwParam ) //  [IN]
{
    ERR err = JET_errSuccess;
    if ( !fOSRMPreinitPostTerm )
    {
        m_critInitTerm.Enter();
    }
    if ( !m_lookaside.FInit() )
    {
        LONG lTemp;
        switch ( resop )
        {
            case JET_resoperTag:
                if ( NULL != dwParam )
                {
                    memcpy( m_rgchTag, (void *)dwParam, sizeof( m_rgchTag ) );
                }
                else
                {
                    memset( m_rgchTag, 0, sizeof( m_rgchTag ) );
                }
                break;
            case JET_resoperSize:
                lTemp = m_cbObjectSize;
                m_cbObjectSize = (LONG)dwParam;
                if ( 0 >= (LONG)dwParam || CalcObjectsPerSection() == 0 )
                {
                    m_cbObjectSize = lTemp;
                    err = ErrERRCheck( JET_errInvalidParameter );
                }
                break;
            case JET_resoperAlign:
                lTemp = m_cbObjectAlign;
                m_cbObjectAlign = (LONG)dwParam;
                
                if ( sizeof( DWORD_PTR ) > (LONG)dwParam || CalcObjectsPerSection() == 0 || !FPowerOf2( dwParam ) )
                {
                    m_cbObjectAlign = lTemp;
                    err = ErrERRCheck( JET_errInvalidParameter );
                }
                break;
            case JET_resoperMinUse:
                if ( 0 > (LONG)dwParam )
                {
                    err = ErrERRCheck( JET_errInvalidParameter );
                }
                else
                {
                    m_cObjectsMin = (LONG)dwParam;
                }
                break;
            case JET_resoperMaxUse:
                if ( 0 > (LONG)dwParam || CQuota::QUOTA_MAX < (LONG)dwParam )
                {
                    err = ErrERRCheck( JET_errInvalidParameter );
                }
                else
                {
                    m_cObjectsMax = (LONG)dwParam;
                }
                break;
            case JET_resoperChunkSize:
                if ( 0 == dwParam || dwParam > cbChunkMax )
                {
                    err = ErrERRCheck( JET_errInvalidParameter );
                }
                else
                {
                    m_cbChunkSize = ( dwParam + cbSectionSize - 1 ) & maskSection;
                }
                break;
            case JET_resoperLookasideEntries:
                if ( INT_MAX < dwParam )
                {
                    err = ErrERRCheck( JET_errInvalidParameter );
                }
                else
                {
                    err = m_lookaside.ErrSetSize( (INT)dwParam );
                }
                break;
            case JET_resoperRFOLOffset:
                if ( dwParam > cbSectionSize - sizeof( CResourceSection ) - sizeof( CResourceFreeObjectList * ) )
                {
                    err = ErrERRCheck( JET_errInvalidParameter );
                }
                else
                {
                    m_cbRFOLOffset = (LONG)dwParam; // will be strictly verified on init time
                }
                break;
            case JET_resoperGuard:
                lTemp = (LONG)m_fGuarded;
                m_fGuarded = ( (BOOL)dwParam ? fTrue : fFalse );
                if ( CalcObjectsPerSection() == 0 )
            {
                    m_fGuarded = ( (BOOL)lTemp ? fTrue : fFalse );
                    err = ErrERRCheck( JET_errInvalidParameter );
            }
                break;
            case JET_resoperAllocTopDown:
                m_fAllocTopDown = ( (BOOL)dwParam ? fTrue : fFalse );
                break;
            case JET_resoperPreserveFreed:
                m_fPreserveFreed = ( (BOOL)dwParam ? fTrue : fFalse );
                break;
            case JET_resoperAllocFromHeap:
#ifdef DEBUG
                //  never alloc from the heap in DEBUG builds because it disables
                //  too much of our object validation logic
                m_fAllocFromHeap = fFalse;
#else  //  !DEBUG
                if ( m_fAllocFromHeap )
                {
                    AtomicDecrement( (LONG*)&cAllocFromHeap );
                }
                m_fAllocFromHeap = ( (BOOL)dwParam ? fTrue : fFalse );
                if ( m_fAllocFromHeap )
                {
                    AtomicIncrement( (LONG*)&cAllocFromHeap );
                }
#endif  //  DEBUG
                break;
            default:
                err = ErrERRCheck( JET_errInvalidParameter );
        }
    }
    else
    {
        err = ErrERRCheck( JET_errAlreadyInitialized );
    }
    if ( !fOSRMPreinitPostTerm )
    {
        m_critInitTerm.Leave();
    }
    return err;
}


//======================================
ERR CResourceManager::ErrGetParam(
    JET_RESOPER resop,  //  [IN]
    DWORD_PTR * const pdwParam ) const  //  [OUT]
{
    ERR err = JET_errSuccess;
    if ( NULL != pdwParam )
    {
        if ( JET_resoperTag != resop )
        {
            //  cannot clear if we requested tag info because pointer might not be DWORD_PTR aligned
            *pdwParam = 0;
        }
        switch ( resop )
        {
            case JET_resoperTag:
                memcpy( pdwParam, m_rgchTag, JET_resTagSize );
                break;
            case JET_resoperSize:
                *pdwParam = m_cbObjectSize;
                break;
            case JET_resoperAlign:
                *pdwParam = (DWORD_PTR)m_cbObjectAlign;
                break;
            case JET_resoperMinUse:
                *pdwParam = m_cObjectsMin;
                break;
            case JET_resoperMaxUse:
                *pdwParam = m_cObjectsMax;
                break;
            case JET_resoperCurrentUse:
                if ( m_lookaside.FInit() )
                {
                    *pdwParam = m_cAllocatedRCI - m_cFreeRCI;
                }
                else
                {
                    err = ErrERRCheck( JET_errNotInitialized );
                }
                break;
            case JET_resoperChunkSize:
                *pdwParam = m_cbChunkSize;
                break;
            case JET_resoperLookasideEntries:
                *pdwParam = m_lookaside.CSize();
                break;
            case JET_resoperRFOLOffset:
                *pdwParam = m_cbRFOLOffset;
                break;
            case JET_resoperObjectsPerChunk:
                //  do we know the number?
                if ( m_lookaside.FInit() )
                {
                    *pdwParam = m_cObjectsPerChunk;
                }
                //  Oh, we have ot calculate it
                else
                {
                    INT cbAlignedObject = AlignUpMask( m_cbObjectSize, m_cbObjectAlign );
                    if ( 0 == cbAlignedObject )
                    {
                        err = ErrERRCheck( JET_errInvalidParameter );
                        break;
                    }

                    *pdwParam = CalcObjectsPerSection () * ( m_cbChunkSize/cbSectionSize );
                }
                break;
            case JET_resoperGuard:
                *pdwParam = m_fGuarded;
                break;
            case JET_resoperAllocTopDown:
                *pdwParam = m_fAllocTopDown;
                break;
            case JET_resoperPreserveFreed:
                *pdwParam = m_fPreserveFreed;
                break;
            case JET_resoperAllocFromHeap:
                *pdwParam = m_fAllocFromHeap;
                break;
            case JET_resoperRCIList:
                *pdwParam = ( DWORD_PTR ) m_pRCIList;
                break;
            case JET_resoperSectionHeader:
                *pdwParam = m_cbSectionHeader;
                break;
            default:
                err = ErrERRCheck( JET_errInvalidParameter );
        }
    }
    else
    {
        err = ErrERRCheck( JET_errInvalidParameter );
    }
    return err;
}


//======================================
ERR CResourceManager::ErrInit()
//  sets all the internal parameters and allocates initial number of chunks
//  if any are specified.
//  NOTE: if min allocated chunks is lMax it means that none of the free
//  chunks will ever be released to the memory, but will not be performed any
//  initial chunks allocation.
{
    ERR err = JET_errSuccess;
    ENTERCRITICALSECTION enter( &m_critInitTerm );
    if ( !m_lookaside.FInit() )
    {
        Assert( 0 < m_cbObjectAlign );
        Assert( FPowerOf2( m_cbObjectAlign ) );
        Assert( 0 == ( m_cbChunkSize % cbSectionSize ) );
        Assert( 0 < ( m_cbChunkSize / cbSectionSize ) );

        //  set attributes
        m_cbAlignedObject = AlignUpMask( m_cbObjectSize, m_cbObjectAlign );
        m_1_cbAlignedObject = ( ( 1 << shfAlignedObject ) + m_cbAlignedObject - 1 ) / m_cbAlignedObject;

        if ( 0 >= m_cbObjectSize
            || m_cObjectsMin > m_cObjectsMax
            //  save space for the allocation status word at the start of the aligned object
            || sizeof( DWORD_PTR ) < m_cbRFOLOffset
            //  save space for the allocation status word at the end of the aligned object
            || m_cbRFOLOffset + sizeof( CResourceFreeObjectList * ) > m_cbAlignedObject - sizeof( DWORD_PTR ) )
        {
            Call( ErrERRCheck( JET_errInvalidParameter ) );
        }
        Call( m_lookaside.ErrInit() );

        //  determines the number of objects that can fit in one chunk
        m_cObjectsPerSection = CalcObjectsPerSection( &m_cbSectionHeader );

        Assert (m_cObjectsPerSection > 0);

        //  check if the chunk is too large
        if ( LONG( lMax / ( m_cbChunkSize / cbSectionSize ) ) < m_cObjectsPerSection )
        {
            Call( ErrERRCheck( JET_errInvalidParameter ) );
        }
        m_cObjectsPerChunk = (INT)( m_cObjectsPerSection * ( m_cbChunkSize / cbSectionSize ) );
        m_cAllocatedChunksMin = (INT)( (DWORD)( m_cObjectsMin + m_cObjectsPerChunk - 1 ) / m_cObjectsPerChunk );
        m_cAllocatedRCIMax = (INT)( (DWORD)( m_cObjectsMax + m_cObjectsPerChunk - 1 ) / m_cObjectsPerChunk );
        //  preallocates requested number of objects
        LONG iChunks;
        for ( iChunks = m_cAllocatedChunksMin; !m_fAllocFromHeap && iChunks-- > 0; )
        {
            CResourceChunkInfo *pRCI = NULL;
            //  OK, obviously we have to few chunks
            pRCI = new CResourceChunkInfo( this );
            if ( NULL != pRCI )
            {
                pRCI->m_pRCINext = m_pRCIList;
                m_pRCIList = pRCI;
                m_cAllocatedRCI++;
                m_cFreeRCI++;
                //  allocate the chunk
                pRCI->m_pvData = PvOSMemoryPageAllocEx( m_cbChunkSize, NULL, m_fAllocTopDown );
                if ( NULL != pRCI->m_pvData )
                {
                    m_cFreeRCI--;
                    pRCI->m_pRCINextNotFull = m_pRCINotFullList;
                    m_pRCINotFullList = pRCI;

                    //  Initialize the header of chunk's sections
                    DWORD_PTR iSections;
                    CHAR *pchData;
                    pchData = (CHAR *)pRCI->m_pvData;
#ifdef DEBUG
                    memset( pchData, bCRESAllocFill, m_cbChunkSize );
#endif // DEBUG
                    for ( iSections = m_cbChunkSize/cbSectionSize; iSections-- > 0; pchData += cbSectionSize )
                    {
                        CResourceSection *pRS = (CResourceSection *)pchData;
                        memset( pchData, 0, m_cbSectionHeader );
                        pRS->m_pRCI = pRCI;
                        memcpy( pRS->m_rgchTag, m_rgchTag, sizeof( m_rgchTag ) );
                        pRS->m_cbSectionHeader = m_cbSectionHeader;

                        if (m_fGuarded)
                        {
                            OSMemoryPageDecommit(
                                pchData + cbSectionSize - OSMemoryPageCommitGranularity(),
                                OSMemoryPageCommitGranularity() );
                        }
                    }

                    pRCI->m_cNextAlloc = 0;
                    pRCI->m_cUsed = 0;

                    continue;
                }
            }
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }
        m_cAvgCount = 0;
    }
    else
    {
        err = ErrERRCheck( JET_errAlreadyInitialized );
    }
    return err;
HandleError:
    Term( fTrue );
    return err;
}


//======================================
VOID CResourceManager::Term( BOOL fDuringInit )
{
    if ( !fOSRMPreinitPostTerm && !fDuringInit )
    {
        m_critInitTerm.Enter();
    }

    const BOOL fMayLeakResource = FRFSKnownResourceLeak() || FUtilProcessAbort();
    
    AssertRTL( fMayLeakResource || 0 == m_cCResourceLinks );
    if ( m_lookaside.FInit() )
    {
        VOID *pv = NULL;
        CResourceChunkInfo *pRCI = NULL;

        //  empty the lookaside array
        while ( NULL != ( pv = m_lookaside.PvFlush() ) )
        {
            CResourceSection    *pRS            = NULL;

            MarkAsFreed_( pv, RCI_Free );

            //  Decrement the number of used objects
            pRS = (CResourceSection *)((DWORD_PTR)pv & maskSection );
            pRCI = pRS->m_pRCI;
            pRCI->m_cUsed--;
            Assert( 0 <= pRCI->m_cUsed );
            //  add the object to the RFOL
#ifdef RM_DEFERRED_FREE
            pRCI->m_critAlloc.Enter();

            CResourceFreeObjectList::RFOLAddObject(
                &pRCI->m_pRFOLHead,
                &pRCI->m_pRFOLTail,
                (CHAR *)pv + m_cbRFOLOffset );
            
            pRCI->m_cDeferredFrees++;
            
            pRCI->m_critAlloc.Leave();
#else // RM_DEFERRED_FREE
            CResourceFreeObjectList::RFOLAddObject( &pRCI->m_pRFOL, (CHAR *)pv + m_cbRFOLOffset );
#endif // !RM_DEFERRED_FREE
        }
        if ( NULL != m_pRCIToAlloc )
        {
            if ( cChunkProtect <= m_pRCIToAlloc->m_cUsed )
            {
                m_pRCIToAlloc->m_cUsed -= cChunkProtect;
            }
            m_pRCIToAlloc = NULL;
        }
        //  free all the RCIs in the list

        //  WARNING: overly clever code here: pRCI starts out invalid with pRCI->m_pRCINext pointing
        //  to the head of the list.
        
        pRCI = (CResourceChunkInfo *)( (CHAR *)&m_pRCIList - OffsetOf( CResourceChunkInfo, m_pRCINext ) );
        while ( NULL != pRCI->m_pRCINext )
        {
            CResourceChunkInfo *pRCITemp = pRCI->m_pRCINext;

            // verify that exactly the right number of ROFLs are in the list
            if ( cRCIIsFree != pRCITemp->m_cUsed )
            {

                //  number of allocations that could be in the RFOL list
                INT cAllocs = pRCITemp->m_cNextAlloc;
                
                //  number of allocations that have been handed out
                INT cUsed = pRCITemp->m_cUsed;

#ifdef RM_DEFERRED_FREE
                CResourceFreeObjectList *pRFOL = pRCITemp->m_pRFOLHead;
#else // RM_DEFERRED_FREE
                CResourceFreeObjectList *pRFOL = pRCITemp->m_pRFOL;
#endif // !RM_DEFERRED_FREE

                for ( INT i = 0; i < cAllocs - cUsed; i++ )
                {
                    pRFOL = pRFOL->m_pRFOLNext;
                }
                AssertRTL( pRFOL == NULL );
            }
            
            // if pvData is allocated, free it
            if ( 0 == pRCITemp->m_cUsed )
            {
                pRCITemp->m_cUsed = cRCIIsFree;
                OSMemoryPageFree( pRCITemp->m_pvData );
                pRCITemp->m_pvData = NULL;
                m_cFreeRCI++;
            }
            
            // if we're done with the RCI, free it
            if ( cRCIIsFree == pRCITemp->m_cUsed )
            {
                pRCI->m_pRCINext = pRCITemp->m_pRCINext;
                delete pRCITemp;
                m_cFreeRCI--;
                Assert( 0 <= m_cFreeRCI || ( fMayLeakResource && m_cFreeRCI == -1 ) );
                m_cAllocatedRCI--;
                Assert( 0 <= m_cAllocatedRCI || ( fMayLeakResource && m_cAllocatedRCI == -1 ) );
            }
            else
            {
                pRCI = pRCITemp;
            }
        }
        Assert( 0 == m_cFreeRCI || ( fMayLeakResource && m_cFreeRCI == -1 ) );

        if ( NULL != m_pRCIList )
        {
            if ( !fMayLeakResource )
            {
#ifdef MEM_CHECK
                IDumpAlloc( L"Assert.txt" );
#endif  //  MEM_CHECK
#ifdef SILENT_FCB_LEAK
                Assert( fFalse );
                if ( JET_residFCB == m_resid || JET_residIDB == m_resid || JET_residTDB == m_resid )
                {
                    const WCHAR *rgpszT[1] = {
                        JET_residFCB == m_resid? L"There are allocated FCBs left":
                        JET_residIDB == m_resid? L"There are allocated IDBs left":
                        JET_residTDB == m_resid? L"There are allocated TDBs left":
                        L"OOPS missed case in FCB LEAK" };
                    UtilReportEvent( eventWarning, GENERAL_CATEGORY, PLAIN_TEXT_ID, 1, rgpszT );

                    pRCI = (CResourceChunkInfo *)( (CHAR *)&m_pRCIList - OffsetOf( CResourceChunkInfo, m_pRCINext ) );
                    while ( NULL != pRCI->m_pRCINext )
                    {
                        CResourceChunkInfo *pRCITemp = pRCI->m_pRCINext;
                        pRCI->m_pRCINext = pRCITemp->m_pRCINext;
                        OSMemoryPageFree( pRCITemp->m_pvData );
                        delete pRCITemp;
                        m_cAllocatedRCI--;
                        Assert( 0 <= m_cAllocatedRCI || ( fMayLeakResource && m_cAllocatedRCI == -1 ) );
                    }
                    Assert( NULL == m_pRCIList );
                }
                else
                {
#endif // SILENT_FCB_LEAK
                if ( !fOSRMPreinitPostTerm )
                {
                    // we are just RTL asserting instead of EnforeFail as
                    // we do have leak (FCB) which we haven't figure out
                    // At least we should not kill the process on the way out.
                    AssertSzRTL( fMayLeakResource, "Memory leak detected in resource manager" );
                }
                else
                {
                    fMemoryLeak = fTrue;
                }
#ifdef SILENT_FCB_LEAK
                }
#endif // SILENT_FCB_LEAK
            }
            m_pRCIList = NULL;
            m_cAllocatedRCI = 0;
        }
        m_pRCIFreeList = NULL;
        m_pRCINotFullList = NULL;
        m_lookaside.Term();
    }
    Assert( NULL == m_pRCIList );
    Assert( NULL == m_pRCIFreeList );
    Assert( NULL == m_pRCINotFullList );
    Assert( 0 == m_cFreeRCI || ( fMayLeakResource && m_cFreeRCI == -1 ) );
    Assert( 0 == m_cAllocatedRCI );
    Assert( NULL == m_pRCIToAlloc );
    if ( !fOSRMPreinitPostTerm && !fDuringInit )
    {
        m_critInitTerm.Leave();
    }
}


//======================================
BOOL CResourceManager::FLink( CResource * const )
{
    ENTERCRITICALSECTION enter( &m_critInitTerm );
    if ( !m_lookaside.FInit() )
    {
        AssertSz( fFalse, "Trying to link to uninitialized RM" );
        return fFalse;
    }
    AtomicIncrement( const_cast<LONG *>( &m_cCResourceLinks ) );
    return fTrue;
}


//======================================
VOID CResourceManager::Unlink( CResource * const )
{
    ENTERCRITICALSECTION enter( &m_critInitTerm );
    LONG lCurr;
    Assert( m_lookaside.FInit() );
    lCurr = AtomicDecrement( const_cast<LONG *>( &m_cCResourceLinks ) );
    Assert( 0 <= lCurr );
}

//======================================
VOID CResourceManager::Free( VOID * const pv )
//  frees an object
//
//  First it tries to enter the object in LA if fails then the object is
//  returned to the chunk. If this is first object returned to the full chunk
//  and chunk is not default one, the corresponding RCI goes in
//  m_pRCINotFullList. If the chunk is empty, not default and above min
//  requested qouta of avalable chunks then chunk is freed and RCI is marked
//  as free (m_cUsed == cRCIIsFree).
{
    //  instrumentation
    //
    if ( pv )
    {
#if ( defined MEM_CHECK || defined RM_DEFERRED_FREE )
        memset( pv, bCRESFreeFill, m_cbAlignedObject );
#endif  //  MEM_CHECK || RM_DEFERRED_FREE
        MarkAsFreed_( pv, RCI_InLA );
    }
#ifdef RM_STATISTICS
    m_cFreeCalls++;
#endif // RM_STATISTICS

    //  ignore frees of NULL pointers
    //
    if ( NULL == pv )
    {
        return;
    }

    m_cUsedObjects--;
    
    //  we are redirecting all object allocations to the heap
    //
    if ( m_fAllocFromHeap )
    {
        //  objects with huge alignment should use raw VA
        //
        if ( m_cbObjectAlign >= 256 )
        {
            OSMemoryPageFree( pv );
        }

        //  objects with medium alignment should use aligned heap blocks
        //
        else if ( m_cbObjectAlign > sizeof( void* ) )
        {
            OSMemoryHeapFreeAlign( pv );
        }

        //  objects with tiny alignment can just use ordinary heap blocks
        //
        else
        {
            OSMemoryHeapFree( pv );
        }
    }

    //  we are not redirecting all object allocations to the heap
    //
    else
    {
#ifndef RM_DEFERRED_FREE
        //  Try to put the object in the lookaside list
        BOOL fResult = fFalse;
        fResult = m_lookaside.FReturn( DwUtilThreadId(), pv );
        if ( fResult )
        {
            return;
        }
#endif // !RM_DEFERRED_FREE

        //  If fails return the object to the chunk
        CResourceChunkInfo  *pRCI           = NULL;
        LONG                cUsedInChunk    = 0;

        MarkAsFreed_( pv, RCI_Free );
        //  Decrement the number of used objects
        pRCI = ((CResourceSection *)((DWORD_PTR)pv & maskSection ))->m_pRCI;
        //  add the object to the RFOL

#ifdef RM_DEFERRED_FREE
        pRCI->m_critAlloc.Enter();

        CResourceFreeObjectList::RFOLAddObject(
            &pRCI->m_pRFOLHead,
            &pRCI->m_pRFOLTail,
            (CHAR *)pv + m_cbRFOLOffset );

        pRCI->m_cDeferredFrees++;

        pRCI->m_critAlloc.Leave();
#else // RM_DEFERRED_FREE
        CResourceFreeObjectList::RFOLAddObject( &pRCI->m_pRFOL, (CHAR *)pv + m_cbRFOLOffset );
#endif // !RM_DEFERRED_FREE
        
        cUsedInChunk = AtomicDecrement( const_cast<LONG *>( &pRCI->m_cUsed ) );

        Assert( 0 <= cUsedInChunk );
        Assert( cUsedInChunk < cChunkProtect + m_cObjectsPerChunk );
        Assert( cChunkProtect <= cUsedInChunk || m_cObjectsPerChunk > cUsedInChunk );
        //  if the chunk switched from completely full to not completely full
        //  add it to the list of not completely full chunks
        if ( m_cObjectsPerChunk-1 == cUsedInChunk )
        {
            Assert( NULL == pRCI->m_pRCINextNotFull );
            OSSYNC_FOREVER
            {
                CResourceChunkInfo *pRCINotFull;
                pRCINotFull = m_pRCINotFullList;
                pRCI->m_pRCINextNotFull = pRCINotFull;
                if ( AtomicCompareExchangePointer( (void **)&m_pRCINotFullList, pRCINotFull, pRCI ) == pRCINotFull )
                {
                    break;
                }
            }
        }

        //  if it is the last used object in the chunk, try to free the chunk
        if ( 0 == cUsedInChunk && !m_fPreserveFreed )
        {
            //  try to get the permition to free the chunk if we haven't
            //  reached the minimal quota of allocated chunks
            LONG cFreeRCI;
            OSSYNC_FOREVER
            {
                cFreeRCI = m_cFreeRCI;
                Assert( m_cAllocatedRCI - cFreeRCI >= m_cAllocatedChunksMin );
                //  if we've reached the minimum of allocated chunks
                //  or for some reason we were failed to allocate the minimum required chunks initially
                //  (most common will be OOM)
                //  do not free the chunk
                if ( m_cAllocatedRCI - cFreeRCI <= m_cAllocatedChunksMin )
                {
                    return;
                }
                if ( AtomicCompareExchange( const_cast<LONG *>( &m_cFreeRCI ), cFreeRCI, cFreeRCI+1 ) == cFreeRCI )
                {
                    break;
                }
            }
            VOID *pvData;
            pvData = pRCI->m_pvData;
            //  if somebody already reused that chunk
            if ( 0 != AtomicCompareExchange( const_cast<LONG *>( &pRCI->m_cUsed ), 0, cRCIIsFree ) )
            {
                //  rollback the number of free RCIs
                AtomicDecrement( const_cast<LONG *>( &m_cFreeRCI ) );
            }
            else
            {
                //  from this point on the pRCI and it's members do not belong to us. Anybody can reuse it
                OSMemoryPageFree( pvData );
            }
        }
    }
}


//======================================
BOOL CResourceManager::FGetNotFullChunk_()
//  search for non empty chunk in m_pRCINotFullList. If empty RCIs are found
//  during the search they are moved to m_pRCIFreeList. If the non empty chunk
//  is found it is set as default chunk.
{
    //  Try to find another not completely full chunk to use
    CResourceChunkInfo *pRCI;
    pRCI = NULL;
    while ( NULL != m_pRCINotFullList )
    {
        OSSYNC_FOREVER
        {
            pRCI = m_pRCINotFullList;
            if ( AtomicCompareExchangePointer( (void **)&m_pRCINotFullList, pRCI, pRCI->m_pRCINextNotFull ) == pRCI )
            {
                break;
            }
        }
        pRCI->m_pRCINextNotFull = NULL;
        LONG    cUsedInChunk = cRCIIsFree;
        const LONG cLock = cChunkProtect + 1;
        OSSYNC_FOREVER
        {
            cUsedInChunk = pRCI->m_cUsed;
            Assert( m_cObjectsPerChunk > cUsedInChunk );
            //  we exit from the loop if ...
            //  ... the chunk is free
            if ( cRCIIsFree == cUsedInChunk )
            {
                break;
            }
            //  ... or we succeed to lock it
            Assert( 0 <= cUsedInChunk );
            if ( AtomicCompareExchange( const_cast<LONG *>( &pRCI->m_cUsed ), cUsedInChunk, cUsedInChunk + cLock ) == cUsedInChunk )
            {
                Assert( cChunkProtect < cUsedInChunk + cLock );
                Assert( cChunkProtect + m_cObjectsPerChunk >= cUsedInChunk + cLock );
                break;
            }
        }
        //  if found and locked the proper chunk we can use it
        if ( 0 <= cUsedInChunk )
        {
            //  it cannot be protected already
            Assert( cChunkProtect > cUsedInChunk );
            Assert( m_cObjectsPerChunk > cUsedInChunk );
            m_pRCIToAlloc = pRCI;
            Assert( NULL == m_pRCIToAlloc->m_pRCINextNotFull );
            return fTrue;
        }
        //  else the chunk is empty and then move the RCI to the list of free RCIs
        else
        {
            Assert( cRCIIsFree == cUsedInChunk );
            pRCI->m_pRCINextFree = m_pRCIFreeList;
            m_pRCIFreeList = pRCI;
            Assert( NULL == m_pRCIFreeList->m_pRCINextNotFull );
        }
    }
    return fFalse;
}


//======================================
BOOL CResourceManager::FAllocateNewChunk_()
//  alocates and initialzises new chunk.
//
//  Checks the m_pRCIFreeList for reusable free RCI
{
    CResourceChunkInfo *pRCI = NULL;
    //  OK, obviously we have to allocate another chunk
    //  check the RCI free list first
    if ( NULL != m_pRCIFreeList )
    {
        VOID *pvData;
        pvData = PvOSMemoryPageAllocEx( m_cbChunkSize, NULL, m_fAllocTopDown );
        if ( NULL == pvData )
        {
            return fFalse;
        }
        AtomicDecrement( const_cast<LONG *>( &m_cFreeRCI ) );
        pRCI = m_pRCIFreeList;
        m_pRCIFreeList = pRCI->m_pRCINextFree;

        //  We need to call the destructor here before recycling the memory because the object
        //  may have allocated memory that we want to make sure gets freed. For objects that
        //  don't get recycled, ::Term() will take care of it by calling delete explicitly
        //  on the object. Calling the destructor on ::FGetNotFullChunk_() would be an option
        //  too, if it were not for the fact that we'll destroy the object again in ::Term() if
        //  it doesn't get reused by then.
        pRCI->~CResourceChunkInfo();

        new(pRCI) CResourceChunkInfo( this );
        pRCI->m_pvData = pvData;
    }
    //  we can allocate RCI only if we are not out of resources
    else if ( m_cAllocatedRCIMax > m_cAllocatedRCI )
    {
        pRCI = new CResourceChunkInfo( this );
        if ( NULL == pRCI )
        {
            return fFalse;
        }
        //  allocate the chunk
        pRCI->m_pvData = PvOSMemoryPageAllocEx( m_cbChunkSize, NULL, m_fAllocTopDown );
        if ( NULL == pRCI->m_pvData )
        {
            delete pRCI;
            return fFalse;
        }
        pRCI->m_pRCINext = m_pRCIList;
        m_pRCIList = pRCI;
        m_cAllocatedRCI++;
    }
    else
    {
        return fFalse;
    }

    //  Initialize the header of chunk's sections
    DWORD_PTR i;
    CHAR *pchData;
    pchData = (CHAR *)pRCI->m_pvData;
#ifdef DEBUG
    memset( pchData, bCRESAllocFill, m_cbChunkSize );
#endif // DEBUG
    for ( i = m_cbChunkSize/cbSectionSize; i-- > 0; pchData += cbSectionSize )
    {
        CResourceSection *pRS = (CResourceSection *)pchData;
        memset( pchData, 0, m_cbSectionHeader );
        pRS->m_pRCI = pRCI;
        memcpy( pRS->m_rgchTag, m_rgchTag, sizeof( m_rgchTag ) );
        pRS->m_cbSectionHeader = m_cbSectionHeader;

        if (m_fGuarded)
        {
            OSMemoryPageDecommit(
                pchData + cbSectionSize - OSMemoryPageCommitGranularity(),
                OSMemoryPageCommitGranularity() );
        }
    }

    //  protect from freeing
    Assert( NULL == pRCI->m_pRCINextFree );
    Assert( NULL == pRCI->m_pRCINextNotFull );
    pRCI->m_cNextAlloc = 1;
    pRCI->m_cUsed = cChunkProtect + 1;
    m_pRCIToAlloc = pRCI;
    return fTrue;
}


//======================================
VOID *CResourceManager::PvRFOLAlloc_( CResourceChunkInfo * const pRCI )
//  Allocates an object from chunk's free list (RFOL)
//
//  the check that RFOL is not empty is performed on higher level
{
    Assert( NULL != pRCI );
    CResourceFreeObjectList *pRFOL      = NULL;
    //  take from the free list
    //  we have an option to avoid the critical section ( cmpxgch8b )
#ifdef RM_STATISTICS
    if ( !pRCI->m_critAlloc.FTryEnter() )
    {
        m_cRFOLWait++;
        pRCI->m_critAlloc.Enter();
    }
#else // RM_STATISTICS
    pRCI->m_critAlloc.Enter();
#endif // RM_STATISTICS

#ifdef DEBUG
        LONG cUsed = pRCI->m_cUsed;
        Assert( 0 < cUsed );
        Assert( cChunkProtect + m_cObjectsPerChunk >= cUsed );
        Assert( m_cObjectsPerChunk >= cUsed || cChunkProtect < cUsed );
#endif // DEBUG

#ifdef RM_DEFERRED_FREE
    pRFOL = CResourceFreeObjectList::PRFOLRemoveObject(
                &pRCI->m_pRFOLHead,
                &pRCI->m_pRFOLTail );
    
    pRCI->m_cDeferredFrees--;
    
#else // RM_DEFERRED_FREE
    pRFOL = CResourceFreeObjectList::PRFOLRemoveObject( &pRCI->m_pRFOL );
#endif // !RM_DEFERRED_FREE
    
    pRCI->m_critAlloc.Leave();
    return (VOID *)((CHAR *)pRFOL-m_cbRFOLOffset);
}


//======================================
INLINE VOID *CResourceManager::PvNewAlloc_( CResourceChunkInfo * const pRCI )
//  tries to allocate an new (unused) object from the chunk
//
//  return null if it fails
{
    Assert( NULL != pRCI );
    OSSYNC_FOREVER
    {
        LONG cNextAlloc = pRCI->m_cNextAlloc;
        Assert( 0 <= cNextAlloc );
        Assert( cNextAlloc <= m_cObjectsPerChunk );
#ifdef DEBUG
        LONG cUsed = pRCI->m_cUsed;
        Assert( 0 < cUsed );
        Assert( cChunkProtect + m_cObjectsPerChunk >= cUsed );
        Assert( m_cObjectsPerChunk >= cUsed || cChunkProtect < cUsed );
#endif // DEBUG
        //  we exit if ...
        //  ... the chunk does not have unused object
        if ( cNextAlloc == m_cObjectsPerChunk )
        {
            return NULL;
        }
        //  ... or we successfully allocate one
        if ( AtomicCompareExchange( const_cast<LONG *>( &pRCI->m_cNextAlloc ), cNextAlloc, cNextAlloc+1 ) == cNextAlloc )
        {
            Assert( cNextAlloc < m_cObjectsPerChunk );
            DWORD_PTR cbChunkOffset;
            DWORD_PTR cbSectionOffset;
            cbSectionOffset = cNextAlloc / m_cObjectsPerSection * cbSectionSize + m_cbSectionHeader;
            cbChunkOffset = cbSectionOffset + (cNextAlloc % m_cObjectsPerSection) * m_cbAlignedObject;
            return (VOID *)( (CHAR *)pRCI->m_pvData + cbChunkOffset );
        }
    }
}


//======================================
BOOL CResourceManager::FReleaseAllocChunk_()
//  checks if we have to get rid of the current default chunks because it is full
//
//  if it manages to aquire an object reference from the RCI it returns false,
//  and we will succeed to allocate an object from the chunk
{
    CResourceChunkInfo *pRCI = m_pRCIToAlloc;
    if ( NULL != pRCI )
    {
        OSSYNC_FOREVER
        {
            const LONG cUsedInChunk = pRCI->m_cUsed;
            //  if the chunk is full, remove the protection bit
            Assert( cChunkProtect <= cUsedInChunk );
            Assert( cChunkProtect + m_cObjectsPerChunk >= cUsedInChunk );
            Assert( NULL == pRCI->m_pRCINextNotFull );
            Assert( NULL == pRCI->m_pRCINextFree );
            if ( cChunkProtect + m_cObjectsPerChunk == cUsedInChunk )
            {
                if ( AtomicCompareExchange( const_cast<LONG *>( &pRCI->m_cUsed ), cUsedInChunk, m_cObjectsPerChunk ) == cUsedInChunk )
                {
                    m_pRCIToAlloc = NULL;
                    return fTrue;
                }
            }
            //  else try to allocate an object in it
            else if ( AtomicCompareExchange( const_cast<LONG *>( &pRCI->m_cUsed ), cUsedInChunk, cUsedInChunk + 1 ) == cUsedInChunk )
            {
                return fFalse;
            }
        }
    }
    return fTrue;
}


//======================================
VOID *CResourceManager::PvAllocFromChunk_( CResourceChunkInfo * const pRCI )
//  Tries to allocate an object from the given chunk
//
//  first aquires an object refrence from RCI it guarantees that allocation
//  will succeed. Then tries to get a new object if it fails gets an object
//  from RFOL
{
    VOID *pvResult = NULL;
    Assert( NULL != pRCI );
    OSSYNC_FOREVER
    {
        const LONG cUsedInChunk = pRCI->m_cUsed;
        Assert( cRCIIsFree == cUsedInChunk || 0 <= cUsedInChunk );
        Assert( cChunkProtect + m_cObjectsPerChunk >= cUsedInChunk );
        Assert( m_cObjectsPerChunk >= cUsedInChunk || cChunkProtect <= cUsedInChunk );
        //  we can allocate if the chunk is not full nor free
        //  BUT we cannot allocate the last free object, if the chunk is not protected,
        //  because this chunk is in the not full list, and we will brake the logic
        if ( ( cChunkProtect <= cUsedInChunk && cChunkProtect + m_cObjectsPerChunk > cUsedInChunk )
                || ( 0 <= cUsedInChunk && cUsedInChunk < m_cObjectsPerChunk-1 ) )
        {
            //  try to allocate an object in the chunk
            if ( AtomicCompareExchange( const_cast<LONG *>( &pRCI->m_cUsed ), cUsedInChunk, cUsedInChunk + 1 ) == cUsedInChunk )
            {
                if ( pRCI->m_cNextAlloc < m_cObjectsPerChunk )
                {
                    pvResult = PvNewAlloc_( pRCI );
                }
                if ( NULL == pvResult )
                {
                    pvResult = PvRFOLAlloc_( pRCI );
                }
                return pvResult;
            }
        }
        else
        {
            return NULL;
        }
    }
}


//======================================
VOID *CResourceManager::PvAlloc_(
//  Allocates an object.
//
//  There is five different steps to allocate a object. They are executed in
//  ascending order and if any one of them succeeds we return the allocated
//  object.
//  (1) Tries to get an object from LA.
//  (2) Tries to get an object from the default chunk
//  (3) Tries to aquire lock on alloc chunk critical section if fails then
//      traverses m_pRCINotFullList and tries to steal an object from
//      one of non empty chunks
//  (4) Locks the alloc chunk critical section, traverses m_pRCINotFullList
//      in order to find new default chunk (non empty one) and aquires an
//      object referense from its RCI.
//  (5) Allocates a new chunk and aquires
#ifdef MEM_CHECK
    __in PCSTR szFile,
    LONG lLine
#endif  //  MEM_CHECK
    )
{
    VOID *pvResult = NULL;

    //  RFS
    //
    if ( !RFSAlloc( UnknownAllocResource ) )
    {
        pvResult = NULL;
    }

    //  we are redirecting all object allocations to the heap
    //
    else if ( m_fAllocFromHeap )
    {
        //  objects with huge alignment should use raw VA
        //
        if ( m_cbObjectAlign >= 256 )
        {
            pvResult = PvOSMemoryPageAllocEx( m_cbAlignedObject, NULL, m_fAllocTopDown );
        }

        //  objects with medium alignment should use aligned heap blocks
        //
        else if ( m_cbObjectAlign > sizeof( void* ) )
        {
            pvResult = PvOSMemoryHeapAllocAlign( m_cbAlignedObject, m_cbObjectAlign );
        }

        //  objects with tiny alignment can just use ordinary heap blocks
        //
        else
        {
            pvResult = PvOSMemoryHeapAlloc( m_cbAlignedObject );
        }
    }

    //  we are not redirecting all object allocations to the heap
    //
    else
    {
#ifdef RM_DEFERRED_FREE
        AssertSz( NULL == m_lookaside.PvGet( DwUtilThreadId() ), "Lookaside should be always empty with deferred frees on." );
#else // RM_DEFERRED_FREE
        //  check the lookaside array
        pvResult = m_lookaside.PvGet( DwUtilThreadId() );
#endif // !RM_DEFERRED_FREE

        //  if we haven't found one
        if ( NULL == pvResult )
        {
            CResourceChunkInfo *pRCI;
            pRCI = m_pRCIToAlloc;

            //  acquire object from the current chunk
            if ( NULL == pRCI || NULL == ( pvResult = PvAllocFromChunk_( pRCI ) ) )
            {
                //  there is no current chunk or the chunk is empty
                Assert( NULL == pvResult );

                pRCI = m_pRCINotFullList;
#ifdef RM_STATISTICS
                m_cWaitAllocTries++;
#endif // RM_STATISTICS
                if ( NULL != pRCI )
                {
                    while ( !m_critLARefiller.FTryEnter() )
                    {
#ifdef RM_STATISTICS
                        m_cWaitAllocLoops++;
#endif // RM_STATISTICS
                        pvResult = PvAllocFromChunk_( pRCI );
                        if ( NULL != pvResult )
                        {
#ifdef RM_STATISTICS
                            m_cWaitAllocSuccess++;
#endif // RM_STATISTICS
                            goto End;
                        }
                        pRCI = pRCI->m_pRCINextNotFull;
                        if ( NULL == pRCI )
                        {
                            m_critLARefiller.Enter();
                            break;
                        }
                    }
                }
                else
                {
                    m_critLARefiller.Enter();
                }
                //  release the current chunk and grab one form the list of not full chunks
                //  release might fail if meanwhile somebody has replaced the current chunk
                //  or the object from the chunk got freed
                if ( !FReleaseAllocChunk_() || FGetNotFullChunk_() )
                {
                    //  We have already reserved one object from the chunk, so we will get one
                    pRCI = m_pRCIToAlloc;
                    Assert( cChunkProtect < pRCI->m_cUsed );
                    Assert( cChunkProtect + m_cObjectsPerChunk >= pRCI->m_cUsed );
                    m_critLARefiller.Leave();

                    Assert( NULL == pvResult );
                    //  IF the chunk got replaced by somebody else
                    //  and the current chunk is just allocated
                    //  OR there are preallocated chunks on init time (JET_resoperMinUse)
                    //  we grab from the new objects
                    if ( pRCI->m_cNextAlloc < m_cObjectsPerChunk )
                    {
                        pvResult = PvNewAlloc_( pRCI );
                    }
                    if ( NULL == pvResult )
                    {
                        pvResult = PvRFOLAlloc_( pRCI );
                    }
                }
                //  we have to allocate new chunk
                else if ( FAllocateNewChunk_() )
                {
                    //  we have reserved the first object
                    pRCI = m_pRCIToAlloc;
                    Assert( cChunkProtect < pRCI->m_cUsed );
                    Assert( cChunkProtect + m_cObjectsPerChunk >= pRCI->m_cUsed );
                    m_critLARefiller.Leave();

                    Assert( NULL == pvResult );
                    pvResult = (VOID *)( (CHAR *)pRCI->m_pvData + m_cbSectionHeader );
                }
                //  everything failed, leave with failure
                else
                {
                    m_critLARefiller.Leave();

                    Assert( NULL == pvResult );
                    m_cAvgCount--;
                }
            }
        }
    }

End:

    //  instrumentation
    //
    if ( pvResult )
    {
#if ( defined MEM_CHECK || defined RM_DEFERRED_FREE )
        memset( (BYTE*)pvResult + sizeof( DWORD_PTR ), bCRESFreeFill, m_cbAlignedObject - 2 * sizeof( DWORD_PTR ) );
#endif  //  MEM_CHECK || RM_DEFERRED_FREE
        MarkAsAllocated_( pvResult, RCI_Allocated, szFile, lLine );

        m_cUsedObjects++;
    }
#ifdef RM_STATISTICS
    m_cAllocCalls++;
#endif // RM_STATISTICS

    return pvResult;
}

//======================================
INLINE ULONG_PTR CResourceManager::CbAllocated() const
{
    return m_cbChunkSize * ( m_cAllocatedRCI - m_cFreeRCI );
}

//======================================
INLINE ULONG_PTR CResourceManager::CbUsed() const
{
    const ULONG_PTR cbAlignedObject = AlignUpMask( m_cbObjectSize, m_cbObjectAlign );
    return cbAlignedObject * m_cUsedObjects;
}

//======================================
INLINE ULONG_PTR CResourceManager::CbQuota() const
{
    return m_cAllocatedRCIMax * m_cbChunkSize;
}

//======================================
INT CResourceManager::CalcObjectsPerSection(INT *pcbSectionHeader) const
{
    Assert( m_cbObjectSize > 0 );
    
    const INT cbAlignedObject = AlignUpMask( m_cbObjectSize, m_cbObjectAlign );
    Assert( 0 != cbAlignedObject );
    const INT cbSection = (INT)( cbSectionSize - ( m_fGuarded ? OSMemoryPageCommitGranularity() : 0 ) );
    INT cbSectionHeader = 0;
    INT cObjectsPerSection = 0;

#ifdef MEM_CHECK
    cObjectsPerSection = (INT)( ( cbSection - sizeof( CResourceSection ) )
            / ( cbAlignedObject + sizeof( CResourceSection::FileLineData ) ) );
    cbSectionHeader = AlignUpMask(
            (INT)sizeof( CResourceSection )
                + cObjectsPerSection * ( (INT)sizeof( CResourceSection::FileLineData ) ),
            m_cbObjectAlign );
#else  //  MEM_CHECK
    cObjectsPerSection = (INT)( ( cbSection - sizeof( CResourceSection ) ) / cbAlignedObject );
    cbSectionHeader = AlignUpMask( (INT)sizeof( CResourceSection ), m_cbObjectAlign );
#endif  //  MEM_CHECK

    while ( cbSectionHeader + cbAlignedObject * cObjectsPerSection > cbSection )
    {
        cObjectsPerSection--;
#ifdef MEM_CHECK
        cbSectionHeader = AlignUpMask(
                (INT)sizeof( CResourceSection )
                    + cObjectsPerSection * ( (INT)sizeof( CResourceSection::FileLineData ) ),
                m_cbObjectAlign );
#endif  //  MEM_CHECK
    }

    if (NULL != pcbSectionHeader)
    {
        *pcbSectionHeader = cbSectionHeader;
    }

    return cObjectsPerSection;
}


//======================================
VOID CResourceManager::MarkAsAllocated__(
    VOID * const    pv
#ifdef MEM_CHECK
    ,
    LONG            option,
    __in PCSTR      szFile,
    LONG            lLine
#endif  //  MEM_CHECK
    )
{
#ifdef MEM_CHECK
    CResourceSection    *pRS        = NULL;
    INT                 iObject     = 0;
    CHAR                *szOldFile  = NULL;
    OBJECTLINEDBGINFO   lOldLine;
    OBJECTLINEDBGINFO   lNewLine;
    pRS = (CResourceSection *)( (DWORD_PTR)pv & maskSection );
    Assert( NULL != pRS );
    Assert( NULL != pRS->m_pRCI );
    Assert( pRS->m_cbSectionHeader == m_cbSectionHeader );
    AssertSz( ( (DWORD_PTR)pv & (-m_cbObjectAlign) ) == (DWORD_PTR)pv, "Object is not properly aligned" );
    iObject = (INT)( ( (DWORD_PTR)pv - (DWORD_PTR)pRS - m_cbSectionHeader ) / m_cbAlignedObject );
    Assert( 0 <= iObject && m_cObjectsPerSection > iObject );
    szOldFile = pRS->m_rgFLD[iObject].szFile;
    lOldLine.lLine = pRS->m_rgFLD[iObject].lLine;
    lNewLine.lLine = lOldLine.lLine;
    Assert( !( lOldLine.uFlags & option ) );
    if ( szOldFile != AtomicExchangePointer( (void **)&pRS->m_rgFLD[iObject].szFile, (void*)szFile ) )
    {
        AssertSz( fFalse, "RM( Alloc ): Somebody else is touching the object, too." );
    }
    lNewLine.uLine = (ULONG)lLine;
    lNewLine.uFlags = option;
    if ( lOldLine.lLine != AtomicExchange( const_cast<LONG *>( &pRS->m_rgFLD[iObject].lLine ), lNewLine.lLine ) )
    {
        AssertSz( fFalse, "RM( Alloc ): Somebody else is touching the object, too." );
    }
#endif  //  MEM_CHECK

    //  write a special status value into the object so that we can determine
    //  its allocation state
    //
    *( (DWORD_PTR*)( (BYTE*)pv + 0 ) ) = DWORD_PTR( &CResourceManager::AllocatedResource );
    *( (DWORD_PTR*)( (BYTE*)pv + m_cbAlignedObject - sizeof( DWORD_PTR ) ) ) = DWORD_PTR( &CResourceManager::AllocatedResource );
}


//======================================
VOID CResourceManager::MarkAsFreed__(
    VOID * const    pv
#ifdef MEM_CHECK
    ,
    LONG            option
#endif  //  MEM_CHECK
    )
{
#ifdef MEM_CHECK
    CResourceSection    *pRS        = NULL;
    INT                 iObject     = 0;
    OBJECTLINEDBGINFO   lOldLine;
    OBJECTLINEDBGINFO   lNewLine;
    AssertSz( NULL != pv, "Attempt to free NULL pointer." );
    pRS = (CResourceSection *)( (DWORD_PTR)pv & maskSection );
    AssertSz( NULL != pRS, "Attempt to free invalid pointer (Section level)" );
    AssertSz( NULL != pRS->m_pRCI, "Attempt to free invalid pointer (RCI level)" );
    AssertSz( pRS->m_pRCI->m_pRMOwner == this, "Attempt to free invalid pointer (belongs to different RM)" );
    Assert( pRS->m_cbSectionHeader == m_cbSectionHeader );
    iObject = (INT)( ( (DWORD_PTR)pv - (DWORD_PTR)pRS - m_cbSectionHeader ) / m_cbAlignedObject );
    Assert( 0 <= iObject && m_cObjectsPerSection > iObject );
    AssertSz( (DWORD_PTR)pv == (DWORD_PTR)pRS + pRS->m_cbSectionHeader + iObject*m_cbAlignedObject,
                "Object is not properly aligned" );
    lOldLine.lLine = pRS->m_rgFLD[iObject].lLine;
    AssertSz( !( lOldLine.uFlags & option ), "Double free detected." );
    AssertSz( ( lOldLine.uFlags & ( RCI_Allocated | RCI_InLA ) ), "Double free detected."  );
    lNewLine.lLine = lOldLine.lLine;
    if ( !( lOldLine.uFlags & RCI_InLA ) )
    {
        lNewLine.uVersion++;
    }
    lNewLine.uFlags = option;
    if ( lOldLine.lLine != AtomicExchange( const_cast<LONG *>( &pRS->m_rgFLD[iObject].lLine ), lNewLine.lLine ) )
    {
        AssertSz( fFalse, "RM( Free ): Somebody else is touching the object." );
    }
#endif  //  MEM_CHECK

    //  write a special status value into the object so that we can determine
    //  its allocation state
    //
    *( (DWORD_PTR*)( (BYTE*)pv + 0 ) ) = DWORD_PTR( &CResourceManager::FreedResource );
    *( (DWORD_PTR*)( (BYTE*)pv + m_cbAlignedObject - sizeof( DWORD_PTR ) ) ) = DWORD_PTR( &CResourceManager::FreedResource );
}


#ifdef MEM_CHECK
//======================================
VOID CResourceManager::IDumpAlloc( const WCHAR* szDumpFile )
{
    CPRINTFFILE cprintf( szDumpFile );

    cprintf( "ResourceID = %i Tag: \"%.*s\" object size = %i\r\n\r\n",
        ResID(),
        sizeof( m_rgchTag ),
        m_rgchTag,
        m_cbObjectSize );

    cprintf( "Address             File(Line)\r\n" );
    cprintf( "==================  ==========================================\r\n" );

    //  dump the data
    DWORD dwObjects = 0;
    CResourceChunkInfo *pRCI;

    pRCI = m_pRCIList;

    while ( NULL != pRCI )
    {
        CResourceSection *pRS;
        pRS = (CResourceSection *)pRCI->m_pvData;
        DWORD_PTR iSection;
        for ( iSection = m_cbChunkSize/cbSectionSize; iSection-- > 0; pRS = (CResourceSection *)( (CHAR *)pRS + cbSectionSize ) )
        {
            INT iObject;
            for ( iObject = 0; iObject < m_cObjectsPerSection; iObject++ )
            {
                OBJECTLINEDBGINFO OLDILine;
                OLDILine.lLine = pRS->m_rgFLD[iObject].lLine;
                if ( RCI_Allocated == OLDILine.uFlags )
                {
                    cprintf( "0x%016I64X  %s(%d)\r\n",
                                    QWORD( (CHAR *)pRS + m_cbSectionHeader + iObject*m_cbAlignedObject ),
                                    pRS->m_rgFLD[iObject].szFile,
                                    pRS->m_rgFLD[iObject].lLine & 0xffff );
                    dwObjects++;
                }
            }
        }
        pRCI = pRCI->m_pRCINext;
    }

    cprintf( "==================  ==========================================\r\n" );
    cprintf( "%i object in %i chunks allocated\r\n\r\n", dwObjects, m_cAllocatedRCI );
}
#endif  //  MEM_CHECK


#ifdef DEBUG
//======================================
VOID AssertValid( JET_RESID resid, const VOID * const pv )
{
    CResourceSection    *pRS        = NULL;
    INT                 iObject     = 0;
    CResourceManager    *pRM;

    AssertSzRTL( NULL != pv, "NULL pointer" );
    pRS = (CResourceSection *)( (DWORD_PTR)pv & maskSection );
    AssertSzRTL( NULL != pRS, "Invalid pointer (Section level)" );
    AssertSzRTL( NULL != pRS->m_pRCI, "Invalid pointer (RCI level)" );
    AssertRTL( (ULONG)pRS->m_cbSectionHeader <= (DWORD_PTR)pv - (DWORD_PTR)pRS );
    pRM = pRS->m_pRCI->m_pRMOwner;
    AssertSzRTL( NULL != pRM, "Invalid pointer (there is no owner RM)" );
    AssertSzRTL( resid == pRM->ResID(), "Invalid pointer (belongs to different RM)" );
    iObject = (INT)( ( (DWORD_PTR)pv - (DWORD_PTR)pRS - pRS->m_cbSectionHeader ) / pRM->m_cbAlignedObject );
    AssertRTL( 0 <= iObject && iObject < pRM->m_cObjectsPerSection );
    AssertSzRTL( (DWORD_PTR)pv == (DWORD_PTR)pRS + pRS->m_cbSectionHeader + iObject*pRM->m_cbAlignedObject,
                "Object is not properly aligned" );

#ifdef MEM_CHECK
    CResourceManager::OBJECTLINEDBGINFO OLDILine;
    OLDILine.lLine  = pRS->m_rgFLD[iObject].lLine;
    AssertSzRTL( OLDILine.uFlags == CResourceManager::RCI_Allocated, "Invalid pointer (object is not allocated)" );
#endif  //  MEM_CHECK

    AssertSzRTL(    *( (DWORD_PTR*)( (BYTE*)pv + 0 ) ) != DWORD_PTR( &CResourceManager::FreedResource ) ||
                    *( (DWORD_PTR*)( (BYTE*)pv + pRM->m_cbAlignedObject - sizeof( DWORD_PTR ) ) ) != DWORD_PTR( &CResourceManager::FreedResource ), "Invalid pointer (object is not allocated)" );
}
#endif  //  DEBUG


//======================================
CResourceManager *CResourceManager::PRMFromResid( JET_RESID resid )
{
    CResourceManager *pRM;
    pRM = CRMContainer::PRMFind( resid );
    if ( NULL != pRM && pRM->m_lookaside.FInit() )
        return pRM;
    return NULL;
}

//======================================
BOOL CResourceManager::FMemoryLeak() { return fMemoryLeak; }

//======================================
//  Class CResource

//======================================
ERR CResource::ErrSetParam( JET_RESOPER resop, DWORD_PTR dwParam )
{
    ERR err = JET_errSuccess;
    if ( NULL != m_pRM )
    {
        err = ErrERRCheck( JET_errAlreadyInitialized );
    }
    else
    {
        switch ( resop )
        {
            case JET_resoperMaxUse:
                err = m_quota.ErrSetQuota( (LONG)dwParam );
                break;
            case JET_resoperEnableMaxUse:
                err = m_quota.ErrEnableQuota( (BOOL)dwParam );
                break;
            default:
                err = ErrERRCheck( JET_errInvalidParameter );
        }
    }
    return err;
}


//======================================
ERR CResource::ErrGetParam( JET_RESOPER resop, DWORD_PTR * const pdwParam ) const
{
    ERR err = JET_errSuccess;
    if ( NULL != pdwParam )
    {
        if ( JET_resoperTag != resop )
        {
            //  cannot clear if we requested tag info because pointer might not be DWORD_PTR aligned
            *pdwParam = 0;
        }
        switch ( resop )
        {
            case JET_resoperTag:
                err = m_pRM->ErrGetParam( resop, pdwParam );
                break;
            case JET_resoperSize:
                err = m_pRM->ErrGetParam( resop, pdwParam );
                break;
            case JET_resoperAlign:
                err = m_pRM->ErrGetParam( resop, pdwParam );
                break;
            case JET_resoperMinUse:
                err = m_pRM->ErrGetParam( resop, pdwParam );
                break;
            case JET_resoperMaxUse:
                *pdwParam = (DWORD_PTR)m_quota.GetQuota();
                break;
            case JET_resoperCurrentUse:
                if ( NULL != m_pRM )
                {
                    *pdwParam = (DWORD_PTR)( m_quota.GetQuota() - m_quota.GetQuotaFree() );
                }
                else
                {
                    err = ErrERRCheck( JET_errNotInitialized );
                }
                break;
            case JET_resoperChunkSize:
                err = m_pRM->ErrGetParam( resop, pdwParam );
                break;
            case JET_resoperLookasideEntries:
                err = m_pRM->ErrGetParam( resop, pdwParam );
                break;
            case JET_resoperRFOLOffset:
                err = m_pRM->ErrGetParam( resop, pdwParam );
                break;
            case JET_resoperObjectsPerChunk:
                err = m_pRM->ErrGetParam( resop, pdwParam );
                break;
            case JET_resoperGuard:
                err = m_pRM->ErrGetParam( resop, pdwParam );
                break;
            case JET_resoperAllocTopDown:
                err = m_pRM->ErrGetParam( resop, pdwParam );
                break;
            case JET_resoperPreserveFreed:
                err = m_pRM->ErrGetParam( resop, pdwParam );
                break;
            case JET_resoperAllocFromHeap:
                err = m_pRM->ErrGetParam( resop, pdwParam );
                break;
            case JET_resoperEnableMaxUse:
                *pdwParam = (DWORD_PTR)( m_quota.GetQuota() != CQuota::QUOTA_MAX );
                break;
            default:
                err = ErrERRCheck( JET_errInvalidParameter );
        }
    }
    else
    {
        err = ErrERRCheck( JET_errInvalidParameter );
    }
    return err;
}

//======================================
BOOL CResource::FCloseToQuota()
{
    LONG cQuota = m_quota.GetQuota();

    // If there is no quota, we can't be close to it.
    if ( CQuota::QUOTA_MAX == cQuota )
    {
        return fFalse;
    }
    
    // An instance that is in recovery has no
    // quota.
    if ( m_pinst != NULL &&
        m_pinst->FRecovering() )
    {
        return fFalse;
    }
    
    LONG cQuotaFree = m_quota.GetQuotaFree();

    // If we have less than 20% free of the quota, we're close
    // to it.
    if ( cQuotaFree <= cQuota * 0.2 )
    {
        return fTrue;
    }
    
    return fFalse;
}

//======================================
ERR CResource::ErrInit(
    JET_RESID resid )   // [IN] RM's resid
//  links to the RM by the given resid
{
    ERR err = JET_errSuccess;
    Assert( NULL == m_pRM );
    CResourceManager *pRM;
    pRM = CResourceManager::PRMFromResid( resid );
    if ( NULL == pRM )
    {
        err = ErrERRCheck( JET_errInternalError );
    }
    else
    {
        if ( pRM->FLink( this ) )
        {
            err = m_quota.ErrInit();
            if ( JET_errSuccess <= err )
            {
                m_pRM = pRM;
            }
            else
            {
                pRM->Unlink( this );
            }
        }
        else
        {
            err = ErrERRCheck( JET_errNotInitialized );
        }
    }
    return err;
}


//======================================
VOID CResource::Term()
{
    CResourceManager *pRM;
    pRM = (CResourceManager *)AtomicExchangePointer( (void **)&m_pRM, NULL );
    if ( NULL != pRM )
    {
        pRM->Unlink( this );
        m_quota.Term();
    }
}


//======================================
VOID CResource::Free( VOID * const pv )
{
    if ( pv )
    {
        Assert( NULL != m_pRM );
        m_pRM->Free( pv );
        m_quota.Release();
    }
}


//======================================
VOID *CResource::PvAlloc_(
#ifdef MEM_CHECK
    __in PCSTR szFile,
    LONG lLine
#endif  //  MEM_CHECK
    )
{
    Assert( NULL != m_pRM );

    Assert( m_pRM->ResID() != JET_residPAGE );  //  this is deprecated

    VOID *pv = NULL;
    if ( m_quota.FAcquire() )
    {
        pv = m_pRM->PvRESAlloc_( szFile, lLine );
        if ( NULL == pv )
        {
            m_quota.Release();
        }
    }
    else
    {
        //  indicate that we should remount the database to clear the out of quota condition
        //
        
        char    szTag[ JET_resTagSize + 1 ]     = { 0 };
        wchar_t wszAdditional[ 64 ]             = { 0 };
        (void)ErrGetParam( JET_resoperTag, (DWORD_PTR *)szTag );

        DWORD_PTR cQuotaMax;
        (void)ErrGetParam( JET_resoperMaxUse, &cQuotaMax );

        //  output the name of the tag and the current quota setting to improve diagnostics
        OSStrCbFormatW( wszAdditional, sizeof( wszAdditional ), L"%hs %I64u", szTag, (QWORD)cQuotaMax );
        
        OSUHAEmitFailureTagEx( m_pinst, HaDbFailureTagRemount, L"d0cec001-e80a-4a13-9bec-8f16fe41102e", wszAdditional );
    }
    return pv;
}


#ifdef DEBUG

//======================================
VOID CResource::AssertValid( const VOID * const pv )
//======================================
{
    Assert( NULL != m_pRM );
    ::AssertValid( m_pRM->ResID(), pv );
}

//======================================
VOID CResource::AssertValid( const JET_RESID resid, const VOID * const pv )
//======================================
{
    ::AssertValid( resid, pv );
}

#endif // DEBUG

//  Determines if a given pointer indicates a valid resource of the given type.
//  This is usually caused by the calling program passing in a bad value (e.g. a
//  previously closed handle/resource).
//
BOOL CResource::FCallingProgramPassedValidJetHandle( _In_ const JET_RESID resid, _In_ const VOID * const pv )
{
    BOOL                fInvalid    = fFalse;
    CResourceSection*   pRS         = NULL;
    CResourceManager*   pRM         = NULL;

    //  normally NULL/MAXINT pointers are caught by the exception handler but
    //  we will shortcut that here to avoid that
    if( NULL == pv || (void *)~0 == pv )
    {
        return fFalse;
    }
    
    //  if ANYONE is allocating from the heap then we can not definitively say
    //  that a given resource is valid.  we can only say that it is definitely
    //  invalid if it has the signature of a freed block.  so we will skip all
    //  but the signature check.  this is no good and should eventually be fixed
    //  somehow (by adding a header and footer to each heap block?)
    //
    if ( CResourceManager::cAllocFromHeap > 0 )
    {
        pRM = CResourceManager::PRMFromResid( resid );
    }
    else
    {
        //  lookup the resource section for the specified pointer
        //
        pRS = (CResourceSection*)( (DWORD_PTR)pv & maskSection );

        //  lookup the resource manager for this resource section
        //
        pRM = pRS->m_pRCI->m_pRMOwner;

        //  verify that we are looking at the correct resource manager
        //
        if ( resid != pRM->ResID() )
        {
            fInvalid = fTrue;
        }

        //  verify that the resource section really goes with the resource manager
        //
        else if ( memcmp( pRS->m_rgchTag, pRM->m_rgchTag, JET_resTagSize ) )
        {
            fInvalid = fTrue;
        }

        else
        {
            Assert( !fInvalid );

            //  determine the (nearest) object index for the pointer in this section
            //
            const DWORD dwOffset = DWORD( (DWORD_PTR)pv - (DWORD_PTR)pRS - pRS->m_cbSectionHeader );
            const DWORD iObject = dwOffset * ( DWORD )pRM->m_1_cbAlignedObject >> ( DWORD )pRM->shfAlignedObject;

            //  verify that the computed object index is valid
            //
            if ( iObject >= (size_t)pRM->m_cObjectsPerSection )
            {
                fInvalid = fTrue;
            }

            //  verify that the pointer is to the start of a valid object
            //
            else if ( (DWORD_PTR)pv != (DWORD_PTR)pRS + pRS->m_cbSectionHeader + iObject * pRM->m_cbAlignedObject )
            {
                fInvalid = fTrue;
            }

#ifdef DEBUG
            // verify the multiplication/right shift generates identical result as division
            const size_t iObjectDiv = ( (DWORD_PTR)pv - (DWORD_PTR)pRS - pRS->m_cbSectionHeader ) / pRM->m_cbAlignedObject;
            const BOOL fInvalidDiv =
                ( size_t )pRM->m_cObjectsPerSection <= iObjectDiv ||
                ( DWORD_PTR )pv != ( DWORD_PTR )pRS + pRS->m_cbSectionHeader + pRM->m_cbAlignedObject * iObjectDiv;

            Assert( !fInvalidDiv == !fInvalid );
#endif
        }
    }

    //  verify that the object is currently not freed
    //
    if (    *( (DWORD_PTR*)( (BYTE*)pv + 0 ) ) == DWORD_PTR( &CResourceManager::FreedResource ) &&
            ( NULL == pRM ||
                *( (DWORD_PTR*)( (BYTE*)pv + pRM->m_cbAlignedObject - sizeof( DWORD_PTR ) ) ) == DWORD_PTR( &CResourceManager::FreedResource ) ) )
    {
        fInvalid = fTrue;
    }

    //  return the validity of this pointer
    //
    return !fInvalid;
}


#ifdef RTM
#else // RTM
static LONG lCRUnitTestCounter = 0;
static CAutoResetSignal asigCRUnitTest( CSyncBasicInfo( "CRUnitTest::asig" ) );
static CSemaphore semCRUnitTest( CSyncBasicInfo( "CRUnitTest::sem" ) );

DWORD CRUnitTest1( DWORD_PTR resid )
{
    CResource CR;
    void *av[1000];
    INT i, j = 0;
    ERR err;
    DWORD_PTR size;
    semCRUnitTest.Acquire();
    CallS( ErrRESGetResourceParam( NULL, (JET_RESID)resid, JET_resoperSize, &size ) );
    Assert( FPowerOf2( cbAlignDefault ) );
    for ( i = 1; (1 << i) < cbAlignDefault; i++ )
    {
        //  NOTHING
    }
    for ( j = 1; (1 << (j+1)) - 1 < (INT)size; j++ )
    {
        // NOTHING
    }
    j -= i - 1;
    if ( j < 1 )
    {
        j = 1;
    }
    const INT max = 1000; // j;
    memset( av, 0, sizeof( *av )*max );
    Call( CR.ErrInit( (JET_RESID)resid ) );
    for ( i = 0; i < 10000; i++ )
    {
        LONG l;
        l = rand() % max;
        if ( NULL == av[l] )
        {
            av[l] = CR.PvRESAlloc();
        }
        else
        {
            CR.Free( av[l] );
            av[l] = NULL;
        }
    }
HandleError:
    for ( i = 0; i < max; i++)
    {
        if ( NULL != av[i] )
        {
            CR.Free( av[i] );
            av[i] = NULL;
        }
    }
    CR.Term();
    if ( AtomicDecrement( &lCRUnitTestCounter ) == 0 )
    {
        asigCRUnitTest.Set();
    }
    return 0;
}

DWORD CRUnitTest2( DWORD_PTR resid )
{
    CResource CR;
    void *av[1000];
    INT i, j = 0;
    ERR err;
    DWORD_PTR size;
    INT         iHead   = 0;
    INT         iTail   = 0;

    semCRUnitTest.Acquire();
    CallS( ErrRESGetResourceParam( NULL, (JET_RESID)resid, JET_resoperSize, &size ) );
    Assert( FPowerOf2( cbAlignDefault ) );
    for ( i = 1; (1 << i) < cbAlignDefault; i++ )
    {
        //  NOTHING
    }
    for ( j = 1; (1 << (j+1)) - 1 < (INT)size; j++ )
    {
        // NOTHING
    }
    j -= i - 1;
    if ( j < 1 )
    {
        j = 1;
    }

    const INT   max     = 10*j;

    memset( av, 0, sizeof( *av )*max );
    Call( CR.ErrInit( (JET_RESID)resid ) );

    for ( i = 0; i < 10000; i++ )
    {
        LONG l;
        l = rand() % ( max - 1 );

        //  Allocate if there is a free space
//      if ( ( 0 == l && ( ( iHead + 1 + max - iTail ) % max ) != 0 ) || NULL == av[iTail] )
        if ( l >= ( ( iHead + max - iTail ) % max ) )
        {
            av[iHead] = CR.PvRESAlloc();
            if ( NULL != av[iHead] )
            {
                iHead = ( iHead + 1 ) % max;
            }
        }
        else
        {
            Enforce( NULL != av[iTail] );
            CR.Free( av[iTail] );
            av[iTail] = NULL;
            iTail = ( iTail + 1 ) % max;
        }
    }
HandleError:
    for ( i = iTail; NULL != av[i]; i = ( i + 1 ) % max )
    {
        CR.Free( av[i] );
        av[i] = NULL;
    }
    CR.Term();
    if ( AtomicDecrement( &lCRUnitTestCounter ) == 0 )
    {
        asigCRUnitTest.Set();
    }
    return 0;
}

//======================================
VOID CResource::UnitTest()
{
    srand( (unsigned)TickOSTimeCurrent() );
    for ( JET_RESID resid = (JET_RESID)(JET_residNull + 1); resid != JET_residINST; resid = (JET_RESID)(resid + 1) )
    {
        if ( JET_residVERBUCKET == resid || JET_residPAGE == resid )
        {
            continue;
        }
        char szName[JET_resTagSize + 1];
        CallS( ErrRESGetResourceParam( NULL, resid, JET_resoperTag, (DWORD_PTR *)szName ) );
        (*CPRINTFSTDOUT::PcprintfInstance())( "%-4.4s:", szName );
        DWORD dwTime = TickOSTimeCurrent();
        for ( INT j = 1; j < 40; j++ )
        {
            lCRUnitTestCounter = j;
            (*CPRINTFSTDOUT::PcprintfInstance())( "%2i", j );
            for ( INT i = 0; i < j; i++ )
            {
                THREAD hthread;
                CallS( ErrUtilThreadCreate( CRUnitTest1, 0, priorityNormal, &hthread, (DWORD_PTR)resid ) );
            }
            semCRUnitTest.Release( j );
            asigCRUnitTest.Wait();
        }
        (*CPRINTFSTDOUT::PcprintfInstance())( "(%i)\n", TickOSTimeCurrent() - dwTime );
    }
    return;
}
#endif // RTM

//  ================================================================
template< INT T_cLookasideEntries >
class CResourceTestFixture : public JetTestFixture
//  ================================================================
//
//  This fixture contains an unitialized CResource
//
//-
{
    private:
        CResource m_resource;
        static const JET_RESID m_resid = JET_residTest;

        // Used to store pointers during testing. All pointers allocated
        // here will be freed in the TearDown_() function.
        void* m_rgpv[1024];

    public:
        CResourceTestFixture() { memset( m_rgpv, 0, sizeof(m_rgpv) ); }
        ~CResourceTestFixture() {}
        
    protected:
    
        virtual bool SetUp_()
        {
            // Register the resid. The test resource manager may already be initialized
            // so we have to tear it down first.

            TearDown_();
            
            if ( !CRMContainer::FAdd( m_resid ) )
                return false;

            CResourceManager * const prm = CRMContainer::PRMFind( m_resid );
            if ( !prm )
                return false;
            
            ERR err;
            Call( prm->ErrSetParam( JET_resoperTag, (DWORD_PTR)"test" ) );
            Call( prm->ErrSetParam( JET_resoperSize, 64 ) );
            Call( prm->ErrSetParam( JET_resoperAlign, cbAlignDefault ) );
            Call( prm->ErrSetParam( JET_resoperChunkSize, cbChunkDefault) );
            Call( prm->ErrSetParam( JET_resoperLookasideEntries, T_cLookasideEntries ) );
            Call( prm->ErrSetParam( JET_resoperMaxUse, CQuota::QUOTA_MAX ) );
            Call( prm->ErrSetParam( JET_resoperRFOLOffset, cbRFOLOffsetDefault ) );
            Call( prm->ErrSetParam( JET_resoperMinUse, 0 ) );

            Call( prm->ErrInit() );
            
            return true;

        HandleError:
            return false;
        }

        // Free any items we allocated, destroy the resource
        // and unregister the resid
        void TearDown_()
        {
            for(INT i = 0; i < _countof(m_rgpv); ++i )
            {
                if( m_rgpv[i] )
                {
                    m_resource.Free( m_rgpv[i] );
                    m_rgpv[i] = NULL;
                }
            }
            
            m_resource.Term();
            CResourceManager * const prm = CRMContainer::PRMFind( m_resid );
            if ( prm )
                prm->Term();
            
            CRMContainer::Delete( m_resid );
        }
        
    public:

        // Allocating more objects than allowed by the quota fails
        void TestOutOfQuota()
        {
            const DWORD_PTR cresourcesMax = 100;
            C_ASSERT( cresourcesMax <= _countof(m_rgpv) );
            CHECK( JET_errSuccess == m_resource.ErrSetParam( JET_resoperMaxUse, cresourcesMax ) );
            CHECK( JET_errSuccess == m_resource.ErrInit( m_resid ) );

            for( INT i = 0; i < cresourcesMax; ++i )
            {
                m_rgpv[i] = m_resource.PvRESAlloc();
                CHECK( NULL != m_rgpv[i]);
            }

            CHECK( NULL == m_resource.PvRESAlloc() );
        }

        // Test that the quota parameter can be set and retrieved
        void TestSetAndRetrieveQuota()
        {
            const DWORD_PTR cresourcesExpected = 999;
            DWORD_PTR cresourcesActual = 0;
            CHECK( JET_errSuccess == m_resource.ErrSetParam( JET_resoperMaxUse, cresourcesExpected ) );
            CHECK( JET_errSuccess == m_resource.ErrGetParam( JET_resoperMaxUse, &cresourcesActual ) );
            CHECK( cresourcesExpected == cresourcesActual );
        }

        // Test create/free in a tight loop
        void TestCreateFree()
        {
            CHECK( JET_errSuccess == m_resource.ErrInit( m_resid ) );
            TestOneCreateFree( this );
        }

        // Test that multithreaded creates and frees don't crash
        void TestMultithreadedCreateFree()
        {
            CHECK( JET_errSuccess == m_resource.ErrInit( m_resid ) );

            CGPTaskManager tasklist;
            CHECK( JET_errSuccess == tasklist.ErrTMInit( 128 ) );

#ifdef DEBUG
            INT cTasks = 2;
#else
            INT cTasks = 1000;
#endif

            for ( INT i = 0; i < cTasks; i++ )
            {
                CHECK( JET_errSuccess == tasklist.ErrTMPost( (CGPTaskManager::PfnCompletion)TestOneCreateFree, this ) );
            }

            while ( tasklist.CPostedTasks() )
            {
                UtilSleep( 1 );
            }
        }
        
        static void TestOneCreateFree( CResourceTestFixture *pThis )
        {
            const INT cHoard = 1000;
            void *rgpv[ cHoard ];
            
            for ( INT i = 0; i < cHoard; i++ )
            {
                rgpv[ i ] = pThis->m_resource.PvRESAlloc();
            }

            for ( INT i = 0; i < 10000; i++ )
            {
                void *pvTmp = pThis->m_resource.PvRESAlloc();
                pThis->m_resource.Free( pvTmp );
            }

            for ( INT i = 0; i < cHoard; i++ )
            {
                pThis->m_resource.Free( rgpv[ i ] );
            }

        }

        void TestDoubleFree()
        {
            CHECK( JET_errSuccess == m_resource.ErrInit( m_resid ) );

            void * pv = m_resource.PvRESAlloc();
            m_resource.Free( pv );

            // Ignore the double free assert that will fire here to try to catch the invalid case of a 
            // block being given out of use twice after being double free'd.
            m_resource.Free( pv );
            void * pv2 = m_resource.PvRESAlloc();
            CHECK( pv2 == pv );
            void * pv3 = m_resource.PvRESAlloc();
            CHECK( pv3 == pv );
        }
};

#ifdef ENABLE_JET_UNIT_TEST

typedef CResourceTestFixture< cpresLookasideDefault > CResourceTestFixtureDefault;
typedef CResourceTestFixture< 0 >                     CResourceTestFixtureNoLookaside;

static const JetTestCaller<CResourceTestFixtureDefault> crtf1("CResource.OutOfQuota", &CResourceTestFixtureDefault::TestOutOfQuota);
static const JetTestCaller<CResourceTestFixtureDefault> crtf2("CResource.SetAndRetrieveQuota", &CResourceTestFixtureDefault::TestSetAndRetrieveQuota);
static const JetTestCaller<CResourceTestFixtureDefault> crtf3("CResource.CreateFree", &CResourceTestFixtureDefault::TestCreateFree);
static const JetTestCaller<CResourceTestFixtureDefault> crtf4("CResource.MultithreadedCreateFree", &CResourceTestFixtureDefault::TestMultithreadedCreateFree);
static const JetTestCaller<CResourceTestFixtureNoLookaside> crtf5("CResource.CreateFreeWithNoLookaside", &CResourceTestFixtureNoLookaside::TestCreateFree);
static const JetTestCaller<CResourceTestFixtureNoLookaside> crtf6("CResource.MultithreadedCreateFreeWithNoLookaside", &CResourceTestFixtureNoLookaside::TestMultithreadedCreateFree);

#endif // ENABLE_JET_UNIT_TEST


//======================================
//  class CRMContainer

INLINE CRMContainer::CRMContainer( JET_RESID resid ) :
        m_pNext( NULL ),
        m_RM( resid )
    {}


//======================================
CResourceManager *CRMContainer::PRMFind( JET_RESID resid )
{
    CRMContainer *pRMC;
    pRMC = g_pRMContainer;
    while ( NULL != pRMC && resid != pRMC->m_RM.ResID() )
    {
        pRMC = pRMC->m_pNext;
    }
    if ( NULL != pRMC )
    {
        return &pRMC->m_RM;
    }
    return NULL;
}


//======================================
BOOL CRMContainer::FAdd( JET_RESID resid )
{
    CRMContainer *pRMC = NULL;
    if ( !fOSRMPreinitPostTerm )
    {
        s_critAddDelete.Enter();
    }
    if ( JET_residNull != resid && NULL == PRMFind( resid ) )
    {
        pRMC = new CRMContainer( resid );
        if ( NULL != pRMC )
        {
            pRMC->m_pNext = g_pRMContainer;
            g_pRMContainer = pRMC;
        }
    }
    if ( !fOSRMPreinitPostTerm )
    {
        s_critAddDelete.Leave();
    }
    return NULL != pRMC ? fTrue: fFalse;
}


//======================================
VOID CRMContainer::Delete( JET_RESID resid )
{
    if ( !fOSRMPreinitPostTerm )
    {
        s_critAddDelete.Enter();
    }
    CRMContainer *pRMC;
    pRMC = (CRMContainer *)((CHAR *)&g_pRMContainer - OffsetOf( CRMContainer, m_pNext ));
    while ( NULL != pRMC->m_pNext && resid != pRMC->m_pNext->m_RM.ResID() )
    {
        pRMC = pRMC->m_pNext;
    }
    if ( NULL != pRMC->m_pNext )
    {
        CRMContainer *pRMCTemp;
        pRMCTemp = pRMC->m_pNext;
        pRMC->m_pNext = pRMCTemp->m_pNext;
        delete pRMCTemp;
    }
    if ( !fOSRMPreinitPostTerm )
    {
        s_critAddDelete.Leave();
    }
}

//======================================
INLINE VOID CRMContainer::CalcAllocatedObjects( JET_RESID resid, void* pvBuf )
{
    if ( !fOSRMPreinitPostTerm )
    {
        s_critAddDelete.Enter();
    }

    if ( pvBuf )
    {
        CResourceManager * pRM = CRMContainer::PRMFind( resid );
        if ( NULL == pRM )
        {
            goto HandleError;
        }

        *( (unsigned __int64*) pvBuf ) = pRM->CbAllocated();
    }

HandleError:

    if ( !fOSRMPreinitPostTerm )
    {
        s_critAddDelete.Leave();
    }
}

//======================================
INLINE VOID CRMContainer::CalcUsedObjects( JET_RESID resid, void* pvBuf )
{
    if ( !fOSRMPreinitPostTerm )
    {
        s_critAddDelete.Enter();
    }

    if ( pvBuf )
    {
        CResourceManager * pRM = CRMContainer::PRMFind( resid );
        if ( NULL == pRM )
        {
            goto HandleError;
        }

        *( (unsigned __int64*) pvBuf ) = pRM->CbUsed();
    }
    
    HandleError:
    
    if ( !fOSRMPreinitPostTerm )
    {
        s_critAddDelete.Leave();
    }
}

//======================================
INLINE VOID CRMContainer::CalcQuotaObjects( JET_RESID resid, void* pvBuf )
{
    if ( !fOSRMPreinitPostTerm )
    {
        s_critAddDelete.Enter();
    }

    if ( pvBuf )
    {
        CResourceManager * pRM = CRMContainer::PRMFind( resid );
        if ( NULL == pRM )
        {
            goto HandleError;
        }

        *( (unsigned __int64*) pvBuf ) = pRM->CbQuota();
    }
    
HandleError:
    
    if ( !fOSRMPreinitPostTerm )
    {
        s_critAddDelete.Leave();
    }
}

//======================================
//  Global functions

//======================================
LOCAL CResource *PCResourceFromResid( INST * const pinst, JET_RESID resid )
{
    if ( pinstNil != pinst )
    {
        switch ( resid )
        {
            case JET_residFCB:
                return &pinst->m_cresFCB;
            case JET_residFUCB:
                return &pinst->m_cresFUCB;
            case JET_residIDB:
                return &pinst->m_cresIDB;
            case JET_residPIB:
                return &pinst->m_cresPIB;
            case JET_residSCB:
                return &pinst->m_cresSCB;
            case JET_residTDB:
                return &pinst->m_cresTDB;
            case JET_residVERBUCKET:
                return &pinst->m_pver->m_cresBucket;
        }
    }
    return NULL;
}

//======================================
BOOL FResCloseToQuota( INST * const pinst, JET_RESID resid )
{
    CResource * pres = PCResourceFromResid( pinst, resid );

    return pres->FCloseToQuota();
}

//======================================
ERR ErrRESSetResourceParam(
        INST * const pinst,
        JET_RESID resid,
        JET_RESOPER resop,
        DWORD_PTR dwParam )
{
    CResource *pcres;
    pcres = PCResourceFromResid( pinst, resid );
    //  set per instance parameter

    Assert( resid != JET_residPAGE );   //  this is deprecated

    if ( NULL != pcres )
    {
        return pcres->ErrSetParam( resop, dwParam );
    }
    //  some of the parameters cannot be changed for the
    //  default resources
    if ( resid < JET_residMax )
    {
        switch ( resop )
        {
            case JET_resoperSize:
                //  the size of only a few resources is mutable
                //
                if ( JET_residVERBUCKET != resid )
                {
                    return ErrERRCheck( JET_errInvalidParameter );
                }
                break;

            case JET_resoperTag:
                return ErrERRCheck( JET_errInvalidParameter );

            default:
                break;
        }
    }

    //  set global parameter
    CResourceManager *pRM;

    pRM = CRMContainer::PRMFind( resid );
    if ( NULL == pRM )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    return pRM->ErrSetParam( resop, dwParam );
}

//======================================
ERR ErrRESGetResourceParam(
        INST * const pinst,
        JET_RESID resid,
        JET_RESOPER resop,
        DWORD_PTR *pdwParam )
{
    ERR err = JET_errSuccess;
    if ( NULL == pdwParam )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    CResource *pcres;
    pcres = PCResourceFromResid( pinst, resid );
    //  set per instance parameter
    if ( NULL != pcres )
    {
        return pcres->ErrGetParam( resop, pdwParam );
    }
    //  set global parameter
    CResourceManager *pRM;

    pRM = CRMContainer::PRMFind( resid );
    if ( NULL == pRM )
    {
        CallR( ErrERRCheck( JET_errInvalidParameter ) );
    }
    return pRM->ErrGetParam( resop, pdwParam );
}


//  Global resources
CResource   RESINST;
CResource   RESLOG;
CResource   RESSPLIT;
CResource   RESSPLITPATH;
CResource   RESMERGE;
CResource   RESMERGEPATH;
CResource   RESKEY;
CResource   RESBOOKMARK;
CResource   RESLRUKHIST;

//======================================
VOID OSRMTerm()
{
    JET_RESID resid;
    //  Terminate global resources
    RESLOG.Term();
    RESINST.Term();
    RESMERGEPATH.Term();
    RESMERGE.Term();
    RESSPLITPATH.Term();
    RESSPLIT.Term();
    RESKEY.Term();
    RESBOOKMARK.Term();
    RESLRUKHIST.Term();

    for ( resid = (JET_RESID)(JET_residNull + 1); resid < JET_residMax; resid = (JET_RESID)(resid + 1) )
    {
        CResourceManager *pRM;
        pRM = CRMContainer::PRMFind( resid );
        if ( NULL != pRM )
        {
            pRM->Term();
        }
    }
}


//======================================
ERR ErrOSRMInit()
{
    ERR err = JET_errSuccess;
    JET_RESID resid;

    for ( resid = (JET_RESID)(JET_residNull + 1); resid < JET_residMax; resid = (JET_RESID)(resid + 1) )
    {
#ifdef RTM
#else // RTM
        // OOM prepare for resource unit tests
//      CallS( ErrRESSetResourceParam( NULL, resid, JET_resoperMaxUse, 1 ) );
#endif // RTM
        CResourceManager *pRM;
        pRM = CRMContainer::PRMFind( resid );
        if ( NULL != pRM )
        {
            Call( pRM->ErrInit() );
        }
    }

#ifdef RTM
#else // RTM
    // run some unit tests
/// CResource::UnitTest();
/// Call( ErrERRCheck( JET_errOutOfMemory ) );
#endif // RTM
    //  Initialize global resources
    Call( RESSPLIT.ErrInit( JET_residSPLIT ) );
    Call( RESSPLITPATH.ErrInit( JET_residSPLITPATH ) );
    Call( RESMERGE.ErrInit( JET_residMERGE ) );
    Call( RESMERGEPATH.ErrInit( JET_residMERGEPATH ) );
    Call( RESINST.ErrInit( JET_residINST ) );
    Call( RESLOG.ErrInit( JET_residLOG ) );
    Call( RESKEY.ErrInit( JET_residKEY ) );
    Call( RESBOOKMARK.ErrInit( JET_residBOOKMARK ) );
    Call( RESLRUKHIST.ErrInit( JET_residLRUKHIST ) );
    return err;
HandleError:
    OSRMTerm();
    return err;
}


//======================================
VOID OSRMPostterm()
{
    JET_RESID resid;
    fOSRMPreinitPostTerm = fTrue;

#ifdef RM_STATISTICS
    CPRINTFFILE cprintf( "rmstat.txt" );
    cprintf( "\r\n\r\n%ws\r\n", WszUtilProcessName() );
    cprintf( "RESID   Alloc  (RFOLWait)   Free   (WaitChnk WaitLoop WaitSucc) ( LAGACX  LAGFst LAGHit) ( LARACX  LARFst LARHit)\r\n" );
    cprintf( "=================================================================================================================" );
    g_pcprintf = &cprintf;
#endif // RM_STATISTICS
    for ( resid = (JET_RESID)(JET_residNull + 1); resid < JET_residMax; resid = (JET_RESID)(resid + 1) )
    {
        CResourceManager *pRM;
        pRM = CRMContainer::PRMFind( resid );
        if ( NULL != pRM )
        {
            CRMContainer::Delete( resid );
        }
    }
#ifdef RM_STATISTICS
    g_pcprintf = NULL;
#endif // RM_STATISTICS
    EnforceSz( !CResourceManager::FMemoryLeak() || FUtilProcessAbort() || FRFSKnownResourceLeak(), "CResourceManagerLeak" );
}

#include "_bf.hxx"

const struct
{
    JET_RESID m_resid;
    CHAR m_rgchTag[JET_resTagSize+1];
    LONG cbObjectSize;
    LONG cbObjectAlign;
    DWORD_PTR cbChunkSize;
    LONG cLookasideSize;
    LONG cObjectsMax;
    BOOL fGuard;
    LONG cObjectsMin;
    LONG cbRFOLOffset;
} RMDefaults[] = {{
    // resid            tag     object size                     object align        chunk size      lookaside size          objects max         fGuard  objects min     RFOL
    JET_residFCB,       "FCB",  sizeof( FCB ),                  cbAlignDefault,     cbChunkDefault, cpresLookasideDefault,  CQuota::QUOTA_MAX,  fFalse, 0,              cbRFOLOffsetDefault },{
    JET_residFUCB,      "FUCB", sizeof( FUCB ),                 cbAlignDefault,     cbChunkDefault, cpresLookasideDefault,  CQuota::QUOTA_MAX,  fFalse, 0,              cbRFOLOffsetDefault },{
    JET_residTDB,       "TDB",  sizeof( TDB ),                  cbAlignDefault,     cbChunkDefault, cpresLookasideDefault,  CQuota::QUOTA_MAX,  fFalse, 0,              cbRFOLOffsetDefault },{
    JET_residIDB,       "IDB",  sizeof( IDB ),                  cbAlignDefault,     cbChunkDefault, cpresLookasideDefault,  CQuota::QUOTA_MAX,  fFalse, 0,              cbRFOLOffsetDefault },{
    JET_residPIB,       "PIB",  sizeof( PIB ),                  cbAlignDefault,     cbChunkDefault, cpresLookasideDefault,  CQuota::QUOTA_MAX,  fFalse, 0,              cbRFOLOffsetDefault },{
    JET_residSCB,       "SCB",  sizeof( SCB ),                  cbAlignDefault,     cbChunkDefault, cpresLookasideDefault,  CQuota::QUOTA_MAX,  fFalse, 0,              cbRFOLOffsetDefault },{
    JET_residVERBUCKET, "VERB", cbBucketLegacy - cbRESHeader,   cbAlignDefault,     1024 * 1024,    8,                      CQuota::QUOTA_MAX,  fFalse, cbucketDefault, cbRFOLOffsetDefault },{
    JET_residPAGE,      "PAGE", g_cbPageDefault,                g_cbPageDefault,    256 * 1024,     cpresLookasideDefault,  CQuota::QUOTA_MAX,  fFalse, 0,              cbRFOLOffsetDefault },{
    JET_residSPLIT,     "SPT",  sizeof( SPLIT ),                cbAlignDefault,     cbChunkDefault, cpresLookasideDefault,  CQuota::QUOTA_MAX,  fFalse, 0,              cbRFOLOffsetDefault },{
    JET_residSPLITPATH, "SPTP", sizeof( SPLITPATH ),            cbAlignDefault,     cbChunkDefault, cpresLookasideDefault,  CQuota::QUOTA_MAX,  fFalse, 0,              cbRFOLOffsetDefault },{
    JET_residMERGE,     "MRG",  sizeof( MERGE ),                cbAlignDefault,     cbChunkDefault, cpresLookasideDefault,  CQuota::QUOTA_MAX,  fFalse, 0,              cbRFOLOffsetDefault },{
    JET_residMERGEPATH, "MRGP", sizeof( MERGEPATH ),            cbAlignDefault,     cbChunkDefault, cpresLookasideDefault,  CQuota::QUOTA_MAX,  fFalse, 0,              cbRFOLOffsetDefault },{
    JET_residINST,      "INST", sizeof( INST ),                 cbAlignDefault,     cbChunkDefault, 0,                      CQuota::QUOTA_MAX,  fFalse, 0,              cbRFOLOffsetDefault },{
    JET_residLOG,       "LOG",  sizeof( LOG ),                  cbAlignDefault,     cbChunkDefault, 0,                      CQuota::QUOTA_MAX,  fFalse, 0,              cbRFOLOffsetDefault },{
    JET_residKEY,       "KEY",  cbKeyAlloc,                     cbAlignDefault,     cbChunkDefault, cpresLookasideDefault,  CQuota::QUOTA_MAX,  fFalse, 0,              cbRFOLOffsetDefault },{
    JET_residBOOKMARK,  "BM",   cbBookmarkAlloc,                cbAlignDefault,     cbChunkDefault, cpresLookasideDefault,  CQuota::QUOTA_MAX,  fFalse, 0,              cbRFOLOffsetDefault },{
    JET_residLRUKHIST,  "HIST", sizeof( BFLRUK::CHistory ),     sizeof( void* ),    cbChunkDefault, cpresLookasideDefault,  CQuota::QUOTA_MAX,  fFalse, 0,              cbRFOLOffsetDefault },{
    JET_residRBSBuf  ,  "RBS",  sizeof( CSnapshotBuffer ),      cbAlignDefault,     cbChunkDefault, cpresLookasideDefault,  CQuota::QUOTA_MAX,  fFalse, 0,              cbRFOLOffsetDefault },{
    JET_residTest,      "TEST", 64,                             cbAlignDefault,     cbChunkDefault, cpresLookasideDefault,  CQuota::QUOTA_MAX,  fFalse, 0,              cbRFOLOffsetDefault }
};

//======================================
BOOL FOSRMPreinit()
{
    ERR err = JET_errSuccess;
    INT i;
    BOOL fGuardOverride = fFalse;
    BOOL fGuardPage = fFalse;
    WCHAR wszBuffer[4];

    EnforceSz( 0 == ( cbRESHeader % cbCacheLine ), "NonCacheAlignedCbRESHeader" );

    Assert( cbBucketLegacy > cbRESHeader );
    
    cbSectionSize = OSMemoryPageReserveGranularity();
    Assert( 0 < cbSectionSize );
    Assert( FPowerOf2( cbSectionSize ) );

    cbSectionSize = min( cbSectionSize, cbChunkDefault );

    //  Insufficient bits allocation to simulate div with mul/sar in CResource::FCallingProgramPassedValidJetHandle()
    EnforceSz( cbSectionSize <= ( 1 << CResourceManager::shfAlignedObject ), "CResourceInsufficientBits" );

    //  read the override registry key for the guard page
    if ( FOSConfigGet ( L"Resource Manager", L"Guard Page", wszBuffer, sizeof( wszBuffer ) ) &&
        wszBuffer[0] )
    {
        fGuardOverride = fTrue;
        fGuardPage = _wtol (wszBuffer) != 0? fTrue: fFalse;
    }
    
    //  initialize the resource managers with their default parameters
    for ( i = 0; i < sizeof( RMDefaults ) / sizeof( RMDefaults[0] ); i++ )
    {
        CResourceManager *pRM;
        if ( !CRMContainer::FAdd( RMDefaults[i].m_resid ) )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }
        pRM = CRMContainer::PRMFind( RMDefaults[i].m_resid );
        Assert( NULL != pRM );
        CallA( pRM->ErrSetParam( JET_resoperTag, (DWORD_PTR)RMDefaults[i].m_rgchTag ) );
        CallA( pRM->ErrSetParam( JET_resoperSize, RMDefaults[i].cbObjectSize ) );
        CallA( pRM->ErrSetParam( JET_resoperAlign, RMDefaults[i].cbObjectAlign ) );
        CallA( pRM->ErrSetParam( JET_resoperChunkSize, RMDefaults[i].cbChunkSize ) );
        CallA( pRM->ErrSetParam( JET_resoperLookasideEntries, RMDefaults[i].cLookasideSize ) );
        CallA( pRM->ErrSetParam( JET_resoperMaxUse, RMDefaults[i].cObjectsMax ) );
        CallA( pRM->ErrSetParam( JET_resoperRFOLOffset, RMDefaults[i].cbRFOLOffset ) );
        CallA( pRM->ErrSetParam( JET_resoperMinUse, RMDefaults[i].cObjectsMin ) );

//      //  read the override registry key for the guard page for each resource
//      _TCHAR szPath[256];
//      _stprintf( szPath, _T( "Resource Manager/%.*s" ), sizeof( RMDefaults[i].m_rgchTag ), RMDefaults[i].m_rgchTag );
//
//      if ( FOSConfigGet ( szPath, _T( "Guard Page" ), szBuffer, sizeof( szBuffer ) ) &&
//          szBuffer[0] )
//          {
//          BOOL fIndividualGuardPage = _ttol (szBuffer) != 0? fTrue: fFalse;
//          //  ignore the error because we might try to set guard page for someting
//          //  that is not possible to be guarded. Version buckets for example.
//          pRM->ErrSetParam( JET_resoperGuard, fIndividualGuardPage );
//          }
//      else
        if ( fGuardOverride )
        {
            //  ignore the error because we might try to set guard page for someting
            //  that is not possible to be guarded. Version buckets for example.
            pRM->ErrSetParam( JET_resoperGuard, fGuardPage );
        }
        else
        {
            //  by default we should know if the guard page is settable or not
            Call( pRM->ErrSetParam( JET_resoperGuard, RMDefaults[i].fGuard ) );
        }
    }

    fOSRMPreinitPostTerm = fFalse;
    return fTrue;

HandleError:
    fOSRMPreinitPostTerm = fFalse;
    OSRMPostterm();
    return fFalse;
}

#ifdef DEBUG
INT CProfileStorage::iMode = PROFILEMODE_NONE;
LONG volatile CProfileStorage::g_iPSIndex = opMax;
CProfileStorage *CProfileStorage::pPSFirst = NULL;
CProfileStorage * volatile CProfileStorage::pPSLast = NULL;
QWORD CProfileStorage::csecTimeOffset = 0;
FILE *CProfileStorage::pFile = NULL;

CProfileStorage::CProfileStorage( const char * const szName ) :
    m_szName( szName ),
    m_dhrtTotal( 0 ),
    m_dhrtMin( HRT( ~0 ) ),
    m_dhrtMax( 0 ),
    m_iCalls( 0 ),
    m_critSummary( CLockBasicInfo( CSyncBasicInfo( "CProfileStorage::m_critSummary" ), 0, 0 ) ),
    m_pPSNext( NULL )
{
    CProfileStorage *pPS;
    OSSYNC_FOREVER
    {
        pPS = pPSLast;
        //  add the profile storage to the list of all PSes
        if ( AtomicCompareExchangePointer( (VOID **)&pPSLast, pPS, this ) == pPS )
        {
            break;
        }
    }
    if ( NULL != pPS )
    {
        pPS->m_pPSNext = this;
    }
    else
    {
        pPSFirst = this;
    }
#ifdef PROFILE_JET_API
    extern CProfileStorage ProfileData[];
    if ( &ProfileData[0] <= this && &ProfileData[opMax] >= this )
    {
        m_iIndex = this - &ProfileData[0];
    }
    else
    {
        m_iIndex = AtomicIncrement( const_cast<LONG *>( &g_iPSIndex ) );
    }
#else // PROFILE_JET_API
    m_iIndex = AtomicIncrement( const_cast<LONG *>( &g_iPSIndex ) );
#endif // PROFILE_JET_API
}

CProfileStorage::~CProfileStorage()
{
    //  check that storage is not an auto variable
    EnforceSz( FUtilProcessAbort() || NULL == pFile, "ProfileStorageNotGlobalOrStatic." );
}

//======================================
VOID CProfileStorage::Update( const HRT hrtStart, const HRT dhrtExecute )
{
    if ( PROFILEMODE_NONE == Mode() || opMax == m_iIndex )
    {
        return;
    }
    if ( opMax > m_iIndex && !( PROFILEMODE_LOG_JET_OP & Mode() ) )
    {
        return;
    }

    char szT[50] = "";
    if ( PROFILEMODE_TIME & Mode() )
    {
        QWORD csecTime  = hrtStart/HrtHRTFreq() + csecTimeOffset;
        INT hour        = (INT)((csecTime/3600)%24);
        INT min         = (INT)((csecTime/60)%60);
        INT sec         = (INT)((csecTime)%60);
        OSStrCbFormatA( szT + strlen(szT), sizeof(szT)-strlen(szT), "%02d:%02d:%02d", hour, min, sec );
    }
    if ( PROFILEMODE_COUNT & Mode() )
    {
        if ( NULL != m_szName )
        {
            OSStrCbFormatA( szT + strlen(szT), sizeof(szT)-strlen(szT), "0x%X %-25s", DwUtilThreadId(), m_szName );
        }
        else
        {
            OSStrCbFormatA( szT + strlen(szT), sizeof(szT)-strlen(szT), "0x%X ProfileCounter#%-10i", DwUtilThreadId(), m_iIndex );
        }
    }
    if ( PROFILEMODE_TIMING & Mode() )
    {
        OSStrCbFormatA( szT + strlen(szT), sizeof(szT)-strlen(szT), " %.3f ms", dhrtExecute * 1000.0/HrtHRTFreq() );
    }
    if ( 0 != szT[ 0 ] )
    {
        OSStrCbFormatA( szT + strlen(szT), sizeof(szT)-strlen(szT), "\r\n" );
        fwrite( szT, strlen( szT ), 1, pFile );
        fflush( pFile );
    }
    if ( PROFILEMODE_SUMMARY & Mode() )
    {
        ENTERCRITICALSECTION enter( &m_critSummary );
        m_iCalls++;
        m_dhrtTotal += dhrtExecute;
        if ( m_dhrtMin > dhrtExecute )
        {
            m_dhrtMin = dhrtExecute;
        }
        if ( m_dhrtMax < dhrtExecute )
        {
            m_dhrtMax = dhrtExecute;
        }
    }
}

//======================================
VOID CProfileStorage::Print( const char * const sz )
{
    if ( NULL != pFile )
    {
        fprintf( pFile, "%s\r\n", sz );
        fflush( pFile );
    }
}

//======================================
VOID CProfileStorage::Summary()
{
    Assert( PROFILEMODE_SUMMARY & Mode() );
    if ( 0 == m_iCalls )
    {
        return;
    }

    if ( NULL != m_szName )
    {
        fprintf( pFile, "%-25s", m_szName );
    }
    else
    {
        fprintf( pFile, "ProfileCounter#%-10i", m_iIndex );
    }
    fprintf( pFile, " %9i %10.3f %10.3f %10.3f %10.3f\r\n",
        m_iCalls,
        m_dhrtTotal * 1000.0 / HrtHRTFreq(),
        m_dhrtMin * 1000.0 / HrtHRTFreq(),
        m_dhrtMax * 1000.0 / HrtHRTFreq(),
        m_dhrtTotal * 1000.0 / HrtHRTFreq() / m_iCalls );
}

//======================================
ERR CProfileStorage::ErrInit( const char * const szFileName, const INT iSetMode )
{
    ERR err = JET_errSuccess;
    Assert( NULL == pFile );
    //  no file name specified
    iMode = iSetMode;
    if ( 0 == szFileName[0] )
    {
        iMode = 0;
    }
    if ( 0 < iMode )
    {
        (void) fopen_s( &pFile, szFileName, "at" );
        if ( NULL != pFile )
        {
            DATETIME datetime;
            csecTimeOffset = HrtHRTCount();
            UtilGetCurrentDateTime( &datetime );
            csecTimeOffset =
                (HRT)(datetime.hour*3600 + datetime.minute*60 + datetime.second)
                - csecTimeOffset/HrtHRTFreq();
            fprintf(
                pFile,
                "\r\n%02d:%02d:%02d [BEGIN %ws]\r\n",
                datetime.hour,
                datetime.minute,
                datetime.second,
                WszUtilProcessName() );
        }
        else
        {
            Call( ErrERRCheck( JET_errFileAccessDenied ) );
        }
    }

HandleError:
    if ( JET_errSuccess > err )
    {
        Assert( NULL == pFile );
        iMode = 0;
    }
    else if ( 0 != iMode )
    {
        Assert( NULL != pFile );
    }
    else
    {
        Assert( NULL == pFile );
    }
    return err;
}

VOID CProfileStorage::Term()
{
    if ( NULL == pFile )
    {
        return;
    }
    if ( PROFILEMODE_SUMMARY & Mode() )
    {
        fprintf( pFile, "\r\n Active Profile Counters    Calls   Total time  Min time   Max time   Avg. time" );
        fprintf( pFile, "\r\n===============================================================================\r\n" );
        CProfileStorage *pPS;
        for ( pPS = pPSFirst; NULL != pPS; pPS = pPS->m_pPSNext )
        {
            pPS->Summary();
        }
    }
    DATETIME datetime;
    UtilGetCurrentDateTime( &datetime );
    fprintf( pFile, "%02d:%02d:%02d [END %ws]\r\n", datetime.hour, datetime.minute, datetime.second, WszUtilProcessName() );
    iMode = 0;
    fclose( pFile );
    pFile = NULL;
}

CProfileCounter::CProfileCounter( CProfileStorage *pPS, BOOL fAutoStart ) :
    m_dhrtExecute( 0 ),
    m_hrtStart( 0 ),
    m_pPS( pPS )
{
    Assert( NULL != m_pPS );
    if ( fAutoStart )
    {
        Start();
    }
}

CProfileCounter::CProfileCounter( CProfileStorage *pPS, const char * const szName ) :
    m_dhrtExecute( 0 ),
    m_hrtStart( 0 ),
    m_pPS( pPS )
{
    Assert( NULL != m_pPS );
    m_pPS->SetName( szName );
    Start();
}

CProfileCounter::~CProfileCounter()
{
    //  we are still running
    if ( 0 != m_hrtStart )
    {
        Stop();
    }
}

VOID CProfileCounter::Start()
{
    if ( 0 == CProfileStorage::Mode() )
    {
        return;
    }
    Assert( 0 == m_hrtStart );
    if ( ( PROFILEMODE_TIME | PROFILEMODE_TIMING | PROFILEMODE_SUMMARY ) & CProfileStorage::Mode() )
    {
        m_hrtStart = m_dhrtExecute = HrtHRTCount();
    }
    else
    {
        m_hrtStart = 1;
    }
}
VOID CProfileCounter::Pause()
{
    if ( ( PROFILEMODE_TIME | PROFILEMODE_TIMING | PROFILEMODE_SUMMARY ) & CProfileStorage::Mode() )
    {
        Assert( 0 != m_hrtStart );
        //  we are in running mode
        if ( 0 < (LONG64)m_dhrtExecute )
        {
            //  negative value means that we paused
            m_dhrtExecute -= HrtHRTCount();
        }
    }
}
VOID CProfileCounter::Continue()
{
    if ( ( PROFILEMODE_TIME | PROFILEMODE_TIMING | PROFILEMODE_SUMMARY ) & CProfileStorage::Mode() )
    {
        Assert( 0 != m_hrtStart );
        //  We are in pause mode
        if ( 0 > (LONG64)m_dhrtExecute )
        {
            m_dhrtExecute += HrtHRTCount();
        }
    }
}

VOID CProfileCounter::Stop()
{
    if ( 0 == CProfileStorage::Mode() )
    {
        return;
    }
    Assert( 0 != m_hrtStart );
    if ( ( PROFILEMODE_TIME | PROFILEMODE_TIMING | PROFILEMODE_SUMMARY ) & CProfileStorage::Mode() )
    {
        //  We are in running mode
        if ( 0 < (LONG64)m_dhrtExecute )
        {
            m_dhrtExecute = HrtHRTCount() - m_dhrtExecute;
        }
        else
        {
            m_dhrtExecute = HRT( 0 - m_dhrtExecute );
        }
    }
    m_pPS->Update( m_hrtStart, m_dhrtExecute );
    m_dhrtExecute = 0;
    m_hrtStart = 0;
}
#endif // DEBUG

