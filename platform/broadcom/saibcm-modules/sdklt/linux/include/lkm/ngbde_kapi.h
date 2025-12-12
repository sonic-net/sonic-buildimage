/*! \file ngbde_kapi.h
 *
 * NGBDE kernel API.
 *
 * This file is intended for use by other kernel modules relying on the BDE.
 *
 */
/*
 *
 * Copyright 2018-2025 Broadcom. All rights reserved.
 * The term 'Broadcom' refers to Broadcom Inc. and/or its subsidiaries.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * A copy of the GNU General Public License version 2 (GPLv2) can
 * be found in the LICENSES folder.
 */

#ifndef NGBDE_KAPI_H
#define NGBDE_KAPI_H

#include <linux/types.h>

/*! Maximum number of switch devices supported. */
#ifndef NGBDE_NUM_SWDEV_MAX
#define NGBDE_NUM_SWDEV_MAX     16
#endif

/*! Device has been removed. */
#define NGBDE_EVENT_DEV_REMOVE  1

/*!
 * \brief KNET handler.
 *
 * The KNET handler with the Linux kernel.
 *
 * \param [in] kdev Switch device number.
 * \param [in] event PCI event, see NGBDE_EVENT_xxx for event definitions.
 * \param [in] data KNET handler context.
 *
 */
typedef int (*knet_func_f)(int kdev, int event, void *data);

/*!
 * \brief Get Linux PCI device handle for a switch device.
 *
 * \param [in] kdev Device number.
 *
 * \return Linux PCI device handle or NULL if unavailable.
 */
extern struct pci_dev *
ngbde_kapi_pci_dev_get(int kdev);

/*!
 * \brief Get Linux kernel device handle for a switch device.
 *
 * \param [in] kdev Device number.
 *
 * \return Linux kernel device handle or NULL if unavailable.
 */
extern struct device *
ngbde_kapi_dma_dev_get(int kdev);

/*!
 * \brief Convert DMA bus address to virtual address.
 *
 * This API will convert a physical DMA bus address to a kernel
 * virtual address for a memory location that belongs to one of the
 * DMA memory pools allocated by the BDE module.
 *
 * \param [in] kdev Device number.
 * \param [in] baddr Physical DMA bus address for this device.
 *
 * \return Virtual kernel address or NULL on error.
 */
extern void *
ngbde_kapi_dma_bus_to_virt(int kdev, dma_addr_t baddr);

/*!
 * \brief Convert virtual address to DMA bus address.
 *
 * This API will convert a kernel virtual address to a physical DMA
 * bus address for a memory location that belongs to one of the DMA
 * memory pools allocated by the BDE module.
 *
 * \param [in] kdev Device number.
 * \param [in] vaddr Virtual kernel address.
 *
 * \return Physical DMA bus address for this device or 0 on error.
 */
extern dma_addr_t
ngbde_kapi_dma_virt_to_bus(int kdev, void *vaddr);

/*!
 * \brief Allocate physically continguous memory.
 *
 * This function can be used to allocate a large physically contiguous
 * block of memory suitable for DMA operations.
 *
 * Use the kernel API dma_map_single to map the memory to a physical
 * device. A suitable DMA device for this operation can be obtained
 * via \ref ngbde_kapi_dma_dev_get.
 *
 * Memory should be freed via \ref ngbde_kapi_dma_free.
 *
 * \param [in] size Number of bytes to allocate.
 *
 * \return Pointer to allocated memory or NULL on error.
 */
extern void *
ngbde_kapi_dma_alloc(size_t size);

/*!
 * \brief Free physically continguous memory.
 *
 * Free memory previously allocated via \ref ngbde_kapi_dma_alloc.
 *
 * If the memory has been used for DMA operation, then it must first
 * be unmapped via the kernel API dma_unmap_single.
 *
 * \param [in] ptr Pointer to memory to be freed.
 *
 * \retval 0 No errors.
 * \retval -1 Invalid memory pointer.
 */
extern int
ngbde_kapi_dma_free(void *ptr);

/*!
 * \brief Write a memory-mapped register in kernel driver.
 *
 * \param [in] kdev Device number.
 * \param [in] offs Register address offset.
 * \param [in] val Value to write to register.
 *
 * \return Nothing.
 */
extern void
ngbde_kapi_pio_write32(int kdev, uint32_t offs, uint32_t val);

/*!
 * \brief Read a memory-mapped register in kernel driver.
 *
 * \param [in] kdev Device number.
 * \param [in] offs Register address offset.
 *
 * \return Value read from register.
 */
extern uint32_t
ngbde_kapi_pio_read32(int kdev, uint32_t offs);

/*!
 * \brief Get base address of memory-mapped I/O memory.
 *
 * The logical base address returned can be used with ioread32, etc.
 *
 * \param [in] kdev Device number.
 *
 * \return Logical base address or NULL if unavailable.
 */
extern void *
ngbde_kapi_pio_membase(int kdev);

/*!
 * \brief Write a memory-mapped interrupt controller register.
 *
 * \param [in] kdev Device number.
 * \param [in] offs Register address offset.
 * \param [in] val Value to write to register.
 *
 * \return Nothing.
 */
extern void
ngbde_kapi_iio_write32(int kdev, uint32_t offs, uint32_t val);

/*!
 * \brief Read a memory-mapped interrupt controller register.
 *
 * \param [in] kdev Device number.
 * \param [in] offs Register address offset.
 *
 * \return Value read from register.
 */
extern uint32_t
ngbde_kapi_iio_read32(int kdev, uint32_t offs);

/*!
 * \brief Get base address of memory-mapped interrupt controller memory.
 *
 * The logical base address returned can be used with ioread32, etc.
 *
 * \param [in] kdev Device number.
 *
 * \return Logical base address or NULL if unavailable.
 */
extern void *
ngbde_kapi_iio_membase(int kdev);

/*!
 * \brief Write a memory-mapped PCI bridge register.
 *
 * \param [in] kdev Device number.
 * \param [in] offs Register address offset.
 * \param [in] val Value to write to register.
 *
 * \return Nothing.
 */
extern void
ngbde_kapi_paxb_write32(int kdev, uint32_t offs, uint32_t val);

/*!
 * \brief Read a memory-mapped PCI bridge register.
 *
 * \param [in] kdev Device number.
 * \param [in] offs Register address offset.
 *
 * \return Value read from register.
 */
extern uint32_t
ngbde_kapi_paxb_read32(int kdev, uint32_t offs);

/*!
 * \brief Get base address of memory-mapped PCI bridge memory.
 *
 * The logical base address returned can be used with ioread32, etc.
 *
 * \param [in] kdev Device number.
 *
 * \return Logical base address or NULL if unavailable.
 */
extern void *
ngbde_kapi_paxb_membase(int kdev);

/*!
 * \brief Install kernel mode interrupt handler.
 *
 * \param [in] kdev Device number.
 * \param [in] irq_num Interrupt number (MSI vector).
 * \param [in] isr_func Interrupt handler function.
 * \param [in] isr_data Interrupt handler context.
 *
 * \retval 0 No errors
 */
extern int
ngbde_kapi_intr_connect(int kdev, unsigned int irq_num,
                        int (*isr_func)(void *), void *isr_data);

/*!
 * \brief Uninstall kernel mode interrupt handler.
 *
 * \param [in] kdev Device number.
 * \param [in] irq_num Interrupt number (MSI vector).
 *
 * \retval 0 No errors
 */
extern int
ngbde_kapi_intr_disconnect(int kdev, unsigned int irq_num);

/*!
 * \brief Write shared interrupt mask register.
 *
 * This function is used by an interrupt handler when a shared
 * interrupt mask register needs to be updated.
 *
 * Note that the mask register to access is referenced by the
 * corrsponding status register. This is because the mask register may
 * be different depending on the host CPU interface being used
 * (e.g. PCI vs. AXI). On the other hand, the status register is the
 * same irrespective of the host CPU interface.
 *
 * \param [in] kdev Device number.
 * \param [in] irq_num Interrupt number (MSI vector).
 * \param [in] status_reg Corresponding interrupt status register offset.
 * \param [in] mask_val New value to write to mask register.
 *
 * \retval 0 No errors
 * \retval -1 Something went wrong.
 */
extern int
ngbde_kapi_intr_mask_write(int kdev, unsigned int irq_num,
                           uint32_t status_reg, uint32_t mask_val);

/*!
 * \brief Install KNET callback handler.
 *
 * Register a callback function to handle BDE events on KNET.
 *
 * \param [in] kdev Device number.
 * \param [in] knet_func KNET callback function.
 * \param [in] knet_data Context of KNET callback function.
 *
 * \retval 0 No errors
 */
extern int
ngbde_kapi_knet_connect(int kdev, knet_func_f knet_func, void *knet_data);

/*!
 * \brief Uninstall KNET callback handler.
 *
 * \param [in] kdev Device number.
 *
 * \retval 0 No errors
 */
extern int
ngbde_kapi_knet_disconnect(int kdev);

#endif /* NGBDE_KAPI_H */
