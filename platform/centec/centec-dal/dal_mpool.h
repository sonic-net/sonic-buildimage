/**
 @file dal_mpool.h

 @author  Copyright (C) 2011 Centec Networks Inc.  All rights reserved.

 @date 2012-5-10

 @version v2.0

  This file contains the dma memory init, allocation and free APIs
*/

#ifndef _DMA_MPOOL_H
#define _DMA_MPOOL_H
#ifdef __cplusplus
extern "C" {
#endif

#define DAL_MPOOL_MAX_DESX_SIZE (1024*1024)

enum dal_mpool_type_e
{
    DAL_MPOOL_TYPE_USELESS,     /* just compatible with GB */
    DAL_MPOOL_TYPE_DESC,          /* dma mpool op for desc */
    DAL_MPOOL_TYPE_DATA           /* dma mpool op for data */
};
typedef enum dal_mpool_type_e dal_mpool_type_t;

struct dal_mpool_mem_s
{
    unsigned char* address;
    int size;
    int type;
    struct dal_mpool_mem_s* next;
};
typedef struct dal_mpool_mem_s dal_mpool_mem_t;

/**
 @brief This function is to alloc dma memory

 @param[in] size   size of memory

 @return NULL

*/
extern int
dal_mpool_init(unsigned char lchip);

extern int
dal_mpool_deinit(unsigned char lchip);

extern dal_mpool_mem_t*
dal_mpool_create(unsigned char lchip, void* base_ptr, int size);

extern void*
dal_mpool_alloc(unsigned char lchip, dal_mpool_mem_t* pool, int size, int type);

extern void
dal_mpool_free(unsigned char lchip, dal_mpool_mem_t* pool, void* addr);

extern int
dal_mpool_destroy(unsigned char lchip, dal_mpool_mem_t* pool);

extern int
dal_mpool_usage(dal_mpool_mem_t* pool, int type);

extern int
dal_mpool_debug(dal_mpool_mem_t* pool);
#ifdef __cplusplus
}
#endif

#endif /* !_DMA_MPOOL_H */

