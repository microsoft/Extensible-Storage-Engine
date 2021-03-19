// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

#pragma prefast(push)
#pragma prefast(disable:28196, "Do not bother us with intsafe, someone else owns that.")
#pragma prefast(disable:28252, "Do not bother us with intsafe, someone else owns that.")
#pragma prefast(disable:28253, "Do not bother us with intsafe, someone else owns that.")
#include <intsafe.h>
#pragma prefast(pop)

REF_TRACE_LOG * g_rgprtlFixed[(LONG_PTR)ostrlMaxFixed];
C_ASSERT( _countof(g_rgprtlFixed) >= (ULONG_PTR)ostrlMaxFixed );

REF_TRACE_LOG * PrtlOSTraceRefLogIGetFixedLog( POSTRACEREFLOG ptracelog )
{
    if ( ptracelog < ostrlMaxFixed )
    {
        return g_rgprtlFixed[(ULONG_PTR)ptracelog];
    }
    return (REF_TRACE_LOG*)ptracelog;
}

ERR ErrOSTraceCreateRefLog( _In_ ULONG cLogSize, _In_ ULONG cbExtraBytes, _Out_ POSTRACEREFLOG *ppTraceLog )
/*++

Routine Description:

    Creates a new (empty) ref count trace log buffer.

Arguments:

    LogSize - The number of entries in the log.
    ExtraBytesInHeader - The number of extra bytes to include in the
        log header. This is useful for adding application-specific
        data to the log.
    ppTraceLog - Pointer to the newly created log if successful, NULL otherwise.

Return Value:

    ERR

--*/
{
    ULONG cbEntrySize = 0;
    ULONG cbTotalSize = 0;
    REF_TRACE_LOG *pLog = NULL;
    ERR err = JET_errSuccess;

    *ppTraceLog = NULL;

    // cbEntrySize = sizeof(REF_TRACE_LOG_ENTRY) + cbExtraBytes
    if ( FAILED( ULongAdd( sizeof(REF_TRACE_LOG_ENTRY), cbExtraBytes, &cbEntrySize ) ) )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    // cbTotalSize = cLogSize * cbEntrySize;
    if ( FAILED( ULongMult( cLogSize, cbEntrySize, &cbTotalSize ) ) )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    // cbTotalSize = cbTotalSize + sizeof(REF_TRACE_LOG)
    if ( FAILED( ULongAdd( sizeof( REF_TRACE_LOG ), cbTotalSize, &cbTotalSize ) ) )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    if ( cbTotalSize > (ULONG) 0x7FFFFFFF )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    // Allocate the log structure.
    pLog = (REF_TRACE_LOG *)LocalAlloc( LPTR, cbTotalSize );
    if( pLog == NULL )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    // Initialize it.
    ZeroMemory( pLog, cbTotalSize );

    pLog->Signature = REF_TRACE_LOG_SIGNATURE;
    pLog->cLogSize = cLogSize;
    pLog->NextEntry = REF_TRACE_LOG_NO_ENTRY;
    pLog->cbExtraInfo = cbExtraBytes;
    pLog->cbEntrySize = cbEntrySize;
    pLog->pLogBuffer = (PBYTE)( pLog + 1 );

    *ppTraceLog = pLog;

HandleError:
    return err;
}   // ErrOSTraceCreateRefTraceLog

VOID OSTraceDestroyRefLog( _In_ POSTRACEREFLOG pLog )
/*++

Routine Description:

    Destroys a ref count trace log buffer created with CreateRefTraceLog().

Arguments:

    pLog - The ref count trace log buffer to destroy.

--*/
{
    REF_TRACE_LOG * Log = PrtlOSTraceRefLogIGetFixedLog( pLog );
    if ( Log != NULL )
    {
        Log->Signature = REF_TRACE_LOG_SIGNATURE_X;
        LocalFree( Log );
        if ( pLog < ostrlMaxFixed )
        {
            Expected( pLog == ostrlSystemFixed );   //  didn't test 2
            g_rgprtlFixed[(ULONG_PTR)pLog] = NULL;
        }
    }
}   // OSTraceDestroyRefLog

//
// N.B. For RtlCaptureBacktrace() to work properly, the calling function
// *must* be __cdecl, and must have a "normal" stack frame. So, we decorate
// WriteRefTraceLog[Ex]() with the __cdecl modifier and disable the frame
// pointer omission (FPO) optimization.
//

#pragma warning( disable : 4918 )   // invalid character in pragma optimization list
BEGIN_PRAGMA_OPTIMIZE_DISABLE( "y", 6520702, "FPO Must be disabled for stack-traces to work." );

VOID __cdecl OSTraceWriteRefLog(
    _In_ POSTRACEREFLOG pLog,
    _In_ LONG NewRefCount,
    _In_ void * pContext,
    __in_bcount(cbExtraInformation) void * pExtraInformation,
    _In_ LONG cbExtraInformation
    )
/*++

Routine Description:

    Writes a new entry to the specified ref count trace log. The entry
    written contains the updated reference count and a stack backtrace
    leading up to the current caller.

Arguments:

    pLog - The log to write to.
    NewRefCount - The updated reference count.
    pContext - An uninterpreted context to associate with the log entry.
    pExtraInformation - extra information to put in the log entry
    cbExtraInformation - size of the extra information

--*/
{
    REF_TRACE_LOG *Log = PrtlOSTraceRefLogIGetFixedLog( pLog );

    if ( Log == NULL && pLog < ostrlMaxFixed )
    {
        Expected( pLog == ostrlSystemFixed );   //  didn't test 2

#ifdef DEBUG
        const ULONG centries = 8 * 73;  //  ~64 KB
#else
        const ULONG centries = 72;  //  ~8 KB
#endif
        POSTRACEREFLOG posrtlAllocated = NULL;
        OnDebug( ERR errT = )ErrOSTraceCreateRefLog( centries, cbSystemRefLogExtra, &posrtlAllocated );
        //  error / failure to allocate is handled below

        if ( NULL == AtomicCompareExchangePointer( (void**)&(g_rgprtlFixed[(ULONG_PTR)pLog]), NULL, posrtlAllocated ) )
        {
            //  we won!  Yeah.
        }
        else
        {
            //  we lost!  Dump our useless posrtlAllocated now.
            OSTraceDestroyRefLog( posrtlAllocated );
        }

        Log = PrtlOSTraceRefLogIGetFixedLog( pLog );
        Assert( errT < JET_errSuccess || Log != NULL );
    }
    if ( Log == NULL )
    {
        return;
    }

    // Find the next slot, copy the info to the slot.

    const ULONG iEntry = ( (ULONG) AtomicIncrement( &Log->NextEntry ) ) % (ULONG) Log->cLogSize;
    REF_TRACE_LOG_ENTRY *pEntry = (REF_TRACE_LOG_ENTRY *)( Log->pLogBuffer + ( iEntry * Log->cbEntrySize ) );

    //  Set log entry members.

    memset( pEntry, 0, Log->cbEntrySize );
    pEntry->NewRefCount = NewRefCount;
    pEntry->pContext = pContext;
    pEntry->ThreadId = GetCurrentThreadId();
    Assert( cbExtraInformation <= Log->cbExtraInfo );
    memcpy( pEntry->pExtraInformation,
            pExtraInformation,
            min(cbExtraInformation, Log->cbExtraInfo) );

    RtlCaptureStackBackTrace( 1, REF_TRACE_LOG_STACK_DEPTH, pEntry->Stack, NULL );

    pEntry->hrt = HrtHRTCount();
}   // OSTraceWriteRefLog

// restore frame pointer omission (FPO)
END_PRAGMA_OPTIMIZE();
#pragma warning( default : 4918 )   // invalid character in pragma optimization list

BOOL FOSRefTraceErrors()
{
#ifdef DEBUG
    return Postls()->fTrackErrors;
#else
    return fFalse;
#endif
}

COSTraceTrackErrors::COSTraceTrackErrors( const CHAR * const szFunc )
{
    if ( szFunc )
    {
        OSTraceWriteRefLog( ostrlSystemFixed, 6 /*sysosrtlErrorTrackStart*/, (void*)szFunc );
    }
    OnDebug( Postls()->fTrackErrors = fFalse );
}

COSTraceTrackErrors::~COSTraceTrackErrors()
{
    OnDebug( Postls()->fTrackErrors = fFalse );
}

