#include "project.h"

#include "common/common_firmware.h"

#include "sdk.h"

#include <math.h>

#ifdef FW_SPEED_INFO
CredoError_t common_fw_get_speed_index(CredoSlice_t* slice, int lane, unsigned* speed_index) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, FW_SPEED_INFO, speed_index));
    return CR_OK;
}
#endif

#if HAL_SUPPORT_FW_ADAPT_COUNT
CREDOAPI CredoError_t common_fw_get_adapt_count(CredoSlice_t* slice, int lane, unsigned* count) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(hal_fw_debug_cmd(slice, lane, FW_ADAPT_COUNT_NRZ, count));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(hal_fw_debug_cmd(slice, lane, FW_ADAPT_COUNT_PAM4, count));
            break;
        default:
            LOGS_WARN("[Get FW Adapt Count] Unknown mode %d, return 0", mode);
            *count = 0;
            break;
    }

    return CR_OK;
}
#endif

#ifdef FW_READAPT_COUNT
CREDOAPI CredoError_t common_fw_get_readapt_count(CredoSlice_t* slice, int lane, unsigned* count) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, FW_READAPT_COUNT, count));
    return CR_OK;
}
#endif

#ifdef FW_LINK_LOST_COUNT
CREDOAPI CredoError_t common_fw_get_link_lost_count(CredoSlice_t* slice, int lane, unsigned* count) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, FW_LINK_LOST_COUNT, count));
    return CR_OK;
}
#endif

#ifdef FW_LOS_COUNT
CREDOAPI CredoError_t common_fw_get_los_count(CredoSlice_t* slice, int lane, unsigned* count) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, FW_LOS_COUNT, count));
    return CR_OK;
}
#endif

#if HAL_SUPPORT_FW_RATIO
CredoError_t common_fw_get_channel_estimate(CredoSlice_t* slice, int lane, double* chan_est) {
    CredoLaneMode_t mode;
    unsigned val = 0;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(hal_fw_debug_cmd(slice, lane, FW_RX_RATIO_NRZ, &val));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(hal_fw_debug_cmd(slice, lane, FW_RX_RATIO_PAM4, &val));
            break;
        default:
            LOGS_WARN("[Get Channel Estimate] Unknown mode %d, returning 0", mode);
            *chan_est = 0;
            break;
    }

    *chan_est = val / 256.0f;
    return CR_OK;
}
#endif

#if HAL_SUPPORT_FW_OF
CredoError_t common_fw_get_of(CredoSlice_t* slice, int lane, unsigned* of) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(hal_fw_debug_cmd(slice, lane, FW_RX_OF_NRZ, of));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(hal_fw_debug_cmd(slice, lane, FW_RX_OF_PAM4, of));
            break;
        default:
            LOGS_WARN("[Get OF] Unknown mode %d, returning 0", mode);
            *of = 0;
            break;
    }

    return CR_OK;
}
#endif

#if HAL_SUPPORT_FW_HF
CredoError_t common_fw_get_hf(CredoSlice_t* slice, int lane, unsigned* hf) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(hal_fw_debug_cmd(slice, lane, FW_RX_HF_NRZ, hf));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(hal_fw_debug_cmd(slice, lane, FW_RX_HF_PAM4, hf));
            break;
        default:
            LOGS_WARN("[Get HF] Unknown mode %d, returning 0", mode);
            *hf = 0;
            break;
    }

    return CR_OK;
}
#endif

#if HAL_SUPPORT_FW_DFE
CredoError_t common_fw_get_ths(CredoSlice_t* slice, int lane, int ths[]) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, FW_DFE_PAM4, NULL));
    ERR_PROPS(common_fw_info_data(slice, 12, ths));
    return CR_OK;
}

CredoError_t common_fw_get_dfe(CredoSlice_t* slice, int lane, double dfe_taps[]) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    if (mode == CR_LMODE_NRZ) {
        return hal_get_dfe(slice, lane, dfe_taps);
    }

    if (mode != CR_LMODE_PAM4) {
        LOGS_WARN("[Get FW DFE] Unknown mode %d, returning 0", mode);
        return CR_OK;
    }

    // PAM4 mode
    int ths_list[12] = {0};
    ERR_PROPS(common_fw_get_ths(slice, lane, ths_list));
    dfe_taps[0] = (-3.0f / 16) * ((ths_list[0] - ths_list[2]) + (ths_list[3] - ths_list[5]) +
                                  (ths_list[6] - ths_list[8]) + (ths_list[9] - ths_list[11]));
    dfe_taps[1] = (-3.0f / 20) *
                  ((ths_list[0] + ths_list[1] + ths_list[2] - ths_list[9] - ths_list[10] - ths_list[11]) +
                   (1.0f / 3) * (ths_list[3] + ths_list[4] + ths_list[5] - ths_list[6] - ths_list[7] - ths_list[8]));
    dfe_taps[0] /= 2048.0;
    dfe_taps[1] /= 2048.0;
    dfe_taps[2] = (dfe_taps[0] == 0) ? 0 : dfe_taps[1] / dfe_taps[0];

    return CR_OK;
}
#endif

#if HAL_SUPPORT_FW_EYE
CredoError_t common_fw_get_eye(CredoSlice_t* slice, int lane, int eyes[3]) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
#ifdef FW_EYE_NRZ
        {
            int dac_val;
            ERR_PROPS(hal_get_rx_dac(slice, lane, &dac_val));
            ERR_PROPS(hal_fw_debug_cmd(slice, lane, FW_EYE_NRZ, NULL));
            ERR_PROPS(common_fw_info_data(slice, 1, eyes));

            eyes[0] = (eyes[0] / 2048.0) * (200 + 50.0 * dac_val);
        }
#else
            return CR_UNSUPPORTED;
#endif
        break;
        case CR_LMODE_PAM4:
#ifdef FW_EYE_PAM4
            ERR_PROPS(hal_fw_debug_cmd(slice, lane, FW_EYE_PAM4, NULL));
            ERR_PROPS(common_fw_info_data(slice, 3, eyes));
            break;
#else
            return CR_UNSUPPORTED;
#endif
        default:
            LOGS_WARN("[Get FW EYE] Unknown mode %d, returning 0", mode);
            eyes[0] = 0;
            break;
    }

    return CR_OK;
}
#endif

#if HAL_SUPPORT_FW_ISI
CredoError_t common_fw_get_isi(CredoSlice_t* slice, int lane, int isi[]) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
#ifdef FW_ISI_NRZ
            ERR_PROPS(hal_fw_debug_cmd(slice, lane, FW_ISI_NRZ, NULL));
            ERR_PROPS(common_fw_info_data(slice, 16, isi));
            break;
#else
            return CR_UNSUPPORTED;
#endif
        case CR_LMODE_PAM4:
#ifdef FW_ISI_PAM4
            ERR_PROPS(hal_fw_debug_cmd(slice, lane, FW_ISI_PAM4, NULL));
            ERR_PROPS(common_fw_info_data(slice, 16, isi));
            break;
#else
            return CR_UNSUPPORTED;
#endif
        default:
            LOGS_WARN("[Get FW ISI] Unknown mode %d", mode);
            break;
    }

    return CR_OK;
}
#endif

CredoError_t common_fw_get_rx_ffe(CredoSlice_t* slice, int lane, int ffe[]) {
    CredoLaneMode_t mode;
    int taps_len, sum_len;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    ERR_PROPS(hal_get_rx_ffe_range(slice, lane, &taps_len, &sum_len));

    switch (mode) {
        case CR_LMODE_NRZ:
            LOGS_WARN("[Get FW FFE] NRZ mode not implement");
            break;
        case CR_LMODE_PAM4:
#if HAL_SUPPORT_FW_FFE
            ERR_PROPS(hal_fw_debug_cmd(slice, lane, PAM4_INFO, PAM4_INFO_FFE, NULL));
            ERR_PROPS(common_fw_info_data(slice, taps_len + sum_len, ffe));
#else
            LOGS_WARN("[Get FW FFE] PAM4 mode not implement");
#endif
            break;
        default:
            LOGS_WARN("[Get FW FFE] Unknown mode %d", mode);
            break;
    }

    return CR_OK;
}

CredoError_t common_fw_get_rx_ffe_nbias(CredoSlice_t* slice, int lane, int nbias[]) {
    CredoLaneMode_t mode;
    int taps_len, sum_len;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    ERR_PROPS(hal_get_rx_ffe_range(slice, lane, &taps_len, &sum_len));

    switch (mode) {
        case CR_LMODE_NRZ:
            LOGS_WARN("[Get FW FFE NBIAS] NRZ mode not implement");
            break;
        case CR_LMODE_PAM4:
#if HAL_SUPPORT_FW_FFE_NBIAS
            ERR_PROPS(hal_fw_debug_cmd(slice, lane, FW_FFE_NBIAS, NULL));
            ERR_PROPS(common_fw_info_data(slice, taps_len + sum_len, nbias));
#else
            LOGS_WARN("[Get FW FFE NBIAS] PAM4 mode not implement");
#endif
            break;
        default:
            LOGS_WARN("[Get FW FFE NBIAS] Unknown mode %d", mode);
            break;
    }

    return CR_OK;
}

CredoError_t common_fw_get_rx_ffe_kaccu(CredoSlice_t* slice, int lane, double kaccu[]) {
    CredoLaneMode_t mode;
    int taps_len, sum_len;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    ERR_PROPS(hal_get_rx_ffe_range(slice, lane, &taps_len, &sum_len));

    switch (mode) {
        case CR_LMODE_NRZ:
            LOGS_WARN("[Get FW FFE KACCU] NRZ mode not implement");
            break;
        case CR_LMODE_PAM4:
#if HAL_SUPPORT_FW_FFE_KACCU
        {
            int data[32] = {0};
            ERR_PROPS(hal_fw_debug_cmd(slice, lane, FW_FFE_KACCU, NULL));
            ERR_PROPS(common_fw_info_data(slice, taps_len, data));
            for (int i = 0; i < taps_len; i++) {
                kaccu[i] = data[i] / pow(2, FW_FFE_ACCU_SCALE);
            }
        }
#else
            LOGS_WARN("[Get FW FFE KACCU] PAM4 mode not implement");
#endif
        break;
        default:
            LOGS_WARN("[Get FW FFE KACCU] Unknown mode %d", mode);
            break;
    }

    return CR_OK;
}

CredoError_t common_fw_get_rx_ffe_flip_counter(CredoSlice_t* slice, int lane, int flip_counter[]) {
    CredoLaneMode_t mode;
    int taps_len, sum_len;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    ERR_PROPS(hal_get_rx_ffe_range(slice, lane, &taps_len, &sum_len));

    switch (mode) {
        case CR_LMODE_NRZ:
            LOGS_WARN("[Get FW FFE JUMP] NRZ mode not implement");
            break;
        case CR_LMODE_PAM4:
#if HAL_SUPPORT_FW_FFE_JUMP
            ERR_PROPS(hal_fw_debug_cmd(slice, lane, FW_FFE_JUMP, NULL));
            ERR_PROPS(common_fw_info_data(slice, taps_len, flip_counter));
#else
            LOGS_WARN("[Get FW FFE JUMP] PAM4 mode not implement");
#endif
            break;
        default:
            LOGS_WARN("[Get FW FFE JUMP] Unknown mode %d", mode);
            break;
    }

    return CR_OK;
}
