/***********************************************************************************
 *                                                                                 *
 * Copyright: (c) 2021 Broadcom.                                                   *
 * Broadcom Proprietary and Confidential. All rights reserved.                     *
 *                                                                                 *
 *  Revision      :                                                                *
 *                                                                                 *
 *  Description   :  This is an auto generated diagnostic dump tool file           *
 *                                                                                 *
 **********************************************************************************/

#include "peregrine5_pc_collect_custom_fields.h"
#include "peregrine5_pc_functions.h"
#include "peregrine5_pc_select_defns.h"
#include "peregrine5_pc_access.h"
#include "peregrine5_pc_debug_functions.h"
#include "peregrine5_pc_config.h"
#include "peregrine5_pc_internal.h"
#include "peregrine5_pc_decode_print.h"

#ifndef SMALL_FOOTPRINT
err_code_t plp_aperta2_peregrine5_pc_collect_custom_fields_peregrine5_pc_lane_state_st_pre(srds_access_t *sa__, peregrine5_pc_lane_state_st_t *st) {
    INIT_SRDS_ERR_CODE

    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_pmd_lock_status(sa__,&st->pmd_lock, &st->pmd_lock_chg));
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_stop_state_lane_state(sa__, st)); /* Dependency on st->pmd_lock. Updates st->stop_state AND stop micro as well */
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_collect_custom_fields_peregrine5_pc_lane_state_st_post(srds_access_t *sa__, peregrine5_pc_lane_state_st_t *st) {
    INIT_SRDS_ERR_CODE
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_decode_br_os_mode(sa__, &st->CDR));
    st->laneid = plp_aperta2_peregrine5_pc_acc_get_lane(sa__);

    ESTM(st->tx_disable_status = rd_tx_disable_status());
    EFUN(plp_aperta2_peregrine5_pc_get_tx_asymm_nlc_pct(sa__, &st->txfir_nlc_upper_pct, &st->txfir_nlc_lower_pct));
    
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_rx_pam_mode_lane_state(sa__, st));

    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_osr_mode(sa__, &st->osr_mode));
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_osr_str_lane_state(sa__, st)); /* Dependency on st->osr_mode */
    
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_uc_cfg_lane_state(sa__, st));

    ESTM(st->UC_STS = rdv_status_byte());
    ESTM(st->UC_STS_EXT = rdv_ext_status_word());
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_sigdet_status(sa__,&st->sig_det, &st->sig_det_chg));
    
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_rx_ppm(sa__, &st->ppm));
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_tx_ppm(sa__, &st->tx_ppm));
    EFUN(plp_aperta2_peregrine5_pc_get_rx_tuning_status(sa__, &st->rx_tuning_done));

    ESTM(st->linktrn_en = rd_linktrn_ieee_training_enable());

    EFUN(plp_aperta2_peregrine5_pc_read_rx_afe(sa__, RX_AFE_DFE1, &(st->dfe_taps[0])));
    EFUN(plp_aperta2_peregrine5_pc_read_rx_afe(sa__, RX_AFE_DFE2, &(st->dfe_taps[1])));

    ESTM(st->TP1 = rdv_usr_chan_loss_est());

    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_txfir_lane_state(sa__, st));
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_disable_eye_lane_state(sa__, st));
    ESTM(st->EYE_U = rdv_usr_sts_veye_margin_2() * 2);
    ESTM(st->EYE_M = rdv_usr_sts_veye_margin_1() * 2);
    ESTM(st->EYE_L = rdv_usr_sts_veye_margin_0() * 2);

    EFUN(plp_aperta2_peregrine5_pc_read_rx_afe(sa__, RX_AFE_VGA, &st->VGA));
    ESTM(st->tx_pi_en = rd_tx_pi_en());
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_snr_dfe_lane_state(sa__, st)); /* Dependency on st->rx_pam_mode */

    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_link_time(sa__, &st->link_time));
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_pmd_lock_chg_lane_state(sa__, st));  /* Depedency on st->pmd_lock */

    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_resume_micro_lane_state(sa__, st)); /* Dependency on st->stop_state */

    {
        struct peregrine5_pc_ber_data_st ber_data = BER_DATA_ST_INIT;
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_BER_data_and_string(sa__, SRDS_BER_MEASURE_TIME_MS, &ber_data, st->BER, sizeof(st->BER)));
    }

    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_rx_ffe_lane_state(sa__, st)); /* RX FFE lane state RAM vars are updated after resuming micro */
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_uc_sts_decoded_lane_state(sa__, st)); /* Dependency on st->UC_STS and st->UC_STS_EXT */

    return ERR_CODE_NONE;
}
#endif

