#ifndef __NB_DRV_H__
#define __NB_DRV_H__

#include "knet_dev.h"

int
nb_driver_init(uint32_t unit, clx_drv_cb_t **pptr_clx_drv);
int
nb_driver_deinit(uint32_t unit, clx_drv_cb_t **pptr_clx_drv);

#endif // __NB_DRV_H__