/*
 * $Id: $
 *
 * $Copyright: 2017-2025 Broadcom Inc. All rights reserved.
 * 
 * Permission is granted to use, copy, modify and/or distribute this
 * software under either one of the licenses below.
 * 
 * License Option 1: GPL
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
 * 
 * 
 * License Option 2: Broadcom Open Network Switch APIs (OpenNSA) license
 * 
 * This software is governed by the Broadcom Open Network Switch APIs license:
 * https://www.broadcom.com/products/ethernet-connectivity/software/opennsa $
 * 
 * 
 *
 */

#ifndef __SHBDE_PCI_H__
#define __SHBDE_PCI_H__

#include <shbde.h>

extern unsigned int
shbde_pci_pcie_cap(shbde_hal_t *shbde, void *pci_dev);

extern int
shbde_pci_is_pcie(shbde_hal_t *shbde, void *pci_dev);

extern int
shbde_pci_is_iproc(shbde_hal_t *shbde, void *pci_dev, int *cmic_bar);

extern int
shbde_pci_max_payload_set(shbde_hal_t *shbde, void *pci_dev, int maxpayload);

extern int
shbde_pci_iproc_version_get(shbde_hal_t *shbde, void *pci_dev,
                            unsigned int *iproc_ver,
                            unsigned int *cmic_ver,
                            unsigned int *cmic_rev);

#endif /* __SHBDE_PCI_H__ */
