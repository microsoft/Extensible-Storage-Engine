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
                /// <summary>
                /// File Quality Of Service
                /// </summary>
                [Flags]
                public enum class FileQOS : Int64
                {
                    /// <summary>
                    /// Normal priority IO.
                    /// </summary>
                    IOOSNormalPriority = 0,

                    /// <summary>
                    /// Low priority IO.
                    /// </summary>
                    IOOSLowPriority = 0x2,

                    /// <summary>
                    /// IO priority mask.
                    /// </summary>
                    IOOSMask = IOOSNormalPriority | IOOSLowPriority,

                    /// <summary>
                    /// Pool QOS - preserves IO to prevent deadlock.
                    /// </summary>
                    IOPoolReserved = 0x4,

                    /// <summary>
                    /// Used to indicate sync-APIs want a wrnIOSlow for slow IOs.
                    /// </summary>
                    IOSignalSlowSyncIO = 0x8,

                    /// <summary>
                    /// IO misc options mask.
                    /// </summary>
                    IOMiscOptionsMask = IOPoolReserved | IOSignalSlowSyncIO,

                    /// <summary>
                    /// IO dispatch immediate.
                    /// </summary>
                    IODispatchImmediate = 0x10,

                    /// <summary>
                    /// Normal IO.
                    /// </summary>
                    IONormal = IODispatchImmediate,

                    /// <summary>
                    /// IO dispatch idle.
                    /// </summary>
                    IODispatchIdle = 0x20,

                    /// <summary>
                    /// IO dispatch background.
                    /// </summary>
                    IODispatchBackground = 0x40,

                    /// <summary>
                    /// IO dispatch urgent background level min.
                    /// </summary>
                    IODispatchUrgentBackgroundLevelMin = 0x0001,

                    /// <summary>
                    /// IO dispatch urgent background level max.
                    /// </summary>
                    IODispatchUrgentBackgroundLevelMax = 0x007F,

                    /// <summary>
                    /// IO dispatch urgent background shift.
                    /// </summary>
                    IODispatchUrgentBackgroundShft = 8,

                    /// <summary>
                    /// IO dispatch urgent background mask.
                    /// </summary>
                    IODispatchUrgentBackgroundMask = 0x7F00,

                    /// <summary>
                    /// IO dispatch urgent background min.
                    /// </summary>
                    IODispatchUrgentBackgroundMin = 0x0100,

                    /// <summary>
                    /// IO dispatch urgent background max.
                    /// </summary>
                    IODispatchUrgentBackgroundMax = 0x7F00,

                    /// <summary>
                    /// IO dispatch mask.
                    /// </summary>
                    IODispatchMask = IODispatchImmediate | IODispatchUrgentBackgroundMask | IODispatchBackground | IODispatchIdle,

                    /// <summary>
                    /// IO optimize combinable.
                    /// </summary>
                    IOOptimizeCombinable = 0x10000,

                    /// <summary>
                    /// IO optimize override max IO limits.
                    /// </summary>
                    IOOptimizeOverrideMaxIOLimits = 0x20000,

                    /// <summary>
                    /// IO optimize overwrite tracing.
                    /// </summary>
                    IOOptimizeOverwriteTracing = 0x80000,

                    /// <summary>
                    /// IO optimize mask.
                    /// </summary>
                    IOOptimizeMask = IOOptimizeCombinable | IOOptimizeOverrideMaxIOLimits | IOOptimizeOverwriteTracing,

                    /// <summary>
                    /// IO dispatch write meted.
                    /// </summary>
                    IODispatchWriteMeted = 0x40000,

                    /// <summary>
                    /// IO read copy zero.
                    /// </summary>
                    IOReadCopyZero = 0x100000,

                    /// <summary>
                    /// IO read copy N.
                    /// </summary>
                    IOReadCopyN = 0x600000,

                    /// <summary>
                    /// IO read copy test exclusive IO.
                    /// </summary>
                    IOReadCopyTestExclusiveIo = 0x700000,

                    /// <summary>
                    /// IO read copy mask.
                    /// </summary>
                    IOReadCopyMask = IOReadCopyTestExclusiveIo,

                    /// <summary>
                    /// User priority class id mask.
                    /// </summary>
                    UserPriorityClassIdMask = 0x0F000000,

                    /// <summary>
                    /// User priority mark as maintenance.
                    /// </summary>
                    UserPriorityMarkAsMaintenance = 0x40000000,

                    /// <summary>
                    /// User priority trace mask.
                    /// </summary>
                    IOUserPriorityTraceMask = UserPriorityClassIdMask | UserPriorityMarkAsMaintenance,

                    /// <summary>
                    /// IO input mask.
                    /// </summary>
                    IOInMask = IOOSMask | IODispatchMask | IOOptimizeMask | IOReadCopyMask | IOMiscOptionsMask,

                    /// <summary>
                    /// IO complete io combined.
                    /// </summary>
                    IOCompleteIoCombined = 0x1000000000000,

                    /// <summary>
                    /// IO complete read game on.
                    /// </summary>
                    IOCompleteReadGameOn = 0x2000000000000,

                    /// <summary>
                    /// IO complete write game on.
                    /// </summary>
                    IOCompleteWriteGameOn = 0x4000000000000,

                    /// <summary>
                    /// IO complete IO slow.
                    /// </summary>
                    IOCompleteIoSlow = 0x8000000000000,

                    /// <summary>
                    /// IO complete mask.
                    /// </summary>
                    IOCompleteMask = IOCompleteIoCombined | IOCompleteReadGameOn | IOCompleteWriteGameOn | IOCompleteIoSlow,
                };
            }
        }
    }
}
