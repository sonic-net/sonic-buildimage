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



#include <string.h>
#include <stdlib.h>

#include "datatypes.h"
#include "commdefs.h"
#include "osapi_priv.h"
#include "osapi_sem.h"
#include "osapi.h"

#define OSAPI_USE_OS_SEM
#ifdef OSAPI_USE_OS_SEM
#include <errno.h>

typedef struct os_sem_s 
{
  sem_t sem;
  pthread_mutex_t mutex;
   BOOL sem_is_mutex;
} os_sem_t;
#endif /* OSAPI_USE_OS_SEM */

#define OSAPI_SEM_HISTORY_SIZE 8


#define SEM_DELETED 0x00000001
#define SEM_FLUSHED 0x00000002
#define SEM_BINARY  0x00000004
#define SEM_COUNTING    0x00000008
#define SEM_MUTEX   0x00000010
#define SEM_Q_FIFO  0x00000020
#define SEM_Q_PRIO  0x00000040
#define SEM_DEL_SAFE    0x00000080
#define SEM_INV_SAFE    0x00000100

pthread_mutex_t sem_list_lock = PTHREAD_MUTEX_INITIALIZER;
osapi_sem_t *sem_list_head = NULL;

#if defined ( STACKING_PACKAGE)
extern char *atp_dump_filename;
extern void osapiLogSingleStackTrace( uchar8 *fileName, uint32 pid);
extern RC_t hapiBroadDrivPrintFileStart( uchar8 *fileName,  BOOL appendToFile);
extern void hapiBroadDrivPrintFileStop();
extern  BOOL atpDebug;
#endif

#ifndef OSAPI_USE_OS_SEM
static void osapiSemaHistoryRecord (osapi_sem_t * sem,  BOOL give,
                                    void *caller)
{
  osapiSemHistory_t *ent;

  sem->historyIndex++;

  sem->historyIndex %= OSAPI_SEM_HISTORY_SIZE;

  ent = &sem->history[sem->historyIndex];

  ent->caller = caller;

  ent->time_stamp = osapiUpTimeMillisecondsGet();

  if ( TRUE == give)
  {
    ent->give = 1;
    sem->holdTime = 0; 
    sem->blocked = 0;
  } 
  else
  {
    ent->give = 0;
    /*
     * Record the last take time. We do not record the time a task waits for a semaphore.
     * Most tasks use semaphores for event signaling and they dont count as semaphores
     * that are potentially blocked out.
     */
    sem->holdTime = ent->time_stamp;
  }
}
#endif /* !OSAPI_USE_OS_SEM */

 BOOL osapiSemaIsUsed (osapi_sem_t * sem)
{
  if (sem->history[sem->historyIndex].caller !=  NULLPTR)
  {
    return  TRUE;
  }
  return  FALSE;
}

#ifndef OSAPI_USE_OS_SEM
static uint32 osapiSemaNameSizeCompute ( char8 * name,  int32 inst)
{
  if (inst != 0)
  {
    return strlen (name) + 9;
  }
  return strlen (name) + 1;
}

static void osapiSemaNameSet (osapi_sem_t * sem,  char8 * name,  int32 inst,
                              uint32 len)
{
  if (inst != 0)
  {
    osapiSnprintf (sem->name, len, "%s:%d", name, inst);
  }
  else
  {
    osapiStrncpySafe (sem->name, name, len);
  }
}
#endif /* !OSAPI_USE_OS_SEM */

/**************************************************************************
 *
 * @purpose  Create a binary Semaphore
 *
 * @param    name @b{(input)}           name of the semaphore
 * @param    inst @b{(input)}           optional instance of semaphore
 *                                      value = 0 to ignore
 * @param    options @b{(input)}        queueing style - either Priority style or FIFO style
 * @param    initialState @b{(input)}   FULL(available) or EMPTY (not
   available)
 *
 * @returns  ptr to the semaphore or NULL if memory cannot be allocated
 *
 * @comments    none.
 *
 * @end
 *
 *************************************************************************/
void *osapiSemaBCreateTrack ( char8 * name,  int32 inst,  int32 options,
                             OSAPI_SEM_B_STATE initialState)
{
#ifdef OSAPI_USE_OS_SEM
  os_sem_t  *sema;
  unsigned int count;
  int rv;

  do 
  {
    sema = osapiMalloc ( OSAPI_COMPONENT_ID, sizeof (os_sem_t));
    if (0 == sema)
    {
      break;
    }
    sema->sem_is_mutex =  FALSE;
    if (initialState == OSAPI_SEM_EMPTY)
    {
      count = 0;
    } else
    { 
      count = 1;
    }
    rv = sem_init (&sema->sem, 0, count);
    if (rv < 0)
    {
      osapiFree ( OSAPI_COMPONENT_ID, sema);
      sema = 0;
      break; 
    }
   
  } while (0);

  return sema;
#else
  uint32 q_options;
  osapi_sem_t *newSem;
  uint32 len;

  len = osapiSemaNameSizeCompute (name, inst);
  newSem = osapiMalloc ( OSAPI_COMPONENT_ID, sizeof (*newSem) + len + 1);
  if (newSem == NULL)
  {
    return newSem;
  }

  pthread_mutex_init (&(newSem->lock), (pthread_mutexattr_t *) NULL);

  /* Do the options remapping */
  if ((options & OSAPI_SEM_Q_FIFO) != 0)
  {
    newSem->flags |= SEM_Q_FIFO;
    q_options = WAITQ_FIFO;
  }
  else                          /* if ((options & OSAPI_SEM_Q_PRIORITY) != 0) */
  {
    newSem->flags |= SEM_Q_PRIO;
    q_options = WAITQ_PRIO;
  }
  if ((options & OSAPI_SEM_DELETE_SAFE) != 0)
  {
    /* invalid option */
    osapiFree ( OSAPI_COMPONENT_ID, newSem);
    return NULL;
  }
  if ((options & OSAPI_SEM_INVERSION_SAFE) != 0)
  {
    /* invalid option */
    osapiFree ( OSAPI_COMPONENT_ID, newSem);
    return NULL;
  }
  newSem->flags |= SEM_BINARY;

  if(initialState == OSAPI_SEM_EMPTY)
  {
    /* osapiSemaHistoryRecord (newSem,  FALSE, __FP_CALLER__); */
    newSem->init_count = 0;
  }
  else
  {
    /* osapiSemaHistoryRecord (newSem,  TRUE, __FP_CALLER__); */
    newSem->init_count = 1;
  }
  
  newSem->cur_count = newSem->init_count;
  newSem->depth = 0;
  newSem->blocked = 0;

  newSem->owner = (osapi_task_t *) NULL;
  newSem->last_owner = (osapi_task_t *) NULL;
  newSem->num_waiting = 0;
  osapiSemaNameSet (newSem, name, inst, len);

  osapi_waitq_create (&(newSem->queue), &(newSem->lock), q_options);

  pthread_mutex_lock (&sem_list_lock);

  if (sem_list_head != NULL)
  {
    sem_list_head->chain_prev = newSem;
  }
  newSem->chain_next = sem_list_head;
  sem_list_head = newSem;
  newSem->chain_prev = NULL;

  pthread_mutex_unlock (&sem_list_lock);

  return (void *) newSem;
#endif /* OSAPI_USE_OS_SEM */
}

/**************************************************************************
 *
 * @purpose  create a Counting Semaphore
 *
 * @param    name @b{(input)}           name of the semaphore
 * @param    inst @b{(input)}           optional instance of semaphore
 *                                      value = 0 to ignore
 * @param    options @b{(input)}        queueing style
 *                                      Priority style or FIFO style
 * @param    initialCount @b{(input)}   initialized to the specified initial count
 *
 * @returns  ptr to the semaphore or NULL if memory cannot be allocated
 *
 * @comments    none.
 *
 * @end
 *
 *************************************************************************/
void *osapiSemaCCreateTrack ( char8 * name,  int32 inst,  int32 options,
                              int32 initialCount)
{
#ifdef OSAPI_USE_OS_SEM
  os_sem_t  *sema;
  unsigned int count;
  int rv;

  do 
  {
    sema = osapiMalloc ( OSAPI_COMPONENT_ID, sizeof (os_sem_t));
    if (0 == sema)
    {
      break;
    }
    sema->sem_is_mutex =  FALSE;
    count = initialCount;

    rv = sem_init (&sema->sem, 0, count);
    if (rv < 0)
    {
      osapiFree ( OSAPI_COMPONENT_ID, sema);
      sema = 0;
      break; 
    }
   
  } while (0);

  return sema;
#else
  uint32 q_options;
  osapi_sem_t *newSem;
  uint32 len;

  len = osapiSemaNameSizeCompute (name, inst);
  newSem = osapiMalloc ( OSAPI_COMPONENT_ID, sizeof (*newSem) + len + 1);
  if (newSem == NULL)
  {
    return newSem;
  }

  pthread_mutex_init (&(newSem->lock), (pthread_mutexattr_t *) NULL);

  /* Do the options remapping */
  if ((options & OSAPI_SEM_Q_FIFO) != 0)
  {
    newSem->flags |= SEM_Q_FIFO;
    q_options = WAITQ_FIFO;
  }
  else                          /* if ((options & OSAPI_SEM_Q_PRIORITY) != 0) */
  {
    newSem->flags |= SEM_Q_PRIO;
    q_options = WAITQ_PRIO;
  }
  if ((options & OSAPI_SEM_DELETE_SAFE) != 0)
  {
    /* invalid option */
    osapiFree ( OSAPI_COMPONENT_ID, newSem);
    return NULL;
  }
  if ((options & OSAPI_SEM_INVERSION_SAFE) != 0)
  {
    /* invalid option */
    osapiFree ( OSAPI_COMPONENT_ID, newSem);
    return NULL;
  }

  newSem->flags |= SEM_COUNTING;

  newSem->init_count = initialCount;
  newSem->cur_count = newSem->init_count;
  newSem->depth = 0;
  newSem->blocked = 0;

  newSem->owner = (osapi_task_t *) NULL;
  newSem->last_owner = (osapi_task_t *) NULL;
  newSem->num_waiting = 0;
  osapiSemaNameSet (newSem, name, inst, len);

  osapi_waitq_create (&(newSem->queue), &(newSem->lock), q_options);

  pthread_mutex_lock (&sem_list_lock);

  if (sem_list_head != NULL)
  {
    sem_list_head->chain_prev = newSem;
  }
  newSem->chain_next = sem_list_head;
  sem_list_head = newSem;
  newSem->chain_prev = NULL;

  pthread_mutex_unlock (&sem_list_lock);

  return (void *) newSem;
#endif /* OSAPI_USE_OS_SEM */
}

/**************************************************************************
 *
 * @purpose  create a Mutual Exclusion Semaphore
 *
 * @param    name @b{(input)}           name of the semaphore
 * @param    inst @b{(input)}           optional instance of semaphore
 *                                      value = 0 to ignore
 * @param    options @b{(input)}        queueing style
 *                                      Priority style or FIFO style
 *
 * @returns  ptr to the semaphore or NULL if memory cannot be allocated
 *
 * @comments    none.
 *
 * @end
 *
 *************************************************************************/
void *osapiSemaMCreateTrack ( char8 * name,  int32 inst,  int32 options)
{
#ifdef OSAPI_USE_OS_SEM
  os_sem_t  *sema;
  pthread_mutexattr_t attr;
  int rv;

  do 
  {
    sema = osapiMalloc ( OSAPI_COMPONENT_ID, sizeof (os_sem_t));
    if (0 == sema)
    {
      break;
    }
    sema->sem_is_mutex =  TRUE;

    pthread_mutexattr_init (&attr);
    pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);

    rv = pthread_mutex_init (&sema->mutex, &attr);
    if (rv != 0)
{
      osapiFree ( OSAPI_COMPONENT_ID, sema);
      sema = 0;
      break; 
    }
   
  } while (0);

  return sema;
#else
  uint32 q_options;
  osapi_sem_t *newSem;
  uint32 len;

  len = osapiSemaNameSizeCompute (name, inst);
  newSem = osapiMalloc ( OSAPI_COMPONENT_ID, sizeof (*newSem) + len + 1);
  if (newSem == NULL)
  {
    return newSem;
  }

  pthread_mutex_init (&(newSem->lock), (pthread_mutexattr_t *) NULL);

  /* Do the options remapping */
  if ((options & OSAPI_SEM_Q_FIFO) != 0)
  {
    newSem->flags |= SEM_Q_FIFO;
    q_options = WAITQ_FIFO;
  }
  else                          /* if ((options & OSAPI_SEM_Q_PRIORITY) != 0) */
  {
    newSem->flags |= SEM_Q_PRIO;
    q_options = WAITQ_PRIO;
  }
  if ((options & OSAPI_SEM_DELETE_SAFE) != 0)
  {
    newSem->flags |= SEM_DEL_SAFE;
  }
  if ((options & OSAPI_SEM_INVERSION_SAFE) != 0)
  {
    newSem->flags |= SEM_INV_SAFE;
  }

  newSem->flags |= SEM_MUTEX;

  newSem->init_count = 1;
  newSem->cur_count = newSem->init_count;
  newSem->depth = 0;
  newSem->blocked = 0;


  newSem->owner = (osapi_task_t *) NULL;
  newSem->last_owner = (osapi_task_t *) NULL;
  newSem->num_waiting = 0;
  osapiSemaNameSet (newSem, name, inst, len);

  osapi_waitq_create (&(newSem->queue), &(newSem->lock), q_options);

  pthread_mutex_lock (&sem_list_lock);

  if (sem_list_head != NULL)
  {
    sem_list_head->chain_prev = newSem;
  }
  newSem->chain_next = sem_list_head;
  sem_list_head = newSem;
  newSem->chain_prev = NULL;

  pthread_mutex_unlock (&sem_list_lock);

  return (void *) newSem;
#endif /* OSAPI_USE_OS_SEM */
}

/**************************************************************************
 *
 * @purpose  Delete a Semaphore
 *
 * @param    SemID @b{(input)}   ID of the semaphore to delete
 *
 * @returns  OK, or error semaphore is invalid
 *
 * @comments    none.
 *
 * @end
 *
 *************************************************************************/
RC_t osapiSemaDelete (void *sem)
{
#ifdef OSAPI_USE_OS_SEM
  os_sem_t  *sema = sem;
  int rv;

  if ( TRUE == sema->sem_is_mutex)
  {
    rv = pthread_mutex_destroy (&sema->mutex);
  } else
  {
    rv = sem_destroy (&sema->sem);
  }
  if (rv != 0)
{
    return  FAILURE;
  }
  return  SUCCESS;
#else
  osapi_sem_t *osapiSem = (osapi_sem_t *) sem;

  pthread_mutex_lock (&(osapiSem->lock));

  /* need to account for blocked tasks */
  /*  -- mark semaphore as deleted     */
  /*  -- mark semaphore as flushed     */
  /*  -- flush wait queue              */
  /*  -- delete wait queue             */

  osapiSem->flags |= (SEM_DELETED | SEM_FLUSHED);

  osapi_waitq_flush (&(osapiSem->queue));

  osapi_waitq_destroy (&(osapiSem->queue));

  pthread_mutex_unlock (&(osapiSem->lock));

  pthread_mutex_destroy (&(osapiSem->lock));

  pthread_mutex_lock (&sem_list_lock);

  if (osapiSem->chain_next != NULL)
  {
    osapiSem->chain_next->chain_prev = osapiSem->chain_prev;
  }

  if (osapiSem->chain_prev != NULL)
  {
    osapiSem->chain_prev->chain_next = osapiSem->chain_next;
  }
  else
    /* must be head of list */
  {
    sem_list_head = osapiSem->chain_next;
  }

  pthread_mutex_unlock (&sem_list_lock);

  osapiFree ( OSAPI_COMPONENT_ID, osapiSem);

  return  SUCCESS;
#endif /* OSAPI_USE_OS_SEM */
}

/**************************************************************************
 *
 * @purpose  Unblock any and all tasks waiting on the semaphore
 *
 * @param    SemID @b{(input)}   ID of the semaphore
 *
 * @returns  OK, or error semaphore is invalid
 *
 * @comments    none.
 *
 * @end
 *
 *************************************************************************/
RC_t osapiSemaFlush (void *sem)
{
  osapi_sem_t *osapiSem = (osapi_sem_t *) sem;

  pthread_mutex_lock (&(osapiSem->lock));

  /* need to account for blocked tasks */
  /*  -- mark semaphore as flushed     */
  /*  -- flush wait queue              */
  /*  -- unmark semaphore as flushed   */

  osapiSem->flags |= SEM_FLUSHED;

  osapi_waitq_flush (&(osapiSem->queue));

  osapiSem->num_waiting = 0;
  osapiSem->flags &= ~SEM_FLUSHED;

  pthread_mutex_unlock (&(osapiSem->lock));

  return  SUCCESS;
}

/**************************************************************************
 *
 * @purpose  Give a Semaphore
 *
 * @param    SemID @b{(input)}   ID of the semaphore to release
 *
 * @returns  OK, or error semaphore is invalid
 *
 * @comments    none.
 *
 * @end
 *
 *************************************************************************/
RC_t osapiSemaGive (void *sem)
{
#ifdef OSAPI_USE_OS_SEM
  os_sem_t  *sema;
  int rv;

  if (0 == sem)
  {
    return  FAILURE;
  }

  sema = sem;
  if ( TRUE == sema->sem_is_mutex)
  {
    rv = pthread_mutex_unlock (&sema->mutex);
  } else
  {
    rv = sem_post (&sema->sem);
  }

  if (rv != 0)
{
    return  FAILURE;
  }

  return  SUCCESS;
#else
  RC_t rc =  SUCCESS;
  osapi_sem_t *osapiSem = (osapi_sem_t *) sem;

  if (osapiSem ==  NULL)
  {
    return  FAILURE;
  }

  pthread_mutex_lock (&(osapiSem->lock));

  if ((osapiSem->flags & (SEM_DELETED | SEM_FLUSHED)) == 0)
  {
    if (osapiSem->depth == 0)
    {
      if ((((osapiSem->flags & SEM_COUNTING) != 0)
           || (osapiSem->cur_count != 1))
          && (((osapiSem->flags & SEM_MUTEX) == 0)
              || (osapiSem->owner == (void *) osapiTaskIdSelf ())))
      {
        if (++osapiSem->cur_count == 1)
        {
          osapiSem->last_owner = (void *) osapiTaskIdSelf ();
          osapi_waitq_dequeue (&(osapiSem->queue));
          osapiSemaHistoryRecord (osapiSem,  TRUE, __FP_CALLER__);
        }

        osapiSem->curr_owner = NULL;
        if ((osapiSem->flags & SEM_MUTEX) != 0)
        {
          osapiSem->owner = NULL;

          if ((osapiSem->flags & SEM_DEL_SAFE) != 0)
          {
            pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
          }
        }
      }
      else
      {
        rc =  FAILURE;
      }
    }
    else
    {
      if (((osapiSem->flags & SEM_MUTEX) == 0)
          || (osapiSem->owner != (void *) osapiTaskIdSelf ()))
      {
        rc =  FAILURE;
      }
      else
      {
        osapiSem->depth--;
      }
    }
  }
  else
  {
    rc =  FAILURE;
  }

  pthread_mutex_unlock (&(osapiSem->lock));

  return rc;
#endif /* OSAPI_USE_OS_SEM */
}

#ifndef OSAPI_USE_OS_SEM
/*********************************************************************
* @purpose Callback to check if the semaphore can be dequeued
*
* @param sem @b{(input)}   ID of the requested semaphore
*
* @notes none
*
* @end
*********************************************************************/
static int osapi_sem_waitq_remove_check (void *sem)
{
  osapi_sem_t *osapiSem = (osapi_sem_t *) sem;

  if (osapiSem->cur_count > 0)
  {
    if ((osapiSem->num_waiting < 2) ||
        (osapiSem->last_owner != (void *) osapiTaskIdSelf ()))
    {
      return WAITQ_REMOVE_OK;
    }
  }

  if ((osapiSem->flags & SEM_FLUSHED) != 0)
  {
    return WAITQ_REMOVE_OK;
  }

  return ~WAITQ_REMOVE_OK;
}
#endif /* !OSAPI_USE_OS_SEM */

/**************************************************************************
 *
 * @purpose  Take a Semaphore
 *
 * @param    SemID @b{(input)}   ID of the requested semaphore
 * @param    timeout @b{(input)}  time to wait in milliseconds, forever (-1), or no wait (0)
 *
 * @returns  OK, or error if timeout or if semaphore does not exist
 *
 * @comments    none.
 *
 * @end
 *
 *************************************************************************/
RC_t osapiSemaTake (void *sem,  int32 timeout)
{
#ifdef OSAPI_USE_OS_SEM
  os_sem_t  *sema;
  int rv;

  if (0 == sem)
  {
    return  FAILURE;
  }

  sema = sem;
  if ( TRUE == sema->sem_is_mutex)
  {
    if (timeout ==  WAIT_FOREVER)
    {
      rv = pthread_mutex_lock(&sema->mutex);
    } else if (timeout ==  NO_WAIT) 
    {
      rv = pthread_mutex_trylock(&sema->mutex);
    } else 
    {
      struct timespec wait;
      (void) clock_gettime (CLOCK_REALTIME, &wait);
      wait.tv_sec += timeout / 1000;
      wait.tv_nsec += (timeout % 1000) * 1000000;
      if (wait.tv_nsec >= 1000000000)
      {
        wait.tv_sec++;
        wait.tv_nsec -= 1000000000;
      }
      rv = pthread_mutex_timedlock (&sema->mutex, &wait);
    }
  } else
  {
    if (timeout ==  WAIT_FOREVER)
    {
      do 
      {
        rv = sem_wait (&sema->sem);
        if (rv == 0) break;
        if ((rv < 0) && (errno != EINTR)) break;
      } while (rv < 0);
    } else if (timeout ==  NO_WAIT) 
    {
      do 
      {
        rv = sem_trywait (&sema->sem);
        if (rv == 0) break;
        if ((rv < 0) && (errno != EINTR)) break;
      } while (rv < 0);
    } else 
    {
      struct timespec wait;
      (void) clock_gettime (CLOCK_REALTIME, &wait);
      wait.tv_sec += timeout / 1000;
      wait.tv_nsec += (timeout % 1000) * 1000000;
      if (wait.tv_nsec >= 1000000000)
      {
        wait.tv_sec++;
        wait.tv_nsec -= 1000000000;
      }
      do 
      {
        rv = sem_timedwait (&sema->sem, &wait);
        if (rv == 0) break;
        if ((rv < 0) && (errno != EINTR)) break;
      } while (rv < 0);
    }
  }
  if (rv != 0)
{
    return  FAILURE;
  }

  return  SUCCESS;
#else

  RC_t rc =  SUCCESS;
  osapi_sem_t *osapiSem = (osapi_sem_t *) sem;

  if (osapiSem ==  NULL)
  {
    return  FAILURE;
  }

  pthread_mutex_lock (&(osapiSem->lock));

  if ((osapiSem->flags & (SEM_DELETED | SEM_FLUSHED)) == 0)
  {
    if (((osapiSem->flags & SEM_MUTEX) == 0)
        || (osapiSem->owner != (void *) osapiTaskIdSelf ()))
    {
      while ((osapiSem->cur_count == 0) ||
             ((osapiSem->last_owner == (void *) osapiTaskIdSelf ()) &&
              (osapiSem->num_waiting != 0)))
      {
        osapiSem->num_waiting++;
        rc = osapi_waitq_enqueue (&(osapiSem->queue), timeout,
                                  osapi_sem_waitq_remove_check,
                                  (void *) osapiSem, __FP_CALLER__);
        osapiSem->num_waiting--;

        if ((rc !=  SUCCESS) || ((osapiSem->flags & SEM_FLUSHED) != 0))
        {
          rc =  FAILURE;
          break;
        }
      }
      osapiSemaHistoryRecord (osapiSem,  FALSE, __FP_CALLER__);

      if ((rc ==  SUCCESS) &&
          ((osapiSem->num_waiting == 0) ||
           (osapiSem->last_owner != (void *) osapiTaskIdSelf ())))
      {
        osapiSem->cur_count--;
        osapiSem->curr_owner = (void *) osapiTaskIdSelf ();
        osapiSem->last_owner = NULL;

        if ((osapiSem->flags & SEM_MUTEX) != 0)
        {
          osapiSem->owner = (void *) osapiTaskIdSelf ();

          if ((osapiSem->flags & SEM_DEL_SAFE) != 0)
          {
            pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, NULL);
          }
        }
      }
    }
    else
    {
      osapiSem->depth++;
    }
  }
  else
  {
    rc =  FAILURE;
  }

  pthread_mutex_unlock (&(osapiSem->lock));

  return rc;
#endif /* OSAPI_USE_OS_SEM */
}

/*********************************************************************
* @purpose  Initializes OSAPI Semaphore Library
*
* @returns   SUCCESS
*
* @returns none
*
* @end
*
*********************************************************************/
RC_t osapiSemaInit (void)
{
  return  SUCCESS;
}

/**************************************************************************
 *
 * @purpose  Print information about semaphores
 *
 * @param    showHistory  - Print history data of semaphore
 * @param    hideInactive - Print even if the semaphore is not used even once
 * @param    query - specific semaphore for printing information about
 * @param    only_blocked - Show only blocked tasks
 *
 * @returns  none
 *
 * @end
 *
 *************************************************************************/
#define OSAPI_SEM_HANG_TIME (10 * 1000)

