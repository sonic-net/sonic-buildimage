#ifndef __WB_PCIE_DEV_H__
#define __WB_PCIE_DEV_H__
#include <wb_logic_dev_common.h>

typedef struct pci_dev_device_s {
    char pci_dev_name[MAX_NAME_SIZE];
    char pci_dev_alias[MAX_NAME_SIZE];
    int pci_domain;
    int pci_bus;
    int pci_slot;
    int pci_fn;
    int pci_bar;
    int bus_width;
    uint32_t check_pci_id;
    uint32_t pci_id;
    int device_flag;
    int search_mode;
    int bridge_bus;
    int bridge_slot;
    int bridge_fn;
    uint32_t status_check_type;                 /* 0: Not supported 1: Readback verification, 2: Readback anti-verification */
    uint32_t test_reg_num;                      /* The number of test registers is 8 at most */
    uint32_t test_reg[TEST_REG_MAX_NUM];        /* Test register address */
    uint32_t log_num;                           /* The number of write record registers is 64 at most */
    uint32_t log_index[BSP_KEY_DEVICE_NUM_MAX]; /* Write record register address = (base_addr & 0xFFFF) | (len << 16) */
} pci_dev_device_t;

#endif
