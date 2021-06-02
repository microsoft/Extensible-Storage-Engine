// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_MEMORY_HXX_INCLUDED
#define _OS_MEMORY_HXX_INCLUDED

#pragma warning(push)
#pragma warning(disable:4595) // non-member operator new or delete functions may not be declared inline

const BYTE bGlobalAllocFill     = (BYTE)0xba;
const BYTE bGlobalFreeFill      = (BYTE)0xbf;

//  Offset computation

//  computes the offset of a given element in a struct

#define IbElementInStruct( struct, element )    ( (BYTE*)&struct.element - (BYTE*)&struct )

//  computes the minimum amount of bytes required to store a given struct up to a certain element

#define CbElementInStruct( struct, element )    ( IbElementInStruct( struct, element ) + sizeof( struct.element ) )

//  System Memory Attributes

//  returns the system page reserve granularity

DWORD OSMemoryPageReserveGranularity();

//  returns the system page commit granularity

DWORD OSMemoryPageCommitGranularity();

//  returns the current available physical memory in the system

QWORD OSMemoryAvailable();

//  returns the total physical memory in the system

QWORD OSMemoryTotal();

//  returns the current available virtual address space in the process

DWORD_PTR OSMemoryPageReserveAvailable();

//  returns the total virtual address space in the process

DWORD_PTR OSMemoryPageReserveTotal();

//  returns the peak working set size of the process

DWORD_PTR OSMemoryPageWorkingSetPeak();

//  returns the total number of physical memory pages evicted from the system

DWORD OSMemoryPageEvictionCount();

//  retrieves the current processes memory usage stats

typedef struct _MEMSTAT
{
    DWORD  cPageFaultCount;
    SIZE_T cbWorkingSetSize;
    SIZE_T cbPeakWorkingSetSize;
    SIZE_T cbPagefileUsage;
    SIZE_T cbPeakPagefileUsage;
    SIZE_T cbPrivateUsage;
} MEMSTAT;

void OSMemoryGetProcessMemStats( MEMSTAT * const pmemstat );

//  Bitmap API

class IBitmapAPI  //  bmapi
{
    public:

        enum class ERR
        {
            errSuccess,
            errInvalidParameter,
            errOutOfMemory,
        };

        virtual ~IBitmapAPI() {}

        virtual ERR ErrInitBitmap( _In_ const size_t cbit ) = 0;
        virtual ERR ErrSet( _In_ const size_t iBit, _In_ const BOOL fValue ) = 0;
        virtual ERR ErrGet( _In_ const size_t iBit, _Out_ BOOL* const pfValue ) = 0;
};

//  Simple fixed bitmap (useful for stack allocation) implementation of IBitmapAPI

#define CbFromCbit( cbit )          ( cbit / 8 + 1 )
#define PbCbStackBitmap( cbit )     alloca( CbFromCbit( cbit ) ), CbFromCbit( cbit )

class CFixedBitmap  //  fbm
    :   public IBitmapAPI
{
    public:  //  specialized API

        //  ctor

        CFixedBitmap( _Out_writes_bytes_(cbBuffer) void * pbBuffer, _In_ ULONG cbBuffer );


    public:  //  IBitmapAPI

        virtual ~CFixedBitmap();

        virtual ERR ErrInitBitmap( _In_ const size_t cbit );
        virtual ERR ErrSet( _In_ const size_t iBit, _In_ const BOOL fValue );
        virtual ERR ErrGet( _In_ const size_t iBit, _Out_ BOOL* const pfValue );

    private:

        size_t  m_cbit;
        void*   m_rgbit;

};

//  Sparse Bitmap implementation of IBitmapAPI

class CSparseBitmap  //  sbm
    :   public IBitmapAPI
{
    public:  //  specialized API

        //  ctor

        CSparseBitmap();

        //  sets the bitmap to read only mode

        ERR ErrDisableUpdates();

        //  sets the bitmap to write again and blank
        
        ERR ErrReset( const size_t cbit );

    public:  //  IBitmapAPI

        virtual ~CSparseBitmap();

        virtual ERR ErrInitBitmap( const size_t cbit );
        virtual ERR ErrSet( const size_t iBit, const BOOL fValue );
        virtual ERR ErrGet( _In_ const size_t iBit, _Out_ BOOL* const pfValue );

    private:

        size_t  m_cbit;
        void*   m_rgbit;
        size_t  m_cbitUpdate;
        size_t  m_cbitCommit;
        void*   m_rgbitCommit;
        size_t  m_shfCommit;
};


//  these 3 functions work together to provide a bitmap representing the residence
//  of individual virtual memory pages in physical memory.
//
//    ScanStart() - begins a scan, and takes the maximum size of memory that will
//                  later be retrieved.
//    Retrieve()  - retrieves a bitmap for the segment of virtual memory provide,
//                  that must not be larger than cbMax provided to XxxScanStart().
//    ScanStop()  - cleans up any allocated memory, including the bitmap returned
//                  by XxxRetrieve().  Do not use bitmap after this func.

ERR ErrOSMemoryPageResidenceMapScanStart( const size_t cbMax, __out DWORD * const pdwUpdateId );
ERR ErrOSMemoryPageResidenceMapRetrieve( void* const pv, const size_t cb, IBitmapAPI** const ppbmapi );
VOID OSMemoryPageResidenceMapScanStop();

//  returns fTrue if the virtual memory pages backing the specified buffer are
//  either in the working set of the process or are held elsewhere in system
//  memory

BOOL FOSMemoryPageResident( void* const pv, const size_t cb );

//  Memory Alloc Type Test Routines

//  Returns true if it is PvOSMemoryPageAlloc() or PvOSMemoryPageReserve() + FOSMemoryCommit() type
//  memory.  All pages must be of the allocated type memory or this function returns false.
//  Note: This is to be used in comparison with FOSMemoryFileMapped*, it can not be used to reliably 
//  determine the nature of potentially heap allocated memory
//     NOTE: This routine is only available in debug or OS unit tests.

BOOL FOSMemoryPageAllocated( const void * const pv, const size_t cb );

//  Determines if memory is mapped to a file with MapViewOfFileEx()
//  Returns true if memory is mapped to a file with MapViewOfFileEx().  All pages must be mapped
//  to a file or this function returns false.
//     NOTE: This routine is only available in debug or OS unit tests.

BOOL FOSMemoryFileMapped( const void * const pv, const size_t cb );

//  Determines if memory is mapped to a file with MapViewOfFileEx() AND also has been modified and
//  thus copied (on the write).  Unlike previous functions, this returns true if _ANY_ page is copied.
//     NOTE: This routine is only available in debug or OS unit tests.

BOOL FOSMemoryFileMappedCowed( const void * const pv, const size_t cb );


//  returns the total physical memory in the system taking any existing process
//  quotas into account

DWORD_PTR OSMemoryQuotaTotal();


//  Heap Memory Allocation

//  allocate a chunk of memory from the process heap of the specifed size,
//  returning NULL if there is insufficient heap memory available to satisfy
//  the request.  you may (optionally) specify an alignment for the block.  The
//  memory is not zero filled.

void* PvOSMemoryHeapAlloc__( const size_t cbSize );
void* PvOSMemoryHeapAllocAlign__( const size_t cbSize, const size_t cbAlign );

#ifdef MEM_CHECK

void* PvOSMemoryHeapAlloc_( const size_t cbSize, __in_z const CHAR* szFile, LONG lLine );
void* PvOSMemoryHeapAllocAlign_( const size_t cbSize, const size_t cbAlign, __in_z const CHAR* szFile, LONG lLine );

#define PvOSMemoryHeapAlloc( cbSize ) PvOSMemoryHeapAlloc_( cbSize, __FILE__, __LINE__ )
#define PvOSMemoryHeapAllocAlign( cbSize, cbAlign ) PvOSMemoryHeapAllocAlign_( cbSize, cbAlign, __FILE__, __LINE__ )

#else  //  !MEM_CHECK

#define PvOSMemoryHeapAlloc( cbSize ) ( PvOSMemoryHeapAlloc__( cbSize ) )
#define PvOSMemoryHeapAllocAlign( cbSize, cbAlign ) ( PvOSMemoryHeapAllocAlign__( cbSize, cbAlign ) )

#endif  //  MEM_CHECK

//  free the specified chunk of memory back to the process heap

void OSMemoryHeapFree( void* const pv );
void OSMemoryHeapFreeAlign( void* const pv );


//  Test hooks

const void* PvOSMemoryHookNtQueryInformationProcess( const void* const pfnNew );
const void* PvOSMemoryHookNtQuerySystemInformation( const void* const pfnNew );
const void* PvOSMemoryHookGlobalMemoryStatus( const void* const pfnNew );


//  Global C++ Heap Allocation Operators

extern INT g_fMemCheck;


#ifdef MEM_CHECK
INLINE const CHAR * const SzNewFile();
INLINE ULONG UlNewLine();
#endif

_Ret_maybenull_ _Post_writable_byte_size_(cbSize)
INLINE void* __cdecl operator new( const size_t cbSize )
{
#ifdef MEM_CHECK
    return g_fMemCheck?
            PvOSMemoryHeapAlloc_( cbSize, SzNewFile(), UlNewLine() ):
            PvOSMemoryHeapAlloc__( cbSize );
#else  //  !MEM_CHECK
    return PvOSMemoryHeapAlloc( cbSize );
#endif  //  MEM_CHECK
}

#ifdef MEM_CHECK
BOOL FOSMemoryNewMemCheck_( __in_z const CHAR* const szFileName, const ULONG ulLine );

//  this #define is a huge hack used to facilitate tracking file/line info for allocations
//  (note that "new" MUST be in the 'else' clause in order to properly associate the rest
//  of the line with the operator)
//
#define new     ( g_fMemCheck && !FOSMemoryNewMemCheck_( __FILE__, __LINE__ ) ) ? NULL : new
#endif  //  MEM_CHECK

INLINE void __cdecl operator delete( void* const pv )
{
    OSMemoryHeapFree( pv );
}

#pragma push_macro("new")
#undef new
_Ret_maybenull_ _Post_writable_byte_size_(size)
inline void* __cdecl operator new[](size_t size) { return operator new(size); }
inline void __cdecl operator delete[](void* p) { operator delete(p); }
#pragma pop_macro("new")


//  Page Memory Control

//  reserves and commits a range of virtual addresses of the specifed size,
//  returning NULL if there is insufficient address space or backing store to
//  satisfy the request.  Note that the page reserve granularity applies to
//  this range

void * PvOSMemoryPageAlloc__(
    const size_t    cbSize,
    void * const    pv,
    const BOOL      fAllocTopDown );

#ifdef MEM_CHECK

void * PvOSMemoryPageAlloc_(
    const size_t    cbSize,
    void * const    pv,
    const BOOL      fAllocTopDown,
    __in_z const CHAR * szFile,
    const LONG      lLine );

#define PvOSMemoryPageAlloc( cbSize, pv )                   PvOSMemoryPageAlloc_( cbSize, pv, fFalse, __FILE__, __LINE__ )
#define PvOSMemoryPageAllocEx( cbSize, pv, fAllocTopDown )  PvOSMemoryPageAlloc_( cbSize, pv, fAllocTopDown, __FILE__, __LINE__ )

#else  //  !MEM_CHECK

#define PvOSMemoryPageAlloc( cbSize, pv )                   PvOSMemoryPageAlloc__( cbSize, pv, fFalse )
#define PvOSMemoryPageAllocEx( cbSize, pv, fAllocTopDown )  PvOSMemoryPageAlloc__( cbSize, pv, fAllocTopDown )

#endif  //  MEM_CHECK

//  free the reserved range of virtual addresses starting at the specified
//  address, freeing any backing store committed to this range

void OSMemoryPageFree( void* const pv );

//  reserve a range of virtual addresses of the specified size, returning NULL
//  if there is insufficient address space to satisfy the request.  Note that
//  the page reserve granularity applies to this range

void* PvOSMemoryPageReserve__( const size_t cbSize, void* const pv );
#ifdef MEM_CHECK
void* PvOSMemoryPageReserve_( const size_t cbSize, void* const pv, __in_z const CHAR* szFile, LONG lLine );
#define PvOSMemoryPageReserve( cbSize, pv ) PvOSMemoryPageReserve_( cbSize, pv, __FILE__, __LINE__ )
#else   //  !MEM_CHECK
#define PvOSMemoryPageReserve( cbSize, pv ) PvOSMemoryPageReserve__( cbSize, pv )
#endif  //  MEM_CHECK

//  reset the dirty bit for the specified range of virtual addresses.  this
//  results in the contents of the memory being thrown away instead of paged
//  to disk if the OS needs its physical memory for another process.  a value
//  of fTrue for fToss results in a hint to the OS to throw the specified
//  memory out of our working set.  Note that the page commit granularity
//  applies to this range

void OSMemoryPageReset( void* const pv, const size_t cbSize, const BOOL fToss = fFalse );

//  set the specified range of virtual addresses as read only.  Note that the
//  page commit granularity applies to this range

void OSMemoryPageProtect( void* const pv, const size_t cbSize );

//  set the specified range of virtual addresses as read / write.  Note that
//  the page commit granularity applies to this range

void OSMemoryPageUnprotect( void* const pv, const size_t cbSize );

//  commit the specified range of virtual addresses, returning fFalse if there
//  is insufficient backing store to satisfy the request.  Note that the page
//  commit granularity applies to this range

BOOL FOSMemoryPageCommit( void* const pv, const size_t cb );

//  decommit the specified range of virtual addresses, freeing any backing
//  store committed to this range.  Note that the page commit granularity
//  applies to this range

void OSMemoryPageDecommit( void* const pv, const size_t cb );

//  locks the specified range of virtual addresses, returning fFalse if there
//  is insufficient quota to satisfy the request.  Note that the page commit
//  granularity applies to this range

BOOL FOSMemoryPageLock( void* const pv, const size_t cb );

//  unlocks the specified range of virtual addresses.  Note that the page
//  commit granularity applies to this range

void OSMemoryPageUnlock( void* const pv, const size_t cb );


//  Memory Mapping

class COSMemoryMap
{
    public:

        enum class ERR
        {
            errSuccess,
            errOutOfBackingStore,
            errMappingFailed,
            errOutOfAddressSpace,
            errOutOfMemory,
        };

    public:

        //  ctor/dtor

        COSMemoryMap();
        ~COSMemoryMap();


        //  init/term

        ERR ErrOSMMInit();
        VOID OSMMTerm();


        //  basic API

        static BOOL FCanMultiMap();

        ERR ErrOSMMReserve__(   const size_t        cbMap,
                                const size_t        cMap,
                                __inout_ecount( cMap ) void** const     rgpvMap,
                                const BOOL* const   rgfProtect );
#ifdef MEM_CHECK
        ERR ErrOSMMReserve_(    const size_t        cbMap,
                                const size_t        cMap,
                                void** const        rgpvMap,
                                const BOOL* const   rgfProtect,
                                __in_z const CHAR*      szFile,
                                LONG                lLine );
#endif  //  MEM_CHECK
        BOOL FOSMMCommit( const size_t cbCommit );
        VOID OSMMFree( void *const pv );


        //  pattern API

        ERR ErrOSMMPatternAlloc__(  const size_t    cbPattern,
                                    const size_t    cbSize,
                                    void** const    ppvPattern );
#ifdef MEM_CHECK
        ERR ErrOSMMPatternAlloc_(   const size_t    cbPattern,
                                    const size_t    cbSize,
                                    void** const    ppvPattern,
                                    __in_z const CHAR*  szFile,
                                    LONG            lLine );
#endif  //  MEM_CHECK
        VOID OSMMPatternFree();


#ifdef MEM_CHECK
        static VOID OSMMDumpAlloc( __in_z const WCHAR* szFile );
#endif  //  MEM_CHECK

    private:

        void            *m_pvMap;       //  location of the first mapping
        size_t          m_cbMap;        //  size of the mapping
        size_t          m_cMap;         //  total number of mappings

        size_t          m_cbReserve;    //  amount of address space consumed by this page store (bytes)
        size_t          m_cbCommit;     //  amount of committed backing store (bytes)

#ifdef MEM_CHECK

        COSMemoryMap    *m_posmmNext;
        BOOL            m_fInList;
        CHAR            *m_szFile;
        LONG            m_lLine;

#endif  //  MEM_CHECK

};


#ifdef MEM_CHECK
#define ErrOSMMReserve( cbMap, cMap, rgpvMap, rgfProtect ) ErrOSMMReserve_( cbMap, cMap, rgpvMap, rgfProtect, __FILE__, __LINE__ )
#define ErrOSMMPatternAlloc( cbPattern, cbSize, ppvPattern ) ErrOSMMPatternAlloc_( cbPattern, cbSize, ppvPattern, __FILE__, __LINE__ )
#else   //  !MEM_CHECK
#define ErrOSMMReserve( cbMap, cMap, rgpvMap, rgfProtect ) ErrOSMMReserve__( cbMap, cMap, rgpvMap, rgfProtect )
#define ErrOSMMPatternAlloc( cbPattern, cbSize, ppvPattern ) ErrOSMMPatternAlloc__( cbPattern, cbSize, ppvPattern )
#endif  //  MEM_CHECK


//  Memory Manipulation

__forceinline void UtilMemCpy( __out_bcount(cb) void * const pvDest, __in_bcount(cb) const void *pvSrc, const size_t cb )
{
    memcpy( (BYTE*)pvDest, (BYTE*)pvSrc, cb );
}

__forceinline void UtilMemMove( __out_bcount(cb) void * const pvDest, __in_bcount(cb) const void *pvSrc, const size_t cb )
{
    memmove( (BYTE*)pvDest, (BYTE*)pvSrc, cb );
}

BOOL FUtilZeroed( __in_bcount(cbData) const BYTE * pbData, __in const size_t cbData );
size_t IbUtilLastNonZeroed( __in_bcount(cbData) const BYTE * pbData, __in const size_t cbData );

#if defined( MEM_CHECK ) && !defined( DEBUG )
#error "MEM_CHECK can be enabled only in DEBUG mode"
#endif // MEM_CHECK && !DEBUG

#endif  //  _OS_MEMORY_HXX_INCLUDED


