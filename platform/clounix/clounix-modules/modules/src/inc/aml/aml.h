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

/* FILE NAME:  aml.h
 * PURPOSE:
 *  1. Provide whole AML resource initialization API.
 *  2. Provide configuration access APIs.
 *  3. Provide ISR registration and deregistration APIs.
 *  4. Provide memory access.
 *  5. Provide DMA management APIs.
 *  6. Provide address translation APIs.
 * NOTES:
 */

#ifndef AML_H
#define AML_H


/* INCLUDE FILE DECLARATIONS
 */
#include <clx_types.h>
#include <clx_error.h>


/* NAMING CONSTANT DECLARATIONS
 */

/* #define AML_EN_I2C             */
/* #define AML_EN_CUSTOM_DMA_ADDR */

/* MACRO FUNCTION DECLARATIONS
 */

/* DATA TYPE DECLARATIONS
 */
typedef enum
{
    AML_DEV_TYPE_PCI,
    AML_DEV_TYPE_I2C,
    AML_DEV_TYPE_SPI,
    AML_DEV_TYPE_LAST

} AML_HW_IF_T;

typedef CLX_ERROR_NO_T
(*AML_DEV_READ_FUNC_T)(
    const UI32_T    unit,
    const UI32_T    addr_offset,
    UI32_T          *ptr_data,
    const UI32_T    len);

typedef CLX_ERROR_NO_T
(*AML_DEV_WRITE_FUNC_T)(
    const UI32_T    unit,
    const UI32_T    addr_offset,
    const UI32_T    *ptr_data,
    const UI32_T    len);

typedef CLX_ERROR_NO_T
(*AML_DEV_ISR_FUNC_T)(
    void            *ptr_data);

/* To mask the chip interrupt in kernel interrupt routine. */
typedef struct
{
    UI32_T                  mask_addr;
    UI32_T                  mask_val;

} AML_DEV_ISR_DATA_T;

/* To read or write the HW-intf registers. */
typedef struct
{
    AML_DEV_READ_FUNC_T     read_callback;
    AML_DEV_WRITE_FUNC_T    write_callback;

} AML_DEV_ACCESS_T;

typedef struct
{
    UI32_T                  vendor;
    UI32_T                  device;
    UI32_T                  revision;

} AML_DEV_ID_T;


typedef struct
{
    AML_HW_IF_T             if_type;
    AML_DEV_ID_T            id;
    AML_DEV_ACCESS_T        access;

} AML_DEV_T;
#endif  /* #ifndef AML_H */
