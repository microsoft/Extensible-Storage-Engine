// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _SYNC_HXX_INCLUDED
#define _SYNC_HXX_INCLUDED

//  Build Options

#define SYNC_USE_X86_ASM            //  use x86 assembly for atomic memory manipulation
//#define SYNC_ANALYZE_PERFORMANCE  //  analyze performance of synchronization objects
#ifdef SYNC_ANALYZE_PERFORMANCE
#define SYNC_DUMP_PERF_DATA         //  dump performance analysis of synchronization objects
#endif  //  SYNC_ANALYZE_PERFORMANCE


//  don't enable deadlock detection for offical NT builds
//  (due to potential memory allocation problems during NT stress)
//
#if defined( DEBUG ) && ( !defined( DBG ) || !defined( OFFICIAL_BUILD ) ) && !defined( MINIMAL_FUNCTIONALITY )
#define SYNC_DEADLOCK_DETECTION     //  always perform deadlock detection in DEBUG
#define SYNC_VALIDATE_IRKSEM_USAGE  //  always validate IRKSEM (CReferencedKernelSemaphore) usage in DEBUG
#endif

#ifdef _MSC_VER
#include <intrin.h>
#endif

#include <limits.h>
#include <new>          //  note: On some platforms, new.h and new are not equivalent and interchangeable.

#include <stdarg.h>
#include <stdlib.h>

//  calling convention

#ifdef _MSC_VER
#define OSSYNCAPI __stdcall
#else
#define OSSYNCAPI
#endif

//  basic types

#include "cc.hxx"

//  Assertions

//  Assertion Failure action
//
//  called to indicate to the developer that an assumption is not true

void __stdcall AssertFail( const char * szMessage, const char * szFilename, LONG lLine, ... );

//  Assert Macros

//  asserts that the given expression is true or else fails with the specified message

#ifdef OSSYNCAssertSzRTL
#else  //  !OSSYNCAssertSzRTL
#define OSSYNCAssertSzRTL( exp, sz )    ( ( exp ) ? (void) 0 : AssertFail( sz, __FILE__, __LINE__ ) )
#endif

#ifdef OSSYNCAssertSz
#else  //  !OSSYNCAssertSz
#ifdef DEBUG
#define OSSYNCAssertSz( exp, sz )       OSSYNCAssertSzRTL( exp, sz )
#else  //  !DEBUG
#define OSSYNCAssertSz( exp, sz )
#endif  //  DEBUG
#endif

//  asserts that the given expression is true or else fails with that expression

#ifdef OSSYNCAssertRTL
#else  //  !OSSYNCAssertRTL
#define OSSYNCAssertRTL( exp )          OSSYNCAssertSzRTL( exp, #exp )
#endif
#ifdef OSSYNCAssert
#else  //  !OSSYNCAssert
#define OSSYNCAssert( exp )             OSSYNCAssertSz( exp, #exp )
#endif


//  Enforces

//  Enforce Failure action
//
//  called when a strictly enforced condition has been violated

void OSSYNCAPI EnforceFail( const char* szMessage, const char* szFilename, LONG lLine );

//  Enforce Macros

//  the given expression MUST be true or else fails with the specified message

#define OSSYNCEnforceSz( exp, sz )      ( ( exp ) ? (void) 0 : EnforceFail( sz, __FILE__, __LINE__ ) )

//  the given expression MUST be true or else fails with that expression

#define OSSYNCEnforce( exp )            OSSYNCEnforceSz( exp, #exp )

#ifdef SYNC_VALIDATE_IRKSEM_USAGE
#define OSSYNCEnforceIrksem( exp, sz )  OSSYNCEnforceSz( exp, sz )
#else  //  !SYNC_VALIDATE_IRKSEM_USAGE
#define OSSYNCEnforceIrksem( exp, sz )
#endif  //  SYNC_VALIDATE_IRKSEM_USAGE


//  OSSYNC_FOREVER marks all convergence loops

#if defined( _M_IX86 ) || defined( _M_AMD64 )
inline void OSSyncPause() { _mm_pause(); }
#elif defined( _M_ARM ) || defined( _M_ARM64 )
inline void OSSyncPause() { __yield(); }
#else
inline void OSSyncPause() {};
#endif

#ifdef DEBUG
#define OSSYNC_FOREVER for ( INT cLoop = 0; ; cLoop++, OSSyncPause() )
#else  //  !DEBUG
#define OSSYNC_FOREVER for ( ; ; OSSyncPause() )
#endif  //  DEBUG

#ifdef DEBUG
#define OSSYNC_INNER_FOREVER for ( INT cInnerLoop = 0; ; cInnerLoop++, OSSyncPause() )
#else  //  !DEBUG
#define OSSYNC_INNER_FOREVER for ( ; ; OSSyncPause() )
#endif  //  DEBUG


//  supported intrinsics
//
//  NOTE:  DO NOT INCLUDE WINDOWS HEADERS IN THIS FILE
//
//         if you would like to know why, please talk to SOMEONE

#ifdef _MSC_VER

extern "C" {
#ifdef _M_IX86
#else  //  !_M_IX86
    void* __cdecl _InterlockedExchangePointer( void* volatile *Target, void* Value );
    void* __cdecl _InterlockedCompareExchangePointer( void* volatile *_Destination, void* _Exchange, void* _Comparand );

    #pragma intrinsic( _InterlockedExchangePointer )
    #pragma intrinsic( _InterlockedCompareExchangePointer )
#endif  //  _M_IX86
} // extern "C"

#else // !_MSC_VER

//  Details:
//      http://gcc.gnu.org/onlinedocs/gcc-4.1.2/gcc/Atomic-Builtins.html
//      But there is another set here:
//          http://gcc.gnu.org/wiki/Atomic/GCCMM/LIbrary
//      that I didn't use, that maybe we should've.

LONG __cdecl _InterlockedExchange( LONG volatile * _Target, LONG _Value )
{
    return __sync_lock_test_and_set( _Target, _Value );
}

inline void* __cdecl _InterlockedExchangePointer( void* volatile *Target, void* Value )
{
    return __sync_lock_test_and_set( Target, Value );
}

inline LONG _InterlockedCompareExchange( LONG volatile * _Destination, LONG _Exchange, LONG _Comparand )
{
    return __sync_val_compare_and_swap( _Destination, _Comparand, _Exchange );
}

inline LONG64 _InterlockedCompareExchange64( LONG64 volatile * _Destination, LONG64 _Exchange, LONG64 _Comparand )
{
    return __sync_val_compare_and_swap( _Destination, _Comparand, _Exchange );
}

inline void* __cdecl _InterlockedCompareExchangePointer( void* volatile *_Destination, void* _Exchange, void* _Comparand )
{
    return __sync_val_compare_and_swap( _Destination, _Comparand, _Exchange );
}

inline LONG __cdecl _InterlockedExchangeAdd( LONG volatile * _Addend, LONG _Value )
{
    return __sync_fetch_and_add( _Addend, _Value );
}

#endif // _MSC_VER


//================================================================
// Macros to work around current version SourceInsight parser's no symbol lookup into namespace.
// Also, the "//" at the end of NAMESPACE_BEGIN() is intentional to nullify ";"
//================================================================
#define NAMESPACE_BEGIN(x) namespace x { //
#define NAMESPACE_END(x) }

NAMESPACE_BEGIN( OSSYNC );


class CDumpContext;


//  Context Local Storage

class COwner;
class CLockDeadlockDetectionInfo;

struct CLS
{
#ifdef SYNC_DEADLOCK_DETECTION

    COwner*                     pownerLockHead;             //  list of locks owned by this context
    DWORD                       cDisableOwnershipTracking;  //  lock ownerships are not tracked for this context
    DWORD                       cOverrideDeadlock;          //  next lock ownership will not be a deadlock
    DWORD                       cDisableDeadlockDetection;  //  deadlock detection is not enabled for this context
    DWORD                       cDisableLockCheckOnApiExit; //  do not assert on thread holding lock on API exit

    CLockDeadlockDetectionInfo* plddiLockWait;              //  lock for which this context is waiting
    DWORD                       groupLockWait;              //  lock group for which this context is waiting

#endif  //  SYNC_DEADLOCK_DETECTION
};

//  returns the pointer to the current context's local storage

CLS* const OSSYNCAPI Pcls();


//  Processor Information

//  returns the maximum number of processors this process can utilize

INT OSSYNCAPI OSSyncGetProcessorCountMax();

//  returns the current number of processors this process can utilize

INT OSSYNCAPI OSSyncGetProcessorCount();

//  returns the processor number that the current context _MAY_ be executing on
//
//  NOTE:  the current context may change processors at any time

INT OSSYNCAPI OSSyncGetCurrentProcessor();

//  sets the processor number returned by OSSyncGetCurrentProcessor()

void OSSYNCAPI OSSyncSetCurrentProcessor( const INT iProc );


//  Processor Local Storage

//  configures the size of processor local storage

BOOL OSSYNCAPI FOSSyncConfigureProcessorLocalStorage( const size_t cbPLS );

//  retrieves a pointer to the current context's processor local storage

void* OSSYNCAPI OSSyncGetProcessorLocalStorage();

//  retrieves a pointer to a given processor's local storage

void* OSSYNCAPI OSSyncGetProcessorLocalStorage( const size_t iProc );


#ifdef SYNC_ANALYZE_PERFORMANCE // only OSSYNC should depend upon this function, isolate to catch

//  High Resolution Timer

//  returns the current HRT frequency

QWORD OSSYNCAPI QwOSSyncIHRTFreq();

//  returns the current HRT count

QWORD OSSYNCAPI QwOSSyncIHRTCount();

#endif  // SYNC_ANALYZE_PERFORMANCE


#ifndef RTM // only OSSYNC should depend upon this function, isolate to catch

//  Timer

//  returns the current tick count where one tick is one millisecond

DWORD OSSYNCAPI DwOSSyncITickTime();

#endif  // !RTM


//  Global Synchronization Constants

//    wait time used for testing the state of the kernel object

extern const INT cmsecTest;

//    wait time used for infinite wait on a kernel object

extern const INT cmsecInfinite;

//    maximum wait time on a kernel object before a deadlock is suspected

extern const INT cmsecDeadlock;

//    wait time used for infinite wait on a kernel object without deadlock

extern const INT cmsecInfiniteNoDeadlock;

//    cache line size

extern const INT cbCacheLine;

//    system is aborting unhappily

extern BOOL g_fSyncProcessAbort;


extern INT g_cSpinMax;


//  Atomic Memory Manipulations

//  returns fTrue if the given data is properly aligned for atomic modification

inline const BOOL IsAtomicallyModifiable( LONG* plTarget )
{
    return ULONG_PTR( plTarget ) % sizeof( LONG ) == 0;
}

inline const BOOL IsAtomicallyModifiable( SHORT* psTarget )
{
    return ULONG_PTR( psTarget ) % sizeof( SHORT ) == 0;
}

inline const BOOL IsAtomicallyModifiablePointer( void*const* ppvTarget )
{
    return ULONG_PTR( ppvTarget ) % sizeof( void* ) == 0;
}

//  atomically sets the target to the specified value, returning the target's
//  initial value.  the target must be aligned to a four byte boundary
//
inline LONG AtomicExchange( LONG* const plTarget, const LONG lValue )
{
    OSSYNCAssert( IsAtomicallyModifiable( plTarget ) );

    return _InterlockedExchange( plTarget, lValue );
}

//  atomically sets the target to the specified value, returning the target's
//  initial value.  the target must be aligned to a pointer boundary
//
inline void* AtomicExchangePointer( void** const ppvTarget, const void* const pvValue )
{
    OSSYNCAssert( IsAtomicallyModifiablePointer( ppvTarget ) );

#ifndef _WIN64
    //  HACK: cast to LONG_PTR, then to long in order to permit compiling with /Wp64
    //
    return (void *)(DWORD_PTR)AtomicExchange( (LONG* const)ppvTarget, (const LONG)(LONG_PTR)pvValue );
#else  //  !_WIN64
    return _InterlockedExchangePointer( ppvTarget, (void*)pvValue );
#endif  //  _M_IX86
}

inline __int64 AtomicRead( __int64 * const pi64Target );
inline __int64 AtomicCompareExchange( _Inout_ volatile __int64 * const pi64Target, const __int64 i64Initial, const __int64 i64Final );
inline __int64 AtomicExchange( _Inout_ volatile __int64 * const pi64Target, const __int64 i64Value )
{
    OSSYNCAssert( IsAtomicallyModifiablePointer( (void *const *)pi64Target ) );
    __int64 i64Initial = AtomicRead( (__int64 * const)pi64Target );

    // We would use _InterlockedExchange64(), but it is not available on x86 ... so this
    //  accomplishes the same thing.
    OSSYNC_FOREVER
    {
        if ( i64Initial == AtomicCompareExchange( pi64Target, i64Initial, i64Value ) )
        {
            break;
        }

        //  Missed our chance, drat, update and try again ...
        i64Initial = AtomicRead( (__int64 * const)pi64Target );
    }

    return i64Initial;
}

//  atomically reads the specified value of the target.
//  the target must be aligned to a four byte boundary.
//
inline LONG AtomicRead( LONG * const plTarget )
{
    OSSYNCAssert( IsAtomicallyModifiable( plTarget ) );
    return *( (volatile LONG *)plTarget );
}

inline ULONG AtomicRead( ULONG * const pulTarget )
{
    return ( ULONG )AtomicRead( ( LONG *)pulTarget );
}

//  atomically reads the specified value of the target.
//  the target must be aligned to a pointer boundary.
//
inline __int64 AtomicRead( __int64 * const pi64Target )
{
    OSSYNCAssert( IsAtomicallyModifiablePointer( (void *const *)pi64Target ) );
#ifndef _WIN64
    return AtomicCompareExchange( (volatile __int64 * const)pi64Target, 0, 0 );
#else  //  !_WIN64
    return *( (volatile __int64 *)pi64Target );
#endif  //  _WIN64
}

inline unsigned __int64 AtomicRead( unsigned __int64 * const pui64Target )
{
    return ( unsigned __int64 ) AtomicRead( (__int64 *) pui64Target );
}

//  atomically reads the specified value of the target.
//  the target must be aligned to a pointer boundary.
//
inline void* AtomicReadPointer( void** const ppvTarget )
{
#ifndef _WIN64
    return (void*)AtomicRead( (LONG*)ppvTarget );
#else  //  !_WIN64
    return (void*)AtomicRead( (__int64*)ppvTarget );
#endif  //  _WIN64
}

//  atomically adds the specified value to the target, returning the target's
//  initial value.  the target must be aligned to a four byte boundary
//
inline LONG AtomicExchangeAdd( LONG * const plTarget, const LONG lValue )
{
    OSSYNCAssert( IsAtomicallyModifiable( plTarget ) );

    return _InterlockedExchangeAdd( plTarget, lValue );
}

//  atomically adds the specified value to the target, returning the target's
//  initial value.  the target must be aligned to a two byte boundary
//
inline SHORT AtomicExchangeAdd( SHORT * const psTarget, const SHORT sValue )
{
    OSSYNCAssert( IsAtomicallyModifiable( psTarget ) );

    return _InterlockedExchangeAdd16( psTarget, sValue );
}

//  atomically compares the current value of the target with the specified
//  initial value and if equal sets the target to the specified final value.
//  the initial value of the target is returned.  the exchange is successful
//  if the value returned equals the specified initial value.  the target
//  must be aligned to a four byte boundary
//
inline LONG AtomicCompareExchange( LONG * const plTarget, const LONG lInitial, const LONG lFinal )
{
    OSSYNCAssert( IsAtomicallyModifiable( plTarget ) );

    return _InterlockedCompareExchange( plTarget, lFinal, lInitial );
}

inline ULONG AtomicCompareExchange( ULONG * const pulTarget, const ULONG ulInitial, const ULONG ulFinal )
{
    return (ULONG)AtomicCompareExchange( (LONG *)pulTarget, (LONG)ulInitial, (LONG)ulFinal );
}

inline unsigned __int64 AtomicCompareExchange( unsigned __int64 * const pullTarget, const unsigned __int64 ullInitial, const unsigned __int64 ullFinal )
{
    return (unsigned __int64)AtomicCompareExchange( (__int64 *)pullTarget, ( __int64)ullInitial, ( __int64)ullFinal );
}

inline SHORT AtomicCompareExchange( SHORT * const psTarget, const SHORT sInitial, const SHORT sFinal )
{
    OSSYNCAssert( IsAtomicallyModifiable( psTarget ) );

    return _InterlockedCompareExchange16( psTarget, sFinal, sInitial );
}

inline void* AtomicCompareExchangePointer( void** const ppvTarget, const void* const pvInitial, const void* const pvFinal )
{
    OSSYNCAssert( IsAtomicallyModifiablePointer( ppvTarget ) );

#ifndef _WIN64
    //  HACK: cast to LONG_PTR, then to long in order to permit compiling with /Wp64
    //
    return (void *)(DWORD_PTR)AtomicCompareExchange( (LONG* const)ppvTarget, (const LONG)(LONG_PTR)pvInitial, (const LONG)(LONG_PTR)pvFinal );
#else  //  !_WIN64
    return _InterlockedCompareExchangePointer( ppvTarget, const_cast< void* >( pvFinal ), const_cast< void* >( pvInitial ) );
#endif  //  _WIN64
}

inline __int64 AtomicCompareExchange( _Inout_ volatile __int64 * const pi64Target, const __int64 i64Initial, const __int64 i64Final )
{
    OSSYNCAssert( IsAtomicallyModifiablePointer( (void *const *)pi64Target ) );

    return _InterlockedCompareExchange64( pi64Target, i64Final, i64Initial );
}

// atomically add a new node into a single link list
// pvNode:  address of the new node to add into the list
// ppvNext: address of the "next" field in new node struct
// ppvHead: address of the pointer pointing to the head of the list
//
inline void AtomicAddLinklistNode( void* const pvNode, void** const ppvNext, void* volatile* const ppvHead )
{
    // pvNode is a pointer, but technically doesn't have to be atomically modifiable.
    OSSYNCAssert( IsAtomicallyModifiablePointer( ppvNext ) );
    OSSYNCAssert( IsAtomicallyModifiablePointer( (void *const *)ppvHead ) );
    
    OSSYNC_FOREVER
    {
        // attach new node to link list head
        void* const pvHead = AtomicReadPointer( (void** const)ppvHead );
        *( void* volatile* )ppvNext = pvHead;

        // elect new node to be link list head
        const void* const pvResult = AtomicCompareExchangePointer( (void** const)ppvHead, pvHead, pvNode );
        if ( pvResult == pvHead )
        {
            break;
        }
    }
}

//  atomically retrieves a single link list, assuming AtomicAddLinklistNode() is being
//  used to construct the list.  Note this is a destructive action, so the list is
//  empty after the operation is performed.
//  ppvHead:    address of the pointer pointing to the head of the list
//
inline void* PvAtomicRetrieveLinklist( void* volatile* const ppvHead )
{
    // pvNode is a pointer, but technically doesn't have to be atomically modifiable.
    OSSYNCAssert( IsAtomicallyModifiablePointer( (void *const *)ppvHead ) );

    return AtomicExchangePointer( (void **const)ppvHead, NULL );
}


//  atomically adds the specified value to the target, returning the target's
//  initial value.  the target must be aligned to a pointer boundary.

inline void* AtomicExchangeAddPointer( void** const ppvTarget, void* const pvValue )
{
    void*   pvInitial;
    void*   pvFinal;
    void*   pvResult;

    OSSYNCAssert( IsAtomicallyModifiablePointer( ppvTarget ) );

    OSSYNC_FOREVER
    {
        pvInitial   = AtomicReadPointer( ppvTarget );
        pvFinal     = (void*)( ULONG_PTR( pvInitial ) + ULONG_PTR( pvValue ) );
        pvResult    = AtomicCompareExchangePointer( ppvTarget, pvInitial, pvFinal );

        if ( pvResult == pvInitial )
        {
            break;
        }
    }

    return pvResult;
}

//  atomically set the bits from the mask in the target, returning the original.  the
//  target must be aligned to a four byte boundary

inline ULONG AtomicExchangeSet( ULONG* const pulTarget, const ULONG ulMask )
{
    OSSYNCAssert( IsAtomicallyModifiable( (LONG *)pulTarget ) );

    LONG lOld;
    OSSYNC_FOREVER
    {
        lOld                = AtomicRead( (LONG *)pulTarget );
        const LONG lWanted  = ( lOld | ulMask );
        const LONG lFinal   = AtomicCompareExchange( (LONG *)pulTarget, lOld, lWanted );
        
        if( lFinal == lOld )
        {
            break;
        }
    }

    return (ULONG)lOld;
}

//  atomically resets the bits from the mask in the target, returning the original.  the
//  target must be aligned to a four byte boundary

inline ULONG AtomicExchangeReset( ULONG* const pulTarget, const ULONG ulMask )
{
    OSSYNCAssert( IsAtomicallyModifiable( (LONG *)pulTarget ) );

    LONG lOld;
    OSSYNC_FOREVER
    {
        lOld                = AtomicRead( (LONG *)pulTarget );
        const LONG lWanted  = ( lOld & ~ulMask );
        const LONG lFinal   = AtomicCompareExchange( (LONG *)pulTarget, lOld, lWanted );
        
        if( lFinal == lOld )
        {
            break;
        }
    }

    return (ULONG)lOld;
}

//  atomically compares the target against the specified value and replaces
//  the target with the value if the value is less than the target.
//  the original value stored in the target is returned.
//  note unlikely many of the other Atomic family of functions, it's
//  not always safe to cast signed/unsigned as inputs, therefore make
//  sure any unsigned values involved are not larger than lMax.
//
inline LONG AtomicExchangeMin( LONG * const plTarget, const LONG lValue )
{
    LONG lInitial;

    OSSYNC_FOREVER
    {
        lInitial = AtomicRead( plTarget );

        if ( lValue >= lInitial )
        {
            break;
        }

        if ( AtomicCompareExchange( plTarget, lInitial, lValue ) == lInitial )
        {
            break;
        }
    }

    return lInitial;
}

//  atomically compares the target against the specified value and replaces
//  the target with the value if the value is less than the target.
//  the original value stored in the target is returned.
//  note unlikely many of the other Atomic family of functions, it's
//  not always safe to cast signed/unsigned as inputs, therefore make
//  sure any unsigned values involved are not larger than llMax.
//
inline __int64 AtomicExchangeMin( __int64 * const pi64Target, const __int64 i64Value )
{
    __int64 i64Initial;

    OSSYNC_FOREVER
    {
        i64Initial = AtomicRead( pi64Target );

        if ( i64Value >= i64Initial )
        {
            break;
        }

        if ( AtomicCompareExchange( pi64Target, i64Initial, i64Value ) == i64Initial )
        {
            break;
        }
    }

    return i64Initial;
}

//  atomically compares the target against the specified value and replaces
//  the target with the value if the value is less than the target.
//  the original value stored in the target is returned.
//  note unlikely many of the other Atomic family of functions, it's
//  not always safe to cast signed/unsigned as inputs, therefore make
//  sure any unsigned values involved are not larger than llMax.
//
inline QWORD AtomicExchangeMin( QWORD * const pqwTarget, const QWORD qwValue )
{
    QWORD qwInitial;

    OSSYNC_FOREVER
    {
        qwInitial = AtomicRead( (__int64*)pqwTarget );

        if ( qwValue >= qwInitial )
        {
            break;
        }

        if ( (QWORD)AtomicCompareExchange( (__int64*)pqwTarget, (__int64)qwInitial, (__int64)qwValue ) == qwInitial )
        {
            break;
        }
    }

    return qwInitial;
}

//  atomically compares the target against the specified value and replaces
//  the target with the value if the value is greater than the target.
//  the original value stored in the target is returned.
//  note unlikely many of the other Atomic family of functions, it's
//  not always safe to cast signed/unsigned as inputs, therefore make
//  sure any unsigned values involved are not larger than lMax.
//
inline LONG AtomicExchangeMax( LONG * const plTarget, const LONG lValue )
{
    LONG lInitial;

    OSSYNC_FOREVER
    {
        lInitial = AtomicRead( plTarget );

        if ( lValue <= lInitial )
        {
            break;
        }

        if ( AtomicCompareExchange( plTarget, lInitial, lValue ) == lInitial )
        {
            break;
        }
    }

    return lInitial;
}

//  atomically compares the target against the specified value and replaces
//  the target with the value if the value is greater than the target.
//  the original value stored in the target is returned.
//  note unlikely many of the other Atomic family of functions, it's
//  not always safe to cast signed/unsigned as inputs, therefore make
//  sure any unsigned values involved are not larger than llMax.
//
inline __int64 AtomicExchangeMax( __int64 * const pi64Target, const __int64 i64Value )
{
    __int64 i64Initial;

    OSSYNC_FOREVER
    {
        i64Initial = AtomicRead( pi64Target );

        if ( i64Value <= i64Initial )
        {
            break;
        }

        if ( AtomicCompareExchange( pi64Target, i64Initial, i64Value ) == i64Initial )
        {
            break;
        }
    }

    return i64Initial;
}

//  atomically compares the target against the specified value and replaces
//  the target with the value if the value is greater than the target.
//  the original value stored in the target is returned.
//  note unlikely many of the other Atomic family of functions, it's
//  not always safe to cast signed/unsigned as inputs, therefore make
//  sure any unsigned values involved are not larger than llMax.
//
inline QWORD AtomicExchangeMax( QWORD * const pqwTarget, const QWORD qwValue )
{
    QWORD qwInitial;

    OSSYNC_FOREVER
    {
        qwInitial = AtomicRead( (__int64*)pqwTarget ); // this sign cast doesn't matter.

        if ( qwValue <= qwInitial )
        {
            break;
        }

        if ( (QWORD)AtomicCompareExchange( (__int64*)pqwTarget, (__int64)qwInitial, (__int64)qwValue ) == qwInitial )
        {
            break;
        }
    }

    return qwInitial;
}


//  atomically increments the target, returning the incremented value.  the
//  target must be aligned to a four byte boundary

inline LONG AtomicIncrement( LONG* const plTarget )
{
    return AtomicExchangeAdd( plTarget, 1 ) + 1;
}

inline ULONG AtomicIncrement( ULONG* const pulTarget )
{
    return (ULONG) AtomicIncrement( (LONG *)pulTarget );
}

inline SHORT AtomicIncrement( SHORT * const psTarget )
{
    return AtomicExchangeAdd( psTarget, 1 ) + 1;
}

//  atomically decrements the target, returning the decremented value.  the
//  target must be aligned to a four byte boundary

inline LONG AtomicDecrement( LONG* const plTarget )
{
    return AtomicExchangeAdd( plTarget, -1 ) - 1;
}

inline ULONG AtomicDecrement( ULONG* const pulTarget )
{
    return AtomicDecrement( (LONG *)pulTarget );
}

inline SHORT AtomicDecrement( SHORT * const psTarget )
{
    return AtomicExchangeAdd( psTarget, -1 ) - 1;
}

//  atomically adds the specified value to the target.  the target must be
//  aligned to a four byte boundary.  the final value is returned

inline QWORD AtomicAdd( QWORD* const pqwTarget, const QWORD qwValue )
{
#ifdef _WIN64
    return (QWORD)AtomicExchangeAddPointer( (VOID **)pqwTarget, (VOID *)qwValue ) + qwValue;
#else
    //  get the value atomically by doing a nop with AtomicCompareExchange.
    QWORD qwInitial = (QWORD)AtomicCompareExchange( (volatile __int64 * const)pqwTarget, 0, 0 );

    //  for x86 and ARM, regarding 64-bit operands, there are only compare-and-swap native instructions, not fetch-and-add. So
    //  we will spin and try to increment the value.
    OSSYNC_FOREVER
    {
        const QWORD qwFinal = qwInitial + qwValue;

        const QWORD qwActual = (QWORD)AtomicCompareExchange( (volatile __int64 * const)pqwTarget, (const __int64)qwInitial, (const __int64)qwFinal );
        
        if ( qwInitial == qwActual )
        {
            return qwFinal;
        }

        //  no luck, let's try again from the current value.
        qwInitial = qwActual;
    }
#endif
}


//  Atomically increments a DWORD counter, returning TRUE if the final
//  value is less than or equal to a specified maximum, or FALSE otherwise.
//  The pre-incremented value is returned in *pdwInitial
//  WARNING: to determine if the maximum value has been reached, an UNSIGNED
//  comparison is performed

inline BOOL FAtomicIncrementMax(
    _Inout_ volatile DWORD * const  pdw,
    _Out_   DWORD * const           pdwInitial,
    _In_    const DWORD             dwMaxValue )
{
    OSSYNC_FOREVER
    {
        const DWORD     dwInitial   = *pdw;
        if ( dwInitial < dwMaxValue )
        {
            const DWORD dwFinal     = dwInitial + 1;
            if ( dwInitial == (DWORD)AtomicCompareExchange( (LONG *)pdw, (LONG)dwInitial, (LONG)dwFinal ) )
            {
                *pdwInitial = dwInitial;
                return fTrue;
            }
        }
        else
            return fFalse;
    }
}


//  Atomically increments a pointer-sized counter, returning TRUE if the final
//  value is less than or equal to a specified maximum, or FALSE otherwise.
//  The pre-incremented value is returned in *ppvInitial
//  WARNING: to determine if the maximum value has been reached, an UNSIGNED
//  comparison is performed

inline BOOL FAtomicIncrementPointerMax(
    volatile VOID ** const  ppv,
    VOID ** const           ppvInitial,
    const VOID * const      pvMax )
{
    OSSYNC_FOREVER
    {
        VOID * const pvInitial = AtomicReadPointer( (void** const)ppv );
        if ( pvInitial < pvMax )
        {
            const VOID * const pvFinal = (void*)( (ULONG_PTR)pvInitial + 1 );
            if ( pvInitial == AtomicCompareExchangePointer( (void **const)ppv, pvInitial, pvFinal ) )
            {
                *ppvInitial = pvInitial;
                return fTrue;
            }
        }
        else
        {
            return fFalse;
        }
    }
}


#ifndef ESE_DISABLE_NOT_YET_PORTED


//  Enhanced Synchronization Object State Container
//
//  This class manages a "simple" or normal state for an arbitrary sync object
//  and its "enhanced" counterpart.  Which type is used depends on the build.
//  The goal is to maintain a footprint equal to the normal state so that other
//  classes that contain this object do not fluctuate in size depending on what
//  build options have been selected.  For example, a performance build might
//  need extra storage to collect performance stats on the object.  This data
//  will force the object to be allocated elsewhere in memory but will not change
//  the size of the object in its containing class.
//
//  Template Arguments:
//
//    CState            sync object state class
//    CStateInit        sync object state class ctor arg type
//    CInformation      sync object information class
//    CInformationInit  sync object information class ctor arg type

void* OSSYNCAPI ESMemoryNew( size_t cb );
void OSSYNCAPI ESMemoryDelete( void* pv );

//    determine when enhanced state is needed

#if defined( SYNC_ANALYZE_PERFORMANCE ) || defined( SYNC_DEADLOCK_DETECTION )
#define SYNC_ENHANCED_STATE
#endif  //  SYNC_ANALYZE_PERFORMANCE || SYNC_DEADLOCK_DETECTION

template< class CState, class CStateInit, class CInformation, class CInformationInit >
class CEnhancedStateContainer
{
    public:

        //  types

        //    enhanced state

        class CEnhancedState
            :   public CState,
                public CInformation
        {
            public:

                CEnhancedState( const CStateInit& si, const CInformationInit& ii )
                    :   CState( si ),
                        CInformation( ii )

                {
                }

                void* operator new( size_t cb ) { return ESMemoryNew( cb ); }
                void operator delete( void* pv ) { ESMemoryDelete( pv ); }
        };

        //  member functions

        //    ctors / dtors

        CEnhancedStateContainer( const CStateInit& si, const CInformationInit& ii )
        {
#ifdef SYNC_ENHANCED_STATE

            m_pes = new CEnhancedState( si, ii );

#else  //  !SYNC_ENHANCED_STATE

            new( (CState*) m_rgbState ) CState( si );

#endif  //  SYNC_ENHANCED_STATE
        }

        ~CEnhancedStateContainer()
        {
#ifdef SYNC_ENHANCED_STATE

            delete m_pes;
#ifdef DEBUG
            m_pes = NULL;
#endif  //  DEBUG

#else  //  !SYNC_ENHANCED_STATE

            ( (CState*) m_rgbState )->~CState();

#endif  //  SYNC_ENHANCED_STATE
        }

        //    accessors

        CEnhancedState& State() const
        {
#ifdef SYNC_ENHANCED_STATE

            return *m_pes;

#else  //  !SYNC_ENHANCED_STATE

            //  NOTE:  this assumes that CInformation has no storage!

            return *( (CEnhancedState*) m_rgbState );

#endif  //  SYNC_ENHANCED_STATE
        }

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  data members

        //    either a pointer to the enhanced state elsewhere in memory or the
        //      actual state data, depending on the mode of the sync object

        union
        {
            CEnhancedState* m_pes;
            BYTE            m_rgbState[ sizeof( CState ) ];
        };


        //    disallowed ctors / dtors

    private:

        CEnhancedStateContainer( const CEnhancedStateContainer& state ) = delete;
};


//  Synchronization Object Base Class
//
//  All Synchronization Objects are derived from this class

class CSyncObject
{
    public:

        //  member functions

        //    ctors / dtors

        CSyncObject() {}
        ~CSyncObject() {}

    private:

        //  member functions

        //    operators

        CSyncObject& operator=( CSyncObject& ) = delete;  //  disallowed
};


//  Synchronization Object Basic Information

class CSyncBasicInfo
{
    public:

        //  member functions

        //    ctors / dtors

#ifdef SYNC_ENHANCED_STATE

        CSyncBasicInfo( const char* szInstanceName );
        ~CSyncBasicInfo();

#else  //  !SYNC_ENHANCED_STATE

        CSyncBasicInfo( const char* szInstanceName ) {}
        ~CSyncBasicInfo() {}

#endif  //  SYNC_ENHANCED_STATE

        //    manipulators

        void SetTypeName( const char* szTypeName );
        void SetInstance( const CSyncObject* const psyncobj );

        //    accessors

#ifdef SYNC_ENHANCED_STATE

        const char* SzInstanceName() const { return m_szInstanceName; }
        const char* SzTypeName() const { return m_szTypeName; }
        const CSyncObject* const Instance() const { return m_psyncobj; }

#endif  //  SYNC_ENHANCED_STATE

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  member functions

        //    operators

        CSyncBasicInfo& operator=( CSyncBasicInfo& ) = delete;  //  disallowed

        //  data members

#ifdef SYNC_ENHANCED_STATE

        //    Instance Name

        const char*     m_szInstanceName;

        //    Type Name

        const char*     m_szTypeName;

        //    Instance

        const CSyncObject*  m_psyncobj;

#endif  //  SYNC_ENHANCED_STATE
};

//  sets the type name for the synchronization object

inline void CSyncBasicInfo::SetTypeName( const char* szTypeName )
{
#ifdef SYNC_ENHANCED_STATE

    m_szTypeName = szTypeName;

#endif  //  SYNC_ENHANCED_STATE
}

//  sets the instance pointer for the synchronization object

inline void CSyncBasicInfo::SetInstance( const CSyncObject* const psyncobj )
{
#ifdef SYNC_ENHANCED_STATE

    m_psyncobj = psyncobj;

#endif  //  SYNC_ENHANCED_STATE
}


//  Synchronization Object Performance:  Wait Times

class CSyncPerfWait
{
    public:

        //  member functions

        //    ctors / dtors

        CSyncPerfWait();
        ~CSyncPerfWait();

        //  member functions

        //    manipulators

        void StartWait();
        void StopWait();

        //    accessors

#ifdef SYNC_ANALYZE_PERFORMANCE

        QWORD   CWaitTotal() const      { return m_cWait; }
        double  CsecWaitElapsed() const { return    (double)(signed __int64)m_qwHRTWaitElapsed /
                                                    (double)(signed __int64)QwOSSyncIHRTFreq(); }

#endif  //  SYNC_ANALYZE_PERFORMANCE

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  member functions

        //    operators

        CSyncPerfWait& operator=( CSyncPerfWait& ) = delete;  //  disallowed

        //  data members

#ifdef SYNC_ANALYZE_PERFORMANCE

        //    wait count

        volatile QWORD m_cWait;

        //    elapsed wait time

        volatile QWORD m_qwHRTWaitElapsed;

#endif  //  SYNC_ANALYZE_PERFORMANCE
};

//  starts the wait timer for the sync object

inline void CSyncPerfWait::StartWait()
{
#ifdef SYNC_ANALYZE_PERFORMANCE

    //  increment the wait count

    AtomicAdd( (QWORD*)&m_cWait, 1 );

    //  subtract the start wait time from the elapsed wait time.  this starts
    //  an elapsed time computation for this context.  StopWait() will later
    //  add the end wait time to the elapsed time, causing the following net
    //  effect:
    //
    //  m_qwHRTWaitElapsed += <end time> - <start time>
    //
    //  we simply choose to go ahead and do the subtraction now to save storage

    AtomicAdd( (QWORD*)&m_qwHRTWaitElapsed, QWORD( -__int64( QwOSSyncIHRTCount() ) ) );

#endif  //  SYNC_ANALYZE_PERFORMANCE
}

//  stops the wait timer for the sync object

inline void CSyncPerfWait::StopWait()
{
#ifdef SYNC_ANALYZE_PERFORMANCE

    //  add the end wait time to the elapsed wait time.  this completes the
    //  computation started in StartWait()

    AtomicAdd( (QWORD*)&m_qwHRTWaitElapsed, QwOSSyncIHRTCount() );

#endif  //  SYNC_ANALYZE_PERFORMANCE
}


//  Null Synchronization Object State Initializer

class CSyncStateInitNull
{
};

extern const CSyncStateInitNull syncstateNull;


//  Kernel Semaphore Information

class CKernelSemaphoreInfo
    :   public CSyncBasicInfo,
        public CSyncPerfWait
{
    public:

        //  member functions

        //    ctors / dtors

        CKernelSemaphoreInfo( const CSyncBasicInfo& sbi )
            :   CSyncBasicInfo( sbi )
        {
        }

        //    debugging support

        void Dump( const CDumpContext& dc ) const;
};


//  Kernel Semaphore State

class CKernelSemaphoreState
{
    public:

        //  member functions

        //    ctors / dtors

        CKernelSemaphoreState( const CSyncStateInitNull& null ) : m_handle( 0 ) {}

        //    manipulators

        void SetHandle( void * handle ) { m_handle = handle; }

        //    accessors

        void* Handle() { return m_handle; }

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  member functions

        //    operators

        CKernelSemaphoreState& operator=( CKernelSemaphoreState& ) = delete;  //  disallowed

        //  data members

        //    kernel semaphore handle

        void* m_handle;
};


//  Kernel Semaphore

class CKernelSemaphore
    :   private CSyncObject,
        private CEnhancedStateContainer< CKernelSemaphoreState, CSyncStateInitNull, CKernelSemaphoreInfo, CSyncBasicInfo >
{
    public:

        //  member functions

        //    ctors / dtors

        CKernelSemaphore( const CSyncBasicInfo& sbi );
        ~CKernelSemaphore();

        //    init / term

        const BOOL FInit();
        void Term();

        //    manipulators

        void Acquire();
        const BOOL FTryAcquire();
        const BOOL FAcquire( const INT cmsecTimeout );
        void Release( const INT cToRelease = 1 );

        //    accessors

        const BOOL FReset();

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  member functions

        //    operators

        CKernelSemaphore& operator=( CKernelSemaphore& ) = delete;  //  disallowed

        //    accessors

        const BOOL FInitialized();
};

//  acquire one count of the semaphore, waiting forever if necessary

inline void CKernelSemaphore::Acquire()
{
    //  semaphore should be initialized

    OSSYNCAssert( FInitialized() );

    //  wait for the semaphore

    const BOOL fAcquire = FAcquire( cmsecInfinite );
    OSSYNCAssert( fAcquire );
}

//  try to acquire one count of the semaphore without waiting.  returns 0 if a
//  count could not be acquired

inline const BOOL CKernelSemaphore::FTryAcquire()
{
    //  semaphore should be initialized

    OSSYNCAssert( FInitialized() );

    //  test the semaphore

    return FAcquire( cmsecTest );
}

//  returns fTrue if the semaphore has no available counts

inline const BOOL CKernelSemaphore::FReset()
{
    //  semaphore should be initialized

    OSSYNCAssert( FInitialized() );

    //  test the semaphore

    return !FTryAcquire();
}

//  returns fTrue if the semaphore has been initialized

inline const BOOL CKernelSemaphore::FInitialized()
{
    return State().Handle() != 0;
}


//  Kernel Semaphore Pool

class CKernelSemaphorePool
{
    public:

        //  types

        //    index to a ref counted kernel semaphore

        typedef USHORT IRKSEM;
        enum { irksemAllocated = 0xFFFE, irksemNil = 0xFFFF };

        //  member functions

        //    ctors / dtors

        CKernelSemaphorePool();
        ~CKernelSemaphorePool();

        //    init / term

        const BOOL FInit();
        void Term();

        //    manipulators

        const IRKSEM Allocate( const CSyncObject* const psyncobj );
        void Reference( const IRKSEM irksem );
        void Unreference( const IRKSEM irksem );

        //    accessors

        CKernelSemaphore& Ksem( const IRKSEM irksem, const CSyncObject* const psyncobj ) const;

        const BOOL FInitialized() const;

        LONG CksemAlloc() const { return m_cksem; }

    private:

        //  types

        //    reference counted kernel semaphore

        class CReferencedKernelSemaphore
            :   public CKernelSemaphore
        {
            public:

                //  member functions

                //    ctors / dtors

                CReferencedKernelSemaphore();
                ~CReferencedKernelSemaphore();

                //    init / term

                const BOOL FInit();
                void Term();

                //    manipulators

                BOOL FAllocate();
                void Release();

                // note: CSyncObject must be fully-qualified since it is an inaccessible base class of CReferencedKernelSemaphore
                void SetUser( const OSSYNC::CSyncObject* const psyncobj );

                void Reference();
                const BOOL FUnreference();

                //    accessors

                const BOOL FInUse() const { return m_fInUse; }
                const INT CReference() const { return m_cReference; }

#ifdef SYNC_VALIDATE_IRKSEM_USAGE
                const OSSYNC::CSyncObject* const PsyncobjUser() const { return m_psyncobjUser; }
#endif  //  SYNC_VALIDATE_IRKSEM_USAGE

            private:

                //  member functions

                //    operators

                CReferencedKernelSemaphore& operator=( CReferencedKernelSemaphore& ) = delete;  //  disallowed

                //  data members

                //    transacted state representation

                union
                {
                    volatile LONG       m_l;
                    struct
                    {
                        volatile USHORT m_cReference:15;    //  0 <= m_cReference <= ( 1 << 15 ) - 1
                        volatile USHORT m_fInUse:1;         //  m_fInUse = { 0, 1 }
                    };
                };

                volatile LONG       m_fAvailable;

#ifdef SYNC_VALIDATE_IRKSEM_USAGE

                //    sync object currently using this semaphore

                const OSSYNC::CSyncObject* volatile m_psyncobjUser;

#endif  //  SYNC_VALIDATE_IRKSEM_USAGE
        };

        //  member functions

        //    operators

        CKernelSemaphorePool& operator=( CKernelSemaphorePool& ) = delete;  //  disallowed

        //    manipulators

        const IRKSEM AllocateNew();
        void Free( const IRKSEM irksem );

        //  data members

        //    semaphore index to semaphore map

        CReferencedKernelSemaphore*     m_mpirksemrksem;

        //    semaphore count

        volatile LONG                   m_cksem;
};

//  allocates an IRKSEM from the pool on behalf of the specified sync object
//
//  NOTE:  the returned IRKSEM has one reference count

inline const CKernelSemaphorePool::IRKSEM CKernelSemaphorePool::Allocate( const CSyncObject* const psyncobj )
{
    //  semaphore pool should be initialized

    OSSYNCAssert( FInitialized() );

    //  there are semaphores in the semaphore pool

    IRKSEM irksem = irksemNil;
    if ( m_cksem )
    {
        //  hash into the semaphore pool based on this context's stack frame

        IRKSEM irksemHash = IRKSEM( ( DWORD_PTR( &irksem ) + DWORD_PTR( &irksem ) / 65536 ) % m_cksem );
        OSSYNCAssert( irksemHash >= 0 && irksemHash < m_cksem );

        //  try to allocate a semaphore, scanning forwards through the pool

        for (   LONG cLoop = 0;
                cLoop < m_cksem;
                cLoop++, irksemHash = IRKSEM( ++irksemHash % m_cksem ) )
        {
            if ( m_mpirksemrksem[ irksemHash ].FAllocate() )
            {
                irksem = irksemHash;
                break;
            }
        }
    }

    //  if we do not yet have a semaphore, allocate one

    if ( irksem == irksemNil )
    {
        irksem = AllocateNew();
    }

    //  validate irksem retrieved

    OSSYNCAssert( irksem != irksemNil );
    OSSYNCAssert( irksem >= 0 );
    OSSYNCAssert( irksem < m_cksem );

    //  set the user for this semaphore

    m_mpirksemrksem[irksem].SetUser( psyncobj );

    //  ensure that the semaphore we retrieved is reset

    OSSYNCEnforceIrksem(    m_mpirksemrksem[irksem].FReset(),
                            "Illegal allocation of a Kernel Semaphore with available counts!"  );

    //  return the allocated semaphore

    return irksem;
}

//  add a reference count to an IRKSEM

inline void CKernelSemaphorePool::Reference( const IRKSEM irksem )
{
    //  validate IN args

    OSSYNCAssert( irksem != irksemNil );
    OSSYNCAssert( irksem >= 0 );
    OSSYNCAssert( irksem < m_cksem );

    //  semaphore pool should be initialized

    OSSYNCAssert( FInitialized() );

    //  increment the reference count for this IRKSEM

    m_mpirksemrksem[irksem].Reference();
}

//  remove a reference count from an IRKSEM, freeing it if the reference count
//  drops to zero and it is not currently in use

inline void CKernelSemaphorePool::Unreference( const IRKSEM irksem )
{
    //  validate IN args

    OSSYNCAssert( irksem != irksemNil );
    OSSYNCAssert( irksem >= 0 );
    OSSYNCAssert( irksem < m_cksem );

    //  semaphore pool should be initialized

    OSSYNCAssert( FInitialized() );

    //  decrement the reference count for this IRKSEM

    const BOOL fFree = m_mpirksemrksem[irksem].FUnreference();

    //  we need to free the semaphore

    if ( fFree )
    {
        //  free the IRKSEM back to the allocation stack

        Free( irksem );
    }
}

//  returns the CKernelSemaphore object associated with the given IRKSEM

inline CKernelSemaphore& CKernelSemaphorePool::Ksem( const IRKSEM irksem, const CSyncObject* const psyncobj ) const
{
    //  validate IN args

    OSSYNCAssert( irksem != irksemNil );
    OSSYNCAssert( irksem >= 0 );
    OSSYNCAssert( irksem < m_cksem );

    //  semaphore pool should be initialized

    OSSYNCAssert( FInitialized() );

    //  we had better be retrieving this semaphore for the right sync object

    OSSYNCEnforceIrksem(    m_mpirksemrksem[irksem].PsyncobjUser() == psyncobj,
                            "Illegal use of a Kernel Semaphore by another Synchronization Object"  );

    //  return kernel semaphore

    return m_mpirksemrksem[irksem];
}

//  returns fTrue if the semaphore pool has been initialized

inline const BOOL CKernelSemaphorePool::FInitialized() const
{
    return m_mpirksemrksem != NULL;
}

//  frees the given IRKSEM back to the allocation stack

inline void CKernelSemaphorePool::Free( const IRKSEM irksem )
{
    //  validate IN args

    OSSYNCAssert( irksem != irksemNil );
    OSSYNCAssert( irksem >= 0 );
    OSSYNCAssert( irksem < m_cksem );

    //  semaphore pool should be initialized

    OSSYNCAssert( FInitialized() );

    //  the semaphore to free had better not be in use

    OSSYNCEnforceIrksem(    !m_mpirksemrksem[irksem].FInUse(),
                            "Illegal free of a Kernel Semaphore that is still in use"  );

    //  the semaphore had better not already be freed

    OSSYNCEnforceIrksem(    !m_mpirksemrksem[irksem].FAllocate(),
                            "Illegal free of a Kernel Semaphore that is already free"  );

    //  ensure that the semaphore to free is reset

    OSSYNCEnforceIrksem(    m_mpirksemrksem[irksem].FReset(),
                            "Illegal free of a Kernel Semaphore that has available counts"  );

    //  release the semaphore to the pool

    m_mpirksemrksem[irksem].Release();
}


//  Referenced Kernel Semaphore

//  attempts to allocate the semaphore, returning fTrue on success

inline BOOL CKernelSemaphorePool::CReferencedKernelSemaphore::FAllocate()
{
    return m_fAvailable && AtomicExchange( (LONG*)&m_fAvailable, 0 );
}

//  releases the semaphore

inline void CKernelSemaphorePool::CReferencedKernelSemaphore::Release()
{
    AtomicExchange( (LONG*)&m_fAvailable, 1 );
}

//  sets the user for the semaphore and gives the user an initial reference
//  note: CSyncObject must be fully qualified since it is an inaccessible base class of CReferencedKernelSemaphore

inline void CKernelSemaphorePool::CReferencedKernelSemaphore::SetUser( const OSSYNC::CSyncObject* const psyncobj )
{
    //  this semaphore had better not already be in use

    OSSYNCEnforceIrksem(    !m_fInUse,
                            "Illegal allocation of a Kernel Semaphore that is already in use" );
    OSSYNCEnforceIrksem(    !m_psyncobjUser,
                            "Illegal allocation of a Kernel Semaphore that is already in use" );

    //  mark this semaphore as in use and add an initial reference count for the
    //  user

    AtomicExchangeAdd( (LONG*) &m_l, 0x00008001 );
#ifdef SYNC_VALIDATE_IRKSEM_USAGE
    m_psyncobjUser  = psyncobj;
#endif  //  SYNC_VALIDATE_IRKSEM_USAGE
}

//  add a reference count to the semaphore

inline void CKernelSemaphorePool::CReferencedKernelSemaphore::Reference()
{
    //  increment the reference count

    AtomicIncrement( (LONG*) &m_l );

    //  there had better be at least one reference count!

    OSSYNCAssert( m_cReference > 0 );
}

//  remove a reference count from the semaphore, returning fTrue if the last
//  reference count on the semaphore was removed and the semaphore was in use
//  (this is the condition on which we can free the semaphore to the stack)

inline const BOOL CKernelSemaphorePool::CReferencedKernelSemaphore::FUnreference()
{
    //  there had better be at least one reference count!

    OSSYNCAssert( m_cReference > 0 );

    //  try forever until we succeed in removing our reference count

    LONG lBI;
    OSSYNC_FOREVER
    {
        //  read the current state of the control word as our expected before image

        const LONG lBIExpected = m_l;

        //  compute the after image of the control word by decrementing the
        //  reference count and reseting the In Use bit if and only if we are
        //  removing the last reference count

        const LONG lAI =    lBIExpected +
                            (   lBIExpected == 0x00008001 ?
                                    0xFFFF7FFF :
                                    0xFFFFFFFF );

        //  attempt to perform the transacted state transition on the control word

        lBI = AtomicCompareExchange( (LONG*)&m_l, lBIExpected, lAI );

        //  the transaction failed

        if ( lBI != lBIExpected )
        {
            //  try again

            continue;
        }

        //  the transaction succeeded

        else
        {
            // we're done

            break;
        }
    }

    //  return fTrue if we removed the last reference count and reset the In Use bit

    if ( lBI == 0x00008001 )
    {
#ifdef SYNC_VALIDATE_IRKSEM_USAGE
        m_psyncobjUser = NULL;
#endif  //  SYNC_VALIDATE_IRKSEM_USAGE
        return fTrue;
    }
    else
    {
        return fFalse;
    }
}


//  Global Kernel Semaphore Pool

extern CKernelSemaphorePool g_ksempoolGlobal;


//  Synchronization Object Performance:  Acquisition

class CSyncPerfAcquire
{
    public:

        //  member functions

        //    ctors / dtors

        CSyncPerfAcquire();
        ~CSyncPerfAcquire();

        //  member functions

        //    manipulators

        void SetAcquire();

        void SetContend();

        //    accessors

#ifdef SYNC_ANALYZE_PERFORMANCE

        QWORD   CAcquireTotal() const   { return m_cAcquire; }
        QWORD   CContendTotal() const   { return m_cContend; }

#endif  //  SYNC_ANALYZE_PERFORMANCE

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  member functions

        //    operators

        CSyncPerfAcquire& operator=( CSyncPerfAcquire& ) = delete;  //  disallowed

        //  data members

#ifdef SYNC_ANALYZE_PERFORMANCE

        //    acquire count

        volatile QWORD m_cAcquire;

        //    contend count

        volatile QWORD m_cContend;

#endif  //  SYNC_ANALYZE_PERFORMANCE
};

//  specifies that the sync object was acquired

inline void CSyncPerfAcquire::SetAcquire()
{
#ifdef SYNC_ANALYZE_PERFORMANCE

    AtomicAdd( (QWORD*)&m_cAcquire, 1 );

#endif  //  SYNC_ANALYZE_PERFORMANCE
}

//  specifies that a contention occurred while acquiring the sync object

inline void CSyncPerfAcquire::SetContend()
{
#ifdef SYNC_ANALYZE_PERFORMANCE

    AtomicAdd( (QWORD*)&m_cContend, 1 );

#endif  //  SYNC_ANALYZE_PERFORMANCE
}


//  Semaphore Information

class CSemaphoreInfo
    :   public CSyncBasicInfo,
        public CSyncPerfWait,
        public CSyncPerfAcquire
{
    public:

        //  member functions

        //    ctors / dtors

        CSemaphoreInfo( const CSyncBasicInfo& sbi )
            :   CSyncBasicInfo( sbi )
        {
        }

        //    debugging support

        void Dump( const CDumpContext& dc ) const;
};


//  Semaphore State

class CSemaphoreState
{
    public:

        //  member functions

        //    ctors / dtors

        CSemaphoreState( const CSyncStateInitNull& null ) : m_cAvail( 0 ), m_cWait( 0 ) {}
        CSemaphoreState( const INT cAvail, const INT cWait ) : m_cAvail( cAvail ), m_cWait( cWait ) {}
        CSemaphoreState( const CSemaphoreState& state ) : m_qwState( AtomicRead( (QWORD*)&state.m_qwState ) ) {}
        ~CSemaphoreState() {}

        //    operators

        CSemaphoreState& operator=( CSemaphoreState& state ) { m_qwState = AtomicRead( (QWORD*)&state.m_qwState );  return *this; }

        //    manipulators

        const BOOL FChange( const CSemaphoreState& stateCur, const CSemaphoreState& stateNew );
        void IncAvail( const INT cToInc );
        const BOOL FDecAvail();
        void IncWait();
        void DecWait();

        //    accessors

        const INT CAvail() const { return (INT)m_cAvail; }
        const INT CWait() const { return (INT)m_cWait; }
        volatile void * PAvail() { return &m_cAvail; }

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  data members

        //    transacted state representation

        union
        {
            volatile QWORD      m_qwState;

            struct
            {
                volatile DWORD  m_cAvail;
                volatile DWORD  m_cWait;
            };
        };
};

//  changes the transacted state of the semaphore using a transacted memory
//  compare/exchange operation, returning fFalse on failure

inline const BOOL CSemaphoreState::FChange( const CSemaphoreState& stateCur, const CSemaphoreState& stateNew )
{
    return AtomicCompareExchange( (QWORD*)&m_qwState, stateCur.m_qwState, stateNew.m_qwState ) == stateCur.m_qwState;
}

//  increase the available count on the semaphore by the count given using
//  a transacted memory compare/exchange operation

__forceinline void CSemaphoreState::IncAvail( const INT cToInc )
{
    AtomicExchangeAdd( (LONG*)&m_cAvail, cToInc );
}

//  tries to decrease the available count on the semaphore by one using a
//  transacted memory compare/exchange operation, returning fFalse on failure

__forceinline const BOOL CSemaphoreState::FDecAvail()
{
    OSSYNC_FOREVER
    {
        const LONG cAvail = m_cAvail;

        if ( cAvail == 0 )
        {
            return fFalse;
        }
        else if ( AtomicCompareExchange( (LONG*)&m_cAvail, cAvail, cAvail - 1 ) == cAvail )
        {
            return fTrue;
        }
    }
}

__forceinline void CSemaphoreState::IncWait()
{
    AtomicIncrement( (LONG*)&m_cWait );
}

__forceinline void CSemaphoreState::DecWait()
{
    AtomicDecrement( (LONG*)&m_cWait );
}


//  Semaphore

class CSemaphore
    :   private CSyncObject,
        private CEnhancedStateContainer< CSemaphoreState, CSyncStateInitNull, CSemaphoreInfo, CSyncBasicInfo >
{
    public:

        //  member functions

        //    ctors / dtors

        CSemaphore( const CSyncBasicInfo& sbi );
        ~CSemaphore();

        //    manipulators

        void Acquire();
        void Wait();
        const BOOL FTryAcquire();
        const BOOL FAcquire( const INT cmsecTimeout );
        const BOOL FWait( const INT cmsecTimeout );
        void Release( const INT cToRelease = 1 );
        void ReleaseAllWaiters();

        //    accessors

        const INT CWait() const;
        const INT CAvail() const;

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  member functions

        //    operators

        CSemaphore& operator=( CSemaphore& ) = delete;  //  disallowed

        //    manipulators

        // Resolves the internal timeout value to an OS level timeout.
        static const DWORD _DwOSTimeout( const INT cmsecTimeout );

        const BOOL _FTryAcquire( const INT cSpin );
        const BOOL _FAcquire( const DWORD dwTimeout );
        void _Release( const INT cToRelease );

        // Waits until the semaphore counter value changes.
        // This method has the same semantics as the WaitOnAddress() function and is
        // guaranteed to return when the address is signaled, but it is also allowed
        // to return for other reasons.  The caller should compare the new value with
        // the original.
        const BOOL _FWait( const INT cAvail, const DWORD dwTimeout );
        const BOOL _FOSWait( const INT cAvail, const DWORD dwTimeout );
};

//  acquire one count of the semaphore, waiting forever if necessary

inline void CSemaphore::Acquire()
{
    //  we will wait forever, so we should not timeout

    INT fAcquire = FAcquire( cmsecInfinite );
    OSSYNCAssert( fAcquire );
}

//  wait for the semaphore to become available without acquiring it, waiting forever if necessary.
//  we may briefly acquire the semaphore in case we need to actually wait.

inline void CSemaphore::Wait()
{
    //  we will wait forever, so we should not timeout

    INT fWait = FWait( cmsecInfinite );
    OSSYNCAssert( fWait );
}

//  try to acquire one count of the semaphore without waiting or spinning.
//  returns fFalse if a count could not be acquired

inline const BOOL CSemaphore::FTryAcquire()
{
    if ( _FTryAcquire( 0 ) )
    {
        State().SetAcquire();
        return fTrue;
    }
    else
    {
        State().SetContend();
        return fFalse;
    }
}

//  acquire one count of the semaphore, waiting only for the specified interval.
//  returns fFalse if the wait timed out before a count could be acquired

inline const BOOL CSemaphore::FAcquire( const INT cmsecTimeout )
{
    //  first try to quickly grab an available count.  if that doesn't work,
    //  retry acquiring using the full state machine

    if ( _FTryAcquire( g_cSpinMax ) || _FAcquire( _DwOSTimeout( cmsecTimeout ) ) )
    {
        State().SetAcquire();
        return fTrue;
    }
    else
    {
        State().SetContend();
        return fFalse;
    }
}

//  wait for the semaphore to become available without acquiring it, waiting only for the specified interval.
//  we may briefly acquire the semaphore in case we need to actually wait.
//  returns fFalse if the wait timed out before the semaphore is available.

inline const BOOL CSemaphore::FWait( const INT cmsecTimeout )
{
    if ( State().CAvail() > 0 )
    {
        return fTrue;
    }

    if ( _FTryAcquire( 0 ) || _FAcquire( _DwOSTimeout( cmsecTimeout ) ) )
    {
        _Release( 1 );
        State().SetAcquire();
        return fTrue;
    }
    else
    {
        State().SetContend();
        return fFalse;
    }
}

//  releases the given number of counts to the semaphore, waking the appropriate
//  number of waiters

inline void CSemaphore::Release( const INT cToRelease )
{
    _Release( cToRelease );
}

//  returns the number of execution contexts waiting on the semaphore

inline const INT CSemaphore::CWait() const
{
    OSSYNC_FOREVER
    {
        const CSemaphoreState stateCur = State();

        if ( stateCur.CAvail() > 0 && stateCur.CWait() > 0 )
        {
            // The existing waiters are in transition.
            continue;
        }
        else
        {
            return stateCur.CWait();
        }
    }
}

//  returns the number of available counts on the semaphore

inline const INT CSemaphore::CAvail() const
{
    return State().CAvail();
}


inline const BOOL CSemaphore::_FTryAcquire( const INT cSpin )
{
    INT cSpinRemaining = cSpin;

    OSSYNC_FOREVER
    {
        const CSemaphoreState stateCur = State();

        // Do not acquire the semaphore with waiting threads to avoid inadvertently
        // stealing it from those waiting threads themselves.
        if ( stateCur.CAvail() == 0 || stateCur.CWait() > 0 )
        {
            if ( cSpinRemaining )
            {
                cSpinRemaining--;
                continue;
            }
            else
            {
                return fFalse;
            }
        }
        else if ( State().FDecAvail() )
        {
            return fTrue;
        }
    }
}


//  Auto-Reset Signal Information

class CAutoResetSignalInfo
    :   public CSyncBasicInfo,
        public CSyncPerfWait,
        public CSyncPerfAcquire
{
    public:

        //  member functions

        //    ctors / dtors

        CAutoResetSignalInfo( const CSyncBasicInfo& sbi )
            :   CSyncBasicInfo( sbi )
        {
        }

        //    debugging support

        void Dump( const CDumpContext& dc ) const;
};


//  Auto-Reset Signal State

class CAutoResetSignalState
{
    public:

        //  member functions

        //    ctors / dtors

        CAutoResetSignalState( const CSyncStateInitNull& null ) : m_fSet( 0 ) {}
        CAutoResetSignalState( const INT fSet );
        CAutoResetSignalState( const INT cWait, const INT irksem );
        CAutoResetSignalState( const CAutoResetSignalState& state )
        {
            // This copy constructor is required because the default copy constructor tries to copy all members of the union
            // on top of each other, which is a problem because atomicity is a requirement when copying sync state objects.
            C_ASSERT( OffsetOf( CAutoResetSignalState, m_irksem ) == OffsetOf( CAutoResetSignalState, m_fSet ) );
            C_ASSERT( OffsetOf( CAutoResetSignalState, m_cWaitNeg ) > OffsetOf( CAutoResetSignalState, m_fSet ) );
            C_ASSERT( ( OffsetOf( CAutoResetSignalState, m_cWaitNeg ) + sizeof ( m_cWaitNeg ) ) <= ( OffsetOf( CAutoResetSignalState, m_fSet ) + sizeof ( m_fSet ) ) );
            m_fSet = state.m_fSet;
        }
        ~CAutoResetSignalState() {}

        //    operators

        CAutoResetSignalState& operator=( CAutoResetSignalState& state ) { m_fSet = state.m_fSet;  return *this; }

        //    manipulators

        const BOOL FChange( const CAutoResetSignalState& stateCur, const CAutoResetSignalState& stateNew );
        const BOOL FSimpleSet();
        const BOOL FSimpleReset();

        //    accessors

        const BOOL FNoWait() const { return m_fSet >= 0; }
        const BOOL FWait() const { return m_fSet < 0; }
        const BOOL FNoWaitAndSet() const { return m_fSet > 0; }
        const BOOL FNoWaitAndNotSet() const { return m_fSet == 0; }

        const BOOL FIsSet() const { return m_fSet > 0; }
        const INT CWait() const { OSSYNCAssert( FWait() ); return -m_cWaitNeg; }
        const CKernelSemaphorePool::IRKSEM Irksem() const { OSSYNCAssert( FWait() ); return CKernelSemaphorePool::IRKSEM( m_irksem ); }

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  data members

        //    transacted state representation (switched on bit 31)

        union
        {
            //  Mode 0:  no waiters

            volatile LONG       m_fSet;         //  m_fSet = { 0, 1 }

            //  Mode 1:  waiters

            struct
            {
                volatile USHORT m_irksem;       //  0 <= m_irksem <= ( 1 << 16 ) - 2
                volatile SHORT  m_cWaitNeg;     //  -( ( 1 << 15 ) - 1 ) <= m_cWaitNeg <= -1
            };
        };
};

//  ctor

inline CAutoResetSignalState::CAutoResetSignalState( const INT fSet )
{
    //  validate IN args

    OSSYNCAssert( fSet == 0 || fSet == 1 );

    //  set state

    m_fSet = LONG( fSet );
}

//  ctor

inline CAutoResetSignalState::CAutoResetSignalState( const INT cWait, const INT irksem )
{
    //  validate IN args

    OSSYNCAssert( cWait > 0 );
    OSSYNCAssert( cWait <= 0x7FFF );
    OSSYNCAssert( irksem >= 0 );
    OSSYNCAssert( irksem <= 0xFFFE );

    //  set waiter count

    m_cWaitNeg = SHORT( -cWait );

    //  set semaphore

    m_irksem = (USHORT) irksem;
}

//  changes the transacted state of the signal using a transacted memory
//  compare/exchange operation, returning 0 on failure

inline const BOOL CAutoResetSignalState::FChange( const CAutoResetSignalState& stateCur, const CAutoResetSignalState& stateNew )
{
    return AtomicCompareExchange( (LONG*)&m_fSet, stateCur.m_fSet, stateNew.m_fSet ) == stateCur.m_fSet;
}

//  tries to set the signal state from either the set or reset with no waiters states
//  using a transacted memory compare/exchange operation, returning fFalse on failure

__forceinline const BOOL CAutoResetSignalState::FSimpleSet()
{
    //  try forever to change the state of the signal

    OSSYNC_FOREVER
    {
        //  get current value

        const LONG fSet = m_fSet;

        //  munge start value such that the transaction will only work if we are in
        //  mode 0 (we do this to save a branch)

        const LONG fSetStart = fSet & 0x7FFFFFFF;

        //  compute end value relative to munged start value

        const LONG fSetEnd = 1;

        //  validate transaction

        OSSYNCAssert( fSet < 0 || ( ( fSetStart == 0 || fSetStart == 1 ) && fSetEnd == 1 ) );

        //  attempt the transaction

        const LONG fSetOld = AtomicCompareExchange( (LONG*)&m_fSet, fSetStart, fSetEnd );

        //  the transaction succeeded

        if ( fSetOld == fSetStart )
        {
            //  return success

            return fTrue;
        }

        //  the transaction failed

        else
        {
            //  the transaction failed because of a collision with another context

            if ( fSetOld >= 0 )
            {
                //  try again

                continue;
            }

            //  the transaction failed because there are waiters

            else
            {
                //  return failure

                return fFalse;
            }
        }
    }
}

//  tries to reset the signal state from either the set or reset with no waiters states
//  using a transacted memory compare/exchange operation, returning fFalse on failure

__forceinline const BOOL CAutoResetSignalState::FSimpleReset()
{
    //  try forever to change the state of the signal

    OSSYNC_FOREVER
    {
        //  get current value

        const LONG fSet = m_fSet;

        //  munge start value such that the transaction will only work if we are in
        //  mode 0 (we do this to save a branch)

        const LONG fSetStart = fSet & 0x7FFFFFFF;

        //  compute end value relative to munged start value

        const LONG fSetEnd = 0;

        //  validate transaction

        OSSYNCAssert( fSet < 0 || ( ( fSetStart == 0 || fSetStart == 1 ) && fSetEnd == 0 ) );

        //  attempt the transaction

        const LONG fSetOld = AtomicCompareExchange( (LONG*)&m_fSet, fSetStart, fSetEnd );

        //  the transaction succeeded

        if ( fSetOld == fSetStart )
        {
            //  return success

            return fTrue;
        }

        //  the transaction failed

        else
        {
            //  the transaction failed because of a collision with another context

            if ( fSetOld >= 0 )
            {
                //  try again

                continue;
            }

            //  the transaction failed because there are waiters

            else
            {
                //  return failure

                return fFalse;
            }
        }
    }
}


//  Auto-Reset Signal

class CAutoResetSignal
    :   private CSyncObject,
        private CEnhancedStateContainer< CAutoResetSignalState, CSyncStateInitNull, CAutoResetSignalInfo, CSyncBasicInfo >
{
    public:

        //  member functions

        //    ctors / dtors

        CAutoResetSignal( const CSyncBasicInfo& sbi );
        ~CAutoResetSignal();

        //    manipulators

        void Wait();
        const BOOL FTryWait();
        const BOOL FWait( const INT cmsecTimeout );
        const BOOL FIsSet() const;

        void Set();
        void Reset();

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  member functions

        //    operators

        CAutoResetSignal& operator=( CAutoResetSignal& ) = delete;  //  disallowed

        //    manipulators

        const BOOL _FWait( const INT cmsecTimeout );

        void _Set();
};

//  waits for the signal to be set, forever if necessary.  when the wait completes,
//  the signal will be reset

inline void CAutoResetSignal::Wait()
{
    //  we will wait forever, so we should not timeout

    const BOOL fWait = FWait( cmsecInfinite );
    OSSYNCAssert( fWait );
}

//  tests the state of the signal without waiting or spinning, returning fFalse
//  if the signal was not set.  if the signal was set, the signal will be reset.
//  there is a significant difference between this method and FIsSet(), which will
//  also test for the current state, but will not reset it in case it is set.

inline const BOOL CAutoResetSignal::FTryWait()
{
    //  we can satisfy the wait if we can successfully change the state of the
    //  signal from set to reset with no waiters

    const BOOL fSuccess = State().FChange( CAutoResetSignalState( 1 ), CAutoResetSignalState( 0 ) );

    //  we did not successfully wait for the signal

    if ( !fSuccess )
    {
        //  this is a contention

        State().SetContend();
    }

    //  we did successfully wait for the signal

    else
    {
        //  note that we acquired the signal

        State().SetAcquire();
    }

    return fSuccess;
}

//  wait for the signal to be set, but only for the specified interval,
//  returning fFalse if the wait timed out before the signal was set.  if the
//  wait completes, the signal will be reset

inline const BOOL CAutoResetSignal::FWait( const INT cmsecTimeout )
{
    //  first try to quickly pass through the signal.  if that doesn't work,
    //  retry wait using the full state machine

    return FTryWait() || _FWait( cmsecTimeout );
}

//  tests the state of the signal without waiting or spinning, returning fFalse
//  if the signal was not set.  if the signal was set, the signal will not be reset.
//  there is a significant difference between this method and FTryWait(), which will
//  also test for the current state, but will reset it in case it is set.

inline const BOOL CAutoResetSignal::FIsSet() const
{
    return State().FIsSet();
}

//  sets the signal, releasing up to one waiter.  if a waiter is released, then
//  the signal will be reset.  if a waiter is not released, the signal will
//  remain set

inline void CAutoResetSignal::Set()
{
    //  we failed to change the signal state from reset with no waiters to set
    //  or from set to set (a nop)

    if ( !State().FSimpleSet() )
    {
        //  retry set using the full state machine

        _Set();
    }
}

//  resets the signal

inline void CAutoResetSignal::Reset()
{
    //  if and only if the signal is in the set state, change it to the reset state

    State().FChange( CAutoResetSignalState( 1 ), CAutoResetSignalState( 0 ) );
}


//  Manual-Reset Signal Information

class CManualResetSignalInfo
    :   public CSyncBasicInfo,
        public CSyncPerfWait,
        public CSyncPerfAcquire
{
    public:

        //  member functions

        //    ctors / dtors

        CManualResetSignalInfo( const CSyncBasicInfo& sbi )
            :   CSyncBasicInfo( sbi )
        {
        }

        //    debugging support

        void Dump( const CDumpContext& dc ) const;
};


//  Manual-Reset Signal State

class CManualResetSignalState
{
    public:

        //  member functions

        //    ctors / dtors

        CManualResetSignalState( const CSyncStateInitNull& null ) : m_fSet( 0 ) {}
        CManualResetSignalState( const INT fSet );
        CManualResetSignalState( const INT cWait, const INT irksem );
        CManualResetSignalState( const CManualResetSignalState& state )
        {
            // This copy constructor is required because the default copy constructor tries to copy all members of the union
            // on top of each other, which is a problem because atomicity is a requirement when copying sync state objects.
            C_ASSERT( OffsetOf( CManualResetSignalState, m_irksem ) == OffsetOf( CManualResetSignalState, m_fSet ) );
            C_ASSERT( OffsetOf( CManualResetSignalState, m_cWaitNeg ) > OffsetOf( CManualResetSignalState, m_fSet ) );
            C_ASSERT( ( OffsetOf( CManualResetSignalState, m_cWaitNeg ) + sizeof ( m_cWaitNeg ) ) <= ( OffsetOf( CManualResetSignalState, m_fSet ) + sizeof ( m_fSet ) ) );
            m_fSet = state.m_fSet;
        }
        ~CManualResetSignalState() {}

        //    operators

        CManualResetSignalState& operator=( CManualResetSignalState& state ) { m_fSet = state.m_fSet;  return *this; }

        //    manipulators

        const BOOL FChange( const CManualResetSignalState& stateCur, const CManualResetSignalState& stateNew );
        const CManualResetSignalState Set();
        const CManualResetSignalState Reset();

        //    accessors

        const BOOL FNoWait() const { return m_fSet >= 0; }
        const BOOL FWait() const { return m_fSet < 0; }
        const BOOL FNoWaitAndSet() const { return m_fSet > 0; }
        const BOOL FNoWaitAndNotSet() const { return m_fSet == 0; }

        const BOOL FIsSet() const { return m_fSet > 0; }
        const INT CWait() const { OSSYNCAssert( FWait() ); return -m_cWaitNeg; }
        const CKernelSemaphorePool::IRKSEM Irksem() const { OSSYNCAssert( FWait() ); return CKernelSemaphorePool::IRKSEM( m_irksem ); }

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  data members

        //    transacted state representation (switched on bit 31)

        union
        {
            //  Mode 0:  no waiters

            volatile LONG       m_fSet;         //  m_fSet = { 0, 1 }

            //  Mode 1:  waiters

            struct
            {
                volatile USHORT m_irksem;       //  0 <= m_irksem <= ( 1 << 16 ) - 2
                volatile SHORT  m_cWaitNeg;     //  -( ( 1 << 15 ) - 1 ) <= m_cWaitNeg <= -1
            };
        };
};

//  ctor

inline CManualResetSignalState::CManualResetSignalState( const INT fSet )
{
    //  set state

    m_fSet = LONG( fSet );
}

//  ctor

inline CManualResetSignalState::CManualResetSignalState( const INT cWait, const INT irksem )
{
    //  validate IN args

    OSSYNCAssert( cWait > 0 );
    OSSYNCAssert( cWait <= 0x7FFF );
    OSSYNCAssert( irksem >= 0 );
    OSSYNCAssert( irksem <= 0xFFFE );

    //  set waiter count

    m_cWaitNeg = SHORT( -cWait );

    //  set semaphore

    m_irksem = (USHORT) irksem;
}

//  changes the transacted state of the signal using a transacted memory
//  compare/exchange operation, returning fFalse on failure

inline const BOOL CManualResetSignalState::FChange( const CManualResetSignalState& stateCur, const CManualResetSignalState& stateNew )
{
    return AtomicCompareExchange( (LONG*)&m_fSet, stateCur.m_fSet, stateNew.m_fSet ) == stateCur.m_fSet;
}

//  changes the transacted state of the signal to set using a transacted memory
//  exchange operation and returns the original state of the signal

inline const CManualResetSignalState CManualResetSignalState::Set()
{
    const CManualResetSignalState stateNew( 1 );
    return CManualResetSignalState( AtomicExchange( (LONG*)&m_fSet, stateNew.m_fSet ) );
}

//  changes the transacted state of the signal to reset using a transacted memory
//  exchange operation and returns the original state of the signal

inline const CManualResetSignalState CManualResetSignalState::Reset()
{
    const CManualResetSignalState stateNew( 0 );
    return CManualResetSignalState( AtomicExchange( (LONG*)&m_fSet, stateNew.m_fSet ) );
}


//  Manual-Reset Signal

class CManualResetSignal
    :   private CSyncObject,
        private CEnhancedStateContainer< CManualResetSignalState, CSyncStateInitNull, CManualResetSignalInfo, CSyncBasicInfo >
{
    public:

        //  member functions

        //    ctors / dtors

        CManualResetSignal( const CSyncBasicInfo& sbi );
        ~CManualResetSignal();

        //    manipulators

        void Wait();
        const BOOL FTryWait();
        const BOOL FWait( const INT cmsecTimeout );
        const BOOL FIsSet() const;

        void Set();
        void Reset();

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  member functions

        //    operators

        CManualResetSignal& operator=( CManualResetSignal& ) = delete;  //  disallowed

        //    manipulators

        const BOOL _FWait( const INT cmsecTimeout );
};

//  waits for the signal to be set, forever if necessary

inline void CManualResetSignal::Wait()
{
    //  we will wait forever, so we should not timeout

    INT fWait = FWait( cmsecInfinite );
    OSSYNCAssert( fWait );
}

//  tests the state of the signal without waiting or spinning, returning fFalse
//  if the signal was not set

inline const BOOL CManualResetSignal::FTryWait()
{
    const BOOL fSuccess = State().FNoWaitAndSet();

    //  we did not successfully wait for the signal

    if ( !fSuccess )
    {
        //  this is a contention

        State().SetContend();
    }

    //  we did successfully wait for the signal

    else
    {
        //  note that we acquired the signal

        State().SetAcquire();
    }

    return fSuccess;
}

//  wait for the signal to be set, but only for the specified interval,
//  returning fFalse if the wait timed out before the signal was set

inline const BOOL CManualResetSignal::FWait( const INT cmsecTimeout )
{
    //  first try to quickly pass through the signal.  if that doesn't work,
    //  retry wait using the full state machine

    return FTryWait() || _FWait( cmsecTimeout );
}

//  tests if the signal is set

inline const BOOL CManualResetSignal::FIsSet() const
{
    return State().FIsSet();
}

//  sets the signal, releasing any waiters

inline void CManualResetSignal::Set()
{
    //  change the signal state to set

    const CManualResetSignalState stateOld = State().Set();

    //  there were waiters on the signal

    if ( stateOld.FWait() )
    {
        //  release all the waiters

        g_ksempoolGlobal.Ksem( stateOld.Irksem(), this ).Release( stateOld.CWait() );
    }
}

//  resets the signal

inline void CManualResetSignal::Reset()
{
    //  if and only if the signal is in the set state, change it to the reset state

    State().FChange( CManualResetSignalState( 1 ), CManualResetSignalState( 0 ) );
}


//  Lock Object Base Class
//
//  All Lock Objects are derived from this class

class CLockObject
    :   public CSyncObject
{
    public:

        //  member functions

        //    ctors / dtors

        CLockObject() {}
        ~CLockObject() {}

    private:

        //  member functions

        //    operators

        CLockObject& operator=( CLockObject& ) = delete;  //  disallowed
};


//  Lock Object Basic Information

class CLockBasicInfo
    :   public CSyncBasicInfo
{
    public:

        //  member functions

        //    ctors / dtors

#ifdef SYNC_ENHANCED_STATE

        CLockBasicInfo( const CSyncBasicInfo& sbi, const INT rank, const INT subrank );
        ~CLockBasicInfo();

#else  //  !SYNC_ENHANCED_STATE

        CLockBasicInfo( const CSyncBasicInfo& sbi, const INT rank, const INT subrank ) : CSyncBasicInfo( sbi ) {}
        ~CLockBasicInfo() {}

#endif  //  SYNC_ENHANCED_STATE

        //    accessors

#ifdef SYNC_DEADLOCK_DETECTION

        const INT Rank() const { return m_rank; }
        const INT SubRank() const { return m_subrank; }

#endif  //  SYNC_DEADLOCK_DETECTION

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  member functions

        //    operators

        CLockBasicInfo& operator=( CLockBasicInfo& ) = delete;  //  disallowed

        //  data members

#ifdef SYNC_DEADLOCK_DETECTION

        //    Rank and Subrank

        INT m_rank;
        INT m_subrank;

#endif  //  SYNC_DEADLOCK_DETECTION
};


//  Lock Object Performance:  Hold

class CLockPerfHold
{
    public:

        //  member functions

        //    ctors / dtors

        CLockPerfHold();
        ~CLockPerfHold();

        //  member functions

        //    manipulators

        void StartHold();
        void StopHold();

        //    accessors

#ifdef SYNC_ANALYZE_PERFORMANCE

        QWORD   CHoldTotal() const      { return m_cHold; }
        double  CsecHoldElapsed() const { return    (double)(signed __int64)m_qwHRTHoldElapsed /
                                                    (double)(signed __int64)QwOSSyncIHRTFreq(); }

#endif  //  SYNC_ANALYZE_PERFORMANCE

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  member functions

        //    operators

        CLockPerfHold& operator=( CLockPerfHold& ) = delete;  //  disallowed

        //  data members

#ifdef SYNC_ANALYZE_PERFORMANCE

        //    hold count

        volatile QWORD m_cHold;

        //    elapsed hold time

        volatile QWORD m_qwHRTHoldElapsed;

#endif  //  SYNC_ANALYZE_PERFORMANCE
};

//  starts the hold timer for the lock object

inline void CLockPerfHold::StartHold()
{
#ifdef SYNC_ANALYZE_PERFORMANCE

    //  increment the hold count

    AtomicAdd( (QWORD*)&m_cHold, 1 );

    //  subtract the start hold time from the elapsed hold time.  this starts
    //  an elapsed time computation for this context.  StopHold() will later
    //  add the end hold time to the elapsed time, causing the following net
    //  effect:
    //
    //  m_qwHRTHoldElapsed += <end time> - <start time>
    //
    //  we simply choose to go ahead and do the subtraction now to save storage

    AtomicAdd( (QWORD*)&m_qwHRTHoldElapsed, QWORD( -__int64( QwOSSyncIHRTCount() ) ) );

#endif  //  SYNC_ANALYZE_PERFORMANCE
}

//  stops the hold timer for the lock object

inline void CLockPerfHold::StopHold()
{
#ifdef SYNC_ANALYZE_PERFORMANCE

    //  add the end hold time to the elapsed hold time.  this completes the
    //  computation started in StartHold()

    AtomicAdd( (QWORD*)&m_qwHRTHoldElapsed, QwOSSyncIHRTCount() );

#endif  //  SYNC_ANALYZE_PERFORMANCE
}


//  Lock Owner Record
//  Ownership information is maintained through 2 linked-lists:
//    1. Current context's owned list: tracked through CLS::pownerLockHead
//    2. A lock's owning context's list: tracked through CLockDeadlockDetectionInfo::m_ownerHead
//  Each COwner* is linked into these lists through m_pclsOwner and m_pownerContextNext members, respectively.
//  The CLS list can be accessed locklessly because only 1 context (thread) touches it.
//  Access to the lock's owner list is protected by CLockDeadlockDetectionInfo::m_semOwnerList.
//
//  m_ownerHead guarantees a fast-path for tracking ownership for cases where only 1 context owns the lock (e.g. critical sections).
//  On this fast-path: instead of doing alloc+list maintenance, we claim m_ownerHead by setting its m_pclsOwner to the current cls locklessly and reuse it.
//  Once we are done, we set m_pclsOwner to null to release ownership.

class CLockDeadlockDetectionInfo;

class COwner
{
    public:

        //  member functions

        //    ctors / dtors

        COwner();
        ~COwner();

        void* operator new( size_t cb ) { return ESMemoryNew( cb ); }
        void operator delete( void* pv ) { ESMemoryDelete( pv ); }

    public:

        //  member functions

        //    operators

        COwner& operator=( COwner& ) = delete;  //  disallowed

        //  data members

        //    owning context

        CLS*                            m_pclsOwner;

        //    next context owning this lock

        COwner*                         m_pownerContextNext;

        //    owned lock object

        CLockDeadlockDetectionInfo*     m_plddiOwned;

        //    next lock owned by this context

        COwner*                         m_pownerLockNext;

        //    owning group for this context and lock

        DWORD                           m_group;
};


//  Lock Object Deadlock Detection Information

class CLockDeadlockDetectionInfo
{
    public:

        //  types

        //    subrank used to disable deadlock detection at the subrank level

        enum
        {
            subrankNoDeadlock = INT_MAX
        };

        //  member functions

        //    ctors / dtors

        CLockDeadlockDetectionInfo( const CLockBasicInfo& lbi );
        ~CLockDeadlockDetectionInfo();

        //  member functions

        //    manipulators

        void AddAsWaiter( const DWORD group = -1 );
        void RemoveAsWaiter( const DWORD group = -1 );

        void AddAsOwner( const DWORD group = -1 );
        void RemoveAsOwner( const DWORD group = -1 );

        static void OSSYNCAPI NextOwnershipIsNotADeadlock();
        static void OSSYNCAPI DisableDeadlockDetection();
        static void OSSYNCAPI EnableDeadlockDetection();
        static void OSSYNCAPI DisableOwnershipTracking();
        static void OSSYNCAPI EnableOwnershipTracking();
        static void OSSYNCAPI DisableLockCheckOnApiExit();
        static void OSSYNCAPI EnableLockCheckOnApiExit();

        //    accessors

#ifdef SYNC_DEADLOCK_DETECTION

        const BOOL FOwner( const DWORD group = -1 );
        const BOOL FNotOwner( const DWORD group = -1 );
        const BOOL FOwned();
        const BOOL FNotOwned();

        const VOID AssertCanBeWaiter();
        const BOOL FWaiter( const DWORD group = -1 );
        const BOOL FNotWaiter( const DWORD group = -1 );

        static void OSSYNCAPI GetApiEntryState(
            __out DWORD *pcDisableDeadlockDetection,
            __out DWORD *pcDisableOwnershipTracking,
            __out DWORD *pcLocks );
        static void OSSYNCAPI AssertCleanApiExit(
            DWORD cDisableDeadlockDetection,
            DWORD cDisableOwnershipTracking,
            DWORD cLocks );

        const CLockBasicInfo& Info() { return *m_plbiParent; }

#else   //  !SYNC_DEADLOCK_DETECTION

#ifdef DEBUG

        const BOOL FOwner( const DWORD group = -1 ) { return fTrue; }
        const BOOL FNotOwner( const DWORD group = -1 ) { return fTrue; }
        const BOOL FOwned() { return fTrue; }
        const BOOL FNotOwned() { return fTrue; }

        const BOOL FWaiter( const DWORD group = -1 ) { return fTrue; }
        const BOOL FNotWaiter( const DWORD group = -1 ) { return fTrue; }

#endif  //  DEBUG

        const VOID AssertCanBeWaiter() {}
        static void OSSYNCAPI GetApiEntryState(
            __out DWORD *pcDisableDeadlockDetection,
            __out DWORD *pcDisableOwnershipTracking,
            __out DWORD *pcLocks ) {}
        static void OSSYNCAPI AssertCleanApiExit(
            DWORD cDisableDeadlockDetection,
            DWORD cDisableOwnershipTracking,
            DWORD cLocks ) {}

#endif  //  SYNC_DEADLOCK_DETECTION

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  member functions

        //    operators

        CLockDeadlockDetectionInfo& operator=( CLockDeadlockDetectionInfo& ) = delete;  //  disallowed

        //  data members

#ifdef SYNC_DEADLOCK_DETECTION

        //    parent lock object information

        const CLockBasicInfo*           m_plbiParent;

        //    semaphore protecting owner list

        CSemaphore                      m_semOwnerList;

        //    owner list head

        COwner                          m_ownerHead;

#endif  //  SYNC_DEADLOCK_DETECTION
};

//  adds the current context as a waiter for the lock object as a member of the
//  specified group

inline void CLockDeadlockDetectionInfo::AddAsWaiter( const DWORD group )
{
#ifdef SYNC_DEADLOCK_DETECTION

    //  this context had better not be a waiter for the lock

    OSSYNCAssert( FNotWaiter( group ) );

    //  we had better not already be waiting for something else!

    CLS* const pcls = Pcls();
    OSSYNCAssert( !pcls->plddiLockWait );
    OSSYNCAssert( !pcls->groupLockWait );

    //  add this context as a waiter for the lock

    pcls->plddiLockWait = this;
    pcls->groupLockWait = group;

    //  this context had better be a waiter for the lock

    OSSYNCAssert( FWaiter( group ) );

#endif  //  SYNC_DEADLOCK_DETECTION
}

//  removes the current context as a waiter for the lock object

inline void CLockDeadlockDetectionInfo::RemoveAsWaiter( const DWORD group )
{
#ifdef SYNC_DEADLOCK_DETECTION

    //  this context had better be a waiter for the lock

    OSSYNCAssert( FWaiter( group ) );

    //  remove this context as a waiter for the lock

    CLS* const pcls = Pcls();
    pcls->plddiLockWait = NULL;
    pcls->groupLockWait = 0;

    //  this context had better not be a waiter for the lock anymore

    OSSYNCAssert( FNotWaiter( group ) );

#endif  //  SYNC_DEADLOCK_DETECTION
}

//  adds the current context as an owner of the lock object as a member of the
//  specified group

inline void CLockDeadlockDetectionInfo::AddAsOwner( const DWORD group )
{
#ifdef SYNC_DEADLOCK_DETECTION

    //  this context had better not be an owner of the lock

    OSSYNCAssert( FNotOwner( group ) );

    //  add this context as an owner of the lock

    CLS* const pcls = Pcls();

    if ( !pcls->cDisableOwnershipTracking )
    {
        COwner* powner  = &m_ownerHead;

        if ( AtomicCompareExchangePointer( (void **)&powner->m_pclsOwner, NULL, pcls ) != NULL )
        {
            powner = new COwner;
            OSSYNCEnforceSz( powner,  "Failed to allocate Deadlock Detection Owner Record"  );

            m_semOwnerList.Acquire();

            powner->m_pclsOwner             = pcls;
            powner->m_pownerContextNext     = m_ownerHead.m_pownerContextNext;
            m_ownerHead.m_pownerContextNext = powner;

            m_semOwnerList.Release();
        }

        powner->m_plddiOwned        = this;
        powner->m_pownerLockNext    = pcls->pownerLockHead;
        pcls->pownerLockHead        = powner;
        powner->m_group             = group;
    }

    //  reset override

    if ( pcls->cOverrideDeadlock )
    {
        pcls->cOverrideDeadlock--;
    }

    //  this context had better be an owner of the lock

    OSSYNCAssert( FOwner( group ) );

#endif  //  SYNC_DEADLOCK_DETECTION
}

//  removes the current context as an owner of the lock object

inline void CLockDeadlockDetectionInfo::RemoveAsOwner( const DWORD group )
{
#ifdef SYNC_DEADLOCK_DETECTION

    //  this context had better be an owner of the lock

    OSSYNCAssert( FOwner( group ) );

    //  remove this context as an owner of the lock

    CLS* const pcls = Pcls();

    //  First remove it from the ownership list of the current cls
    
    if ( !pcls->cDisableOwnershipTracking )
    {
        COwner** ppownerLock = &pcls->pownerLockHead;
        while ( (*ppownerLock)->m_plddiOwned != this || (*ppownerLock)->m_group != group )
        {
            ppownerLock = &(*ppownerLock)->m_pownerLockNext;
        }

        COwner* pownerLock = *ppownerLock;
        *ppownerLock = pownerLock->m_pownerLockNext;

        pownerLock->m_plddiOwned        = NULL;
        pownerLock->m_pownerLockNext    = NULL;

        //  Now remove it from the ownership list of the lock
        //  Fast-path: if m_ownerHead was used to track ownership info, we release it by setting m_pclsOwner to NULL
        //  else Slow-path: traverse the list, find the correct COwner and remove it.
        
        if ( m_ownerHead.m_pclsOwner == pcls && m_ownerHead.m_group == group )
        {
            m_ownerHead.m_group = 0;
            CLS* const pOldCls = (CLS* const) AtomicExchangePointer( (void**) &m_ownerHead.m_pclsOwner, NULL );
            OSSYNCAssert( pOldCls == pcls );
        }
        else
        {
            m_semOwnerList.Acquire();

            COwner** ppownerContext = &m_ownerHead.m_pownerContextNext;
            while ( (*ppownerContext)->m_pclsOwner != pcls || (*ppownerContext)->m_group != group )
            {
                ppownerContext = &(*ppownerContext)->m_pownerContextNext;
            }

            COwner* pownerContext = *ppownerContext;
            *ppownerContext = pownerContext->m_pownerContextNext;

            m_semOwnerList.Release();

            delete pownerContext;
        }
    }

    //  this context had better not be an owner of the lock anymore

    OSSYNCAssert( FNotOwner( group ) );

#endif  //  SYNC_DEADLOCK_DETECTION
}

//  overrides deadlock detection using ranks for the next ownership

inline void OSSYNCAPI CLockDeadlockDetectionInfo::NextOwnershipIsNotADeadlock()
{
#ifdef SYNC_DEADLOCK_DETECTION

    Pcls()->cOverrideDeadlock++;

#endif  //  SYNC_DEADLOCK_DETECTION
}

//  disables deadlock detection for this context

inline void OSSYNCAPI CLockDeadlockDetectionInfo::DisableDeadlockDetection()
{
#ifdef SYNC_DEADLOCK_DETECTION

    Pcls()->cDisableDeadlockDetection++;

#endif  //  SYNC_DEADLOCK_DETECTION
}

//  enables deadlock detection for this context

inline void OSSYNCAPI CLockDeadlockDetectionInfo::EnableDeadlockDetection()
{
#ifdef SYNC_DEADLOCK_DETECTION

    Pcls()->cDisableDeadlockDetection--;

#endif  //  SYNC_DEADLOCK_DETECTION
}

//  disables ownership tracking for this context

inline void OSSYNCAPI CLockDeadlockDetectionInfo::DisableOwnershipTracking()
{
#ifdef SYNC_DEADLOCK_DETECTION

    Pcls()->cDisableOwnershipTracking++;

#endif  //  SYNC_DEADLOCK_DETECTION
}

//  enables ownership tracking for this context

inline void OSSYNCAPI CLockDeadlockDetectionInfo::EnableOwnershipTracking()
{
#ifdef SYNC_DEADLOCK_DETECTION

    Pcls()->cDisableOwnershipTracking--;

#endif  //  SYNC_DEADLOCK_DETECTION
}

// disables assert on API exit for no locks being held

inline void OSSYNCAPI CLockDeadlockDetectionInfo::DisableLockCheckOnApiExit()
{
#ifdef SYNC_DEADLOCK_DETECTION
    
    Pcls()->cDisableLockCheckOnApiExit++;
    
#endif  //  SYNC_DEADLOCK_DETECTION
}

// enables assert on API exit for no locks being held

inline void OSSYNCAPI CLockDeadlockDetectionInfo::EnableLockCheckOnApiExit()
{
#ifdef SYNC_DEADLOCK_DETECTION
    
    Pcls()->cDisableLockCheckOnApiExit--;
    
#endif  //  SYNC_DEADLOCK_DETECTION
}


#ifdef SYNC_DEADLOCK_DETECTION

inline void OSSYNCAPI CLockDeadlockDetectionInfo::GetApiEntryState(
    __out DWORD *pcDisableDeadlockDetection,
    __out DWORD *pcDisableOwnershipTracking,
    __out DWORD *pcLocks )
{
    CLS* const pcls     = Pcls();
    *pcDisableDeadlockDetection = pcls->cDisableDeadlockDetection;
    *pcDisableOwnershipTracking = pcls->cDisableOwnershipTracking;
    DWORD cLocks = 0;
    COwner *pownerLock = Pcls()->pownerLockHead;
    while (pownerLock)
    {
        cLocks++;
        pownerLock = pownerLock->m_pownerLockNext;
    }
    *pcLocks = cLocks;
}

// check that we are not leaking deadlock/ownership disabling or locks on API exit
//
// it is possible that certain APIs may need to take locks which are released later by another API
// You can fix the assert for those APIs by switching to JET_TRY_(func, fTrue) rather than JET_TRY( func )

inline void OSSYNCAPI CLockDeadlockDetectionInfo::AssertCleanApiExit(
    DWORD cDisableDeadlockDetection,
    DWORD cDisableOwnershipTracking,
    DWORD cLocks )
{
    CLS* const  pcls        = Pcls();
    OSSYNCAssertSz( pcls->cDisableDeadlockDetection == cDisableDeadlockDetection, "deadlock detection state changed within the API|worker task" );
    OSSYNCAssertSz( pcls->cDisableOwnershipTracking == cDisableOwnershipTracking, "ownership tracking state changed within the API|worker task" );
    if ( !pcls->cDisableLockCheckOnApiExit )
    {
        DWORD cCurrentLocks = 0;
        COwner *pownerLock = Pcls()->pownerLockHead;
        while (pownerLock)
        {
            cCurrentLocks++;
            pownerLock = pownerLock->m_pownerLockNext;
        }
        OSSYNCAssertSz( cCurrentLocks == cLocks, "locks leaked by API|worker task" );
    }
}

//  returns fTrue if the current context is an owner of the lock object

inline const BOOL CLockDeadlockDetectionInfo::FOwner( const DWORD group )
{
    if ( Pcls()->cDisableOwnershipTracking != 0 )
    {
        return fTrue;
    }

    COwner* pownerLock = Pcls()->pownerLockHead;
    while ( pownerLock && ( pownerLock->m_plddiOwned != this || pownerLock->m_group != group ) )
    {
        pownerLock = pownerLock->m_pownerLockNext;
    }

    return pownerLock && pownerLock->m_group == group;
}

//  returns fTrue if the current context is not an owner of the lock object
 
inline const BOOL CLockDeadlockDetectionInfo::FNotOwner( const DWORD group )
{
    return Pcls()->cDisableOwnershipTracking != 0 || !FOwner( group );
}

//  returns fTrue if any context is an owner of the lock object
 
inline const BOOL CLockDeadlockDetectionInfo::FOwned()
{
    return m_ownerHead.m_pclsOwner || m_ownerHead.m_pownerContextNext;
}

//  returns fTrue if no context is an owner of the lock object
 
inline const BOOL CLockDeadlockDetectionInfo::FNotOwned()
{
    return !FOwned();
}

//  checks if the current context can wait for the lock object without
//  violating any deadlock constraints and asserts otherwise

inline const VOID CLockDeadlockDetectionInfo::AssertCanBeWaiter()
{
    //  find the minimum rank, subrank of all locks owned by the current context

    CLS* const  pcls        = Pcls();
    COwner*     powner      = pcls->pownerLockHead;
    COwner*     pownerMin   = pcls->pownerLockHead;
    INT         Rank        = INT_MAX;
    INT         SubRank     = INT_MAX;
    

    while ( powner )
    {
        if (    powner->m_plddiOwned->Info().Rank() < Rank ||
                (   powner->m_plddiOwned->Info().Rank() == Rank &&
                    powner->m_plddiOwned->Info().SubRank() < SubRank ) )
        {
            Rank      = powner->m_plddiOwned->Info().Rank();
            SubRank   = powner->m_plddiOwned->Info().SubRank();
            pownerMin = powner;
        }

        powner = powner->m_pownerLockNext;
    }

    //  test this rank, subrank against our rank, subrank

    OSSYNCAssertSzRTL( Rank > Info().Rank()                                                                         ||
                       ( Rank == Info().Rank() && SubRank > Info().SubRank() )                                      ||
                       ( Rank == Info().Rank() && SubRank == Info().SubRank() && SubRank == subrankNoDeadlock )     ||
                       pcls->cOverrideDeadlock                                                                      ||
                       pcls->cDisableDeadlockDetection, "Potential Deadlock Detected (Rank Violation)" );
}

//  returns fTrue if the current context is a waiter of the lock object
 
inline const BOOL CLockDeadlockDetectionInfo::FWaiter( const DWORD group )
{
    CLS* const pcls = Pcls();
    return pcls->plddiLockWait == this && pcls->groupLockWait == group;
}

//  returns fTrue if the current context is not a waiter of the lock object
 
inline const BOOL CLockDeadlockDetectionInfo::FNotWaiter( const DWORD group )
{
    return !FWaiter( group );
}

#endif  //  SYNC_DEADLOCK_DETECTION

//  This is a simple init/term lock that requires no synchronization 
//  objects (ksemaphore or critical sections or other) and quiesces
//  to a running state via a sleep(1).

class CInitTermLock
{
    private:

        typedef ULONG ControlWord;

        volatile ControlWord            m_cwState;

        const static ControlWord        fInitInProgress = 0x80000000;
        const static ControlWord        fTermInProgress = 0x40000000;
        const static ControlWord        mskRefCount     = ~( fInitInProgress | fTermInProgress );
#ifdef DEBUG
        const static ControlWord        mskBadRefCount  = 0x3ffff800;       // should not have 2048+ consumers ...
#else
        const static ControlWord        mskBadRefCount  = 0x30000000;       // should not have 268M+ consumers ...
#endif

        C_ASSERT( mskBadRefCount == ( mskBadRefCount & mskRefCount ) );

        //  Allowed transitions (From -> To):
        //      0x00000000 -> 0x80000001    //  Begin Init
        //      0x80000001 -> 0x00000001    //  Complete Init (successfully)
        //                 -> 0x00000000    //  Decomplete Init (failed init)
        //      0x00000001 -> 0x00000002    //  Increment consumers
        //                 -> 0x40000000    //  Begin Term
        //      0x40000000 -> 0x00000000    //  Complete Term
        //      0x00000002 -> 0x00000001    //  Decrement consumers
        //                 -> 0x00000003    //  Increment consumers
        VOID AssertValidStateTransition_( const ControlWord cwInitial, const ControlWord cwFinal )
        {
            OSSYNCAssert( 0 == ( mskBadRefCount & cwInitial ) );
            OSSYNCAssert( 0 == ( mskBadRefCount & cwFinal ) );

            switch ( cwInitial )
            {
                case 0:
                    OSSYNCAssert( cwFinal == ( fInitInProgress | 0x1 ) );
                    break;

                case ( fInitInProgress | 0x1 ):
                    OSSYNCAssert( ( cwFinal == 0x1 ) ||
                                ( cwFinal == 0x0 ) );
                    break;

                case 1:
                    OSSYNCAssert( ( cwFinal == 0x2 ) ||
                                ( cwFinal == fTermInProgress ) );
                    break;

                case fTermInProgress:
                    OSSYNCAssert( cwFinal == 0x0 );
                    break;

                default:
                    if ( ( cwInitial & mskRefCount ) < 2 )
                    {
                        OSSYNCAssertSz( fFalse, "Should be impossible for the initial ref count to be 1 or smaller." );
                    }
                    if ( cwInitial & ( fInitInProgress | fTermInProgress ) )
                    {
                        OSSYNCAssertSz( fFalse, "Should be impossible for the initial word to be in init/term here." );
                    }
                    if ( cwFinal & ( fInitInProgress | fTermInProgress ) )
                    {
                        OSSYNCAssertSz( fFalse, "Should be impossible for the final word to be in init/term here." );
                    }

                    //  we have a ref count of 2+, ...
                    OSSYNCAssert( ( cwFinal == ( cwInitial + 1 ) ) ||
                                ( cwFinal == ( cwInitial - 1 ) ) );
                    break;

            }
        }

        //  States (in an imaginary runtime order)
        //      0x00000000  - Library uninitialized.  No initializers have run (OR library terminated).
        //      0x80000001  - Initialization in progress, currently being initialized by consumer.
        //      0x00000001  - One consumer / initial initializer has finished.
        //      0x00000002  - Two consumers have run, ref count = 2.
        //      0x00000001  - One or the other consumer has terminated.
        //      0x40000000  - Termination in progress.
        //      0x00000000  - Library uninitialized.

        //  allows us to sleep away our CPU quanta so another thread can make progress.

        void SleepAwayQuanta();

#define KEEP_SYNC_INITTERM_STATISTICS
#ifdef KEEP_SYNC_INITTERM_STATISTICS
        LONG            m_cActualInits;
        LONG            m_cRefCountInits;
        LONG            m_cCancelledInits;
        LONG            m_cActualTerms;
        LONG            m_cRefCountTerms;

        //  Helper for when we are doing stats ...
        #define KeepStats( code )   code
#else
        #define KeepStats( code )
#endif  //  KEEP_SYNC_INITTERM_STATISTICS

    public:

        CInitTermLock()
        {
            OSSYNCEnforce( m_cwState == 0 );  //  assumed init to 0 by the loader
        }

        ~CInitTermLock()
        {
            // debug tracking
            //wprintf(L"~CInitTermLock ... m_cwStats %d\n", m_cwState );
            //wprintf(L"~CInitTermLock stats: \n\t m_cActualInits = %d\n\t m_cRefCountInits = %d\n\t m_cCancelledInits = %d \n\t"
            //      L" m_cActualTerms = %d\n\t m_cRefCountTerms = %d\n", 
            //      m_cActualInits, m_cRefCountInits, m_cCancelledInits, m_cActualTerms, m_cRefCountTerms );

            OSSYNCEnforce( ( m_cwState == 0 ) || g_fSyncProcessAbort );
            OSSYNCEnforce( ( m_cActualInits == m_cActualTerms ) || g_fSyncProcessAbort );
            OSSYNCEnforce( ( m_cRefCountInits == m_cRefCountTerms ) || g_fSyncProcessAbort );
        }
        
    private:
        //  copy constructors disallowed on a lock ... that'd just be silly.
        CInitTermLock( const CInitTermLock& );
        CInitTermLock& operator=( const CInitTermLock& );

    public:

        //    API Error Codes

        enum class ERR
        {
            errSuccess,     //  initialization is already done, or termination not needed yet.
            errInitBegun,   //  this thread must finish init, and call InitFinish().
            errTermBegun,   //  this thread must finish term, and call TermFinish().
        };

        //  Returns the number of current consumers of the library init/term lock.
        //
        //  Note: Should not use this for any code or test transitions, should use 
        //  the state transition functions.

        LONG CConsumers() const
        {
            //  if either these are true, probably not an accurate count

            OSSYNCAssert( 0 == ( m_cwState & fInitInProgress ) );
            OSSYNCAssert( 0 == ( m_cwState & fTermInProgress ) );

            return mskRefCount & m_cwState;
        }

        //  Begin Initialization ... attempting to begin initialization ends in
        //  one of two cases:
        //  1. We've acquired the right to initialize (and thus implicitly no one
        //     has done it before, or we've termed to zero consumers), and thus
        //     must perform library initialization and call InitFinish( fSuccess ).
        //     CInitTermLock::errInitBegun is returned in this case.
        //  2. The library was already initialized, and we increment the ref count
        //     so the library stays initialized for the caller's usage.
        //     CInitTermLock::errSuccess is returned in this case.
        //  After a library is successfully initialize by the consumer and when the
        //  consumer is done with the library, the consumer must call ErrTermBegin() 
        //  and possibly TermFinish() if the consumer is the last consumer.

        ERR ErrInitBegin()
        {
            OSSYNC_FOREVER
            {
                OSSYNCAssert( 0 == ( mskBadRefCount & m_cwState ) );

                //  grab the current state

                const ControlWord cwBIExpected = m_cwState;

                //  first we expect a pure ref count increment, or 0 to ref count = 
                //  1 + init bit transition, otherwise if we're init or term, we 
                //  need to give up our CPU quanta and wait for whoever is in 
                //  charge of init or term to finish.

                if ( cwBIExpected & ( fInitInProgress | fTermInProgress ) )
                {
                    //  Give up our CPU quanta, and wait for init|term to progress enough, so
                    //  we need to know if we need to re-init, or to inc the ref count

                    SleepAwayQuanta();

                    continue;
                }

                //  we need to increment the ref count and possibly set the 
                //  init in progress bit

                OSSYNCAssert( cwBIExpected == ( cwBIExpected & mskRefCount ) );

                //  the after image starts with the before image, and then we transform it appropriately

                LONG cwAI = cwBIExpected;

                //  increment the ref count of users/consumers of this library

                cwAI++;

                if ( cwBIExpected == 0 )
                {
                    //  we are the very first initer as well, signal init is in progress
    
                    cwAI |= fInitInProgress;
                }

                //  validate the state change

                OSSYNCAssert( ( ( cwAI & mskRefCount ) >= 2 ) || ( cwAI & fInitInProgress ) );
                AssertValidStateTransition_( cwBIExpected, cwAI );

                //  perform the state change

                const ControlWord cwBI = AtomicCompareExchange( (LONG*)&m_cwState, cwBIExpected, cwAI );

                //  check the transition succeeded

                if ( cwBI != cwBIExpected )
                {
                    //  transition failed, conflict updating latch, try again

                    continue;
                }

                //  state transition successful.

                OSSYNCAssert( 0 == ( cwBI & ( fInitInProgress | fTermInProgress ) ) );

                if ( cwAI & fInitInProgress )
                {
                    //  we've grabbed the right to init, tell consumer

                    OSSYNCAssert( 0 == ( mskBadRefCount & m_cwState ) );

                    return ERR::errInitBegun;
                }
                else
                {
                    // we succeeded, we're done.

                    break;
                }
            }

            OSSYNCAssert( 0 == ( mskBadRefCount & m_cwState ) );

            //  update statistics
 
            KeepStats( AtomicIncrement( &m_cRefCountInits ) );

            return ERR::errSuccess;
        }

        //  remove interlock for init-in-progress ... this lets other init'ers 
        //  through, who will presumeably either 
        //  A. increment the ref count if fSuccessfulInit is true
        //  B. re-attempt init (if our attempt failed)
        //

        enum InitFinishStatus { fSuccessfulInit = 1, fFailedInit = 2 };

        void InitFinish( InitFinishStatus fInitStatus )
        {

            //  validate we have a good initial state to complete init

            OSSYNCAssert( m_cwState & fInitInProgress );
            OSSYNCAssert( 0 == ( m_cwState & fTermInProgress ) );

            //  validate we're succeeding or failing

            OSSYNCAssert( fInitStatus == fSuccessfulInit || fInitStatus == fFailedInit );

            //  we expect to be in the init in progress state
            
            const ControlWord cwBIExpected = ( fInitInProgress | 0x1 );

            OSSYNCAssert( m_cwState == cwBIExpected );

            if ( fInitStatus == fSuccessfulInit )
            {
                //  successful init, complete initialization (case A)

                //  update statistics

                KeepStats( AtomicIncrement( &m_cActualInits ) );

                //  perform the state change

                const ControlWord cwBI = AtomicExchangeReset( (ULONG*)&m_cwState, fInitInProgress );

                //  validate the state change

                OSSYNCAssert( cwBI == cwBIExpected );
                AssertValidStateTransition_( cwBI, cwBI & ~fInitInProgress );
                OSSYNCAssert( cwBI & fInitInProgress );

                OSSYNCAssert( 0 == ( m_cwState & fInitInProgress ) );
                OSSYNCAssert( 0 == ( m_cwState & fTermInProgress ) );
            }
            else
            {
                //  failed init, cancel initialization (case B)

                //  update statistics

                KeepStats( AtomicIncrement( &m_cCancelledInits ) );

                //  validate the state change

                AssertValidStateTransition_( cwBIExpected, 0x0 );

                //  perform the state change

                const ControlWord cwBI = AtomicCompareExchange( (LONG*)&m_cwState, cwBIExpected, 0x0 );

                //  validate the state change happened as expected

                OSSYNCAssert( cwBI == cwBIExpected );
            }

            //  ensure we didn't underflow or anything weird

            OSSYNCAssert( 0 == ( mskBadRefCount & m_cwState ) );
        }

        //  Begin Termination ... attempting to begin termination ends in
        //  one of two cases:
        //  1. We've acquired the right to terminate (and thus implicitly we are 
        //     the last consumer to need this library), and thus must perform 
        //     full library termination and call TermFinish( fSuccess).
        //     CInitTermLock::errTermBegun is returned in this case.
        //  2. The library needs to stay initialized, and we only decrement the
        //     ref count so the library stays initialized for other caller's usage.
        //     CInitTermLock::errSuccess is returned in this case.

        ERR ErrTermBegin()
        {
            OSSYNC_FOREVER
            {
                OSSYNCAssert( 0 == ( mskBadRefCount & m_cwState ) );
 
                //  grab the current state

                const ControlWord cwBIExpected = m_cwState;

                //  shouldn't be possible to have init in progress

                OSSYNCAssert( 0 == ( cwBIExpected & fInitInProgress ) );

                //  first we expect a pure ref count decrement, or ref count 1
                //  to 0 + term bit transition, otherwise if we're init or term, 
                //  we need to give up our CPU quanta and wait for whoever is 
                //  in charge of init or term to finish.

                if ( cwBIExpected & ( fInitInProgress | fTermInProgress ) )
                {
                    //  we won't be the person to term, but can't tell the 
                    //  user until someone finishes term

                    OSSYNCAssertSz( fFalse, "Can this happen?  I don't think so, as either we'll be the one to set term in progress or our ref count stops someone else from doing this." );

                    SleepAwayQuanta();

                    continue;
                }

                //  we need to increment the ref count and possibly set the 
                //  term in progress bit

                OSSYNCAssert( cwBIExpected == ( cwBIExpected & mskRefCount ) );

                //  the after image starts with the before image, and then 
                //  we transform it appropriately

                LONG cwAI = cwBIExpected;


                if ( 1 == cwBIExpected )
                {
                    //  we are the last termer, decrement our ref count AND
                    //  set term in progress

                    cwAI = fTermInProgress;
                }
                else
                {
                    //  decrement the ref count of users/consumers of this library

                    cwAI--;
                }

                OSSYNCAssert( ( ( cwAI & mskRefCount ) >= 1 ) || ( cwAI & fTermInProgress ) );


                //  validate the state transition

                AssertValidStateTransition_( cwBIExpected, cwAI );

                //  perform the state transition

                const ControlWord cwBI = AtomicCompareExchange( (LONG*)&m_cwState, cwBIExpected, cwAI );

                //  check the transition succeeded

                if ( cwBI != cwBIExpected )
                {
                    //  transition failed, conflict updating latch, try again

                    continue;
                }

                //  state transition successful.

                if ( cwAI & fTermInProgress )
                {
                    //  we've grabbed the right to term, tell caller

                    OSSYNCAssert( 0 == ( mskBadRefCount & m_cwState ) );

                    return ERR::errTermBegun;
                }
                else
                {
                    // we succeeded, we're done.

                    break;
                }
            }

            OSSYNCAssert( 0 == ( mskBadRefCount & m_cwState ) );

            //  update statistics

            KeepStats( AtomicIncrement( &m_cRefCountTerms ) );

            return ERR::errSuccess;
        }

        //  remove interlock for term-in-progress ... this lets new init'ers 
        //  through.

        void TermFinish()
        {
            //  validate we have a good initial state to complete term

            OSSYNCAssert( m_cwState == fTermInProgress );

            //  update statistics

            KeepStats( AtomicIncrement( &m_cActualTerms ) );

            //  remove interlock for term-in-progress

            const ControlWord cwBI = AtomicExchangeReset( (ULONG*)&m_cwState, fTermInProgress );

            // validate the state transition

            AssertValidStateTransition_( cwBI, 0x0 );
            OSSYNCAssert( cwBI == fTermInProgress );

            //  We can't assert anything about the current m_cwState anymore, as
            //  once we've removed the interlock for term-in-progress, another
            //  thread may have started init right back up again.
        }
};


//  Critical Section (non-nestable) Information

class CCriticalSectionInfo
    :   public CLockBasicInfo,
        public CLockPerfHold,
        public CLockDeadlockDetectionInfo
{
    public:

        //  member functions

        //    ctors / dtors

        CCriticalSectionInfo( const CLockBasicInfo& lbi )
            :   CLockBasicInfo( lbi ),
                CLockDeadlockDetectionInfo( (CLockBasicInfo&) *this )
        {
        }

        //    debugging support

        void Dump( const CDumpContext& dc ) const;
};


//  Critical Section (non-nestable) State

class CCriticalSectionState
{
    public:

        //  member functions

        //    ctors / dtors

        CCriticalSectionState( const CSyncBasicInfo& sbi );
        ~CCriticalSectionState();

        //    accessors

        CSemaphore& Semaphore() { return m_sem; }

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  member functions

        //    operators

        CCriticalSectionState& operator=( CCriticalSectionState& ) = delete;  //  disallowed

        //  data members

        //    semaphore

        CSemaphore      m_sem;
};


//  Critical Section (non-nestable)

class CCriticalSection
    :   private CLockObject,
        private CEnhancedStateContainer< CCriticalSectionState, CSyncBasicInfo, CCriticalSectionInfo, CLockBasicInfo >
{
    public:

        //  member functions

        //    ctors / dtors

        CCriticalSection( const CLockBasicInfo& lbi );
        ~CCriticalSection();

        //    manipulators

        void Enter();
        const BOOL FTryEnter();
        const BOOL FEnter( const INT cmsecTimeout );
        void Leave();

        //    accessors

        const INT CWait() { return State().Semaphore().CWait(); }

#ifdef DEBUG

        const BOOL FAcquired() { return ( State().Semaphore().CAvail() == 0 ); }

#endif  //  DEBUG


#ifdef SYNC_DEADLOCK_DETECTION

        const BOOL FOwner() const       { return State().FOwner(); }
        const BOOL FNotOwner() const    { return State().FNotOwner(); }

#else   //  !SYNC_DEADLOCK_DETECTION

#ifdef DEBUG

        const BOOL FOwner() const       { return fTrue; }
        const BOOL FNotOwner() const    { return fTrue; }

#endif  //  DEBUG

#endif  //  SYNC_DEADLOCK_DETECTION

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  member functions

        //    operators

        CCriticalSection& operator=( CCriticalSection& ) = delete;  //  disallowed
};

//  enter the critical section, waiting forever if someone else is currently the
//  owner.  the critical section can not be re-entered until it has been left

inline void CCriticalSection::Enter()
{
    //  check for deadlock

    State().AssertCanBeWaiter();

    //  acquire the semaphore

    State().AddAsWaiter();

    State().Semaphore().Acquire();

    State().RemoveAsWaiter();

    //  there had better be no available counts on the semaphore

    OSSYNCAssert( !State().Semaphore().CAvail() );

    //  we are now the owner of the critical section

    State().AddAsOwner();
    State().StartHold();
}

//  try to enter the critical section without waiting or spinning, returning
//  fFalse if someone else currently is the owner.  the critical section can not
//  be re-entered until it has been left

inline const BOOL CCriticalSection::FTryEnter()
{
    //  try to acquire the semaphore without waiting or spinning
    //
    //  NOTE:  there is no potential for deadlock here, so don't bother to check

    BOOL fAcquire = State().Semaphore().FTryAcquire();

    //  we are now the owner of the critical section

    if ( fAcquire )
    {
        //  there had better be no available counts on the semaphore

        OSSYNCAssert( !State().Semaphore().CAvail() );

        //  add ourself as the owner

        State().AddAsOwner();
        State().StartHold();
    }

    return fAcquire;
}

//  try to enter the critical section waiting only for the specified interval,
//  returning fFalse if the wait timed out before the critical section could be
//  acquired.  the critical section can not be re-entered until it has been left

inline const BOOL CCriticalSection::FEnter( const INT cmsecTimeout )
{
    //  check for deadlock if we are waiting forever

    if ( cmsecTimeout == cmsecInfinite )
    {
        State().AssertCanBeWaiter();
    }

    //  try to acquire the semaphore, timing out as requested
    //
    //  NOTE:  there is still a potential for deadlock, but that will be detected
    //  at the OS level

    State().AddAsWaiter();

    BOOL fAcquire = State().Semaphore().FAcquire( cmsecTimeout );

    State().RemoveAsWaiter();

    //  we are now the owner of the critical section

    if ( fAcquire )
    {
        //  there had better be no available counts on the semaphore

        OSSYNCAssert( !State().Semaphore().CAvail() );

        //  add ourself as the owner

        State().AddAsOwner();
        State().StartHold();
    }

    return fAcquire;
}

//  leaves the critical section, releasing it for ownership by someone else

inline void CCriticalSection::Leave()
{
    //  remove ourself as the owner

    State().RemoveAsOwner();

    //  we are no longer holding the lock

    State().StopHold();

    //  there had better be no available counts on the semaphore

    OSSYNCAssert( !State().Semaphore().CAvail() );

    //  release the semaphore

    State().Semaphore().Release();
}


//  Nestable Critical Section Information

class CNestableCriticalSectionInfo
    :   public CLockBasicInfo,
        public CLockPerfHold,
        public CLockDeadlockDetectionInfo
{
    public:

        //  member functions

        //    ctors / dtors

        CNestableCriticalSectionInfo( const CLockBasicInfo& lbi )
            :   CLockBasicInfo( lbi ),
                CLockDeadlockDetectionInfo( (CLockBasicInfo&) *this )
        {
        }

        //    debugging support

        void Dump( const CDumpContext& dc ) const;
};


//  Nestable Critical Section State

class CNestableCriticalSectionState
{
    public:

        //  member functions

        //    ctors / dtors

        CNestableCriticalSectionState( const CSyncBasicInfo& sbi );
        ~CNestableCriticalSectionState();

        //    manipulators

        void SetOwner( CLS* const pcls );

        void Enter();
        void Leave();

        //    accessors

        CSemaphore& Semaphore() { return m_sem; }
        CLS* PclsOwner() { return m_pclsOwner; }
        INT CEntry() { return m_cEntry; }

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  member functions

        //    operators

        CNestableCriticalSectionState& operator=( CNestableCriticalSectionState& ) = delete;  //  disallowed

        //  data members

        //    semaphore

        CSemaphore      m_sem;

        //    owning context (protected by the semaphore)

        CLS* volatile   m_pclsOwner;

        //    entry count (only valid when the owner id is valid)

        volatile INT    m_cEntry;
};

//  set the owner

inline void CNestableCriticalSectionState::SetOwner( CLS* const pcls )
{
    //  we had either be clearing the owner or setting a new owner.  we should
    //  never be overwriting another owner

    OSSYNCAssert( !pcls || !m_pclsOwner );

    //  set the new owner

    m_pclsOwner = pcls;
}

//  increment the entry count

inline void CNestableCriticalSectionState::Enter()
{
    //  we had better have an owner already!

    OSSYNCAssert( m_pclsOwner );

    //  we should not overflow the entry count

    OSSYNCAssert( INT( m_cEntry + 1 ) >= 1 );

    //  increment the entry count

    m_cEntry++;
}

//  decrement the entry count

inline void CNestableCriticalSectionState::Leave()
{
    //  we had better have an owner already!

    OSSYNCAssert( m_pclsOwner );

    //  decrement the entry count

    m_cEntry--;
}


//  Nestable Critical Section

class CNestableCriticalSection
    :   private CLockObject,
        private CEnhancedStateContainer< CNestableCriticalSectionState, CSyncBasicInfo, CNestableCriticalSectionInfo, CLockBasicInfo >
{
    public:

        //  member functions

        //    ctors / dtors

        CNestableCriticalSection( const CLockBasicInfo& lbi );
        ~CNestableCriticalSection();

        //    manipulators

        void Enter();
        const BOOL FTryEnter();
        const BOOL FEnter( const INT cmsecTimeout );
        void Leave();

        //    accessors

        const INT CWait() { return State().Semaphore().CWait(); }

#ifdef SYNC_DEADLOCK_DETECTION

        const BOOL FOwner() { return State().FOwner(); }
        const BOOL FNotOwner() { return State().FNotOwner(); }

#else   //  !SYNC_DEADLOCK_DETECTION

#ifdef DEBUG

        const BOOL FOwner() const       { return fTrue; }
        const BOOL FNotOwner() const    { return fTrue; }

#endif  //  DEBUG

#endif  //  SYNC_DEADLOCK_DETECTION

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  member functions

        //    operators

        CNestableCriticalSection& operator=( CNestableCriticalSection& ) = delete;  //  disallowed
};

//  enter the critical section, waiting forever if someone else is currently the
//  owner.  the critical section can be reentered without waiting or deadlocking

inline void CNestableCriticalSection::Enter()
{
    //  get our context

    CLS* const pcls = Pcls();

    //  we own the critical section

    if ( State().PclsOwner() == pcls )
    {
        //  there had better be no available counts on the semaphore

        OSSYNCAssert( !State().Semaphore().CAvail() );

        //  we should have at least one entry count

        OSSYNCAssert( State().CEntry() >= 1 );

        //  increment our entry count

        State().Enter();
    }

    //  we do not own the critical section

    else
    {
        OSSYNCAssert( State().PclsOwner() != pcls );

        //  check for deadlock

        State().AssertCanBeWaiter();

        //  acquire the semaphore

        State().AddAsWaiter();

        State().Semaphore().Acquire();

        State().RemoveAsWaiter();

        //  there had better be no available counts on the semaphore

        OSSYNCAssert( !State().Semaphore().CAvail() );

        //  we are now the owner of the critical section

        State().AddAsOwner();
        State().StartHold();

        //  save our context as the owner

        State().SetOwner( pcls );

        //  set initial entry count

        State().Enter();
    }
}

//  try to enter the critical section without waiting or spinning, returning
//  fFalse if someone else currently is the owner.  the critical section can be
//  reentered without waiting or deadlocking

inline const BOOL CNestableCriticalSection::FTryEnter()
{
    //  get our context

    CLS* const pcls = Pcls();

    //  we own the critical section

    if ( State().PclsOwner() == pcls )
    {
        //  there had better be no available counts on the semaphore

        OSSYNCAssert( !State().Semaphore().CAvail() );

        //  we should have at least one entry count

        OSSYNCAssert( State().CEntry() >= 1 );

        //  increment our entry count

        State().Enter();

        //  return success

        return fTrue;
    }

    //  we do not own the critical section

    else
    {
        OSSYNCAssert( State().PclsOwner() != pcls );

        //  try to acquire the semaphore without waiting or spinning
        //
        //  NOTE:  there is no potential for deadlock here, so don't bother to check

        const BOOL fAcquired = State().Semaphore().FTryAcquire();

        //  we now own the critical section

        if ( fAcquired )
        {
            //  there had better be no available counts on the semaphore

            OSSYNCAssert( !State().Semaphore().CAvail() );

            //  add ourself as the owner

            State().AddAsOwner();
            State().StartHold();

            //  save our context as the owner

            State().SetOwner( pcls );

            //  set initial entry count

            State().Enter();
        }

        //  return result

        return fAcquired;
    }
}

//  try to enter the critical section waiting only for the specified interval,
//  returning fFalse if the wait timed out before the critical section could be
//  acquired.  the critical section can be reentered without waiting or
//  deadlocking

inline const BOOL CNestableCriticalSection::FEnter( const INT cmsecTimeout )
{
    //  get our context

    CLS* const pcls = Pcls();

    //  we own the critical section

    if ( State().PclsOwner() == pcls )
    {
        //  there had better be no available counts on the semaphore

        OSSYNCAssert( !State().Semaphore().CAvail() );

        //  we should have at least one entry count

        OSSYNCAssert( State().CEntry() >= 1 );

        //  increment our entry count

        State().Enter();

        //  return success

        return fTrue;
    }

    //  we do not own the critical section

    else
    {
        OSSYNCAssert( State().PclsOwner() != pcls );

        //  check for deadlock if we are waiting forever

        if ( cmsecTimeout == cmsecInfinite )
        {
            State().AssertCanBeWaiter();
        }

        //  try to acquire the semaphore, timing out as requested
        //
        //  NOTE:  there is still a potential for deadlock, but that will be detected
        //  at the OS level

        State().AddAsWaiter();

        const BOOL fAcquired = State().Semaphore().FAcquire( cmsecTimeout );

        State().RemoveAsWaiter();

        //  we now own the critical section

        if ( fAcquired )
        {
            //  there had better be no available counts on the semaphore

            OSSYNCAssert( !State().Semaphore().CAvail() );

            //  add ourself as the owner

            State().AddAsOwner();
            State().StartHold();

            //  save our context as the owner

            State().SetOwner( pcls );

            //  set initial entry count

            State().Enter();
        }

        //  return result

        return fAcquired;
    }
}

//  leave the critical section.  if leave has been called for every enter that
//  has completed successfully, the critical section is released for ownership
//  by someone else

inline void CNestableCriticalSection::Leave()
{
    //  we had better be the current owner

    OSSYNCAssert( State().PclsOwner() == Pcls() );

    //  there had better be no available counts on the semaphore

    OSSYNCAssert( !State().Semaphore().CAvail() );

    //  there had better be at least one entry count

    OSSYNCAssert( State().CEntry() >= 1 );

    //  release one entry count

    State().Leave();

    //  we released the last entry count

    if ( !State().CEntry() )
    {
        //  reset the owner id

        State().SetOwner( 0 );

        //  remove ourself as the owner

        State().RemoveAsOwner();

        //  we are no longer holding the lock

        State().StopHold();

        //  release the semaphore

        State().Semaphore().Release();
    }
}


//  Gate Information

class CGateInfo
    :   public CSyncBasicInfo,
        public CSyncPerfWait
{
    public:

        //  member functions

        //    ctors / dtors

        CGateInfo( const CSyncBasicInfo& sbi )
            :   CSyncBasicInfo( sbi )
        {
        }

        //    debugging support

        void Dump( const CDumpContext& dc ) const;
};


//  Gate State

class CGateState
{
    public:

        //  member functions

        //    ctors / dtors

        CGateState( const CSyncStateInitNull& null ) : m_cWait( 0 ), m_irksem( CKernelSemaphorePool::irksemNil ) {}
        CGateState( const INT cWait, const INT irksem );
        ~CGateState() {}

        //    manipulators

        void SetWaitCount( const INT cWait );
        void SetIrksem( const CKernelSemaphorePool::IRKSEM irksem );

        //    accessors

        const INT CWait() const { return m_cWait; }
        const CKernelSemaphorePool::IRKSEM Irksem() const { return CKernelSemaphorePool::IRKSEM( m_irksem ); }

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  member functions

        //    operators

        CGateState& operator=( CGateState& ) = delete;  //  disallowed

        //  data members

        //    waiter count

        volatile SHORT  m_cWait;            //  0 <= m_cWait <= ( 1 << 15 ) - 1

        //    reference kernel semaphore

        volatile USHORT m_irksem;   //  0 <= m_irksem <= ( 1 << 16 ) - 2
};

//  sets the wait count for the gate

inline void CGateState::SetWaitCount( const INT cWait )
{
    //  it must be a valid wait count

    OSSYNCAssert( cWait >= 0 );
    OSSYNCAssert( cWait <= 0x7FFF );

    //  set the wait count

    m_cWait = (USHORT) cWait;
}

//  sets the referenced kernel semaphore for the gate

inline void CGateState::SetIrksem( const CKernelSemaphorePool::IRKSEM irksem )
{
    //  it must be a valid irksem

    OSSYNCAssert( irksem >= 0 );
    OSSYNCAssert( irksem <= 0xFFFF );

    //  set the irksem

    m_irksem = (USHORT) irksem;
}


//  Gate

class CGate
    :   private CSyncObject,
        private CEnhancedStateContainer< CGateState, CSyncStateInitNull, CGateInfo, CSyncBasicInfo >
{
    public:

        //  member functions

        //    ctors / dtors

        CGate( const CSyncBasicInfo& sbi );
        ~CGate();

        //    manipulators

        void Wait( CCriticalSection& crit );
        void Release( CCriticalSection& crit, const INT cToRelease );
        void ReleaseAll( CCriticalSection& crit );

        //    accessors

        const INT CWait() const { return State().CWait(); }

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  member functions

        void ReleaseAndHold( CCriticalSection& crit, const INT cToRelease ); // never been used.

        //    operators

        CGate& operator=( CGate& ) = delete;  //  disallowed
};


//  Null Lock Object State Initializer

class CLockStateInitNull
{
};

extern const CLockStateInitNull lockstateNull;


//  Binary Lock Performance Information

class CBinaryLockPerfInfo
    :   public CSyncPerfWait,
        public CSyncPerfAcquire,
        public CLockPerfHold
{
    public:

        //  member functions

        //    ctors / dtors

        CBinaryLockPerfInfo()
        {
        }

        //    debugging support

        void Dump( const CDumpContext& dc ) const;
};


//  Binary Lock Group Information

class CBinaryLockGroupInfo
{
    public:

        //  member functions

        //    ctors / dtors

        CBinaryLockGroupInfo() {}
        ~CBinaryLockGroupInfo() {}

        //    manipulators

        void StartWait( const INT iGroup ) { m_rginfo[iGroup].StartWait(); }
        void StopWait( const INT iGroup ) { m_rginfo[iGroup].StopWait(); }

        void SetAcquire( const INT iGroup ) { m_rginfo[iGroup].SetAcquire(); }
        void SetContend( const INT iGroup ) { m_rginfo[iGroup].SetContend(); }

        void StartHold( const INT iGroup ) { m_rginfo[iGroup].StartHold(); }
        void StopHold( const INT iGroup ) { m_rginfo[iGroup].StopHold(); }

        //    accessors

#ifdef SYNC_ANALYZE_PERFORMANCE

        QWORD   CWaitTotal( const INT iGroup ) const        { return m_rginfo[iGroup].CWaitTotal(); }
        double  CsecWaitElapsed( const INT iGroup ) const   { return m_rginfo[iGroup].CsecWaitElapsed(); }

        QWORD   CAcquireTotal( const INT iGroup ) const     { return m_rginfo[iGroup].CAcquireTotal(); }
        QWORD   CContendTotal( const INT iGroup ) const     { return m_rginfo[iGroup].CContendTotal(); }

        QWORD   CHoldTotal( const INT iGroup ) const        { return m_rginfo[iGroup].CHoldTotal(); }
        double  CsecHoldElapsed( const INT iGroup ) const   { return m_rginfo[iGroup].CsecHoldElapsed(); }

#endif  //  SYNC_ANALYZE_PERFORMANCE

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  member functions

        //    operators

        CBinaryLockGroupInfo& operator=( CBinaryLockGroupInfo& ) = delete;  //  disallowed

        //  data members

        //    performance info for each group

        CBinaryLockPerfInfo m_rginfo[2];
};


//  Binary Lock Information

class CBinaryLockInfo
    :   public CLockBasicInfo,
        public CBinaryLockGroupInfo,
        public CLockDeadlockDetectionInfo
{
    public:

        //  member functions

        //    ctors / dtors

        CBinaryLockInfo( const CLockBasicInfo& lbi )
            :   CLockBasicInfo( lbi ),
                CLockDeadlockDetectionInfo( (CLockBasicInfo&) *this )
        {
        }

        //    debugging support

        void Dump( const CDumpContext& dc ) const;
};


//  Binary Lock State

class CBinaryLockState
{
    public:

        //  types

        //    control word

        typedef DWORD ControlWord;

        //  member functions

        //    ctors / dtors

        CBinaryLockState( const CSyncBasicInfo& sbi );
        ~CBinaryLockState();

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

        //  data members

        //    control word

        union
        {
            volatile ControlWord    m_cw;

            struct
            {
                volatile DWORD          m_cOOW1:15;
                volatile DWORD          m_fQ1:1;
                volatile DWORD          m_cOOW2:15;
                volatile DWORD          m_fQ2:1;
            };
        };

        //    quiesced owner count

        volatile DWORD          m_cOwner;

        //    sempahore used by Group 1 to wait for lock ownership

        CSemaphore              m_sem1;

        //    sempahore used by Group 2 to wait for lock ownership

        CSemaphore              m_sem2;

    private:

        //  member functions

        //    operators

        CBinaryLockState& operator=( CBinaryLockState& ) = delete;  //  disallowed
};


//  Binary Lock

class CBinaryLock
    :   private CLockObject,
        private CEnhancedStateContainer< CBinaryLockState, CSyncBasicInfo, CBinaryLockInfo, CLockBasicInfo >
{
    public:

        //  types

        //    control word

        typedef CBinaryLockState::ControlWord ControlWord;

        //    transition reasons for state machine

        enum TransitionReason
        {
            trIllegal = 0,
            trEnter1 = 1,
            trLeave1 = 2,
            trEnter2 = 4,
            trLeave2 = 8,
        };

        //  member functions

        //    ctors / dtors

        CBinaryLock( const CLockBasicInfo& lbi );
        ~CBinaryLock();

        //    manipulators

        void Enter1();
        const BOOL FTryEnter1();
        void Leave1();

        void Enter2();
        const BOOL FTryEnter2();
        void Leave2();

        void ClaimOwnership( const DWORD group );
        void ReleaseOwnership( const DWORD group );

        //    accessors

        const BOOL FGroup1Quiesced()    { return State().m_cw & 0x00008000; }
        const BOOL FGroup2Quiesced()    { return State().m_cw & 0x80000000; }

#ifdef SYNC_DEADLOCK_DETECTION

        const BOOL FMemberOfGroup1()    { return State().FOwner( 0 ); }
        const BOOL FNotMemberOfGroup1() { return State().FNotOwner( 0 ); }
        const BOOL FMemberOfGroup2()    { return State().FOwner( 1 ); }
        const BOOL FNotMemberOfGroup2() { return State().FNotOwner( 1 ); }

#else   //  !SYNC_DEADLOCK_DETECTION

#ifdef DEBUG

        const BOOL FMemberOfGroup1()    { return fTrue; }
        const BOOL FNotMemberOfGroup1() { return fTrue; }
        const BOOL FMemberOfGroup2()    { return fTrue; }
        const BOOL FNotMemberOfGroup2() { return fTrue; }

#endif  //  DEBUG

#endif  //  SYNC_DEADLOCK_DETECTION

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  member functions

        //    operators

        CBinaryLock& operator=( CBinaryLock& ) = delete;  //  disallowed

        //    verification

        INT _StateFromControlWord( const ControlWord cw );
        BOOL _FValidStateTransition(    const ControlWord cwBI,
                                        const ControlWord cwAI,
                                        const TransitionReason tr );

        //    manipulators

        void _Enter1( const ControlWord cwBIOld );
        void _Enter2( const ControlWord cwBIOld );

        void _UpdateQuiescedOwnerCountAsGroup1( const DWORD cOwnerDelta );
        void _UpdateQuiescedOwnerCountAsGroup2( const DWORD cOwnerDelta );
};

//  enters the binary lock as a member of Group 1, waiting forever if necessary
//
//  NOTE:  trying to enter the lock as a member of Group 1 when you already own
//  the lock as a member of Group 2 will cause a deadlock.


inline void CBinaryLock::Enter1()
{
    //  we had better not already own this lock as either group

    OSSYNCAssert( State().FNotOwner( 0 ) );
    OSSYNCAssert( State().FNotOwner( 1 ) );

    //  check for deadlock

    State().AssertCanBeWaiter();

    //  try forever until we successfully change the lock state

    OSSYNC_FOREVER
    {
        //  read the current state of the control word as our expected before image

        const ControlWord cwBIExpected = State().m_cw;

        //  compute the after image of the control word by performing the global
        //  transform for the Enter1 state transition

        const ControlWord cwAI =    ControlWord( ( ( cwBIExpected & ( ( LONG_PTR( LONG( cwBIExpected ) ) >> 31 ) |
                                    0x0000FFFF ) ) | 0x80000000 ) + 0x00000001 );

        //  validate the transaction

        OSSYNCAssert( _FValidStateTransition( cwBIExpected, cwAI, trEnter1 ) );

        //  attempt to perform the transacted state transition on the control word

        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

        //  the transaction failed or Group 1 was quiesced from ownership

        if ( ( cwBI ^ cwBIExpected ) | ( cwBI & 0x00008000 ) )
        {
            //  the transaction failed because another context changed the control word

            if ( cwBI != cwBIExpected )
            {
                //  try again

                continue;
            }

            //  the transaction succeeded but Group 1 was quiesced from ownership

            else
            {
                //  this is a contention for Group 1

                State().SetContend( 0 );

                //  wait to own the lock as a member of Group 1

                _Enter1( cwBI );

                //  we now own the lock, so we're done

                break;
            }
        }

        //  the transaction succeeded and Group 1 was not quiesced from ownership

        else
        {
            //  we now own the lock, so we're done

            break;
        }
    }

    //  we are now an owner of the lock for Group 1

    State().SetAcquire( 0 );
    State().AddAsOwner( 0 );
    State().StartHold( 0 );
}


//  tries to enter the binary lock as a member of Group 1 without waiting or
//  spinning, returning fFalse if Group 1 is quiesced from ownership
//
//  NOTE:  trying to enter the lock as a member of Group 1 when you already own
//  the lock as a member of Group 2 will cause a deadlock.


inline const BOOL CBinaryLock::FTryEnter1()
{
    //  we had better not already own this lock as group 1

    OSSYNCAssert( State().FNotOwner( 0 ) );

    //  try forever until we successfully change the lock state

    OSSYNC_FOREVER
    {
        //  read the current state of the control word as our expected before image

        ControlWord cwBIExpected = State().m_cw;

        //  change the expected before image so that the transaction will only work if
        //  Group 1 ownership is not quiesced

        cwBIExpected = cwBIExpected & 0xFFFF7FFF;

        //  compute the after image of the control word by performing the global
        //  transform for the Enter1 state transition

        const ControlWord cwAI =    ControlWord( ( ( cwBIExpected & ( ( LONG_PTR( LONG( cwBIExpected ) ) >> 31 ) |
                                    0x0000FFFF ) ) | 0x80000000 ) + 0x00000001 );

        //  validate the transaction

        OSSYNCAssert(   _StateFromControlWord( cwBIExpected ) < 0 ||
                _FValidStateTransition( cwBIExpected, cwAI, trEnter1 ) );

        //  attempt to perform the transacted state transition on the control word

        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

        //  the transaction failed

        if ( cwBI != cwBIExpected )
        {
            //  the transaction failed because Group 1 ownership is quiesced

            if ( cwBI & 0x00008000 )
            {
                //  return failure

                return fFalse;
            }

            //  the transaction failed because another context changed the control word

            else
            {
                //  try again

                continue;
            }
        }

        //  the transaction succeeded

        else
        {
            //  we are now an owner of the lock for Group 1

            State().SetAcquire( 0 );
            State().AddAsOwner( 0 );
            State().StartHold( 0 );

            //  return success

            return fTrue;
        }
    }
}


//  leaves the binary lock as a member of Group 1
//
//  NOTE:  you must leave the lock as a member of the same Group for which you entered
//  the lock or deadlocks may occur

inline void CBinaryLock::Leave1()
{
    //  we are no longer an owner of the lock

    State().RemoveAsOwner( 0 );

    //  we are no longer holding the lock

    State().StopHold( 0 );

    //  try forever until we successfully change the lock state

    OSSYNC_FOREVER
    {
        //  read the current state of the control word as our expected before image

        ControlWord cwBIExpected = State().m_cw;

        //  change the expected before image so that the transaction will only work if
        //  Group 1 ownership is not quiesced

        cwBIExpected = cwBIExpected & 0xFFFF7FFF;

        //  compute the after image of the control word by performing the transform that
        //  will take us either from state 2 to state 0 or state 2 to state 2

        ControlWord cwAI =  cwBIExpected + 0xFFFFFFFF;
        cwAI = cwAI - ( ( ( cwAI + 0xFFFFFFFF ) << 16 ) & 0x80000000 );

        //  validate the transaction

        OSSYNCAssert(   _StateFromControlWord( cwBIExpected ) < 0 ||
                _FValidStateTransition( cwBIExpected, cwAI, trLeave1 ) );

        //  attempt to perform the transacted state transition on the control word

        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

        //  the transaction failed

        if ( cwBI != cwBIExpected )
        {
            //  the transaction failed because Group 1 ownership is quiesced

            if ( cwBI & 0x00008000 )
            {
                //  leave the lock as a quiesced owner

                _UpdateQuiescedOwnerCountAsGroup1( 0xFFFFFFFF );

                //  we're done

                break;
            }

            //  the transaction failed because another context changed the control word

            else
            {
                //  try again

                continue;
            }
        }

        //  the transaction succeeded

        else
        {
            // we're done

            break;
        }
    }
}

//  enters the binary lock as a member of Group 2, waiting forever if necessary
//
//  NOTE:  trying to enter the lock as a member of Group 2 when you already own
//  the lock as a member of Group 1 will cause a deadlock.


inline void CBinaryLock::Enter2()
{
    //  we had better not already own this lock as either group

    OSSYNCAssert( State().FNotOwner( 0 ) );
    OSSYNCAssert( State().FNotOwner( 1 ) );

    //  check for deadlock

    State().AssertCanBeWaiter();

    //  try forever until we successfully change the lock state

    OSSYNC_FOREVER
    {
        //  read the current state of the control word as our expected before image

        const ControlWord cwBIExpected = State().m_cw;

        //  compute the after image of the control word by performing the global
        //  transform for the Enter2 state transition

        const ControlWord cwAI =    ControlWord( ( ( cwBIExpected & ( ( LONG( cwBIExpected << 16 ) >> 31 ) |
                                    0xFFFF0000 ) ) | 0x00008000 ) + 0x00010000 );

        //  validate the transaction

        OSSYNCAssert( _FValidStateTransition( cwBIExpected, cwAI, trEnter2 ) );

        //  attempt to perform the transacted state transition on the control word

        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

        //  the transaction failed or Group 2 was quiesced from ownership

        if ( ( cwBI ^ cwBIExpected ) | ( cwBI & 0x80000000 ) )
        {
            //  the transaction failed because another context changed the control word

            if ( cwBI != cwBIExpected )
            {
                //  try again

                continue;
            }

            //  the transaction succeeded but Group 2 was quiesced from ownership

            else
            {
                //  this is a contention for Group 2

                State().SetContend( 1 );

                //  wait to own the lock as a member of Group 2

                _Enter2( cwBI );

                //  we now own the lock, so we're done

                break;
            }
        }

        //  the transaction succeeded and Group 2 was not quiesced from ownership

        else
        {
            //  we now own the lock, so we're done

            break;
        }
    }

    //  we are now an owner of the lock for Group 2

    State().SetAcquire( 1 );
    State().AddAsOwner( 1 );
    State().StartHold( 1 );
}


//  tries to enter the binary lock as a member of Group 2 without waiting or
//  spinning, returning fFalse if Group 2 is quiesced from ownership
//
//  NOTE:  trying to enter the lock as a member of Group 2 when you already own
//  the lock as a member of Group 1 will cause a deadlock.

inline const BOOL CBinaryLock::FTryEnter2()
{
    //  we had better not already own this lock as group 2

    OSSYNCAssert( State().FNotOwner( 1 ) );

    //  try forever until we successfully change the lock state

    OSSYNC_FOREVER
    {
        //  read the current state of the control word as our expected before image

        ControlWord cwBIExpected = State().m_cw;

        //  change the expected before image so that the transaction will only work if
        //  Group 2 ownership is not quiesced

        cwBIExpected = cwBIExpected & 0x7FFFFFFF;

        //  compute the after image of the control word by performing the global
        //  transform for the Enter2 state transition

        const ControlWord cwAI =    ControlWord( ( ( cwBIExpected & ( ( LONG( cwBIExpected << 16 ) >> 31 ) |
                                    0xFFFF0000 ) ) | 0x00008000 ) + 0x00010000 );

        //  validate the transaction

        OSSYNCAssert(   _StateFromControlWord( cwBIExpected ) < 0 ||
                _FValidStateTransition( cwBIExpected, cwAI, trEnter2 ) );

        //  attempt to perform the transacted state transition on the control word

        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

        //  the transaction failed

        if ( cwBI != cwBIExpected )
        {
            //  the transaction failed because Group 2 ownership is quiesced

            if ( cwBI & 0x80000000 )
            {
                //  return failure

                return fFalse;
            }

            //  the transaction failed because another context changed the control word

            else
            {
                //  try again

                continue;
            }
        }

        //  the transaction succeeded

        else
        {
            //  we are now an owner of the lock for Group 2

            State().SetAcquire( 1 );
            State().AddAsOwner( 1 );
            State().StartHold( 1 );

            //  return success

            return fTrue;
        }
    }
}


//  leaves the binary lock as a member of Group 2
//
//  NOTE:  you must leave the lock as a member of the same Group for which you entered
//  the lock or deadlocks may occur

inline void CBinaryLock::Leave2()
{
    //  we are no longer an owner of the lock

    State().RemoveAsOwner( 1 );

    //  we are no longer holding the lock

    State().StopHold( 1 );

    //  try forever until we successfully change the lock state

    OSSYNC_FOREVER
    {
        //  read the current state of the control word as our expected before image

        ControlWord cwBIExpected = State().m_cw;

        //  change the expected before image so that the transaction will only work if
        //  Group 2 ownership is not quiesced

        cwBIExpected = cwBIExpected & 0x7FFFFFFF;

        //  compute the after image of the control word by performing the transform that
        //  will take us either from state 1 to state 0 or state 1 to state 1

        ControlWord cwAI =  cwBIExpected + 0xFFFF0000;
        cwAI = cwAI - ( ( ( cwAI + 0xFFFF0000 ) >> 16 ) & 0x00008000 );

        //  validate the transaction

        OSSYNCAssert(   _StateFromControlWord( cwBIExpected ) < 0 ||
                _FValidStateTransition( cwBIExpected, cwAI, trLeave2 ) );

        //  attempt to perform the transacted state transition on the control word

        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

        //  the transaction failed

        if ( cwBI != cwBIExpected )
        {
            //  the transaction failed because Group 2 ownership is quiesced

            if ( cwBI & 0x80000000 )
            {
                //  leave the lock as a quiesced owner

                _UpdateQuiescedOwnerCountAsGroup2( 0xFFFFFFFF );

                //  we're done

                break;
            }

            //  the transaction failed because another context changed the control word

            else
            {
                //  try again

                continue;
            }
        }

        //  the transaction succeeded

        else
        {
            // we're done

            break;
        }
    }
}

//  claims ownership of the lock for the specified group for deadlock detection

inline void CBinaryLock::ClaimOwnership( const DWORD group )
{
#ifndef DEBUG
#ifdef DEBUGGER_EXTENSION
#ifndef RTM
    OSSYNCAssertSzRTL( ( group == 1 ) || ( group == 2 ), "Unknown group!" );
#endif // !RTM
#endif // DEBUGGER_EXTENSION
#endif // !DEBUG
    State().AddAsOwner( group - 1 );
}

//  releases ownership of the lock for the specified group for deadlock detection

inline void CBinaryLock::ReleaseOwnership( const DWORD group )
{
#ifndef DEBUG
#ifdef DEBUGGER_EXTENSION
#ifndef RTM
    OSSYNCAssertSzRTL( ( group == 1 ) || ( group == 2 ), "Unknown group!" );
#endif // !RTM
#endif // DEBUGGER_EXTENSION
#endif // !DEBUG
    State().RemoveAsOwner( group - 1 );
}

//  Reader / Writer Lock Performance Information

class CReaderWriterLockPerfInfo
    :   public CSyncPerfWait,
        public CSyncPerfAcquire,
        public CLockPerfHold
{
    public:

        //  member functions

        //    ctors / dtors

        CReaderWriterLockPerfInfo()
        {
        }

        //    debugging support

        void Dump( const CDumpContext& dc ) const;
};


//  Reader / Writer Lock Group Information

class CReaderWriterLockGroupInfo
{
    public:

        //  member functions

        //    ctors / dtors

        CReaderWriterLockGroupInfo() {}
        ~CReaderWriterLockGroupInfo() {}

        //    manipulators

        void StartWait( const INT iGroup ) { m_rginfo[iGroup].StartWait(); }
        void StopWait( const INT iGroup ) { m_rginfo[iGroup].StopWait(); }

        void SetAcquire( const INT iGroup ) { m_rginfo[iGroup].SetAcquire(); }
        void SetContend( const INT iGroup ) { m_rginfo[iGroup].SetContend(); }

        void StartHold( const INT iGroup ) { m_rginfo[iGroup].StartHold(); }
        void StopHold( const INT iGroup ) { m_rginfo[iGroup].StopHold(); }

        //    accessors

#ifdef SYNC_ANALYZE_PERFORMANCE

        QWORD   CWaitTotal( const INT iGroup ) const        { return m_rginfo[iGroup].CWaitTotal(); }
        double  CsecWaitElapsed( const INT iGroup ) const   { return m_rginfo[iGroup].CsecWaitElapsed(); }

        QWORD   CAcquireTotal( const INT iGroup ) const     { return m_rginfo[iGroup].CAcquireTotal(); }
        QWORD   CContendTotal( const INT iGroup ) const     { return m_rginfo[iGroup].CContendTotal(); }

        QWORD   CHoldTotal( const INT iGroup ) const        { return m_rginfo[iGroup].CHoldTotal(); }
        double  CsecHoldElapsed( const INT iGroup ) const   { return m_rginfo[iGroup].CsecHoldElapsed(); }

#endif  //  SYNC_ANALYZE_PERFORMANCE

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  member functions

        //    operators

        CReaderWriterLockGroupInfo& operator=( CReaderWriterLockGroupInfo& ) = delete;  //  disallowed

        //  data members

        //    performance info for each group

        CReaderWriterLockPerfInfo m_rginfo[2];
};


//  Reader / Writer Lock Information

class CReaderWriterLockInfo
    :   public CLockBasicInfo,
        public CReaderWriterLockGroupInfo,
        public CLockDeadlockDetectionInfo
{
    public:

        //  member functions

        //    ctors / dtors

        CReaderWriterLockInfo( const CLockBasicInfo& lbi )
            :   CLockBasicInfo( lbi ),
                CLockDeadlockDetectionInfo( (CLockBasicInfo&) *this )
        {
        }

        //    debugging support

        void Dump( const CDumpContext& dc ) const;
};

//
// Self-Atomizing Reader / Writer Lock
//
// Data Structure:
//
//  31      30-16       15      14-0    
//  fQR     cOOWR       fQW     cOAOWW      Control Word
//
// fQR:     Quiesce Reader ownership
// cOOWR:   Count of owning or waiting Readers
// fQW:     Quiesce Writer ownership
// cOAOWW:  Count of owning and/or waiting Writers
// cOwner:      Count of quiesced owners
// semWriter:   Semaphore used by Writers to wait for lock ownership
// semReader:   Semaphore used by Readers to wait for lock ownership
//
// Legal States:
//
// State    Control Word    Description
// 0        0, 0, 0, 0      No owners; No waiters
// 1        0, n, 1, 0  n   Readers; No waiters
// 2        1, 0, 0, m  1 Writer; m - 1 Writers waiting
// 3        1, 0, 1, m  ? Readers; m Writers waiting
// 4        1, n, 1, 0      1 Writer; n Readers waiting
// 5        1, n, 1, m  ? ?; n Readers waiting and m Writers waiting
//
//   Note: A bit confusing but readers are group 1, writers are group 0.

//  Reader / Writer Lock State

class CReaderWriterLockState
{
    public:

        //  types

        //    control word

        typedef DWORD ControlWord;

        //  member functions

        //    ctors / dtors

        CReaderWriterLockState( const CSyncBasicInfo& sbi );
        ~CReaderWriterLockState();

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

        //  data members

        //    control word

        union
        {
            volatile ControlWord    m_cw;

            struct
            {
                volatile DWORD          m_cOAOWW:15;
                volatile DWORD          m_fQW:1;
                volatile DWORD          m_cOOWR:15;
                volatile DWORD          m_fQR:1;
            };
        };

        //    quiesced owner count

        volatile DWORD          m_cOwner;

        //    sempahore used by writers to wait for lock ownership

        CSemaphore              m_semWriter;

        //    sempahore used by readers to wait for lock ownership

        CSemaphore              m_semReader;

    private:

        //  member functions

        //    operators

        CReaderWriterLockState& operator=( CReaderWriterLockState& ) = delete;  //  disallowed
};


//  Reader / Writer Lock

class CReaderWriterLock
    :   private CLockObject,
        private CEnhancedStateContainer< CReaderWriterLockState, CSyncBasicInfo, CReaderWriterLockInfo, CLockBasicInfo >
{
    public:

        //  types

        //    control word

        typedef CReaderWriterLockState::ControlWord ControlWord;

        //    transition reasons for state machine

        enum TransitionReason
        {
            trIllegal = 0,
            trEnterAsWriter = 1,
            trLeaveAsWriter = 2,
            trEnterAsReader = 4,
            trLeaveAsReader = 8,
        };

        //  member functions

        //    ctors / dtors

        CReaderWriterLock( const CLockBasicInfo& lbi );
        ~CReaderWriterLock();

        //    manipulators

        void EnterAsWriter();
        const BOOL FTryEnterAsWriter();
        void LeaveAsWriter();

        void EnterAsReader();
        const BOOL FTryEnterAsReader();
        void LeaveAsReader();

        //    accessors

        const BOOL FWritersQuiesced() const   { return State().m_cw & 0x00008000; }
        const BOOL FReadersQuiesced() const   { return State().m_cw & 0x80000000; }

#ifdef SYNC_DEADLOCK_DETECTION

        const BOOL FWriter() const      { return State().FOwner( 0 ); }
        const BOOL FNotWriter() const   { return State().FNotOwner( 0 ); }
        const BOOL FReader() const      { return State().FOwner( 1 ); }
        const BOOL FNotReader() const   { return State().FNotOwner( 1 ); }

#else   //  !SYNC_DEADLOCK_DETECTION

#ifdef DEBUG

        const BOOL FWriter() const      { return fTrue; }
        const BOOL FNotWriter() const   { return fTrue; }
        const BOOL FReader() const      { return fTrue; }
        const BOOL FNotReader() const   { return fTrue; }

#endif  //  DEBUG

#endif  //  SYNC_DEADLOCK_DETECTION

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  member functions

        //    operators

        CReaderWriterLock& operator=( CReaderWriterLock& ) = delete;  //  disallowed

        //    verification

        INT _StateFromControlWord( const ControlWord cw );
        BOOL _FValidStateTransition(    const ControlWord cwBI,
                                        const ControlWord cwAI,
                                        const TransitionReason tr );

        //    manipulators

        void _EnterAsWriter( const ControlWord cwBIOld );
        void _EnterAsReader( const ControlWord cwBIOld );

        void _UpdateQuiescedOwnerCountAsWriter( const DWORD cOwnerDelta );
        void _UpdateQuiescedOwnerCountAsReader( const DWORD cOwnerDelta );
};

//  enters the reader / writer lock as a writer, waiting forever if necessary
//
//  NOTE:  trying to enter the lock as a writer when you already own the lock
//  as a reader will cause a deadlock.
//
// State transitions for entering as a Writer:
//
// 0   2:   { 0, 0, 0, 0 }   { 1, 0, 0, 1 }
// 1   3:   { 0, n, 1, 0 }   { 1, 0, 1, 1 };  cOwner += n
// 2   2:   { 1, 0, 0, m }   { 1, 0, 0, m + 1 }
// 3   3:   { 1, 0, 1, m }   { 1, 0, 1, m + 1 }
// 4   5:   { 1, n, 1, 0 }   { 1, n, 1, 1 }
// 5   5:   { 1, n, 1, m }   { 1, n, 1, m + 1 }
//
// *   *:   { f, n, g, m }   { 1, f * n, g, m + 1 }
//

inline void CReaderWriterLock::EnterAsWriter()
{
    //  we had better not already own this lock as either a reader or a writer

    OSSYNCAssert( State().FNotOwner( 0 ) );
    OSSYNCAssert( State().FNotOwner( 1 ) );

    //  check for deadlock

    State().AssertCanBeWaiter();

    //  try forever until we successfully change the lock state

    OSSYNC_FOREVER
    {
        //  read the current state of the control word as our expected before image

        const ControlWord cwBIExpected = State().m_cw;

        //  compute the after image of the control word by performing the global
        //  transform for the EnterAsWriter state transition

        const ControlWord cwAI =    ControlWord( ( ( cwBIExpected & ( ( LONG_PTR( LONG( cwBIExpected ) ) >> 31 ) |
                                    0x0000FFFF ) ) | 0x80000000 ) + 0x00000001 );

        //  validate the transaction

        OSSYNCAssert( _FValidStateTransition( cwBIExpected, cwAI, trEnterAsWriter ) );

        //  attempt to perform the transacted state transition on the control word

        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

        //  the transaction failed or writers are quiesced from ownership or a
        //  writer already owns the lock

        if ( ( cwBI ^ cwBIExpected ) | ( cwBI & 0x0000FFFF ) )
        {
            //  the transaction failed because another context changed the control word

            if ( cwBI != cwBIExpected )
            {
                //  try again

                continue;
            }

            //  the transaction succeeded but writers are quiesced from ownership
            //  or a writer already owns the lock

            else
            {
                //  this is a contention for writers

                State().SetContend( 0 );

                //  wait to own the lock as a writer

                _EnterAsWriter( cwBI );

                //  we now own the lock, so we're done

                break;
            }
        }

        //  the transaction succeeded and writers were not quiesced from ownership
        //  and a writer did not already own the lock

        else
        {
            //  we now own the lock, so we're done

            break;
        }
    }

    //  we are now an owner of the lock for writers

    State().SetAcquire( 0 );
    State().AddAsOwner( 0 );
    State().StartHold( 0 );
}


//  tries to enter the reader / writer lock as a writer without waiting or
//  spinning, returning fFalse if writers are quiesced from ownership or
//  another writer already owns the lock
//
//  NOTE:  trying to enter the lock as a writer when you already own the lock
//  as a reader will cause a deadlock. Double acquires for write will always
//  fail.

inline const BOOL CReaderWriterLock::FTryEnterAsWriter()
{

    // Taking a write lock after taking the read lock is not allowed

    OSSYNCAssert( State().FNotOwner( 1 ) );

    //  check for double write acquires and early out for other contention scenarios

    if ( State().m_cw & 0x0000FFFF )
    {
        return fFalse;
    }

    //  we had better not already own this lock as a writer 
    
    OSSYNCAssert( State().FNotOwner( 0 ) );

    //  try forever until we successfully change the lock state

    OSSYNC_FOREVER
    {
        //  read the current state of the control word as our expected before image

        ControlWord cwBIExpected = State().m_cw;

        //  change the expected before image so that the transaction will only work if
        //  writers were not quiesced from ownership and another writer doesn't already
        //  own the lock

        cwBIExpected = cwBIExpected & 0xFFFF0000;

        //  compute the after image of the control word by performing the global
        //  transform for the EnterAsWriter state transition

        const ControlWord cwAI =    ControlWord( ( ( cwBIExpected & ( ( LONG_PTR( LONG( cwBIExpected ) ) >> 31 ) |
                                    0x0000FFFF ) ) | 0x80000000 ) + 0x00000001 );

        //  validate the transaction

        OSSYNCAssert(   _StateFromControlWord( cwBIExpected ) < 0 ||
                _FValidStateTransition( cwBIExpected, cwAI, trEnterAsWriter ) );

        //  attempt to perform the transacted state transition on the control word

        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

        //  the transaction failed

        if ( cwBI != cwBIExpected )
        {
            //  the transaction failed because writers were quiesced from ownership
            //  or another writer already owns the lock

            if ( cwBI & 0x0000FFFF )
            {
                //  return failure

                return fFalse;
            }

            //  the transaction failed because another context changed the control word

            else
            {
                //  try again

                continue;
            }
        }

        //  the transaction succeeded

        else
        {
            //  we are now an owner of the lock for writers

            State().SetAcquire( 0 );
            State().AddAsOwner( 0 );
            State().StartHold( 0 );

            // return success

            return fTrue;
        }
    }
}


//  leaves the reader / writer lock as a writer
//
//  NOTE:  you must leave the lock as a member of the same group for which you entered
//  the lock or deadlocks may occur
//
// State transitions for leaving as a Writer:
//
// 2   0:   { 1, 0, 0, 1 }   { 0, 0, 0, 0 }
// 2   2:   { 1, 0, 0, m }   { 1, 0, 0, m - 1 }
// 4   1:   { 1, n, 1, 0 }   { 0, n, 1, 0 };  cOwner--
// 5   3:   { 1, n, 1, m }   { 1, 0, 1, m };  cOwner--;  cOwner += n (can not result in 0)
//

inline void CReaderWriterLock::LeaveAsWriter()
{
    //  we are no longer an owner of the lock

    State().RemoveAsOwner( 0 );

    //  we are no longer holding the lock

    State().StopHold( 0 );

    //  try forever until we successfully change the lock state

    OSSYNC_FOREVER
    {
        //  read the current state of the control word as our expected before image

        ControlWord cwBIExpected = State().m_cw;

        //  change the expected before image so that the transaction will only work if
        //  writers were not quiesced from ownership

        cwBIExpected = cwBIExpected & 0xFFFF7FFF;

        //  compute the after image of the control word by performing the transform that
        //  will take us either from state 2 to state 0 or state 2 to state 2

        ControlWord cwAI =  cwBIExpected + 0xFFFFFFFF;
        cwAI = cwAI - ( ( ( cwAI + 0xFFFFFFFF ) << 16 ) & 0x80000000 );

        //  validate the transaction

        OSSYNCAssert(   _StateFromControlWord( cwBIExpected ) < 0 ||
                _FValidStateTransition( cwBIExpected, cwAI, trLeaveAsWriter ) );

        //  attempt to perform the transacted state transition on the control word

        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

        //  the transaction failed

        if ( cwBI != cwBIExpected )
        {
            //  the transaction failed because writers were quiesced from ownership

            if ( cwBI & 0x00008000 )
            {
                //  leave the lock as a quiesced owner

                _UpdateQuiescedOwnerCountAsWriter( 0xFFFFFFFF );

                //  we're done

                break;
            }

            //  the transaction failed because another context changed the control word

            else
            {
                //  try again

                continue;
            }
        }

        //  the transaction succeeded

        else
        {
            //  there were other writers waiting for ownership of the lock

            if ( cwAI & 0x00007FFF )
            {
                //  release the next writer into ownership of the lock

                State().m_semWriter.Release();
            }

            // we're done

            break;
        }
    }
}

//  enters the reader / writer lock as a reader, waiting forever if necessary
//
//  NOTE:  trying to enter the lock as a reader when you already own the lock
//  as a writer will cause a deadlock. disallowing nested readers on principle.
//
// State transitions for entering as a Reader:
//
// 0   1:   { 0, 0, 0, 0 }   { 0, 1, 1, 0 }
// 1   1:   { 0, n, 1, 0 }   { 0, n + 1, 1, 0 }
// 2   4:   { 1, 0, 0, 1 }   { 1, 1, 1, 0 };  cOwner++
// 2   5:   { 1, 0, 0, m > 1 }   { 1, 1, 1, m - 1 };  cOwner++
// 4   4:   { 1, n, 1, 0 }   { 1, n + 1, 1, 0 }
// 3   5:   { 1, 0, 1, m }   { 1, 1, 1, m }
// 5   5:   { 1, n, 1, m }   { 1, n + 1, 1, m }
//
// *   *:   { f, n, g, m }   { f, n + 1, 1, m ? m + g - 1 : 0 }
//

inline void CReaderWriterLock::EnterAsReader()
{
    //  we had better not already own this lock as either a reader or a writer

    OSSYNCAssert( State().FNotOwner( 0 ) );
    OSSYNCAssert( State().FNotOwner( 1 ) );

    //  check for deadlock

    State().AssertCanBeWaiter();

    //  try forever until we successfully change the lock state

    OSSYNC_FOREVER
    {
        //  read the current state of the control word as our expected before image

        const ControlWord cwBIExpected = State().m_cw;

        //  compute the after image of the control word by performing the global
        //  transform for the EnterAsReader state transition

        const ControlWord cwAI =    ( cwBIExpected & 0xFFFF7FFF ) +
                                    (   ( cwBIExpected & 0x80008000 ) == 0x80000000 ?
                                            0x00017FFF :
                                            0x00018000 );

        //  validate the transaction

        OSSYNCAssert( _FValidStateTransition( cwBIExpected, cwAI, trEnterAsReader ) );

        //  attempt to perform the transacted state transition on the control word

        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

        //  the transaction failed or readers were quiesced from ownership

        if ( ( cwBI ^ cwBIExpected ) | ( cwBI & 0x80000000 ) )
        {
            //  the transaction failed because another context changed the control word

            if ( cwBI != cwBIExpected )
            {
                //  try again

                continue;
            }

            //  the transaction succeeded but readers were quiesced from ownership

            else
            {
                //  this is a contention for readers

                State().SetContend( 1 );

                //  wait to own the lock as a reader

                _EnterAsReader( cwBI );

                //  we now own the lock, so we're done

                break;
            }
        }

        //  the transaction succeeded and readers were not quiesced from ownership

        else
        {
            //  we now own the lock, so we're done

            break;
        }
    }

    //  we are now an owner of the lock for readers

    State().SetAcquire( 1 );
    State().AddAsOwner( 1 );
    State().StartHold( 1 );
}

//  tries to enter the reader / writer lock as a reader without waiting or
//  spinning, returning fFalse if readers are quiesced from ownership
//
//  NOTE:  trying to enter the lock as a reader when you already own the lock
//  as a writer will cause a deadlock. disallowing nested readers on principle.

inline const BOOL CReaderWriterLock::FTryEnterAsReader()
{
    //  we had better not already own this lock as either a reader or a writer

    OSSYNCAssert( State().FNotOwner( 0 ) );
    OSSYNCAssert( State().FNotOwner( 1 ) );

    //  try forever until we successfully change the lock state

    OSSYNC_FOREVER
    {
        //  read the current state of the control word as our expected before image

        ControlWord cwBIExpected = State().m_cw;

        //  change the expected before image so that the transaction will only work if
        //  readers were not quiesced from ownership

        cwBIExpected = cwBIExpected & 0x7FFFFFFF;

        //  compute the after image of the control word by performing the global
        //  transform for the EnterAsReader state transition

        const ControlWord cwAI =    ( cwBIExpected & 0xFFFF7FFF ) +
                                    (   ( cwBIExpected & 0x80008000 ) == 0x80000000 ?
                                            0x00017FFF :
                                            0x00018000 );

        //  validate the transaction

        OSSYNCAssert(   _StateFromControlWord( cwBIExpected ) < 0 ||
                _FValidStateTransition( cwBIExpected, cwAI, trEnterAsReader ) );

        //  attempt to perform the transacted state transition on the control word

        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

        //  the transaction failed

        if ( cwBI != cwBIExpected )
        {
            //  the transaction failed because readers were quiesced from ownership

            if ( cwBI & 0x80000000 )
            {
                //  return failure

                return fFalse;
            }

            //  the transaction failed because another context changed the control word

            else
            {
                //  try again

                continue;
            }
        }

        //  the transaction succeeded

        else
        {
            //  we are now an owner of the lock for readers

            State().SetAcquire( 1 );
            State().AddAsOwner( 1 );
            State().StartHold( 1 );

            // return success

            return fTrue;
        }
    }
}

//  leaves the reader / writer lock as a reader
//
//  NOTE:  you must leave the lock as a member of the same group for which you entered
//  the lock or deadlocks may occur
//
// State transitions for leaving as a Reader:
//
// 1   0:   { 0, 1, 1, 0 }   { 0, 0, 0, 0 }
// 1   1:   { 0, n, 1, 0 }   { 0, n - 1, 1, 0 }
// 3   2:   { 1, 0, 1, m }   { 1, 0, 0, m };  cOwner--
// 3   3:   { 1, 0, 1, m }   { 1, 0, 1, m };  cOwner-- (can never result in 0 by definition)
// 5   4:   { 1, n, 1, 1 }   { 1, n, 1, 0 };  cOwner--;  cOwner++ (can not result in 0)
// 5   5:   { 1, n, 1, m > 1 }   { 1, n, 1, m - 1 };  cOwner--;  cOwner++ (can not result in 0)
// 5   5:   { 1, n, 1, m }   { 1, n, 1, m };  cOwner-- (can never result in 0 by definition)
//

inline void CReaderWriterLock::LeaveAsReader()
{
    //  we are no longer an owner of the lock

    State().RemoveAsOwner( 1 );

    //  we are no longer holding the lock

    State().StopHold( 1 );

    //  try forever until we successfully change the lock state

    OSSYNC_FOREVER
    {
        //  read the current state of the control word as our expected before image

        ControlWord cwBIExpected = State().m_cw;

        //  change the expected before image so that the transaction will only work if
        //  readers were not quiesced from ownership

        cwBIExpected = cwBIExpected & 0x7FFFFFFF;

        //  compute the after image of the control word by performing the transform that
        //  will take us either from state 1 to state 0 or state 1 to state 1

        const ControlWord cwAI =    ControlWord( cwBIExpected + 0xFFFF0000 +
                                    ( ( LONG_PTR( LONG( cwBIExpected + 0xFFFE0000 ) ) >> 31 ) & 0xFFFF8000 ) );

        //  validate the transaction

        OSSYNCAssert(   _StateFromControlWord( cwBIExpected ) < 0 ||
                _FValidStateTransition( cwBIExpected, cwAI, trLeaveAsReader ) );

        //  attempt to perform the transacted state transition on the control word

        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

        //  the transaction failed

        if ( cwBI != cwBIExpected )
        {
            //  the transaction failed because readers were quiesced from ownership

            if ( cwBI & 0x80000000 )
            {
                //  leave the lock as a quiesced owner

                _UpdateQuiescedOwnerCountAsReader( 0xFFFFFFFF );

                //  we're done

                break;
            }

            //  the transaction failed because another context changed the control word

            else
            {
                //  try again

                continue;
            }
        }

        //  the transaction succeeded

        else
        {
            // we're done

            break;
        }
    }
}



//  Metered Section

class CMeteredSection
    :   private CSyncObject
{
    public:

        //  types

        //  Note only max active for the active or quiscing group, a clever partition
        //  at the right time would double the true max active you can have.

        const static INT    cMaxActive = 0x7ffe;

        //    group

        //  Note: This type is insufficient for using in bit-fields, such as
        //     Group    m_group:1;
        //  because it would get sign extended to -1 when say passed back to
        //  ms.Leave( m_group ).  There it is better to use a DWORD/ULONG.
        typedef INT Group;
        const static Group  groupInvalidNil         = -1;
        const static Group  groupTooManyActiveErr   = -2;

        //    control word

        typedef DWORD ControlWord;

        //  callback used to notify the user when a partition of the current
        //  group has been completed

        typedef void (*PFNPARTITIONCOMPLETE)( const DWORD_PTR dwCompletionKey );

        //  member functions

        //    ctors / dtors

        CMeteredSection();
        ~CMeteredSection();

        //    manipulators

        INT Enter();
        Group GroupEnter();
        void Leave( const Group group );

        void Partition( const PFNPARTITIONCOMPLETE  pfnPartitionComplete    = NULL,
                        const DWORD_PTR             dwCompletionKey         = NULL );

        //    accessors

        Group GroupActive() const   { const INT iRet = INT( m_groupCurrent ); OSSYNCAssert( iRet == 0 || iRet == 1 ); return iRet; }
        Group GroupInactive() const { return 1 - GroupActive(); }
        BOOL FEmpty() const         { return ( m_cw & 0x7FFF7FFF ) == 0; }
#ifndef DEBUG
        BOOL FQuiescing() const     { return m_cQuiesced > 0; }
        INT CActiveUsers() const    { return INT( m_cCurrent ); }
        INT CQuiescingUsers() const { return INT( m_cQuiesced ); }
#endif  //  !DEBUG

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  data members

        //    partition complete callback

        PFNPARTITIONCOMPLETE    m_pfnPartitionComplete;
        DWORD_PTR               m_dwPartitionCompleteKey;

        //    control word

        //  Basic Layout (as DWORD and member variables):
        //      m_cCurrent = 0x42; m_groupCurrent = 1; m_cQuiesced = 0x7; m_groupQuiesced = 0;
        //          -> 0x00078042   { 0x42, 1, 0x7, 0 };
        //      m_cCurrent = 0x42; m_groupCurrent = 0; m_cQuiesced = 0x7; m_groupQuiesced = 1;
        //          -> 0x80070042   { 0x42, 1, 0x7, 0 }
        //               g  cg  c   { m_cCurrent, m_groupCurrent, m_cQuiesced, m_groupQuiesced };
        //               r  Qr  Curr
        //               Qui Curr
        //
        //  Sample Code and State:
        //      CMeteredSection     msTest;
        //          -> 0x80000000 { 0, 0, 0, 1 }
        //      i = msTest.Enter() x 3;
        //          -> 0x80000003 { 3, 0, 0, 1 }
        //      msTest.Partition( FooBar, &msTest );    - Begin
        //          -> 0x00038000 { 0, 1, 3, 0 }
        //      msTest.Leave( i );
        //          -> 0x00028000 { 0, 1, 2, 0 }
        //      i2 = msTest.Enter() x 6;
        //          -> 0x00028006 { 6, 1, 2, 0 }
        //      msTest.Leave( i ) x 2;
        //          -> triggers FooBar / .Partition()   - End
        //          -> 0x00008006 { 6, 1, 0, 0 }
        //      msTest.Leave( i2 );
        //          -> 0x00008005 { 5, 1, 0, 0 }
        //      msTest.Partition( FooBar, &msTest );    - Begin2
        //          -> 0x80050000 { 0, 0, 5, 1 }
        //      msTest.Leave( i2 );
        //          -> 0x80040000 { 0, 0, 4, 1 }
        //      msTest.Leave( i2 ) x 4;
        //          -> triggers FooBar / .Partition()   - End2
        //          -> 0x80000000 { 0, 0, 0, 1 }
        //
        //      Note: Another common "empty" state, would be this:
        //          -> 0x00008000 { 0, 1, 0, 0 }
        //          But the i2 x 6 Enter() above, left 6 in m_cCurrent @ 0x00008006.
        //
        union
        {
            volatile ControlWord    m_cw;

            struct
            {
                volatile DWORD          m_cCurrent:15;
                volatile DWORD          m_groupCurrent:1;
                volatile DWORD          m_cQuiesced:15;
                volatile DWORD          m_groupQuiesced:1;
            };
        };

        //  member functions

        //    operators

        CMeteredSection& operator=( CMeteredSection& ) = delete;  //  disallowed

        //    manipulators

        void _PartitionAsync(   const PFNPARTITIONCOMPLETE  pfnPartitionComplete,
                                const DWORD_PTR             dwCompletionKey );
        static void _PartitionSyncComplete( CAutoResetSignal* const pasig );

        //    disallowed ctors / dtors

    private:

        CMeteredSection( const CMeteredSection& ms ) = delete;
};


//  ctor

inline CMeteredSection::CMeteredSection()
    :   m_cw( 0x80000000 ),
        m_pfnPartitionComplete( NULL ),
        m_dwPartitionCompleteKey( NULL )
{
}

//  dtor

inline CMeteredSection::~CMeteredSection()
{
    //  asserts that all enters/leaves are matched at this point and 
    //  no partitions are pending
    
    OSSYNCAssert( FEmpty() || g_fSyncProcessAbort );
}

//  enter the metered section, returning the group id for which the current
//  context has acquired the metered section

inline CMeteredSection::Group CMeteredSection::GroupEnter()
{
    Group groupRet = groupInvalidNil;

    //  try forever until we successfully enter (or get too many active users)

    OSSYNC_FOREVER
    {
        //  increment the count for the current group
        //  old code: const DWORD cwBI = AtomicExchangeAdd( (LONG*) &m_cw, (LONG) 1 /* inc ref count */ );

        //  read the current state of the control word as our expected before image

        const ControlWord cwBIExpected = m_cw;

        //  compute the after image of the control word

        const ControlWord cwAI = cwBIExpected + 1 /* inc ref count */;

        if ( ( cwAI & 0x00007fff ) > cMaxActive )
        {
            //  we can not consume / allow in another active user, fail the caller out ...
            //  note: we are failing 1 earlier than we strickly need to for simplicity.

            groupRet = groupTooManyActiveErr;
            break;
        }

        //  had better be protected from any overflow at this point!

        OSSYNCAssert( ( cwBIExpected & 0x80008000 ) == ( cwAI & 0x80008000 ) );

        //  attempt to perform the transacted state transition on the control word

        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&m_cw, cwBIExpected, cwAI );

        if ( cwBI != cwBIExpected )
        {
            //  the transaction failed, try again ...

            continue;
        }
        else
        {
            //  the transaction succeeded

            //  there had better not be any overflow!

            OSSYNCAssert( ( cwBI & 0x80008000 ) == ( ( cwBI + 1 /* for inc ref count above */ ) & 0x80008000 ) );
            OSSYNCAssert( ( cwBI & 0x00007fff ) != 0x7fff  );

            //  the group should not have unexpectedly changed on us ...

            OSSYNCAssert( ( cwBIExpected & 0x80008000 ) == ( cwBI & 0x80008000 ) );

            //  we're done, assign group

            groupRet = INT( ( cwBI >> 15 ) & 1 );

            OSSYNCAssert( groupRet == 0 || groupRet == 1 );

            break;
        }
    }

    //  validated return group (or "err")

    OSSYNCAssert( groupRet != groupInvalidNil );
    OSSYNCAssert( groupRet == 0 || groupRet == 1 || groupRet == groupTooManyActiveErr );

    //  return the group we referenced

    return groupRet;
}

//  legacy enter function, prefer people move to the one that fails.

inline INT CMeteredSection::Enter()
{
    const Group groupRet = GroupEnter();

    //  use metered sections for a lot of critical data, we actually should panic out if this happens

    OSSYNCEnforceSz( groupRet != groupTooManyActiveErr, "Too many active threads/workers in this section, must quit to avoid consistency issues!" );

    return groupRet;
}

//  leave the metered section using the specified group id.  this group id must
//  be the group id returned by the corresponding call to Enter()

inline void CMeteredSection::Leave( const Group group )
{
    //  must be a valid group or all h will break loose

    OSSYNCAssert( group == 0 || group == 1 );
    
    //  try forever until we successfully leave

    OSSYNC_FOREVER
    {
        //  read the current state of the control word as our expected before image

        const ControlWord cwBIExpected = m_cw;

        //  we should never have left ourselves in a state w/ this many active users

        OSSYNCAssert( ( cwBIExpected & 0x00007fff ) != 0x7fff );

        //  compute the after image of the control word

        const ControlWord cwAI = cwBIExpected - ( ( ( ( cwBIExpected & 0x80008000 ) ^ 0x80008000 ) >> 15 ) ^ ( ( group << 16 ) | group ) );

        //  there had better not be any underflow!

        OSSYNCAssert( ( cwAI & 0x00007fff ) != 0x7fff );
        OSSYNCAssert( ( cwAI >> 16 ) != 0x7fff );
        OSSYNCAssert( ( cwBIExpected & 0x80008000 ) == ( cwAI & 0x80008000 ) );

        //  use metered sections for a lot of critical data, we actually should panic out if this happens

        OSSYNCEnforceSz( ( cwBIExpected & 0x80008000 ) == ( cwAI & 0x80008000 ),  "Underflow of a metered section, left a group we didn't enter!!!" );

        //  attempt to perform the transacted state transition on the control word

        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&m_cw, cwBIExpected, cwAI );

        //  the transaction failed

        if ( cwBI != cwBIExpected )
        {
            //  try again

            continue;
        }

        //  the transaction succeeded

        else
        {
            //  our update resulted in a partition completion

            if ( ( cwBI & 0x7FFF0000 ) + ( cwAI & 0x7FFF0000 ) == 0x00010000 )
            {
                //  execute the completion function

                m_pfnPartitionComplete( m_dwPartitionCompleteKey );
            }

            //  we're done

            break;
        }
    }
}

//  partitions all execution contexts entering the metered section into two groups.
//  all contexts entering the section after this call are in a different group than
//  all the contexts that entered the section before this call.  when all contexts
//  in the old group have left the metered section, the partition will be completed
//
//  there are two ways to complete a partition:  asynchronously and synchronously.
//  asynchronous operation is selected if a completion function and key are provided.
//  the last thread to leave the metered section for the previous group will
//  execute asynchronous completions
//
//  NOTE:  it is illegal to have multiple concurrent partition requests.  any attempt
//  to do so will result in undefined behavior

inline void CMeteredSection::Partition( const PFNPARTITIONCOMPLETE  pfnPartitionComplete,
                                        const DWORD_PTR             dwCompletionKey )
{
    //  this is an async partition request

    if ( pfnPartitionComplete )
    {
        //  execute the parititon request

        _PartitionAsync( pfnPartitionComplete, dwCompletionKey );
    }

    //  this is a sync partition request

    else
    {
        //  create a signal to wait for completion

        CAutoResetSignal asig( CSyncBasicInfo( "CMeteredSection::Partition()::asig" ) );

        //  issue an async partition request

        _PartitionAsync(    PFNPARTITIONCOMPLETE( _PartitionSyncComplete ),
                            DWORD_PTR( &asig ) );

        //  wait for the partition to complete

        asig.Wait();
    }
}

//  performs an async partition request

inline void CMeteredSection::_PartitionAsync(   const PFNPARTITIONCOMPLETE  pfnPartitionComplete,
                                                const DWORD_PTR             dwCompletionKey )
{
    //  we should not be calling this if there is already a partition pending

    OSSYNCAssertSz( !( m_cw & 0x7FFF0000 ), "Illegal concurrent use of Partitioning" );

    //  save the callback and key for the future completion

    m_pfnPartitionComplete      = pfnPartitionComplete;
    m_dwPartitionCompleteKey    = dwCompletionKey;

    //  try forever until we successfully partition

    OSSYNC_FOREVER
    {
        //  read the current state of the control word as our expected before image

        const ControlWord cwBIExpected = m_cw;

        //  compute the after image of the control word

        const ControlWord cwAI = ( cwBIExpected >> 16 ) | ( cwBIExpected << 16 );

        //  there should never be 0x7fff active OR quiescing users

        OSSYNCAssert( ( cwBIExpected & 0x00007fff ) != 0x7fff );
        OSSYNCAssert( ( cwBIExpected >> 16 ) != 0x7fff );
        OSSYNCAssert( ( cwAI & 0x00007fff ) != 0x7fff );
        OSSYNCAssert( ( cwAI >> 16 ) != 0x7fff );

        //  attempt to perform the transacted state transition on the control word

        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&m_cw, cwBIExpected, cwAI );

        //  the transaction failed

        if ( cwBI != cwBIExpected )
        {
            //  try again

            continue;
        }

        //  the transaction succeeded

        else
        {
            //  our update resulted in a partition completion

            if ( !( cwAI & 0x7FFF0000 ) )
            {
                //  execute the completion function

                m_pfnPartitionComplete( m_dwPartitionCompleteKey );
            }

            //  we're done

            break;
        }
    }
}

//  partition completion function used for sync partition requests

inline void CMeteredSection::_PartitionSyncComplete( CAutoResetSignal* const pasig )
{
    //  set the signal

    pasig->Set();
}


//  S / X / W Latch Performance Information

class CSXWLatchPerfInfo
    :   public CSyncPerfWait,
        public CSyncPerfAcquire,
        public CLockPerfHold
{
    public:

        //  member functions

        //    ctors / dtors

        CSXWLatchPerfInfo()
        {
        }

        //    debugging support

        void Dump( const CDumpContext& dc ) const;
};


//  S / X / W Latch Group Information

class CSXWLatchGroupInfo
{
    public:

        //  member functions

        //    ctors / dtors

        CSXWLatchGroupInfo() {}
        ~CSXWLatchGroupInfo() {}

        //    manipulators

        void StartWait( const INT iGroup ) { m_rginfo[iGroup].StartWait(); }
        void StopWait( const INT iGroup ) { m_rginfo[iGroup].StopWait(); }

        void SetAcquire( const INT iGroup ) { m_rginfo[iGroup].SetAcquire(); }
        void SetContend( const INT iGroup ) { m_rginfo[iGroup].SetContend(); }

        void StartHold( const INT iGroup ) { m_rginfo[iGroup].StartHold(); }
        void StopHold( const INT iGroup ) { m_rginfo[iGroup].StopHold(); }

        //    accessors

#ifdef SYNC_ANALYZE_PERFORMANCE

        QWORD   CWaitTotal( const INT iGroup ) const        { return m_rginfo[iGroup].CWaitTotal(); }
        double  CsecWaitElapsed( const INT iGroup ) const   { return m_rginfo[iGroup].CsecWaitElapsed(); }

        QWORD   CAcquireTotal( const INT iGroup ) const     { return m_rginfo[iGroup].CAcquireTotal(); }
        QWORD   CContendTotal( const INT iGroup ) const     { return m_rginfo[iGroup].CContendTotal(); }

        QWORD   CHoldTotal( const INT iGroup ) const        { return m_rginfo[iGroup].CHoldTotal(); }
        double  CsecHoldElapsed( const INT iGroup ) const   { return m_rginfo[iGroup].CsecHoldElapsed(); }

#endif  //  SYNC_ANALYZE_PERFORMANCE

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

    private:

        //  member functions

        //    operators

        CSXWLatchGroupInfo& operator=( CSXWLatchGroupInfo& ) = delete;  //  disallowed

        //  data members

        //    performance info for each group

        CSXWLatchPerfInfo m_rginfo[3];
};


//  S / X / W Latch Information

class CSXWLatchInfo
    :   public CLockBasicInfo,
        public CSXWLatchGroupInfo,
        public CLockDeadlockDetectionInfo
{
    public:

        //  member functions

        //    ctors / dtors

        CSXWLatchInfo( const CLockBasicInfo& lbi )
            :   CLockBasicInfo( lbi ),
                CLockDeadlockDetectionInfo( (CLockBasicInfo&) *this )
        {
        }

        //    debugging support

        void Dump( const CDumpContext& dc ) const;
};


//  S / X / W Latch State

class CSXWLatchState
{
    public:

        //  types

        //    control word

        typedef DWORD ControlWord;

        //  member functions

        //    ctors / dtors

        CSXWLatchState( const CSyncBasicInfo& sbi );
        ~CSXWLatchState();

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

        //  data members

        //    control word

        union
        {
            volatile ControlWord    m_cw;

            struct
            {
                volatile DWORD          m_cOOWS:15;
                volatile DWORD          m_fQS:1;
                volatile DWORD          m_cOAWX:16;
            };
        };

        //    quiesced share latch count

        volatile DWORD          m_cQS;

        //    sempahore used to wait for the shared latch

        CSemaphore              m_semS;

        //    sempahore used to wait for the exclusive latch

        CSemaphore              m_semX;

        //    sempahore used to wait for the write latch

        CSemaphore              m_semW;

    private:

        //  member functions

        //    operators

        CSXWLatchState& operator=( CSXWLatchState& ) = delete;  //  disallowed
};


//  S / X / W Latch

class CSXWLatch
    :   private CLockObject,
        private CEnhancedStateContainer< CSXWLatchState, CSyncBasicInfo, CSXWLatchInfo, CLockBasicInfo >
{
    public:

        //  types

        //    control word

        typedef CSXWLatchState::ControlWord ControlWord;

        //    API Error Codes

        enum class ERR
        {
            errSuccess,
            errWaitForSharedLatch,
            errWaitForExclusiveLatch,
            errWaitForWriteLatch,
            errLatchConflict
        };

        //    latch groups

        enum iGroup
        {
            iSharedGroup = 0,
            iExclusiveGroup = 1,
            iWriteGroup = 2,
            iNoGroup = 3
        };

        //  member functions

        //    ctors / dtors

        CSXWLatch( const CLockBasicInfo& lbi );
        ~CSXWLatch();

        //    manipulators

        ERR ErrAcquireSharedLatch();
        ERR ErrTryAcquireSharedLatch();
        ERR ErrAcquireExclusiveLatch();
        ERR ErrTryAcquireExclusiveLatch();
        ERR ErrTryAcquireWriteLatch();

        ERR ErrUpgradeSharedLatchToExclusiveLatch();
        ERR ErrUpgradeSharedLatchToWriteLatch();
        ERR ErrUpgradeExclusiveLatchToWriteLatch();
        void UpgradeExclusiveLatchToWriteLatch();

        ERR ErrTryUpgradeSharedLatchToWriteLatch();
        ERR ErrTryUpgradeExclusiveLatchToWriteLatch();

        void AcquireSharedLatch();
        void AcquireExclusiveLatch();
        void AcquireWriteLatch();

        void DowngradeWriteLatchToExclusiveLatch();
        void DowngradeWriteLatchToSharedLatch();
        void DowngradeExclusiveLatchToSharedLatch();

        void ReleaseWriteLatch();
        void ReleaseExclusiveLatch();
        void ReleaseSharedLatch();
        void ReleaseLatch( const iGroup igroup );

        void WaitForSharedLatch();
        void WaitForExclusiveLatch();
        void WaitForWriteLatch();

        void ClaimOwnership( const DWORD group );
        void ReleaseOwnership( const DWORD group );

        //    accessors

        const INT CWaitSharedLatch() const      { return State().m_semS.CWait(); }
        const INT CWaitExclusiveLatch() const   { return State().m_semX.CWait(); }
        const INT CWaitWriteLatch() const       { return State().m_semW.CWait(); }

        //  Note: Remember there is no thread safe test of a latch, other than to acquire it.
        BOOL FLatched()  const                  { return State().m_cw != 0; }


#if defined( DEBUG ) || defined( DEBUGGER_EXTENSION )


        // We are making this specifically only compile in retail, not debug and private
        //  so these APIs aren't used except in the debugger extension and asserts

        //  Note: Remember there is no thread safe test of a latch, other than to acquire it.
        BOOL FSharedLatched() const
        {
            //  Note: This is NOT concurrently safe, see CSharers().
            return CSharers() >= 0x00000001;
        }
        BOOL FExclusiveLatched() const
        {
            return ( ( State().m_cw & 0xFFFF0000 ) >= 0x00010000 ) &&
                    ( 0x8000 != ( State().m_cw & 0x00008000 ) );
        }
        BOOL FWriteLatched() const
        {
            //  Note: This is NOT concurrently safe, there is a small window where a w-latch
            //  could have removed the lower 0x7fff sharers from the control word, and not
            //  yet added them to m_cQS / quiescing sharers count.
            return ( ( State().m_cw & 0xFFFF0000 ) >= 0x00010000 ) &&
                    ( 0x8000 == ( State().m_cw & 0x00008000 ) &&
                    ( 0 == State().m_cQS ) );
        }

        INT CSharers() const
        {
            //  Note: This is NOT concurrently safe, there is a small window where a w-latch
            //  could have removed the lower 0x7fff sharers from the control word, and not
            //  yet added them to m_cQS / quiescing sharers count.
            return ( 0x8000 != ( State().m_cw & 0x00008000 ) ) ?
                        ( State().m_cw & 0x00007FFF ) :
                        State().m_cQS;
        }

    public:

#endif  //  defined( DEBUG ) || defined( DEBUGGER_EXTENSION )


#ifdef SYNC_DEADLOCK_DETECTION

        BOOL FOwnSharedLatch()                  { return State().FOwner( 0 ); }
        BOOL FNotOwnSharedLatch()               { return State().FNotOwner( 0 ); }
        BOOL FOwnExclusiveLatch()               { return State().FOwner( 1 ); }
        BOOL FNotOwnExclusiveLatch()            { return State().FNotOwner( 1 ); }
        BOOL FOwnWriteLatch()                   { return State().FOwner( 2 ); }
        BOOL FNotOwnWriteLatch()                { return State().FNotOwner( 2 ); }
        BOOL FOwner()                           { return FOwnSharedLatch() || FOwnExclusiveLatch() || FOwnWriteLatch(); }
        BOOL FNotOwner()                        { return FNotOwnSharedLatch() && FNotOwnExclusiveLatch() && FNotOwnWriteLatch(); }

#else   //  !SYNC_DEADLOCK_DETECTION

#ifdef DEBUG

        BOOL FOwnSharedLatch()                  { return fTrue; }
        BOOL FNotOwnSharedLatch()               { return fTrue; }
        BOOL FOwnExclusiveLatch()               { return fTrue; }
        BOOL FNotOwnExclusiveLatch()            { return fTrue; }
        BOOL FOwnWriteLatch()                   { return fTrue; }
        BOOL FNotOwnWriteLatch()                { return fTrue; }
        BOOL FOwner()                           { return fTrue; }
        BOOL FNotOwner()                        { return fTrue; }

#endif  //  DEBUG

#endif  //  SYNC_DEADLOCK_DETECTION

        //    debugging support

        void Dump( const CDumpContext& dc ) const;

        void AssertValid( __in const iGroup iGroupOwnsOrWaits = iNoGroup );

    private:

        //  member functions

        //    operators

        CSXWLatch& operator=( CSXWLatch& ) = delete;  //  disallowed

        //    manipulators

        void _UpdateQuiescedSharedLatchCount( const DWORD cQSDelta );
};

//  checks certain immutable properties of the latch.  the group argument indicates
//  that at the call site we either own or are at least waiting.

inline void CSXWLatch::AssertValid( __in const iGroup iGroupOwnsOrWaits )
{
    OSSYNCEnforce( ( State().m_cw & 0x00007fff ) != 0x00007fff ); // likely underflow of s-latch, someone released a latch they didn't have?
    OSSYNCEnforce( ( State().m_cw & 0xffff0000 ) != 0xffff0000 ); // likely underflow of x|w-latch, someone released a latch they didn't have?

#ifndef RTM
    switch( iGroupOwnsOrWaits )
    {
        case iSharedGroup:      // shared latch
            // This isn't exactly immutable, because there is a timing window
            // between when a writer quiesced shared latches, and updated the
            // quiescing count / m_cQS.  Note we have 64-bit atomic compare
            // exchange functions now, we could merge m_cw and m_cQW into a
            // single piece of state.
            //OSSYNCEnforce( State().m_cw & 0x00007fff || // should have at least 1 s-latch state
            //                  State().m_cQS );            //  or quiescing s-latches
            break;
        case iExclusiveGroup:   // exclusive latch
            OSSYNCEnforce( State().m_cw & 0xffff0000 );   // should have at least 1 x|w-latch state
            break;
        case iWriteGroup:       // write latch
            OSSYNCEnforce( State().m_cw & 0xffff0000 );   // should have at least 1 x|w-latch state
            OSSYNCEnforce( ( State().m_cw & 0x00008000 ) == 0x00008000 );
            break;
        case iNoGroup:
            break;
        default:
            OSSYNCAssertSzRTL( fFalse, "Unknown latch group." );
    }
#endif
}

//  declares the current context as an owner or waiter of a shared latch.  if
//  the shared latch is acquired immediately, errSuccess will be returned.  if
//  the shared latch is not acquired immediately, errWaitForSharedLatch will be
//  returned and WaitForSharedLatch() must be called to gain ownership of the
//  shared latch

inline CSXWLatch::ERR CSXWLatch::ErrAcquireSharedLatch()
{
    //  we had better not already have a shared, exclusive, or write latch

    OSSYNCAssert( FNotOwner() );

    //  validate immutable properties are not broken

    AssertValid();

    //  add ourself as an owner or waiter for the shared latch

    const ControlWord cwDelta = 0x00000001;
    const ControlWord cwBI = AtomicExchangeAdd( (LONG*)&State().m_cw, cwDelta );

    //  shared latches are quiesced

    if ( cwBI & 0x00008000 )
    {
        //  this is a contention for a shared latch

        State().SetContend( 0 );

        //  we are now a waiter for the shared latch

        State().AddAsWaiter( 0 );
        State().StartWait( 0 );

        AssertValid( iSharedGroup );

        //  we will need to block

        return ERR::errWaitForSharedLatch;
    }

    //  shared latches are not quiesced

    else
    {
        //  we are now an owner of a shared latch

        State().SetAcquire( 0 );
        State().AddAsOwner( 0 );
        State().StartHold( 0 );

        AssertValid( iSharedGroup );

        //  we now own the shared latch

        return ERR::errSuccess;
    }
}

//  tries to declare the current context as an owner of a shared latch.  if
//  the shared latch is acquired immediately, errSuccess will be returned.  if
//  the shared latch is not acquired immediately, errLatchConflict will be
//  returned

inline CSXWLatch::ERR CSXWLatch::ErrTryAcquireSharedLatch()
{
    //  we had better not already have a shared latch

    OSSYNCAssert( FNotOwnSharedLatch() );

    //  try forever until we successfully change the latch state

    OSSYNC_FOREVER
    {
        //  validate immutable properties are not broken

        AssertValid();

        //  read the current state of the control word as our expected before image

        ControlWord cwBIExpected = State().m_cw;

        //  change the expected before image so that the transaction will only work if
        //  shared latches are not quiesced

        cwBIExpected = cwBIExpected & 0xFFFF7FFF;

        //  compute the after image of the control word by performing the transform
        //  that will acquire a shared latch iff shared latches are not quiesced

        const ControlWord cwAI = cwBIExpected + 0x00000001;

        //  attempt to perform the transacted state transition on the control word

        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

        //  the transaction failed

        if ( cwBI != cwBIExpected )
        {
            //  the transaction failed because shared latches were quiesced

            if ( cwBI & 0x00008000 )
            {
                //  this is a contention for the shared latch

                State().SetContend( 0 );

                //  validate immutable properties are not broken

                AssertValid();

                //  this is a latch conflict

                return ERR::errLatchConflict;
            }

            //  the transaction failed because another context changed the control
            //  word

            else
            {
                //  try again

                continue;
            }
        }

        //  the transaction succeeded

        else
        {
            //  we are now an owner of a shared latch

            State().SetAcquire( 0 );
            State().AddAsOwner( 0 );
            State().StartHold( 0 );

            AssertValid( iSharedGroup );

            //  we now own the shared latch

            return ERR::errSuccess;
        }
    }
}

//  declares the current context as an owner or waiter of the exclusive latch.
//  if the exclusive latch is acquired immediately, errSuccess will be returned.
//  if the exclusive latch is not acquired immediately, errWaitForExclusiveLatch
//  will be returned and WaitForExclusiveLatch() must be called to gain ownership
//  of the exclusive latch

inline CSXWLatch::ERR CSXWLatch::ErrAcquireExclusiveLatch()
{
    //  we had better not already have a shared, exclusive, or write latch

    OSSYNCAssert( FNotOwner() );

    //  validate immutable properties are not broken

    AssertValid();

    //  add ourself as an owner or waiter for the exclusive latch

    const ControlWord cwDelta = 0x00010000;
    const ControlWord cwBI = AtomicExchangeAdd( (LONG*)&State().m_cw, cwDelta );

    //  we are not the owner of the exclusive latch

    if ( cwBI & 0xFFFF0000 )
    {
        //  this is a contention for the exclusive latch

        State().SetContend( 1 );

        //  we are now a waiter for the exclusive latch

        State().AddAsWaiter( 1 );
        State().StartWait( 1 );

        //  validate immutable properties are not broken

        AssertValid( iExclusiveGroup );

        //  we will need to block

        return ERR::errWaitForExclusiveLatch;
    }

    //  we are the owner of the exclusive latch

    else
    {
        //  we are now an owner of the exclusive latch

        State().SetAcquire( 1 );
        State().AddAsOwner( 1 );
        State().StartHold( 1 );

        //  validate immutable properties are not broken

        AssertValid( iExclusiveGroup );

        //  we now own the exclusive latch

        return ERR::errSuccess;
    }
}

//  tries to declare the current context as an owner of the exclusive latch.  if
//  the exclusive latch is acquired immediately, errSuccess will be returned.  if
//  the exclusive latch is not acquired immediately, errLatchConflict will be
//  returned

inline CSXWLatch::ERR CSXWLatch::ErrTryAcquireExclusiveLatch()
{
    //  we had better not already have the exclusive latch

    OSSYNCAssert( FNotOwnSharedLatch() );   // technically we could leave this off
    OSSYNCAssert( FNotOwnExclusiveLatch() );
    OSSYNCAssert( FNotOwnWriteLatch() );    // technically we could leave this off

    //  Note technically we allow ourselves to "double enter" once as shared latch,
    //  and a second time as an x-latch.  In such a case however, we are somewhat
    //  protected from deadlock (with ourselves) in that upgrade from x-latch to
    //  w-latch asserts we don't have the s-latch.

    //  try forever until we successfully change the latch state

    OSSYNC_FOREVER
    {
        //  validate immutable properties are not broken

        AssertValid();

        //  read the current state of the control word as our expected before image

        ControlWord cwBIExpected = State().m_cw;

        //  change the expected before image so that the transaction will only work if
        //  the exclusive latch is not already owned

        cwBIExpected = cwBIExpected & 0x0000FFFF;

        //  compute the after image of the control word by performing the transform
        //  that will acquire the exclusive latch iff it is not already owned

        const ControlWord cwAI = cwBIExpected + 0x00010000;

        //  attempt to perform the transacted state transition on the control word

        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

        //  the transaction failed

        if ( cwBI != cwBIExpected )
        {
            //  the transaction failed because the exclusive latch was already
            //  owned

            if ( cwBI & 0xFFFF0000 )
            {
                //  this is a contention for the exclusive latch

                State().SetContend( 1 );

                //  validate immutable properties are not broken

                AssertValid();

                //  this is a latch conflict

                return ERR::errLatchConflict;
            }

            //  the transaction failed because another context changed the control
            //  word

            else
            {
                //  try again

                continue;
            }
        }

        //  the transaction succeeded

        else
        {
            //  we are now an owner of the exclusive latch

            State().SetAcquire( 1 );
            State().AddAsOwner( 1 );
            State().StartHold( 1 );

            //  validate immutable properties are not broken

            AssertValid( iExclusiveGroup );

            //  we now own the exclusive latch

            return ERR::errSuccess;
        }
    }
}

//  tries to declare the current context as an owner of the write latch.  if
//  the write latch is acquired immediately, errSuccess will be returned.  if
//  the write latch is not acquired immediately, errLatchConflict will be
//  returned.  note that a latch conflict will effectively occur if any other
//  context currently owns or is waiting to own any type of latch

inline CSXWLatch::ERR CSXWLatch::ErrTryAcquireWriteLatch()
{
    //  we had better not already have the write latch

    OSSYNCAssert( FNotOwnWriteLatch() );

    //  validate immutable properties are not broken

    AssertValid();

    //  set the expected before image so that the transaction will only work if
    //  no other context currently owns or is waiting to own any type of latch

    const ControlWord cwBIExpected = 0x00000000;

    //  set the after image of the control word to a single write latch

    const ControlWord cwAI = 0x00018000;

    //  attempt to perform the transacted state transition on the control word

    const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

    //  the transaction failed

    if ( cwBI != cwBIExpected )
    {
        //  this is a contention for the write latch

        State().SetContend( 2 );

        //  validate immutable properties are not broken

        AssertValid();

        //  this is a latch conflict

        return ERR::errLatchConflict;
    }

    //  the transaction succeeded

    else
    {
        //  we are now an owner of the write latch

        State().SetAcquire( 2 );
        State().AddAsOwner( 2 );
        State().StartHold( 2 );

        //  validate immutable properties are not broken

        AssertValid( iWriteGroup );

        //  we now own the write latch

        return ERR::errSuccess;
    }
}

//  attempts to upgrade a shared latch to the exclusive latch.  if the exclusive
//  latch is not available, errLatchConflict will be returned.  if the exclusive
//  latch is available, it will be acquired and errSuccess will be returned

inline CSXWLatch::ERR CSXWLatch::ErrUpgradeSharedLatchToExclusiveLatch()
{
    //  we had better already have only a shared latch

    OSSYNCAssert( FOwnSharedLatch() );
    OSSYNCAssert( FNotOwnExclusiveLatch() );
    OSSYNCAssert( FNotOwnWriteLatch() );

    //  try forever until we successfully change the latch state

    OSSYNC_FOREVER
    {
        //  validate immutable properties are not broken

        AssertValid( iSharedGroup );

        //  read the current state of the control word as our expected before image

        ControlWord cwBIExpected = State().m_cw;

        //  change the expected before image so that the transaction will only work if
        //  the exclusive latch is not already owned

        cwBIExpected = cwBIExpected & 0x0000FFFF;

        //  compute the after image of the control word by performing the transform
        //  that will set an exclusive latch iff there is no current owner of the
        //  exclusive latch and release our shared latch

        const ControlWord cwAI = cwBIExpected + 0x0000FFFF;

        //  attempt to perform the transacted state transition on the control word

        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

        //  the transaction failed

        if ( cwBI != cwBIExpected )
        {
            //  the transaction failed because the exclusive latch was already owned

            if ( cwBI & 0xFFFF0000 )
            {
                //  this is a contention for the exclusive latch

                State().SetContend( 1 );

                //  validate immutable properties are not broken

                AssertValid( iSharedGroup );

                //  this is a latch conflict

                return ERR::errLatchConflict;
            }

            //  the transaction failed because another context changed the control
            //  word

            else
            {
                //  try again

                continue;
            }
        }

        //  the transaction succeeded

        else
        {
            //  we are no longer an owner of a shared latch

            State().RemoveAsOwner( 0 );
            State().StopHold( 0 );

            //  we are now an owner of the exclusive latch

            State().SetAcquire( 1 );
            State().AddAsOwner( 1 );
            State().StartHold( 1 );

            //  validate immutable properties are not broken

            AssertValid( iExclusiveGroup );

            //  we now own the exclusive latch

            return ERR::errSuccess;
        }
    }
}

//  attempts to upgrade a shared latch to the write latch.  if the write latch
//  is not available, errLatchConflict will be returned.  if the write latch is
//  available, it will be acquired.  if the write latch is acquired immediately,
//  errSuccess will be returned.  if the write latch is not acquired immediately,
//  errWaitForWriteLatch will be returned and WaitForWriteLatch() must be called
//  (after which the thread has gained ownership of the write latch).

inline CSXWLatch::ERR CSXWLatch::ErrUpgradeSharedLatchToWriteLatch()
{
    //  we had better already have only a shared latch

    OSSYNCAssert( FOwnSharedLatch() );
    OSSYNCAssert( FNotOwnExclusiveLatch() );
    OSSYNCAssert( FNotOwnWriteLatch() );

    //  try forever until we successfully change the latch state

    OSSYNC_FOREVER
    {
        //  validate immutable properties are not broken

        AssertValid( iSharedGroup );

        //  read the current state of the control word as our expected before image

        ControlWord cwBIExpected = State().m_cw;

        //  change the expected before image so that the transaction will only work if
        //  the exclusive latch is not already owned

        cwBIExpected = cwBIExpected & 0x0000FFFF;

        //  compute the after image of the control word by performing the transform
        //  that will set a write latch iff there is no current owner of the
        //  exclusive latch, quiescing any remaining shared latches

        const ControlWord cwAI = 0x00018000;

        //  attempt to perform the transacted state transition on the control word

        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

        //  the transaction failed

        if ( cwBI != cwBIExpected )
        {
            //  the transaction failed because the write latch was already owned

            if ( cwBI & 0xFFFF0000 )
            {
                //  this is a contention for the write latch

                State().SetContend( 2 );

                //  validate immutable properties are not broken

                AssertValid( iSharedGroup );

                //  this is a latch conflict

                return ERR::errLatchConflict;
            }

            //  the transaction failed because another context changed the control
            //  word

            else
            {
                //  try again

                continue;
            }
        }

        //  the transaction succeeded

        else
        {
            //  shared latches were just quiesced

            if ( cwBI != 0x00000001 )
            {
                //  we are no longer an owner of a shared latch

                State().RemoveAsOwner( 0 );
                State().StopHold( 0 );

                //  update the quiesced shared latch count with the shared latch count
                //  that we displaced from the control word, possibly releasing waiters.
                //  we update the count as if we we had a shared latch as a write latch
                //  (namely ours) can be released.  don't forget to deduct our shared
                //  latch from this count

                _UpdateQuiescedSharedLatchCount( cwBI - 1 );

                //  we are now a waiter for the write latch

                State().AddAsWaiter( 2 );
                State().StartWait( 2 );

                //  validate immutable properties are not broken

                AssertValid( iWriteGroup );

                //  we will need to block

                return ERR::errWaitForWriteLatch;
            }

            //  shared latches were not just quiesced

            else
            {
                //  we are no longer an owner of a shared latch

                State().RemoveAsOwner( 0 );
                State().StopHold( 0 );

                //  we are now an owner of the write latch

                State().SetAcquire( 2 );
                State().AddAsOwner( 2 );
                State().StartHold( 2 );

                //  validate immutable properties are not broken

                AssertValid( iWriteGroup );

                //  we now own the write latch

                return ERR::errSuccess;
            }
        }
    }
}

//  upgrades the exclusive latch to the write latch.  if the write latch is
//  acquired immediately, errSuccess will be returned.  if the write latch is
//  not acquired immediately, errWaitForWriteLatch is returned and
//  WaitForWriteLatch() must be called (after which the thread has gained
//  ownership of the write latch).

inline CSXWLatch::ERR CSXWLatch::ErrUpgradeExclusiveLatchToWriteLatch()
{
    //  we had better already have only an exclusive latch

    OSSYNCAssert( FNotOwnSharedLatch() );
    OSSYNCAssert( FOwnExclusiveLatch() );
    OSSYNCAssert( FNotOwnWriteLatch() );

    //  we are no longer an owner of the exclusive latch

    //  note: we remove ourselves as owner here, before the transistion ... unlike
    //  ErrUpgradeExclusiveLatchToWriteLatch()? I wonder why?
    State().RemoveAsOwner( 1 );
    State().StopHold( 1 );

    //  try forever until we successfully change the latch state

    OSSYNC_FOREVER
    {
        //  validate immutable properties are not broken

        AssertValid( iExclusiveGroup );

        //  read the current state of the control word as our expected before image

        const ControlWord cwBIExpected = State().m_cw;

        //  compute the after image of the control word by performing the transform that
        //  will quiesce shared latches by setting the fQS flag and removing the current
        //  shared latch count from the control word

        const ControlWord cwAI = ( cwBIExpected & 0xFFFF0000 ) | 0x00008000;

        //  attempt to perform the transacted state transition on the control word

        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

        //  the transaction failed

        if ( cwBI != cwBIExpected )
        {
            //  try again

            continue;
        }

        //  the transaction succeeded

        else
        {
            //  shared latches were just quiesced

            if ( cwBI & 0x00007FFF )
            {
                //  this is a contention for the write latch

                State().SetContend( 2 );

                //  update the quiesced shared latch count with the shared latch
                //  count that we displaced from the control word, possibly
                //  releasing waiters.  we update the count as if we we had a
                //  shared latch as a write latch (namely ours) can be released

                _UpdateQuiescedSharedLatchCount( cwBI & 0x00007FFF );

                //  we are now a waiter for the write latch

                State().AddAsWaiter( 2 );
                State().StartWait( 2 );

                //  validate immutable properties are not broken

                AssertValid( iWriteGroup );

                //  we will need to block

                return ERR::errWaitForWriteLatch;
            }

            //  shared latches were not just quiesced

            else
            {
                //  we are now an owner of the write latch

                State().SetAcquire( 2 );
                State().AddAsOwner( 2 );
                State().StartHold( 2 );

                //  validate immutable properties are not broken

                AssertValid( iWriteGroup );

                //  we now own the write latch

                return ERR::errSuccess;
            }
        }
    }
}

//  upgrades the exclusive latch to the write latch.

inline void CSXWLatch::UpgradeExclusiveLatchToWriteLatch()
{
    OSSYNCAssert( FOwnExclusiveLatch() );

    ERR err = ErrUpgradeExclusiveLatchToWriteLatch();

    if ( ERR::errWaitForWriteLatch == err )
    {
        WaitForWriteLatch();
        err = ERR::errSuccess;
    }

    OSSYNCAssert( ERR::errSuccess == err );
}

//  tries to upgrade a shared latch owned by this context to the write latch.
//  if the write latch is acquired immediately, errSuccess will be returned.
//  if the write latch is not acquired immediately, errLatchConflict will be
//  returned.  note that a latch conflict will effectively occur if any other
//  context currently owns or is waiting to own any type of latch

inline CSXWLatch::ERR CSXWLatch::ErrTryUpgradeSharedLatchToWriteLatch()
{
    //  we had better already have only a shared latch

    OSSYNCAssert( FOwnSharedLatch() );
    OSSYNCAssert( FNotOwnExclusiveLatch() );
    OSSYNCAssert( FNotOwnWriteLatch() );

    //  validate immutable properties are not broken

    AssertValid( iSharedGroup );

    //  set the expected before image so that the transaction will only work if
    //  we are the only owner of a latch and it is a share latch

    const ControlWord cwBIExpected = 0x00000001;

    //  set the after image of the control word to a single write latch

    const ControlWord cwAI = 0x00018000;

    //  attempt to perform the transacted state transition on the control word

    const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

    //  the transaction failed

    if ( cwBI != cwBIExpected )
    {
        //  this is a contention for the write latch

        State().SetContend( 2 );

        //  validate immutable properties are not broken

        AssertValid( iSharedGroup );

        //  this is a latch conflict

        return ERR::errLatchConflict;
    }

    //  the transaction succeeded

    else
    {
        //  we are no longer an owner of a shared latch

        State().RemoveAsOwner( 0 );
        State().StopHold( 0 );

        //  we are now an owner of the write latch

        State().SetAcquire( 2 );
        State().AddAsOwner( 2 );
        State().StartHold( 2 );

        //  validate immutable properties are not broken

        AssertValid( iWriteGroup );

        //  we now own the write latch

        return ERR::errSuccess;
    }
}

//  tries to upgrade a exclusive latch owned by this context to the write latch.
//  if the write latch is acquired immediately, errSuccess will be returned.
//  if the write latch is not acquired immediately, errLatchConflict will be
//  returned.  note that a latch conflict will effectively occur if any other
//  context currently owns or is waiting to own any type of latch

inline CSXWLatch::ERR CSXWLatch::ErrTryUpgradeExclusiveLatchToWriteLatch()
{
    //  we had better already have only a exclusive latch

    OSSYNCAssert( FNotOwnSharedLatch() );
    OSSYNCAssert( FOwnExclusiveLatch() );
    OSSYNCAssert( FNotOwnWriteLatch() );

    //  validate immutable properties are not broken

    AssertValid( iExclusiveGroup );

    //  validate immutable properties are not broken

    AssertValid( iExclusiveGroup );

    //  at least one person should have the x-latch, b/c this thread is supposed
    //  to have it to call this function.

    OSSYNCAssert( State().m_cw & 0x7FFF0000 );

    //  set the expected before image so that the transaction will only work if
    //  we are the only owner of a latch and it is a exclusive latch

    const ControlWord cwBIExpected = 0x00010000;

    //  set the after image of the control word to a single write latch

    const ControlWord cwAI = 0x00018000;

    //  attempt to perform the transacted state transition on the control word

    const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

    //  the transaction failed

    if ( cwBI != cwBIExpected )
    {
        //  this is a contention for the write latch

        State().SetContend( 2 );

        //  validate immutable properties are not broken

        AssertValid( iExclusiveGroup );

        //  this is a latch conflict

        return ERR::errLatchConflict;
    }

    //  the transaction succeeded

    else
    {
        //  we are no longer an owner of a exclusive latch

        State().RemoveAsOwner( 1 );
        State().StopHold( 1 );

        //  we are now an owner of the write latch

        State().SetAcquire( 2 );
        State().AddAsOwner( 2 );
        State().StartHold( 2 );

        //  validate immutable properties are not broken

        AssertValid( iWriteGroup );

        //  we now own the write latch

        return ERR::errSuccess;
    }
}

//  releases the write latch in exchange for the exclusive latch

inline void CSXWLatch::DowngradeWriteLatchToExclusiveLatch()
{
    //  we had better already have only a write latch

    OSSYNCAssert( FNotOwnSharedLatch() );
    OSSYNCAssert( FNotOwnExclusiveLatch() );
    OSSYNCAssert( FOwnWriteLatch() );

    //  validate immutable properties are not broken

    AssertValid( iWriteGroup );

    //  stop quiescing shared latches by resetting the fQS flag

    const ControlWord cwDelta = 0xFFFF8000;
    const ControlWord cwBI = AtomicExchangeAdd( (LONG*)&State().m_cw, cwDelta );

    //  transfer ownership from the write latch to the exclusive latch

    State().RemoveAsOwner( 2 );
    State().StopHold( 2 );

    State().SetAcquire( 1 );
    State().AddAsOwner( 1 );
    State().StartHold( 1 );

    //  release any quiesced shared latches

    if ( cwBI & 0x00007FFF )
    {
        //  in the event we have to release someone, we can check there is a shared waiter

        AssertValid( iSharedGroup );

        State().m_semS.Release( cwBI & 0x00007FFF );
    }

    //  validate immutable properties are not broken

    AssertValid( iExclusiveGroup );
}

//  releases the write latch in exchange for a shared latch

inline void CSXWLatch::DowngradeWriteLatchToSharedLatch()
{
    //  we had better already have only a write latch

    OSSYNCAssert( FNotOwnSharedLatch() );
    OSSYNCAssert( FNotOwnExclusiveLatch() );
    OSSYNCAssert( FOwnWriteLatch() );

    //  validate immutable properties are not broken

    AssertValid( iWriteGroup );

    //  stop quiescing shared latches by resetting the fQS flag, release our
    //  exclusive latch, and acquire a shared latch

    const ControlWord cwDelta = 0xFFFE8001;
    const ControlWord cwBI = AtomicExchangeAdd( (LONG*)&State().m_cw, cwDelta );

    //  transfer ownership from the write latch to a shared latch

    State().RemoveAsOwner( 2 );
    State().StopHold( 2 );

    State().SetAcquire( 0 );
    State().AddAsOwner( 0 );
    State().StartHold( 0 );

    //  validate immutable properties are not broken

    AssertValid();

    //  release any quiesced shared latches

    if ( cwBI & 0x00007FFF )
    {
        //  in the event we have to release a sharer, we can check there is one

        AssertValid( iSharedGroup );

        State().m_semS.Release( cwBI & 0x00007FFF );
    }

    //  release a waiter for the exclusive latch, if any

    if ( cwBI >= 0x00020000 )
    {
        //  in the event we have to release an exclusive waiter, we can check there is one

        AssertValid( iExclusiveGroup );

        State().m_semX.Release();
    }

    //  validate immutable properties are not broken

    AssertValid( iSharedGroup );
}

//  releases the exclusive latch in exchange for a shared latch

inline void CSXWLatch::DowngradeExclusiveLatchToSharedLatch()
{
    //  we had better already have only an exclusive latch

    OSSYNCAssert( FNotOwnSharedLatch() );
    OSSYNCAssert( FOwnExclusiveLatch() );
    OSSYNCAssert( FNotOwnWriteLatch() );

    //  validate immutable properties are not broken

    AssertValid( iExclusiveGroup );

    //  release our exclusive latch and acquire a shared latch

    const ControlWord cwDelta = 0xFFFF0001;
    const ControlWord cwBI = AtomicExchangeAdd( (LONG*)&State().m_cw, cwDelta );

    //  transfer ownership from the exclusive latch to a shared latch

    State().RemoveAsOwner( 1 );
    State().StopHold( 1 );

    State().SetAcquire( 0 );
    State().AddAsOwner( 0 );
    State().StartHold( 0 );

    //  validate immutable properties are not broken

    AssertValid();

    //  release a waiter for the exclusive latch, if any

    if ( cwBI >= 0x00020000 )
    {
        //  in the event we have to release an exclusive waiter, we can check there is one

        AssertValid( iExclusiveGroup );

        State().m_semX.Release();
    }

    //  validate immutable properties are not broken

    AssertValid( iSharedGroup );
}

//  releases the write latch

inline void CSXWLatch::ReleaseWriteLatch()
{
    //  we had better already have only a write latch

    OSSYNCAssert( FNotOwnSharedLatch() );
    OSSYNCAssert( FNotOwnExclusiveLatch() );
    OSSYNCAssert( FOwnWriteLatch() );

    //  validate immutable properties are not broken

    AssertValid( iWriteGroup );

    //  release ownership of the write latch

    State().RemoveAsOwner( 2 );
    State().StopHold( 2 );

    //  stop quiescing shared latches by resetting the fQS flag and release our
    //  exclusive latch

    const ControlWord cwDelta = 0xFFFE8000;
    const ControlWord cwBI = AtomicExchangeAdd( (LONG*)&State().m_cw, cwDelta );

    //  release any quiesced shared latches

    if ( cwBI & 0x00007FFF )
    {
        //  in the event we have to release a sharer, we can check there is one

        AssertValid( iSharedGroup );

        State().m_semS.Release( cwBI & 0x00007FFF );
    }

    //  we can't validate the object here because the client code may have
    //  potentially freed this object after the release

    //AssertValid();

    //  release a waiter for the exclusive latch, if any

    if ( cwBI >= 0x00020000 )
    {
        //  in the event we have to release an exclusive waiter, we can check there is one

        AssertValid( iExclusiveGroup );

        State().m_semX.Release();
    }

    //  we can't validate the object here because the client code may have
    //  potentially freed this object after the release

    //AssertValid();
}

//  releases the exclusive latch

inline void CSXWLatch::ReleaseExclusiveLatch()
{
    //  we had better already have only an exclusive latch

    OSSYNCAssert( FOwnExclusiveLatch() );
    OSSYNCAssert( FNotOwnWriteLatch() );

    //  validate immutable properties are not broken

    AssertValid( iExclusiveGroup );

    //  release ownership of the exclusive latch

    State().RemoveAsOwner( 1 );
    State().StopHold( 1 );

    //  release our exclusive latch

    const ControlWord cwDelta = 0xFFFF0000;
    const ControlWord cwBI = AtomicExchangeAdd( (LONG*)&State().m_cw, cwDelta );

    //  release a waiter for the exclusive latch, if any

    if ( cwBI >= 0x00020000 )
    {
        //  in the event we have to release an exclusive waiter, we can check there is one

        AssertValid( iExclusiveGroup );

        State().m_semX.Release();
    }

    //  we can't validate the object here because the client code may have
    //  potentially freed this object after the release

    //AssertValid();
}

//  releases a shared latch

inline void CSXWLatch::ReleaseSharedLatch()
{
    //  we had better already have only a shared latch

    OSSYNCAssert( FOwnSharedLatch() );
    OSSYNCAssert( FNotOwnExclusiveLatch() );
    OSSYNCAssert( FNotOwnWriteLatch() );

    //  we are no longer an owner of a shared latch

    State().RemoveAsOwner( 0 );
    State().StopHold( 0 );

    //  try forever until we successfully change the latch state

    OSSYNC_FOREVER
    {
        //  validate immutable properties are not broken

        AssertValid( iSharedGroup );

        //  read the current state of the control word as our expected before image

        ControlWord cwBIExpected = State().m_cw;

        //  change the expected before image so that the transaction will only work if
        //  shared latches are not quiesced

        cwBIExpected = cwBIExpected & 0xFFFF7FFF;

        //  compute the after image of the control word by performing the transform that
        //  will release our shared latch

        const ControlWord cwAI = cwBIExpected + 0xFFFFFFFF;

        //  attempt to perform the transacted state transition on the control word

        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

        //  the transaction failed

        if ( cwBI != cwBIExpected )
        {
            //  the transaction failed because shared latches were quiesced

            if ( cwBI & 0x00008000 )
            {
                //  if latches were quiesced, there must be someone waiting for the write latch

                AssertValid( iWriteGroup ); // note we're not asserting our latch, but that someone else is entering the write latch state

                //  leave the latch as a quiesced shared latch

                _UpdateQuiescedSharedLatchCount( 0xFFFFFFFF );

                //  we're done

                break;
            }

            //  the transaction failed because another context changed the control word

            else
            {
                //  try again

                continue;
            }
        }

        //  the transaction succeeded

        else
        {
            // we're done

            break;
        }
    }

    //  we can't validate the object here because the client code may have
    //  potentially freed this object after the release

    //AssertValid();
}

//  releases a latch of a given type/group

inline void CSXWLatch::ReleaseLatch( const iGroup igroup )
{
    switch ( igroup )
    {
        case iSharedGroup:
            ReleaseSharedLatch();
            break;

        case iExclusiveGroup:
            ReleaseExclusiveLatch();
            break;

        case iWriteGroup:
            ReleaseWriteLatch();
            break;

        case iNoGroup:
            //  no-op.
            break;

        default:
            OSSYNCAssert( fFalse ); //  unhandled case.
            break;
    }
}

//  waits for ownership of a shared latch in response to receiving an
//  errWaitForSharedLatch from the API.  this function must not be called at any
//  other time

inline void CSXWLatch::WaitForSharedLatch()
{
    //  check for deadlock

    State().AssertCanBeWaiter();

    //  we had better already be declared a waiter

    OSSYNCAssert( State().FWaiter( 0 ) );
    AssertValid( iSharedGroup );

    //  wait for ownership of a shared latch on the shared latch semaphore

    State().m_semS.Acquire();

    State().StopWait( 0 );
    State().RemoveAsWaiter( 0 );

    State().SetAcquire( 0 );
    State().AddAsOwner( 0 );
    State().StartHold( 0 );

    //  validate immutable properties are not broken

    AssertValid( iSharedGroup );
}

//  waits for ownership of the exclusive latch in response to receiving an
//  errWaitForExclusiveLatch from the API.  this function must not be called at any
//  other time

inline void CSXWLatch::WaitForExclusiveLatch()
{
    //  check for deadlock

    State().AssertCanBeWaiter();

    //  we had better already be declared a waiter

    OSSYNCAssert( State().FWaiter( 1 ) );
    AssertValid( iExclusiveGroup );

    //  wait for ownership of the exclusive latch on the exclusive latch semaphore

    State().m_semX.Acquire();

    State().StopWait( 1 );
    State().RemoveAsWaiter( 1 );

    State().SetAcquire( 1 );
    State().AddAsOwner( 1 );
    State().StartHold( 1 );

    //  validate immutable properties are not broken

    AssertValid( iExclusiveGroup );
}

//  waits for ownership of the write latch in response to receiving an
//  errWaitForWriteLatch from the API.  this function must not be called at any
//  other time

inline void CSXWLatch::WaitForWriteLatch()
{
    //  check for deadlock

    State().AssertCanBeWaiter();

    //  we had better already be declared a waiter

    OSSYNCAssert( State().FWaiter( 2 ) );
    AssertValid( iWriteGroup );

    //  wait for ownership of the write latch on the write latch semaphore

    State().m_semW.Acquire();

    State().StopWait( 2 );
    State().RemoveAsWaiter( 2 );

    State().SetAcquire( 2 );
    State().AddAsOwner( 2 );
    State().StartHold( 2 );

    //  validate immutable properties are not broken

    AssertValid( iWriteGroup );
}

//  Waits for the shared latch to be granted.

inline void CSXWLatch::AcquireSharedLatch()
{
    //  we had better not already have a shared, exclusive, or write latch

    OSSYNCAssert( FNotOwner() );

    //  try to get the shared latch and wait if necessary

    ERR err = ErrAcquireSharedLatch();

    if ( ERR::errWaitForSharedLatch == err )
    {
        WaitForSharedLatch();
        err = ERR::errSuccess;
    }
    OSSYNCAssert( ERR::errSuccess == err );

}

//  Waits for the exclusive latch to be granted.

inline void CSXWLatch::AcquireExclusiveLatch()
{
    //  we had better not already have a shared, exclusive, or write latch

    OSSYNCAssert( FNotOwner() );

    //  try to get the exclusive latch and wait if necessary

    ERR err = ErrAcquireExclusiveLatch();

    if ( ERR::errWaitForExclusiveLatch == err )
    {
        WaitForExclusiveLatch();
        err = ERR::errSuccess;
    }
    OSSYNCAssert( ERR::errSuccess == err );

}

//  Waits for the write latch to be granted.

inline void CSXWLatch::AcquireWriteLatch()
{
    //  we had better not already have a shared, exclusive, or write latch

    OSSYNCAssert( FNotOwner() );

    //  grab the exclusive latch

    AcquireExclusiveLatch();

    //  upgrade to a write latch

    UpgradeExclusiveLatchToWriteLatch();
}


//  claims ownership of the latch for the specified group for deadlock detection

inline void CSXWLatch::ClaimOwnership( const DWORD group )
{
    //  this validates that _someone_ (not necessarily us) has the given
    //  latch that we're about to claim in retail pre-RTM flag turned on.
// This doesn't build in test:appetite, dunno why ...
//#if ( !DEBUG && DEBUGGER_EXTENSION && !RTM )
#ifndef DEBUG
#ifdef DEBUGGER_EXTENSION
#ifndef RTM
    switch ( group )
    {
        case iSharedGroup:
        {
            // Unfortunately due to a concurrency issue in a writer installing themselves as a writer
            // and making CSharers(see this function description above) temporarily empty this assert
            // can go off:
            //  OSSYNCEnforce( FSharedLatched() );
            // This assert() would fix it, but be weaker.
            //  OSSYNCEnforce( FSharedLatched() || State().m_cw & 0x00018000 );
            // So we'll for now do the ridiculous thing, and busy wait for the latch to be shared, and 
            // Assert() if it didn't become shared w/in 2 minutes.
            DWORD tick = DwOSSyncITickTime();
            while ( !FSharedLatched() )
            {
                OSSYNCAssertSzRTL( ( DwOSSyncITickTime() - tick ) < 2 * 60 * 1000, "FSharedLatched() should've been true by now." );
            }
        }
            break;
        case iExclusiveGroup:
            OSSYNCEnforce( FExclusiveLatched() );
            break;
        case iWriteGroup:
            OSSYNCEnforce( FWriteLatched() );
            break;
        default:
            OSSYNCAssertSzRTL( fFalse, "Unknown group!" );
    }
//#endif // !RTM && DEBUGGER_EXTENSION && !DEBUG
#endif // !RTM
#endif // DEBUGGER_EXTENSION
#endif // !DEBUG
    State().AddAsOwner( group );
}

//  releases ownership of the latch for the specified group for deadlock detection

inline void CSXWLatch::ReleaseOwnership( const DWORD group )
{
    //  this validates that _someone_ (not necessarily us) has the given
    //  latch that we're about to release in retail pre-RTM flag turned on.
//#if ( !DEBUG && DEBUGGER_EXTENSION && !RTM )
#ifndef DEBUG
#ifdef DEBUGGER_EXTENSION
#ifndef RTM
    switch ( group )
    {
        case iSharedGroup:
            OSSYNCEnforce( FSharedLatched() );
            break;
        case iExclusiveGroup:
            OSSYNCEnforce( FExclusiveLatched() );
            break;
        case iWriteGroup:
            OSSYNCEnforce( FWriteLatched() );
            break;
        default:
            OSSYNCAssertSzRTL( fFalse, "Unknown group!" );
    }
//#endif // !RTM && DEBUGGER_EXTENSION && !DEBUG
#endif // !RTM
#endif // DEBUGGER_EXTENSION
#endif // !DEBUG
    State().RemoveAsOwner( group );
}

//  updates the quiesced shared latch count, possibly releasing a waiter for
//  the write latch

inline void CSXWLatch::_UpdateQuiescedSharedLatchCount( const DWORD cQSDelta )
{
    //  update the quiesced shared latch count using the provided delta

    const DWORD cQSBI = AtomicExchangeAdd( (LONG*)&State().m_cQS, cQSDelta );
    const DWORD cQSAI = cQSBI + cQSDelta;

    //  our update resulted in a zero quiesced shared latch count

    if ( !cQSAI )
    {
        //  in the event we have to release a writer, we can check there is one

        AssertValid( iWriteGroup );

        //  release the waiter for the write latch

        State().m_semW.Release();
    }
}

template< typename Ret, typename FuncType, typename... ArgTypes >
class CInitOnce
{
 public:
    CInitOnce()
      : m_semInit( CSyncBasicInfo( "CInitOnce::m_semInit" ) ),
        m_sigInit( CSyncBasicInfo( "CInitOnce::m_sigInit" ) )
    {
        m_semInit.Release();
    }

    BOOL FIsInit() const { return m_sigInit.FIsSet(); }

    Ret Init( FuncType func, ArgTypes... args )
    {
        if ( m_sigInit.FTryWait() )
        {
        }
        else if ( m_semInit.FTryAcquire() )
        {
            m_ret = func( args... );
            m_sigInit.Set();
        }
        else
        {
            m_sigInit.Wait();
        }

        return m_ret;
    }

    VOID Reset()
    {
        // In case Init was never called
        m_semInit.FTryAcquire();
        m_semInit.Release();
        m_sigInit.Reset();
    }

 private:
    CSemaphore          m_semInit;
    CManualResetSignal  m_sigInit;
    Ret                 m_ret;
};

//  init sync subsystem

BOOL OSSYNCAPI FOSSyncPreinit();

//  enable/disable timeout deadlock detection

void OSSYNCAPI OSSyncConfigDeadlockTimeoutDetection( const BOOL fEnable );

//  signal the sync subsystem that the process is terminating unexpectedly

void OSSYNCAPI OSSyncProcessAbort();

//  terminate sync subsystem

void OSSYNCAPI OSSyncPostterm();


//  special init/term API's for Enhanced State only

const BOOL OSSYNCAPI FOSSyncInitForES();
void OSSYNCAPI OSSyncTermForES();


//  Thread Wait Notifications

typedef void ( OSSYNCAPI *PfnThreadWait )();

void OSSYNCAPI OSSyncOnThreadWaitBegin( PfnThreadWait pfn );
void OSSYNCAPI OSSyncOnThreadWaitEnd( PfnThreadWait pfn );

#endif // !DISABLE_NOT_YET_PORTED

NAMESPACE_END( OSSYNC );

using namespace OSSYNC;


#endif  //  _SYNC_HXX_INCLUDED

