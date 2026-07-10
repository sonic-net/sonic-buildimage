#ifndef __FW_UPG_SPI_LOGIC_DEV_H__
#define __FW_UPG_SPI_LOGIC_DEV_H__

#define FIRMWARE_FPGA_WORD_LEN         (4)

#define FIRMWARE_LOGIC_DEV_NAME_LEN          (64)    /* the macro definition needs to same as FIRMWARE_DEV_NAME_LEN in firmware_sysfs_upgrade.h */
#define FIRMWARE_SPI_LOGIC_UPG_RETRY_CNT     (10)
#define FIRMWARE_SPI_LOGIC_UPG_BUFF_SIZE     (256)
#define FIRMWARE_SPI_LOGIC_SECTOR_SIZE       (0x10000)     /* One sector is 64Kk */

#define FIRMWARE_UPG_RETRY_SLEEP_TIME        (10)   /* 10us */
#define FIRMWARE_UPG_RETRY_TIME_CNT          (1000)
#define FPGA_UPG_WAIT_SPI_RETRY_CNT           (100)
#define FPGA_UPG_WAIT_SPI_RETRY_SLEEP_TIME    (1000 * 10)   /* 10ms */

#define FIRMWARE_FPGA_UPG_RETRY_CNT          (100)

/* FPGA upgrades related instruction definitions */
#define FPGA_UPG_INSTRUTION_SE                (0xD8)
#define FPGA_UPG_INSTRUTION_RDSR              (0x05)
#define FPGA_UPG_INSTRUTION_WREN              (0x06)
#define FPGA_UPG_INSTRUTION_PP                (0x02)
#define FPGA_UPG_INSTRUTION_FR                (0x0B)
#define FPGA_UPG_INSTRUTION_BE                (0xC7)
#define FPGA_UPG_STATUS_MASK                  (0x1)
#define FPGA_UPG_ACCESS_ENABLE                (0x3)
#define FPGA_UPG_SPI_STATUS_MASK              (0x1)
#define FFPGA_UPG_DATA_SIZE                   (256)

#define FPGA_UPG_RETRY_TIMES                  (3)

/* FPGA upgrades the offset of the associated register */
#define FPGA_UPG_STATUS_REG                   (0x180)
#define FPGA_UPG_SPI_CTRL_REG                 (0x184)
#define FPGA_UPG_WR_FLASH_STATUS_REG          (0x188)
#define FPGA_UPG_RD_FLASH_STATUS_REG          (0x18C)
#define FPGA_UPG_INSTRUCTION_REG              (0x190)
#define FPGA_UPG_ADDR_REG                     (0x194)
#define FPGA_UPG_LENGTH_REG                   (0x198)
#define FPGA_UPG_DEVICE_ID_REG                (0x19C)
#define FPGA_UPG_DROP_REQ_NUM_REG             (0x1A8)

typedef struct firmware_spi_logic_info_s {
    char logic_dev_name[FIRMWARE_LOGIC_DEV_NAME_LEN]; /* Logical device name */
    uint32_t flash_base;                                   /* Flash Upgrade Address */
    uint32_t ctrl_base;                                    /* SPI upgrade control register base address */
    uint32_t test_base;                                    /* Test flash address */
    uint32_t test_size;                                    /* Test flash size */
} firmware_spi_logic_info_t;

typedef struct firmware_spi_logic_upg_s {
    char dev_path[FIRMWARE_LOGIC_DEV_NAME_LEN];
    uint32_t flash_base;      /* Flash Upgrade Address */
    uint32_t ctrl_base;       /* SPI upgrade control register base address */
    uint32_t status_reg;
    uint32_t spi_ctrl_reg;
    uint32_t wr_flash_status_reg;
    uint32_t rd_flash_status_reg;
    uint32_t instruction_reg;
    uint32_t addr_reg;
    uint32_t length_reg;
    uint32_t device_id_reg;
    uint32_t drop_reg_num_reg;
    uint32_t test_base;          /* Test flash address */
    uint32_t test_size;          /* Test flash size */
}firmware_spi_logic_upg_t;

typedef enum firmware_spi_flash_rv_s {
    FW_SPI_FLASH_RV_OK      = 0,
    FW_SPI_FLASH_STATUS_ERR,
    FW_SPI_FLASH_BUSY,
    FW_SPI_FLASH_SPI_BUSY,
    FW_SPI_FLASH_WR_ENABLE_ERR,
    FW_SPI_FLASH_ERASE_ADDR_ERR,
    FW_SPI_FLASH_ERASE_SECTOR_ERR,
    FW_SPI_FLASH_WR_ERR,
    FW_SPI_FLASH_RD_ERR,
    FW_SPI_FLASH_PARAM_ERR,
    FW_SPI_FLASH_UPG_ERR,
    FW_SPI_FLASH_WR_LENGTH_ERR,
    FW_SPI_FLASH_WR_ADDR_ERR,
    FW_SPI_FLASH_SET_ACCESS_ERR,
    FW_SPI_FLASH_DATA_CMP_ERR,
    FW_SPI_FLASH_GET_INFO_ERR,
    FW_SPI_FLASH_NOT_SUPPORT_TEST,
} firmware_spi_flash_rv_t;

int fpga_test_spi_logic_flash(int argc, char *argv[]);

#endif /* End of __FW_UPG_SPI_LOGIC_DEV_H__ */
