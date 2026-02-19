/*******************************************************************************
 *  Copyright Statement:
 *  --------------------
 *  This software and the information contained therein are protected by
 *  copyright and other intellectual property laws and terms herein is
 *  confidential. The software may not be copied and the information
 *  contained herein may not be used or disclosed except with the written
 *  permission of Clounix (Shanghai) Technology Limited. (C) 2020-2025
 *
 *  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("CLOUNIX SOFTWARE")
 *  RECEIVED FROM CLOUNIX AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
 *  AN "AS-IS" BASIS ONLY. CLOUNIX EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 *  NEITHER DOES CLOUNIX PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 *  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 *  SUPPLIED WITH THE CLOUNIX SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
 *  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. CLOUNIX SHALL ALSO
 *  NOT BE RESPONSIBLE FOR ANY CLOUNIX SOFTWARE RELEASES MADE TO BUYER'S
 *  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *  BUYER'S SOLE AND EXCLUSIVE REMEDY AND CLOUNIX'S ENTIRE AND CUMULATIVE
 *  LIABILITY WITH RESPECT TO THE CLOUNIX SOFTWARE RELEASED HEREUNDER WILL BE,
 *  AT CLOUNIX'S OPTION, TO REVISE OR REPLACE THE CLOUNIX SOFTWARE AT ISSUE,
 *  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
 *  CLOUNIX FOR SUCH CLOUNIX SOFTWARE AT ISSUE.
 *
 *  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
 *  WITH THE LAWS OF THE PEOPLE'S REPUBLIC OF CHINA, EXCLUDING ITS CONFLICT OF
 *  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
 *  RELATED THERETO SHALL BE SETTLED BY LAWSUIT IN SHANGHAI,CHINA UNDER.
 *
 *******************************************************************************/

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
typedef bool (*usr_interrupt_irq)(uint32_t unit);

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

    // other interrupt
    usr_interrupt_irq usr_interrupt_exist;

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