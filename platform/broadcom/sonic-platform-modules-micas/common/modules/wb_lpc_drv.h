/*
 * A header definition for wb_lpc_drv driver
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

#ifndef __WB_LPC_DRV_H__
#define __WB_LPC_DRV_H__

#define LPC_IO_NAME_MAX_LEN (64)

typedef struct lpc_drv_device_s {
    char lpc_io_name[LPC_IO_NAME_MAX_LEN];
    int pci_domain;
    int pci_bus;
    int pci_slot;
    int pci_fn;
    int lpc_io_base;
    int lpc_io_size;
    int lpc_gen_dec;
    int device_flag;
} lpc_drv_device_t;

#endif
