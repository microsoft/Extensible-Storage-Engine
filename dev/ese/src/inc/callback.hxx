// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


const CHAR chCallbackSep                = '!';

ERR ErrCALLBACKInit();
VOID CALLBACKTerm();
ERR ErrCALLBACKResolve( const CHAR * const szCallback, JET_CALLBACK * pcallback );

