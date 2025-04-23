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
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>
//#include "proc_osapi.h"
#include "datatypes.h"
#include "commdefs.h"

typedef struct 
{
  unsigned int max_size;  /* Maximum messages in the queue */
  unsigned int msg_size;  /* Size of each message in the queue */
  unsigned int num_msgs;  /* Current number of messages in the queue */

  unsigned int head; /* First message in the queue (Next to be dequeued) */
  unsigned int tail; /* Last message in the queue */

  unsigned char *buf;  /* Buffer for storing the messages */
  unsigned int  buf_size; /* Number of bytes in the queue buffer */

  sem_t tx_sema; /* Block callers when queue is full */
  sem_t rx_sema; /* Block callers when queue is empty */

  pthread_mutex_t mutex; /* Protect access to the queue structure */
   
} proc_osapi_msgq_t; 


/**************************************************************************
* @purpose  Create a message queue.
*
* @param    queue_name    @b{(input)}  Not used, but kept for backwards compatability.
* @param    queue_size    @b{(input)}  is the max number of the messages on the queue.
* @param    message_size  @b{(input)}  is the size of each message in bytes.
*
* @returns  pointer to the Queue ID structure or  NULLPTR if the create failed.
*
* @comments    This routine creates a message queue capable of holding up to
* @comments    queue_size messages, each up to message_size bytes long.  The
* @comments    routine returns a void ptr used to identify the created message queue
* @comments    in all subsequent calls to routines in this library. The queue will be
* @comments    created as a FIFO queue.
*
* @end
*************************************************************************/
void * osapiMsgQueueCreate( char8 *queue_name, uint32 queue_size,
                           uint32 message_size)
{
  proc_osapi_msgq_t *msgq;
  pthread_mutexattr_t attr;

  msgq = malloc(sizeof(proc_osapi_msgq_t));

  msgq->max_size = queue_size;
  msgq->msg_size = message_size;
  msgq->num_msgs = 0;
  msgq->head = 0;
  msgq->tail = 0;

  msgq->buf_size = queue_size * message_size;
  msgq->buf = malloc (msgq->buf_size);
  memset (msgq->buf, 0, msgq->buf_size);

  pthread_mutexattr_init(&attr);
  pthread_mutex_init(&msgq->mutex, &attr);

  sem_init (&msgq->tx_sema, 0, queue_size);
  sem_init (&msgq->rx_sema, 0, 0);

  return msgq;
}

/**************************************************************************
* @purpose  Returns the current number of messages on the specified message queue.
*
* @param    queue_ptr  @b{(input)}  Pointer to message queue.
* @param    bptr  @b{(output)} Pointer to area to return number
*
* @returns   SUCCESS or  ERROR.
*
* @comments    none.
*
* @end
*************************************************************************/
RC_t osapiMsgQueueGetNumMsgs(void *queue_ptr,  int32 *bptr)
{
  proc_osapi_msgq_t *msgq = queue_ptr;

  pthread_mutex_lock(&msgq->mutex);
  *bptr = msgq->num_msgs;
  pthread_mutex_unlock(&msgq->mutex);

  return  SUCCESS;
}


/**************************************************************************
* @purpose  Returns the content of the message without removing the
*           message from the queue.
*
* @param    queue_ptr  @b{(input)}  Pointer to message queue.
* @param    Message - Buffer allocated by the caller to put the message.
* @param    Size - Size of the buffer pointed by Message.
* @param    msgOffset - Number of messages to skip from the first
*                       message in the queue before reading the data.
*
* @returns   SUCCESS or  FAILURE.
*
* @comments    none.
*
* @end
*************************************************************************/
RC_t osapiMessagePeek(void *queue_ptr, void *Message,
                            uint32 Size, uint32 msgOffset)
{
  proc_osapi_msgq_t *msgq = queue_ptr;
  unsigned int msg_to_read;

  pthread_mutex_lock(&msgq->mutex);
  if ((msgq->num_msgs == 0) || ((msgq->num_msgs - 1) < msgOffset))
  {
    pthread_mutex_unlock(&msgq->mutex);
    return  FAILURE;
  }

  msg_to_read = (msgq->head + (msgOffset * msgq->msg_size)) % msgq->buf_size;
  memcpy(Message, &msgq->buf[msg_to_read],
         ((Size < msgq->msg_size) ? Size : msgq->msg_size));


  pthread_mutex_unlock(&msgq->mutex);
  return  SUCCESS;
}

/**************************************************************************
* @purpose  Receive a message from a message queue.
*
* @param    Queue_ptr @b{(input)}   Pointer to message queue.
* @param    Message @b{(output)}    Place to put the message.
* @param    Size @b{(input)}        Number of bytes to move into the message.
* @param    Wait @b{(input)}        a flag to wait or not.  NO_WAIT or  WAIT_FOREVER.
*                                   The function does not support timed waits, so only
*                                   "No Wait" or "Wait Forever" is accepted.
*
* @returns   SUCCESS on success or  ERROR if an error occured.
*
* @comments    This routine receives a message from the message queue queue_ptr. The received message is
* @comments    copied into the specified buffer, Message, which is Size bytes in length.
* @comments    If the message is longer than Size, the remainder of the message is discarded (no
* @comments    error indication is returned).
*
* @end
*************************************************************************/
RC_t osapiMessageReceive(void *queue_ptr, void *Message,
                            uint32 Size, uint32 Wait)
{
  proc_osapi_msgq_t *msgq = queue_ptr;
  int err;

  if((Wait !=  WAIT_FOREVER) && (Wait !=  NO_WAIT))
  {
    return  ERROR;
  }

  if(Wait ==  WAIT_FOREVER)
  {
    do {
        err = sem_wait(&msgq->rx_sema);
    } while (err != 0 && errno == EINTR);
  } else
  {
    do
    {
      err = sem_trywait(&msgq->rx_sema);
    } while ((err < 0) && (errno == EINTR));
  }
  if(err)
  {
    return  FAILURE;
  }

  pthread_mutex_lock(&msgq->mutex);
  memcpy(Message, &msgq->buf[msgq->head],
         ((Size < msgq->msg_size) ? Size : msgq->msg_size));
  msgq->head += msgq->msg_size;
  msgq->head %= msgq->buf_size;
  msgq->num_msgs--;
  pthread_mutex_unlock(&msgq->mutex);

  sem_post (&msgq->tx_sema);

  return  SUCCESS;
}

/**************************************************************************
*
* @purpose  Send a message to a message queue.
*
* @param    queue_ptr    @b{(input)}  Pointer to message queue.               
* @param    Message      @b{(input)}  pointer to the message.          
* @param    Size         @b{(input)}  size of the message in bytes.
* @param    Wait         @b{(input)}  a flag to wait or not.  NO_WAIT or  WAIT_FOREVER.
*                                     This function does not accept a timeout value, so
*                                     only "No Wait" or "Wait Forever" options are allowed.
* @param    Priority     @b{(input)}  Ignored.
*
* @returns   SUCCESS
* @returns   ERROR
*
* @comments This routine sends the message in buffer Message of length Size bytes
*           to the message queue queue_ptr. If any tasks are already waiting to
*           receive messages on the queue, the message will immediately
*           be delivered to the first waiting task. If no task is waiting to receive
*           messages, the message is saved in the message queue.
*
* @end
*
*************************************************************************/
RC_t osapiMessageSend(void *queue_ptr, void *Message, uint32 Size,
                         uint32 Wait, uint32 Priority)
{
  proc_osapi_msgq_t *msgq = queue_ptr;
  int err;

  if((Wait !=  WAIT_FOREVER) && (Wait !=  NO_WAIT))
  {
    return  ERROR;
  }

  if(Wait ==  WAIT_FOREVER)
  {
    do {
        err = sem_wait(&msgq->tx_sema);
    } while (err != 0 && errno == EINTR);
  } else
  {
    do
    {
      err = sem_trywait(&msgq->tx_sema);
    } while ((err < 0) && (errno == EINTR));
  }
  if(err)
  {
    return  FAILURE;
  }

  pthread_mutex_lock(&msgq->mutex);
  memcpy(&msgq->buf[msgq->tail], Message,
         ((Size < msgq->msg_size) ? Size : msgq->msg_size));
  msgq->tail += msgq->msg_size;
  msgq->tail %= msgq->buf_size;
  msgq->num_msgs++;
  pthread_mutex_unlock(&msgq->mutex);

  sem_post (&msgq->rx_sema);

  return  SUCCESS;
}

/**************************************************************************
* @purpose  Delete a message queue.
*
* @param    queue_ptr     @b{(input)}  Pointer to message queue to delete
*
* @returns   SUCCESS or  ERROR
*
* @comments    This routine deletes a message queue.
*
* @end
*************************************************************************/
RC_t osapiMsgQueueDelete(void *queue_ptr)
{
  proc_osapi_msgq_t *msgq = queue_ptr;

  pthread_mutex_destroy(&msgq->mutex);

  sem_destroy (&msgq->tx_sema);
  sem_destroy (&msgq->rx_sema);

  free (msgq->buf);
  free (msgq);

  return  SUCCESS;
}

/**************************************************************************
* @purpose  Returns the maximum number of elements that may be in a message queue.
*
* @param    queue_ptr   @b{(input)} Pointer to message queue.
* @param    qLimit      @b{(output)} output value.
*
* @returns   SUCCESS
* @returns   ERROR
*
* @comments None.
*
* @end
*************************************************************************/
RC_t osapiMsgQueueLimitGet(void *queue_ptr, uint32 *qLimit)
{
  proc_osapi_msgq_t *msgq = queue_ptr;

  pthread_mutex_lock(&msgq->mutex);
  *qLimit = msgq->max_size;
  pthread_mutex_unlock(&msgq->mutex);

  return  SUCCESS;
}

