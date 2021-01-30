// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_THREAD_HXX_INCLUDED
#define _OS_THREAD_HXX_INCLUDED





const DWORD dwTlsInvalid    = 0xFFFFFFFF;


struct TLS;


TLS* const Ptls();




void UtilSleep( const DWORD cmsec );


enum EThreadPriority
{
    priorityIdle,
    priorityLowest,
    priorityBelowNormal,
    priorityNormal,
    priorityAboveNormal,
    priorityHighest,
    priorityTimeCritical,
};


const ERR ErrUtilThreadICreate( const PUTIL_THREAD_PROC pfnStart, const DWORD cbStack, const EThreadPriority priority, THREAD* const pThread, const DWORD_PTR dwParam, const _TCHAR* const szStart );
#define ErrUtilThreadCreate( pfnStart, cbStack, priority, phThread, dwParam )   \
    ( ErrUtilThreadICreate( pfnStart, cbStack, priority, phThread, dwParam, _T( #pfnStart ) ) )


const DWORD UtilThreadEnd( const THREAD Thread );


const DWORD DwUtilThreadId();


void UtilThreadBeginLowIOPriority(void);
void UtilThreadEndLowIOPriority(void);


#endif

