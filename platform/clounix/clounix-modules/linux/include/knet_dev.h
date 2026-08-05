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

#ifndef __CLX_DEV_H__
#define __CLX_DEV_H__

#include "knet_types.h"
#include "knet_netif.h"
#include "knet_intr.h"
#include "knet_dma.h"
#include "knet_common.h"

#include <linux/wait.h>
#include <linux/kfifo.h>
#include <linux/spinlock.h>
#include <linux/miscdevice.h>

#define clx_pci_cb(unit)    clx_misc_dev->clx_pci_dev[unit]->clx_drv->pci_drv
#define clx_dma_drv(unit)   clx_misc_dev->clx_pci_dev[unit]->clx_drv->dma_drv
#define clx_intr_drv(unit)  clx_misc_dev->clx_pci_dev[unit]->clx_drv->intr_drv
#define clx_netif_drv(unit) clx_misc_dev->clx_pci_dev[unit]->clx_drv->pkt_drv

typedef int (*pci_read_func_t)(int unit, uint32_t offset, uint32_t *ptr_data, uint32_t len);

typedef int (*pci_write_func_t)(int unit, uint32_t offset, uint32_t *ptr_data, uint32_t len);

/* PCI DRV */
typedef struct {
    uint32_t dma_bit_mask;
    uint32_t mmio_bar;
} clx_pci_drv_cb_t;

typedef struct {
    clx_pci_drv_cb_t *pci_drv;
    clx_netif_drv_cb_t *pkt_drv;
    clx_intr_drv_cb_t *intr_drv;
    clx_dma_drv_cb_t *dma_drv;
} clx_drv_cb_t;

struct clx_pci_dev_s {
    uint32_t unit;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t revision;
    void __iomem *bar_virt;
    unsigned long bar_phys;
    unsigned long bar_len;
    pci_read_func_t read_cb;
    pci_write_func_t write_cb;
    struct pci_dev *pci_dev;
    clx_drv_cb_t *clx_drv;
};

// test netif performance
typedef struct {
    // control
    uint32_t enable_test;
    uint32_t target_len;
    uint32_t target_count;
    // statistic
    uint64_t packet_count;
    uint64_t byte_count;
    uint64_t interrupt_count;
    unsigned long start_time;
    unsigned long end_time;
} netif_perf_t;

typedef struct {
    struct miscdevice misc_dev;
    // struct kfifo intr_fifo;
    DECLARE_KFIFO_PTR(intr_fifo, clx_intr_info_t);
    spinlock_t fifo_lock;
    wait_queue_head_t isr_wait_queue;

    netif_perf_t test_perf;
    struct clx_pci_dev_s *clx_pci_dev[CLX_MAX_CHIP_NUM]; // pci device
    uint32_t pci_dev_num;                                // chip num
} clx_device_t;

#define CLX_CHECK_PTR(__ptr__)                                     \
    do {                                                           \
        if (NULL == (__ptr__)) {                                   \
            dbg_print(DBG_CRIT, "%s is null pointer\n", #__ptr__); \
            return (-EINVAL);                                      \
        }                                                          \
    } while (0)

/* This flag value will be specified when user inserts kernel module. */
#define DBG_CRIT       (0x1UL << 0)
#define DBG_ERR        (0x1UL << 1)
#define DBG_WARN       (0x1UL << 2)
#define DBG_INFO       (0x1UL << 3)
#define DBG_TX         (0x1UL << 4)
#define DBG_RX         (0x1UL << 5)
#define DBG_TX_PAYLOAD (0x1UL << 6)
#define DBG_RX_PAYLOAD (0x1UL << 7)
#define DBG_INTF       (0x1UL << 8)
#define DBG_PROFILE    (0x1UL << 9)
#define DBG_NETLINK    (0x1UL << 10)
#define DBG_INTR       (0x1UL << 11)
#define DBG_DEBUG      (0x1UL << 12)

#define dbg_print(__flag__, fmt, ...)                                              \
    do {                                                                           \
        if (0 != ((__flag__) & (verbosity))) {                                     \
            printk("CLX_KERN %s:%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        }                                                                          \
    } while (0)

extern uint32_t verbosity;
extern clx_device_t *clx_misc_dev;

int
clx_drv_init(uint32_t unit, struct pci_dev *dev);
int
clx_drv_cleanup(uint32_t unit, struct pci_dev *dev);
int
clx_ioctl_get_pci_dev_info(uint32_t unit, unsigned long arg);

#endif // __CLX_DEV_H__