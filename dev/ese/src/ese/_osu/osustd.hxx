// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//** SYSTEM **********************************************************

#pragma once

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

#include <algorithm>
#include <functional>
#if _MSC_VER >= 1100
using namespace std;
#endif

//** COMPILER CONTROL ************************************************

#pragma warning ( disable : 4127 )  //  conditional expression is constant
#pragma warning ( disable : 4200 )  //  we allow zero sized arrays
#pragma warning ( disable : 4201 )  //  we allow unnamed structs/unions
#pragma warning ( disable : 4355 )  //  we allow the use of this in ctor-inits
#pragma warning ( 3 : 4244 )        //  do not hide data truncations
#pragma inline_depth( 255 )
#pragma inline_recursion( on )

//** OSAL *************************************************************

#include "osu.hxx"              //  OS Abstraction Layer

//** JET API **********************************************************

#include "jet.h"                    //  Public JET API definitions


