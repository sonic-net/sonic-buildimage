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
#define HAL_MTK_VENDOR_ID           (0x0E8D)
#define HAL_NP_VENDOR_ID            (0x1D9F)

#define HAL_DEVICE_ID_MT3257        (0x3257)
#define HAL_DEVICE_ID_MT3258        (0x3258)

#define HAL_DEVICE_ID_NP8363        (0x8363) /* 1.08T 1Bin */
#define HAL_DEVICE_ID_NP8365        (0x8365) /* 1.8T 1Bin */
#define HAL_DEVICE_ID_NP8366        (0x8366) /* 2.4T 1Bin */
#define HAL_DEVICE_ID_NP8367        (0x8367) /* 3.2T 1Bin */
#define HAL_DEVICE_ID_NP8368        (0x8368) /* 3.2T 2Bin */
#define HAL_DEVICE_ID_NP8369        (0x8369) /* 6.4T 2Bin */

#define HAL_REVISION_ID_E1          (0x01)
#define HAL_REVISION_ID_E2          (0x02)

#define HAL_INVALID_DEVICE_ID       (0xFFFFFFFF)

#endif  /* #ifndef HAL_DEV_H */
