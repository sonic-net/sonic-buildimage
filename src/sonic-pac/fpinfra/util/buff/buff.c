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
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "commdefs.h"
#include "datatypes.h"
#include "buff_api.h"
#include "buff.h"
#include "log.h"
#include "osapi.h"
#include "resources.h"


/* Static variables.
*/
static uint32 NumBufferPools = 0; /* Number of created buffer pools */
static uint32 MaxBufferPools = 0; /* Maximum number of pools that have
                                     ** ever been created.
                                     */
/* Array of buffer pool descriptors.
*/
static bufferPoolType BufferPoolList[ MAX_BUFFER_POOLS];

static pthread_mutex_t BufferPoolLockSem = PTHREAD_MUTEX_INITIALIZER;

/*********************************************************************
* @purpose  Allocates and creates a buffer pool
*
*
* @param   comp_id          -  COMPONENT ID of the caller
* @param   num_buffers      - Number of buffers requested
* @param   buffer_size      - Size of buffers in the pool. The buffer size
*                             must be a multiple of four.
* @param   buffer_description - Text string up to 16 characters describing the
*                               buffer pool. The string is displayed by the
*                               bufferPoolShow command.
* @param   buffer_pool_id    (output) - Application uses this ID to reference the
*                            buffer pool.

* @returns  None
*
* @notes   This routine both allocates the memory for a buffer pool and creates
*          the buffer pool. It is preferred over bufferPoolCreate().
*
*          Function calls LOG_ERROR if the it detects memory corruption.
*
* @end
*********************************************************************/
uint32 bufferPoolInit ( COMPONENT_IDS_t comp_id, uint32 num_buffers,
                          uint32 buffer_size,  char8  *description, 
                          uint32 * buffer_pool_id)
{
  RC_t rc;
  uint32 pool_size;
   uchar8 *pool;
  uint32 buff_count;

  /* Determine how much memory we need for the desired number of buffers.
  */
  pool_size = bufferPoolSizeCompute (num_buffers, buffer_size);

  /* Allocate memory for the buffer pool.
  */
  pool = osapiMalloc (comp_id, pool_size);


  if (pool ==  NULLPTR)
  {
    LOG_ERROR_OPT_RESET( LOG_SEVERITY_ERROR,
                         0, 
                         "Failed to allocate pool. "
                         "The system may be in inconsistent state. "
                         "Recommend rebooting the system now.");
    return  FAILURE;

  }
  /* Create the buffer pool.
  */
  rc = bufferPoolCreate (pool,
                         pool_size,
                         buffer_size,
                         description,
                         buffer_pool_id,
                         &buff_count);
  if (rc !=  SUCCESS)
  {
    LOG_ERROR_OPT_RESET( LOG_SEVERITY_ERROR,
                         0, 
                         "Failed to create buffer pool. "
                         "The system may be in inconsistent state. "
                         "Recommend rebooting the system now.");
    return  FAILURE;
  }

  /* Since we use bufferPoolSizeCompute function to determine pool size, the
  ** number of buffers in the pool should be equal to the number of buffers we want.
  */
  if (buff_count != num_buffers)
  {
    LOG_ERROR(buff_count);
  }
  return rc;
}

/*********************************************************************
*
* @purpose  Create a buffer pool.
*
* @param   buffer_pool_addr - Buffer pool memory address.
*                             This memory must be statically or dynamically
*                             allocated by the caller.
* @param   buffer_pool_size - Number of bytes in the buffer_pool_addr.
* @param   buffer_size - Size of buffers in the pool. The buffer size must be
*                        multiple of four.
* @param   buffer_description - Text string up to 16 characters describing the
*                               buffer pool. The string is displayed by the
*                               bufferPoolShow command.
* @param   buffer_pool_id    (output) - Application uses this ID to reference the
*                            buffer pool.
* @param   buffer_count      (output) - Number of buffers in the pool.
*
* @returns   SUCCESS, if success
*
* @returns   ERROR
*                 - Buffer memory size is too small for even one buffer.
* @returns   FAILURE
*                 - Maximum number of buffer pools have already been created.
*
*
* @notes
*
* @end
*********************************************************************/
RC_t bufferPoolCreate (void * buffer_pool_addr,
                          uint32 buffer_pool_size,
                          uint32 buffer_size,
                           char8  * description,

                          uint32 * buffer_pool_id,
                          uint32 * buffer_count)
{
  uint32 num_bufs;
  uint32 tot_buf_size; /* Buffer size plus overhead */
  uint32 pool_id;
  bufferPoolType *pool;
  uint32 i;
   uchar8 *user_data; /* Start of data buffer area */
  bufferDescrType *descr;
   uchar8 *pool_addr;

  /* If we reached the maximum number of buffer pools then return an
  ** error.
  */
  if (NumBufferPools >=  MAX_BUFFER_POOLS)
  {
     LOGF( LOG_SEVERITY_INFO,
            "we reached the maximum number (%d) of buffer pools\n", NumBufferPools);
    return  FAILURE;
  }

  /* Make sure that buffer pool address is alligned on a four-byte
  ** boundary.
  */
  pool_addr =  buffer_pool_addr;
  if (((unsigned long) pool_addr) & 0x3)
  {
    return   ERROR;
  }

  /* Make sure that buffer size is a multiple of eight.
  ** Round up the buffer size if necessary.
  */
  if (buffer_size != (buffer_size & 0xFFFFFF8))
  {
    buffer_size += 8;
    buffer_size &= 0xFFFFFFF8;
  }

  /* Zero out the buffer storage area.
  */
  memset (buffer_pool_addr, 0, buffer_pool_size);

  /* Compute how much memory is required for each buffer. The overhead
  ** includes buffer descriptor and a pointer in the free list.
  */
  tot_buf_size = buffer_size +
                 sizeof (bufferDescrType) +
                 sizeof (void *);

  /* Determine how many buffer we can allocate.
  */
  num_bufs = buffer_pool_size / tot_buf_size;

  /* If we can't allocate any buffers then return an error.
  */
  if (num_bufs == 0)
  {
    return  ERROR;
  }

  /* >>>>>>>>>>> Start Critical Section
  ** Determine first unused pool ID.
  */
  (void) pthread_mutex_lock (&BufferPoolLockSem);

  pool_id = 0;
  while (pool_id < MaxBufferPools)
  {
    if (BufferPoolList[pool_id].pool_size == 0)
      break;  /* Found an unused buffer pool */
    pool_id++;
  }
  NumBufferPools++;
  pool = &BufferPoolList[pool_id];
  pool->id = pool_id +  LOW_BUFFER_POOL_ID;
  pool->pool_size = buffer_pool_size;

  if (NumBufferPools > MaxBufferPools)
  {
    MaxBufferPools = NumBufferPools;
  }

  (void) pthread_mutex_unlock (&BufferPoolLockSem);

  /* <<<<<<<<<<< End Critical Section
  */

  /* Set up the buffer pool.
  */
  pool->addr =  buffer_pool_addr;
  pool->buf_size = buffer_size;
  pool->total = num_bufs;
  osapiStrncpySafe(pool->descr,description, MAX_BUFFER_DESCR_SIZE);
  pool->descr[ MAX_BUFFER_DESCR_SIZE-1] = 0;

  pool->free_count = num_bufs;

  pool->num_allocs = 0;
  pool->no_buffers_count = 0;
  pool->floor = 0;
  pool->high_watermark = 0;

  pool->free_list = (bufferDescrType **) buffer_pool_addr;

  user_data = (( uchar8 *) buffer_pool_addr) +
              (sizeof (void *) * num_bufs);

  for (i = 0; i < num_bufs; i++)
  {
    pool->free_list[i] = (bufferDescrType *) user_data;

    descr = pool->free_list[i];
    descr->id = ( ushort16) (pool_id +  LOW_BUFFER_POOL_ID);
    descr->in_use = 0;

    user_data += (buffer_size + sizeof (bufferDescrType));
  }

  *buffer_pool_id = pool_id +  LOW_BUFFER_POOL_ID;
  *buffer_count = num_bufs;

  return  SUCCESS;
}



/*********************************************************************
* @purpose  Deletes a buffer pool and deallocates its memory.
*
*
* @param   comp_id            COMPONENT ID of the caller
* @param   buffer_pool_id    Application uses this ID to reference the
*                            buffer pool.

* @returns   SUCCESS
* @returns   FAILURE
* @returns   ERROR
*
* @notes   This routine both deletes a buffer pool and frees the memory
* @notes   for the buffer pool. It is preferred over bufferPoolDelete().
*
* @end
*********************************************************************/
RC_t bufferPoolTerminate ( COMPONENT_IDS_t comp_id, uint32  buffer_pool_id)
{
   uchar8 * buffer_pool_addr;
  uint32 pool_id;

  pool_id = buffer_pool_id -  LOW_BUFFER_POOL_ID;
  buffer_pool_addr = BufferPoolList[pool_id].addr;

  if (buffer_pool_addr ==   NULLPTR)
  {
    LOG_ERROR_OPT_RESET( LOG_SEVERITY_ERROR,
                         1, 
                         "The given buffer poolID is not valid. "
                         "The system may be in inconsistent state. "
                         "Recommend rebooting the system now.");
    return  FAILURE;
  }

  if (bufferPoolDelete(buffer_pool_id) !=  SUCCESS)
  {
       LOGF( LOG_SEVERITY_INFO,
              "Cannot delete buffer pool- pool id = %x\n", buffer_pool_id);
      return  FAILURE;
  }


  /* Deallocate memory for the buffer pool.
  */
  osapiFree(comp_id, (void *)buffer_pool_addr);

  return  SUCCESS;
}


/*********************************************************************
*
* @purpose  Delete a buffer pool.
*
* @param   buffer_pool_id - Buffer pool to be deleted.
*
* @returns   SUCCESS, if success
*
* @returns   FAILURE
*                 - Buffer pool does not exist.
*
*
* @notes
*   Invalid Input is probably due to memory corruption.
*   If in the future we have a recovery mechanism
*
* @end
*********************************************************************/
RC_t bufferPoolDelete (uint32 buffer_pool_id)
{
  uint32 pool_id;

  pool_id = buffer_pool_id -  LOW_BUFFER_POOL_ID;

  if (pool_id >= MaxBufferPools)
  {
    LOG_ERROR_OPT_RESET( LOG_SEVERITY_ERROR,
                         buffer_pool_id, 
                         "The given buffer poolID is "
                         "greater than the max available buffer pools. "
                         "The system may be in inconsistent state. "
                         "Recommend rebooting the system now.");
    return  FAILURE;
  }


  /*>>>>>>>>>>>>>>>>> Start Critical Section
  */
  (void) pthread_mutex_lock (&BufferPoolLockSem);

  if ((BufferPoolList[pool_id].pool_size == 0) ||
      (BufferPoolList[pool_id].id != buffer_pool_id))
  {
    /*<<< Exit Critical Section.
    */
	(void) pthread_mutex_unlock (&BufferPoolLockSem);


    return  FAILURE;
  }

  BufferPoolList[pool_id].id = 0;
  BufferPoolList[pool_id].pool_size = 0;

  NumBufferPools--;

  (void) pthread_mutex_unlock (&BufferPoolLockSem);
  /* <<<<<<<<<<<<<<<< End Critical Section
  */

  return  SUCCESS;
}


/*********************************************************************
*
* @purpose  Allocate a buffer from the buffer pool.
*
* @param   buffer_pool_id - Buffer pool from which to allocate buffer.
* @param   buffer_addr - (output) Address of the buffer.
*
* @returns   SUCCESS, if success
*
* @returns   ERROR
*                 - Buffer pool does not have any more free buffers.
*
* @notes
*
* @end
*********************************************************************/
RC_t bufferPoolAllocate (uint32 buffer_pool_id,
                             uchar8 ** buffer_addr)
{
  bufferDescrType * descr;
  uint32 pool_id, current_alloc;

  pool_id = buffer_pool_id -  LOW_BUFFER_POOL_ID;

  if (pool_id >= MaxBufferPools)
  {
    LOG_ERROR (buffer_pool_id);
  }


  if ((BufferPoolList[pool_id].pool_size == 0) ||
      (BufferPoolList[pool_id].id != buffer_pool_id))
  {
    LOG_ERROR (buffer_pool_id);
  }

  /*>>>>>>>>>>>>>>>>> Start Critical Section
  */
  (void) pthread_mutex_lock (&BufferPoolLockSem);

  /* Return an error if we don't have any more free buffers.
  */
  if (BufferPoolList[pool_id].free_count == BufferPoolList[pool_id].floor)
  {
    BufferPoolList[pool_id].no_buffers_count++;

    /*<<< Exit Critical Section.
    */
	(void) pthread_mutex_unlock (&BufferPoolLockSem);

    return  ERROR;
  }

  /* Allocate a buffer
  */

  BufferPoolList[pool_id].free_count--;
  descr = BufferPoolList[pool_id].free_list[BufferPoolList[pool_id].free_count];
  BufferPoolList[pool_id].num_allocs++;
  current_alloc = BufferPoolList[pool_id].total - BufferPoolList[pool_id].free_count;
  if (current_alloc > BufferPoolList[pool_id].high_watermark)
  {
    BufferPoolList[pool_id].high_watermark = current_alloc;
  }

  (void) pthread_mutex_unlock (&BufferPoolLockSem);

  /* <<<<<<<<<<<<<<<< End Critical Section
  */

  /* Make sure that the buffer is not corrputed.
  */
  if (descr->in_use)
  {
    LOG_ERROR ((uint32) ((unsigned long) descr));
  }


  descr->in_use = 1; /* Mark this buffer "In Use" */

  *buffer_addr = &descr->data[0];

  return  SUCCESS;
}


/*********************************************************************
*
* @purpose  Return a buffer to the buffer pool.
*
* @param   buffer_pool_id - Buffer pool from which to allocate buffer.
* @param   buffer_addr -  Address of the buffer.
*
* @returns  None
*
* @notes
*      Function calls LOG_ERROR if buffer is corrupted or returned
*      to the wrong pool.
*
* @end
*********************************************************************/
void bufferPoolFree (uint32 buffer_pool_id,  uchar8 * buffer_addr)
{
  bufferDescrType * descr;
  uint32 pool_id;

  if (buffer_addr == NULL)
  {
       LOGF( LOG_SEVERITY_INFO,
              "bufferPoolFree: ID %d, buffer_addr NULL!\n", buffer_pool_id);
      return;
  }

  pool_id = buffer_pool_id -  LOW_BUFFER_POOL_ID;

  /* Verify that the buffer pool ID is valid.
  */
  if (pool_id >= MaxBufferPools)
  {
    LOG_ERROR_OPT_RESET( LOG_SEVERITY_ERROR,
                         buffer_pool_id, 
                         "The buffer poolID is not valid. "
                         "The system may be in inconsistent state. "
                         "Recommend rebooting the system now.");
    return;
  }

  if ((BufferPoolList[pool_id].pool_size == 0) ||
      (BufferPoolList[pool_id].id != buffer_pool_id))
  {
    LOG_ERROR_OPT_RESET( LOG_SEVERITY_ERROR,
                         buffer_pool_id, 
                         "The buffer poolsize is 0 or poolID is not correct. "
                         "The system may be in inconsistent state. "
                         "Recommend rebooting the system now.");
    return;
  }

  if (BufferPoolList[pool_id].free_count >=
      BufferPoolList[pool_id].total )
  {
    LOG_ERROR_OPT_RESET( LOG_SEVERITY_ERROR,
                         buffer_pool_id, 
                         "The buffer pool freecount is . "
                         "greater than or equal to total buffer pool count. "
                         "The system may be in inconsistent state. "
                         "Recommend rebooting the system now.");
    return;
  }

  /* Set up a pointer to the buffer descriptor.
  */
  descr = (bufferDescrType *) (((unsigned char *) buffer_addr) - sizeof (bufferDescrType));

  /* Verify that the buffer belongs to this pool and is not
  ** corrupted.
  */
  if (descr->in_use == 0)
  {
     LOGF( LOG_SEVERITY_ERROR,
            "Trying to free buffer at address %p (descriptor at %p) that has already been freed. "
            "The system may be in an inconsistent state. "
            "Recommend rebooting the system now.", buffer_addr, descr);
    return;
  }

  if (descr->id != ( ushort16) buffer_pool_id)
  {
    LOG_ERROR_OPT_RESET( LOG_SEVERITY_ERROR,
                         (uint32) ((unsigned long) descr), 
                         "The buffer pool ID is not same as descriptor ID . "
                         "The system may be in inconsistent state. "
                         "Recommend rebooting the system now.");
    return;
  }


  /* Looks like the buffer pool and the buffer are OK.
  ** Return the buffer into the pool.
  */

  descr->in_use = 0;

  /*>>>>>>>>>>>>>>>>> Start Critical Section
  */
  (void) pthread_mutex_lock (&BufferPoolLockSem);

  BufferPoolList[pool_id].free_list[BufferPoolList[pool_id].free_count] = descr;
  BufferPoolList[pool_id].free_count++;

  (void) pthread_mutex_unlock (&BufferPoolLockSem);
  /* <<<<<<<<<<<<<<<< End Critical Section
  */

  return;
}

/*********************************************************************
*
* @purpose  Gets the number of free and used buffers in the bufferpool 
*
* @param   buffer_pool_id - (output) ID of buffer pool
*          free_buffs     - Number of free buffers in the pool
*
* @returns   SUCCESS, if success
*
* @notes
*      Function calls LOG_ERROR if buffer is corrupted
*
* @end
*********************************************************************/
RC_t bufferPoolBuffInfoGet(uint32 buffer_pool_id, uint32 *free_buffs)
{
  uint32 pool_id;
  pool_id = buffer_pool_id -  LOW_BUFFER_POOL_ID;
  /* Verify that the buffer pool ID is valid.
  */
  if (pool_id >= MaxBufferPools)
  {
    LOG_ERROR (buffer_pool_id);
  }

  if ((BufferPoolList[pool_id].pool_size == 0) ||
      (BufferPoolList[pool_id].id != buffer_pool_id))
  {
    LOG_ERROR (buffer_pool_id);
  }
  *free_buffs = BufferPoolList[pool_id].free_count;
  return  SUCCESS;
}




/*********************************************************************
*
* @purpose  Gets the pool ID that a buffer belongs to
*
* @param   buffer_addr -  Address of the buffer.
* @param   buffer_pool_id - (output) ID of buffer pool
*
* @returns   SUCCESS, if success
*
* @notes
*      Function calls LOG_ERROR if buffer is corrupted
*
* @end
*********************************************************************/
RC_t bufferPoolIdGet ( uchar8 * buffer_addr, uint32 *buffer_pool_id)
{
  bufferDescrType * descr;

  /* Set up a pointer to the buffer descriptor.
  */
  descr = (bufferDescrType *) (((unsigned char *) buffer_addr) - sizeof (bufferDescrType));

  /* Verify that the buffer belongs to this pool and is not
  ** corrupted.
  */
  if (descr->in_use == 0)
  {
    LOG_ERROR ((uint32) ((unsigned long) descr));
  }


  *buffer_pool_id = (uint32) descr->id;

  return  SUCCESS;
}

/*********************************************************************
*
* @purpose  Tell the caller how much memory is needed for a
*           given number of buffers with a given buffer size.
*           The information may be used to figure out how much
*           memory to allocate before calling bufferPoolCreate()
*
* @param   num_buffers - Number of buffers desired in the pool.
* @param   buffer_size - Size of buffers in the pool.
*
* @returns  Required number of bytes for specified buffer pool attributes.
*
* @notes
*
* @end
*********************************************************************/
uint32 bufferPoolSizeCompute (uint32 num_buffers,
                                 uint32 buffer_size)
{
  uint32 mem_size, tot_buf_size;

  /* If buffer size is not a multiple of eight then round it up.
  */
  if ((buffer_size & 0xFFFFFFF8) != buffer_size)
  {
    buffer_size += 8;
    buffer_size &= 0xFFFFFFF8;
  }

  /* Compute how much memory is required for each buffer. The overhead
  ** includes buffer descriptor and a pointer in the free list.
  */
  tot_buf_size = buffer_size +
                 sizeof (bufferDescrType) +
                 sizeof (void *);

  mem_size = tot_buf_size * num_buffers;

  return mem_size;
}


/*********************************************************************
*
* @purpose  Show information about all buffer pools.
*
* @param   buffer_pool_id - 0 - Show detailed information about all buffers
*                           1 - Show summary information about all buffers
*                           X - Show detailed information about buffer pool X
*
* @returns  None
*
* @notes
*     This is a debug function that can be called only through devshell.
*
* @end
*********************************************************************/
void bufferPoolShow (uint32 buffer_pool_id)
{
  uint32 i;
  bufferPoolType *pool;

  sysapiPrintf("\nTotal Buffer Pools: %d.\n",
         NumBufferPools);


  for (i = 0; i < MaxBufferPools; i++)
  {
    /* Don't display information about deleted buffer pools.
    */
    if (BufferPoolList[i].pool_size == 0)
    {
      continue;
    }
   
    pool = &BufferPoolList[i];

    if ((buffer_pool_id != 0) && (buffer_pool_id != pool->id))
    {
      continue;
    }
    sysapiPrintf("------\n");
    sysapiPrintf("Pool ID: %d, Pool Address: %p, Pool Size: %d, Description: %s \n",
           pool->id,
           pool->addr,
           pool->pool_size,
           pool->descr);
    sysapiPrintf("Tot. Buffs: %d, Free Buffs: %d, Buff. Size: %d, Num Allocs: %d, Num Empty: %d High watermark: %d\n",
           pool->total,
           pool->free_count,
           pool->buf_size,
           pool->num_allocs,
           pool->no_buffers_count,
           pool->high_watermark);
  }
}


/*********************************************************************
*
* @purpose  Reduce the number of buffers available in the buffer pool.
*           This function is intended to force an out-of-buffer condition
*           in order to unit-test applications.
*
* @param   buffer_pool_id - Buffer for which to set the new size.
* @param   buffer_pool_num - New number of buffers in the pool.
*
*
* @returns   SUCCESS, New buffer pool size has been set.
* @returns   ERROR, Invalid size for the specified buffer pool.
* @returns   FAILURE, Invalid pool ID.
*
* @notes
*     This is a debug function that can be called only through devshell.
*     The specified buffer pool size must be less or equal to the
*     initial buffer pool size setting.
*     Also the new buffer size may not be set to 0.
*
* @end
*********************************************************************/
uint32 bufferPoolSizeSet (uint32 buffer_pool_id,
                             uint32 buffer_pool_num)
{
  uint32 pool_id;

  pool_id = buffer_pool_id -  LOW_BUFFER_POOL_ID;

  /* Verify that the buffer pool ID is valid.
  */
  if (pool_id >= MaxBufferPools)
  {
    return  FAILURE;
  }

  if ((BufferPoolList[pool_id].pool_size == 0) ||
      (BufferPoolList[pool_id].id != buffer_pool_id))
  {
    return  FAILURE;
  }

  if (buffer_pool_num > BufferPoolList[pool_id].total)
  {
    return  ERROR;
  }

  if (buffer_pool_num <= 0)
  {
    return  ERROR;
  }

  /* Set the new floor for the buffer pool.
  */
  BufferPoolList[pool_id].floor = BufferPoolList[pool_id].total -
                                  buffer_pool_num;


  return  SUCCESS;
}



/*************************************************************************
**************************************************************************
Temporary test functions.
**************************************************************************
*************************************************************************/
#if 1
/* Create a buffer pool
** Allocate a buffer
** Delete a buffer pool.
*/
void bpool1 (void)
{
  RC_t rc;
  char    *pool_area;
  int     pool_size = 1000;
  int     pool_id;
  int     buff_count;
   uchar8    *buffer_addr;

  pool_area = malloc (pool_size);

  rc = bufferPoolCreate (pool_area,
                         pool_size,
                         128,     /* Buffer Size */
                         "First Pool",
                         &pool_id,
                         &buff_count);


  printf("bpool1: Create - rc = %d, id = %d, count = %d\n",
         rc, pool_id, buff_count);

  rc = bufferPoolAllocate (pool_id,  &buffer_addr);


  printf("bpool1: Allocate - rc = %d, addr = %p\n",
         rc,  buffer_addr);

  bufferPoolShow (0);

  bufferPoolFree (pool_id, buffer_addr);

  rc = bufferPoolDelete (pool_id);

  printf("bpool1: Delete Pool - rc = %d\n",
         rc);

  free (pool_area);

  bufferPoolShow (0);
}

/* Create a buffer pool
** Attempt to allocate more buffers than available.
** Delete a buffer pool.
*/
void bpool2 (void)
{
  RC_t rc;
  char    *pool_area;
  int     pool_size;
  int     pool_id;
  int     buff_count;
   uchar8    *buffer_addr;

  pool_size = bufferPoolSizeCompute (7, 127);

  pool_area = malloc (pool_size);

  rc = bufferPoolCreate (pool_area,
                         pool_size,
                         127,     /* Buffer Size */
                         "Second Pool",
                         &pool_id,
                         &buff_count);


  printf("bpool2: Create - rc = %d, id = %d, count = %d\n",
         rc, pool_id, buff_count);



  do
  {
    rc = bufferPoolAllocate (pool_id,  &buffer_addr);
    printf("bpool2: Allocate - rc = %d, addr = %p\n",
           rc,  buffer_addr);

    bufferPoolShow (0);

  } while (rc ==  SUCCESS);


  rc = bufferPoolDelete (pool_id);

  printf("bpool2: Delete Pool - rc = %d\n",
         rc);

  free (pool_area);

  bufferPoolShow (0);
}

/* Create maximum number of buffer pools.
** Attempt to allocate another buffer pool.
** Delete a buffer pool.
** Create a buffer pool.
*/
void bpool3 (void)
{
  RC_t rc;
  char    *pool_area[ MAX_BUFFER_POOLS + 1];
  int     pool_size;
  int     buff_count;
  uint32 pool_id[ MAX_BUFFER_POOLS + 1];
  int i;

  pool_size = bufferPoolSizeCompute (7, 127);

  /* Create MAX pools.
  */
  rc =  FAILURE;
  for (i = 0; i <  MAX_BUFFER_POOLS; i++)
  {
    pool_area[i] = malloc (pool_size);

    rc = bufferPoolCreate (pool_area[i],
                           pool_size,
                           127,     /* Buffer Size */
                           "Third Pool",
                           &pool_id[i],
                           &buff_count);


    if (rc !=  SUCCESS)
      break;
  }

  if (rc ==  SUCCESS)
  {
    printf("bpool3: Successfully allocated %d buffer pools.\n", i);
  }

  /* Try to allocate another buffer pool. This should fail.
  */
  rc = bufferPoolCreate (pool_area[i],
                         pool_size,
                         127,     /* Buffer Size */
                         "Third Pool",
                         &pool_id[i],
                         &buff_count);
  printf("bpool3: Allocate too many pools, rc = %d\n", rc);


  /* Delete one pool at random.
  */
  rc = bufferPoolDelete (pool_id[20]);

  printf("bpool3: Delete Pool - rc = %d\n",
         rc);

  /* Create another pool. This should work.
  */
  rc = bufferPoolCreate (pool_area[20],
                         pool_size,
                         127,     /* Buffer Size */
                         "Third Pool",
                         &pool_id[20],
                         &buff_count);
  printf("bpool3: Created another pool, rc = %d\n", rc);

  bufferPoolShow (0);


  /* Delete all pools
  */
  rc =  FAILURE;
  for (i = 0; i <  MAX_BUFFER_POOLS; i++ )
  {
    rc = bufferPoolDelete (pool_id[i]);
    if (rc !=  SUCCESS)
      break;

    free (pool_area[i]);
  }

  if (rc ==  SUCCESS)
  {
    printf("bpool3: Successfuly deleted %d pools.\n", i);
  }


  bufferPoolShow (0);
}


/* Create a buffer pool
** Allocate all buffers buffer
** Fill buffers with data.
** Return buffers in reverse order.
** Delete a buffer pool.
*/
void bpool4 (void)
{
  RC_t rc;
  char    *pool_area;
  int     pool_size = 1000;
  int     pool_id;
  int     buff_count;
   uchar8    *buffer_addr[10];
  int i, j;

  pool_area = malloc (pool_size);

  rc = bufferPoolCreate (pool_area,
                         pool_size,
                         128,     /* Buffer Size */
                         "Fourth Pool",
                         &pool_id,
                         &buff_count);


  printf("bpool4: Create - rc = %d, id = %d, count = %d\n",
         rc, pool_id, buff_count);

  for (j = 0; j < 2; j++)
  {
    printf("\nbpool4: >>>>>>>>> Iteration %d\n", j);

    /* Allocate all buffers from the pool, set content to loop index.
    */
    for (i = 0; i < buff_count; i++)
    {
      rc = bufferPoolAllocate (pool_id,  &buffer_addr[i]);


      printf("bpool4: Allocate - rc = %d, addr = %p\n",
             rc,  buffer_addr[i]);

      bufferPoolShow (0);

      memset (buffer_addr[i], (char) i, 128);
    }

    /* Free buffers.
    */
    for (i = 0; i < buff_count; i++)
    {
      bufferPoolFree (pool_id, buffer_addr[i]);

      bufferPoolShow (0);
    };


  }

  rc = bufferPoolDelete (pool_id);
  printf("bpool4: Delete Pool - rc = %d\n",
         rc);

  free (pool_area);

  bufferPoolShow (0);
};


/* Create a buffer pool
** Reduce the size of the buffer pool.
** Attempt to allocate more buffers than available.
** Delete a buffer pool.
*/
void bpool5 (void)
{
  RC_t rc;
  char    *pool_area;
  int     pool_size;
  int     pool_id;
  int     buff_count;
   uchar8    *buffer_addr;

  pool_size = bufferPoolSizeCompute (7, 127);

  pool_area = malloc (pool_size);

  rc = bufferPoolCreate (pool_area,
                         pool_size,
                         127,     /* Buffer Size */
                         "Second Pool",
                         &pool_id,
                         &buff_count);


  printf("bpool5: Create - rc = %d, id = %d, count = %d\n",
         rc, pool_id, buff_count);

  bufferPoolShow (0);

  rc = bufferPoolSizeSet (pool_id, (buff_count - 2));

  printf("bpool5: Reduce size by 2 - rc = %d,\n",
         rc);

  bufferPoolShow (0);
  do
  {
    rc = bufferPoolAllocate (pool_id,  &buffer_addr);
    printf("bpool5: Allocate - rc = %d, addr = %p\n",
           rc,  buffer_addr);

    bufferPoolShow (0);

  } while (rc ==  SUCCESS);


  rc = bufferPoolDelete (pool_id);

  printf("bpool5: Delete Pool - rc = %d\n",
         rc);

  free (pool_area);

  bufferPoolShow (0);
}


#endif
