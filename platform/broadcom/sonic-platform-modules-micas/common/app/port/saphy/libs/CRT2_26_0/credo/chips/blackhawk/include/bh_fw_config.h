#ifndef BH_CONFIG_MODE_H
#define BH_CONFIG_MODE_H

#include "credo/base.h"
#include "credo/rsfec.h"

typedef struct fw_gearbox_info {
    CredoRSFECFifo_t fifo;

    unsigned cfg_pdiff_rx;
    unsigned cfg_pdiff_tx;

    unsigned cfg_pdiff_hw_rx_min;
    unsigned cfg_pdiff_hw_rx_max;
    unsigned cfg_pdiff_hw_tx_min;
    unsigned cfg_pdiff_hw_tx_max;

    unsigned cfg_pdiff_dev_rx;
    unsigned cfg_pdiff_dev_tx;

    unsigned ncw_avg_rx;
    unsigned ncw_avg_tx;

    unsigned bip_sum_rx;
} fw_gearbox_info_t;

typedef struct fw_gearbox_error_info {
    unsigned gb_error_code;

    unsigned fail_pdiff_rx;
    unsigned fail_pdiff_tx;
    unsigned fail_pdiff_rx_min;
    unsigned fail_pdiff_rx_max;
    unsigned fail_pdiff_rx_dev;
    unsigned fail_pdiff_tx_min;
    unsigned fail_pdiff_tx_max;
    unsigned fail_pdiff_tx_dev;

    unsigned fail_ncw_avg_rx;
    unsigned fail_ncw_avg_tx;
    unsigned fail_bip;

    unsigned first_fail_pdiff_rx;
    unsigned first_fail_pdiff_tx;
    unsigned first_fail_pdiff_rx_min;
    unsigned first_fail_pdiff_rx_max;
    unsigned first_fail_pdiff_rx_dev;
    unsigned first_fail_pdiff_tx_min;
    unsigned first_fail_pdiff_tx_max;
    unsigned first_fail_pdiff_tx_dev;

    unsigned first_fail_ncw_avg_rx;
    unsigned first_fail_ncw_avg_tx;
    unsigned first_fail_bip;

} fw_gearbox_error_info_t;

#define FW_FEATURE_ENABLE                      0xFFFFFF
#define FW_FEATURE_SUPPORT_40B_DATA            (1 << 0)
#define FW_FEATURE_GEARBOX_FIFO_ADVANCED       (1 << 1)
#define FW_FEATURE_GEARBOX_FW_PDIFF            (1 << 2)
#define FW_FEATURE_BITMUX_FW_PDIFF             (1 << 3)
#define FW_FEATURE_NEW_SPI_FW_CMD_AND_ECHO_REG (1 << 4)
#define FW_FEATURE_GEARBOX_FEC_CNT_FREEZE      (1 << 5)
#define FW_FEATURE_ACFG                        (1 << 6)

#define FW_OPTION_LINE_SIDE_ANLT_ENABLE (1 << 12)  // detail 1
#define FW_OPTION_SYS_SIDE_LT_ENABLE    (1 << 13)  // detail 1
#define FW_OPTION_AUTONEG_OVERRIDE      (1 << 14)  // detail 1

#define FW_OPTION_LINE_SIDE_AN_ENABLE (1 << 4)  // detail 2
#define FW_OPTION_LINE_SIDE_LT_ENABLE (1 << 5)  // detail 2

#define FW_LOAD_MAGIC_WORD 0x6A6A
#define FW_LOADING_SPI     0x2000
#define FW_LOAD_FROM_SPI   0xFFF0
#define FW_UNLOAD_WORD     0xFFF1
#define FW_UNLOAD_DONE     0x4000

#define FW_EM_HSTEP_SIDE      16
#define FW_EM_VSTEP_SIDE_PAM4 63
#define FW_EM_VSTEP_SIDE_NRZ  48
#define FW_EM_VSTEP_SEPARATOR 0x7FFF

#define FW_FFE_ACCU_SCALE 7
#define FW_FFE_WT_ROW     5
#define FW_FFE_WT_COL     7

#define MODE_AN_D_OVERRIDEN 0xFE

#define FW_TOPPLL_STATE_MASK 0xFF00
#define FW_TOPPLL_STATE_DONE 0xDF00

#define FW_RT_STATE_START          0
#define FW_RT_STATE_WAIT_PHY       1
#define FW_RT_STATE_WAIT_LT        2
#define FW_RT_STATE_ADJUST_FIFO    3
#define FW_RT_STATE_TRACK          4
#define FW_RT_STATE_IN_RECOVER     (1 << 4)
#define FW_RT_STATE_RECOVER_SERDES (FW_RT_STATE_IN_RECOVER + 1)
#define FW_RT_STATE_RECOVER_PHASE  (FW_RT_STATE_IN_RECOVER + 2)

#define FW_GB_STATE_START         0
#define FW_GB_STATE_WAIT_PHY      1
#define FW_GB_STATE_WAIT_LT       2
#define FW_GB_STATE_CHECK_RX_FIFO 3
#define FW_GB_STATE_RX2TX_WAIT    4
#define FW_GB_STATE_CHECK_FEC_A   5
#define FW_GB_STATE_CHECK_TX_FIFO 6
#define FW_GB_STATE_DONE          7
#define FW_GB_STATE_RELEASE_FIFO  8
#define FW_GB_STATE_PATCH_FIFO    9

#define FW_GB_BG_ERROR_MASK                     (1 << 12)
#define FW_GB_FEC_ERROR_CODE_MASK               (0xFFF)
#define FW_GB_FEC_ERROR_CODE_OK                 (1)
#define FW_GB_FEC_ERROR_CODE_RX                 (0xA00)
#define FW_GB_FEC_ERROR_CODE_TX                 (0xB00)
#define FW_GB_FEC_ERROR_FIFO_MESSED_UP_HW_PDIFF (0x10)
#define FW_GB_FEC_ERROR_FIFO_MESSED_UP_AVG      (0x11)
#define FW_GB_FEC_ERROR_FIFO_MESSED_UP_DEV      (0x12)
#define FW_GB_FEC_ERROR_FIFO_MESSED_UP_PRE      (0x14)
#define FW_GB_FEC_ERROR_FIFO_MESSED_UP_NCW      (0x18)
#define FW_GB_FEC_ERROR_FIFO_MESSED_UP_BIP      (0x20)

#define FW_GB_RX_FEC_ERROR_CODE_MISSING_ALIGNMENT 0xA00
#define FW_GB_RX_FEC_ERROR_CODE_BURST_ERROR       0xA01
#define FW_GB_RX_FEC_ERROR_CODE_UNCORRECTABLE_CFG 0xA02
#define FW_GB_RX_FEC_ERROR_CODE_FIFO_MESSED_UP_HW_PDIFF \
    (FW_GB_FEC_ERROR_CODE_RX | FW_GB_FEC_ERROR_FIFO_MESSED_UP_HW_PDIFF)
#define FW_GB_RX_FEC_ERROR_CODE_FIFO_MESSED_UP_AVG (FW_GB_FEC_ERROR_CODE_RX | FW_GB_FEC_ERROR_FIFO_MESSED_UP_AVG)
#define FW_GB_RX_FEC_ERROR_CODE_FIFO_MESSED_UP_DEV (FW_GB_FEC_ERROR_CODE_RX | FW_GB_FEC_ERROR_FIFO_MESSED_UP_DEV)
#define FW_GB_RX_FEC_ERROR_CODE_FIFO_MESSED_UP_PRE (FW_GB_FEC_ERROR_CODE_RX | FW_GB_FEC_ERROR_FIFO_MESSED_UP_PRE)
#define FW_GB_RX_FEC_ERROR_CODE_FIFO_MESSED_UP_NCW (FW_GB_FEC_ERROR_CODE_RX | FW_GB_FEC_ERROR_FIFO_MESSED_UP_NCW)

#define FW_GB_TX_FEC_ERROR_CODE_FIFO_MESSED_UP_HW_PDIFF \
    (FW_GB_FEC_ERROR_CODE_TX | FW_GB_FEC_ERROR_FIFO_MESSED_UP_HW_PDIFF)
#define FW_GB_TX_FEC_ERROR_CODE_FIFO_MESSED_UP_AVG (FW_GB_FEC_ERROR_CODE_TX | FW_GB_FEC_ERROR_FIFO_MESSED_UP_AVG)
#define FW_GB_TX_FEC_ERROR_CODE_FIFO_MESSED_UP_DEV (FW_GB_FEC_ERROR_CODE_TX | FW_GB_FEC_ERROR_FIFO_MESSED_UP_DEV)
#define FW_GB_TX_FEC_ERROR_CODE_FIFO_MESSED_UP_PRE (FW_GB_FEC_ERROR_CODE_TX | FW_GB_FEC_ERROR_FIFO_MESSED_UP_PRE)
#define FW_GB_TX_FEC_ERROR_CODE_FIFO_MESSED_UP_NCW (FW_GB_FEC_ERROR_CODE_TX | FW_GB_FEC_ERROR_FIFO_MESSED_UP_NCW)
#define FW_GB_TX_FEC_ERROR_CODE_FIFO_MESSED_UP_BIP (FW_GB_FEC_ERROR_CODE_TX | FW_GB_FEC_ERROR_FIFO_MESSED_UP_BIP)
#define FW_GB_FEC_ERROR_CODE_PHY_LOSS              0xC00
#define FW_GB_CFG_TIMEOUT                          0xD00
#define FW_GB_FEC_ERROR_CODE_NCW_TIMEOUT           0xE00

#define FW_BM_STATE_START        0
#define FW_BM_STATE_WAIT_PHY     1
#define FW_BM_STATE_WAIT_LT      2
#define FW_BM_STATE_RELEASE_FIFO 3
#define FW_BM_STATE_CHECK_FIFO   4
#define FW_BM_STATE_FUNCTIONAL   5
#define FW_BM_STATE_DONE         6

/* Firmware commands */
// TODO, change all enums to define
#define FW_CMD_FREEZE   0xD000
#define FW_CMD_UNFREEZE 0xD001

unsigned bh_fw_cmd_spi_read(CredoSlice_t* slice);
unsigned bh_fw_cmd_spi_write(CredoSlice_t* slice);
unsigned bh_fw_cmd_spi_gpi(CredoSlice_t* slice);
unsigned bh_fw_cmd_spi_status(CredoSlice_t* slice);
unsigned bh_fw_cmd_spi_erase(CredoSlice_t* slice);
#define FW_CMD_SPI_READ   bh_fw_cmd_spi_read(slice)
#define FW_CMD_SPI_WRITE  bh_fw_cmd_spi_write(slice)
#define FW_CMD_SPI_GPI    bh_fw_cmd_spi_gpi(slice)
#define FW_CMD_SPI_STATUS bh_fw_cmd_spi_status(slice)
#define FW_CMD_SPI_ERASE  bh_fw_cmd_spi_erase(slice)

typedef enum {
    FW_CMD_EYE_MON_START = 0x1000,
    FW_CMD_EYE_MON_PROG = 0x2000,
    FW_CMD_EYE_MON_READ = 0x3000,
    FW_CMD_EYE_MON_STOP = 0x4000,
    FW_CMD_STATE_LOAD_TOP = 0x5000,  // same as SPI currently
    FW_CMD_SPI_READ_OLD = 0x5000,
    FW_CMD_SPI_WRITE_OLD = 0x5010,
    FW_CMD_SPI_GPI_OLD = 0x5020,
    FW_CMD_SPI_STATUS_OLD = 0x5030,
    FW_CMD_SPI_ERASE_OLD = 0x5040,
    FW_CMD_SPI_READ_FB4 = 0x7090,    // feature bit 4
    FW_CMD_SPI_WRITE_FB4 = 0x7190,   // feature bit 4
    FW_CMD_SPI_GPI_FB4 = 0x7290,     // feature bit 4
    FW_CMD_SPI_STATUS_FB4 = 0x7390,  // feature bit 4
    FW_CMD_SPI_ERASE_FB4 = 0x7490,   // feature bit 4
    FW_CMD_CONFIG_TX = 0x7000,
    FW_CMD_TRF_CONTROL = 0x7020,
    FW_CMD_TX_PI_CONTROL = 0x7030,
    FW_CMD_LANE_RESET = 0x7040,
    FW_CMD_FORCE_LOOPBACK = 0x7050,
    FW_CMD_LANE_DISABLE = 0x7060,
    FW_CMD_TX_PRECODER_CTRL = 0x7070,
    FW_CMD_CLK_OUTPUT_CTRL = 0x7080,
    FW_CMD_RSFEC_FREEZE_CTRL = 0x70A0,

    FW_CMD_CONFIG_MODE = 0x8000,
    FW_CMD_DESTROY_MODE = 0x9000,
    FW_CMD_PORT_REGISTER = 0xA000,
    FW_CMD_PORT_QUERY = 0xA010,
    FW_CMD_PORT_FP_CONTROL = 0xA020,
    FW_CMD_PORT_QUERY_ALL = 0xA030,
    FW_CMD_INFO = 0xB000,
    FW_CMD_DUMP_MEM = 0xC000,
    FW_CMD_DUMP_MDIO = 0xC002,
    FW_CMD_INTERNAL_REG_READ = 0xE010,
    FW_CMD_INTERNAL_REG_WRITE = 0xE020,

    FW_CMD_HASH_READ = 0xF000,
    FW_CMD_CRC_READ = 0xF001,
    FW_CMD_DATE_READ = 0xF002,
    FW_CMD_VER_READ = 0xF003,
    FW_CMD_FEATURE_EN = 0xF005,
    FW_CMD_FEATURE = 0xF006,
} FwCommand_t;

typedef enum {
    MODE_RETIMER = 0,
    MODE_BITMUX_A1B2 = 1,
    MODE_BITMUX_A2B1 = 2,
    MODE_GEARBOX_50G = 6,
    MODE_PHY = 7,
    MODE_LOOPBACK = 8,
    MODE_GEARBOX_100G = 9,
} FirmwareMode_t;

typedef enum {
    SPEED_UNKNOWN = -1,
    SPEED_OFF = 0,
    SPEED_10G = 1,
    SPEED_20G = 2,
    SPEED_25G = 3,
    SPEED_26G = 4,
    SPEED_1G = 5,
    SPEED_27_9G = 6,  // 27.95237
    SPEED_28G = 7,    // 28.076177
    SPEED_53G = 8,
    SPEED_55_9G = 9,    // 55.90474
    SPEED_56G = 10,     // 56.25
    SPEED_55G = 11,     // 55.0
    SPEED_56_15G = 12,  // 56.152354
    SPEED_28_125G = 13,
} FirmwareSpeed_t;

typedef enum {
    CONFIG_1G = 1000,
    CONFIG_10G = 10000,
    CONFIG_20G = 20000,
    CONFIG_25G = 25000,
    CONFIG_26G = 26000,
    CONFIG_27G = 27000,
    CONFIG_27_95G = 27950,
    CONFIG_28G = 28000,
    CONFIG_28_125G = 28125,
    CONFIG_40G = 40000,
    CONFIG_50G = 50000,
    CONFIG_51G = 51000,
    CONFIG_53G = 53000,
    CONFIG_55G = 55000,
    CONFIG_55_9G = 55900,
    CONFIG_56G = 56000,
    CONFIG_56_15G = 56150,
    CONFIG_56_25G = 56250,
    CONFIG_100G = 100000,
    CONFIG_106G = 106000,
} Speed_t;

typedef enum {
    FW_FEC_OFF = 0,
    FW_FEC_FIRE = 1,
    FW_FEC_528 = 2,
    FW_FEC_544 = 3,
} FirmwareFecType_t;

typedef enum {
    TOP_DEBUG = 0,
    NRZ_DEBUG = 1,
    PAM4_DEBUG = 2,
    RETIMER_DEBUG = 3,
    BITMUX_DEBUG = 4,
    GEARBOX_DEBUG = 5,
    ANLT_DEBUG = 7,
    TOP_INFO = 8,
    NRZ_INFO = 9,
    PAM4_INFO = 10,
} InfoSection_t;

typedef enum {
    TOP_DEBUG_MODE_CHANGE = 0,
    TOP_DEBUG_OPT_MODE = 1,
    TOP_DEBUG_RESET_BIT = 2,
    TOP_DEBUG_EXIT_CODE = 3,
    TOP_DEBUG_SPEED = 4,
    TOP_DEBUG_PLL_CAL = 5,
    TOP_DEBUG_CAL_START_TIME = 6,
    TOP_DEBUG_CAL_DONE_TIME = 7,
    TOP_DEBUG_CAL_COUNT = 8,
    TOP_DEBUG_VCOCAP_RX = 10,
    TOP_DEBUG_VCOCAP_TX = 11,
    TOP_DEBUG_RESTART_COUNT = 15,
    TOP_DEBUG_ANLT_ON = 17,
    TOP_DEBUG_SQUELCH_MODE_ENABLE = 20,
    TOP_DEBUG_TX_DISABLE_FLAG = 21,
    TOP_DEBUG_TOP_FLAGS = 37,
    TOP_DEBUG_CONFIG_SEL = 38,
    TOP_DEBUG_TX_PRECODER = 43,
    TOP_DEBUG_CLK_OUTPUT_SQUELCH = 44,
    TOP_DEBUG_VCOCAP_TX_RESULT = 45,
    TOP_DEBUG_VCOCAP_RX_RESULT = 46,
    TOP_DEBUG_VCOCAP_COST_TIME = 47,
    TOP_DEBUG_VCOCAP_TRGT_CNT_MSB = 48,
    TOP_DEBUG_VCOCAP_TRGT_CNT_LSB = 49,
    TOP_DEBUG_ACFG_STATE = 60,
    TOP_DEBUG_ACFG_ERROR_CODE = 61,
    TOP_DEBUG_ACFG_MCU_ID_LSB = 62,
    TOP_DEBUG_ACFG_MCU_ID_MSB = 63,
    TOP_DEBUG_ACFG_PRE_HW_REGISTERS = 64,
    TOP_DEBUG_VCOCAP_TOP_DEBUG = 200,
    TOP_DEBUG_VCOCAP_RX_DEBUG = 400,
    TOP_DEBUG_VCOCAP_TX_DEBUG = 600,
    TOP_DEBUG_ALL_EXIT_CODES = 100,
    TOP_DEBUG_INTERNAL_VER = 999,
} TopDebugCode_t;

typedef enum {
    NRZ_DEBUG_RX_STATE = 0,
    NRZ_DEBUG_RX_OF = 4,
    NRZ_DEBUG_RX_HF = 5,
    NRZ_DEBUG_RX_RATIO = 6,
    NRZ_DEBUG_TIME_CRASHED = 20,
    NRZ_DEBUG_TIME_SIGNAL_RECOVER = 21,
    NRZ_DEBUG_TIME_LINK_BACK = 22,
    NRZ_DEBUG_RECOVER_COUNT = 23,
    NRZ_DEBUG_RECOVER_DELTA = 24,
    NRZ_DEBUG_RECOVER_EYE_RAW = 25,
    NRZ_DEBUG_ATTN_OF = 100,
    NRZ_DEBUG_CA_OF_CNT = 120,
    NRZ_DEBUG_CA_HF_CNT = 140,
    NRZ_DEBUG_DC_SEARCH_CNT = 150,
    NRZ_DEBUG_CTLE_EM = 300,
    NRZ_DEBUG_DFE = 310,
    NRZ_DEBUG_TIMERS = 320,
    NRZ_DEBUG_RECOVER_THS = 340,
} NRZDebugCode_t;

typedef enum {
    PAM4_DEBUG_RX_STATE = 0,
    PAM4_DEBUG_RX_OF = 4,
    PAM4_DEBUG_RX_HF = 5,
    PAM4_DEBUG_RX_RATIO = 6,
    PAM4_DEBUG_SKEF_BEST = 9,
    PAM4_DEBUG_EDGE_FM1 = 10,
    PAM4_DEBUG_FFE_POL_BEFORE = 11,
    PAM4_DEBUG_FFE_POL_AFTER = 12,
    PAM4_DEBUG_F13_INIT = 13,
    PAM4_DEBUG_TIMERS = 80,
    PAM4_DEBUG_SKEF_ISI2 = 200,
    PAM4_DEBUG_ISI_CTLE_SEARCH1 = 220,
    PAM4_DEBUG_ISI_CTLE_SEARCH2 = 224,
    PAM4_DEBUG_CTLE_SEARCH = 230,
    PAM4_DEBUG_CA_HF_CNT = 300,
    PAM4_DEBUG_CA_CNT3 = 310,
    PAM4_DEBUG_CA_CNT0 = 320,
    PAM4_DEBUG_SKEF_RECORD = 330,
    PAM4_DEBUG_DELTA_FM1 = 350,
    PAM4_DEBUG_ATTN_OF = 360,
    PAM4_DEBUG_DELTA_DELTA = 380,

    // LF
    PAM4_DEBUG_SMART_CHECK_THS = 1000,
    PAM4_DEBUG_FORCE_THS = 1100,
    PAM4_DEBUG_PLUS_MARGIN = 1200,
    PAM4_DEBUG_MINUS_MARGIN = 1300,
    PAM4_DEBUG_LF_RESULT = 1400,
    PAM4_DEBUG_EM_DEBUG = 1500,
} PAM4DebugCode_t;

typedef enum {
    RETIMER_DEBUG_STATE = 0,
    RETIMER_DEBUG_FIFO_HEAL_COUNT = 1,
} RetimerDebugCode_t;

typedef enum {
    BITMUX_DEBUG_STATE = 0,
    BITMUX_DEBUG_FIFO_HEAL_COUNT = 7,
    BITMUX_DEBUG_RD_DIFF_PT = 20,
} BitmuxDebugCode_t;

typedef enum {
    GEARBOX_DEBUG_STATE = 0,
    GEARBOX_DEBUG_TX_CFG_PDIFF = 20,
    GEARBOX_DEBUG_RX_CFG_PDIFF = 21,
    GEARBOX_DEBUG_LAST_BG_ERROR_CODE = 23,
    GEARBOX_DEBUG_FIFO_HEAL_COUNT = 30,
    GEARBOX_DEBUG_ERROR_CODE = 31,
    GEARBOX_DEBUG_TX_BG_PDIFF = 32,
    GEARBOX_DEBUG_RX_BG_PDIFF = 33,
    GEARBOX_DEBUG_TX_BG_PDIFF_MIN = 34,
    GEARBOX_DEBUG_TX_BG_PDIFF_MAX = 35,
    GEARBOX_DEBUG_RX_BG_PDIFF_MIN = 36,
    GEARBOX_DEBUG_RX_BG_PDIFF_MAX = 37,
    GEARBOX_DEBUG_TX_BG_PDIFF_DEV = 38,
    GEARBOX_DEBUG_RX_BG_PDIFF_DEV = 39,
    GEARBOX_DEBUG_TX_BG_FAIL_PDIFF = 60,
    GEARBOX_DEBUG_RX_BG_FAIL_PDIFF = 61,
    GEARBOX_DEBUG_TX_BG_FAIL_PDIFF_MIN = 62,
    GEARBOX_DEBUG_TX_BG_FAIL_PDIFF_MAX = 63,
    GEARBOX_DEBUG_RX_BG_FAIL_PDIFF_MIN = 64,
    GEARBOX_DEBUG_RX_BG_FAIL_PDIFF_MAX = 65,
    GEARBOX_DEBUG_TX_BG_FAIL_PDIFF_FIRST = 66,
    GEARBOX_DEBUG_RX_BG_FAIL_PDIFF_FIRST = 67,
    GEARBOX_DEBUG_TX_BG_FAIL_PDIFF_MIN_FIRST = 68,
    GEARBOX_DEBUG_TX_BG_FAIL_PDIFF_MAX_FIRST = 69,
    GEARBOX_DEBUG_RX_BG_FAIL_PDIFF_MIN_FIRST = 70,
    GEARBOX_DEBUG_RX_BG_FAIL_PDIFF_MAX_FIRST = 71,
    GEARBOX_DEBUG_TX_BG_FAIL_PDIFF_DEV = 72,
    GEARBOX_DEBUG_RX_BG_FAIL_PDIFF_DEV = 73,
    GEARBOX_DEBUG_TX_BG_FAIL_PDIFF_DEV_FIRST = 74,
    GEARBOX_DEBUG_RX_BG_FAIL_PDIFF_DEV_FIRST = 75,
    GEARBOX_DEBUG_TX_BG_NCW_AVG_MSB = 83,
    GEARBOX_DEBUG_TX_BG_NCW_AVG_LSB = 84,
    GEARBOX_DEBUG_RX_BG_NCW_AVG_MSB = 85,
    GEARBOX_DEBUG_RX_BG_NCW_AVG_LSB = 86,
    GEARBOX_DEBUG_TX_BG_FAIL_NCW_AVG_MSB = 87,
    GEARBOX_DEBUG_TX_BG_FAIL_NCW_AVG_LSB = 88,
    GEARBOX_DEBUG_RX_BG_FAIL_NCW_AVG_MSB = 89,
    GEARBOX_DEBUG_RX_BG_FAIL_NCW_AVG_LSB = 90,
    GEARBOX_DEBUG_TX_BG_FAIL_NCW_AVG_FIRST_MSB = 91,
    GEARBOX_DEBUG_TX_BG_FAIL_NCW_AVG_FIRST_LSB = 92,
    GEARBOX_DEBUG_RX_BG_FAIL_NCW_AVG_FIRST_MSB = 93,
    GEARBOX_DEBUG_RX_BG_FAIL_NCW_AVG_FIRST_LSB = 94,
    GEARBOX_DEBUG_RX_BG_FEC_BIP_MSB = 95,
    GEARBOX_DEBUG_RX_BG_FEC_BIP_LSB = 96,
    GEARBOX_DEBUG_TX_CFG_PDIFF_HW_MIN = 98,
    GEARBOX_DEBUG_TX_CFG_PDIFF_HW_MAX = 99,
    GEARBOX_DEBUG_RX_CFG_PDIFF_HW_MIN = 100,
    GEARBOX_DEBUG_RX_CFG_PDIFF_HW_MAX = 101,
    GEARBOX_DEBUG_BG_FAIL_BIP_MSB = 102,
    GEARBOX_DEBUG_BG_FAIL_BIP_LSB = 103,
    GEARBOX_DEBUG_BG_FAIL_BIP_FIRST_MSB = 104,
    GEARBOX_DEBUG_BG_FAIL_BIP_FIRST_LSB = 105,
    GEARBOX_DEBUG_UNCORR_CW = 160,
    GEARBOX_DEBUG_TOTAL_BLK_CNTR = 161,
} GearboxDebugCode_t;

typedef enum {
    ANLT_DEBUG_AN_STATE = 0,
    ANLT_DEBUG_AN_MODE = 2,
    ANLT_DEBUG_LD_PAGES = 300,
    ANLT_DEBUG_LP_PAGES = 330,
} ANLTDebugCode_t;

typedef enum {
    TOP_INFO_LINK_LOST_COUNT = 0,
    TOP_INFO_READAPT_COUNT = 1,
    TOP_INFO_PORT_INFO = 2,
    TOP_INFO_LINE_MAC_STATUS = 3,
    TOP_INFO_SYS_MAC_STATUS = 4,
    TOP_INFO_TX_STATUS = 5,
    TOP_INFO_FP_STATUS = 6,
    TOP_INFO_LOS_COUNT = 7,
    TOP_INFO_SD_DELAY = 10,
    TOP_INFO_SD_TIMEOUT = 11,
    TOP_INFO_LANE_LINK = 39,
    TOP_INFO_LANE_LINK_ALL = 40,
    TOP_INFO_LVL_STATE_DONE = 99,
} TopInfoCode_t;

typedef enum {
    NRZ_INFO_ISI = 0,
} NRZInfoCode_t;

typedef enum {
    PAM4_INFO_ISI = 0,
    PAM4_INFO_THRESHOLD = 1,
    PAM4_INFO_FFE = 2,
    PAM4_INFO_FFE_NBIAS = 3,
    PAM4_INFO_FFE_KACCU = 4,
    PAM4_INFO_EYE = 5,
    PAM4_INFO_FFE_WT0 = 10,
    PAM4_INFO_FFE_WT1 = 11,
    PAM4_INFO_FFE_WT2 = 12,
    PAM4_INFO_FFE_WT3 = 13,
    PAM4_INFO_FFE_WT4 = 14,
    PAM4_INFO_FFE_JUMP = 16,
} PAM4InfoCode_t;

typedef enum {
    TOP_LOAD_RECOVER_SD_TIMEOUT = 0x8001,
    TOP_LOAD_SD_DELAY = 0x8002,
} TopLoadCode_t;

typedef enum {
    FW_TX_SOURCE_QUIET = 0,
    FW_TX_SOURCE_PRBS_PAM4 = 1,
    FW_TX_SOURCE_PRBS_NRZ = 2,
    FW_TX_SOURCE_DATA = 3,
    FW_TX_SOURCE_NOFORCE = 0xFF,
} FwTxSource_t;

typedef enum {
    FW_RX_CONTROL_RESET = 0,
    FW_RX_CONTROL_ENABLE = 1,
    FW_RX_CONTROL_DISABLE = 5,
} FwRxControl_t;

typedef enum {
    TRF_Off = 0,
    TRF_OnetoOne = 1,
    TRF_NOFORCE = 0xFF,
} TRF_t;

typedef enum {
    FW_EM_SUCCESS = 0,
    FW_EM_PROG_REPORT = 1,
    FW_EM_DATA_RETURN = 2,
    FW_EM_ERROR = 3,  // fw command error too
    FW_EM_ERROR_NOT_READY = 4,
    FW_EM_ERROR_GOING_ON = 5,
    FW_EM_ERROR_CANCELL = 6,
    FW_EM_ERROR_NOT_START = 7,
} FirmwareEMState_t;

/* Firmware internal regs */
// Refer: https://stackoverflow.com/a/11314760/9174589
#define TOP_BIT_SET -2147483648  // ISO C standard doesnt support (1 << 31) as enum treated as int

#define FWREG(SECTION, OFFSET)   (((SECTION) << 16) + (OFFSET))
#define FWREG32(SECTION, OFFSET) ((SECTION) << 16) + (OFFSET) + TOP_BIT_SET

typedef enum {
    FW_REG_TOP = 0,
    FW_REG_QUAILLP_TOP = 1,
    FW_REG_QUAILLP_PAM4 = 2,
    FW_REG_QUAILLP_NRZ = 3,
    FW_REG_AN = 4,
    FW_REG_LT = 5,
} FirmwareRegSection_t;

#define FWREG_TOP_SM_ENABLE          FWREG(FW_REG_TOP, 2)
#define FWREG_TOP_OPTIONS            FWREG(FW_REG_TOP, 6)
#define FWREG_TOP_TEMPERATURE        FWREG(FW_REG_TOP, 11)
#define FWREG_TOP_TX_PRECODER        FWREG(FW_REG_TOP, 13)
#define FWREG_TOP_SCRATCH            FWREG32(FW_REG_TOP, 16)
#define FWREG_TOP_UNCORR_MONITOR     FWREG(FW_REG_TOP, 25)
#define FWREG_TOP_CKO_INFO           FWREG32(FW_REG_TOP, 26)
#define FWREG_TOP_VCOCAP_STATE       FWREG(FW_REG_TOP, 29)
#define FWREG_TOP_1G_OVERSAMPLE      FWREG(FW_REG_TOP, 185)
#define FWREG_QTOP_VCO_LOCK_CRITERIA FWREG(FW_REG_QUAILLP_TOP, 16)
#define FWREG_PAM4_SD_DELAY_OPTICAL  FWREG(FW_REG_QUAILLP_PAM4, 236)
#define FWREG_PAM4_SD_DELAY          FWREG(FW_REG_QUAILLP_PAM4, 237)
#define FWREG_NRZ_SD_DELAY_OPTICAL   FWREG(FW_REG_QUAILLP_NRZ, 85)
#define FWREG_NRZ_SD_DELAY           FWREG(FW_REG_QUAILLP_NRZ, 86)

#define TOP_OPTION_VCOCAP_CAL        (1 << 0)
#define TOP_OPTION_BG_AUTO_RECOVER   (1 << 4)
#define TOP_OPTION_ANLT_POWER_SAVING (1 << 6)
#define TOP_OPTION_FEC_ADV_INFO      (1 << 7)

#define DEFSPEED(...)      EXPAND__(DEFSPEED_n, NUM_ARGS(__VA_ARGS__), __VA_ARGS__)
#define DEFSPEED_n(N, ...) DEFSPEED_##N(__VA_ARGS__)
#define DEFSPEED_1(A)      DEFSPEED_2(A, A)
#define DEFSPEED_2(A, B)   (GLUE__(SPEED_, A) << 4 | GLUE__(SPEED_, B))
#define DEFLANE_INDEX(A, LANE) \
    (((LANE) > 11) ? ((A & 0xF000) | (LANE & 0x003F)) : ((A & 0xF000) | ((LANE << 6) & 0x0FC0)))

#define DEFLANE_INDEX_n(N, ...) DEFLANE_INDEX_##N(__VA_ARGS__)
#define DEFLANE_INDEX_1(LANE)   (((LANE) > 11) ? ((LANE & 0x003F)) : ((LANE << 6) & 0x0FC0))
#define DEFLANE_INDEX_2(A, B)   ((A << 6) | B)

// exit_codes
#define ALL_EXIT_CODES_START                          (TOP_DEBUG_ALL_EXIT_CODES)
#define ALL_EXIT_CODES_END                            (ALL_EXIT_CODES_START + 16)
#define GET_FW_EXIT_CODES(slice, lane, index, pvalue) hal_fw_debug_cmd(slice, lane, TOP_DEBUG, index, pvalue)

#define TOP_DEBUG_ACFG_STATE_OK (255)

#endif
