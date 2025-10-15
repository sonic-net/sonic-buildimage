#ifndef __WB_LPC_BMC_H__
#define __WB_LPC_BMC_H__

/* LPC Interface */
#define LPC_ADDR_PORT_4E               (0x4E)
#define LPC_DATA_PORT_4F               (0x4F)
#define LPC_ADDR_PORT_2E               (0x2E)
#define LPC_DATA_PORT_2F               (0x2F)

#define BMC_REG_DEV_NAME            "lpc_bmc"
#define BMC_MASTER_FLASH_NAME       "bmc_flash_master"
#define BMC_SLAVE_FLASH_NAME        "bmc_flash_slave"
#define DEV_RDWR_MAX_LEN            (256)
#define DEV_WIDTH                   (4)
#define DEV_REG_MAX_LEN             (0x20000000)
#define FLASH_WR_MAX_LEN            (1024)

#define TYPE_AST2500       (1)
#define TYPE_AST2600       (2)

/* FMC REGISTER ADDR */
#define FMC_BASE_ADDR                             (0x1E620000)
#define FMC_CE_TYPE_SETTING_REG                   (FMC_BASE_ADDR + 0x00)
#define CE_CONTROL_REGISTER                       (FMC_BASE_ADDR + 0x04)
#define INR_STATUS_CONTROL_REGISTER               (FMC_BASE_ADDR + 0x08)
#define COMMAND_CONTROL_REGISTER                  (FMC_BASE_ADDR + 0x0C)
#define CE0_CONTROL_REGISTER                      (FMC_BASE_ADDR + 0x10)
#define CE1_CONTROL_REGISTER                      (FMC_BASE_ADDR + 0x14)
#define CE0_ADDRESS_RANGE_REGISTER                (FMC_BASE_ADDR + 0x30)
#define CE1_ADDRESS_RANGE_REGISTER                (FMC_BASE_ADDR + 0x34)
#define FMC_WDT2_CONTROL_STATUS_REGISTER          (FMC_BASE_ADDR + 0x64)
#define FMC_WDT2_TIMER_RELOAD_VALUE_REGISTER      (FMC_BASE_ADDR + 0x68)
#define WRITE_ADDR_FILTER_REGISTER                (FMC_BASE_ADDR + 0xA4)

/* SCU REGISTER ADDR */
#define SCU_ADDR                    (0x1E6E2000)
/* AST25 */
#define AST25_HARDWARE_STRAP_REGISTER             (SCU_ADDR + 0x70)
#define AST25_REBOOT_CPU_REGISTER                 (SCU_ADDR + 0x7C)
#define AST25_SILICON_REVISION_ID_REGISTER        (SCU_ADDR + 0x7C)

/* AST26 */
#define AST26_PROTECTION_KEY_REGISTER_1           (SCU_ADDR + 0)
#define AST26_SILICON_REVISION_ID_REGISTER        (SCU_ADDR + 0x04)
#define AST26_PROTECTION_KEY_REGISTER_2           (SCU_ADDR + 0x10)
#define AST26_HARDWARE_STRAP_REGISTER             (SCU_ADDR + 0x500)
#define AST26_HARDWARE_STRAP_REGISTER_CLEAR       (SCU_ADDR + 0x504)

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

#define AST26_WATCHDOG2_ADDR_BASE         (0x1E785040)
#define AST26_WATCHDOG2_RELOAD_VALUE      (AST26_WATCHDOG2_ADDR_BASE + 0x04)
#define AST26_WATCHDOG2_COUNTER_RST       (AST26_WATCHDOG2_ADDR_BASE + 0x08)
#define AST26_WATCHDOG2_CONTROL           (AST26_WATCHDOG2_ADDR_BASE + 0x0C)
#define AST26_WATCHDOG2_TSR               (AST26_WATCHDOG2_ADDR_BASE + 0x10)
#define AST26_WATCHDOG2_CLEAR_STATUS      (AST26_WATCHDOG2_ADDR_BASE + 0x14)
#define AST26_WATCHDOG2_RESET_FUN_MASK_1  (AST26_WATCHDOG2_ADDR_BASE + 0x1C)
#define AST26_WATCHDOG2_RESET_FUN_MASK_2  (AST26_WATCHDOG2_ADDR_BASE + 0x20)

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
#define DISABLE_ADDR_LINE           (0x000000F0)
#define DISABLE_DATA_LINE           (0x0000000F)
#define ENABLE_ADDR_LINE            (0x00000000)
#define ENABLE_DATA_LINE            (0x00000000)

#define CHIP_ERASE_TIME             (60)                /* Wait 60 seconds for full erase */
#define CHIP_ERASE_TIMEOUT          (300 * 1000 * 1000) /* The total chip erase timeout period of polling is 300s */
#define CHIP_ERASE_SLEEP_TIME       (5 * 1000 * 1000)   /* Poll the whole tablet to erase the sleep time 5s */
#define BLOCK_ERASE_TIMEOUT         (10 * 1000 * 1000)  /* The polling block erasing timeout period is 10s */
#define BLOCK_ERASE_SLEEP_TIME      (100 * 1000)        /* Polling blocks erase sleep time 100ms */
#define PAGE_PROGRAM_TIMEOUT        (100 * 1000)        /* The polling page program timeout duration is 100ms */
#define PAGE_PROGRAM_SLEEP_TIME     (1000)              /* Polling page program Sleep time 1ms */
#define FLASH_WEL_TIMEOUT           (100 * 1000)        /* The polling write status timeout duration is 100ms */
#define FLASH_WEL_SLEEP_TIME        (1000)              /* Polling write enabled state Sleep duration 1ms */
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

#define AST26_WATCHDOG_GATEMASK_2       (0x03FFFFF3)
#define DISABLE_AST26_WATCHDOG          (0x00000000)
#define AST26_ENABLE_WATCH_CMD          (0x33)
#define AST26_ENABLE_FMC_WATCH_CMD      (0x1)
#define AST26_FMC_WATCHDOG_NEW_COUNT    (0x3)
#define AST26_FMC_WATCHDOG_DISABLE      (0x0)

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
#define AST26_AHB_BASE_PHY_ADDR     (0x20000000)
#define AST25_SEGMENT_ADDR_START(_r)      ((((_r) >> 16) & 0xFF) << 23)
#define AST26_SEGMENT_ADDR_START(_r)      ((((_r) & 0xFFFF) << 16) | AST26_AHB_BASE_PHY_ADDR)

#define CE0_ADDRESS_4               (0x00000001)
#define CE1_ADDRESS_4               (0x00000002)
#define CE2_ADDRESS_4               (0x00000004)
#define CE0_DIV2_MODE               (0x00000100)
#define CE1_DIV2_MODE               (0x00000200)
#define CE2_DIV2_MODE               (0x00000400)
#define CE_CONTROL_SET              (CE0_ADDRESS_4 | CE1_ADDRESS_4 | CE0_DIV2_MODE | \
                                         CE1_DIV2_MODE | CE2_DIV2_MODE)

#define DEFAULT_BASE_VALUE          (0x80000000)
#define CE0_WRITE_ENABLE            (0x00010000)
#define CE1_WRITE_ENABLE            (0x00020000)
#define CE2_WRITE_ENABLE            (0x00040000)
#define CE0_SPI_TYPE                (0x00000002)
#define CE1_SPI_TYPE                (0x00000008)
#define CE2_SPI_TYPE                (0x00000020)
#define BASE_SET                    (DEFAULT_BASE_VALUE | CE0_WRITE_ENABLE | CE1_WRITE_ENABLE |     \
                                        CE2_WRITE_ENABLE | CE0_SPI_TYPE | CE1_SPI_TYPE | CE2_SPI_TYPE)

#define WRITE_ADDR_FILTER_EN1       (0x00000002)
#define WRITE_ADDR_FILTER_EN2       (0x00000008)
#define WRITE_ADDR_FILTER_EN3       (0x00000020)
#define WRITE_ADDR_FILTER_EN4       (0x00000080)
#define WRITE_ADDR_FILTER_EN5       (0x00000200)
#define WRITE_ADDR_FILTER_ENABLE    (WRITE_ADDR_FILTER_EN1 | WRITE_ADDR_FILTER_EN2 | \
                                        WRITE_ADDR_FILTER_EN3 | WRITE_ADDR_FILTER_EN4 | \
                                        WRITE_ADDR_FILTER_EN5)

#define WRITE_ADDR_FILTER_DIS1      (0x00000000)
#define WRITE_ADDR_FILTER_DIS2      (0x00000000)
#define WRITE_ADDR_FILTER_DIS3      (0x00000000)
#define WRITE_ADDR_FILTER_DIS4      (0x00000000)
#define WRITE_ADDR_FILTER_DIS5      (0x00000000)
#define WRITE_ADDR_FILTER_DISABLE   (WRITE_ADDR_FILTER_DIS1 | WRITE_ADDR_FILTER_DIS2 | \
                                        WRITE_ADDR_FILTER_DIS3 | WRITE_ADDR_FILTER_DIS4 | \
                                        WRITE_ADDR_FILTER_DIS5)

#define DISABLE_ADDR_DATA_LINE      (DISABLE_ADDR_LINE | DISABLE_DATA_LINE)

/* Write/Read Command */
#define FLASH_WRITE_ENABLE          (0x00060000)

#define DECODE_DATA                 (0x00000000)
#define WRITE_ENABLE                (FLASH_WRITE_ENABLE | WRITE_MODE)

#define ENABLE_ADDR_DATA_LINE       (ENABLE_ADDR_LINE | ENABLE_DATA_LINE)

typedef enum lpc_addr_type {
    MODE_4E = 0,
    MODE_2E = 1,
} lpc_addr_type_t;

/* User Mode Command */
#define MASK                        (0xFF)

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
    GD25B512M = 0X1a47c8,
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

typedef enum ast_bmc_id_list {
    AST2500_A0  = 0x04000303,
    AST2510_A0  = 0x04000103,
    AST2520_A0  = 0x04000203,
    AST2530_A0  = 0x04000403,

    AST2500_A1  = 0x04010303,
    AST2510_A1  = 0x04010103,
    AST2520_A1  = 0x04010203,
    AST2530_A1  = 0x04010403,

    AST2500_A2  = 0x04030303,
    AST2510_A2  = 0x04030103,
    AST2520_A2  = 0x04030203,
    AST2530_A2  = 0x04030403,

    AST2600_A0  = 0x05000303,
    AST2600_A1  = 0x05010303,
    AST2600_A2  = 0x05010303,
    AST2600_A3  = 0x05030303,

    AST2620_A1  = 0x05010203,
    AST2620_A2  = 0x05010203,
    AST2620_A3  = 0x05030203,
} ast_bmc_id_list_t;

typedef struct bmc_id_list_s {
    uint32_t bmc_ids[64];
    int bmc_type;
    uint32_t bmc_id_reg;
} bmc_id_list_t;

static flash_info_t flash_info[] = {
    {
        .flash_name = "M25L6433F",
        .flash_size = M32,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = MX25L6433F,
        .block_size = STEP_64,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "S25FL512S",
        .flash_size = M64,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = S25FL512S,
        .block_size = STEP_256,
        .full_erase = 0,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "MX25l512",
        .flash_size = M64,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = MX25l512,
        .block_size = STEP_64,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "STM25P64",
        .flash_size = M12,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = STM25P64,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "STM25P128",
        .flash_size = M16,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = STM25P128,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "N25Q256",
        .flash_size = M16,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = N25Q256,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "N25Q512",
        .flash_size = M16,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = N25Q512,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "W25X16",
        .flash_size = M3,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = W25X16,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "W25X64",
        .flash_size = M12,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = W25X64,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "W25Q64BV",
        .flash_size = M12,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = W25Q64BV,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "W25Q128BV",
        .flash_size = M16,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = W25Q128BV,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "W25Q256FV",
        .flash_size = M16,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = W25Q256FV,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "MX25L1605D",
        .flash_size = M32,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = MX25L1605D,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "MX25L12805D",
        .flash_size = M32,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = MX25L12805D,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "MX66L1G45G",
        .flash_size = M128,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = MX66L1G45G,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "GD25Q256",
        .flash_size = M16,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = GD25Q256,
        .block_size = STEP_256,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
    {
        .flash_name = "GD25B512M",
        .flash_size = M64,
        .flash_type = SPI,
        .page_size = BYTE_256,
        .flash_id = GD25B512M,
        .block_size = STEP_64,
        .full_erase = 1,
        .erase_block_command = BLOCK_ERASE_64,
        .page_program = COMMON_PAGE_PROGRAM,
    },
};

#endif
