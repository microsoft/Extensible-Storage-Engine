// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef _OS_ERROR_HXX_INCLUDED
#error ERROR.HXX already included
#endif
#define _OS_ERROR_HXX_INCLUDED



INT UtilMessageBoxW( IN const WCHAR * const wszText, IN const WCHAR * const wszCaption, IN const UINT uType );
void KernelDebugBreakPoint();

const CHAR * SzSourceFileName( const CHAR * szFilePath );


#ifdef DEBUG


BOOL IsDebuggerAttached();
#endif



#define ENABLE_EXCEPTIONS

#ifdef DEBUG

#ifdef RTM
#error: RTM defined in DEBUG build
#endif

#define RFS2

#endif

#ifndef RTM
#define TEST_INJECTION
#define FAULT_INJECTION
#define CONFIGOVERRIDE_INJECTION
#define HANG_INJECTION
#endif



#ifdef DEBUG

BOOL FOSSetCleanupState( BOOL fInCleanupState );

#else

#define FOSSetCleanupState( fInCleanupState ) (fInCleanupState)
#define FOSGetCleanupState() (fFalse)

#endif




#ifdef RFS2


#define RFSAlloc(type)              ( UtilRFSAlloc( L#type ,type ) )
#define LogJETCall( err )           { UtilRFSLogJETCall( __FUNCTION__, err, __FILE__, __LINE__ ); }
#define LogJETErr( err, label )     { UtilRFSLogJETErr( err, #label, __FILE__, __LINE__ ); }


BOOL UtilRFSAlloc( const WCHAR* const wszType, const INT Type );
BOOL FRFSFailureDetected( const UINT Type );
BOOL FRFSAnyFailureDetected();
void RFSSetKnownResourceLeak();
BOOL FRFSKnownResourceLeak();
LONG RFSThreadDisable( const LONG cRFSCountdown );
void RFSThreadReEnable( const LONG cRFSCountdownOld );
void UtilRFSLogJETCall( const CHAR* const szFunc, const ERR err, const CHAR* const szFile, const unsigned Line );
void UtilRFSLogJETErr( const ERR err, const CHAR* const szLabel, const CHAR* const szFile, const unsigned Line );



#define CSRAllocResource            0
#define FCBAllocResource            0
#define FUCBAllocResource           0
#define TDBAllocResource            0
#define IDBAllocResource            0
#define PIBAllocResource            0
#define SCBAllocResource            0
#define VERAllocResource            0
#define UnknownAllocResource        0

#define OSMemoryHeap                0
#define OSMemoryPageAddressSpace    0
#define OSMemoryPageBackingStore    0

#define OSFileDiskSpace             1
#define OSFileISectorSize           1
#define OSFileCreateDirectory       1
#define OSFileRemoveDirectory       1
#define OSFileFindExact             1
#define OSFileFindFirst             1
#define OSFileFindNext              1
#define OSFilePathComplete          1
#define OSFileCreate                1
#define OSFileOpen                  1
#define OSFileSetSize               1
#define OSFileDelete                1
#define OSFileMove                  1
#define OSFileCopy                  1
#define OSFileSize                  1
#define OSFileIsReadOnly            1
#define OSFileIsDirectory           1
#define OSFileRead                  1
#define OSFileWrite                 1
#define OSFileFlush                 1

#define RFSTypeMax                  2

#else

#define RFSAlloc( type )            (fTrue)
#define FRFSFailureDetected( type ) (fFalse)
#define FRFSAnyFailureDetected()    (fFalse)
#define RFSSetKnownResourceLeak()
#define FRFSKnownResourceLeak()     (fFalse)
#define RFSThreadDisable( cRFSCountdown ) (cRFSCountdown)
#define RFSThreadReEnable( cRFSCountdownOld )
#define LogJETCall( err )       { (err); }
#define LogJETErr( err, label )

#endif




void __stdcall AssertFail( const CHAR* szMessage, const CHAR* szFilename, LONG lLine, ... );

void AssertErr( const ERR err, const CHAR* szFilename, const LONG lLine );




#ifdef RTM
#define AssertSzRTL( exp, sz, ... )
#else
#define AssertSzRTL( exp, sz, ... )     ( ( exp ) ? (void) 0 : AssertFail( sz, __FILE__, __LINE__, __VA_ARGS__ ) )
#endif

#ifdef DEBUG
#define AssertSz( exp, sz, ... )            ( ( exp ) ? (void) 0 : AssertFail( sz, __FILE__, __LINE__, __VA_ARGS__ ) )
#else
#define AssertSz( exp, sz, ... )
#endif



#define AssertRTL( exp )            AssertSzRTL( exp, "%s", #exp )
#define Assert( exp )               AssertSz( exp, "%s", #exp )



#if ( defined _PREFIX_ || defined _PREFAST_ )
#define AssertPREFIX( exp )         ( ( exp ) ? (void)0 : exit(-1) )
#else
#define AssertPREFIX( exp )         AssertSz( exp, "%s", #exp )
#endif

#if ( defined _PREFIX_ || defined _PREFAST_ )
#define AssertRTLPREFIX( exp )          ( ( exp ) ? (void)0 : exit(-1) )
#else
#define AssertRTLPREFIX( exp )          AssertSzRTL( exp, "%s", #exp )
#endif

#define AssumePREFAST( exp )            __analysis_assume( exp )



#define Expected    Assert
#define ExpectedSz  AssertSz



#ifdef DEBUG
#define OnDebug( code )             code
#define OnDebug2( code1, code2 )    code1, code2
#else
#define OnDebug( code )
#define OnDebug2( code1, code2 )
#endif

#ifndef DEBUG
#define rtlconst            const
#else
#define rtlconst
#endif

#ifndef RTM
#define OnNonRTM( code )    code
#define OnRTM( code )
#else
#define OnNonRTM( code )
#define OnRTM( code )       code
#endif

#ifdef DEBUG
#define OnDebugOrRetail( debugvalue, retailvalue )      debugvalue
#else
#define OnDebugOrRetail( debugvalue, retailvalue )      retailvalue
#endif

#ifdef DEBUG
#define OnDebugOrRetailOrRtm( debugvalue, retailvalue, rtmvalue )       debugvalue
#else
#ifdef RTM
#define OnDebugOrRetailOrRtm( debugvalue, retailvalue, rtmvalue )       rtmvalue
#else
#define OnDebugOrRetailOrRtm( debugvalue, retailvalue, rtmvalue )       retailvalue
#endif
#endif




#define FireWallAt( szTag, szFilename, lLine )  if ( !FNegTest( fInvalidUsage ) )                                                                               \
                                                {                                                                                                               \
                                                    OSTraceSuspendGC_();                                                                                        \
                                                    const CHAR* const szTagT = szTag;                                                                           \
                                                    AssertFail( OSFormat( "FirewallTag%s%s", szTagT ? ":" : "", szTagT ? szTagT : "" ), szFilename, lLine );    \
                                                    OSTraceResumeGC_();                                                                                         \
                                                }

#define FireWall( szTag )                       FireWallAt( szTag, __FILE__, __LINE__ )



#define AssertTrackAt( exp, szTag, szFilename, lLine )  if ( !( exp ) && !FNegTest( fInvalidUsage ) )                                                                   \
                                                        {                                                                                                               \
                                                            OSTraceSuspendGC_();                                                                                        \
                                                            const CHAR* const szTagT = szTag;                                                                           \
                                                            AssertFail( OSFormat( "AssertTrackTag%s%s", szTagT ? ":" : "", szTagT ? szTagT : "" ), szFilename, lLine ); \
                                                            OSTraceResumeGC_();                                                                                         \
                                                        }

#define AssertTrack( exp, szTag )                       AssertTrackAt( exp, szTag, __FILE__, __LINE__ )


#define AssertTrackSz( exp, szTag, ... )                if ( !( exp ) )                                                                \
                                                        {                                                                              \
                                                            AssertFail( "AssertTrackTag:" szTag, __FILE__, __LINE__, __VA_ARGS__ );    \
                                                        }


#ifdef DEBUG
#define ASSERT_VALID( pobject )     ( (pobject)->AssertValid() )
#define ASSERT_VALID_OBJ( object ) ( (object).AssertValid() )
#else
#define ASSERT_VALID( pobject )
#define ASSERT_VALID_OBJ( object )
#endif

#ifndef RTM
#define ASSERT_VALID_RTL( pobject ) ( (pobject)->AssertValid() )
#define ASSERT_VALID_RTL_OBJ( object ) ( (object).AssertValid() )
#else
#define ASSERT_VALID_RTL( object )
#define ASSERT_VALID_RTL_OBJ( object )
#endif



void __stdcall EnforceFail( const CHAR* szMessage, const CHAR* szFilename, LONG lLine );
void __stdcall EnforceContextFail( const WCHAR * wszContext, const CHAR* szMessage, const CHAR* szFilename, LONG lLine );

VOID DefaultReportEnforceFailure( const WCHAR* wszContext, const CHAR* szMessage, const WCHAR* wszIssueSource );
extern void (__stdcall *g_pfnEnforceContextFail)( const WCHAR* wszContext, const CHAR* szMessage, const CHAR* szFilename, LONG lLine );



#if ( defined _PREFIX_ || defined _PREFAST_ )
#define EnforceAtSz( exp, szTag, szFilename, lLine )    ( ( exp ) ? (void)0 : exit(-1) )

#define Enforce( exp )                                  ( ( exp ) ? (void)0 : exit(-1) )
#else
#define EnforceAtSz( exp, szTag, szFilename, lLine )    if ( !( exp ) )                                                                                                 \
                                                        {                                                                                                               \
                                                            OSTraceSuspendGC_();                                                                                        \
                                                            const CHAR* const szTagT = szTag;                                                                           \
                                                            EnforceFail( OSFormat( "EnforceTag%s%s", szTagT ? ":" : "", szTagT ? szTagT : "" ), szFilename, lLine );    \
                                                            OSTraceResumeGC_();                                                                                         \
                                                        }

#define Enforce( exp )                                  if ( !( exp ) )                                 \
                                                        {                                               \
                                                            EnforceFail( #exp, __FILE__, __LINE__ );    \
                                                        }
#endif


#define EnforceSz( exp, szTag )     EnforceAtSz( exp, szTag, __FILE__, __LINE__ )



#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]

#define S_ASSERT(e)                     \
    __pragma(warning(push))             \
    __pragma(warning(disable:4296))     \
    typedef char __C_ASSERT__[(e)?1:-1] \
    __pragma(warning(pop))




#include <excpt.h>


typedef DWORD_PTR EXCEPTION;

#define ExceptionInfo() ( EXCEPTION( GetExceptionInformation() ) )


const DWORD ExceptionId( EXCEPTION exception );


enum EExceptionFilterAction
{
    efaContinueExecution = -1,
    efaContinueSearch,
    efaExecuteHandler,
};


EExceptionFilterAction _ExceptionFail( const CHAR* szMessage, EXCEPTION exception );
#define ExceptionFail( szMessage ) ( _ExceptionFail( ( szMessage ), ExceptionInfo() ) )


#ifdef ENABLE_EXCEPTIONS
#define TRY __try
#else
#define TRY if ( 1 )
#endif


#ifdef ENABLE_EXCEPTIONS
#define EXCEPT( filter )    __except( filter )
#else
#define EXCEPT( filter )    if ( 0 )
#endif


#ifdef ENABLE_EXCEPTIONS
#define ENDEXCEPT
#else
#define ENDEXCEPT
#endif




#ifdef _PREFAST_
#define DISABLE_ERR_CHECK 1
#endif

class CErrFrameSimple
{
private:
    const CHAR *                m_szFile;
    ULONG               m_ulLine;
    INT                         m_err;

public:
    INLINE void Set( __in const CHAR* szFile, __in const LONG lLine, __in const ERR err )
    {
        m_szFile = szFile;
        m_ulLine = lLine;
        m_err = err;
    }

    const CHAR * SzFile() const
    {
        if ( NULL == m_szFile )
        {
            return "";
        }
        return m_szFile;
    }
    const ULONG UlLine() const
    {
        return m_ulLine;
    }
    const ERR Err() const
    {
        return m_err;
    }

};

__forceinline CErrFrameSimple * PefLastThrow();

__forceinline ERR ErrERRSetLastThrow( __in const CHAR* szFile, __in const LONG lLine, __in const ERR err )
{
    PefLastThrow()->Set( szFile, lLine, err );
    AssertRTL( err > -65536 && err < 65536 );
    return err;
}


ULONG UlLineLastCall();



ERR ErrERRSetErrTrap( const ERR errSet );


#define ErrERRCheckDefault( err )       err


#ifdef DEBUG

ERR ErrERRCheck_( const ERR err, const CHAR* szFile, const LONG lLine );
#define ErrERRCheck( err )      ErrERRCheck_( err, __FILE__, __LINE__ )

#else


#ifdef DISABLE_ERR_CHECK

#define ErrERRCheck( err )      (err)
#define ErrERRCheck_( err )     (err)

#else

#define ErrERRCheck( err )  ErrERRCheck_( err, __FILE__, __LINE__ )

__forceinline ERR ErrERRCheck_( __in const ERR err, __in const CHAR* szFile, __in const LONG lLine )
{
    extern ERR g_errTrap;
    PefLastThrow()->Set( szFile, lLine, err );
    if ( g_errTrap == err )
    {
        KernelDebugBreakPoint();
    }
    AssertRTL( err > -65536 && err < 65536 );
    return err;
}

#endif

#endif


#ifdef DEBUG
void ERRSetLastCall( __in const CHAR * szFile, __in const LONG lLine, __in const ERR err );
#else
#define ERRSetLastCall( szFile, lLine, err )
#endif



#define CallR( func )                                   \
{                                                       \
    LogJETCall( err = (func) );                         \
    AssertRTL( err > -65536 && err < 65536 );           \
    if ( err < 0 )                                      \
    {                                                   \
        ERRSetLastCall( __FILE__, __LINE__, err );      \
        return err;                                     \
    }                                                   \
}


#define CallJ( func, label )                            \
{                                                       \
    LogJETCall( err = (func) );                         \
    AssertRTL( err > -65536 && err < 65536 );           \
    if ( err < 0 )                                      \
    {                                                   \
        ERRSetLastCall( __FILE__, __LINE__, err );      \
        goto label;                                     \
    }                                                   \
}


#define Call( func )            CallJ( func, HandleError )


#define CallS( func )                                   \
{                                                       \
    ERR errCallSOnlyT;                                  \
    LogJETCall( errCallSOnlyT = (func) );               \
    if ( JET_errSuccess != errCallSOnlyT )              \
    {                                                   \
        AssertSz( errCallSOnlyT == JET_errSuccess, "errCallSOnlyT(%d) == JET_errSuccess", errCallSOnlyT );      \
    }                                                   \
}


#define CallSx( func, errCallSxOnlyX )                  \
{                                                       \
    ERR errCallSxOnlyT;                                 \
    LogJETCall( errCallSxOnlyT = (func) );              \
    if ( ( JET_errSuccess != errCallSxOnlyT ) && ( (errCallSxOnlyX) != errCallSxOnlyT ) )   \
    {                                                   \
        Assert( JET_errSuccess == errCallSxOnlyT );     \
    }                                                   \
}


#define CallA( func )                                   \
{                                                       \
    LogJETCall( err = (func) );                         \
    AssertRTL( err > -65536 && err < 65536 );           \
    Assert( err >= 0 );                                 \
    if ( err < 0 )                                      \
    {                                                   \
        ERRSetLastCall( __FILE__, __LINE__, err );      \
        goto HandleError;                               \
    }                                                   \
}


#ifdef RFS2
class RFSError
{
    ERR m_err;
    RFSError();
    RFSError( const RFSError& );
public:
    RFSError( ERR err ) { Assert( err != 0 ); m_err = err; }
    BOOL Check( ERR err, ... ) const;
};

#define CallSRFS( func, rfsErrList )                    \
{                                                       \
    ERR errCallSRFSOnlyT;                               \
    LogJETCall( errCallSRFSOnlyT = (func) )             \
    if ( JET_errSuccess != errCallSRFSOnlyT )           \
    {                                                   \
        extern BOOL g_fLogJETCall;                      \
        if ( !g_fLogJETCall )                           \
        {                                               \
            AssertErr( errCallSRFSOnlyT, __FILE__, __LINE__ );  \
        }                                               \
        else                                            \
        {                                               \
            RFSError rfserrorT( errCallSRFSOnlyT );     \
            if ( !(rfserrorT.Check rfsErrList) )        \
            {                                           \
                AssertErr( errCallSRFSOnlyT, __FILE__, __LINE__ );  \
            }                                           \
        }                                               \
    }                                                   \
}
#else
#define CallSRFS( func, rfsErrList )                    \
    CallS( func )
#endif


#define Error( func )                                   \
{                                                       \
    const ERR   errFuncT    = (func);                   \
    LogJETErr( errFuncT, label );                       \
    err = errFuncT;                                     \
    goto HandleError;                                   \
}



#define AllocR( func )                                  \
{                                                       \
    if ( NULL != static_cast< void* >( func ) )         \
    {                                                   \
        err = JET_errSuccess;                           \
    }                                                   \
    else                                                \
    {                                                   \
        return ErrERRCheck( JET_errOutOfMemory );       \
    }                                                   \
}


#define Alloc( func )                       \
{                                                       \
    if ( NULL != static_cast< void* >( func ) )         \
    {                                                   \
        err = JET_errSuccess;                           \
    }                                                   \
    else                                                \
    {                                                   \
        Error( ErrERRCheck( JET_errOutOfMemory ) );     \
    }                                                   \
}



#if defined( FAULT_INJECTION ) && !defined( TEST_INJECTION )
#error "TEST_INJECTION must be enabled in order for FAULT_INJECTION to work"
#endif

#if defined( CONFIGOVERRIDE_INJECTION ) && !defined( TEST_INJECTION )
#error "TEST_INJECTION must be enabled in order for CONFIGOVERRIDE_INJECTION to work"
#endif

#if defined( HANG_INJECTION ) && !defined( TEST_INJECTION )
#error "TEST_INJECTION must be enabled in order for HANG_INJECTION to work"
#endif

ERR ErrEnableTestInjection( const ULONG ulID, const ULONG_PTR pv, const INT type, const ULONG ulProbability, const DWORD grbit );

#ifdef FAULT_INJECTION

ERR ErrFaultInjection_( const ULONG ulID, const CHAR * const szFile, const LONG lLine );
#define ErrFaultInjection( ulID )   ErrFaultInjection_( ulID, __FILE__, __LINE__ )
#define FFaultInjection( ulID )     ( ErrFaultInjection_( ulID, __FILE__, __LINE__ ) != 0 )

template< class T >
inline T* PvFaultInj_( T* pvAlloced )
{
    return pvAlloced;
}
#define PvFaultInj( ulID, pvAllocFunc )     \
    ( ( ErrFaultInjection( ulID ) >= JET_errSuccess ) ?     \
            PvFaultInj_( pvAllocFunc ) :                    \
            NULL )

QWORD ChitsFaultInj( const ULONG ulID );

VOID RFSSuppressFaultInjection( const ULONG ulID );
VOID RFSUnsuppressFaultInjection( const ULONG ulID );

#else

#define ErrFaultInjection( ulID )               JET_errSuccess
#define PvFaultInj( ulID, pv )                  pv
#define FFaultInjection( ulID )                 0

#define RFSSuppressFaultInjection( ulID )       ;
#define RFSUnsuppressFaultInjection( ulID )     ;

#define ChitsFaultInj( ulID )                   ShouldNotUseChitsFaultInjInRetail

#endif


#ifdef CONFIGOVERRIDE_INJECTION
ULONG_PTR UlConfigOverrideInjection_( const ULONG ulID, const ULONG_PTR ulDefault );
#define UlConfigOverrideInjection( ulID, ulDefault )    UlConfigOverrideInjection_( ulID, ulDefault )
#else
#define UlConfigOverrideInjection( ulID, ulDefault )    (ulDefault)
#endif


#ifdef HANG_INJECTION
void HangInjection_( const ULONG ulID );
#define HangInjection( ulID )                   HangInjection_( ulID )
#else
#define HangInjection( ulID )
#endif



#ifndef RTM

extern DWORD    g_grbitNegativeTesting;
#define FNegTest( specificsins )        ( 0 != ( (g_grbitNegativeTesting) & (specificsins) ) )

INLINE bool FNegTestSet( DWORD ibitNegativeTest )
{
    const bool fSet = FNegTest( ibitNegativeTest );
    g_grbitNegativeTesting = ( g_grbitNegativeTesting | ibitNegativeTest );
    return fSet;
}

INLINE void FNegTestUnset( DWORD ibitNegativeTest )
{
    g_grbitNegativeTesting = ( g_grbitNegativeTesting & ( ~ibitNegativeTest ) );
}

#else

#define FNegTest( specificsins )            ( fFalse )
#define FNegTestSet( ibitNegativeTest )     ( fFalse )
#define FNegTestUnset( ibitNegativeTest )   ( (void)fFalse )

#endif



class CCondition
{
#ifdef DEBUG

private:
    BOOL    m_fHit;

#endif

public:
    CCondition()
    {
        OnDebug( m_fHit = fFalse );
    }

    void Reset()
    {
        OnDebug( m_fHit = fFalse );
    }

#ifdef DEBUG

    void Hit()
    {
        m_fHit = fTrue;
    }
    void Hit( BOOL fCondition )
    {
        if ( fCondition )
        {
            m_fHit = fTrue;
        }
    }

#endif

    BOOL FCheckHit() const
    {
        return OnDebugOrRetail( m_fHit, fTrue );
    }

    BOOL FCheckNotHit() const
    {
        return OnDebugOrRetail( !m_fHit, fTrue );
    }

};



BOOL FUtilRepairIntegrityMsgBox( const WCHAR * const wszMsg );

VOID OSErrorRegisterForWer( VOID *pv, DWORD cb );
