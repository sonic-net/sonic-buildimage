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
#include <nps_types.h>
#include <nps_error.h>
#include <nps_cfg.h>


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

typedef NPS_ERROR_NO_T
(*AML_DEV_READ_FUNC_T)(
    const UI32_T    unit,
    const UI32_T    addr_offset,
    UI32_T          *ptr_data,
    const UI32_T    len);

typedef NPS_ERROR_NO_T
(*AML_DEV_WRITE_FUNC_T)(
    const UI32_T    unit,
    const UI32_T    addr_offset,
    const UI32_T    *ptr_data,
    const UI32_T    len);

typedef NPS_ERROR_NO_T
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

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */

/* FUNCTION NAME:   aml_getRunMode
 * PURPOSE:
 *      To get current SDK running mode.
 * INPUT:
 *      unit        -- the device unit
 * OUTPUT:
 *      ptr_mode    -- current running mode
 * RETURN:
 *      NPS_E_OK    -- Successfully get the running mode.
 * NOTES:
 *      none
 */
NPS_ERROR_NO_T
aml_getRunMode(
    const UI32_T     unit,
    UI32_T           *ptr_mode);

/* FUNCTION NAME:   aml_init
 * PURPOSE:
 *      To initialize the DMA memory and interface-related kernel source
 *      such as PCIe/I2C/SPI.
 * INPUT:
 *      none
 * OUTPUT:
 *      none
 * RETURN:
 *      NPS_E_OK        -- Successfully initialize AML module.
 *      NPS_E_OTHERS    -- Failed to initialize AML module.
 * NOTES:
 *      none
 */
NPS_ERROR_NO_T
aml_deinit(void);

/* FUNCTION NAME:   aml_init
 * PURPOSE:
 *      To initialize the DMA memory and interface-related kernel source
 *      such as PCIe/I2C/SPI.
 * INPUT:
 *      none
 * OUTPUT:
 *      none
 * RETURN:
 *      NPS_E_OK        -- Successfully initialize AML module.
 *      NPS_E_OTHERS    -- Failed to initialize AML module.
 * NOTES:
 *      none
 */
NPS_ERROR_NO_T
aml_init(void);

/* FUNCTION NAME:   aml_getNumberOfChip
 * PURPOSE:
 *      To get the number of chips connected to host CPU.
 * INPUT:
 *      none
 * OUTPUT:
 *      ptr_num     -- pointer for the chip number
 * RETURN:
 *      NPS_E_OK    -- Successfully get the number of chips.
 * NOTES:
 *      none
 */
NPS_ERROR_NO_T
aml_getNumberOfChip(
    UI32_T  *ptr_num);

/* FUNCTION NAME:   aml_connectIsr
 * PURPOSE:
 *      To enable the system intterupt and specify the ISR handler.
 * INPUT:
 *      unit        -- the device unit
 *      handler     -- the ISR hanlder
 *      ptr_cookie  -- pointer for the data as an argument of the handler
 * OUTPUT:
 *      none
 * RETURN:
 *      NPS_E_OK     -- Successfully connect the ISR handler to the system.
 *      NPS_E_OTHERS -- Failed to connect the ISR handler to the system.
 * NOTES:
 *      none
 */
NPS_ERROR_NO_T
aml_connectIsr(
    const UI32_T        unit,
    AML_DEV_ISR_FUNC_T  handler,
    AML_DEV_ISR_DATA_T  *ptr_cookie);

/* FUNCTION NAME:   aml_disconnectIsr
 * PURPOSE:
 *      To disable the system intterupt notification.
 * INPUT:
 *      unit        -- the device unit
 * OUTPUT:
 *      none
 * RETURN:
 *      NPS_E_OK     -- Successfully disconnect the ISR handler to the system.
 *      NPS_E_OTHERS -- Failed to disconnect the ISR handler to the system.
 * NOTES:
 *      none
 */
NPS_ERROR_NO_T
aml_disconnectIsr(
    const UI32_T unit);

/* FUNCTION NAME:   aml_getDeviceId
 * PURPOSE:
 *      To get the vendor/device/revision ID of the specified chip unit.
 * INPUT:
 *      unit        -- the device unit
 * OUTPUT:
 *      ptr_vendor_id   -- pointer for the vendor ID
 *      ptr_device_id   -- pointer for the device ID
 *      ptr_revision_id -- pointer for the revision ID
 * RETURN:
 *      NPS_E_OK     -- Successfully get the IDs.
 *      NPS_E_OTHERS -- Failed to get the IDs.
 * NOTES:
 *      none
 */
NPS_ERROR_NO_T
aml_getDeviceId(
    const UI32_T    unit,
    UI32_T          *ptr_vendor_id,
    UI32_T          *ptr_device_id,
    UI32_T          *ptr_revision_id);

/* FUNCTION NAME:   aml_readReg
 * PURPOSE:
 *      To read data from the register of the specified chip unit.
 * INPUT:
 *      unit        -- the device unit
 *      addr_offset -- the address of register
 *      len         -- data size read
 * OUTPUT:
 *      ptr_data    -- pointer for the register data
 * RETURN:
 *      NPS_E_OK     -- Successfully read the data.
 *      NPS_E_OTHERS -- Failed to read the data.
 * NOTES:
 *      none
 */
NPS_ERROR_NO_T
aml_readReg(
    const UI32_T    unit,
    const UI32_T    addr_offset,
    UI32_T          *ptr_data,
    const UI32_T    len);

/* FUNCTION NAME:   aml_writeReg
 * PURPOSE:
 *      To write data to the register of the specified chip unit.
 * INPUT:
 *      unit        -- the device unit
 *      addr_offset -- the address of register
 *      ptr_data    -- pointer for the written data
 *      len         -- data size read
 * OUTPUT:
 *      none
 * RETURN:
 *      NPS_E_OK     -- Successfully write the data.
 *      NPS_E_OTHERS -- Failed to write the data.
 * NOTES:
 *      none
 */
NPS_ERROR_NO_T
aml_writeReg(
    const UI32_T    unit,
    const UI32_T    addr_offset,
    const UI32_T    *ptr_data,
    const UI32_T    len);

/* FUNCTION NAME:   aml_convertVirtToPhy
 * PURPOSE:
 *      To get the physical address of the corresponding virtual
 *      address input.
 * INPUT:
 *      ptr_virt_addr   -- pointer to the virtual address
 * OUTPUT:
 *      ptr_phy_addr    -- pointer to the physical address
 * RETURN:
 *      NPS_E_OK     -- Successfully convert the address.
 *      NPS_E_OTHERS -- Failed to convert the address.
 *                      The memory might be not allocated by AML.
 * NOTES:
 *      none
 */
NPS_ERROR_NO_T
aml_convertVirtToPhy(
    void            *ptr_virt_addr,
    NPS_ADDR_T      *ptr_phy_addr);

/* FUNCTION NAME:   aml_convertPhyToVirt
 * PURPOSE:
 *      To get the virtual address of the corresponding physical
 *      address input.
 * INPUT:
 *      ptr_virt_addr   -- pointer for the physical address
 * OUTPUT:
 *      pptr_virt_addr  -- pointer for the virtual address pointer
 * RETURN:
 *      NPS_E_OK     -- Successfully convert the address.
 *      NPS_E_OTHERS -- Failed to convert the address.
 *                      The memory might be not allocated by AML.
 * NOTES:
 *      none
 */
NPS_ERROR_NO_T
aml_convertPhyToVirt(
    const NPS_ADDR_T    phy_addr,
    void                **pptr_virt_addr);

/* FUNCTION NAME:   aml_flushCache
 * PURPOSE:
 *      To update the data from CPU cache to the physical memory.
 * INPUT:
 *      ptr_virt_addr   -- pointer for the data
 *      size            -- target data size to be updated
 * OUTPUT:
 *      none
 * RETURN:
 *      NPS_E_OK     -- Successfully update the data from CPU cache
 *                      to the physical memory.
 *      NPS_E_OTHERS -- Failed to pdate the data from CPU cache
 *                      to the physical memory.
 * NOTES:
 *      none
 */
NPS_ERROR_NO_T
aml_flushCache(
    void            *ptr_virt_addr,
    const UI32_T    size);

/* FUNCTION NAME:   aml_invalidateCache
 * PURPOSE:
 *      To update the data from physical memory to the CPU cache.
 * INPUT:
 *      ptr_virt_addr   -- pointer for the data
 *      size            -- target data size to be updated
 * OUTPUT:
 *      none
 * RETURN:
 *      NPS_E_OK     -- Successfully update the data from physical memory
 *                      to the CPU cache.
 *      NPS_E_OTHERS -- Failed to pdate the data from physical memory
 *                      to the CPU cache.
 * NOTES:
 *      none
 */
NPS_ERROR_NO_T
aml_invalidateCache(
    void            *ptr_virt_addr,
    const UI32_T    size);

#endif  /* #ifndef AML_H */
