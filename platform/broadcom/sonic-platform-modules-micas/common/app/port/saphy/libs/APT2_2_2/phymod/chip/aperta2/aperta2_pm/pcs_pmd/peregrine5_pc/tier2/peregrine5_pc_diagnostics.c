
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
#include <peregrine5_pc.h>
#include <peregrine5_pc_diagnostics.h>
#include "peregrine5_pc/tier1/common/srds_api_enum.h"
#include "peregrine5_pc/tier1/peregrine5_pc_common.h"
#include "peregrine5_pc/tier1/peregrine5_pc_interface.h"
#include "peregrine5_pc/tier1/peregrine5_pc_dependencies.h"
#include "peregrine5_pc/tier1/peregrine5_pc_internal.h"
#include "peregrine5_pc/tier1/peregrine5_pc_access.h"
#include "peregrine5_pc/tier1/peregrine5_pc_types.h"
#include "peregrine5_pc/tier1/peregrine5_pc_prbs.h"
#include "peregrine5_pc/tier1/peregrine5_pc_cfg_seq.h"

#define PATTERN_MAX_LENGTH 240
#define PEREGRINE5_PRBS_MAX_ERROR_COUNT 0x7FFFFFFF

#ifdef PHYMOD_LINKCAT_PEREGRINE5_PC_SUPPORT
#include <include/LinkCAT_lib.h>
#endif

#ifdef PHYMOD_APERTA2_SUPPORT

STATIC
int _plp_aperta2_peregrine5_pc_prbs_poly_phymod_to_osprey(plp_aperta2_phymod_prbs_poly_t phymod_poly, enum plp_aperta2_srds_prbs_polynomial_enum *osprey_poly)
{
    switch(phymod_poly){
    case phymodPrbsPoly7:
        *osprey_poly = PRBS_7;
        break;
    case phymodPrbsPoly9:
        *osprey_poly = PRBS_9;
        break;
    case phymodPrbsPoly11:
        *osprey_poly = PRBS_11;
        break;
    case phymodPrbsPoly15:
        *osprey_poly = PRBS_15;
        break;
    case phymodPrbsPoly23:
        *osprey_poly = PRBS_23;
        break;
    case phymodPrbsPoly31:
        *osprey_poly = PRBS_31;
        break;
    case phymodPrbsPoly58:
        *osprey_poly = PRBS_58;
        break;
    case phymodPrbsPoly49:
        *osprey_poly = PRBS_49;
        break;
    case phymodPrbsPoly10:
        *osprey_poly = PRBS_10;
        break;
    case phymodPrbsPoly20:
        *osprey_poly = PRBS_20;
        break;
    case phymodPrbsPoly13:
        *osprey_poly = PRBS_13;
        break;
    default:
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("unsupported poly for blackhawk %u"), phymod_poly));
    }
    return PHYMOD_E_NONE;
}

STATIC
int _plp_aperta2_peregrine5_pc_prbs_poly_osprey_to_phymod(enum plp_aperta2_srds_prbs_polynomial_enum  *osprey_poly, plp_aperta2_phymod_prbs_poly_t *phymod_poly)
{
    switch(*osprey_poly){
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
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_INTERNAL, (_PHYMOD_MSG("uknown poly %u"), *osprey_poly));
    }
    return PHYMOD_E_NONE;
}


int plp_aperta2_peregrine5_pc_phy_prbs_config_set(const plp_aperta2_phymod_phy_access_t* phy,
                                       uint32_t flags,
                                       const plp_aperta2_phymod_prbs_t* prbs)
{
    enum plp_aperta2_srds_prbs_polynomial_enum osprey_poly;
    plp_aperta2_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, i;
    phymod_firmware_lane_config_t fw_config;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /* next to see if the port is PAM4 or not */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_phy_firmware_lane_config_get(phy, &fw_config));

    PHYMOD_IF_ERR_RETURN(_plp_aperta2_peregrine5_pc_prbs_poly_phymod_to_osprey(prbs->poly, &osprey_poly));
    /*first check which direction */
    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 1 << (start_lane + i);
        if (PHYMOD_PRBS_DIRECTION_RX_GET(flags)) {
            /* if PAM4 speed, use mode 2 */
            if (fw_config.ForcePAM4Mode) {
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_peregrine5_pc_config_rx_prbs(&phy_copy, osprey_poly, PRBS_INITIAL_SEED_NO_HYSTERESIS,  prbs->invert));
            } else {
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_peregrine5_pc_config_rx_prbs(&phy_copy, osprey_poly, PRBS_INITIAL_SEED_HYSTERESIS,  prbs->invert));
            }
        } else if (PHYMOD_PRBS_DIRECTION_TX_GET(flags)) {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_config_tx_prbs(&phy_copy, osprey_poly, prbs->invert));
        } else {
            /* if PAM4 speed, use mode 2 */
            if (fw_config.ForcePAM4Mode) {
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_peregrine5_pc_config_rx_prbs(&phy_copy, osprey_poly, PRBS_INITIAL_SEED_NO_HYSTERESIS,  prbs->invert));
            } else {
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_peregrine5_pc_config_rx_prbs(&phy_copy, osprey_poly, PRBS_INITIAL_SEED_HYSTERESIS,  prbs->invert));
            }
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_config_tx_prbs(&phy_copy, osprey_poly, prbs->invert));
        }
    }
    return PHYMOD_E_NONE;
}

int plp_aperta2_peregrine5_pc_phy_prbs_config_get(const plp_aperta2_phymod_phy_access_t* phy,
                                       uint32_t flags,
                                       plp_aperta2_phymod_prbs_t* prbs)
{
    plp_aperta2_phymod_prbs_t config_tmp;
    enum plp_aperta2_srds_prbs_polynomial_enum osprey_poly;
    enum plp_aperta2_srds_prbs_checker_mode_enum prbs_checker_mode;
    plp_aperta2_phymod_phy_access_t phy_copy;
    uint8_t invert;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));


    if (PHYMOD_PRBS_DIRECTION_TX_GET(flags)) {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_get_tx_prbs_config(&phy_copy, &osprey_poly, &invert));
        config_tmp.invert = invert;
        PHYMOD_IF_ERR_RETURN(_plp_aperta2_peregrine5_pc_prbs_poly_osprey_to_phymod(&osprey_poly, &config_tmp.poly));
        prbs->invert = config_tmp.invert;
        prbs->poly = config_tmp.poly;
    } else if (PHYMOD_PRBS_DIRECTION_RX_GET(flags)) {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_get_rx_prbs_config(&phy_copy,
                                                                &osprey_poly,
                                                                &prbs_checker_mode,
                                                                &invert));
        config_tmp.invert = invert;
        PHYMOD_IF_ERR_RETURN(_plp_aperta2_peregrine5_pc_prbs_poly_osprey_to_phymod(&osprey_poly, &config_tmp.poly));
        prbs->invert = config_tmp.invert;
        prbs->poly = config_tmp.poly;
    } else {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_get_tx_prbs_config(&phy_copy, &osprey_poly,  &invert));
        config_tmp.invert = invert;
        PHYMOD_IF_ERR_RETURN(_plp_aperta2_peregrine5_pc_prbs_poly_osprey_to_phymod(&osprey_poly, &config_tmp.poly));
        prbs->invert = config_tmp.invert;
        prbs->poly = config_tmp.poly;
    }
    return PHYMOD_E_NONE;
}

int plp_aperta2_peregrine5_pc_phy_prbs_enable_set(const plp_aperta2_phymod_phy_access_t* phy,
                                       uint32_t flags, uint32_t enable)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, i;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /*first check which direction */
    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 1 << (start_lane + i);
        if (PHYMOD_PRBS_DIRECTION_TX_GET(flags)) {
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_tx_prbs_en(&phy_copy, enable));
        } else if (PHYMOD_PRBS_DIRECTION_RX_GET(flags)) {
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_rx_prbs_en(&phy_copy, enable));
        } else {
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_tx_prbs_en(&phy_copy, enable));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_rx_prbs_en(&phy_copy, enable));
        }
    }
    return PHYMOD_E_NONE;
}

int plp_aperta2_peregrine5_pc_phy_prbs_enable_get(const plp_aperta2_phymod_phy_access_t* phy,
                                       uint32_t flags, uint32_t* enable)
{
    uint8_t enable_tmp;
    plp_aperta2_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    if (PHYMOD_PRBS_DIRECTION_TX_GET(flags)) {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_get_tx_prbs_en(&phy_copy, &enable_tmp));
        *enable = enable_tmp;
    } else if (PHYMOD_PRBS_DIRECTION_RX_GET(flags)) {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_get_rx_prbs_en(&phy_copy, &enable_tmp));
        *enable = enable_tmp;
    } else {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_get_tx_prbs_en(&phy_copy, &enable_tmp));
        *enable = enable_tmp;
        PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_get_rx_prbs_en(&phy_copy, &enable_tmp));
        *enable &= enable_tmp;
    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_peregrine5_pc_phy_prbs_status_get(const plp_aperta2_phymod_phy_access_t* phy,
                                       uint32_t flags,
                                       plp_aperta2_phymod_prbs_status_t* prbs_status)
{
    uint8_t status = 0, status_ori = 0;
    uint32_t prbs_err_count = 0, prbs_err_count_temp = 0;
    int i, start_lane, num_lane;
    plp_aperta2_phymod_phy_access_t phy_copy;
    enum plp_aperta2_srds_prbs_polynomial_enum osprey_poly;
    enum plp_aperta2_srds_prbs_checker_mode_enum prbs_checker_mode;
    uint8_t invert, chk_en;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    /* next figure out the lane num and start_lane based on the input */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    prbs_status->error_count = 0;
    prbs_status->prbs_lock_loss = 0;
    prbs_status->prbs_lock = 1;

    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 0x1 << (i + start_lane);

        PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_get_rx_prbs_config(&phy_copy,
                                                                  &osprey_poly,
                                                                  &prbs_checker_mode,
                                                                  &invert));
        if (prbs_checker_mode == PRBS_INITIAL_SEED_HYSTERESIS) {
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_prbs_chk_lock_state(&phy_copy, &status));
            if (status) {
                /*next check the lost of lock and error count */
                status = 0;
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_peregrine5_pc_prbs_err_count_state(&phy_copy, &prbs_err_count, &status));
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
        } else if (prbs_checker_mode == PRBS_INITIAL_SEED_NO_HYSTERESIS) {
            /* When PRBS_CHK_MODE is 2, PRBS checker will declare out-of-lock and
             * resync only if PRBS checker is re-enabled or CDR loses lock.
             * As a result, for this checker mode, we need to toggle the prbs_chk_en
             * bit to determine whether current PRBS is locked or not.
             */
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_get_rx_prbs_en(&phy_copy, &chk_en));
            if (!chk_en) {
                PHYMOD_DEBUG_INFO((" Lane :: %d PRBS not locked\n", i ));
                prbs_status->prbs_lock = 0;
            } else {
                /* 1. Read current PRBS error count. If we confirm PRBS is locked and
                 * this value is not saturated, this will be the error count returned
                 * by this function.
                 */
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_peregrine5_pc_prbs_err_count_state(&phy_copy,
                                                            &prbs_err_count,
                                                            &status_ori));
                /* 2. Toggle prbs_chk_en bit. Read the PRBS counter to clear
                 * the counter and lock_loss status. Then check PRBS lock status.
                 */
                PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_rx_prbs_en(&phy_copy, 0));
                PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_rx_prbs_en(&phy_copy, 1));
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_peregrine5_pc_prbs_err_count_state(&phy_copy,
                                                            &prbs_err_count_temp,
                                                            &status));
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_peregrine5_pc_prbs_chk_lock_state(&phy_copy,
                                                           &status));
                /* 3. If current PRBS is locked while in step 1 we have counter
                 * value saturated or status_ori == 1 (PRBS lost lock),
                 * it means PRBS loses lock in between.
                 * Otherwise, PRBS is always locked and we take the counter
                 * value in step 1.
                 *
                 * If current PRBS is not locked, return prbs_lock = 0.
                 */
                if (status) {
                    if ((prbs_err_count == PEREGRINE5_PRBS_MAX_ERROR_COUNT) || status_ori) {
                        prbs_status->prbs_lock_loss = 1;
                    } else {
                        prbs_status->error_count += prbs_err_count;
                    }
                } else {
                    PHYMOD_DEBUG_INFO((" Lane :: %d PRBS not locked\n", i ));
                    prbs_status->prbs_lock = 0;
                }
            }
        } else {
            return PHYMOD_E_UNAVAIL;
        }
    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_peregrine5_pc_phy_pattern_config_set(const plp_aperta2_phymod_phy_access_t* phy,
                                         const plp_aperta2_phymod_pattern_t* pattern)
{
    int i,j = 0, bit;
    char patt[PATTERN_MAX_LENGTH+1];
    plp_aperta2_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    for (i=0; i< PATTERN_MAX_SIZE; i++)
    {
      for (j=0;j<32 && i*32+j <= PATTERN_MAX_LENGTH; j++)
      {
          if (i*32+j == (int)pattern->pattern_len) {
              break;
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
      if (i*32+j == (int)pattern->pattern_len && i*32+j <= PATTERN_MAX_LENGTH) {
          /* coverity[overrun-local] */
          patt[i*32+j] = '\0';
          break;
      }
    }
    /* coverity[divide_by_zero : FALSE] */
    PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_config_shared_tx_pattern (&phy_copy,
                (uint8_t) pattern->pattern_len, (const char *) patt));

    return PHYMOD_E_NONE;
}

int plp_aperta2_peregrine5_pc_phy_pattern_config_get(const plp_aperta2_phymod_phy_access_t* phy,
                                          plp_aperta2_phymod_pattern_t* pattern)
{
    plp_aperta2_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_config_shared_tx_pattern_idx_get(&phy_copy,
                                  &pattern->pattern_len,
                                  pattern->pattern));

    return PHYMOD_E_NONE;
}

int plp_aperta2_peregrine5_pc_phy_pattern_enable_set(const plp_aperta2_phymod_phy_access_t* phy,
                                          uint32_t enable,
                                          const plp_aperta2_phymod_pattern_t* pattern)
{
    plp_aperta2_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_tx_shared_patt_gen_en(&phy_copy, (uint8_t) enable, (uint8_t)pattern->pattern_len));

    return PHYMOD_E_NONE;
}

int plp_aperta2_peregrine5_pc_phy_pattern_enable_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t* enable, const plp_aperta2_phymod_pattern_t *tx_pattern)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    uint8_t enable_8;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
       (plp_aperta2_peregrine5_pc_tx_shared_patt_gen_en_get(&phy_copy, &enable_8));

    *enable = enable_8;

    return PHYMOD_E_NONE;
}
#if 0
int  peregrine5_pc_phy_tx_pattern_set(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_pattern_t tx_pattern)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    plp_aperta2_phymod_pattern_t custom_pattern;
    uint32_t pattern_word[PHYMOD_NUM_CUSTOM_PATTERN_WORD];

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    switch (tx_pattern) {
        case phymodPhyTxPatternOff:
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_config_tx_jp03b_pattern(&phy_copy, 0));
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_config_tx_linearity_pattern(&phy_copy, 0));
            break;
        case phymodPhyTxPatternPAM4_JP03B:
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_config_tx_linearity_pattern(&phy_copy, 0));
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_config_tx_jp03b_pattern(&phy_copy, 1));
            break;
        case phymodPhyTxPatternPAM4_Linear:
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_config_tx_jp03b_pattern(&phy_copy, 0));
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_config_tx_linearity_pattern(&phy_copy, 1));
            break;
        case phymodPhyTxPatternCustom:
            custom_pattern.pattern = pattern_word;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_phy_pattern_config_get(&phy_copy, &custom_pattern));
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_config_tx_jp03b_pattern(&phy_copy, 0));
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_config_tx_linearity_pattern(&phy_copy, 0));
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_phy_pattern_enable_set(&phy_copy, 1, &custom_pattern));
            break;
        default:
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("unsupported tx pattern  %d"),
                                   tx_pattern));
    }
    return PHYMOD_E_NONE;
}

int peregrine5_pc_phy_tx_pattern_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_pattern_t* tx_pattern)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    uint32_t JP03B_enable=0;
    uint32_t linear_enable=0;
    uint8_t pattern_gen_enable=0;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_pam4_tx_pattern_enable_get(&phy_copy,
                                                    phymod_PAM4TxPattern_JP03B,
                                                    &JP03B_enable));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_pam4_tx_pattern_enable_get(&phy_copy,
                                                    phymod_PAM4TxPattern_Linear,
                                                    &linear_enable));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_tx_shared_patt_gen_en_get(&phy_copy,
                                                   &pattern_gen_enable));
    /*
     * JP03B need to check first since that is higher prioirty over
     * Linear. If both are enabled, JP03B will take affect.
     */
    if (pattern_gen_enable) {
        if (JP03B_enable) {
            *tx_pattern = phymodPhyTxPatternPAM4_JP03B;
        } else if (linear_enable) {
            *tx_pattern = phymodPhyTxPatternPAM4_Linear;
        } else {
            *tx_pattern = phymodPhyTxPatternCustom;
        }
    } else {
        *tx_pattern = phymodPhyTxPatternOff;
    }
    return PHYMOD_E_NONE;
}
#endif
int plp_aperta2_peregrine5_pc_phy_pmd_info_dump(const plp_aperta2_phymod_phy_access_t* phy, char *type)
{
    int start_lane, num_lane, i, j;
    uint32_t tmp_lane_mask;
    plp_aperta2_phymod_phy_access_t phy_copy;

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
    } else if ((!PHYMOD_STRCMP(type, "state_eye")) ||
            (!PHYMOD_STRCMP(type, "State_Eye")) ||
            (!PHYMOD_STRCMP(type, "STATE_EYE"))) {
        cmd_type = phymod_diag_STATE_EYE;
    } else if ((!PHYMOD_STRCMP(type, "verbose")) ||
            (!PHYMOD_STRCMP(type, "Verbose")) ||
            (!PHYMOD_STRCMP(type, "VERBOSE"))) {
        cmd_type = phymod_diag_ALL;
    } else if (!PHYMOD_STRCMP(type, "STD")) {
        cmd_type = phymod_diag_DSC_STD;
    } else {
        cmd_type = phymod_diag_STATE;
    }

    PHYMOD_DEBUG_ERROR((" %s:%d type = %d laneMask  = 0x%X\n", __func__, __LINE__, cmd_type, phy->access.lane_mask));

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /* Make sure information table is initialized */
    tsc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr(&phy_copy);
    if (tsc_info_ptr->signature == 0) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_init_peregrine5_pc_info(&phy_copy));
    }
    /*next figure out the lane num and start_lane based on the input*/
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    if (cmd_type == phymod_diag_DSC) {
        for (i = start_lane; i < start_lane + num_lane; i++) {
            if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, i)) {
                continue;
            }
            phy_copy.access.lane_mask = 0x1 << i ;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_display_diag_data(&phy_copy, SRDS_DIAG_CORE));
        }
    } else if (cmd_type == phymod_diag_DSC_STD) {
        PHYMOD_DIAG_OUT(("    +--------------------------------------------------------------------+\n"));
        PHYMOD_DIAG_OUT(("    | DSC Phy: 0x%03x lane_mask: 0x%02x                                 |\n", phy->access.addr, phy->access.lane_mask));
        PHYMOD_DIAG_OUT(("    +--------------------------------------------------------------------+\n"));
        tmp_lane_mask = phy_copy.access.lane_mask;
        for (j = start_lane; j < (start_lane+num_lane); j++) {
            PHYMOD_DIAG_OUT(("    +--------------------------------------------------------------------+\n"));
            PHYMOD_DIAG_OUT(("    +OCTAL: %d +\n", (j >= 8) ? 1 : 0));
            PHYMOD_DIAG_OUT(("    +--------------------------------------------------------------------+\n"));
            phy_copy.access.lane_mask = 0x1 << j;
            if (j == start_lane)  {
                /* Serdes dump event log for all the core, so
                 * for first time use 0xFF0*/
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_peregrine5_pc_display_diag_data(&phy_copy, 0x7));
            } else {
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_peregrine5_pc_display_diag_data(&phy_copy, 0x1));
            }
        }
    } else if (cmd_type == phymod_diag_STATE_EYE) {
        PHYMOD_DEBUG_ERROR((" %s:%d type = DEF\n", __func__, __LINE__));
        for (j = start_lane; j < (start_lane+num_lane); j++) {
            phy_copy.access.lane_mask = 0x1 << j;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_display_diag_data(&phy_copy, SRDS_DIAG_CORE | SRDS_DIAG_LANE | SRDS_DIAG_EVENT | SRDS_DIAG_REG_LANE | SRDS_DIAG_REG_CORE | SRDS_DIAG_UC_LANE | SRDS_DIAG_UC_CORE | SRDS_DIAG_EYE));
        }
    } else {
        switch (cmd_type) {
            case phymod_diag_CFG:
                PHYMOD_DEBUG_ERROR((" %s:%d type = CFG\n", __func__, __LINE__));
                tmp_lane_mask = phy_copy.access.lane_mask;
                phy_copy.access.lane_mask = 1;
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_peregrine5_pc_display_core_config(&phy_copy));
                phy_copy.access.lane_mask = tmp_lane_mask;
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_peregrine5_pc_display_lane_config(&phy_copy));
                break;

            case phymod_diag_CL72:
                PHYMOD_DEBUG_ERROR((" %s:%d type = CL72\n", __func__, __LINE__));
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_peregrine5_pc_display_linktrn_status(&phy_copy));
                break;

            case phymod_diag_DEBUG:
                PHYMOD_DEBUG_ERROR((" %s:%d type = DBG\n", __func__, __LINE__));
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_peregrine5_pc_display_lane_debug_status(&phy_copy));
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
                for (j = start_lane; j < (start_lane+num_lane); j++) {
                    PHYMOD_DIAG_OUT(("    +--------------------------------------------------------------------+\n"));
                    PHYMOD_DIAG_OUT(("    +OCTAL: %d +\n", (j >= 8) ? 1 : 0));
                    PHYMOD_DIAG_OUT(("    +--------------------------------------------------------------------+\n"));

                    phy_copy.access.lane_mask = 0x1 << j;
                    /* Serdes dump event log for all the core, so
                     * for first time use 0xFF0*/
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta2_peregrine5_pc_display_diag_data(&phy_copy, 0x7F0));
                }
                break;
            case phymod_diag_STATE:
            default:
                PHYMOD_DEBUG_ERROR((" %s:%d type = DEF\n", __func__, __LINE__));
                for (j = 0; j < 8; j++) {
                    phy_copy.access.lane_mask = 0x1 << j;
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta2_peregrine5_pc_display_diag_data(&phy_copy, SRDS_DIAG_CORE | SRDS_DIAG_LANE | SRDS_DIAG_EVENT));
                }
                break;
        }
    }
    return PHYMOD_E_NONE;
}

static void _plp_aperta2_peregrine5_tc_diag_uc_reg_dump(plp_aperta2_phymod_phy_access_t *pa)
{
    err_code_t errc;

    COMPILER_REFERENCE(errc);

    PHYMOD_DIAG_OUT(("+-------------------------------------------------+\n"));
    PHYMOD_DIAG_OUT(("|    MICRO CODE USR CTRL CONFIGURATION REGISTERS  |\n"));
    PHYMOD_DIAG_OUT(("+-------------------------------------------------+\n"));
    PHYMOD_DIAG_OUT(("|    disable_steady_state     [0x00]: 0x%08X      |\n",    (((uint32_t)plp_aperta2_peregrine5_pc_rdwlp_uc_var(pa,&errc,0x2)<<16)|(uint32_t)plp_aperta2_peregrine5_pc_rdwlp_uc_var(pa,&errc,0x0))));
    PHYMOD_DIAG_OUT(("|    disable_startup          [0x04]: 0x%08X      |\n",    (((uint32_t)plp_aperta2_peregrine5_pc_rdwlp_uc_var(pa,&errc,0x6)<<16)|(uint32_t)plp_aperta2_peregrine5_pc_rdwlp_uc_var(pa,&errc,0x4))));
    PHYMOD_DIAG_OUT(("|    disable_blind            [0x08]: 0x%08X      |\n",    (((uint32_t)plp_aperta2_peregrine5_pc_rdwlp_uc_var(pa,&errc,0xa)<<16)|(uint32_t)plp_aperta2_peregrine5_pc_rdwlp_uc_var(pa,&errc,0x8))));
    PHYMOD_DIAG_OUT(("|    usr_misc_ctrl_word       [0x0C]: 0x%04X      |\n",    plp_aperta2_peregrine5_pc_rdwlp_uc_var(pa,&errc,0xc)));
    PHYMOD_DIAG_OUT(("|    lane_event_log_level     [0x0E]: 0x%04X      |\n",    plp_aperta2_peregrine5_pc_rdblp_uc_var(pa,&errc,0xe)));
    PHYMOD_DIAG_OUT(("|    cl93n72_frc_byte         [0x0F]: 0x%04X      |\n",    plp_aperta2_peregrine5_pc_rdblp_uc_var(pa,&errc,0xf)));
    PHYMOD_DIAG_OUT(("|    config_word              [0x10]: 0x%04X      |\n",    plp_aperta2_peregrine5_pc_rdwlp_uc_var(pa,&errc,0x10)));
    PHYMOD_DIAG_OUT(("+-------------------------------------------------+\n"));
    PHYMOD_DIAG_OUT(("|         MICRO CODE USER STATUS REGISTERS        |\n"));
    PHYMOD_DIAG_OUT(("+-------------------------------------------------+\n"));
    PHYMOD_DIAG_OUT(("|    restart_counter          [0x12]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdblp_uc_var(pa,&errc,0x12)));
    PHYMOD_DIAG_OUT(("|    reset_counter            [0x13]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdblp_uc_var(pa,&errc,0x13)));
    PHYMOD_DIAG_OUT(("|    micro_stopped            [0x19]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdblp_uc_var(pa,&errc,0x16)));
    PHYMOD_DIAG_OUT(("|    pmd_lock_counter         [0x14]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdblp_uc_var(pa,&errc,0x1a)));
    PHYMOD_DIAG_OUT(("|    link_time                [0x08]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwl_uc_var(pa,&errc,0x8)));
    PHYMOD_DIAG_OUT(("|    veye_upper               [0x0A]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwl_uc_var(pa,&errc,0xa)));
    PHYMOD_DIAG_OUT(("|    veye_lower               [0x0C]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwl_uc_var(pa,&errc,0xc)));
    PHYMOD_DIAG_OUT(("|    tp_metric_1              [0x0E]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdbl_uc_var(pa,&errc,0xe)));
    PHYMOD_DIAG_OUT(("|    tp_metric_2              [0x0F]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdbl_uc_var(pa,&errc,0xf)));
    PHYMOD_DIAG_OUT(("|    flt_sum                  [0x10]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwl_uc_var(pa,&errc,0x10)));
    PHYMOD_DIAG_OUT(("|    flt_max                  [0x12]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdbls_uc_var(pa,&errc,0x12)));
    PHYMOD_DIAG_OUT(("|    rxffe_taps_0             [0x14]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwls_uc_var(pa,&errc,0x14)));
    PHYMOD_DIAG_OUT(("|    rxffe_taps_1             [0x16]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwls_uc_var(pa,&errc,0x16)));
    PHYMOD_DIAG_OUT(("|    rxffe_taps_2             [0x18]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwls_uc_var(pa,&errc,0x18)));
    PHYMOD_DIAG_OUT(("|    rxffe_taps_3             [0x1A]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwls_uc_var(pa,&errc,0x1a)));
    PHYMOD_DIAG_OUT(("|    rxffe_taps_4             [0x1C]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwls_uc_var(pa,&errc,0x1c)));
    PHYMOD_DIAG_OUT(("|    rxffe_taps_5             [0x1E]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwls_uc_var(pa,&errc,0x1e)));
    PHYMOD_DIAG_OUT(("|    sar_offset_precal_min    [0x26]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdbls_uc_var(pa,&errc,0x26)));
    PHYMOD_DIAG_OUT(("|    sar_offset_precal_max    [0x27]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdbls_uc_var(pa,&errc,0x27)));
    PHYMOD_DIAG_OUT(("|    sar_offset_postcal_min   [0x28]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdbls_uc_var(pa,&errc,0x28)));
    PHYMOD_DIAG_OUT(("|    sar_offset_postcal_max   [0x29]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdbls_uc_var(pa,&errc,0x29)));
    PHYMOD_DIAG_OUT(("|    sar_gain_precal_min      [0x2A]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwls_uc_var(pa,&errc,0x2a)));
    PHYMOD_DIAG_OUT(("|    sar_gain_precal_max      [0x2C]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwls_uc_var(pa,&errc,0x2c)));
    PHYMOD_DIAG_OUT(("|    sar_gain_precal_ref      [0x2E]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwls_uc_var(pa,&errc,0x2e)));
    PHYMOD_DIAG_OUT(("|    sar_gain_postcal_min     [0x30]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwls_uc_var(pa,&errc,0x30)));
    PHYMOD_DIAG_OUT(("|    sar_gain_postcal_max     [0x32]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwls_uc_var(pa,&errc,0x32)));
    PHYMOD_DIAG_OUT(("|    sar_gain_postcal_ref     [0x34]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwls_uc_var(pa,&errc,0x34)));
    PHYMOD_DIAG_OUT(("|    tuningdone_time          [0x3A]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwl_uc_var(pa,&errc,0x3a)));
    PHYMOD_DIAG_OUT(("|    dyn_lane_status_word     [0x4E]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwl_uc_var(pa,&errc,0x4e)));
    PHYMOD_DIAG_OUT(("|    veye_margin_0            [0x5E]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwls_uc_var(pa,&errc,0x5e)));
    PHYMOD_DIAG_OUT(("|    veye_margin_1            [0x60]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwls_uc_var(pa,&errc,0x60)));
    PHYMOD_DIAG_OUT(("|    veye_margin_2            [0x62]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwls_uc_var(pa,&errc,0x62)));
    PHYMOD_DIAG_OUT(("+-------------------------------------------------+\n"));
    PHYMOD_DIAG_OUT(("|            MICRO CODE MISC REGISTERS            |\n"));
    PHYMOD_DIAG_OUT(("+-------------------------------------------------+\n"));
    PHYMOD_DIAG_OUT(("|    usr_diag_mode            [0x18]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwlp_uc_var(pa,&errc,0x18)));
    PHYMOD_DIAG_OUT(("|    usr_diag_wr_ptr          [0x20]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwl_uc_var(pa,&errc,0x20)));
    PHYMOD_DIAG_OUT(("|    usr_diag_rd_ptr          [0x22]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwl_uc_var(pa,&errc,0x22)));
    PHYMOD_DIAG_OUT(("|    usr_diag_status          [0x24]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdwl_uc_var(pa,&errc,0x24)));
    PHYMOD_DIAG_OUT(("|    max_time_control         [0x11]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdbc_uc_var(pa,&errc,0x11)));
    PHYMOD_DIAG_OUT(("|    max_err_control          [0x12]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdbls_uc_var(pa,&errc,0x12)));
    PHYMOD_DIAG_OUT(("|    status_byte              [0x17]: 0x%04X     |\n",    plp_aperta2_peregrine5_pc_rdblp_uc_var(pa,&errc,0x17)));
    PHYMOD_DIAG_OUT(("+-------------------------------------------------+\n"));
}

STATIC int plp_aperta2_peregrine5_pc_diagnostics_eyescan_run_uc(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, int intrusive_eyescan)
{
    int                 rc = PHYMOD_E_NONE;
    int                 j ;
    plp_aperta2_phymod_phy_access_t phy_copy;
    enum peregrine5_pc_rx_pam_mode_enum pam_mode = NRZ;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    if(PHYMOD_EYESCAN_F_PROCESS_GET(flags)) {
        for(j=0; j< PHYMOD_CONFIG_MAX_LANES_PER_CORE; j++) { /* Loop for all lanes. */
            if ((phy->access.lane_mask & (1<<j))==0) continue;

            phy_copy.access.lane_mask = (phy->access.lane_mask & (1<<j));

            PHYMOD_DIAG_OUT(("\n\n\n"));
            PHYMOD_DIAG_OUT(("    +--------------------------------------------------------------------+\n"));
            PHYMOD_DIAG_OUT(("    | EYESCAN Phy: 0x%03x lane_mask: 0x%02x                                 |\n", phy_copy.access.addr, phy_copy.access.lane_mask));
            PHYMOD_DIAG_OUT(("    +--------------------------------------------------------------------+\n"));

            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_INTERNAL_get_rx_pam_mode(&(phy_copy), &pam_mode));
            if (pam_mode == NRZ) {
                if (!intrusive_eyescan) {
                    rc = plp_aperta2_peregrine5_pc_display_eye_scan(&(phy_copy));
                } else {
                    PHYMOD_DEBUG_ERROR((" NRZ mode doesn't support Intrusive eyescan\n" ));
                }
            } else if ((pam_mode == PAM4_NR) || (pam_mode == PAM4_ER)) {
                if (intrusive_eyescan) {
                    /* Intrusive Eye scan for PAM4 */
                    PHYMOD_DIAG_OUT((" Running Intrusive Eye Scan during traffic can affact link performance\n\n"));
                    rc = plp_aperta2_peregrine5_pc_display_intrusive_eye_scan(&(phy_copy));
                } else {
                    rc = plp_aperta2_peregrine5_pc_display_eye_scan(&(phy_copy));
                }
            }
            if(rc != PHYMOD_E_NONE) {
                _plp_aperta2_peregrine5_tc_diag_uc_reg_dump(&(phy_copy));
                PHYMOD_IF_ERR_RETURN(rc);
            }
        }
    }
    return PHYMOD_E_NONE;
}

int plp_aperta2_peregrine5_pc_phy_eyescan_run(const plp_aperta2_phymod_phy_access_t* phy,
                           uint32_t flags,
                           plp_aperta2_phymod_eyescan_mode_t mode,
                           const plp_aperta2_phymod_phy_eyescan_options_t* eyescan_options)
{
    uint8_t  pmd_rx_lock=0;
    plp_aperta2_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
      (plp_aperta2_peregrine5_pc_pmd_lock_status(&phy_copy, &pmd_rx_lock));

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

    switch(mode) {
        case phymodEyescanModeFast:
            return plp_aperta2_peregrine5_pc_diagnostics_eyescan_run_uc(phy, flags, 0);
        /*case phymodEyescanModeIntrusive:
            return plp_aperta2_peregrine5_pc_diagnostics_eyescan_run_uc(phy, flags, 1); */
        default:
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("unsupported eyescan mode %u"), mode));
    }

    return PHYMOD_E_NONE;
}
#if 0
int peregrine5_pc_phy_PAM4_tx_pattern_enable_set(const plp_aperta2_phymod_phy_access_t* phy,
                                                  phymod_PAM4_tx_pattern_t pattern_type,
                                                  uint32_t enable)
{
   plp_aperta2_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    switch (pattern_type) {
    case phymod_PAM4TxPattern_JP03B:
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_config_tx_jp03b_pattern(&phy_copy, (uint8_t) enable));
        break;
    case phymod_PAM4TxPattern_Linear:
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_config_tx_linearity_pattern(&phy_copy, (uint8_t) enable));
        break;
    default:
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("unsupported PAM4 tx pattern  %u"), pattern_type));
    }

    return PHYMOD_E_NONE;
}

int peregrine5_pc_phy_PAM4_tx_pattern_enable_get(const plp_aperta2_phymod_phy_access_t* phy,
                                                  phymod_PAM4_tx_pattern_t pattern_type,
                                                  uint32_t* enable)
{
    plp_aperta2_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_pam4_tx_pattern_enable_get(&phy_copy, pattern_type, enable));

    return PHYMOD_E_NONE;
}

static
int _peregrine5_pc_phy_post_fec_ber_proj(const plp_aperta2_phymod_phy_access_t* phy, const phymod_phy_ber_proj_options_t* options)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    peregrine5_pc_prbs_err_analyzer_lane_config_st err_analyzer;
    peregrine5_pc_prbs_err_analyzer_common_config_st err_analyzer_common_config;
    peregrine5_pc_prbs_err_analyzer_lane_status_st err_analyzer_lane_status;
    int start_lane, num_lane, i;
    enum peregrine5_pc_rx_pam_mode_enum pam_mode = NRZ;
    uint8_t hrs = 0, mins = 0, secs = 0;
#ifdef SERDES_API_FLOATING_POINT
    USR_DOUBLE ber;
    peregrine5_pc_prbs_err_analyzer_report_st err_analyzer_rept;
    uint16_t x = 0, y = 0, z = 0;
#endif /* SERDES_API_FLOATING_POINT */

    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_MEMSET(&err_analyzer, 0x0, sizeof(err_analyzer));
    PHYMOD_MEMSET(&err_analyzer_common_config, 0, sizeof(peregrine5_pc_prbs_err_analyzer_common_config_st));
    PHYMOD_MEMSET(&err_analyzer_lane_status, 0, sizeof(err_analyzer_lane_status));

    switch (options->ber_proj_phase) {
        case PHYMOD_BER_PROJ_PHASE_F_PRE:
            /*
             *  This is pre-config stage to calculate optimal hist_errcnt_threshold.
             *  Not applicable to BH7 since errcnt_threshold is inherently handled by tier 1 functions.
             *  Value passed by users are ignored.
             */
            break;
        case PHYMOD_BER_PROJ_PHASE_F_CONFIG:
            if (options->ber_proj_fec_size == 0) {
                /* If user use unsupported FEC type, Portmod will set fec_size to 0.
                 * So here we check whether fec_size equals to 0. If so, return error.
                 */
                PHYMOD_RETURN_WITH_ERR
                    (PHYMOD_E_PARAM, (_PHYMOD_MSG("Unsupported FEC type for Post FEC BER Projection.")));
            }
            for (i = 0; i < num_lane; i++) {
                phy_copy.access.lane_mask = 1 << (start_lane + i);
                /* fec error analyzer works at PAM4 mode only */
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_peregrine5_pc_INTERNAL_get_rx_pam_mode(&phy_copy, &pam_mode));
                if ( pam_mode != PAM4_NR && pam_mode != PAM4_ER ) {
                    PHYMOD_RETURN_WITH_ERR
                        (PHYMOD_E_PARAM, (_PHYMOD_MSG("Error: FEC error analyzer only supported in PAM4 mode.\n")));
                }
            }
            for (i = 0; i < num_lane; i++) {
                phy_copy.access.lane_mask = 1 << (start_lane + i);

                PHYMOD_MEMSET(&err_analyzer, 0, sizeof(peregrine5_pc_prbs_err_analyzer_lane_config_st));
                PHYMOD_MEMSET(&err_analyzer_common_config, 0, sizeof(peregrine5_pc_prbs_err_analyzer_common_config_st));

                /* Dump PRBS Error Analyzer errors in Diag display format to be used later by decoder to decode*/
                err_analyzer_common_config.encrypt_mode = 0x1;
                err_analyzer_common_config.prbs_err_aggregate_mode = PEREGRINE5_PC_FEC_100GE;
                err_analyzer_common_config.lane_mask_aggregate = 0xff;

                err_analyzer.common_config_ptr = &err_analyzer_common_config;
                err_analyzer.timeout_s = options->ber_proj_timeout_s;
                if (options->ber_proj_fec_size == 5440) {
                    err_analyzer.fec_code_type = PEREGRINE5_PC_RS_544_514_10;
                } else {
                    err_analyzer.fec_code_type = PEREGRINE5_PC_RS_528_514_10;
                }
                err_analyzer.prbs_err_fec_size = options->ber_proj_fec_size;

                /* Reset PRBS Error Analyzer and clear DAC */
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_peregrine5_pc_prbs_error_analyzer_reset(&phy_copy, &err_analyzer_common_config));
                /* Configure PRBS Error Analyzer Aggregation Mode*/
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_peregrine5_pc_prbs_error_analyzer_aggregate_config(&phy_copy, &err_analyzer));
            }
            break;
        case PHYMOD_BER_PROJ_PHASE_F_START:
            for (i = 0; i < num_lane; i++) {
                phy_copy.access.lane_mask = 1 << (start_lane + i);
                PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_prbs_error_analyzer_start(&phy_copy, &err_analyzer));
            }
            if (options->without_print != 1) {
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_peregrine5_pc_INTERNAL_seconds_to_displayformat(options->ber_proj_timeout_s, &hrs, &mins, &secs));
                PHYMOD_DIAG_OUT((" \n Waiting for PRBS Error Analyzer measurement: time approx %d seconds (%d hr:%d mins: %ds) \n",
                    options->ber_proj_timeout_s, hrs, mins, secs));
            }
            break;
        case PHYMOD_BER_PROJ_PHASE_F_COLLECT:
            /* Stop PRBS Error Analyzer on all lanes */
            for (i = 0; i < num_lane; i++) {
                phy_copy.access.lane_mask = 1 << (start_lane + i);
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_peregrine5_pc_prbs_error_analyzer_stop(&phy_copy, &err_analyzer));
            }
            /* Display PRBS Error Analyzer Err_Counts and projections */
            for (i = 0; i < num_lane; i++) {
                phy_copy.access.lane_mask = 1 << (start_lane + i);

                /* Clear out prbs err counts from previous lane */
                for (i = 0; i < PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS; i++) {
                   err_analyzer_lane_status.prbs_errcnt[i] = 0;
                }
                err_analyzer_lane_status.prbs_frames_all = 0;
#ifdef SERDES_API_FLOATING_POINT
                err_analyzer_lane_status.prbs_bit_errcnt = 0;
#endif
                PHYMOD_MEMSET(&err_analyzer, 0, sizeof(peregrine5_pc_prbs_err_analyzer_lane_config_st));
                PHYMOD_MEMSET(&err_analyzer_common_config, 0, sizeof(peregrine5_pc_prbs_err_analyzer_common_config_st));

                /* Dump PRBS Error Analyzer errors in Diag display format to be used later by decoder to decode*/
                err_analyzer_common_config.encrypt_mode = 0x1;
                err_analyzer_common_config.prbs_err_aggregate_mode = PEREGRINE5_PC_FEC_100GE;
                err_analyzer_common_config.lane_mask_aggregate = 0xff;

                err_analyzer.common_config_ptr = &err_analyzer_common_config;
                err_analyzer.timeout_s = options->ber_proj_timeout_s;
                if (options->ber_proj_fec_size == 5440) {
                    err_analyzer.fec_code_type = PEREGRINE5_PC_RS_544_514_10;
                } else {
                    err_analyzer.fec_code_type = PEREGRINE5_PC_RS_528_514_10;
                }
                err_analyzer.prbs_err_fec_size = options->ber_proj_fec_size;

                /* Display PRBS Error Analyzer Config */
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_peregrine5_pc_get_prbs_error_analyzer_err_count(&phy_copy, &err_analyzer, &err_analyzer_lane_status));
                PHYMOD_MEMCPY(options->ber_proj_prbs_errcnt[i].prbs_errcnt,
                    err_analyzer_lane_status.prbs_errcnt, sizeof(err_analyzer_lane_status.prbs_errcnt));
            }
            break;
        case PHYMOD_BER_PROJ_PHASE_F_CAL:
            for (i = 0; i < num_lane; i++) {
                phy_copy.access.lane_mask = 1 << (start_lane + i);
                PHYMOD_MEMSET(&err_analyzer, 0, sizeof(peregrine5_pc_prbs_err_analyzer_lane_config_st));
                PHYMOD_MEMSET(&err_analyzer_common_config, 0, sizeof(peregrine5_pc_prbs_err_analyzer_common_config_st));

                /* Dump PRBS Error Analyzer errors in Diag display format to be used later by decoder to decode*/
                err_analyzer_common_config.encrypt_mode = 0x1;
                err_analyzer_common_config.prbs_err_aggregate_mode = PEREGRINE5_PC_FEC_100GE;
                err_analyzer_common_config.lane_mask_aggregate = 0xff;

                err_analyzer.common_config_ptr = &err_analyzer_common_config;
                err_analyzer.timeout_s = options->ber_proj_timeout_s;
                if (options->ber_proj_fec_size == 5440) {
                    err_analyzer.fec_code_type = PEREGRINE5_PC_RS_544_514_10;
                } else {
                    err_analyzer.fec_code_type = PEREGRINE5_PC_RS_528_514_10;
                }
                err_analyzer.prbs_err_fec_size = options->ber_proj_fec_size;
                /*
                 * Retrieve counters again so that display_prbs_error_analyzer prints correct lane #.
                 * Actual err_count is overwritten with values from options->ber_proj_prbs_errcnt[i].
                 * Alternative: use err_analyzer->pp_field2 = plp_aperta2_peregrine5_pc_get_lane(&phy_copy.access).
                 */
                if (options->without_print != 1) {
                    PHYMOD_IF_ERR_RETURN(
                        plp_aperta2_peregrine5_pc_get_prbs_error_analyzer_err_count(&phy_copy, &err_analyzer, &err_analyzer_lane_status));
                }

                PHYMOD_MEMCPY(err_analyzer_lane_status.prbs_errcnt,
                    options->ber_proj_prbs_errcnt[i].prbs_errcnt, sizeof(err_analyzer_lane_status.prbs_errcnt));
                if (options->without_print != 1) {
                    PHYMOD_IF_ERR_RETURN(
                        plp_aperta2_peregrine5_pc_display_prbs_error_analyzer_struct(&phy_copy, &err_analyzer, &err_analyzer_lane_status));
                    PHYMOD_IF_ERR_RETURN(
                        plp_aperta2_peregrine5_pc_prbs_error_analyzer_compute_proj(&phy_copy, &err_analyzer, &err_analyzer_lane_status));
                }
#ifdef SERDES_API_FLOATING_POINT
                else {
                    PHYMOD_IF_ERR_RETURN(
                        plp_aperta2_peregrine5_pc_prbs_error_analyzer_report_proj(&phy_copy, &err_analyzer, &err_analyzer_lane_status, &err_analyzer_rept));
                    /* save ber x.y e-z  in uint32: bit 31-bit 24 : x value, bit23 - bit16: y value, bit15-bit 0: z value */
                    /* for example, if the BER is 4.3e-9, then x=4, y=3 and z=9 and The expected returned value will be 0x04030009*/
                    z = 0; x = 0, y = 0;
                    ber = err_analyzer_rept.proj_ber;
                    while(((int)ber) <=0 ) {
                        ber = ber * 10;
                        z++;
                    }
                    x = (uint16_t)ber;
                    /* keep 2 digits for y */
                    y = (ber - (USR_DOUBLE)x)*100 + 0.5;
                    /* printf("\n  BER = %0.2e\n\n",err_analyzer_rep.proj_ber); */
                    options->ber_proj_prbs_errcnt[i].prbs_proj_ber = x << 24 | y << 16 | z;
                }
#else
                options->ber_proj_prbs_errcnt[i].prbs_proj_ber = 0;
#endif /* SERDES_API_FLOATING_POINT */
            }
            break;
        default:
            return PHYMOD_E_PARAM;
    }

    return PHYMOD_E_NONE;
}

int peregrine5_pc_phy_ber_proj(const plp_aperta2_phymod_phy_access_t* phy,
                                phymod_ber_proj_mode_t mode,
                                const phymod_phy_ber_proj_options_t* options)
{
    switch (mode) {
        case phymodBERProjModePostFEC:
            PHYMOD_IF_ERR_RETURN(_peregrine5_pc_phy_post_fec_ber_proj(phy, options));
            break;
        default:
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("unsupported BER PROJECTION mode  %u"), mode));
    }

    return PHYMOD_E_NONE;
}

int peregrine5_pc_phy_fast_ber_proj_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t* ber_proj_data)
{
    struct peregrine5_pc_ber_data_st ber_data_local;
    int start_lane, num_lane;
    plp_aperta2_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /* next figure out the lane num and start_lane based on the input */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    phy_copy.access.lane_mask = 0x1 << start_lane;

    /* collect prbs in 100 ms */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_INTERNAL_get_BER_data(&phy_copy, 100, &ber_data_local, USE_SW_TIMERS));

    /* first check if PRBS enabled or prbs lost lock */
    if ((ber_data_local.prbs_chk_en == 0) || (ber_data_local.lcklost == 1)) {
        *ber_proj_data = 0xffffffff;
    } else {
        uint16_t x = 0,y = 0,z = 0, div_local;

        if (COMPILER_64_GE(ber_data_local.num_errs, ber_data_local.num_bits)) {
            x = 1;
            y = 0;
            z = 0;
        } else {
            uint64_t tmp_num_errs, tmp_num_bits, tmp_div;
            while (1) {
                /*
                 * div = (uint16_t)(((ber_data_local.num_errs<<1) + ber_data_local.num_bits)/(ber_data_local.num_bits<<1));
                 */
                COMPILER_64_COPY(tmp_num_errs, ber_data_local.num_errs);
                COMPILER_64_COPY(tmp_num_bits, ber_data_local.num_bits);

                /* first check if the  number of error is 0 or the error is too small */
                if (COMPILER_64_IS_ZERO(tmp_num_errs) || (z > 40))  {
                    /* the prbs error count is too small to have a meaningful estimate, return 0 */
                    *ber_proj_data = 0;
                    return PHYMOD_E_NONE;
                }
                /* ber_data_local.num_errs << 1 */
                COMPILER_64_SHL(tmp_num_errs, 1);

                /* (ber_data_local.num_errs << 1) + ber_data_local.num_bits */
                COMPILER_64_ADD_64(tmp_num_errs, ber_data_local.num_bits);

                /* ber_data_local.num_bits << 1 */
                COMPILER_64_SHL(tmp_num_bits, 1);

                COMPILER_64_COPY(tmp_div, tmp_num_errs);
                COMPILER_64_UDIV_64(tmp_div, tmp_num_bits);
                div_local = (uint16_t)(COMPILER_64_LO(tmp_div));

                if (div_local >= 10) break;
                /*
                 * ber_data_local.num_errs = ber_data_local.num_errs*10;
                 */
                COMPILER_64_UMUL_32(ber_data_local.num_errs, (uint32_t) 10);
                z = z + 1;
            }
            if(div_local >= 100) {
                div_local = div_local / 10;
                z = z - 1;
            }
            x = div_local / 10;
            y = div_local - 10 * x;
            z = z - 1;
        }
        *ber_proj_data = x << 24 | y << 16 | z;
    }

    return PHYMOD_E_NONE;
}

int peregrine5_pc_phy_pmd_lane_diag_debug_level_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t level)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, i;
    uint32_t event_group_mask;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    if (level > 4) {
        event_group_mask = EVENT_GROUP_PRIORITY_5;
    } else if (level == 4) {
        event_group_mask = EVENT_GROUP_PRIORITY_4;
    } else if (level == 3) {
        event_group_mask = EVENT_GROUP_PRIORITY_3;
    } else if (level == 2) {
        event_group_mask = EVENT_GROUP_PRIORITY_2;
    } else if (level == 1) {
        event_group_mask = EVENT_GROUP_PRIORITY_1;
    } else { /* == 0 */
        event_group_mask = EVENT_GROUP_PRIORITY_0;
    }

    for (i = 0; i < num_lane; i++) {
        phy_copy.access.lane_mask = 1 << (start_lane + i);
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_set_usr_event_log_group_mask(&phy_copy, event_group_mask));
    }

    return PHYMOD_E_NONE;
}

int peregrine5_pc_phy_pmd_lane_diag_debug_level_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t *level)
{
    uint32_t event_group_mask;
    plp_aperta2_phymod_phy_access_t phy_copy;
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_get_usr_event_log_group_mask(&phy_copy, &event_group_mask));
    if (event_group_mask == EVENT_GROUP_PRIORITY_5) {
        *level = 5;
    } else if (event_group_mask == EVENT_GROUP_PRIORITY_4) {
        *level = 4;
    } else if (event_group_mask == EVENT_GROUP_PRIORITY_3) {
        *level = 3;
    } else if (event_group_mask == EVENT_GROUP_PRIORITY_2) {
        *level = 2;
    } else if (event_group_mask == EVENT_GROUP_PRIORITY_1) {
        *level = 1;
    } else {
        *level = 0;
    }

    return PHYMOD_E_NONE;
}

int peregrine5_pc_phy_linkcat(const plp_aperta2_phymod_phy_access_t* phy,
                               phymod_linkcat_config_t config)
{
    return PHYMOD_E_NONE;
}

#endif

#endif /* PHYMOD_APERTA2_SUPPORT */
