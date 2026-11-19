/********************************************************************************
 * Copyright(C) 2026 Micas Network. All rights reserved.
 ********************************************************************************
 * PHY Application Layer Implementation BCM81394 APERTA
 ********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#include "aperta_phy.h"
#include "common.h"
#include "mdio.h"
#include "aperta_phy_sdk.h"

#define CHIP_NAME_APERTA "aperta"

#define PORT_LANE_SPEED_10G  (10000)
#define PORT_LANE_SPEED_20G  (20000)
#define PORT_LANE_SPEED_25G  (25000)
#define PORT_LANE_SPEED_50G  (50000)
#define PORT_LANE_SPEED_100G (100000)

/* PAM4 for >= 53.125G lanes, NRZ otherwise (sap_bcm_81394.c MODULATION_MODE_GET) */
#define MODULATION_MODE_GET(lane_data_rate) \
    ((lane_data_rate >= bcmplpApertaLaneDataRate_53P125G) ? \
     bcmplpApertaModulationPAM4 : bcmplpApertaModulationNRZ)

#define GET_LANE_SWAP_OCTAL(map, idx) ((map) >> (4 * (idx)) & 0xf)

#define GET_PORT_LANE_NUM(lane_map, lane_num) \
    do { \
        int _bc, _lc; \
        for (_bc = 0, _lc = 0; _bc < (int)sizeof(lane_map) * 8; _bc++) { \
            if ((lane_map & (1 << _bc)) != 0) _lc++; \
        } \
        lane_num = _lc; \
    } while (0)

#define GET_PORT_LANE_RATE(speed, lane_num, lane_rate) \
    do { \
        if ((speed / lane_num) == PORT_LANE_SPEED_10G) { \
            lane_rate = bcmplpApertaLaneDataRate_10P3125G; \
        } else if ((speed / lane_num) == PORT_LANE_SPEED_20G) { \
            lane_rate = bcmplpApertaLaneDataRate_20P625G; \
        } else if ((speed / lane_num) == PORT_LANE_SPEED_25G) { \
            lane_rate = bcmplpApertaLaneDataRate_25P78125G; \
        } else if ((speed / lane_num) == PORT_LANE_SPEED_50G) { \
            lane_rate = bcmplpApertaLaneDataRate_53P125G; \
        } else if ((speed / lane_num) == PORT_LANE_SPEED_100G) { \
            lane_rate = bcmplpApertaLaneDataRate_106P25G; \
        } else { \
            /* Default: 25.781G lane rate (matches fw_init pll1_vco_rate=1) */ \
            lane_rate = bcmplpApertaLaneDataRate_25P78125G; \
        } \
    } while (0)

#define GET_PORT_SINGLE_LANE_INDEX(lane_map, lane_idx) \
    do { \
        int _bc; \
        for (_bc = 0; _bc < (int)sizeof(lane_map) * 8; _bc++) { \
            if ((lane_map & (1 << _bc)) != 0) { \
                lane_idx = _bc; \
                break; \
            } \
        } \
    } while (0)

#define POINT_CHECK_RV(ptr, ptr_name, rv) \
    do { \
        if (ptr == NULL) { \
            LOG_INFO("%s is null, rv:%d", ptr_name, rv); \
            return rv; \
        } \
    } while (0)

#define RV_CHECK(phy_info, string, rv) \
    do { \
        if (rv != STATUS_SUCCESS) { \
            LOG_INFO("phy_addr:%d, %s failed, rv=%d", \
                        phy_info->phy_addr, string, rv); \
            return STATUS_FAILURE; \
        } \
    } while (0)

/* ==========================================================================
 * Bus function wrappers for phy_sdk
 * These adapt the mdio_read/mdio_write callbacks to the phy_sdk bus API.
 * ========================================================================== */
static int _bus_read_wrap(void *ctxt, uint32_t phy_addr, uint32_t reg_addr, uint32_t *data)
{
    phy_info_t *pi = (phy_info_t *)ctxt;
    sleep(0.01); /* for phy init - matches app sap_bcm_mdio_read */
    if (pi && pi->mdio_read)
        return pi->mdio_read(NULL, phy_addr, reg_addr, data);
    return -1;
}

static int _bus_write_wrap(void *ctxt, uint32_t phy_addr, uint32_t reg_addr, uint32_t data)
{
    phy_info_t *pi = (phy_info_t *)ctxt;
    sleep(0.01); /* for phy init - matches app sap_bcm_mdio_write */
    if (pi && pi->mdio_write)
        return pi->mdio_write(NULL, phy_addr, reg_addr, data);
    return -1;
}

/* ==========================================================================
 * Global variables
 * ========================================================================== */
static uint16_t g_port_num = 0;
static phy_info_t *g_phy_infos = NULL;
static int g_slot_num = 0;
static int g_phy_num = 0;
static int g_fw_load_method = 0;

/* ==========================================================================
 * Internal function declarations
 * ========================================================================== */
static int phy_config_init(int unit);
static int phy_init(int unit, int slot_id);
static int phy_fw_load(phy_info_t *phy_info);
static int phy_laneswap_polarity_init(phy_info_t *phy_info);
static int phy_chip_mode_config(port_info_t *port_info);
static int phy_port_linktraining_config(port_info_t *port_info);
static int phy_clean_up(phy_info_t *phy_info);
static int phy_get_profile_info(int profile_id, int speed, int split_num, int port_id, port_info_t *port_info);
static int phy_bcm_mode_config_set(port_info_t *port_info, int if_side);

#define PHYMOD_REG_APERTA_TSCBH         0x18000000u   /* TSC register base */

static void _phy_final_state_dump(void)
{
    phy_info_t *phy_info = g_phy_infos;
    int side;
    if (phy_info == NULL) return;

    for (side = 0; side < 2; side++) {
        phy_sdk_access_t sdk_access;
        plp_aperta_phymod_phy_access_t phy, pc;
        uint32_t d148 = 0, pll_sel = 0, rslvd = 0, latch = 0;
        uint32_t sig_det = 0, pmd_lock = 0;
        uint32_t rx_pwrdn = 0, tx_pwrdn = 0, cdmac_ctrl = 0;
        uint32_t tx_ctrl = 0, txfifo = 0;

        memset(&sdk_access, 0, sizeof(sdk_access));
        sdk_access.platform_ctxt = phy_info;
        sdk_access.phy_addr = phy_info->phy_addr;
        sdk_access.if_side = (side == 0) ? PHY_SDK_LINE_SIDE : PHY_SDK_SYSTEM_SIDE;
        sdk_access.lane_map = 0xF;
        phy_sdk_acc_to_phy(&sdk_access, &phy);
        phy.access.lane_mask = 0x1;
        memcpy(&pc, &phy, sizeof(pc));
        pc.access.pll_idx = APERTA_TVCO_PLL_INDEX;

        plp_aperta_reg32_read(&pc, PHYMOD_REG_APERTA_TSCBH | 0xd148, &d148);
        plp_aperta_reg32_read(&pc, PHYMOD_REG_APERTA_TSCBH | 0xd0b7, &pll_sel);
        plp_aperta_reg32_read(&pc, PHYMOD_REG_APERTA_TSCBH | 0xc070, &rslvd);
        plp_aperta_reg32_read(&pc, PHYMOD_REG_APERTA_TSCBH | 0xd0e8, &sig_det);   /* RX signal detect bit15 */
        plp_aperta_reg32_read(&pc, PHYMOD_REG_APERTA_TSCBH | 0xd16c, &pmd_lock);  /* RX PMD lock bit0 (TLB_RX_PMD_RX_LOCK_STATUS pmd_rx_lock) */
        plp_aperta_reg32_read(&phy, PHYMOD_REG_APERTA_TSCBH | 0xc160, &latch);
        pc.access.pll_idx = 0;
        plp_aperta_reg32_read(&pc, PHYMOD_REG_APERTA_TSCBH | 0xd1a1, &rx_pwrdn);  /* RX lane pwrdn bit0 */
        plp_aperta_reg32_read(&pc, PHYMOD_REG_APERTA_TSCBH | 0xd1b1, &tx_pwrdn);  /* TX lane pwrdn bit0 */
        pc.access.pll_idx = APERTA_TVCO_PLL_INDEX;
        plp_aperta_reg32_read(&phy, 0x1400010b, &cdmac_ctrl);                    /* CDMAC_CTRL (per side) */
        plp_aperta_reg32_read(&phy, 0x1500010d, &tx_ctrl);                       /* CDMAC TX_CTRL */
        plp_aperta_reg32_read(&phy, 0x1400011c, &txfifo);                        /* CDMAC TXFIFO_CELL_CNT */

        LOG_INFO("FINAL %s: PLL1 lock=%d PLL_SEL=0x%x SC_RSLVD spd=%d | "
                 "RX_SIGDET(0xd0e8)=0x%x b0=%d b15=%d PMD_LOCK(0xd16c)=0x%x lock=%d | "
                 "PCS(0xc160)=0x%x LIVE=%d | RX_PWRDN(0xd1a1)=0x%x TX_PWRDN(0xd1b1)=0x%x "
                 "CDMAC_CTRL(0x1400010b)=0x%x TX_EN=%d RX_EN=%d SRST=%d | "
                 "TX_CTRL(0x1500010d)=0x%x TX_THR=%d RECOVER_AM=%d DISCARD=%d "
                 "TXFIFO_CELLS(0x1400011c)=0x%x -> %s",
                 (side == 0) ? "LINE" : "SYS ",
                 (int)((d148 >> 8) & 1),
                 (unsigned)(pll_sel & 0xFFFF),
                 (int)((rslvd >> 10) & 0x3F),
                 (unsigned)(sig_det & 0xFFFF), (int)(sig_det & 1), (int)((sig_det >> 15) & 1),
                 (unsigned)(pmd_lock & 0xFFFF), (int)(pmd_lock & 1),
                 (unsigned)(latch & 0xFFFF), (int)((latch >> 10) & 1),
                 (unsigned)(rx_pwrdn & 0xFFFF), (unsigned)(tx_pwrdn & 0xFFFF),
                 (unsigned)(cdmac_ctrl & 0xFFFF), (int)(cdmac_ctrl & 1),
                 (int)((cdmac_ctrl >> 1) & 1), (int)((cdmac_ctrl >> 6) & 1),
                 (unsigned)(tx_ctrl & 0xFFFFFFFF), (int)((tx_ctrl >> 20) & 0x1F),
                 (int)((tx_ctrl >> 19) & 1), (int)((tx_ctrl >> 2) & 1),
                 (unsigned)(txfifo & 0xFFFF),
                 (((latch >> 10) & 1) && !((latch >> 2) & 1)) ? "LINK UP" : "no link");
    }
}

int aperta_phy_platform_init(int unit)
{
    int slot_id, rv;
    if (file_exists(SAP_BCM_CFG_FILE) != 1) {
        LOG_INFO("%s not found", SAP_BCM_CFG_FILE);
        return STATUS_NOT_SUPPORTED;
    }
    rv = phy_config_init(unit);
    if (STATUS_SUCCESS != rv) {
        LOG_INFO("phy_config_init fail");
        return STATUS_FAILURE;
    }

    for (slot_id = 0; slot_id < g_slot_num; slot_id++) {
        if (phy_init(unit, slot_id) != STATUS_SUCCESS) {
            return STATUS_FAILURE;
        }
    }
    _phy_final_state_dump();
    return STATUS_SUCCESS;
}

/* ==========================================================================
 * Chip ID read
 * ========================================================================== */
int phy_chip_id_get(phy_info_t *phy_info, uint32_t phy_addr, uint32_t *chip_id)
{
    int rv;
    uint32_t data;

    *chip_id = 0xFFFF;
    rv = phy_info->mdio_read(NULL, phy_addr, BCM_81394_CHIP_ID_ADDR, &data);
    if (rv != STATUS_SUCCESS) {
        LOG_INFO("mdio_read fail rv: %d", rv);
        return rv;
    }
    LOG_INFO("mdio_read chip id addr:0x%x data:0x%x", BCM_81394_CHIP_ID_ADDR, data);
    *chip_id = data;
    /* DIAG: POR-state bootloader status (base'd addrs, matching the app SDK).
     * Tells us whether the bootloader is already waiting for MDIO download
     * (then a hard reset is harmful) or not running at all. */
    {
        uint32_t d = 0;
        rv = phy_info->mdio_read(NULL, phy_addr, BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r, &d);
        LOG_INFO("POR GENCTL1 0x%x rv=%d val=0x%x\n", BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r, rv, d);
        d = 0;
        rv = phy_info->mdio_read(NULL, phy_addr, BCMI_APERTA_D_GEN_CNTRLS_BOOTr, &d);
        LOG_INFO("POR BOOT    0x%x rv=%d val=0x%x\n", BCMI_APERTA_D_GEN_CNTRLS_BOOTr, rv, d);
        d = 0;
        rv = phy_info->mdio_read(NULL, phy_addr, BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr, &d);
        LOG_INFO("POR BOOTPOR 0x%x rv=%d val=0x%x\n", BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr, rv, d);
        d = 0;
        rv = phy_info->mdio_read(NULL, phy_addr, BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr, &d);
        LOG_INFO("POR MSGOUT  0x%x rv=%d val=0x%x\n", BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr, rv, d);
        d = 0;
        rv = phy_info->mdio_read(NULL, phy_addr, BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r, &d);
        LOG_INFO("POR GENCTL2 0x%x rv=%d val=0x%x\n", BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r, rv, d);
    }
    return STATUS_SUCCESS;
}

/* ==========================================================================
 * PHY SDK Access get
 * ========================================================================== */
int phy_sdk_access_get(int unit, int port, int if_side, phy_sdk_access_t *sdk_access)
{
    int rv;
    port_info_t *port_info;

    if (sdk_access == NULL) {
        return STATUS_INVALID_PARAMETER;
    }
    rv = phy_port_info_get(unit, port, &port_info);
    if (rv != STATUS_SUCCESS) {
        LOG_INFO("unit[%d] port[%d] port_info_get failed", unit, port);
        return STATUS_FAILURE;
    }

    memset(sdk_access, 0, sizeof(*sdk_access));
    if_side = (if_side == 1) ? PHY_SDK_SYSTEM_SIDE : PHY_SDK_LINE_SIDE;
    sdk_access->platform_ctxt = port_info->phy_info;
    sdk_access->if_side = if_side;
    sdk_access->phy_addr = port_info->phy_info->phy_addr;
    sdk_access->lane_map = (if_side == PHY_SDK_SYSTEM_SIDE) ? port_info->lanemap_sys : port_info->lanemap_line;
    return STATUS_SUCCESS;
}

/* ==========================================================================
 * Port info get
 * ========================================================================== */
int phy_port_info_get(int unit, int port, port_info_t **port_info)
{
    int i, pi;
    for (i = 0; i < g_phy_num; i++) {
        for (pi = 0; pi < g_phy_infos[i].port_num; pi++) {
            port_info_t *p = (port_info_t *)g_phy_infos[i].port_infos[pi];
            if (p == NULL) {
                continue;
            }
            if (p->unit == unit && p->port == port) {
                *port_info = p;
                return STATUS_SUCCESS;
            }
        }
    }
    return STATUS_ITEM_NOT_FOUND;
}

/* ==========================================================================
 * parse PHY info from config file
 * ========================================================================== */
static int phy_config_init(int unit)
{
    int vals[CFG_FILE_ARR_MAX];
    int linktrain_sys[PHY_MAX_PORT_NUM];
    int ports[PHY_MAX_PORT_NUM];
    int speeds[PHY_MAX_PORT_NUM];
    int phy_addrs[PHY_MAX_NUM];
    char config_prefix[CFG_FILE_STR_MAX];
    int linktrain_line[PHY_MAX_PORT_NUM];
    int ports_lane_num[PHY_MAX_PORT_NUM];
    int index, port, port_id;
    int g_linktrain_sys = 0;
    int g_linktrain_line = 0;
    uint32_t chip_id;
    port_info_t *port_info;
    phy_info_t *phy_info;
    int i, j, rv, val;

    if (unit != 0) {
        return STATUS_SUCCESS;
    }

    /* slot_num */
    rv = get_cfg_info(SAP_BCM_CFG_FILE, "plp_slot_num", &g_slot_num, false);
    if (rv != STATUS_SUCCESS) {
        LOG_INFO("plp_slot_num get empty.");
        return STATUS_ITEM_NOT_FOUND;
    }
    /* fw_load_method */
    rv = get_cfg_info(SAP_BCM_CFG_FILE, "plp_fw_load_method", &g_fw_load_method, false);
    if (rv != STATUS_SUCCESS) {
        LOG_INFO("plp_fw_load_method get empty.");
        return STATUS_ITEM_NOT_FOUND;
    }
    /* linktrain_sys/line (global defaults, optional) */
    get_cfg_info(SAP_BCM_CFG_FILE, "plp_port_linktrain_sys", &g_linktrain_sys, false);
    get_cfg_info(SAP_BCM_CFG_FILE, "plp_port_linktrain_line", &g_linktrain_line, false);

    /* phy_addrs */
    memset(phy_addrs, -1, sizeof(phy_addrs));
    rv = get_cfg_info(SAP_BCM_CFG_FILE, "plp_phy_addrs", phy_addrs, true);
    if (rv != STATUS_SUCCESS) {
        LOG_INFO("plp_phy_addrs get empty.");
        return STATUS_ITEM_NOT_FOUND;
    }
    g_phy_num = 0;
    for (i = 0; i < PHY_MAX_NUM; i++) {
        if (phy_addrs[i] == -1) {
            break;
        }
        g_phy_num++;
    }
    g_phy_infos = (phy_info_t *)malloc(sizeof(phy_info_t) * g_phy_num);

    for (i = 0; i < g_phy_num; i++) {
        phy_info = &g_phy_infos[i];
        memset(phy_info, 0, sizeof(*phy_info));
        phy_info->phy_addr = (unsigned int)phy_addrs[i];
        phy_info->chip_name = CHIP_NAME_APERTA;
        phy_info->tx_pol_line = 0;
        phy_info->tx_pol_sys = 0;
        phy_info->rx_pol_line = 0;
        phy_info->rx_pol_sys = 0;

        /* card_id */
        snprintf(config_prefix, sizeof(config_prefix), "plp_phy_card_id:0x%x", phy_addrs[i]);
        rv = get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
        if (rv != STATUS_SUCCESS) {
            LOG_INFO("%s get empty.", config_prefix);
            return STATUS_ITEM_NOT_FOUND;
        }
        phy_info->card_id = val;

        /* mdio_id */
        snprintf(config_prefix, sizeof(config_prefix), "plp_phy_mdio_id:0x%x", phy_addrs[i]);
        rv = get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
        if (rv != STATUS_SUCCESS) {
            LOG_INFO("%s get empty.", config_prefix);
            return STATUS_ITEM_NOT_FOUND;
        }
        phy_info->mdio_id = val;

        /* mdio_type */
        snprintf(config_prefix, sizeof(config_prefix), "plp_mdio_accs_type:%d", val);
        rv = get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
        if (rv != STATUS_SUCCESS) {
            LOG_INFO("%s get empty.", config_prefix);
            return STATUS_ITEM_NOT_FOUND;
        }
        phy_info->mdio_type = val;

        /* mdio_func */
        rv = get_mdio_write_func(phy_info->mdio_type, &phy_info->mdio_write);
        if (rv != STATUS_SUCCESS) {
            LOG_INFO("PHY-0x%x mdio write func not reg!", phy_info->phy_addr);
            return STATUS_FAILURE;
        }
        rv = get_mdio_read_func(phy_info->mdio_type, &phy_info->mdio_read);
        if (rv != STATUS_SUCCESS) {
            LOG_INFO("PHY-0x%x mdio read func not reg!", phy_info->phy_addr);
            return STATUS_FAILURE;
        }

        /* check chip id */
        chip_id = 0xFFFF;
        rv = phy_chip_id_get(phy_info, phy_info->phy_addr, &chip_id);
        if (rv != STATUS_SUCCESS) {
            LOG_INFO("PHY-0x%x phy_chip_id_get fail", phy_info->phy_addr);
            return STATUS_FAILURE;
        }
        if (chip_id == 0xFFFF) {
            LOG_INFO("PHY-0x%x chip_id invalid", phy_info->phy_addr);
            return STATUS_FAILURE;
        }
        if (chip_id != BCM_81394_CHIP_ID) {
            LOG_INFO("PHY-0x%x chip_id isn't 81394", phy_info->phy_addr);
            free(g_phy_infos);
            return STATUS_NOT_SUPPORTED;
        }

        /* unit */
        snprintf(config_prefix, sizeof(config_prefix), "plp_phy_unit_id:0x%x", phy_addrs[i]);
        rv = get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
        if (rv != STATUS_SUCCESS) {
            LOG_INFO("%s get empty.", config_prefix);
            return STATUS_ITEM_NOT_FOUND;
        }
        phy_info->unit = val;

        /* macsec_option (optional) */
        snprintf(config_prefix, sizeof(config_prefix), "plp_phy_macsec_option:0x%x", phy_addrs[i]);
        if (get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false) == STATUS_SUCCESS) {
            phy_info->macsec_option = val;
        }

        /* pll1_vco_rate */
        snprintf(config_prefix, sizeof(config_prefix), "plp_phy_pll1_vco_rate:0x%x", phy_addrs[i]);
        if (get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false) == STATUS_SUCCESS) {
            phy_info->pll1_vco_rate = val;
        }
        
        /* ptp_option (optional) */
        snprintf(config_prefix, sizeof(config_prefix), "plp_phy_ptp_option:0x%x", phy_addrs[i]);
        if (get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false) == STATUS_SUCCESS) {
            phy_info->ptp_option = val;
        }

        /* profile_id */
        snprintf(config_prefix, sizeof(config_prefix), "plp_phy_init_profile_id:0x%x", phy_addrs[i]);
        rv = get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
        if (rv != STATUS_SUCCESS) {
            LOG_INFO("%s get empty.", config_prefix);
            return STATUS_ITEM_NOT_FOUND;
        }
        phy_info->profile_id = val;

        /* polarity (all optional, default 0) */
        snprintf(config_prefix, sizeof(config_prefix), "plp_tx_polarity_flip_line:0x%x", phy_addrs[i]);
        if (get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false) == STATUS_SUCCESS) {
            phy_info->tx_pol_line = val;
        }

        snprintf(config_prefix, sizeof(config_prefix), "plp_rx_polarity_flip_line:0x%x", phy_addrs[i]);
        if (get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false) == STATUS_SUCCESS) {
            phy_info->rx_pol_line = val;
        }

        snprintf(config_prefix, sizeof(config_prefix), "plp_tx_polarity_flip_sys:0x%x", phy_addrs[i]);
        if (get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false) == STATUS_SUCCESS) {
            phy_info->tx_pol_sys = val;
        }

        snprintf(config_prefix, sizeof(config_prefix), "plp_rx_polarity_flip_sys:0x%x", phy_addrs[i]);
        if (get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false) == STATUS_SUCCESS) {
            phy_info->rx_pol_sys = val;
        }

        /* lane_map (all optional, remain -1 if not configured) */
        memset(phy_info->tx_lane_map_line, -1, sizeof(phy_info->tx_lane_map_line));
        snprintf(config_prefix, sizeof(config_prefix), "plp_phy_tx_lane_map_line:0x%x", phy_addrs[i]);
        get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, phy_info->tx_lane_map_line, true);

        memset(phy_info->rx_lane_map_line, -1, sizeof(phy_info->rx_lane_map_line));
        snprintf(config_prefix, sizeof(config_prefix), "plp_phy_rx_lane_map_line:0x%x", phy_addrs[i]);
        get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, phy_info->rx_lane_map_line, true);

        memset(phy_info->tx_lane_map_sys, -1, sizeof(phy_info->tx_lane_map_sys));
        snprintf(config_prefix, sizeof(config_prefix), "plp_phy_tx_lane_map_sys:0x%x", phy_addrs[i]);
        get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, phy_info->tx_lane_map_sys, true);

        memset(phy_info->rx_lane_map_sys, -1, sizeof(phy_info->rx_lane_map_sys));
        snprintf(config_prefix, sizeof(config_prefix), "plp_phy_rx_lane_map_sys:0x%x", phy_addrs[i]);
        get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, phy_info->rx_lane_map_sys, true);

        /* VCO (optional, default 0) */
        snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_init_vco_octal_0_sys", phy_info->profile_id);
        if (get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false) == STATUS_SUCCESS) {
            phy_info->vco_octal_0_sys = val;
        }

        snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_init_vco_octal_0_line", phy_info->profile_id);
        if (get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false) == STATUS_SUCCESS) {
            phy_info->vco_octal_0_line = val;
        }

        snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_init_vco_octal_1_sys", phy_info->profile_id);
        if (get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false) == STATUS_SUCCESS) {
            phy_info->vco_octal_1_sys = val;
        }

        snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_init_vco_octal_1_line", phy_info->profile_id);
        if (get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false) == STATUS_SUCCESS) {
            phy_info->vco_octal_1_line = val;
        }
        /* lanes */
        memset(vals, -1, sizeof(vals));
        memset(phy_info->lanes, -1, sizeof(phy_info->lanes));
        snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_init_lanes:0x%x", phy_info->profile_id, phy_addrs[i]);
        rv = get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, vals, true);
        if (rv != STATUS_SUCCESS) {
            LOG_INFO("%s get empty.", config_prefix);
            return STATUS_ITEM_NOT_FOUND;
        }

        phy_info->lane_num = 0;
        for (j = 0; j < PHY_MAX_PORT_NUM; j++) {
            if (vals[j] == -1) {
                break;
            }
            phy_info->lanes[j] = vals[j];
            phy_info->lane_num++;
        }

        /* ports */
        memset(ports, -1, sizeof(ports));
        snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_init_ports:0x%x", phy_info->profile_id, phy_addrs[i]);
        rv = get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, ports, true);
        if (rv != STATUS_SUCCESS) {
            LOG_INFO("%s get empty.", config_prefix);
            return STATUS_ITEM_NOT_FOUND;
        }
        LOG_INFO("cfg: profile=%d addr=0x%x init_ports[0]=%d", phy_info->profile_id, phy_addrs[i], ports[0]);

        /* ports_lane_num */
        memset(ports_lane_num, -1, sizeof(ports_lane_num));
        snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_init_ports_lane_num:0x%x", phy_info->profile_id, phy_addrs[i]);
        rv = get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, ports_lane_num, true);
        if (rv != STATUS_SUCCESS) {
            LOG_INFO("%s get empty.", config_prefix);
            return STATUS_ITEM_NOT_FOUND;
        }

        /* speeds */
        memset(speeds, -1, sizeof(speeds));
        snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_init_speed_mode:0x%x", phy_info->profile_id, phy_addrs[i]);
        rv = get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, speeds, true);
        if (rv != STATUS_SUCCESS) {
            LOG_INFO("%s get empty.", config_prefix);
            return STATUS_ITEM_NOT_FOUND;
        }

        /* linktrain (per-PHY, optional - fallback to global defaults) */
        memset(linktrain_sys, -1, sizeof(linktrain_sys));
        snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_init_linktrain_sys:0x%x", phy_info->profile_id, phy_addrs[i]);
        get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, linktrain_sys, true);

        memset(linktrain_line, -1, sizeof(linktrain_line));
        snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_init_linktrain_line:0x%x", phy_info->profile_id, phy_addrs[i]);
        get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, linktrain_line, true);

        /* Build port_info linked list */
        memset(phy_info->ports, -1, sizeof(phy_info->ports));
        memset(phy_info->ports_lane_num, -1, sizeof(phy_info->ports_lane_num));
        index = 0;
        for (j = 0; j < PHY_MAX_PORT_NUM; j++) {
            if (ports[j] == -1) {
                break;
            }
            index += (j > 0) ? ports_lane_num[j - 1] : 0;
            phy_info->ports[index] = ports[j];
            phy_info->ports_lane_num[index] = ports_lane_num[j];
        }

        port_id = 0;
        for (index = 0; index < PHY_MAX_PORT_NUM; index++) {
            port = phy_info->ports[index];
            if (port == -1) {
                continue;
            }

            port_info = (port_info_t *)malloc(sizeof(port_info_t));
            if (port_info == NULL) {
                LOG_INFO("malloc failed!");
                continue;
            }

            memset(port_info, 0, sizeof(*port_info));
            port_info->phy_info       = phy_info;
            port_info->port           = port;
            port_info->unit           = phy_info->unit;
            port_info->phy_lane0      = phy_info->lanes[index];
            port_info->speed          = speeds[port_id];
            port_info->lanes          = phy_info->ports_lane_num[index];
            port_info->linktrain_sys  = (linktrain_sys[port_id] != -1) ? linktrain_sys[port_id] : g_linktrain_sys;
            port_info->linktrain_line = (linktrain_line[port_id] != -1) ? linktrain_line[port_id] : g_linktrain_line;

            phy_get_profile_info(phy_info->profile_id, port_info->speed, phy_info->lane_num / port_info->lanes, index, port_info);

            phy_info->port_infos[port_id] = port_info;
            phy_info->port_num = port_id + 1;
            g_port_num++;
            LOG_INFO("cfg: built port %d (unit=%d phy=0x%x speed=%d lanes=%d fec_sys=%d fec_line=%d) g_port_num=%d",
                     port, port_info->unit, phy_info->phy_addr, port_info->speed,
                     port_info->lanes, port_info->fec_sys, port_info->fec_line, g_port_num);
            port_id++;
        }
    }
    return STATUS_SUCCESS;
}

/* ==========================================================================
 * Profile info get
 * ========================================================================== */
static int phy_get_profile_info(int profile_id, int speed, int split_num, int port_id, port_info_t *port_info)
{
    int rv, val;
    char config_prefix[CFG_FILE_STR_MAX];
    if (port_info == NULL) {
        return STATUS_INVALID_PARAMETER;
    }
    snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_speed_%d_split_%d.%d:fec_line", profile_id, speed, split_num, port_id);
    rv = get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
    if (rv != STATUS_SUCCESS) {
        LOG_INFO("%s get empty.", config_prefix);
        return STATUS_FAILURE; 
    } else {
        port_info->fec_line = val;
    }
    snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_speed_%d_split_%d.%d:fec_sys", profile_id, speed, split_num, port_id);
    rv = get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
    if (rv != STATUS_SUCCESS) {
        LOG_INFO("%s get empty.", config_prefix);
        return STATUS_FAILURE; 
    } else {
        port_info->fec_sys = val;
    }
    snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_speed_%d_split_%d.%d:iftype_line", profile_id, speed, split_num, port_id);
    rv = get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
    if (rv != STATUS_SUCCESS) {
        LOG_INFO("%s get empty.", config_prefix);
        return STATUS_FAILURE; 
    } else {
        port_info->if_type_line = val;
    }
    snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_speed_%d_split_%d.%d:iftype_sys", profile_id, speed, split_num, port_id);
    rv = get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
    if (rv != STATUS_SUCCESS) {
        LOG_INFO("%s get empty.", config_prefix);
        return STATUS_FAILURE; 
    } else {
        port_info->if_type_sys = val;
    }
    snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_speed_%d_split_%d.%d:lanemap_line", profile_id, speed, split_num, port_id);
    rv = get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
    if (rv != STATUS_SUCCESS) {
        LOG_INFO("%s get empty.", config_prefix);
        return STATUS_FAILURE; 
    } else {
        port_info->lanemap_line = val;
    }
    snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_speed_%d_split_%d.%d:lanemap_sys", profile_id, speed, split_num, port_id);
    rv = get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
    if (rv != STATUS_SUCCESS) {
        LOG_INFO("%s get empty.", config_prefix);
        return STATUS_FAILURE; 
    } else {
        port_info->lanemap_sys = val;
    }
    snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_speed_%d_split_%d.%d:force_nr_sys", profile_id, speed, split_num, port_id);
    rv = get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
    if (rv != STATUS_SUCCESS) {
        LOG_INFO("%s get empty.", config_prefix);
        return STATUS_FAILURE; 
    } else {
        port_info->force_nr_sys = val;
    }
    snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_speed_%d_split_%d.%d:force_nr_line", profile_id, speed, split_num, port_id);
    rv = get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
    if (rv != STATUS_SUCCESS) {
        LOG_INFO("%s get empty.", config_prefix);
        return STATUS_FAILURE; 
    } else {
        port_info->force_nr_line = val;
    }
    snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_speed_%d_split_%d.%d:force_er_sys", profile_id, speed, split_num, port_id);
    rv = get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
    if (rv != STATUS_SUCCESS) {
        LOG_INFO("%s get empty.", config_prefix);
        return STATUS_FAILURE; 
    } else {
        port_info->force_er_sys = val;
    }
    snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_speed_%d_split_%d.%d:force_er_line", profile_id, speed, split_num, port_id);
    rv = get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
    if (rv != STATUS_SUCCESS) {
        LOG_INFO("%s get empty.", config_prefix);
        return STATUS_FAILURE; 
    } else {
        port_info->force_er_line = val;
    }
    return STATUS_SUCCESS;
}

/* ==========================================================================
 * Slot init
 * ========================================================================== */
static int phy_init(int unit, int slot_id)
{
    int i, rv;
    phy_info_t *phy_info;
    port_info_t *port_info;

    LOG_INFO("slot %d init", slot_id);

    for (i = 0; i < g_phy_num; i++) {
        phy_info = &g_phy_infos[i];
        if (phy_info->card_id != slot_id || phy_info->unit != unit) {
            continue;
        }

        rv = phy_fw_load(phy_info);
        if (rv != STATUS_SUCCESS) {
            phy_clean_up(phy_info);
            rv = phy_fw_load(phy_info);
            if (rv != STATUS_SUCCESS) {
                LOG_INFO("PHY-0x%x fw load failed again, rv=%d", phy_info->phy_addr, rv);
                continue;
            }
        }

        phy_sdk_access_t sdk_access;
        plp_aperta_phymod_phy_access_t phy;
        memset(&sdk_access, 0, sizeof(sdk_access));
        sdk_access.platform_ctxt = phy_info;
        sdk_access.phy_addr = phy_info->phy_addr;
        sdk_access.if_side = PHY_SDK_LINE_SIDE;
        sdk_access.lane_map = PHY_SDK_ALL_LANE_MAP;
        phy_sdk_acc_to_phy(&sdk_access, &phy);
        rv = plp_aperta_phy_macsec_init(&phy, 1);
        if (rv != PHYMOD_E_NONE) {
            LOG_INFO("PHY-0x%x phy_phy_init failed, rv=%d", phy_info->phy_addr, rv);
            continue;
        }

        rv = phy_laneswap_polarity_init(phy_info);
        if (rv != STATUS_SUCCESS) {
            LOG_INFO("PHY-0x%x init failed, rv=%d", phy_info->phy_addr, rv);
            continue;
        }
        phy_info->loaded = 1;

        int pi;
        port_info_t *p0 = (port_info_t *)phy_info->port_infos[0];
        LOG_INFO("slot %d: port-config %d port(s) fec_sys=%d fec_line=%d",
                    slot_id, phy_info->port_num, p0 ? p0->fec_sys : -1, p0 ? p0->fec_line : -1);
        for (pi = 0; pi < phy_info->port_num; pi++) {
            port_info = (port_info_t *)phy_info->port_infos[pi];
            if (port_info == NULL || port_info->phy_info != phy_info) {
                continue;
            }
            rv = phy_chip_mode_config(port_info);
            if (rv == STATUS_SUCCESS) {
                port_info->inited = 1;
            }
        }
        for (pi = 0; pi < phy_info->port_num; pi++) {
            port_info = (port_info_t *)phy_info->port_infos[pi];
            if (port_info == NULL || port_info->phy_info != phy_info) {
                continue;
            }
            phy_port_linktraining_config(port_info);
        }
    }
    return STATUS_SUCCESS;
}

/* ==========================================================================
 * FW load
 * ========================================================================== */
static int phy_fw_load(phy_info_t *phy_info)
{
    int rv;
    phy_sdk_access_t sdk_access;
    static int bus_registered = 0;

    if (phy_info == NULL) {
         return STATUS_INVALID_PARAMETER;
    }

    /* Register bus functions once */
    if (!bus_registered) {
        phy_sdk_set_bus_funcs(_bus_read_wrap, _bus_write_wrap);
        bus_registered = 1;
    }

    memset(&sdk_access, 0, sizeof(sdk_access));
    sdk_access.platform_ctxt = phy_info;
    sdk_access.phy_addr = phy_info->phy_addr;
    
    phy_sdk_aperta_fw_init_t fw_init;
    memset(&fw_init, 0, sizeof(fw_init));
    fw_init.macsec_static_bypass = 1;
    /* (0=20.625G, 1=25.781G, 2=26.562G) */
    fw_init.pll1_vco_rate = phy_info->pll1_vco_rate;

    rv = phy_sdk_fw_load(&sdk_access, &fw_init, (fw_load_method_e)g_fw_load_method);
    if (rv != PHY_SDK_SUCCESS) {
        LOG_INFO("PHY-0x%x fw_load failed, rv=%d", phy_info->phy_addr, rv);
        return rv;
    }

    return STATUS_SUCCESS;
}

/* ==========================================================================
 * PHY Lane init: firmware info + lane swap + polarity
 * ========================================================================== */
static int phy_laneswap_polarity_init(phy_info_t *phy_info)
{
    int rv, lane_index;
    uint32_t read_tx_pol, read_rx_pol;
    phy_sdk_fw_info_t fw_info;
    phy_sdk_access_t sdk_access;
    phy_sdk_laneswap_map_t sys_lane_map, line_lane_map;

    if (phy_info == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    memset(&sdk_access, 0, sizeof(sdk_access));
    sdk_access.platform_ctxt = phy_info;
    sdk_access.phy_addr = phy_info->phy_addr;

    /* 1. Lane Swap System side */
    memset(&sys_lane_map, 0, sizeof(sys_lane_map));
    memset(&line_lane_map, 0, sizeof(line_lane_map));
    sys_lane_map.num_of_lanes = PHY_SDK_MAX_LANE;
    line_lane_map.num_of_lanes = PHY_SDK_MAX_LANE;

    for (lane_index = 0; lane_index < PHY_SDK_MAX_LANE; lane_index++) {
        sys_lane_map.lane_map_rx[lane_index] = GET_LANE_SWAP_OCTAL(phy_info->rx_lane_map_sys[0], lane_index);
        sys_lane_map.lane_map_tx[lane_index] = GET_LANE_SWAP_OCTAL(phy_info->tx_lane_map_sys[0], lane_index);
        line_lane_map.lane_map_rx[lane_index] = GET_LANE_SWAP_OCTAL(phy_info->rx_lane_map_line[0], lane_index);
        line_lane_map.lane_map_tx[lane_index] = GET_LANE_SWAP_OCTAL(phy_info->tx_lane_map_line[0], lane_index);
    }
    /* 2. Lane Swap set*/
    /* 2.1 Lane Swap System side FIRST */
    sdk_access.if_side = PHY_SDK_SYSTEM_SIDE;
    sdk_access.lane_map = PHY_SDK_ALL_LANE_MAP;
    rv = phy_sdk_rxtx_laneswap_set(&sdk_access, &sys_lane_map);
    if (rv != PHY_SDK_SUCCESS) {
        LOG_INFO("PHY-0x%x sys lane_map_set failed, rv=%d", phy_info->phy_addr, rv);
        return rv;
    }

    /* 2.2 Lane Swap Line side */
    sdk_access.if_side = PHY_SDK_LINE_SIDE;
    rv = phy_sdk_rxtx_laneswap_set(&sdk_access, &line_lane_map);
    if (rv != PHY_SDK_SUCCESS) {
        LOG_INFO("PHY-0x%x line lane_map_set failed, rv=%d", phy_info->phy_addr, rv);
        return rv;
    }

    /* 2.3 Lane swap readback verification - read back the full 8-lane RX/TX
     * map on BOTH sides */
    {
        int side;
        for (side = 0; side < 2; side++) {
            plp_aperta_phymod_phy_access_t core_acc;
            plp_aperta_phymod_lane_map_t rd_map;
            sdk_access.if_side = (side == 0) ? PHY_SDK_SYSTEM_SIDE
                                             : PHY_SDK_LINE_SIDE;
            sdk_access.lane_map = PHY_SDK_ALL_LANE_MAP;
            phy_sdk_acc_to_phy(&sdk_access, &core_acc);
            memset(&rd_map, 0, sizeof(rd_map));
            /* 2.4 Raw PCS lane-swap MPP readback - one-shot proof that the value
             * WRITTEN into the PCS register 
             *   SYS  cfg 0x76543210 -> set [3,2,1,0] -> MPP0 = 3|2<<3|1<<6|0<<9 = 0x053
             *   LINE cfg 0x76542301 -> set [2,3,0,1] -> MPP0 = 2|3<<3|0<<6|1<<9 = 0x21a
             * If raw MPP0 != 0x053 (SYS) / 0x21a (LINE), the lane-swap write is wrong. */
            {
                plp_aperta_phymod_phy_access_t mphy;
                uint32_t raw_tx = 0, raw_rx = 0;
                phy_sdk_acc_to_phy(&sdk_access, &mphy);
                mphy.access.lane_mask = 0x1;   /* MPP0 = physical lanes 0-3 */
                plp_aperta_reg32_read(&mphy, PHYMOD_REG_APERTA_TSCBH | 0x9200, &raw_tx);
                plp_aperta_reg32_read(&mphy, PHYMOD_REG_APERTA_TSCBH | 0x9225, &raw_rx);
                LOG_INFO("PHY-0x%x %s raw PCS lane-swap MPP0 TX(0x9200)=0x%x RX(0x9225)=0x%x (expect SYS 0x053 / LINE 0x21a)",
                         phy_info->phy_addr, (side == 0) ? "sys" : "line", (unsigned)(raw_tx & 0xFFFF), (unsigned)(raw_rx & 0xFFFF));
            }
            rv = plp_aperta_tscbh_core_lane_map_get(&core_acc, &rd_map);
            if (rv != PHYMOD_E_NONE) {
                LOG_INFO("PHY-0x%x bcm_plp_logical_lane_get failed at %s side, rv=%d",
                         phy_info->phy_addr, (side == 0) ? "System" : "Line", rv);
                return rv;
            }
            for (lane_index = 0; lane_index < APERTA_MAX_LANES; lane_index++) {
                if (rd_map.lane_map_rx[lane_index] <= 3) {
                    rd_map.lane_map_rx[lane_index] = 3 - rd_map.lane_map_rx[lane_index];
                }
                if (rd_map.lane_map_tx[lane_index] <= 3) {
                    rd_map.lane_map_tx[lane_index] = 3 - rd_map.lane_map_tx[lane_index];
                }
            }
            LOG_INFO("PHY-0x%x Successfully get rx logical lane (%s : 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x) for PHY-0x%x at %s side!",
                     phy_info->phy_addr, (side == 0) ? "sys" : "line",
                     rd_map.lane_map_rx[0], rd_map.lane_map_rx[1], rd_map.lane_map_rx[2], rd_map.lane_map_rx[3],
                     rd_map.lane_map_rx[4], rd_map.lane_map_rx[5], rd_map.lane_map_rx[6], rd_map.lane_map_rx[7],
                     phy_info->phy_addr, (side == 0) ? "System" : "Line");
            LOG_INFO("PHY-0x%x Successfully get tx logical lane (%s : 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x) for PHY-0x%x at %s side!",
                     phy_info->phy_addr, (side == 0) ? "sys" : "line",
                     rd_map.lane_map_tx[0], rd_map.lane_map_tx[1], rd_map.lane_map_tx[2], rd_map.lane_map_tx[3],
                     rd_map.lane_map_tx[4], rd_map.lane_map_tx[5], rd_map.lane_map_tx[6], rd_map.lane_map_tx[7],
                     phy_info->phy_addr, (side == 0) ? "System" : "Line");
        }
    }

    /* 3 Polarity set */
    /* 3.1 Polarity System side */
    sdk_access.if_side = PHY_SDK_SYSTEM_SIDE;
    sdk_access.lane_map = PHY_SDK_ALL_LANE_MAP;
    rv = phy_sdk_rxtx_polarity_get(&sdk_access, &read_tx_pol, &read_rx_pol);
    if (rv != PHY_SDK_SUCCESS) {
        LOG_INFO("PHY-0x%x sys polarity_get failed, rv=%d", phy_info->phy_addr, rv);
        return rv;
    }
    LOG_INFO("PHY-0x%x sys polarity GET TX=0x%x RX=0x%x", phy_info->phy_addr, read_tx_pol, read_rx_pol);
    read_tx_pol ^= phy_info->tx_pol_sys;
    read_rx_pol ^= phy_info->rx_pol_sys;
    rv = phy_sdk_rxtx_polarity_set(&sdk_access, read_tx_pol, read_rx_pol);
    if (rv != PHY_SDK_SUCCESS) {
        LOG_INFO("PHY-0x%x sys polarity_set failed, rv=%d", phy_info->phy_addr, rv);
        return rv;
    }
    LOG_INFO("PHY-0x%x sys polarity set TX=0x%x RX=0x%x", phy_info->phy_addr, read_tx_pol, read_rx_pol);

    /* 3.2 Polarity Line side */
    sdk_access.if_side = PHY_SDK_LINE_SIDE;
    rv = phy_sdk_rxtx_polarity_get(&sdk_access, &read_tx_pol, &read_rx_pol);
    if (rv != PHY_SDK_SUCCESS) {
        LOG_INFO("PHY-0x%x line polarity_get failed, rv=%d", phy_info->phy_addr, rv);
        return rv;
    }
    LOG_INFO("PHY-0x%x line polarity GET TX=0x%x RX=0x%x", phy_info->phy_addr, read_tx_pol, read_rx_pol);
    read_tx_pol ^= phy_info->tx_pol_line;
    read_rx_pol ^= phy_info->rx_pol_line;
    rv = phy_sdk_rxtx_polarity_set(&sdk_access, read_tx_pol, read_rx_pol);
    if (rv != PHY_SDK_SUCCESS) {
        LOG_INFO("PHY-0x%x line polarity_set failed, rv=%d", phy_info->phy_addr, rv);
        return rv;
    }
    LOG_INFO("PHY-0x%x line polarity set TX=0x%x RX=0x%x", phy_info->phy_addr, read_tx_pol, read_rx_pol);

    return STATUS_SUCCESS;
}

/* ==========================================================================
 * Cleanup
 * ========================================================================== */
static int phy_clean_up(phy_info_t *phy_info)
{
    phy_sdk_access_t sdk_access;

    if (phy_info == NULL) {
        return STATUS_INVALID_PARAMETER;
    }
    memset(&sdk_access, 0, sizeof(sdk_access));
    sdk_access.platform_ctxt = phy_info;
    sdk_access.phy_addr = phy_info->phy_addr;

    int rv = phy_sdk_cleanup(&sdk_access);
    if (rv != PHY_SDK_SUCCESS) {
        LOG_INFO("PHY-0x%x cleanup failed, rv=%d", phy_info->phy_addr, rv);
        return STATUS_FAILURE;
    }
    return STATUS_SUCCESS;
}



/* ==========================================================================
 * Chip Mode Config
 * configure System and Line side port modes
 * SYSTEM side to be configured FIRST,chip required
 * ========================================================================== */
static int phy_chip_mode_config(port_info_t *port_info)
{
    int rv;

    /* System side FIRST */
    rv = phy_bcm_mode_config_set(port_info, PHY_SDK_SYSTEM_SIDE);
    if (rv != STATUS_SUCCESS) {
        LOG_INFO("port %d sys mode_config_set failed", port_info->port);
        return rv;
    }

    /* Line side SECOND */
    rv = phy_bcm_mode_config_set(port_info, PHY_SDK_LINE_SIDE);
    if (rv != STATUS_SUCCESS) {
        LOG_INFO("port %d line mode_config_set failed", port_info->port);
        return rv;
    }

    return STATUS_SUCCESS;
}

/* ==========================================================================
 * port linktraining config
 * side when configured (cfg: linktrain_sys=0, linktrain_line=0 -> skipped).
 * ========================================================================== */
static int phy_port_linktraining_config(port_info_t *port_info)
{
    phy_sdk_access_t sdk_access;
    plp_aperta_phymod_phy_access_t phy;
    int rv;
    
    if (port_info == NULL) {
        return STATUS_INVALID_PARAMETER;
    }
    
    if (port_info->linktrain_sys == 1) {
        rv = phy_sdk_access_get(port_info->unit, port_info->port, PHY_SDK_SYSTEM_SIDE, &sdk_access);
        if (rv == STATUS_SUCCESS) {
            phy_sdk_acc_to_phy(&sdk_access, &phy);
            rv = plp_aperta_tscbh_phy_cl72_set(&phy, 1);
            if (rv == PHYMOD_E_NONE) {
                LOG_INFO("unit[%d] port[%d], sys link training enable success", port_info->unit, port_info->port);
            } else {
                LOG_INFO("unit[%d] port[%d], sys link training enable failed with rv=%d", port_info->unit, port_info->port, rv);
            }
        } else {
            LOG_INFO("unit[%d] port[%d], phy_sdk_access_get failed for sys side", port_info->unit, port_info->port);
        }
    }
    if (port_info->linktrain_line == 1) {
        rv = phy_sdk_access_get(port_info->unit, port_info->port, PHY_SDK_LINE_SIDE, &sdk_access);
        if (rv == STATUS_SUCCESS) {
            phy_sdk_acc_to_phy(&sdk_access, &phy);
            rv = plp_aperta_tscbh_phy_cl72_set(&phy, 1);
            if (rv == PHYMOD_E_NONE) {
                LOG_INFO("unit[%d] port[%d], line link training enable success", port_info->unit, port_info->port);
            } else {
                LOG_INFO("unit[%d] port[%d], line link training enable failed with rv=%d", port_info->unit, port_info->port, rv);
            }
        } else {
            LOG_INFO("unit[%d] port[%d], phy_sdk_access_get failed for line side", port_info->unit, port_info->port);
        }
    }
    return STATUS_SUCCESS;
}

/* ==========================================================================
 * Mode Config Set - configure port speed / FEC / interface type
 * via device_aux_modes
 * ========================================================================== */
static int phy_bcm_mode_config_set(port_info_t *port_info, int if_side)
{
    phy_sdk_access_t sdk_access;
    plp_aperta_phymod_phy_access_t phy;
    plp_aperta_phymod_phy_inf_config_t cfg;
    aperta_device_aux_modes_t aux_mode;
    int sys_lane_count, line_lane_count, lane_count;
    int lane_rate, port_type;
    int rv;

    if (port_info == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    GET_PORT_LANE_NUM(port_info->lanemap_sys, sys_lane_count);
    GET_PORT_LANE_NUM(port_info->lanemap_line, line_lane_count);

    /* port_type: Passthrough / GearBox / ReverseGearBox */
    if (port_info->lanemap_sys == port_info->lanemap_line) {
        port_type = bcmplpApertaPortTypePassthrough;
    } else if (sys_lane_count == line_lane_count) {
        port_type = bcmplpApertaPortTypePassthrough;
    } else if (sys_lane_count > line_lane_count) {
        port_type = bcmplpApertaPortTypeGearBox;
    } else {
        port_type = bcmplpApertaPortTypeReverseGearBox;
    }

    rv = phy_sdk_access_get(port_info->unit, port_info->port, if_side, &sdk_access);
    if (rv != STATUS_SUCCESS) {
        return rv;
    }
    phy_sdk_acc_to_phy(&sdk_access, &phy);

    lane_count = (if_side == PHY_SDK_SYSTEM_SIDE) ? sys_lane_count : line_lane_count;
    if (lane_count == 0) {
        lane_count = 1;
    }
    GET_PORT_LANE_RATE(port_info->speed, lane_count, lane_rate);

    memset(&cfg, 0, sizeof(cfg));
    memset(&aux_mode, 0, sizeof(aux_mode));
    aux_mode.lane_data_rate  = lane_rate;
    aux_mode.modulation_mode = MODULATION_MODE_GET(lane_rate);
    aux_mode.fec_mode_sel    = (if_side == PHY_SDK_SYSTEM_SIDE) ? port_info->fec_sys : port_info->fec_line;
    aux_mode.port_type       = port_type;

    cfg.data_rate            = port_info->speed;
    cfg.interface_type       = (if_side == PHY_SDK_SYSTEM_SIDE) ? port_info->if_type_sys : port_info->if_type_line;
    cfg.ref_clock            = 0;   /* 156.25 MHz */
    cfg.device_aux_modes     = &aux_mode;
    if (if_side == PHY_SDK_SYSTEM_SIDE) {
        phy.access.lane_mask = port_info->lanemap_sys;
    }
    else {
        phy.access.lane_mask = port_info->lanemap_line;
    }

    LOG_INFO("mode_config: port=%d if_side=%d speed=%d fec=%d type=%d lane=0x%x lane_rate=%d port_type=%d",
             port_info->port, if_side, cfg.data_rate, aux_mode.fec_mode_sel,
             cfg.interface_type, phy.access.lane_mask, lane_rate, port_type);

    rv = plp_aperta_phy_interface_config_set(&phy, &cfg);
    if (rv != PHYMOD_E_NONE) {
        LOG_INFO("port %d if_side=%d interface_config_set failed rv=%d", port_info->port, if_side, rv);
        return STATUS_FAILURE;
    }
    return STATUS_SUCCESS;
}

int phy_fw_version_get(int unit, int port, sap_version_t *version_info)
{
    phy_sdk_access_t sdk_access;
    phy_sdk_fw_info_t fw_info;
    int rv = phy_sdk_access_get(unit, port, 0, &sdk_access);
    if (rv != STATUS_SUCCESS) return rv;

    rv = phy_sdk_core_firmware_info_get(&sdk_access, &fw_info);
    if (rv == PHY_SDK_SUCCESS) {
        version_info->fw_ver = fw_info.fw_version;
        version_info->fw_crc = fw_info.fw_crc;
    }
    return (rv == PHY_SDK_SUCCESS) ? STATUS_SUCCESS : STATUS_FAILURE;
}

/* ==========================================================================
 * _phy_link_status_get - read PCS latched link status for one side.
 * ========================================================================== */
static bool _phy_link_status_get(int unit, int port, int if_side)
{
    phy_sdk_access_t sdk_access;
    plp_aperta_phymod_phy_access_t phy;
    uint32_t latch = 0;

    if (phy_sdk_access_get(unit, port, if_side, &sdk_access) != STATUS_SUCCESS) {
        return false;
    }
    phy_sdk_acc_to_phy(&sdk_access, &phy);
    if (plp_aperta_reg32_read(&phy, PHYMOD_REG_APERTA_TSCBH | 0xc160, &latch) != PHYMOD_E_NONE) {
        return false;
    }
    return (((latch >> 10) & 1) && !((latch >> 2) & 1)) ? true : false;
}

int phy_port_status_get(int unit, int port, sap_port_status_t *port_status)
{
    port_info_t *port_info;
    int rv = phy_port_info_get(unit, port, &port_info);
    if (rv != STATUS_SUCCESS) {
        return rv;
    }

    memset(port_status, 0, sizeof(*port_status));
    port_status->speed = port_info->speed;
    port_status->fec_sys = port_info->fec_sys;
    port_status->fec_line = port_info->fec_line;
    port_status->host_lane_num = port_info->lanes;
    port_status->line_lane_num = port_info->lanes;
    port_status->host_admin = port_info->inited;
    port_status->line_admin = port_info->inited;
    snprintf(port_status->mode, sizeof(port_status->mode), "%s", port_info->phy_info->chip_name);
    /* Read live link status for both sides (app host_link_up/line_link_up). */
    port_status->host_link_up = _phy_link_status_get(unit, port, PHY_SDK_SYSTEM_SIDE);
    port_status->line_link_up = _phy_link_status_get(unit, port, PHY_SDK_LINE_SIDE);
    return STATUS_SUCCESS;
}

phy_apis_t extphy_aperta_apis = {
    .type                           = "BCM_81394",
    .phy_platform_init              = aperta_phy_platform_init
};
