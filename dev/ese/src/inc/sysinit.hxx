// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//------------------------ system init/terminate functions------------------------

//  function prototypes
//
ERR ErrITSetConstants( INST * pinst = NULL );


// These options are used to turn expensive debug code on and off
enum ExpensiveDebugCode
{
    Debug_Invalid = 0,
    Debug_Min = 1,
    Debug_Page_Shake = Debug_Min,
    Debug_Node_Seek,
    Debug_Node_Insert,
    Debug_BT_Split,
    Debug_VER_AssertValid,
    Debug_Max,
};

bool FExpensiveDebugCodeEnabled( __in_range( Debug_Min, Debug_Max - 1 ) const ExpensiveDebugCode code );

#if defined( DEBUG ) || defined( PERFDUMP ) || !defined( RTM )
//  these are actually also used in non-RTM for the Error Trap
#define DEBUG_ENV_VALUE_LEN         256
const WCHAR wszDEBUGRoot[]          = L"DEBUG";
#endif

#ifdef DEBUG
WCHAR* GetDebugEnvValue( __in PCWSTR wszEnvVar );

#endif

