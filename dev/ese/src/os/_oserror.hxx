// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef __OSERROR_HXX_INCLUDED
#define __OSERROR_HXX_INCLUDED

ERR ErrOSErrFromWin32Err(__in DWORD dwWinError, __in ERR errDefault);
ERR ErrOSErrFromWin32Err(__in DWORD dwWinError);

#endif


