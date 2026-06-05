/*
 *
 * $Id: phymod.xml,v 1.1.2.5 2013/09/12 10:43:06 nirf Exp $
 *
 * $Copyright:
 * All Rights Reserved.$
 *
 *
 */

#include <phymod/phymod.h>
#include <phymod/phymod_util.h>
#include <phymod/phymod_dispatch.h>
#include <bcmi_blackhawk_xgxs_defs.h>
#include <blackhawk.h>
#include <blackhawk/tier1/blackhawk_cfg_seq.h>
#include <blackhawk/tier1/blackhawk_tsc_enum.h>
#include <blackhawk/tier1/blackhawk_tsc_common.h>
#include <blackhawk/tier1/blackhawk_tsc_interface.h>
#include <blackhawk/tier1/blackhawk_tsc_dependencies.h>
#include <blackhawk/tier1/blackhawk_tsc_internal.h>
#include <blackhawk/tier1/public/blackhawk_api_uc_vars_rdwr_defns_public.h>
#include <blackhawk/tier1/blackhawk_tsc_access.h>




extern unsigned char plp_aperta_blackhawk_ucode[];
extern unsigned int  plp_aperta_blackhawk_ucode_len;
extern unsigned short plp_aperta_blackhawk_ucode_crc;
extern unsigned short plp_aperta_blackhawk_ucode_stack_size;


#define BLACKHAWK_MODEL               0x26
#define BLACKHAWK_NOF_LANES_IN_CORE   0x8
#define BLACKHAWK_PHY_ALL_LANES       0xff
#define BLACKHAWK_TX_TAP_NUM          12
#define BLACKHAWK_PMD_CRC_UCODE       1


#define RAMON_REF_CLOCK_HZ            312500000

#define BLACKHAWK_CORE_TO_PHY_ACCESS(_phy_access, _core_access) \
    do{\
        PHYMOD_MEMCPY(&(_phy_access)->access, &(_core_access)->access, sizeof((_phy_access)->access));\
        (_phy_access)->type           = (_core_access)->type; \
        (_phy_access)->port_loc       = (_core_access)->port_loc; \
        (_phy_access)->device_op_mode = (_core_access)->device_op_mode; \
        (_phy_access)->access.lane_mask = BLACKHAWK_PHY_ALL_LANES; \
    }while(0)


int plp_aperta_blackhawk_core_identify(const plp_aperta_phymod_core_access_t* core, uint32_t core_id, uint32_t* is_identified)
{
    plp_aperta_phymod_phy_access_t  phy_copy;

    blackhawk_rev_id0_t rev_id0;
    blackhawk_rev_id1_t rev_id1;
    *is_identified = 0;

    PHYMOD_MEMCPY(&phy_copy, core, sizeof(phy_copy));


    /* PHY IDs match - now check model */
    PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_identify(&phy_copy, &rev_id0, &rev_id1));
    if (rev_id0.revid_model == BLACKHAWK_MODEL)  {
            *is_identified = 1;
    }

    return PHYMOD_E_NONE;

}


int plp_aperta_blackhawk_core_info_get(const plp_aperta_phymod_core_access_t* core, plp_aperta_phymod_core_info_t* info)
{
    /*info->core_version = phymodCoreVersionBlackhawk16;*/
    info->serdes_id = 0;
    info->phy_id0 = 0;
    info->phy_id1 = 0;

    return PHYMOD_E_NONE;

}


int plp_aperta_blackhawk_core_lane_map_get(const plp_aperta_phymod_core_access_t* core, plp_aperta_phymod_lane_map_t* lane_map)
{
    plp_aperta_phymod_phy_access_t  phy_copy;
    uint32_t tx_lane_map, rx_lane_map;
    int i = 0;

    PHYMOD_MEMCPY(&phy_copy, core, sizeof(phy_copy));
    phy_copy.access.lane_mask = 0x1;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_pmd_lane_map_get(&phy_copy, &tx_lane_map, &rx_lane_map));

    /*next get the lane map into serdes spi format */
    for (i = 0; i < BLACKHAWK_NOF_LANES_IN_CORE; i++) {
        lane_map->lane_map_tx[tx_lane_map >> (4 * i) & 0xf] = i;
        lane_map->lane_map_rx[rx_lane_map >> (4 * i) & 0xf] = i;
    }

     return PHYMOD_E_NONE;
}


int plp_aperta_blackhawk_core_reset_set(const plp_aperta_phymod_core_access_t* core, plp_aperta_phymod_reset_mode_t reset_mode, plp_aperta_phymod_reset_direction_t direction)
{
    return PHYMOD_E_NONE;

}

int plp_aperta_blackhawk_core_reset_get(const plp_aperta_phymod_core_access_t* core, plp_aperta_phymod_reset_mode_t reset_mode, plp_aperta_phymod_reset_direction_t* direction)
{
    return PHYMOD_E_NONE;

}



int plp_aperta_blackhawk_phy_tx_lane_control_set(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_phy_tx_lane_control_t tx_control)
{
    int start_lane, num_lane, i;
    uint32_t lane_reset;
    plp_aperta_phymod_phy_access_t pm_phy_copy;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    /*first check if lane is in reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset_get(&pm_phy_copy, &lane_reset));

    /* if lane is not in reset, then reset the lane first */
    if (!lane_reset) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_lane_soft_reset(&pm_phy_copy, 1));
    }

    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        switch (tx_control)
        {
            case phymodTxElectricalIdleEnable:
                PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_electrical_idle_set(&pm_phy_copy, 1));
                break;
            case phymodTxElectricalIdleDisable:
                PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_electrical_idle_set(&pm_phy_copy, 0));
                break;
            case phymodTxSquelchOn:
                PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_tx_disable(&pm_phy_copy, 1));
                break;
            case phymodTxSquelchOff:
                PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_tx_disable(&pm_phy_copy, 0));
                break;
            default:
                PHYMOD_DEBUG_ERROR(("This control is NOT SUPPORTED!! (plp_aperta_blackhawk_phy_tx_lane_control_set) \n"));
                break;
        }
    }

    /* if lane is not in reset, then reset the lane first */
    if (!lane_reset) {
        PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_lane_soft_reset(&pm_phy_copy, 0));
    }

    return PHYMOD_E_NONE;
}


int plp_aperta_blackhawk_phy_tx_lane_control_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_phy_tx_lane_control_t *tx_control)
{

    uint8_t disable, idle_enable;
    plp_aperta_phymod_phy_access_t pm_phy_copy;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));


    *tx_control = phymodTxSquelchOff;

    PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_tx_disable_get(&pm_phy_copy, &disable));
    if(disable) {
      *tx_control = phymodTxSquelchOn;
    } else {
      PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_electrical_idle_get(&pm_phy_copy, &idle_enable));
      if (!idle_enable) {
        *tx_control = phymodTxElectricalIdleDisable;
      }
    }

    return PHYMOD_E_NONE;
}

/*Rx control*/
int plp_aperta_blackhawk_phy_rx_lane_control_set(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_phy_rx_lane_control_t rx_control)
{
    plp_aperta_phymod_phy_access_t pm_phy_copy;
    int start_lane, num_lane, i;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    /* next program the tx fir taps and driver current based on the input */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    /*put the lane into dp reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset(&pm_phy_copy, 1));


    switch (rx_control) {
    case phymodRxSquelchOn:
        for (i = 0; i < num_lane; i++) {
            if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
                continue;
            }
            pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_pmd_force_signal_detect(&pm_phy_copy, 1, 0));
        }
        break;
    case phymodRxSquelchOff:
        for (i = 0; i < num_lane; i++) {
            if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
                continue;
            }
            pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_pmd_force_signal_detect(&pm_phy_copy, 0, 0));
        }
        break;
    default:
        break;
    }

    /*release the lane dp reset */
    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset(&pm_phy_copy, 0));

    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_rx_lane_control_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_phy_rx_lane_control_t* rx_control)
{
    uint8_t force_en, force_val;
    plp_aperta_phymod_phy_access_t pm_phy_copy;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    /* first get the force enabled bit and forced value */
    PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_pmd_force_signal_detect_get(&pm_phy_copy, &force_en, &force_val));

    if ((force_en) && (force_val == 0)) {
        *rx_control = phymodRxSquelchOn;
    } else {
        *rx_control = phymodRxSquelchOff;
    }
    return PHYMOD_E_NONE;

}

int plp_aperta_blackhawk_phy_autoneg_ability_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_autoneg_ability_t* an_ability_set_type)
{


    /* Not supported */
    PHYMOD_DEBUG_ERROR(("This function is NOT SUPPORTED!! (plp_aperta_blackhawk_phy_rx_lane_control_get) \n"));


    return PHYMOD_E_NONE;

}

int plp_aperta_blackhawk_phy_autoneg_ability_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_autoneg_ability_t* an_ability_get_type)
{
    /* Not supported */
    PHYMOD_DEBUG_ERROR(("This function is NOT SUPPORTED!! (plp_aperta_blackhawk_phy_autoneg_ability_get) \n"));
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta_blackhawk_phy_autoneg_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_autoneg_control_t* an)
{
    /* Not supported */
    PHYMOD_DEBUG_ERROR(("This function is NOT SUPPORTED!! (plp_aperta_blackhawk_phy_autoneg_set) \n"));
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta_blackhawk_phy_autoneg_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_autoneg_control_t* an, uint32_t* an_done)
{
    /* Not supported */
    PHYMOD_DEBUG_ERROR(("This function is NOT SUPPORTED!! (plp_aperta_blackhawk_phy_autoneg_get) \n"));
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta_blackhawk_phy_autoneg_status_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_autoneg_status_t* status)
{
    /* Not supported */
    PHYMOD_DEBUG_ERROR(("This function is NOT SUPPORTED!! (plp_aperta_blackhawk_phy_autoneg_status_get) \n"));
    return PHYMOD_E_UNAVAIL;
}

/* load tscf fw. the fw_loader parameter is valid just for external fw load*/
STATIC
int _plp_aperta_blackhawk_core_firmware_load(const plp_aperta_phymod_core_access_t* core, const plp_aperta_phymod_core_init_config_t* init_config)
{
    int wait;
    plp_aperta_phymod_phy_access_t  phy_copy;

    PHYMOD_MEMCPY(&phy_copy, core, sizeof(phy_copy));

    switch(init_config->firmware_load_method){
    case phymodFirmwareLoadMethodInternal:
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_ucode_mdio_load(&phy_copy, plp_aperta_blackhawk_ucode, plp_aperta_blackhawk_ucode_len));
        break;
    case phymodFirmwareLoadMethodExternal:
        if(!PHYMOD_CORE_INIT_F_RESUME_AFTER_FW_LOAD_GET(init_config)) {
            PHYMOD_NULL_CHECK(init_config->firmware_loader);
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_ucode_init(&phy_copy));
            if(PHYMOD_CORE_INIT_F_UNTIL_FW_LOAD_GET(init_config)) {
                wait = 0;
            } else {
                wait = 1;
            }
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_pram_firmware_enable(&phy_copy, 1, wait));

            if(PHYMOD_CORE_INIT_F_UNTIL_FW_LOAD_GET(init_config)) {
                return PHYMOD_E_NONE;
            }

            /*PHYMOD_IF_ERR_RETURN(init_config->firmware_loader(core, plp_aperta_blackhawk_ucode_len, plp_aperta_blackhawk_ucode));*/
        }
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_pram_firmware_enable(&phy_copy, 0, 0));
        break;
    case phymodFirmwareLoadMethodNone:
        break;
    default:
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, (_PHYMOD_MSG("illegal fw load method %u"), init_config->firmware_load_method));
    }

    return PHYMOD_E_NONE;

}

int plp_aperta_blackhawk_phy_firmware_core_config_set(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_firmware_core_config_t fw_config)
{
    struct blackhawk_tsc_uc_core_config_st serdes_firmware_core_config;
    uint32_t is_write_disabled;
    plp_aperta_phymod_phy_access_t pm_phy_copy;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    PHYMOD_IF_ERR_RETURN(PHYMOD_IS_WRITE_DISABLED(&phy->access, &is_write_disabled));
    if (is_write_disabled){
        return PHYMOD_E_NONE;
    }

    PHYMOD_MEMSET(&serdes_firmware_core_config, 0, sizeof(serdes_firmware_core_config));
    PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_set_core_config_from_pcs(&pm_phy_copy, fw_config.CoreConfigFromPCS));

    return PHYMOD_E_NONE;
}


int plp_aperta_blackhawk_phy_firmware_core_config_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_firmware_core_config_t* fw_config)
{
    /* this function is not supported on BH */
    PHYMOD_DEBUG_ERROR(("Unsupported feature in BH \n"));
    return PHYMOD_E_UNAVAIL;
}


int plp_aperta_blackhawk_phy_firmware_lane_config_get(const plp_aperta_phymod_phy_access_t* phy, phymod_firmware_lane_config_t* fw_config)
{
    struct blackhawk_tsc_uc_lane_config_st lane_config;
    plp_aperta_phymod_phy_access_t pm_phy_copy;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    PHYMOD_MEMSET(&lane_config, 0x0, sizeof(lane_config));
    PHYMOD_MEMSET(fw_config, 0, sizeof(*fw_config));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_get_uc_lane_cfg(&pm_phy_copy, &lane_config));

    fw_config->LaneConfigFromPCS     = lane_config.field.lane_cfg_from_pcs;
    fw_config->AnEnabled             = lane_config.field.an_enabled;
    fw_config->DfeOn                 = lane_config.field.dfe_on;
    fw_config->LpDfeOn               = lane_config.field.dfe_lp_mode;
    fw_config->ForceBrDfe            = lane_config.field.force_brdfe_on;
    fw_config->MediaType             = lane_config.field.media_type;
    fw_config->UnreliableLos         = lane_config.field.unreliable_los;
    fw_config->Cl72AutoPolEn         = lane_config.field.cl72_auto_polarity_en;
    fw_config->ScramblingDisable     = lane_config.field.scrambling_dis;
    fw_config->Cl72RestTO            = lane_config.field.cl72_restart_timeout_en;
    fw_config->ForceES     = lane_config.field.force_es;
    fw_config->ForceNS      = lane_config.field.force_ns;
    fw_config->LinkPartnerPrecoderEn     = lane_config.field.lp_has_prec_en;
    fw_config->ForcePAM4Mode         = lane_config.field.force_pam4_mode;
    fw_config->ForceNRZMode          = lane_config.field.force_nrz_mode;

    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_pam4_tx_set(const plp_aperta_phymod_phy_access_t* phy, const phymod_pam4_tx_t* tx)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, i;
    int16_t main_tap;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 1));

    main_tap = tx->main & 0x00ff;

    for (i = 0; i < num_lane; i++) {
        phy_copy.access.lane_mask = 1 << (start_lane + i);
        /*next check 3 tap mode or 6 tap mode */
        if (tx->serdes_tx_tap_mode == phymodTxTapModeNRZ_LP_3TAP ||
                tx->serdes_tx_tap_mode == phymodTxTapModePAM4_LP_3TAP ) {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_tsc_apply_txfir_cfg(&phy_copy,
                                                 (tx->serdes_tx_tap_mode == phymodTxTapModeNRZ_LP_3TAP) ? NRZ_LP_3TAP : PAM4_LP_3TAP,
                                                 0,
                                                 tx->pre,
                                                 main_tap,
                                                 tx->post,
                                                 0,
                                                 0));
        } else {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_tsc_apply_txfir_cfg(&phy_copy,
                                                 (tx->serdes_tx_tap_mode == phymodTxTapModeNRZ_6TAP) ? NRZ_6TAP: PAM4_6TAP,
                                                 tx->pre2,
                                                 tx->pre,
                                                 main_tap,
                                                 tx->post,
                                                 tx->post2,
                                                 tx->post3));
        }
    }

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 0));

    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_media_type_tx_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_media_typed_t media, plp_aperta_phymod_tx_t* tx)
{

    return PHYMOD_E_NONE;


}

/*
 * set lane swapping for core
 */

int plp_aperta_blackhawk_core_lane_map_set(const plp_aperta_phymod_core_access_t* core, const plp_aperta_phymod_lane_map_t* lane_map)
{
    plp_aperta_phymod_phy_access_t  phy_copy;
    uint8_t pmd_tx_addr[8], pmd_rx_addr[8];
    int i = 0;
    uint8_t tmp_phy_lane;

    PHYMOD_MEMCPY(&phy_copy, core, sizeof(phy_copy));
    phy_copy.access.lane_mask = 0x1;

    /*next get the lane map into serdes spi format */
    for (i = 0; i < BLACKHAWK_NOF_LANES_IN_CORE; i++) {
        tmp_phy_lane = (uint8_t) lane_map->lane_map_tx[i];
        pmd_tx_addr[tmp_phy_lane] = i;
        tmp_phy_lane = (uint8_t) lane_map->lane_map_rx[i];
        pmd_rx_addr[tmp_phy_lane] = i;
    }

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_map_lanes(&phy_copy,
                                   BLACKHAWK_NOF_LANES_IN_CORE,
                                   pmd_tx_addr,
                                   pmd_rx_addr));

    return PHYMOD_E_NONE;
}

int _plp_aperta_blackhawk_phy_firmware_lane_config_set(const plp_aperta_phymod_phy_access_t* phy, phymod_firmware_lane_config_t fw_config)
{
    uint32_t is_warm_boot;
    struct blackhawk_tsc_uc_lane_config_st serdes_firmware_config;
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_MEMSET(&serdes_firmware_config, 0x0, sizeof(serdes_firmware_config));
    serdes_firmware_config.field.lane_cfg_from_pcs      = fw_config.LaneConfigFromPCS;
    serdes_firmware_config.field.an_enabled             = fw_config.AnEnabled;
    serdes_firmware_config.field.dfe_on                 = fw_config.DfeOn;
    serdes_firmware_config.field.force_brdfe_on         = fw_config.ForceBrDfe;
    /* serdes_firmware_config.field.cl72_emulation_en = fw_config.Cl72Enable; */
    serdes_firmware_config.field.scrambling_dis         = fw_config.ScramblingDisable;
    serdes_firmware_config.field.unreliable_los         = fw_config.UnreliableLos;
    serdes_firmware_config.field.media_type             = fw_config.MediaType;
    serdes_firmware_config.field.dfe_lp_mode            = fw_config.LpDfeOn;
    serdes_firmware_config.field.cl72_auto_polarity_en  = fw_config.Cl72AutoPolEn;
    serdes_firmware_config.field.cl72_restart_timeout_en = fw_config.Cl72RestTO;
    serdes_firmware_config.field.force_es               = fw_config.ForceES;
    serdes_firmware_config.field.force_ns               = fw_config.ForceNS;
    serdes_firmware_config.field.force_nrz_mode         = fw_config.ForceNRZMode;
    serdes_firmware_config.field.force_pam4_mode        = fw_config.ForcePAM4Mode;
    serdes_firmware_config.field.lp_has_prec_en         = fw_config.LinkPartnerPrecoderEn;

    PHYMOD_IF_ERR_RETURN(PHYMOD_IS_WRITE_DISABLED(&phy->access, &is_warm_boot));

    if(!is_warm_boot) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_set_uc_lane_cfg(&phy_copy, serdes_firmware_config));
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_firmware_lane_config_set(const plp_aperta_phymod_phy_access_t* phy, phymod_firmware_lane_config_t fw_config)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, i;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 1));

    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 1 << (start_lane + i);
        PHYMOD_IF_ERR_RETURN
            (_plp_aperta_blackhawk_phy_firmware_lane_config_set(&phy_copy, fw_config));
    }

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 0));

    return PHYMOD_E_NONE;
}


/* reset rx sequencer
 * flags - unused parameter
 */
int plp_aperta_blackhawk_phy_rx_restart(const plp_aperta_phymod_phy_access_t* phy)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, i;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 1));

    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 1 << (start_lane + i);
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_rx_restart(&phy_copy, 1));
    }

    PHYMOD_USLEEP(1000);

    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 1 << (start_lane + i);
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_rx_restart(&phy_copy, 0));
    }

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 0));

    return PHYMOD_E_NONE;
}


int plp_aperta_blackhawk_phy_polarity_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_polarity_t* polarity)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tx_rx_polarity_set(&phy_copy, polarity->tx_polarity, polarity->rx_polarity));

    return PHYMOD_E_NONE;
}


int plp_aperta_blackhawk_phy_polarity_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_polarity_t* polarity)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tx_rx_polarity_get(&phy_copy, &polarity->tx_polarity, &polarity->rx_polarity));

    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_pam4_tx_get(const plp_aperta_phymod_phy_access_t* phy, phymod_pam4_tx_t* tx)
{
    uint8_t pmd_tx_tap_mode;
    int16_t val;
    uint16_t tx_tap_nrz_mode = 0;
    plp_aperta_phymod_phy_access_t phy_copy;
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));


    /* read current tx tap mode */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_tx_tap_mode_get(&phy_copy, &pmd_tx_tap_mode));

    /*read current tx NRZ mode control info */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_tx_nrz_mode_get(&phy_copy, &tx_tap_nrz_mode));

    if (pmd_tx_tap_mode == 0) {
        /* 3 tap mode */
        if (tx_tap_nrz_mode) {
            tx->serdes_tx_tap_mode = phymodTxTapModeNRZ_LP_3TAP;
        } else {
            tx->serdes_tx_tap_mode = phymodTxTapModePAM4_LP_3TAP;
        }
    } else {
        if (tx_tap_nrz_mode) {
            tx->serdes_tx_tap_mode = phymodTxTapModeNRZ_6TAP;
        } else {
            tx->serdes_tx_tap_mode = phymodTxTapModePAM4_6TAP;
        }
    }

    /*next check 3 tap mode or 6 tap mode */
    if (tx->serdes_tx_tap_mode == phymodTxTapModeNRZ_LP_3TAP) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_read_tx_afe(&phy_copy, TX_AFE_TAP0, &val));
            tx->pre = (int8_t) val;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_read_tx_afe(&phy_copy, TX_AFE_TAP1, &val));
            tx->main = (int8_t) val;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_read_tx_afe(&phy_copy, TX_AFE_TAP2, &val));
            tx->post = (int8_t) val;
            tx->pre2 = 0;
            tx->post2 = 0;
            tx->post3 = 0;
    } else {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_read_tx_afe(&phy_copy, TX_AFE_TAP0, &val));
            tx->pre2 = (int8_t) val;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_read_tx_afe(&phy_copy, TX_AFE_TAP1, &val));
            tx->pre = (int8_t) val;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_read_tx_afe(&phy_copy, TX_AFE_TAP2, &val));
            tx->main = (int8_t) val;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_read_tx_afe(&phy_copy, TX_AFE_TAP3, &val));
            tx->post = (int8_t) val;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_read_tx_afe(&phy_copy, TX_AFE_TAP4, &val));
            tx->post2 = (int8_t) val;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_read_tx_afe(&phy_copy, TX_AFE_TAP5, &val));
            tx->post3 = (int8_t) val;
    }


    return PHYMOD_E_NONE;
}



int plp_aperta_blackhawk_phy_tx_override_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_tx_override_t* tx_override)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, i;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 1));

    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 1 << (start_lane + i);
        PHYMOD_IF_ERR_RETURN
             (plp_aperta_blackhawk_tsc_tx_pi_freq_override(&phy_copy,
                                                 tx_override->phase_interpolator.enable,
                                                 tx_override->phase_interpolator.value));
    }

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 0));

    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_tx_override_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_tx_override_t* tx_override)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    int16_t value;
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tx_pi_control_get(&phy_copy, &value));

    tx_override->phase_interpolator.value = (int32_t) value;

    return PHYMOD_E_NONE;
}


int plp_aperta_blackhawk_phy_rx_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_rx_t* rx)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, i;
    uint8_t uc_lane_stopped;
    phymod_phy_signalling_method_t signalling_mode;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /* next read the PAM4 mode or  not */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_signalling_mode_status_get(&phy_copy, &signalling_mode));

    for (i = 0; i < num_lane; i++) {
        int j = 0;
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 1 << (start_lane + i);
        /* first check if uc lane is stopped already */
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_stop_uc_lane_status(&phy_copy, &uc_lane_stopped));
        if (!uc_lane_stopped) {
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_stop_rx_adaptation(&phy_copy, 1));
        }
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_VGA, (int8_t) rx->vga.value));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_PF, (int8_t) rx->peaking_filter.value));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_PF2, (int8_t) rx->low_freq_peaking_filter.value));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_PF3, (int8_t) rx->high_freq_peaking_filter.value));

        for (j = 0 ; j < rx->num_of_dfe_taps ; j++){
            switch (j) {
                case 0:
                    if (signalling_mode == phymodSignallingMethodNRZ) {
                        PHYMOD_IF_ERR_RETURN
                            (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE1, (int8_t) rx->dfe[j].value));
                    } else {
                        if (rx->dfe[0].enable) {
                            PHYMOD_DEBUG_ERROR(("ERROR :: DFE1 is not supported on PAM4 mode \n"));
                            return PHYMOD_E_PARAM;
                        }
                    }
                    break;
                case 1:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE2, (int8_t) rx->dfe[j].value));
                    break;
                case 2:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE3, (int8_t) rx->dfe[j].value));
                    break;
                case 3:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE4, (int8_t) rx->dfe[j].value));
                    break;
                case 4:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE5, (int8_t) rx->dfe[j].value));
                    break;
                case 5:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE6, (int8_t) rx->dfe[j].value));
                    break;
                case 6:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE7, (int8_t) rx->dfe[j].value));
                    break;
                case 7:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE8, (int8_t) rx->dfe[j].value));
                    break;
                case 8:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE9, (int8_t) rx->dfe[j].value));
                    break;
                case 9:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE10, (int8_t) rx->dfe[j].value));
                    break;
                case 10:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE11,(int8_t)  rx->dfe[j].value));
                    break;
                case 11:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE12, (int8_t) rx->dfe[j].value));
                    break;
                case 12:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE13, (int8_t) rx->dfe[j].value));
                    break;
                case 13:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE14, (int8_t) rx->dfe[j].value));
                    break;
                default:
                    return PHYMOD_E_PARAM;
            }
        }
    }

    return PHYMOD_E_NONE;
}


int plp_aperta_blackhawk_phy_rx_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_rx_t* rx)
{
    int j;
    int8_t val;
    plp_aperta_phymod_phy_access_t phy_copy;
    phymod_phy_signalling_method_t signalling_mode;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    /* next read the PAM4 mode or  not */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_signalling_mode_status_get(&phy_copy, &signalling_mode));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_VGA, &val));
    rx->vga.value = val;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_PF, &val));
    rx->peaking_filter.value = val;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_PF2, &val));
    rx->low_freq_peaking_filter.value = val;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_PF3, &val));
    rx->high_freq_peaking_filter.value = val;

    rx->num_of_dfe_taps = 14;

    for (j = 0 ; j < rx->num_of_dfe_taps ; j++){
        switch (j) {
            case 0:
                if (signalling_mode == phymodSignallingMethodNRZ) {
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE1, &val));
                    rx->dfe[0].enable = 1;
                } else {
                    /* for PAM4 mode, DFE1 tap is not supported */
                    rx->dfe[0].enable = 0;
                }
                break;
            case 1:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE2, &val));
                break;
            case 2:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE3, &val));
                break;
            case 3:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE4, &val));
                break;
            case 4:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE5, &val));
                break;
            case 5:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE6, &val));
                break;
            case 6:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE7, &val));
                break;
            case 7:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE8, &val));
                break;
            case 8:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE9, &val));
                break;
            case 9:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE10, &val));
                break;
            case 10:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE11, &val));
                break;
            case 11:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE12, &val));
                break;
            case 12:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE13, &val));
                break;
            case 13:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE14, &val));
                break;
            default:
                return PHYMOD_E_PARAM;
        }
        rx->dfe[j].value = val;
    }

    for (j = 1 ; j < rx->num_of_dfe_taps ; j++){
        rx->dfe[j].enable = 1;
    }
    rx->vga.enable = 1;
    rx->peaking_filter.enable = 1;
    rx->low_freq_peaking_filter.enable = 1;
    rx->high_freq_peaking_filter.enable = 1;

    return PHYMOD_E_NONE;
}


int plp_aperta_blackhawk_phy_reset_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_phy_reset_t* reset)
{

    return PHYMOD_E_UNAVAIL;
}


int plp_aperta_blackhawk_phy_reset_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_phy_reset_t* reset)
{

    return PHYMOD_E_UNAVAIL;

}


int plp_aperta_blackhawk_phy_power_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_phy_power_t* power)
{
    plp_aperta_phymod_phy_access_t pm_phy_copy;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    if ((power->tx == phymodPowerOff) && (power->rx == phymodPowerOff)) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_lane_pwrdn(&pm_phy_copy, PWRDN));
    } else if ((power->tx == phymodPowerOn) && (power->rx == phymodPowerOn)) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_lane_pwrdn(&pm_phy_copy, PWR_ON));
    } else if ((power->tx == phymodPowerOff) && (power->rx == phymodPowerOn)) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_lane_pwrdn(&pm_phy_copy, PWRDN_TX));
    } else if ((power->tx == phymodPowerOn) && (power->rx == phymodPowerOff)) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_lane_pwrdn(&pm_phy_copy, PWRDN_RX));
    } else {
       return PHYMOD_E_CONFIG;
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_power_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_phy_power_t* power)
{
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta_blackhawk_phy_speed_config_set(const plp_aperta_phymod_phy_access_t* phy,
                                   const phymod_phy_speed_config_t* speed_config,
                                   const phymod_phy_pll_state_t* old_pll_state,
                                   phymod_phy_pll_state_t* new_pll_state)
{
    uint32_t pll_0_is_free, pll_1_is_free;
    uint32_t pll_0_is_pwrdn, pll_1_is_pwrdn;
    plp_aperta_phymod_phy_access_t pm_phy_copy;
    uint32_t  pll_0_div = 0, pll_1_div = 0, request_pll_div, pll_index = 0;
    uint32_t is_pam4, osr_mode;
    uint32_t loss_in_db;
    int i, start_lane, num_lane;
    phymod_firmware_lane_config_t firmware_lane_config;
    plp_aperta_phymod_firmware_core_config_t firmware_core_config;

    firmware_lane_config = speed_config->pmd_lane_config;

    PHYMOD_MEMSET(&firmware_core_config, 0x0, sizeof(firmware_core_config));

    /* first copy the PLL state */
    *new_pll_state = *old_pll_state;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    /* first check if any PLL is free */
    if (old_pll_state->pll0_lanes_bitmap) {
        pll_0_is_free = 0;
    } else {
        pll_0_is_free = 1;
    }

    if (old_pll_state->pll1_lanes_bitmap) {
        pll_1_is_free = 0;
    } else {
        pll_1_is_free = 1;
    }

    /* Check if PLLs are power down */
    pm_phy_copy.access.pll_idx = 0;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_pll_pwrdn_get(&pm_phy_copy, &pll_0_is_pwrdn));

    pm_phy_copy.access.pll_idx = 1;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_pll_pwrdn_get(&pm_phy_copy, &pll_1_is_pwrdn));

    /* get the VCO if PLL0 is active */
    if (!pll_0_is_pwrdn) {
        pm_phy_copy.access.pll_idx = 0;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_INTERNAL_read_pll_div(&pm_phy_copy, &pll_0_div));
    }

    if (!pll_1_is_pwrdn) {
        pm_phy_copy.access.pll_idx = 1;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_INTERNAL_read_pll_div(&pm_phy_copy, &pll_1_div));
    }

    /* next check the request speed VCO */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_speed_config_get(speed_config->data_rate, 1, &request_pll_div, &is_pam4, &osr_mode));

    /* first assert the ln dp reset */
    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset(&pm_phy_copy, 1));

    /* first to check if any PLL is free and needs to configured */
    if (  ((pll_0_is_free) && (pll_1_is_free)) ||
          ((pll_0_is_free) && (pll_1_div != request_pll_div)) ||
          ((pll_1_is_free) && (pll_0_div != request_pll_div)) ) {
        /*choose the the right PLL to config */
        if (PHYMOD_SPEED_CONFIG_ONLY_PLL0_IS_ACTIVE_GET(speed_config))
        {
            pll_index = 0;
        }
        else
        {
            pll_index = (pll_1_is_free)? 1 : 0;
        }
        pm_phy_copy.access.pll_idx = pll_index;

        if (((pll_index == 0) && pll_0_is_pwrdn) || ((pll_index == 1) && pll_1_is_pwrdn)) {
            /* Power up PLL */
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_tsc_core_pwrdn(&pm_phy_copy, PWR_ON));
        }

        /*toggle core dp reset */
        pm_phy_copy.access.lane_mask = 0x1;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_core_dp_reset(&pm_phy_copy, 1));

        /*config the PLL to the requested VCO */
        PHYMOD_IF_ERR_RETURN
             (plp_aperta_blackhawk_tsc_configure_pll_refclk_div(&pm_phy_copy,
                                                       BLACKHAWK_TSC_PLL_REFCLK_312P5MHZ,
                                                       request_pll_div));

        /* release core soft reset */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_core_dp_reset(&pm_phy_copy, 0));

    } else if ( ((pll_0_is_free) && (pll_1_div == request_pll_div)) ||
            ((pll_1_is_free) && (pll_0_div == request_pll_div)) ) {
        pll_index = (pll_0_div == request_pll_div)? 0 : 1;
    } else if ((!pll_0_is_free) && (!pll_1_is_free)) {
        /*next if both pll0 and ppl1 are active and the new speed can be
        supported with existing VCO */
        if ((pll_0_div == request_pll_div) || (pll_1_div == request_pll_div)) {

            pll_index = (pll_0_div == request_pll_div)? 0 : 1;
        }
    } else {
        /*this speed request can not be configured */
        PHYMOD_DEBUG_ERROR(("ERROR :: this speed can not be configured \n"));
        return PHYMOD_E_CONFIG;
    }

    /* choose the right pll index for the port */
    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_lane_pll_selection_set(&pm_phy_copy, pll_index));
    }

    /* config oversample for each lane */
    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_osr_mode_set(&pm_phy_copy, osr_mode));
    }

    /*next need to set certain firmware lane config to be zero*/
    firmware_lane_config.LaneConfigFromPCS = 0;
    firmware_lane_config.AnEnabled = 0;

    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane_mask = 0x1 << (i + start_lane);
        PHYMOD_IF_ERR_RETURN
             (_plp_aperta_blackhawk_phy_firmware_lane_config_set(&pm_phy_copy, firmware_lane_config));
    }

    /* if the PAM4 mode, need to program the channel loss. In NRZ mode it is zeroed. */
    loss_in_db = firmware_lane_config.ForcePAM4Mode? speed_config->PAM4_channel_loss : 0;
    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane_mask = 0x1 << (i + start_lane);
        PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_channel_loss_set(&pm_phy_copy, loss_in_db));
    }

    /* next need to enable/disable link training based on the input */
    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane_mask = 0x1 << (i + start_lane);
        PHYMOD_IF_ERR_RETURN
             (plp_aperta_blackhawk_clause72_control(&pm_phy_copy, speed_config->linkTraining));
    }

    /* next release the ln dp reset */
    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset(&pm_phy_copy, 0));

    /* first clear the current lane _mask from both */
    new_pll_state->pll1_lanes_bitmap &= ~(phy->access.lane_mask);
    new_pll_state->pll0_lanes_bitmap &= ~(phy->access.lane_mask);

    /* need to update the pll_state */
    if (pll_index) {
        new_pll_state->pll1_lanes_bitmap |= phy->access.lane_mask;
    } else {
        new_pll_state->pll0_lanes_bitmap |= phy->access.lane_mask;
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_speed_config_get(const plp_aperta_phymod_phy_access_t* phy, phymod_phy_speed_config_t* speed_config)
{
    int osr_mode;
    uint32_t pll_div, vco_freq_khz, cl72_enable, channel_loss;
    uint32_t pll_index;
    plp_aperta_phymod_phy_access_t phy_copy;
    phymod_firmware_lane_config_t firmware_lane_config;
    int start_lane, num_lane;


    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    phy_copy.access.lane_mask = 0x1 << start_lane;

    /*first figure out which pll the current port is using */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_pll_selection_get(&phy_copy, &pll_index));

    phy_copy.access.pll_idx = pll_index;
     /* get the PLL div from HW */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_INTERNAL_read_pll_div(&phy_copy, &pll_div));

    /* for now only 321.5H ref clock is used for  DNX device */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_get_vco_from_refclk_div(&phy_copy, RAMON_REF_CLOCK_HZ, pll_div, &vco_freq_khz, 0));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_osr_mode_get(&phy_copy, &osr_mode));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_phy_firmware_lane_config_get(&phy_copy, &firmware_lane_config));

    speed_config->pmd_lane_config.AnEnabled          = firmware_lane_config.AnEnabled;
    speed_config->pmd_lane_config.Cl72AutoPolEn      = firmware_lane_config.Cl72AutoPolEn;
    speed_config->pmd_lane_config.Cl72RestTO         = firmware_lane_config.Cl72RestTO;
    speed_config->pmd_lane_config.DfeOn              = firmware_lane_config.DfeOn;
    speed_config->pmd_lane_config.ForceBrDfe         = firmware_lane_config.ForceBrDfe;
    speed_config->pmd_lane_config.ForceES  = firmware_lane_config.ForceES;
    speed_config->pmd_lane_config.ForceNS   = firmware_lane_config.ForceNS;
    speed_config->pmd_lane_config.ForceNRZMode       = firmware_lane_config.ForceNRZMode;
    speed_config->pmd_lane_config.ForcePAM4Mode      = firmware_lane_config.ForcePAM4Mode;
    speed_config->pmd_lane_config.LaneConfigFromPCS  = firmware_lane_config.LaneConfigFromPCS;
    speed_config->pmd_lane_config.LpDfeOn            = firmware_lane_config.LpDfeOn;
    speed_config->pmd_lane_config.LinkPartnerPrecoderEn  = firmware_lane_config.LinkPartnerPrecoderEn;
    speed_config->pmd_lane_config.MediaType          = firmware_lane_config.MediaType;
    speed_config->pmd_lane_config.ScramblingDisable  = firmware_lane_config.ScramblingDisable;
    speed_config->pmd_lane_config.UnreliableLos      = firmware_lane_config.UnreliableLos;

    /* next get the cl72 enable status */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_clause72_control_get(&phy_copy, &cl72_enable));
    speed_config->linkTraining = cl72_enable;


    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_channel_loss_get(&phy_copy, &channel_loss));
    speed_config->PAM4_channel_loss = channel_loss;

    /* next check if PAM4 mode enabled */
    if (firmware_lane_config.ForcePAM4Mode) {
        speed_config->data_rate = (vco_freq_khz  << 1) / 1000;
    } else {
        /* to get the over sample value */
        if (osr_mode == 0) {
            speed_config->data_rate = (vco_freq_khz) / 1000;
        } else if(osr_mode == 1) {
            speed_config->data_rate = (vco_freq_khz  >> 1) / 1000;
        } else if (osr_mode == 2) {
            speed_config->data_rate = (vco_freq_khz  >> 2) / 1000;
        }
    }

    return PHYMOD_E_NONE;
}


int plp_aperta_blackhawk_phy_cl72_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t cl72_en)
{
    int start_lane, num_lane, i;
    uint32_t lane_reset;
    plp_aperta_phymod_phy_access_t pm_phy_copy;
    phymod_firmware_lane_config_t firmware_lane_config;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    /*first check if lane is in reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset_get(&pm_phy_copy, &lane_reset));

    /* if lane is not in reset, then reset the lane first */
    if (!lane_reset) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_lane_soft_reset(&pm_phy_copy, 1));
    }

    /* next need to clear both force ER and NR config on the firmware lane config side
    if link training enable is passed*/
    if (cl72_en) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_phy_firmware_lane_config_get(phy, &firmware_lane_config));

        firmware_lane_config.ForceNS = 0;
        firmware_lane_config.ForceES = 0;

         PHYMOD_IF_ERR_RETURN
            (_plp_aperta_blackhawk_phy_firmware_lane_config_set(phy, firmware_lane_config));
    }
    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_clause72_control(&pm_phy_copy, cl72_en));
    }

    /* release the ln dp reset */
    if (!lane_reset) {
        PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_lane_soft_reset(&pm_phy_copy, 0));
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_cl72_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* cl72_en)
{
    plp_aperta_phymod_phy_access_t pm_phy_copy;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_clause72_control_get(&pm_phy_copy, cl72_en));

    return PHYMOD_E_NONE;
}


int plp_aperta_blackhawk_phy_cl72_status_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_cl72_status_t* status)
{
    int i;
    uint32_t tmp_status;
    int start_lane, num_lane;
    plp_aperta_phymod_phy_access_t phy_copy;


    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    status->locked = 1;

    /* next figure out the lane num and start_lane based on the input */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 0x1 << (i + start_lane);
        tmp_status = 1;
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_pmd_cl72_receiver_status(&phy_copy, &tmp_status));
        if (tmp_status == 0) {
            status->locked = 0;
            return PHYMOD_E_NONE;
        }
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_loopback_set(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_loopback_mode_t loopback, uint32_t enable)
{
    int i;
    int start_lane, num_lane;
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /* next figure out the lane num and start_lane based on the input */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    switch (loopback) {
    case phymodLoopbackGlobal :
    case phymodLoopbackGlobalPMD :
        for (i = 0; i < num_lane; i++) {
            if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
                continue;
            }
            phy_copy.access.lane_mask = 0x1 << (i + start_lane);
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_dig_lpbk(&phy_copy, (uint8_t) enable));
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_pmd_force_signal_detect(&phy_copy,  (int) enable, (int) enable));
        }
        break;
    case phymodLoopbackRemotePMD :
        for (i = 0; i < num_lane; i++) {
            if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
                continue;
            }
            phy_copy.access.lane_mask = 0x1 << (i + start_lane);
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_rmt_lpbk(&phy_copy, (uint8_t)enable));
        }
        break;
    case phymodLoopbackRemotePCS :
        PHYMOD_DEBUG_ERROR(("ERROR :: this mode is not supported\n"));
        break;
    default :
        break;
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_loopback_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_loopback_mode_t loopback, uint32_t* enable)
{
    int start_lane, num_lane;
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /*next figure out the lane num and start_lane based on the input*/
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    switch (loopback) {
    case phymodLoopbackGlobal :
        PHYMOD_DEBUG_ERROR(("ERROR :: this mode is not supported\n"));
        break;
    case phymodLoopbackGlobalPMD :
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_dig_lpbk_get(&phy_copy, enable));
        break;
    case phymodLoopbackRemotePMD :
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_rmt_lpbk_get(&phy_copy, enable));
        break;
    case phymodLoopbackRemotePCS :
        PHYMOD_DEBUG_ERROR(("ERROR :: this mode is not supported\n"));
        break;
    default :
        break;
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_core_init(const plp_aperta_phymod_core_access_t* core, const plp_aperta_phymod_core_init_config_t* init_config, const plp_aperta_phymod_core_status_t* core_status)
{
    int rv;
    int lane = 0;
    int pll_index = 0;
    plp_aperta_phymod_phy_access_t phy_access, phy_access_copy;
    plp_aperta_phymod_phy_access_t  phy_copy;
    plp_aperta_phymod_core_access_t  core_copy;
    plp_aperta_phymod_firmware_core_config_t  firmware_core_config_tmp;
    uint32_t uc_enable = 0;
    plp_aperta_phymod_polarity_t tmp_pol;
    ucode_info_t ucode;

    BLACKHAWK_CORE_TO_PHY_ACCESS(&phy_access, core);
    PHYMOD_MEMSET(&tmp_pol, 0x0, sizeof(tmp_pol));
    phy_access_copy = phy_access;
    PHYMOD_MEMCPY(&phy_copy, core, sizeof(phy_copy));
    PHYMOD_MEMCPY(&core_copy, core, sizeof(core_copy));
    core_copy.access.lane_mask = 0x1;
    phy_copy.access.lane_mask = 0x1;
    phy_access_copy = phy_access;
    phy_access_copy.access = core->access;
    phy_access_copy.access.lane_mask = 0x1;
    phy_access_copy.type = core->type;

    /* 1. De-assert PMD core power and core data path reset */
    if(!PHYMOD_CORE_INIT_F_RESUME_AFTER_FW_LOAD_GET(init_config)) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_pmd_reset_seq(&phy_copy, core_status->pmd_active));

        /*wait until com clk and ref clk is stable */
        PHYMOD_USLEEP(1000);

        /* De-assert PMD lane reset */

        for (lane = 0; lane < BLACKHAWK_NOF_LANES_IN_CORE; lane++) {
            phy_access_copy.access.lane_mask = 1 << lane;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_lane_hard_soft_reset_release(&phy_access_copy, 0));
        }


        for (lane = 0; lane < BLACKHAWK_NOF_LANES_IN_CORE; lane++) {
            phy_access_copy.access.lane_mask = 1 << lane;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_lane_hard_soft_reset_release(&phy_access_copy, 1));
        }


        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_uc_active_get(&phy_access, &uc_enable));
        if (uc_enable) return PHYMOD_E_NONE;

        /* 2. Set the heart beat, default is for 156.25M */
        if (init_config->interface.ref_clock != phymodRefClk156Mhz) {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_refclk_set(&phy_copy, init_config->interface.ref_clock));
        }

        /*now config the lane mapping*/
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_core_lane_map_set(&core_copy, &init_config->lane_map));
    }

    /*  set the micro stack size */
    ucode.stack_size = plp_aperta_blackhawk_ucode_stack_size;
    ucode.ucode_size = plp_aperta_blackhawk_ucode_len;
    PHYMOD_IF_ERR_RETURN
    (plp_aperta_blackhawk_tsc_uc_reset_with_info(&phy_copy , 1, ucode));

    rv = _plp_aperta_blackhawk_core_firmware_load(&core_copy, init_config);

    if (rv != PHYMOD_E_NONE) {
        PHYMOD_DEBUG_ERROR(("devad 0x%"PRIx32" lane 0x%"PRIx32": UC firmware-load failed\n", core->access.addr, core->access.lane_mask));
        PHYMOD_IF_ERR_RETURN(rv);
    }

    if(PHYMOD_CORE_INIT_F_UNTIL_FW_LOAD_GET(init_config)) {
        return PHYMOD_E_NONE;
    }

#ifndef BLACKHAWK_PMD_CRC_UCODE
        if(PHYMOD_CORE_INIT_F_FIRMWARE_LOAD_VERIFY_GET(init_config)) {
            rv = plp_aperta_blackhawk_tsc_ucode_load_verify(&phy_copy, (uint8_t *) &plp_aperta_blackhawk_ucode, plp_aperta_blackhawk_ucode_len);

            if (rv != PHYMOD_E_NONE) {
                PHYMOD_DEBUG_ERROR(("devad 0x%x lane 0x%x: UC load-verify failed\n", core->access.addr, core->access.lane_mask));
                PHYMOD_IF_ERR_RETURN(rv);
            }
        }
#endif



    /*next we need to check if the load is correct or not */
    if(init_config->firmware_load_method != phymodFirmwareLoadMethodNone) {
         /*next we need to set the uc active and release uc */
         PHYMOD_IF_ERR_RETURN
         (plp_aperta_blackhawk_uc_active_set(&phy_copy ,1));

         /*release the uc reset */
         PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_uc_reset_with_info(&phy_copy , 0, ucode));

         PHYMOD_IF_ERR_RETURN (plp_aperta_blackhawk_tsc_wait_uc_active(&phy_copy));

        for (lane = 0; lane < BLACKHAWK_NOF_LANES_IN_CORE; lane++) {
            phy_access_copy.access.lane_mask = 1 << lane;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_pmd_ln_h_rstb_pkill_override( &phy_access_copy, 0x1));
        }

        /* we need to wait at least 10ms for the uc to settle */
        PHYMOD_USLEEP(10000);

        /* 7. Initialize software information table for the micro */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(&phy_copy));


       /* poll the ready bit in 10 ms */
#ifndef BLACKHAWK_PMD_CRC_UCODE
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_INTERNAL_poll_uc_dsc_ready_for_cmd_equals_1(&phy_access_copy, 1, CMD_READ_DIAG_DATA_BYTE));

#else
        if(PHYMOD_CORE_INIT_F_FIRMWARE_LOAD_VERIFY_GET(init_config)) {
            rv = plp_aperta_blackhawk_tsc_ucode_crc_verify(&phy_copy, plp_aperta_blackhawk_ucode_len, plp_aperta_blackhawk_ucode_crc);
            if (rv != PHYMOD_E_NONE) {
                PHYMOD_DEBUG_ERROR(("devad 0x%"PRIx32" lane 0x%"PRIx32": UC load-verify failed\n", core->access.addr, core->access.lane_mask));
                PHYMOD_IF_ERR_RETURN(rv);
            }
        }
#endif
        for (lane = 0; lane < BLACKHAWK_NOF_LANES_IN_CORE; lane++) {
            phy_access_copy.access.lane_mask = 1 << lane;
           PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_pmd_ln_h_rstb_pkill_override( &phy_access_copy, 0x0));

        }
    }
    else
    {
        /*PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_firmware_load_none_init_blackhawk_tsc_info(&phy_copy));*/
    }

    /* AFE/PLL config */
    if (init_config->afe_pll.afe_pll_change_default) {
        for (pll_index = 0; pll_index < 2; pll_index++) {
            core_copy.access.pll_idx = pll_index;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_afe_pll_reg_set(&phy_copy, &init_config->afe_pll));
        }
    }

    /* program the rx/tx polarity */
    for (lane = 0; lane < BLACKHAWK_NOF_LANES_IN_CORE; lane++) {
        phy_access_copy.access.lane_mask = 1 << lane;
        tmp_pol.tx_polarity = (init_config->polarity_map.tx_polarity) >> lane & 0x1;
        tmp_pol.rx_polarity = (init_config->polarity_map.rx_polarity) >> lane & 0x1;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_phy_polarity_set(&phy_access_copy, &tmp_pol));
        /* clear the tmp vairiable */
        PHYMOD_MEMSET(&tmp_pol, 0x0, sizeof(tmp_pol));

    }

    /* default PLL config for both PLL0 and PLL1 plldiv*/
    /* for now PLL0 will be set to 20.625G and PLL1 will be set to 25.78125G */
    core_copy.access.pll_idx = 0;
    PHYMOD_IF_ERR_RETURN
         (plp_aperta_blackhawk_tsc_configure_pll_refclk_div(&phy_copy,
                                                   BLACKHAWK_TSC_PLL_REFCLK_312P5MHZ,
                                                   init_config->pll0_div_init_value));
    core_copy.access.pll_idx = 1;
    PHYMOD_IF_ERR_RETURN
         (plp_aperta_blackhawk_tsc_configure_pll_refclk_div(&phy_copy,
                                                   BLACKHAWK_TSC_PLL_REFCLK_312P5MHZ,
                                                   init_config->pll1_div_init_value));

    /*don't overide the fw that set in config set if not specified*/
    firmware_core_config_tmp = init_config->firmware_core_config;
    firmware_core_config_tmp.CoreConfigFromPCS = 0;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_phy_firmware_core_config_set(&phy_access_copy, firmware_core_config_tmp));


    /* release core soft reset for both PLL's */
    core_copy.access.lane_mask = 0x1;
    core_copy.access.pll_idx = 0;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_core_dp_reset(&phy_copy, 0));
    core_copy.access.pll_idx = 1;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_core_dp_reset(&phy_copy, 0));

    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_init(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_phy_init_config_t* init_config)
{
    const plp_aperta_phymod_access_t *pm_acc = &phy->access;
    plp_aperta_phymod_phy_access_t pm_phy_copy;
    int start_lane, num_lane, i;
    int lane_bkup;
    phymod_firmware_lane_config_t firmware_lane_config;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(pm_acc, &start_lane, &num_lane));
    /*per lane based  dp reset release */

    lane_bkup = pm_phy_copy.access.lane_mask;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset(&pm_phy_copy, 0));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset(&pm_phy_copy, 1));

    pm_phy_copy.access.lane_mask = lane_bkup;

    /* clearing all the lane config */
    PHYMOD_MEMSET(&firmware_lane_config, 0x0, sizeof(firmware_lane_config));

    /*for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane_mask = 0x1 << (i + start_lane);
        PHYMOD_IF_ERR_RETURN
            (blackhawk_phy_tx_set(&pm_phy_copy, &init_config->tx[i]));
    }*/

    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane_mask = 0x1 << (i + start_lane);
        PHYMOD_IF_ERR_RETURN
             (plp_aperta_blackhawk_phy_firmware_lane_config_set(&pm_phy_copy, firmware_lane_config));
    }

    return PHYMOD_E_NONE;

}


/* this function gives the PMD_RX_LOCK_STATUS */
int plp_aperta_blackhawk_phy_link_status_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* link_status)
{
    PHYMOD_DEBUG_ERROR(("This function is NOT SUPPORTED!! (plp_aperta_blackhawk_phy_link_status_get) \n"));
    return PHYMOD_E_UNAVAIL;
}


int plp_aperta_blackhawk_phy_rx_pmd_locked_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* pmd_lock)
{
    int start_lane, num_lane, i;
    plp_aperta_phymod_phy_access_t pm_phy_copy;
    uint8_t tmp_lock;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    *pmd_lock = 1;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_pmd_lock_status(&pm_phy_copy, &tmp_lock));
        *pmd_lock &= (uint32_t) tmp_lock;
    }
    return PHYMOD_E_NONE;

}

/* this function gives the PMD_RX_LOCK_STATUS */
int plp_aperta_blackhawk_phy_rx_signal_detect_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* signal_detect)
{
    int start_lane, num_lane, i;
    plp_aperta_phymod_phy_access_t pm_phy_copy;
    uint32_t tmp_detect;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    *signal_detect = 1;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_signal_detect(&pm_phy_copy, &tmp_detect));
        *signal_detect &= tmp_detect;
    }
    return PHYMOD_E_NONE;

}

int plp_aperta_blackhawk_phy_reg_read(const plp_aperta_phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t* val)
{
    PHYMOD_IF_ERR_RETURN(PHYMOD_BUS_READ(&phy->access, reg_addr, val));
    return PHYMOD_E_NONE;
}


int plp_aperta_blackhawk_phy_reg_write(const plp_aperta_phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t val)
{
    PHYMOD_IF_ERR_RETURN(PHYMOD_BUS_WRITE(&phy->access, reg_addr, val));
    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_tx_taps_default_get(const plp_aperta_phymod_phy_access_t* phy, phymod_phy_signalling_method_t mode, plp_aperta_phymod_tx_t* tx)
{
    /*always default to 6-taps mode : NA */
#if 0
    if (mode == phymodSignallingMethodNRZ) {
        tx->pre2 = (int8_t) 0;
        tx->pre =  (int8_t) -12;
        tx->main = (int8_t) 88;
        tx->post = (int8_t) -26;
        tx->post2 = (int8_t) 0;
        tx->post3 = (int8_t) 0;
    } else {
        tx->pre2 = (int8_t) 0;
        tx->pre =  (int8_t) -24;
        tx->main = (int8_t) 132;
        tx->post = (int8_t) -12;
        tx->post2 = (int8_t) 0;
        tx->post3 = (int8_t) 0;
    }
#endif
    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_lane_config_default_get(const plp_aperta_phymod_phy_access_t* phy, phymod_phy_signalling_method_t mode, phymod_firmware_lane_config_t* lane_config)
{
    /* default always assume backplane as the medium type and with dfe on */
    if (mode == phymodSignallingMethodNRZ) {
        lane_config->ForceNRZMode = 1;
        lane_config->ForcePAM4Mode = 0;
    } else {
        lane_config->ForceNRZMode = 0;
        lane_config->ForcePAM4Mode = 1;
    }
    lane_config->LaneConfigFromPCS = 0;
    lane_config->AnEnabled = 0;
    lane_config->DfeOn = 1;
    lane_config->LpDfeOn = 0;
    lane_config->ForceBrDfe = 0;
    lane_config->MediaType = 0;
    lane_config->ScramblingDisable = 0;
    lane_config->Cl72AutoPolEn = 0;
    lane_config->Cl72RestTO    = 0;
    lane_config->ForceES = 0;
    lane_config->ForceNS  = 1;
    lane_config->LinkPartnerPrecoderEn = 0;
    lane_config->UnreliableLos = 0;

    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_pll_multiplier_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* core_vco_pll_multiplier)
{
    plp_aperta_phymod_phy_access_t pm_phy_copy;
    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_INTERNAL_read_pll_div(&pm_phy_copy,  core_vco_pll_multiplier));
    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_firmware_load_info_get(const plp_aperta_phymod_phy_access_t* phy, phymod_firmware_load_info_t* info)
{
    info->ucode_ptr = &plp_aperta_blackhawk_ucode[0];
    info->ucode_len = plp_aperta_blackhawk_ucode_len;
    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_rx_adaptation_resume(const plp_aperta_phymod_phy_access_t* phy)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    uint8_t uc_lane_stopped;
    int start_lane, num_lane, i;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 1 << (start_lane + i);
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_stop_uc_lane_status(&phy_copy, &uc_lane_stopped));
        if (uc_lane_stopped) {
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_stop_rx_adaptation(&phy_copy, 0));
        }
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_tx_pam4_precoder_enable_set(const plp_aperta_phymod_phy_access_t* phy, int enable)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, i;

        PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 1));

    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 1 << (start_lane + i);
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_tx_pam4_precoder_enable_set(&phy_copy, enable));
    }

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 0));

    return PHYMOD_E_NONE;
}

/* Power down PLL*/
int plp_aperta_blackhawk_phy_pll_pwrdn(const plp_aperta_phymod_phy_access_t* phy, uint32_t pll_index, uint32_t pwrdn)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    uint32_t is_pwrdn = 0;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane_mask = 1 << 0;
    phy_copy.access.pll_idx = pll_index;

    PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_pll_pwrdn_get(&phy_copy, &is_pwrdn));
    if (is_pwrdn != pwrdn) {
        if (pwrdn) {
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_core_pwrdn(&phy_copy, PWRDN));
        } else {
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_core_pwrdn(&phy_copy, PWR_ON));
        }
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_tx_pam4_precoder_enable_get(const plp_aperta_phymod_phy_access_t* phy, int *enable)
{
    plp_aperta_phymod_phy_access_t pm_phy_copy;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_tx_pam4_precoder_enable_get(&pm_phy_copy, enable));
    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_tx_phase_lock_set(const plp_aperta_phymod_phy_access_t* phy, uint8_t enable)
{
     plp_aperta_phymod_phy_access_t phy_copy;
     PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

     PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_ext_loop_timing(&phy_copy, enable));
     return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_eye_margin_est_get(const plp_aperta_phymod_phy_access_t* phy, phymod_eye_margin_mode_t eye_margin_mode, uint32_t* value)
{
    int start_lane, num_lane;
    uint16_t hz_l, hz_r, vt_u, vt_d;
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane_mask = 0x1 << start_lane;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_INTERNAL_get_eye_margin_est(&phy_copy, &hz_l, &hz_r, &vt_u, &vt_d));

    switch (eye_margin_mode) {
    case phymod_eye_marign_HZ_L:
        *value = hz_l;
        break;
    case phymod_eye_marign_HZ_R:
        *value = hz_r;
        break;
    case phymod_eye_marign_VT_U:
        *value = vt_u;
        break;
    case phymod_eye_marign_VT_D:
        *value = vt_d;
        break;
    default:
        *value = 0;
        break;
    }

    return PHYMOD_E_NONE;

}
