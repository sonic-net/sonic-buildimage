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



#ifndef _BUFF_H_
#define _BUFF_H_


#define  MAX_BUFFER_DESCR_SIZE 16

/* Lowest buffer pool ID.
*/
#define  LOW_BUFFER_POOL_ID  100


/* Header for each buffer 
*/
typedef struct 
{
   ushort16 id;  /* Buffer pool ID */

   ushort16 in_use; /* Flag indicating that the buffer is in use */

   uchar8 data[0];  /* Start of user data. This address is returned
                      ** to the caller of bufferPoolAllocate.
                      */
} bufferDescrType;

/* This structure is maintained for every buffer pool.
*/
typedef struct 
{
  uint32 id;     /* Buffer pool ID */
   uchar8 *addr;   /* Buffer pool address */
  uint32 pool_size; /* Buffer pool size */
  uint32 buf_size;  /* buffer size */
  uint32 total;    /* Total number of buffers in the pool */
   uchar8 descr [ MAX_BUFFER_DESCR_SIZE]; /* Buffer pool description */

  uint32 free_count; /* Number of free buffers in the pool */
  uint32 num_allocs; /* Number of allocations from this buffer pool */

  uint32 floor;  /* Last free buffer index. This variable is used normally set
                    ** to zero, but may be increased to simulate out of buffer scenario.
                    */

  uint32 no_buffers_count; /* Number of buffer requests from an empty pool */
  uint32 high_watermark; 

  bufferDescrType  **free_list; /* List of pointers to free buffers. */
} bufferPoolType;

#endif
