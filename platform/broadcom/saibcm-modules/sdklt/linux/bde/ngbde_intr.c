/*! \file ngbde_intr.c
 *
 * API for controlling a thread-based user-mode interrupt handler.
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

#include <ngbde.h>

/*! \cond */
static int intr_debug = 0;
module_param(intr_debug, int, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(intr_debug,
"Interrupt debug output enable (default 0).");
/*! \endcond */

/*!
 * \brief Shared register write.
 *
 * This function is used for writing to registers where the calling
 * context only owns a subset of bits within the register.
 *
 * \param [in] sd Software device information.
 * \param [in] ic Interrupt control information.
 * \param [in] reg_offs Shared register address offset.
 * \param [in] reg_val Shared register value.
 * \param [in] shr_mask Register bits owned by this context.
 *
 * \retval 0 No errors.
 * \retval -1 Unknown shared register.
 */
static int
ngbde_intr_shared_write32(struct ngbde_dev_s *sd, struct ngbde_intr_ctrl_s *ic,
                          uint32_t reg_offs, uint32_t reg_val, uint32_t shr_mask)
{
    unsigned long flags;
    struct ngbde_shr_reg_s *sr;
    int idx;

    sr = NULL;
    for (idx = 0; idx < NGBDE_NUM_INTR_SHR_REGS_MAX; idx++) {
        if (sd->intr_shr_reg[idx].reg_offs == 0) {
            /* If not found, then we add a new entry */
            sd->intr_shr_reg[idx].reg_offs = reg_offs;
        }
        if (sd->intr_shr_reg[idx].reg_offs == reg_offs) {
            sr = &sd->intr_shr_reg[idx];
            break;
        }
    }

    if (sr == NULL) {
        return -1;
    }

    spin_lock_irqsave(&sd->lock, flags);

    sr->cur_val &= ~shr_mask;
    sr->cur_val |= (reg_val & shr_mask);

    NGBDE_IOWRITE32(sr->cur_val, ic->iomem + reg_offs);

    spin_unlock_irqrestore(&sd->lock, flags);

    return 0;
}

/*!
 * \brief Interrupt handler for user mode thread.
 *
 * This function will determine whether a user-mode interrupt has
 * occurred by reading the configured interrupt status and mask
 * registers.
 *
 * If an interrupt has occurred, any waiting user-mode thread is woken
 * up.
 *
 * \param [in] ic Interrupt control information.
 *
 * \retval 1 One or more user mode interrupts occurred.
 * \retval 0 No user mode interrupts occurred.
 */
static int
ngbde_user_isr(ngbde_intr_ctrl_t *ic)
{
    int idx;
    int active_interrupts = 0;
    uint32_t stat = 0, mask = 0;

    if (intr_debug >= 2) {
        printk("INTR: Run user ISR (%d)\n", ic->irq_vect);
    }

    /*
     * If this interrupt vector is shared between user mode and kernel
     * mode, then we want to avoid invoking the user mode handler if
     * only kernel mode interrupts are active.
     */
    if (ic->run_kernel_isr) {
        /* Check if any enabled user mode interrupts are active */
        for (idx = 0; idx < ic->num_regs; idx++) {
            ngbde_irq_reg_t *ir = &ic->regs[idx];

            stat = NGBDE_IOREAD32(&ic->iomem[ir->status_reg]);
            if (!ir->status_is_masked) {
                /* Get enabled interrupts by applying mask register */
                mask = NGBDE_IOREAD32(&ic->iomem[ir->mask_reg]);
                stat &= mask;
            }
            if (stat & ir->umask) {
                active_interrupts = 1;
                break;
            }
        }

        /* No active user mode interrupts to service */
        if (!active_interrupts) {
            return 0;
        }
    }

    /* Disable (mask off) all user mode interrupts */
    for (idx = 0; idx < ic->num_regs; idx++) {
        ngbde_irq_reg_t *ir = &ic->regs[idx];
        struct ngbde_dev_s *sd;

        if (ir->umask == 0) {
            /* Kernel driver owns all interrupts in this register */
            continue;
        }
        if (ir->mask_w1tc) {
            /* Clear all interrupt mask bits owned by this user mode ISR */
            NGBDE_IOWRITE32(ir->umask, &ic->iomem[ir->mask_reg]);
            continue;
        }
        if (ir->umask == 0xffffffff) {
            /* Direct write when all bits are owned by this user mode ISR */
            NGBDE_IOWRITE32(0, &ic->iomem[ir->mask_reg]);
            continue;
        }
        /* Synchronized write when some bits are owned by another ISR */
        sd = ngbde_swdev_get(ic->kdev);
        if (sd) {
            if (ngbde_intr_shared_write32(sd, ic, ir->mask_reg, 0, ir->umask) < 0) {
                printk(KERN_WARNING
                       "%s: Failed to write shared register for device %d\n",
                       MOD_NAME, ic->kdev);
                /* Fall back to normal write to ensure interrupts are masked */
                NGBDE_IOWRITE32(0, &ic->iomem[ir->mask_reg]);
            }
        }
    }

    atomic_set(&ic->run_user_thread, 1);
    wake_up_interruptible(&ic->user_thread_wq);

    return 1;
}

/*!
 * \brief Interrupt handler for kernel driver.
 *
 * Typically used by the KNET driver.
 *
 * \param [in] ic Interrupt control information.
 *
 * \retval 1 One or more kernel mode interrupts occurred.
 * \retval 0 No kernel mode interrupts occurred.
 */
static int
ngbde_kernel_isr(ngbde_intr_ctrl_t *ic)
{
    if (intr_debug >= 2) {
        printk("INTR: Run kernel ISR (%d)\n", ic->irq_vect);
    }

    if (ic->isr_func) {
        return ic->isr_func(ic->isr_data);
    }
    return 0;
}

/*!
 * \brief Update interrupt dispatcher.
 *
 * Check which interrupts handlers (kernel/user) should be invoked for
 * this interrupt line.
 *
 * \param [in] ic Interrupt control information.
 *
 * \retval 0
 */
static int
ngbde_intr_dispatch_update(ngbde_intr_ctrl_t *ic)
{
    struct ngbde_irq_reg_s *ir;
    unsigned int idx;
    uint32_t umask = 0;
    uint32_t kmask = 0;

    for (idx = 0; idx < ic->num_regs; idx++) {
        ir = &ic->regs[idx];
        umask |= ir->umask;
        kmask |= ir->kmask;
    }

    ic->run_user_isr = (umask != 0);
    ic->run_kernel_isr = (kmask != 0);

    return 0;
}

/*!
 * \brief Acknowledge interrupt.
 *
 * \param [in] ic Interrupt control information.
 *
 * \retval 0
 */
static int
ngbde_intr_ack(ngbde_intr_ctrl_t *ic)
{
    struct ngbde_dev_s *sd = ngbde_swdev_get(ic->kdev);
    struct ngbde_intr_ack_reg_s *ar = &ic->intr_ack;

    if (!sd) {
        return 0;
    }

    if (sd->use_msi && ar->ack_valid) {
        if (intr_debug >= 2) {
            printk("INTR: ACK interrupt vector %d\n", ic->irq_vect);
        }
        if (ar->ack_domain == NGBDE_INTR_ACK_IO_PAXB) {
            ngbde_paxb_write32(sd, ar->ack_reg, ar->ack_val);
        } else {
            ngbde_pio_write32(sd, ar->ack_reg, ar->ack_val);
        }
    }

    return 0;
}

/*!
 * \brief Linux ISR
 *
 * Will call the user-mode interrupts handler and optionally also a
 * kernel mode interrupt handler (typically KNET).
 *
 * \param [in] irq_num Interrupt vector from kernel.
 * \param [in] data Interrupt control information
 *
 * \retval IRQ_NONE Interrupt not recognized.
 * \retval IRQ_HANDLED Interrupt recognized and handled (masked off).
 */
static irqreturn_t
ngbde_isr(int irq_num, void *data)
{
    struct ngbde_intr_ctrl_s *ic = (struct ngbde_intr_ctrl_s *)data;
    irqreturn_t rv = IRQ_NONE;

    if (intr_debug >= 2) {
        printk("INTR: Process interrupt vector %d\n", ic->irq_vect);
    }

    if (ic->run_user_isr) {
        if (ngbde_user_isr(ic)) {
            rv = IRQ_HANDLED;
        }
    }

    if (ic->run_kernel_isr) {
        if (ngbde_kernel_isr(ic)) {
            rv = IRQ_HANDLED;
        }
    }

    ngbde_intr_ack(ic);

    return rv;
}

int
ngbde_intr_alloc(int kdev, unsigned int num_irq)
{
    struct ngbde_dev_s *sd;
    unsigned long irq_types;
    int irq, vect;

    if (intr_debug) {
        printk("INTR: Request %d interrupts\n", num_irq);
    }

    sd = ngbde_swdev_get(kdev);
    if (!sd) {
        return -1;
    }

    if (num_irq == 0) {
        return -1;
    }

    if (sd->active_irqs) {
        if (intr_debug) {
            printk("INTR: Skip reallocating active interrupts\n");
        }
        return sd->irq_max;
    }

    if (sd->irq_max > 0) {
        if (intr_debug) {
            printk("INTR: Interrupts already allocated\n");
        }
        return sd->irq_max;
    }

    /* Use new API if available (Linux 4.8 and newer) */
    irq_types = PCI_IRQ_INTX;
    if (sd->use_msi) {
        irq_types |= PCI_IRQ_MSI;
        if (sd->use_msi == NGBDE_MSI_T_MSIX) {
            irq_types |= PCI_IRQ_MSIX;
        } else {
            /* Only allow one IRQ line if not MSI-X */
            num_irq = 1;
        }
    }
    sd->irq_max = pci_alloc_irq_vectors(sd->pci_dev, 1, num_irq, irq_types);
    if (sd->irq_max < 1) {
        printk(KERN_WARNING "%s: Failed to allocate IRQs for device %d\n",
               MOD_NAME, kdev);
        return -1;
    }
    if (intr_debug) {
        printk("INTR: Allocated %d interrupt vector(s)\n", sd->irq_max);
    }
    for (irq = 0; irq < sd->irq_max; irq++) {
        vect = pci_irq_vector(sd->pci_dev, irq);
        if (intr_debug) {
            printk("INTR: Interrupt vector %d = %d\n", irq, vect);
        }
        sd->intr_ctrl[irq].irq_vect = vect;
    }

    return sd->irq_max;
}

int
ngbde_intr_free(int kdev)
{
    struct ngbde_dev_s *sd;

    if (intr_debug) {
        printk("INTR: Free interrupts\n");
    }

    sd = ngbde_swdev_get(kdev);
    if (!sd) {
        return -1;
    }

    if (sd->active_irqs) {
        if (intr_debug) {
            printk("INTR: Skip freeing active interrupts\n");
        }
        return 0;
    }

    pci_free_irq_vectors(sd->pci_dev);

    sd->irq_max = 0;

    return 0;
}

int
ngbde_intr_connect(int kdev, unsigned int irq_num)
{
    struct ngbde_dev_s *sd;
    struct ngbde_intr_ctrl_s *ic;
    unsigned long irq_flags;

    if (intr_debug) {
        printk("INTR: Interrupt connect (%d)\n", irq_num);
    }

    sd = ngbde_swdev_get(kdev);
    if (!sd) {
        return -1;
    }

    if (irq_num >= NGBDE_NUM_IRQS_MAX) {
        return -1;
    }

    ic = &sd->intr_ctrl[irq_num];

    if (ic->irq_active) {
        if (intr_debug) {
            printk("INTR: Interrupt already connected (%d)\n", irq_num);
        }
        return 0;
    }

    if (sd->irq_line < 0) {
        printk(KERN_WARNING "%s: No IRQ line for device %d\n",
               MOD_NAME, kdev);
        return -1;
    }

    if (sd->pio_mem == NULL) {
        printk(KERN_WARNING "%s: No memory-mapped I/O for device %d\n",
               MOD_NAME, kdev);
        return -1;
    }

    /*
     * Check for old application that does not support interrupt line
     * allocation.
     */
    if (sd->irq_max == 0) {
        ngbde_intr_alloc(kdev, 1);
        if (sd->irq_max == 0) {
            return -1;
        }
    }

    if (sd->active_irqs >= sd->irq_max) {
        printk(KERN_WARNING "%s: Too many IRQs for device %d\n",
               MOD_NAME, kdev);
        return -1;
    }

    ic->kdev = kdev;
    ic->iomem = sd->pio_mem;
    if (sd->iio_mem) {
        if (intr_debug) {
            printk("INTR: Using dedicated interrupt controller\n");
        }
        ic->iomem = sd->iio_mem;
    }
    init_waitqueue_head(&ic->user_thread_wq);
    atomic_set(&ic->run_user_thread, 0);
    irq_flags = IRQF_SHARED;

    if (intr_debug) {
        printk("INTR: Request IRQ %d\n", ic->irq_vect);
    }
    if (request_irq(ic->irq_vect, ngbde_isr, irq_flags, MOD_NAME, ic) < 0) {
        printk(KERN_WARNING "%s: Could not get IRQ %d for device %d\n",
               MOD_NAME, ic->irq_vect, kdev);
        return -1;
    }
    ic->irq_active = 1;
    sd->active_irqs++;

    return 0;
}

int
ngbde_intr_disconnect(int kdev, unsigned int irq_num)
{
    struct ngbde_dev_s *sd;
    struct ngbde_intr_ctrl_s *ic;

    if (intr_debug) {
        printk("INTR: Interrupt disconnect (%d)\n", irq_num);
    }

    sd = ngbde_swdev_get(kdev);
    if (!sd) {
        return -1;
    }

    if (irq_num >= NGBDE_NUM_IRQS_MAX) {
        return -1;
    }

    if (sd->active_irqs == 0) {
        return -1;
    }

    ic = &sd->intr_ctrl[irq_num];

    if (!ic->irq_active) {
        return 0;
    }

    if (ic->isr_func) {
        printk(KERN_WARNING "%s: Disconnecting IRQ %d blocked by kernel ISR\n",
               MOD_NAME, irq_num);
        return 0;
    }

    if (ic->irq_vect >= 0) {
        free_irq(ic->irq_vect, ic);
    }

    ic->irq_active = 0;
    sd->active_irqs--;

    if (sd->active_irqs == 0 && sd->irq_max == 1) {
        ngbde_intr_free(kdev);
    }

    return 0;
}

void
ngbde_intr_cleanup(void)
{
    struct ngbde_dev_s *swdev;
    unsigned int num_swdev, idx, irq_num;

    ngbde_swdev_get_all(&swdev, &num_swdev);

    for (idx = 0; idx < num_swdev; idx++) {
        for (irq_num = 0; irq_num < NGBDE_NUM_IRQS_MAX; irq_num++) {
            ngbde_intr_disconnect(idx, irq_num);
        }
        ngbde_intr_free(idx);
    }
}

int
ngbde_intr_wait(int kdev, unsigned int irq_num)
{
    struct ngbde_dev_s *sd;
    struct ngbde_intr_ctrl_s *ic;

    sd = ngbde_swdev_get(kdev);
    if (!sd) {
        return -1;
    }

    if (irq_num >= NGBDE_NUM_IRQS_MAX) {
        return -1;
    }

    ic = &sd->intr_ctrl[irq_num];

    if (!ic->irq_active) {
        return 0;
    }

    if (intr_debug >= 2) {
        printk("INTR: User wait for interrupt (%d)\n", ic->irq_vect);
    }
    wait_event_interruptible(ic->user_thread_wq,
                             atomic_read(&ic->run_user_thread) != 0);
    atomic_set(&ic->run_user_thread, 0);
    if (intr_debug >= 2) {
        printk("INTR: User process interrupt (%d)\n", ic->irq_vect);
    }

    return 0;
}

int
ngbde_intr_stop(int kdev, unsigned int irq_num)
{
    struct ngbde_dev_s *sd;
    struct ngbde_intr_ctrl_s *ic;

    sd = ngbde_swdev_get(kdev);
    if (!sd) {
        return -1;
    }

    if (irq_num >= NGBDE_NUM_IRQS_MAX) {
        return -1;
    }

    ic = &sd->intr_ctrl[irq_num];

    if (!ic->irq_active) {
        return 0;
    }

    /* Wake up user thread */
    atomic_set(&ic->run_user_thread, 1);
    wake_up_interruptible(&ic->user_thread_wq);

    return 0;
}

int
ngbde_intr_regs_clr(int kdev, unsigned int irq_num)
{
    struct ngbde_dev_s *sd;
    struct ngbde_intr_ctrl_s *ic;

    sd = ngbde_swdev_get(kdev);
    if (!sd) {
        return -1;
    }

    if (irq_num >= NGBDE_NUM_IRQS_MAX) {
        return -1;
    }

    ic = &sd->intr_ctrl[irq_num];

    if (ic->irq_active) {
        /* Do not clear configuration with interrupt connected */
        return 0;
    }

    ic->num_regs = 0;
    memset(ic->regs, 0, sizeof(ic->regs));

    return 0;
}

int
ngbde_intr_reg_add(int kdev, unsigned int irq_num,
                   struct ngbde_irq_reg_s *ireg)
{
    struct ngbde_dev_s *sd;
    struct ngbde_intr_ctrl_s *ic;
    struct ngbde_irq_reg_s *ir;
    unsigned int idx;

    sd = ngbde_swdev_get(kdev);
    if (!sd) {
        return -1;
    }

    if (irq_num >= NGBDE_NUM_IRQS_MAX) {
        return -1;
    }

    ic = &sd->intr_ctrl[irq_num];

    if (ic->irq_active) {
        /*
         * If the interrupt is connected, then we only update the
         * kernel mask for existing entries, and only if the kernel
         * mask is marked as valid and differs from the existing mask.
         */
        for (idx = 0; idx < ic->num_regs; idx++) {
            ir = &ic->regs[idx];
            if (ir->status_reg == ireg->status_reg &&
                ir->mask_reg == ireg->mask_reg) {
                if (ir->kmask != ireg->kmask && ireg->kmask_valid) {
                    ir->kmask = ireg->kmask;
                    ir->umask = ireg->umask;
                    if (intr_debug) {
                        printk("INTR: Updated interrupt register "
                               "0x%08x/0x%08x [u:0x%08x,k:0x%08x] (%d)\n",
                               ir->status_reg, ir->mask_reg,
                               ir->umask, ir->kmask, irq_num);
                    }
                    ngbde_intr_dispatch_update(ic);
                }
                return 0;
            }
        }
        return -1;
    }

    if (ic->num_regs >= NGBDE_NUM_IRQ_REGS_MAX) {
        return -1;
    }

    ir = &ic->regs[ic->num_regs++];
    memcpy(ir, ireg, sizeof (*ir));
    if (intr_debug) {
        printk("INTR: Added interrupt register "
               "0x%08x/0x%08x [u:0x%08x,k:0x%08x] (%d)\n",
               ir->status_reg, ir->mask_reg,
               ir->umask, ir->kmask, irq_num);
    }
    ngbde_intr_dispatch_update(ic);

    return ic->num_regs;
}

int
ngbde_intr_ack_reg_add(int kdev, unsigned int irq_num,
                       struct ngbde_intr_ack_reg_s *ackreg)
{
    struct ngbde_dev_s *sd;
    struct ngbde_intr_ctrl_s *ic;
    struct ngbde_intr_ack_reg_s *ar;

    sd = ngbde_swdev_get(kdev);
    if (!sd) {
        return -1;
    }

    if (irq_num >= NGBDE_NUM_IRQS_MAX) {
        return -1;
    }

    ic = &sd->intr_ctrl[irq_num];

    if (ic->irq_active) {
        /* Ignore request if interrupt is connected */
        return 0;
    }

    ar = &ic->intr_ack;

    memcpy(ar, ackreg, sizeof (*ar));

    if (intr_debug) {
        printk("INTR: Adding interrupt ACK register 0x%08x/0x%08x[%d] (%d)\n",
               ar->ack_reg, ar->ack_val, ar->ack_domain, irq_num);
    }

    return 0;
}

int
ngbde_intr_mask_write(int kdev, unsigned int irq_num, int kapi,
                      uint32_t status_reg, uint32_t mask_val)
{
    struct ngbde_dev_s *sd;
    struct ngbde_intr_ctrl_s *ic;
    struct ngbde_irq_reg_s *ir;
    unsigned int idx;
    uint32_t bmask;

    sd = ngbde_swdev_get(kdev);
    if (!sd) {
        return -1;
    }

    if (irq_num >= NGBDE_NUM_IRQS_MAX) {
        return -1;
    }

    ic = &sd->intr_ctrl[irq_num];

    ir = ic->regs;
    for (idx = 0; idx < ic->num_regs; idx++) {
        if (ir->status_reg == status_reg) {
            bmask = kapi ? ir->kmask : ~ir->kmask;
            ngbde_intr_shared_write32(sd, ic, ir->mask_reg, mask_val, bmask);
            return 0;
        }
        ir++;
    }

    return -1;
}
