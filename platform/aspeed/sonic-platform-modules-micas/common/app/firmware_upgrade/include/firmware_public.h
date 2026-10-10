#ifndef __FIRMWARE_PUBLIC_H__
#define __FIRMWARE_PUBLIC_H__

#define FIRMWARE_XILINX_FPGA_LOGIC_DEV        (0)
#define FIRMWARE_INTEL_FPGA_LOGIC_DEV         (1)
#define FIRMWARE_INTEL_FPGA_MAILBOX           (2)
#define FIRMWARE_DEV_NAME_LEN                 (64)    /* the macro definition needs to same as app space define */
#define FIRMWARE_LOGIC_DEV_NAME_LEN           (FIRMWARE_DEV_NAME_LEN)

typedef struct firmware_spi_logic_info_s {
    char dev_name[FIRMWARE_LOGIC_DEV_NAME_LEN];             /* Logical device name */
    uint32_t flash_base;                                    /* Flash Upgrade Address */
    uint32_t ctrl_base;                                     /* SPI upgrade control register base address */
    uint32_t test_base;                                     /* Test flash address */
    uint32_t test_size;                                     /* Test flash size */
    uint32_t logic_dev_type;                                /* Logical device type, 0: XILINX, 1: Intel */
} firmware_spi_logic_info_t;

#endif /* __FIRMWARE_PUBLIC_H__ */
