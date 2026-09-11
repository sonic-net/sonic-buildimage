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

#ifndef OSAPIPROTO_HEADER
#define OSAPIPROTO_HEADER

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/socket.h>

#define sysapiPrintf printf
#define sysapiVaPrintf vprintf
#define sysapiFprintf fprintf
#define osapiSnprintf snprintf 
#define osapiStrncat strncat
#define osapiStrncmp strncmp
#define osapiStrncpy strncpy
#define osapiStrnCasecmp strncasecmp
#define osapiStrnlen strnlen
#define osapiStrStr strstr

#define osapiHtons htons
#define osapiHtonl htonl
#define osapiNtohs ntohs
#define osapiNtohl ntohl

#define OSAPI_STRNCAT(_dst, _src) \
  osapiStrncat((_dst), (_src), sizeof((_dst))-strlen((_dst))-1)

/* strncat macro resembling snprintf */
#define OSAPI_STRBCAT(_dst, _size, _src) \
  osapiStrncat((_dst), (_src), (_size)-strlen((_dst))-1)

/* strncpy macro resembling snprintf */
#define OSAPI_STRBCPY(_dst, _size, _src) \
  osapiStrncpySafe((_dst), (_src), (_size)-1)

/* This allows us to use a real type, while allowing the implementation
   to vary between operating systems... */
typedef struct osapiRWLock_s {
   void *handle;
} osapiRWLock_t;

typedef enum osapiRWLockOptions_e {
   OSAPI_RWLOCK_Q_FIFO = 0,
   OSAPI_RWLOCK_Q_PRIORITY
} osapiRWLockOptions_t;


typedef enum  {
    OSAPI_MSG_QUE_QUEUE_NAME,
    OSAPI_MSG_QUE_MESSAGES_IN_QUEUE,
    OSAPI_MSG_QUE_THREADS_SEND,
    OSAPI_MSG_QUE_WAITING_TO_RECIEVE,
    OSAPI_MSG_QUE_MESSAGES_HIGH,
    OSAPI_MSG_QUE_SEND_WAIT_ADDRESS,
    OSAPI_MSG_QUE_RECV_WAIT_ADDRESS,
    OSAPI_MSG_QUE_MESSAGES_MAX
 }MsgQueueAttribute_t;

/*********************************************************************
 * *
 * * @structures  clocktime_t
 * *
 * * @purpose
 * *
 * * @comments
 * *
 * *********************************************************************/
typedef struct  clocktime_t
{
  time_t seconds;
  time_t nanoseconds;
}   clocktime;

#define OSAPI_SEM_Q_FIFO             0x02
#define OSAPI_SEM_Q_PRIORITY         0x01
#define OSAPI_SEM_DELETE_SAFE        0x04
#define OSAPI_SEM_INVERSION_SAFE     0x08

/*********************************************************************
*
* @structures osapiTimerCallback_t
*
* @purpose
*
* @comments
*
*********************************************************************/
typedef void (*osapiTimerCallback_t)( uint32 parm1, uint32 parm2 );

/*********************************************************************
*
* @structures osapiTimerCallback64_t
*
* @purpose
*
* @comments
*
*********************************************************************/
typedef void (*osapiTimerCallback64_t)(  uint64 parm1,  uint64 parm2 );

/*********************************************************************
*
* @structures osapiTimerDescr_t
*
* @purpose
*
* @comments
*
*********************************************************************/
typedef struct osapiTimerDescr_s
{
  osapiTimerCallback_t  callback32;
  osapiTimerCallback64_t  callback64;
   uint64         parm1;
   uint64         parm2;
  uint32         timer_in_use;
  uint32         timer_running;
  uint32         time_count;
  uint32         orig_count;
  uint32         index;
  uint32         execution_time;

} osapiTimerDescr_t;

typedef struct osapiTimerChangeEntry_s
{
  osapiTimerDescr_t     *osapitimer;
  uint32             newTimeCount;

} osapiTimerChangeEntry;



typedef  int32 (*sysapiVaPrintf_t) (void *, const  char8 *, va_list) PRINTF_FMT(2,0);;

/**************************************************************************
*
* @purpose  Free memory
*
* @param    compId    @b{(input)}  the component Id from  COMPONENT_IDS_t
* @param    memory    @b{(input)}  Pointer to the memory to free.
*
* @returns  none.
*
* @comments
*
* @end
*
*************************************************************************/
#define osapiFree(comp, ptr) free(ptr)

/**************************************************************************
*
* @purpose  Allocate nbytes of memory.
*
* @param    compId    @b{(input)}  the component Id from  COMPONENT_IDS_t
* @param    nbytes    @b{(input)}  number of bytes to allocate.
*
* @returns  pointer to memory allocated.
*
* @comments  This function is implemented as a macro that
*            passes the file name and line number to the memory allocation
*            function. The file and line information is used for debugging
*            memory corruption or memory loss problems.
*
* @end
*
*************************************************************************/
#define osapiMalloc(compId, mem_size)  calloc(1,mem_size)

/**************************************************************************
*
* @purpose  Allocate nbytes of memory non fatal
*
* @param    compId    @b{(input)}  the component Id from  COMPONENT_IDS_t
* @param    nbytes    @b{(input)}  number of bytes to allocate.
*
* @returns  pointer to memory allocated.
*
* @comments  This function is implemented as a macro that
*            passes the file name and line number to the memory allocation
*            function. The file and line information is used for debugging
*            memory corruption or memory loss problems.
*
* @end
*
*************************************************************************/
#define osapiMallocTry(compId, mem_size)  calloc(1, mem_size)

/**************************************************************************
*
* @purpose  Reallocate nbytes of memory.
*
* @param    compId    @b{(input)}  the component Id from  COMPONENT_IDS_t
* @param    pBlock    @b{(input)}  memory to reallocate.
* @param    nbytes    @b{(input)}  number of bytes to reallocate.
*
* @returns  pointer to memory allocated.
*
* @comments  This function is implemented as a macro that
*            passes the file name and line number to the memory allocation
*            function. The file and line information is used for debugging
*            memory corruption or memory loss problems.
*
* @end
*
*************************************************************************/
#define osapiRealloc(compId, pBlock, mem_size)  realloc(pBlock, mem_size)

/**************************************************************************
*
* @purpose  Sleep for a given number of seconds.
*
* @param    seconds    @b{(input)}  number of seconds to sleep.
*
* @returns  none.
*
* @comments
*
* @end
*
*************************************************************************/
EXT_API void     osapiSleep( uint32 seconds);

/**************************************************************************
*
* @purpose  Sleep for a given number of milliseconds
*
* @param    msec   @b{(input)}  Number of milliseconds to sleep.
*
* @returns  none.
*
* @comments    CAUTION! The precision is in system ticks per second as determined
*           by the system clock rate, even though the units are in milliseconds.
*
* @end
*
*************************************************************************/
EXT_API void     osapiSleepMSec( uint32 msec);
#define OSAPI_MSEC_SLEEP(milliseconds) osapiSleepMSec(milliseconds)

/**************************************************************************
*
* @purpose  Create a task.
*
* @param    task_name    @b{(input)}  name to give the task.
* @param    task_entry   @b{(input)}  function pointer to begin the task.
* @param    argc         @b{(input)}  number of arguments. Must be < 10.
* @param    argv         @b{(input)}  pointer to start of list for arguments.
* @param    stack_size   @b{(input)}  number of bytes for the stack. Must be > 0.
* @param    priority     @b{(input)}  task priority. Must be <= 255.
* @param    time_slice   @b{(input)}  flag to allow time slicing for the task.
*
* @returns  task ID
* @returns  0 - Error
*
* @comments
*
* @end
*
*************************************************************************/
EXT_API void * osapiTaskCreate( char8 *task_name,
                                void *task_entry,
                                uint32 argc,
                                /*@null@*/void *argv,
                                uint32 stack_size,
                                uint32 priority,
                                uint32 time_slice );

/**************************************************************************
*
* @purpose  Waits for a task to complete initializaiton
*
* @param    synchandle  @b{(input)}  handle of task to wait for
* @param    timeout     @b{(input)}  timeout in milliseconds to wait for task
*
* @returns   SUCCESS  task is initialized, continue normal
* @returns   FAILURE  task is not init or could not sync up
*
* @comments
*
* @end
*
*************************************************************************/
EXT_API RC_t osapiWaitForTaskInit( uint32 syncHandle,  int32 timeout );

/**************************************************************************
*
* @purpose  Signals to a waiting task that this task has completed initialization
*
* @param    syncHandle  @b{(input)}  handle of the giving task
*
* @returns   SUCCESS if a task is waiting on the syncHandle
* @returns   FAILURE if a task is not waiting on the syncHandle
*
* @comments
*
* @end
*
*************************************************************************/
EXT_API RC_t osapiTaskInitDone(uint32 syncHandle);

/**************************************************************************
*
* @purpose  Get the task ID of a running task.
*
* @param    none
*
* @returns  task_id
*
* @comments
*
* @end
*
*************************************************************************/
EXT_API void *  osapiTaskIdSelf( void );

/**************************************************************************
*
* @purpose  Delete a task.
*
* @param    task_id   @b{(input)}  handle for the task to be deleted.
*
* @returns  none.
*
* @comments
*
* @end
*
*************************************************************************/
EXT_API void osapiTaskDelete( void * task_id);

/**************************************************************************
*
* @purpose  Delay a task for the number of system ticks
*
* @param    ticks   @b{(input)}  number of clock ticks to delay
*
* @returns   SUCCESS
* @returns   FAILURE  if called from interrupt level or if the calling task receives a signal that is
*           not blocked or ignored.
*
* @comments
*
* @end
*
*************************************************************************/
EXT_API RC_t  osapiTaskDelay (  int32 ticks );

/**************************************************************************
* @purpose  Lowers a task's priority to the lowest priority then returns
* @purpose  the task to it's previous priority
*
* @param    none
*
* @returns   SUCCESS
* @returns   FAILURE If called from interrupt context
*
* @end
*************************************************************************/
EXT_API RC_t osapiTaskYield ( void );

EXT_API RC_t osapiUTCTimeGet( clocktime * ct);
/**************************************************************************
*
* @purpose  Retrieve number of seconds since last reset
*
* @param    none
*
* @returns  rawtime
*
* @comments
*
* @end
*
*************************************************************************/
EXT_API uint32 osapiUpTimeRaw ( void );

/**************************************************************************
 *
 * @purpose  Retrieve number of milliseconds since last reset
 *
 * @param    void
 *
 * @returns  rawtime in milliseconds
 *
 * @comments
 *
 * @end
 *
 *************************************************************************/
uint32 osapiUpTimeMillisecondsGet ( void );

/**************************************************************************
 *
 * @purpose  Retrieve number of milliseconds
 *          AVOID THIS FUNCTION
 *          Use the osapiUpTimeMillisecondsGet()
 *
 * @param    void
 *
 * @returns  milliseconds since last reset
 *
 * @notes
 *
 * @end
 *
 *************************************************************************/
uint32 osapiTimeMillisecondsGet( void );

/*********************************************************************
* @purpose  Returns the function containing a given address
*
* @param    addr, uint32
*           funcName,  char8* buffer allocated by caller
*           funcNameLen, uint32 length of name buffer
*           offset, uint32* pointer to uint32 allocated by caller.
*                   Address's offset from beginning of function stored there.
*
* @returns   SUCCESS if function found, name printed into funcName buffer,
*                      offset set into *offset.
*            FAILURE if function not found, buffers untouched.
*
* @comments
*
* @end
*********************************************************************/
EXT_API RC_t osapiFunctionLookup(void * addr,  char8 *funcName,
                    uint32 funcNameLen, uint32 *offset);


/**************************************************************************
*
* @purpose  Convert a dot notation Internet address to a long integer
*
* @param    address @b{(input)} IP address in dotted notation
*
* @returns  IP address in long integer
*
* @end
*
*************************************************************************/
uint32 osapiInet_addr( uchar8 *address);

/**************************************************************************
*
* @purpose     Convert an IP address from a hex value to an ASCII string
*
* @param       ipAddr     @b{(input)}  IP address to be converted (host byte
*                                        order)
* @param       buf        @b{(output)} location to store IP address string
*
* @returns     void
*
* @comments    Caller must provide an output buffer of at least
* @comments    OSAPI_INET_NTOA_BUF_SIZE bytes (as defined in
* @comments    osapi.h).
* @comments    'ipAddr' is supplied in HOST BYTE ORDER
*
* @end
*
*************************************************************************/
void osapiInetNtoa (uint32 ipAddr,  uchar8 *buf);

/**************************************************************************
*
* @purpose      create ascii displayable string from IP (4/6) address
*
* @param        family          address faminly
*               addr            pointer to address
*               str             pointer to ouput string
*               len             length of output string
*
* @returns      output string or  NULL
*
*
* @end
*
*************************************************************************/
 uchar8 *osapiInetNtop(uint32 family,  uchar8 *addr,  uchar8 *str, uint32 len);

/*********************************************************************
* @purpose  Returns the string representation of given address
*
* @param    addr, void *
*           buf,  char8* buffer allocated by caller
*           bufsize, uint32 length of name buffer
*
* @returns  None
*
* @comments if function found, address, name and offset are printed
*           info passed buffer else only address is printed
*
* @comments
*
* @end
*********************************************************************/
void osapiAddressStringify (void *addr,  char8 *buf, uint32 bufsize);

/**************************************************************************
*
* @purpose  Create a read/write lock
*
* @param    rwlock    ptr to ID of the requested lock
* @param    options @b{(input)} queueing style - either Priority or FIFO
*
* @returns  ptr to the lock or NULL if lock cannot be allocated
*
* @comments None.
*
* @end
*
*************************************************************************/
EXT_API RC_t osapiRWLockCreate(osapiRWLock_t *rwlock,
                                  osapiRWLockOptions_t options);

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
EXT_API RC_t osapiRWLockDelete(osapiRWLock_t rwlock);

/**************************************************************************
*
* @purpose  Receive a message from a message queue.
*
* @param    Queue_ptr    @b{(input)}  Pointer to message queue.
* @param    Message      @b{(input)}  Place to put the message.
* @param    Size         @b{(input)}  Number of bytes to move into the message.
* @param    Wait         @b{(input)}  a flag to wait or not.  NO_WAIT or  WAIT_FOREVER.
*
* @returns   SUCCESS
* @returns   ERROR
*
* @comments    This routine receives a message from the message queue queue_ptr. The received message is
*           copied into the specified buffer, Message, which is Size bytes in length.
*           If the message is longer than Size, the remainder of the message is discarded (no
*           error indication is returned).
*
* @end
*
*************************************************************************/
EXT_API RC_t osapiMessageReceive(  void *queue_ptr, void *Message, uint32 Size, uint32 Wait);

/**************************************************************************
*
* @purpose  Send a message to a message queue.
*
* @param    queue_ptr    @b{(input)}  Pointer to message queue.
* @param    Message      @b{(input)}  pointer to the message.
* @param    Size         @b{(input)}  size of the message in bytes.
* @param    Wait         @b{(input)}  a flag to wait or not.  NO_WAIT or  WAIT_FOREVER.
* @param    Priority     @b{(input)}   MSG_PRIORITY_NORM or  MSG_PRIORITY_HI.
*
* @returns   SUCCESS
* @returns   ERROR
*
* @comments    This routine sends the message in buffer Message of length Size bytes
*           to the message queue queue_ptr. If any tasks are already waiting to
*           receive messages on the queue, the message will immediately
*           be delivered to the first waiting task. If no task is waiting to receive
*           messages, the message is saved in the message queue.
*
* @end
*
*************************************************************************/
EXT_API RC_t osapiMessageSend( void *queue_ptr, void *Message, uint32 Size, uint32 Wait, uint32 Priority);

/**************************************************************************
*
* @purpose  Create a message queue.
*
* @param    queue_name      @b{(input)}  Caller's desired name for this queue.
* @param    queue_size      @b{(input)}  is the max number of the messages on the queue.
* @param    message_size    @b{(input)}  is the size of each message in bytes.
*
* @returns  pointer to the Queue ID structure or  NULLPTR if the create failed.
*
* @comments    This routine creates a message queue capable of holding up to
*           queue_size messages, each up to message_size bytes long. The
*           routine returns a void ptr used to identify the created message queue
*           in all subsequent calls to routines in this library. The queue will be
*           created as a FIFO queue.
*
* @end
*
*************************************************************************/
EXT_API void * osapiMsgQueueCreate(    char8 *queue_name, uint32 queue_size, uint32 message_size);

/**************************************************************************
*
* @purpose  Delete a message queue.
*
* @param    queue_ptr       @b{(input)}  Pointer to message queue to delete
*
* @returns   SUCCESS
* @returns   ERROR
*
* @comments    This routine deletes a message queue.
*
* @end
*
*************************************************************************/
EXT_API RC_t  osapiMsgQueueDelete( void * queue_ptr);

/**************************************************************************
* @purpose  Returns the size of a message queue in number of elements.
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
RC_t osapiMsgQueueLimitGet(void *queue_ptr, uint32 *qLimit);

/**************************************************************************
* @purpose  Returns the high water mark of the number of messages in
*           the message queue.
*
* @param    queue_ptr   @b{(input)} Pointer to message queue.
* @param    highWater   @b{(output)} output value
*
* @returns   SUCCESS
* @returns   ERROR
*
* @comments None.
*
* @end
*************************************************************************/
RC_t osapiMsgQueueHwGet(void *queue_ptr, uint32 *highWater);

/**************************************************************************
* @purpose  Clears the high water mark on s message queue.
*
* @param    queue_ptr   @b{(input)} Pointer to message queue.
*
* @returns   SUCCESS
* @returns   ERROR
*
* @comments None.
*
* @end
*************************************************************************/
RC_t osapiMsgQueueHwClear(void *queue_ptr);


/**************************************************************************
* @purpose  Returns the current number of messages on the specified message queue.
*
* @param    queue_ptr   @b{(input)}  Pointer to message queue.
* @param    bptr        @b{(output)}  Pointer to area to return number.
*
* @returns   SUCCESS
* @returns   ERROR
*
* @comments
*
* @end
*************************************************************************/
EXT_API RC_t osapiMsgQueueGetNumMsgs( void * queue_ptr,  int32 *bptr);

/**************************************************************************
* @purpose  Returns the high water mark of messages on the specified message queue.
*
* @param    queue_ptr  @b{(input)}  Pointer to message queue.
* @param    queue_ptr  @b{(output)} Pointer to area to return number
*
* @returns   SUCCESS or  ERROR.
*
* @comments    none.
*
* @end
*************************************************************************/
EXT_API RC_t osapiMsgQueueGetHighWaterMark(void *queue_ptr,  int32 *bptr);

/**************************************************************************
 **
 ** @purpose  Copy a string to a buffer with a bounded length (with safeguards)
 **
 ** @param    *dest       @b{(input)}  destination location
 ** @param    *src        @b{(input)}  source string ptr
 ** @param    n           @b{(input)}  maximum number of characters to copy
 **
 ** @returns  Pointer to destination location (always)
 **
 ** @comments The dest and src strings must not overlap.  The dest location
 **           must have enough space for n additional characters.
 **
 ** @comments No more than n characters are copied from src to dest.  If there
 **           is no '\0' character within the first n > 0 characters of src, the
 **           n-th byte of dest string will be written with the '\0'
 **           string termination character (e.g., if n=10, *(dest+9)='\0').
 **
 ** @comments If the src string length is less than n, the remainder of
 **           dest is padded with nulls.
 **
 ** @comments To maintain consistency with the POSIX functions, the des
 **           pointer is returned, even if no characters were actually copied
 **           (e.g., src string has zero length, src ptr is null, n is 0,
 * *           etc.)
 * *
 * * @end
 * *
 * *************************************************************************/
 char8 *osapiStrncpySafe( char8 *dest, const  char8 *src, uint32 n);

/**************************************************************************
*
* @purpose  Signals to a waiting task that this task has completed initialization
*
* @param    syncHandle  @b{(input)}  handle of the giving task
*
* @returns   SUCCESS if a task is waiting on the syncHandle
* @returns   FAILURE if a task is not waiting on the syncHandle
*
* @comments
*
* @end
*
*************************************************************************/
RC_t osapiTaskInitDone(uint32 syncHandle);

/**************************************************************************
* @purpose  Task that wakes up periodically and invokes active timers.
*
* @param    none.
*
* @returns  none.
*
* @comments
*
* @end
*************************************************************************/
EXT_API void osapiTimerHandler(void) ;

/**************************************************************************
*
* @purpose  This is the user function to setup a new timeout call.
*
* @param    func           @b{(input)}  is the function to call when expired.
* @param    arg(2)         @b{(input)}  arguments to pass to the callback function.
* @param    milliseconds   @b{(input)}  number of milli-seconds to wait before timeout.
* @param    pTimerHolder   @b{(input)}  ptr to an osapiTimerDescr_t struct used to deallocate this timer by the user
*
* @returns  none.
*
* @comments
*
* @end
*
*************************************************************************/
EXT_API void osapiTimerAdd ( void (*func)( uint32, uint32 ), uint32 arg1, uint32 arg2,  int32 milliseconds, osapiTimerDescr_t **pTimerHolder );

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
 * *************************************************************************/
RC_t osapiChangeUserTimer(osapiTimerDescr_t *osapitimer,
                             uint32 newTimeCount);


/**************************************************************************
*
* @purpose  This is the user function to setup a new timeout call.
*           This function accepts 64 bit user parameters.
*           This function should be used when passing pointers as
*           timer arguments.
*
* @param    func           @b{(input)}  is the function to call when expired.
* @param    arg(2)         @b{(input)}  arguments to pass to the callback function.
* @param    milliseconds   @b{(input)}  number of milli-seconds to wait before timeout.
* @param    pTimerHolder   @b{(input)}  ptr to an osapiTimerDescr_t struct used to deallocate this timer by the user
*
* @returns  none.
*
* @comments
*
* @end
*
*************************************************************************/
EXT_API void osapiTimer64Add ( void (*func)(  uint64,  uint64 ),  uint64 arg1,  uint64 arg2,  int32 milliseconds, osapiTimerDescr_t **pTimerHolder );

/**************************************************************************
*
* @purpose  Remove a function for the timeout list.
*
* @param    pTimerHolder   @b{(input)}  ptr to an osapi timer descriptor, that was returned in osapiTimerAdd(), to deallocate.
*
* @returns  none.
*
* @comments
*
* @end
*
*************************************************************************/
EXT_API void osapiTimerFree ( osapiTimerDescr_t *pTimer );

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
EXT_API RC_t osapiWriteLockGive(osapiRWLock_t rwlock);

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
EXT_API RC_t osapiWriteLockTake(osapiRWLock_t rwlock,  int32 timeout);

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
EXT_API RC_t osapiReadLockTake(osapiRWLock_t rwlock,  int32 timeout);

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
EXT_API RC_t osapiReadLockGive(osapiRWLock_t rwlock);

EXT_API int osapiDebugQTestStart(int test_num, int msg_count, int msg_size, int queue_size);

/*******************************************************************************
* @purpose      To locate character in string
*
* @param   *s    @b{(input)} input string to search for the character
* @param   len   @b{(input)} len of the input string
* @param   c         @b{(input)} character to search for
* @returns      a pointer to the matched character or NULL if the character is not found
*
* @comments
*
* @end
*******************************************************************************/
EXT_API  char8 *osapiStrnchr (const  char8 *s, uint32 len,  char8 c);

#endif /* OSAPIPROTO_HEADER */
