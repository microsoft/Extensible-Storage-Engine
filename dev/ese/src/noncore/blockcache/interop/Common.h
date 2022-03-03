// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once


#include <msclr\appdomain.h>
#include <msclr\auto_gcroot.h>
#include <vcclr.h>

#define _CRT_RAND_S

// needed for JET errors
#if defined(BUILD_ENV_IS_NT) || defined(BUILD_ENV_IS_WPHONE)
#include <esent_x.h>
#endif
#ifdef BUILD_ENV_IS_EX
#include <jet.h>
#endif

#include <windows.h>
#include <cstdio>
#include <stdlib.h>

#include <functional>


#include <tchar.h>
#include "os.hxx"

#include "tcconst.hxx"


using namespace System;
using namespace System::IO;
using namespace System::Runtime::InteropServices;

#using <Microsoft.Isam.Esent.Interop.dll> as_friend
using namespace Microsoft::Isam::Esent::Interop;

//  one last macro for old time's sake
#define ExCall( f )                             \
{                                               \
    try                                         \
    {                                           \
        (f);                                    \
        err = JET_errSuccess;                   \
    }                                           \
    catch( EsentErrorException^ ex )            \
    {                                           \
        err = (ERR)(int)ex->Error;              \
    }                                           \
    catch( Exception^ )                         \
    {                                           \
        Enforce( !"Unhandled C# exception" );   \
    }                                           \
    Call( err );                                \
}

INLINE EsentErrorException^ EseException( const JET_ERR err )
{
    return  err >= JET_errSuccess ? nullptr : EsentExceptionHelper::JetErrToException( (JET_err)err );
}
