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


#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "datatypes.h"
#include "commdefs.h"
#include "osapi_priv.h"
#include "osapi_sem.h"
#include "osapi.h"

static pthread_mutex_t queue_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static osapi_waitq_t *queue_list_head = NULL;

void osapi_waitq_create (osapi_waitq_t * queue, pthread_mutex_t * lock,
                         uint32 flags)
{
  pthread_condattr_t cat;


  queue->flags = flags;

  queue->lock = lock;

  queue->count = 0;

  queue->taken =  NULLPTR;

  pthread_cond_init (&(queue->control), NULL);

  if (WAITQ_POLICY (queue) == WAITQ_FIFO)
  {
    queue->policy.fifo.head = queue->policy.fifo.tail = NULL;
  }
  else
    /* priority queue */
  {
    pthread_condattr_init(&cat);

    pthread_condattr_setclock(&cat, CLOCK_MONOTONIC);
    pthread_cond_init (&(queue->policy.prio.cond), &cat);
  }

  if (pthread_mutex_lock (&queue_list_mutex) != 0)
  {
    osapi_printf ("osapi_waitq_create: queue_list_mutex error\n");
  }

  if (queue_list_head != NULL)
  {
    queue_list_head->chain_prev = queue;
  }

  queue->chain_next = queue_list_head;

  queue_list_head = queue;

  queue->chain_prev = NULL;

  pthread_mutex_unlock (&queue_list_mutex);
}

void osapi_waitq_destroy (osapi_waitq_t * queue)
{
  /* assume flush already done */

  /* assume queue is already locked */

  pthread_cond_destroy (&(queue->control));

  if (WAITQ_POLICY (queue) == WAITQ_PRIO)
  {
    pthread_cond_destroy (&(queue->policy.prio.cond));
    queue->policy.fifo.head = NULL;
  }

  if (pthread_mutex_lock (&queue_list_mutex) != 0)
  {
    osapi_printf ("osapi_waitq_destroy: queue_list_mutex error\n");
  }

  if (queue->chain_next != NULL)
  {
    queue->chain_next->chain_prev = queue->chain_prev;
  }

  if (queue->chain_prev != NULL)
  {
    queue->chain_prev->chain_next = queue->chain_next;
  }
  else
    /* must be head of list */
  {
    queue_list_head = queue->chain_next;
  }

  pthread_mutex_unlock (&queue_list_mutex);
}

static void osapi_waitq_unlock (osapi_task_t * self)
{
  osapi_waitq_t *queue = self->waiting;
  if (queue !=  NULLPTR)
  {
    queue->taken =  NULLPTR;
  }
  pthread_mutex_unlock (&self->lock);
}

#define osapi_waitq_lock(self, caller, where) \
  if (self->waiting !=  NULLPTR) \
  { \
    self->waiting->taken = caller; \
  } \
  if (pthread_mutex_lock (&(self->lock)) != 0) \
  { \
    osapi_printf ("osapi_waitq_enqueue: self->lock error %d\n", where); \
  } \

RC_t osapi_waitq_enqueue (osapi_waitq_t * queue, uint32 wait_msec,
                             removal_check_t removal_check,
                             void *removal_check_data, void *caller)
{
  struct timespec timeout = {};
  RC_t rc =  SUCCESS;
  osapi_task_t *self;
  int task_deleted = 0;
  int count = 0;
  int wait_rc;

  /* assume queue is already locked */

  if (wait_msec !=  WAIT_FOREVER)
  {
    (void) clock_gettime(CLOCK_MONOTONIC, &timeout);

    if (wait_msec !=  NO_WAIT)
    {
      timeout.tv_sec  += (wait_msec / 1000);
      timeout.tv_nsec += (((wait_msec % 1000) * 1000) * 1000);
    }
    /* Yes, this seems to matter to WRL 2.0's glibc */
    while (timeout.tv_nsec >= 1000000000)
    {
      timeout.tv_sec += 1;
      timeout.tv_nsec -= 1000000000;
    }
  }

  self = (void *) osapiTaskIdSelf ();

  if (self == NULL)
  {
    osapi_printf ("%s: invalid osapi_task_key\n", __FUNCTION__);
    return  ERROR;
  }

  osapi_waitq_lock (self, caller, __LINE__);
  task_deleted = (self->flags & TASK_DELETED);

  /*
     Check if the current task has been deleted, if not, we can add
     it to the waitq.
   */

  if (!task_deleted)
  {
    queue->count++;

    self->waiting = queue;
    /*
       Only seconds matter for wait time recording.
     */
    self->wait_time = timeout.tv_sec;
  }
  else
  {
    /*
       If the current task has been deleted, return an error to
       the calling task.  The sleep is to allow a lower-priority
       deleting task to be able to get the queue semaphore when
       the deleted task is running at high priority
     */
    osapi_waitq_unlock (self);
    /*
       osapiSleepMSec(100);
     */
    return  ERROR;
  }

  osapi_waitq_unlock (self);

  if (WAITQ_POLICY (queue) == WAITQ_PRIO)
  {
    do
    {
      if (wait_msec ==  WAIT_FOREVER)
      {
        pthread_cond_wait (&(queue->policy.prio.cond), queue->lock);
      }
      else
      {
        wait_rc = pthread_cond_timedwait (&(queue->policy.prio.cond),
                                          queue->lock, &timeout);
        if (wait_rc != 0)
        {
          rc =  ERROR;
          break;
        }
      }

      osapi_waitq_lock (self, caller, __LINE__);
      task_deleted = (self->flags & TASK_DELETED);

      /*
         If the task has now been deleted, we want to exit from the
         main waitq waiting loop
       */

      osapi_waitq_unlock (self);
    }
    while ((removal_check (removal_check_data) != WAITQ_REMOVE_OK)
           && (task_deleted == 0));

    osapi_waitq_lock (self, caller, __LINE__);
    task_deleted = (self->flags & TASK_DELETED);

    /*
       If task has not been deleted, lower the count to reflect
       removal from the queue; if the task has been deleted, then
       the deleting task will take care of reducing the count
     */

    if (task_deleted == 0)
    {
      count = --queue->count;
    }
    else
    {
      count = queue->count;
    }

    if ((count == 0) && ((queue->flags & WAITQ_FLUSHED) != 0))
    {
      pthread_cond_signal (&(queue->control));
    }

    self->waiting = NULL;
    self->wait_time = 0;

    osapi_waitq_unlock (self);
  }
  else
    /* FIFO queue */
  {
    if (queue->policy.fifo.head == NULL)        /* empty list */
    {
      queue->policy.fifo.head = queue->policy.fifo.tail = self;
    }
    else
    {
      self->fifo_prev = queue->policy.fifo.tail;

      queue->policy.fifo.tail->fifo_next = self;

      queue->policy.fifo.tail = self;
    }

    do
    {
      if (wait_msec ==  WAIT_FOREVER)
      {
        pthread_cond_wait (&(self->fifo_cond), queue->lock);
      }
      else
      {
        wait_rc = pthread_cond_timedwait (&(self->fifo_cond), queue->lock,
                                          &timeout);
        if (wait_rc != 0)
        {
          rc =  ERROR;
          break;
        }
      }

      osapi_waitq_lock (self, caller, __LINE__);
      task_deleted = (self->flags & TASK_DELETED);

      /*
         If the task has now been deleted, we want to exit from the
         main waitq waiting loop
       */

      osapi_waitq_unlock (self);
    }
    while ((removal_check (removal_check_data) != WAITQ_REMOVE_OK)
           && (task_deleted == 0));

    osapi_waitq_lock (self, caller, __LINE__);

    if (self->fifo_prev == NULL)
    {
      queue->policy.fifo.head = self->fifo_next;
    }
    else
    {
      self->fifo_prev->fifo_next = self->fifo_next;
    }

    if (self->fifo_next == NULL)
    {
      queue->policy.fifo.tail = self->fifo_prev;
    }
    else
    {
      self->fifo_next->fifo_prev = self->fifo_prev;
    }

    self->fifo_prev = self->fifo_next = NULL;

    task_deleted = (self->flags & TASK_DELETED);

    /*
       If task has not been deleted, lower the count to reflect
       removal from the queue; if the task has been deleted, then
       the deleting task will take care of reducing the count
     */

    if (task_deleted == 0)
    {
      queue->count--;
    }

    if ((queue->flags & WAITQ_FLUSHED) != 0)
    {
      pthread_cond_signal (&(queue->control));
    }

    self->waiting = NULL;
    self->wait_time = 0;

    osapi_waitq_unlock (self);
  }

  return rc;
}

void osapi_waitq_dequeue (osapi_waitq_t * queue)
{
  osapi_task_t *tptr;
   
  /* assume queue is already locked */

  if (queue->count != 0)
  {
    if (WAITQ_POLICY (queue) == WAITQ_PRIO)
    {
      pthread_cond_signal (&(queue->policy.prio.cond));
    }
    else
      /* FIFO queue */
    {
      tptr = queue->policy.fifo.head;
      while (tptr)
      {
        pthread_cond_signal (&(tptr->fifo_cond));
        tptr = tptr->fifo_next;
      }
    }
  }
}

/* Not quite the same as osapi_waitq_flush(). This does not wait for all the
   wakeups to finish, osapi_waitq_flush() does. */
void osapi_waitq_dequeue_all (osapi_waitq_t * queue)
{
  osapi_task_t *cur;

  /* assume queue is already locked */
  if (queue->count != 0)
  {
    if (WAITQ_POLICY (queue) == WAITQ_PRIO)
    {
      pthread_cond_broadcast (&(queue->policy.prio.cond));
    }
    else                        /* FIFO queue */
    {
      cur = queue->policy.fifo.head;
      while (cur)
      {
        pthread_cond_signal (&(cur->fifo_cond));
        cur = cur->fifo_next;
      }
    }
  }
}

void osapi_waitq_flush (osapi_waitq_t * queue)
{
  /* assume queue is already locked */

  if (queue->count != 0)
  {
    queue->flags |= WAITQ_FLUSHED;

    if (WAITQ_POLICY (queue) == WAITQ_PRIO)
    {
      pthread_cond_broadcast (&(queue->policy.prio.cond));

      while (queue->count != 0)
      {
        pthread_cond_wait (&(queue->control), queue->lock);
      }
    }
    else
      /* FIFO queue */
    {
      while (queue->count != 0)
      {
        pthread_cond_signal (&(queue->policy.fifo.head->fifo_cond));

        pthread_cond_wait (&(queue->control), queue->lock);
      }
    }

    queue->flags &= ~WAITQ_FLUSHED;
  }
}

void osapi_waitq_remove (osapi_waitq_t * queue, osapi_task_t * task)
{
  /* assume queue is already locked */

  /* assume task is already locked */

  if (WAITQ_POLICY (queue) == WAITQ_FIFO)
  {
    if (task->waiting == queue)
    {
      pthread_cond_signal (&(task->fifo_cond));
    }
  }

  /* nothing to do for WAITQ_PRIO... tasks are always FIFO */
}

/* Rather than use system("echo foo > /proc/bar"), this way avoids fork/exec and
   "system"'s security problems. Returns -1 on error, 0 on success. */
int osapi_proc_set (const char *path, const char *value)
{
  int rc, fd;
  struct stat stat_info;

  /* Stat first, as some kernel bugs will hang the system if you open() a /proc
     file that isn't there. */
  rc = stat (path, &stat_info);
  if (rc < 0)
  {
    return -1;
  }
  fd = open (path, O_WRONLY);
  if (fd < 0)
  {
    return -2;
  }

  do
  {
    rc = write (fd, value, strlen (value));
  } while ((0 > rc) && (EINTR == errno));

  close (fd);
  if (rc < 0)
  {
    return -3;
  }
  return 0;
}

int osapi_proc_get (const char *path, char *value, int size)
{
  int rc, fd;
  struct stat stat_info;

  /* Stat first, as some kernel bugs will hang the system if you open() a /proc
     file that isn't there. */
  rc = stat (path, &stat_info);
  if (rc < 0)
  {
    return -1;
  }
  fd = open (path, O_RDONLY);
  if (fd < 0)
  {
    return -2;
  }

  do 
  {
    rc = read (fd, value, size);
  } while ((0 > rc) && (EINTR == errno));

  close (fd);
  if (rc < 0)
  {
    return -3;
  }
  if ((rc > 0) && (value [rc-1] == '\n'))
  {
    value [rc-1] = 0;
  }
  return 0;
}

void osapiTimedSemaDebug(int type)
{
  void *myDebugSema;


  if(type == 1)
  {
    printf("%s:%d:Creating the PRIORITY myDebugSema.\n",__FUNCTION__,__LINE__);
    myDebugSema = osapiSemaBCreate(OSAPI_SEM_Q_PRIORITY, OSAPI_SEM_EMPTY);
    if( NULLPTR == myDebugSema)
    {
      printf("%s:%d:Failed to create the myDebugSema.\n",__FUNCTION__,__LINE__);
      return;
    }
  }
  else
  {
    printf("%s:%d:Creating the FIFO myDebugSema.\n",__FUNCTION__,__LINE__);
    myDebugSema = osapiSemaBCreate(OSAPI_SEM_Q_FIFO, OSAPI_SEM_EMPTY);
    if( NULLPTR == myDebugSema)
    {
      printf("%s:%d:Failed to create the myDebugSema.\n",__FUNCTION__,__LINE__);
      return;
    }
  }

  printf("%s:%d:Taking the myDebugSema:3min.\n",__FUNCTION__,__LINE__);
  if( SUCCESS != osapiSemaTake(myDebugSema,180000))
  {
    printf("%s:%d:Failed to take the myDebugSema.\n",__FUNCTION__,__LINE__);
    printf("%s:%d:Deleting the myDebugSema.\n",__FUNCTION__,__LINE__);
    osapiSemaDelete(myDebugSema); 
    return;
  }

  printf("%s:%d:Giving the myDebugSema.\n",__FUNCTION__,__LINE__);
  if( SUCCESS != osapiSemaGive(myDebugSema))
  {
    printf("%s:%d:Failed to give the myDebugSema.\n",__FUNCTION__,__LINE__);
  }

  printf("%s:%d:Deleting the myDebugSema.\n",__FUNCTION__,__LINE__);
  osapiSemaDelete(myDebugSema);
}
