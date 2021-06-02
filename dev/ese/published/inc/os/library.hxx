// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_LIBRARY_HXX_INCLUDED
#define _OS_LIBRARY_HXX_INCLUDED

//  -------------------------------------------------------------------------
//  Compile Options
//

//  Does simply printfs during DLL load, and load library operations because
//  these operations happen before OSTrace() initialization, and so this is 
//  the only real rational way to debug this layer.
//#define ENABLE_LOAD_LIB_TRACE 1

#ifdef ENABLE_LOAD_LIB_TRACE
#define PreinitTrace( ... )     wprintf( L"[%hs:%d] ", SzSourceFileName( __FILE__ ), __LINE__ ); wprintf( __VA_ARGS__ )
#else
#define PreinitTrace( ... )
#endif


//  -------------------------------------------------------------------------
//  Dynamically Loaded (User) Libraries
//

//  loads the library from the specified file, returning fTrue and the library
//  handle on success

BOOL FUtilLoadLibrary( const WCHAR* wszLibrary, LIBRARY* plibrary, const BOOL fPermitDialog );

//  retrieves the function pointer from the specified library or NULL if that
//  function is not found in the library

PFN PfnUtilGetProcAddress( LIBRARY library, const char* szFunction );

//  unloads the specified library

void UtilFreeLibrary( LIBRARY library );

//  -------------------------------------------------------------------------
//  Auto-Loaded OS Functions
//

// Technically only used (and intended) internally in the OS layer, but published
// here so that the unit tests can get at it.  Also potentially useful for other
// tools to use.


enum OSLoadFlags    // oslf
{
    oslfNone            = 0x0,

    oslfRequired        = 0x01,     // required means the code will assert and not work if the functionality is not found

    oslfExpectedOnWin5x = 0x02,     // it is expected we would find this functionality on WinXP/Win2k3+
    oslfExpectedOnWin6  = 0x08,     // it is expected we would find this functionality on WinVi+
    oslfExpectedOnWin7  = 0x10,     // it is expected we would find this functionality on Win7+
    oslfExpectedOnWin8  = 0x20,     // it is expected we would find this functionality on Phone8/Win8+
    oslfNotExpectedOnCoreSystem = 0x40, // we do not expect to find this functionality on Win8 Core System/Phone8
    oslfExpectedOnWin10 = 0x80,     // it is expected we would find this functionality on Win10+

    oslfNonSystem       = 0x0800,   // for testing and use with esent.dll / ese.dll

    oslfStrictFree      = 0x1000,   // calls free library when destructed

    oslfHookable        = 0x2000,   // specifies the functionality can be hooked by test
    oslfReplacement     = 0x4000,   // indicates the hook is a replacement API, resetting the load error

    oslfCtord           = 0x8000,   // private, for FunctionLoader<> internal use
};

DEFINE_ENUM_FLAG_OPERATORS_BASIC( OSLoadFlags );


extern DWORD ErrorThunkNotSupported();
extern INT NtstatusThunkNotSupported();

//
//  FunctionLoader() error thunk implementations, see NTOSFunc* macros for usage and meaning.
//

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

//  This is just the simpliest and quickest way I could implement a case insensitive strstr.
//  Note: That this is not a smart / locale aware method, so reserved only for use by the
//  DLL load library testing.

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

#endif // DEBUG

//  worker function that loads the DLL.

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

    //  configuration
    //
    const wchar_t * const   m_mwszzDlls;
    const char * const      m_szFunction;
    pfnT                    m_pfnFailThunk;
    SHORT/*OSLoadFlags*/    m_fOsLoadFlags;

    //  load
    //
    SHORT                   m_ichDllLoaded;
    ERR                     m_errLoad;
    pfnT                    m_pfn;

    ERR ErrLoadPfn()
    {
        Expected( m_pfn == NULL );  // if we start using this function concurrently this may one day fail


        void * pvFunc = NULL;   // compiler: won't allow "&((void*)m_pfn)" to be passed for ppfn
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
        //  validate args in are sensible

        Assert( !( oslf & oslfReplacement ) );

        INT iDll = 0;
        SIZE_T ichDll = 0;
        while( m_mwszzDlls[ichDll] != L'\0' )
        {
            // if more than 5, suspect they didn't properly double NUL terminate string.
            Assert( iDll < 5 );
            // of if no .dll extention, probably didn't double NUL terminate string
            Expected( WszDllSubStr( &(m_mwszzDlls[ichDll]), L".dll" ) );
            iDll++;
            ichDll += wcslen( &(m_mwszzDlls[ichDll]) ) + 1;
        }
        Assert( iDll > 0 ); // should be at least one DLL listed
        Assert( ichDll > 0 );
        Assert( ichDll < 0x7fff );

        Assert( szFunction );

        //  validate that functionality matches all ESE policies

        OSLibraryValidateLoaderPolicy( m_mwszzDlls, oslf );

        //  if the functionality is required or expected, check it is present

        //  cheating: we are assuming we are running on win7 plus for test passes and dev
        //  boxes.  if not, we can either improve the code to check the appropriate OS
        //  revision and make the assert correct, or just remove the oslfExpectedOnWin7
        //  clause to indicate we're still testing on Vista / Win2k8.

        if (
             !( m_fOsLoadFlags & oslfNotExpectedOnCoreSystem ) &&
             ( m_fOsLoadFlags & oslfRequired ||
               m_fOsLoadFlags & oslfExpectedOnWin5x ||
               m_fOsLoadFlags & oslfExpectedOnWin6 ||
               m_fOsLoadFlags & oslfExpectedOnWin7 ) )
        {
            const ERR errT = ErrIsPresent();
            Assert( errT == 0 /* JET_errSuccess */ );
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
                Assert( m_errLoad == 0 /* JET_errSuccess */ );
                FreeLoadedModule( &(m_mwszzDlls[m_ichDllLoaded]) );
                OnDebug( OSLibraryTrackingFree( m_mwszzDlls ) );    // a little asymetric
                m_ichDllLoaded = -1;
            }
        }
    }


    //  this function tests to see if the OS functionality can be loaded.

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

    //  for test access

    const WCHAR * SzDllLoaded() const
    {
        if ( m_ichDllLoaded == -1 )
        {
            return NULL;
        }
        return &(m_mwszzDlls[m_ichDllLoaded]);
    }

    //  this is the magic, when you call g_pfnFoo( xxx, yyy, zzz ) it magically returns
    //  the pointer to the delay loaded function and executes it.  if there was a failure
    //  in loading the function, we will instead execute the m_pfnFailThunk.

    INLINE operator pfnT()
    {
        //  This is an Assert to check if called too early, pre _CRT_INIT(), probably in FOSPreinit().  We
        //  can't do this before _CRT_INIT b/c the object m_* variables won't be initialized until the .ctors
        //  are called via _CRT_INIT.  See usage of NTOSFuncPD and NTOSFunc*Preinit to solve.
        AssertSz( oslfCtord & m_fOsLoadFlags, "Using auto-loader before _CRT_INIT, use NTOSFuncPD and NTOSFunc*Preinit" );

        if ( NULL == m_pfn )
        {
            (void)ErrLoadPfn();
            Assert( m_pfn );    // should be valid or m_pfnFailThunk
        }

        return m_pfn;
    }
    
    const void* PfnLoadHook( const void* const pfnNew, OSLoadFlags oslf = oslfNone ) // updating the m_pfn by pfnNew
    {
        Assert( oslfHookable & m_fOsLoadFlags ); // hookable is not set for this function loader

        //  we use *this here (instead of m_pfn) to force it to load the function if it hasn't been yet.
        const void* const pfnOld = *this;
        Assert( pfnOld != NULL );
        Assert( pfnNew != NULL );

        PreinitTrace( L"\tLOADSWITCH: Swapping function %hs (%p) with %p %hs\n", m_szFunction, m_pfn, pfnNew, ( oslf & oslfReplacement ? "(replacement)" : "(testhook)" ) );
    
        m_pfn = (pfnT)pfnNew;

        if ( oslf & oslfReplacement )
        {
            // would have to ensure not FreeLibrary() on something that is not loaded
            Expected( !( oslfStrictFree & m_fOsLoadFlags ) );

            m_errLoad = 0 /* JET_errSuccess */;
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

#else // !ENABLE_STATIC_COMPILED_LOAD_DEPENDENCIES

//  The way these work, you identify the return type and pick the right decl macro:
//
//      NTOSFuncError( g_pfnRegOpenKeyExW, g_mwszzRegistryLibs, RegOpenKeyExW, oslfRequired );
//
//  This defines a FunctionLoader<> / functor enabled class by the name of g_pfnRegOpenKeyExW, and
//  can be called simply like:
//  
//      HANDLE hValid = g_pfnRegOpenKeyExW( HKEY_LOCAL_MACHINE, wszPath, 0, KEY_READ, &hkeyPath );
//
//  Also the FunctionLoader<> has a few other useful members:
//
//      g_pfnRegOpenKeyExW.ErrIsPresent();  // which gives back the error we hit trying to retrieve the functionality
//      g_pfnRegOpenKeyExW.SzDllLoaded();   // intended for test, gives back the specific DLL that was choosen / loaded
//

//  This is the typical error you will see if you do not have the FFailedWithGLE, or CFailedWithGLE, etc type function
//  defined with the right number of arguments.
//      2>c:\src\e15\eseosrefactor\sources\test\ese\src\devlibtest\oslayer\oslayerunit\librarytest.cxx(45) : error C2664: 'FunctionLoader<pfnT>::Funct
//      ionLoader(pfnT,const char *const ,const char *const ,OSLoadFlags)' : cannot convert parameter 1 from 'Ret (__cdecl *)(ArgA,ArgB,ArgC,ArgD)' to
//       'int (__cdecl *)(DATA_BLOB *,LPCWSTR,DATA_BLOB *,PVOID,CRYPTPROTECT_PROMPTSTRUCT *,DWORD,DATA_BLOB *)'

//  Use NTOSFuncVoid() with care ... while the error thunk does SetLastError(), most NT functions that have a void
//  return type may not actually communicate errors in any way.  So it is highly recommended that a consuming code
//  check pfn.ErrIsPresent() before, to distinguish the not loaded error from the actual APIs semantics.

#define NTOSFuncVoid( g_pfn, mszzDlls, func, oslf )             \
            FunctionLoader<decltype(&func)> g_pfn( VoidWithGLE, mszzDlls, #func, oslf );

//  returns a NTSTATUS error, load error returns the standard NTSTATUS thunk value

#define NTOSFuncNtStd( g_pfn, mszzDlls, func, oslf )                \
            FunctionLoader<decltype(&func)> g_pfn( NtstatusFailed, mszzDlls, #func, oslf );

//  returns a BOOL fSuccess value, load error yields FALSE return w/ GLE set to standard error thunk value

#define NTOSFuncStd( g_pfn, mszzDlls, func, oslf )              \
            FunctionLoader<decltype(&func)> g_pfn( FFailedWithGLE, mszzDlls, #func, oslf );

//  returns Win32 error, load error returns the standard error thunk value

#define NTOSFuncError( g_pfn, mszzDlls, func, oslf )                \
            FunctionLoader<decltype(&func)> g_pfn( ErrorFailed, mszzDlls, #func, oslf );

//  returns a pointer type value, load error yields NULL return w/ GLE set to standard error thunk value

#define NTOSFuncPtr( g_pfn, mszzDlls, func, oslf )              \
            FunctionLoader<decltype(&func)> g_pfn( PnullFailedWithGLE, mszzDlls, #func, oslf );

//  returns a integer count, load error yields zero for count w/ GLE set to standard error thunk value

#define NTOSFuncCount( g_pfn, mszzDlls, func, oslf )                \
            FunctionLoader<decltype(&func)> g_pfn( CFailedWithGLE, mszzDlls, #func, oslf );

//  allows the consumer to specify the error thunk function / behavior

#define NTOSFuncCustom( g_pfn, mszzDlls, func, errorthunk, oslf )               \
            FunctionLoader<decltype(&func)> g_pfn( errorthunk, mszzDlls, #func, oslf );

//  If you need a function before _CRT_INIT (such as in FOSPreinit()), then you will need these
//

//  This uses a tricky piece of code by declaring the object as byte array, and then using placement new to 
//  construct the object before _CRT_INIT() starts, so people can use the "Auto-Loader" (a bit light on
//  auto-loading though) as early as they want.
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


#endif  //  ENABLE_STATIC_COMPILED_LOAD_DEPENDENCIES


#endif  //  _OS_LIBRARY_HXX_INCLUDED


