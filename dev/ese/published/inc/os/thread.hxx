// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_THREAD_HXX_INCLUDED
#define _OS_THREAD_HXX_INCLUDED


//  Thread Local Storage


//  this is the value returned by TlsAlloc() on error

const DWORD dwTlsInvalid    = 0xFFFFFFFF;

//  if the user wants to define a custom TLS, it must be named "TLS"

struct TLS;

//  returns the pointer to the current context's local storage

TLS* const Ptls();


//  Thread Management

//  suspends execution of the current context for the specified interval

void UtilSleep( const DWORD cmsec );

//  thread priorities

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

//  creates a thread with the specified attributes

const ERR ErrUtilThreadICreate( const PUTIL_THREAD_PROC pfnStart, const DWORD cbStack, const EThreadPriority priority, THREAD* const pThread, const DWORD_PTR dwParam, const _TCHAR* const szStart );
#define ErrUtilThreadCreate( pfnStart, cbStack, priority, phThread, dwParam )   \
    ( ErrUtilThreadICreate( pfnStart, cbStack, priority, phThread, dwParam, _T( #pfnStart ) ) )

//  waits for the specified thread to exit and returns its return value

const DWORD UtilThreadEnd( const THREAD Thread );

//  returns the current thread's ID

const DWORD DwUtilThreadId();

//  Adjusts the thread's io priority

// Can't be nested 
// Each call to begin must be matched with one call to end
void UtilThreadBeginLowIOPriority(void);
void UtilThreadEndLowIOPriority(void);


#endif  //  _OS_THREAD_HXX_INCLUDED

