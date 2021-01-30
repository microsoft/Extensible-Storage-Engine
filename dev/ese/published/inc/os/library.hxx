// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_LIBRARY_HXX_INCLUDED
#define _OS_LIBRARY_HXX_INCLUDED



#ifdef ENABLE_LOAD_LIB_TRACE
#define PreinitTrace( ... )     wprintf( L"[%hs:%d] ", SzSourceFileName( __FILE__ ), __LINE__ ); wprintf( __VA_ARGS__ )
#else
#define PreinitTrace( ... )
#endif




BOOL FUtilLoadLibrary( const WCHAR* wszLibrary, LIBRARY* plibrary, const BOOL fPermitDialog );


PFN PfnUtilGetProcAddress( LIBRARY library, const char* szFunction );


void UtilFreeLibrary( LIBRARY library );




enum OSLoadFlags
{
    oslfNone            = 0x0,

    oslfRequired        = 0x01,

    oslfExpectedOnWin5x = 0x02,
    oslfExpectedOnWin6  = 0x08,
    oslfExpectedOnWin7  = 0x10,
    oslfExpectedOnWin8  = 0x20,
    oslfNotExpectedOnCoreSystem = 0x40,
    oslfExpectedOnWin10 = 0x80,

    oslfNonSystem       = 0x0800,

    oslfStrictFree      = 0x1000,

    oslfHookable        = 0x2000,
    oslfReplacement     = 0x4000,

    oslfCtord           = 0x8000,
};

DEFINE_ENUM_FLAG_OPERATORS_BASIC( OSLoadFlags );


extern DWORD ErrorThunkNotSupported();
extern INT NtstatusThunkNotSupported();


template< typename... Args >
INLINE VOID VoidWithGLE( Args... )
{
    SetLastError( ErrorThunkNotSupported() );
}

template< typename Ret, typename... Args >
INLINE Ret NtstatusFailed( Args... )
{
    return Ret( NtstatusThunkNotSupported() );
}

template< typename Ret, typename... Args >
INLINE Ret FFailedWithGLE( Args... )
{
    SetLastError( ErrorThunkNotSupported() );
    return Ret( fFalse );
}

template< typename Ret, typename... Args >
INLINE Ret ErrorFailed( Args... )
{
    return Ret( ErrorThunkNotSupported() );
}

template< typename Ret, typename... Args >
INLINE Ret PnullFailedWithGLE( Args... )
{
    SetLastError( ErrorThunkNotSupported() );
    return Ret( NULL );
}

template< typename Ret, typename... Args >
INLINE Ret CFailedWithGLE( Args... )
{
    SetLastError( ErrorThunkNotSupported() );
    return Ret( 0 );
}


#ifdef DEBUG


INLINE const WCHAR * WszDllSubStr( const WCHAR * wszSource, const WCHAR * wszSubStr )
{
    WCHAR wszSourceI[300];
    WCHAR wszSubStrI[100];

    OSStrCbCopyW( wszSourceI, sizeof(wszSourceI), wszSource );
    OSStrCbCopyW( wszSubStrI, sizeof(wszSubStrI), wszSubStr );

    _wcsupr_s( wszSourceI, _countof( wszSourceI ) );
    _wcsupr_s( wszSubStrI, _countof( wszSubStrI ) );

    const WCHAR * const pwsz = wcsstr( wszSourceI, wszSubStrI );

    return pwsz ? ( wszSource + ( pwsz - wszSourceI ) ) : NULL;
}

#endif


ERR ErrMultiLoadPfn(
    const WCHAR * const mszzDlls,
    const BOOL          fNonSystemDll,
    const CHAR * const  szFunction,
    SHORT * const       pichDll,
    void ** const       ppfn );

VOID FreeLoadedModule(
    const WCHAR * const szDll );

#ifdef DEBUG
VOID OSLibraryValidateLoaderPolicy( const WCHAR * const mwszzDlls, OSLoadFlags oslf );
VOID OSLibraryTrackingFree( const WCHAR * const mwszzDlls );
#endif

template<class pfnT>
class FunctionLoader
{
private:

    const wchar_t * const   m_mwszzDlls;
    const char * const      m_szFunction;
    pfnT                    m_pfnFailThunk;
    SHORT    m_fOsLoadFlags;

    SHORT                   m_ichDllLoaded;
    ERR                     m_errLoad;
    pfnT                    m_pfn;

    ERR ErrLoadPfn()
    {
        Expected( m_pfn == NULL );


        void * pvFunc = NULL;
        const ERR err = ErrMultiLoadPfn( m_mwszzDlls, !!( m_fOsLoadFlags & oslfNonSystem ), m_szFunction, &m_ichDllLoaded, &pvFunc );
        m_pfn = (pfnT)pvFunc;
        m_errLoad = err;

        if ( NULL == m_pfn )
        {
            Assert( err );
            m_pfn = m_pfnFailThunk;
        }

        return err;
    }

public:

    FunctionLoader( pfnT pfnError, const wchar_t * const mszzDlls, const char * const szFunction, OSLoadFlags oslf ) :
        m_mwszzDlls( mszzDlls ),
        m_szFunction( szFunction ),
        m_fOsLoadFlags( (SHORT)( oslf | oslfCtord ) ),
        m_ichDllLoaded( -1 ),
        m_errLoad( -1 ),
        m_pfnFailThunk( pfnError ),
        m_pfn( NULL )
    {
#ifdef DEBUG

        Assert( !( oslf & oslfReplacement ) );

        INT iDll = 0;
        SIZE_T ichDll = 0;
        while( m_mwszzDlls[ichDll] != L'\0' )
        {
            Assert( iDll < 5 );
            Expected( WszDllSubStr( &(m_mwszzDlls[ichDll]), L".dll" ) );
            iDll++;
            ichDll += wcslen( &(m_mwszzDlls[ichDll]) ) + 1;
        }
        Assert( iDll > 0 );
        Assert( ichDll > 0 );
        Assert( ichDll < 0x7fff );

        Assert( szFunction );


        OSLibraryValidateLoaderPolicy( m_mwszzDlls, oslf );



        if (
             !( m_fOsLoadFlags & oslfNotExpectedOnCoreSystem ) &&
             ( m_fOsLoadFlags & oslfRequired ||
               m_fOsLoadFlags & oslfExpectedOnWin5x ||
               m_fOsLoadFlags & oslfExpectedOnWin6 ||
               m_fOsLoadFlags & oslfExpectedOnWin7 ) )
        {
            const ERR errT = ErrIsPresent();
            Assert( errT == 0  );
        }
#endif
    }

    ~FunctionLoader()
    {
        OnDebug( m_fOsLoadFlags = SHORT( ~oslfCtord & m_fOsLoadFlags ) );

        if ( m_fOsLoadFlags & oslfStrictFree )
        {
            if ( m_ichDllLoaded != -1 )
            {
                Assert( m_errLoad == 0  );
                FreeLoadedModule( &(m_mwszzDlls[m_ichDllLoaded]) );
                OnDebug( OSLibraryTrackingFree( m_mwszzDlls ) );
                m_ichDllLoaded = -1;
            }
        }
    }



    INLINE ERR ErrIsPresent()
    {
        if ( NULL == m_pfn )
        {
            return ErrLoadPfn();
        }
        return m_errLoad;
    }

    INLINE BOOL FCtord() const
    {
        return m_mwszzDlls != NULL;
    }


    const WCHAR * SzDllLoaded() const
    {
        if ( m_ichDllLoaded == -1 )
        {
            return NULL;
        }
        return &(m_mwszzDlls[m_ichDllLoaded]);
    }


    INLINE operator pfnT()
    {
        AssertSz( oslfCtord & m_fOsLoadFlags, "Using auto-loader before _CRT_INIT, use NTOSFuncPD and NTOSFunc*Preinit" );

        if ( NULL == m_pfn )
        {
            (void)ErrLoadPfn();
            Assert( m_pfn );
        }

        return m_pfn;
    }
    
    const void* PfnLoadHook( const void* const pfnNew, OSLoadFlags oslf = oslfNone )
    {
        Assert( oslfHookable & m_fOsLoadFlags );

        const void* const pfnOld = *this;
        Assert( pfnOld != NULL );
        Assert( pfnNew != NULL );

        PreinitTrace( L"\tLOADSWITCH: Swapping function %hs (%p) with %p %hs\n", m_szFunction, m_pfn, pfnNew, ( oslf & oslfReplacement ? "(replacement)" : "(testhook)" ) );
    
        m_pfn = (pfnT)pfnNew;

        if ( oslf & oslfReplacement )
        {
            Expected( !( oslfStrictFree & m_fOsLoadFlags ) );

            m_errLoad = 0 ;
        }

        return pfnOld;
    }
};


#ifdef ENABLE_STATIC_COMPILED_LOAD_DEPENDENCIES


#define NTOSFuncStd( g_pfn, mszzDlls, func, oslf )  \
                        decltype(&func) g_pfn = func;

#define NTOSFuncPtr( g_pfn, mszzDlls, func, oslf )  \
                        decltype(&func) g_pfn = func;

#define NTOSFuncCount( g_pfn, mszzDlls, func, oslf )    \
                        decltype(&func) g_pfn = func;

#else




#define NTOSFuncVoid( g_pfn, mszzDlls, func, oslf )             \
            FunctionLoader<decltype(&func)> g_pfn( VoidWithGLE, mszzDlls, #func, oslf );


#define NTOSFuncNtStd( g_pfn, mszzDlls, func, oslf )                \
            FunctionLoader<decltype(&func)> g_pfn( NtstatusFailed, mszzDlls, #func, oslf );


#define NTOSFuncStd( g_pfn, mszzDlls, func, oslf )              \
            FunctionLoader<decltype(&func)> g_pfn( FFailedWithGLE, mszzDlls, #func, oslf );


#define NTOSFuncError( g_pfn, mszzDlls, func, oslf )                \
            FunctionLoader<decltype(&func)> g_pfn( ErrorFailed, mszzDlls, #func, oslf );


#define NTOSFuncPtr( g_pfn, mszzDlls, func, oslf )              \
            FunctionLoader<decltype(&func)> g_pfn( PnullFailedWithGLE, mszzDlls, #func, oslf );


#define NTOSFuncCount( g_pfn, mszzDlls, func, oslf )                \
            FunctionLoader<decltype(&func)> g_pfn( CFailedWithGLE, mszzDlls, #func, oslf );


#define NTOSFuncCustom( g_pfn, mszzDlls, func, errorthunk, oslf )               \
            FunctionLoader<decltype(&func)> g_pfn( errorthunk, mszzDlls, #func, oslf );


#define NTOSFuncPD( g_pfn, func  )                          \
            BYTE rgb_##g_pfn[sizeof(FunctionLoader<decltype(&func)>)] = { 0 };      \
            FunctionLoader<decltype(&func)>& g_pfn = *(FunctionLoader<decltype(&func)>*)rgb_##g_pfn;


#define NTOSFuncStdPreinit( g_pfn, mszzDlls, func, oslf )       \
            if ( !g_pfn.FCtord() )                              \
            {                                                   \
                new( rgb_##g_pfn ) FunctionLoader<decltype(&func)>( FFailedWithGLE, mszzDlls, #func, oslf );    \
                Assert( g_pfn.FCtord() );                       \
            }

#define NTOSFuncPtrPreinit( g_pfn, mszzDlls, func, oslf )       \
            if ( !g_pfn.FCtord() )                              \
            {                                                   \
                new( rgb_##g_pfn ) FunctionLoader<decltype(&func)>( PnullFailedWithGLE, mszzDlls, #func, oslf );    \
                Assert( g_pfn.FCtord() );                       \
            }

#define NTOSFuncErrorPreinit( g_pfn, mszzDlls, func, oslf )     \
            if ( !g_pfn.FCtord() )                              \
            {                                                   \
                new( rgb_##g_pfn ) FunctionLoader<decltype(&func)>( ErrorFailed, mszzDlls, #func, oslf );   \
                Assert( g_pfn.FCtord() );                       \
            }


#endif


#endif


