// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//** SYSTEM **********************************************************

#pragma once

#define _CRT_RAND_S
#include <stdlib.h>
#include <string.h>
#pragma prefast(push)
#pragma prefast(disable:26006, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:26007, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:28718, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:28726, "Dont bother us with tchar, someone else owns that.")
#include <tchar.h>
#pragma prefast(pop)
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <minmax.h>
#include <ctype.h>

#include <specstrings.h>

typedef __nullterminated char* PSTR;
typedef __nullterminated const char* PCSTR;
typedef __nullterminated wchar_t* PWSTR;
typedef __nullterminated const wchar_t* PCWSTR;

#include <algorithm>
#include <functional>
#include <memory>
using namespace std;

#pragma prefast(push)
#pragma prefast(disable:28196, "Do not bother us with strsafe, someone else owns that.")
#pragma prefast(disable:28205, "Do not bother us with strsafe, someone else owns that.")
#include <strsafe.h>
#pragma prefast(pop)

// some security macros taken from exwarning.h

// Here are three macros which correctly answer the question:  "How many BYTEs (TCHARs, WCHARs)
// are left in a variable sized object 'pObject', of total size 'cbTotal', starting at pointer 'pCurrent'?"  
// These macros are written to return 0 if the values are out of bounds, and they are not vulnerable to
// any integer overflow.
#define CbRemainingInObject(pObject,pCurrent,cbTotal)  \
    ( ( (((BYTE*)(pCurrent) < ((BYTE*)(pObject))+(cbTotal))) && \
        (((BYTE*)(pObject) <= ((BYTE*)(pObject))+(cbTotal))) ) ? \
            ((BYTE*)(pObject))+(cbTotal) - (BYTE*)(pCurrent) : 0  )

#define TcRemainingInObject(pObject,pCurrent,cbTotal)  \
    ( ( (((BYTE*)(pCurrent) < ((BYTE*)(pObject))+(cbTotal))) && \
        (((BYTE*)(pObject) <= ((BYTE*)(pObject))+(cbTotal))) ) ? \
            (((BYTE*)(pObject))+(cbTotal) - (BYTE*)(pCurrent))/sizeof(TCHAR) : 0  )

#define WcRemainingInObject(pObject,pCurrent,cbTotal)  \
    ( ( (((BYTE*)(pCurrent) < ((BYTE*)(pObject))+(cbTotal))) && \
        (((BYTE*)(pObject) <= ((BYTE*)(pObject))+(cbTotal))) ) ? \
            (((BYTE*)(pObject))+(cbTotal) - (BYTE*)(pCurrent))/sizeof(WCHAR) : 0  )


//** COMPILER CONTROL *************************************************

#pragma warning ( disable : 4100 )  //  unreferenced formal parameter
#pragma warning ( disable : 4200 )  //  we allow zero sized arrays
#pragma warning ( disable : 4201 )  //  we allow unnamed structs/unions
#pragma warning ( 3 : 4244 )        //  do not hide data truncations
#pragma warning ( disable : 4238 )  //  nonstandard extension used : class rvalue used as lvalue
#pragma warning ( disable : 4239 )  //  nonstandard extension used : 'token' : conversion from 'type' to 'type'
#pragma warning ( disable : 4315 )  // 'struct' : 'this' pointer for member 'struct::member' may not be aligned X as expected by the constructor
#pragma warning ( disable : 4355 )  //  we allow the use of this in ctor-inits
#pragma warning ( disable : 4512 )  //  assignment operator could not be generated
#pragma warning ( disable : 4706 )  //  assignment within conditional expression
#pragma warning ( disable : 4815 )  //  zero-sized array in stack object will have no elements (unless the object is an aggregate that has been aggregate initialized)
#if defined( DEBUG ) && !defined( DBG )
#pragma inline_depth( 0 )
#else  //  !DEBUG || DBG
#pragma inline_depth( 255 )
#pragma inline_recursion( on )
#endif  //  DEBUG && !DBG

//** OSAL *************************************************************

#include "osu.hxx"              //  OS Abstraction Layer

#include "collection.hxx"       //  Collection Classes

//** JET API **********************************************************

#include "jet.h"                //  Public JET API definitions
#include "_jet.hxx"             //  Private JET definitions
#include "isamapi.hxx"          //  Direct ISAM APIs needed to support Jet API

//** RISKY LATE HYPER-* STUFF *****************************************

//  A set of late breaking changes to Fix DaTaCenter issues ...

//  Hyper-Reorg - This allows ESE to re-organize a page during _some_ / most 
//  dehydration operations.
//#define FDTC_0_REORG_DEHYDRATE 1
#define FDTC_0_REORG_DEHYDRATE_PLAN_B 1


//** DAE ISAM *********************************************************

#include "tcconst.hxx"
#include "tls.hxx"
#include "taskmgr.hxx"
#include "cresmgr.hxx"
#include "daedef.hxx"
#include "dbutil.hxx"
#include "sysinit.hxx"
#include "bf.hxx"
#include "log.hxx"
#include "pib.hxx"
#include "kvpstore.hxx"
#include "cpage.hxx"
#include "dbscan.hxx"
#include "revertsnapshot.h"
#include "fmp.hxx"
#include "flushmap.hxx"
#include "io.hxx"
#include "dbapi.hxx"
#include "ccsr.hxx"
#include "idb.hxx"
#include "callback.hxx"
#include "fcb.hxx"
#include "fucb.hxx"
#include "scb.hxx"
#include "tdb.hxx"
#include "dbtask.hxx"
#include "ver.hxx"
#include "node.hxx"
#include "space.hxx"
#include "btsplit.hxx"
#include "bt.hxx"
#include "dir.hxx"
#include "logapi.hxx"
#include "logstream.hxx"
#include "compression.hxx"
#include "rec.hxx"
#include "tagfld.hxx"
#include "fldenum.hxx"
#include "file.hxx"
#include "cat.hxx"
#include "stats.hxx"
#include "sortapi.hxx"
#include "ttmap.hxx"
#include "dbshrink.hxx"
#include "repair.hxx"
#include "old.hxx"
#include "scrub.hxx"
#include "upgrade.hxx"
#include "lv.hxx"
#include "dataserializer.hxx"
#include "prl.hxx"

//** UNIT TESTS *************************************************************

#include "jettest.hxx"

