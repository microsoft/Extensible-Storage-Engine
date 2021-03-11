// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


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
#if _MSC_VER >= 1100
using namespace std;
#endif

#pragma prefast(push)
#pragma prefast(disable:28196, "Do not bother us with strsafe, someone else owns that.")
#pragma prefast(disable:28205, "Do not bother us with strsafe, someone else owns that.")
#include <strsafe.h>
#pragma prefast(pop)


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



#pragma warning ( disable : 4100 )
#pragma warning ( disable : 4200 )
#pragma warning ( disable : 4201 )
#pragma warning ( 3 : 4244 )
#pragma warning ( disable : 4238 )
#pragma warning ( disable : 4239 )
#pragma warning ( disable : 4315 )
#pragma warning ( disable : 4355 )
#pragma warning ( disable : 4512 )
#pragma warning ( disable : 4706 )
#pragma warning ( disable : 4815 )
#if defined( DEBUG ) && !defined( DBG )
#pragma inline_depth( 0 )
#else
#pragma inline_depth( 255 )
#pragma inline_recursion( on )
#endif


#include "osu.hxx"

#include "collection.hxx"


#include "jet.h"
#include "_jet.hxx"
#include "isamapi.hxx"



#define FDTC_0_REORG_DEHYDRATE_PLAN_B 1



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


#include "jettest.hxx"

