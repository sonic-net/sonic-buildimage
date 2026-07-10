/* Copyright (C) 2020  MediaTek, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program.
 */

/* FILE NAME:  osal_isymbol.c
* PURPOSE:
*      It provide global OSAL symbol export for linux kernel module
* NOTES:
*/
#include <linux/init.h>
#include <linux/module.h>

/* ----------------------------------------------------- */
#include <osal_mdc.h>
/* dma */
extern struct pci_dev   *_ptr_ext_pci_dev;
EXPORT_SYMBOL(_ptr_ext_pci_dev);

#if defined(NPS_LINUX_USER_MODE)
EXPORT_SYMBOL(osal_mdc_readPciReg);
EXPORT_SYMBOL(osal_mdc_writePciReg);
#if defined(NPS_EN_NETIF)
/* intr */
/* for kernel module, this API will be exported by script with other OSAL functions in osal_symbol.c */
EXPORT_SYMBOL(osal_mdc_registerIsr);
#endif
#endif
