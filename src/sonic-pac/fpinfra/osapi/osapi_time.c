/*
 * Copyright 2024 Broadcom Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
//#include "l7_linux_version.h"
#include <sys/time.h>
#include <pthread.h>

//#include "pacinfra_common.h"
#include "datatypes.h"
#include "commdefs.h"
#include "resources.h"
#include "osapi.h"
#include "log.h"

#include "osapi_priv.h"

typedef struct osapiTimerListEntry_s {

  osapiTimerDescr_t timer;
  struct timespec ts_expiry;
  struct osapiTimerListEntry_s *next, *prev;

} osapiTimerListEntry_t;

typedef struct osapiTimerAddEntry_s
{
  void                          (*func32)(uint32, uint32);
  void                          (*func64)( uint64,  uint64);
   uint64                     arg1;
   uint64                     arg2;
  uint32                     milliseconds;
  osapiTimerDescr_t     **pTimerHolder;

} osapiTimerAddEntry;

static osapiTimerListEntry_t *osapiTimerList = NULL;
static osapiTimerListEntry_t *osapiTimerExpired = NULL;
static osapiTimerListEntry_t *osapiTimerTmp = NULL;
static osapiTimerListEntry_t *osapiTimerListOrig = NULL;
static osapiTimerListEntry_t *osapiTimerListEndOrig = NULL;

static osapiTimerListEntry_t *osapiTimerFreeListHead = NULL;
static osapiTimerListEntry_t *osapiTimerFreeListTail = NULL;
static uint32 osapiDebugTimerActiveCount = 0, osapiDebugTimerFailAddCount = 0;
static osapiTimerDescr_t *osapiDebugTimerDetail[OSAPI_MAX_TIMERS];
static uint32 osapiDebugTimerCallbackDetailEnableFlag = 0;

#ifdef COMMENTED_OUT

static pthread_mutex_t osapiTimerFreeLock = PTHREAD_MUTEX_INITIALIZER;

#define OSAPI_TIMER_FREE_SEM_TAKE               \
  pthread_mutex_lock(&osapiTimerFreeLock)

#define OSAPI_TIMER_FREE_SEM_GIVE pthread_mutex_unlock(&osapiTimerFreeLock)

#endif /* COMMENTED_OUT */

static pthread_mutex_t osapiTimerLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t osapiPeriodicTimerLock = PTHREAD_MUTEX_INITIALIZER;

#define OSAPI_TIMER_SYNC_SEM_TAKE               \
  pthread_mutex_lock(&osapiTimerLock)

#define OSAPI_TIMER_SYNC_SEM_GIVE pthread_mutex_unlock(&osapiTimerLock)

#define OSAPI_TIMER_PERIODIC_SEM_TAKE           \
  pthread_mutex_lock(&osapiPeriodicTimerLock)

#define OSAPI_TIMER_PERIODIC_SEM_GIVE pthread_mutex_unlock(&osapiPeriodicTimerLock)

static pthread_cond_t osapiTimerCond;

/**************************************************************************
 * Provide periodic timer resources
 *************************************************************************/
/* define the number of periodic timers (COUNT) and a miss cather (MISS_CNT) */
#define OSAPI_PERIODIC_TIMER_COUNT     20
#define OSAPI_PERIODIC_TIMER_MISS_CNT   2

/* Callback Execution Time in milliseconds */
#define OSAPI_TIMER_CALLBACK_NOMINAL_EXECUTION_TIME_MS 100
typedef struct {
  uint32     handle;
  uint32     period;
  uint32     nextTime;
  uint32     taskId;
} OSAPI_PERIODIC_TIMER_t;
static OSAPI_PERIODIC_TIMER_t  osapiPeriodicTimer[OSAPI_PERIODIC_TIMER_COUNT + 1];

/**************************************************************************
 * @purpose  Sleep for a given number of seconds.
 *
 * @param    sec @b{(input)}   number of seconds to sleep.
 *
 * @returns  none.
 *
 * @comments    none.
 *
 * @end
 *************************************************************************/
void osapiSleep( uint32 sec) {

  struct timespec delay, remains;

  if ( sec == 0 ) {

    sec = 1;

  }

  delay.tv_sec = sec;
  delay.tv_nsec = 0;

  while (nanosleep (&delay, &remains) != 0) {
     
    memcpy(&delay, &remains, sizeof(struct timespec));

  }

  return;

}

/**************************************************************************
 * @purpose  Sleep for a given number of micro seconds
 *
 * @param    usec @b{(input)}  Number of micro-seconds to sleep.
 *
 * @returns  none.
 *
 * @comments    CAUTION! The precision is in system ticks per second as
 * @comments    determined by the system clock rate, even though the units
 * @comments    are in microseconds.
 *
 * @end
 *************************************************************************/
void osapiSleepUSec( uint32 usec) {

  struct timespec delay, remains;

  if (usec < OSAPI_TICK_USEC) {

    usec = OSAPI_TICK_USEC;

  }

  delay.tv_sec = usec / 1000000;
  delay.tv_nsec = (usec % 1000000)*1000;

  while (nanosleep (&delay, &remains) != 0) {

    memcpy(&delay, &remains, sizeof(struct timespec));

  }

  return;

}

/**************************************************************************
 * @purpose  Sleep for a given number of milliseconds
 *
 * @param    msec @b{(input)}  Number of milliseconds to sleep.
 *
 * @returns  none.
 *
 * @comments    CAUTION! The precision is in system ticks per second as
 * @comments    determined by the system clock rate, even though the units
 * @comments    are in milliseconds.
 *
 * @end
 *************************************************************************/
void osapiSleepMSec( uint32 msec) {

  struct timespec delay, remains;

  if (msec < (OSAPI_TICK_USEC / 1000)) {

    msec = (OSAPI_TICK_USEC / 1000);

  }

  delay.tv_sec = msec / 1000;
  delay.tv_nsec = (msec % 1000)*1000000;

  while (nanosleep (&delay, &remains) != 0) {

    memcpy(&delay, &remains, sizeof(struct timespec));

  }

  return;

}

/**************************************************************************
 * @purpose  stop an already running timer
 *
 * @param    osapitimer ptr to an osapi timer descriptor
 *
 * @returns   SUCCESS
 * @returns   FAILURE if osapitimer is not in timer table
 *
 * @comments    none.
 *
 * @end
 *************************************************************************/
RC_t osapiStopUserTimerMain (osapiTimerDescr_t *osapitimer)
{
  osapiTimerListEntry_t *curEntry = (osapiTimerListEntry_t *)osapitimer;
  uint32 running;

  running = curEntry->timer.timer_running;

  if (running != 0)
  {
    if (curEntry->next != NULL) {

      curEntry->next->prev = curEntry->prev;

    }

    if (curEntry->prev != NULL) {

      curEntry->prev->next = curEntry->next;

    } else { /* This timer is head of list... */

      if (curEntry == osapiTimerList)
      {
        osapiTimerList = curEntry->next;

        /* Notify timer task of new list head... */
        pthread_cond_signal(&osapiTimerCond);
      }
    }

    curEntry->next = NULL;
    curEntry->prev = NULL;
    curEntry->timer.timer_running = 0;
  }

  return  SUCCESS;
}

/**************************************************************************
 * @purpose  stop an already running timer
 *
 * @param    osapitimer ptr to an osapi timer descriptor
 *
 * @returns   SUCCESS
 * @returns   FAILURE if osapitimer is not in timer table
 *
 * @comments    none.
 *
 * @end
 *************************************************************************/
RC_t osapiStopUserTimer (osapiTimerDescr_t *osapitimer)
{
  int SaveCancelType;

  if (osapitimer < (osapiTimerDescr_t *) osapiTimerListOrig)
  {
    osapi_printf("osapiStopUserTimer: Timer %p out of range (<)!\n", osapitimer);
    return  FAILURE;
  }
  if (osapitimer > (osapiTimerDescr_t *) osapiTimerListEndOrig)
  {
    osapi_printf("osapiStopUserTimer: Timer %p out of range (>)!\n", osapitimer);
    return  FAILURE;
  }

  OSAPI_TIMER_SYNC_SEM_TAKE;

  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &SaveCancelType);
  (void) osapiStopUserTimerMain ((void *)osapitimer);
  pthread_setcanceltype(SaveCancelType, NULL);

  OSAPI_TIMER_SYNC_SEM_GIVE;

  return  SUCCESS;
}

/**************************************************************************
 * @purpose  restarts a stopped timer
 *
 * @param    osapitimer ptr to an osapi timer descriptor to reset
 *
 * @returns   SUCCESS
 * @returns   FAILURE if osapitimer is not in timer table
 *
 * @comments    none.
 *
 * @end
 *************************************************************************/
RC_t osapiRestartUserTimerMain (osapiTimerDescr_t *osapitimer)
{
  struct timeval curTime;
  osapiTimerListEntry_t *curEntry, *prevEntry;
  osapiTimerListEntry_t *newEntry = (osapiTimerListEntry_t *)osapitimer;
  uint32 running, in_use;

  in_use = newEntry->timer.timer_in_use;

  /*
    If the timer has been freed before the call to
    osapiRestartUserTimer, make sure the timer does not get
    added to the queue
  */

  if (in_use != 0)
  {
    /*
      If this timer is already running, don't start it again
    */

    running = newEntry->timer.timer_running;

    if (running != 1)
    {
      {
        struct timespec tp;
        int rc;

        rc = clock_gettime (CLOCK_MONOTONIC, &tp);
        if (rc) 
        {
          LOG_ERROR(rc);
        }
        curTime.tv_sec = tp.tv_sec;
        curTime.tv_usec = tp.tv_nsec / 1000;
      }

      newEntry->ts_expiry.tv_sec = (curTime.tv_sec
                                    + (newEntry->timer.time_count / 1000));
      newEntry->ts_expiry.tv_nsec = ((curTime.tv_usec
                                      + ((newEntry->timer.time_count % 1000) * 1000))
                                     * 1000);

      if (newEntry->ts_expiry.tv_nsec >= 1000000000) {

        newEntry->ts_expiry.tv_nsec -= 1000000000;
        newEntry->ts_expiry.tv_sec++;

      }

      /* Check for new timer list head... */
      if ((osapiTimerList == NULL)
          || (newEntry->ts_expiry.tv_sec < osapiTimerList->ts_expiry.tv_sec)
          || ((newEntry->ts_expiry.tv_sec == osapiTimerList->ts_expiry.tv_sec)
              && (newEntry->ts_expiry.tv_nsec < osapiTimerList->ts_expiry.tv_nsec))) {

        /* Notify timer task of new list head... */
        pthread_cond_signal(&osapiTimerCond);

        /* Establish proper links in chain... */
        newEntry->next = osapiTimerList;
        newEntry->prev = NULL;
        osapiTimerList = newEntry;

      } else { /* Insert new timer at proper list position... */

        curEntry = osapiTimerList;

        do { /* Calculate correct time delta... */

          prevEntry = curEntry;

          curEntry = curEntry->next;

        } while ((curEntry != NULL)
                 && ((newEntry->ts_expiry.tv_sec > curEntry->ts_expiry.tv_sec)
                     || ((newEntry->ts_expiry.tv_sec == curEntry->ts_expiry.tv_sec)
                         && (newEntry->ts_expiry.tv_nsec > curEntry->ts_expiry.tv_nsec))));

        /* Establish proper links in chain... */
        newEntry->next = curEntry;
        prevEntry->next = newEntry;
        newEntry->prev = prevEntry;

      }

      if (newEntry->next != NULL)
      {
        newEntry->next->prev = newEntry;
      }

      newEntry->timer.timer_running = 1;
    }
  }

  return  SUCCESS;
}

/**************************************************************************
 * @purpose  restarts a stopped timer
 *
 * @param    osapitimer ptr to an osapi timer descriptor to reset
 *
 * @returns   SUCCESS
 * @returns   FAILURE if osapitimer is not in timer table
 *
 * @comments    none.
 *
 * @end
 *************************************************************************/
RC_t osapiRestartUserTimer (osapiTimerDescr_t *osapitimer)
{
  int SaveCancelType;

  if (osapitimer < (osapiTimerDescr_t *) osapiTimerListOrig)
  {
    osapi_printf("osapiRestartUserTimer: Timer %p out of range (<)!\n", osapitimer);
    return  FAILURE;
  }
  if (osapitimer > (osapiTimerDescr_t *) osapiTimerListEndOrig)
  {
    osapi_printf("osapiRestartUserTimer: Timer %p out of range (>)!\n", osapitimer);
    return  FAILURE;
  }

  OSAPI_TIMER_SYNC_SEM_TAKE;

  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &SaveCancelType);
  (void) osapiRestartUserTimerMain ((void *) osapitimer);
  pthread_setcanceltype(SaveCancelType, NULL);

  OSAPI_TIMER_SYNC_SEM_GIVE;

  return  SUCCESS;
}

/**************************************************************************
 * @purpose  change a running timer's count value
 *
 * @param    osapitimer ptr to an osapi timer descriptor
 *
 * @returns   SUCCESS
 * @returns   FAILURE if osapitimer is not in timer table
 *
 * @comments    none.
 *
 * @end
 *************************************************************************/
RC_t osapiChangeUserTimerMain(osapiTimerChangeEntry *Entry)
{
  (void) osapiStopUserTimerMain(Entry->osapitimer);

  (Entry->osapitimer)->time_count = Entry->newTimeCount;
  (Entry->osapitimer)->orig_count = Entry->newTimeCount;

  osapiRestartUserTimerMain(Entry->osapitimer);

  return  SUCCESS;
}

/**************************************************************************
 * @purpose  change a running timer's count value
 *
 * @param    osapitimer ptr to an osapi timer descriptor
 *
 * @returns   SUCCESS
 * @returns   FAILURE if osapitimer is not in timer table
 *
 * @comments    none.
 *
 * @end
 *************************************************************************/
RC_t osapiChangeUserTimer(osapiTimerDescr_t *osapitimer,
                             uint32 newTimeCount)
{
  osapiTimerChangeEntry Entry;
  int SaveCancelType;

  if (osapitimer < (osapiTimerDescr_t *) osapiTimerListOrig)
  {
    osapi_printf("osapiChangeUserTimer: Timer %p out of range (<)!\n", osapitimer);
    return  FAILURE;
  }
  if (osapitimer > (osapiTimerDescr_t *) osapiTimerListEndOrig)
  {
    osapi_printf("osapiChangeUserTimer: Timer %p out of range (>)!\n", osapitimer);
    return  FAILURE;
  }

  Entry.osapitimer = osapitimer;
  Entry.newTimeCount = newTimeCount;

  OSAPI_TIMER_SYNC_SEM_TAKE;

  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &SaveCancelType);
  (void) osapiChangeUserTimerMain ((void *) &Entry);
  pthread_setcanceltype(SaveCancelType, NULL);

  OSAPI_TIMER_SYNC_SEM_GIVE;

  return  SUCCESS;
}

/**************************************************************************
 * @purpose  This is the user function to setup a new timeout call.
 *
 * @param    func           is the function to call when expired.
 * @param    arg(2)         arguments to pass to the callback function.
 * @param    milliseconds   number of milli-seconds to wait before timeout.
 * @param    pTimerHolder   ptr to an osapiTimerDescr_t struct used to deallocate this timer by the user
 *
 * @returns  none.
 *
 * @comments    none.
 *
 * @end
 *************************************************************************/
void osapiTimerAddMain (osapiTimerAddEntry *Entry)
{
  osapiTimerListEntry_t *tmpEntry;

  /* this is not a mistake; head of osapiTimerListEntry_t is an
     osapiTimerDescr_t */

  if ((*(Entry->pTimerHolder) = (osapiTimerDescr_t *)osapiTimerFreeListHead) != NULL)
  {
    tmpEntry = osapiTimerFreeListHead;
    osapiTimerFreeListHead = osapiTimerFreeListHead->next;

    if (osapiTimerFreeListHead == NULL)
    {
      osapiTimerFreeListTail = NULL;
    }

    tmpEntry->next = NULL;

    (*(Entry->pTimerHolder))->callback32 = Entry->func32;
    (*(Entry->pTimerHolder))->callback64 = Entry->func64;
    (*(Entry->pTimerHolder))->parm1 = Entry->arg1;
    (*(Entry->pTimerHolder))->parm2 = Entry->arg2;
    (*(Entry->pTimerHolder))->timer_in_use = 1;
    (*(Entry->pTimerHolder))->timer_running = 0;
    (*(Entry->pTimerHolder))->time_count = Entry->milliseconds;
    (*(Entry->pTimerHolder))->orig_count = Entry->milliseconds;

    osapiDebugTimerActiveCount++;
  }
  else
  {
    osapi_printf("osapiTimerAddMain: No free timers available!\n");
    osapiDebugTimerFailAddCount++;
    LOG_ERROR(osapiDebugTimerActiveCount);
  }

  if (*(Entry->pTimerHolder) != NULL)
  {
    osapiRestartUserTimerMain(*(Entry->pTimerHolder));
  }

  return;
}

/**************************************************************************
 * @purpose  This is the user function to setup a new timeout call.
 *
 * @param    func           is the function to call when expired.
 * @param    arg(2)         arguments to pass to the callback function.
 * @param    milliseconds   number of milli-seconds to wait before timeout.
 * @param    pTimerHolder   ptr to an osapiTimerDescr_t struct used to deallocate this timer by the user
 *
 * @returns  none.
 *
 * @comments    none.
 *
 * @end
 *************************************************************************/
void osapiTimerAdd(void (*func)( uint32, uint32 ),
                   uint32 arg1, uint32 arg2,
                    int32 milliseconds, osapiTimerDescr_t **pTimerHolder)
{
  osapiTimerAddEntry Entry;
  int SaveCancelType;

  if (func == NULL)
  {
    osapi_printf("osapiTimerAdd: Timer with NULL callback NOT added!\n");
    return;
  }

  Entry.func32 = func;
  Entry.func64 = 0;
  Entry.arg1 = arg1;
  Entry.arg2 = arg2;
  Entry.milliseconds = milliseconds;
  Entry.pTimerHolder = pTimerHolder;

  OSAPI_TIMER_SYNC_SEM_TAKE;

  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &SaveCancelType);
  (void) osapiTimerAddMain ((void *) &Entry);
  pthread_setcanceltype(SaveCancelType, NULL);

  OSAPI_TIMER_SYNC_SEM_GIVE;

  return;

}

/**************************************************************************
 * @purpose  This is the user function to setup a new timeout call.
*           This function accepts 64 bit user parameters.
*           This function should be used when passing pointers as
*           timer arguments.
 *
 * @param    func           is the function to call when expired.
 * @param    arg(2)         arguments to pass to the callback function.
 * @param    milliseconds   number of milli-seconds to wait before timeout.
 * @param    pTimerHolder   ptr to an osapiTimerDescr_t struct used to deallocate this timer by the user
 *
 * @returns  none.
 *
 * @comments    none.
 *
 * @end
 *************************************************************************/
void osapiTimer64Add(void (*func)(  uint64,  uint64 ),
                    uint64 arg1,  uint64 arg2,
                    int32 milliseconds, osapiTimerDescr_t **pTimerHolder)
{
  osapiTimerAddEntry Entry;
  int SaveCancelType;

  if (func == NULL)
  {
    osapi_printf("osapiTimerAdd: Timer with NULL callback NOT added!\n");
    return;
  }

  Entry.func64 = func;
  Entry.func32 = 0;
  Entry.arg1 = arg1;
  Entry.arg2 = arg2;
  Entry.milliseconds = milliseconds;
  Entry.pTimerHolder = pTimerHolder;

  OSAPI_TIMER_SYNC_SEM_TAKE;

  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &SaveCancelType);
  (void) osapiTimerAddMain ((void *) &Entry);
  pthread_setcanceltype(SaveCancelType, NULL);

  OSAPI_TIMER_SYNC_SEM_GIVE;

  return;

}

/**************************************************************************
 * @purpose  Remove a function from the timeout list.
 *
 * @param    pTimerHolder   ptr to an osapi timer descriptor, that was returned in osapiTimerAdd(), to deallocate.
 *
 * @returns  none.
 *
 * @comments    none.
 *
 * @end
 *************************************************************************/
void osapiTimerFreeMain(osapiTimerDescr_t *pTimer)
{
  uint32     in_use;

  in_use = pTimer->timer_in_use;

  /*
    Don't allow a timer that has just been freed to be freed
    again
  */

  if (in_use != 0)
  {
    (void) osapiStopUserTimerMain(pTimer);


    /* osapiStopUserTimerMain may not set the next/prev pointer to null
     * if the timer has already stopped.
     */
    ((osapiTimerListEntry_t *)pTimer)->next = NULL;
    ((osapiTimerListEntry_t *)pTimer)->prev = NULL;

    /* this is not a mistake; head of osapiTimerListEntry_t is an
       osapiTimerDescr_t */

    if (osapiTimerFreeListTail == NULL)
    {
      osapiTimerFreeListTail = (osapiTimerListEntry_t *)pTimer;
      osapiTimerFreeListHead = (osapiTimerListEntry_t *)pTimer;

    } else
    {
      osapiTimerFreeListTail->next = (osapiTimerListEntry_t *)pTimer;
      osapiTimerFreeListTail = osapiTimerFreeListTail->next;
    }
                

    pTimer->timer_in_use = 0;
    --osapiDebugTimerActiveCount;
  }

  return;
}

/**************************************************************************
 * @purpose  Remove a function from the timeout list.
 *
 * @param    pTimerHolder   ptr to an osapi timer descriptor, that was returned in osapiTimerAdd(), to deallocate.
 *
 * @returns  none.
 *
 * @comments    none.
 *
 * @end
 *************************************************************************/
void osapiTimerFree(osapiTimerDescr_t *pTimer)
{
  int SaveCancelType;

  if (pTimer ==  NULL) 
  {
    return;
  }

  if (pTimer < (osapiTimerDescr_t *) osapiTimerListOrig)
  {
    osapi_printf("osapiTimerFree: Timer %p out of range (<)!\n", pTimer);
    return;
  }
  if (pTimer > (osapiTimerDescr_t *) osapiTimerListEndOrig)
  {
    osapi_printf("osapiTimerFree: Timer %p out of range (>)!\n", pTimer);
    return;
  }

  OSAPI_TIMER_SYNC_SEM_TAKE;

  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &SaveCancelType);
  (void) osapiTimerFreeMain ((void *)pTimer);
  pthread_setcanceltype(SaveCancelType, NULL);

  OSAPI_TIMER_SYNC_SEM_GIVE;

  return;
}

/**************************************************************************
 * @purpose  Task that wakes up periodically and invokes active timers.
 *
 * @param    none.
 *
 * @returns  none.
 *
 * @comments    none.
 *
 * @end
 *************************************************************************/
void osapiTimerHandler(void)
{
  int i;
  osapiTimerDescr_t expTimer;
  struct timespec pop_time;
  struct timespec diff_time;
  pthread_condattr_t cat;
  uint32 preCallbackTime;
  uint32 postCallbackTime;
  osapiTimerDescr_t *pTimer = NULL;
  uint32 offset;
   char8  nameBuf[30];
  RC_t   rc;

  (void) pthread_condattr_init (&cat);
  (void) pthread_condattr_setclock (&cat, CLOCK_MONOTONIC);
  (void) pthread_cond_init (&osapiTimerCond, &cat);

  expTimer.parm1 = 0;
  expTimer.parm2 = 0;
  OSAPI_TIMER_SYNC_SEM_TAKE;
  memset(&pop_time, 0x0, sizeof(pop_time));

  /* Allocate and initialize timer free list */
  osapiTimerFreeListHead = (osapiTimerListEntry_t *)osapiMalloc( OSAPI_COMPONENT_ID, 
                                                                sizeof(osapiTimerListEntry_t) * OSAPI_MAX_TIMERS);

  if (osapiTimerFreeListHead != NULL) {

    osapiTimerTmp = osapiTimerFreeListHead;

    for (i = 0; i < (OSAPI_MAX_TIMERS - 1); i++) {

      osapiTimerTmp->next = (osapiTimerTmp + 1);
      osapiTimerTmp++;

    }

    osapiTimerFreeListTail = osapiTimerTmp;
    osapiTimerListEndOrig = osapiTimerTmp;
    osapiTimerTmp = osapiTimerTmp->next = NULL;

  }

  osapiTimerListOrig = osapiTimerFreeListHead;

  OSAPI_TIMER_SYNC_SEM_GIVE;

  (void) osapiTaskInitDone( OSAPI_TIMER_TASK_SYNC);

  if (osapiTimerFreeListHead == NULL) {

    return; /* kills this task */

  }

  expTimer.callback32 = NULL;
  expTimer.callback64 = NULL;
  expTimer.parm1 = 0;
  expTimer.parm2 = 0;

  for (;;) {

    OSAPI_TIMER_SYNC_SEM_TAKE;

    if (osapiTimerList == NULL) {

      pthread_cond_wait(&osapiTimerCond, &osapiTimerLock);

    } else {

      /* setup time value */

      if (pthread_cond_timedwait(&osapiTimerCond, &osapiTimerLock,
                                 &(osapiTimerList->ts_expiry)) != 0) {

        /* Get popped timer... */
        osapiTimerExpired = osapiTimerList;

        if (osapiTimerExpired != NULL)
        {
          /*
            Note that the pthread_cond_timedwait must unlock the
            osapiTimerLock semaphore.  That means that the list
            could have changed while waiting on the timer.
          */

          /* copy the time we popped for later use */
          memcpy(&pop_time,&(osapiTimerExpired->ts_expiry),sizeof(struct timespec));

          /* Advance to next list entry... */
          osapiTimerList = osapiTimerExpired->next;

          if (osapiTimerList != NULL)
          {
            /*
              Head of list must have NULL prev pointer, or
              a subsequent osapiStopUserTimer call will
              not notice that the timer stopped was at the
              head of the list
            */

            osapiTimerList->prev = NULL;
          }

          if ((osapiTimerExpired->timer.callback32 == NULL) &&
               (osapiTimerExpired->timer.callback64 == NULL))
          {
            osapi_printf("osapiTimerHandler: Timer %p callback NULL!, next %p, prev %p\n",
                         &(osapiTimerExpired->timer),
                         osapiTimerExpired->next,
                         osapiTimerExpired->prev);
          }

          osapiTimerExpired->timer.time_count = 0;
          osapiTimerExpired->timer.timer_running = 0;

          expTimer.callback32 = osapiTimerExpired->timer.callback32;
          expTimer.callback64 = osapiTimerExpired->timer.callback64;
          expTimer.parm1 = osapiTimerExpired->timer.parm1;
          expTimer.parm2 = osapiTimerExpired->timer.parm2;

          osapiTimerFreeMain(&(osapiTimerExpired->timer));

          pTimer = &(osapiTimerExpired->timer);

          osapiTimerExpired = NULL;
        }
      }
    }

    OSAPI_TIMER_SYNC_SEM_GIVE;

    if ((expTimer.callback32 != NULL) ||
        (expTimer.callback64 != NULL))
    {
      preCallbackTime = osapiUpTimeMillisecondsGet();
      /* Execute popped timer's callback... */
      if (expTimer.callback32) 
      {
        (*expTimer.callback32)(expTimer.parm1, expTimer.parm2);
      } else
      {
        (*expTimer.callback64)(expTimer.parm1, expTimer.parm2);
      }
      postCallbackTime = osapiUpTimeMillisecondsGet();

      if (osapiDebugTimerCallbackDetailEnableFlag)
      {
        if (pTimer != NULL)
        {
          pTimer->execution_time = postCallbackTime - preCallbackTime;
          if (pTimer->execution_time > OSAPI_TIMER_CALLBACK_NOMINAL_EXECUTION_TIME_MS)
          {
            memset(nameBuf, 0x0, sizeof(nameBuf));
            if (pTimer->callback32) 
            {
              rc = osapiFunctionLookup(pTimer->callback32, 
                                       nameBuf, sizeof(nameBuf), &offset);
            } else
            {
              rc = osapiFunctionLookup(pTimer->callback64, 
                                       nameBuf, sizeof(nameBuf), &offset);
            }
            osapi_printf("Timer callback function %s taking %d ms, longer than expected\n", 
                         ((rc ==  SUCCESS)? nameBuf: "TBD"), pTimer->execution_time);
          }
          pTimer = NULL;
        }
      }

      expTimer.callback32 = NULL;
      expTimer.callback64 = NULL;
      expTimer.parm1 = 0;
      expTimer.parm2 = 0;

      /* set diff time to invalid values to ensure that we
       * don't find ourselves in a situation where
       * osapiTimerList is NULL when testing the if conditional
       * but non NULL when testing the while loop condition
       * if that happens we may be expiring a timer that is not
       * yet ready to pop
       * This situation can happen if our schedule quanta
       * expires after the if but before the while
       */

      diff_time.tv_sec = 0x7fffffff; /*signed value*/
      diff_time.tv_nsec = 0x7fffffff; /*signed value*/

      /* Check to see if any subsequent timers have expired */
      if(osapiTimerList != NULL)
      {
        diff_time.tv_sec = osapiTimerList->ts_expiry.tv_sec - pop_time.tv_sec;
        diff_time.tv_nsec = osapiTimerList->ts_expiry.tv_nsec - pop_time.tv_nsec;
      }

      while((osapiTimerList != NULL) &&
            (diff_time.tv_sec == 0) &&
            (diff_time.tv_nsec < 10000000) &&
            (diff_time.tv_nsec > 0))
      {
        /*
         *falling into the while loop condition
         *indicates that the next timer pop is
         *less than 1 jiffy away (10000000 nanoseconds)
         *which means we may as well handle it now
         *The alternative is to sleep until we are
         *next scheduled, which will guarantee
         *a timer skew
         */

        OSAPI_TIMER_SYNC_SEM_TAKE;

        /* Get popped timer... */
        osapiTimerExpired = osapiTimerList;

        if (osapiTimerExpired != NULL)
        {
          /* Advance to next list entry... */
          osapiTimerList = osapiTimerExpired->next;

          if (osapiTimerList != NULL)
          {
            /*
              Head of list must have NULL prev pointer, or
              a subsequent osapiStopUserTimer call will
              not notice that the timer stopped was at the
              head of the list
            */

            osapiTimerList->prev = NULL;
          }

          osapiTimerExpired->timer.time_count = 0;
          osapiTimerExpired->timer.timer_running = 0;

          expTimer.callback32 = osapiTimerExpired->timer.callback32;
          expTimer.callback64 = osapiTimerExpired->timer.callback64;
          expTimer.parm1 = osapiTimerExpired->timer.parm1;
          expTimer.parm2 = osapiTimerExpired->timer.parm2;

          osapiTimerFreeMain(&(osapiTimerExpired->timer));

          pTimer = &(osapiTimerExpired->timer);

          osapiTimerExpired = NULL;
        }

        OSAPI_TIMER_SYNC_SEM_GIVE;

        if ((expTimer.callback32 != NULL) ||
            (expTimer.callback64 != NULL))
        {
          /* Execute popped timer's callback... */
          preCallbackTime = osapiUpTimeMillisecondsGet();
          if (expTimer.callback32) 
          {
            (*expTimer.callback32)(expTimer.parm1, expTimer.parm2);
          } else
          {
            (*expTimer.callback64)(expTimer.parm1, expTimer.parm2);
          }
          postCallbackTime = osapiUpTimeMillisecondsGet();

          if (osapiDebugTimerCallbackDetailEnableFlag)
          {
            if (pTimer != NULL)
            {
              pTimer->execution_time = postCallbackTime - preCallbackTime;
  
              if (pTimer->execution_time > OSAPI_TIMER_CALLBACK_NOMINAL_EXECUTION_TIME_MS)
              {
                memset(nameBuf, 0x0, sizeof(nameBuf));
                if (pTimer->callback32) 
                {
                  rc = osapiFunctionLookup(pTimer->callback32, 
                                           nameBuf, sizeof(nameBuf), &offset);
                } else
                {
                  rc = osapiFunctionLookup(pTimer->callback64, 
                                           nameBuf, sizeof(nameBuf), &offset);
                }
                osapi_printf("Timer callback function %s taking %d ms, longer than expected\n",
                             ((rc ==  SUCCESS)? nameBuf: "TBD"), pTimer->execution_time);
              }
              pTimer = NULL;
            }
          }

          expTimer.callback32 = NULL;
          expTimer.callback64 = NULL;
          expTimer.parm1 = 0;
          expTimer.parm2 = 0;
        }

        /*
         *recompute the difference to the next timer in the list
         *Note: We don't move the pop_time structure forward here.
         *not sure if we should or not. It seems doing so would
         *open up the possibility that the timer may begin to
         *execute timers before they actually expire (by more
         *than 1 jiffy at least)
         */

        diff_time.tv_sec = 0x7fffffff;
        diff_time.tv_nsec = 0x7fffffff;

        if(osapiTimerList != NULL)
        {
          diff_time.tv_sec = osapiTimerList->ts_expiry.tv_sec - pop_time.tv_sec;
          diff_time.tv_nsec = osapiTimerList->ts_expiry.tv_nsec - pop_time.tv_nsec;
        }

      } /* end while */
    } /* end if */
  } /* end for */

  return;

}

/**************************************************************************
 * @purpose  Provide periodic timer indications to delay sensitive tasks. Use
 *           of these utilities minimizes accumulated skew.
 *
 * @param    uint32  period   The fixed period.
 *           uint32 *handle   User provided storage for assigned handle.
 *
 * @returns  Failure message is no timers remain
 *
 * @comments The first available timer is assigned and the handle is returned
 *           via the handle argument.
 *
 * @end
 *************************************************************************/
RC_t osapiPeriodicUserTimerRegister(uint32 period, uint32 *handle)
{
  uint32 index;
  RC_t rc =  TABLE_IS_FULL;

  OSAPI_TIMER_PERIODIC_SEM_TAKE;
  for (index = 1; index < OSAPI_PERIODIC_TIMER_COUNT; index++)
  {
    if (osapiPeriodicTimer[index].handle ==  0)
    {
      osapiPeriodicTimer[index].handle   = index;
      osapiPeriodicTimer[index].period   = period;
      osapiPeriodicTimer[index].nextTime = osapiTimeMillisecondsGet();
      osapiPeriodicTimer[index].taskId   =  0;
      *handle = osapiPeriodicTimer[index].handle;
      rc =  SUCCESS;
      break;
    }
  }

  OSAPI_TIMER_PERIODIC_SEM_GIVE;
  return rc;
}

/**************************************************************************
 * @purpose  Wait on a previously defined periodic timer.
 *
 * @param    uint32 handle   Assigned periodic timer handle.
 *
 * @returns  Nothing
 *
 * @comments Timer period previously supplied is used.
 *
 * @end
 *************************************************************************/
void osapiPeriodicUserTimerWait(uint32 handle)
{
  uint32 now, waitTime;

  OSAPI_TIMER_PERIODIC_SEM_TAKE;
  osapiPeriodicTimer[handle].nextTime += osapiPeriodicTimer[handle].period;
  now = osapiTimeMillisecondsGet();
  waitTime = osapiPeriodicTimer[handle].nextTime - now;


  if((now > osapiPeriodicTimer[handle].nextTime) || (waitTime > osapiPeriodicTimer[handle].period))
  {
    osapiPeriodicTimer[handle].nextTime = now + osapiPeriodicTimer[handle].period;
    waitTime = osapiPeriodicTimer[handle].period;
  }

  OSAPI_TIMER_PERIODIC_SEM_GIVE;
  osapiSleepMSec(waitTime);
}

/**************************************************************************
 * @purpose  Wait on a previously defined periodic timer.
 *
 * @param    uint32 handle   Assigned periodic timer handle.
 *
 * @returns  Nothing
 *
 * @comments Timer period previously supplied is used.
 *
 * @end
 *************************************************************************/
RC_t osapiPeriodicUserTimerDeregister(uint32 handle)
{
  if ((0 == osapiPeriodicTimer[handle].taskId) &&
      (handle  == osapiPeriodicTimer[handle].handle))
  {
    osapiPeriodicTimer[handle].handle = 0;
    osapiPeriodicTimer[handle].taskId = 0;
    return  SUCCESS;
  }

  return  FAILURE;
}

void osapiPrintTimerDetail(osapiTimerDescr_t *ptimer)
{
  osapiTimerListEntry_t *Entry = (osapiTimerListEntry_t *) ptimer;

  osapi_printf("  Timer.callback32: %p\n", ptimer->callback32);
  osapi_printf("  Timer.callback64: %p\n", ptimer->callback64);
  osapi_printf("  Timer.parm1: 0x%llx\n", ptimer->parm1);
  osapi_printf("  Timer.parm2: 0x%llx\n", ptimer->parm2);
  osapi_printf("  Timer.timer_running: %d\n", ptimer->timer_running);
  osapi_printf("  Timer.timer_in_use: %d\n", ptimer->timer_in_use);
  osapi_printf("  Timer.time_count: %d\n", ptimer->time_count);
  osapi_printf("  Timer.orig_count: %d\n", ptimer->orig_count);
  osapi_printf("  next: %p\n", Entry->next);
  osapi_printf("  prev: %p\n", Entry->prev);
}

/* Be careful with running this on switches in production environment 
 *  as it can lock out the osapiTimer task for a long period of time. 
 *  Use the osapiDebugTimerDetailShow instead.
 */
void osapiPrintTimerList(int type, int detail)
{
  int i = 0;

  OSAPI_TIMER_SYNC_SEM_TAKE;

  if (type == 0)
  {
    osapiTimerTmp = osapiTimerFreeListHead;
  }
  else if (type == 1)
  {
    osapiTimerTmp = osapiTimerList;
  }

  while (osapiTimerTmp != NULL)
  {
    if (detail == 0)
    {
      osapi_printf("Timer %d, %p, running: %d, next: %p, prev: %p\n",
                   i, osapiTimerTmp,
                   osapiTimerTmp->timer.timer_running,
                   osapiTimerTmp->next,
                   osapiTimerTmp->prev); 
    }
    else
    {
      osapi_printf("Timer %d, %p:\n", i, osapiTimerTmp);
      osapiPrintTimerDetail(&(osapiTimerTmp->timer));
    }

    i++;
    osapiTimerTmp = osapiTimerTmp->next;
  }

  OSAPI_TIMER_SYNC_SEM_GIVE;

  return;
}

void osapiPrintOrphanTimers(int type, int detail, int start, int end)
{
  int i;

  OSAPI_TIMER_SYNC_SEM_TAKE;

  if (start < 0)
  {
    start = 0;
  }

  if ((end <= 0) || (end > OSAPI_MAX_TIMERS))
  {
    end = OSAPI_MAX_TIMERS;
  }

  if (start > end)
  {
    start = end;
  }

  for (i=start; i<end; i++)
  {
    if (type == 0)
    {
      if (osapiTimerListOrig[i].timer.timer_in_use == 1 &&
          osapiTimerListOrig[i].next == NULL &&
          osapiTimerListOrig[i].prev == NULL)
      {
        if (detail == 0)
        {
          osapi_printf("Timer %d, %p, running: %d, next: %p, prev: %p\n",
                       i, &(osapiTimerListOrig[i]),
                       osapiTimerListOrig[i].timer.timer_running,
                       osapiTimerListOrig[i].next,
                       osapiTimerListOrig[i].prev);
        }
        else
        {
          osapi_printf("Timer %d, %p:\n", i, &(osapiTimerListOrig[i]));
          osapiPrintTimerDetail(&(osapiTimerListOrig[i].timer));
        }
      }
    }
    else if (type == 1)
    {
      if (detail == 0)
      {
        osapi_printf("Timer %d, %p orphaned, running = %d\n", i, &(osapiTimerListOrig[i]),
                     osapiTimerListOrig[i].timer.timer_running);
      }
      else
      {
        osapi_printf("Timer %d, %p:\n", i, &(osapiTimerListOrig[i]));
        osapiPrintTimerDetail(&(osapiTimerListOrig[i].timer));
      }
    }
  }

  OSAPI_TIMER_SYNC_SEM_GIVE;
}


void osapiDebugTimerTmpListFree(uint32 lastIndex)
{
  uint32 index;
  for (index = 0; index < lastIndex; index++)
  {
    osapiFree( OSAPI_COMPONENT_ID, osapiDebugTimerDetail[index]);    
  }
  return;
}

RC_t osapiDebugTimerTmpListAllocate()
{
  uint32 index;

  for (index = 0; index < OSAPI_MAX_TIMERS; index++)
  {
    osapiDebugTimerDetail[index] = osapiMalloc( OSAPI_COMPONENT_ID, 
                                               sizeof(*osapiDebugTimerDetail[index]));     
    if (osapiDebugTimerDetail[index] ==  NULLPTR)
    {
      osapiDebugTimerTmpListFree(index);
      return  FAILURE;
    }
  }

  return  SUCCESS;
}


/* Prints timer stats without taking the timer_sync_sem. Useful to run in production switches.*/
void osapiDebugTimerStats()
{
  osapi_printf("Number of active timers = %u\n", osapiDebugTimerActiveCount);
  osapi_printf("Number of failed timer adds = %u\n", osapiDebugTimerFailAddCount);
}

/* Shows the details of active timers in non-intrusive way. Useful to run in production switches. */
/* type = 0 for active timers */
/* type = 1 for all timers */
/* non-zero for timers with non-zero execution time */
void osapiDebugTimerDetailShow(int type, int non_zero)
{
  uint32 timerCount = 0, index = 0, offset;
   char8  tmpBuf[128], printBuf[512], nameBuf[30];
  RC_t   rc;
  void *timer_callback;
 
  if (osapiDebugTimerTmpListAllocate() ==  FAILURE)
  {
    osapi_printf("Failed to allocate memory for timer list\n");
    return;
  }

  
  /* Copy the timer information to the temporary list */  
  OSAPI_TIMER_SYNC_SEM_TAKE;

  if (type == 1)
  {
    osapiTimerTmp = osapiTimerFreeListHead;
  }
  else
  {
    osapiTimerTmp = osapiTimerList;
  }

  while ((osapiTimerTmp != NULL) && (timerCount < OSAPI_MAX_TIMERS))
  {
    memcpy(osapiDebugTimerDetail[timerCount], &(osapiTimerTmp->timer), 
           sizeof(osapiTimerDescr_t));      
    timerCount++;       
    osapiTimerTmp = osapiTimerTmp->next;
  }

  osapiDebugTimerStats();

  OSAPI_TIMER_SYNC_SEM_GIVE;

  
  printf("Timer Callback   FuncName                         Parm1      Parm2       State      Req Time(ms) Time Left(ms) Execution Time(ms)\n");
  printf("----- ---------- ------------------------------ ---------  ----------  ----------  ------------- ------------  -----------------\n");
  for (index = 0; index < timerCount; index++)
  {
    if (non_zero)
    {
      if (osapiDebugTimerDetail[index]->execution_time == 0)
      {
        continue;
      }
    }
    memset(printBuf, 0x0, sizeof(printBuf));
    memset(nameBuf, 0x0, sizeof(nameBuf));

    if (osapiDebugTimerDetail[index]->callback32) 
    {
      rc = osapiFunctionLookup(osapiDebugTimerDetail[index]->callback32, 
                               nameBuf, sizeof(nameBuf), &offset);
      timer_callback = osapiDebugTimerDetail[index]->callback32;
    } else
    {
      rc = osapiFunctionLookup(osapiDebugTimerDetail[index]->callback64, 
                               nameBuf, sizeof(nameBuf), &offset);
      timer_callback = osapiDebugTimerDetail[index]->callback64;
    }
    osapiSnprintf(tmpBuf, sizeof(tmpBuf), 
                  "%-4d  0x%08lx %-30s 0x%08llx 0x%08llx   ",
                  index,
                  (unsigned long)timer_callback,
                  (rc ==  SUCCESS)? nameBuf: "TBD",
                  osapiDebugTimerDetail[index]->parm1,
                  osapiDebugTimerDetail[index]->parm2);
    osapiStrncat(printBuf, tmpBuf, sizeof(printBuf) - 1);
    if (osapiDebugTimerDetail[index]->timer_in_use ==  TRUE)
    {
      osapiSnprintf(tmpBuf, sizeof(tmpBuf),"Used/");
    }
    else
    {
      osapiSnprintf(tmpBuf, sizeof(tmpBuf),"Un-Used/");
    }

    osapiStrncat(printBuf, tmpBuf, sizeof(printBuf) - 1);
    if (osapiDebugTimerDetail[index]->timer_running ==  TRUE)
    {
      osapiSnprintf(tmpBuf, sizeof(tmpBuf), "Run");
    }
    else
    {
      osapiSnprintf(tmpBuf, sizeof(tmpBuf), "Stopped");
    }

    osapiStrncat(printBuf, tmpBuf, sizeof(printBuf) - 1);
    osapiSnprintf(tmpBuf, sizeof(tmpBuf),"    %-10d   %-10d   %-10d\n", 
                  osapiDebugTimerDetail[index]->orig_count, 
                  osapiDebugTimerDetail[index]->time_count,
                  osapiDebugTimerDetail[index]->execution_time);

    osapiStrncat(printBuf, tmpBuf, sizeof(printBuf) - 1);

    osapi_printf("%s", printBuf);
    
  }

  osapiDebugTimerTmpListFree(OSAPI_MAX_TIMERS);
  
  return;
}

/* Debug Functions */
osapiTimerDescr_t *pDebugTimerHolder, *pDebugTimer;
void osapiDebugTimerfn(uint32 parm1, uint32 T1)
{
  uint32 T2 = osapiTimeMillisecondsGet();

  if ( (parm1+T1) > T2)
    osapi_printf("\n%d millisecond timer expired %d milliseconds early\n", ( int32)parm1, ( int32)((parm1+T1) - T2));
  else if ( (parm1+T1) < T2)
    osapi_printf("\n%d millisecond timer expired %d milliseconds late\n", ( int32)parm1, ( int32)(T2 - (parm1+T1)));
  else
    osapi_printf("\n%d millisecond timer expired on time\n",( int32)parm1);

}

void osapiTimerTest(int noOfTimers, uint32 milliseconds)
{
  int i;
  for (i = milliseconds; i < (milliseconds*noOfTimers); i += milliseconds)
  {
    osapiTimerAdd(osapiDebugTimerfn, (uint32)i, osapiTimeMillisecondsGet(), i, &pDebugTimerHolder);
  }
}

/*********************************************************************
* @purpose  Debug API to enable the prints for timer callbacks
*           consuming more than the nominal time. 
*
* @param    enable  @b{(input)} flag
* @notes    Use this API very cautiously. It will increase the CPU load
*           due to overhead for calculating and displaying Timer
*           Callback Execution duration.

*********************************************************************/
void osapiDebugTimerCallbackDetailEnable (uint32 enable)
{
  osapiDebugTimerCallbackDetailEnableFlag = enable;
}
