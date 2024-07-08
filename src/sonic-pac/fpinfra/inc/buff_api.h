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

#ifndef _BUFF_API_H_
#define _BUFF_API_H_

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

//#include "commdefs.h"
//#include "datatypes.h"

/*********************************************************************
* @purpose  Allocates and creates a buffer pool 
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

* @returns   SUCCESS
* @returns   FAILURE
* @returns   ERROR
*
* @notes   This routine both allocates the memory for a buffer pool and creates
* @notes   the buffer pool. It is preferred over bufferPoolCreate().
*
* @end
*********************************************************************/
EXT_API uint32 bufferPoolInit ( COMPONENT_IDS_t comp_id, uint32 num_buffers,
                          uint32 buffer_size,  char8  *description,
                          uint32 * buffer_pool_id);


/*********************************************************************
*
* @purpose  Create a buffer pool.
*
* @param   buffer_pool_addr - Buffer pool memory address. 
*                             This memory must be statically or dynamically
*                             allocated by the caller. 
* @param   buffer_pool_size - Number of bytes in the buffer_pool_addr. 
* @param   buffer_size - Size of buffers in the pool. The buffer size is rounded
*                        up to the next multiple of four.
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
                            uint32 * buffer_count);




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
EXT_API RC_t bufferPoolTerminate ( COMPONENT_IDS_t comp_id, uint32  buffer_pool_id);


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
*
* @end
*********************************************************************/
  RC_t bufferPoolDelete (uint32 buffer_pool_id);


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
EXT_API RC_t bufferPoolAllocate (uint32 buffer_pool_id,
                               uchar8 ** buffer_addr);


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
EXT_API void bufferPoolFree (uint32 buffer_pool_id,  uchar8 * buffer_addr);

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
RC_t bufferPoolIdGet ( uchar8 * buffer_addr, uint32 *buffer_pool_id);


/*********************************************************************
*
* @purpose  Gets the number of free buffers in the bufferpool
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
EXT_API RC_t bufferPoolBuffInfoGet(uint32 pool_id, uint32 *free_buffs);

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
                                   uint32 buffer_size);

/*********************************************************************
*
* @purpose  Show information about buffer pools.
*
* @param   buffer_pool_id - 0 - Show detailed information about all buffers
*                           1 - Show summary information about all buffers
*                           X - Show detailed information about buffer pool X
*                                     
*
* @returns  None
*
* @notes  
*     This is a debug function that can be called only through devshell.
*
* @end
*********************************************************************/
  void bufferPoolShow (uint32 buffer_pool_id);


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
                               uint32 buffer_pool_num);


  /* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* _BUFF_API_H_ */

