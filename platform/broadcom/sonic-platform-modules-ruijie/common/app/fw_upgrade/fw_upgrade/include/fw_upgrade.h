#ifndef _FW_UPGRADE_H_
#define _FW_UPGRADE_H_

#include "fw_upgrade_debug.h"

#define dbg_print(debug, fmt, arg...)  \
    if (debug == DEBUG_APP_ON || debug == DEBUG_ALL_ON) \
        { do{printf(fmt,##arg);} while(0); }

/* LPC Interface */
#define LPC_ADDR_PORT               (0x4E)
#define LPC_DATA_PORT               (0x4F)

/* FMC REGISTER ADDR */
#define FMC_BASE_ADDR               (0x1E620000)
#define FMC_CE_TYPE_SETTING_REG     (FMC_BASE_ADDR + 0x00)
#define CE_CONTROL_REGISTER         (FMC_BASE_ADDR + 0x04)
#define INR_STATUS_CONTROL_REGISTER (FMC_BASE_ADDR + 0x08)
#define COMMAND_CONTROL_REGISTER    (FMC_BASE_ADDR + 0x0C)
#define CE0_CONTROL_REGISTER        (FMC_BASE_ADDR + 0x10)
#define CE1_CONTROL_REGISTER        (FMC_BASE_ADDR + 0x14)
#define CE0_ADDRESS_RANGE_REGISTER  (FMC_BASE_ADDR + 0x30)
#define CE1_ADDRESS_RANGE_REGISTER  (FMC_BASE_ADDR + 0x34)

/* SCU REGISTER ADDR */
#define SCU_ADDR                    (0x1E6E2000)
#define HARDWARE_STRAP_REGISTER     (SCU_ADDR + 0x70)
#define REBOOT_CPU_REGISTER         (SCU_ADDR + 0x7C)

/* SCU KEY */
#define UNLOCK_SCU_KEY              (0x1688A8A8)
#define LOCK_SCU_KEY                (0x11111111)

/* WATCHDOG REGISTER ADDR */
#define WATCHDOG_ADDR               (0x1E785000)
#define WATCHDOG1_RELOAD_VALUE      (WATCHDOG_ADDR + 0x04)
#define WATCHDOG1_COUNTER_RST       (WATCHDOG_ADDR + 0x08)
#define WATCHDOG1_CONTROL           (WATCHDOG_ADDR + 0x0C)
#define WATCHDOG1_TSR               (WATCHDOG_ADDR + 0x10)
#define WATCHDOG1_CLEAR_STATUS      (WATCHDOG_ADDR + 0x14)
#define WATCHDOG1_RESET_FUN_MASK    (WATCHDOG_ADDR + 0x1C)

#define WATCHDOG2_RELOAD_VALUE      (WATCHDOG_ADDR + 0x24)
#define WATCHDOG2_COUNTER_RST       (WATCHDOG_ADDR + 0x28)
#define WATCHDOG2_CONTROL           (WATCHDOG_ADDR + 0x2C)
#define WATCHDOG2_TSR               (WATCHDOG_ADDR + 0x30)
#define WATCHDOG2_CLEAR_STATUS      (WATCHDOG_ADDR + 0x34)
#define WATCHDOG2_RESET_FUN_MASK    (WATCHDOG_ADDR + 0x3C)

/* User Mode Command */
#define WRITE_STATUS                (0x01)
#define COMMON_PAGE_PROGRAM         (0x02)
#define COMMON_FLASH_READ           (0x03)
#define WRITE_DISABLE_FLASH         (0x04)
#define READ_FLASH_STATUS           (0x05)
#define WRITE_ENABLE_FLASH          (0x06)
#define PAGE_PROGRAM_FLASH          (0x12)
#define SECTOR_ERASE                (0x20)
#define CLEAR_FLAG                  (0x50)
#define SUBBLOCK_ERASE              (0x52)
#define CHIP_ERASE_FLASH            (0x60)
#define BLOCK_ERASE_64              (0xD8)
#define READID                      (0x9F)
#define ENABLE_BYTE4                (0xB7)
#define EXIT_OTP                    (0xC1)
#define RSTEN                       (0x66)
#define RST                         (0x99)

#define BIT1                        (0x01)
#define BIT2                        (0x02)
#define BIT3                        (0x04)
#define BIT4                        (0x08)
#define BIT5                        (0x10)
#define BIT6                        (0x20)
#define BIT7                        (0x40)
#define BIT8                        (0x80)
#define RIGHT_SHIFT_8(reg)          (reg >> 8)
#define RIGHT_SHIFT_16(reg)         (reg >> 16)
#define RIGHT_SHIFT_24(reg)         (reg >> 24)
#define MASK                        (0xFF)
#define FLASH_TYPE_MASK             (BIT1 | BIT2)
#define BOOT_DEFAULT_MASK           (BIT8)
#define HEAD_MASK                   (0x00FFFF00)
#define MASK_BYTE                   (0xFF000000)
#define BYTE1                       (1)
#define BYTE2                       (2)
#define BYTE4                       (4)
#define BYTE1_VAL                   (0)
#define BYTE2_VAL                   (1)
#define BYTE4_VAL                   (2)
#define BYTE_RESERVED               (3)

/* SuperIO */
#define SUPERIO_07                  (0x07)
#define SUPERIO_30                  (0x30)
#define SUPERIO_A0                  (0xA0)
#define SUPERIO_A2                  (0xA2)
#define SUPERIO_REG0                (0xF0)
#define SUPERIO_REG1                (0xF1)
#define SUPERIO_REG2                (0xF2)
#define SUPERIO_REG3                (0xF3)
#define SUPERIO_REG4                (0xF4)
#define SUPERIO_REG5                (0xF5)
#define SUPERIO_REG6                (0xF6)
#define SUPERIO_REG7                (0xF7)
#define SUPERIO_REG8                (0xF8)
#define SUPERIO_FE                  (0xFE)

/* SPI Command */
#define HIGH_CLOCK                  (0x00000000)
#define NORMAL_READ                 (0x00000000)
#define READ_MODE                   (0x00000001)
#define WRITE_MODE                  (0x00000002)
#define USER_MODE                   (0x00000003)
#define PULL_DOWN                   (0x00000000)
#define PULL_UP                     (0x00000004)

#define CHIP_ERASE_TIME             (60)
#define CHIP_ERASE_TIMEOUT          (300 * 1000 * 1000)
#define CHIP_ERASE_SLEEP_TIME       (5 * 1000 * 1000)
#define BLOCK_ERASE_TIMEOUT         (10 * 1000 * 1000)
#define BLOCK_ERASE_SLEEP_TIME      (100 * 1000)
#define PAGE_PROGRAM_TIMEOUT        (100 * 1000)
#define PAGE_PROGRAM_SLEEP_TIME     (1000)
#define FLASH_WEL_TIMEOUT           (100 * 1000)
#define FLASH_WEL_SLEEP_TIME        (1000)
#define FLASH_WIP_MASK              (0x00000001)
#define FLASH_WRITE_ENABLE_MASK     (0x00000002)

#define DATA_LENGTH_MASK            (0xA2)
#define TOGGLE_WRITE                (0xCF)
#define DISABLE_LPC                 (0xAA)
#define ENABLE_LPC                  (0xA5)
#define LPC_TO_AHB                  (0x0D)
#define ENABLE_LPC_TO_AHB           (0x01)
#define DISABLE_LPC_TO_AHB          (0x00)
#define ENABLE_BMC_CPU_BOOT         (0xF10BD286)
#define DISABLE_BMC_CPU_BOOT        (0xF10BD287)
#define SET_BMC_CPU_BOOT            (0x01)
#define CLEAR_WATCHDOG_STATUS       (0x01)
#define DISABLE_WATCHDOG            (0x00000030)
#define ENABLE_WATCHDOG             (0x00000033)
#define WATCHDOG_GATEMASK           (0x033FFFF3)
#define WATCHDOG_NEW_COUNT          (0x00050000)
#define WATCHDOG_RELOAD_COUNTER     (0x4755)

#define CE0_SPI_TYPE                (0x00000002)
#define CE1_SPI_TYPE                (0x00000008)
#define ERROR_COMMAND               (0x00000400)
#define ADDRESS_PROTECT             (0x00000200)
#define CLEAR_INR_STATUS_CONTROL    (ERROR_COMMAND | ADDRESS_PROTECT)
#define USER_MODE_PULL_CE_DOWN      (HIGH_CLOCK | USER_MODE | PULL_DOWN)
#define USER_MODE_PULL_CE_UP        (HIGH_CLOCK | USER_MODE | PULL_UP)

#define STEP_64                     (64 * 1024)
#define STEP_256                    (256 * 1024)
#define BYTE_256                    (256)

#define CE0                         (0)
#define CE1                         (1)
#define BOTHFLASH                   (2)
#define SOC_SYS                     (0)
#define FULL_CHIP                   (1)
#define ARM_CPU                     (2)
#define FULL_ERASE                  (0)
#define BLOCK_ERASE                 (1)
#define READ_ALL                    (2)
#define CURRENT_SLAVE               (1)
#define CURRENT_MASTER              (0)
#define REGISTER_HEAD               (0x1e000000)
#define DEFAULT_WIDTH               (16)
#define MAX_FILENAME_LENGTH         (64)
#define SEGMENT_ADDR_START(_r)      ((((_r) >> 16) & 0xFF) << 23)

typedef struct flash_info {
    uint32_t flash_size;
    int cs;
    int flash_type;
    uint32_t flash_id;
    int page_size;
    char flash_name[64];
    int erase_block_command;
    int page_program;
    int block_size;
    int full_erase;
    uint32_t ce_control_reg;
    uint32_t flash_base_addr;
} flash_info_t;

typedef enum flash_id {
    MX25L6433F = 0x1920c2,
    S25FL512S = 0x200201,
    MX25l512 = 0x1a20c2,
    STM25P64 = 0x172020,
    STM25P128 = 0x182020,
    N25Q256 = 0x19ba20,
    N25Q512 = 0x20ba20,
    W25X16 = 0x1530ef,
    W25X64 = 0x1730ef,
    W25Q64BV = 0x1740ef,
    W25Q128BV = 0x1840ef,
    W25Q256FV = 0x1940ef,
    MX25L1605D = 0x1520C2,
    MX25L12805D = 0x1820C2,
    MX66L1G45G = 0x1B20C2,
    SST25VF016B	= 0x4125bf,
    SST25VF064C = 0x4b25bf,
    SST25VF040B	= 0x8d25bf,
    AT25DF161 = 0x02461F,
    AT25DF321 = 0x01471F,
    GD25Q256 = 0X1940c8,
} flash_id_t;

typedef enum flash_type {
    NOR = 0,
    SPI = 2,
} flash_type_t;

typedef enum flash_size {
    M1  = 0x00080000,
    M3  = 0x00200000, /* 3M */
    M6  = 0x00400000, /* 6M */
    M12 = 0x00800000, /* 12M */
    M16 = 0x01000000, /* 16M */
    M32 = 0x02000000, /* 32M */
    M64 = 0x04000000, /* 64M */
    M128 = 0x08000000, /* 128M */
} flash_size_t;

#endif  /*_FW_UPGRADE_H_*/
