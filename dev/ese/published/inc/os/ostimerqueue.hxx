// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_TIMER_QUEUE_HXX_INCLUDED
#define _OS_TIMER_QUEUE_HXX_INCLUDED


//  handle and task prototype for OSTimerTasks

typedef void* POSTIMERTASK;
typedef void (*PfnTimerTask)( VOID* pvGroupContext, VOID* pvRuntimeContext );

//  creates a timer task scheduler for one-shot task scheduling.  the pfnTask
//  and the pvTaskGroupContext are the "primary key" for the identity of a task.
//  note this doesn't schedule the task, use OSTimerTaskScheduleTask for that.
//
ERR ErrOSTimerTaskCreate(
    PfnTimerTask const              pfnTask,
    const void * const              pvTaskGroupContext,
    POSTIMERTASK *                  pposttTimerHandle );

//  schedules a task for asynchronous execution at a given time with a fuzzy
//  slop delay.  the task gets scheduled to run once.  the task can be stopped
//  with OSTimerStopTask()
//
//  the task will run between dtickMinDelay and dtickMinDelay + dtickSlopDelay,
//  according to the preferences of the underlying OS.  Often this means the
//  task is scheduled the longest out at dtickMinDelay + dtickSlopDelay.
//
//  Note: The pvRuntimeContext must be used with care, because if two competing
//  threads attempt to post/schedule a task and they both have different runtime
//  contexts, one will win and the other will lose (won't get scheduled with its
//  runtime context).
//
VOID OSTimerTaskScheduleTask(
    POSTIMERTASK                    postt,
    const void * const              pvRuntimeContext,
    const TICK                      dtickMinDelay,
    const TICK                      dtickSlopDelay,
    const void **                   ppvRuntimeContextCancelled = NULL );

#define dtickSlopNoWake             0xFFFFFFFF

//  cancels (and quiesces) the outstanding task that was scheduled with 
//  OSTimerTaskScheduleTask().  If the task is scheduled in the future it
//  will be cancelled without running.  If it is currently running, this task
//  will wait for the callback / pfnTask to be completed.  Due to timing a
//  self-re-scheduling (immediate or short delay / 10 ms) task may be able 
//  to be scheduled again while this function is in operation, but will be 
//  done by the time this function finishes.
//
//  can not call this from the PfnTimerTask callback.
//
VOID OSTimerTaskCancelTask( POSTIMERTASK postt, _Out_opt_ const void ** ppvRuntimeContextCancelled = NULL );

//  cleans up and frees the timer task scheduler object
//
//  can not call this from the PfnTimerTask callback.
//
VOID OSTimerTaskDelete( POSTIMERTASK postt );

#ifdef DEBUG
//  checks if a timer task is active (either running or scheduled).
//
BOOL FOSTimerTaskActive( POSTIMERTASK postt );
#endif

#endif  //  _OS_TIMER_QUEUE_HXX_INCLUDED

