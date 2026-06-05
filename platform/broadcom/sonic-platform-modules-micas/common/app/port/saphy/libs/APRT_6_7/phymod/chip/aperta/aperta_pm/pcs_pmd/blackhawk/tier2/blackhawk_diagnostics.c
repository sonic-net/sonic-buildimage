
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
#include <phymod/phymod_config.h>
#include <phymod/phymod_diagnostics.h>
#include <phymod/phymod_diag.h>
#include <phymod/phymod_diagnostics_dispatch.h>
#include <blackhawk.h>
#include <blackhawk_diagnostics.h>
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



#define PATTERN_MAX_LENGTH 255
#ifdef PHYMOD_APERTA_SUPPORT

#define __ERR &__err
#define APERTA_IS_ON              1
#define APERTA_SSPRQ_PATTERN      0
#define APERTA_QPRBS13_PATTERN    1
#define APERTA_SQUAREWAVE_PATTERN 2
#define APERTA_PCSSCRIDLE_PATTERN 3
#define APERTA_KP4PRBS_PATTERN    4

#define APERTA_ONE_2_CONSECUTIVE  0
#define APERTA_ONE_4_CONSECUTIVE  1
#define APERTA_ONE_8_CONSECUTIVE  2
#define APERTA_ONE_16_CONSECUTIVE 3


/*phymod, internal enum mappings*/
STATIC
int _plp_aperta_blackhawk_prbs_poly_phymod_to_blackhawk(plp_aperta_phymod_prbs_poly_t phymod_poly, enum plp_aperta_srds_prbs_polynomial_enum *blackhawk_poly)
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
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("unsupported poly for blackhawk %u"), phymod_poly));
    }
    return PHYMOD_E_NONE;
}

STATIC
int _plp_aperta_blackhawk_prbs_poly_blackhawk_to_phymod(enum plp_aperta_srds_prbs_polynomial_enum  *blackhawk_poly, plp_aperta_phymod_prbs_poly_t *phymod_poly)
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

int plp_aperta_blackhawk_phy_rx_slicer_position_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, const plp_aperta_phymod_slicer_position_t* position)
{


    /* Not supported */
    PHYMOD_DEBUG_ERROR(("plp_aperta_blackhawk_phy_rx_slicer_position_set function is NOT SUPPORTED!!\n"));


    return PHYMOD_E_NONE;

}

int plp_aperta_blackhawk_phy_rx_slicer_position_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, plp_aperta_phymod_slicer_position_t* position)
{


    /* Not supported */
    PHYMOD_DEBUG_ERROR(("plp_aperta_blackhawk_phy_rx_slicer_position_get function is NOT SUPPORTED!!\n"));


    return PHYMOD_E_NONE;

}


int plp_aperta_blackhawk_phy_rx_slicer_position_max_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, const plp_aperta_phymod_slicer_position_t* position_min, const plp_aperta_phymod_slicer_position_t* position_max)
{


    /* Not supported */
    PHYMOD_DEBUG_ERROR(("plp_aperta_blackhawk_phy_rx_slicer_position_max_get function is NOT SUPPORTED!!\n"));


    return PHYMOD_E_NONE;

}


int plp_aperta_blackhawk_phy_prbs_config_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags , const plp_aperta_phymod_prbs_t* prbs)
{
    enum plp_aperta_srds_prbs_polynomial_enum blackhawk_poly;
    plp_aperta_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, i;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN(_plp_aperta_blackhawk_prbs_poly_phymod_to_blackhawk(prbs->poly, &blackhawk_poly));
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

int plp_aperta_blackhawk_phy_prbs_config_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags , plp_aperta_phymod_prbs_t* prbs)
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
        PHYMOD_IF_ERR_RETURN(_plp_aperta_blackhawk_prbs_poly_blackhawk_to_phymod(&blackhawk_poly, &config_tmp.poly));
        prbs->invert = config_tmp.invert;
        prbs->poly = config_tmp.poly;
    } else if (PHYMOD_PRBS_DIRECTION_RX_GET(flags)) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_get_rx_prbs_config(&phy_copy,
                                                                &blackhawk_poly,
                                                                &prbs_checker_mode,
                                                                &invert));
        config_tmp.invert = invert;
        PHYMOD_IF_ERR_RETURN(_plp_aperta_blackhawk_prbs_poly_blackhawk_to_phymod(&blackhawk_poly, &config_tmp.poly));
        prbs->invert = config_tmp.invert;
        prbs->poly = config_tmp.poly;
    } else {
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_get_tx_prbs_config(&phy_copy, &blackhawk_poly,  &invert));
        config_tmp.invert = invert;
        PHYMOD_IF_ERR_RETURN(_plp_aperta_blackhawk_prbs_poly_blackhawk_to_phymod(&blackhawk_poly, &config_tmp.poly));
        prbs->invert = config_tmp.invert;
        prbs->poly = config_tmp.poly;
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_prbs_enable_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags , uint32_t enable)
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

int plp_aperta_blackhawk_phy_prbs_enable_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags , uint32_t* enable)
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


int plp_aperta_blackhawk_phy_prbs_status_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, plp_aperta_phymod_prbs_status_t* prbs_status)
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
        PHYMOD_DEBUG_INFO((" Lane :: %d PRBS Error count :: %d\n", i, prbs_err_count));
        if (status) {
        /*temp lost of lock */
            prbs_status->prbs_lock_loss = 1;
        } else {
                prbs_status->error_count += prbs_err_count;
        }
    } else {
            PHYMOD_DEBUG_INFO((" Lane :: %d PRBS not locked\n", i ));
            prbs_status->prbs_lock = 0;
            /* return PHYMOD_E_NONE; */
        }
    }
    return PHYMOD_E_NONE;

}


int plp_aperta_blackhawk_phy_pattern_config_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_pattern_t* pattern)
{
    int i,j = 0, bit;
    char patt[PATTERN_MAX_LENGTH+1];
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    for (i=0; i< PATTERN_MAX_SIZE; i++)
    {
      for (j=0;j<32 && i*32+j <= PATTERN_MAX_LENGTH; j++)
      {
          if (i*32+j == pattern->pattern_len) {
              if (i*32+j <= PATTERN_MAX_LENGTH) {
                  /* coverity[overrun-local] */
                  patt[i*32+j] = '\0';
                  break;
              }
          }
          bit = pattern->pattern[i] >> j & 00000001;
          switch (bit) {
          case (1):
              patt[i*32+j] = '1';
              break;
          default:
              patt[i*32+j] = '0';
              break;
          }
      }
    }
    PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_config_shared_tx_pattern (&phy_copy,
                    (uint8_t) pattern->pattern_len, (const char *) patt));
    return PHYMOD_E_NONE;

}

int plp_aperta_blackhawk_phy_pattern_config_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_pattern_t* pattern)
{
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_config_shared_tx_pattern_idx_get(&phy_copy,
                                  &pattern->pattern_len,
                                  pattern->pattern));

    return PHYMOD_E_NONE;

}


int plp_aperta_blackhawk_phy_pattern_enable_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t enable, const plp_aperta_phymod_pattern_t* patt)
{
    plp_aperta_phymod_phy_access_t pa;
    uint8_t patt_len;
    unsigned int *patt_type;
    uint8_t patt_switch;
    uint8_t patt_mode_len;
    const char *hexpatt = NULL;

    if ((patt == NULL) || (patt->pattern == NULL)) {
        PHYMOD_RETURN_WITH_ERR
                (PHYMOD_E_PARAM,\
                (_PHYMOD_MSG("User passed NULL parameter")));
    }
    PHYMOD_MEMCPY(&pa, phy, sizeof(pa));
    patt_type = patt->pattern;
    patt_switch = enable;
    patt_mode_len = patt->pattern_len;
    patt_len = 16;
    if (patt_switch == APERTA_IS_ON) {
        switch(*patt_type) {
            case APERTA_SQUAREWAVE_PATTERN:
                switch(patt_mode_len) {
                    case APERTA_ONE_2_CONSECUTIVE:
                        hexpatt = "0xCCCC";
                        break;
                    case APERTA_ONE_4_CONSECUTIVE:
                        hexpatt = "0xF0F0";
                        break;
                    case APERTA_ONE_8_CONSECUTIVE:
                        hexpatt = "0xFF00";
                        break;
                    case APERTA_ONE_16_CONSECUTIVE:
                        hexpatt = "0xFFFF0000";
                        patt_len = 32;
                        break;
                    default:
                        PHYMOD_DEBUG_ERROR(("Error : Only 0xCCCC/0xF0F0/0xFF00/0xFFFF0000 Patterns supported for now\n"));
                        return PHYMOD_E_PARAM;
                 }
                 break;

             default:
                 PHYMOD_DEBUG_ERROR(("Error : Only Square Wave Pattern supported for now\n"));
                 return PHYMOD_E_PARAM;
        }
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_config_shared_tx_pattern(&pa, patt_len, hexpatt));
    }
    PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_tx_shared_patt_gen_en(&pa, patt_switch, patt_len));
    return PHYMOD_E_NONE;
}

int _plp_aperta_blackhawk_phy_pattern_enable_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* enable, plp_aperta_phymod_pattern_t* pattern)
{
    unsigned int *patt_type;
    uint16_t msb,lsb = 0;
    uint32_t patt_hw = 0;

    srds_access_t *sa__ = (srds_access_t *)phy;
    err_code_t __err = 0;
    if ((pattern == NULL) || (pattern->pattern == NULL)) {
        PHYMOD_RETURN_WITH_ERR
                (PHYMOD_E_PARAM,\
                (_PHYMOD_MSG("User passed NULL parameter")));
    }
    patt_type = pattern->pattern;
    switch(*patt_type) {
        case APERTA_SQUAREWAVE_PATTERN:
            break;
        default:
            PHYMOD_DEBUG_ERROR(("Error : Only Square Wave Pattern supported for now\n"));
            return PHYMOD_E_PARAM;
    }
    *enable = rd_patt_gen_en();
    lsb = rdc_patt_gen_seq_0();
    msb = rdc_patt_gen_seq_1();
    patt_hw = (msb << 16) | lsb;
    if((msb == 0xFF00) || (msb == 0x00FF)) {
        pattern->pattern_len = APERTA_ONE_8_CONSECUTIVE;
    } else if((patt_hw == 0xFFFF0000) || (patt_hw == 0x0000FFFF)){
        pattern->pattern_len = APERTA_ONE_16_CONSECUTIVE;
    } else if((msb == 0xF0F0) || (msb == 0x0F0F)){
        pattern->pattern_len = APERTA_ONE_4_CONSECUTIVE;
    } else if(msb == 0xCCCC){
        pattern->pattern_len = APERTA_ONE_2_CONSECUTIVE;
    } else {
        pattern->pattern_len = 0;
    }
    if (__err) {
        return (PHYMOD_E_INTERNAL);
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_pattern_enable_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* enable, const plp_aperta_phymod_pattern_t* pattern)
{
    return(_plp_aperta_blackhawk_phy_pattern_enable_get(phy, enable, (plp_aperta_phymod_pattern_t*) pattern));
}

int plp_aperta_blackhawk_core_diagnostics_get(const plp_aperta_phymod_core_access_t* core, plp_aperta_phymod_core_diagnostics_t* diag)
{

    return PHYMOD_E_NONE;
}


int plp_aperta_blackhawk_phy_diagnostics_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_phy_diagnostics_t* diag)
{

    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_phy_pmd_info_dump(const plp_aperta_phymod_phy_access_t* phy, char* type)
{
    int start_lane, num_lane;
    uint32_t i, j, tmp_lane_mask;
    plp_aperta_phymod_phy_access_t phy_copy;

    phymod_diag_type_t cmd_type;
    srds_info_t  *tsc_info_ptr = NULL;

    if (!type) {
        cmd_type = phymod_diag_DSC;
    } else if ((!PHYMOD_STRCMP(type, "ber")) ||
               (!PHYMOD_STRCMP(type, "Ber")) ||
               (!PHYMOD_STRCMP(type, "BER"))) {
        cmd_type = phymod_diag_BER;
    } else if ((!PHYMOD_STRCMP(type, "config")) ||
               (!PHYMOD_STRCMP(type, "Config")) ||
               (!PHYMOD_STRCMP(type, "CONFIG"))) {
        cmd_type = phymod_diag_CFG;
    } else if ((!PHYMOD_STRCMP(type, "cl72")) ||
               (!PHYMOD_STRCMP(type, "Cl72")) ||
               (!PHYMOD_STRCMP(type, "CL72"))) {
        cmd_type = phymod_diag_CL72;
    } else if ((!PHYMOD_STRCMP(type, "debug")) ||
               (!PHYMOD_STRCMP(type, "Debug")) ||
               (!PHYMOD_STRCMP(type, "DEBUG"))) {
        cmd_type = phymod_diag_DEBUG;
    } else if ((!PHYMOD_STRCMP(type, "state")) ||
               (!PHYMOD_STRCMP(type, "State")) ||
               (!PHYMOD_STRCMP(type, "STATE"))) {
        cmd_type = phymod_diag_STATE;
    } else if ((!PHYMOD_STRCMP(type, "verbose")) ||
               (!PHYMOD_STRCMP(type, "Verbose")) ||
               (!PHYMOD_STRCMP(type, "VERBOSE"))) {
        cmd_type = phymod_diag_ALL;
    } else if (!PHYMOD_STRCMP(type, "STD")) {
        cmd_type = phymod_diag_DSC_STD;
    } else {
        /*type = phymod_diag_STATE;*/
    }
    (void)cmd_type;
    (void)tmp_lane_mask;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /* Make sure information table is initialized */
    tsc_info_ptr = plp_aperta_blackhawk_tsc_INTERNAL_get_blackhawk_tsc_info_ptr(&phy_copy);
    if (tsc_info_ptr->signature == 0) {
     PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(&phy_copy));
    }
    /*next figure out the lane num and start_lane based on the input*/
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    if (type != NULL) {
    if (*type == 0) {
       for (i = start_lane; i < start_lane + num_lane; i++) {
            phy_copy.access.lane_mask = 0x1 << i ;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_tsc_display_diag_data(&phy_copy, SRDS_DIAG_CORE));
       }
    } else if (*type == 1) {

            PHYMOD_DIAG_OUT(("    +--------------------------------------------------------------------+\n"));
            PHYMOD_DIAG_OUT(("    | DSC Phy: 0x%03x lane_mask: 0x%02x                                 |\n", phy->access.addr, phy->access.lane_mask));
            PHYMOD_DIAG_OUT(("    +--------------------------------------------------------------------+\n"));
             tmp_lane_mask = phy_copy.access.lane_mask;
             for (j = 0; j < 8; j++) {
               phy_copy.access.lane_mask = 0x1 << j;
               PHYMOD_IF_ERR_RETURN
                  (plp_aperta_blackhawk_tsc_display_diag_data(&phy_copy, SRDS_DIAG_CORE | SRDS_DIAG_LANE | SRDS_DIAG_EVENT));
             }

    } else {
        for (i = 0; i < num_lane; i++) {
            phy_copy.access.lane_mask = 0x1 << (i + start_lane);

            switch(*type) {
#if 0
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

            casephymod_diag_CL72:
                PHYMOD_DEBUG_ERROR((" %s:%d type = CL72\n", __func__, __LINE__));
                PHYMOD_IF_ERR_RETURN
                    (blackhawk_tsc_display_cl93n72_status(&phy_copy.access));
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

#if 0
                PHYMOD_DEBUG_ERROR((" %s:%d type = CL72\n", __func__, __LINE__));
                PHYMOD_IF_ERR_RETURN
                    (blackhawk_tsc_display_cl93n72_status(&phy_copy.access));
#endif

                PHYMOD_DEBUG_ERROR((" %s:%d type = DBG\n", __func__, __LINE__));
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_display_lane_debug_status(&phy_copy));
                break;

            case phymod_diag_STATE:
#endif
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

static void _plp_aperta_blackhawk_diag_uc_reg_dump(plp_aperta_phymod_phy_access_t *pa)
{
    err_code_t errc;

    COMPILER_REFERENCE(errc);

    PHYMOD_DIAG_OUT(("+-------------------------------------------------+\n"));
    PHYMOD_DIAG_OUT(("|    MICRO CODE USR CTRL CONFIGURATION REGISTERS  |\n"));
    PHYMOD_DIAG_OUT(("+-------------------------------------------------+\n"));
    PHYMOD_DIAG_OUT(("|    config_word              [0x00]: 0x%04X      |\n",    plp_aperta_blackhawk_tsc_rdwl_uc_var(pa,&errc,0x0)));
    PHYMOD_DIAG_OUT(("|    usr_misc_ctrl_word       [0x04]: 0x%04X      |\n",    plp_aperta_blackhawk_tsc_rdwl_uc_var(pa,&errc,0x4)));
    PHYMOD_DIAG_OUT(("|    retune_after_restart     [0x06]: 0x%04X      |\n",    plp_aperta_blackhawk_tsc_rdbl_uc_var(pa,&errc,0x6)));
    PHYMOD_DIAG_OUT(("|    clk90_offset_adjust      [0x07]: 0x%04X      |\n",    plp_aperta_blackhawk_tsc_rdbls_uc_var(pa,&errc,0x7)));
    PHYMOD_DIAG_OUT(("|    clk90_offset_override    [0x08]: 0x%04X      |\n",    plp_aperta_blackhawk_tsc_rdbl_uc_var(pa,&errc,0x8)));
    PHYMOD_DIAG_OUT(("|    lane_event_log_level     [0x09]: 0x%04X      |\n",    plp_aperta_blackhawk_tsc_rdbl_uc_var(pa,&errc,0x9)));
    PHYMOD_DIAG_OUT(("|    pam4_chn_loss            [0x0A]: 0x%04X      |\n",    plp_aperta_blackhawk_tsc_rdbl_uc_var(pa,&errc,0xa)));
    PHYMOD_DIAG_OUT(("|    cl93n72_frc_byte         [0x0B]: 0x%04X      |\n",    plp_aperta_blackhawk_tsc_rdbl_uc_var(pa,&errc,0xb)));
    PHYMOD_DIAG_OUT(("|    disable_startup          [0x0C]: 0x%04X      |\n",    plp_aperta_blackhawk_tsc_rdwl_uc_var(pa,&errc,0xc)));
    PHYMOD_DIAG_OUT(("|    disable_steady_state     [0x0E]: 0x%04X      |\n",    plp_aperta_blackhawk_tsc_rdwl_uc_var(pa,&errc,0xe)));
    PHYMOD_DIAG_OUT(("|    disable_startup_dfe      [0x10]: 0x%04X      |\n",    plp_aperta_blackhawk_tsc_rdbl_uc_var(pa,&errc,0x10)));
    PHYMOD_DIAG_OUT(("|    disable_steady_state_dfe [0x11]: 0x%04X      |\n",    plp_aperta_blackhawk_tsc_rdbl_uc_var(pa,&errc,0x11)));
    PHYMOD_DIAG_OUT(("+-------------------------------------------------+\n"));
    PHYMOD_DIAG_OUT(("|         MICRO CODE USER STATUS REGISTERS        |\n"));
    PHYMOD_DIAG_OUT(("+-------------------------------------------------+\n"));
    PHYMOD_DIAG_OUT(("|    restart_counter          [0x12]: 0x%04X     |\n",    plp_aperta_blackhawk_tsc_rdbl_uc_var(pa,&errc,0x12)));
    PHYMOD_DIAG_OUT(("|    reset_counter            [0x13]: 0x%04X     |\n",    plp_aperta_blackhawk_tsc_rdbl_uc_var(pa,&errc,0x13)));
    PHYMOD_DIAG_OUT(("|    pmd_lock_counter         [0x14]: 0x%04X     |\n",    plp_aperta_blackhawk_tsc_rdbl_uc_var(pa,&errc,0x14)));
    PHYMOD_DIAG_OUT(("|    heye_left                [0x15]: 0x%04X     |\n",    plp_aperta_blackhawk_tsc_rdbl_uc_var(pa,&errc,0x15)));
    PHYMOD_DIAG_OUT(("|    heye_right               [0x16]: 0x%04X     |\n",    plp_aperta_blackhawk_tsc_rdbl_uc_var(pa,&errc,0x16)));
    PHYMOD_DIAG_OUT(("|    veye_upper               [0x17]: 0x%04X     |\n",    plp_aperta_blackhawk_tsc_rdbl_uc_var(pa,&errc,0x17)));
    PHYMOD_DIAG_OUT(("|    veye_lower               [0x18]: 0x%04X     |\n",    plp_aperta_blackhawk_tsc_rdbl_uc_var(pa,&errc,0x18)));
    PHYMOD_DIAG_OUT(("|    micro_stopped            [0x19]: 0x%04X     |\n",    plp_aperta_blackhawk_tsc_rdbl_uc_var(pa,&errc,0x19)));
    PHYMOD_DIAG_OUT(("|    link_time                [0x1C]: 0x%04X     |\n",    plp_aperta_blackhawk_tsc_rdwl_uc_var(pa,&errc,0x1c)));
    PHYMOD_DIAG_OUT(("+-------------------------------------------------+\n"));
    PHYMOD_DIAG_OUT(("|            MICRO CODE MISC REGISTERS            |\n"));
    PHYMOD_DIAG_OUT(("+-------------------------------------------------+\n"));
    PHYMOD_DIAG_OUT(("|    usr_diag_status          [0x1E]: 0x%04X     |\n",    plp_aperta_blackhawk_tsc_rdwl_uc_var(pa,&errc,0x1e)));
    PHYMOD_DIAG_OUT(("|    usr_diag_rd_ptr          [0x20]: 0x%04X     |\n",    plp_aperta_blackhawk_tsc_rdbl_uc_var(pa,&errc,0x20)));
    PHYMOD_DIAG_OUT(("|    usr_diag_mode            [0x21]: 0x%04X     |\n",    plp_aperta_blackhawk_tsc_rdbl_uc_var(pa,&errc,0x21)));
    PHYMOD_DIAG_OUT(("|    usr_main_tap_est         [0x22]: 0x%04X     |\n",    plp_aperta_blackhawk_tsc_rdwls_uc_var(pa,&errc,0x22)));
    PHYMOD_DIAG_OUT(("|    usr_sts_phase_hoffset    [0x24]: 0x%04X     |\n",    plp_aperta_blackhawk_tsc_rdbls_uc_var(pa,&errc,0x24)));
    PHYMOD_DIAG_OUT(("|    usr_diag_wr_ptr          [0x25]: 0x%04X     |\n",    plp_aperta_blackhawk_tsc_rdbl_uc_var(pa,&errc,0x25)));
    PHYMOD_DIAG_OUT(("|    status_byte              [0x26]: 0x%04X     |\n",    plp_aperta_blackhawk_tsc_rdbl_uc_var(pa,&errc,0x26)));
    PHYMOD_DIAG_OUT(("+-------------------------------------------------+\n"));
}

STATIC int plp_aperta_blackhawk_diagnostics_eyescan_run_uc(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags)
{
    int                 rc = PHYMOD_E_NONE;
    int                 j ;
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    if(PHYMOD_EYESCAN_F_PROCESS_GET(flags)) {
        for(j=0; j< PHYMOD_CONFIG_MAX_LANES_PER_CORE; j++) { /* Loop for all lanes. */
            if ((phy->access.lane_mask & (1<<j))==0) continue;

            phy_copy.access.lane_mask = (phy->access.lane_mask & (1<<j));

            PHYMOD_DIAG_OUT(("\n\n\n"));
            PHYMOD_DIAG_OUT(("    +--------------------------------------------------------------------+\n"));
            PHYMOD_DIAG_OUT(("    | EYESCAN Phy: 0x%03x lane_mask: 0x%02x                                 |\n", phy_copy.access.addr, phy_copy.access.lane_mask));
            PHYMOD_DIAG_OUT(("    +--------------------------------------------------------------------+\n"));

            rc  = plp_aperta_blackhawk_tsc_display_eye_scan(&(phy_copy));
            if(rc != PHYMOD_E_NONE) {
                _plp_aperta_blackhawk_diag_uc_reg_dump(&(phy_copy));
                PHYMOD_IF_ERR_RETURN(rc);
            }
        }
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_blackhawk_diagnostics_eye_margin_proj( const plp_aperta_phymod_phy_access_t* phy, uint32_t flags,
                                        const plp_aperta_phymod_phy_eyescan_options_t* eyescan_options)
{
#ifdef SERDES_API_FLOATING_POINT
  USR_DOUBLE data_rate, data_rate_in_Mhz;
  plp_aperta_phymod_phy_access_t phy_copy;
  int start_lane, num_lane, i, found=0;

  if(PHYMOD_EYESCAN_F_PROCESS_GET(flags)) {
     PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
     PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
     for (i = 0; i < num_lane; i++) {
         phy_copy.access.lane_mask = 1 << (start_lane + i);

         if (found == 0) {
             data_rate = eyescan_options->linerate_in_khz * 1000.0;
             data_rate_in_Mhz = eyescan_options->linerate_in_khz / 1000;
             found = 1;
         }
         if(num_lane > 1) {
             PHYMOD_DIAG_OUT((" l=%0d, data rate  = %fMB/s \n", i, data_rate_in_Mhz));
         } else {
             PHYMOD_DIAG_OUT((" data rate       = %fMB/s \n", data_rate_in_Mhz));
         }
         PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_eye_margin_proj(&phy_copy, data_rate, eyescan_options->ber_proj_scan_mode,
                                    eyescan_options->ber_proj_timer_cnt, eyescan_options->ber_proj_err_cnt));
     }
  }
#else
      PHYMOD_RETURN_WITH_ERR(PHYMOD_E_INTERNAL, (_PHYMOD_MSG("BER Proj is supported with SERDES_API_FLOATING_POINT only\n")));
#endif

  return PHYMOD_E_NONE;
}


int plp_aperta_blackhawk_phy_eyescan_run(const plp_aperta_phymod_phy_access_t* phy,
                           uint32_t flags,
                           plp_aperta_phymod_eyescan_mode_t mode,
                           const plp_aperta_phymod_phy_eyescan_options_t* eyescan_options)
{
    uint8_t  pmd_rx_lock=0;
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
      (plp_aperta_blackhawk_tsc_pmd_lock_status(&phy_copy, &pmd_rx_lock));

    if(pmd_rx_lock == 0) {
      PHYMOD_RETURN_WITH_ERR(PHYMOD_E_INTERNAL, (_PHYMOD_MSG("Can not get eyescan when pmd_rx is not locked\n")));
    }
    /* If stage isn't set - perform all stages*/
    if(!PHYMOD_EYESCAN_F_ENABLE_GET(flags)
       && !PHYMOD_EYESCAN_F_PROCESS_GET(flags)
       && !PHYMOD_EYESCAN_F_DONE_GET(flags))
    {
        PHYMOD_EYESCAN_F_ENABLE_SET(flags);
        PHYMOD_EYESCAN_F_PROCESS_SET(flags);
        PHYMOD_EYESCAN_F_DONE_SET(flags);
    }

    /* mode phymodEyescanModeBERProj gives BER projection */
    switch(mode) {
        case phymodEyescanModeFast:
            return plp_aperta_blackhawk_diagnostics_eyescan_run_uc(phy, flags);
        case phymodEyescanModeBERProj:
            return plp_aperta_blackhawk_diagnostics_eye_margin_proj(phy, flags, eyescan_options);
        default:
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("unsupported eyescan mode %u"), mode));
    }

  return PHYMOD_E_NONE;
}
#if 0
int blackhawk_phy_PAM4_tx_pattern_enable_set(const plp_aperta_phymod_phy_access_t* phy, phymod_PAM4_tx_pattern_t pattern_type, uint32_t enable)
{
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    switch (pattern_type) {
    case phymod_PAM4TxPattern_JP03B:
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_config_tx_jp03b_pattern(&phy_copy, (uint8_t) enable));
        break;
    case phymod_PAM4TxPattern_Linear:
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_config_tx_linearity_pattern(&phy_copy, (uint8_t) enable));
        break;
    default:
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("unsupported PAM4 tx pattern  %u"), pattern_type));
    }
    return PHYMOD_E_NONE;

}

int blackhawk_phy_PAM4_tx_pattern_enable_get(const plp_aperta_phymod_phy_access_t* phy, phymod_PAM4_tx_pattern_t pattern_type, uint32_t* enable)
{
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (blackhawk_tsc_pam4_tx_pattern_enable_get(&phy_copy, pattern_type, enable));
    return PHYMOD_E_NONE;

}

/* BER Projection*/
int blackhawk_phy_ber_proj(const plp_aperta_phymod_phy_access_t* phy, phymod_ber_proj_mode_t mode, const phymod_phy_ber_proj_options_t* options)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, i;

    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    switch (mode) {
        case phymodBERProjModePostFEC:
            if ((options->ber_proj_hist_errcnt_thresh < 3)
                || (options->ber_proj_hist_errcnt_thresh > 7)) {
                PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("hist_errcnt_threshold is out of range. Valid range is [3,7].")));
            }
            if ((options->ber_proj_timeout_s < 60)
                || (options->ber_proj_timeout_s > 120)) {
                PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("sample_time is out of range. Valid range is [60, 120].")));
            }
            if (options->ber_proj_fec_size == 0) {
                /* If user use unsupported FEC type, Portmod will set fec_size to 0.
                 * So here we check whether fec_size equals to 0. If so, return error.
                 */
                PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("Unsupported FEC type for Post FEC BER Projection.")));
            }
            for (i = 0; i < num_lane; i++) {
                phy_copy.access.lane_mask = 1 << (start_lane + i);
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_display_prbs_error_analyzer_proj(&phy_copy, options->ber_proj_fec_size,
                                                                    options->ber_proj_hist_errcnt_thresh,
                                                                    options->ber_proj_timeout_s));
            }
            break;
        default:
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("unsupported BER PROJECTION mode  %u"), mode));
    }
    return PHYMOD_E_NONE;
}
#endif

#endif /* PHYMOD_BLACKHAWK_SUPPORT */
