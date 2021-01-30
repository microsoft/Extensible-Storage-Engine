// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_MEMORY_HXX_INCLUDED
#define _OS_MEMORY_HXX_INCLUDED

#pragma warning(push)
#pragma warning(disable:4595)

const BYTE bGlobalAllocFill     = (BYTE)0xba;
const BYTE bGlobalFreeFill      = (BYTE)0xbf;



#define IbElementInStruct( struct, element )    ( (BYTE*)&struct.element - (BYTE*)&struct )


#define CbElementInStruct( struct, element )    ( IbElementInStruct( struct, element ) + sizeof( struct.element ) )



DWORD OSMemoryPageReserveGranularity();


DWORD OSMemoryPageCommitGranularity();


QWORD OSMemoryAvailable();


QWORD OSMemoryTotal();


DWORD_PTR OSMemoryPageReserveAvailable();


DWORD_PTR OSMemoryPageReserveTotal();


DWORD_PTR OSMemoryPageWorkingSetPeak();


DWORD OSMemoryPageEvictionCount();


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


class IBitmapAPI
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


#define CbFromCbit( cbit )          ( cbit / 8 + 1 )
#define PbCbStackBitmap( cbit )     alloca( CbFromCbit( cbit ) ), CbFromCbit( cbit )

class CFixedBitmap
    :   public IBitmapAPI
{
    public:


        CFixedBitmap( _Out_writes_bytes_(cbBuffer) void * pbBuffer, _In_ ULONG cbBuffer );


    public:

        virtual ~CFixedBitmap();

        virtual ERR ErrInitBitmap( _In_ const size_t cbit );
        virtual ERR ErrSet( _In_ const size_t iBit, _In_ const BOOL fValue );
        virtual ERR ErrGet( _In_ const size_t iBit, _Out_ BOOL* const pfValue );

    private:

        size_t  m_cbit;
        void*   m_rgbit;

};


class CSparseBitmap
    :   public IBitmapAPI
{
    public:


        CSparseBitmap();


        ERR ErrDisableUpdates();

        
        ERR ErrReset( const size_t cbit );

    public:

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



ERR ErrOSMemoryPageResidenceMapScanStart( const size_t cbMax, __out DWORD * const pdwUpdateId );
ERR ErrOSMemoryPageResidenceMapRetrieve( void* const pv, const size_t cb, IBitmapAPI** const ppbmapi );
VOID OSMemoryPageResidenceMapScanStop();


BOOL FOSMemoryPageResident( void* const pv, const size_t cb );



BOOL FOSMemoryPageAllocated( const void * const pv, const size_t cb );


BOOL FOSMemoryFileMapped( const void * const pv, const size_t cb );


BOOL FOSMemoryFileMappedCowed( const void * const pv, const size_t cb );



DWORD_PTR OSMemoryQuotaTotal();




void* PvOSMemoryHeapAlloc__( const size_t cbSize );
void* PvOSMemoryHeapAllocAlign__( const size_t cbSize, const size_t cbAlign );

#ifdef MEM_CHECK

void* PvOSMemoryHeapAlloc_( const size_t cbSize, __in_z const CHAR* szFile, LONG lLine );
void* PvOSMemoryHeapAllocAlign_( const size_t cbSize, const size_t cbAlign, __in_z const CHAR* szFile, LONG lLine );

#define PvOSMemoryHeapAlloc( cbSize ) PvOSMemoryHeapAlloc_( cbSize, __FILE__, __LINE__ )
#define PvOSMemoryHeapAllocAlign( cbSize, cbAlign ) PvOSMemoryHeapAllocAlign_( cbSize, cbAlign, __FILE__, __LINE__ )

#else

#define PvOSMemoryHeapAlloc( cbSize ) ( PvOSMemoryHeapAlloc__( cbSize ) )
#define PvOSMemoryHeapAllocAlign( cbSize, cbAlign ) ( PvOSMemoryHeapAllocAlign__( cbSize, cbAlign ) )

#endif


void OSMemoryHeapFree( void* const pv );
void OSMemoryHeapFreeAlign( void* const pv );



const void* PvOSMemoryHookNtQueryInformationProcess( const void* const pfnNew );
const void* PvOSMemoryHookNtQuerySystemInformation( const void* const pfnNew );
const void* PvOSMemoryHookGlobalMemoryStatus( const void* const pfnNew );



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
#else
    return PvOSMemoryHeapAlloc( cbSize );
#endif
}

#ifdef MEM_CHECK
BOOL FOSMemoryNewMemCheck_( __in_z const CHAR* const szFileName, const ULONG ulLine );

#define new     ( g_fMemCheck && !FOSMemoryNewMemCheck_( __FILE__, __LINE__ ) ) ? NULL : new
#endif

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

#else

#define PvOSMemoryPageAlloc( cbSize, pv )                   PvOSMemoryPageAlloc__( cbSize, pv, fFalse )
#define PvOSMemoryPageAllocEx( cbSize, pv, fAllocTopDown )  PvOSMemoryPageAlloc__( cbSize, pv, fAllocTopDown )

#endif


void OSMemoryPageFree( void* const pv );


void* PvOSMemoryPageReserve__( const size_t cbSize, void* const pv );
#ifdef MEM_CHECK
void* PvOSMemoryPageReserve_( const size_t cbSize, void* const pv, __in_z const CHAR* szFile, LONG lLine );
#define PvOSMemoryPageReserve( cbSize, pv ) PvOSMemoryPageReserve_( cbSize, pv, __FILE__, __LINE__ )
#else
#define PvOSMemoryPageReserve( cbSize, pv ) PvOSMemoryPageReserve__( cbSize, pv )
#endif


void OSMemoryPageReset( void* const pv, const size_t cbSize, const BOOL fToss = fFalse );


void OSMemoryPageProtect( void* const pv, const size_t cbSize );


void OSMemoryPageUnprotect( void* const pv, const size_t cbSize );


BOOL FOSMemoryPageCommit( void* const pv, const size_t cb );


void OSMemoryPageDecommit( void* const pv, const size_t cb );


BOOL FOSMemoryPageLock( void* const pv, const size_t cb );


void OSMemoryPageUnlock( void* const pv, const size_t cb );



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


        COSMemoryMap();
        ~COSMemoryMap();



        ERR ErrOSMMInit();
        VOID OSMMTerm();



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
#endif
        BOOL FOSMMCommit( const size_t cbCommit );
        VOID OSMMFree( void *const pv );



        ERR ErrOSMMPatternAlloc__(  const size_t    cbPattern,
                                    const size_t    cbSize,
                                    void** const    ppvPattern );
#ifdef MEM_CHECK
        ERR ErrOSMMPatternAlloc_(   const size_t    cbPattern,
                                    const size_t    cbSize,
                                    void** const    ppvPattern,
                                    __in_z const CHAR*  szFile,
                                    LONG            lLine );
#endif
        VOID OSMMPatternFree();


#ifdef MEM_CHECK
        static VOID OSMMDumpAlloc( __in_z const WCHAR* szFile );
#endif

    private:

        void            *m_pvMap;
        size_t          m_cbMap;
        size_t          m_cMap;

        size_t          m_cbReserve;
        size_t          m_cbCommit;

#ifdef MEM_CHECK

        COSMemoryMap    *m_posmmNext;
        BOOL            m_fInList;
        CHAR            *m_szFile;
        LONG            m_lLine;

#endif

};


#ifdef MEM_CHECK
#define ErrOSMMReserve( cbMap, cMap, rgpvMap, rgfProtect ) ErrOSMMReserve_( cbMap, cMap, rgpvMap, rgfProtect, __FILE__, __LINE__ )
#define ErrOSMMPatternAlloc( cbPattern, cbSize, ppvPattern ) ErrOSMMPatternAlloc_( cbPattern, cbSize, ppvPattern, __FILE__, __LINE__ )
#else
#define ErrOSMMReserve( cbMap, cMap, rgpvMap, rgfProtect ) ErrOSMMReserve__( cbMap, cMap, rgpvMap, rgfProtect )
#define ErrOSMMPatternAlloc( cbPattern, cbSize, ppvPattern ) ErrOSMMPatternAlloc__( cbPattern, cbSize, ppvPattern )
#endif



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
#endif

#endif


