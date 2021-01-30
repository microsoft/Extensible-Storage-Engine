// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


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


#pragma warning ( disable : 4127 )
#pragma warning ( disable : 4200 )
#pragma warning ( disable : 4201 )
#pragma warning ( disable : 4355 )
#pragma warning ( 3 : 4244 )
#pragma inline_depth( 255 )
#pragma inline_recursion( on )


#include "osu.hxx"


#include "jet.h"


