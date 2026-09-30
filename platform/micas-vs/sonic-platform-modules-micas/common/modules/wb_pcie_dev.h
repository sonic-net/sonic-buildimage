/*
 * A header definition for wb_pcie_dev driver
 *
 * Copyright (C) 2024 Micas Networks Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __WB_PCIE_DEV_H__
#define __WB_PCIE_DEV_H__
#include <linux/string.h>

#define mem_clear(data, size) memset((data), 0, (size))

#define UPG_TYPE 'U'
#define GET_FPGA_UPG_CTL_BASE              _IOR(UPG_TYPE, 0, int)
#define GET_FPGA_UPG_FLASH_BASE            _IOR(UPG_TYPE, 1, int)

#define PCI_DEV_NAME_MAX_LEN (64)

typedef struct pci_dev_device_s {
    char pci_dev_name[PCI_DEV_NAME_MAX_LEN];
    int pci_domain;
    int pci_bus;
    int pci_slot;
    int pci_fn;
    int pci_bar;
    int bus_width;
    uint32_t check_pci_id;
    uint32_t pci_id;
    int upg_ctrl_base;
    int upg_flash_base;
    int device_flag;
    int search_mode;
    int bridge_bus;
    int bridge_slot;
    int bridge_fn;
} pci_dev_device_t;

#endif
