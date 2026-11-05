/********************************************************************************
 * Copyright(C) 2026 Micas Network. All rights reserved.
 ********************************************************************************
 * aperta_pm_port.c - APERTA (81394) PM port deep configuration
 *
 * Self-contained, register-level port of the Broadcom APERTA PM port-init
 * path plus the deep TSCBH lane-rate / PLL / AN-timer programming that was
 * previously missing.
 *
 * Implemented pieces :
 *   1. Firmware mailbox message protocol            
 *   2. CONFIG_PORT / ENABLE_PORT / DISABLE_PORT FW msgs
 *   3. PM port active + CDMAC TX/RX enable           
 *   4. VCO->PLL mapping + AMS PLL reconfig           
 *   5. TSCBH lane-rate: lane soft reset, PLL select, OSR mode
 *   6. Speed-ID table upload through the LMI mem-access protocol
 *   7. AN-timer programming                          
 * TSC register base is PHYMOD_REG_APERTA_TSCBH (0x18000000).
 ********************************************************************************/
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "aperta_phy_sdk.h"
#include "aperta_tbhmod_defs.h"

/* ==========================================================================
 * Externs
 * ========================================================================== */
extern int plp_aperta_reg32_read(const plp_aperta_phymod_phy_access_t *phy, uint32_t reg_addr, uint32_t *data);
extern int plp_aperta_reg32_write(const plp_aperta_phymod_phy_access_t *phy, uint32_t reg_addr, uint32_t data);
extern int plp_aperta_direct_reg_read(const plp_aperta_phymod_phy_access_t *phy, uint32_t reg_addr, uint32_t *data);
extern int plp_aperta_direct_reg_write(const plp_aperta_phymod_phy_access_t *phy, uint32_t reg_addr, uint32_t data);
extern aperta_pm_info_t _plp_aperta_pm_info[APERTA_MAX_PM_INFO];

/* ==========================================================================
 * Base addresses
 * ========================================================================== */
#define PHYMOD_REG_APERTA_TSCBH         0x18000000u

/* DIRECT register space base. */
#define APERTA_DIRECT_BASE              0x1000000u

/* PM MAC (CDMAC) registers - line-side 0x14xxxxxx, system-side 0x15xxxxxx */
#define APERTA_CDMAC_CTRLr              0x1400010b
#define APERTA_CDMAC_TX_CTRLr           0x1500010d
#define APERTA_CDMAC_CTRL_TX_EN         0x1
#define APERTA_CDMAC_CTRL_RX_EN         0x2
#define APERTA_CDMAC_CTRL_SOFT_RESET    0x40
#define APERTA_CDMAC_TX_CTRL_DISCARD    0x4

#define APERTA_CDPORT_MODEr              0x10020009
#define APERTA_CDMAC_4_LANES_SEPARATE    0
#define APERTA_CDMAC_3_TRI_0_1_2_2       1
#define APERTA_CDMAC_3_TRI_0_0_2_3       2
#define APERTA_CDMAC_2_LANES_DUAL        3
#define APERTA_CDMAC_4_LANES_TOGETHER    4

/* Direct register space */
#define APERTA_FW_MSG_IN_BUFFER_ADDR    (0x1a000 | APERTA_DIRECT_BASE)
#define APERTA_FW_MSG_OUT_BUFFER_ADDR   (0x1a080 | APERTA_DIRECT_BASE)
#define APERTA_LMI_CMDr                 (0x19001 | APERTA_DIRECT_BASE)
#define APERTA_LMI_CMD_SEQr             (0x19006 | APERTA_DIRECT_BASE)
#define APERTA_LMI_STATUSr              (0x1900a | APERTA_DIRECT_BASE)
/* CDMAC 16B space (bit28 0x10000000) - separate from DIRECT space, keep as is */
#define APERTA_MEM_ACCESS_ENr           0x10020015
#define APERTA_SYSTEM_SIDE_NO_OF_LANES  BCMI_APERTA_D_CTRL_SWGPREG04r

#define APERTA_PORTn_CONFIGr(n)         (BCMI_APERTA_D_CTRL_PORT0_CONFIGr + (n))

/* phy_init registers - BCMI control/SWS/GPIO
 * registers all live in the DIRECT (0x1000000) base space. ai_app's
 * plp_aperta_direct_reg_* is a raw passthrough, so pass the full bus address. */
#define APERTA_CTRL_SWGPREG1Fr          (0x18b4f | APERTA_DIRECT_BASE)
#define APERTA_SWS_SWREG_002r           (0x1a802 | APERTA_DIRECT_BASE)
#define APERTA_GPIO_INPUT_SEL_1r        (0x18ad9 | APERTA_DIRECT_BASE)

/* ==========================================================================
 * FW message constants
 * ========================================================================== */
#define APERTA_FW_MSG_IN_BUFFER         0x000
#define APERTA_STS_SENT                 0x1
#define APERTA_STS_PROCD                0x3
#define APERTA_OP_WRITE                 0x0
#define APERTA_OP_READ                  0x1
#define APERTA_OP_READ_EXT              0x3
#define APERTA_OP_START                 0x0
#define APERTA_OP_START_RESULT          0x1
#define APERTA_OP_PROCESSING            0xD
#define APERTA_OP_SUCCESS               0xE
#define APERTA_OP_ERROR                 0xF

#define APERTA_FUNC_PM_REGS             0x10
#define APERTA_FUNC_TSC_REGS            0x11
#define APERTA_FUNC_MACSEC_REGS         0x12
#define APERTA_FUNC_CHIP_IND_REGS       0x13
#define APERTA_FUNC_CONFIG_PHY          0x17
#define APERTA_FUNC_CONFIG_PORT         0x18
#define APERTA_FUNC_PAUSE_PORT          0x19
#define APERTA_FUNC_RESUME_PORT         0x1A
#define APERTA_FUNC_ENABLE_PORT         0x20
#define APERTA_FUNC_DISABLE_PORT        0x21
#define APERTA_FUNC_FLUSH_PORT          0x22

#define APERTA_FW_PORT_OP_ENABLE        1
#define APERTA_FW_PORT_OP_DISABLE       2
#define APERTA_FW_PORT_OP_FLUSH         3
#define APERTA_FW_PORT_OP_PAUSE         4
#define APERTA_FW_PORT_OP_RESUME        5

#define APERTA_FW_MSG_RETRY_CNT         100
#define APERTA_FW_MSG_TIMEOUT           500

#define APERTA_FW_FUN_SUPPORT_START_RESULT(fn) \
    (((fn) == APERTA_FUNC_ENABLE_PORT)  || \
     ((fn) == APERTA_FUNC_DISABLE_PORT) || \
     ((fn) == APERTA_FUNC_FLUSH_PORT))

/* Firmware Port Speed enum */
#define APERTA_FW_SP_10G                0x00
#define APERTA_FW_SP_25G                0x01
#define APERTA_FW_SP_40G                0x02
#define APERTA_FW_SP_50G_NRZ            0x03
#define APERTA_FW_SP_50G_PAM4           0x04
#define APERTA_FW_SP_100G_NRZ           0x05
#define APERTA_FW_SP_100G_PAM4          0x06
#define APERTA_FW_SP_200G_PAM4          0x07
#define APERTA_FW_SP_400G_PAM4          0x08

/* Port config register bit shifts */
#define APERTA_FAULT_SHIFT              12
#define APERTA_FLOW_CTRL_SHIFT          13
#define APERTA_S_F_SHIFT                14
#define APERTA_PRT_ACTIVE_SHIFT         15

#define APERTA_PORT_TYPE_REPEATER       0
#define APERTA_PORT_TYPE_GEARBOX        1
#define APERTA_PORT_TYPE_R_GEARBOX      2
#define APERTA_MAX_PORT                 8
#define APERTA_REV_B0                   0xB0

/* CONFIG_PORT FW message payload */
typedef struct aperta_config_port_s {
    uint8_t  PortNum;      /*Hardware Port Number*/
    uint8_t  PortType;     /*PortType (Repeater/Gearbox/Rev-Gearbox)*/
    uint8_t  PortMode;     /*PortMode (Regular/FailoverMUX)*/
    uint8_t  PortSpeed;    /*PortSpeed*/
    uint8_t  SPMPortID;    /*System-side Port Macro's Port ID*/
    uint8_t  LPMPortID;    /*Line-side Port Macro's Port ID*/
    uint8_t  PortOptions;  /*Port configuration options*/
    uint16_t IngFixedLatency; /*ingress Fixed-Latency Value*/
    uint16_t EgrFixedLatency; /*egress Fixed-Latency Value*/
    uint8_t  FOOptions;    /*Fail-over MUX Options*/
    uint8_t  FOPortNum;    /*Fail-over Hardware Port Number*/
    uint8_t  FOPortID;     /*Fail-over Port Macro Port ID*/
    uint16_t EgrptpFixedLatency;
} aperta_config_port_t;

/* LMI memory types */
#define APERTA_MEM_TYPE_SPEED_ID        0
#define APERTA_MEM_TYPE_AM_TABLE        1
#define APERTA_MEM_TYPE_UM_TABLE        2
#define APERTA_MEM_TYPE_SPEED_PRIORITY  3
#define APERTA_MEM_TYPE_PM_MIB          4

#define APERTA_TSCBH_FORCED_SPEED_ID_OFFSET 56

#define APERTA_LINE_SIDE_PM             1
#define APERTA_SYS_SIDE_PM              9

/* ==========================================================================
 * TSC (Blackhawk) register offsets (0x18000000 | offset)
 * ========================================================================== */
#define BH_SC_X4_CTL                    0xc050  /* [5:0] SW_SPEED_ID [8] SW_SPEED_CHANGE */
#define BH_OSR_MODE                     0xd0b0  /* [3:0] osr mode, [15] force    */
#define BH_LANE_SOFT_RESET              0xd0b1  /* [0] ln_dp_s_rstb               */
#define BH_PLL_SELECT                   0xd0b7  /* [0] pll select                 */
#define BH_AMS_PLL_COM_CTL_110         0xd110  /* [8] fp3_rh [4] fp3_ctrl       */
#define BH_AMS_PLL_VCO2_15G             0xd111  /* [7]                           */
#define BH_AMS_PLL_IQP                  0xd112  /* [3:0] ams_pll_iqp             */
#define BH_AMS_PLL_CTRL_114             0xd114  /* [9:8] pll2rx_clkbw [13:12] kvh [14] force_kvh_bw */
#define BH_AMS_PLL_REFCLK_CTRL          0xd116  /* [15] refclk_doubler [13:12] doubler_res [11:8] doubler_cap [2] div4 [1] div2 */
#define BH_VCO_STEP_TIME                0xd140  /* [7:0] vco_step_time          */
#define BH_REFCLK_DIVCNT                0xd145  /* [13:0] refclk_divcnt         */
#define BH_AMS_PLL_NDIV_FRAC_VALID      0xd117  /* [4] ndiv_frac_valid            */
#define BH_AMS_PLL_FRACN_DIV            0xd118  /* [15:0]                         */
#define BH_AMS_PLL_FRACN_CTRL           0xd119  /* [1:0] div_17_16 [2] divrange [3] bypass [13:4] ndiv_int [15] sel */
#define BH_AMS_PLL_PWRDN                0xd11b  /* [2] ams_pll_pwrdn              */
#define BH_AMS_PLL_MODE                 0xd147  /* [4:0] pll_mode                 */
#define BH_PLL_LOCK                     0xd148  /* [8] pll_lock                   */
#define BH_TOP_USER_CONTROL             0xd184  /* [13] core_dp_s_rstb [14] afe_s_pll_pwrdn */
#define BH_UC_CORE_CONFIG               0xd18d  /* [8] core_cfg_from_pcs [7:0] vco_rate */

/* AN timers */
#define BH_MAIN0_TICK_CTL1r             0x09003 /* [15] override, [14:0] numerator upper */
#define BH_MAIN0_TICK_CTL0r             0x09004 /* [15:12] num lower, [11:2] denominator */
#define BH_AN_CL73_BRK_LNKr             0x09250
#define BH_AN_CL73_ERRr                 0x09251
#define BH_AN_IGNORE_LNK_TMRr           0x09254
#define BH_AN_LNK_FAIL_INHBT_CL72r      0x09255
#define BH_AN_LNK_FAIL_INHBT_NOT_CL72r  0x09256
#define BH_AN_IGNORE_LNK_TMR_PAM4r      0x09259
#define BH_AN_LNK_FAIL_INHBT_CL72_PAM4r 0x0925a
#define BH_AN_LNK_FAIL_INHBT_NOT_CL72_PAM4r 0x0925b
#define BH_RX_X4_RS_FEC_TMRr            0x0c130

/* PLL divider lookup - integer-mode pll_mode values */
static const uint16_t _pll_mode_div_lkup[] = {
    64, 66, 80, 128, 132, 140, 160, 165, 168, 170,
    175, 180, 184, 200, 224, 264, 96, 120, 144, 198
};

#define APERTA_IS_LINE_SIDE(phy)  ((phy)->port_loc == phymodPortLocLine)
#define APERTA_IS_SYSTEM_SIDE(phy) ((phy)->port_loc == phymodPortLocSys)

/* ==========================================================================
 * Message byte-buffer helpers (LITTLE-ENDIAN,  put_half_word/get_half_word 
 * read/write the LSB first). The FW mailbox
 * protocol is little-endian; big-endian here scrambled every message's
 * side/cnt/addr fields.
 * ========================================================================== */
static void _pm_msg_put_half_word(uint8_t **buf, uint16_t val)
{
    *(*buf)++ = (uint8_t)(val & 0xFF);
    *(*buf)++ = (uint8_t)(val >> 8);
}

static void _pm_msg_put_byte(uint8_t **buf, uint8_t val)
{
    *(*buf)++ = val;
}

static uint16_t _pm_msg_get_half_word(uint8_t **buf)
{
    uint16_t v = (uint16_t)(*buf)[0] | ((uint16_t)(*buf)[1] << 8);
    *buf += 2;
    return v;
}

/* ==========================================================================
 * _pm_msg_send
 * ========================================================================== */
int _pm_msg_send(const plp_aperta_phymod_phy_access_t *phy, uint8_t function, uint8_t operation,
                 uint8_t *tx_msg, uint8_t *rx_msg, uint8_t *result)
{
    uint32_t msgout = 0, msgin = 0, length = 0, value = 0;
    uint32_t addr = APERTA_FW_MSG_IN_BUFFER_ADDR, retry_cnt = APERTA_FW_MSG_TIMEOUT;
    uint8_t *msg = tx_msg;

    length = _pm_msg_get_half_word(&msg);
    (void)plp_aperta_direct_reg_read(phy, BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr, &msgout);
    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(phy, addr, length));
    addr++;
    length = (length + 1) / 2;
    while (length) {
        value = _pm_msg_get_half_word(&msg);
        PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(phy, addr, value));
        addr++;
        length--;
    }

    /* Trigger message processing */
    if (APERTA_FW_FUN_SUPPORT_START_RESULT(function) && (operation != APERTA_OP_START_RESULT)) {
        msgin = (function << 8) | (operation << 4) | APERTA_STS_SENT;
        PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(phy, BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr, msgin));
    }
    if (!APERTA_FW_FUN_SUPPORT_START_RESULT(function)) {
        msgin = (function << 8) | (operation << 4) | APERTA_STS_SENT;
        PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(phy, BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr, msgin));
    }
    /* Wait for STS_PROCD */
    do {
        if (APERTA_FW_FUN_SUPPORT_START_RESULT(function) && (operation == APERTA_OP_START_RESULT)) {
            msgin = (function << 8) | (operation << 4) | APERTA_STS_SENT;
            PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(phy, BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr, msgin));
        }
        PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr, &msgout));
        PHYMOD_USLEEP(100);
    } while (((msgout & 0xFF0F) != ((function << 8) | APERTA_STS_PROCD)) && (--retry_cnt));

    if (retry_cnt == 0) {
        LOG_INFO("FW msg_send timeout fn=0x%x op=0x%x addr=%d msgout=0x%x msgin=0x%x\n",
                 function, operation, phy->access.addr, msgout, msgin);
        return PHYMOD_E_TIMEOUT;
    }

    *result = (uint8_t)((msgout >> 4) & 0xF);
    if (*result != APERTA_OP_SUCCESS) {
        /* APERTA_OP_PROCESSING (0xD) is the NORMAL ack for START ops
         * (ENABLE/DISABLE/FLUSH): the caller then polls START_RESULT until
         * SUCCESS. Do not print it as a failure. */
        if (!((*result == APERTA_OP_PROCESSING) && APERTA_FW_FUN_SUPPORT_START_RESULT(function))) {
            LOG_INFO("FW msg_send fail fn=0x%x op=0x%x result=0x%x msgout=0x%x", function, operation, *result, msgout);
        }
    }

    /* Skip response read for write ops with success result */
    if ((APERTA_OP_WRITE == operation) && (APERTA_OP_SUCCESS == *result)) {
        _pm_msg_put_half_word(&rx_msg, 0x0000);
        return PHYMOD_E_NONE;
    }
    if (APERTA_OP_ERROR == *result) {
        /* Read the output buffer into rx_msg (also clears the error state) */
        addr = APERTA_FW_MSG_OUT_BUFFER_ADDR;
        PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, addr, &length));
        _pm_msg_put_half_word(&rx_msg, (uint16_t)length);
        addr++;
        length = (length + 1) / 2;
        while (length) {
            PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, addr, &value));
            _pm_msg_put_half_word(&rx_msg, (uint16_t)value);
            addr++;
            length--;
        }
    } else if (!APERTA_FW_FUN_SUPPORT_START_RESULT(function) && (operation == APERTA_OP_READ || operation == APERTA_OP_READ_EXT)) {
        addr = APERTA_FW_MSG_OUT_BUFFER_ADDR;
        PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, addr, &length));
        _pm_msg_put_half_word(&rx_msg, (uint16_t)length);
        addr++;
        length = (length + 1) / 2;
        while (length) {
            PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, addr, &value));
            _pm_msg_put_half_word(&rx_msg, (uint16_t)value);
            addr++;
            length--;
        }
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_get_chip_rev - read APERTA chip revision (B0 handling)
 * ========================================================================== */
static int _pm_get_chip_rev(const plp_aperta_phymod_phy_access_t *phy, uint32_t *rev)
{
    uint32_t rd = 0;
    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, BCMI_APERTA_D_CTRL_CHIP_REVISIONr, &rd));
    *rev = rd & 0xFF;
    return PHYMOD_E_NONE;
}

static int _pm_count_no_bits(uint32_t val)
{
    int n = 0;
    while (val) {
        val &= (val - 1);
        n++;
    }
    return n;
}

/* ==========================================================================
 * _pm_fill_port_cfg - build CONFIG_PORT FW message payload
 * ========================================================================== */
static int _pm_fill_port_cfg(const plp_aperta_phymod_phy_access_t *phy, const plp_aperta_phymod_phy_inf_config_t *config,
                             aperta_device_aux_modes_t *auxmode, aperta_config_port_t *fw_port_config)
{
    uint32_t data = 0, sys_no_lanes = 0, port = 0, lane = 0, rev_id = 0;
    uint32_t line_no_lanes;

    _pm_get_port_from_lm_sp(config->data_rate, auxmode->lane_data_rate, phy->access.lane_mask, &port, &lane);
    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, APERTA_SYSTEM_SIDE_NO_OF_LANES, &sys_no_lanes));

    line_no_lanes = (uint32_t)_pm_count_no_bits(phy->access.lane_mask);
    fw_port_config->SPMPortID = (uint8_t)((sys_no_lanes & 0xF0) >> 4);
    fw_port_config->LPMPortID = (uint8_t)port;
    fw_port_config->PortNum   = (uint8_t)port;
    fw_port_config->PortType  = ((sys_no_lanes & 0xF) == line_no_lanes) ? APERTA_PORT_TYPE_REPEATER :
                                ((sys_no_lanes & 0xF) > line_no_lanes)  ? APERTA_PORT_TYPE_GEARBOX : APERTA_PORT_TYPE_R_GEARBOX;
    fw_port_config->PortMode  = 0;

    if (config->data_rate == APERTA_SPEED_10G) {
        fw_port_config->PortSpeed = APERTA_FW_SP_10G;
    } else if (config->data_rate == APERTA_SPEED_25G) {
        fw_port_config->PortSpeed = APERTA_FW_SP_25G;
    } else if (config->data_rate == APERTA_SPEED_40G) {
        fw_port_config->PortSpeed = APERTA_FW_SP_40G;
    } else if (config->data_rate == APERTA_SPEED_50G &&
               auxmode->modulation_mode == bcmplpApertaModulationNRZ) {
        fw_port_config->PortSpeed = APERTA_FW_SP_50G_NRZ;
    } else if (config->data_rate == APERTA_SPEED_50G &&
               auxmode->modulation_mode == bcmplpApertaModulationPAM4) {
        fw_port_config->PortSpeed = APERTA_FW_SP_50G_PAM4;
    } else if (config->data_rate == APERTA_SPEED_100G &&
               auxmode->modulation_mode == bcmplpApertaModulationNRZ) {
        fw_port_config->PortSpeed = APERTA_FW_SP_100G_NRZ;
    } else if (config->data_rate == APERTA_SPEED_100G &&
               auxmode->modulation_mode == bcmplpApertaModulationPAM4) {
        fw_port_config->PortSpeed = APERTA_FW_SP_100G_PAM4;
    } else if (config->data_rate == APERTA_SPEED_200G &&
               auxmode->modulation_mode == bcmplpApertaModulationPAM4) {
        fw_port_config->PortSpeed = APERTA_FW_SP_200G_PAM4;
    } else if (config->data_rate == APERTA_SPEED_400G &&
               auxmode->modulation_mode == bcmplpApertaModulationPAM4) {
        fw_port_config->PortSpeed = APERTA_FW_SP_400G_PAM4;
    } else {
        LOG_INFO("Incorrect data rate %d\n", config->data_rate);
        return PHYMOD_E_PARAM;
    }

    /* Port options from PORTn_CONFIG (S/F, FlowCtrl, Fault) + ts_config */
    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, APERTA_PORTn_CONFIGr(port), &data));
    if (data & (1 << APERTA_S_F_SHIFT)) {
        fw_port_config->PortOptions |= 1;
    }
    if (data & (1 << APERTA_FLOW_CTRL_SHIFT)) {
        fw_port_config->PortOptions |= 2;
    }
    if (data & (1 << APERTA_FAULT_SHIFT)) {
        fw_port_config->PortOptions |= 4;
    }
    PHYMOD_IF_ERR_RETURN(_pm_get_chip_rev(phy, &rev_id));
    if (rev_id == APERTA_REV_B0) {
        fw_port_config->PortOptions |= (((auxmode->ts_config >> 3) & 1) << 3);
        fw_port_config->PortOptions |= (auxmode->ts_config & 7) << 4;
        fw_port_config->EgrptpFixedLatency = (uint16_t)auxmode->egr_ptp_fixed_latency;
    } else {
        fw_port_config->PortOptions |= (auxmode->ts_config) << 4;
    }
    if (auxmode->fixed_latency_config.enable) {
        fw_port_config->PortOptions |= 0x80;
        fw_port_config->IngFixedLatency = (uint16_t)auxmode->fixed_latency_config.igr_dp_ck_cycles;
        fw_port_config->EgrFixedLatency = (uint16_t)auxmode->fixed_latency_config.egr_dp_ck_cycles;
    }
    if ((fw_port_config->PortOptions & 0x8) && !(fw_port_config->PortOptions & 0x80)) {
        LOG_INFO("Port cannot be configured without enabling fixed latency\n");
        return PHYMOD_E_NONE;
    }

    /* Line side failover (kept minimal; ai_app uses no failover) */
    if (auxmode->failover_config.lane_map != 0) {
        uint32_t fo_port = 0, fo_lane = 0;
        fw_port_config->FOOptions = 0x1;
        if (auxmode->failover_config.mux_location & 1) {
            fw_port_config->FOOptions |= 0x4;
        }
        fw_port_config->PortMode = 1;
        _pm_get_port_from_lm_sp(config->data_rate, auxmode->lane_data_rate,
                                auxmode->failover_config.lane_map, &fo_port, &fo_lane);
        fw_port_config->FOPortNum = (uint8_t)fo_port;
        fw_port_config->FOPortID  = (uint8_t)fo_port;
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_fw_config_port_set - CONFIG_PORT.WRITE
 * ========================================================================== */
static int _pm_fw_config_port_set(const plp_aperta_phymod_phy_access_t *phy, aperta_config_port_t *port_cfg)
{
    uint8_t tx_buf[64], rx_buf[64];
    uint8_t *tx_msg = &tx_buf[0], *rx_msg = &rx_buf[0];
    uint8_t result = 0;
    uint16_t length = 0;
    uint32_t rev_id = 0;

    memset(tx_buf, 0, sizeof(tx_buf));
    memset(rx_buf, 0, sizeof(rx_buf));

    _pm_msg_put_half_word(&tx_msg, 0x0000);          /* Length placeholder */
    _pm_msg_put_byte(&tx_msg, port_cfg->PortNum);
    _pm_msg_put_byte(&tx_msg, port_cfg->PortType);
    _pm_msg_put_byte(&tx_msg, port_cfg->PortMode);
    _pm_msg_put_byte(&tx_msg, port_cfg->PortSpeed);
    _pm_msg_put_byte(&tx_msg, port_cfg->SPMPortID);
    _pm_msg_put_byte(&tx_msg, port_cfg->LPMPortID);
    _pm_msg_put_byte(&tx_msg, port_cfg->PortOptions);
    length += 7;
    if (port_cfg->PortOptions & 0x80) {
        _pm_msg_put_half_word(&tx_msg, port_cfg->IngFixedLatency);
        _pm_msg_put_half_word(&tx_msg, port_cfg->EgrFixedLatency);
        length += 4;
    }
    if (port_cfg->PortMode) {
        _pm_msg_put_byte(&tx_msg, port_cfg->FOOptions);
        _pm_msg_put_byte(&tx_msg, port_cfg->FOPortNum);
        _pm_msg_put_byte(&tx_msg, port_cfg->FOPortID);
        length += 3;
    }
    PHYMOD_IF_ERR_RETURN(_pm_get_chip_rev(phy, &rev_id));
    if (rev_id == APERTA_REV_B0) {
        if (port_cfg->PortOptions & 0x08) {
            _pm_msg_put_half_word(&tx_msg, port_cfg->EgrptpFixedLatency);
            length += 2;
        }
    } else {
        if (port_cfg->PortOptions & 0x08) {
            return PHYMOD_E_PARAM;
        }
    }
    tx_msg = &tx_buf[0];
    _pm_msg_put_half_word(&tx_msg, length);
    PHYMOD_IF_ERR_RETURN(_pm_msg_send(phy, APERTA_FUNC_CONFIG_PORT, APERTA_OP_WRITE, &tx_buf[0], &rx_buf[0], &result));
    if (APERTA_OP_SUCCESS != result) {
        LOG_INFO("FW ConfigPort failed: 0x%x\n", result);
        return PHYMOD_E_INTERNAL;
    }
    return PHYMOD_E_NONE;
}
static int _pm_active_flag_set(const plp_aperta_phymod_phy_access_t *phy, int value);

/* ==========================================================================
 * _pm_fw_config_phy_set - CONFIG_PHY.WRITE.
 * MACsecOpt=0x3 (macsec static bypass, matching fw_init
 * macsec_static_bypass=1) + IOOpt=0x0. The FW applies the MACsec bypass from
 * this message; without it the FW may route frames through a non-bypassed
 * MACsec path and never build the SYS<->LINE passthrough.
 * ========================================================================== */

static int _pm_fw_config_phy_set(const plp_aperta_phymod_phy_access_t *phy, uint8_t macsec_opt, uint8_t io_opt)
{
    uint8_t tx_buf[64], rx_buf[64];
    uint8_t *tx_msg = &tx_buf[0], *rx_msg = &rx_buf[0];
    uint8_t result = 0;
    uint16_t length = 0;

    memset(tx_buf, 0, sizeof(tx_buf));
    memset(rx_buf, 0, sizeof(rx_buf));

    _pm_msg_put_half_word(&tx_msg, 0x0000);     /* Length placeholder */
    _pm_msg_put_byte(&tx_msg, macsec_opt);      /* MACsecOpt */
    _pm_msg_put_byte(&tx_msg, io_opt);          /* IOOpt */
    length += 2;

    tx_msg = &tx_buf[0];
    _pm_msg_put_half_word(&tx_msg, length);
    PHYMOD_IF_ERR_RETURN(_pm_msg_send(phy, APERTA_FUNC_CONFIG_PHY, APERTA_OP_WRITE, &tx_buf[0], &rx_buf[0], &result));
    if (APERTA_OP_SUCCESS != result) {
        LOG_INFO("FW ConfigPhy failed: 0x%x\n", result);
        return PHYMOD_E_INTERNAL;
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * plp_aperta_phy_macsec_init - . The reference runs this INSIDE fw_load, after core_init
 * and before the upper-app lane-swap/polarity. It sends:
 *   IS_ACTIVE (port_attach) + CONFIG_PHY (MACsec bypass) + SWREG_002
 *   (macsec-mode persist) + GPIO INPUT_SEL_1 (module IO) + SWGPREG1F (PLL
 *   active-lane bitmap).
 * ========================================================================== */
int plp_aperta_phy_macsec_init(const plp_aperta_phymod_phy_access_t *phy, int macsec_static_bypass)
{
    uint8_t macsec_opt = macsec_static_bypass ? 0x3 : 0x0;

    if (phy == NULL) {
        return PHYMOD_E_PARAM;
    }

    /* IS_ACTIVE (SWGPREG0E bit1): PM8x50_IS_ACTIVE_SET in port_attach.
     * The FW reads IS_ACTIVE / IS_CORE_INITIALIZED to build the passthrough
     * crossbar; without it the FW never builds the SYS RX -> LINE TX path. */
    PHYMOD_IF_ERR_RETURN(_pm_active_flag_set(phy, 1));

    /* CONFIG_PHY: tell the FW the MACsec bypass option */
    PHYMOD_IF_ERR_RETURN(_pm_fw_config_phy_set(phy, macsec_opt, 0x0));

    /* SWREG_002 = macsec mode (restored after a soft reset) */
    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(phy, APERTA_SWS_SWREG_002r, (uint32_t)macsec_static_bypass));

    /* GPIO INPUT_SEL_1 = 0x402 (module GPIO mux, 1.8V IO signaling) */
    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(phy, APERTA_GPIO_INPUT_SEL_1r, 0x402));

    /* SWGPREG1F = 0xFFFF: PLL0 (low byte) + PLL1 (high byte) active-lane
     * bitmaps = all 8 lanes active. The FW reads this to know which lanes to
     * bring up in the passthrough crossbar. */
    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(phy, APERTA_CTRL_SWGPREG1Fr, 0xFFFF));

    LOG_INFO("phy_init: CONFIG_PHY macsec_opt=0x%x SWREG_002=%d "
             "GPIO=0x402 SWGPREG1F=0xFFFF\n", macsec_opt, macsec_static_bypass);
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_fw_port_op - ENABLE / DISABLE / FLUSH / PAUSE / RESUME port
 * ========================================================================== */
static int _pm_fw_port_op_start(const plp_aperta_phymod_phy_access_t *phy, uint8_t port_num, int operation)
{
    uint8_t tx_buf[64], rx_buf[64];
    uint8_t *tx_msg = &tx_buf[0], *rx_msg = &rx_buf[0];
    uint8_t result = 0, func = 0;

    memset(tx_buf, 0, sizeof(tx_buf));
    memset(rx_buf, 0, sizeof(rx_buf));

    if (operation == APERTA_FW_PORT_OP_ENABLE) {
        func = APERTA_FUNC_ENABLE_PORT;
    } else if (operation == APERTA_FW_PORT_OP_DISABLE) {
        func = APERTA_FUNC_DISABLE_PORT;
    } else if (operation == APERTA_FW_PORT_OP_FLUSH) {
        func = APERTA_FUNC_FLUSH_PORT;
    } else if (operation == APERTA_FW_PORT_OP_PAUSE) {
        func = APERTA_FUNC_PAUSE_PORT;
    } else if (operation == APERTA_FW_PORT_OP_RESUME) {
        func = APERTA_FUNC_RESUME_PORT;
    } else {
        return PHYMOD_E_PARAM;
    }

    if (operation == APERTA_FW_PORT_OP_FLUSH) {
        _pm_msg_put_half_word(&tx_msg, 0x0002);
        _pm_msg_put_byte(&tx_msg, port_num);
        _pm_msg_put_byte(&tx_msg, 1);      /* Reset credits */
    } else {
        _pm_msg_put_half_word(&tx_msg, 0x0001);
        _pm_msg_put_byte(&tx_msg, port_num);
    }

    if ((operation != APERTA_FW_PORT_OP_PAUSE) &&
        (operation != APERTA_FW_PORT_OP_RESUME)) {
        PHYMOD_IF_ERR_RETURN(_pm_msg_send(phy, func, APERTA_OP_START, &tx_buf[0], &rx_buf[0], &result));
        if (APERTA_OP_PROCESSING != result) {
            LOG_INFO("FW port op 0x%x start failed: 0x%x\n", func, result);
            return PHYMOD_E_INTERNAL;
        }
    } else {
        PHYMOD_IF_ERR_RETURN(_pm_msg_send(phy, func, APERTA_OP_WRITE, &tx_buf[0], &rx_buf[0], &result));
        if (APERTA_OP_SUCCESS != result) {
            LOG_INFO("FW port op 0x%x failed: 0x%x\n", func, result);
            return PHYMOD_E_INTERNAL;
        }
    }
    return PHYMOD_E_NONE;
}

static int _pm_fw_port_op_result(const plp_aperta_phymod_phy_access_t *phy, uint8_t port_num, int operation)
{
    uint8_t tx_buf[64], rx_buf[64];
    uint8_t *tx_msg = &tx_buf[0], *rx_msg = &rx_buf[0];
    uint8_t result = 0, func = 0;
    uint32_t retry_cnt = APERTA_FW_MSG_RETRY_CNT;

    memset(tx_buf, 0, sizeof(tx_buf));
    memset(rx_buf, 0, sizeof(rx_buf));

    if (operation == APERTA_FW_PORT_OP_ENABLE) {
        func = APERTA_FUNC_ENABLE_PORT;
    } else if (operation == APERTA_FW_PORT_OP_DISABLE) {
        func = APERTA_FUNC_DISABLE_PORT;
    } else if (operation == APERTA_FW_PORT_OP_FLUSH) {
        func = APERTA_FUNC_FLUSH_PORT;
    } else {
        return PHYMOD_E_PARAM;
    }

    _pm_msg_put_half_word(&tx_msg, 0x0001);
    _pm_msg_put_byte(&tx_msg, port_num);
    do {
        PHYMOD_IF_ERR_RETURN(_pm_msg_send(phy, func, APERTA_OP_START_RESULT, &tx_buf[0], &rx_buf[0], &result));
        PHYMOD_USLEEP(100);
    } while ((APERTA_OP_PROCESSING == result) && (retry_cnt--));

    if (retry_cnt == 0) {
        LOG_INFO("FW port op 0x%x not completing: %d\n", func, result);
        return PHYMOD_E_INTERNAL;
    }
    if (APERTA_OP_SUCCESS == result) {
        return PHYMOD_E_NONE;
    }
    return PHYMOD_E_INTERNAL;
}

static int _pm_fw_port_op(const plp_aperta_phymod_phy_access_t *phy, uint8_t port_num, int operation)
{
    PHYMOD_IF_ERR_RETURN(_pm_fw_port_op_start(phy, port_num, operation));
    if ((operation != APERTA_FW_PORT_OP_PAUSE) && (operation != APERTA_FW_PORT_OP_RESUME)) {
        PHYMOD_IF_ERR_RETURN(_pm_fw_port_op_result(phy, port_num, operation));
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_port_active_set - set PORTn_CONFIG bit15 (APERTA_PRT_ACTIVE)
 * ========================================================================== */
static int _pm_port_active_set(const plp_aperta_phymod_phy_access_t *phy, uint32_t port_num, uint32_t enable)
{
    uint32_t rd;
    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, APERTA_PORTn_CONFIGr(port_num), &rd));
    if (enable) {
        rd |= (1 << APERTA_PRT_ACTIVE_SHIFT);
    } else {
        rd &= ~(1u << APERTA_PRT_ACTIVE_SHIFT);
    }
    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(phy, APERTA_PORTn_CONFIGr(port_num), rd));
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_fault_option_set - set PORTn_CONFIG FAULT_OPTION (bit12).
 * on the SYS side before mode_config.
 * FAULT_OPTION = 1 (Pass through) makes the SYS MAC pass the LINE-side
 * fault/link-down through to the switch, so the switch ce port goes down when
 * the far end drops the LINE interface. Without it (reset 0 = Terminate &
 * generate) the retimer terminates the fault and keeps the SYS link up.
 * ========================================================================== */
static int _pm_fault_option_set(const plp_aperta_phymod_phy_access_t *phy, uint32_t port, int fault_option)
{
    uint32_t data;
    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, APERTA_PORTn_CONFIGr(port), &data));
    data &= ~(1u << APERTA_FAULT_SHIFT);
    data |= ((uint32_t)(fault_option & 1) << APERTA_FAULT_SHIFT);
    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(phy, APERTA_PORTn_CONFIGr(port), data));
    LOG_INFO("pm_intf_cfg: FAULT_OPTION set=%d (PORT%d_CONFIG=0x%x)\n",
             (int)(fault_option & 1), (int)port, (unsigned)(data & 0xFFFFu));
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_tx_rx_enable - CDMAC TX/RX enable/disable
 * ========================================================================== */
static int _pm_tx_rx_enable(const plp_aperta_phymod_phy_access_t *phy, int enable, int single_port, int failover)
{
    int side = 0, count = 0;
    uint32_t temp = 0, system_lane_map = 0;
    plp_aperta_phymod_phy_access_t temp_access;
    plp_aperta_phymod_phy_access_t phy1;

    PHYMOD_MEMCPY(&temp_access, phy, sizeof(temp_access));
    PHYMOD_MEMCPY(&phy1, phy, sizeof(phy1));

    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, APERTA_SYSTEM_SIDE_NO_OF_LANES, &system_lane_map));
    system_lane_map >>= 8;

    if (!enable) {
        for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
            temp_access.port_loc = (plp_aperta_phymod_port_loc_t)side;
            for (count = 0; count < APERTA_PM_NUM_LANES; count++) {
                if (!(phy->access.lane_mask & (1 << count))) {
                    continue;
                }
                temp_access.access.lane_mask = 1 << count;
                /* Clear RX_EN on the current side CDMAC_CTRL */
                PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&temp_access, APERTA_CDMAC_CTRLr, &temp));
                if (temp & APERTA_CDMAC_CTRL_RX_EN) {
                    temp &= ~(uint32_t)APERTA_CDMAC_CTRL_RX_EN;
                    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(&temp_access, APERTA_CDMAC_CTRLr, temp));
                }
                /* Set DISCARD on the opposite-side CDMAC_TX_CTRL */
                temp_access.port_loc = (temp_access.port_loc == phymodPortLocLine) ?
                                       phymodPortLocSys : phymodPortLocLine;
                PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&temp_access, APERTA_CDMAC_TX_CTRLr, &temp));
                if ((temp & APERTA_CDMAC_TX_CTRL_DISCARD) != APERTA_CDMAC_TX_CTRL_DISCARD) {
                    temp |= APERTA_CDMAC_TX_CTRL_DISCARD;
                    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(&temp_access, APERTA_CDMAC_TX_CTRLr, temp));
                }
                temp_access.port_loc = (plp_aperta_phymod_port_loc_t)side;
            }
        }
    } else {
        /* 1. clear DISCARD, 2. clear SOFT_RESET, 3. TX_EN, 4. RX_EN on both sides */
        for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
            temp_access.port_loc = (plp_aperta_phymod_port_loc_t)side;
            if (side == phymodPortLocSys) {
                if (!failover) {
                    phy1.access.lane_mask = system_lane_map;
                }
                if (failover && phy->port_loc == phymodPortLocLine) {
                    continue;
                }
            } else {
                if (phy->port_loc == phymodPortLocSys) {
                    continue;
                }
                phy1.access.lane_mask = phy->access.lane_mask;
            }
            for (count = 0; count < APERTA_PM_NUM_LANES; count++) {
                if (!(phy1.access.lane_mask & (1 << count))) {
                    continue;
                }
                temp_access.access.lane_mask = 1 << count;
                PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&temp_access, APERTA_CDMAC_TX_CTRLr, &temp));
                temp &= ~(uint32_t)APERTA_CDMAC_TX_CTRL_DISCARD;
                PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(&temp_access, APERTA_CDMAC_TX_CTRLr, temp));
            }
        }
        for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
            temp_access.port_loc = (plp_aperta_phymod_port_loc_t)side;
            if (side == phymodPortLocSys) {
                if (!failover) {
                    phy1.access.lane_mask = system_lane_map;
                }
                if (failover && phy->port_loc == phymodPortLocLine) {
                    continue;
                }
            } else {
                if (phy->port_loc == phymodPortLocSys) {
                    continue;
                }
                phy1.access.lane_mask = phy->access.lane_mask;
            }
            for (count = 0; count < APERTA_PM_NUM_LANES; count++) {
                if (!(phy1.access.lane_mask & (1 << count))) {
                    continue;
                }
                temp_access.access.lane_mask = 1 << count;
                PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&temp_access, APERTA_CDMAC_CTRLr, &temp));
                temp &= ~(uint32_t)APERTA_CDMAC_CTRL_SOFT_RESET;
                PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(&temp_access, APERTA_CDMAC_CTRLr, temp));
            }
        }
        for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
            temp_access.port_loc = (plp_aperta_phymod_port_loc_t)side;
            if (side == phymodPortLocSys) {
                if (!failover) {
                    phy1.access.lane_mask = system_lane_map;
                }
                if (failover && phy->port_loc == phymodPortLocLine) {
                    continue;
                }
            } else {
                if (phy->port_loc == phymodPortLocSys) {
                    continue;
                }
                phy1.access.lane_mask = phy->access.lane_mask;
            }
            for (count = 0; count < APERTA_PM_NUM_LANES; count++) {
                if (!(phy1.access.lane_mask & (1 << count))) {
                    continue;
                }
                temp_access.access.lane_mask = 1 << count;
                PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&temp_access, APERTA_CDMAC_CTRLr, &temp));
                temp |= APERTA_CDMAC_CTRL_TX_EN;
                PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(&temp_access, APERTA_CDMAC_CTRLr, temp));
            }
        }
        for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
            temp_access.port_loc = (plp_aperta_phymod_port_loc_t)side;
            if (side == phymodPortLocSys) {
                if (!failover) {
                    phy1.access.lane_mask = system_lane_map;
                }
                if (failover && phy->port_loc == phymodPortLocLine) {
                    continue;
                }
            } else {
                if (phy->port_loc == phymodPortLocSys) {
                    continue;
                }
                phy1.access.lane_mask = phy->access.lane_mask;
            }
            for (count = 0; count < APERTA_PM_NUM_LANES; count++) {
                if (!(phy1.access.lane_mask & (1 << count))) {
                    continue;
                }
                temp_access.access.lane_mask = 1 << count;
                PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&temp_access, APERTA_CDMAC_CTRLr, &temp));
                temp |= APERTA_CDMAC_CTRL_RX_EN;
                PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(&temp_access, APERTA_CDMAC_CTRLr, temp));
            }
        }
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_tx_rx_enable_post - finish the CDMAC disable.
 * for every lane on both sides clear TX_EN (bit0) then set
 * SOFT_RESET (bit6) on CDMAC_CTRL (0x1400010b), so the datapath is held in
 * reset while the SC applies the new speed. Skipping this leaves the lanes
 * running into the speed change.
 * ========================================================================== */
static int _pm_tx_rx_enable_post(const plp_aperta_phymod_phy_access_t *phy, int enable)
{
    int side, count;
    uint32_t rd;
    plp_aperta_phymod_phy_access_t temp_access;
    PHYMOD_MEMCPY(&temp_access, phy, sizeof(temp_access));
    if (enable) {
        return PHYMOD_E_NONE;
    }
    for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
        temp_access.port_loc = (plp_aperta_phymod_port_loc_t)side;
        for (count = 0; count < APERTA_PM_NUM_LANES; count++) {
            if (!(phy->access.lane_mask & (1 << count))) {
                continue;
            }
            temp_access.access.lane_mask = 1 << count;
            /* clear TX_EN */
            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&temp_access, APERTA_CDMAC_CTRLr, &rd));
            rd &= ~(uint32_t)APERTA_CDMAC_CTRL_TX_EN;
            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(&temp_access, APERTA_CDMAC_CTRLr, rd));
            /* set SOFT_RESET */
            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&temp_access, APERTA_CDMAC_CTRLr, &rd));
            rd |= (uint32_t)APERTA_CDMAC_CTRL_SOFT_RESET;
            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(&temp_access, APERTA_CDMAC_CTRLr, rd));
        }
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * TSCBH register helpers (base 0x18000000, 16-bit writes like the SDK)
 * ========================================================================== */
static int _pm_bh_rd(const plp_aperta_phymod_phy_access_t *phy, uint16_t off, uint32_t *val)
{
    int rv;
    rv = plp_aperta_reg32_read(phy, PHYMOD_REG_APERTA_TSCBH | off, val);
    if (rv != PHYMOD_E_NONE) {
        LOG_INFO("BH rd off=0x%x rv=%d\n", off, rv);
        return rv;
    }
    *val &= 0xFFFF;
    return PHYMOD_E_NONE;
}

static int _pm_bh_wr(const plp_aperta_phymod_phy_access_t *phy, uint16_t off, uint32_t val)
{
    int rv;
    rv = plp_aperta_reg32_write(phy, PHYMOD_REG_APERTA_TSCBH | off, val & 0xFFFF);
    if (rv != PHYMOD_E_NONE) {
        LOG_INFO("BH wr off=0x%x rv=%d\n", off, rv);
        return rv;
    }
    return PHYMOD_E_NONE;
}

/* Modify a bitfield (RMW) on a TSC register */
static int _pm_bh_mwr(const plp_aperta_phymod_phy_access_t *phy, uint16_t off, uint32_t mask, uint32_t shift, uint32_t val)
{
    uint32_t rd;
    PHYMOD_IF_ERR_RETURN(_pm_bh_rd(phy, off, &rd));
    rd = (rd & ~mask) | ((val << shift) & mask);
    PHYMOD_IF_ERR_RETURN(_pm_bh_wr(phy, off, rd));
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_phy_power_on - power on TX+RX for both sides. clears the per-lane
 * TX (0xd1b1 bit0) and RX (0xd1a1 bit0) power-down bits for both Line and Sys
 * sides. The SDK does this BEFORE the speed config; if the SYS lanes are left
 * powered-down from reset the SYS SC cannot resolve the forced speed and SYS
 * TX never outputs -> the switch never locks -> ce0 stays down.
 * ========================================================================== */
static int _pm_phy_power_on(const plp_aperta_phymod_phy_access_t *phy)
{
    int side, lane;
    plp_aperta_phymod_phy_access_t pc;
    PHYMOD_MEMCPY(&pc, phy, sizeof(pc));
    pc.access.pll_idx = 0;
    for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
        pc.port_loc = (plp_aperta_phymod_port_loc_t)side;
        for (lane = 0; lane < APERTA_PM_NUM_LANES; lane++) {
            if (!(phy->access.lane_mask & (1u << lane))) {
                continue;
            }
            pc.access.lane_mask = 1u << lane;
            PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&pc, 0xd1b1, 0x1, 0, 0)); /* TX pwrdn off */
            PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&pc, 0xd1a1, 0x1, 0, 0)); /* RX pwrdn off */
        }
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * blackhawk lane/core helpers
 * ========================================================================== */
static int _pm_lane_soft_reset(const plp_aperta_phymod_phy_access_t *phy, int reset)
{
    /* LN_DP_S_RSTB (0xd0b1 bit0) is ACTIVE-LOW: writing 0 asserts the lane
     * datapath soft reset, writing 1 releases it. The SDK's
     * plp_aperta_blackhawk_lane_soft_reset(enable): enable=1 (hold) -> bit=0,
     * enable=0 (release) -> bit=1. (This was previously inverted, leaving the
     * lanes stuck in reset after every speed config -> RX PMD could never
     * lock, SYS SC never resolved, no link ever came up.) */
    return _pm_bh_mwr(phy, BH_LANE_SOFT_RESET, 0x1, 0, reset ? 0 : 1);
}

static int _pm_pll_selection_set(const plp_aperta_phymod_phy_access_t *phy, uint32_t pll_index)
{
    return _pm_bh_mwr(phy, BH_PLL_SELECT, 0x1, 0, pll_index);
}

static int _pm_osr_mode_set(const plp_aperta_phymod_phy_access_t *phy, uint32_t osr_mode)
{
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(phy, BH_OSR_MODE, 0xF, 0, osr_mode));
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(phy, BH_OSR_MODE, 0x8000, 15, 1));
    return PHYMOD_E_NONE;
}

static int _pm_core_dp_reset(const plp_aperta_phymod_phy_access_t *phy, int reset)
{
    /* core_dp_s_rstb: 0 = assert reset, 1 = de-assert */
    return _pm_bh_mwr(phy, BH_TOP_USER_CONTROL, 0x2000, 13, reset ? 0 : 1);
}

static int __attribute__((unused)) _pm_pll_pwrdn_get(const plp_aperta_phymod_phy_access_t *phy, uint32_t *pwrdn)
{
    uint32_t rd;
    PHYMOD_IF_ERR_RETURN(_pm_bh_rd(phy, BH_AMS_PLL_PWRDN, &rd));
    *pwrdn = (rd >> 2) & 0x1;
    return PHYMOD_E_NONE;
}

static int _pm_pll_lock_get(const plp_aperta_phymod_phy_access_t *phy, uint32_t *locked)
{
    uint32_t rd;
    PHYMOD_IF_ERR_RETURN(_pm_bh_rd(phy, BH_PLL_LOCK, &rd));
    *locked = (rd >> 8) & 0x1;
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_vco_to_pll_get - map lane data rate -> PLL divider mode
 * ========================================================================== */
static int _pm_vco_to_pll_get(uint32_t lane_rate, uint32_t *pll_div)
{
    switch (lane_rate) {
        case bcmplpApertaLaneDataRate_20P625G:
            *pll_div = APERTA_TBHMOD_PLL_MODE_DIV_132;
            break;
        case bcmplpApertaLaneDataRate_25P78125G:
            *pll_div = APERTA_TBHMOD_PLL_MODE_DIV_165;
            break;
        case bcmplpApertaLaneDataRate_26P5625G:
            *pll_div = APERTA_TBHMOD_PLL_MODE_DIV_170;
            break;
        case bcmplpApertaLaneDataRate_10P3125G:
            *pll_div = APERTA_TBHMOD_PLL_MODE_DIV_132;
            break;
        default:
            /* PAM4 / other rates not in the 8x50 NRZ table yet */
            return PHYMOD_E_UNAVAIL;
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_pll_program - AMS PLL register programming (no reset / lock handling)
 * Port of the register writes in plp_aperta_blackhawk_tsc_INTERNAL_configure_pll
 * Caller must hold the core in DP reset. pll_div is
 * an APERTA_TBHMOD_PLL_MODE_DIV_* value.
 * ========================================================================== */
static int _pm_pll_program(const plp_aperta_phymod_phy_access_t *phy, uint32_t pll_div)
{
    uint32_t is_fractional = (pll_div & 0x80000000) ? 1 : 0;
    uint32_t div_integer = pll_div & 0xFF;
    uint32_t vco_freq_khz = div_integer * 156250u;   /* refclk 156.25MHz -> kHz */
    uint32_t int_configured = 0;
    uint32_t pll_mode = 0xFF, i;
    plp_aperta_phymod_phy_access_t phy_copy;

    /* Faithful port of plp_aperta_blackhawk_tsc_INTERNAL_configure_pll
     * EVERY write, in the EXACT same order. Caller
     * must hold the core in DP reset. */
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane_mask = 1;

    /* Power ON the PLL core (afe_s_pll_pwrdn = 0xd184 bit14, active-high
     * powerdown). Calls core_pwndn(PWR_ON)
     * before configuring; without it a powered-down PLL never locks. */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_TOP_USER_CONTROL, 0x4000, 14, 0));

    /* ---- Default-reset phase */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_MODE, 0x1F, 0, 0x7));          /* pll_mode=7      */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_VCO2_15G, 0x80, 7, 0));        /* vco2_15g=0      */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_FRACN_CTRL, 0x3FF0, 4, 0));    /* fracn_ndiv_int  */
    PHYMOD_IF_ERR_RETURN(_pm_bh_wr(&phy_copy, BH_AMS_PLL_FRACN_DIV, 0));                 /* fracn_div       */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_FRACN_CTRL, 0x3, 0, 0));       /* div_17_16       */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_FRACN_CTRL, 0x8, 3, 0));       /* fracn_bypass    */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_FRACN_CTRL, 0x4, 2, 0));       /* fracn_divrange  */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_FRACN_CTRL, 0x4000, 14, 0));   /* ditheren        */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_FRACN_CTRL, 0x8000, 15, 0));   /* fracn_sel       */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_CTRL_114, 0x3000, 12, 0));     /* kvh_force       */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_CTRL_114, 0x4000, 14, 0));     /* force_kvh_bw    */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_REFCLK_CTRL, 0x8000, 15, 0));  /* refclk_doubler  */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_REFCLK_CTRL, 0x7000, 12, 0));  /* doubler_res     */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_REFCLK_CTRL, 0x0700, 8, 0));   /* doubler_cap     */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_REFCLK_CTRL, 0x2, 1, 0));      /* div2            */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_REFCLK_CTRL, 0x4, 2, 0));      /* div4            */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_PWRDN, 0x2, 1, 0));            /* div4_2_sel      */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_NDIV_FRAC_VALID, 0x10, 4, 0)); /* ndiv_frac_valid */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_CTRL_114, 0x0300, 8, 0));      /* pll2rx_clkbw    */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_REFCLK_DIVCNT, 0x3FFF, 0, 0x27));      /* refclk_divcnt   */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_VCO_STEP_TIME, 0xFF, 0, 0x3));         /* vco_step_time=3 */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_COM_CTL_110, 0x0100, 8, 0));   /* fp3_rh          */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_COM_CTL_110, 0x00f0, 4, 0));   /* fp3_ctrl        */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_IQP, 0xF, 0, 0x4));            /* iqp=4           */

    /* ---- Clear AMS PLL powerdown ------------------------------------------ */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_PWRDN, 0x4, 2, 0));

    /* ---- VCO2 selection (use VCO2 for 22.6GHz and below) ------------------ */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_VCO2_15G, 0x80, 7, (vco_freq_khz < 22800000u) ? 1 : 0));

    /* ---- Integer / fractional divider ------------------------------------- */
    int_configured = 0;
    if (!is_fractional) {
        for (i = 0; i < sizeof(_pll_mode_div_lkup) / sizeof(_pll_mode_div_lkup[0]); i++) {
            if (_pll_mode_div_lkup[i] == div_integer) { pll_mode = (uint32_t)i; break; }
        }
        if (pll_mode == 0xFF) {
            LOG_INFO("Unsupported integer PLL div %d\n", div_integer);
            return PHYMOD_E_CONFIG;
        }
        int_configured = 1;
        PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_MODE, 0x1F, 0, pll_mode));
    } else {
        /* Fractional mode (e.g. DIV_82P5): div by 2 with fraction. */
        const uint32_t div_fraction_width = 28;
        const uint32_t pll_fracn_frac_bits = 18;
        uint32_t div_int2 = div_integer >> 1;
        uint32_t div_frac = (uint32_t)(((div_integer & 1) << (div_fraction_width - 1)));
        uint32_t div_frac_0p5 = 1 << (div_fraction_width - pll_fracn_frac_bits - 1);
        uint32_t frac_plus = div_frac + div_frac_0p5;
        uint32_t carry = frac_plus >> div_fraction_width;
        uint32_t ndiv_int = div_int2 + carry;
        uint32_t fracn_div = ((frac_plus & ((1u << div_fraction_width) - 1)) >>
                              (div_fraction_width - pll_fracn_frac_bits));

        PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_MODE, 0x1F, 0, 0));
        PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_FRACN_CTRL, 0x3FF0, 4, ndiv_int));
        PHYMOD_IF_ERR_RETURN(_pm_bh_wr(&phy_copy, BH_AMS_PLL_FRACN_DIV, fracn_div & 0xFFFF));
        PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_FRACN_CTRL, 0x3, 0, (fracn_div >> 16) & 0x3));
        PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_FRACN_CTRL, 0x8, 3, 0));      /* bypass      */
        PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_FRACN_CTRL, 0x4, 2, ndiv_int < 91 ? 1 : 0)); /* divrange */
        PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_FRACN_CTRL, 0x8000, 15, 1));  /* fracn_sel=1 */
        PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_NDIV_FRAC_VALID, 0x10, 4, 1));
        PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_NDIV_FRAC_VALID, 0x10, 4, 0));
    }

    /* ---- fracn_sel (integer -> 0, fractional -> 1) ------------------------- */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_FRACN_CTRL, 0x8000, 15,
                                    int_configured ? 0 : 1));

    /* ---- KVH / pll2rx_clkbw (vco-based, CRSRDSAPI-137/FW-138) -------------- */
    {
        uint32_t kvh_force = 1, pll2rx_clkbw = 1;
        if (vco_freq_khz > 27400000u) kvh_force = 0;
        else if (vco_freq_khz <= 18750000u) kvh_force = 3;
        if (vco_freq_khz > 25781250u) pll2rx_clkbw = 0;
        else if (vco_freq_khz <= 22600000u) pll2rx_clkbw = 3;
        PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_CTRL_114, 0x3000, 12, kvh_force));
        PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_CTRL_114, 0x0300, 8, pll2rx_clkbw));
        PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_AMS_PLL_CTRL_114, 0x4000, 14, 1)); /* force_kvh_bw */
    }

    /* ---- Force vco_step_time = 8 (CRSRDSAPI-229), final value -------------- */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_VCO_STEP_TIME, 0xFF, 0, 0x8));

    /* ---- Update micro core-config with the VCO rate (uc_core_config) -------
     * SDK configure_pll ends with set_uc_core_config: the micro needs to know
     * the target VCO frequency (vco_rate, 0xd18d bits 7:0) before it can run
     * the PLL calibration sequencer (cap/freq/lock). Missing before -> the
     * sequencer never started (0xd148 bits 9-14 stayed 0).
     * vco_rate = MHZ_TO_VCO_RATE(mhz) = ((mhz*2 + 62)/125) - 232. */
    {
        uint32_t vco_rate_mhz = (vco_freq_khz + 500u) / 1000u;
        uint32_t vco_rate = (((vco_rate_mhz * 2u) + 62u) / 125u) - 232u;
        uint32_t cfg_word;
        PHYMOD_IF_ERR_RETURN(_pm_bh_rd(&phy_copy, BH_UC_CORE_CONFIG, &cfg_word));
        cfg_word = (cfg_word & 0x100u) | (vco_rate & 0xFFu); /* keep core_cfg_from_pcs */
        PHYMOD_IF_ERR_RETURN(_pm_bh_wr(&phy_copy, BH_UC_CORE_CONFIG, cfg_word));
        LOG_INFO("PLL program: uc_core_config(0xd18d)=0x%x vco_rate=0x%x (%uMHz)\n",
                 cfg_word, vco_rate, vco_rate_mhz);
    }

    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_pll_reconfig - AMS PLL reconfiguration (assert reset -> program ->
 * release reset -> wait lock). Used by the port speed-config path.
 * ========================================================================== */
static int _pm_pll_reconfig(const plp_aperta_phymod_phy_access_t *phy, uint32_t pll_div)
{
    uint32_t cnt, pll_lock = 0;
    plp_aperta_phymod_phy_access_t phy_copy;

    /* PLL is a core resource; always address it through lane 0, and the TVCO
     * PLL. The 0xd1xx AMS PLL registers are PMD-type: their address embeds
     * pll_micro_sel = access.pll_idx, so a pll_idx of 0 (from phy_sdk_acc_to_phy
     * memset) would read/write PLL0 (unconfigured) instead of the TVCO PLL1
     * that PASS2 programmed -> the lock check always sees "not locked" and a
     * reconfig would reprogram the wrong PLL. */
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane_mask = 1;
    /* TVCO always sits on PLL1 in this build; set pll_idx explicitly (the
     * shared phy_sdk_acc_to_phy only sets access.tvco_pll_index). */
    phy_copy.access.pll_idx = APERTA_TVCO_PLL_INDEX;

    /* If the PLL is ALREADY configured at the requested divider (PASS2
     * core-init set pll_mode), skip reconfiguration. The SDK's pm_vco_reconfig
     * only re-programs the PLL when the TVCO changes; the reference leaves the
     * PLL untouched after PASS2 and the micro firmware runs the calibration
     * (which sets 0xd148 bit8 PLL_LOCK). Re-asserting DP reset and
     * re-programming an already-configured PLL here disturbs that -> no lock,
     * no lane clock -> no link. Integer mode only (25.781G DIV_165). */
    if (!(pll_div & 0x80000000u)) {
        uint32_t cur_mode = 0, target_mode = 0xFF, i;
        PHYMOD_IF_ERR_RETURN(_pm_bh_rd(&phy_copy, BH_AMS_PLL_MODE, &cur_mode));
        cur_mode &= 0x1F;
        for (i = 0; i < sizeof(_pll_mode_div_lkup) / sizeof(_pll_mode_div_lkup[0]); i++) {
            if (_pll_mode_div_lkup[i] == (pll_div & 0xFF)) { target_mode = (uint32_t)i; break; }
        }
        if (target_mode != 0xFF && cur_mode == target_mode) {
            LOG_INFO("pm_intf_cfg: PLL already configured at div=%d (mode=%d), skip reconfig\n", pll_div & 0xFF, cur_mode);
            return PHYMOD_E_NONE;
        }
    }
    pll_lock = 0;

    /* Hold the PLL in reset while reconfiguring */
    PHYMOD_IF_ERR_RETURN(_pm_core_dp_reset(&phy_copy, 1));
    PHYMOD_IF_ERR_RETURN(_pm_pll_program(&phy_copy, pll_div));
    PHYMOD_IF_ERR_RETURN(_pm_core_dp_reset(&phy_copy, 0));

    /* Wait for PLL lock (50ms @ 10us steps; the SDK uses 5ms, but give the
     * calibration sequencer more time in case it is slow). */
    for (cnt = 0; cnt < 5000; cnt++) {
        PHYMOD_IF_ERR_RETURN(_pm_pll_lock_get(&phy_copy, &pll_lock));
        if (pll_lock) {
            break;
        }
        PHYMOD_USLEEP(10);
    }
    if (!pll_lock) {
        LOG_INFO("PLL not locked within 50ms (div=0x%x addr=%d)\n", pll_div, phy->access.addr);
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_mem_write - LMI memory write 
 * mem_type: APERTA_MEM_TYPE_SPEED_ID / AM_TABLE / UM_TABLE / SPEED_PRIORITY
 * ========================================================================== */
static int _pm_prg_mem_prog_len(uint32_t mem_type, uint32_t *access_prog, uint32_t *len)
{
    switch (mem_type) {
        case APERTA_MEM_TYPE_SPEED_ID:
            *access_prog = 0x7 | (0x2 << 3);   /* 0x17 */
            *len = 10;
            break;
        case APERTA_MEM_TYPE_AM_TABLE:
            *access_prog = 0x7 | (0x3 << 3);   /* 0x1F */
            *len = 5;
            break;
        case APERTA_MEM_TYPE_UM_TABLE:
            *access_prog = 0x7 | (0x4 << 3);   /* 0x27 */
            *len = 4;
            break;
        case APERTA_MEM_TYPE_SPEED_PRIORITY:
            *access_prog = 0x7 | (0xB << 3);   /* 0x5F */
            *len = 18;
            break;
        case APERTA_MEM_TYPE_PM_MIB:
            *access_prog = 0x7 | (0x0 << 3);   /* 0x7 (app phymodMemPmMib) */
            *len = 64;                         /* 32 x 32-bit words = 64 half-words */
            break;
        default:
            *access_prog = 0x7;
            *len = 0;
            return PHYMOD_E_PARAM;
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_fw_mem_type_get - FW memory type
 * ========================================================================== */
static int _pm_fw_mem_type_get(uint32_t mem_type, uint8_t *fw_mem_type)
{
    switch (mem_type) {
        case APERTA_MEM_TYPE_SPEED_ID:       *fw_mem_type = 0x2; break;
        case APERTA_MEM_TYPE_AM_TABLE:       *fw_mem_type = 0x3; break;
        case APERTA_MEM_TYPE_UM_TABLE:       *fw_mem_type = 0x4; break;
        case APERTA_MEM_TYPE_SPEED_PRIORITY: *fw_mem_type = 0xB; break;
        case APERTA_MEM_TYPE_PM_MIB:         *fw_mem_type = 0x0; break;
        default:                             *fw_mem_type = 0x1; break;
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_is_fw_dloaded - pm_info is_fw_dloaded flag 
 * ========================================================================== */
static int _pm_is_fw_dloaded(uint32_t phy_id, uint32_t *active)
{
    unsigned short cnt;
    *active = 0;
    for (cnt = 0; cnt < APERTA_MAX_PM_INFO; cnt++) {
        if (_plp_aperta_pm_info[cnt].phy_id == phy_id) {
            *active = _plp_aperta_pm_info[cnt].is_fw_dloaded;
            break;
        }
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_fw_mem_write - firmware-mediated memory write
 * ========================================================================== */
static int _pm_fw_mem_write(const plp_aperta_phymod_phy_access_t *phy, uint32_t mem_type, uint32_t mem_index, uint32_t *data)
{
    uint8_t tx_buf[256], rx_buf[256];
    uint8_t *tx_msg, result = 0, fw_mem_type = 1;
    uint32_t access_prog = 0, len = 0, write_count = 0, i;
    int rv;

    rv = _pm_prg_mem_prog_len(mem_type, &access_prog, &len);
    if (rv != PHYMOD_E_NONE || len == 0) {
        return (rv != PHYMOD_E_NONE) ? rv : PHYMOD_E_PARAM;
    }
    _pm_fw_mem_type_get(mem_type, &fw_mem_type);

    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
    PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));
    tx_msg = &tx_buf[0];

    /* Length, SideSel, AccessType, DataLen, Mask, Cnt, Addr, data */
    _pm_msg_put_half_word(&tx_msg, (uint16_t)(0x7 + (len * 2)));
    _pm_msg_put_byte(&tx_msg, APERTA_IS_LINE_SIDE(phy) ? 1 : 0);
    _pm_msg_put_byte(&tx_msg, (uint8_t)(0xF0 | fw_mem_type));
    _pm_msg_put_byte(&tx_msg, (uint8_t)(len * 2));
    _pm_msg_put_byte(&tx_msg, 0x0);               /* Mask */
    _pm_msg_put_byte(&tx_msg, 0x1);               /* Cnt */
    _pm_msg_put_half_word(&tx_msg, (uint16_t)mem_index);
    /* Exactly 'len' data half-words (LSB then MSB of each word); the FW reads
     * DataLen bytes, the extra half-words the SDK loop emits are ignored. */
    write_count = 0;
    for (i = 0; write_count < len; i++) {
        _pm_msg_put_half_word(&tx_msg, (uint16_t)(data[i] & 0xFFFF));
        if (++write_count >= len) break;
        _pm_msg_put_half_word(&tx_msg, (uint16_t)((data[i] >> 16) & 0xFFFF));
        if (++write_count >= len) break;
    }

    rv = _pm_msg_send(phy, APERTA_FUNC_PM_REGS, APERTA_OP_WRITE, tx_buf, rx_buf, &result);
    if (rv != PHYMOD_E_NONE) {
        LOG_INFO("FW mem[%d] idx=%d send rv=%d\n", mem_type, mem_index, rv);
        return rv;
    }
    if (APERTA_OP_SUCCESS != result) {
        LOG_INFO("FW mem[%d] idx=%d failed result=0x%x\n", mem_type, mem_index, result);
        return PHYMOD_E_INTERNAL;
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_fw_mem_read - read a TSC memory table entry through the FW mailbox.
 * Response: [len_lo len_hi data...], data is mem_len half-words little-endian.
 * ========================================================================== */
static int _pm_fw_mem_read(const plp_aperta_phymod_phy_access_t *phy, uint32_t mem_type, uint32_t mem_index, uint32_t *data)
{
    uint8_t tx_buf[64], rx_buf[256];
    uint8_t *tx_msg;
    uint8_t result = 0, fw_mem_type = 1;
    uint32_t access_prog = 0, len = 0;
    int rv, i;
    uint16_t rlen;

    rv = _pm_prg_mem_prog_len(mem_type, &access_prog, &len);
    if (rv != PHYMOD_E_NONE || len == 0) {
        return PHYMOD_E_PARAM;
    }
    _pm_fw_mem_type_get(mem_type, &fw_mem_type);

    memset(tx_buf, 0, sizeof(tx_buf));
    memset(rx_buf, 0, sizeof(rx_buf));
    tx_msg = &tx_buf[0];

    _pm_msg_put_half_word(&tx_msg, 0x6);
    _pm_msg_put_byte(&tx_msg, APERTA_IS_LINE_SIDE(phy) ? 1 : 0); /* SideSel */
    _pm_msg_put_byte(&tx_msg, (uint8_t)(0xF0 | fw_mem_type));     /* AccessType */
    _pm_msg_put_byte(&tx_msg, (uint8_t)(len * 2));                /* DataLen */
    _pm_msg_put_byte(&tx_msg, 0x1);                               /* Cnt */
    _pm_msg_put_half_word(&tx_msg, (uint16_t)mem_index);          /* Addr */

    rv = _pm_msg_send(phy, APERTA_FUNC_PM_REGS, APERTA_OP_READ, tx_buf, rx_buf, &result);
    if (rv != PHYMOD_E_NONE) {
        return rv;
    }
    if (APERTA_OP_SUCCESS != result) {
        return PHYMOD_E_INTERNAL;
    }

    rlen = (uint16_t)(rx_buf[0] | ((uint16_t)rx_buf[1] << 8));
    for (i = 0; i < (int)(rlen / 4); i++) {
        data[i] = (uint32_t)rx_buf[2 + i * 4] |
                  ((uint32_t)rx_buf[2 + i * 4 + 1] << 8) |
                  ((uint32_t)rx_buf[2 + i * 4 + 2] << 16) |
                  ((uint32_t)rx_buf[2 + i * 4 + 3] << 24);
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_mem_write - write a TSC memory table.
 * ========================================================================== */
static int _pm_mem_write(const plp_aperta_phymod_phy_access_t *phy, uint32_t mem_type, uint32_t mem_index, uint32_t *data)
{
    uint32_t access_prog = 0, len = 0, done = 0, retry_cnt = 100;
    uint32_t lmi_cmd = 0, i, write_count = 0;
    uint32_t is_dloaded = 0;
    int rv;

    rv = _pm_prg_mem_prog_len(mem_type, &access_prog, &len);
    if (rv != PHYMOD_E_NONE || len == 0) {
        return (rv != PHYMOD_E_NONE) ? rv : PHYMOD_E_PARAM;
    }

    /* Once the ucode is booted (is_fw_dloaded==1, i.e. PASS2), memory tables
     * must be written through the firmware mailbox; the raw LMI path does not
     * complete then (post-timeout sts=0x100) and can hang the PHY. */
    _pm_is_fw_dloaded(phy->access.addr, &is_dloaded);
    if (is_dloaded) {
        return _pm_fw_mem_write(phy, mem_type, mem_index, data);
    }

    /* Wait for LMI not busy (DONE==0). DONE is LMI_STATUS bit0
     * (aperta_d_defs.h: LMI_LMI_STATUSr_DONEf_GET(r) = (r)&0x1). */
    retry_cnt = 100;
    do {
        rv = plp_aperta_direct_reg_read(phy, APERTA_LMI_STATUSr, &done);
        if (rv != PHYMOD_E_NONE) { LOG_INFO("LMI mem[%d] idx=%d sts rv=%d\n", mem_type, mem_index, rv); return rv; }
        PHYMOD_USLEEP(100);
    } while (((done & 0x1) != 0) && (--retry_cnt));
    if (retry_cnt == 0) {
        LOG_INFO("LMI mem[%d] pre-timeout sts=0x%x\n", mem_type, done);
        return PHYMOD_E_TIMEOUT;
    }

    /* Build LMI_CMD with the SDK field layout:
     *   USE_CMD_EXT bit15 | PORT_SEL [14:12] | SLAVE_SEL [11:8]
     *   | ACCESS_PROG [7:1] | WRITE_EN bit0 */
    lmi_cmd  = ((uint32_t)(APERTA_IS_LINE_SIDE(phy) ? APERTA_LINE_SIDE_PM
                                                    : APERTA_SYS_SIDE_PM) & 0xF) << 8; /* SLAVE_SEL */
    lmi_cmd |= (access_prog & 0x7F) << 1;   /* ACCESS_PROG */
    lmi_cmd |= (0 & 0x7) << 12;             /* PORT_SEL = 0 */
    lmi_cmd |= (1 & 0x1);                   /* WRITE_EN = 1 */
    lmi_cmd |= (0 & 0x1) << 15;             /* USE_CMD_EXT = 0 */

    /* Program LMI_CMD via LMI_CMD_SEQ */
    rv = plp_aperta_direct_reg_write(phy, APERTA_LMI_CMD_SEQr, lmi_cmd);
    if (rv != PHYMOD_E_NONE) { LOG_INFO("LMI mem[%d] idx=%d cmd rv=%d\n", mem_type, mem_index, rv); return rv; }

    /* Write exactly 'len' data half-words (LSB then MSB of each word); the
     * hardware entry width is len half-words (AM=5, UM=4, SpeedID=10...). */
    write_count = 0;
    for (i = 0; write_count < len; i++) {
        rv = plp_aperta_direct_reg_write(phy, APERTA_LMI_CMD_SEQr, data[i] & 0xFFFF);
        if (rv != PHYMOD_E_NONE) { LOG_INFO("LMI mem[%d] idx=%d data[%d] rv=%d\n", mem_type, mem_index, i, rv); return rv; }
        if (++write_count >= len) break;
        rv = plp_aperta_direct_reg_write(phy, APERTA_LMI_CMD_SEQr, (data[i] >> 16) & 0xFFFF);
        if (rv != PHYMOD_E_NONE) { LOG_INFO("LMI mem[%d] idx=%d data[%d]hi rv=%d\n", mem_type, mem_index, i, rv); return rv; }
        if (++write_count >= len) break;
    }
    /* Write the memory address */
    rv = plp_aperta_direct_reg_write(phy, APERTA_LMI_CMD_SEQr, mem_index);
    if (rv != PHYMOD_E_NONE) { LOG_INFO("LMI mem[%d] idx=%d addr rv=%d\n", mem_type, mem_index, rv); return rv; }

    /* Wait for DONE==1 */
    retry_cnt = 100;
    do {
        rv = plp_aperta_direct_reg_read(phy, APERTA_LMI_STATUSr, &done);
        if (rv != PHYMOD_E_NONE) { LOG_INFO("LMI mem[%d] idx=%d sts2 rv=%d\n", mem_type, mem_index, rv); return rv; }
        PHYMOD_USLEEP(100);
    } while (((done & 0x1) != 1) && (--retry_cnt));
    if (retry_cnt == 0) {
        LOG_INFO("LMI mem[%d] post-timeout sts=0x%x idx=%d\n", mem_type, done, mem_index);
        return PHYMOD_E_TIMEOUT;
    }

    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_mem_read - read a TSC memory table entry.
 * Symmetric to _pm_mem_write: routes on is_fw_dloaded (LMI before ucode boot,
 * FW mailbox after boot). LMI read sequence : wait DONE==0 -> LMI_CMD_SEQ=cmd(W RITE_EN=0) -> LMI_CMD_SEQ=
 * mem_index -> wait RD_DATA_RDY==1 (LMI_STATUS bit1) -> read 'len' half-words
 * back from LMI_CMD_SEQ (LSB then MSB of each word).
 * ========================================================================== */
static int _pm_mem_read(const plp_aperta_phymod_phy_access_t *phy, uint32_t mem_type, uint32_t mem_index, uint32_t *data)
{
    uint32_t access_prog = 0, len = 0, done = 0, retry_cnt = 100;
    uint32_t lmi_cmd = 0, i, read_count = 0;
    uint32_t is_dloaded = 0;
    int rv;

    rv = _pm_prg_mem_prog_len(mem_type, &access_prog, &len);
    if (rv != PHYMOD_E_NONE || len == 0) {
        return (rv != PHYMOD_E_NONE) ? rv : PHYMOD_E_PARAM;
    }

    /* Once the ucode is booted (is_fw_dloaded==1, i.e. PASS2), memory tables
     * must be read through the firmware mailbox; the raw LMI path does not
     * complete then. */
    _pm_is_fw_dloaded(phy->access.addr, &is_dloaded);
    if (is_dloaded) {
        return _pm_fw_mem_read(phy, mem_type, mem_index, data);
    }

    /* Wait for LMI not busy (DONE==0) */
    retry_cnt = 100;
    do {
        rv = plp_aperta_direct_reg_read(phy, APERTA_LMI_STATUSr, &done);
        if (rv != PHYMOD_E_NONE) { LOG_INFO("LMI rd mem[%d] idx=%d sts rv=%d\n", mem_type, mem_index, rv); return rv; }
        PHYMOD_USLEEP(100);
    } while (((done & 0x1) != 0) && (--retry_cnt));
    if (retry_cnt == 0) {
        LOG_INFO("LMI rd mem[%d] pre-timeout sts=0x%x\n", mem_type, done);
        return PHYMOD_E_TIMEOUT;
    }

    /* LMI_CMD with WRITE_EN=0 (read) */
    lmi_cmd  = ((uint32_t)(APERTA_IS_LINE_SIDE(phy) ? APERTA_LINE_SIDE_PM
                                                    : APERTA_SYS_SIDE_PM) & 0xF) << 8; /* SLAVE_SEL */
    lmi_cmd |= (access_prog & 0x7F) << 1;   /* ACCESS_PROG */
    lmi_cmd |= (0 & 0x7) << 12;             /* PORT_SEL = 0 */
    lmi_cmd |= (0 & 0x1);                   /* WRITE_EN = 0 (read) */
    lmi_cmd |= (0 & 0x1) << 15;             /* USE_CMD_EXT = 0 */
    rv = plp_aperta_direct_reg_write(phy, APERTA_LMI_CMD_SEQr, lmi_cmd);
    if (rv != PHYMOD_E_NONE) { LOG_INFO("LMI rd mem[%d] idx=%d cmd rv=%d\n", mem_type, mem_index, rv); return rv; }

    /* Memory address */
    rv = plp_aperta_direct_reg_write(phy, APERTA_LMI_CMD_SEQr, mem_index);
    if (rv != PHYMOD_E_NONE) { LOG_INFO("LMI rd mem[%d] idx=%d addr rv=%d\n", mem_type, mem_index, rv); return rv; }

    /* Wait for RD_DATA_RDY==1 (LMI_STATUS bit1) */
    retry_cnt = 100;
    do {
        rv = plp_aperta_direct_reg_read(phy, APERTA_LMI_STATUSr, &done);
        if (rv != PHYMOD_E_NONE) { LOG_INFO("LMI rd mem[%d] idx=%d sts2 rv=%d\n", mem_type, mem_index, rv); return rv; }
        PHYMOD_USLEEP(100);
    } while (((done & 0x2) != 0x2) && (--retry_cnt));
    if (retry_cnt == 0) {
        LOG_INFO("LMI rd mem[%d] rd-data-timeout sts=0x%x idx=%d\n", mem_type, done, mem_index);
        return PHYMOD_E_TIMEOUT;
    }

    /* Read exactly 'len' data half-words (LSB then MSB of each word) */
    read_count = 0;
    for (i = 0; read_count < len; i++) {
        uint32_t hw;
        rv = plp_aperta_direct_reg_read(phy, APERTA_LMI_CMD_SEQr, &hw);
        if (rv != PHYMOD_E_NONE) { LOG_INFO("LMI rd mem[%d] idx=%d data[%d] rv=%d\n", mem_type, mem_index, i, rv); return rv; }
        data[i] = hw & 0xFFFF;
        if (++read_count >= len) break;
        rv = plp_aperta_direct_reg_read(phy, APERTA_LMI_CMD_SEQr, &hw);
        if (rv != PHYMOD_E_NONE) { LOG_INFO("LMI rd mem[%d] idx=%d data[%d]hi rv=%d\n", mem_type, mem_index, i, rv); return rv; }
        data[i] |= (hw & 0xFFFF) << 16;
        if (++read_count >= len) break;
    }

    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_speed_id_set - determine spd_intf from (data_rate, num_lane, fec)
 * ========================================================================== */
static int _pm_speed_id_set(uint32_t data_rate, uint32_t num_lane, uint32_t fec, aperta_tbhmod_spd_intfc_type_t *spd_intf)
{
    /* The speed-id entry depends on BOTH data_rate AND num_lane. Selecting the
     * 2-lane 100G RS-528 entry for a 4-lane 100G RS-FEC port programs the
     * wrong FEC/PCS config -> no link. 4-lane 100G + RS-528(CL91) must use
     * SPD_100G_MLD_X4 (PLL DIV_165). */
    if (num_lane == 8) {
        if (data_rate == 400000 &&
            (fec == bcmplpApertaRS544 || fec == bcmplpApertaRS544_2XN)) {
            *spd_intf = APERTA_TBHMOD_SPD_400G_BRCM_FEC_544_2XN_X8;
        } else {
            return PHYMOD_E_UNAVAIL;
        }
    } else if (num_lane == 4) {
        switch (data_rate) {
            case 40000:
                *spd_intf = (fec == bcmplpApertaNoFEC) ?
                            APERTA_TBHMOD_SPD_40G_MLD_X4 :
                            APERTA_TBHMOD_SPD_40G_FEC_BASE_R_KR4_CR4;
                break;
            case 100000:
                if (fec == bcmplpApertaNoFEC) {
                    *spd_intf = APERTA_TBHMOD_SPD_100G_MLD_NO_FEC_X4;
                } else if (fec == bcmplpApertaRSFEC) {   /* CL91 / RS-528 */
                    *spd_intf = APERTA_TBHMOD_SPD_100G_MLD_X4;
                } else if (fec == bcmplpApertaRS544) {
                    *spd_intf = APERTA_TBHMOD_SPD_100G_BRCM_FEC_544_1XN_KR4_CR4;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            case 200000:
                if (fec == bcmplpApertaNoFEC) {
                    *spd_intf = APERTA_TBHMOD_SPD_200G_BRCM_NO_FEC_KR4_CR4;
                } else if (fec == bcmplpApertaRS544) {
                    *spd_intf = APERTA_TBHMOD_SPD_200G_BRCM_FEC_544_1XN_KR4_CR4;
                } else if (fec == bcmplpApertaRS544_2XN) {
                    *spd_intf = APERTA_TBHMOD_SPD_200G_IEEE_FEC_544_2XN_KR4_CR4;
                } else if (fec == bcmplpApertaRS272) {
                    *spd_intf = APERTA_TBHMOD_SPD_200G_BRCM_FEC_272_1XN_KR4_CR4;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            default:
                return PHYMOD_E_UNAVAIL;
        }
    } else if (num_lane == 2) {
        switch (data_rate) {
            case 40000:
                *spd_intf = APERTA_TBHMOD_SPD_40G_MLD_X2;
                break;
            case 50000:
                if (fec == bcmplpApertaNoFEC) {
                    *spd_intf = APERTA_TBHMOD_SPD_50G_MLD_X2;
                } else if (fec == bcmplpApertaRSFEC) {
                    *spd_intf = APERTA_TBHMOD_SPD_50G_MLD_FEC_528_X2;
                } else if (fec == bcmplpApertaRS544) {
                    *spd_intf = APERTA_TBHMOD_SPD_50G_BRCM_FEC_544_CR2_KR2;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            case 100000:
                if (fec == bcmplpApertaNoFEC) {
                    *spd_intf = APERTA_TBHMOD_SPD_100G_BRCM_NOFEC_KR2_CR2;
                } else if (fec == bcmplpApertaRSFEC) {
                    *spd_intf = APERTA_TBHMOD_SPD_100G_BRCM_FEC_528_KR2_CR2;
                } else if (fec == bcmplpApertaRS544) {
                    *spd_intf = APERTA_TBHMOD_SPD_100G_IEEE_KR2_CR2;
                } else if (fec == bcmplpApertaRS272) {
                    *spd_intf = APERTA_TBHMOD_SPD_100G_BRCM_FEC_272_KR2_CR2;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            default:
                return PHYMOD_E_UNAVAIL;
        }
    } else if (num_lane == 1) {
        switch (data_rate) {
            case 10000:
                if (fec == bcmplpApertaNoFEC) {
                    *spd_intf = APERTA_TBHMOD_SPD_10000_XFI;
                } else if (fec == bcmplpApertaBaseR) {   /* CL74 / BASE-R */
                    *spd_intf = APERTA_TBHMOD_SPD_10G_FEC_BASE_R_KR1_CR1;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            case 20000:
                if (fec == bcmplpApertaNoFEC) {
                    *spd_intf = APERTA_TBHMOD_SPD_20000_XFI;
                } else if (fec == bcmplpApertaBaseR) {   /* CL74 / BASE-R */
                    *spd_intf = APERTA_TBHMOD_SPD_20G_FEC_BASE_R_KR1_CR1;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            case 25000:
                if (fec == bcmplpApertaNoFEC) {
                    *spd_intf = APERTA_TBHMOD_SPD_25000_XFI;
                } else if (fec == bcmplpApertaBaseR) {   /* CL74 / BASE-R */
                    *spd_intf = APERTA_TBHMOD_SPD_25G_FEC_BASE_R_KR1_CR1;
                } else if (fec == bcmplpApertaRSFEC) {   /* CL91 / RS-528 */
                    *spd_intf = APERTA_TBHMOD_SPD_25G_FEC_RS_FEC_KR1_CR1;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            case 50000:
                if (fec == bcmplpApertaRSFEC) {          /* CL91 / RS-528 */
                    *spd_intf = APERTA_TBHMOD_SPD_50G_BRCM_FEC_528_CR1_KR1;
                } else if (fec == bcmplpApertaRS544) {
                    *spd_intf = APERTA_TBHMOD_SPD_50G_IEEE_KR1_CR1;
                } else if (fec == bcmplpApertaRS272) {
                    *spd_intf = APERTA_TBHMOD_SPD_50G_BRCM_FEC_272_KR1_CR1;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            default:
                return PHYMOD_E_UNAVAIL;
        }
    } else {
        return PHYMOD_E_UNAVAIL;
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_tscbh_speed_config_set - TSCBH lane-rate programming ethernet path, NRZ:
 *   lane soft reset -> forced speed-id entry (mem_write) -> pll selection
 *   -> OSR mode -> release reset
 * ========================================================================== */
static int _pm_tscbh_speed_config_set(const plp_aperta_phymod_phy_access_t *phy, uint32_t data_rate, uint32_t num_lane,
                                      uint32_t lane_rate, uint32_t fec, uint32_t lane_map)
{
    aperta_tbhmod_spd_intfc_type_t spd_intf = APERTA_TBHMOD_SPD_ZERO;
    int mapped_speed_id = -1;
    uint32_t pll_div = 0;
    uint32_t *spd_id_tbl = NULL;
    uint32_t start_lane = 0, i;
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /* Determine the requested speed-id and PLL divider */
    PHYMOD_IF_ERR_RETURN(_pm_speed_id_set(data_rate, num_lane, fec, &spd_intf));
    PHYMOD_IF_ERR_RETURN(plp_aperta_tbhmod_get_mapped_speed(spd_intf, &mapped_speed_id));
    if (mapped_speed_id < 0 || mapped_speed_id >= APERTA_TSCBH_SPEED_ID_TABLE_SIZE) {
        LOG_INFO("Invalid mapped speed id %d\n", mapped_speed_id);
        return PHYMOD_E_CONFIG;
    }
    PHYMOD_IF_ERR_RETURN(_pm_vco_to_pll_get(lane_rate, &pll_div));

    /* Select the Speed-ID table copy matching the TVCO divider */
    if ((pll_div == APERTA_TBHMOD_PLL_MODE_DIV_170) || (pll_div == APERTA_TBHMOD_PLL_MODE_DIV_85)) {
        spd_id_tbl = &plp_aperta_spd_id_entry_26[0][0];
    } else if ((pll_div == APERTA_TBHMOD_PLL_MODE_DIV_165) || (pll_div == APERTA_TBHMOD_PLL_MODE_DIV_82P5)) {
        spd_id_tbl = &plp_aperta_spd_id_entry_25[0][0];
    } else {
        spd_id_tbl = &plp_aperta_spd_id_entry_20[0][0];
    }

    while (start_lane < 32 && !(lane_map & (1u << start_lane))) {
        start_lane++;
    }

    /* SC_X4_CTL speed-change handshake: tell the SC/micro to use the forced
     * speed-id entry at FORCED_SPEED_ID_OFFSET+start_lane. Without this the
     * micro does not apply the forced speed-id / the lane never runs. */
    for (i = 0; i < num_lane; i++) {
        if (!(lane_map & (1u << (start_lane + i)))) {
            continue;
        }
        phy_copy.access.lane_mask = 1u << (start_lane + i);
        PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_SC_X4_CTL, 0x100, 8, 0)); /* SW_SPEED_CHANGE=0 */
    }
    phy_copy.access.lane_mask = lane_map;
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_SC_X4_CTL, 0x3F, 0, 0));            /* SW_SPEED_ID=0 */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_SC_X4_CTL, 0x100, 8, 0));
    phy_copy.access.lane_mask = 1u << start_lane;
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_SC_X4_CTL, 0x3F, 0,
                                    APERTA_TSCBH_FORCED_SPEED_ID_OFFSET + start_lane));
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_SC_X4_CTL, 0x100, 8, 0));
    phy_copy.access.lane_mask = lane_map;

    /* Hold per-lane PMD soft reset for every lane of the port */
    for (i = 0; i < num_lane; i++) {
        if (!(lane_map & (1u << (start_lane + i)))) {
            continue;
        }
        phy_copy.access.lane_mask = 1u << (start_lane + i);
        PHYMOD_IF_ERR_RETURN(_pm_lane_soft_reset(&phy_copy, 1));
    }

    /* update_port_mode: for a 4-lane
     * port the SC needs MAIN0_SETUP(0x9000) PORT_MODE_SEL = SINGLE_PORT (4),
     * otherwise it doesn't group the lanes as one port and the forced speed
     * never resolves. Reset value of PORT_MODE_SEL is 0. */
    {
        plp_aperta_phymod_phy_access_t pc;
        uint32_t mode_reg, port_mode_sel;
        PHYMOD_MEMCPY(&pc, phy, sizeof(pc));
        pc.access.lane_mask = (lane_map & 0xF) ? 0x1 : 0x10;   /* MMP0 / MMP1 */
        PHYMOD_IF_ERR_RETURN(_pm_bh_rd(&pc, 0x9000, &mode_reg));
        if ((lane_map == 0xF) || (lane_map == 0xF0)) {
            port_mode_sel = 4;   /* cl82_port_mode_SINGLE_PORT */
        } else {
            port_mode_sel = 0;
        }
        PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&pc, 0x9000, 0x70, 4, port_mode_sel));
        LOG_INFO("speed_cfg: update_port_mode MAIN0_SETUP(0x9000)=0x%x PORT_MODE_SEL=%d\n",
                 (unsigned)(mode_reg & 0xFFFF), port_mode_sel);
    }

    /* Program the forced speed-id entry for this port (lane-mapped) */
    phy_copy.access.lane_mask = 1u << start_lane;
    PHYMOD_IF_ERR_RETURN(_pm_mem_write(&phy_copy, APERTA_MEM_TYPE_SPEED_ID,
        APERTA_TSCBH_FORCED_SPEED_ID_OFFSET + start_lane,
        spd_id_tbl + mapped_speed_id * APERTA_TSCBH_SPEED_ID_ENTRY_SIZE));

    /* Pick the PLL index and select it for all lanes. The SDK picks the PLL
     * whose divider matches the request; for the 100G/25.781G case that is
     * the TVCO PLL (index 1). phy->access.tvco_pll_index is 0 (the shared
     * phy_sdk_acc_to_phy memset), so use the constant: selecting PLL0 (20.625G
     * OVCO) instead of PLL1 (25.78G TVCO) leaves the lanes at the wrong rate
     * -> no valid signal -> no link (PCS LIVE stays 0 even after SC resolves).
     * Read back the selection to confirm it landed. */
    for (i = 0; i < num_lane; i++) {
        if (!(lane_map & (1u << (start_lane + i)))) {
            continue;
        }
        phy_copy.access.lane_mask = 1u << (start_lane + i);
        PHYMOD_IF_ERR_RETURN(
            _pm_pll_selection_set(&phy_copy, APERTA_TVCO_PLL_INDEX));
    }
    {
        uint32_t sel = 0;
        phy_copy.access.lane_mask = 1u << start_lane;
        PHYMOD_IF_ERR_RETURN(_pm_bh_rd(&phy_copy, BH_PLL_SELECT, &sel));
        LOG_INFO("speed_cfg: PLL_SELECT(0xd0b7)=0x%x (want PLL1=%d)\n",
                 (unsigned)(sel & 0xFFFF), APERTA_TVCO_PLL_INDEX);
    }

    /* Program OSR mode per lane from the sc_pmd_entry table */
    for (i = 0; i < num_lane; i++) {
        if (!(lane_map & (1u << (start_lane + i)))) {
            continue;
        }
        phy_copy.access.lane_mask = 1u << (start_lane + i);
        PHYMOD_IF_ERR_RETURN(_pm_osr_mode_set(&phy_copy,
            (uint32_t)plp_aperta_tscbh_sc_pmd_entry[mapped_speed_id].t_pma_os_mode));
    }

    /* Program the micro lane-config (0xd1ad = LN_CFG_FWAPI_DATA0 /
     * RX_CKRST_CTRL_TSC_LANE_UC_CONFIG). writes the decoded
     * firmware lane config to the micro for every lane of every speed config
     * (while the lane soft reset is held). Field layout:
     *   bit15 force_nrz_mode, bit14 force_pam4_mode, bit13 lp_has_prec_en,
     *   bit12 force_ns, bit11 force_es, bit10 cl72_restart_timeout_en,
     *   bit9 cl72_auto_polarity_en, bit8 scrambling_dis, bit7 unreliable_los,
     *   bits[6:5] media_type, bit4 force_brdfe_on, bit3 dfe_lp_mode,
     *   bit2 dfe_on, bit1 an_enabled, bit0 lane_cfg_from_pcs.
     * For NRZ 100G KR4 backplane (this build): force_nrz=1 (bit15), dfe_on=1
     * (bit2), media=backplane (bits[6:5]=0), lane_cfg_from_pcs=0 (bit0),
     * an_enabled=0 (bit1) -> 0x8004. Skipping this leaves the micro at the
     * reset default (0x0, lane_cfg_from_pcs=0 but force_nrz_mode=0) -> the
     * micro does not know the lane is NRZ and the SYS SC never completes
     * (RSLVD stays 0). */
    for (i = 0; i < num_lane; i++) {
        if (!(lane_map & (1u << (start_lane + i)))) {
            continue;
        }
        phy_copy.access.lane_mask = 1u << (start_lane + i);
        PHYMOD_IF_ERR_RETURN(_pm_bh_wr(&phy_copy, 0xd1ad, 0x8004));
        {
            uint32_t lc = 0;
            PHYMOD_IF_ERR_RETURN(_pm_bh_rd(&phy_copy, 0xd1ad, &lc));
            LOG_INFO("speed_cfg: lane_uc_cfg(0xd1ad)=0x%x force_nrz=%d dfe=%d "
                     "lane_cfg_from_pcs=%d\n",
                     (unsigned)(lc & 0xFFFF), (int)((lc >> 15) & 1),
                     (int)((lc >> 2) & 1), (int)(lc & 1));
        }
    }

    /* Disable IEEE link training (CL72) per lane */
    for (i = 0; i < num_lane; i++) {
        if (!(lane_map & (1u << (start_lane + i)))) {
            continue;
        }
        phy_copy.access.lane_mask = 1u << (start_lane + i);
        PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, 0x0096, 0x2, 1, 0));
        {
            uint32_t lt = 0;
            PHYMOD_IF_ERR_RETURN(_pm_bh_rd(&phy_copy, 0x0096, &lt));
            LOG_INFO("speed_cfg: link_train_en(0x0096)=0x%x (want bit1=0)\n",
                     (unsigned)(lt & 0xFFFF));
        }
    }

    /* Release the lane soft reset for every lane of the port */
    for (i = 0; i < num_lane; i++) {
        if (!(lane_map & (1u << (start_lane + i)))) {
            continue;
        }
        phy_copy.access.lane_mask = 1u << (start_lane + i);
        PHYMOD_IF_ERR_RETURN(_pm_lane_soft_reset(&phy_copy, 0));
    }
    {
        /* Diagnostic: confirm the lane datapath is OUT of reset (LN_DP_S_RSTB
         * active-low -> 0xd0b1 bit0 must read 1) before the SC trigger. The
         * previous build left it 0 (stuck in reset), which blocked the SYS SC
         * and kept PMD_RX_LOCK/PCS live at 0 on both sides. */
        uint32_t db1 = 0;
        phy_copy.access.lane_mask = 1u << start_lane;
        PHYMOD_IF_ERR_RETURN(_pm_bh_rd(&phy_copy, BH_LANE_SOFT_RESET, &db1));
        LOG_INFO("speed_cfg: lane soft reset released 0xd0b1=0x%x (want bit0=1)\n", (unsigned)(db1 & 0xFFFF));
    }

    /* Trigger the speed change */
    phy_copy.access.lane_mask = 1u << start_lane;
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_SC_X4_CTL, 0x100, 8, 1));   /* SW_SPEED_CHANGE */

    /* Diagnostic + robustness: did the SC apply the forced speed? Read back
     * SC_X4_CTL and poll SC_X4_STS SW_SPEED_CHANGE_DONE / SC_X4_RSLVD_SPD
     * (SDK poll_for_sc_done). RSLVD should equal FORCED_SPEED_ID_OFFSET +
     * start_lane (56 + lane). The SYS-side SC has been observed NOT resolving
     * on the first trigger (intermittent); if so, toggle SW_SPEED_CHANGE 0->1
     * to re-trigger and poll again. */
    {
        plp_aperta_phymod_phy_access_t pc;
        uint32_t sc_ctl = 0, sc_sts = 0, rslvd = 0, cnt2, want;
        PHYMOD_MEMCPY(&pc, phy, sizeof(pc));
        pc.access.lane_mask = 1u << start_lane;
        want = APERTA_TSCBH_FORCED_SPEED_ID_OFFSET + (uint32_t)start_lane;
        plp_aperta_reg32_read(&pc, PHYMOD_REG_APERTA_TSCBH | 0xc050, &sc_ctl);
        for (cnt2 = 0; cnt2 < 500; cnt2++) {   /* up to 5s */
            plp_aperta_reg32_read(&pc, PHYMOD_REG_APERTA_TSCBH | 0xc051, &sc_sts);
            if (sc_sts & 0x1) {
                break;   /* SW_SPEED_CHANGE_DONE (read-clear) */
            }
            PHYMOD_USLEEP(10000);
        }
        plp_aperta_reg32_read(&pc, PHYMOD_REG_APERTA_TSCBH | 0xc070, &rslvd);
        LOG_INFO("speed_cfg: SC_CTL(0xc050)=0x%x CHG=%d ID=%d IGNORE_TX_VLD=%d | "
                 "STS(0xc051)=0x%x DONE=%d VLD=%d | RSLVD(0xc070)=0x%x spd=%d (want 0x%x)\n",
                 (unsigned)(sc_ctl & 0xFFFF), (int)((sc_ctl >> 8) & 1),
                 (int)(sc_ctl & 0x3F), (int)((sc_ctl >> 9) & 1),
                 (unsigned)(sc_sts & 0xFFFF),
                 (int)(sc_sts & 1), (int)((sc_sts >> 1) & 1),
                 (unsigned)(rslvd & 0xFFFF), (int)((rslvd >> 10) & 0x3F),
                 want);

        /* Diagnostic: full side-state dump for BOTH sides right after the SC
         * trigger. 50+ rounds have shown every config register identical
         * between the resolving LINE side and the non-resolving SYS side; dump
         * a broad register set side-by-side to find ANY asymmetric state. */
        {
            static const uint16_t dump_regs[] = {
                0xc050, 0xc051, 0xc070, 0xc054,   /* SC ctl/sts/rslvd + FSM status */
                0x9264,                  /* SC_X1_STS resolved port mode */
                0xc014, 0xc1c0,          /* PMD_X4_OVRR rx-lock ovrd, AN_X4_CL73_CFG */
                0xc111, 0xc133, 0xc160,   /* PCS TX/RX + link */
                0xd0b0, 0xd0b1, 0xd0b7,   /* OSR, lane soft reset, PLL sel */
                0xd0e1, 0xd0e8, 0xd16c,   /* sigdet force, RX sigdet, RX pmd lock */
                0xd1ad, 0xd21a, 0xd148    /* lane uc cfg, micro sts, PLL lock */
            };
            int dside;
            for (dside = phymodPortLocLine; dside <= phymodPortLocSys; dside++) {
                plp_aperta_phymod_phy_access_t dd;
                uint32_t v[sizeof(dump_regs) / sizeof(dump_regs[0])];
                char buf[256];
                int n = 0, ri;
                PHYMOD_MEMCPY(&dd, &pc, sizeof(dd));
                dd.port_loc = (plp_aperta_phymod_port_loc_t)dside;
                dd.access.lane_mask = 1u << start_lane;
                dd.access.pll_idx = 0;
                for (ri = 0; ri < (int)(sizeof(dump_regs) / sizeof(dump_regs[0])); ri++) {
                    if (plp_aperta_reg32_read(&dd, PHYMOD_REG_APERTA_TSCBH | dump_regs[ri], &v[ri]) != PHYMOD_E_NONE) {
                        v[ri] = 0xFFFFFFFF;
                    }
                }
                n += snprintf(buf + n, sizeof(buf) - (size_t)n, "%s:", (dside == phymodPortLocLine) ? "LINE" : "SYS ");
                for (ri = 0; ri < (int)(sizeof(dump_regs) / sizeof(dump_regs[0])); ri++) {
                    n += snprintf(buf + n, sizeof(buf) - (size_t)n, " %04x=%04x",
                                  dump_regs[ri], (unsigned)(v[ri] & 0xFFFF));
                }
                LOG_INFO("speed_cfg: SIDE_DUMP %s\n", buf);
            }
        }

        /* Diagnostic: read back the forced speed-id entry to verify the
         * side's table write actually landed (SYS SC intermittently fails to
         * resolve -> suspicious the SYS entry is missing/empty). */
        {
            uint32_t entry[5] = {0, 0, 0, 0, 0};
            if (_pm_fw_mem_read(&pc, APERTA_MEM_TYPE_SPEED_ID, want, entry)
                == PHYMOD_E_NONE) {
                LOG_INFO("speed_cfg: forced entry[%d] = %08x %08x %08x %08x %08x\n",
                         (int)want, (unsigned)entry[0], (unsigned)entry[1],
                         (unsigned)entry[2], (unsigned)entry[3], (unsigned)entry[4]);
            } else {
                LOG_INFO("speed_cfg: forced entry read FAILED\n");
            }
        }
    }

    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_set_an_timers - AN timer programming 
 * ========================================================================== */
static int _pm_set_an_timers(const plp_aperta_phymod_phy_access_t *phy,
                             uint32_t ref_clock)
{
    uint32_t rd;
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane_mask = 1;   /* MAIN0 / AN_X1 registers are core-level */

    /* Tick control: set 60us tick (156.25MHz: 0xf / 0x249; 312.5MHz: 0xe / 0x493) */
    PHYMOD_IF_ERR_RETURN(_pm_bh_rd(&phy_copy, BH_MAIN0_TICK_CTL0r, &rd));
    rd = (rd & ~0xFFFC) | (1u << 2) | ((ref_clock == 312500000) ? (0xe << 12) : (0xf << 12));
    PHYMOD_IF_ERR_RETURN(_pm_bh_wr(&phy_copy, BH_MAIN0_TICK_CTL0r, rd));

    PHYMOD_IF_ERR_RETURN(_pm_bh_rd(&phy_copy, BH_MAIN0_TICK_CTL1r, &rd));
    rd = (rd & ~0x7FFF) | ((ref_clock == 312500000) ? 0x493 : 0x249);
    rd |= 0x8000;    /* TICK_OVERRIDE */
    PHYMOD_IF_ERR_RETURN(_pm_bh_wr(&phy_copy, BH_MAIN0_TICK_CTL1r, rd));

    /* NRZ AN timers */
    PHYMOD_IF_ERR_RETURN(_pm_bh_wr(&phy_copy, BH_AN_CL73_BRK_LNKr, 0x10f0));           /* 65.04 ms */
    PHYMOD_IF_ERR_RETURN(_pm_bh_wr(&phy_copy, BH_AN_CL73_ERRr, 0x0));                  /* 25.02 ms */
    PHYMOD_IF_ERR_RETURN(_pm_bh_wr(&phy_copy, BH_AN_IGNORE_LNK_TMRr, 0x29c));          /* 10.02 ms */
    PHYMOD_IF_ERR_RETURN(_pm_bh_wr(&phy_copy, BH_AN_LNK_FAIL_INHBT_CL72r, 0x8236));     /* 500.04 ms */
    PHYMOD_IF_ERR_RETURN(_pm_bh_wr(&phy_copy, BH_AN_LNK_FAIL_INHBT_NOT_CL72r, 0x29ab)); /* 160.98 ms */
    PHYMOD_IF_ERR_RETURN(_pm_bh_wr(&phy_copy, BH_RX_X4_RS_FEC_TMRr, 0xfa0));            /* 60 ms */

    /* PAM4 AN timers (default values) */
    PHYMOD_IF_ERR_RETURN(_pm_bh_wr(&phy_copy, BH_AN_IGNORE_LNK_TMR_PAM4r, 0x2a));            /* 10.08 ms */
    PHYMOD_IF_ERR_RETURN(_pm_bh_wr(&phy_copy, BH_AN_LNK_FAIL_INHBT_CL72_PAM4r, 0x3415));     /* 3199.9 ms */
    PHYMOD_IF_ERR_RETURN(_pm_bh_wr(&phy_copy, BH_AN_LNK_FAIL_INHBT_NOT_CL72_PAM4r, 0x106a)); /* 1000.08 ms */

    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_get_lane_config_word - build the 16-bit firmware lane-config word
 * ========================================================================== */
static uint16_t _pm_get_lane_config_word(const plp_aperta_phymod_phy_inf_config_t *config, const aperta_device_aux_modes_t *auxmode)
{
    uint16_t word = 0;

    if (auxmode->modulation_mode == bcmplpApertaModulationNRZ) {
        word |= (1u << 0);      /* force_nrz */
    } else if (auxmode->modulation_mode == bcmplpApertaModulationPAM4) {
        word |= (1u << 1);      /* force_pam4 */
    }
    /* media_type [10:9]: 0=backplane,1=copper,2=fiber */
    if (config->interface_modes & PHYMOD_INTF_MODES_COPPER) {
        word |= (1u << 9);
    } else if (config->interface_modes & PHYMOD_INTF_MODES_FIBER) {
        word |= (2u << 9);
    }
    word |= (1u << 15);         /* lane_cfg_from_pcs */
    return word;
}

/* ==========================================================================
 * _pm_tx_lane_control_release - release TX PMD lane reset + enable.
 * Port of plp_aperta_tbhmod_tx_lane_control(1,0): TX_X4_MISC (0xc111)
 * RSTB_TX_LANE bit1 + ENABLE_TX_LANE bit0.
 * ========================================================================== */
static int _pm_tx_lane_control_release(const plp_aperta_phymod_phy_access_t *phy, uint32_t lane_map)
{
    uint32_t i;
    plp_aperta_phymod_phy_access_t phy_copy;
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    for (i = 0; i < 8; i++) {
        if (!(lane_map & (1u << i))) {
            continue;
        }
        phy_copy.access.lane_mask = 1u << i;
        /* TX lane: RSTB_TX_LANE=1, ENABLE_TX_LANE=1 */
        PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, 0xc111, 0x3, 0, 0x3));
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_rx_lane_control_release - release RX PMD lane reset. RX_X4_PMA_CTL0 (0xc133)
 * RSTB_LANE bit0. Called for BOTH sides before the speed config, which releases RX
 * before the SC and works. The lane datapath soft
 * reset (0xd0b1) is what actually blocked the SYS SC / link-up - fixed in
 * _pm_lane_soft_reset.
 * ========================================================================== */
static int _pm_rx_lane_control_release(const plp_aperta_phymod_phy_access_t *phy,
                                       uint32_t lane_map)
{
    uint32_t i;
    plp_aperta_phymod_phy_access_t phy_copy;
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    for (i = 0; i < 8; i++) {
        if (!(lane_map & (1u << i))) {
            continue;
        }
        phy_copy.access.lane_mask = 1u << i;
        /* RX lane: RSTB_LANE=1 */
        PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, 0xc133, 0x1, 0, 0x1));
    }
    return PHYMOD_E_NONE;
}

static int _pm_cdmac_port_mode_update(const plp_aperta_phymod_phy_access_t *phy, uint32_t lane_map)
{
    uint32_t num_lane = (uint32_t)_pm_count_no_bits(lane_map);
    uint32_t start_lane = 0, mac_stage_id, first_lane_local;
    uint32_t rd, mac_port_mode, mac_new_port_mode = 0;
    uint32_t mac_mode_mask, mac_mode_shift;

    while (start_lane < 32 && !(lane_map & (1u << start_lane))) {
        start_lane++;
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(phy, APERTA_CDPORT_MODEr, &rd));

    mac_stage_id      = start_lane / 4;                 /* 0 = MAC0, 1 = MAC1 */
    first_lane_local  = start_lane % 4;
    mac_mode_mask     = (mac_stage_id) ? 0x38u : 0x7u;  /* MAC1 [5:3] / MAC0 [2:0] */
    mac_mode_shift    = (mac_stage_id) ? 3u : 0u;
    mac_port_mode     = (rd & mac_mode_mask) >> mac_mode_shift;

    if (num_lane == 8) {
        /* 400G single-port mode: both octals together */
        rd &= ~0x7Fu;
        rd |= (1u << 6) | (APERTA_CDMAC_4_LANES_TOGETHER << 3) |
              APERTA_CDMAC_4_LANES_TOGETHER;
    } else if (num_lane == 4) {
        if ((rd >> 6) & 1) {                            /* was 400G single port */
            rd &= ~0x7Fu;                               /* clear 400G + both modes */
        }
        rd = (rd & ~mac_mode_mask) |
             (APERTA_CDMAC_4_LANES_TOGETHER << mac_mode_shift);
    } else if (num_lane == 2) {
        rd &= ~(1u << 6);                               /* clear 400G single mode */
        switch (mac_port_mode) {
            case APERTA_CDMAC_4_LANES_SEPARATE:
                mac_new_port_mode = (first_lane_local == 0) ?
                    APERTA_CDMAC_3_TRI_0_0_2_3 : APERTA_CDMAC_3_TRI_0_1_2_2;
                break;
            case APERTA_CDMAC_3_TRI_0_1_2_2:
                mac_new_port_mode = (first_lane_local == 0) ?
                    APERTA_CDMAC_2_LANES_DUAL : APERTA_CDMAC_3_TRI_0_1_2_2;
                break;
            case APERTA_CDMAC_3_TRI_0_0_2_3:
                mac_new_port_mode = (first_lane_local == 0) ?
                    APERTA_CDMAC_3_TRI_0_0_2_3 : APERTA_CDMAC_2_LANES_DUAL;
                break;
            case APERTA_CDMAC_2_LANES_DUAL:
            case APERTA_CDMAC_4_LANES_TOGETHER:
                mac_new_port_mode = APERTA_CDMAC_2_LANES_DUAL;
                break;
            default:
                break;
        }
        rd = (rd & ~mac_mode_mask) |
             ((mac_new_port_mode & 0x7) << mac_mode_shift);
    } else if (num_lane == 1) {
        rd &= ~(1u << 6);                               /* clear 400G single mode */
        switch (mac_port_mode) {
            case APERTA_CDMAC_4_LANES_SEPARATE:
                mac_new_port_mode = APERTA_CDMAC_4_LANES_SEPARATE;
                break;
            case APERTA_CDMAC_3_TRI_0_1_2_2:
                mac_new_port_mode = ((first_lane_local == 0) || (first_lane_local == 1)) ?
                    APERTA_CDMAC_3_TRI_0_1_2_2 : APERTA_CDMAC_4_LANES_SEPARATE;
                break;
            case APERTA_CDMAC_3_TRI_0_0_2_3:
                mac_new_port_mode = ((first_lane_local == 0) || (first_lane_local == 1)) ?
                    APERTA_CDMAC_4_LANES_SEPARATE : APERTA_CDMAC_3_TRI_0_0_2_3;
                break;
            case APERTA_CDMAC_2_LANES_DUAL:
                mac_new_port_mode = ((first_lane_local == 0) || (first_lane_local == 1)) ?
                    APERTA_CDMAC_3_TRI_0_1_2_2 : APERTA_CDMAC_3_TRI_0_0_2_3;
                break;
            case APERTA_CDMAC_4_LANES_TOGETHER:
                mac_new_port_mode = APERTA_CDMAC_4_LANES_SEPARATE;
                break;
            default:
                break;
        }
        rd = (rd & ~mac_mode_mask) |
             ((mac_new_port_mode & 0x7) << mac_mode_shift);
    }
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(phy, APERTA_CDPORT_MODEr, rd));
    LOG_INFO("speed_cfg: CDPORT_MODE(0x%x)=0x%x single=%d mac1=%d mac0=%d "
             "(num_lane=%u start_lane=%u mac%u)\n",
             APERTA_CDPORT_MODEr, (unsigned)(rd & 0xFFFFFFFF),
             (int)((rd >> 6) & 1), (int)((rd >> 3) & 0x7), (int)(rd & 0x7),
             num_lane, start_lane, mac_stage_id);
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_cdmac_init - program the retimer CDMAC (MAC) for packet forwarding.
 * per-field write-enable "valid" bits live
 * in [16+lsb .. 16+msb] for RX_MAX_SIZE / MIB_COUNTER_CTRL / RX_CTRL /
 * ECC_CTRL / RX_LSS_CTRL / PFC_CTRL / FAULT_LINK_STATUS; TX_CTRL /
 * PAUSE_CTRL / RSV_MASK have NO valid bits (value written directly).
 * ========================================================================== */
static int _pm_cdmac_init(const plp_aperta_phymod_phy_access_t *phy)
{
    uint32_t rd, wr, lo;

    /* 0. RSV_MASK (0x15000120): mask RCV_TERM/CODE_ERR(3)|CRC_ERR(4)|
     * TRUNCATED(6)|RUNT(17) -> 0x20058. No valid bits (MASK [18:0]). */
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(phy, 0x15000120, &rd));
    wr = (rd & ~0x7FFFFu) | 0x20058u;
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(phy, 0x15000120, wr));

    /* 1. RX_MAX_SIZE (0x14000111): [13:0] = 0x3FFF (jumbo). Valid 0x3FFF<<16. */
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(phy, 0x14000111, &rd));
    lo = (rd & 0xFFFFC000u) | 0x3FFFu;
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(phy, 0x14000111, lo | (0x3FFFu << 16)));

    /* 2. MIB_COUNTER_CTRL (0x14000121):
     *   a. CNTMAXSIZE [15:2] = 0x3FFF (valid 0x3FFF<<18)
     *   b. ENABLE[0]=1 + CNT_CLEAR[1]=1 (valid 1<<16 | 1<<17)
     *   c. CNT_CLEAR[1]=0 (ENABLE stays 1) */
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(phy, 0x14000121, &rd));
    lo = (rd & 0x3u) | (0x3FFFu << 2);
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(phy, 0x14000121, lo | (0x3FFFu << 18)));
    lo = (lo & ~0x3u) | 0x3u;
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(phy, 0x14000121, lo | 0x30000u));
    lo = (lo & ~0x2u);
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(phy, 0x14000121, lo | 0x30000u));

    /* 3. RX_CTRL (0x1400010f): STRIP_CRC[2]=1 RX_PASS_PAUSE[14]=0
     * RX_PASS_PFC[15]=0 RX_PASS_CTRL[13]=1 RUNT_THRESHOLD[10:4]=64.
     * Valid 1<<18 | 0x7f<<20 | 1<<29 | 1<<30 | 1<<31. */
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(phy, 0x1400010f, &rd));
    lo = rd & 0xFFFFu;
    lo = (lo & ~(0x1u << 2)) | (0x1u << 2);    /* STRIP_CRC=1 */
    lo = (lo & ~(0x7Fu << 4)) | (64u << 4);   /* RUNT_THRESHOLD=64 */
    lo = (lo & ~(0x1u << 13)) | (0x1u << 13); /* RX_PASS_CTRL=1 */
    lo = (lo & ~(0x1u << 14));                /* RX_PASS_PAUSE=0 */
    lo = (lo & ~(0x1u << 15));                /* RX_PASS_PFC=0 */
    wr = lo | (0x1u << 18) | (0x7Fu << 20) | (0x1u << 29) |
             (0x1u << 30) | (0x1u << 31);
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(phy, 0x1400010f, wr));

    /* 4. TX_CTRL (0x1500010d): CRC_MODE[1:0]=0 DISCARD[2]=0 PAD_EN[4]=0
     * PAD_THRESHOLD[11:5]=64 AVERAGE_IPG[17:12]=12 TX_THRESHOLD[24:20]=1.
     * RECOVER_AM_IDLES[19]=1 must stay set (reset value) - the reference
     * cdmac_init does a RMW and keeps bit19=1 (0x18c808); hardcoding 0x10c808
     * cleared RECOVER_AM_IDLES -> TX does not recover AM idles -> the RS-FEC
     * retimer never forwards (TPKT stays 0). */
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(phy, 0x1500010d, &rd));
    wr = (rd & ~0x1FFFFFFu) | 0x18C808u;
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(phy, 0x1500010d, wr));

    /* 5. ECC_CTRL (0x14020103, gen-access): TX_CDC_ECC_CTRL_EN[0]=1
     * MIB_COUNTER_ECC_CTRL_EN[3]=1. Valid 1<<16 | 1<<19. */
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(phy, 0x14020103, &rd));
    lo = (rd & 0xFFFFu) | 0x1u | (0x1u << 3);
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(phy, 0x14020103, lo | 0x90000u));

    /* 6. RX_LSS_CTRL (0x14000113): remote-fault detect enable (DISABLE[1]=0,
     * DROP_TX_ON_REMOTE_FAULT[5]=1; valid 1<<17|1<<21) then local
     * (DISABLE[0]=0, DROP_TX_ON_LOCAL_FAULT[4]=1; valid 1<<16|1<<20). */
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(phy, 0x14000113, &rd));
    lo = rd & 0xFFFFu;
    lo = (lo & ~(0x1u << 1));                       /* REMOTE_FAULT_DISABLE=0 */
    lo = (lo & ~(0x1u << 5)) | (0x1u << 5);         /* DROP_TX_ON_REMOTE_FAULT=1 */
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(phy, 0x14000113, lo | 0x220000u));
    lo = (lo & ~0x1u);                              /* LOCAL_FAULT_DISABLE=0 */
    lo = (lo & ~(0x1u << 4)) | (0x1u << 4);         /* DROP_TX_ON_LOCAL_FAULT=1 */
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(phy, 0x14000113, lo | 0x110000u));

    /* 7. PFC_CTRL (0x14000116): PFC_STATS_EN[1]=1 FORCE_PFC_XON[0]=0
     * TX_PFC_EN[3]=0 RX_PFC_EN[2]=0. Valid 1<<16|1<<17|1<<18|1<<19. */
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(phy, 0x14000116, &rd));
    lo = (rd & 0xFFFFu) & ~0xFu;                    /* clear bits0-3 */
    lo |= (0x1u << 1);                              /* PFC_STATS_EN=1 */
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(phy, 0x14000116, lo | 0xF0000u));

    /* 8. CDPORT_FAULT_LINK_STATUS (0x10000001): LOCAL_FAULT[0]=1
     * REMOTE_FAULT[1]=1. Valid 1<<16 | 1<<17. */
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(phy, 0x10000001, &rd));
    lo = (rd & 0xFFFFu) | 0x3u;
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(phy, 0x10000001, lo | 0x30000u));

    LOG_INFO("speed_cfg: CDMAC init done (RSV_MASK/RX_MAX/MIB/RX_CTRL/TX_CTRL/ECC/LSS/PFC/FAULT)\n");
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_tx_ctrl_force - force CDMAC TX_CTRL to the reference working value
 * 0x18C808 (TX_THRESHOLD[24:20]=1 + RECOVER_AM_IDLES[19]=1 + CAP_DIC_TO_0=0 +
 * AVERAGE_IPG=12 + PAD_THRESHOLD=64 + DISCARD=0). The reference FW programs
 * 0x18c808 by itself after SC/ENABLE; on ai_app the FW leaves TX_THRESHOLD=0
 * (0xc808) which stops the CDMAC TX (TPKT stays 0). Write + poll: the FW
 * re-writes TX_CTRL a few ms later, so re-assert until it sticks.
 * ========================================================================== */
static int _pm_tx_ctrl_force(const plp_aperta_phymod_phy_access_t *phy)
{
    plp_aperta_phymod_phy_access_t pc;
    uint32_t rd, cnt;
    PHYMOD_MEMCPY(&pc, phy, sizeof(pc));
    for (cnt = 0; cnt < 15; cnt++) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&pc, APERTA_CDMAC_TX_CTRLr, &rd));
        rd = (rd & 0xFE000000u) | 0x18C808u;   /* keep bits[31:25], force low 25 */
        PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(&pc, APERTA_CDMAC_TX_CTRLr, rd));
        PHYMOD_USLEEP(200000);                 /* let any FW overwrite settle */
        PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&pc, APERTA_CDMAC_TX_CTRLr, &rd));
        LOG_INFO("pm_intf_cfg: TX_CTRL force readback=0x%x (try %u)\n", (unsigned)(rd & 0xFFFFFFFF), (unsigned)cnt);
        if ((rd & 0x1FFFFFFu) == 0x18C808u) break;   /* low 25 bits match app */
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_warmboot_speed_set - persist per-lane speed + lane-map into the warmboot
 * SWGPREG registers ( APERTA_SPEED / APERTA_SYS_SPEED): LINE -> SWGPREG11..14 
 * (0x18b41..44), SYS -> SWGPREG1A..1D
 * (0x18b4a..4d), two lanes per 16-bit register. The FW reads these per-lane
 * speed/lane-map values when building the passthrough crossbar;.
 * ========================================================================== */
static int _pm_warmboot_speed_set(const plp_aperta_phymod_phy_access_t *phy, uint32_t data_rate, uint32_t lane_map)
{
    uint32_t lane_num = (uint32_t)_pm_count_no_bits(lane_map);
    uint32_t speed_bits, temp, reg_base, rd;
    int lane;

    switch(data_rate) {
        case 400000:
            speed_bits = 1;
            break;
        case 200000:
            speed_bits = 2;
            break;
        case 100000:
            speed_bits = 3;
            break;
        case 10000:
            speed_bits = 4;
            break;
        case 25000:
            speed_bits = 5;
            break;
        case 20000:
            speed_bits = 6;
            break;
        case 40000:
            speed_bits = 7;
            break;
        case 50000:
            speed_bits = 8;
            break;
        default:
            speed_bits = 1;
            break;
    }

    temp = (lane_num << 4) | (speed_bits & 0xF);

    reg_base = APERTA_IS_LINE_SIDE(phy) ? (0x18b41u | APERTA_DIRECT_BASE) : (0x18b4Au | APERTA_DIRECT_BASE); /* 11 / 1A */

    for (lane = 0; lane < APERTA_PM_NUM_LANES; lane++) {
        if (!(lane_map & (1u << lane))) {
            continue;
        }
        PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, reg_base + (uint32_t)(lane / 2), &rd));
        rd &= ~(0xFFu << ((uint32_t)(lane % 2) * 8));
        rd |= (temp << ((uint32_t)(lane % 2) * 8));
        PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(phy, reg_base + (uint32_t)(lane / 2), rd));
    }
    LOG_INFO("pm_intf_cfg: warmboot speed regs base=0x%x temp=0x%x (lane_num=%u speed_bits=%u)\n", reg_base, temp, lane_num, speed_bits);
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_active_flag_set - set IS_ACTIVE (SWGPREG0E bit1).
 * writes the APERTA_ISACTIVE 
 * var, which also writes SWGPREG0E bit1 via plp_aperta_write_warmboot_reg.
 * The FW reads IS_ACTIVE when building the passthrough crossbar; without it
 * the FW never builds the SYS RX -> LINE TX crossbar (TXFIFO empty, TPKT=0).
 * ========================================================================== */
static int _pm_active_flag_set(const plp_aperta_phymod_phy_access_t *phy, int value)
{
    uint32_t rd;
    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, BCMI_APERTA_D_CTRL_SWGPREG0Er, &rd));
    rd &= ~(1u << 1);
    rd |= ((uint32_t)(value & 1) << 1);
    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(phy, BCMI_APERTA_D_CTRL_SWGPREG0Er, rd));
    LOG_INFO("pm_intf_cfg: IS_ACTIVE set (SWGPREG0E=0x%x)\n", (unsigned)(rd & 0xFFFFu));
    return PHYMOD_E_NONE;
}

/* tbhmod_disable_set / tbhmod_enable_set: SW_SPEED_CHANGE (SC_X4_CTL 0xc050
 * bit8) = 0 / 1, once per lane in lane_mask. */
static int _pm_tbhmod_disable_set(const plp_aperta_phymod_phy_access_t *phy)
{
    int i;
    plp_aperta_phymod_phy_access_t pc;
    PHYMOD_MEMCPY(&pc, phy, sizeof(pc));
    for (i = 0; i < 8; i++) {
        if (!(phy->access.lane_mask & (1u << i))) continue;
        pc.access.lane_mask = 1u << i;
        PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&pc, BH_SC_X4_CTL, 0x100, 8, 0));
    }
    return PHYMOD_E_NONE;
}

static int _pm_tbhmod_enable_set(const plp_aperta_phymod_phy_access_t *phy)
{
    int i;
    plp_aperta_phymod_phy_access_t pc;
    PHYMOD_MEMCPY(&pc, phy, sizeof(pc));
    for (i = 0; i < 8; i++) {
        if (!(phy->access.lane_mask & (1u << i))) continue;
        pc.access.lane_mask = 1u << i;
        PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&pc, BH_SC_X4_CTL, 0x100, 8, 1));
    }
    return PHYMOD_E_NONE;
}

/* Read the PLL divider from AMS_PLL_MODE (0xd147 pll_mode, low 5 bits) */
static int _pm_pll_div_read(const plp_aperta_phymod_phy_access_t *phy, uint32_t *pll_div)
{
    uint32_t reg = 0, pll_mode;
    PHYMOD_IF_ERR_RETURN(_pm_bh_rd(phy, BH_AMS_PLL_MODE, &reg));
    pll_mode = reg & 0x1F;
    switch (pll_mode) {
        case 0:  *pll_div = 64;  break;
        case 1:  *pll_div = 66;  break;
        case 2:  *pll_div = 80;  break;
        case 3:  *pll_div = 128; break;
        case 4:  *pll_div = 132; break;
        case 5:  *pll_div = 140; break;
        case 6:  *pll_div = 160; break;
        case 7:  *pll_div = 165; break;   /* 25G TVCO */
        case 8:  *pll_div = 168; break;
        case 9:  *pll_div = 170; break;   /* 26G TVCO */
        case 10: *pll_div = 175; break;
        case 11: *pll_div = 180; break;
        case 12: *pll_div = 184; break;
        case 13: *pll_div = 200; break;
        case 14: *pll_div = 224; break;
        case 15: *pll_div = 264; break;
        case 16: *pll_div = 96;  break;
        case 17: *pll_div = 120; break;
        case 18: *pll_div = 144; break;
        case 19: *pll_div = 198; break;
        default: *pll_div = APERTA_TBHMOD_PLL_MODE_DIV_165; break;
    }
    return PHYMOD_E_NONE;
}

/* update_port_mode: MAIN0_SETUP (0x9000) PORT_MODE_SEL = SINGLE_PORT(4) for a
 * 4-lane port (SDK plp_aperta_tbhmod_update_port_mode, 4-lane path). */
static int _pm_update_port_mode(const plp_aperta_phymod_phy_access_t *phy, uint32_t lane_map)
{
    plp_aperta_phymod_phy_access_t pc;
    uint32_t mode_reg, port_mode_sel;
    PHYMOD_MEMCPY(&pc, phy, sizeof(pc));
    pc.access.lane_mask = (lane_map & 0xF) ? 0x1 : 0x10;   /* MMP0 / MMP1 */
    PHYMOD_IF_ERR_RETURN(_pm_bh_rd(&pc, 0x9000, &mode_reg));
    if ((lane_map == 0xF) || (lane_map == 0xF0)) {
        port_mode_sel = 4;   /* cl82_port_mode_SINGLE_PORT */
    } else {
        port_mode_sel = 0;
    }
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&pc, 0x9000, 0x70, 4, port_mode_sel));
    return PHYMOD_E_NONE;
}

/* Full flexport SW WAR:
 * re-apply the forced speed-id for the current PLL and re-trigger the SC,
 * with RX lock override protection. */
static int _pm_flexport_sw_workaround(const plp_aperta_phymod_phy_access_t *phy)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, mapped_speed_id;
    uint32_t pll_div = 0, pll_index = 0;
    uint32_t packed_entry[APERTA_TSCBH_SPEED_ID_ENTRY_SIZE] = {0};
    uint32_t lane_map, sts = 0, cnt;
    aperta_tbhmod_spd_intfc_type_t spd_intf = 0;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    lane_map = phy->access.lane_mask;

    /* Hold the PCS lane (SDK tbhmod_disable_set) */
    phy_copy.access.lane_mask = 1u << start_lane;
    PHYMOD_IF_ERR_RETURN(_pm_tbhmod_disable_set(&phy_copy));

    /* get PLL index + PLL div (SDK lane_pll_selection_get + read_pll_div) */
    PHYMOD_IF_ERR_RETURN(_pm_bh_rd(&phy_copy, BH_PLL_SELECT, &pll_index));
    pll_index &= 0x1;
    phy_copy.access.lane_mask = 0x1;
    phy_copy.access.pll_idx = pll_index;
    PHYMOD_IF_ERR_RETURN(_pm_pll_div_read(&phy_copy, &pll_div));

    /* 26G -> 50G, 25G -> 25G, else -> 10G speed-id */
    if ((pll_div == APERTA_TBHMOD_PLL_MODE_DIV_170) ||
        (pll_div == APERTA_TBHMOD_PLL_MODE_DIV_85)) {
        spd_intf = APERTA_TBHMOD_SPD_50G_IEEE_KR1_CR1;
    } else if ((pll_div == APERTA_TBHMOD_PLL_MODE_DIV_165) ||
               (pll_div == APERTA_TBHMOD_PLL_MODE_DIV_82P5)) {
        spd_intf = APERTA_TBHMOD_SPD_25000_XFI;
    } else {
        spd_intf = APERTA_TBHMOD_SPD_10000_XFI;
    }
    PHYMOD_IF_ERR_RETURN(plp_aperta_tbhmod_get_mapped_speed(spd_intf, &mapped_speed_id));
    if (mapped_speed_id < 0 || mapped_speed_id >= APERTA_TSCBH_SPEED_ID_TABLE_SIZE) {
        LOG_INFO("flexport WAR: invalid mapped speed id %d\n", mapped_speed_id);
        return PHYMOD_E_CONFIG;
    }

    /* read the speed-id entry and copy it to the forced speed-id slot */
    phy_copy.access.lane_mask = 1u << 0;
    PHYMOD_IF_ERR_RETURN(_pm_mem_read(&phy_copy, APERTA_MEM_TYPE_SPEED_ID,
                                      (uint32_t)mapped_speed_id, packed_entry));
    PHYMOD_IF_ERR_RETURN(_pm_mem_write(&phy_copy, APERTA_MEM_TYPE_SPEED_ID,
                                       APERTA_TSCBH_FORCED_SPEED_ID_OFFSET + (uint32_t)start_lane,
                                       packed_entry));

    /* update the port mode */
    phy_copy.access.lane_mask = 1u << start_lane;
    PHYMOD_IF_ERR_RETURN(_pm_update_port_mode(&phy_copy, lane_map));

    /* RX lock override on, clear SC FSM/done, enable, poll SC done */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, 0xc014, 0x1, 0, 1));
    { uint32_t tmp; PHYMOD_IF_ERR_RETURN(_pm_bh_rd(&phy_copy, 0xc054, &tmp)); }
    { uint32_t tmp; PHYMOD_IF_ERR_RETURN(_pm_bh_rd(&phy_copy, 0xc051, &tmp)); }
    PHYMOD_IF_ERR_RETURN(_pm_tbhmod_enable_set(&phy_copy));
    for (cnt = 0; cnt < 5000; cnt++) {          /* up to 5s */
        PHYMOD_IF_ERR_RETURN(_pm_bh_rd(&phy_copy, 0xc051, &sts));
        if (sts & 0x1) break;                   /* SW_SPEED_CHANGE_DONE */
        PHYMOD_USLEEP(1000);
    }
    PHYMOD_IF_ERR_RETURN(_pm_tbhmod_disable_set(&phy_copy));
    PHYMOD_USLEEP(10000);
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, 0xc014, 0x1, 0, 0));
    return PHYMOD_E_NONE;
}

/* _pm_pcs_enable_set: 0=disable, 1=enable, else=flexport WAR */
static int _pm_pcs_enable_set(const plp_aperta_phymod_phy_access_t *phy,
                              uint32_t enable)
{
    if (enable == 1) {
        return _pm_tbhmod_enable_set(phy);
    } else if (enable == 0) {
        return _pm_tbhmod_disable_set(phy);
    }
    return _pm_flexport_sw_workaround(phy);
}

/* _pm_get_enabled_port: PORTn_CONFIG bit15 (PRT_ACTIVE) per port */
static int _pm_get_enabled_port(const plp_aperta_phymod_phy_access_t *phy,
                                uint8_t *enabled_port_list)
{
    int port;
    for (port = 0; port < APERTA_MAX_PORT; port++) {
        uint32_t data = 0;
        PHYMOD_IF_ERR_RETURN(
            plp_aperta_direct_reg_read(phy, APERTA_PORTn_CONFIGr((uint32_t)port), &data));
        enabled_port_list[port] = ((data >> 15) & 1) ? 1 : 0;
    }
    return PHYMOD_E_NONE;
}

/* pm_info speed read (SYS or LINE based on phy->port_loc). The PM-info entry
 * stores data_rate | (lane_map << 24), like the SDK. Only the lanes of THIS
 * port are read, and an entry without a stored lane-map (pm_init VCO default,
 * never CONFIG_PORT'd) is not a real config and is skipped. */
static int _pm_pm_info_speed_read(const plp_aperta_phymod_phy_access_t *phy,
                                  uint32_t *speed, uint32_t *lane_map)
{
    unsigned short cnt, lane_index;
    for (cnt = 0; cnt < APERTA_MAX_PM_INFO; cnt++) {
        if (_plp_aperta_pm_info[cnt].phy_id != phy->access.addr) continue;
        for (lane_index = 0; lane_index < APERTA_PM_NUM_LANES; lane_index++) {
            uint32_t data, stored_lm;
            if (!(phy->access.lane_mask & (1u << lane_index))) continue;
            data = (phy->port_loc == phymodPortLocLine) ?
                (uint32_t)_plp_aperta_pm_info[cnt].speed[lane_index] :
                (uint32_t)_plp_aperta_pm_info[cnt].sys_speed[lane_index];
            stored_lm = (data >> 24) & 0xFF;
            if ((data == 0) || (stored_lm == 0)) continue;
            *speed = data & 0xFFFFFF;
            *lane_map = stored_lm;
            return PHYMOD_E_NONE;
        }
        break;
    }
    *speed = 0;
    *lane_map = 0;
    return PHYMOD_E_NONE;
}

/* APERTA_PCS_UPDATE: disable PCS + flexport WAR on SYS then LINE (non-failover). */
static int _pm_pcs_update(const plp_aperta_phymod_phy_access_t *phy,
                          uint32_t line_lane_map)
{
    plp_aperta_phymod_phy_access_t pc;
    PHYMOD_MEMCPY(&pc, phy, sizeof(pc));
    /* SYS side (phy->lane_mask already set by caller to the SYS lane-map) */
    PHYMOD_IF_ERR_RETURN(_pm_pcs_enable_set(&pc, 0));
    PHYMOD_IF_ERR_RETURN(_pm_pcs_enable_set(&pc, 4000));
    /* LINE side */
    pc = *phy;
    pc.access.lane_mask = line_lane_map;
    pc.port_loc = phymodPortLocLine;
    PHYMOD_IF_ERR_RETURN(_pm_pcs_enable_set(&pc, 0));
    PHYMOD_IF_ERR_RETURN(_pm_pcs_enable_set(&pc, 4000));
    return PHYMOD_E_NONE;
}

/* _pm_reset_pcs: restart the PCS datapath on SYS (+LINE) so the
 * PCS applies the new speed/FEC/lane config. Only acts on ports that are
 * currently enabled (PORTn_CONFIG bit15); on first-time bring-up (LINE not
 * enabled yet) it is a no-op, matching the SDK's enabled_port_list check. */
static int _pm_reset_pcs(const plp_aperta_phymod_phy_access_t *phy, const plp_aperta_phymod_phy_inf_config_t *config)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    uint8_t enabled_port_list[APERTA_MAX_PORT];
    int possible_port_list = 0, max_ports = 0, temp;
    uint32_t line_prev_speed = 0, line_prev_lanemap = 0, line_prev_port = 0;
    uint32_t line_prev_no_lanes = 0;
    uint32_t prev_speed = 0, prev_lanemap = 0, prev_port = 0, prev_no_lanes = 0;
    uint32_t unused = 0;
    int port_index = 0;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /* SYS prev speed / port (from PM-info bookkeeping) */
    _pm_pm_info_speed_read(phy, &prev_speed, &prev_lanemap);
    prev_no_lanes = (uint32_t)_pm_count_no_bits(prev_lanemap);
    if (prev_no_lanes == 0) prev_no_lanes = 1;
    _pm_get_port_from_lm_sp(prev_speed, (prev_speed / prev_no_lanes),
                            prev_lanemap, &prev_port, &unused);

    /* LINE prev speed / port */
    phy_copy.port_loc = phymodPortLocLine;
    _pm_pm_info_speed_read(&phy_copy, &line_prev_speed, &line_prev_lanemap);
    line_prev_no_lanes = (uint32_t)_pm_count_no_bits(line_prev_lanemap);
    if (line_prev_no_lanes == 0) line_prev_no_lanes = 1;
    _pm_get_port_from_lm_sp(line_prev_speed, (line_prev_speed / line_prev_no_lanes),
                            line_prev_lanemap, &line_prev_port, &unused);

    /* back to SYS */
    phy_copy.port_loc = phymodPortLocSys;
    PHYMOD_IF_ERR_RETURN(_pm_get_enabled_port(phy, enabled_port_list));

    if ((config->data_rate == APERTA_SPEED_400G) && enabled_port_list[0]) {
        /* 400G single-port: restart PCS on all 8 lanes */
        phy_copy.access.lane_mask = 0xFF;
        PHYMOD_IF_ERR_RETURN(_pm_pcs_update(&phy_copy, 0xFF));
    } else {
        /* Only restart the PCS of a port that really was configured before on
         * THESE lanes (prev_speed != 0). On a fresh multi-port bring-up the
         * pm-init default leaves prev_speed == 0, so an already-up sibling
         * port (e.g. port0 when port1/2/3 are being configured) is NOT
         * restarted and its link is preserved. */
        if ((prev_speed != 0) && enabled_port_list[line_prev_port]) {
            /* SYS lane-map PCS restart */
            phy_copy.access.lane_mask = prev_lanemap;
            PHYMOD_IF_ERR_RETURN(_pm_pcs_update(&phy_copy, line_prev_lanemap));
        }
        /* APERTA_LM_TO_POSSIBLE_PORT_LIST(phy->access.lane_mask, list, max) */
        if (phy->access.lane_mask == 0xF) { possible_port_list = 0x0123; max_ports = 4; }
        else if (phy->access.lane_mask == 0xF0) { possible_port_list = 0x7654; max_ports = 4; }
        else if (phy->access.lane_mask == 0x3) { possible_port_list = 0x01; max_ports = 2; }
        else if (phy->access.lane_mask == 0xC) { possible_port_list = 0x23; max_ports = 2; }
        else if (phy->access.lane_mask == 0x30) { possible_port_list = 0x45; max_ports = 2; }
        else if (phy->access.lane_mask == 0xC0) { possible_port_list = 0x67; max_ports = 2; }
        else if (_pm_count_no_bits(phy->access.lane_mask) == 1) {
            uint32_t lm = phy->access.lane_mask;
            int bi = 0;
            while (lm && !(lm & 1)) { lm >>= 1; bi++; }
            possible_port_list = bi; max_ports = 1;
        } else {
            possible_port_list = 0; max_ports = 0;
        }
        for (port_index = 0; port_index < max_ports; port_index++) {
            temp = (possible_port_list >> (port_index * 4)) & 0xF;
            if (enabled_port_list[temp] && (temp != (int)line_prev_port)) {
                phy_copy.port_loc = phymodPortLocSys;
                phy_copy.access.lane_mask = (uint32_t)(1 << temp);
                line_prev_lanemap = (uint32_t)(1 << temp);
                PHYMOD_IF_ERR_RETURN(_pm_pcs_update(&phy_copy, line_prev_lanemap));
            }
        }
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * plp_aperta_pm_interface_config_set - full PM port config
 * ========================================================================== */
int plp_aperta_pm_interface_config_set(const plp_aperta_phymod_phy_access_t *phy, const plp_aperta_phymod_phy_inf_config_t *config)
{
    aperta_device_aux_modes_t auxmode;
    aperta_config_port_t fw_port_config;
    uint32_t unused = 0;
    uint32_t prev_speed = 0, prev_lanemap = 0, prev_port = 0, prev_no_lanes = 0;
    uint32_t num_lane, lmap, cnt, lane_index;
    uint32_t pll_div = 0, system_port = 0;
    uint16_t lane_cfg_word;

    if (phy == NULL || config == NULL) {
        return PHYMOD_E_PARAM;
    }
    if (config->device_aux_modes == NULL) {
        return PHYMOD_E_PARAM;
    }
    PHYMOD_MEMCPY(&auxmode, config->device_aux_modes, sizeof(auxmode));
    memset(&fw_port_config, 0, sizeof(fw_port_config));

    lmap = phy->access.lane_mask;
    num_lane = (uint32_t)_pm_count_no_bits(lmap);
    if (num_lane == 0) {
        num_lane = 1;
    }

    LOG_INFO("pm_intf_cfg: addr=0x%x side=%d speed=%d lanes=%d lanemap=0x%x "
             "lane_rate=%d fec=%d mod=%d port_type=%d\n",
             phy->access.addr, phy->port_loc, config->data_rate, num_lane, lmap,
             auxmode.lane_data_rate, auxmode.fec_mode_sel,
             auxmode.modulation_mode, auxmode.port_type);


    PHYMOD_IF_ERR_RETURN(_pm_phy_power_on(phy));

    /* ---- Disable phase ------------------------------------------------- */
    PHYMOD_IF_ERR_RETURN(_pm_tx_rx_enable(phy, 0, 0, 0));

    /* ---- SYS side: PCS reset --------------
     * The reference restarts the PCS datapath (disable + flexport SW WAR) on
     * SYS (+LINE) when configuring the SYS side, so the PCS applies the new
     * speed/FEC/lane config. Skipping this left the PCS with stale state on
     * reconfiguration. (First-time bring-up is a no-op: the LINE port is not
     * enabled yet, matching the SDK's enabled_port_list check.) */
    if (APERTA_IS_SYSTEM_SIDE(phy)) {
        /* set PORTn_CONFIG FAULT_OPTION=1 so the LINE-side fault/link-down passes
         * through to SYS -> the switch ce port drops when the far end downs
         * the LINE interface. Must run BEFORE _pm_fill_port_cfg reads bit12
         * into the CONFIG_PORT PortOptions. */
        {
            uint32_t fault_port = 0;
            _pm_get_port_from_lm_sp(config->data_rate, auxmode.lane_data_rate, lmap, &fault_port, &unused);
            PHYMOD_IF_ERR_RETURN(_pm_fault_option_set(phy, fault_port, 1));
        }
        PHYMOD_IF_ERR_RETURN(_pm_reset_pcs(phy, config));
    }

    for (cnt = 0; cnt < APERTA_MAX_PM_INFO; cnt++) {
        if (_plp_aperta_pm_info[cnt].phy_id != phy->access.addr) {
            continue;
        }
        for (lane_index = 0; lane_index < APERTA_PM_NUM_LANES; lane_index++) {
            int *spd;
            uint32_t stored_lm;
            if (!(lmap & (1u << lane_index))) {
                continue;
            }
            spd = APERTA_IS_LINE_SIDE(phy) ?
                  &_plp_aperta_pm_info[cnt].speed[lane_index] :
                  &_plp_aperta_pm_info[cnt].sys_speed[lane_index];
            stored_lm = ((uint32_t)*spd >> 24) & 0xFF;
            if ((*spd == 0) || (stored_lm == 0)) {
                prev_speed = 0;   /* no real previous config on these lanes */
            } else {
                prev_speed   = (uint32_t)*spd & 0xFFFFFF;
                prev_lanemap = (int)stored_lm;
            }
            break;
        }
        break;
    }
    if (prev_speed != 0) {
        prev_no_lanes = _pm_count_no_bits((uint32_t)prev_lanemap);
        if (prev_no_lanes == 0) {
            prev_no_lanes = 1;
        }
        _pm_get_port_from_lm_sp(prev_speed, (prev_speed / prev_no_lanes),
                                (uint32_t)prev_lanemap, &prev_port, &unused);
        {
            uint32_t port_cfg = 0;
            int port_enabled = 0;
            if ((prev_port < APERTA_MAX_PORT) &&
                (plp_aperta_direct_reg_read(phy, APERTA_PORTn_CONFIGr(prev_port), &port_cfg) == PHYMOD_E_NONE)) {
                port_enabled = (port_cfg & (1u << 15)) ? 1 : 0; /* PORT_ACTIVE */
            }
            if (port_enabled) {
                PHYMOD_IF_ERR_RETURN(_pm_fw_port_op(phy, (uint8_t)prev_port, APERTA_FW_PORT_OP_DISABLE));
            } else {
                LOG_INFO("pm_intf_cfg: skip DISABLE prev port %d (not enabled, prev_speed=%d, port_cfg=0x%x)\n",
                         prev_port, prev_speed, port_cfg);
            }
        }
    }

    /* ---- Update PM info speed bookkeeping ----------------------------- */
    for (cnt = 0; cnt < APERTA_MAX_PM_INFO; cnt++) {
        if (_plp_aperta_pm_info[cnt].phy_id != phy->access.addr) {
            continue;
        }
        for (lane_index = 0; lane_index < APERTA_PM_NUM_LANES; lane_index++) {
            if (!(lmap & (1u << lane_index))) {
                continue;
            }
            if (APERTA_IS_LINE_SIDE(phy)) {
                _plp_aperta_pm_info[cnt].speed[lane_index] = (int)config->data_rate | (int)(lmap << 24);
            } else {
                _plp_aperta_pm_info[cnt].sys_speed[lane_index] = (int)config->data_rate | (int)(lmap << 24);
            }
        }
        break;
    }

    /* Persist per-lane speed + lane-map into the SWGPREG registers */
    PHYMOD_IF_ERR_RETURN(_pm_warmboot_speed_set(phy, (uint32_t)config->data_rate, lmap));

    /* ---- Finish the disable hold
     * the CDMAC datapath in SOFT_RESET / TX_EN=0 on both sides while the SC
     * applies the new speed. */
    PHYMOD_IF_ERR_RETURN(_pm_tx_rx_enable_post(phy, 0));

    /* ---- VCO / PLL reconfiguration ------------------------------------ */
    if (_pm_vco_to_pll_get(auxmode.lane_data_rate, &pll_div) != PHYMOD_E_NONE) {
        /* PAM4 rates are not yet in the NRZ VCO table; skip PLL reconfig */
        LOG_INFO("No VCO->PLL mapping for lane_rate=%d (PAM4 path TBD)\n", auxmode.lane_data_rate);
        pll_div = APERTA_TBHMOD_PLL_MODE_DIV_165;
    }
    PHYMOD_IF_ERR_RETURN(_pm_pll_reconfig(phy, pll_div));

    /* ---- Line side: CONFIG_PORT FW msg / Sys side: system lane count ---- */
    if (APERTA_IS_LINE_SIDE(phy)) {
        PHYMOD_IF_ERR_RETURN(_pm_fill_port_cfg(phy, config, &auxmode, &fw_port_config));
        PHYMOD_IF_ERR_RETURN(_pm_fw_config_port_set(phy, &fw_port_config));
    } else {
        _pm_get_port_from_lm_sp(config->data_rate, auxmode.lane_data_rate, lmap, &system_port, &unused);
        system_port = (system_port << 4) | (num_lane & 0xF);
        system_port |= (lmap << 8);
        PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(phy, APERTA_SYSTEM_SIDE_NO_OF_LANES, system_port));
    }

    /* ---- Release TX/RX PMD lane reset + enable (SDK phy_init step) ------
     * The SDK port_attach calls f_phymod_phy_init (tbhmod_tx_lane_control(1,0)
     * + rx_lane_control(1)) BEFORE the speed config. If the TX/RX lanes stay
     * in reset while the SC applies the forced speed, the SC cannot complete.
     * Both sides release TX and RX identically, matching the SDK.
     *
     * (The SYS SC non-resolution seen in earlier rounds was caused by the lane
     * datapath soft reset 0xd0b1 staying asserted after every speed config -
     * now fixed in _pm_lane_soft_reset - NOT by the SYS RX incoming signal.
     * Holding the SYS RX in reset during the SC (previous round) is reverted
     * to match the SDK, which releases RX before the SC and works.) */
    PHYMOD_IF_ERR_RETURN(_pm_tx_lane_control_release(phy, lmap));
    PHYMOD_IF_ERR_RETURN(_pm_rx_lane_control_release(phy, lmap));

    /* ---- CDMAC port mode (4 lanes = one port, SDK port_mode_update) ----- */
    PHYMOD_IF_ERR_RETURN(_pm_cdmac_port_mode_update(phy, lmap));

    /* ---- CDMAC init : RSV_MASK /
     * RX_CTRL / TX_CTRL / ECC / LSS / PFC / FAULT / MIB. The reference runs
     * this whenever the current CDMAC speed != requested speed; without it
     * the MAC has no RX/TX frame path (ce0 RPKT stuck at 0). */
    PHYMOD_IF_ERR_RETURN(_pm_cdmac_init(phy));

    /* ---- Deep TSCBH lane-rate programming ------------------------------ */
    PHYMOD_IF_ERR_RETURN(_pm_tscbh_speed_config_set(phy, config->data_rate, num_lane,
                                   auxmode.lane_data_rate, auxmode.fec_mode_sel, lmap));

    /* ---- Line side: active + ENABLE_PORT + TX/RX enable ----------------- */
    if (APERTA_IS_LINE_SIDE(phy)) {
        uint32_t line_port = 0;
        _pm_get_port_from_lm_sp(config->data_rate, auxmode.lane_data_rate, lmap, &line_port, &unused);
        PHYMOD_IF_ERR_RETURN(_pm_port_active_set(phy, line_port, 1));
        PHYMOD_IF_ERR_RETURN(_pm_fw_port_op(phy, (uint8_t)line_port, APERTA_FW_PORT_OP_ENABLE));
        PHYMOD_IF_ERR_RETURN(_pm_tx_rx_enable(phy, 1, 0, 0));
    }

    /* Diagnostic: did the TVCO PLL1 lock after the speed config (incl. the
     * SC_X4_CTL speed handshake) and port enable? Up to 1s. */
    {
        plp_aperta_phymod_phy_access_t pc;
        uint32_t pll_lock = 0, raw = 0, cnt2;
        PHYMOD_MEMCPY(&pc, phy, sizeof(pc));
        pc.access.lane_mask = 1;
        pc.access.pll_idx = APERTA_TVCO_PLL_INDEX;
        for (cnt2 = 0; cnt2 < 100; cnt2++) {
            PHYMOD_IF_ERR_RETURN(_pm_pll_lock_get(&pc, &pll_lock));
            if (pll_lock) {
                break;
            }
            PHYMOD_USLEEP(10000);
        }
        plp_aperta_reg32_read(&pc, PHYMOD_REG_APERTA_TSCBH | 0xd148, &raw);
        LOG_INFO("pm_intf_cfg: PLL1 lock=%d after %u x10ms raw_d148=0x%x\n", pll_lock, cnt2, (unsigned)(raw & 0xFFFF));
    }

    /* Diagnostic: PCS link status (RX_X4_PCS_LATCH_STS1 = 0xc160). bit10 =
     * PCS_LINK_STATUS_LIVE, bit2 = LINK_STATUS_LL (latched low, clear on
     * read). Link up iff LIVE==1 && LL==0. */
    {
        plp_aperta_phymod_phy_access_t pc;
        uint32_t latch = 0;
        PHYMOD_MEMCPY(&pc, phy, sizeof(pc));
        plp_aperta_reg32_read(&pc, PHYMOD_REG_APERTA_TSCBH | 0xc160, &latch);
        LOG_INFO("pm_intf_cfg: PCS_LATCH_STS1(0xc160)=0x%x LIVE=%d LL=%d -> %s\n",
                 (unsigned)(latch & 0xFFFF), (int)((latch >> 10) & 1),
                 (int)((latch >> 2) & 1),
                 (((latch >> 10) & 1) && !((latch >> 2) & 1))
                     ? "LINK UP" : "no link");
    }

    /* Force CDMAC TX_CTRL to the reference working value 0x18c808
     * (TX_THRESHOLD=1 + RECOVER_AM_IDLES=1). The reference FW programs this
     * after SC/ENABLE; the FW leaves TX_THRESHOLD=0 which stops the
     * CDMAC TX (TPKT stays 0). Run AFTER the LINE-side ENABLE + tx_rx_enable
     * so any FW TX_CTRL write is overridden. */
    PHYMOD_IF_ERR_RETURN(_pm_tx_ctrl_force(phy));

    /* Log the lane-config word used for the FW lane config (informational) */
    lane_cfg_word = _pm_get_lane_config_word(config, &auxmode);
    LOG_INFO("pm_intf_cfg done: lane_cfg_word=0x%04x pll_div=0x%x\n", lane_cfg_word, pll_div);

    return PHYMOD_E_NONE;
}

/* TVCO rate markers */
#define APERTA_TSCBH_VCO_NONE       0x0
#define APERTA_TSCBH_VCO_20G        0x1
#define APERTA_TSCBH_VCO_25G        0x2
#define APERTA_TSCBH_VCO_26G        0x4

/* Micro RAM access registers */
#define BH_UC_CTRL                  0xd202 /* [13] autoinc_rd [12] autoinc_wr [5:4] rd_datasize [1:0] wr_datasize */
#define BH_UC_WRADDR_LSW            0xd204
#define BH_UC_WRADDR_MSW            0xd205
#define BH_UC_WRDATA_LSW            0xd206
#define BH_UC_RDADDR_LSW            0xd208
#define BH_UC_RDADDR_MSW            0xd209
#define BH_UC_RDDATA_LSW            0xd20a

/* uC info table in program RAM */
#define APERTA_UC_INFO_TABLE_BASE   0x100u
#define APERTA_UC_INFO_CORE_MEM_PTR 0x14u

/* micro_clk_source_select uses uc var 0x11 (misc_ctrl_byte) bit2 */
#define APERTA_UC_VAR_MISC_CTRL     0x11u

/* ==========================================================================
 * _pm_tscbh_pll_to_vco_get - PLL divider -> TVCO rate
 * ========================================================================== */
static int _pm_tscbh_pll_to_vco_get(uint32_t ref_clock, uint32_t pll, uint32_t *vco)
{
    if (ref_clock == 0) {                  /* phymodRefClk156Mhz */
        switch (pll) {
            case APERTA_TBHMOD_PLL_MODE_DIV_170: *vco = APERTA_TSCBH_VCO_26G; break;
            case APERTA_TBHMOD_PLL_MODE_DIV_165: *vco = APERTA_TSCBH_VCO_25G; break;
            case APERTA_TBHMOD_PLL_MODE_DIV_132: *vco = APERTA_TSCBH_VCO_20G; break;
            case APERTA_TSCBH_PLL_DIVNONE:       *vco = APERTA_TSCBH_VCO_NONE; break;
            default:                             *vco = APERTA_TSCBH_VCO_NONE; break;
        }
    } else if (ref_clock == 1) {             /* phymodRefClk312Mhz */
        switch (pll) {
            case APERTA_TBHMOD_PLL_MODE_DIV_85:   *vco = APERTA_TSCBH_VCO_26G; break;
            case APERTA_TBHMOD_PLL_MODE_DIV_82P5: *vco = APERTA_TSCBH_VCO_25G; break;
            case APERTA_TBHMOD_PLL_MODE_DIV_66:   *vco = APERTA_TSCBH_VCO_20G; break;
            case APERTA_TSCBH_PLL_DIVNONE:        *vco = APERTA_TSCBH_VCO_NONE; break;
            default:                              *vco = APERTA_TSCBH_VCO_NONE; break;
        }
    } else {
        return PHYMOD_E_UNAVAIL;
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * uC program/data RAM access through the micro_ra register interface
 * ========================================================================== */

/* Read a 32-bit value from uC program RAM (msw=0) or data RAM (msw=0x2000),
 * following the two-read auto-increment pattern of rd_long_uc_*. */
static int _pm_uc_ram_rd32(const plp_aperta_phymod_phy_access_t *phy, uint16_t addr_msw, uint16_t addr_lsw, uint32_t *val)
{
    uint32_t lo, hi;
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(phy, BH_UC_CTRL, 0x2000, 13, 1));  /* autoinc_rdaddr_en */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(phy, BH_UC_CTRL, 0x0030, 4, 0x1)); /* rd_datasize=16bit */
    PHYMOD_IF_ERR_RETURN(_pm_bh_wr(phy, BH_UC_RDADDR_MSW, addr_msw));
    PHYMOD_IF_ERR_RETURN(_pm_bh_wr(phy, BH_UC_RDADDR_LSW, addr_lsw & 0xFFFE));
    PHYMOD_IF_ERR_RETURN(_pm_bh_rd(phy, BH_UC_RDDATA_LSW, &lo));
    PHYMOD_IF_ERR_RETURN(_pm_bh_rd(phy, BH_UC_RDDATA_LSW, &hi));        /* auto-incremented word */
    *val = ((hi & 0xFFFF) << 16) | (lo & 0xFFFF);
    return PHYMOD_E_NONE;
}

/* uC core variable byte write 
 * core_var_ram_base is read from the uC info table (program RAM 0x114). */
static int _pm_uc_var_wrb(const plp_aperta_phymod_phy_access_t *phy,
                          uint8_t addr, uint8_t val)
{
    uint32_t core_var_ram_base;
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane_mask = 1;   /* core-level uc var */

    PHYMOD_IF_ERR_RETURN(_pm_uc_ram_rd32(&phy_copy, 0,
        APERTA_UC_INFO_TABLE_BASE + APERTA_UC_INFO_CORE_MEM_PTR,
        &core_var_ram_base));

    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_UC_CTRL, 0x1000, 12, 0)); /* autoinc_wraddr_en=0 */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_UC_CTRL, 0x0003, 0, 0x0)); /* wr_datasize=8bit */
    PHYMOD_IF_ERR_RETURN(_pm_bh_wr(&phy_copy, BH_UC_WRADDR_MSW, 0x2000));    /* uc data RAM */
    PHYMOD_IF_ERR_RETURN(_pm_bh_wr(&phy_copy, BH_UC_WRADDR_LSW,
        (uint16_t)((core_var_ram_base + addr) & 0xFFFF)));
    PHYMOD_IF_ERR_RETURN(_pm_bh_wr(&phy_copy, BH_UC_WRDATA_LSW, val & 0xFF));
    return PHYMOD_E_NONE;
}

/* uC core variable byte read */
static int _pm_uc_var_rdb(const plp_aperta_phymod_phy_access_t *phy, uint8_t addr, uint8_t *val)
{
    uint32_t core_var_ram_base, rd;
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane_mask = 1;

    PHYMOD_IF_ERR_RETURN(_pm_uc_ram_rd32(&phy_copy, 0,
        APERTA_UC_INFO_TABLE_BASE + APERTA_UC_INFO_CORE_MEM_PTR,
        &core_var_ram_base));

    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_UC_CTRL, 0x2000, 13, 1)); /* autoinc_rdaddr_en */
    PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy_copy, BH_UC_CTRL, 0x0030, 4, 0x0)); /* rd_datasize=8bit */
    PHYMOD_IF_ERR_RETURN(_pm_bh_wr(&phy_copy, BH_UC_RDADDR_MSW, 0x2000));
    PHYMOD_IF_ERR_RETURN(_pm_bh_wr(&phy_copy, BH_UC_RDADDR_LSW,
        (uint16_t)((core_var_ram_base + addr) & 0xFFFF)));
    PHYMOD_IF_ERR_RETURN(_pm_bh_rd(&phy_copy, BH_UC_RDDATA_LSW, &rd));
    *val = (uint8_t)(rd & 0xFF);
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_micro_clk_source_select - PMD micro clock source -> TVCO PLL
 * ========================================================================== */
static int _pm_micro_clk_source_select(const plp_aperta_phymod_phy_access_t *phy, uint32_t pll_index)
{
    uint8_t misc_ctrl;
    PHYMOD_IF_ERR_RETURN(_pm_uc_var_rdb(phy, APERTA_UC_VAR_MISC_CTRL, &misc_ctrl));
    misc_ctrl = (uint8_t)((misc_ctrl & ~0x4) | ((pll_index & 1) << 2));
    PHYMOD_IF_ERR_RETURN(_pm_uc_var_wrb(phy, APERTA_UC_VAR_MISC_CTRL, misc_ctrl));
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * plp_aperta_tscbh_core_init_pass1_self - AM / UM / SpeedPriorityMap upload
 * ========================================================================== */
int plp_aperta_tscbh_core_init_pass1_self(
    const plp_aperta_phymod_phy_access_t *core)
{
    int rv;
    uint32_t i;
    int side;
    plp_aperta_phymod_phy_access_t phy;

    PHYMOD_MEMCPY(&phy, core, sizeof(phy));

    for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
        phy.port_loc = (plp_aperta_phymod_port_loc_t)side;
        phy.access.lane_mask = 0x1;

        LOG_INFO("Uploading AM table phy:%d side:%s\n", phy.access.addr, (side == phymodPortLocLine) ? "LINE" : "SYS");
        for (i = 0; i < APERTA_TSCBH_AM_TABLE_SIZE; i++) {
            rv = _pm_mem_write(&phy, APERTA_MEM_TYPE_AM_TABLE, i, &plp_aperta_am_table_entry[i][0]);
            if (rv != PHYMOD_E_NONE) {
                LOG_INFO("AM table idx=%d upload failed rv=%d\n", i, rv);
                return rv;
            }
        }

        LOG_INFO("Uploading UM table phy:%d side:%s\n", phy.access.addr, (side == phymodPortLocLine) ? "LINE" : "SYS");
        for (i = 0; i < APERTA_TSCBH_UM_TABLE_SIZE; i++) {
            rv = _pm_mem_write(&phy, APERTA_MEM_TYPE_UM_TABLE, i, &plp_aperta_um_table_entry[i][0]);
            if (rv != PHYMOD_E_NONE) {
                LOG_INFO("UM table idx=%d upload failed rv=%d\n", i, rv);
                return rv;
            }
        }

        /* Speed priority mapping table (single entry) */
        LOG_INFO("Uploading SpeedPriority table phy:%d side:%s\n", phy.access.addr,
                 (side == phymodPortLocLine) ? "LINE" : "SYS");
        rv = _pm_mem_write(&phy, APERTA_MEM_TYPE_SPEED_PRIORITY, 0, &plp_aperta_speed_priority_mapping_table[0][0]);
        LOG_INFO("SpeedPriority upload rv=%d\n", rv);
        if (rv != PHYMOD_E_NONE) {
            return rv;
        }
    }

    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * plp_aperta_tscbh_core_init_pass2_self - SpeedID upload + PLL/AFE/micro clk
 * ========================================================================== */
int plp_aperta_tscbh_core_init_pass2_self(
    const plp_aperta_phymod_phy_access_t *core,
    const plp_aperta_phymod_core_init_config_t *init_config)
{
    uint32_t tvco_rate = 0, speed_id_load_size, i, lane;
    uint32_t tvco_pll_index;
    uint32_t ref_clock_hz;
    uint32_t *spd_id_tbl;
    plp_aperta_phymod_phy_access_t phy;

    PHYMOD_MEMCPY(&phy, core, sizeof(phy));
    phy.access.lane_mask = 0x1;
    tvco_pll_index = core->access.tvco_pll_index;
    LOG_INFO("pass2_self entry phy=0x%x pll0=%x pll1=%x refclk=%d tvco_pll=%d\n",
             core->access.addr,
             init_config->pll0_div_init_value,
             init_config->pll1_div_init_value,
             init_config->interface.ref_clock,
             tvco_pll_index);

    /* 1. Hold both PLLs in core DP reset */
    phy.access.pll_idx = 1;
    PHYMOD_IF_ERR_RETURN(_pm_core_dp_reset(&phy, 1));
    LOG_INFO("pass2_self pll1 dp_reset done\n");
    phy.access.pll_idx = 0;
    PHYMOD_IF_ERR_RETURN(_pm_core_dp_reset(&phy, 1));
    LOG_INFO("pass2_self pll0 dp_reset done\n");

    /* 2. Resolve the TVCO rate from the configured PLL init div value */
    PHYMOD_IF_ERR_RETURN(_pm_tscbh_pll_to_vco_get(
        init_config->interface.ref_clock,
        (tvco_pll_index == 1) ? init_config->pll1_div_init_value
                              : init_config->pll0_div_init_value,
        &tvco_rate));
    LOG_INFO("pass2_self tvco_rate=%d\n", tvco_rate);

    /* 3. Upload the Speed-ID table matching the TVCO */
    LOG_INFO("Uploading Speed ID table\n");
    speed_id_load_size = APERTA_TSCBH_SPEED_ID_TABLE_SIZE >
                         APERTA_TSCBH_HW_SPEED_ID_TABLE_SIZE ?
                         APERTA_TSCBH_HW_SPEED_ID_TABLE_SIZE :
                         APERTA_TSCBH_SPEED_ID_TABLE_SIZE;
    if (tvco_rate == APERTA_TSCBH_VCO_26G) {
        spd_id_tbl = &plp_aperta_spd_id_entry_26[0][0];
    } else if (tvco_rate == APERTA_TSCBH_VCO_25G) {
        spd_id_tbl = &plp_aperta_spd_id_entry_25[0][0];
    } else {
        spd_id_tbl = &plp_aperta_spd_id_entry_20[0][0];
    }

    plp_aperta_phymod_phy_access_t sp;
    PHYMOD_MEMCPY(&sp, &phy, sizeof(sp));
    /* LINE-side speed-id table */
    sp.port_loc = phymodPortLocLine;
    for (i = 0; i < speed_id_load_size; i++) {
        PHYMOD_IF_ERR_RETURN(_pm_mem_write(&phy, APERTA_MEM_TYPE_SPEED_ID, i,
            spd_id_tbl + i * APERTA_TSCBH_SPEED_ID_ENTRY_SIZE));
    }

    /* SYS-side speed-id table */
    sp.port_loc = phymodPortLocSys;
    for (i = 0; i < speed_id_load_size; i++) {
        PHYMOD_IF_ERR_RETURN(_pm_mem_write(&sp, APERTA_MEM_TYPE_SPEED_ID, i,
            spd_id_tbl + i * APERTA_TSCBH_SPEED_ID_ENTRY_SIZE));
    }

    /* 4. PMD lane hard-reset per-lane workaround */
    for (lane = 0; lane < APERTA_TSCBH_NOF_LANES_IN_CORE; lane++) {
        phy.access.lane_mask = 1u << lane;
        PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy, 0xd0b3, 0x1, 0, 0x1));
    }
    PHYMOD_USLEEP(10000);
    for (lane = 0; lane < APERTA_TSCBH_NOF_LANES_IN_CORE; lane++) {
        phy.access.lane_mask = 1u << lane;
        PHYMOD_IF_ERR_RETURN(_pm_bh_mwr(&phy, 0xd0b3, 0x1, 0, 0x0));
    }

    /* 5. PLL divider configuration (AFE pll reg-set is a no-op by default:
     *      afe_pll_change_default is 0 unless the config changes it). */
    LOG_INFO("Configuring PLL 0:%x 1:%x\n", init_config->pll0_div_init_value, init_config->pll1_div_init_value);
    phy.access.lane_mask = 0x1;
    if (init_config->pll0_div_init_value != APERTA_TSCBH_PLL_DIVNONE) {
        phy.access.pll_idx = 0;
        PHYMOD_IF_ERR_RETURN(_pm_pll_program(&phy, init_config->pll0_div_init_value));
    }
    if (init_config->pll1_div_init_value != APERTA_TSCBH_PLL_DIVNONE) {
        phy.access.pll_idx = 1;
        PHYMOD_IF_ERR_RETURN(_pm_pll_program(&phy, init_config->pll1_div_init_value));
    }

    /* 5b. SYS-side PLLs: the SDK core-init runs PASS2 once per side and
     * each side has its OWN PLL0/PLL1. Only LINE was programmed before -> SYS
     * PLL never locked (SYS 0xd148=0x0) -> SYS SC has no PLL clock -> cannot
     * resolve the forced speed (RSLVD=0) -> no SYS TX signal -> switch cannot
     * lock -> ce0 down. */
    {
        plp_aperta_phymod_phy_access_t sp;
        PHYMOD_MEMCPY(&sp, &phy, sizeof(sp));
        sp.port_loc = phymodPortLocSys;
        sp.access.lane_mask = 0x1;
        sp.access.pll_idx = 1;
        PHYMOD_IF_ERR_RETURN(_pm_core_dp_reset(&sp, 1));
        sp.access.pll_idx = 0;
        PHYMOD_IF_ERR_RETURN(_pm_core_dp_reset(&sp, 1));
        if (init_config->pll0_div_init_value != APERTA_TSCBH_PLL_DIVNONE) {
            sp.access.pll_idx = 0;
            PHYMOD_IF_ERR_RETURN(_pm_pll_program(&sp, init_config->pll0_div_init_value));
        }
        if (init_config->pll1_div_init_value != APERTA_TSCBH_PLL_DIVNONE) {
            sp.access.pll_idx = 1;
            PHYMOD_IF_ERR_RETURN(_pm_pll_program(&sp, init_config->pll1_div_init_value));
        }
        /* Micro clock source for the SYS side too (SDK pass2 runs per side and
         * each side has its own uc var / SC; without this the SYS SC may not
         * resolve the forced speed reliably -> SYS TX invalid -> no link). */
        sp.access.lane_mask = 0x1;
        PHYMOD_IF_ERR_RETURN(_pm_micro_clk_source_select(&sp, tvco_pll_index));
        sp.access.pll_idx = 1;
        PHYMOD_IF_ERR_RETURN(_pm_core_dp_reset(&sp, 0));
        sp.access.pll_idx = 0;
        PHYMOD_IF_ERR_RETURN(_pm_core_dp_reset(&sp, 0));
    }

    /* 6. AN timers for MMP0 (lanes 0-3) and MMP1 (lanes 4-7) */
    ref_clock_hz = (init_config->interface.ref_clock == 1) ? 312500000u : 156250000u;
    phy.access.pll_idx = 0;
    phy.access.lane_mask = 0x1;
    PHYMOD_IF_ERR_RETURN(_pm_set_an_timers(&phy, ref_clock_hz));
    phy.access.lane_mask = 0x10;
    PHYMOD_IF_ERR_RETURN(_pm_set_an_timers(&phy, ref_clock_hz));

    /* 7. Micro clock source -> TVCO PLL */
    phy.access.lane_mask = 0x1;
    PHYMOD_IF_ERR_RETURN(_pm_micro_clk_source_select(&phy, tvco_pll_index));

    /* 8. Release core DP soft reset for both PLLs */
    phy.access.pll_idx = 1;
    PHYMOD_IF_ERR_RETURN(_pm_core_dp_reset(&phy, 0));
    phy.access.pll_idx = 0;
    PHYMOD_IF_ERR_RETURN(_pm_core_dp_reset(&phy, 0));

    return PHYMOD_E_NONE;
}
