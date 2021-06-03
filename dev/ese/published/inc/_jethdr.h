// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  A universal header for both NT and Ex that redirects to whereever the 
//  actual full / internal jethdr.w (or jet.h or esent_x.h) is.

#ifdef ESENT
#include "esent_x.h"
#else
#include "jet.h"
#endif
