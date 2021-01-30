// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_TIMER_QUEUE_HXX_INCLUDED
#define _OS_TIMER_QUEUE_HXX_INCLUDED



typedef void* POSTIMERTASK;
typedef void (*PfnTimerTask)( VOID* pvGroupContext, VOID* pvRuntimeContext );

ERR ErrOSTimerTaskCreate(
    PfnTimerTask const              pfnTask,
    const void * const              pvTaskGroupContext,
    POSTIMERTASK *                  pposttTimerHandle );

VOID OSTimerTaskScheduleTask(
    POSTIMERTASK                    postt,
    const void * const              pvRuntimeContext,
    const TICK                      dtickMinDelay,
    const TICK                      dtickSlopDelay,
    const void **                   ppvRuntimeContextCancelled = NULL );

#define dtickSlopNoWake             0xFFFFFFFF

VOID OSTimerTaskCancelTask( POSTIMERTASK postt, _Out_opt_ const void ** ppvRuntimeContextCancelled = NULL );

VOID OSTimerTaskDelete( POSTIMERTASK postt );

#ifdef DEBUG
BOOL FOSTimerTaskActive( POSTIMERTASK postt );
#endif

#endif

