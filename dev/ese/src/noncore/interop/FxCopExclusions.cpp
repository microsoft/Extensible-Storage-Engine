// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// ***************************************************************
// <summary>
//    It is Exchange policy that all FxCop message suppressions are specified at the module level
//    and that all suppression attributes are contained in a single file called "FxCopExclusions.cs" 
//    or "FxCopExclusions.cpp".
// </summary>
// ***************************************************************

using namespace System::Diagnostics::CodeAnalysis;

// See comment about exceptions we handle and why in MJetInitCallbackOwn()
[module: SuppressMessage("Microsoft.Security", "CA2102:CatchNonClsCompliantExceptionsInGeneralHandlers", Scope="member", Target="Microsoft.Exchange.Isam.Interop.MJetInitCallbackOwn(System.UInt32,System.UInt32,System.UInt32,System.Void*):System.Int32")];
[module: SuppressMessage("Microsoft.Security", "CA2102:CatchNonClsCompliantExceptionsInGeneralHandlers", Scope="member", Target="Microsoft.Exchange.Isam.Interop.MJetInitCallbackOwn(System.UInt64,System.UInt32,System.UInt32,System.Void*):System.Int32")];

// Some of the ESE APIs take opaque DWORD_PTR values. We expose those as IntPtr.
[module: SuppressMessage("Microsoft.Security", "CA2111:PointersShouldNotBeVisible", Scope="member", Target="Microsoft.Exchange.Isam.MJET_SESSIONINFO.TrxContext")];
