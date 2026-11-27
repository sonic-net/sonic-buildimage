#ifndef __netif_DEV_H__
#define __netif_DEV_H__

#include "netif_types.h"
#include "netif_pub.h"

#include <linux/wait.h>
#include <linux/kfifo.h>
#include <linux/spinlock.h>
#include <linux/miscdevice.h>

/* to be updated， netif object*/
#define USE_VIRT_DEV
#if defined(USE_VIRT_DEV)
#define gt_netif_obj(chip_id) gt_netif_obj->gt_virt_dev[chip_id]->gt_drv->pkt_drv
#else
#define gt_netif_drv(unit) gt_misc_dev->gt_pci_dev[unit]->gt_drv->pkt_drv
#endif

typedef int (*pci_read_func_t)(int unit, uint32_t offset, uint32_t *ptr_data, uint32_t len);

typedef int (*pci_write_func_t)(int unit, uint32_t offset, uint32_t *ptr_data, uint32_t len);

/* PCI DRV */
typedef struct {
    uint32_t dma_bit_mask;
    uint32_t mmio_bar;
} gt_pci_drv_cb_t;

typedef struct {
    gt_pci_drv_cb_t *pci_drv;
    gt_netif_drv_cb_t *pkt_drv;
} gt_drv_cb_t;

struct gt_pci_dev_s {
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
    gt_drv_cb_t *gt_drv;
};

// test netif performance
typedef struct {
    // control
    uint32_t rx_enable_test;
    uint32_t tx_enable_test;
    uint32_t target_len;
    uint32_t target_count;
    // statistic
    uint64_t packet_count;
    uint64_t byte_count;
    uint64_t interrupt_count;
    unsigned long start_time;
    unsigned long end_time;
} netif_perf_t;

struct gt_virt_dev_s {
    uint32_t chip;
    gt_drv_cb_t       *gt_drv;   /* full driver cb for virt dev init */
};

typedef struct {
    struct miscdevice misc_dev;
    // struct kfifo intr_fifo;
    DECLARE_KFIFO_PTR(intr_fifo, gt_intr_info_t);
    spinlock_t fifo_lock;
    wait_queue_head_t isr_wait_queue;

    netif_perf_t test_perf;
    struct gt_pci_dev_s *gt_pci_dev[gt_MAX_CHIP_NUM]; // pci device
    uint32_t pci_dev_num;                                // chip num
    struct gt_virt_dev_s *gt_virt_dev[gt_MAX_VIRT_DEV_NUM]; // virtual  device
    uint32_t virt_dev_num;
} gt_device_t;

extern gt_device_t *gt_netif_obj;

#define GT_CHECK_PTR(__ptr__)                                     \
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
        if (0 != ((__flag__) & (ext_dbg_flag))) {                                     \
            printk("GT_KERN %s:%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        }                                                                          \
    } while (0)


int
gt_drv_init(uint32_t unit, struct pci_dev *dev);
int
gt_drv_deinit(uint32_t unit, struct pci_dev *dev);

int
gt_pkt_drv_init(uint32_t unit);

void
gt_pkt_drv_deinit(uint32_t unit);

int
netif_obj_init(uint32_t unit, struct pci_dev *dev);

int
gt_virt_dev_init(uint32_t unit);

int
netif_virt_init(uint32_t unit, gt_drv_cb_t *ptr_gt_drv);

#endif // __netif_DEV_H__
