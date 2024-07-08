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



#include "datatypes.h"
#include "commdefs.h"

//#include "log.h"

#include "osapi.h"
#include "osapi_priv.h"

typedef struct osapi_rwlock_s
{
  volatile uint32 flags;
  pthread_mutex_t lock;
  volatile uint32 rcount;
  volatile uint32 wcount;
  osapi_waitq_t rqueue;
  osapi_waitq_t wqueue;
  struct osapi_rwlock_s *chain_prev, *chain_next;
} osapi_rwlock_t;

#define RWLOCK_DELETED     0x00000001
#define RWLOCK_W_PENDING   0x00000002
#define RWLOCK_Q_FIFO      0x00000004
#define RWLOCK_Q_PRIO      0x00000008

static pthread_mutex_t rwlock_list_lock = PTHREAD_MUTEX_INITIALIZER;
static osapi_rwlock_t *rwlock_list_head = NULL;

/**************************************************************************
 *
 * @purpose  Create a read/write lock
 *
 * @param    options @b{(input)} queueing style - either Priority or FIFO
 *
 * @returns  ptr to the lock or NULL if lock cannot be allocated
 *
 * @comments None.
 *
 * @end
 *
 *************************************************************************/
RC_t osapiRWLockCreate (osapiRWLock_t * rwlock, osapiRWLockOptions_t options)
{
  uint32 q_options;
  osapi_rwlock_t *newRWLock;

  newRWLock = (osapi_rwlock_t *) osapiMalloc ( OSAPI_COMPONENT_ID,
                                              sizeof (osapi_rwlock_t));
  if (newRWLock == NULL)
  {
    rwlock->handle = (void *) newRWLock;
    return  FAILURE;
  }

  pthread_mutex_init (&(newRWLock->lock), (pthread_mutexattr_t *) NULL);

  /* Do the options remapping */
  if (options == OSAPI_RWLOCK_Q_FIFO)
  {
    newRWLock->flags |= RWLOCK_Q_FIFO;
    q_options = WAITQ_FIFO;
  }
  else                          /* if (options == OSAPI_RWLOCK_Q_PRIORITY) */
  {
    newRWLock->flags |= RWLOCK_Q_PRIO;
    q_options = WAITQ_PRIO;
  }

  newRWLock->rcount = 0;
  newRWLock->wcount = 0;
  osapi_waitq_create (&(newRWLock->rqueue), &(newRWLock->lock), q_options);
  osapi_waitq_create (&(newRWLock->wqueue), &(newRWLock->lock), q_options);

  pthread_mutex_lock (&rwlock_list_lock);

  if (rwlock_list_head != NULL)
  {
    rwlock_list_head->chain_prev = newRWLock;
  }
  newRWLock->chain_next = rwlock_list_head;
  rwlock_list_head = newRWLock;
  newRWLock->chain_prev = NULL;

  pthread_mutex_unlock (&rwlock_list_lock);

  rwlock->handle = (void *) newRWLock;
  return  SUCCESS;
}

static int osapi_rwlock_r_waitq_remove_check (void *rwlock)
{
  osapi_rwlock_t *osapiRWLock = (osapi_rwlock_t *) rwlock;

  if (osapiRWLock->wcount == 0)
  {
    return WAITQ_REMOVE_OK;
  }

  if ((osapiRWLock->flags & RWLOCK_DELETED) != 0)
  {
    return WAITQ_REMOVE_OK;
  }

  return ~WAITQ_REMOVE_OK;
}

/**************************************************************************
 *
 * @purpose  Take a read lock
 *
 * @param    rwlock    ID of the requested lock returned from osapiRWLockCreate
 * @param    timeout   time to wait in milliseconds,  WAIT_FOREVER,
 *                     or  NO_WAIT
 *
 * @returns   SUCCESS
 * @returns   FAILURE if timeout or if lock does not exist
 *
 * @comments None.
 *
 * @end
 *
 *************************************************************************/
RC_t osapiReadLockTake (osapiRWLock_t rwlock,  int32 timeout)
{
  RC_t rc =  SUCCESS;
  osapi_rwlock_t *osapiRWLock = (osapi_rwlock_t *) rwlock.handle;

  /* Timeout is already in milliseconds, which is what
     osapi_waitq_enqueue() needs */

  pthread_mutex_lock (&(osapiRWLock->lock));

  if ((osapiRWLock->flags & RWLOCK_DELETED) == 0)
  {
    /* wait for writes to complete */
    while (osapiRWLock->wcount > 0)
    {
      rc = osapi_waitq_enqueue (&(osapiRWLock->rqueue), timeout,
                                osapi_rwlock_r_waitq_remove_check,
                                (void *) osapiRWLock, __FP_CALLER__);

      if ((rc !=  SUCCESS) || ((osapiRWLock->flags & RWLOCK_DELETED) != 0))
      {
        rc =  FAILURE;
        break;
      }
    }

    if (rc ==  SUCCESS)
    {
      osapiRWLock->rcount++;
    }
  }
  else
  {
    rc =  FAILURE;
  }

  pthread_mutex_unlock (&(osapiRWLock->lock));

  return rc;
}

/**************************************************************************
 *
 * @purpose  Give a read lock
 *
 * @param    rwlock     ID of the requested lock returned from osapiRWLockCreate
 *
 * @returns   SUCCESS
 * @returns   FAILURE if lock is invalid
 *
 * @comments None.
 *
 * @end
 *
 *************************************************************************/
RC_t osapiReadLockGive (osapiRWLock_t rwlock)
{
  RC_t rc =  SUCCESS;
  osapi_rwlock_t *osapiRWLock = (osapi_rwlock_t *) rwlock.handle;

  pthread_mutex_lock (&(osapiRWLock->lock));

  if (osapiRWLock->rcount == 0)
  {
    rc =  ERROR;
  }
  else
  {
    if (--osapiRWLock->rcount == 0)
    {
      osapi_waitq_dequeue (&(osapiRWLock->wqueue));
    }
  }
  pthread_mutex_unlock (&(osapiRWLock->lock));

  return rc;
}

static int osapi_rwlock_w_waitq_remove_check (void *rwlock)
{
  osapi_rwlock_t *osapiRWLock = (osapi_rwlock_t *) rwlock;

  if ((osapiRWLock->rcount == 0) && (osapiRWLock->wcount == 0))
  {
    return WAITQ_REMOVE_OK;
  }

  if ((osapiRWLock->flags & RWLOCK_DELETED) != 0)
  {
    return WAITQ_REMOVE_OK;
  }

  return ~WAITQ_REMOVE_OK;
}

/**************************************************************************
 *
 * @purpose  Take a write lock
 *
 * @param    rwlock    ID of the requested lock returned from osapiRWLockCreate
 * @param    timeout   time to wait in milliseconds,  WAIT_FOREVER,
 *                     or  NO_WAIT
 *
 * @returns   SUCCESS
 * @returns   FAILURE if timeout or if lock does not exist
 *
 * @comments None.
 *
 * @end
 *
 *************************************************************************/
RC_t osapiWriteLockTake (osapiRWLock_t rwlock,  int32 timeout)
{
  RC_t rc =  SUCCESS;
  osapi_rwlock_t *osapiRWLock = (osapi_rwlock_t *) rwlock.handle;

  /* Timeout is already in milliseconds, which is what
     osapi_waitq_enqueue() needs */

  pthread_mutex_lock (&(osapiRWLock->lock));

  if ((osapiRWLock->flags & RWLOCK_DELETED) == 0)
  {
    /* indicate pending write */
    osapiRWLock->flags |= RWLOCK_W_PENDING;

    /* wait for reads to complete */
    while ((osapiRWLock->rcount > 0) || (osapiRWLock->wcount > 0))
    {
      rc = osapi_waitq_enqueue (&(osapiRWLock->wqueue), timeout,
                                osapi_rwlock_w_waitq_remove_check,
                                (void *) osapiRWLock, __FP_CALLER__);

      if ((rc !=  SUCCESS) || ((osapiRWLock->flags & RWLOCK_DELETED) != 0))
      {
        osapiRWLock->flags &= ~(RWLOCK_W_PENDING);
        rc =  FAILURE;
        break;
      }
    }

    if (rc ==  SUCCESS)
    {
      osapiRWLock->wcount++;    /* result should always equal 1 */
      osapiRWLock->flags &= ~(RWLOCK_W_PENDING);
    }
  }
  else
  {
    rc =  FAILURE;
  }

  pthread_mutex_unlock (&(osapiRWLock->lock));

  return rc;
}

/**************************************************************************
 *
 * @purpose  Give a write lock
 *
 * @param    rwlock     ID of the requested lock returned from osapiRWLockCreate
 *
 * @returns   SUCCESS
 * @returns   FAILURE if lock is invalid
 *
 * @comments None.
 *
 * @end
 *
 *************************************************************************/
RC_t osapiWriteLockGive (osapiRWLock_t rwlock)
{
  RC_t rc =  SUCCESS;
  osapi_rwlock_t *osapiRWLock = (osapi_rwlock_t *) rwlock.handle;

  pthread_mutex_lock (&(osapiRWLock->lock));

  if (osapiRWLock->wcount == 0)
  {
    rc =  ERROR;
  }
  else
  {
    osapiRWLock->wcount--;      /* result should always equal 0 */
    osapi_waitq_dequeue (&(osapiRWLock->wqueue));
    /* Wake up _all_ waiting readers */
    osapi_waitq_dequeue_all (&(osapiRWLock->rqueue));
  }
  pthread_mutex_unlock (&(osapiRWLock->lock));
  return rc;
}

/**************************************************************************
 *
 * @purpose  Delete a read/write lock
 *
 * @param    rwlock     ID of the lock to delete
 *
 * @returns   SUCCESS
 * @returns   FAILURE if lock is invalid
 *
 * @comments Routine will suspend until there are no outstanding reads or writes.
 *
 * @end
 *
 *************************************************************************/
RC_t osapiRWLockDelete (osapiRWLock_t rwlock)
{
  RC_t rc =  SUCCESS;
  osapi_rwlock_t *osapiRWLock = (osapi_rwlock_t *) rwlock.handle;

  pthread_mutex_lock (&(osapiRWLock->lock));

  /* mark rwlock as deleted and wait for current readers and writers */

  osapiRWLock->flags |= RWLOCK_DELETED;

  while ((osapiRWLock->rcount > 0) || (osapiRWLock->wcount > 0))
  {
    /* wait for reads and writes to complete */
    rc = osapi_waitq_enqueue (&(osapiRWLock->wqueue),  WAIT_FOREVER,
                              osapi_rwlock_w_waitq_remove_check,
                              (void *) osapiRWLock, __FP_CALLER__);

    if (rc !=  SUCCESS)
    {
      /* This SHOULD NOT happen... */
      break;
    }
  }

  osapi_waitq_destroy (&(osapiRWLock->rqueue));

  osapi_waitq_destroy (&(osapiRWLock->wqueue));

  pthread_mutex_unlock (&(osapiRWLock->lock));

  pthread_mutex_destroy (&(osapiRWLock->lock));

  pthread_mutex_lock (&rwlock_list_lock);

  if (osapiRWLock->chain_next != NULL)
  {
    osapiRWLock->chain_next->chain_prev = osapiRWLock->chain_prev;
  }

  if (osapiRWLock->chain_prev != NULL)
  {
    osapiRWLock->chain_prev->chain_next = osapiRWLock->chain_next;
  }
  else
    /* must be head of list */
  {
    rwlock_list_head = osapiRWLock->chain_next;
  }

  pthread_mutex_unlock (&rwlock_list_lock);

  osapiFree ( OSAPI_COMPONENT_ID, osapiRWLock);

  return  SUCCESS;
}

/*********************************************************************
* @purpose  Initializes OSAPI Read/Write Lock Library
*
* @returns   SUCCESS
*
* @returns none
*
* @end
*
*********************************************************************/
RC_t osapiRWLockInit (void)
{
  return  SUCCESS;
}

/**************************************************************************
 *
 * @purpose  Print information about read/write lock
 *
 * @param    void
 *
 * @returns   SUCCESS
 *
 * @end
 *
 *************************************************************************/
RC_t osapiDebugRWLockPrint (void)
{
   char8 addrstr[0x80];
  osapi_rwlock_t *rwlock;
  uint32 count = 0;

  for (rwlock = rwlock_list_head; rwlock !=  NULLPTR;
       rwlock = rwlock->chain_next)
  {
    count++;
  }

  sysapiPrintf ("\r\nTotal Number of RW Locks : %d\r\n", count);

  sysapiPrintf
    ("Lock ID Flags       rcount   wcount   Read         Write      \r\n");
  sysapiPrintf
    ("------- ----------  -------- -------- -----------  -----------\r\n");

  for (rwlock = rwlock_list_head; rwlock !=  NULLPTR;
       rwlock = rwlock->chain_next)
  {
    sysapiPrintf ("%p ",  rwlock);
    sysapiPrintf ("%08x ", rwlock->flags);
    sysapiPrintf ("%8d ", rwlock->rcount);
    sysapiPrintf ("%8d ", rwlock->wcount);
    osapiAddressStringify (rwlock->rqueue.taken, addrstr, sizeof (addrstr));
    sysapiPrintf ("%s ", addrstr);
    osapiAddressStringify (rwlock->wqueue.taken, addrstr, sizeof (addrstr));
    sysapiPrintf ("%s ", addrstr);
    sysapiPrintf ("\r\n");
  }
  return  SUCCESS;
}
