#ifndef __CLX_INTR_H__
#define __CLX_INTR_H__

#include "knet_types.h"
#include <linux/interrupt.h>
#include "knet_dev.h"
#include "knet_pci.h"
#include "knet_common.h"

typedef int (*mask_top_irq)(uint32_t unit);
typedef int (*mask_dma_irq)(uint32_t unit, uint32_t channel);
typedef int (*unmask_dma_irq)(uint32_t unit, uint32_t channel);
typedef int (*clear_dma_irq)(uint32_t unit, uint32_t channel);
typedef int (*read_dma_irq)(uint32_t unit, uint32_t *channel_bmp);
typedef int (*register_msi_irq)(uint32_t unit, uint32_t irq);

typedef struct {
    irq_handler_t handler;
    void *msi_cookie;
} msi_isr_vector_t;

/* INTERRUPT DRV */
typedef struct {
    uint32_t intr_mode;
    uint32_t msi_cnt;
    msi_isr_vector_t *msi_vector;

    mask_top_irq top_irq_mask;
    mask_top_irq top_irq_unmask;
    // dma interrupt
    mask_top_irq dma_irq_mask_all;
    mask_top_irq dma_irq_unmask_all;
    read_dma_irq read_dma_irq_status;
    mask_dma_irq mask_dma_channel_irq;
    unmask_dma_irq unmask_dma_channel_irq;
    clear_dma_irq clear_dma_channel_irq;
    // dma error interrupt
    read_dma_irq read_dma_error_irq_status;
    mask_dma_irq mask_dma_channel_error_irq;
    unmask_dma_irq unmask_dma_channel_error_irq;
    clear_dma_irq clear_dma_channel_error_irq;

    register_msi_irq register_msi_irq;
} clx_intr_drv_cb_t;

irqreturn_t
clx_msi_usr_handler(int irq, void *cookie);

/* ioctl */
int
clx_intx_connect_isr(uint32_t unit, unsigned long arg);
int
clx_disconnect_isr(uint32_t unit, unsigned long arg);

/* init */
int
clx_interrupt_init(uint32_t intr_mode);
void
clx_interrupt_deinit(void);

#endif // __CLX_INTR_H__