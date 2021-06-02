// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// Used to manage the persistant callbacks. Make sure we only open each module once
//

const CHAR chCallbackSep                = '!';  //  callbacks are of the form DLL!Function

ERR ErrCALLBACKInit();
VOID CALLBACKTerm();
ERR ErrCALLBACKResolve( const CHAR * const szCallback, JET_CALLBACK * pcallback );

