
/*
 *
 * $Id: phymod.xml,v 1.1.2.5 Broadcom SDK $
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 *
 */

#include <phymod/phymod.h>
#include <phymod/phymod_util.h>
#include <phymod/phymod_config.h>
#include <phymod/phymod_diagnostics.h>
#include <phymod/phymod_diagnostics_dispatch.h>
#include <aperta_tscbh.h>
#include <aperta_tscbh_diagnostics.h>

#include <blackhawk/tier1/blackhawk_cfg_seq.h>
#include <blackhawk/tier1/common/srds_api_enum.h>
#include <blackhawk/tier1/blackhawk_tsc_enum.h>
#include <blackhawk/tier1/blackhawk_tsc_common.h>
#include <blackhawk/tier1/blackhawk_tsc_interface.h>
#include <blackhawk/tier1/blackhawk_tsc_dependencies.h>
#include <blackhawk/tier1/blackhawk_tsc_internal.h>
#include <blackhawk/tier1/public/blackhawk_api_uc_vars_rdwr_defns_public.h>
#include <blackhawk/tier1/blackhawk_tsc_access.h>
#include <blackhawk/tier1/blackhawk_tsc_types.h>
#include <aperta_tscbh/tier1/aperta_tbhmod.h>


#define PATTERN_MAX_LENGTH 240
#ifdef PHYMOD_APERTA_SUPPORT


/*phymod, internal enum mappings*/
STATIC
int _plp_aperta_tscbh_prbs_poly_phymod_to_blackhawk(plp_aperta_phymod_prbs_poly_t phymod_poly, enum plp_aperta_srds_prbs_polynomial_enum *blackhawk_poly)
{
    switch(phymod_poly){
    case phymodPrbsPoly7:
        *blackhawk_poly = PRBS_7;
        break;
    case phymodPrbsPoly9:
        *blackhawk_poly = PRBS_9;
        break;
    case phymodPrbsPoly11:
        *blackhawk_poly = PRBS_11;
        break;
    case phymodPrbsPoly15:
        *blackhawk_poly = PRBS_15;
        break;
    case phymodPrbsPoly23:
        *blackhawk_poly = PRBS_23;
        break;
    case phymodPrbsPoly31:
        *blackhawk_poly = PRBS_31;
        break;
    case phymodPrbsPoly58:
        *blackhawk_poly = PRBS_58;
        break;
    case phymodPrbsPoly49:
        *blackhawk_poly = PRBS_49;
        break;
    case phymodPrbsPoly10:
        *blackhawk_poly = PRBS_10;
        break;
    case phymodPrbsPoly20:
        *blackhawk_poly = PRBS_20;
        break;
    case phymodPrbsPoly13:
        *blackhawk_poly = PRBS_13;
        break;
    default:
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("unsupported poly for tscf %u"), phymod_poly));
    }
    return PHYMOD_E_NONE;
}

STATIC
int _plp_aperta_tscbh_prbs_poly_tscbh_to_phymod(enum plp_aperta_srds_prbs_polynomial_enum  *blackhawk_poly, plp_aperta_phymod_prbs_poly_t *phymod_poly)
{
    switch(*blackhawk_poly){
    case PRBS_7:
        *phymod_poly = phymodPrbsPoly7;
        break;
    case PRBS_9:
        *phymod_poly = phymodPrbsPoly9;
        break;
    case PRBS_11:
        *phymod_poly = phymodPrbsPoly11;
        break;
    case PRBS_15:
        *phymod_poly = phymodPrbsPoly15;
        break;
    case PRBS_23:
        *phymod_poly = phymodPrbsPoly23;
        break;
    case PRBS_31:
        *phymod_poly = phymodPrbsPoly31;
        break;
    case PRBS_58:
        *phymod_poly = phymodPrbsPoly58;
        break;
    case PRBS_49:
        *phymod_poly = phymodPrbsPoly49;
        break;
    case PRBS_10:
        *phymod_poly = phymodPrbsPoly10;
        break;
    case PRBS_20:
        *phymod_poly = phymodPrbsPoly20;
        break;
    case PRBS_13:
        *phymod_poly = phymodPrbsPoly13;
        break;
    default:
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_INTERNAL, (_PHYMOD_MSG("uknown poly %u"), *blackhawk_poly));
    }
    return PHYMOD_E_NONE;
}


int plp_aperta_tscbh_phy_rx_slicer_position_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, const plp_aperta_phymod_slicer_position_t* position)
{
    /* Not supported */
    PHYMOD_DEBUG_ERROR(("plp_aperta_tscbh_phy_rx_slicer_position_set function is NOT SUPPORTED!!\n"));

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_rx_slicer_position_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, plp_aperta_phymod_slicer_position_t* position)
{
    /* Not supported */
    PHYMOD_DEBUG_ERROR(("plp_aperta_tscbh_phy_rx_slicer_position_get function is NOT SUPPORTED!!\n"));

    return PHYMOD_E_NONE;
}


int plp_aperta_tscbh_phy_rx_slicer_position_max_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, const plp_aperta_phymod_slicer_position_t* position_min, const plp_aperta_phymod_slicer_position_t* position_max)
{
    /* Not supported */
    PHYMOD_DEBUG_ERROR(("plp_aperta_tscbh_phy_rx_slicer_position_max_get function is NOT SUPPORTED!!\n"));

    return PHYMOD_E_NONE;
}


int plp_aperta_tscbh_phy_prbs_config_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags , const plp_aperta_phymod_prbs_t* prbs)
{
    enum plp_aperta_srds_prbs_polynomial_enum blackhawk_poly;
    plp_aperta_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, i;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN(_plp_aperta_tscbh_prbs_poly_phymod_to_blackhawk(prbs->poly, &blackhawk_poly));
    /*first check which direction */
    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 1 << (start_lane + i);
        if (PHYMOD_PRBS_DIRECTION_RX_GET(flags)) {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_tsc_config_rx_prbs(&phy_copy, blackhawk_poly,PRBS_INITIAL_SEED_HYSTERESIS,  prbs->invert));
        } else if (PHYMOD_PRBS_DIRECTION_TX_GET(flags)) {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_tsc_config_tx_prbs(&phy_copy, blackhawk_poly, prbs->invert));
        } else {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_tsc_config_rx_prbs(&phy_copy, blackhawk_poly, PRBS_INITIAL_SEED_HYSTERESIS, prbs->invert));
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_tsc_config_tx_prbs(&phy_copy, blackhawk_poly, prbs->invert));
        }
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_prbs_config_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags , plp_aperta_phymod_prbs_t* prbs)
{
    plp_aperta_phymod_prbs_t config_tmp;
    enum plp_aperta_srds_prbs_polynomial_enum blackhawk_poly;
    enum plp_aperta_srds_prbs_checker_mode_enum prbs_checker_mode;
    plp_aperta_phymod_phy_access_t phy_copy;
    uint8_t invert;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));


    if (PHYMOD_PRBS_DIRECTION_TX_GET(flags)) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_get_tx_prbs_config(&phy_copy, &blackhawk_poly, &invert));
        config_tmp.invert = invert;
        PHYMOD_IF_ERR_RETURN(_plp_aperta_tscbh_prbs_poly_tscbh_to_phymod(&blackhawk_poly, &config_tmp.poly));
        prbs->invert = config_tmp.invert;
        prbs->poly = config_tmp.poly;
    } else if (PHYMOD_PRBS_DIRECTION_RX_GET(flags)) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_get_rx_prbs_config(&phy_copy,
                                                                &blackhawk_poly,
                                                                &prbs_checker_mode,
                                                                &invert));
        config_tmp.invert = invert;
        PHYMOD_IF_ERR_RETURN(_plp_aperta_tscbh_prbs_poly_tscbh_to_phymod(&blackhawk_poly, &config_tmp.poly));
        prbs->invert = config_tmp.invert;
        prbs->poly = config_tmp.poly;
    } else {
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_get_tx_prbs_config(&phy_copy, &blackhawk_poly,  &invert));
        config_tmp.invert = invert;
        PHYMOD_IF_ERR_RETURN(_plp_aperta_tscbh_prbs_poly_tscbh_to_phymod(&blackhawk_poly, &config_tmp.poly));
        prbs->invert = config_tmp.invert;
        prbs->poly = config_tmp.poly;
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_prbs_enable_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags , uint32_t enable)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, i;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /*first check which direction */
    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 1 << (start_lane + i);
        if (PHYMOD_PRBS_DIRECTION_TX_GET(flags)) {
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_tx_prbs_en(&phy_copy, enable));
        } else if (PHYMOD_PRBS_DIRECTION_RX_GET(flags)) {
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_rx_prbs_en(&phy_copy, enable));
        } else {
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_tx_prbs_en(&phy_copy, enable));
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_rx_prbs_en(&phy_copy, enable));
        }
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_prbs_enable_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags , uint32_t* enable)
{
    uint8_t enable_tmp;
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    if (PHYMOD_PRBS_DIRECTION_TX_GET(flags)) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_get_tx_prbs_en(&phy_copy, &enable_tmp));
        *enable = enable_tmp;
    } else if (PHYMOD_PRBS_DIRECTION_RX_GET(flags)) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_get_rx_prbs_en(&phy_copy, &enable_tmp));
        *enable = enable_tmp;
    } else {
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_get_tx_prbs_en(&phy_copy, &enable_tmp));
        *enable = enable_tmp;
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_get_rx_prbs_en(&phy_copy, &enable_tmp));
        *enable &= enable_tmp;
    }

    return PHYMOD_E_NONE;
}


int plp_aperta_tscbh_phy_prbs_status_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, plp_aperta_phymod_prbs_status_t* prbs_status)
{
    uint8_t status = 0;
    uint32_t prbs_err_count = 0;
    int i, start_lane, num_lane;
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    /* next figure out the lane num and start_lane based on the input */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    prbs_status->prbs_lock = 0;
    prbs_status->error_count = 0;
    prbs_status->prbs_lock_loss = 0;
    prbs_status->prbs_lock = 1;

    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 0x1 << (i + start_lane);
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_prbs_chk_lock_state(&phy_copy, &status));
    if (status) {
        /*next check the lost of lock and error count */
        status = 0;
        PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_tsc_prbs_err_count_state(&phy_copy, &prbs_err_count, &status));
        /*PHYMOD_DEBUG_INFO((" Lane :: %d PRBS Error count :: %d\n", i, prbs_err_count));*/
        if (status) {
        /*temp lost of lock */
            prbs_status->prbs_lock_loss = 1;
        } else {
                prbs_status->error_count += prbs_err_count;
        }
    } else {
            /*PHYMOD_DEBUG_INFO((" Lane :: %d PRBS not locked\n", i ));*/
            prbs_status->prbs_lock = 0;
            /* return PHYMOD_E_NONE; */
        }
    }
    return PHYMOD_E_NONE;
}

#if 0
int aperta_tscbh_phy_pattern_config_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_pattern_t* pattern)
{
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_config_shared_tx_pattern (&phy_copy.access,
                    (uint8_t) pattern->pattern_len, (const char *) pattern->pattern));
    return PHYMOD_E_NONE;
}

int aperta_tscbh_phy_pattern_config_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_pattern_t* pattern)
{
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_config_shared_tx_pattern_idx_get(&phy_copy.access,
                                  &pattern->pattern_len,
                                  pattern->pattern));
    return PHYMOD_E_NONE;
}


int aperta_tscbh_phy_pattern_enable_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t enable, const plp_aperta_phymod_pattern_t* pattern)
{
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_tx_shared_patt_gen_en(&phy_copy.access, (uint8_t) enable, (uint8_t)pattern->pattern_len));
    return PHYMOD_E_NONE;
}

int aperta_tscbh_phy_pattern_enable_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* enable, const plp_aperta_phymod_pattern_t* pattern)
{
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
       (plp_aperta_blackhawk_tsc_tx_shared_patt_gen_en_get(&phy_copy.access, (uint8_t *) enable));
    return PHYMOD_E_NONE;
}

#endif

int plp_aperta_tscbh_phy_fec_cl91_correctable_counter_get(const plp_aperta_phymod_phy_access_t* phy, phymod_fec_type_t fec_type, uint32_t* count)
{
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
       (plp_aperta_tbhmod_fec_correctable_counter_get(&phy_copy, fec_type, count));
    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_fec_cl91_uncorrectable_counter_get(const plp_aperta_phymod_phy_access_t* phy, phymod_fec_type_t fec_type, uint32_t* count)
{
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
       (plp_aperta_tbhmod_fec_uncorrectable_counter_get(&phy_copy, fec_type, count));
    return PHYMOD_E_NONE;

}


int plp_aperta_tscbh_core_diagnostics_get(const plp_aperta_phymod_core_access_t* core, plp_aperta_phymod_core_diagnostics_t* diag)
{

    return PHYMOD_E_NONE;
}


int plp_aperta_tscbh_phy_diagnostics_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_phy_diagnostics_t* diag)
{

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_pmd_info_dump(const plp_aperta_phymod_phy_access_t* phy, char *arg)
{
    int start_lane, num_lane;
    uint32_t i, j, tmp_lane_mask;
    plp_aperta_phymod_phy_access_t phy_copy;

    char *cmd_str = (char*)arg;
    phymod_diag_type_t type;

    if (!cmd_str) {
        type = phymod_diag_DSC;
    } else if((!PHYMOD_STRCMP(cmd_str,"ber"))||(!PHYMOD_STRCMP(cmd_str,"Ber"))||(!PHYMOD_STRCMP(cmd_str,"BER"))) {
        type = phymod_diag_BER;
    } else if((!PHYMOD_STRCMP(cmd_str,"config"))||(!PHYMOD_STRCMP(cmd_str,"Config"))||(!PHYMOD_STRCMP(cmd_str,"CONFIG"))) {
        type = phymod_diag_CFG;
    } else if((!PHYMOD_STRCMP(cmd_str,"cl72"))||(!PHYMOD_STRCMP(cmd_str,"Cl72"))||(!PHYMOD_STRCMP(cmd_str,"CL72"))) {
        type = phymod_diag_CL72;
    } else if((!PHYMOD_STRCMP(cmd_str,"debug"))||(!PHYMOD_STRCMP(cmd_str,"Debug"))||(!PHYMOD_STRCMP(cmd_str,"DEBUG"))) {
        type = phymod_diag_DEBUG;
    } else if((!PHYMOD_STRCMP(cmd_str,"state"))||(!PHYMOD_STRCMP(cmd_str,"State"))||(!PHYMOD_STRCMP(cmd_str,"STATE"))) {
        type = phymod_diag_STATE;
    } else if((!PHYMOD_STRCMP(cmd_str,"verbose"))||(!PHYMOD_STRCMP(cmd_str,"Verbose"))||(!PHYMOD_STRCMP(cmd_str,"VERBOSE"))) {
        type = phymod_diag_ALL;
    } else if (!PHYMOD_STRCMP(cmd_str,"STD")) {
        type = phymod_diag_DSC_STD;
    } else {
        type = phymod_diag_STATE;
    }

    PHYMOD_DEBUG_ERROR((" %s:%d type = %d Phy: 0x%03x laneMask  = 0x%X\n",
                        __func__, __LINE__, type, phy->access.addr, phy->access.lane_mask));

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /*next figure out the lane num and start_lane based on the input*/
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    if (type == phymod_diag_DSC) {
       for (i = 0; i < 8; i++) {
            phy_copy.access.lane_mask = 0x1 << i ;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_tsc_display_diag_data(&phy_copy, SRDS_DIAG_CORE));
       }
    } else if (type == phymod_diag_DSC_STD) {

            PHYMOD_DIAG_OUT(("    +--------------------------------------------------------------------+\n"));
            PHYMOD_DIAG_OUT(("    | DSC Phy: 0x%03x lane_mask: 0x%02x                                 |\n", phy->access.addr, phy->access.lane_mask));
            PHYMOD_DIAG_OUT(("    +--------------------------------------------------------------------+\n"));
             tmp_lane_mask = phy_copy.access.lane_mask;
             for (j = 0; j < 8; j++) {
                if (phy->access.lane_mask & ( 1<<j)) {
               phy_copy.access.lane_mask = 0x1 << j;
               PHYMOD_IF_ERR_RETURN
                  (plp_aperta_blackhawk_tsc_display_diag_data(&phy_copy, SRDS_DIAG_CORE | SRDS_DIAG_LANE | SRDS_DIAG_EVENT));
                }
             }

    } else {
        for (i = 0; i < num_lane; i++) {
            if (phy->access.lane_mask & (1 << (i + start_lane))) {
                phy_copy.access.lane_mask = 0x1 << (i + start_lane);
               switch(type) {
               case phymod_diag_CFG:
                    PHYMOD_DEBUG_ERROR((" %s:%d type = CFG\n", __func__, __LINE__));
                    if(i==0) {
                        tmp_lane_mask = phy_copy.access.lane_mask;
                        phy_copy.access.lane_mask = 1;
                        PHYMOD_IF_ERR_RETURN
                         (plp_aperta_blackhawk_tsc_display_core_config(&phy_copy));
                        phy_copy.access.lane_mask = tmp_lane_mask;
                    }
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_display_lane_config(&phy_copy));
                    break;

                case phymod_diag_DEBUG:
                    PHYMOD_DEBUG_ERROR((" %s:%d type = DBG\n", __func__, __LINE__));
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_display_lane_debug_status(&phy_copy));
                    break;

                case phymod_diag_BER:
                    PHYMOD_DEBUG_ERROR((" %s:%d type = BER\n", __func__, __LINE__));
                    break;

                /*
                 * COVERITY
                 *
                 * TEFMOD_DIAG_ALL branch involve information in TEFMOD_DIAG_STATE branch
                 */
                /* coverity[unterminated_case] */
                case phymod_diag_ALL:
                    PHYMOD_DEBUG_ERROR((" %s:%d type = CFG\n", __func__, __LINE__));
                    if(i==0) {
                        tmp_lane_mask = phy_copy.access.lane_mask;
                        phy_copy.access.lane_mask = 1;
                        PHYMOD_IF_ERR_RETURN
                            (plp_aperta_blackhawk_tsc_display_core_config(&phy_copy));
                        phy_copy.access.lane_mask = tmp_lane_mask;
                    }
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_display_lane_config(&phy_copy));

                    PHYMOD_DEBUG_ERROR((" %s:%d type = DBG\n", __func__, __LINE__));
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_display_lane_debug_status(&phy_copy));
                    break;

                case phymod_diag_STATE:
                default:
                    PHYMOD_DEBUG_ERROR((" %s:%d type = DEF\n", __func__, __LINE__));
                    for (j = 0; j < 8; j++) {
                         phy_copy.access.lane_mask = 0x1 << j;
                      PHYMOD_IF_ERR_RETURN
                         (plp_aperta_blackhawk_tsc_display_diag_data(&phy_copy, SRDS_DIAG_CORE | SRDS_DIAG_LANE | SRDS_DIAG_EVENT));
                    }
                    break;
               }
           }
        }
    }
    return PHYMOD_E_NONE;
}


#endif /* PHYMOD_BLACKHAWK_SUPPORT */
