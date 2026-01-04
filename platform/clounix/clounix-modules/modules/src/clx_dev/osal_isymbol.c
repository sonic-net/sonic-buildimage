/*
 * Copyright 2022 Clounix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 */

/* FILE NAME:  osal_isymbol.c
* PURPOSE:
*      It provide global OSAL symbol export for linux kernel module
* NOTES:
*/
#include <linux/init.h>
#include <linux/module.h>

/* ----------------------------------------------------- */
#include <osal/osal_mdc.h>
/* dma */
extern struct pci_dev   *_ptr_ext_pci_dev;
EXPORT_SYMBOL(_ptr_ext_pci_dev);

#if defined(CLX_LINUX_KERNEL_MODE)
#include <clx_init.h>
extern CLX_INIT_WRITE_FUNC_T   _ext_dsh_write_func;
EXPORT_SYMBOL(_ext_dsh_write_func);
#endif

#if defined(CLX_LINUX_USER_MODE)
EXPORT_SYMBOL(osal_mdc_readPciReg);
EXPORT_SYMBOL(osal_mdc_writePciReg);
#if defined(CLX_EN_NETIF)
/* intr */
/* for kernel module, this API will be exported by script with other OSAL functions in osal_symbol.c */
EXPORT_SYMBOL(osal_mdc_registerIsr);
#endif
#endif
