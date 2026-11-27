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

#ifndef OSAPI_PRIV_H_INCLUDED
#define OSAPI_PRIV_H_INCLUDED

/* System includes, in case users don't include already... */
#include <sys/types.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include <semaphore.h>

#include "osapi.h"


#define OSAPI_TICK_USEC (1000000/100)

#define OSAPI_MAX_PRIO 255

typedef int (*osapi_task_entry_t) (uint32, void *);

typedef struct osapi_task_s
{
  uint32 flags;
  pthread_mutex_t lock;
  pthread_cond_t control;
  uint32 osapi_prio;
   int32 pthread_prio;
   int32 cur_prio;
  char *name;
  osapi_task_entry_t *entry;
  uint32 argc;
  void *argv;
   uchar8 *stack_base_addr;
  uint32 stack_size;
  struct osapi_waitq_s *waiting;
  /* Records how long a task is waiting on a queue in seconds */
  uint32 wait_time;
  pthread_cond_t fifo_cond;
  struct osapi_task_s *fifo_prev, *fifo_next;
  pthread_t thread;
  pthread_attr_t attr;
  struct sched_param param;
  struct osapi_task_s *chain_prev, *chain_next;
  pid_t PID;
  int signal_lock;
#ifdef  STACK_USAGE_STATS
  uint32 stack_usage_read_time;
#endif
   uint32  core_number; /* Core number or -1 for any core */
} osapi_task_t;


/* Need the following:
      -- wait queue of tasks waiting for this task's deletion
      -- delete safety count
      -- flags to indicate task's state
 */

#define TASK_TIME_SLICED    0x00000001
#define TASK_DELETE_SAFE    0x00000002
#define TASK_DELETED        0x00000004


typedef struct osapi_waitq_s
{
  uint32 flags;
  pthread_mutex_t *lock;
  pthread_cond_t control;
  void *taken;
  uint32 count;
  union
  {
    struct
    {
      osapi_task_t *head, *tail;
    } fifo;
    struct
    {
      pthread_cond_t cond;
    } prio;
  } policy;
  struct osapi_waitq_s *chain_prev, *chain_next;
} osapi_waitq_t;

typedef struct osapi_msg_s
{
  struct osapi_msg_s *next;
  char buf[1]; /* length (1) is placeholder for actual message length */
} osapi_msg_t;


typedef struct osapi_msgq_s
{
  uint32 flags;
  pthread_mutex_t lock;
  pthread_cond_t control;
  char *name;
  uint32 msg_count, msg_max, msg_len, msg_high;
  osapi_msg_t *base, *free;
  osapi_msg_t *head, *tail;
  osapi_waitq_t send_wait, recv_wait;
  struct osapi_msgq_s *chain_prev, *chain_next;
  uint32  mem_size;
  uint32 max_elements;       /* maximum number of elements in queue */
} osapi_msgq_t;



typedef struct
{
  void *caller;
  uint32 time_stamp;
   uint8 give;
} osapiSemHistory_t;

#define OSAPI_SEM_HISTORY_SIZE 8

/* still need delete safety and inversion safety */
typedef struct osapi_sem_s
{
  uint32 flags;
  pthread_mutex_t lock;
  uint32 init_count, cur_count;
  osapi_task_t *owner;          /* only for mutex */
   int32 depth;               /* only for mutex */
  osapi_waitq_t queue;
  osapi_task_t *curr_owner;
  osapi_task_t *last_owner;
  uint32 num_waiting;
  struct osapi_sem_s *chain_prev, *chain_next;
  osapiSemHistory_t history[OSAPI_SEM_HISTORY_SIZE];
   ushort16 historyIndex;
  uint32 holdTime;
   uint8 blocked;
   char8 name[1];
} osapi_sem_t;


#define WAITQ_FIFO      0
#define WAITQ_PRIO      1
#define WAITQ_POLICY_MASK   1
#define WAITQ_POLICY(q) (q->flags & WAITQ_POLICY_MASK)

#define WAITQ_FLUSHED       2


typedef int (*removal_check_t) (void *);

extern void osapi_waitq_create (osapi_waitq_t * queue, pthread_mutex_t * lock,
                                uint32 flags);
extern void osapi_waitq_destroy (osapi_waitq_t * queue);
extern RC_t osapi_waitq_enqueue (osapi_waitq_t * queue,
                                    uint32 wait_msec,
                                    removal_check_t removal_check,
                                    void *removal_check_data, void *caller);
extern void osapi_waitq_dequeue (osapi_waitq_t * queue);
extern void osapi_waitq_dequeue_all (osapi_waitq_t * queue);
extern void osapi_waitq_flush (osapi_waitq_t * queue);
extern void osapi_waitq_remove (osapi_waitq_t * queue, osapi_task_t * task);

extern int osapi_printf (const char *, ...) PRINTF_FMT(1,2);

typedef int (*osapiPrinter_t) (void *arg, const char *format, ...) PRINTF_FMT(2,3);

#define WAITQ_REMOVE_OK 0


#define __FP_CALLER__ __builtin_return_address(0)

#endif /* OSAPI_PRIV_H_INCLUDED */
