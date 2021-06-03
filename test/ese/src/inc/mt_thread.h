// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#ifndef MT_THREAD_H
#define MT_THREAD_H

//#define MULTITEST_DEBUG 1

#include <mt_object.h>
#include <stdio.h>

/*******************************************************************
  JET calls made in the __finally section may also have errors.
  Use this call to report these (as opposed to CALLJET_SEH
*******************************************************************/
/*
#define CALLJET_TERMINATE(func)                                           \
  jErr = (func);                                                          \
  if ( jErr < JET_errSuccess )                                            \
  {                                                                       \
    tprintf("THREAD TERM: Failed with %ld in function %s [%s:%ld]\r\n",   \
             jErr, #func, __FILE__, __LINE__ );                           \
  }
*/
/*******************************************************************
 Thread routines that use with MT_JET objects
  - Usually passed as the worked function for an MT_JET_THREAD 
    object
*******************************************************************/
DWORD WINAPI RandInsert( LPVOID sCurrentThreadInfo );
DWORD WINAPI SeqInsert( LPVOID sCurrentThreadInfo );
DWORD WINAPI RandDelete( LPVOID sCurrentThreadInfo );
DWORD WINAPI SeqDelete( LPVOID sCurrentThreadInfo );
DWORD WINAPI RandReplace( LPVOID sCurrentThreadInfo );
DWORD WINAPI SeqReplace( LPVOID sCurrentThreadInfo );

DWORD WINAPI NewRandReplace ( LPVOID pv );

JET_ERR JTRandReplace(
    JET_SESID jSesId,
    JET_TABLEID jTabId,
    JET_SETCOLUMN* pJSetColumn,
    ULONG ulSetColumn,
    long lRecord );

JET_ERR JTPeriodicCommit(
    JET_SESID jSesId,
    BOOL fLazy,
    long lCount,
    long lPeriod );

JET_ERR JTCommitTransactionWhileFrozen(  JET_SESID jSesId, JET_GRBIT jGrBit );

#endif

