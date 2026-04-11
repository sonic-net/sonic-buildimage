#ifndef __WB_INDIRECT_DEV_H__
#define __WB_INDIRECT_DEV_H__
#include <wb_logic_dev_common.h>

typedef struct indirect_dev_device_s {
    char dev_name[MAX_NAME_SIZE];
    char dev_alias[MAX_NAME_SIZE];
    char logic_dev_name[MAX_NAME_SIZE];
    uint32_t data_bus_width;
    uint32_t indirect_len;
    uint32_t wr_data;
    uint32_t addr_low;
    uint32_t addr_high;
    uint32_t rd_data;
    uint32_t opt_ctl;
    uint32_t logic_func_mode;
    uint32_t lock_mode;
    int device_flag;
    uint32_t status_check_type;                 /* 0: Not supported 1: Readback verification, 2: Readback anti-verification */
    uint32_t test_reg_num;                      /* The number of test registers is 8 at most */
    uint32_t test_reg[TEST_REG_MAX_NUM];        /* Test register address */
    uint32_t log_num;                           /* The number of write record registers is 64 at most */
    uint32_t log_index[BSP_KEY_DEVICE_NUM_MAX]; /* Write record register address = (base_addr & 0xFFFF) | (len << 16) */
} indirect_dev_device_t;

#endif /* __WB_INDIRECT_DEV_H__ */
