/*! \file ngbde.h
 *
 * Shared definitions and APIs for NGBDE kernel module.
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

#ifndef NGBDE_H
#define NGBDE_H

#include <lkm/lkm.h>
#include <lkm/ngbde_kapi.h>

/*! Module name. */
#define MOD_NAME        "linux_ngbde"

/*! Major number for associated charcter device file. */
#define MOD_MAJOR       120

/*! Read memory-mapped device register without byte-swap. */
#define NGBDE_IOREAD32(_a) __raw_readl(_a)

/*! Write memory-mapped device register without byte-swap. */
#define NGBDE_IOWRITE32(_v, _a) __raw_writel(_v, _a)

/*! Maximum number of I/O windows supported per device. */
#define NGBDE_NUM_IOWIN_MAX     3

/*! Maximum number of DMA memory pools supported per device. */
#define NGBDE_NUM_DMAPOOL_MAX   2

/*! Maximum number of IRQ status registers per interrupt source. */
#define NGBDE_NUM_IRQ_REGS_MAX  16

/*! Maximum number of IRQ lines (MSI vectors) per device. */
#define NGBDE_NUM_IRQS_MAX      16

/*!
 * Maximum number of interrupt controller registers which may be
 * written from both a user mode driver and a kernel mode driver.
 *
 * This feature is used when the kernel mode driver owns a subset of
 * bits within a register, which is also used by the user mode driver.
 *
 * Both drivers must access such registers through a lock-protected
 * access function.
 */
#define NGBDE_NUM_INTR_SHR_REGS_MAX  1

/*! I/O memory window definition. */
struct ngbde_memwin_s {

    /*! Physical address of I/O window. */
    phys_addr_t addr;

    /*! Size of I/O window (in bytes). */
    phys_addr_t size;
};

/*!
 * \brief Shared register value.
 *
 * This structure contains the current value of a register where user
 * mode and kernel mode owns different bits within the same
 * register. In this case access must be carefully controlled to avoid
 * that one context overwrites the bits owned by the other context.
 *
 * The structure also contains the offset of the shared register in
 * order to identify the register (in case there is more than one
 * shared register).
 */
typedef struct ngbde_shr_reg_s {

    /*! Offset of the shared register. */
    uint32_t reg_offs;

    /*! Current value of the shared register. */
    uint32_t cur_val;

} ngbde_shr_reg_t;

/*!
 * \brief Shared interrupt mask register control.
 *
 * This defines which bits of an interrupt mask register are owned by
 * user mode context, and which are owned by kernel context.
 *
 * The structure contains the corresponding interrupt status register
 * in order to allow identification of the interrupt mask register
 * irrespective of the host CPU being used.
 *
 * For example, if the host CPU is connected via PCI, then we use one
 * mask register, but if the host CPU is an embedded ARM CPU, then we
 * use a different mask register (for the same interrupt status
 * register). By using the status register to identify the shared mask
 * register, the kernel mode driver does not need to know which host
 * CPU it is running off.
 */
typedef struct ngbde_irq_reg_s {

    /*! Interrupt status register corresponding to the mask register. */
    uint32_t status_reg;

    /*! Interrupt status register is a bitwise AND of mask and raw status. */
    bool status_is_masked;

    /*! Shared interrupt mask register. */
    uint32_t mask_reg;

    /*! Mask register is of type "write 1 to clear". */
    bool mask_w1tc;

    /*!
     * Indicates that the kmask value is valid. This is mainly to
     * distinguish a mask value of zero from the mask value being
     * uninitialized, as this matters during a warm boot.
     */
    bool kmask_valid;

    /*! Mask identifying the register bits owned by the kernel mode driver. */
    uint32_t kmask;

    /*! Mask identifying the register bits owned by the user mode driver. */
    uint32_t umask;

} ngbde_irq_reg_t;

/*!
 * \name Interrupt ACK register domains.
 * \anchor NGBDE_INTR_ACK_IO_xxx
 */

/*! \{ */

/*! ACK registers reside in the default device I/O window. */
#define NGBDE_INTR_ACK_IO_DEV           0

/*! ACK registers reside in the interrupt controller I/O window. */
#define NGBDE_INTR_ACK_IO_INTR          1

/*! ACK registers reside in the PCI bridge I/O window. */
#define NGBDE_INTR_ACK_IO_PAXB          2

/*! \} */

/*!
 * \brief Interrupt ACK register control.
 *
 * The structure contains the corresponding register offset
 * and value in order to acknowledge interrupt in kernel driver.
 *
 * For example, if the host CPU is connected via PCI, then we use one
 * ACK register, but if the host CPU is an embedded ARM CPU, then we
 * use a different ACK register.
 */
typedef struct ngbde_intr_ack_reg_s {

    /*! ACK register information is valid. */
    bool ack_valid;

    /*! ACK register domain (\ref NGBDE_INTR_ACK_IO_xxx). */
    uint32_t ack_domain;

    /*! ACK register offset. */
    uint32_t ack_reg;

    /*! ACK value. */
    uint32_t ack_val;

} ngbde_intr_ack_reg_t;

/*!
 * \brief BDE interrupt handler.
 *
 * The BDE will use a function of this type to register an interrupt
 * handler with the Linux kernel.
 *
 * \param [in] data Interrupt handler context.
 *
 * \retval 0 Interrupt not recognized.
 * \retval 1 Interrupt recognized and handled.
 */
typedef int (*ngbde_isr_f)(void *data);

/*!
 * \brief Kernel interrupt control.
 *
 * This structure controls the sharing of interrupt processing between
 * a user mode thread and a kernel mode interrupt handler.
 */
typedef struct ngbde_intr_ctrl_s {

    /*! Handle for device I/O (for writing interrupt registers). */
    uint8_t *iomem;

    /*! Kernel device number (similar to user mode unit number). */
    int kdev;

    /*! Indicates that our interrupt handler is connected to the kernel. */
    int irq_active;

    /*! Interrupt number (IRQ# or MSI vector). */
    int irq_vect;

    /*! Number of interrupt status/mask register pairs. */
    int num_regs;

    /*! Interrupt status/mask register pairs for this device. */
    ngbde_irq_reg_t regs[NGBDE_NUM_IRQ_REGS_MAX];

    /*! Interrupt ACK register/value for this device. */
    ngbde_intr_ack_reg_t intr_ack;

    /*! Wait queue for user mode interrupt thread. */
    wait_queue_head_t user_thread_wq;

    /*! Flag to wake up user mode interrupt thread. */
    atomic_t run_user_thread;

    /*! Optional interrupt handler. */
    ngbde_isr_f isr_func;

    /*! Context for optional kernel interrupt handler. */
    void *isr_data;

    /*! Run kernel mode interrupt handler for this interrupt line. */
    bool run_kernel_isr;

    /*! Run user mode interrupt handler for this interrupt line. */
    bool run_user_isr;

} ngbde_intr_ctrl_t;

/*! Convenience macro for 1 kilobyte. */
#define ONE_KB 1024

/*! Convenience macro for 1 megabyte. */
#define ONE_MB (1024*1024)

/*!
 * \name DMA allocation types.
 * \anchor NGBDE_DMA_T_xxx
 */

/*! \{ */

/*!
 * Do not allocate any DMA memory.
 */
#define NGBDE_DMA_T_NONE        0

/*!
 * Try different allocation methods until DMA memory is successfully
 * allocated.
 */
#define NGBDE_DMA_T_AUTO        1

/*! Use kernel DMA API (dma_alloc_coherent). */
#define NGBDE_DMA_T_KAPI        2

/*! Use page allocator and map to physical address manually. */
#define NGBDE_DMA_T_PGMEM       3

/*! \} */

/*! DMA memory allocation control structure. */
typedef struct ngbde_dmactrl_s {

    /*! Requested size of DMA memory block (in bytes). */
    size_t size;

    /*! Kernel flags for memory allocation. */
    gfp_t flags;

    /*! Preferred DMA memory type (NGBDE_DMA_T_xxx). */
    int pref_type;

    /*! Kernel device for DMA memory management. */
    struct device *dev;

} ngbde_dmactrl_t;

/*! DMA memory descriptor. */
typedef struct ngbde_dmamem_s {

    /*! Logical address of DMA memory block. */
    void *vaddr;

    /*! Physical address of DMA memory block. */
    dma_addr_t paddr;

    /*! Bus address of DMA memory block. */
    dma_addr_t baddr;

    /*! Actual size of DMA memory block (in bytes). */
    size_t size;

    /*! Actual DMA memory type (NGBDE_DMA_T_xxx). */
    int type;

    /*! Kernel device for DMA memory management. */
    struct device *dev;

} ngbde_dmamem_t;

/*! DMA memory pool. */
typedef struct ngbde_dmapool_s {

    /*! DMA control parameters. */
    struct ngbde_dmactrl_s dmactrl;

    /*! DMA memory resources. */
    struct ngbde_dmamem_s dmamem;

} ngbde_dmapool_t;

/*!
 * \name MSI interrupt support.
 * \anchor NGBDE_MSI_T_xxx
 */

/*! \{ */

/*! Use legacy interrupts. */
#define NGBDE_MSI_T_NONE        0

/*! Use MSI interrupts. */
#define NGBDE_MSI_T_MSI         1

/*! Use MSI-X interrupts. */
#define NGBDE_MSI_T_MSIX        2

/*! \} */

/*! Switch device descriptor. */
struct ngbde_dev_s {

    /*! Vendor ID (typically PCI vendor ID). */
    uint16_t vendor_id;

    /*! Device ID (typically PCI device ID). */
    uint16_t device_id;

    /*! Device revision (typically PCI revision). */
    uint16_t revision;

    /*! Additional device identification when primary ID is not unique. */
    uint16_t model;

    /*! Domain number (typically PCI domain number). */
    int domain_no;

    /*! Bus number (typically PCI bus number). */
    int bus_no;

    /*! Slot number (typically PCI slot number). */
    int slot_no;

    /*! Interrupt line associated with this device. */
    int irq_line;

    /*! Number of available interrupt lines (typically MSI vectors). */
    int irq_max;

    /*! Number of active interrupt lines (typically MSI vectors). */
    int active_irqs;

    /*! Use MSI interrupts with this device (\ref NGBDE_MSI_T_xxx). */
    int use_msi;

    /*! Non-zero if device was removed. */
    int inactive;

    /*! Physical I/O window for kernel driver device access. */
    struct ngbde_memwin_s pio_win;

    /*! Memory mapped I/O window for kernel driver device access. */
    uint8_t *pio_mem;

    /*! Physical I/O window for interrupt controller access. */
    struct ngbde_memwin_s iio_win;

    /*! Memory mapped I/O window for interrupt controller access. */
    uint8_t *iio_mem;

    /*! Physical I/O window for device PCI bridge access. */
    struct ngbde_memwin_s paxb_win;

    /*! Memory mapped I/O window for device PCI bridge access. */
    uint8_t *paxb_mem;

    /*! Current value of shared register (typically an IRQ mask register). */
    struct ngbde_shr_reg_s intr_shr_reg[NGBDE_NUM_INTR_SHR_REGS_MAX];

    /*! Lock for shared register synchronization. */
    spinlock_t lock;

    /*! Interrupt control information. */
    struct ngbde_intr_ctrl_s intr_ctrl[NGBDE_NUM_IRQS_MAX];

    /*! Linux PCI handle. */
    struct pci_dev *pci_dev;

    /*! Kernel device for DMA memory management. */
    struct device *dma_dev;

    /*! Physical device I/O. */
    struct ngbde_memwin_s iowin[NGBDE_NUM_IOWIN_MAX];

    /*! DMA memory pools. */
    struct ngbde_dmapool_s dmapool[NGBDE_NUM_DMAPOOL_MAX];

    /*! KNET handler. */
    knet_func_f knet_func;

    /*! Context for KNET handler. */
    void *knet_data;

};

/*!
 * \brief Linux IOCTL handler.
 *
 * This function handles communication between user mode and kernel
 * mode.
 *
 * \param [in] file Device file handle.
 * \param [in] cmd IOCTL command.
 * \param [in] arg IOCTL command argument.
 *
 * \retval 0 No errors
 */
extern long
ngbde_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

/*!
 * \brief Initialize procfs for BDE driver.
 *
 * Create procfs read interface for dumping probe information.
 *
 * \return 0 if no errors, otherwise -1.
 */
extern int
ngbde_procfs_init(void);

/*!
 * \brief Clean up procfs for BDE driver.
 *
 * Clean up resources allocated by \ref ngbde_procfs_init.
 *
 * \return 0 if no errors, otherwise -1.
 */
extern int
ngbde_procfs_cleanup(void);

/*!
 * \brief Allocate DMA memory pools for all probed devices.
 *
 * \return 0 if no errors, otherwise -1.
 */
extern int
ngbde_dma_init(void);

/*!
 * \brief Free DMA memory pools for all probed devices.
 *
 * \return Nothing.
 */
extern void
ngbde_dma_cleanup(void);

/*!
 * \brief Allocate interrupt lines.
 *
 * This function will update irq_max member in the device descriptor
 * with the number of interrupt lines actually allocated.
 *
 * No action is taken if a kernel ISR is already active (e.g. after a
 * warm-boot).
 *
 * \param [in] kdev Device number.
 * \param [in] num_irq Number of interrupt lines wanted.
 *
 * \return Number of allocated interrupt lines or -1 if error.
 */
extern int
ngbde_intr_alloc(int kdev, unsigned int num_irq);

/*!
 * \brief Free interrupt lines.
 *
 * Free interrupt lines previously allocated via \ref
 * ngbde_intr_alloc.
 *
 * No action is taken if a kernel ISR is still active.
 *
 * \param [in] kdev Device number.
 *
 * \retval 0 No errors
 * \retval -1 Something went wrong.
 */
extern int
ngbde_intr_free(int kdev);

/*!
 * \brief Connect to hardware interrupt handler.
 *
 * \param [in] kdev Device number.
 * \param [in] irq_num Interrupt number (MSI vector).
 *
 * \retval 0 No errors
 * \retval -1 Something went wrong.
 */
extern int
ngbde_intr_connect(int kdev, unsigned int irq_num);

/*!
 * \brief Disconnect from hardware interrupt handler.
 *
 * \param [in] kdev Device number.
 * \param [in] irq_num Interrupt number (MSI vector).
 *
 * \retval 0 No errors
 * \retval -1 Something went wrong.
 */
extern int
ngbde_intr_disconnect(int kdev, unsigned int irq_num);

/*!
 * \brief Disconnect from all hardware interrupt handlers.
 */
void
ngbde_intr_cleanup(void);

/*!
 * \brief Wait for hardware interrupt.
 *
 * A user mode thread will call this function and sleep until a
 * hardware interrupt occurs.
 *
 * \param [in] kdev Device number.
 * \param [in] irq_num Interrupt number (MSI vector).
 *
 * \retval 0 No errors
 * \retval -1 Something went wrong.
 */
extern int
ngbde_intr_wait(int kdev, unsigned int irq_num);

/*!
 * \brief Wake up sleeping interrupt thread.
 *
 * Wake up interrupt thread even if no interrupt has occurred.
 *
 * Intended for graceful shut-down procedure.
 *
 * \param [in] kdev Device number.
 * \param [in] irq_num Interrupt number (MSI vector).
 *
 * \retval 0 No errors
 * \retval -1 Something went wrong.
 */
extern int
ngbde_intr_stop(int kdev, unsigned int irq_num);

/*!
 * \brief Clear list of interrupt status/mask registers.
 *
 * This function is typically called before new interrupt register
 * information is added.
 *
 * \param [in] kdev Device number.
 * \param [in] irq_num Interrupt number (MSI vector).
 *
 * \retval 0 No errors
 * \retval -1 Something went wrong.
 */
extern int
ngbde_intr_regs_clr(int kdev, unsigned int irq_num);

/*!
 * \brief Add interrupt status/mask register to monitor.
 *
 * This function adds a new interrupt status/mask register set to the
 * list of registers monitored by the user-mode interrupt handler.
 *
 * The register list is used to determine whether a user-mode
 * interrupt has occurred.
 *
 * See also \ref ngbde_intr_regs_clr.
 *
 * \param [in] kdev Device number.
 * \param [in] irq_num Interrupt number (MSI vector).
 * \param [in] ireg Interrupt status/mask register information.
 *
 * \retval 0 No errors
 * \retval -1 Something went wrong.
 */
extern int
ngbde_intr_reg_add(int kdev, unsigned int irq_num,
                   struct ngbde_irq_reg_s *ireg);

/*!
 * \brief Add interrupt ack register to monitor.
 *
 * This function adds a interrupt register and mask value
 * to acknowledge corresponding irq_num.
 *
 * \param [in] kdev Device number.
 * \param [in] irq_num Interrupt number (MSI vector).
 * \param [in] ackreg Interrupt ack register information.
 *
 * \retval 0 No errors
 * \retval -1 Something went wrong.
 */
extern int
ngbde_intr_ack_reg_add(int kdev, unsigned int irq_num,
                       struct ngbde_intr_ack_reg_s *ackreg);

/*!
 * \brief Write shared interrupt mask register.
 *
 * This function is used by an interrupt handler when a shared
 * interrupt mask register needs to be updated.
 *
 * Since the register is shared between multiple interrupt handlers,
 * access must be protected by a lock.
 *
 * The register information provided via \ref ngbde_intr_reg_add is
 * used to detemine which bits of the mask register belong to the user
 * mode driver.
 *
 * Note that the mask register to access is referenced by the
 * corresponding status register. This is because the mask register
 * may be different depending on the host CPU interface being used
 * (e.g. PCI vs. AXI). On the other hand, the status register is the
 * same irrespective of the host CPU interface.
 *
 * \param [in] kdev Device number.
 * \param [in] irq_num Interrupt number (MSI vector).
 * \param [in] kapi Must be set to 1 if called from kernel API.
 * \param [in] status_reg Corresponding interrupt status register offset.
 * \param [in] mask_val New value to write to mask register.
 *
 * \retval 0 No errors
 * \retval -1 Something went wrong.
 */
extern int
ngbde_intr_mask_write(int kdev, unsigned int irq_num, int kapi,
                      uint32_t status_reg, uint32_t mask_val);

/*!
 * \brief Probe for PCI-attached Broadcom switch devices.
 *
 * \return 0 if no errors, otherwise -1.
 */
extern int
ngbde_pci_probe(void);

/*!
 * \brief Clean up resources for PCI-attached Broadcom switch devices.
 *
 * \return 0 if no errors, otherwise -1.
 */
extern int
ngbde_pci_cleanup(void);

/*!
 * \brief Add new switch device to BDE database.
 *
 * Add device information for probed or fixed switch device.
 *
 * \param [in] nd Switch device information.
 *
 * \return 0 if no errors, otherwise -1.
 */
extern int
ngbde_swdev_add(struct ngbde_dev_s *nd);

/*!
 * \brief Get device information for a BDE switch device.
 *
 * \param [in] kdev Switch device number.
 *
 * \return Pointer to switch device structure or NULL on error.
 */
struct ngbde_dev_s *
ngbde_swdev_get(int kdev);

/*!
 * \brief Get list of all probed switch devices.
 *
 * Return a pointer to the array of registered switch devices.
 *
 * \param [out] nd Pointer to array of switch devices.
 * \param [in] num_nd number of valid entries in switch device array.
 *
 * \retval 0 No errors
 */
extern int
ngbde_swdev_get_all(struct ngbde_dev_s **nd, unsigned int *num_nd);

/*!
 * \brief Allocate memory using page allocator
 *
 * For any sizes less than MEM_CHUNK_SIZE, we ask the page allocator
 * for the entire memory block, otherwise we try to assemble a
 * contiguous cmblock ourselves.
 *
 * Upon successful allocation, the memory block will be added to the
 * global list of allocated memory blocks.
 *
 * \param [in] size Number of bytes to allocate.
 * \param [in] flags Kernel flags (GFP_xxx) for memory allocation.
 *
 * \return Pointer to allocated memory or NULL if failure.
 */
void *
ngbde_pgmem_alloc(size_t size, gfp_t flags);

/*!
 * \brief Free memory block allocated by ngbde_pgmem_alloc.
 *
 * \param [in] ptr Pointer returned by ngbde_pgmem_alloc.
 *
 * \return 0 if succesfully freed, otherwise -1.
 */
extern int
ngbde_pgmem_free(void *ptr);

/*!
 * \brief Free all memory blocks allocated by ngbde_pgmem_alloc.
 *
 * This function will walk the global list of allocated memory blocks
 * and free all associated resources.
 *
 * Intended for a full clean up before the module is unloaded.
 *
 * \return Nothing.
 */
extern void
ngbde_pgmem_free_all(void);

/*!
 * \brief Map I/O memory in kernel driver.
 *
 * This function is used to provide device I/O access to a kernel mode
 * driver.
 *
 * \param [in] devh Device handle (\ref ngbde_dev_s).
 * \param [in] addr Physical address to map.
 * \param [in] size Size of I/O window to map.
 *
 * \return Pointer to mapped I/O memory, or NULL on error.
 */
extern void *
ngbde_pio_map(void *devh, phys_addr_t addr, phys_addr_t size);

/*!
 * \brief Unmap I/O memory in kernel driver.
 *
 * Unmap I/O memory previously mapped via \ref ngbde_pio_map.
 *
 * \param [in] devh Device handle (\ref ngbde_dev_s).
 *
 * \return Nothing.
 */
extern void
ngbde_pio_unmap(void *devh);

/*!
 * \brief Unmap all I/O windows.
 */
extern void
ngbde_pio_cleanup(void);

/*!
 * \brief Write a memory-mapped register from kernel driver.
 *
 * Write a 32-bit register using I/O memory previously mapped via \ref
 * ngbde_pio_map.
 *
 * \param [in] devh Device handle (\ref ngbde_dev_s).
 * \param [in] offs Register address offset.
 * \param [in] val Value to write to register.
 *
 * \return Nothing.
 */
extern void
ngbde_pio_write32(void *devh, uint32_t offs, uint32_t val);

/*!
 * \brief Read a memory-mapped register from kernel driver.
 *
 * Read a 32-bit register using I/O memory previously mapped via \ref
 * ngbde_pio_map.
 *
 * \param [in] devh Device handle (\ref ngbde_dev_s).
 * \param [in] offs Register address offset.
 *
 * \return Value read from register.
 */
extern uint32_t
ngbde_pio_read32(void *devh, uint32_t offs);

/*!
 * \brief Map interrupt controller I/O memory.
 *
 * On some devices the interrupt controller is a device separate from
 * the main switch device. This function is used to provide interrupt
 * controller I/O access to a kernel mode driver.
 *
 * \param [in] devh Device handle (\ref ngbde_dev_s).
 * \param [in] addr Physical address to map.
 * \param [in] size Size of I/O window to map.
 *
 * \return Pointer to mapped I/O memory, or NULL on error.
 */
extern void *
ngbde_iio_map(void *devh, phys_addr_t addr, phys_addr_t size);

/*!
 * \brief Unmap interrupt controller I/O memory.
 *
 * Unmap I/O memory previously mapped via \ref ngbde_iio_map.
 *
 * \param [in] devh Device handle (\ref ngbde_dev_s).
 *
 * \return Nothing.
 */
extern void
ngbde_iio_unmap(void *devh);

/*!
 * \brief Unmap all interrupt controller I/O windows.
 */
extern void
ngbde_iio_cleanup(void);

/*!
 * \brief Write a memory-mapped interrupt controller register.
 *
 * Write a 32-bit register using I/O memory previously mapped via \ref
 * ngbde_iio_map.
 *
 * \param [in] devh Device handle (\ref ngbde_dev_s).
 * \param [in] offs Register address offset.
 * \param [in] val Value to write to register.
 *
 * \return Nothing.
 */
extern void
ngbde_iio_write32(void *devh, uint32_t offs, uint32_t val);

/*!
 * \brief Read a memory-mapped interrupt controller register.
 *
 * Read a 32-bit register using I/O memory previously mapped via \ref
 * ngbde_iio_map.
 *
 * \param [in] devh Device handle (\ref ngbde_dev_s).
 * \param [in] offs Register address offset.
 *
 * \return Value read from register.
 */
extern uint32_t
ngbde_iio_read32(void *devh, uint32_t offs);

/*!
 * \brief Map PCI bridge I/O memory.
 *
 * On some devices the interrupt controller is a device separate from
 * the main switch device. This function is used to provide interrupt
 * controller I/O access to a kernel mode driver.
 *
 * \param [in] devh Device handle (\ref ngbde_dev_s).
 * \param [in] addr Physical address to map.
 * \param [in] size Size of I/O window to map.
 *
 * \return Pointer to mapped I/O memory, or NULL on error.
 */
extern void *
ngbde_paxb_map(void *devh, phys_addr_t addr, phys_addr_t size);

/*!
 * \brief Unmap PCI bridge I/O memory.
 *
 * Unmap I/O memory previously mapped via \ref ngbde_paxb_map.
 *
 * \param [in] devh Device handle (\ref ngbde_dev_s).
 *
 * \return Nothing.
 */
extern void
ngbde_paxb_unmap(void *devh);

/*!
 * \brief Unmap all PCI bridge I/O windows.
 */
extern void
ngbde_paxb_cleanup(void);

/*!
 * \brief Write a memory-mapped PCI bridge register.
 *
 * Write a 32-bit register using I/O memory previously mapped via \ref
 * ngbde_paxb_map.
 *
 * \param [in] devh Device handle (\ref ngbde_dev_s).
 * \param [in] offs Register address offset.
 * \param [in] val Value to write to register.
 *
 * \return Nothing.
 */
extern void
ngbde_paxb_write32(void *devh, uint32_t offs, uint32_t val);

/*!
 * \brief Read a memory-mapped PCI bridge register.
 *
 * Read a 32-bit register using I/O memory previously mapped via \ref
 * ngbde_paxb_map.
 *
 * \param [in] devh Device handle (\ref ngbde_dev_s).
 * \param [in] offs Register address offset.
 *
 * \return Value read from register.
 */
extern uint32_t
ngbde_paxb_read32(void *devh, uint32_t offs);

/*!
 * \brief Probe for Broadcom switch devices on IPROC internal bus.
 *
 * \return 0 if no errors, otherwise -1.
 */
extern int
ngbde_iproc_probe(void);

/*!
 * \brief Clean up resources for Broadcom switch devices on IPROC internal bus.
 *
 * \return 0 if no errors, otherwise -1.
 */
extern int
ngbde_iproc_cleanup(void);

#endif /* NGBDE_H */
