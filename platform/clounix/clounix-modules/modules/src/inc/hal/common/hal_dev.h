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

/* FILE NAME:  hal_dev.h
 * PURPOSE:
 *  Provide a list of device IDs.
 *
 * NOTES:
 */

#ifndef HAL_DEV_H
#define HAL_DEV_H

/* INCLUDE FILE DECLARATIONS
 */
/* NAMING CONSTANT DECLARATIONS
 */
#define HAL_CLX_VENDOR_ID           (0x0E8D)
#define HAL_CL_VENDOR_ID            (0x1D9F)

#define HAL_DEVICE_ID_CL3257        (0x3257)
#define HAL_DEVICE_ID_CL3258        (0x3258)

#define HAL_DEVICE_ID_CL8300        (0x8300) /* chip family */
#define HAL_DEVICE_ID_CL8363        (0x8363) /* 1.08T 1Bin */
#define HAL_DEVICE_ID_CL8365        (0x8365) /* 1.8T 1Bin */
#define HAL_DEVICE_ID_CL8366        (0x8366) /* 2.4T 1Bin */
#define HAL_DEVICE_ID_CL8367        (0x8367) /* 3.2T 1Bin */
#define HAL_DEVICE_ID_CL8368        (0x8368) /* 3.2T 2Bin */
#define HAL_DEVICE_ID_CL8369        (0x8369) /* 6.4T 2Bin */

#define HAL_DEVICE_ID_CL8500        (0x8500) /* chip family */
#define HAL_DEVICE_ID_CL8571        (0x8571) /* 3.2T 2Bin */
#define HAL_DEVICE_ID_CL8573        (0x8573) /* 3.2T 2Bin */
#define HAL_DEVICE_ID_CL8575        (0x8575) /* 4.0T 4Bin */
#define HAL_DEVICE_ID_CL8577        (0x8577) /* 6.4T 4Bin */
#define HAL_DEVICE_ID_CL8578        (0x8578) /* 8.0T 4Bin */
#define HAL_DEVICE_ID_CL8579        (0x8579) /* 12.8T 4Bin */

#define HAL_REVISION_ID_E1          (0x01)
#define HAL_REVISION_ID_E2          (0x02)

#define HAL_INVALID_DEVICE_ID       (0xFFFFFFFF)

#endif  /* #ifndef HAL_DEV_H */
