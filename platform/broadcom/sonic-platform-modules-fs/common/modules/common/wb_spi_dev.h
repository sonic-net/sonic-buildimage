#ifndef __WB_SPI_DEV_H__
#define __WB_SPI_DEV_H__
#include <wb_logic_dev_common.h>

typedef struct spi_dev_device_s {
    char spi_dev_name[MAX_NAME_SIZE];
    char spi_dev_alias[MAX_NAME_SIZE];
    uint32_t data_bus_width;
    uint32_t addr_bus_width;
    uint32_t per_rd_len;
    uint32_t per_wr_len;
    uint32_t spi_len;
    uint32_t status_check_type;                 /* 0: Not supported 1: Readback verification, 2: Readback anti-verification */
    uint32_t test_reg_num;                      /* The number of test registers is 8 at most */
    uint32_t test_reg[TEST_REG_MAX_NUM];        /* Test register address */
    uint32_t log_num;                           /* The number of write record registers is 64 at most */
    uint32_t log_index[BSP_KEY_DEVICE_NUM_MAX]; /* Write record register address = (base_addr & 0xFFFF) | (len << 16) */
    uint8_t share_high_addr_cmd_mask;           /* The command word shares the high address mask */
    uint8_t share_high_addr_rd_cmd;             /* The command word shares the high address read command */
    uint8_t share_high_addr_wr_cmd;             /* The command word shares the high address write command */
} spi_dev_device_t;

#endif
