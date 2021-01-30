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

ERR ErrOSTraceCreateRefLog( IN ULONG cLogSize, IN ULONG cbExtraBytes, OUT POSTRACEREFLOG *ppTraceLog )

{
    ULONG cbEntrySize = 0;
    ULONG cbTotalSize = 0;
    REF_TRACE_LOG *pLog = NULL;
    ERR err = JET_errSuccess;

    *ppTraceLog = NULL;

    if ( FAILED( ULongAdd( sizeof(REF_TRACE_LOG_ENTRY), cbExtraBytes, &cbEntrySize ) ) )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    if ( FAILED( ULongMult( cLogSize, cbEntrySize, &cbTotalSize ) ) )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    if ( FAILED( ULongAdd( sizeof( REF_TRACE_LOG ), cbTotalSize, &cbTotalSize ) ) )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    if ( cbTotalSize > (ULONG) 0x7FFFFFFF )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    pLog = (REF_TRACE_LOG *)LocalAlloc( LPTR, cbTotalSize );
    if( pLog == NULL )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

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
}

VOID OSTraceDestroyRefLog( IN POSTRACEREFLOG pLog )

{
    REF_TRACE_LOG * Log = PrtlOSTraceRefLogIGetFixedLog( pLog );
    if ( Log != NULL )
    {
        Log->Signature = REF_TRACE_LOG_SIGNATURE_X;
        LocalFree( Log );
        if ( pLog < ostrlMaxFixed )
        {
            Expected( pLog == ostrlSystemFixed );
            g_rgprtlFixed[(ULONG_PTR)pLog] = NULL;
        }
    }
}


#pragma warning( disable : 4918 )
BEGIN_PRAGMA_OPTIMIZE_DISABLE( "y", 6520702, "FPO Must be disabled for stack-traces to work." );

VOID __cdecl OSTraceWriteRefLog(
    IN POSTRACEREFLOG pLog,
    IN LONG NewRefCount,
    IN void * pContext,
    __in_bcount(cbExtraInformation) IN void * pExtraInformation,
    IN LONG cbExtraInformation
    )

{
    REF_TRACE_LOG *Log = PrtlOSTraceRefLogIGetFixedLog( pLog );

    if ( Log == NULL && pLog < ostrlMaxFixed )
    {
        Expected( pLog == ostrlSystemFixed );

#ifdef DEBUG
        const ULONG centries = 8 * 73;
#else
        const ULONG centries = 72;
#endif
        POSTRACEREFLOG posrtlAllocated = NULL;
        OnDebug( ERR errT = )ErrOSTraceCreateRefLog( centries, cbSystemRefLogExtra, &posrtlAllocated );

        if ( NULL == AtomicCompareExchangePointer( (void**)&(g_rgprtlFixed[(ULONG_PTR)pLog]), NULL, posrtlAllocated ) )
        {
        }
        else
        {
            OSTraceDestroyRefLog( posrtlAllocated );
        }

        Log = PrtlOSTraceRefLogIGetFixedLog( pLog );
        Assert( errT < JET_errSuccess || Log != NULL );
    }
    if ( Log == NULL )
    {
        return;
    }


    const ULONG iEntry = ( (ULONG) AtomicIncrement( &Log->NextEntry ) ) % (ULONG) Log->cLogSize;
    REF_TRACE_LOG_ENTRY *pEntry = (REF_TRACE_LOG_ENTRY *)( Log->pLogBuffer + ( iEntry * Log->cbEntrySize ) );


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
}

END_PRAGMA_OPTIMIZE();
#pragma warning( default : 4918 )

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
        OSTraceWriteRefLog( ostrlSystemFixed, 6 , (void*)szFunc );
    }
    OnDebug( Postls()->fTrackErrors = fFalse );
}

COSTraceTrackErrors::~COSTraceTrackErrors()
{
    OnDebug( Postls()->fTrackErrors = fFalse );
}

