// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef _OS_ERROR_HXX_INCLUDED
#error ERROR.HXX already included
#endif
#define _OS_ERROR_HXX_INCLUDED


// ------------------------------------------------------------------------------------------------
//
//  Forward Declarations
//

INT UtilMessageBoxW( _In_ const WCHAR * const wszText, _In_ const WCHAR * const wszCaption, _In_ const UINT uType );
void KernelDebugBreakPoint();

const CHAR * SzSourceFileName( const CHAR * szFilePath );

// ------------------------------------------------------------------------------------------------
//
//  Prototypes
//

#ifdef DEBUG

// IsDebuggerAttached() is useful to silence Asserts.  It shouldn't be used in 
// retail code.  

BOOL IsDebuggerAttached();
#endif


// ------------------------------------------------------------------------------------------------
//
//  Build Options
//

#define ENABLE_EXCEPTIONS           //  enable exception handling

#ifdef DEBUG

#ifdef RTM
#error: RTM defined in DEBUG build
#endif

#define RFS2

#endif  //  DEBUG

#ifndef RTM
#define TEST_INJECTION
#define FAULT_INJECTION
#define CONFIGOVERRIDE_INJECTION
#define HANG_INJECTION
#endif  //  RTM


// ------------------------------------------------------------------------------------------------
//
//  Cleanup Path Checking
//
//  This facility allows us to Assert() on any allocations during a "cleanup path",
//  where we would not want allocations to make us fail. 
//

#ifdef DEBUG

BOOL FOSSetCleanupState( BOOL fInCleanupState );

#else  //  !DEBUG

#define FOSSetCleanupState( fInCleanupState ) (fInCleanupState)
#define FOSGetCleanupState() (fFalse)

#endif  //  DEBUG


// ------------------------------------------------------------------------------------------------
//
//  Resource Failure Simulation
//

// Note: Fault Injection implemented separately below.

#ifdef RFS2

//  RFS allocator:  returns 0 if allocation is disallowed.  Also handles RFS logging.
//  g_cRFSAlloc is the global allocation counter.  A value of -1 disables RFS in debug mode.

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


//  RFS allocation types
//
//      Type 0:  general allocation
//           1:  IO
//           2:  sentinel

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

#else  // !RFS2

#define RFSAlloc( type )            (fTrue)
#define FRFSFailureDetected( type ) (fFalse)
#define FRFSAnyFailureDetected()    (fFalse)
#define RFSSetKnownResourceLeak()
#define FRFSKnownResourceLeak()     (fFalse)
#define RFSThreadDisable( cRFSCountdown ) (cRFSCountdown)
#define RFSThreadReEnable( cRFSCountdownOld )
#define LogJETCall( err )       { (err); }
#define LogJETErr( err, label )

#endif  //  RFS2


// ------------------------------------------------------------------------------------------------
//
//  Assertions
//

//  Assertion Failure action
//
//  called to indicate to the developer that an assumption is not true

void __stdcall AssertFail( const CHAR* szMessage, const CHAR* szFilename, LONG lLine, ... );

void AssertErr( const ERR err, const CHAR* szFilename, const LONG lLine );


//  Assert Macros

//  asserts that the given expression is true or else fails with the specified message

#ifdef RTM
#define AssertSzRTL( exp, sz, ... )
#else  //  RTM
#define AssertSzRTL( exp, sz, ... )     ( ( exp ) ? (void) 0 : AssertFail( sz, __FILE__, __LINE__, __VA_ARGS__ ) )
#endif  //  !RTM

#ifdef DEBUG
#define AssertSz( exp, sz, ... )            ( ( exp ) ? (void) 0 : AssertFail( sz, __FILE__, __LINE__, __VA_ARGS__ ) )
#else  //  !DEBUG
#define AssertSz( exp, sz, ... )
#endif  //  DEBUG


//  asserts that the given expression is true or else fails with that expression

#define AssertRTL( exp )            AssertSzRTL( exp, "%s", #exp )
#define Assert( exp )               AssertSz( exp, "%s", #exp )


//  use this to protect invalid code paths that PREfix or PREfast complains about

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

// ------------------------------------------------------------------------------------------------
//
//  Sub-Assertions
//

//  The Expected() macro is like the assert macro in that when it goes off it indicates that an
//  unexpected condition has occurred.  But we consider an Assert() to be a hard condition of the
//  code the assert is declared in.  But an Expected() may mean that either the exact situation
//  has never been fully tested in that code, or currently we expect the client code does not 
//  utilize this condition, and that something is fishy.  It is a lesser assert.  Please use both
//  Expected() and Assert() judiciously, in that we should Assert() things that really must be
//  true, and we Expected() things that should have been true when you originally wrote the code,
//  but that may morph over time.

#define Expected    Assert
#define ExpectedSz  AssertSz


// ------------------------------------------------------------------------------------------------
//
//  Debug / RTM facilities
//

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


// ------------------------------------------------------------------------------------------------
//
//  Fire Wall and Enforces ("DB Panic")
//

//  Fire Wall action
//
//  called whenever something suspicious happens - we've added code to handle it, but don't know why it is occurring

#define FireWallAt( szTag, szFilename, lLine )  if ( !FNegTest( fInvalidUsage ) )                                                                               \
                                                {                                                                                                               \
                                                    OSTraceSuspendGC_();                                                                                        \
                                                    const CHAR* const szTagT = szTag;                                                                           \
                                                    AssertFail( OSFormat( "FirewallTag%s%s", szTagT ? ":" : "", szTagT ? szTagT : "" ), szFilename, lLine );    \
                                                    OSTraceResumeGC_();                                                                                         \
                                                }

#define FireWall( szTag )                       FireWallAt( szTag, __FILE__, __LINE__ )


//  AssertTrack is a more serious version of FireWall, for cases which are not handled. Ideally, we should move all AssertTrack()s to AssertRTL()s. once the latter
//  ones get cleaned up.

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

//  asserts that the specified object is valid using a member function

#ifdef DEBUG
#define ASSERT_VALID( pobject )     ( (pobject)->AssertValid() )
#define ASSERT_VALID_OBJ( object ) ( (object).AssertValid() )
#else  //  !DEBUG
#define ASSERT_VALID( pobject )
#define ASSERT_VALID_OBJ( object )
#endif  //  DEBUG

#ifndef RTM
#define ASSERT_VALID_RTL( pobject ) ( (pobject)->AssertValid() )
#define ASSERT_VALID_RTL_OBJ( object ) ( (object).AssertValid() )
#else
#define ASSERT_VALID_RTL( object )
#define ASSERT_VALID_RTL_OBJ( object )
#endif

//  Enforces

//  Enforce Failure action
//
//  called when a strictly enforced condition has been violated

void __stdcall EnforceFail( const CHAR* szMessage, const CHAR* szFilename, LONG lLine );
void __stdcall EnforceContextFail( const WCHAR * wszContext, const CHAR* szMessage, const CHAR* szFilename, LONG lLine );

VOID DefaultReportEnforceFailure( const WCHAR* wszContext, const CHAR* szMessage, const WCHAR* wszIssueSource );
extern void (__stdcall *g_pfnEnforceContextFail)( const WCHAR* wszContext, const CHAR* szMessage, const CHAR* szFilename, LONG lLine );

//  Enforce  Macros

//  the given expression MUST be true or else fails with the specified message

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

//  the given expression MUST be true or else fails with that expression

#define EnforceSz( exp, szTag )     EnforceAtSz( exp, szTag, __FILE__, __LINE__ )

// ------------------------------------------------------------------------------------------------
//
//  Compiler Assert
//

//
// C_ASSERT() can be used to perform many compile-time assertions:
//            type sizes, field offsets, etc.
//
// An assertion failure results in
//      error C2118: negative subscript.
// If you have not defined a static / compile-time constraint results in
//      error C3861: 'countof': identifier not found
//      error C2086: 'char __C_ASSERT__[1]' : redefinition
//
//  Copied over from winnt.h

#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]
// This C_ASSERT() is not sufficient for a check like this:
//      C_ASSERT( dtickMaintCacheSizeRequest <= dtickMaintCacheStatsPeriod / 2 );
// b/c (at the time) dtickMaintCacheSizeRequest = 0, and so the error 
//      2>e:\src\win8\esent2\ds\esent\src\ese\bf.cxx(11397) : error C4296: '<=' : expression is always true
// is returned.  It is complaining that "0 <= x" ... no value of x can ever
// make this false ... but since 0 can change (b/c it's a constant that may 
// be altered) this is actually a valid C_ASSERT().

// Doing a better C_ASSERT() ...
#define S_ASSERT(e)                     \
    __pragma(warning(push))             \
    __pragma(warning(disable:4296))     \
    typedef char __C_ASSERT__[(e)?1:-1] \
    __pragma(warning(pop))



// ------------------------------------------------------------------------------------------------
//
//  Exceptions
//

#include <excpt.h>

//  Exception Information function for use by an exception filter
//
//  NOTE:  must be called in the scope of the exception filter expression

typedef DWORD_PTR EXCEPTION;

#define ExceptionInfo() ( EXCEPTION( GetExceptionInformation() ) )

//  returns the exception id of an exception

const DWORD ExceptionId( EXCEPTION exception );

//  Exception Filter Actions
//
//  all exception filters must return one of these values

enum EExceptionFilterAction
{
    efaContinueExecution = -1,
    efaContinueSearch,
    efaExecuteHandler,
};

//  Exception Failure action
//
//  used as the filter whenever any exception that occurs is considered a failure

EExceptionFilterAction _ExceptionFail( const CHAR* szMessage, EXCEPTION exception );
#define ExceptionFail( szMessage ) ( _ExceptionFail( ( szMessage ), ExceptionInfo() ) )

//  Exception Guard Block
//
//  prefix guarded code block with this command

#ifdef ENABLE_EXCEPTIONS
#define TRY __try
#else  //  !ENABLE_EXCEPTIONS
#define TRY if ( 1 )
#endif  //  ENABLE_EXCEPTIONS

//  Exception Action Block
//
//  prefix code block to execute in the event of an exception with this command
//
//  NOTE:  this block must come immediately after the guarded block

#ifdef ENABLE_EXCEPTIONS
#define EXCEPT( filter )    __except( filter )
#else  //  !ENABLE_EXCEPTIONS
#define EXCEPT( filter )    if ( 0 )
#endif  //  ENABLE_EXCEPTIONS

//  Exception Block Close
//
//  NOTE:  this keyword must come immediately after the action block

#ifdef ENABLE_EXCEPTIONS
#define ENDEXCEPT
#else  //  !ENABLE_EXCEPTIONS
#define ENDEXCEPT
#endif  //  ENABLE_EXCEPTIONS


// ------------------------------------------------------------------------------------------------
//
//  Error Handling
//

//  Last Error Tracking
//

#ifdef _PREFAST_
// PREfast does not know that ErrERRCheck() always returns err
#define DISABLE_ERR_CHECK 1
#endif  ///  _PREFAST_

class CErrFrameSimple
{
private:
    const CHAR *                m_szFile;           //  file related to error frame
    ULONG               m_ulLine;           //  line number related to error frame
    INT                         m_err;              //  err for the error frame

public:
    INLINE void Set( _In_ const CHAR* szFile, _In_ const LONG lLine, _In_ const ERR err )
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

__forceinline ERR ErrERRSetLastThrow( _In_ const CHAR* szFile, _In_ const LONG lLine, _In_ const ERR err )
{
    PefLastThrow()->Set( szFile, lLine, err );
    AssertRTL( err > -65536 && err < 65536 );
    return err;
}

//  Returns the line of the last call that failed out w/ an error, presumably within this frame.

ULONG UlLineLastCall();

//  Error Generation Check
//

//  Sets the err trap / generation check test value, and returns the old test value.

ERR ErrERRSetErrTrap( const ERR errSet );

//  When we want to pass an error as a default error, we will use this ... the
//  function / code getting the default error will throw the ErrERRCheck() proper 
//  if the default error is taken.

#define ErrERRCheckDefault( err )       err

//  called whenever an error is newly generated

#ifdef DEBUG

ERR ErrERRCheck_( const ERR err, const CHAR* szFile, const LONG lLine );
#define ErrERRCheck( err )      ErrERRCheck_( err, __FILE__, __LINE__ )

#else  //  !DEBUG

//  This disables the last error TLS tracking and the g_errTrap check.

#ifdef DISABLE_ERR_CHECK

#define ErrERRCheck( err )      (err)
#define ErrERRCheck_( err )     (err)

#else

#define ErrERRCheck( err )  ErrERRCheck_( err, __FILE__, __LINE__ )

__forceinline ERR ErrERRCheck_( _In_ const ERR err, _In_ const CHAR* szFile, _In_ const LONG lLine )
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

#endif  // DISABLE_ERR_CHECK

#endif  // DEBUG


#ifdef DEBUG
void ERRSetLastCall( _In_ const CHAR * szFile, _In_ const LONG lLine, _In_ const ERR err );
#else
#define ERRSetLastCall( szFile, lLine, err )
#endif

//  Error Flow Control Macros

//  Sets err, and returns from the function with the error if the call fails, and continues on warning/success error code.

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

//  Sets err, and jumps to the given label if the call fails, and continues on warning/success error code.

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

//  Sets err, jumps to the HandleError label if the function fails, continues on warning/success error code.

#define Call( func )            CallJ( func, HandleError )

//  Asserts if the function fails OR warns, but will NOT change or set err, nor affect code flow (will NOT jump / return).
//
//  Generally used for cleanup past the HandleError, where you do not want to jump back (endless loop) nor affect the 
//  primary operational error of the function.

#define CallS( func )                                   \
{                                                       \
    ERR errCallSOnlyT;                                  \
    LogJETCall( errCallSOnlyT = (func) );               \
    if ( JET_errSuccess != errCallSOnlyT )              \
    {                                                   \
        AssertSz( errCallSOnlyT == JET_errSuccess, "errCallSOnlyT(%d) == JET_errSuccess", errCallSOnlyT );      \
    }                                                   \
}

//  Asserts if the function fails OR warns with an error different than the given one, but will NOT change or set err, 
//  nor affect code flow (will NOT jump / return).
//
//  Generally used for cleanup past the HandleError, where you do not want to jump back (endless loop) nor affect the 
//  primary operational error of the function ... but have a singular out or resource condition that you want to ignore 
//  without changing code flow.

#define CallSx( func, errCallSxOnlyX )                  \
{                                                       \
    ERR errCallSxOnlyT;                                 \
    LogJETCall( errCallSxOnlyT = (func) );              \
    if ( ( JET_errSuccess != errCallSxOnlyT ) && ( (errCallSxOnlyX) != errCallSxOnlyT ) )   \
    {                                                   \
        Assert( JET_errSuccess == errCallSxOnlyT );     \
    }                                                   \
}

//  Sets err, asserts if the function fails only, and jumps to the HandleError label if the function failed, and 
//  continues on warning/success error code.

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

// more RFS2 stuff

#ifdef RFS2
class RFSError
{
    ERR m_err;
    RFSError(); // forbidden
    RFSError( const RFSError& ); // forbidden
public:
    RFSError( ERR err ) { Assert( err != 0 ); m_err = err; }
    BOOL Check( ERR err, ... ) const;
};

//  CallSRFS:
//  when rfs cound not be enabled it is as CallS
//  when g_fLogJETCall is true rfsErrList contains 0 (zero)
//      terminated list of acceptable errors
//
//  Example:
//      CallS( err, ( JET_errOutOfMemory, JET_errOutOfCursos, JET_errDiskIO, 0 ) );
//
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
#else // RFS2
#define CallSRFS( func, rfsErrList )                    \
    CallS( func )
#endif // RFS2

//  Sets err, and jumps to the HandleError label (regardless of success or failure).
//
//  Generally used with straight errors instead of checking function calls.

#define Error( func )                                   \
{                                                       \
    const ERR   errFuncT    = (func);                   \
    LogJETErr( errFuncT, label );                       \
    err = errFuncT;                                     \
    goto HandleError;                                   \
}


//  Sets err (based upon allocation), and returns from the function with the error if the allocation 
//  fails, and continues on warning/success error code.
//
//  Generally used early on in a function to return if some initial allocation or init routine fails
//  and you would not want to run general cleanup / HandleError code.

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

//  Sets err (based upon allocation), and jumps to the HandleError label if the allocation fails.

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


// ------------------------------------------------------------------------------------------------
//
//  LID-based injection tests
//

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

//  Returns the error set by ErrEnableTestInjection or JET_errSuccess.
ERR ErrFaultInjection_( const ULONG ulID, const CHAR * const szFile, const LONG lLine );
#define ErrFaultInjection( ulID )   ErrFaultInjection_( ulID, __FILE__, __LINE__ )
#define FFaultInjection( ulID )     ( ErrFaultInjection_( ulID, __FILE__, __LINE__ ) != 0 )

template< class T >
inline T* PvFaultInj_( T* pvAlloced )
{
    //  This silly little thing, allows the type to be preserved in the below macro nicely.
    return pvAlloced;
}
#define PvFaultInj( ulID, pvAllocFunc )     \
    ( ( ErrFaultInjection( ulID ) >= JET_errSuccess ) ?     \
            PvFaultInj_( pvAllocFunc ) :                    \
            NULL )

QWORD ChitsFaultInj( const ULONG ulID );

VOID RFSSuppressFaultInjection( const ULONG ulID );
VOID RFSUnsuppressFaultInjection( const ULONG ulID );

#else   //  !FAULT_INJECTION

#define ErrFaultInjection( ulID )               JET_errSuccess
#define PvFaultInj( ulID, pv )                  pv
#define FFaultInjection( ulID )                 0

#define RFSSuppressFaultInjection( ulID )       ;
#define RFSUnsuppressFaultInjection( ulID )     ;

#define ChitsFaultInj( ulID )                   ShouldNotUseChitsFaultInjInRetail

#endif  //  FAULT_INJECTION


#ifdef CONFIGOVERRIDE_INJECTION
//  Returns the value set by ErrEnableTestInjection or the default value that was passed in.
ULONG_PTR UlConfigOverrideInjection_( const ULONG ulID, const ULONG_PTR ulDefault );
#define UlConfigOverrideInjection( ulID, ulDefault )    UlConfigOverrideInjection_( ulID, ulDefault )
#else   //  !CONFIGOVERRIDE_INJECTION
#define UlConfigOverrideInjection( ulID, ulDefault )    (ulDefault)
#endif  //  CONFIGOVERRIDE_INJECTION


#ifdef HANG_INJECTION
void HangInjection_( const ULONG ulID );
#define HangInjection( ulID )                   HangInjection_( ulID )
#else   //  !HANG_INJECTION
#define HangInjection( ulID )
#endif  //  HANG_INJECTION


// ------------------------------------------------------------------------------------------------
//
//  Negative Testing
//

#ifndef RTM
//  The FNegTest() macro allows testing to avoid some assert()s in ESE that
//  should not normally happen unless test/user has committed some sin against
//  ESE (e.g. deleted log files, corrupted log filess, etc).

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

//  We assume this is false, until we enable retail / RTM based tests.
#define FNegTest( specificsins )            ( fFalse )
#define FNegTestSet( ibitNegativeTest )     ( fFalse )
#define FNegTestUnset( ibitNegativeTest )   ( (void)fFalse )

#endif


// ------------------------------------------------------------------------------------------------
//
//  Positive Testing
//

//  This class is a very silly basic way to check that a specific condition in some deep layer of 
//  code is hit by a embedded unit test.
//
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

    //  These Hit() functions purposely NOT defined in retail, as I want the callee code to document
    //  that the condition is hit only via debug, with this pattern:
    //      OnDebug( g_condXxxx.Hit() );
    //
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

#endif // DEBUG

    BOOL FCheckHit() const
    {
        return OnDebugOrRetail( m_fHit, fTrue );
    }

    BOOL FCheckNotHit() const
    {
        return OnDebugOrRetail( !m_fHit, fTrue );
    }

};


// ------------------------------------------------------------------------------------------------
//
//  Repair/Integrity Utility
//

// popup a MessageBox for user instruction
BOOL FUtilRepairIntegrityMsgBox( const WCHAR * const wszMsg );

VOID OSErrorRegisterForWer( VOID *pv, DWORD cb );
