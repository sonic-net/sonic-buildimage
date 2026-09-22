/********************************************************************************
 * Copyright(C) 2026 Micas Network. All rights reserved.
 ********************************************************************************
 * PHY SDK Implementation
 ********************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include "aperta_phy_sdk.h"
#include "aperta_tbhmod_defs.h"
#include "mdio.h"
#include "common.h"

/* ==========================================================================
 * Global bus function pointers (set by phy_sdk_set_bus_funcs)
 * ========================================================================== */
static phy_sdk_bus_read_t  g_bus_read  = NULL;
static phy_sdk_bus_write_t g_bus_write = NULL;

void phy_sdk_set_bus_funcs(phy_sdk_bus_read_t read_fn, phy_sdk_bus_write_t write_fn)
{
    g_bus_read  = read_fn;
    g_bus_write = write_fn;
}

/* ==========================================================================
 * Internal bus read/write
 * Extracts phy_addr from the access struct, calls the registered callbacks.
 * ========================================================================== */
int _phy_sdk_bus_read(const plp_aperta_phymod_access_t *pa, uint32_t reg, uint32_t *data)
{
    if (g_bus_read && pa) {
        return g_bus_read(pa->user_acc, pa->addr, reg, data);
    }
    return -1;
}

int _phy_sdk_bus_write(const plp_aperta_phymod_access_t *pa, uint32_t reg, uint32_t data)
{
    if (g_bus_write && pa) {
        return g_bus_write(pa->user_acc, pa->addr, reg, data);
    }
    return -1;
}

/* ==========================================================================
 * External globals
 * ========================================================================== */
extern aperta_pm_info_t _plp_aperta_pm_info[APERTA_MAX_PM_INFO];

/* ==========================================================================
 * Type conversion - phy_sdk_access_t -> plp_aperta_phymod_phy_access_t
 * ========================================================================== */
void phy_sdk_acc_to_phy(const phy_sdk_access_t *src, plp_aperta_phymod_phy_access_t *dst)
{
    memset(dst, 0, sizeof(*dst));
    dst->access.addr = src->phy_addr;
    dst->access.flags = 0;
    dst->access.lane_mask = src->lane_map;
    dst->access.user_acc = src->platform_ctxt;
    dst->port_loc = (src->if_side == PHY_SDK_SYSTEM_SIDE) ? phymodPortLocSys : phymodPortLocLine;
    dst->access.tvco_pll_index = APERTA_TVCO_PLL_INDEX;
}

/* ==========================================================================
 * _fw_info_get - Read FW version and CRC
 * ========================================================================== */
static int _fw_info_get(const plp_aperta_phymod_access_t *acc,
                        plp_aperta_phymod_core_firmware_info_t *fw_info)
{
    PHYMOD_IF_ERR_RETURN(_phy_sdk_bus_read(acc, BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr, &fw_info->fw_version));
    PHYMOD_IF_ERR_RETURN(_phy_sdk_bus_read(acc, BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r, &fw_info->fw_crc));
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _core_reset_set - Core reset via GEN_CONTROL1
 * reset_mode: 0=Hard (RESETBf), 1=Soft (SOFT_RSTBf)
 * must use Hard reset so the boot logic is fully re-initialized and the PHY
 * enters bootloader state ready to accept firmware download.
 * ========================================================================== */
static int _core_reset_set(const plp_aperta_phymod_phy_access_t *core, int reset_mode)
{
    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_t cr;
    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_t rb;
    PHYMOD_MEMSET(&cr, 0, sizeof(cr));
    PHYMOD_MEMSET(&rb, 0, sizeof(rb));
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_GEN_CNTRLS_GEN_CONTROL1r(&core->access, cr));
    LOG_INFO("RESET read  GEN_CTRL1(0x%x)=0x%x phy=0x%x mode=%d\n",
             BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r, cr.v[0], core->access.addr, reset_mode);

    if (reset_mode == 0) {
        /* Hard reset: assert top hard reset. Matches _plp_aperta_core_reset_set
         * which only writes RESETBf=0 once; the boot logic releases itself. */
        BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_RESETBf_SET(cr, 0);
        PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_WRITE_GEN_CNTRLS_GEN_CONTROL1r(&core->access, cr));
    } else {
        /* Soft reset: pulse SOFT_RSTBf low then high */
        BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_SOFT_RSTBf_SET(cr, 0);
        PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_WRITE_GEN_CNTRLS_GEN_CONTROL1r(&core->access, cr));
        PHYMOD_USLEEP(10000);
        BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_SOFT_RSTBf_SET(cr, 1);
        PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_WRITE_GEN_CNTRLS_GEN_CONTROL1r(&core->access, cr));
    }
    
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _check_fw_dl_status: Check FW download status
 * Verifies CRC, DWNLD registers, and micro-controller active state.
 * ========================================================================== */
static int _check_fw_dl_status(const plp_aperta_phymod_phy_access_t *ca)
{
    uint16_t n_img, no_of_img, rcnt = 100;
    BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr_t fw_ver;
    BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_t gpreg_1;
    BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r_t dwnld_01;
    BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r_t dwnld_03;
    uint32_t data = 0, crc = 0, aperta_fw_crc[2] = {0, 0};

    PHYMOD_MEMSET(&gpreg_1, 0, sizeof(gpreg_1));
    PHYMOD_MEMSET(&dwnld_01, 0, sizeof(dwnld_01));
    PHYMOD_MEMSET(&dwnld_03, 0, sizeof(dwnld_03));
    PHYMOD_IF_ERR_RETURN(_phy_sdk_bus_read(&ca->access, BCMI_APERTA_D_GEN_CNTRLS_DWNLD_11r, &aperta_fw_crc[0]));
    PHYMOD_IF_ERR_RETURN(_phy_sdk_bus_read(&ca->access, BCMI_APERTA_D_GEN_CNTRLS_DWNLD_13r, &aperta_fw_crc[1]));
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_GEN_CNTRLS_FIRMWARE_VERSIONr(&ca->access, fw_ver));
    n_img = 2;
    for (no_of_img = 0; no_of_img < n_img; ++no_of_img) {
        PHYMOD_IF_ERR_RETURN(_phy_sdk_bus_read(&ca->access, BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r + (no_of_img * 2), &data));
        if (data != 0x600D) {
            PHYMOD_IF_ERR_RETURN(_phy_sdk_bus_read(&ca->access, BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r + (no_of_img * 2) + 1, &data));
            crc = aperta_fw_crc[no_of_img];
            if (crc != data) {
                LOG_INFO("FW img:%d CRC mismatch got 0x%x exp 0x%x\n", no_of_img, data, crc);
                return PHYMOD_E_INTERNAL;
            }
        }
    }
    do {
        PHYMOD_USLEEP(5000);
        PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_GEN_CNTRLS_GPREG_01r(&ca->access, gpreg_1));
        if (BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_GPREG_01_DATAf_GET(gpreg_1) & 1) {
            if ((BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_GPREG_01_DATAf_GET(gpreg_1) & 0xFF00) != 0xFF00) {
                LOG_INFO("FW init seq failed: %x Phy:%x\n",
                    BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_GPREG_01_DATAf_GET(gpreg_1), ca->access.addr);
                return PHYMOD_E_FAIL;
            }
            PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_GEN_CNTRLS_DWNLD_03r(&ca->access, dwnld_03));
            PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_GEN_CNTRLS_DWNLD_01r(&ca->access, dwnld_01));
            PHYMOD_CRIT_INFO(("FW OK ver:0x%x CRC0:%x CRC1:%x\n",
                BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr_FIRMWARE_VERSION_VALf_GET(fw_ver),
                BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r_DOWNLOAD_01f_GET(dwnld_01),
                BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r_DOWNLOAD_03f_GET(dwnld_03)));
            return PHYMOD_E_NONE;
        }
    } while (--rcnt);
    LOG_INFO("FW download failure GPREG1:%x\n", BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_GPREG_01_DATAf_GET(gpreg_1));
    return PHYMOD_E_INTERNAL;
}

/* ==========================================================================
 * _pm_fw_dloaded_set
 * ========================================================================== */
static int _pm_fw_dloaded_set(const plp_aperta_phymod_phy_access_t *core, uint32_t active)
{
    unsigned short cnt;
    for (cnt = 0; cnt < APERTA_MAX_PM_INFO; cnt++) {
        if (_plp_aperta_pm_info[cnt].phy_id == core->access.addr) {
            _plp_aperta_pm_info[cnt].is_fw_dloaded = active;
            BCMI_APERTA_D_CTRL_SWGPREG0Er_t sw;
            memset(&sw, 0, sizeof(sw));
            PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_CTRL_SWGPREG0Er(&core->access, sw));
            sw.v[0] &= ~8;
            sw.v[0] |= (active & 1) << 3;
            PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_WRITE_CTRL_SWGPREG0Er(&core->access, sw));
            return PHYMOD_E_NONE;
        }
    }
    return PHYMOD_E_INTERNAL;
}

/* ==========================================================================
 * _pm_coreinitialized_flag_set - set a bit in SWGPREG0E warmboot status word.
 * APERTA_ISCOREINITIALIZED (bit0) /
 * APERTA_ISACTIVE (bit1) / APERTA_ISBYPASSED (bit2) / APERTA_FWDLOAD (bit3).
 * The FW reads IS_CORE_INITIALIZED / IS_ACTIVE when building the passthrough
 * crossbar; ai_app only set FWDLOAD (bit3), so the FW saw the port as
 * not-initialized / not-active and never built the crossbar (TXFIFO empty,
 * TPKT=0).
 * ========================================================================== */
static int _pm_coreinitialized_flag_set(const plp_aperta_phymod_phy_access_t *core, uint32_t bit, uint32_t value)
{
    BCMI_APERTA_D_CTRL_SWGPREG0Er_t sw;
    memset(&sw, 0, sizeof(sw));
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_CTRL_SWGPREG0Er(&core->access, sw));
    sw.v[0] &= ~(1u << bit);
    sw.v[0] |= ((value & 1) << bit);
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_WRITE_CTRL_SWGPREG0Er(&core->access, sw));
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_update_vco - persist TVCO for warmboot restore
 * SWGPREG15[15:8] (line side) and SWGPREG16[15:8] (sys side), which
 * will reads back on warm boot.
 * ========================================================================== */
static int _pm_update_vco(const plp_aperta_phymod_phy_access_t *core, uint32_t tvco)
{
    plp_aperta_phymod_phy_access_t phy;
    uint32_t rd;

    PHYMOD_MEMCPY(&phy, core, sizeof(phy));

    /* Line side: SWGPREG15[15:8] = tvco */
    phy.port_loc = phymodPortLocLine;
    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(&phy, BCMI_APERTA_D_CTRL_SWGPREG15r, &rd));
    rd = (rd & ~0xFF00u) | ((tvco & 0xFF) << 8);
    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(&phy, BCMI_APERTA_D_CTRL_SWGPREG15r, rd));

    /* Sys side: SWGPREG16[15:8] = tvco */
    phy.port_loc = phymodPortLocSys;
    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(&phy, BCMI_APERTA_D_CTRL_SWGPREG16r, &rd));
    rd = (rd & ~0xFF00u) | ((tvco & 0xFF) << 8);
    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(&phy, BCMI_APERTA_D_CTRL_SWGPREG16r, rd));

    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _pm_init_self - PM init, register-based
 * fw_init->pll1_vco_rate, PM info speed bookkeeping, SWGPREG0E VCO write,
 * TX driver supply and SWGPREG15/16 TVCO persistence. When fw_init is NULL
 * the previous defaults are used (26.562G VCO, 1.0/1.25V supply).
 * ========================================================================== */
static int _pm_init_self(const plp_aperta_phymod_phy_access_t *core, const phy_sdk_aperta_fw_init_t *fw_init)
{
    int idx, lane;
    uint32_t vco = 2;                 /* default: 26.562G (bcmplpapertaVco26p562G) */
    uint32_t vco_speed = APERTA_SPEED_VCO_400G; /* default: 400000 */
    uint32_t tx_drv_supply = 1;

    if (fw_init != NULL) {
        vco = fw_init->pll1_vco_rate;
        tx_drv_supply = fw_init->tx_drv_supply;
    }
    /* Map VCO -> default PM info speed (plp_aperta_pm_init vco_select) */
    if (vco == 2) {                          /* 26.562G */
        vco_speed = APERTA_SPEED_VCO_400G;   /* 400000 */
    } else if (vco == 1) {                   /* 25.781G */
        vco_speed = APERTA_SPEED_100G;       /* 100000 */
    } else {                                 /* 20.625G */
        vco_speed = APERTA_SPEED_40G;        /* 40000 */
    }

    for (idx = 0; idx < APERTA_MAX_PM_INFO; idx++) {
        if (_plp_aperta_pm_info[idx].phy_id == APERTA_UNINIT_PHYS ||
            _plp_aperta_pm_info[idx].phy_id == 0) {
            _plp_aperta_pm_info[idx].phy_id = core->access.addr;
            for (lane = 0; lane < APERTA_PM_NUM_LANES; lane++) {
                _plp_aperta_pm_info[idx].speed[lane] = (int)vco_speed;
                _plp_aperta_pm_info[idx].sys_speed[lane] = (int)vco_speed;
            }
            break;
        }
    }
    /* Write VCO select to SWGPREG0E (pll1_vco_rate << 8) */
    BCMI_APERTA_D_CTRL_SWGPREG0Er_t sw;
    PHYMOD_MEMSET(&sw, 0, sizeof(sw));
    sw.v[0] = (vco & 0xFF) << 8;
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_WRITE_CTRL_SWGPREG0Er(&core->access, sw));
    /* Write TX driver supply to SWGPREG0E[6] */
    plp_aperta_phymod_phy_access_t phy;
    uint32_t rd;
    PHYMOD_MEMCPY(&phy, core, sizeof(phy));
    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(&phy, 0x18b51, &rd));
    rd &= ~(1 << 6);
    rd |= (tx_drv_supply & 1) << 6;
    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(&phy, 0x18b51, rd));

    /* Persist TVCO for warmboot restore (plp_aperta_update_vco) */
    PHYMOD_IF_ERR_RETURN(_pm_update_vco(core, vco));
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _core_add_self - Core add (PASS1/PASS2)
 * PASS1: download main firmware (APERTA_W_FW=1 build path)
 *        + AM/UM/SpeedPriorityMap table uploads
 * PASS2: SpeedID table upload + PLL/AFE/micro configuration
 * ========================================================================== */
static int _core_add_self(const plp_aperta_phymod_phy_access_t *core, int pass, int method, const plp_aperta_phymod_core_init_config_t *cinit)
{
    if (pass == PORTMOD_PORT_ADD_F_INIT_PASS1) {
        LOG_INFO("core_add PASS1 method=%d addr=0x%x\n", method, core->access.addr);
        /* Main firmware download (line-side, method != None).
         * Matches _plp_aperta_tscbh_core_init_pass1 (APERTA_W_FW=1 build). */
        if (core->port_loc == phymodPortLocLine && method != phymodFirmwareLoadMethodNone) {
            PHYMOD_IF_ERR_RETURN(plp_aperta_dload_fw(core, method));
        }
        /* AM / UM / SpeedPriorityMap table uploads (pass1 tail). */
        PHYMOD_IF_ERR_RETURN(plp_aperta_tscbh_core_init_pass1_self(core));
    } else if (pass == PORTMOD_PORT_ADD_F_INIT_PASS2) {
        LOG_INFO("core_add PASS2 method=%d addr=0x%x\n", method, core->access.addr);
        /* SpeedID table upload + PLL/AFE/micro clk config (pass2). */
        PHYMOD_IF_ERR_RETURN(plp_aperta_tscbh_core_init_pass2_self(core, cinit));
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _core_dload_self
 * ========================================================================== */
static int _core_dload_self(const plp_aperta_phymod_phy_access_t *core, int pass, int fw_method, const plp_aperta_phymod_core_init_config_t *cinit)
{
    return _core_add_self(core, pass, fw_method, cinit);
}

/* ==========================================================================
 * _eeprom_serboot_low
 * ========================================================================== */
static int _eeprom_serboot_low(const plp_aperta_phymod_phy_access_t *core, BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_t boot_por)
{
    int rcnt = APERTA_MICRO_RETRY_COUNT;
    BCMI_APERTA_D_GEN_CNTRLS_BOOTr_t boot;
    uint32_t d1 = 0;
    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_t gc1;
    BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_t mo;
    PHYMOD_USLEEP(12000);
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_GEN_CNTRLS_MST_MSGOUTr(&core->access, mo));
    if (APERTA_MSGOUT_HDR_ERR != mo.v[0]) {
        do {
            PHYMOD_USLEEP(1000);
            PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_GEN_CNTRLS_BOOTr(&core->access, boot));
            d1 = BCMI_APERTA_D_GEN_CNTRLS_BOOTr_SERBOOT_BUSYf_GET(boot);
        } while ((d1 != 0) && (--rcnt));
        if ((rcnt <= 0) && (d1 != 0))
            { LOG_INFO("SERBOOT_BUSY\n"); return PHYMOD_E_INTERNAL; }
    }
    BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SERBOOTf_SET(boot_por, 0);
    BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_MST_DWLD_DONEf_SET(boot_por, 0);
    BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SLV_DWLD_DONEf_SET(boot_por, 0);
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_WRITE_MICRO_BOOT_BOOT_PORr(&core->access, boot_por));
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_GEN_CNTRLS_GEN_CONTROL1r(&core->access, gc1));
    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_SOFT_RSTBf_SET(gc1, 0);
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_WRITE_GEN_CNTRLS_GEN_CONTROL1r(&core->access, gc1));
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * core_init - Core initialization main
 * ========================================================================== */
static int core_init(const plp_aperta_phymod_phy_access_t *core, const plp_aperta_phymod_core_init_config_t *ic,
                           const plp_aperta_phymod_core_status_t *cs, const phy_sdk_aperta_fw_init_t *fw_init)
{
    BCMI_APERTA_D_CTRL_RESET_CTRLr_t reset_ctrl;
    BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_t bcast_en;
    BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_t gpreg1;
    BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_t boot_por;
    BCMI_APERTA_D_GEN_CNTRLS_BOOTr_t boot;
    uint32_t d1 = 0;
    int rcnt = APERTA_MICRO_RETRY_COUNT;
    BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_t msg_out;

    PHYMOD_MEMSET(&reset_ctrl, 0, sizeof(reset_ctrl));
    PHYMOD_MEMSET(&bcast_en, 0, sizeof(bcast_en));
    PHYMOD_MEMSET(&gpreg1, 0, sizeof(gpreg1));
    PHYMOD_MEMSET(&boot_por, 0, sizeof(boot_por));
    PHYMOD_MEMSET(&boot, 0, sizeof(boot));

    /* Mode 0: No FW load (SPI ROM self-boot) */
    if (ic->firmware_load_method == 0) {
        plp_aperta_phymod_core_firmware_info_t fwi;
        PHYMOD_USLEEP(12000);
        do {
            PHYMOD_USLEEP(1000);
            PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_GEN_CNTRLS_MST_MSGOUTr(&core->access, msg_out));
            if (APERTA_MSGOUT_HDR_ERR == msg_out.v[0]) break;
            PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_GEN_CNTRLS_BOOTr(&core->access, boot));
            d1 = BCMI_APERTA_D_GEN_CNTRLS_BOOTr_SERBOOT_BUSYf_GET(boot);
        } while ((d1 != 0) && (--rcnt));
        if ((rcnt <= 0) && (d1 != 0)) {
            LOG_INFO("SERBOOT_BUSY\n");
            return PHYMOD_E_INTERNAL;
        }
        PHYMOD_IF_ERR_RETURN(_pm_init_self(core, fw_init));
        PHYMOD_IF_ERR_RETURN(_check_fw_dl_status(core));
        PHYMOD_IF_ERR_RETURN(_fw_info_get(&core->access, &fwi));
        PHYMOD_IF_ERR_RETURN(_pm_fw_dloaded_set(core, 1));
        PHYMOD_CRIT_INFO(("PHY:0x%x FW ver:0x%x\n", core->access.addr, fwi.fw_version));
        PHYMOD_IF_ERR_RETURN(_core_dload_self(core, PORTMOD_PORT_ADD_F_INIT_PASS1, ic->firmware_load_method, ic));
        PHYMOD_IF_ERR_RETURN(_core_dload_self(core, PORTMOD_PORT_ADD_F_INIT_PASS2, ic->firmware_load_method, ic));
    }

    /* Mode 1: Core Reset + FW load */
    if (PHYMOD_CORE_INIT_F_RESET_CORE_FOR_FW_LOAD_GET(ic)) {
        plp_aperta_phymod_phy_access_t tc;
        if (((core->access.addr & 1) == 0) && (ic->firmware_load_method == 1)) {
            BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_t mc;
            BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_t ps;
            PHYMOD_MEMCPY(&tc, core, sizeof(tc));
            tc.access.addr |= 1;
            PHYMOD_IF_ERR_RETURN( BCMI_APERTA_D_READ_PAD_CNTRL_SERBOOT_STATUSr(&tc.access, ps));
            if (BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_RAWf_GET(ps) == 1) {
                PHYMOD_IF_ERR_RETURN( BCMI_APERTA_D_READ_CTRL_MISC_CONTROL_TYPEr(&tc.access, mc));
                BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCVALf_SET(mc, 1);
                BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCf_SET(mc, 1);
                PHYMOD_IF_ERR_RETURN( BCMI_APERTA_D_WRITE_CTRL_MISC_CONTROL_TYPEr(&tc.access, mc));
            }
        }
        PHYMOD_IF_ERR_RETURN(_core_reset_set(core, 0));

        if (ic->firmware_load_method == 2) {
            PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_READ_MICRO_BOOT_BOOT_PORr(&core->access, boot_por));
            if (BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SERBOOTf_GET(boot_por))
                PHYMOD_IF_ERR_RETURN(_eeprom_serboot_low(core, boot_por));
        }
        PHYMOD_USLEEP(12000);
        do {
            PHYMOD_USLEEP(1000);
            PHYMOD_IF_ERR_RETURN( BCMI_APERTA_D_READ_GEN_CNTRLS_MST_MSGOUTr(&core->access, msg_out));
            if (APERTA_MSGOUT_HDR_ERR == msg_out.v[0]) {
                break;
            }
            PHYMOD_IF_ERR_RETURN( BCMI_APERTA_D_READ_GEN_CNTRLS_BOOTr(&core->access, boot));
            d1 = BCMI_APERTA_D_GEN_CNTRLS_BOOTr_SERBOOT_BUSYf_GET(boot);
        } while ((d1 != 0) && (--rcnt));
        if ((rcnt <= 0) && (d1 != 0)) {
            LOG_INFO("SERBOOT_BUSY\n");
            return PHYMOD_E_INTERNAL;
        }

        PHYMOD_USLEEP(5000);
        PHYMOD_IF_ERR_RETURN(_pm_init_self(core, fw_init));
    }

    /* Mode 2: Broadcast enable */
    if (PHYMOD_CORE_INIT_F_UNTIL_FW_LOAD_GET(ic)) {
        PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_READ_GEN_CNTRLS_MDIO_PHYAD_CTRLr(&core->access, bcast_en));
        BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_MDIO_BRDCST_ENf_SET(bcast_en, 1);
        PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_WRITE_GEN_CNTRLS_MDIO_PHYAD_CTRLr(&core->access, bcast_en));
    }

    /* Execute FW load */
    if (PHYMOD_CORE_INIT_F_EXECUTE_FW_LOAD_GET(ic)) {
        PHYMOD_IF_ERR_RETURN(_core_dload_self(core, PORTMOD_PORT_ADD_F_INIT_PASS1, ic->firmware_load_method, ic));
        BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_MDIO_BRDCST_ENf_SET(bcast_en, 0);
        PHYMOD_IF_ERR_RETURN(BCMI_APERTA_D_WRITE_GEN_CNTRLS_MDIO_PHYAD_CTRLr(&core->access, bcast_en));
    }

    /* Resume after FW load */
    if (PHYMOD_CORE_INIT_F_RESUME_AFTER_FW_LOAD_GET(ic)) {
        plp_aperta_phymod_core_firmware_info_t fwi;
        int rv5;
        LOG_INFO("RESUME stage phy=0x%x\n", core->access.addr);
        rv5 = _check_fw_dl_status(core);
        LOG_INFO("RESUME check_fw_dl_status rv=%d\n", rv5);
        PHYMOD_IF_ERR_RETURN(rv5);
        rv5 = _fw_info_get(&core->access, &fwi);
        LOG_INFO("RESUME fw_info_get rv=%d ver=0x%x\n", rv5, fwi.fw_version);
        PHYMOD_IF_ERR_RETURN(rv5);
        rv5 = _pm_fw_dloaded_set(core, 1);
        LOG_INFO("RESUME fw_dloaded_set rv=%d\n", rv5);
        PHYMOD_IF_ERR_RETURN(rv5);
        PHYMOD_CRIT_INFO(("PHY:0x%x FW ver:0x%x\n", core->access.addr, fwi.fw_version));
        rv5 = _core_dload_self(core, PORTMOD_PORT_ADD_F_INIT_PASS2, ic->firmware_load_method, ic);
        LOG_INFO("RESUME PASS2 rv=%d\n", rv5);
        PHYMOD_IF_ERR_RETURN(rv5);
        /* Mark the core initialized (app sets IS_CORE_INITIALIZED on the SYS
         * core after serdes core-init completes). */
        PHYMOD_IF_ERR_RETURN(_pm_coreinitialized_flag_set(core, 0, 1)); /* bit0 IS_CORE_INITIALIZED */
        LOG_INFO("RESUME core initialized (SWGPREG0E bit0=1)\n");
    }

    /* FW load end - finalize initialization
     * Corresponds to app's bcmpmFirmwareBroadcastEnd stage.
     * After core init is complete, this verifies the FW is ready. */
    if (PHYMOD_CORE_INIT_F_FW_LOAD_END_GET(ic)) {
        plp_aperta_phymod_core_firmware_info_t fwi;
        /* If FW wasn't verified in the resume stage above, verify now */
        if (!PHYMOD_CORE_INIT_F_RESUME_AFTER_FW_LOAD_GET(ic)) {
            PHYMOD_IF_ERR_RETURN(_check_fw_dl_status(core));
            PHYMOD_IF_ERR_RETURN(_fw_info_get(&core->access, &fwi));
            PHYMOD_IF_ERR_RETURN(_pm_fw_dloaded_set(core, 1));
            PHYMOD_CRIT_INFO(("PHY:0x%x FW ver:0x%x\n", core->access.addr, fwi.fw_version));
        }
    }
    return PHYMOD_E_NONE;
}

/* ========================================================================
 * Public API
 * ======================================================================== */

int phy_sdk_core_firmware_info_get(phy_sdk_access_t *access, phy_sdk_fw_info_t *fw_info)
{
    plp_aperta_phymod_phy_access_t core;
    plp_aperta_phymod_core_firmware_info_t sfw;
    phy_sdk_acc_to_phy(access, &core);
    int rv = _fw_info_get(&core.access, &sfw);
    if (rv == PHYMOD_E_NONE) {
        fw_info->fw_version = sfw.fw_version;
        fw_info->fw_crc = sfw.fw_crc;
    }
    return rv;
}

int phy_sdk_rxtx_laneswap_set(phy_sdk_access_t *access, phy_sdk_laneswap_map_t *lm)
{
    plp_aperta_phymod_phy_access_t core;
    plp_aperta_phymod_lane_map_t loc_lane_map;
    unsigned int i, lane_index;
    phy_sdk_acc_to_phy(access, &core);
    PHYMOD_MEMSET(&loc_lane_map, 0, sizeof(loc_lane_map));
    loc_lane_map.num_of_lanes = lm->num_of_lanes;
    for (lane_index = 0; lane_index < APERTA_MAX_LANES; lane_index++) {
        if (lm->lane_map_rx[lane_index] <= 3) {
            loc_lane_map.lane_map_rx[lane_index] = 3 - lm->lane_map_rx[lane_index];
        } else {
            loc_lane_map.lane_map_rx[lane_index] = lm->lane_map_rx[lane_index];
        }

        if (lm->lane_map_tx[lane_index] <= 3) {
            loc_lane_map.lane_map_tx[lane_index] = 3 - lm->lane_map_tx[lane_index];
        } else {
            loc_lane_map.lane_map_tx[lane_index] = lm->lane_map_tx[lane_index];
        }
    }
    PHYMOD_IF_ERR_RETURN(plp_aperta_tscbh_core_lane_map_set(&core, &loc_lane_map));
    return PHYMOD_E_NONE;
}

int phy_sdk_rxtx_polarity_set(phy_sdk_access_t *access, uint32_t tx_pol, uint32_t rx_pol)
{
    unsigned int li;
    plp_aperta_phymod_polarity_t pol, pt;
    plp_aperta_phymod_phy_access_t phy, pt2;
    phy_sdk_acc_to_phy(access, &phy);
    pol.tx_polarity = tx_pol; pol.rx_polarity = rx_pol;
    PHYMOD_MEMCPY(&pt, &pol, sizeof(pt));
    for (li = 0; li < 8; li++) {
        if (phy.access.lane_mask & (1 << li)) {
            PHYMOD_MEMCPY(&pt2, &phy, sizeof(pt2));
            pt2.access.lane_mask = (1 << li);
            if (pol.tx_polarity != 0xFFFF) {
                pt.tx_polarity = (pol.tx_polarity & (1 << li)) >> li;
            }
            if (pol.rx_polarity != 0xFFFF) {
                pt.rx_polarity = (pol.rx_polarity & (1 << li)) >> li;
            }
            PHYMOD_IF_ERR_RETURN(plp_aperta_tscbh_phy_polarity_set(&pt2, &pt));
        }
    }
    return PHYMOD_E_NONE;
}

int phy_sdk_rxtx_polarity_get(phy_sdk_access_t *access, uint32_t *tx_pol, uint32_t *rx_pol)
{
    unsigned int li;
    plp_aperta_phymod_phy_access_t phy, pt;
    plp_aperta_phymod_polarity_t pol, pt2;
    phy_sdk_acc_to_phy(access, &phy);
    PHYMOD_MEMSET(&pol, 0, sizeof(pol));
    for (li = 0; li < 8; li++) {
        PHYMOD_MEMCPY(&pt, &phy, sizeof(pt));
        pt.access.lane_mask = (1 << li);
        PHYMOD_IF_ERR_RETURN(plp_aperta_tscbh_phy_polarity_get(&pt, &pt2));
        pol.tx_polarity |= ((pt2.tx_polarity ? 1 : 0) << li);
        pol.rx_polarity |= ((pt2.rx_polarity ? 1 : 0) << li);
    }
    *tx_pol = pol.tx_polarity; *rx_pol = pol.rx_polarity;
    return PHYMOD_E_NONE;
}

int phy_sdk_core_init(phy_sdk_access_t *access, int init_mode, int fw_load_method, const phy_sdk_aperta_fw_init_t *fw_init)
{
    plp_aperta_phymod_phy_access_t core;
    plp_aperta_phymod_core_init_config_t ic;
    plp_aperta_phymod_core_status_t cs;
    phy_sdk_acc_to_phy(access, &core);
    PHYMOD_MEMSET(&ic, 0, sizeof(ic));
    PHYMOD_MEMSET(&cs, 0, sizeof(cs));
    ic.core_init_mode = init_mode;

    ic.flags = (uint32_t)init_mode;
    ic.firmware_load_method = (plp_aperta_phymod_firmware_load_method_t)fw_load_method;
    ic.interface.ref_clock = 0;                       /* phymodRefClk156Mhz */
    ic.pll0_div_init_value = APERTA_TSCBH_PLL_DIVNONE;
    uint32_t vco = (fw_init != NULL) ? (uint32_t)fw_init->pll1_vco_rate : 2;   /* default 26.562G */
    if (vco == 0) {                /* 20.625G */
        ic.pll1_div_init_value = APERTA_TBHMOD_PLL_MODE_DIV_132;
    } else if (vco == 1) {         /* 25.781G */
        ic.pll1_div_init_value = APERTA_TBHMOD_PLL_MODE_DIV_165;
    } else {                       /* 26.562G */
        ic.pll1_div_init_value = APERTA_TBHMOD_PLL_MODE_DIV_170;
    }
    
    return core_init(&core, &ic, &cs, fw_init);
}

int phy_sdk_cleanup(phy_sdk_access_t *access)
{
    plp_aperta_phymod_phy_access_t core;
    phy_sdk_acc_to_phy(access, &core);
    return _core_reset_set(&core, 1);
}

int phy_sdk_fw_load(phy_sdk_access_t *access, phy_sdk_aperta_fw_init_t *fw_init, fw_load_method_e load_method)
{
    int rv;

    switch (load_method) {
        case FW_LOAD_NONE:
            /* No FW load - SPI ROM self-boot:
             * maps to bcmpmFirmwareLoadMethodNone + bcmpmFirmwareLoadSkip */
            rv = phy_sdk_core_init(access, 0, 0, fw_init);
            if (rv != PHY_SDK_SUCCESS) {
                LOG_INFO("phy_sdk_core_init FW_LOAD_NONE failed with rv=%d\n", rv);
                return rv;
            }
            break;

        case FW_LOAD_UNICAST:
            /* Unicast (Internal FW load, Force):
             * App maps to bcmpmFirmwareLoadMethodInternal + bcmpmFirmwareLoadForce,
             *   Reset -> Execute -> Resume -> End
             * (No UNTIL_FW_LOAD / Enable stage for single-PHY init) */
            rv = phy_sdk_core_init(access, CORE_INIT_F_RESET_CORE, 1, fw_init);
            if (rv != PHY_SDK_SUCCESS) {
                LOG_INFO("phy_sdk_core_init FW_LOAD_UNICAST CORE_INIT_F_RESET_COREfailed with rv=%d\n", rv);
                return rv;
            }
            rv = phy_sdk_core_init(access, CORE_INIT_F_EXECUTE_FW_LOAD, 1, fw_init);
            if (rv != PHY_SDK_SUCCESS) {
                LOG_INFO("phy_sdk_core_init FW_LOAD_UNICAST CORE_INIT_F_EXECUTE_FW_LOAD failed with rv=%d\n", rv);
                return rv;
            }
            rv = phy_sdk_core_init(access, CORE_INIT_F_RESUME_AFTER_FW_LOAD, 1, fw_init);
            if (rv != PHY_SDK_SUCCESS) {
                LOG_INFO("phy_sdk_core_init FW_LOAD_UNICAST CORE_INIT_F_RESUME_AFTER_FW_LOAD failed with rv=%d\n", rv);
                return rv;
            }
            rv = phy_sdk_core_init(access, CORE_INIT_F_FW_LOAD_END, 1, fw_init);
            if (rv != PHY_SDK_SUCCESS) {
                LOG_INFO("phy_sdk_core_init FW_LOAD_UNICAST CORE_INIT_F_FW_LOAD_END failed with rv=%d\n", rv);
                return rv;
            }
            break;

        case FW_LOAD_EEPROM:
            rv = phy_sdk_core_init(access, CORE_INIT_F_RESET_CORE, 2, fw_init);
            if (rv != PHY_SDK_SUCCESS) {
                LOG_INFO("phy_sdk_core_init FW_LOAD_EEPROM CORE_INIT_F_RESET_CORE failed with rv=%d\n", rv);
                return rv;
            }
            rv = phy_sdk_core_init(access, CORE_INIT_F_EXECUTE_FW_LOAD, 2, fw_init);
            if (rv != PHY_SDK_SUCCESS) {
                LOG_INFO("phy_sdk_core_init FW_LOAD_EEPROM CORE_INIT_F_EXECUTE_FW_LOAD failed with rv=%d\n", rv);
                return rv;
            }
            rv = phy_sdk_core_init(access, CORE_INIT_F_RESUME_AFTER_FW_LOAD, 2, fw_init);
            if (rv != PHY_SDK_SUCCESS) {
                LOG_INFO("phy_sdk_core_init FW_LOAD_EEPROM CORE_INIT_F_RESUME_AFTER_FW_LOAD failed with rv=%d\n", rv);
                return rv;
            }
            rv = phy_sdk_core_init(access, CORE_INIT_F_FW_LOAD_END, 2, fw_init);
            if (rv != PHY_SDK_SUCCESS) {
                LOG_INFO("phy_sdk_core_init FW_LOAD_EEPROM CORE_INIT_F_FW_LOAD_END failed with rv=%d\n", rv);
                return rv;
            }
            break;

        case FW_LOAD_UPGRADE:
            rv = phy_sdk_core_init(access, CORE_INIT_F_EXECUTE_FW_LOAD, 2, fw_init);
            if (rv != PHY_SDK_SUCCESS) {
                LOG_INFO("phy_sdk_core_init FW_LOAD_UPGRADE CORE_INIT_F_EXECUTE_FW_LOAD failed with rv=%d\n", rv);
                return rv;
            }
            rv = phy_sdk_core_init(access, CORE_INIT_F_RESUME_AFTER_FW_LOAD |
                                           CORE_INIT_F_FW_LOAD_END, 2, fw_init);
            if (rv != PHY_SDK_SUCCESS) {
                LOG_INFO("phy_sdk_core_init FW_LOAD_UPGRADE CORE_INIT_F_RESUME_AFTER_FW_LOAD | CORE_INIT_F_FW_LOAD_END failed with rv=%d\n", rv);
                return rv;
            }
            break;

        case FW_LOAD_SKIP:
            rv = phy_sdk_core_init(access, 0, 2, fw_init);
            if (rv != PHY_SDK_SUCCESS) {
                LOG_INFO("phy_sdk_core_init FW_LOAD_SKIP failed with rv=%d\n", rv);
                return rv;
            }
            break;

        default:
            LOG_INFO("phy_sdk_core_init INVALID PARAMETER\n");
            return STATUS_INVALID_PARAMETER;
    }
    return rv;
}
