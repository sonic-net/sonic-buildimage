#ifndef BLACKHAWK_H
#define BLACKHAWK_H

#include "bh_fw_config.h"

#include "condor_lp/condor_lp_regmap.h"

extern const RegHive_t Top[], TopPLL[], TopLane[], TopSensor[];

#define HAL_CHIP_12NM
#define HAL_SUPPORT_ANLT        1
#define HAL_SUPPORT_FEC_ANA     1
#define HAL_SUPPORT_TOP_PLL_CAL 1
#define HAL_SUPPORT_LOAD_SPI    1

#define HAL_SUPPORT_EYE_MONITOR    1
#define HAL_SUPPORT_FW_ADAPT_COUNT 1
#define HAL_SUPPORT_FW_DFE         1
#define HAL_SUPPORT_FW_EYE         1
#define HAL_SUPPORT_FW_ISI         1
#define HAL_SUPPORT_FW_FFE         1
#define HAL_SUPPORT_FW_FFE_NBIAS   1
#define HAL_SUPPORT_FW_FFE_KACCU   1
#define HAL_SUPPORT_FW_FFE_JUMP    1
#define HAL_SUPPORT_FW_RATIO       1
#define HAL_SUPPORT_FW_OF          1
#define HAL_SUPPORT_FW_HF          1

#define HAL_SUPPORT_PARAMS_SLICE  1
#define HAL_SUPPORT_PARAMS_PORT   1
#define HAL_SUPPORT_PARAMS_SERDES 1
#define HAL_SUPPORT_PARAMS_LANE   1

#define HAL_SUPPORT_OPTIONS_PORT 1

#define FW_SPEED_INFO       TOP_DEBUG, TOP_DEBUG_SPEED
#define FW_ADAPT_COUNT_NRZ  TOP_DEBUG, TOP_DEBUG_RESTART_COUNT
#define FW_ADAPT_COUNT_PAM4 TOP_DEBUG, TOP_DEBUG_RESTART_COUNT
#define FW_READAPT_COUNT    TOP_INFO, TOP_INFO_READAPT_COUNT
#define FW_LINK_LOST_COUNT  TOP_INFO, TOP_INFO_LINK_LOST_COUNT
#define FW_LOS_COUNT        TOP_INFO, TOP_INFO_LOS_COUNT
#define FW_RX_RATIO_NRZ     NRZ_DEBUG, NRZ_DEBUG_RX_RATIO
#define FW_RX_RATIO_PAM4    PAM4_DEBUG, PAM4_DEBUG_RX_RATIO
#define FW_RX_OF_NRZ        NRZ_DEBUG, NRZ_DEBUG_RX_OF
#define FW_RX_OF_PAM4       PAM4_DEBUG, PAM4_DEBUG_RX_OF
#define FW_RX_HF_NRZ        NRZ_DEBUG, NRZ_DEBUG_RX_HF
#define FW_RX_HF_PAM4       PAM4_DEBUG, PAM4_DEBUG_RX_HF
#define FW_DFE_PAM4         PAM4_INFO, PAM4_INFO_THRESHOLD
#define FW_EYE_PAM4         PAM4_INFO, PAM4_INFO_EYE
#define FW_ISI_NRZ          PAM4_INFO, PAM4_INFO_ISI
#define FW_ISI_PAM4         PAM4_INFO, PAM4_INFO_ISI
#define FW_FFE_NBIAS        PAM4_INFO, PAM4_INFO_FFE_NBIAS
#define FW_FFE_KACCU        PAM4_INFO, PAM4_INFO_FFE_KACCU
#define FW_FFE_JUMP         PAM4_INFO, PAM4_INFO_FFE_JUMP

#define SP_SD_DELAY_OPTI_NRZ                                                                                     \
    "sd_delay_optical_nrz",                                                                                      \
        "signal delay timeout for nrz optical (unit: milliseconds). This option is persistent across port/lane " \
        "reconfigure.",                                                                                          \
        "slice-option"
#define SP_SD_DELAY_NRZ                                                                                             \
    "sd_delay_nrz",                                                                                                 \
        "signal delay timeout for nrz electrical (unit: milliseconds). This option is persistent across port/lane " \
        "recofigure.",                                                                                              \
        "slice-option"
#define SP_SD_DELAY_OPTI_PAM4                                                                                     \
    "sd_delay_optical_pam4",                                                                                      \
        "signal delay timeout for pam4 optical (unit: milliseconds). This option is persistent across port/lane " \
        "reconfigure.",                                                                                           \
        "slice-option"
#define SP_SD_DELAY_PAM4                                                                                             \
    "sd_delay_pam4",                                                                                                 \
        "signal delay timeout for pam4 electrical (unit: milliseconds). This option is persistent across port/lane " \
        "reconfigure.",                                                                                              \
        "slice-option"
#define SP_CLK_OUTPUT_CTRL                                                                                           \
    "clock_output_squelch",                                                                                          \
        "default is 0 (off), only works in port based mode. 0b1 is just clock 0, 0b11 is clock 0,1, 0b111 is clock " \
        "0,1,2",                                                                                                     \
        "slice-option"
#define SP_FW_FIFO_HEALING    "fw_fifo_healing", "Enable/Disable FIFO healing under background mode", "slice-option"
#define SP_FW_ANLT_PWR_SAVING "fw_anlt_power_saving", "Enable/Disable ANLT power saving", "slice-option"
#define SP_SPARE              "spare", "scratch register 0-3", "slice-option"
#define SP_FW_FEC_ADV_RD      "fw_rsfec_clkgate", "Enable/Disable advanced fec corrected bits reading", CR_PARAM_TYPE_OPTION

/* Other registers of Blackhawk specific */
#define HOST_LANES 8
#define LINE_LANES 8
#define CHIP_LANES (HOST_LANES + LINE_LANES)
#define TOP        HIVE(Top)
#define TOPPLL     HIVE(TopPLL)
#define TOPLANE    HIVE(TopLane)
#define FECA       HIVE(FecA)
#define FECB       HIVE(FecB)
#define TOPSENSOR  HIVE(TopSensor)

#define REG_DATA              0x9F00
#define REG_CMD_RAW_FB4       0x9801  // support if fw feature bit 4
#define REG_LANE_REGISTER_RST 0x980E
#define REG_LANE_LOGIC_RST    0x980F
#define REG_MDIO_MAGIC_NUMER  0x985A

// reset value
#define CHIP_SOFT_RST_VAL  0x888
#define CHIP_LOGIC_RST_VAL 0x777
#define CHIP_CPU_RST_VAL   0xAAA
#define CHIP_REG_RST_VAL   0x999

// temperature
#define T_CLK       401.5
#define T_DIV_RATIO 53.0

#define FW_CMD_TIMEOUT      10000
#define FW_LOAD_SPI_TIMEOUT (1000 * 1000)  // 1000 ms

// firmware define
#define REG_MAGIC             REGBITR(TOP, 0x05, 15, 0)
#define REG_CMD               REGBITR(TOP, 0x06, 15, 0)
#define REG_CMD_DETAIL        REGBITR(TOP, 0x07, 15, 0)
#define REG_CMD_DETAIL2       REGBITR(TOP, 0xCF, 15, 0)
#define REG_FW_WATCHDOG_TIMER REGBITR(TOP, 0xC6, 15, 0)
#define REG_FW_REG_VALUE      REGBITR(TOP, 0xC7, 15, 0)
#define REG_FW_PHY_READY      REGBITR(TOP, 0xC9, 15, 0)
#define REG_FW_OPTICAL        REGBITR(TOP, 0xCC, 15, 0)
#define REG_FW_TOPPLL_CALDONE REGBITR(TOP, 0xD0, 0)
#define REG_FW_ACFG_DONE      REGBITR(TOP, 0xD0, 1)
#define REG_FW_ACFG_OFF       REGBITR(TOP, 0xD0, 2)

#define REG_CHIP_RST    REGBITR(TOP, 0x02, 15, 0)
#define REG_MCU_CLK_SEL REGBITR(TOP, 0x0B, 15)

#define REG_OUI_LSB         REGBITR(TOP, 0x08, 15, 0)
#define REG_OUI_MSB         REGBITR(TOP, 0x09, 15, 8)
#define REG_MODEL_NUMBER    REGBITR(TOP, 0x0A, 9, 4)
#define REG_REVISION_NUMBER REGBITR(TOP, 0x0A, 3, 0)

// CR_PORT_RETIMER fifo
#define REG_FOUR_LANE_CROSS REGBITR(TOP, 0x55, 15, 0)
#define REG_ADR_DIFF_A0(LN) REGBITR(TOP, 0x13 + ((LN % 8) * 8), 15, 13)
#define REG_ADR_DIFF_A1(LN) REGBITR(TOP, 0x13 + ((LN % 8) * 8), 12, 10)
#define REG_ADR_DIFF_B0(LN) REGBITR(TOP, 0x13 + ((LN % 8) * 8), 9, 7)
#define REG_ADR_DIFF_B1(LN) REGBITR(TOP, 0x13 + ((LN % 8) * 8), 6, 4)

// Bitmux status
#define REG_BM_CTRL_S        REGBITR(TOP, 0x59, 15, 0)
#define REG_BM_BUF_SEL_LANE  REGBITR(TOP, 0x4D, 14, 12)
#define REG_BM_BUF_SEL_SLICE REGBITR(TOP, 0x4D, 10, 8)
#define REG_BM_BUF_SEL_READ  REGBITR(TOP, 0x4E, 15, 4)

// SW loopback
#define PLL_LOOPBACK_MAGIC 8
#define REG_LOOPBACK_EN    0x9811
#define REG_W_ADJUST       0x9812
#define REG_ADR_DIFF_LB    0x9814

#define ADR_DIFF_A_SHIFT 10
#define ADR_DIFF_B_SHIFT 4

#define A0_DEC_MASK 0x8000
#define B0_DEC_MASK 0x2000
#define A0_INC_MASK 0x0800
#define B0_INC_MASK 0x0200

/* Top PLL */
#define REG_TOP_PLL_EN_REFCLK         REGBITR(TOPPLL, 0x000, 7)
#define REG_TOP_PLL_DIV4              REGBITR(TOPPLL, 0x000, 6)
#define REG_TOP_PLL_PU                REGBITR(TOPPLL, 0x001, 2)
#define REG_TOP_PLL_LCVCOCAP          REGBITR(TOPPLL, 0x001, 12, 6)
#define REG_TOP_PLL_N                 REGBITR(TOPPLL, 0x007, 12, 4)
#define REG_TOP_PLL_FCAL_START        REGBITR(TOPPLL, 0x00D, 15)
#define REG_TOP_PLL_FCAL_TIMER_WINDOW REGBITR(TOPPLL, 0x00D, 14, 0)
#define REG_TOP_PLL_FCAL_DONE         REGBITR(TOPPLL, 0x000F, 15)
#define REG_TOP_PLL_FCAL_CNT_OP       REGBITR(TOPPLL, 0x00E, 15, 0)
#define REG_TOP_PLL_LO_OPEN           REGBITR(TOPPLL, 0x010, 7)
#define REG_TOP_PLL_VCTRL_LOOPEN_SEL  REGBITR(TOPPLL, 0x010, 6, 4)
#define REG_TOP_PLL_PD_FCAL           REGBITR(TOPPLL, 0x012, 3)
#define REG_TOP_PLL_REFCLK_DIV        REGBITR(TOPPLL, 0x013, 15, 7)
#define REG_TOP_PLL_DIV2              REGBITR(TOPPLL, 0x013, 6)

// RS FEC
#define REG_FEC_EN REGBITR(TOP, 0x057, 7, 0)

// Clock Output
#define CKO_STATE_UNCONFIG     0
#define CKO_STATE_SQUELCHED    1
#define CKO_STATE_ACTIVE       2
#define CKO_STATE_INVALID      3
#define REG_TOP_CDR_MUX(N)     REGBITR(TOP, 0xD5, (3 + N * 4), (0 + N * 4))
#define REG_TOP_CDR_DIV_MUX0   REGBITR(TOP, 0xD6, 2, 0)
#define REG_TOP_CDR_DIV_MUX1   REGBITR(TOP, 0xD6, 5, 3)
#define REG_TOP_CDR_DIV_MUX2   REGBITR(TOP, 0xD6, 8, 6)
#define REG_TOP_CDR_CLK_EN0    REGBITR(TOP, 0xD6, 12)
#define REG_TOP_CDR_CLK_EN1    REGBITR(TOP, 0xD6, 13)
#define REG_TOP_CDR_CLK_EN2    REGBITR(TOP, 0xD6, 14)
#define REG_TOP_PU_BG          REGBITR(TOP, 0xD7, 0)
#define REG_TOP_PU_RVDD        REGBITR(TOP, 0xD7, 1)
#define REG_TOP_BYPASS_SG2REG  REGBITR(TOP, 0xD7, 8)
#define REG_TOP_BYPASS_SG1REG  REGBITR(TOP, 0xD7, 9)
#define REG_TOP_VREF_CLK_SG2   REGBITR(TOP, 0xD7, 12, 10)
#define REG_TOP_VREF_CLK_SG1   REGBITR(TOP, 0xD7, 15, 13)
#define REG_TOP_BYPASS_DIFFREG REGBITR(TOP, 0xD8, 2)
#define REG_TOP_VRVDD          REGBITR(TOP, 0xD8, 6, 4)
#define REG_TOP_EN_CKO_SG2     REGBITR(TOP, 0xD8, 3)
#define REG_TOP_EN_CKO_SG1     REGBITR(TOP, 0xD8, 7)
#define REG_TOP_VREF_CLK_DIFF  REGBITR(TOP, 0xD8, 10, 8)
#define REG_TOP_EN_CKO_DIFF    REGBITR(TOP, 0xD8, 11)
#define REG_TOP_EN_CKO         REGBITR(TOP, 0xD8, 13)
#define REG_TOP_BYPASS_RVDDREG REGBITR(TOP, 0xD8, 14)
#define REG_TOP_EN_RVDDVCO     REGBITR(TOP, 0xD8, 15)
#define REG_TOP_VBG_C          REGBITR(TOP, 0xD9, 3, 0)

/* LANE_SLICE_REG */
#define REG_FEC_ANA_EN_A REGBITR(TOPLANE, 0x28, 0)
#define REG_FEC_ANA_EN_B REGBITR(TOPLANE, 0x28, 1)

/* SENSOR REG */
#define REG_TOP_SENSOR_AUTO_VS    REGBITR(TOPSENSOR, 0x3A, 4)
#define REG_TOP_SENSOR_RSTB_VS    REGBITR(TOPSENSOR, 0x3A, 1)
#define REG_TOP_SENSOR_CLK_SEL_VS REGBITR(TOPSENSOR, 0x3F, 15)
#define REG_TOP_SENSOR_CLK_CNT_VS REGBITR(TOPSENSOR, 0x3F, 14, 0)
#define REG_TOP_SENSOR_VS_CFG     REGBITR(TOPSENSOR, 0x4A, 15, 0)
#define REG_TOP_SENSOR_VS_SDE     REGBITR(TOPSENSOR, 0xF6, 7)
#define REG_TOP_SENSOR_VS_RSTN    REGBITR(TOPSENSOR, 0xF6, 10)
#define REG_TOP_SENSOR_VS_RUN     REGBITR(TOPSENSOR, 0xF6, 11)
#define REG_TOP_SENSOR_VS_RDY     REGBITR(TOPSENSOR, 0xF5, 14)
#define REG_TOP_SENSOR_VS_DOUT    REGBITR(TOPSENSOR, 0xF5, 13, 0)

/* ECC */
#define REG_ECC_TEST   REGBITR(TOPPLL, 0x200, 14, 13)
#define REG_ECC_STATUS REGBITR(TOPPLL, 0x201, 15, 0)

// EFUSE
#define EFUSE_BASE_REG 0xB000

#ifndef __EMSCRIPTEN__
void hal_register_sdk(void) __attribute__((constructor));
#else
void hal_register_sdk(void) __attribute__((visibility("default")));
#endif

#endif
