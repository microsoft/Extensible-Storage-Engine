// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _SYNC_HXX_INCLUDED
#define _SYNC_HXX_INCLUDED


#define SYNC_USE_X86_ASM
#ifdef SYNC_ANALYZE_PERFORMANCE
#define SYNC_DUMP_PERF_DATA
#endif


#if defined( DEBUG ) && ( !defined( DBG ) || !defined( OFFICIAL_BUILD ) ) && !defined( MINIMAL_FUNCTIONALITY )
#define SYNC_DEADLOCK_DETECTION
#define SYNC_VALIDATE_IRKSEM_USAGE
#endif

#include "cc.hxx"

#ifdef _MSC_VER
#include <intrin.h>
#endif

#include <limits.h>
#include <new>

#include <stdarg.h>
#include <stdlib.h>


#ifdef _MSC_VER
#define OSSYNCAPI __stdcall
#else
#define OSSYNCAPI
#endif


#include "cc.hxx"



void __stdcall AssertFail( const char * szMessage, const char * szFilename, LONG lLine, ... );



#ifdef OSSYNCAssertSzRTL
#else
#define OSSYNCAssertSzRTL( exp, sz )    ( ( exp ) ? (void) 0 : AssertFail( sz, __FILE__, __LINE__ ) )
#endif

#ifdef OSSYNCAssertSz
#else
#ifdef DEBUG
#define OSSYNCAssertSz( exp, sz )       OSSYNCAssertSzRTL( exp, sz )
#else
#define OSSYNCAssertSz( exp, sz )
#endif
#endif


#ifdef OSSYNCAssertRTL
#else
#define OSSYNCAssertRTL( exp )          OSSYNCAssertSzRTL( exp, #exp )
#endif
#ifdef OSSYNCAssert
#else
#define OSSYNCAssert( exp )             OSSYNCAssertSz( exp, #exp )
#endif




void OSSYNCAPI EnforceFail( const char* szMessage, const char* szFilename, LONG lLine );



#define OSSYNCEnforceSz( exp, sz )      ( ( exp ) ? (void) 0 : EnforceFail( sz, __FILE__, __LINE__ ) )


#define OSSYNCEnforce( exp )            OSSYNCEnforceSz( exp, #exp )

#ifdef SYNC_VALIDATE_IRKSEM_USAGE
#define OSSYNCEnforceIrksem( exp, sz )  OSSYNCEnforceSz( exp, sz )
#else
#define OSSYNCEnforceIrksem( exp, sz )
#endif



#if defined( _M_IX86 ) || defined( _M_AMD64 )
inline void OSSyncPause() { _mm_pause(); }
#elif defined( _M_ARM ) || defined( _M_ARM64 )
inline void OSSyncPause() { __yield(); }
#else
inline void OSSyncPause() {};
#endif

#ifdef DEBUG
#define OSSYNC_FOREVER for ( INT cLoop = 0; ; cLoop++, OSSyncPause() )
#else
#define OSSYNC_FOREVER for ( ; ; OSSyncPause() )
#endif

#ifdef DEBUG
#define OSSYNC_INNER_FOREVER for ( INT cInnerLoop = 0; ; cInnerLoop++, OSSyncPause() )
#else
#define OSSYNC_INNER_FOREVER for ( ; ; OSSyncPause() )
#endif



#ifdef _MSC_VER

extern "C" {
#ifdef _M_IX86
#else
    void* __cdecl _InterlockedExchangePointer( void* volatile *Target, void* Value );
    void* __cdecl _InterlockedCompareExchangePointer( void* volatile *_Destination, void* _Exchange, void* _Comparand );

    #pragma intrinsic( _InterlockedExchangePointer )
    #pragma intrinsic( _InterlockedCompareExchangePointer )
#endif
}

#else


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

#endif


#define NAMESPACE_BEGIN(x) namespace x {
#define NAMESPACE_END(x) }

NAMESPACE_BEGIN( OSSYNC );


class CDumpContext;



class COwner;
class CLockDeadlockDetectionInfo;

struct CLS
{
#ifdef SYNC_DEADLOCK_DETECTION

    COwner*                     pownerLockHead;
    DWORD                       cDisableOwnershipTracking;
    DWORD                       cOverrideDeadlock;
    DWORD                       cDisableDeadlockDetection;
    DWORD                       cDisableLockCheckOnApiExit;

    CLockDeadlockDetectionInfo* plddiLockWait;
    DWORD                       groupLockWait;

#endif
};


CLS* const OSSYNCAPI Pcls();




INT OSSYNCAPI OSSyncGetProcessorCountMax();


INT OSSYNCAPI OSSyncGetProcessorCount();


INT OSSYNCAPI OSSyncGetCurrentProcessor();


void OSSYNCAPI OSSyncSetCurrentProcessor( const INT iProc );




BOOL OSSYNCAPI FOSSyncConfigureProcessorLocalStorage( const size_t cbPLS );


void* OSSYNCAPI OSSyncGetProcessorLocalStorage();


void* OSSYNCAPI OSSyncGetProcessorLocalStorage( const size_t iProc );


#ifdef SYNC_ANALYZE_PERFORMANCE



QWORD OSSYNCAPI QwOSSyncIHRTFreq();


QWORD OSSYNCAPI QwOSSyncIHRTCount();

#endif


#ifndef RTM



DWORD OSSYNCAPI DwOSSyncITickTime();

#endif




extern const INT cmsecTest;


extern const INT cmsecInfinite;


extern const INT cmsecDeadlock;


extern const INT cmsecInfiniteNoDeadlock;


extern const INT cbCacheLine;


extern BOOL g_fSyncProcessAbort;




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

inline LONG AtomicExchange( LONG* const plTarget, const LONG lValue )
{
    OSSYNCAssert( IsAtomicallyModifiable( plTarget ) );

    return _InterlockedExchange( plTarget, lValue );
}

inline void* AtomicExchangePointer( void** const ppvTarget, const void* const pvValue )
{
    OSSYNCAssert( IsAtomicallyModifiablePointer( ppvTarget ) );

#ifndef _WIN64
    return (void *)(DWORD_PTR)AtomicExchange( (LONG* const)ppvTarget, (const LONG)(LONG_PTR)pvValue );
#else
    return _InterlockedExchangePointer( ppvTarget, (void*)pvValue );
#endif
}

inline __int64 AtomicRead( __int64 * const pi64Target );
inline __int64 AtomicCompareExchange( _Inout_ volatile __int64 * const pi64Target, const __int64 i64Initial, const __int64 i64Final );
inline __int64 AtomicExchange( _Inout_ volatile __int64 * const pi64Target, const __int64 i64Value )
{
    OSSYNCAssert( IsAtomicallyModifiablePointer( (void *const *)pi64Target ) );
    __int64 i64Initial = AtomicRead( (__int64 * const)pi64Target );

    OSSYNC_FOREVER
    {
        if ( i64Initial == AtomicCompareExchange( pi64Target, i64Initial, i64Value ) )
        {
            break;
        }

        i64Initial = AtomicRead( (__int64 * const)pi64Target );
    }

    return i64Initial;
}

inline LONG AtomicRead( LONG * const plTarget )
{
    OSSYNCAssert( IsAtomicallyModifiable( plTarget ) );
    return *( (volatile LONG *)plTarget );
}

inline ULONG AtomicRead( ULONG * const pulTarget )
{
    return ( ULONG )AtomicRead( ( LONG *)pulTarget );
}

inline __int64 AtomicRead( __int64 * const pi64Target )
{
    OSSYNCAssert( IsAtomicallyModifiablePointer( (void *const *)pi64Target ) );
#ifndef _WIN64
    return AtomicCompareExchange( (volatile __int64 * const)pi64Target, 0, 0 );
#else
    return *( (volatile __int64 *)pi64Target );
#endif
}

inline unsigned __int64 AtomicRead( unsigned __int64 * const pui64Target )
{
    return ( unsigned __int64 ) AtomicRead( (__int64 *) pui64Target );
}

inline void* AtomicReadPointer( void** const ppvTarget )
{
#ifndef _WIN64
    return (void*)AtomicRead( (LONG*)ppvTarget );
#else
    return (void*)AtomicRead( (__int64*)ppvTarget );
#endif
}

inline LONG AtomicExchangeAdd( LONG * const plTarget, const LONG lValue )
{
    OSSYNCAssert( IsAtomicallyModifiable( plTarget ) );

    return _InterlockedExchangeAdd( plTarget, lValue );
}

inline SHORT AtomicExchangeAdd( SHORT * const psTarget, const SHORT sValue )
{
    OSSYNCAssert( IsAtomicallyModifiable( psTarget ) );

    return _InterlockedExchangeAdd16( psTarget, sValue );
}

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
    return (void *)(DWORD_PTR)AtomicCompareExchange( (LONG* const)ppvTarget, (const LONG)(LONG_PTR)pvInitial, (const LONG)(LONG_PTR)pvFinal );
#else
    return _InterlockedCompareExchangePointer( ppvTarget, const_cast< void* >( pvFinal ), const_cast< void* >( pvInitial ) );
#endif
}

inline __int64 AtomicCompareExchange( _Inout_ volatile __int64 * const pi64Target, const __int64 i64Initial, const __int64 i64Final )
{
    OSSYNCAssert( IsAtomicallyModifiablePointer( (void *const *)pi64Target ) );

    return _InterlockedCompareExchange64( pi64Target, i64Final, i64Initial );
}

inline void AtomicAddLinklistNode( void* const pvNode, void** const ppvNext, void* volatile* const ppvHead )
{
    OSSYNCAssert( IsAtomicallyModifiablePointer( ppvNext ) );
    OSSYNCAssert( IsAtomicallyModifiablePointer( (void *const *)ppvHead ) );
    
    OSSYNC_FOREVER
    {
        void* const pvHead = AtomicReadPointer( (void** const)ppvHead );
        *( void* volatile* )ppvNext = pvHead;

        const void* const pvResult = AtomicCompareExchangePointer( (void** const)ppvHead, pvHead, pvNode );
        if ( pvResult == pvHead )
        {
            break;
        }
    }
}

inline void* PvAtomicRetrieveLinklist( void* volatile* const ppvHead )
{
    OSSYNCAssert( IsAtomicallyModifiablePointer( (void *const *)ppvHead ) );

    return AtomicExchangePointer( (void **const)ppvHead, NULL );
}



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

inline QWORD AtomicExchangeMax( QWORD * const pqwTarget, const QWORD qwValue )
{
    QWORD qwInitial;

    OSSYNC_FOREVER
    {
        qwInitial = AtomicRead( (__int64*)pqwTarget );

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


inline QWORD AtomicAdd( QWORD* const pqwTarget, const QWORD qwValue )
{
#ifdef _WIN64
    return (QWORD)AtomicExchangeAddPointer( (VOID **)pqwTarget, (VOID *)qwValue ) + qwValue;
#else
    QWORD qwInitial = (QWORD)AtomicCompareExchange( (volatile __int64 * const)pqwTarget, 0, 0 );

    OSSYNC_FOREVER
    {
        const QWORD qwFinal = qwInitial + qwValue;

        const QWORD qwActual = (QWORD)AtomicCompareExchange( (volatile __int64 * const)pqwTarget, (const __int64)qwInitial, (const __int64)qwFinal );
        
        if ( qwInitial == qwActual )
        {
            return qwFinal;
        }

        qwInitial = qwActual;
    }
#endif
}



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



void* OSSYNCAPI ESMemoryNew( size_t cb );
void OSSYNCAPI ESMemoryDelete( void* pv );


#if defined( SYNC_ANALYZE_PERFORMANCE ) || defined( SYNC_DEADLOCK_DETECTION )
#define SYNC_ENHANCED_STATE
#endif

template< class CState, class CStateInit, class CInformation, class CInformationInit >
class CEnhancedStateContainer
{
    public:



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



        CEnhancedStateContainer( const CStateInit& si, const CInformationInit& ii )
        {
#ifdef SYNC_ENHANCED_STATE

            m_pes = new CEnhancedState( si, ii );

#else

            new( (CState*) m_rgbState ) CState( si );

#endif
        }

        ~CEnhancedStateContainer()
        {
#ifdef SYNC_ENHANCED_STATE

            delete m_pes;
#ifdef DEBUG
            m_pes = NULL;
#endif

#else

            ( (CState*) m_rgbState )->~CState();

#endif
        }


        CEnhancedState& State() const
        {
#ifdef SYNC_ENHANCED_STATE

            return *m_pes;

#else


            return *( (CEnhancedState*) m_rgbState );

#endif
        }


        void Dump( const CDumpContext& dc ) const;

    private:



        union
        {
            CEnhancedState* m_pes;
            BYTE            m_rgbState[ sizeof( CState ) ];
        };



    private:

        CEnhancedStateContainer( const CEnhancedStateContainer& state ) = delete;
};



class CSyncObject
{
    public:



        CSyncObject() {}
        ~CSyncObject() {}

    private:



        CSyncObject& operator=( CSyncObject& ) = delete;
};



class CSyncBasicInfo
{
    public:



#ifdef SYNC_ENHANCED_STATE

        CSyncBasicInfo( const char* szInstanceName );
        ~CSyncBasicInfo();

#else

        CSyncBasicInfo( const char* szInstanceName ) {}
        ~CSyncBasicInfo() {}

#endif


        void SetTypeName( const char* szTypeName );
        void SetInstance( const CSyncObject* const psyncobj );


#ifdef SYNC_ENHANCED_STATE

        const char* SzInstanceName() const { return m_szInstanceName; }
        const char* SzTypeName() const { return m_szTypeName; }
        const CSyncObject* const Instance() const { return m_psyncobj; }

#endif


        void Dump( const CDumpContext& dc ) const;

    private:



        CSyncBasicInfo& operator=( CSyncBasicInfo& ) = delete;


#ifdef SYNC_ENHANCED_STATE


        const char*     m_szInstanceName;


        const char*     m_szTypeName;


        const CSyncObject*  m_psyncobj;

#endif
};


inline void CSyncBasicInfo::SetTypeName( const char* szTypeName )
{
#ifdef SYNC_ENHANCED_STATE

    m_szTypeName = szTypeName;

#endif
}


inline void CSyncBasicInfo::SetInstance( const CSyncObject* const psyncobj )
{
#ifdef SYNC_ENHANCED_STATE

    m_psyncobj = psyncobj;

#endif
}



class CSyncPerfWait
{
    public:



        CSyncPerfWait();
        ~CSyncPerfWait();



        void StartWait();
        void StopWait();


#ifdef SYNC_ANALYZE_PERFORMANCE

        QWORD   CWaitTotal() const      { return m_cWait; }
        double  CsecWaitElapsed() const { return    (double)(signed __int64)m_qwHRTWaitElapsed /
                                                    (double)(signed __int64)QwOSSyncIHRTFreq(); }

#endif


        void Dump( const CDumpContext& dc ) const;

    private:



        CSyncPerfWait& operator=( CSyncPerfWait& ) = delete;


#ifdef SYNC_ANALYZE_PERFORMANCE


        volatile QWORD m_cWait;


        volatile QWORD m_qwHRTWaitElapsed;

#endif
};


inline void CSyncPerfWait::StartWait()
{
#ifdef SYNC_ANALYZE_PERFORMANCE


    AtomicAdd( (QWORD*)&m_cWait, 1 );


    AtomicAdd( (QWORD*)&m_qwHRTWaitElapsed, QWORD( -__int64( QwOSSyncIHRTCount() ) ) );

#endif
}


inline void CSyncPerfWait::StopWait()
{
#ifdef SYNC_ANALYZE_PERFORMANCE


    AtomicAdd( (QWORD*)&m_qwHRTWaitElapsed, QwOSSyncIHRTCount() );

#endif
}



class CSyncStateInitNull
{
};

extern const CSyncStateInitNull syncstateNull;



class CKernelSemaphoreInfo
    :   public CSyncBasicInfo,
        public CSyncPerfWait
{
    public:



        CKernelSemaphoreInfo( const CSyncBasicInfo& sbi )
            :   CSyncBasicInfo( sbi )
        {
        }


        void Dump( const CDumpContext& dc ) const;
};



class CKernelSemaphoreState
{
    public:



        CKernelSemaphoreState( const CSyncStateInitNull& null ) : m_handle( 0 ) {}


        void SetHandle( void * handle ) { m_handle = handle; }


        void* Handle() { return m_handle; }


        void Dump( const CDumpContext& dc ) const;

    private:



        CKernelSemaphoreState& operator=( CKernelSemaphoreState& ) = delete;



        void* m_handle;
};



class CKernelSemaphore
    :   private CSyncObject,
        private CEnhancedStateContainer< CKernelSemaphoreState, CSyncStateInitNull, CKernelSemaphoreInfo, CSyncBasicInfo >
{
    public:



        CKernelSemaphore( const CSyncBasicInfo& sbi );
        ~CKernelSemaphore();


        const BOOL FInit();
        void Term();


        void Acquire();
        const BOOL FTryAcquire();
        const BOOL FAcquire( const INT cmsecTimeout );
        void Release( const INT cToRelease = 1 );


        const BOOL FReset();


        void Dump( const CDumpContext& dc ) const;

    private:



        CKernelSemaphore& operator=( CKernelSemaphore& ) = delete;


        const BOOL FInitialized();
};


inline void CKernelSemaphore::Acquire()
{

    OSSYNCAssert( FInitialized() );


    const BOOL fAcquire = FAcquire( cmsecInfinite );
    OSSYNCAssert( fAcquire );
}


inline const BOOL CKernelSemaphore::FTryAcquire()
{

    OSSYNCAssert( FInitialized() );


    return FAcquire( cmsecTest );
}


inline const BOOL CKernelSemaphore::FReset()
{

    OSSYNCAssert( FInitialized() );


    return !FTryAcquire();
}


inline const BOOL CKernelSemaphore::FInitialized()
{
    return State().Handle() != 0;
}



class CKernelSemaphorePool
{
    public:



        typedef USHORT IRKSEM;
        enum { irksemAllocated = 0xFFFE, irksemNil = 0xFFFF };



        CKernelSemaphorePool();
        ~CKernelSemaphorePool();


        const BOOL FInit();
        void Term();


        const IRKSEM Allocate( const CSyncObject* const psyncobj );
        void Reference( const IRKSEM irksem );
        void Unreference( const IRKSEM irksem );


        CKernelSemaphore& Ksem( const IRKSEM irksem, const CSyncObject* const psyncobj ) const;

        const BOOL FInitialized() const;

        LONG CksemAlloc() const { return m_cksem; }

    private:



        class CReferencedKernelSemaphore
            :   public CKernelSemaphore
        {
            public:



                CReferencedKernelSemaphore();
                ~CReferencedKernelSemaphore();


                const BOOL FInit();
                void Term();


                BOOL FAllocate();
                void Release();

                void SetUser( const OSSYNC::CSyncObject* const psyncobj );

                void Reference();
                const BOOL FUnreference();


                const BOOL FInUse() const { return m_fInUse; }
                const INT CReference() const { return m_cReference; }

#ifdef SYNC_VALIDATE_IRKSEM_USAGE
                const OSSYNC::CSyncObject* const PsyncobjUser() const { return m_psyncobjUser; }
#endif

            private:



                CReferencedKernelSemaphore& operator=( CReferencedKernelSemaphore& ) = delete;



                union
                {
                    volatile LONG       m_l;
                    struct
                    {
                        volatile USHORT m_cReference:15;
                        volatile USHORT m_fInUse:1;
                    };
                };

                volatile LONG       m_fAvailable;

#ifdef SYNC_VALIDATE_IRKSEM_USAGE


                const OSSYNC::CSyncObject* volatile m_psyncobjUser;

#endif
        };



        CKernelSemaphorePool& operator=( CKernelSemaphorePool& ) = delete;


        const IRKSEM AllocateNew();
        void Free( const IRKSEM irksem );



        CReferencedKernelSemaphore*     m_mpirksemrksem;


        volatile LONG                   m_cksem;
};


inline const CKernelSemaphorePool::IRKSEM CKernelSemaphorePool::Allocate( const CSyncObject* const psyncobj )
{

    OSSYNCAssert( FInitialized() );


    IRKSEM irksem = irksemNil;
    if ( m_cksem )
    {

        IRKSEM irksemHash = IRKSEM( ( DWORD_PTR( &irksem ) + DWORD_PTR( &irksem ) / 65536 ) % m_cksem );
        OSSYNCAssert( irksemHash >= 0 && irksemHash < m_cksem );


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


    if ( irksem == irksemNil )
    {
        irksem = AllocateNew();
    }


    OSSYNCAssert( irksem != irksemNil );
    OSSYNCAssert( irksem >= 0 );
    OSSYNCAssert( irksem < m_cksem );


    m_mpirksemrksem[irksem].SetUser( psyncobj );


    OSSYNCEnforceIrksem(    m_mpirksemrksem[irksem].FReset(),
                            "Illegal allocation of a Kernel Semaphore with available counts!"  );


    return irksem;
}


inline void CKernelSemaphorePool::Reference( const IRKSEM irksem )
{

    OSSYNCAssert( irksem != irksemNil );
    OSSYNCAssert( irksem >= 0 );
    OSSYNCAssert( irksem < m_cksem );


    OSSYNCAssert( FInitialized() );


    m_mpirksemrksem[irksem].Reference();
}


inline void CKernelSemaphorePool::Unreference( const IRKSEM irksem )
{

    OSSYNCAssert( irksem != irksemNil );
    OSSYNCAssert( irksem >= 0 );
    OSSYNCAssert( irksem < m_cksem );


    OSSYNCAssert( FInitialized() );


    const BOOL fFree = m_mpirksemrksem[irksem].FUnreference();


    if ( fFree )
    {

        Free( irksem );
    }
}


inline CKernelSemaphore& CKernelSemaphorePool::Ksem( const IRKSEM irksem, const CSyncObject* const psyncobj ) const
{

    OSSYNCAssert( irksem != irksemNil );
    OSSYNCAssert( irksem >= 0 );
    OSSYNCAssert( irksem < m_cksem );


    OSSYNCAssert( FInitialized() );


    OSSYNCEnforceIrksem(    m_mpirksemrksem[irksem].PsyncobjUser() == psyncobj,
                            "Illegal use of a Kernel Semaphore by another Synchronization Object"  );


    return m_mpirksemrksem[irksem];
}


inline const BOOL CKernelSemaphorePool::FInitialized() const
{
    return m_mpirksemrksem != NULL;
}


inline void CKernelSemaphorePool::Free( const IRKSEM irksem )
{

    OSSYNCAssert( irksem != irksemNil );
    OSSYNCAssert( irksem >= 0 );
    OSSYNCAssert( irksem < m_cksem );


    OSSYNCAssert( FInitialized() );


    OSSYNCEnforceIrksem(    !m_mpirksemrksem[irksem].FInUse(),
                            "Illegal free of a Kernel Semaphore that is still in use"  );


    OSSYNCEnforceIrksem(    !m_mpirksemrksem[irksem].FAllocate(),
                            "Illegal free of a Kernel Semaphore that is already free"  );


    OSSYNCEnforceIrksem(    m_mpirksemrksem[irksem].FReset(),
                            "Illegal free of a Kernel Semaphore that has available counts"  );


    m_mpirksemrksem[irksem].Release();
}




inline BOOL CKernelSemaphorePool::CReferencedKernelSemaphore::FAllocate()
{
    return m_fAvailable && AtomicExchange( (LONG*)&m_fAvailable, 0 );
}


inline void CKernelSemaphorePool::CReferencedKernelSemaphore::Release()
{
    AtomicExchange( (LONG*)&m_fAvailable, 1 );
}


inline void CKernelSemaphorePool::CReferencedKernelSemaphore::SetUser( const OSSYNC::CSyncObject* const psyncobj )
{

    OSSYNCEnforceIrksem(    !m_fInUse,
                            "Illegal allocation of a Kernel Semaphore that is already in use" );
    OSSYNCEnforceIrksem(    !m_psyncobjUser,
                            "Illegal allocation of a Kernel Semaphore that is already in use" );


    AtomicExchangeAdd( (LONG*) &m_l, 0x00008001 );
#ifdef SYNC_VALIDATE_IRKSEM_USAGE
    m_psyncobjUser  = psyncobj;
#endif
}


inline void CKernelSemaphorePool::CReferencedKernelSemaphore::Reference()
{

    AtomicIncrement( (LONG*) &m_l );


    OSSYNCAssert( m_cReference > 0 );
}


inline const BOOL CKernelSemaphorePool::CReferencedKernelSemaphore::FUnreference()
{

    OSSYNCAssert( m_cReference > 0 );


    LONG lBI;
    OSSYNC_FOREVER
    {

        const LONG lBIExpected = m_l;


        const LONG lAI =    lBIExpected +
                            (   lBIExpected == 0x00008001 ?
                                    0xFFFF7FFF :
                                    0xFFFFFFFF );


        lBI = AtomicCompareExchange( (LONG*)&m_l, lBIExpected, lAI );


        if ( lBI != lBIExpected )
        {

            continue;
        }


        else
        {

            break;
        }
    }


    if ( lBI == 0x00008001 )
    {
#ifdef SYNC_VALIDATE_IRKSEM_USAGE
        m_psyncobjUser = NULL;
#endif
        return fTrue;
    }
    else
    {
        return fFalse;
    }
}



extern CKernelSemaphorePool g_ksempoolGlobal;



class CSyncPerfAcquire
{
    public:



        CSyncPerfAcquire();
        ~CSyncPerfAcquire();



        void SetAcquire();

        void SetContend();


#ifdef SYNC_ANALYZE_PERFORMANCE

        QWORD   CAcquireTotal() const   { return m_cAcquire; }
        QWORD   CContendTotal() const   { return m_cContend; }

#endif


        void Dump( const CDumpContext& dc ) const;

    private:



        CSyncPerfAcquire& operator=( CSyncPerfAcquire& ) = delete;


#ifdef SYNC_ANALYZE_PERFORMANCE


        volatile QWORD m_cAcquire;


        volatile QWORD m_cContend;

#endif
};


inline void CSyncPerfAcquire::SetAcquire()
{
#ifdef SYNC_ANALYZE_PERFORMANCE

    AtomicAdd( (QWORD*)&m_cAcquire, 1 );

#endif
}


inline void CSyncPerfAcquire::SetContend()
{
#ifdef SYNC_ANALYZE_PERFORMANCE

    AtomicAdd( (QWORD*)&m_cContend, 1 );

#endif
}



class CSemaphoreInfo
    :   public CSyncBasicInfo,
        public CSyncPerfWait,
        public CSyncPerfAcquire
{
    public:



        CSemaphoreInfo( const CSyncBasicInfo& sbi )
            :   CSyncBasicInfo( sbi )
        {
        }


        void Dump( const CDumpContext& dc ) const;
};



class CSemaphoreState
{
    public:



        CSemaphoreState( const CSyncStateInitNull& null ) : m_cAvail( 0 ) {}
        CSemaphoreState( const INT cAvail );
        CSemaphoreState( const INT cWait, const INT irksem );
        CSemaphoreState( const CSemaphoreState& state )
        {
            C_ASSERT( OffsetOf( CSemaphoreState, m_irksem ) == OffsetOf( CSemaphoreState, m_cAvail ) );
            C_ASSERT( OffsetOf( CSemaphoreState, m_cWaitNeg ) > OffsetOf( CSemaphoreState, m_cAvail ) );
            C_ASSERT( ( OffsetOf( CSemaphoreState, m_cWaitNeg ) + sizeof ( m_cWaitNeg ) ) <= ( OffsetOf( CSemaphoreState, m_cAvail ) + sizeof ( m_cAvail ) ) );
            m_cAvail = state.m_cAvail;
        }
        ~CSemaphoreState() {}


        CSemaphoreState& operator=( CSemaphoreState& state ) { m_cAvail = state.m_cAvail;  return *this; }


        const BOOL FChange( const CSemaphoreState& stateCur, const CSemaphoreState& stateNew );
        const BOOL FIncAvail( const INT cToInc );
        const BOOL FDecAvail();


        const BOOL FNoWait() const { return m_cAvail >= 0; }
        const BOOL FWait() const { return m_cAvail < 0; }
        const BOOL FAvail() const { return m_cAvail > 0; }
        const BOOL FNoWaitAndNoAvail() const { return m_cAvail == 0; }

        const INT CAvail() const { OSSYNCAssert( FNoWait() ); return m_cAvail; }
        const INT CWait() const { OSSYNCAssert( FWait() ); return -m_cWaitNeg; }
        const CKernelSemaphorePool::IRKSEM Irksem() const { OSSYNCAssert( FWait() ); return CKernelSemaphorePool::IRKSEM( m_irksem ); }


        void Dump( const CDumpContext& dc ) const;

    private:



        union
        {

            volatile LONG       m_cAvail;


            struct
            {
                volatile USHORT m_irksem;
                volatile SHORT  m_cWaitNeg;
            };
        };
};


inline CSemaphoreState::CSemaphoreState( const INT cAvail )
{

    OSSYNCAssert( cAvail >= 0 );
    OSSYNCAssert( cAvail <= 0x7FFFFFFF );


    m_cAvail = LONG( cAvail );
}


inline CSemaphoreState::CSemaphoreState( const INT cWait, const INT irksem )
{

    OSSYNCAssert( cWait > 0 );
    OSSYNCAssert( cWait <= 0x7FFF );
    OSSYNCAssert( irksem >= 0 );
    OSSYNCAssert( irksem <= 0xFFFE );


    m_cWaitNeg = SHORT( -cWait );


    m_irksem = (USHORT) irksem;
}


inline const BOOL CSemaphoreState::FChange( const CSemaphoreState& stateCur, const CSemaphoreState& stateNew )
{
    return AtomicCompareExchange( (LONG*)&m_cAvail, stateCur.m_cAvail, stateNew.m_cAvail ) == stateCur.m_cAvail;
}


__forceinline const BOOL CSemaphoreState::FIncAvail( const INT cToInc )
{

    OSSYNC_FOREVER
    {

        const LONG cAvail = m_cAvail;


        const LONG cAvailStart = cAvail & 0x7FFFFFFF;


        const LONG cAvailEnd = cAvailStart + cToInc;


        OSSYNCAssert( cAvail < 0 || ( cAvailStart >= 0 && cAvailEnd <= 0x7FFFFFFF && cAvailEnd == cAvailStart + cToInc ) );


        const LONG cAvailOld = AtomicCompareExchange( (LONG*)&m_cAvail, cAvailStart, cAvailEnd );


        if ( cAvailOld == cAvailStart )
        {

            return fTrue;
        }


        else
        {

            if ( cAvailOld >= 0 )
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


__forceinline const BOOL CSemaphoreState::FDecAvail()
{

    OSSYNC_FOREVER
    {

        const LONG cAvail = m_cAvail;


        OSSYNCAssert( cAvail != 0x80000000 );


        const LONG cAvailEnd = ( cAvail - 1 ) & 0x7FFFFFFF;


        const LONG cAvailStart = cAvailEnd + 1;


        OSSYNCAssert( cAvail <= 0 || ( cAvailStart > 0 && cAvailEnd >= 0 && cAvailEnd == cAvail - 1 ) );


        const LONG cAvailOld = AtomicCompareExchange( (LONG*)&m_cAvail, cAvailStart, cAvailEnd );


        if ( cAvailOld == cAvailStart )
        {

            return fTrue;
        }


        else
        {

            if ( cAvailOld > 0 )
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



class CSemaphore
    :   private CSyncObject,
        private CEnhancedStateContainer< CSemaphoreState, CSyncStateInitNull, CSemaphoreInfo, CSyncBasicInfo >
{
    public:



        CSemaphore( const CSyncBasicInfo& sbi );
        ~CSemaphore();


        void Acquire();
        void Wait();
        const BOOL FTryAcquire();
        const BOOL FAcquire( const INT cmsecTimeout );
        const BOOL FWait( const INT cmsecTimeout );
        void Release( const INT cToRelease = 1 );
        void ReleaseAllWaiters();


        const INT CWait() const;
        const INT CAvail() const;


        void Dump( const CDumpContext& dc ) const;

    private:



        CSemaphore& operator=( CSemaphore& ) = delete;


        const BOOL _FAcquire( const INT cmsecTimeout );
        const BOOL _FWait( const INT cmsecTimeout );
        void _Release( const INT cToRelease );
};


inline void CSemaphore::Acquire()
{

    INT fAcquire = FAcquire( cmsecInfinite );
    OSSYNCAssert( fAcquire );
}


inline void CSemaphore::Wait()
{

    INT fWait = FWait( cmsecInfinite );
    OSSYNCAssert( fWait );
}


inline const BOOL CSemaphore::FTryAcquire()
{

    const BOOL fAcquire = State().FDecAvail();


    if ( !fAcquire )
    {

        State().SetContend();
    }


    else
    {

        State().SetAcquire();
    }

    return fAcquire;
}


inline const BOOL CSemaphore::FAcquire( const INT cmsecTimeout )
{

    return FTryAcquire() || _FAcquire( cmsecTimeout );
}


inline const BOOL CSemaphore::FWait( const INT cmsecTimeout )
{

    return ( CAvail() > 0 ) || _FWait( cmsecTimeout );
}


inline void CSemaphore::Release( const INT cToRelease )
{

    if ( !State().FIncAvail( cToRelease ) )
    {

        _Release( cToRelease );
    }
}


inline const INT CSemaphore::CWait() const
{

    const CSemaphoreState stateCur = (CSemaphoreState&) State();


    return stateCur.FWait() ? stateCur.CWait() : 0;
}


inline const INT CSemaphore::CAvail() const
{

    const CSemaphoreState stateCur = (CSemaphoreState&) State();


    return stateCur.FNoWait() ? stateCur.CAvail() : 0;
}



class CAutoResetSignalInfo
    :   public CSyncBasicInfo,
        public CSyncPerfWait,
        public CSyncPerfAcquire
{
    public:



        CAutoResetSignalInfo( const CSyncBasicInfo& sbi )
            :   CSyncBasicInfo( sbi )
        {
        }


        void Dump( const CDumpContext& dc ) const;
};



class CAutoResetSignalState
{
    public:



        CAutoResetSignalState( const CSyncStateInitNull& null ) : m_fSet( 0 ) {}
        CAutoResetSignalState( const INT fSet );
        CAutoResetSignalState( const INT cWait, const INT irksem );
        CAutoResetSignalState( const CAutoResetSignalState& state )
        {
            C_ASSERT( OffsetOf( CAutoResetSignalState, m_irksem ) == OffsetOf( CAutoResetSignalState, m_fSet ) );
            C_ASSERT( OffsetOf( CAutoResetSignalState, m_cWaitNeg ) > OffsetOf( CAutoResetSignalState, m_fSet ) );
            C_ASSERT( ( OffsetOf( CAutoResetSignalState, m_cWaitNeg ) + sizeof ( m_cWaitNeg ) ) <= ( OffsetOf( CAutoResetSignalState, m_fSet ) + sizeof ( m_fSet ) ) );
            m_fSet = state.m_fSet;
        }
        ~CAutoResetSignalState() {}


        CAutoResetSignalState& operator=( CAutoResetSignalState& state ) { m_fSet = state.m_fSet;  return *this; }


        const BOOL FChange( const CAutoResetSignalState& stateCur, const CAutoResetSignalState& stateNew );
        const BOOL FSimpleSet();
        const BOOL FSimpleReset();


        const BOOL FNoWait() const { return m_fSet >= 0; }
        const BOOL FWait() const { return m_fSet < 0; }
        const BOOL FNoWaitAndSet() const { return m_fSet > 0; }
        const BOOL FNoWaitAndNotSet() const { return m_fSet == 0; }

        const BOOL FIsSet() const { return m_fSet > 0; }
        const INT CWait() const { OSSYNCAssert( FWait() ); return -m_cWaitNeg; }
        const CKernelSemaphorePool::IRKSEM Irksem() const { OSSYNCAssert( FWait() ); return CKernelSemaphorePool::IRKSEM( m_irksem ); }


        void Dump( const CDumpContext& dc ) const;

    private:



        union
        {

            volatile LONG       m_fSet;


            struct
            {
                volatile USHORT m_irksem;
                volatile SHORT  m_cWaitNeg;
            };
        };
};


inline CAutoResetSignalState::CAutoResetSignalState( const INT fSet )
{

    OSSYNCAssert( fSet == 0 || fSet == 1 );


    m_fSet = LONG( fSet );
}


inline CAutoResetSignalState::CAutoResetSignalState( const INT cWait, const INT irksem )
{

    OSSYNCAssert( cWait > 0 );
    OSSYNCAssert( cWait <= 0x7FFF );
    OSSYNCAssert( irksem >= 0 );
    OSSYNCAssert( irksem <= 0xFFFE );


    m_cWaitNeg = SHORT( -cWait );


    m_irksem = (USHORT) irksem;
}


inline const BOOL CAutoResetSignalState::FChange( const CAutoResetSignalState& stateCur, const CAutoResetSignalState& stateNew )
{
    return AtomicCompareExchange( (LONG*)&m_fSet, stateCur.m_fSet, stateNew.m_fSet ) == stateCur.m_fSet;
}


__forceinline const BOOL CAutoResetSignalState::FSimpleSet()
{

    OSSYNC_FOREVER
    {

        const LONG fSet = m_fSet;


        const LONG fSetStart = fSet & 0x7FFFFFFF;


        const LONG fSetEnd = 1;


        OSSYNCAssert( fSet < 0 || ( ( fSetStart == 0 || fSetStart == 1 ) && fSetEnd == 1 ) );


        const LONG fSetOld = AtomicCompareExchange( (LONG*)&m_fSet, fSetStart, fSetEnd );


        if ( fSetOld == fSetStart )
        {

            return fTrue;
        }


        else
        {

            if ( fSetOld >= 0 )
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


__forceinline const BOOL CAutoResetSignalState::FSimpleReset()
{

    OSSYNC_FOREVER
    {

        const LONG fSet = m_fSet;


        const LONG fSetStart = fSet & 0x7FFFFFFF;


        const LONG fSetEnd = 0;


        OSSYNCAssert( fSet < 0 || ( ( fSetStart == 0 || fSetStart == 1 ) && fSetEnd == 0 ) );


        const LONG fSetOld = AtomicCompareExchange( (LONG*)&m_fSet, fSetStart, fSetEnd );


        if ( fSetOld == fSetStart )
        {

            return fTrue;
        }


        else
        {

            if ( fSetOld >= 0 )
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



class CAutoResetSignal
    :   private CSyncObject,
        private CEnhancedStateContainer< CAutoResetSignalState, CSyncStateInitNull, CAutoResetSignalInfo, CSyncBasicInfo >
{
    public:



        CAutoResetSignal( const CSyncBasicInfo& sbi );
        ~CAutoResetSignal();


        void Wait();
        const BOOL FTryWait();
        const BOOL FWait( const INT cmsecTimeout );
        const BOOL FIsSet() const;

        void Set();
        void Reset();


        void Dump( const CDumpContext& dc ) const;

    private:



        CAutoResetSignal& operator=( CAutoResetSignal& ) = delete;


        const BOOL _FWait( const INT cmsecTimeout );

        void _Set();
};


inline void CAutoResetSignal::Wait()
{

    const BOOL fWait = FWait( cmsecInfinite );
    OSSYNCAssert( fWait );
}


inline const BOOL CAutoResetSignal::FTryWait()
{

    const BOOL fSuccess = State().FChange( CAutoResetSignalState( 1 ), CAutoResetSignalState( 0 ) );


    if ( !fSuccess )
    {

        State().SetContend();
    }


    else
    {

        State().SetAcquire();
    }

    return fSuccess;
}


inline const BOOL CAutoResetSignal::FWait( const INT cmsecTimeout )
{

    return FTryWait() || _FWait( cmsecTimeout );
}


inline const BOOL CAutoResetSignal::FIsSet() const
{
    return State().FIsSet();
}


inline void CAutoResetSignal::Set()
{

    if ( !State().FSimpleSet() )
    {

        _Set();
    }
}


inline void CAutoResetSignal::Reset()
{

    State().FChange( CAutoResetSignalState( 1 ), CAutoResetSignalState( 0 ) );
}



class CManualResetSignalInfo
    :   public CSyncBasicInfo,
        public CSyncPerfWait,
        public CSyncPerfAcquire
{
    public:



        CManualResetSignalInfo( const CSyncBasicInfo& sbi )
            :   CSyncBasicInfo( sbi )
        {
        }


        void Dump( const CDumpContext& dc ) const;
};



class CManualResetSignalState
{
    public:



        CManualResetSignalState( const CSyncStateInitNull& null ) : m_fSet( 0 ) {}
        CManualResetSignalState( const INT fSet );
        CManualResetSignalState( const INT cWait, const INT irksem );
        CManualResetSignalState( const CManualResetSignalState& state )
        {
            C_ASSERT( OffsetOf( CManualResetSignalState, m_irksem ) == OffsetOf( CManualResetSignalState, m_fSet ) );
            C_ASSERT( OffsetOf( CManualResetSignalState, m_cWaitNeg ) > OffsetOf( CManualResetSignalState, m_fSet ) );
            C_ASSERT( ( OffsetOf( CManualResetSignalState, m_cWaitNeg ) + sizeof ( m_cWaitNeg ) ) <= ( OffsetOf( CManualResetSignalState, m_fSet ) + sizeof ( m_fSet ) ) );
            m_fSet = state.m_fSet;
        }
        ~CManualResetSignalState() {}


        CManualResetSignalState& operator=( CManualResetSignalState& state ) { m_fSet = state.m_fSet;  return *this; }


        const BOOL FChange( const CManualResetSignalState& stateCur, const CManualResetSignalState& stateNew );
        const CManualResetSignalState Set();
        const CManualResetSignalState Reset();


        const BOOL FNoWait() const { return m_fSet >= 0; }
        const BOOL FWait() const { return m_fSet < 0; }
        const BOOL FNoWaitAndSet() const { return m_fSet > 0; }
        const BOOL FNoWaitAndNotSet() const { return m_fSet == 0; }

        const BOOL FIsSet() const { return m_fSet > 0; }
        const INT CWait() const { OSSYNCAssert( FWait() ); return -m_cWaitNeg; }
        const CKernelSemaphorePool::IRKSEM Irksem() const { OSSYNCAssert( FWait() ); return CKernelSemaphorePool::IRKSEM( m_irksem ); }


        void Dump( const CDumpContext& dc ) const;

    private:



        union
        {

            volatile LONG       m_fSet;


            struct
            {
                volatile USHORT m_irksem;
                volatile SHORT  m_cWaitNeg;
            };
        };
};


inline CManualResetSignalState::CManualResetSignalState( const INT fSet )
{

    m_fSet = LONG( fSet );
}


inline CManualResetSignalState::CManualResetSignalState( const INT cWait, const INT irksem )
{

    OSSYNCAssert( cWait > 0 );
    OSSYNCAssert( cWait <= 0x7FFF );
    OSSYNCAssert( irksem >= 0 );
    OSSYNCAssert( irksem <= 0xFFFE );


    m_cWaitNeg = SHORT( -cWait );


    m_irksem = (USHORT) irksem;
}


inline const BOOL CManualResetSignalState::FChange( const CManualResetSignalState& stateCur, const CManualResetSignalState& stateNew )
{
    return AtomicCompareExchange( (LONG*)&m_fSet, stateCur.m_fSet, stateNew.m_fSet ) == stateCur.m_fSet;
}


inline const CManualResetSignalState CManualResetSignalState::Set()
{
    const CManualResetSignalState stateNew( 1 );
    return CManualResetSignalState( AtomicExchange( (LONG*)&m_fSet, stateNew.m_fSet ) );
}


inline const CManualResetSignalState CManualResetSignalState::Reset()
{
    const CManualResetSignalState stateNew( 0 );
    return CManualResetSignalState( AtomicExchange( (LONG*)&m_fSet, stateNew.m_fSet ) );
}



class CManualResetSignal
    :   private CSyncObject,
        private CEnhancedStateContainer< CManualResetSignalState, CSyncStateInitNull, CManualResetSignalInfo, CSyncBasicInfo >
{
    public:



        CManualResetSignal( const CSyncBasicInfo& sbi );
        ~CManualResetSignal();


        void Wait();
        const BOOL FTryWait();
        const BOOL FWait( const INT cmsecTimeout );
        const BOOL FIsSet() const;

        void Set();
        void Reset();


        void Dump( const CDumpContext& dc ) const;

    private:



        CManualResetSignal& operator=( CManualResetSignal& ) = delete;


        const BOOL _FWait( const INT cmsecTimeout );
};


inline void CManualResetSignal::Wait()
{

    INT fWait = FWait( cmsecInfinite );
    OSSYNCAssert( fWait );
}


inline const BOOL CManualResetSignal::FTryWait()
{
    const BOOL fSuccess = State().FNoWaitAndSet();


    if ( !fSuccess )
    {

        State().SetContend();
    }


    else
    {

        State().SetAcquire();
    }

    return fSuccess;
}


inline const BOOL CManualResetSignal::FWait( const INT cmsecTimeout )
{

    return FTryWait() || _FWait( cmsecTimeout );
}


inline const BOOL CManualResetSignal::FIsSet() const
{
    return State().FIsSet();
}


inline void CManualResetSignal::Set()
{

    const CManualResetSignalState stateOld = State().Set();


    if ( stateOld.FWait() )
    {

        g_ksempoolGlobal.Ksem( stateOld.Irksem(), this ).Release( stateOld.CWait() );
    }
}


inline void CManualResetSignal::Reset()
{

    State().FChange( CManualResetSignalState( 1 ), CManualResetSignalState( 0 ) );
}



class CLockObject
    :   public CSyncObject
{
    public:



        CLockObject() {}
        ~CLockObject() {}

    private:



        CLockObject& operator=( CLockObject& ) = delete;
};



class CLockBasicInfo
    :   public CSyncBasicInfo
{
    public:



#ifdef SYNC_ENHANCED_STATE

        CLockBasicInfo( const CSyncBasicInfo& sbi, const INT rank, const INT subrank );
        ~CLockBasicInfo();

#else

        CLockBasicInfo( const CSyncBasicInfo& sbi, const INT rank, const INT subrank ) : CSyncBasicInfo( sbi ) {}
        ~CLockBasicInfo() {}

#endif


#ifdef SYNC_DEADLOCK_DETECTION

        const INT Rank() const { return m_rank; }
        const INT SubRank() const { return m_subrank; }

#endif


        void Dump( const CDumpContext& dc ) const;

    private:



        CLockBasicInfo& operator=( CLockBasicInfo& ) = delete;


#ifdef SYNC_DEADLOCK_DETECTION


        INT m_rank;
        INT m_subrank;

#endif
};



class CLockPerfHold
{
    public:



        CLockPerfHold();
        ~CLockPerfHold();



        void StartHold();
        void StopHold();


#ifdef SYNC_ANALYZE_PERFORMANCE

        QWORD   CHoldTotal() const      { return m_cHold; }
        double  CsecHoldElapsed() const { return    (double)(signed __int64)m_qwHRTHoldElapsed /
                                                    (double)(signed __int64)QwOSSyncIHRTFreq(); }

#endif


        void Dump( const CDumpContext& dc ) const;

    private:



        CLockPerfHold& operator=( CLockPerfHold& ) = delete;


#ifdef SYNC_ANALYZE_PERFORMANCE


        volatile QWORD m_cHold;


        volatile QWORD m_qwHRTHoldElapsed;

#endif
};


inline void CLockPerfHold::StartHold()
{
#ifdef SYNC_ANALYZE_PERFORMANCE


    AtomicAdd( (QWORD*)&m_cHold, 1 );


    AtomicAdd( (QWORD*)&m_qwHRTHoldElapsed, QWORD( -__int64( QwOSSyncIHRTCount() ) ) );

#endif
}


inline void CLockPerfHold::StopHold()
{
#ifdef SYNC_ANALYZE_PERFORMANCE


    AtomicAdd( (QWORD*)&m_qwHRTHoldElapsed, QwOSSyncIHRTCount() );

#endif
}



class CLockDeadlockDetectionInfo;

class COwner
{
    public:



        COwner();
        ~COwner();

        void* operator new( size_t cb ) { return ESMemoryNew( cb ); }
        void operator delete( void* pv ) { ESMemoryDelete( pv ); }

    public:



        COwner& operator=( COwner& ) = delete;



        CLS*                            m_pclsOwner;


        COwner*                         m_pownerContextNext;


        CLockDeadlockDetectionInfo*     m_plddiOwned;


        COwner*                         m_pownerLockNext;


        DWORD                           m_group;
};



class CLockDeadlockDetectionInfo
{
    public:



        enum
        {
            subrankNoDeadlock = INT_MAX
        };



        CLockDeadlockDetectionInfo( const CLockBasicInfo& lbi );
        ~CLockDeadlockDetectionInfo();



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

#else

#ifdef DEBUG

        const BOOL FOwner( const DWORD group = -1 ) { return fTrue; }
        const BOOL FNotOwner( const DWORD group = -1 ) { return fTrue; }
        const BOOL FOwned() { return fTrue; }
        const BOOL FNotOwned() { return fTrue; }

        const BOOL FWaiter( const DWORD group = -1 ) { return fTrue; }
        const BOOL FNotWaiter( const DWORD group = -1 ) { return fTrue; }

#endif

        const VOID AssertCanBeWaiter() {}
        static void OSSYNCAPI GetApiEntryState(
            __out DWORD *pcDisableDeadlockDetection,
            __out DWORD *pcDisableOwnershipTracking,
            __out DWORD *pcLocks ) {}
        static void OSSYNCAPI AssertCleanApiExit(
            DWORD cDisableDeadlockDetection,
            DWORD cDisableOwnershipTracking,
            DWORD cLocks ) {}

#endif


        void Dump( const CDumpContext& dc ) const;

    private:



        CLockDeadlockDetectionInfo& operator=( CLockDeadlockDetectionInfo& ) = delete;


#ifdef SYNC_DEADLOCK_DETECTION


        const CLockBasicInfo*           m_plbiParent;


        CSemaphore                      m_semOwnerList;


        COwner                          m_ownerHead;

#endif
};


inline void CLockDeadlockDetectionInfo::AddAsWaiter( const DWORD group )
{
#ifdef SYNC_DEADLOCK_DETECTION


    OSSYNCAssert( FNotWaiter( group ) );


    CLS* const pcls = Pcls();
    OSSYNCAssert( !pcls->plddiLockWait );
    OSSYNCAssert( !pcls->groupLockWait );


    pcls->plddiLockWait = this;
    pcls->groupLockWait = group;


    OSSYNCAssert( FWaiter( group ) );

#endif
}


inline void CLockDeadlockDetectionInfo::RemoveAsWaiter( const DWORD group )
{
#ifdef SYNC_DEADLOCK_DETECTION


    OSSYNCAssert( FWaiter( group ) );


    CLS* const pcls = Pcls();
    pcls->plddiLockWait = NULL;
    pcls->groupLockWait = 0;


    OSSYNCAssert( FNotWaiter( group ) );

#endif
}


inline void CLockDeadlockDetectionInfo::AddAsOwner( const DWORD group )
{
#ifdef SYNC_DEADLOCK_DETECTION


    OSSYNCAssert( FNotOwner( group ) );


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


    if ( pcls->cOverrideDeadlock )
    {
        pcls->cOverrideDeadlock--;
    }


    OSSYNCAssert( FOwner( group ) );

#endif
}


inline void CLockDeadlockDetectionInfo::RemoveAsOwner( const DWORD group )
{
#ifdef SYNC_DEADLOCK_DETECTION


    OSSYNCAssert( FOwner( group ) );


    CLS* const pcls = Pcls();

    
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


    OSSYNCAssert( FNotOwner( group ) );

#endif
}


inline void OSSYNCAPI CLockDeadlockDetectionInfo::NextOwnershipIsNotADeadlock()
{
#ifdef SYNC_DEADLOCK_DETECTION

    Pcls()->cOverrideDeadlock++;

#endif
}


inline void OSSYNCAPI CLockDeadlockDetectionInfo::DisableDeadlockDetection()
{
#ifdef SYNC_DEADLOCK_DETECTION

    Pcls()->cDisableDeadlockDetection++;

#endif
}


inline void OSSYNCAPI CLockDeadlockDetectionInfo::EnableDeadlockDetection()
{
#ifdef SYNC_DEADLOCK_DETECTION

    Pcls()->cDisableDeadlockDetection--;

#endif
}


inline void OSSYNCAPI CLockDeadlockDetectionInfo::DisableOwnershipTracking()
{
#ifdef SYNC_DEADLOCK_DETECTION

    Pcls()->cDisableOwnershipTracking++;

#endif
}


inline void OSSYNCAPI CLockDeadlockDetectionInfo::EnableOwnershipTracking()
{
#ifdef SYNC_DEADLOCK_DETECTION

    Pcls()->cDisableOwnershipTracking--;

#endif
}


inline void OSSYNCAPI CLockDeadlockDetectionInfo::DisableLockCheckOnApiExit()
{
#ifdef SYNC_DEADLOCK_DETECTION
    
    Pcls()->cDisableLockCheckOnApiExit++;
    
#endif
}


inline void OSSYNCAPI CLockDeadlockDetectionInfo::EnableLockCheckOnApiExit()
{
#ifdef SYNC_DEADLOCK_DETECTION
    
    Pcls()->cDisableLockCheckOnApiExit--;
    
#endif
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

 
inline const BOOL CLockDeadlockDetectionInfo::FNotOwner( const DWORD group )
{
    return Pcls()->cDisableOwnershipTracking != 0 || !FOwner( group );
}

 
inline const BOOL CLockDeadlockDetectionInfo::FOwned()
{
    return m_ownerHead.m_pclsOwner || m_ownerHead.m_pownerContextNext;
}

 
inline const BOOL CLockDeadlockDetectionInfo::FNotOwned()
{
    return !FOwned();
}


inline const VOID CLockDeadlockDetectionInfo::AssertCanBeWaiter()
{

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


    OSSYNCAssertSzRTL( Rank > Info().Rank()                                                                         ||
                       ( Rank == Info().Rank() && SubRank > Info().SubRank() )                                      ||
                       ( Rank == Info().Rank() && SubRank == Info().SubRank() && SubRank == subrankNoDeadlock )     ||
                       pcls->cOverrideDeadlock                                                                      ||
                       pcls->cDisableDeadlockDetection, "Potential Deadlock Detected (Rank Violation)" );
}

 
inline const BOOL CLockDeadlockDetectionInfo::FWaiter( const DWORD group )
{
    CLS* const pcls = Pcls();
    return pcls->plddiLockWait == this && pcls->groupLockWait == group;
}

 
inline const BOOL CLockDeadlockDetectionInfo::FNotWaiter( const DWORD group )
{
    return !FWaiter( group );
}

#endif


class CInitTermLock
{
    private:

        typedef ULONG ControlWord;

        volatile ControlWord            m_cwState;

        const static ControlWord        fInitInProgress = 0x80000000;
        const static ControlWord        fTermInProgress = 0x40000000;
        const static ControlWord        mskRefCount     = ~( fInitInProgress | fTermInProgress );
#ifdef DEBUG
        const static ControlWord        mskBadRefCount  = 0x3ffff800;
#else
        const static ControlWord        mskBadRefCount  = 0x30000000;
#endif

        C_ASSERT( mskBadRefCount == ( mskBadRefCount & mskRefCount ) );

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

                    OSSYNCAssert( ( cwFinal == ( cwInitial + 1 ) ) ||
                                ( cwFinal == ( cwInitial - 1 ) ) );
                    break;

            }
        }



        void SleepAwayQuanta();

#define KEEP_SYNC_INITTERM_STATISTICS
#ifdef KEEP_SYNC_INITTERM_STATISTICS
        LONG            m_cActualInits;
        LONG            m_cRefCountInits;
        LONG            m_cCancelledInits;
        LONG            m_cActualTerms;
        LONG            m_cRefCountTerms;

        #define KeepStats( code )   code
#else
        #define KeepStats( code )
#endif

    public:

        CInitTermLock()
        {
            OSSYNCEnforce( m_cwState == 0 );
        }

        ~CInitTermLock()
        {

            OSSYNCEnforce( ( m_cwState == 0 ) || g_fSyncProcessAbort );
            OSSYNCEnforce( ( m_cActualInits == m_cActualTerms ) || g_fSyncProcessAbort );
            OSSYNCEnforce( ( m_cRefCountInits == m_cRefCountTerms ) || g_fSyncProcessAbort );
        }
        
    private:
        CInitTermLock( const CInitTermLock& );
        CInitTermLock& operator=( const CInitTermLock& );

    public:


        enum class ERR
        {
            errSuccess,
            errInitBegun,
            errTermBegun,
        };


        LONG CConsumers() const
        {

            OSSYNCAssert( 0 == ( m_cwState & fInitInProgress ) );
            OSSYNCAssert( 0 == ( m_cwState & fTermInProgress ) );

            return mskRefCount & m_cwState;
        }


        ERR ErrInitBegin()
        {
            OSSYNC_FOREVER
            {
                OSSYNCAssert( 0 == ( mskBadRefCount & m_cwState ) );


                const ControlWord cwBIExpected = m_cwState;


                if ( cwBIExpected & ( fInitInProgress | fTermInProgress ) )
                {

                    SleepAwayQuanta();

                    continue;
                }


                OSSYNCAssert( cwBIExpected == ( cwBIExpected & mskRefCount ) );


                LONG cwAI = cwBIExpected;


                cwAI++;

                if ( cwBIExpected == 0 )
                {
    
                    cwAI |= fInitInProgress;
                }


                OSSYNCAssert( ( ( cwAI & mskRefCount ) >= 2 ) || ( cwAI & fInitInProgress ) );
                AssertValidStateTransition_( cwBIExpected, cwAI );


                const ControlWord cwBI = AtomicCompareExchange( (LONG*)&m_cwState, cwBIExpected, cwAI );


                if ( cwBI != cwBIExpected )
                {

                    continue;
                }


                OSSYNCAssert( 0 == ( cwBI & ( fInitInProgress | fTermInProgress ) ) );

                if ( cwAI & fInitInProgress )
                {

                    OSSYNCAssert( 0 == ( mskBadRefCount & m_cwState ) );

                    return ERR::errInitBegun;
                }
                else
                {

                    break;
                }
            }

            OSSYNCAssert( 0 == ( mskBadRefCount & m_cwState ) );

 
            KeepStats( AtomicIncrement( &m_cRefCountInits ) );

            return ERR::errSuccess;
        }


        enum InitFinishStatus { fSuccessfulInit = 1, fFailedInit = 2 };

        void InitFinish( InitFinishStatus fInitStatus )
        {


            OSSYNCAssert( m_cwState & fInitInProgress );
            OSSYNCAssert( 0 == ( m_cwState & fTermInProgress ) );


            OSSYNCAssert( fInitStatus == fSuccessfulInit || fInitStatus == fFailedInit );

            
            const ControlWord cwBIExpected = ( fInitInProgress | 0x1 );

            OSSYNCAssert( m_cwState == cwBIExpected );

            if ( fInitStatus == fSuccessfulInit )
            {


                KeepStats( AtomicIncrement( &m_cActualInits ) );


                const ControlWord cwBI = AtomicExchangeReset( (ULONG*)&m_cwState, fInitInProgress );


                OSSYNCAssert( cwBI == cwBIExpected );
                AssertValidStateTransition_( cwBI, cwBI & ~fInitInProgress );
                OSSYNCAssert( cwBI & fInitInProgress );

                OSSYNCAssert( 0 == ( m_cwState & fInitInProgress ) );
                OSSYNCAssert( 0 == ( m_cwState & fTermInProgress ) );
            }
            else
            {


                KeepStats( AtomicIncrement( &m_cCancelledInits ) );


                AssertValidStateTransition_( cwBIExpected, 0x0 );


                const ControlWord cwBI = AtomicCompareExchange( (LONG*)&m_cwState, cwBIExpected, 0x0 );


                OSSYNCAssert( cwBI == cwBIExpected );
            }


            OSSYNCAssert( 0 == ( mskBadRefCount & m_cwState ) );
        }


        ERR ErrTermBegin()
        {
            OSSYNC_FOREVER
            {
                OSSYNCAssert( 0 == ( mskBadRefCount & m_cwState ) );
 

                const ControlWord cwBIExpected = m_cwState;


                OSSYNCAssert( 0 == ( cwBIExpected & fInitInProgress ) );


                if ( cwBIExpected & ( fInitInProgress | fTermInProgress ) )
                {

                    OSSYNCAssertSz( fFalse, "Can this happen?  I don't think so, as either we'll be the one to set term in progress or our ref count stops someone else from doing this." );

                    SleepAwayQuanta();

                    continue;
                }


                OSSYNCAssert( cwBIExpected == ( cwBIExpected & mskRefCount ) );


                LONG cwAI = cwBIExpected;


                if ( 1 == cwBIExpected )
                {

                    cwAI = fTermInProgress;
                }
                else
                {

                    cwAI--;
                }

                OSSYNCAssert( ( ( cwAI & mskRefCount ) >= 1 ) || ( cwAI & fTermInProgress ) );



                AssertValidStateTransition_( cwBIExpected, cwAI );


                const ControlWord cwBI = AtomicCompareExchange( (LONG*)&m_cwState, cwBIExpected, cwAI );


                if ( cwBI != cwBIExpected )
                {

                    continue;
                }


                if ( cwAI & fTermInProgress )
                {

                    OSSYNCAssert( 0 == ( mskBadRefCount & m_cwState ) );

                    return ERR::errTermBegun;
                }
                else
                {

                    break;
                }
            }

            OSSYNCAssert( 0 == ( mskBadRefCount & m_cwState ) );


            KeepStats( AtomicIncrement( &m_cRefCountTerms ) );

            return ERR::errSuccess;
        }


        void TermFinish()
        {

            OSSYNCAssert( m_cwState == fTermInProgress );


            KeepStats( AtomicIncrement( &m_cActualTerms ) );


            const ControlWord cwBI = AtomicExchangeReset( (ULONG*)&m_cwState, fTermInProgress );


            AssertValidStateTransition_( cwBI, 0x0 );
            OSSYNCAssert( cwBI == fTermInProgress );

        }
};



class CCriticalSectionInfo
    :   public CLockBasicInfo,
        public CLockPerfHold,
        public CLockDeadlockDetectionInfo
{
    public:



        CCriticalSectionInfo( const CLockBasicInfo& lbi )
            :   CLockBasicInfo( lbi ),
                CLockDeadlockDetectionInfo( (CLockBasicInfo&) *this )
        {
        }


        void Dump( const CDumpContext& dc ) const;
};



class CCriticalSectionState
{
    public:



        CCriticalSectionState( const CSyncBasicInfo& sbi );
        ~CCriticalSectionState();


        CSemaphore& Semaphore() { return m_sem; }


        void Dump( const CDumpContext& dc ) const;

    private:



        CCriticalSectionState& operator=( CCriticalSectionState& ) = delete;



        CSemaphore      m_sem;
};



class CCriticalSection
    :   private CLockObject,
        private CEnhancedStateContainer< CCriticalSectionState, CSyncBasicInfo, CCriticalSectionInfo, CLockBasicInfo >
{
    public:



        CCriticalSection( const CLockBasicInfo& lbi );
        ~CCriticalSection();


        void Enter();
        const BOOL FTryEnter();
        const BOOL FEnter( const INT cmsecTimeout );
        void Leave();


        const INT CWait() { return State().Semaphore().CWait(); }

#ifdef DEBUG

        const BOOL FAcquired() { return ( State().Semaphore().CAvail() == 0 ); }

#endif


#ifdef SYNC_DEADLOCK_DETECTION

        const BOOL FOwner() const       { return State().FOwner(); }
        const BOOL FNotOwner() const    { return State().FNotOwner(); }

#else

#ifdef DEBUG

        const BOOL FOwner() const       { return fTrue; }
        const BOOL FNotOwner() const    { return fTrue; }

#endif

#endif


        void Dump( const CDumpContext& dc ) const;

    private:



        CCriticalSection& operator=( CCriticalSection& ) = delete;
};


inline void CCriticalSection::Enter()
{

    State().AssertCanBeWaiter();


    State().AddAsWaiter();

    State().Semaphore().Acquire();

    State().RemoveAsWaiter();


    OSSYNCAssert( !State().Semaphore().CAvail() );


    State().AddAsOwner();
    State().StartHold();
}


inline const BOOL CCriticalSection::FTryEnter()
{

    BOOL fAcquire = State().Semaphore().FTryAcquire();


    if ( fAcquire )
    {

        OSSYNCAssert( !State().Semaphore().CAvail() );


        State().AddAsOwner();
        State().StartHold();
    }

    return fAcquire;
}


inline const BOOL CCriticalSection::FEnter( const INT cmsecTimeout )
{

    if ( cmsecTimeout == cmsecInfinite )
    {
        State().AssertCanBeWaiter();
    }


    State().AddAsWaiter();

    BOOL fAcquire = State().Semaphore().FAcquire( cmsecTimeout );

    State().RemoveAsWaiter();


    if ( fAcquire )
    {

        OSSYNCAssert( !State().Semaphore().CAvail() );


        State().AddAsOwner();
        State().StartHold();
    }

    return fAcquire;
}


inline void CCriticalSection::Leave()
{

    State().RemoveAsOwner();


    State().StopHold();


    OSSYNCAssert( !State().Semaphore().CAvail() );


    State().Semaphore().Release();
}



class CNestableCriticalSectionInfo
    :   public CLockBasicInfo,
        public CLockPerfHold,
        public CLockDeadlockDetectionInfo
{
    public:



        CNestableCriticalSectionInfo( const CLockBasicInfo& lbi )
            :   CLockBasicInfo( lbi ),
                CLockDeadlockDetectionInfo( (CLockBasicInfo&) *this )
        {
        }


        void Dump( const CDumpContext& dc ) const;
};



class CNestableCriticalSectionState
{
    public:



        CNestableCriticalSectionState( const CSyncBasicInfo& sbi );
        ~CNestableCriticalSectionState();


        void SetOwner( CLS* const pcls );

        void Enter();
        void Leave();


        CSemaphore& Semaphore() { return m_sem; }
        CLS* PclsOwner() { return m_pclsOwner; }
        INT CEntry() { return m_cEntry; }


        void Dump( const CDumpContext& dc ) const;

    private:



        CNestableCriticalSectionState& operator=( CNestableCriticalSectionState& ) = delete;



        CSemaphore      m_sem;


        CLS* volatile   m_pclsOwner;


        volatile INT    m_cEntry;
};


inline void CNestableCriticalSectionState::SetOwner( CLS* const pcls )
{

    OSSYNCAssert( !pcls || !m_pclsOwner );


    m_pclsOwner = pcls;
}


inline void CNestableCriticalSectionState::Enter()
{

    OSSYNCAssert( m_pclsOwner );


    OSSYNCAssert( INT( m_cEntry + 1 ) >= 1 );


    m_cEntry++;
}


inline void CNestableCriticalSectionState::Leave()
{

    OSSYNCAssert( m_pclsOwner );


    m_cEntry--;
}



class CNestableCriticalSection
    :   private CLockObject,
        private CEnhancedStateContainer< CNestableCriticalSectionState, CSyncBasicInfo, CNestableCriticalSectionInfo, CLockBasicInfo >
{
    public:



        CNestableCriticalSection( const CLockBasicInfo& lbi );
        ~CNestableCriticalSection();


        void Enter();
        const BOOL FTryEnter();
        const BOOL FEnter( const INT cmsecTimeout );
        void Leave();


        const INT CWait() { return State().Semaphore().CWait(); }

#ifdef SYNC_DEADLOCK_DETECTION

        const BOOL FOwner() { return State().FOwner(); }
        const BOOL FNotOwner() { return State().FNotOwner(); }

#else

#ifdef DEBUG

        const BOOL FOwner() const       { return fTrue; }
        const BOOL FNotOwner() const    { return fTrue; }

#endif

#endif


        void Dump( const CDumpContext& dc ) const;

    private:



        CNestableCriticalSection& operator=( CNestableCriticalSection& ) = delete;
};


inline void CNestableCriticalSection::Enter()
{

    CLS* const pcls = Pcls();


    if ( State().PclsOwner() == pcls )
    {

        OSSYNCAssert( !State().Semaphore().CAvail() );


        OSSYNCAssert( State().CEntry() >= 1 );


        State().Enter();
    }


    else
    {
        OSSYNCAssert( State().PclsOwner() != pcls );


        State().AssertCanBeWaiter();


        State().AddAsWaiter();

        State().Semaphore().Acquire();

        State().RemoveAsWaiter();


        OSSYNCAssert( !State().Semaphore().CAvail() );


        State().AddAsOwner();
        State().StartHold();


        State().SetOwner( pcls );


        State().Enter();
    }
}


inline const BOOL CNestableCriticalSection::FTryEnter()
{

    CLS* const pcls = Pcls();


    if ( State().PclsOwner() == pcls )
    {

        OSSYNCAssert( !State().Semaphore().CAvail() );


        OSSYNCAssert( State().CEntry() >= 1 );


        State().Enter();


        return fTrue;
    }


    else
    {
        OSSYNCAssert( State().PclsOwner() != pcls );


        const BOOL fAcquired = State().Semaphore().FTryAcquire();


        if ( fAcquired )
        {

            OSSYNCAssert( !State().Semaphore().CAvail() );


            State().AddAsOwner();
            State().StartHold();


            State().SetOwner( pcls );


            State().Enter();
        }


        return fAcquired;
    }
}


inline const BOOL CNestableCriticalSection::FEnter( const INT cmsecTimeout )
{

    CLS* const pcls = Pcls();


    if ( State().PclsOwner() == pcls )
    {

        OSSYNCAssert( !State().Semaphore().CAvail() );


        OSSYNCAssert( State().CEntry() >= 1 );


        State().Enter();


        return fTrue;
    }


    else
    {
        OSSYNCAssert( State().PclsOwner() != pcls );


        if ( cmsecTimeout == cmsecInfinite )
        {
            State().AssertCanBeWaiter();
        }


        State().AddAsWaiter();

        const BOOL fAcquired = State().Semaphore().FAcquire( cmsecTimeout );

        State().RemoveAsWaiter();


        if ( fAcquired )
        {

            OSSYNCAssert( !State().Semaphore().CAvail() );


            State().AddAsOwner();
            State().StartHold();


            State().SetOwner( pcls );


            State().Enter();
        }


        return fAcquired;
    }
}


inline void CNestableCriticalSection::Leave()
{

    OSSYNCAssert( State().PclsOwner() == Pcls() );


    OSSYNCAssert( !State().Semaphore().CAvail() );


    OSSYNCAssert( State().CEntry() >= 1 );


    State().Leave();


    if ( !State().CEntry() )
    {

        State().SetOwner( 0 );


        State().RemoveAsOwner();


        State().StopHold();


        State().Semaphore().Release();
    }
}



class CGateInfo
    :   public CSyncBasicInfo,
        public CSyncPerfWait
{
    public:



        CGateInfo( const CSyncBasicInfo& sbi )
            :   CSyncBasicInfo( sbi )
        {
        }


        void Dump( const CDumpContext& dc ) const;
};



class CGateState
{
    public:



        CGateState( const CSyncStateInitNull& null ) : m_cWait( 0 ), m_irksem( CKernelSemaphorePool::irksemNil ) {}
        CGateState( const INT cWait, const INT irksem );
        ~CGateState() {}


        void SetWaitCount( const INT cWait );
        void SetIrksem( const CKernelSemaphorePool::IRKSEM irksem );


        const INT CWait() const { return m_cWait; }
        const CKernelSemaphorePool::IRKSEM Irksem() const { return CKernelSemaphorePool::IRKSEM( m_irksem ); }


        void Dump( const CDumpContext& dc ) const;

    private:



        CGateState& operator=( CGateState& ) = delete;



        volatile SHORT  m_cWait;


        volatile USHORT m_irksem;
};


inline void CGateState::SetWaitCount( const INT cWait )
{

    OSSYNCAssert( cWait >= 0 );
    OSSYNCAssert( cWait <= 0x7FFF );


    m_cWait = (USHORT) cWait;
}


inline void CGateState::SetIrksem( const CKernelSemaphorePool::IRKSEM irksem )
{

    OSSYNCAssert( irksem >= 0 );
    OSSYNCAssert( irksem <= 0xFFFF );


    m_irksem = (USHORT) irksem;
}



class CGate
    :   private CSyncObject,
        private CEnhancedStateContainer< CGateState, CSyncStateInitNull, CGateInfo, CSyncBasicInfo >
{
    public:



        CGate( const CSyncBasicInfo& sbi );
        ~CGate();


        void Wait( CCriticalSection& crit );
        void Release( CCriticalSection& crit, const INT cToRelease );
        void ReleaseAll( CCriticalSection& crit );


        const INT CWait() const { return State().CWait(); }


        void Dump( const CDumpContext& dc ) const;

    private:


        void ReleaseAndHold( CCriticalSection& crit, const INT cToRelease );


        CGate& operator=( CGate& ) = delete;
};



class CLockStateInitNull
{
};

extern const CLockStateInitNull lockstateNull;



class CBinaryLockPerfInfo
    :   public CSyncPerfWait,
        public CSyncPerfAcquire,
        public CLockPerfHold
{
    public:



        CBinaryLockPerfInfo()
        {
        }


        void Dump( const CDumpContext& dc ) const;
};



class CBinaryLockGroupInfo
{
    public:



        CBinaryLockGroupInfo() {}
        ~CBinaryLockGroupInfo() {}


        void StartWait( const INT iGroup ) { m_rginfo[iGroup].StartWait(); }
        void StopWait( const INT iGroup ) { m_rginfo[iGroup].StopWait(); }

        void SetAcquire( const INT iGroup ) { m_rginfo[iGroup].SetAcquire(); }
        void SetContend( const INT iGroup ) { m_rginfo[iGroup].SetContend(); }

        void StartHold( const INT iGroup ) { m_rginfo[iGroup].StartHold(); }
        void StopHold( const INT iGroup ) { m_rginfo[iGroup].StopHold(); }


#ifdef SYNC_ANALYZE_PERFORMANCE

        QWORD   CWaitTotal( const INT iGroup ) const        { return m_rginfo[iGroup].CWaitTotal(); }
        double  CsecWaitElapsed( const INT iGroup ) const   { return m_rginfo[iGroup].CsecWaitElapsed(); }

        QWORD   CAcquireTotal( const INT iGroup ) const     { return m_rginfo[iGroup].CAcquireTotal(); }
        QWORD   CContendTotal( const INT iGroup ) const     { return m_rginfo[iGroup].CContendTotal(); }

        QWORD   CHoldTotal( const INT iGroup ) const        { return m_rginfo[iGroup].CHoldTotal(); }
        double  CsecHoldElapsed( const INT iGroup ) const   { return m_rginfo[iGroup].CsecHoldElapsed(); }

#endif


        void Dump( const CDumpContext& dc ) const;

    private:



        CBinaryLockGroupInfo& operator=( CBinaryLockGroupInfo& ) = delete;



        CBinaryLockPerfInfo m_rginfo[2];
};



class CBinaryLockInfo
    :   public CLockBasicInfo,
        public CBinaryLockGroupInfo,
        public CLockDeadlockDetectionInfo
{
    public:



        CBinaryLockInfo( const CLockBasicInfo& lbi )
            :   CLockBasicInfo( lbi ),
                CLockDeadlockDetectionInfo( (CLockBasicInfo&) *this )
        {
        }


        void Dump( const CDumpContext& dc ) const;
};



class CBinaryLockState
{
    public:



        typedef DWORD ControlWord;



        CBinaryLockState( const CSyncBasicInfo& sbi );
        ~CBinaryLockState();


        void Dump( const CDumpContext& dc ) const;



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


        volatile DWORD          m_cOwner;


        CSemaphore              m_sem1;


        CSemaphore              m_sem2;

    private:



        CBinaryLockState& operator=( CBinaryLockState& ) = delete;
};



class CBinaryLock
    :   private CLockObject,
        private CEnhancedStateContainer< CBinaryLockState, CSyncBasicInfo, CBinaryLockInfo, CLockBasicInfo >
{
    public:



        typedef CBinaryLockState::ControlWord ControlWord;


        enum TransitionReason
        {
            trIllegal = 0,
            trEnter1 = 1,
            trLeave1 = 2,
            trEnter2 = 4,
            trLeave2 = 8,
        };



        CBinaryLock( const CLockBasicInfo& lbi );
        ~CBinaryLock();


        void Enter1();
        const BOOL FTryEnter1();
        void Leave1();

        void Enter2();
        const BOOL FTryEnter2();
        void Leave2();

        void ClaimOwnership( const DWORD group );
        void ReleaseOwnership( const DWORD group );


        const BOOL FGroup1Quiesced()    { return State().m_cw & 0x00008000; }
        const BOOL FGroup2Quiesced()    { return State().m_cw & 0x80000000; }

#ifdef SYNC_DEADLOCK_DETECTION

        const BOOL FMemberOfGroup1()    { return State().FOwner( 0 ); }
        const BOOL FNotMemberOfGroup1() { return State().FNotOwner( 0 ); }
        const BOOL FMemberOfGroup2()    { return State().FOwner( 1 ); }
        const BOOL FNotMemberOfGroup2() { return State().FNotOwner( 1 ); }

#else

#ifdef DEBUG

        const BOOL FMemberOfGroup1()    { return fTrue; }
        const BOOL FNotMemberOfGroup1() { return fTrue; }
        const BOOL FMemberOfGroup2()    { return fTrue; }
        const BOOL FNotMemberOfGroup2() { return fTrue; }

#endif

#endif


        void Dump( const CDumpContext& dc ) const;

    private:



        CBinaryLock& operator=( CBinaryLock& ) = delete;


        INT _StateFromControlWord( const ControlWord cw );
        BOOL _FValidStateTransition(    const ControlWord cwBI,
                                        const ControlWord cwAI,
                                        const TransitionReason tr );


        void _Enter1( const ControlWord cwBIOld );
        void _Enter2( const ControlWord cwBIOld );

        void _UpdateQuiescedOwnerCountAsGroup1( const DWORD cOwnerDelta );
        void _UpdateQuiescedOwnerCountAsGroup2( const DWORD cOwnerDelta );
};



inline void CBinaryLock::Enter1()
{

    OSSYNCAssert( State().FNotOwner( 0 ) );
    OSSYNCAssert( State().FNotOwner( 1 ) );


    State().AssertCanBeWaiter();


    OSSYNC_FOREVER
    {

        const ControlWord cwBIExpected = State().m_cw;


        const ControlWord cwAI =    ControlWord( ( ( cwBIExpected & ( ( LONG_PTR( LONG( cwBIExpected ) ) >> 31 ) |
                                    0x0000FFFF ) ) | 0x80000000 ) + 0x00000001 );


        OSSYNCAssert( _FValidStateTransition( cwBIExpected, cwAI, trEnter1 ) );


        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );


        if ( ( cwBI ^ cwBIExpected ) | ( cwBI & 0x00008000 ) )
        {

            if ( cwBI != cwBIExpected )
            {

                continue;
            }


            else
            {

                State().SetContend( 0 );


                _Enter1( cwBI );


                break;
            }
        }


        else
        {

            break;
        }
    }


    State().SetAcquire( 0 );
    State().AddAsOwner( 0 );
    State().StartHold( 0 );
}




inline const BOOL CBinaryLock::FTryEnter1()
{

    OSSYNCAssert( State().FNotOwner( 0 ) );


    OSSYNC_FOREVER
    {

        ControlWord cwBIExpected = State().m_cw;


        cwBIExpected = cwBIExpected & 0xFFFF7FFF;


        const ControlWord cwAI =    ControlWord( ( ( cwBIExpected & ( ( LONG_PTR( LONG( cwBIExpected ) ) >> 31 ) |
                                    0x0000FFFF ) ) | 0x80000000 ) + 0x00000001 );


        OSSYNCAssert(   _StateFromControlWord( cwBIExpected ) < 0 ||
                _FValidStateTransition( cwBIExpected, cwAI, trEnter1 ) );


        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );


        if ( cwBI != cwBIExpected )
        {

            if ( cwBI & 0x00008000 )
            {

                return fFalse;
            }


            else
            {

                continue;
            }
        }


        else
        {

            State().SetAcquire( 0 );
            State().AddAsOwner( 0 );
            State().StartHold( 0 );


            return fTrue;
        }
    }
}



inline void CBinaryLock::Leave1()
{

    State().RemoveAsOwner( 0 );


    State().StopHold( 0 );


    OSSYNC_FOREVER
    {

        ControlWord cwBIExpected = State().m_cw;


        cwBIExpected = cwBIExpected & 0xFFFF7FFF;


        ControlWord cwAI =  cwBIExpected + 0xFFFFFFFF;
        cwAI = cwAI - ( ( ( cwAI + 0xFFFFFFFF ) << 16 ) & 0x80000000 );


        OSSYNCAssert(   _StateFromControlWord( cwBIExpected ) < 0 ||
                _FValidStateTransition( cwBIExpected, cwAI, trLeave1 ) );


        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );


        if ( cwBI != cwBIExpected )
        {

            if ( cwBI & 0x00008000 )
            {

                _UpdateQuiescedOwnerCountAsGroup1( 0xFFFFFFFF );


                break;
            }


            else
            {

                continue;
            }
        }


        else
        {

            break;
        }
    }
}



inline void CBinaryLock::Enter2()
{

    OSSYNCAssert( State().FNotOwner( 0 ) );
    OSSYNCAssert( State().FNotOwner( 1 ) );


    State().AssertCanBeWaiter();


    OSSYNC_FOREVER
    {

        const ControlWord cwBIExpected = State().m_cw;


        const ControlWord cwAI =    ControlWord( ( ( cwBIExpected & ( ( LONG( cwBIExpected << 16 ) >> 31 ) |
                                    0xFFFF0000 ) ) | 0x00008000 ) + 0x00010000 );


        OSSYNCAssert( _FValidStateTransition( cwBIExpected, cwAI, trEnter2 ) );


        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );


        if ( ( cwBI ^ cwBIExpected ) | ( cwBI & 0x80000000 ) )
        {

            if ( cwBI != cwBIExpected )
            {

                continue;
            }


            else
            {

                State().SetContend( 1 );


                _Enter2( cwBI );


                break;
            }
        }


        else
        {

            break;
        }
    }


    State().SetAcquire( 1 );
    State().AddAsOwner( 1 );
    State().StartHold( 1 );
}



inline const BOOL CBinaryLock::FTryEnter2()
{

    OSSYNCAssert( State().FNotOwner( 1 ) );


    OSSYNC_FOREVER
    {

        ControlWord cwBIExpected = State().m_cw;


        cwBIExpected = cwBIExpected & 0x7FFFFFFF;


        const ControlWord cwAI =    ControlWord( ( ( cwBIExpected & ( ( LONG( cwBIExpected << 16 ) >> 31 ) |
                                    0xFFFF0000 ) ) | 0x00008000 ) + 0x00010000 );


        OSSYNCAssert(   _StateFromControlWord( cwBIExpected ) < 0 ||
                _FValidStateTransition( cwBIExpected, cwAI, trEnter2 ) );


        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );


        if ( cwBI != cwBIExpected )
        {

            if ( cwBI & 0x80000000 )
            {

                return fFalse;
            }


            else
            {

                continue;
            }
        }


        else
        {

            State().SetAcquire( 1 );
            State().AddAsOwner( 1 );
            State().StartHold( 1 );


            return fTrue;
        }
    }
}



inline void CBinaryLock::Leave2()
{

    State().RemoveAsOwner( 1 );


    State().StopHold( 1 );


    OSSYNC_FOREVER
    {

        ControlWord cwBIExpected = State().m_cw;


        cwBIExpected = cwBIExpected & 0x7FFFFFFF;


        ControlWord cwAI =  cwBIExpected + 0xFFFF0000;
        cwAI = cwAI - ( ( ( cwAI + 0xFFFF0000 ) >> 16 ) & 0x00008000 );


        OSSYNCAssert(   _StateFromControlWord( cwBIExpected ) < 0 ||
                _FValidStateTransition( cwBIExpected, cwAI, trLeave2 ) );


        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );


        if ( cwBI != cwBIExpected )
        {

            if ( cwBI & 0x80000000 )
            {

                _UpdateQuiescedOwnerCountAsGroup2( 0xFFFFFFFF );


                break;
            }


            else
            {

                continue;
            }
        }


        else
        {

            break;
        }
    }
}


inline void CBinaryLock::ClaimOwnership( const DWORD group )
{
#ifndef DEBUG
#ifdef DEBUGGER_EXTENSION
#ifndef RTM
    OSSYNCAssertSzRTL( ( group == 1 ) || ( group == 2 ), "Unknown group!" );
#endif
#endif
#endif
    State().AddAsOwner( group - 1 );
}


inline void CBinaryLock::ReleaseOwnership( const DWORD group )
{
#ifndef DEBUG
#ifdef DEBUGGER_EXTENSION
#ifndef RTM
    OSSYNCAssertSzRTL( ( group == 1 ) || ( group == 2 ), "Unknown group!" );
#endif
#endif
#endif
    State().RemoveAsOwner( group - 1 );
}


class CReaderWriterLockPerfInfo
    :   public CSyncPerfWait,
        public CSyncPerfAcquire,
        public CLockPerfHold
{
    public:



        CReaderWriterLockPerfInfo()
        {
        }


        void Dump( const CDumpContext& dc ) const;
};



class CReaderWriterLockGroupInfo
{
    public:



        CReaderWriterLockGroupInfo() {}
        ~CReaderWriterLockGroupInfo() {}


        void StartWait( const INT iGroup ) { m_rginfo[iGroup].StartWait(); }
        void StopWait( const INT iGroup ) { m_rginfo[iGroup].StopWait(); }

        void SetAcquire( const INT iGroup ) { m_rginfo[iGroup].SetAcquire(); }
        void SetContend( const INT iGroup ) { m_rginfo[iGroup].SetContend(); }

        void StartHold( const INT iGroup ) { m_rginfo[iGroup].StartHold(); }
        void StopHold( const INT iGroup ) { m_rginfo[iGroup].StopHold(); }


#ifdef SYNC_ANALYZE_PERFORMANCE

        QWORD   CWaitTotal( const INT iGroup ) const        { return m_rginfo[iGroup].CWaitTotal(); }
        double  CsecWaitElapsed( const INT iGroup ) const   { return m_rginfo[iGroup].CsecWaitElapsed(); }

        QWORD   CAcquireTotal( const INT iGroup ) const     { return m_rginfo[iGroup].CAcquireTotal(); }
        QWORD   CContendTotal( const INT iGroup ) const     { return m_rginfo[iGroup].CContendTotal(); }

        QWORD   CHoldTotal( const INT iGroup ) const        { return m_rginfo[iGroup].CHoldTotal(); }
        double  CsecHoldElapsed( const INT iGroup ) const   { return m_rginfo[iGroup].CsecHoldElapsed(); }

#endif


        void Dump( const CDumpContext& dc ) const;

    private:



        CReaderWriterLockGroupInfo& operator=( CReaderWriterLockGroupInfo& ) = delete;



        CReaderWriterLockPerfInfo m_rginfo[2];
};



class CReaderWriterLockInfo
    :   public CLockBasicInfo,
        public CReaderWriterLockGroupInfo,
        public CLockDeadlockDetectionInfo
{
    public:



        CReaderWriterLockInfo( const CLockBasicInfo& lbi )
            :   CLockBasicInfo( lbi ),
                CLockDeadlockDetectionInfo( (CLockBasicInfo&) *this )
        {
        }


        void Dump( const CDumpContext& dc ) const;
};



class CReaderWriterLockState
{
    public:



        typedef DWORD ControlWord;



        CReaderWriterLockState( const CSyncBasicInfo& sbi );
        ~CReaderWriterLockState();


        void Dump( const CDumpContext& dc ) const;



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


        volatile DWORD          m_cOwner;


        CSemaphore              m_semWriter;


        CSemaphore              m_semReader;

    private:



        CReaderWriterLockState& operator=( CReaderWriterLockState& ) = delete;
};



class CReaderWriterLock
    :   private CLockObject,
        private CEnhancedStateContainer< CReaderWriterLockState, CSyncBasicInfo, CReaderWriterLockInfo, CLockBasicInfo >
{
    public:



        typedef CReaderWriterLockState::ControlWord ControlWord;


        enum TransitionReason
        {
            trIllegal = 0,
            trEnterAsWriter = 1,
            trLeaveAsWriter = 2,
            trEnterAsReader = 4,
            trLeaveAsReader = 8,
        };



        CReaderWriterLock( const CLockBasicInfo& lbi );
        ~CReaderWriterLock();


        void EnterAsWriter();
        const BOOL FTryEnterAsWriter();
        void LeaveAsWriter();

        void EnterAsReader();
        const BOOL FTryEnterAsReader();
        void LeaveAsReader();


        const BOOL FWritersQuiesced() const   { return State().m_cw & 0x00008000; }
        const BOOL FReadersQuiesced() const   { return State().m_cw & 0x80000000; }

#ifdef SYNC_DEADLOCK_DETECTION

        const BOOL FWriter() const      { return State().FOwner( 0 ); }
        const BOOL FNotWriter() const   { return State().FNotOwner( 0 ); }
        const BOOL FReader() const      { return State().FOwner( 1 ); }
        const BOOL FNotReader() const   { return State().FNotOwner( 1 ); }

#else

#ifdef DEBUG

        const BOOL FWriter() const      { return fTrue; }
        const BOOL FNotWriter() const   { return fTrue; }
        const BOOL FReader() const      { return fTrue; }
        const BOOL FNotReader() const   { return fTrue; }

#endif

#endif


        void Dump( const CDumpContext& dc ) const;

    private:



        CReaderWriterLock& operator=( CReaderWriterLock& ) = delete;


        INT _StateFromControlWord( const ControlWord cw );
        BOOL _FValidStateTransition(    const ControlWord cwBI,
                                        const ControlWord cwAI,
                                        const TransitionReason tr );


        void _EnterAsWriter( const ControlWord cwBIOld );
        void _EnterAsReader( const ControlWord cwBIOld );

        void _UpdateQuiescedOwnerCountAsWriter( const DWORD cOwnerDelta );
        void _UpdateQuiescedOwnerCountAsReader( const DWORD cOwnerDelta );
};


inline void CReaderWriterLock::EnterAsWriter()
{

    OSSYNCAssert( State().FNotOwner( 0 ) );
    OSSYNCAssert( State().FNotOwner( 1 ) );


    State().AssertCanBeWaiter();


    OSSYNC_FOREVER
    {

        const ControlWord cwBIExpected = State().m_cw;


        const ControlWord cwAI =    ControlWord( ( ( cwBIExpected & ( ( LONG_PTR( LONG( cwBIExpected ) ) >> 31 ) |
                                    0x0000FFFF ) ) | 0x80000000 ) + 0x00000001 );


        OSSYNCAssert( _FValidStateTransition( cwBIExpected, cwAI, trEnterAsWriter ) );


        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );


        if ( ( cwBI ^ cwBIExpected ) | ( cwBI & 0x0000FFFF ) )
        {

            if ( cwBI != cwBIExpected )
            {

                continue;
            }


            else
            {

                State().SetContend( 0 );


                _EnterAsWriter( cwBI );


                break;
            }
        }


        else
        {

            break;
        }
    }


    State().SetAcquire( 0 );
    State().AddAsOwner( 0 );
    State().StartHold( 0 );
}



inline const BOOL CReaderWriterLock::FTryEnterAsWriter()
{


    OSSYNCAssert( State().FNotOwner( 1 ) );


    if ( State().m_cw & 0x0000FFFF )
    {
        return fFalse;
    }

    
    OSSYNCAssert( State().FNotOwner( 0 ) );


    OSSYNC_FOREVER
    {

        ControlWord cwBIExpected = State().m_cw;


        cwBIExpected = cwBIExpected & 0xFFFF0000;


        const ControlWord cwAI =    ControlWord( ( ( cwBIExpected & ( ( LONG_PTR( LONG( cwBIExpected ) ) >> 31 ) |
                                    0x0000FFFF ) ) | 0x80000000 ) + 0x00000001 );


        OSSYNCAssert(   _StateFromControlWord( cwBIExpected ) < 0 ||
                _FValidStateTransition( cwBIExpected, cwAI, trEnterAsWriter ) );


        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );


        if ( cwBI != cwBIExpected )
        {

            if ( cwBI & 0x0000FFFF )
            {

                return fFalse;
            }


            else
            {

                continue;
            }
        }


        else
        {

            State().SetAcquire( 0 );
            State().AddAsOwner( 0 );
            State().StartHold( 0 );


            return fTrue;
        }
    }
}



inline void CReaderWriterLock::LeaveAsWriter()
{

    State().RemoveAsOwner( 0 );


    State().StopHold( 0 );


    OSSYNC_FOREVER
    {

        ControlWord cwBIExpected = State().m_cw;


        cwBIExpected = cwBIExpected & 0xFFFF7FFF;


        ControlWord cwAI =  cwBIExpected + 0xFFFFFFFF;
        cwAI = cwAI - ( ( ( cwAI + 0xFFFFFFFF ) << 16 ) & 0x80000000 );


        OSSYNCAssert(   _StateFromControlWord( cwBIExpected ) < 0 ||
                _FValidStateTransition( cwBIExpected, cwAI, trLeaveAsWriter ) );


        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );


        if ( cwBI != cwBIExpected )
        {

            if ( cwBI & 0x00008000 )
            {

                _UpdateQuiescedOwnerCountAsWriter( 0xFFFFFFFF );


                break;
            }


            else
            {

                continue;
            }
        }


        else
        {

            if ( cwAI & 0x00007FFF )
            {

                State().m_semWriter.Release();
            }


            break;
        }
    }
}


inline void CReaderWriterLock::EnterAsReader()
{

    OSSYNCAssert( State().FNotOwner( 0 ) );
    OSSYNCAssert( State().FNotOwner( 1 ) );


    State().AssertCanBeWaiter();


    OSSYNC_FOREVER
    {

        const ControlWord cwBIExpected = State().m_cw;


        const ControlWord cwAI =    ( cwBIExpected & 0xFFFF7FFF ) +
                                    (   ( cwBIExpected & 0x80008000 ) == 0x80000000 ?
                                            0x00017FFF :
                                            0x00018000 );


        OSSYNCAssert( _FValidStateTransition( cwBIExpected, cwAI, trEnterAsReader ) );


        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );


        if ( ( cwBI ^ cwBIExpected ) | ( cwBI & 0x80000000 ) )
        {

            if ( cwBI != cwBIExpected )
            {

                continue;
            }


            else
            {

                State().SetContend( 1 );


                _EnterAsReader( cwBI );


                break;
            }
        }


        else
        {

            break;
        }
    }


    State().SetAcquire( 1 );
    State().AddAsOwner( 1 );
    State().StartHold( 1 );
}


inline const BOOL CReaderWriterLock::FTryEnterAsReader()
{

    OSSYNCAssert( State().FNotOwner( 0 ) );
    OSSYNCAssert( State().FNotOwner( 1 ) );


    OSSYNC_FOREVER
    {

        ControlWord cwBIExpected = State().m_cw;


        cwBIExpected = cwBIExpected & 0x7FFFFFFF;


        const ControlWord cwAI =    ( cwBIExpected & 0xFFFF7FFF ) +
                                    (   ( cwBIExpected & 0x80008000 ) == 0x80000000 ?
                                            0x00017FFF :
                                            0x00018000 );


        OSSYNCAssert(   _StateFromControlWord( cwBIExpected ) < 0 ||
                _FValidStateTransition( cwBIExpected, cwAI, trEnterAsReader ) );


        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );


        if ( cwBI != cwBIExpected )
        {

            if ( cwBI & 0x80000000 )
            {

                return fFalse;
            }


            else
            {

                continue;
            }
        }


        else
        {

            State().SetAcquire( 1 );
            State().AddAsOwner( 1 );
            State().StartHold( 1 );


            return fTrue;
        }
    }
}


inline void CReaderWriterLock::LeaveAsReader()
{

    State().RemoveAsOwner( 1 );


    State().StopHold( 1 );


    OSSYNC_FOREVER
    {

        ControlWord cwBIExpected = State().m_cw;


        cwBIExpected = cwBIExpected & 0x7FFFFFFF;


        const ControlWord cwAI =    ControlWord( cwBIExpected + 0xFFFF0000 +
                                    ( ( LONG_PTR( LONG( cwBIExpected + 0xFFFE0000 ) ) >> 31 ) & 0xFFFF8000 ) );


        OSSYNCAssert(   _StateFromControlWord( cwBIExpected ) < 0 ||
                _FValidStateTransition( cwBIExpected, cwAI, trLeaveAsReader ) );


        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );


        if ( cwBI != cwBIExpected )
        {

            if ( cwBI & 0x80000000 )
            {

                _UpdateQuiescedOwnerCountAsReader( 0xFFFFFFFF );


                break;
            }


            else
            {

                continue;
            }
        }


        else
        {

            break;
        }
    }
}




class CMeteredSection
    :   private CSyncObject
{
    public:



        const static INT    cMaxActive = 0x7ffe;


        typedef INT Group;
        const static Group  groupInvalidNil         = -1;
        const static Group  groupTooManyActiveErr   = -2;


        typedef DWORD ControlWord;


        typedef void (*PFNPARTITIONCOMPLETE)( const DWORD_PTR dwCompletionKey );



        CMeteredSection();
        ~CMeteredSection();


        INT Enter();
        Group GroupEnter();
        void Leave( const Group group );

        void Partition( const PFNPARTITIONCOMPLETE  pfnPartitionComplete    = NULL,
                        const DWORD_PTR             dwCompletionKey         = NULL );


        Group GroupActive() const   { const INT iRet = INT( m_groupCurrent ); OSSYNCAssert( iRet == 0 || iRet == 1 ); return iRet; }
        Group GroupInactive() const { return 1 - GroupActive(); }
        BOOL FEmpty() const         { return ( m_cw & 0x7FFF7FFF ) == 0; }
#ifndef DEBUG
        BOOL FQuiescing() const     { return m_cQuiesced > 0; }
        INT CActiveUsers() const    { return INT( m_cCurrent ); }
        INT CQuiescingUsers() const { return INT( m_cQuiesced ); }
#endif


        void Dump( const CDumpContext& dc ) const;

    private:



        PFNPARTITIONCOMPLETE    m_pfnPartitionComplete;
        DWORD_PTR               m_dwPartitionCompleteKey;


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



        CMeteredSection& operator=( CMeteredSection& ) = delete;


        void _PartitionAsync(   const PFNPARTITIONCOMPLETE  pfnPartitionComplete,
                                const DWORD_PTR             dwCompletionKey );
        static void _PartitionSyncComplete( CAutoResetSignal* const pasig );


    private:

        CMeteredSection( const CMeteredSection& ms ) = delete;
};



inline CMeteredSection::CMeteredSection()
    :   m_cw( 0x80000000 ),
        m_pfnPartitionComplete( NULL ),
        m_dwPartitionCompleteKey( NULL )
{
}


inline CMeteredSection::~CMeteredSection()
{
    
    OSSYNCAssert( FEmpty() || g_fSyncProcessAbort );
}


inline CMeteredSection::Group CMeteredSection::GroupEnter()
{
    Group groupRet = groupInvalidNil;


    OSSYNC_FOREVER
    {


        const ControlWord cwBIExpected = m_cw;


        const ControlWord cwAI = cwBIExpected + 1 ;

        if ( ( cwAI & 0x00007fff ) > cMaxActive )
        {

            groupRet = groupTooManyActiveErr;
            break;
        }


        OSSYNCAssert( ( cwBIExpected & 0x80008000 ) == ( cwAI & 0x80008000 ) );


        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&m_cw, cwBIExpected, cwAI );

        if ( cwBI != cwBIExpected )
        {

            continue;
        }
        else
        {


            OSSYNCAssert( ( cwBI & 0x80008000 ) == ( ( cwBI + 1  ) & 0x80008000 ) );
            OSSYNCAssert( ( cwBI & 0x00007fff ) != 0x7fff  );


            OSSYNCAssert( ( cwBIExpected & 0x80008000 ) == ( cwBI & 0x80008000 ) );


            groupRet = INT( ( cwBI >> 15 ) & 1 );

            OSSYNCAssert( groupRet == 0 || groupRet == 1 );

            break;
        }
    }


    OSSYNCAssert( groupRet != groupInvalidNil );
    OSSYNCAssert( groupRet == 0 || groupRet == 1 || groupRet == groupTooManyActiveErr );


    return groupRet;
}


inline INT CMeteredSection::Enter()
{
    const Group groupRet = GroupEnter();


    OSSYNCEnforceSz( groupRet != groupTooManyActiveErr, "Too many active threads/workers in this section, must quit to avoid consistency issues!" );

    return groupRet;
}


inline void CMeteredSection::Leave( const Group group )
{

    OSSYNCAssert( group == 0 || group == 1 );
    

    OSSYNC_FOREVER
    {

        const ControlWord cwBIExpected = m_cw;


        OSSYNCAssert( ( cwBIExpected & 0x00007fff ) != 0x7fff );


        const ControlWord cwAI = cwBIExpected - ( ( ( ( cwBIExpected & 0x80008000 ) ^ 0x80008000 ) >> 15 ) ^ ( ( group << 16 ) | group ) );


        OSSYNCAssert( ( cwAI & 0x00007fff ) != 0x7fff );
        OSSYNCAssert( ( cwAI >> 16 ) != 0x7fff );
        OSSYNCAssert( ( cwBIExpected & 0x80008000 ) == ( cwAI & 0x80008000 ) );


        OSSYNCEnforceSz( ( cwBIExpected & 0x80008000 ) == ( cwAI & 0x80008000 ),  "Underflow of a metered section, left a group we didn't enter!!!" );


        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&m_cw, cwBIExpected, cwAI );


        if ( cwBI != cwBIExpected )
        {

            continue;
        }


        else
        {

            if ( ( cwBI & 0x7FFF0000 ) + ( cwAI & 0x7FFF0000 ) == 0x00010000 )
            {

                m_pfnPartitionComplete( m_dwPartitionCompleteKey );
            }


            break;
        }
    }
}


inline void CMeteredSection::Partition( const PFNPARTITIONCOMPLETE  pfnPartitionComplete,
                                        const DWORD_PTR             dwCompletionKey )
{

    if ( pfnPartitionComplete )
    {

        _PartitionAsync( pfnPartitionComplete, dwCompletionKey );
    }


    else
    {

        CAutoResetSignal asig( CSyncBasicInfo( "CMeteredSection::Partition()::asig" ) );


        _PartitionAsync(    PFNPARTITIONCOMPLETE( _PartitionSyncComplete ),
                            DWORD_PTR( &asig ) );


        asig.Wait();
    }
}


inline void CMeteredSection::_PartitionAsync(   const PFNPARTITIONCOMPLETE  pfnPartitionComplete,
                                                const DWORD_PTR             dwCompletionKey )
{

    OSSYNCAssertSz( !( m_cw & 0x7FFF0000 ), "Illegal concurrent use of Partitioning" );


    m_pfnPartitionComplete      = pfnPartitionComplete;
    m_dwPartitionCompleteKey    = dwCompletionKey;


    OSSYNC_FOREVER
    {

        const ControlWord cwBIExpected = m_cw;


        const ControlWord cwAI = ( cwBIExpected >> 16 ) | ( cwBIExpected << 16 );


        OSSYNCAssert( ( cwBIExpected & 0x00007fff ) != 0x7fff );
        OSSYNCAssert( ( cwBIExpected >> 16 ) != 0x7fff );
        OSSYNCAssert( ( cwAI & 0x00007fff ) != 0x7fff );
        OSSYNCAssert( ( cwAI >> 16 ) != 0x7fff );


        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&m_cw, cwBIExpected, cwAI );


        if ( cwBI != cwBIExpected )
        {

            continue;
        }


        else
        {

            if ( !( cwAI & 0x7FFF0000 ) )
            {

                m_pfnPartitionComplete( m_dwPartitionCompleteKey );
            }


            break;
        }
    }
}


inline void CMeteredSection::_PartitionSyncComplete( CAutoResetSignal* const pasig )
{

    pasig->Set();
}



class CSXWLatchPerfInfo
    :   public CSyncPerfWait,
        public CSyncPerfAcquire,
        public CLockPerfHold
{
    public:



        CSXWLatchPerfInfo()
        {
        }


        void Dump( const CDumpContext& dc ) const;
};



class CSXWLatchGroupInfo
{
    public:



        CSXWLatchGroupInfo() {}
        ~CSXWLatchGroupInfo() {}


        void StartWait( const INT iGroup ) { m_rginfo[iGroup].StartWait(); }
        void StopWait( const INT iGroup ) { m_rginfo[iGroup].StopWait(); }

        void SetAcquire( const INT iGroup ) { m_rginfo[iGroup].SetAcquire(); }
        void SetContend( const INT iGroup ) { m_rginfo[iGroup].SetContend(); }

        void StartHold( const INT iGroup ) { m_rginfo[iGroup].StartHold(); }
        void StopHold( const INT iGroup ) { m_rginfo[iGroup].StopHold(); }


#ifdef SYNC_ANALYZE_PERFORMANCE

        QWORD   CWaitTotal( const INT iGroup ) const        { return m_rginfo[iGroup].CWaitTotal(); }
        double  CsecWaitElapsed( const INT iGroup ) const   { return m_rginfo[iGroup].CsecWaitElapsed(); }

        QWORD   CAcquireTotal( const INT iGroup ) const     { return m_rginfo[iGroup].CAcquireTotal(); }
        QWORD   CContendTotal( const INT iGroup ) const     { return m_rginfo[iGroup].CContendTotal(); }

        QWORD   CHoldTotal( const INT iGroup ) const        { return m_rginfo[iGroup].CHoldTotal(); }
        double  CsecHoldElapsed( const INT iGroup ) const   { return m_rginfo[iGroup].CsecHoldElapsed(); }

#endif


        void Dump( const CDumpContext& dc ) const;

    private:



        CSXWLatchGroupInfo& operator=( CSXWLatchGroupInfo& ) = delete;



        CSXWLatchPerfInfo m_rginfo[3];
};



class CSXWLatchInfo
    :   public CLockBasicInfo,
        public CSXWLatchGroupInfo,
        public CLockDeadlockDetectionInfo
{
    public:



        CSXWLatchInfo( const CLockBasicInfo& lbi )
            :   CLockBasicInfo( lbi ),
                CLockDeadlockDetectionInfo( (CLockBasicInfo&) *this )
        {
        }


        void Dump( const CDumpContext& dc ) const;
};



class CSXWLatchState
{
    public:



        typedef DWORD ControlWord;



        CSXWLatchState( const CSyncBasicInfo& sbi );
        ~CSXWLatchState();


        void Dump( const CDumpContext& dc ) const;



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


        volatile DWORD          m_cQS;


        CSemaphore              m_semS;


        CSemaphore              m_semX;


        CSemaphore              m_semW;

    private:



        CSXWLatchState& operator=( CSXWLatchState& ) = delete;
};



class CSXWLatch
    :   private CLockObject,
        private CEnhancedStateContainer< CSXWLatchState, CSyncBasicInfo, CSXWLatchInfo, CLockBasicInfo >
{
    public:



        typedef CSXWLatchState::ControlWord ControlWord;


        enum class ERR
        {
            errSuccess,
            errWaitForSharedLatch,
            errWaitForExclusiveLatch,
            errWaitForWriteLatch,
            errLatchConflict
        };


        enum iGroup
        {
            iSharedGroup = 0,
            iExclusiveGroup = 1,
            iWriteGroup = 2,
            iNoGroup = 3
        };



        CSXWLatch( const CLockBasicInfo& lbi );
        ~CSXWLatch();


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


        const INT CWaitSharedLatch() const      { return State().m_semS.CWait(); }
        const INT CWaitExclusiveLatch() const   { return State().m_semX.CWait(); }
        const INT CWaitWriteLatch() const       { return State().m_semW.CWait(); }

        BOOL FLatched()  const                  { return State().m_cw != 0; }


#if defined( DEBUG ) || defined( DEBUGGER_EXTENSION )



        BOOL FSharedLatched() const
        {
            return CSharers() >= 0x00000001;
        }
        BOOL FExclusiveLatched() const
        {
            return ( ( State().m_cw & 0xFFFF0000 ) >= 0x00010000 ) &&
                    ( 0x8000 != ( State().m_cw & 0x00008000 ) );
        }
        BOOL FWriteLatched() const
        {
            return ( ( State().m_cw & 0xFFFF0000 ) >= 0x00010000 ) &&
                    ( 0x8000 == ( State().m_cw & 0x00008000 ) &&
                    ( 0 == State().m_cQS ) );
        }

        INT CSharers() const
        {
            return ( 0x8000 != ( State().m_cw & 0x00008000 ) ) ?
                        ( State().m_cw & 0x00007FFF ) :
                        State().m_cQS;
        }

    public:

#endif


#ifdef SYNC_DEADLOCK_DETECTION

        BOOL FOwnSharedLatch()                  { return State().FOwner( 0 ); }
        BOOL FNotOwnSharedLatch()               { return State().FNotOwner( 0 ); }
        BOOL FOwnExclusiveLatch()               { return State().FOwner( 1 ); }
        BOOL FNotOwnExclusiveLatch()            { return State().FNotOwner( 1 ); }
        BOOL FOwnWriteLatch()                   { return State().FOwner( 2 ); }
        BOOL FNotOwnWriteLatch()                { return State().FNotOwner( 2 ); }
        BOOL FOwner()                           { return FOwnSharedLatch() || FOwnExclusiveLatch() || FOwnWriteLatch(); }
        BOOL FNotOwner()                        { return FNotOwnSharedLatch() && FNotOwnExclusiveLatch() && FNotOwnWriteLatch(); }

#else

#ifdef DEBUG

        BOOL FOwnSharedLatch()                  { return fTrue; }
        BOOL FNotOwnSharedLatch()               { return fTrue; }
        BOOL FOwnExclusiveLatch()               { return fTrue; }
        BOOL FNotOwnExclusiveLatch()            { return fTrue; }
        BOOL FOwnWriteLatch()                   { return fTrue; }
        BOOL FNotOwnWriteLatch()                { return fTrue; }
        BOOL FOwner()                           { return fTrue; }
        BOOL FNotOwner()                        { return fTrue; }

#endif

#endif


        void Dump( const CDumpContext& dc ) const;

        void AssertValid( __in const iGroup iGroupOwnsOrWaits = iNoGroup );

    private:



        CSXWLatch& operator=( CSXWLatch& ) = delete;


        void _UpdateQuiescedSharedLatchCount( const DWORD cQSDelta );
};


inline void CSXWLatch::AssertValid( __in const iGroup iGroupOwnsOrWaits )
{
    OSSYNCEnforce( ( State().m_cw & 0x00007fff ) != 0x00007fff );
    OSSYNCEnforce( ( State().m_cw & 0xffff0000 ) != 0xffff0000 );

#ifndef RTM
    switch( iGroupOwnsOrWaits )
    {
        case iSharedGroup:
            break;
        case iExclusiveGroup:
            OSSYNCEnforce( State().m_cw & 0xffff0000 );
            break;
        case iWriteGroup:
            OSSYNCEnforce( State().m_cw & 0xffff0000 );
            OSSYNCEnforce( ( State().m_cw & 0x00008000 ) == 0x00008000 );
            break;
        case iNoGroup:
            break;
        default:
            OSSYNCAssertSzRTL( fFalse, "Unknown latch group." );
    }
#endif
}


inline CSXWLatch::ERR CSXWLatch::ErrAcquireSharedLatch()
{

    OSSYNCAssert( FNotOwner() );


    AssertValid();


    const ControlWord cwDelta = 0x00000001;
    const ControlWord cwBI = AtomicExchangeAdd( (LONG*)&State().m_cw, cwDelta );


    if ( cwBI & 0x00008000 )
    {

        State().SetContend( 0 );


        State().AddAsWaiter( 0 );
        State().StartWait( 0 );

        AssertValid( iSharedGroup );


        return ERR::errWaitForSharedLatch;
    }


    else
    {

        State().SetAcquire( 0 );
        State().AddAsOwner( 0 );
        State().StartHold( 0 );

        AssertValid( iSharedGroup );


        return ERR::errSuccess;
    }
}


inline CSXWLatch::ERR CSXWLatch::ErrTryAcquireSharedLatch()
{

    OSSYNCAssert( FNotOwnSharedLatch() );


    OSSYNC_FOREVER
    {

        AssertValid();


        ControlWord cwBIExpected = State().m_cw;


        cwBIExpected = cwBIExpected & 0xFFFF7FFF;


        const ControlWord cwAI = cwBIExpected + 0x00000001;


        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );


        if ( cwBI != cwBIExpected )
        {

            if ( cwBI & 0x00008000 )
            {

                State().SetContend( 0 );


                AssertValid();


                return ERR::errLatchConflict;
            }


            else
            {

                continue;
            }
        }


        else
        {

            State().SetAcquire( 0 );
            State().AddAsOwner( 0 );
            State().StartHold( 0 );

            AssertValid( iSharedGroup );


            return ERR::errSuccess;
        }
    }
}


inline CSXWLatch::ERR CSXWLatch::ErrAcquireExclusiveLatch()
{

    OSSYNCAssert( FNotOwner() );


    AssertValid();


    const ControlWord cwDelta = 0x00010000;
    const ControlWord cwBI = AtomicExchangeAdd( (LONG*)&State().m_cw, cwDelta );


    if ( cwBI & 0xFFFF0000 )
    {

        State().SetContend( 1 );


        State().AddAsWaiter( 1 );
        State().StartWait( 1 );


        AssertValid( iExclusiveGroup );


        return ERR::errWaitForExclusiveLatch;
    }


    else
    {

        State().SetAcquire( 1 );
        State().AddAsOwner( 1 );
        State().StartHold( 1 );


        AssertValid( iExclusiveGroup );


        return ERR::errSuccess;
    }
}


inline CSXWLatch::ERR CSXWLatch::ErrTryAcquireExclusiveLatch()
{

    OSSYNCAssert( FNotOwnSharedLatch() );
    OSSYNCAssert( FNotOwnExclusiveLatch() );
    OSSYNCAssert( FNotOwnWriteLatch() );



    OSSYNC_FOREVER
    {

        AssertValid();


        ControlWord cwBIExpected = State().m_cw;


        cwBIExpected = cwBIExpected & 0x0000FFFF;


        const ControlWord cwAI = cwBIExpected + 0x00010000;


        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );


        if ( cwBI != cwBIExpected )
        {

            if ( cwBI & 0xFFFF0000 )
            {

                State().SetContend( 1 );


                AssertValid();


                return ERR::errLatchConflict;
            }


            else
            {

                continue;
            }
        }


        else
        {

            State().SetAcquire( 1 );
            State().AddAsOwner( 1 );
            State().StartHold( 1 );


            AssertValid( iExclusiveGroup );


            return ERR::errSuccess;
        }
    }
}


inline CSXWLatch::ERR CSXWLatch::ErrTryAcquireWriteLatch()
{

    OSSYNCAssert( FNotOwnWriteLatch() );


    AssertValid();


    const ControlWord cwBIExpected = 0x00000000;


    const ControlWord cwAI = 0x00018000;


    const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );


    if ( cwBI != cwBIExpected )
    {

        State().SetContend( 2 );


        AssertValid();


        return ERR::errLatchConflict;
    }


    else
    {

        State().SetAcquire( 2 );
        State().AddAsOwner( 2 );
        State().StartHold( 2 );


        AssertValid( iWriteGroup );


        return ERR::errSuccess;
    }
}


inline CSXWLatch::ERR CSXWLatch::ErrUpgradeSharedLatchToExclusiveLatch()
{

    OSSYNCAssert( FOwnSharedLatch() );
    OSSYNCAssert( FNotOwnExclusiveLatch() );
    OSSYNCAssert( FNotOwnWriteLatch() );


    OSSYNC_FOREVER
    {

        AssertValid( iSharedGroup );


        ControlWord cwBIExpected = State().m_cw;


        cwBIExpected = cwBIExpected & 0x0000FFFF;


        const ControlWord cwAI = cwBIExpected + 0x0000FFFF;


        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );


        if ( cwBI != cwBIExpected )
        {

            if ( cwBI & 0xFFFF0000 )
            {

                State().SetContend( 1 );


                AssertValid( iSharedGroup );


                return ERR::errLatchConflict;
            }


            else
            {

                continue;
            }
        }


        else
        {

            State().RemoveAsOwner( 0 );
            State().StopHold( 0 );


            State().SetAcquire( 1 );
            State().AddAsOwner( 1 );
            State().StartHold( 1 );


            AssertValid( iExclusiveGroup );


            return ERR::errSuccess;
        }
    }
}


inline CSXWLatch::ERR CSXWLatch::ErrUpgradeSharedLatchToWriteLatch()
{

    OSSYNCAssert( FOwnSharedLatch() );
    OSSYNCAssert( FNotOwnExclusiveLatch() );
    OSSYNCAssert( FNotOwnWriteLatch() );


    OSSYNC_FOREVER
    {

        AssertValid( iSharedGroup );


        ControlWord cwBIExpected = State().m_cw;


        cwBIExpected = cwBIExpected & 0x0000FFFF;


        const ControlWord cwAI = 0x00018000;


        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );


        if ( cwBI != cwBIExpected )
        {

            if ( cwBI & 0xFFFF0000 )
            {

                State().SetContend( 2 );


                AssertValid( iSharedGroup );


                return ERR::errLatchConflict;
            }


            else
            {

                continue;
            }
        }


        else
        {

            if ( cwBI != 0x00000001 )
            {

                State().RemoveAsOwner( 0 );
                State().StopHold( 0 );


                _UpdateQuiescedSharedLatchCount( cwBI - 1 );


                State().AddAsWaiter( 2 );
                State().StartWait( 2 );


                AssertValid( iWriteGroup );


                return ERR::errWaitForWriteLatch;
            }


            else
            {

                State().RemoveAsOwner( 0 );
                State().StopHold( 0 );


                State().SetAcquire( 2 );
                State().AddAsOwner( 2 );
                State().StartHold( 2 );


                AssertValid( iWriteGroup );


                return ERR::errSuccess;
            }
        }
    }
}


inline CSXWLatch::ERR CSXWLatch::ErrUpgradeExclusiveLatchToWriteLatch()
{

    OSSYNCAssert( FNotOwnSharedLatch() );
    OSSYNCAssert( FOwnExclusiveLatch() );
    OSSYNCAssert( FNotOwnWriteLatch() );


    State().RemoveAsOwner( 1 );
    State().StopHold( 1 );


    OSSYNC_FOREVER
    {

        AssertValid( iExclusiveGroup );


        const ControlWord cwBIExpected = State().m_cw;


        const ControlWord cwAI = ( cwBIExpected & 0xFFFF0000 ) | 0x00008000;


        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );


        if ( cwBI != cwBIExpected )
        {

            continue;
        }


        else
        {

            if ( cwBI & 0x00007FFF )
            {

                State().SetContend( 2 );


                _UpdateQuiescedSharedLatchCount( cwBI & 0x00007FFF );


                State().AddAsWaiter( 2 );
                State().StartWait( 2 );


                AssertValid( iWriteGroup );


                return ERR::errWaitForWriteLatch;
            }


            else
            {

                State().SetAcquire( 2 );
                State().AddAsOwner( 2 );
                State().StartHold( 2 );


                AssertValid( iWriteGroup );


                return ERR::errSuccess;
            }
        }
    }
}


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


inline CSXWLatch::ERR CSXWLatch::ErrTryUpgradeSharedLatchToWriteLatch()
{

    OSSYNCAssert( FOwnSharedLatch() );
    OSSYNCAssert( FNotOwnExclusiveLatch() );
    OSSYNCAssert( FNotOwnWriteLatch() );


    AssertValid( iSharedGroup );


    const ControlWord cwBIExpected = 0x00000001;


    const ControlWord cwAI = 0x00018000;


    const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );


    if ( cwBI != cwBIExpected )
    {

        State().SetContend( 2 );


        AssertValid( iSharedGroup );


        return ERR::errLatchConflict;
    }


    else
    {

        State().RemoveAsOwner( 0 );
        State().StopHold( 0 );


        State().SetAcquire( 2 );
        State().AddAsOwner( 2 );
        State().StartHold( 2 );


        AssertValid( iWriteGroup );


        return ERR::errSuccess;
    }
}


inline CSXWLatch::ERR CSXWLatch::ErrTryUpgradeExclusiveLatchToWriteLatch()
{

    OSSYNCAssert( FNotOwnSharedLatch() );
    OSSYNCAssert( FOwnExclusiveLatch() );
    OSSYNCAssert( FNotOwnWriteLatch() );


    AssertValid( iExclusiveGroup );


    AssertValid( iExclusiveGroup );


    OSSYNCAssert( State().m_cw & 0x7FFF0000 );


    const ControlWord cwBIExpected = 0x00010000;


    const ControlWord cwAI = 0x00018000;


    const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );


    if ( cwBI != cwBIExpected )
    {

        State().SetContend( 2 );


        AssertValid( iExclusiveGroup );


        return ERR::errLatchConflict;
    }


    else
    {

        State().RemoveAsOwner( 1 );
        State().StopHold( 1 );


        State().SetAcquire( 2 );
        State().AddAsOwner( 2 );
        State().StartHold( 2 );


        AssertValid( iWriteGroup );


        return ERR::errSuccess;
    }
}


inline void CSXWLatch::DowngradeWriteLatchToExclusiveLatch()
{

    OSSYNCAssert( FNotOwnSharedLatch() );
    OSSYNCAssert( FNotOwnExclusiveLatch() );
    OSSYNCAssert( FOwnWriteLatch() );


    AssertValid( iWriteGroup );


    const ControlWord cwDelta = 0xFFFF8000;
    const ControlWord cwBI = AtomicExchangeAdd( (LONG*)&State().m_cw, cwDelta );


    State().RemoveAsOwner( 2 );
    State().StopHold( 2 );

    State().SetAcquire( 1 );
    State().AddAsOwner( 1 );
    State().StartHold( 1 );


    if ( cwBI & 0x00007FFF )
    {

        AssertValid( iSharedGroup );

        State().m_semS.Release( cwBI & 0x00007FFF );
    }


    AssertValid( iExclusiveGroup );
}


inline void CSXWLatch::DowngradeWriteLatchToSharedLatch()
{

    OSSYNCAssert( FNotOwnSharedLatch() );
    OSSYNCAssert( FNotOwnExclusiveLatch() );
    OSSYNCAssert( FOwnWriteLatch() );


    AssertValid( iWriteGroup );


    const ControlWord cwDelta = 0xFFFE8001;
    const ControlWord cwBI = AtomicExchangeAdd( (LONG*)&State().m_cw, cwDelta );


    State().RemoveAsOwner( 2 );
    State().StopHold( 2 );

    State().SetAcquire( 0 );
    State().AddAsOwner( 0 );
    State().StartHold( 0 );


    AssertValid();


    if ( cwBI & 0x00007FFF )
    {

        AssertValid( iSharedGroup );

        State().m_semS.Release( cwBI & 0x00007FFF );
    }


    if ( cwBI >= 0x00020000 )
    {

        AssertValid( iExclusiveGroup );

        State().m_semX.Release();
    }


    AssertValid( iSharedGroup );
}


inline void CSXWLatch::DowngradeExclusiveLatchToSharedLatch()
{

    OSSYNCAssert( FNotOwnSharedLatch() );
    OSSYNCAssert( FOwnExclusiveLatch() );
    OSSYNCAssert( FNotOwnWriteLatch() );


    AssertValid( iExclusiveGroup );


    const ControlWord cwDelta = 0xFFFF0001;
    const ControlWord cwBI = AtomicExchangeAdd( (LONG*)&State().m_cw, cwDelta );


    State().RemoveAsOwner( 1 );
    State().StopHold( 1 );

    State().SetAcquire( 0 );
    State().AddAsOwner( 0 );
    State().StartHold( 0 );


    AssertValid();


    if ( cwBI >= 0x00020000 )
    {

        AssertValid( iExclusiveGroup );

        State().m_semX.Release();
    }


    AssertValid( iSharedGroup );
}


inline void CSXWLatch::ReleaseWriteLatch()
{

    OSSYNCAssert( FNotOwnSharedLatch() );
    OSSYNCAssert( FNotOwnExclusiveLatch() );
    OSSYNCAssert( FOwnWriteLatch() );


    AssertValid( iWriteGroup );


    State().RemoveAsOwner( 2 );
    State().StopHold( 2 );


    const ControlWord cwDelta = 0xFFFE8000;
    const ControlWord cwBI = AtomicExchangeAdd( (LONG*)&State().m_cw, cwDelta );


    if ( cwBI & 0x00007FFF )
    {

        AssertValid( iSharedGroup );

        State().m_semS.Release( cwBI & 0x00007FFF );
    }




    if ( cwBI >= 0x00020000 )
    {

        AssertValid( iExclusiveGroup );

        State().m_semX.Release();
    }


}


inline void CSXWLatch::ReleaseExclusiveLatch()
{


    OSSYNCAssert( FOwnExclusiveLatch() );
    OSSYNCAssert( FNotOwnWriteLatch() );


    AssertValid( iExclusiveGroup );


    State().RemoveAsOwner( 1 );
    State().StopHold( 1 );


    const ControlWord cwDelta = 0xFFFF0000;
    const ControlWord cwBI = AtomicExchangeAdd( (LONG*)&State().m_cw, cwDelta );


    if ( cwBI >= 0x00020000 )
    {

        AssertValid( iExclusiveGroup );

        State().m_semX.Release();
    }


}


inline void CSXWLatch::ReleaseSharedLatch()
{

    OSSYNCAssert( FOwnSharedLatch() );
    OSSYNCAssert( FNotOwnExclusiveLatch() );
    OSSYNCAssert( FNotOwnWriteLatch() );


    State().RemoveAsOwner( 0 );
    State().StopHold( 0 );


    OSSYNC_FOREVER
    {

        AssertValid( iSharedGroup );


        ControlWord cwBIExpected = State().m_cw;


        cwBIExpected = cwBIExpected & 0xFFFF7FFF;


        const ControlWord cwAI = cwBIExpected + 0xFFFFFFFF;


        const ControlWord cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );


        if ( cwBI != cwBIExpected )
        {

            if ( cwBI & 0x00008000 )
            {

                AssertValid( iWriteGroup );


                _UpdateQuiescedSharedLatchCount( 0xFFFFFFFF );


                break;
            }


            else
            {

                continue;
            }
        }


        else
        {

            break;
        }
    }


}


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
            break;

        default:
            OSSYNCAssert( fFalse );
            break;
    }
}


inline void CSXWLatch::WaitForSharedLatch()
{

    State().AssertCanBeWaiter();


    OSSYNCAssert( State().FWaiter( 0 ) );
    AssertValid( iSharedGroup );


    State().m_semS.Acquire();

    State().StopWait( 0 );
    State().RemoveAsWaiter( 0 );

    State().SetAcquire( 0 );
    State().AddAsOwner( 0 );
    State().StartHold( 0 );


    AssertValid( iSharedGroup );
}


inline void CSXWLatch::WaitForExclusiveLatch()
{

    State().AssertCanBeWaiter();


    OSSYNCAssert( State().FWaiter( 1 ) );
    AssertValid( iExclusiveGroup );


    State().m_semX.Acquire();

    State().StopWait( 1 );
    State().RemoveAsWaiter( 1 );

    State().SetAcquire( 1 );
    State().AddAsOwner( 1 );
    State().StartHold( 1 );


    AssertValid( iExclusiveGroup );
}


inline void CSXWLatch::WaitForWriteLatch()
{

    State().AssertCanBeWaiter();


    OSSYNCAssert( State().FWaiter( 2 ) );
    AssertValid( iWriteGroup );


    State().m_semW.Acquire();

    State().StopWait( 2 );
    State().RemoveAsWaiter( 2 );

    State().SetAcquire( 2 );
    State().AddAsOwner( 2 );
    State().StartHold( 2 );


    AssertValid( iWriteGroup );
}


inline void CSXWLatch::AcquireSharedLatch()
{

    OSSYNCAssert( FNotOwner() );


    ERR err = ErrAcquireSharedLatch();

    if ( ERR::errWaitForSharedLatch == err )
    {
        WaitForSharedLatch();
        err = ERR::errSuccess;
    }
    OSSYNCAssert( ERR::errSuccess == err );

}


inline void CSXWLatch::AcquireExclusiveLatch()
{

    OSSYNCAssert( FNotOwner() );


    ERR err = ErrAcquireExclusiveLatch();

    if ( ERR::errWaitForExclusiveLatch == err )
    {
        WaitForExclusiveLatch();
        err = ERR::errSuccess;
    }
    OSSYNCAssert( ERR::errSuccess == err );

}


inline void CSXWLatch::AcquireWriteLatch()
{

    OSSYNCAssert( FNotOwner() );


    AcquireExclusiveLatch();


    UpgradeExclusiveLatchToWriteLatch();
}



inline void CSXWLatch::ClaimOwnership( const DWORD group )
{
#ifndef DEBUG
#ifdef DEBUGGER_EXTENSION
#ifndef RTM
    switch ( group )
    {
        case iSharedGroup:
        {
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
#endif
#endif
#endif
    State().AddAsOwner( group );
}


inline void CSXWLatch::ReleaseOwnership( const DWORD group )
{
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
#endif
#endif
#endif
    State().RemoveAsOwner( group );
}


inline void CSXWLatch::_UpdateQuiescedSharedLatchCount( const DWORD cQSDelta )
{

    const DWORD cQSBI = AtomicExchangeAdd( (LONG*)&State().m_cQS, cQSDelta );
    const DWORD cQSAI = cQSBI + cQSDelta;


    if ( !cQSAI )
    {

        AssertValid( iWriteGroup );


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
        m_semInit.FTryAcquire();
        m_semInit.Release();
        m_sigInit.Reset();
    }

 private:
    CSemaphore          m_semInit;
    CManualResetSignal  m_sigInit;
    Ret                 m_ret;
};


BOOL OSSYNCAPI FOSSyncPreinit();


void OSSYNCAPI OSSyncConfigDeadlockTimeoutDetection( const BOOL fEnable );


void OSSYNCAPI OSSyncProcessAbort();


void OSSYNCAPI OSSyncPostterm();



const BOOL OSSYNCAPI FOSSyncInitForES();
void OSSYNCAPI OSSyncTermForES();



typedef void ( OSSYNCAPI *PfnThreadWait )();

void OSSYNCAPI OSSyncOnThreadWaitBegin( PfnThreadWait pfn );
void OSSYNCAPI OSSyncOnThreadWaitEnd( PfnThreadWait pfn );

#endif

NAMESPACE_END( OSSYNC );

using namespace OSSYNC;


#endif

