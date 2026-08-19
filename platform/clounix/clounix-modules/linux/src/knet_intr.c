#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>

#include "knet_pci.h"
#include "knet_dev.h"
#include "knet_dma.h"

/* INTx interrupt*/
static irqreturn_t
clx_intx_usr_handler(int irq, uint32_t unit)
{
    struct clx_pci_dev_s *ptr_clx_dev = clx_misc_dev->clx_pci_dev[unit];
    clx_intr_info_t info;

    info.unit = unit;
    info.irq = irq - ptr_clx_dev->pci_dev->irq;
    info.valid = CLX_INTR_VALID_CODE;

    dbg_print(DBG_INTR, "in info:0x%lx,irq:%d\n", (unsigned long)&info, info.irq);
    spin_lock(&clx_misc_dev->fifo_lock);
    if (!kfifo_put(&clx_misc_dev->intr_fifo, info)) {
        spin_unlock(&clx_misc_dev->fifo_lock);
        return IRQ_HANDLED;
    }
    spin_unlock(&clx_misc_dev->fifo_lock);

    wake_up_interruptible(&clx_misc_dev->isr_wait_queue);

    return IRQ_HANDLED;
}

static irqreturn_t
clx_intr_dma_handler(uint32_t unit)
{
    uint32_t channel_bmp = 0;
    uint32_t channel = 0;

    // 1. get channel irq status
    clx_intr_drv(unit)->read_dma_irq_status(unit, &channel_bmp);

    dbg_print(DBG_INTR, "irq channel_bmp:0x%x\n", channel_bmp);

    // 2. process
    for (channel = 0; channel < clx_dma_drv(unit)->channel_num; channel++) {
        if (!((0x1 << channel) & channel_bmp)) {
            continue;
        }
        dbg_print(DBG_INTR, "irq channel:%d\n", channel);
        clx_intr_drv(unit)->mask_dma_channel_irq(unit, channel);
        if (channel < (clx_dma_drv(unit)->rx_channel_num + clx_dma_drv(unit)->tx_channel_num)) {
            clx_intr_drv(unit)->clear_dma_channel_irq(unit, channel);
        }
        tasklet_schedule(&clx_dma_drv(unit)->clx_dma_intr[channel].dma_tasklets);
    }

    // 3. unmask dma top irq
    clx_intr_drv(unit)->dma_irq_unmask_all(unit);

    return IRQ_HANDLED;
}

static irqreturn_t
clx_intr_handler(int irq, void *cookie)
{
    uint32_t unit = (uint32_t)((clx_addr_t)cookie);
    uint32_t usr_flag = 0;

    dbg_print(DBG_INTR, "irq handler irq:%d\n", irq);

    if (clx_intr_drv(unit)->usr_interrupt_exist(unit)) {
        usr_flag = 1;
    }

    /* 1. mask top interrupt */
    clx_intr_drv(unit)->top_irq_mask(unit);

    /* 2. process pdma interrupt */
    clx_intr_dma_handler(unit);

    /* 3. process other interrupt */
    if (usr_flag) {
        clx_intx_usr_handler(irq, unit);
    } else {
        clx_intr_drv(unit)->top_irq_unmask(unit);
    }

    return IRQ_HANDLED;
}

static int
clx_interrupt_intx_init(uint32_t unit)
{
    struct clx_pci_dev_s *ptr_clx_dev = clx_misc_dev->clx_pci_dev[unit];
    struct pci_dev *pci_dev = ptr_clx_dev->pci_dev;
    int channel;
    int rc;

    for (channel = 0; channel < clx_dma_drv(unit)->channel_num; channel++) {
        tasklet_init(&clx_dma_drv(unit)->clx_dma_intr[channel].dma_tasklets,
                     clx_dma_drv(unit)->clx_dma_intr[channel].dma_handler,
                     (unsigned long)&clx_dma_drv(unit)->clx_dma_intr[channel].channel_cookie);
    }

    /* dma channel error */
    tasklet_init(&clx_dma_drv(unit)->error_channel.dma_tasklet, dma_error_tasklet_func,
                 (unsigned long)&clx_dma_drv(unit)->error_channel.channel_cookie);

    rc =
        request_irq(pci_dev->irq, clx_intr_handler, 0, CLX_DRIVER_NAME, (void *)((clx_addr_t)unit));

    return rc;
}

static void
clx_interrupt_intx_deinit(uint32_t unit)
{
    struct clx_pci_dev_s *ptr_clx_dev = clx_misc_dev->clx_pci_dev[unit];
    struct pci_dev *pci_dev = ptr_clx_dev->pci_dev;
    int channel;

    for (channel = 0; channel < clx_dma_drv(unit)->channel_num; channel++) {
        tasklet_kill(&clx_dma_drv(unit)->clx_dma_intr[channel].dma_tasklets);
    }

    /* dma channel error */
    tasklet_kill(&clx_dma_drv(unit)->error_channel.dma_tasklet);

    dbg_print(DBG_INTR, "free irq:%d\n", pci_dev->irq);
    free_irq(pci_dev->irq, NULL);
}

/* MSI interrupt*/
irqreturn_t
clx_msi_usr_handler(int irq, void *cookie)
{
    clx_intr_info_t *info = (clx_intr_info_t *)cookie;

    dbg_print(DBG_INTR, "in info:0x%lx,irq:%d\n", (unsigned long)info, info->irq);
    spin_lock(&clx_misc_dev->fifo_lock);
    if (!kfifo_put(&clx_misc_dev->intr_fifo, *info)) {
        spin_unlock(&clx_misc_dev->fifo_lock);
        return IRQ_HANDLED;
    }
    spin_unlock(&clx_misc_dev->fifo_lock);

    wake_up_interruptible(&clx_misc_dev->isr_wait_queue);

    return IRQ_HANDLED;
}

static int
clx_interrupt_msi_init(uint32_t unit)
{
    struct clx_pci_dev_s *ptr_clx_dev = clx_misc_dev->clx_pci_dev[unit];
    struct pci_dev *pci_dev = ptr_clx_dev->pci_dev;
    uint32_t msi_cnt;
    int rc;

    msi_cnt = pci_alloc_irq_vectors(pci_dev, 1, clx_intr_drv(unit)->msi_cnt, PCI_IRQ_MSI);
    if (msi_cnt != clx_intr_drv(unit)->msi_cnt) {
        dbg_print(DBG_ERR, "pci_alloc_irq_vectors failed. msi_cnt=%d\n", msi_cnt);
        return -EFAULT;
    }

    rc = clx_intr_drv(unit)->register_msi_irq(unit, pci_dev->irq);
    if (0 != rc) {
        dbg_print(DBG_ERR, "request_irq failed. unit=%d, irq=%d.\n", unit, pci_dev->irq);
        return rc;
    }

    return 0;
}

static void
clx_interrupt_msi_deinit(uint32_t unit)
{
    int i = 0;
    struct pci_dev *pci_dev = clx_misc_dev->clx_pci_dev[unit]->pci_dev;
    for (i = 0; i < clx_intr_drv(unit)->msi_cnt; i++) {
        free_irq(pci_irq_vector(pci_dev, i), clx_intr_drv(unit)->msi_vector[i].msi_cookie);
        kfree(clx_intr_drv(unit)->msi_vector[i].msi_cookie);
    }

    /* Must free the irq before disabling MSI */
    pci_disable_msi(pci_dev);
}

int
clx_interrupt_init(uint32_t intr_mode)
{
    int rc = 0;
    uint32_t unit = 0;

    for (unit = 0; unit < clx_misc_dev->pci_dev_num; unit++) {
        clx_intr_drv(unit)->intr_mode = intr_mode;
        if (intr_mode == INTR_MODE_INTX) {
            rc = clx_interrupt_intx_init(unit);
            if (0 != rc) {
                dbg_print(DBG_INTR, "Failed to init intx interrupt. unit=%d\n", unit);
            }
        } else if (intr_mode == INTR_MODE_MSI) {
            rc = clx_interrupt_msi_init(unit);
            if (0 != rc) {
                dbg_print(DBG_INTR, "Failed to init msi interrupt. unit=%d\n", unit);
            }
        } else {
            dbg_print(DBG_ERR, "Wrong interrupt mode. intr_mode = %d, unit=%d\n", intr_mode, unit);
            rc = -EFAULT;
        }
        return rc;
    }

    return 0;
}

void
clx_interrupt_deinit(void)
{
    uint32_t unit = 0;

    for (unit = 0; unit < clx_misc_dev->pci_dev_num; unit++) {
        if (clx_intr_drv(unit)->intr_mode == INTR_MODE_INTX) {
            clx_interrupt_intx_deinit(unit);
        } else if (clx_intr_drv(unit)->intr_mode == INTR_MODE_MSI) {
            clx_interrupt_msi_deinit(unit);
        } else {
            dbg_print(DBG_ERR, "Failed to init msi interrupt. unit=%d\n", unit);
        }
    }
}

int
clx_intx_connect_isr(uint32_t unit, unsigned long arg)
{
    struct clx_ioctl_intr_cookie kcookie;

    kcookie.intr_mode = clx_intr_drv(unit)->intr_mode;
    if (copy_to_user((void __user *)arg, &kcookie, sizeof(struct clx_ioctl_intr_cookie)))
        return -EFAULT;
    return 0;
}

int
clx_disconnect_isr(uint32_t unit, unsigned long arg)
{
    clx_intr_info_t info;
    unsigned long flags = 0;

    memset(&info, 0, sizeof(clx_intr_info_t));
    info.valid = CLX_INTR_INVALID_MSI;
    dbg_print(DBG_INTR, "in info:0x%lx, irq:%d\n", (unsigned long)&info, info.irq);
    spin_lock_irqsave(&clx_misc_dev->fifo_lock, flags);
    if (!kfifo_put(&clx_misc_dev->intr_fifo, info)) {
        spin_unlock(&clx_misc_dev->fifo_lock);
        return IRQ_HANDLED;
    }
    spin_unlock_irqrestore(&clx_misc_dev->fifo_lock, flags);
    wake_up_interruptible(&clx_misc_dev->isr_wait_queue);

    return 0;
}
