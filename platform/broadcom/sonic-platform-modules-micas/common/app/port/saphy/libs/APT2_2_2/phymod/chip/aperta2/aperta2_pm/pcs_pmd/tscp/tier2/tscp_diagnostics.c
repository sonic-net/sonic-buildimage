
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
#include <phymod/phymod_system.h>
#include <phymod/phymod_util.h>
#include <phymod/phymod_config.h>
#include <phymod/phymod_diagnostics.h>
#include <phymod/phymod_diagnostics_dispatch.h>
#include <phymod/phymod_acc.h>
#include <tscp.h>
#include <tscp_diagnostics.h>
#include "tscp/tier1/tscpmod.h"
#include <tier1/aperta2_reg_access.h>

#ifdef PHYMOD_APERTA2_SUPPORT


int plp_aperta2_tscp_phy_fec_cl91_correctable_counter_get(const plp_aperta2_phymod_phy_access_t* phy, phymod_fec_type_t fec_type, uint32_t* count)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    phymod_phy_speed_config_t speed_config;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscp_phy_speed_config_get(phy, &speed_config));
    PHYMOD_IF_ERR_RETURN
       (plp_aperta2_tscpmod_fec_correctable_counter_get(&phy_copy,speed_config.data_rate, count));

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_fec_cl91_uncorrectable_counter_get(const plp_aperta2_phymod_phy_access_t* phy,phymod_fec_type_t fec_type, uint32_t* count)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    phymod_phy_speed_config_t speed_config;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscp_phy_speed_config_get(phy, &speed_config));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_fec_uncorrectable_counter_get(&phy_copy, speed_config.data_rate, count));

    return PHYMOD_E_NONE;

}

int plp_aperta2_tscp_phy_rsfec_symbol_error_counter_get(const plp_aperta2_phymod_phy_access_t* phy,
                                             int max_count,
                                             int* actual_count,
                                             uint32_t* error_count)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, speed_id, i;
    tscpmod_spd_id_tbl_entry_t speed_config_entry;
    uint32_t packed_entry[5];
    int octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(phy->access.lane_mask);

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    phy_copy.access.lane_mask = 0x1 << start_lane;
    /* first read speed id from resolved status */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_speed_id_get(&phy_copy, &speed_id));

    /* first read the speed entry and then decode the speed and FEC type */
    phy_copy.access.lane_mask = 1 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_mem_read(&phy_copy, phymodMemSpeedIdTable, speed_id, packed_entry));

    /*decode speed entry */
    plp_aperta2_tscpmod_spd_ctrl_unpack_spd_id_tbl_entry(packed_entry, &speed_config_entry);

    switch (speed_config_entry.num_lanes) {
        case 0: num_lane = 1;
            break;
        case 1: num_lane = 2;
            break;
        case 2: num_lane = 4;
            break;
        case 3: num_lane = 8;
            break;
        default:
            PHYMOD_DEBUG_ERROR(("Unsupported number of lane \n"));
            return PHYMOD_E_UNAVAIL;
    }

    /* Update lane mask. During AN, lane mask might change. */
    phy_copy.access.lane_mask = 0;
    for (i = 0; i < num_lane; i ++) {
        phy_copy.access.lane_mask |= 1 << (start_lane + i);
    }

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_rsfec_symbol_error_counter_get(&phy_copy,
                                               speed_config_entry.bit_mux_mode,
                                               max_count,
                                               actual_count,
                                               error_count));

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_fec_error_bits_counter_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t* count)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    phymod_phy_speed_config_t speed_config;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_tscp_phy_speed_config_get(phy, &speed_config));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_fec_error_bits_counter_get(&phy_copy, speed_config.data_rate, count));

    return PHYMOD_E_NONE;
}

#ifdef APERTA2_PM_UNSUPPORTED_API
int tscp_phy_fec_error_inject_set(const plp_aperta2_phymod_phy_access_t* phy,
                                  uint16_t error_control_map,
                                  phymod_fec_error_mask_t bit_error_mask)
{
    uint8_t error_mask_66_bit = 0;
    phymod_phy_speed_config_t speed_config;
    plp_aperta2_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_tscp_phy_speed_config_get(phy, &speed_config));
    switch (speed_config.fec_type) {
        case phymod_fec_CL91 :
            error_mask_66_bit = 1;
            break;
        case phymod_fec_RS544 :
        case phymod_fec_RS272 :
        case phymod_fec_RS544_2XN :
        case phymod_fec_RS272_2XN :
            error_mask_66_bit = 0;
            break;
        case phymod_fec_None :
        default :
            return PHYMOD_E_CONFIG;
    }

    if (error_injection_config->error_mask_bit_79_64 > 0xf) {
        PHYMOD_DEBUG_ERROR((" %s:%d bit_error_mask is invalid! Valid mask range is 67 : 0 bits\n", __func__, __LINE__));
        return PHYMOD_E_CONFIG;
    } else if (error_mask_66_bit && (error_injection_config->error_mask_bit_79_64 > 0x3)) {
        PHYMOD_DEBUG_ERROR((" %s:%d bit_error_mask is invalid! Valid mask range is %s : 0 bits\n", __func__, __LINE__, error_mask_66_bit? "66": "68"));
        return PHYMOD_E_CONFIG;
    }

    PHYMOD_IF_ERR_RETURN(tscpmod_fec_error_inject_config_set(&phy_copy.access, error_injection_config));

    return PHYMOD_E_NONE;
}

int tscp_phy_fec_error_inject_get(const plp_aperta2_phymod_phy_access_t* phy,
                                  uint16_t* error_control_map,
                                  phymod_fec_error_mask_t* bit_error_mask)
{
    uint16_t control_map = 0;
    tscpmod_fec_error_mask_t fec_error_mask;

    plp_aperta2_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_MEMSET(error_injection_config, 0x0, sizeof(phymod_fec_error_injection_config_t));

    PHYMOD_IF_ERR_RETURN(tscpmod_fec_error_inject_config_get(&phy_copy.access, error_injection_config));

    return PHYMOD_E_NONE;
}
int plp_aperta2_tscp_phy_fec_error_bits_counter_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t* count)
{
    plp_aperta2_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN(tscpmod_fec_error_inject_enable_set(&phy_copy.access, error_injection_enable));

    return PHYMOD_E_NONE;
}
#endif


#endif /* PHYMOD_APERTA2_SUPPORT */
