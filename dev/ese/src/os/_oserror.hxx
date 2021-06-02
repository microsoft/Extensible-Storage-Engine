// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef __OSERROR_HXX_INCLUDED
#define __OSERROR_HXX_INCLUDED

//  map Win32 error to JET API error
ERR ErrOSErrFromWin32Err(__in DWORD dwWinError, __in ERR errDefault);
ERR ErrOSErrFromWin32Err(__in DWORD dwWinError);

#endif  //  __OSERROR_HXX_INCLUDED


