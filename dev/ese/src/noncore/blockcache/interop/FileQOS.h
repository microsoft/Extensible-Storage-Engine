// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "stdafx.h"

#pragma once

#pragma managed

namespace Internal
{
    namespace Ese
    {
        namespace BlockCache
        {
            namespace Interop
            {
                [Flags]
                public enum class FileQOS : Int64
                {
                    IOOSNormalPriority = 0,

                    IOOSLowPriority = 0x2,

                    IOOSMask = IOOSNormalPriority | IOOSLowPriority,

                    IOPoolReserved = 0x4,

                    IOSignalSlowSyncIO = 0x8,

                    IOMiscOptionsMask = IOPoolReserved | IOSignalSlowSyncIO,

                    IODispatchImmediate = 0x10,

                    IONormal = IODispatchImmediate,

                    IODispatchIdle = 0x20,

                    IODispatchBackground = 0x40,

                    IODispatchUrgentBackgroundLevelMin = 0x0001,

                    IODispatchUrgentBackgroundLevelMax = 0x007F,

                    IODispatchUrgentBackgroundShft = 8,

                    IODispatchUrgentBackgroundMask = 0x7F00,

                    IODispatchUrgentBackgroundMin = 0x0100,

                    IODispatchUrgentBackgroundMax = 0x7F00,

                    IODispatchMask = IODispatchImmediate | IODispatchUrgentBackgroundMask | IODispatchBackground | IODispatchIdle,

                    IOOptimizeCombinable = 0x10000,

                    IOOptimizeOverrideMaxIOLimits = 0x20000,

                    IOOptimizeOverwriteTracing = 0x80000,

                    IOOptimizeMask = IOOptimizeCombinable | IOOptimizeOverrideMaxIOLimits | IOOptimizeOverwriteTracing,

                    IODispatchWriteMeted = 0x40000,

                    IOReadCopyZero = 0x100000,

                    IOReadCopyN = 0x600000,

                    IOReadCopyTestExclusiveIo = 0x700000,

                    IOReadCopyMask = IOReadCopyTestExclusiveIo,

                    UserPriorityClassIdMask = 0x0F000000,

                    UserPriorityMarkAsMaintenance = 0x40000000,

                    IOUserPriorityTraceMask = UserPriorityClassIdMask | UserPriorityMarkAsMaintenance,

                    IOInMask = IOOSMask | IODispatchMask | IOOptimizeMask | IOReadCopyMask | IOMiscOptionsMask,

                    IOCompleteIoCombined = 0x1000000000000,

                    IOCompleteReadGameOn = 0x2000000000000,

                    IOCompleteWriteGameOn = 0x4000000000000,

                    IOCompleteIoSlow = 0x8000000000000,

                    IOCompleteMask = IOCompleteIoCombined | IOCompleteReadGameOn | IOCompleteWriteGameOn | IOCompleteIoSlow,
                };
            }
        }
    }
}
