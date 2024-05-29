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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "datatypes.h"
#include "commdefs.h"
#include "osapi.h"
#include "osapi_priv.h"
#include "osapi_sem.h"

#undef LOG_ERROR
#define LOG_ERROR(errno) printf("\nERROR: %s\n", strerror(errno))

static void *syncSemaArray[ TASK_SYNC_LAST] = { NULL};
pthread_key_t osapi_task_key;
int osapi_task_key_created;

typedef unsigned int (*proc_osapi_task_entry_t) (unsigned int, void *);

typedef struct
{
  pthread_t thread;
  pid_t PID;
  uint32 argc;
  void *argv;
  void * task_data;
  proc_osapi_task_entry_t *entry;
} proc_osapi_task_t;


void *proc_osapi_task_wrapper(void *arg)
{
  proc_osapi_task_t *task = (proc_osapi_task_t *)arg;

  if (0 == task)
  {
    return(0);
  }

  if (task->task_data)
  {
    pthread_setspecific(osapi_task_key, task->task_data);
  }

  if (0xffffffff == task->argc)
  {
    (*((void (*)(void *))(task->entry)))(task->argv);
  }
  else
  {
    (*((void (*)(int, void *))(task->entry)))(task->argc, task->argv);
  }
  return(0);
}

/**************************************************************************
*
* @purpose  Create a task.
*
* @param    task_name   @b{(input)} name to give the task.
* @param    task_entry  @b{(input)} function pointer to begin the task.
* @param    argc        @b{(input)} number of arguments.
* @param    argv        @b{(input)} pointer to start of list for arguments.
* @param    stack_size  @b{(input)} number of bytes for the stack.
* @param    priority    @b{(input)} task priority.
* @param    time_slice  @b{(input)} flag to allow time slicing for the task.
*
* @returns  task ID or 0.
*
* @comments    none.
*
* @end
*
*************************************************************************/
void * osapiTaskCreate(char *task_name,
                       void *task_entry,
                       unsigned int argc,
                       void *argv,
                       unsigned int stack_size,
                       unsigned int priority,
                       unsigned int time_slice)
{
  proc_osapi_task_t *task = 0;
  void * task_id = 0;
  int errno;
  void * task_data = 0;

  if (!osapi_task_key_created)
  {
    pthread_key_create(&osapi_task_key, NULL);
    osapi_task_key_created = 1;
  }

  task = (void*) malloc(sizeof(proc_osapi_task_t));

  /* for backward compatibility with non-proc osapi */
  task_data = (void*) calloc(1, sizeof(osapi_task_t)); 

  task_id =  task;

  if (0 != task_id)
  {
    task->argc = argc;
    task->argv = argv;
    task->entry = task_entry;
    task->task_data = task_data;

    errno = pthread_create(&(task->thread), 0,
                           (void * (*)(void *))proc_osapi_task_wrapper, (void *)task);
    if (0 != errno)
    {
      LOG_ERROR(errno);
      return 0;
    }
  }
  return task_id;
}

/**************************************************************************
*
* @purpose  Delete a task.
*
* @param    task_id @b{(input)}  handle for the task to be deleted.
*
* @returns  none.
*
* @comments    none.
*
* @end
*
*************************************************************************/
void osapiTaskDelete(void *task_id)
{
  int errno;
  pthread_t tid;
  proc_osapi_task_t *task;

  if (0 == task_id)
  {
    return;
  }

  task = (proc_osapi_task_t *)task_id;
  tid = task->thread;

  if (task->argv)
  {
    free (task->argv);
  }

  if(task->task_data)
  {
    free(task->task_data);
  }
  free(task);

  if (pthread_self() == tid)
  {
    pthread_exit(0);
  }
  else
  {
    errno = pthread_cancel(tid);
  }

  if(0 != errno)
  {
    LOG_ERROR(errno);
  }
  return;
}

/**************************************************************************
*
* @purpose  Delay a task for the number of ticks. Ticks are usually 1/60th of a second
*
* @param    ticks       number of clock ticks to delay
*
* @returns   SUCCESS
* @returns   FAILURE  if called from interrupt level or if the calling task
*                       receives a signal that is not blocked or ignored.
*
* @comments    none.
*
* @end
*
*************************************************************************/
RC_t osapiTaskDelay( int32 ticks)
{

  struct timespec timereq, timerem;
   int32 usec;

  /* don't really need to check for "interrupt context" */

  usec = ticks * OSAPI_TICK_USEC;

  timereq.tv_sec  = ((usec >= 1000000) ? (usec / 1000000) : 0);
  timereq.tv_nsec = ((usec >= 1000000) ? ((usec % 1000000) * 1000)
                     : (usec * 1000));

  if (usec > 0)
  {
    for (;;)
    {
      pthread_testcancel();

      if (nanosleep(&timereq, &timerem) != 0)
      {
        timereq = timerem;
      }
      else
      {
        break;
      }
    }

  }

  return( SUCCESS);
}

/**************************************************************************
*
* @purpose  Signals to a waiting task that this task has completed initialization
*
* @param    syncHandle  handle of the giving task
*
* @returns   SUCCESS if a task is waiting on the syncHandle
* @returns   FAILURE if a task is not waiting on the syncHandle
*
* @comments    none.
*
* @end
*
*************************************************************************/

RC_t osapiTaskInitDone(uint32 syncHandle)
{
  void *syncSema;

  /* Ensure waiting task has set up sync semaphore */
  while((syncSema = ( void * )syncSemaArray[syncHandle]) ==  NULL)
  {
    osapiTaskDelay(1);
  }

  /*
  * Tell waiting task that I am done and can continue...
  */
  (void) osapiSemaGive( syncSema );
  return(  SUCCESS );

}

/**************************************************************************
*
* @purpose  Waits for a task to complete initializaiton
*
* @param    synchandle  handle of task to wait for
* @param    timeout     timeout in milliseconds to wait for task
*
* @returns   SUCCESS  task is initialized, continue normal
* @returns   FAILURE  task is not init or could not sync up
*
* @comments    none.
*
* @end
*
*************************************************************************/
RC_t osapiWaitForTaskInit( uint32 syncHandle,  int32 timeout )
{
  RC_t rc =  FAILURE;

  /* Get a Binary Semaphore for synchronization with a task */
  void *syncSema;

  syncSema = osapiSemaBCreate(OSAPI_SEM_Q_FIFO, OSAPI_SEM_EMPTY);

  if (syncSema ==  NULL)
  {
    LOG_ERROR(0);
    return rc;
  }

  syncSemaArray[syncHandle] = syncSema;

  /* Take the semaphore and wait for the task to give it back */
  if ( osapiSemaTake(syncSema, timeout) !=  ERROR )
  {
    rc =  SUCCESS;
  }

  osapiSemaDelete( syncSema );

  syncSemaArray[syncHandle] =  NULL;

  return(rc);
}

/**************************************************************************
*
* @purpose  Get the task ID of a running task.
*
* @param    void
*
* @returns  task_id
*
* @comments    none.
*
* @end
*
*************************************************************************/
void * osapiTaskIdSelf( void)
{
  return ((void *) pthread_getspecific(osapi_task_key));
}

