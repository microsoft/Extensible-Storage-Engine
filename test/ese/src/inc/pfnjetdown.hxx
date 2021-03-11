// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#pragma once

#ifdef ESENT
const wchar_t * const g_mwszzEse = L"esent.dll\0";
#else
const wchar_t * const g_mwszzEse = L"ese.dll\0";
#endif


#include "_pfnjet.hxx"
