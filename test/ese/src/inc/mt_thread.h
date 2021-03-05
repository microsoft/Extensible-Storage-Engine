// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#ifndef MT_THREAD_H
#define MT_THREAD_H


#include <mt_object.h>
#include <stdio.h>




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

