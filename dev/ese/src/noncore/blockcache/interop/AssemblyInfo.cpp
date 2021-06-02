// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "stdafx.h"

using namespace System;
using namespace System::Reflection;
using namespace System::Runtime::CompilerServices;
using namespace System::Runtime::InteropServices;
using namespace System::Security::Permissions;

//
// General Information about an assembly is controlled through the following
// set of attributes. Change these attribute values to modify the information
// associated with an assembly.
//
#ifdef BUILD_ENV_IS_NT
#include <common.ver>
#endif

#ifdef BUILD_ENV_IS_EX
#include <bldver.cpp>
#endif


[assembly:AssemblyTitleAttribute("BlockCache Interop")];
[assembly:AssemblyDescriptionAttribute("")];
[assembly:AssemblyConfigurationAttribute("")];
[assembly:AssemblyCultureAttribute("")];

[assembly:ComVisible(false)];
[assembly:CLSCompliantAttribute(true)];
