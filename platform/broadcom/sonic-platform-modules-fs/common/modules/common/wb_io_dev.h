#ifndef __WB_IO_DEV_H__
#define __WB_IO_DEV_H__
#include <wb_logic_dev_common.h>

#define IO_DEV_USE_IOPORT             (0)
#define IO_DEV_USE_IOMEM              (1)

#define IO_DATA_MODE(io_type, io_width) \
    (u32)(((u32)(io_type) << 16) | (io_width))

typedef struct io_dev_device_s {
    char io_dev_name[MAX_NAME_SIZE];
    char io_dev_alias[MAX_NAME_SIZE];
    void __iomem * mem_vir_base;
    uint8_t io_type;            /* 0:ioport 1:iomem*/
    uint32_t io_base;
    uint32_t io_len;
    uint32_t indirect_addr;
    uint32_t wr_data;
    uint32_t wr_data_width;
    uint32_t addr_low;
    uint32_t addr_high;
    uint32_t rd_data;
    uint32_t rd_data_width;
    uint32_t opt_ctl;
    int device_flag;
    uint32_t status_check_type;                 /* 0: Not supported 1: Readback verification, 2: Readback anti-verification */
    uint32_t test_reg_num;                      /* The number of test registers is 8 at most */
    uint32_t test_reg[TEST_REG_MAX_NUM];        /* Test register address */
    uint32_t log_num;                           /* The number of write record registers is 64 at most */
    uint32_t log_index[BSP_KEY_DEVICE_NUM_MAX]; /* Write record register address = (base_addr & 0xFFFF) | (len << 16) */
} io_dev_device_t;

#endif
