#ifndef BE_CONFIG_MODE_H
#define BE_CONFIG_MODE_H

#define FW_OPTION_LINE_SIDE_ANLT_ENABLE (1 << 9)
#define FW_OPTION_AUTONEG_OVERRIDE      (1 << 11)

#define HAL_FW_DEBUG_CMD_EX_OLD 1

/* Firmware regs */
#define FWREG(SECTION, OFFSET) (((SECTION) << 16) + (OFFSET))

#define FWREG_TOP_FW_ENABLE   FWREG(0, 9)
#define FWREG_NRZ_OPTICAL     FWREG(0, 20)
#define FWREG_TOP_TEMPERATURE FWREG(0, 170)

/* Firmware commands */
typedef enum {
    FW_CMD_HASH_READ = 0xF000,
    FW_CMD_CRC_READ = 0xF001,
    FW_CMD_DATE_READ = 0xF002,
    FW_CMD_VER_READ = 0xF003,

    FW_CMD_INTERNAL_REG_READ = 0xE010,
    FW_CMD_INTERNAL_REG_WRITE = 0xE020,

    FW_CMD_FW_LANE_RESET = 0xA000,
    FW_CMD_FW_RX_DISABLE = 0xA010,
    FW_CMD_FW_RX_ENABLE = 0xA020,
    FW_CMD_FW_LANE_TOP_RESET = 0xA030,

    FW_CMD_INFO = 0xB000,
    FW_CMD_CONFIG_MODE = 0x8000,
    FW_CMD_DESTROY_MODE = 0x9000,

    FW_CMD_CONFIG_TX = 0x7010,
    FW_CMD_RECOVER_TIMEOUT = 0x7020,
    FW_CMD_SD_DELAY = 0x7030
} FwCommand_t;

typedef enum {
    MODE_RETIMER_NRZ = 0,
    MODE_RETIMER_PAM4 = 1,
    MODE_RETIMER_CROSS_NRZ = 2,
    MODE_RETIMER_CROSS_PAM4 = 3,
    MODE_BITMUX_A1B2_NRZ = 4,
    MODE_BITMUX_A1B2_PAM4 = 5,
    MODE_BITMUX_A2B1_NRZ = 6,
    MODE_BITMUX_A2B1_PAM4 = 7,
    MODE_GEARBOX_100G_NRZ = 8,
    MODE_GEARBOX_100G_PAM4 = 9,
    MODE_GEARBOX_50G_NRZ = 10,
    MODE_GEARBOX_50G_PAM4 = 11,
    MODE_PHY_NRZ = 12,
    MODE_PHY_PAM4 = 13,
    MODE_LOOPBACK_NRZ = 14,
    MODE_LOOPBACK_PAM4 = 15,
} FirmwareMode_t;

typedef enum {
    CONFIG_SYS_SIDE_LT = (1 << 0),
    CONFIG_LINE_SIDE_ANLT = (1 << 1),
    CONFIG_GB_NOFEC = (1 << 2),
    CONFIG_AN_OVERRIDE = (1 << 3),
} ConfigFlag_t;

typedef enum {
    SPEED_UNKNOWN = -1,
    SPEED_OFF = 0,
    SPEED_10G = 1,
    SPEED_20G = 2,
    SPEED_25G = 3,
    SPEED_26G = 4,
    SPEED_28G = 5,
    SPEED_51G = 8,
    SPEED_53G = 9,
    SPEED_56G = 10,
} FirmwareSpeed_t;

typedef enum {
    CONFIG_10G = 10000,
    CONFIG_20G = 20000,  // not ethernet
    CONFIG_25G = 25000,
    CONFIG_26G = 26000,  // not ethernet
    CONFIG_28G = 28000,  // not ethernet
    CONFIG_40G = 40000,
    CONFIG_50G = 50000,
    CONFIG_51G = 51000,  // not ethernet
    CONFIG_53G = 53000,  // not ethernet
    CONFIG_56G = 56000,  // not ethernet
    CONFIG_100G = 100000,
    CONFIG_200G = 200000,
    CONFIG_400G = 400000,
} Speed_t;

#define TOP_DEBUG     0
#define NRZ_DEBUG     1
#define PAM4_DEBUG    2
#define RETIMER_DEBUG 5
#define ANLT_DEBUG    6
#define TOP_INFO      8
#define PAM4_INFO     10

typedef enum {
    TOP_DEBUG_MODE_CHANGE = 0,
    TOP_DEBUG_OPT_MODE = 1,
    TOP_DEBUG_RESET_BIT = 2,
    TOP_DEBUG_EXIT_CODE = 3,
    TOP_DEBUG_SPEED = 4,
    TOP_DEBUG_PLL_CAL = 5,
    TOP_DEBUG_CAL_COUNT = 8,
    TOP_DEBUG_VCOCAP_RX = 10,
    TOP_DEBUG_VCOCAP_TX = 11,
    TOP_DEBUG_SQUELCH_MODE_ENABLE = 20,
    TOP_DEBUG_TX_DISABLE_FLAG = 21,
    TOP_DEBUG_LT_ON = 36,
    TOP_DEBUG_TOP_FLAGS = 37,
    TOP_DEBUG_CONFIG_SEL = 38,
    TOP_DEBUG_LANE_LINK = 39,
    TOP_DEBUG_LANE_LINK_ALL = 40,
    TOP_DEBUG_TOP_LVL_STATE_DONE = 99,
    TOP_DEBUG_ALL_EXIT_CODES = 100,
} TopDebugCode_t;

typedef enum {
    NRZ_DEBUG_RX_STATE = 0,
    NRZ_DEBUG_RX_CTLE = 1,
    NRZ_DEBUG_RX_RATIO = 2,
    NRZ_DEBUG_RX_OF = 4,
    NRZ_DEBUG_RX_HF = 5,
    NRZ_DEBUG_RESTART_COUNT = 10,
} NRZDebugCode_t;

typedef enum {
    PAM4_DEBUG_RX_STATE = 0,
    PAM4_DEBUG_RX_CTLE_INDEX = 1,
    PAM4_DEBUG_RX_RATIO = 2,
    PAM4_DEBUG_RX_ADAPT_MODE = 3,
    PAM4_DEBUG_RX_OF = 4,
    PAM4_DEBUG_RX_HF = 5,
    PAM4_DEBUG_RESTART_COUNT = 7,
} PAM4DebugCode_t;

typedef enum {
    ANLT_DEBUG_AN_STATE = 0,
} ANLTDebugCode_t;

typedef enum {
    TOP_INFO_LINK_LOST_COUNT = 0,
    TOP_INFO_READAPT_COUNT = 1,
    TOP_INFO_TX_STATUS = 5,
    TOP_INFO_SD_DELAY = 10,
    TOP_INFO_SD_TIMEOUT = 11,
    TOP_INFO_LVL_STATE_DONE = 99,
} TopInfoCode_t;

typedef enum {
    PAM4_INFO_ISI = 0,
    PAM4_INFO_THRESHOLD = 1,
    PAM4_INFO_FFE = 3,
    PAM4_INFO_EYE = 5,
} PAM4InfoCode_t;

typedef enum {
    FW_TX_SOURCE_QUIET = 0,
    FW_TX_SOURCE_PRBS_PAM4 = 1,
    FW_TX_SOURCE_PRBS_NRZ = 2,
    FW_TX_SOURCE_DATA = 3,
    FW_TX_SOURCE_NOFORCE = 0xFF,
} FwTxSource_t;

typedef enum {
    TRF_Off,
    TRF_OnetoOne,
    TRF_GearBox,
} TRF_t;

#define DEFSPEED(...)      EXPAND__(DEFSPEED_n, NUM_ARGS(__VA_ARGS__), __VA_ARGS__)
#define DEFSPEED_n(N, ...) DEFSPEED_##N(__VA_ARGS__)
#define DEFSPEED_1(A)      DEFSPEED_2(A, A)
#define DEFSPEED_2(A, B)   (GLUE__(SPEED_, A) << 4 | GLUE__(SPEED_, B))
#define DEFLANE_INDEX(A, LANE) \
    (((LANE) > 11) ? ((A & 0xF000) | (LANE & 0x003F)) : ((A & 0xF000) | ((LANE << 6) & 0x0FC0)))

// exit_codes
#define ALL_EXIT_CODES_START                          (TOP_DEBUG_ALL_EXIT_CODES)
#define ALL_EXIT_CODES_END                            (ALL_EXIT_CODES_START + 8)
#define GET_FW_EXIT_CODES(slice, lane, index, pvalue) hal_fw_debug_cmd(slice, lane, TOP_DEBUG, index, pvalue)

#endif
