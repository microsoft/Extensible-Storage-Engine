// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


using namespace System::Diagnostics::CodeAnalysis;

[module: SuppressMessage("Microsoft.Security", "CA2102:CatchNonClsCompliantExceptionsInGeneralHandlers", Scope="member", Target="Microsoft.Exchange.Isam.Interop.MJetInitCallbackOwn(System.UInt32,System.UInt32,System.UInt32,System.Void*):System.Int32")];
[module: SuppressMessage("Microsoft.Security", "CA2102:CatchNonClsCompliantExceptionsInGeneralHandlers", Scope="member", Target="Microsoft.Exchange.Isam.Interop.MJetInitCallbackOwn(System.UInt64,System.UInt32,System.UInt32,System.Void*):System.Int32")];

[module: SuppressMessage("Microsoft.Security", "CA2111:PointersShouldNotBeVisible", Scope="member", Target="Microsoft.Exchange.Isam.MJET_SESSIONINFO.TrxContext")];
