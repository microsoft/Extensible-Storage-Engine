// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#pragma once

#include <stdlib.h>

#pragma push_macro( "Assert" )
#pragma push_macro( "OnDebug" )

#ifndef Assert
#define Assert( exp )               AssertSz( exp, #exp )
#endif
#define Expected    Assert
#define ExpectedSz  AssertSz



#ifdef DEBUG
#define OnDebug( code )             code
#else
#define OnDebug( code )
#endif

#pragma warning( push )
#pragma warning ( disable : 4201 )
#include "cc.hxx"
#include "types.hxx"
#pragma warning( pop )
#include "string.hxx"
#include "library.hxx"

#pragma pop_macro( "Assert" )
#pragma pop_macro( "OnDebug" )


