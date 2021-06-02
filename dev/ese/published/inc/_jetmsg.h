// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  A universal header for ESENT vs. not ESENT that redirects to whichever header
//  should actually be used.

#ifdef ESENT
#include "jetmsg.h"
#else
#include "jetmsgex.h"
#endif

