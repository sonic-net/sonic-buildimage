/*
 * Common DSP RX information and debugging
 */
#include "project.h"

#include "dsp_series/common_dsp_functions.h"

#include "utility.h"
#include "sdk.h"

#include <math.h>
#include <string.h>

#define SDT_AMP_TH 7
#define SDT_CNT_TH 0
#define SDT_N_SEL  1

static const int FLT_LOC1[4][8] = {{9, 11, 13, 15, 17, 19, 21, 23},
                                   {10, 12, 14, 16, 18, 20, 22, 24},
                                   {11, 13, 15, 17, 19, 21, 23, 25},
                                   {12, 14, 16, 18, 20, 22, 24, 26}};
static const int FLT_LOC2[4][16] = {{13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43},
                                    {14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44},
                                    {15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45},
                                    {16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46}};

static unsigned therm_bin(unsigned tt) {
    tt += 1;
    int i = 0;
    for (i = 0; i <= 20; i++) {
        if (((1 << i) ^ tt) == 0) break;
    }
    return i;
}

static unsigned bin_therm(unsigned bb) {
    if (bb > 20) bb = 20;
    return pow(2, bb) - 1;
}

CredoError_t common_dsp_get_dtl_bb_en(CredoSlice_t* slice, int lane, bool* en) {
    unsigned val = 0;
    ERR_PROPS(readRegLane(slice, lane, REG_RX_DTL_BB_EN, &val));
    *en = (val) ? true : false;
    return CR_OK;
}

static CredoError_t common_dsp_set_rx_cdfl_threshold_a(CredoSlice_t* slice, int lane, int tha) {
    if (tha >= DSP_CDFL_TH_MAX) tha = DSP_CDFL_TH_MAX;
    if (tha <= DSP_CDFL_TH_MIN) tha = DSP_CDFL_TH_MIN;
    ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFLA_ACC_LD_VAL, (tha << (16 - DSP_SYS_CDFL_CW))));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFLA_ACC_LD, 1));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFLA_ACC_LD, 0));
    return CR_OK;
}

static CredoError_t common_dsp_get_rx_cdfl_threshold_a(CredoSlice_t* slice, int lane, int* tha) {
    return readRegSignedLane(slice, lane, REG_RX_CDFLA_THD_LD, tha);
}

static CredoError_t common_dsp_set_rx_cdfl_threshold_b(CredoSlice_t* slice, int lane, int thb) {
    if (thb >= DSP_CDFL_TH_MAX) thb = DSP_CDFL_TH_MAX;
    if (thb <= DSP_CDFL_TH_MIN) thb = DSP_CDFL_TH_MIN;
    ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFLB_ACC_LD_VAL, (thb << (16 - DSP_SYS_CDFL_CW))));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFLB_ACC_LD, 1));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFLB_ACC_LD, 0));
    return CR_OK;
}

static CredoError_t common_dsp_get_rx_cdfl_threshold_b(CredoSlice_t* slice, int lane, int* thb) {
    return readRegSignedLane(slice, lane, REG_RX_CDFLB_THD_LD, thb);
}

static CredoError_t common_dsp_set_rx_cdfl_mu(CredoSlice_t* slice, int lane, int mu[4]) {
    ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFLA_MU_P, mu[0]));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFLA_MU_N, mu[1]));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFLB_MU_P, mu[2]));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFLB_MU_N, mu[3]));
    return CR_OK;
}

// TODO, FIXME
CredoError_t common_dsp_get_rx_cdfl_eye(CredoSlice_t* slice, int lane, int eyes[]) {
    unsigned timeout = 0;
    int dh = 0, dl = 0, d1 = 0;
    int mu_init[4] = {0}, mu_post[4] = {12, 4, 4, 12};
    int initp = 0, initn = 0;
    int f1 = 0;

    ERR_PROP(common_dsp_get_rx_dfe_f1(slice, lane, &f1));
    for (int eye_idx = 13; eye_idx >= 0; eye_idx--) {
        timeout = (eye_idx > 11) ? 30 : 10;

        ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFL_ALL_EYE_PTN0, 0));
        if (eye_idx == 13) {
            ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFL_ALL_ISI_PTN0, 0));
            ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFL_EXCLUDE_ISI_PTN0, 0));
            ERR_PROP(writeRegLane(slice, lane, REG_RX_CDF_SELL0, 2));
            ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFL_ISI_PTN0, 0));
            ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFL_EYE_PTN_B0, 7));
            ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFL_EYE_PTN_A0, 3));
            dh = -1;
            dl = -3;
            d1 = 3;
        } else if (eye_idx == 12) {
            ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFL_ALL_ISI_PTN0, 0));
            ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFL_EXCLUDE_ISI_PTN0, 0));
            ERR_PROP(writeRegLane(slice, lane, REG_RX_CDF_SELL0, 2));
            ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFL_ISI_PTN0, 3));
            ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFL_EYE_PTN_B0, 12));
            ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFL_EYE_PTN_A0, 8));
            dh = 3;
            dl = 1;
            d1 = -3;
        } else {
            ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFL_ALL_ISI_PTN0, 1));
            ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFL_EYE_PTN_B0, eye_idx + 4));
            ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFL_EYE_PTN_A0, eye_idx));
            dh = (((eye_idx >> 2) + 1) << 1) - 3;
            dl = (((eye_idx >> 2) + 0) << 1) - 3;
            d1 = ((eye_idx & 0x3) << 1) - 3;
        }

        ERR_PROP(writeRegLane(slice, lane, REG_RX_CDFL_PHASE_SEL0, 0));
        ERR_PROP(common_dsp_set_rx_cdfl_mu(slice, lane, mu_init));
        initp = (dh << DSP_SYS_CDFL_CF) + ((d1 * (int)f1) << (DSP_SYS_CDFL_CF - DSP_SYS_F1_CF));
        initn = (dl << DSP_SYS_CDFL_CF) + ((d1 * (int)f1) << (DSP_SYS_CDFL_CF - DSP_SYS_F1_CF));
        ERR_PROP(common_dsp_set_rx_cdfl_threshold_b(slice, lane, initp));
        ERR_PROP(common_dsp_set_rx_cdfl_threshold_a(slice, lane, initn));
        ERR_PROP(common_dsp_set_rx_cdfl_mu(slice, lane, mu_post));

        sleep_ms(timeout);

        ERR_PROP(common_dsp_get_rx_cdfl_threshold_a(slice, lane, &initn));
        ERR_PROP(common_dsp_get_rx_cdfl_threshold_b(slice, lane, &initp));

        eyes[eye_idx] = initp - initn;
    }

    return CR_OK;
}

CredoError_t common_dsp_get_rx_eye(CredoSlice_t* slice, int lane, int eyes[3]) {
    eyes[0] = eyes[1] = eyes[2] = 0;

    int cdfl_eyes[14] = {0};
    ERR_PROPS(common_dsp_get_rx_cdfl_eye(slice, lane, cdfl_eyes));
    eyes[0] = (cdfl_eyes[8] + cdfl_eyes[9] + cdfl_eyes[10] + cdfl_eyes[11]) / 4;
    eyes[1] = (cdfl_eyes[4] + cdfl_eyes[5] + cdfl_eyes[6] + cdfl_eyes[7]) / 4;
    eyes[2] = (cdfl_eyes[0] + cdfl_eyes[1] + cdfl_eyes[2] + cdfl_eyes[3]) / 4;
    return CR_OK;
}

CredoError_t common_dsp_get_version_id(CredoSlice_t* slice, int lane, uint64_t* version) {
    unsigned ip_id = 0, revision = 0, subrevision = 0;

    ERR_PROPS(readRegLane(slice, lane, REG_RX_VER_ID_47_32, &ip_id));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_VER_ID_31_16, &revision));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_VER_ID_15_00, &subrevision));
    *version = 0;
    *version = ((uint64_t)ip_id << 32) | (revision << 16) | subrevision;
    return CR_OK;
}

CredoError_t common_dsp_get_rx_sdt(CredoSlice_t* slice, int lane, int amp, int cnt, int n, unsigned* sdt_p,
                                   unsigned* sdt_n, unsigned* cnt_p, unsigned* cnt_n) {
    // SDT_CNT_TH: 0, 1, 2, 3 = 0x8, 0x10, 0x20, 0x40
    // SDT_N_SEL: 0, 1, 2, 3 = 0x40, 0x100, 0x400, x01000
    ERR_PROP(writeRegLane(slice, lane, REG_RX_SDT_AMP_TH, amp));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_SDT_CNT_TH, cnt));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_SDT_N_SEL, n));
    sleep_ms(50);
    ERR_PROP(readRegLane(slice, lane, REG_RX_SDT_OUT_P, sdt_p));
    ERR_PROP(readRegLane(slice, lane, REG_RX_SDT_OUT_N, sdt_n));
    ERR_PROP(readRegLane(slice, lane, REG_RX_SDT_CNT_P, cnt_p));
    ERR_PROP(readRegLane(slice, lane, REG_RX_SDT_CNT_N, cnt_n));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_signal_detect(CredoSlice_t* slice, int lane, int* sd) {
    unsigned sdt_p, sdt_n;
    unsigned cnt_p, cnt_n;

    ERR_PROPS(common_dsp_get_rx_sdt(slice, lane, SDT_AMP_TH, SDT_CNT_TH, SDT_N_SEL, &sdt_p, &sdt_n, &cnt_p, &cnt_n));
    *sd = (sdt_p == sdt_n && sdt_p == 1) ? 1 : 0;

    LOGS_DEBUG(
        "lane: %d, SDT_AMP_TH: %d, SDT_CNT_TH: %d, SDT_N_SEL: %d, SDT_OUT_P: %d, "
        "SDT_OUT_N: %d, SDT_CNT_P: %d, SDT_CNT_N: %d",
        lane, SDT_AMP_TH, SDT_CNT_TH, SDT_N_SEL, sdt_p, sdt_n, cnt_p, cnt_n);

    return CR_OK;
}

CredoError_t common_dsp_get_rx_envelope(CredoSlice_t* slice, int lane, int mode, unsigned amp[]) {
    unsigned sdt_p, sdt_n, cnt_p, cnt_n;
    amp[0] = amp[1] = 0;

    // mode, 0=SDT 1=CDF
    if (mode == 0) {
        for (int A = 24; A > 0; A--) {
            ERR_PROPS(common_dsp_get_rx_sdt(slice, lane, A, 3, 3, &sdt_p, &sdt_n, &cnt_p, &cnt_n));
            amp[0] += sdt_p;
            amp[1] += sdt_n;
        }
    } else {
    }

    return CR_OK;
}

CredoError_t common_dsp_set_rx_ctle(CredoSlice_t* slice, int lane, unsigned ctle[]) {
    unsigned val = bin_therm(ctle[0]);
    ERR_PROP(writeRegLane(slice, lane, REG_RX_CTLE1_DEGEN_MSB, (val >> 14) & 0x3f));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_CTLE1_DEGEN_LSB, val & 0x3fff));

    val = bin_therm(ctle[1]);
    ERR_PROP(writeRegLane(slice, lane, REG_RX_CTLE2_DEGEN_MSB, (val >> 16) & 0xf));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_CTLE2_DEGEN_MID, (val >> 8) & 0xff));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_CTLE2_DEGEN_LSB, val & 0xff));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_ctle(CredoSlice_t* slice, int lane, unsigned ctle[]) {
    unsigned lsb = 0, mid = 0, msb = 0;
    ERR_PROP(readRegLane(slice, lane, REG_RX_CTLE1_DEGEN_MSB, &msb));
    ERR_PROP(readRegLane(slice, lane, REG_RX_CTLE1_DEGEN_LSB, &lsb));
    ctle[0] = therm_bin(msb << 14 | lsb);

    ERR_PROP(readRegLane(slice, lane, REG_RX_CTLE2_DEGEN_MSB, &msb));
    ERR_PROP(readRegLane(slice, lane, REG_RX_CTLE2_DEGEN_MID, &mid));
    ERR_PROP(readRegLane(slice, lane, REG_RX_CTLE2_DEGEN_LSB, &lsb));
    ctle[1] = therm_bin(msb << 16 | mid << 8 | lsb);
    return CR_OK;
}

CredoError_t common_dsp_get_rx_ctle_vs(CredoSlice_t* slice, int lane, unsigned* ctle_vs) {
    ERR_PROP(readRegLane(slice, lane, REG_RX_ADJ_VS, ctle_vs));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_ctle_ictrl(CredoSlice_t* slice, int lane, unsigned ctle_ictrl[]) {
    ERR_PROP(readRegLane(slice, lane, REG_RX_ICTRL_S1, ctle_ictrl + 0));
    ERR_PROP(readRegLane(slice, lane, REG_RX_ICTRL_S2, ctle_ictrl + 1));
    return CR_OK;
}

CredoError_t common_dsp_set_rx_agcgain(CredoSlice_t* slice, int lane, unsigned agcgain[]) {
    unsigned agcgain1 = 0, agcgain2 = 0, agcgain3 = 0, agcgain4 = 0;

    agcgain1 = bin_gray(agcgain[0]);
    agcgain2 = bin_gray(agcgain[1]);
    agcgain3 = bin_gray(agcgain[2]);
    agcgain4 = bin_gray(agcgain[3]);

    ERR_PROP(writeRegLane(slice, lane, REG_RX_AGCGAIN1, agcgain1));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_AGCGAIN2, agcgain2));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_AGCGAIN3, agcgain3));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_AGCGAIN4, agcgain4));

    return CR_OK;
}

CredoError_t common_dsp_get_rx_agcgain(CredoSlice_t* slice, int lane, unsigned agcgain[]) {
    unsigned agcgain1 = 0, agcgain2 = 0, agcgain3 = 0, agcgain4 = 0;

    ERR_PROP(readRegLane(slice, lane, REG_RX_AGCGAIN1, &agcgain1));
    ERR_PROP(readRegLane(slice, lane, REG_RX_AGCGAIN2, &agcgain2));
    ERR_PROP(readRegLane(slice, lane, REG_RX_AGCGAIN3, &agcgain3));
    ERR_PROP(readRegLane(slice, lane, REG_RX_AGCGAIN4, &agcgain4));

    agcgain[0] = gray_bin(agcgain1);
    agcgain[1] = gray_bin(agcgain2);
    agcgain[2] = gray_bin(agcgain3);
    agcgain[3] = gray_bin(agcgain4);
    return CR_OK;
}

CredoError_t common_dsp_set_rx_ind(CredoSlice_t* slice, int lane, unsigned ind[]) {
    ERR_PROP(writeRegLane(slice, lane, REG_RX_EN_IND1, ind[0]));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_EN_IND2, ind[1]));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_ind(CredoSlice_t* slice, int lane, unsigned ind[]) {
    ERR_PROP(readRegLane(slice, lane, REG_RX_EN_IND1, ind));
    ERR_PROP(readRegLane(slice, lane, REG_RX_EN_IND2, ind + 1));
    return CR_OK;
}

CredoError_t common_dsp_set_rx_skef_en(CredoSlice_t* slice, int lane, unsigned enable) {
    ERR_PROP(writeRegLane(slice, lane, REG_RX_EN_SKC, enable));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_skef_en(CredoSlice_t* slice, int lane, unsigned* enable) {
    ERR_PROP(readRegLane(slice, lane, REG_RX_EN_SKC, enable));
    return CR_OK;
}

CredoError_t common_dsp_set_rx_skef_degen(CredoSlice_t* slice, int lane, unsigned degen) {
    ERR_PROP(writeRegLane(slice, lane, REG_RX_SKC_DEGEN, degen));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_skef_degen(CredoSlice_t* slice, int lane, unsigned* degen) {
    ERR_PROP(readRegLane(slice, lane, REG_RX_SKC_DEGEN, degen));
    return CR_OK;
}

CredoError_t common_dsp_set_rx_skef_cap(CredoSlice_t* slice, int lane, unsigned cap) {
    ERR_PROP(writeRegLane(slice, lane, REG_RX_CAP_SKC, cap));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_skef_cap(CredoSlice_t* slice, int lane, unsigned* cap) {
    ERR_PROP(readRegLane(slice, lane, REG_RX_CAP_SKC, cap));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_adc_ref_ctrl(CredoSlice_t* slice, int lane, unsigned* ref_ctrl) {
    ERR_PROP(readRegLane(slice, lane, REG_RX_REF_CTL, ref_ctrl));
    return CR_OK;
}

/* It should not be used when firmware is running */
CredoError_t common_dsp_get_rx_dfe_f0(CredoSlice_t* slice, int lane, unsigned* f0) {
    ERR_PROP(common_dsp_set_lms_clk(slice, lane, false));
    ERR_PROP(readRegLane(slice, lane, REG_RX_DFE_F0_RD, f0));
    ERR_PROP(common_dsp_set_lms_clk(slice, lane, true));
    return CR_OK;
}

CredoError_t common_dsp_set_rx_dfe_f0(CredoSlice_t* slice, int lane, unsigned f0) {
    ERR_PROP(common_dsp_set_lms_clk(slice, lane, false));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_DFE_F0_OW, f0));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_DFE_F0_OWEN, 1));
    ERR_PROP(common_dsp_set_lms_clk(slice, lane, true));

    ERR_PROP(common_dsp_set_lms_clk(slice, lane, false));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_DFE_F0_OWEN, 0));
    ERR_PROP(common_dsp_set_lms_clk(slice, lane, true));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_dfe_f1(CredoSlice_t* slice, int lane, int* f1) {
    ERR_PROP(common_dsp_set_lms_clk(slice, lane, false));
    ERR_PROP(readRegSignedLane(slice, lane, REG_RX_DFE_F1_RD, f1));
    ERR_PROP(common_dsp_set_lms_clk(slice, lane, true));
    return CR_OK;
}

CredoError_t common_dsp_set_rx_dfe_f1(CredoSlice_t* slice, int lane, int f1) {
    ERR_PROP(common_dsp_set_lms_clk(slice, lane, false));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_DFE_F1_OW, f1));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_DFE_F1_OWEN, 1));
    ERR_PROP(common_dsp_set_lms_clk(slice, lane, true));

    ERR_PROP(common_dsp_set_lms_clk(slice, lane, false));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_DFE_F1_OWEN, 0));
    ERR_PROP(common_dsp_set_lms_clk(slice, lane, true));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_dfe(CredoSlice_t* slice, int lane, int dfe_taps[]) {
    ERR_PROP(common_dsp_get_rx_dfe_f0(slice, lane, (unsigned*)(dfe_taps + 0)));
    ERR_PROP(common_dsp_get_rx_dfe_f1(slice, lane, dfe_taps + 1));
    return CR_OK;
}

CredoError_t common_dsp_set_rx_dfe(CredoSlice_t* slice, int lane, int dfe_taps[]) {
    ERR_PROP(common_dsp_set_rx_dfe_f0(slice, lane, (unsigned)dfe_taps[0]));
    ERR_PROP(common_dsp_set_rx_dfe_f1(slice, lane, dfe_taps[1]));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_ppm(CredoSlice_t* slice, int lane, int* ppm) {
    unsigned half_rate_en = 0;
    ERR_PROP(readRegLane(slice, lane, REG_HALFRATE_EN, &half_rate_en));

    bool is_bb_mode = false;
    ERR_PROP(common_dsp_get_dtl_bb_en(slice, lane, &is_bb_mode));

    // double factor = (half_rate_en == 0) ? (1000000.0 / 8388608) : (1000000.0 / 16777216); // 2^23 and 2^24
    double factor = (half_rate_en == 0 || is_bb_mode) ? 0.12 : 0.06;  // sync with firmware

    ERR_PROP(readRegSignedLane(slice, lane, REG_RX_READ_FREQ_ACCU, ppm));
    *ppm = (*ppm) * factor;

    return CR_OK;
}

CredoError_t common_dsp_set_rx_ppm(CredoSlice_t* slice, int lane, int ppm) {
    ERR_PROP(writeRegLane(slice, lane, REG_RX_OW_FREQ_ACC, ppm));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_DTL_RELOAD, 1));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_DTL_RELOAD, 0));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_agc_attn(CredoSlice_t* slice, int lane, unsigned* attn) {
    ERR_PROP(readRegLane(slice, lane, REG_RX_ATTN_S1, attn));
    return CR_OK;
}

CredoError_t common_dsp_set_rx_agc_attn(CredoSlice_t* slice, int lane, unsigned attn) {
    ERR_PROP(writeRegLane(slice, lane, REG_RX_ATTN_S1, attn));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_ffe_all(CredoSlice_t* slice, int lane, int taps[]) {
    int val[DSP_RX_FFE_COUNT] = {0};
    for (int ph = 0; ph < PHASE_NUM; ph++) {
        ERR_PROPS(common_dsp_get_rx_ffe(slice, lane, ph, val));
        memcpy(taps + ph * DSP_RX_FFE_COUNT, val, sizeof(int) * DSP_RX_FFE_COUNT);
    }
    return CR_OK;
}

CredoError_t common_dsp_get_rx_ffe(CredoSlice_t* slice, int lane, int phase, int taps[]) {
    ERR_PROP(common_dsp_set_lms_clk(slice, lane, false));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_FFE_ADAPT_PHASE_OWEN, 1));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_FFE_ADAPT_PHASE_OW, phase));

    ERR_PROP(readRegSignedLane(slice, lane, REG_RX_FFE(PRE, 4), taps + 0));
    ERR_PROP(readRegSignedLane(slice, lane, REG_RX_FFE(PRE, 3), taps + 1));
    ERR_PROP(readRegSignedLane(slice, lane, REG_RX_FFE(PRE, 2), taps + 2));
    ERR_PROP(readRegSignedLane(slice, lane, REG_RX_FFE(PRE, 1), taps + 3));

    ERR_PROP(readRegSignedLane(slice, lane, REG_RX_FFE(PST, 1), taps + 4));
    ERR_PROP(readRegSignedLane(slice, lane, REG_RX_FFE(PST, 2), taps + 5));
    ERR_PROP(readRegSignedLane(slice, lane, REG_RX_FFE(PST, 3), taps + 6));
    ERR_PROP(readRegSignedLane(slice, lane, REG_RX_FFE(PST, 4), taps + 7));
    ERR_PROP(readRegSignedLane(slice, lane, REG_RX_FFE(PST, 5), taps + 8));
    ERR_PROP(readRegSignedLane(slice, lane, REG_RX_FFE(PST, 6), taps + 9));
    ERR_PROP(readRegSignedLane(slice, lane, REG_RX_FFE(PST, 7), taps + 10));
    ERR_PROP(readRegSignedLane(slice, lane, REG_RX_FFE(PST, 8), taps + 11));

    ERR_PROP(readRegSignedLane(slice, lane, REG_RX_FFE(FLT, 0), taps + 12));
    ERR_PROP(readRegSignedLane(slice, lane, REG_RX_FFE(FLT, 1), taps + 13));
    ERR_PROP(readRegSignedLane(slice, lane, REG_RX_FFE(FLT, 2), taps + 14));
    ERR_PROP(readRegSignedLane(slice, lane, REG_RX_FFE(FLT, 3), taps + 15));
    ERR_PROP(readRegSignedLane(slice, lane, REG_RX_FFE(FLT, 4), taps + 16));
    ERR_PROP(readRegSignedLane(slice, lane, REG_RX_FFE(FLT, 5), taps + 17));
    ERR_PROP(readRegSignedLane(slice, lane, REG_RX_FFE(FLT, 6), taps + 18));
    ERR_PROP(readRegSignedLane(slice, lane, REG_RX_FFE(FLT, 7), taps + 19));

    ERR_PROP(common_dsp_set_lms_clk(slice, lane, true));

    ERR_PROP(common_dsp_set_lms_clk(slice, lane, false));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_FFE_ADAPT_PHASE_OWEN, 0));
    ERR_PROP(common_dsp_set_lms_clk(slice, lane, true));

    return CR_OK;
}

CredoError_t common_dsp_set_rx_ffe(CredoSlice_t* slice, int lane, const int taps[]) {
    // TODO, incorrect
    for (int ph = 3; ph >= 0; ph--) {
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_ADAPT_PHASE_OWEN, (1 << ph)));

        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_PRE1_OW, taps[3]));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_PRE2_OW, taps[2]));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_PRE3_OW, taps[1]));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_PRE4_OW, taps[0]));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_PST1_OW, taps[4]));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_PST2_OW, taps[5]));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_PST3_OW, taps[6]));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_PST4_OW, taps[7]));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_PST5_OW, taps[8]));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_PST6_OW, taps[9]));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_PST7_OW, taps[10]));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_PST8_OW, taps[11]));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_FLT0_OW, taps[12]));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_FLT1_OW, taps[13]));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_FLT2_OW, taps[14]));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_FLT3_OW, taps[15]));

        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_PRE_OWEN, 0xF));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_PRE_OWEN, 0x0));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_PST_OWEN, 0xFF));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_PST_OWEN, 0x0));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_FLT_OWEN, 0xF));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_FLT_OWEN, 0x0));

        ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_ADAPT_PHASE_OWEN, 0));
    }

    return CR_OK;
}

CredoError_t common_dsp_get_rx_ffe_cm1(CredoSlice_t* slice, int lane, int* cm1) {
    ERR_PROP(common_dsp_set_lms_clk(slice, lane, false));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_FFE_ADAPT_PHASE_OWEN, 1));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_FFE_ADAPT_PHASE_OW, 0));
    ERR_PROP(readRegSignedLane(slice, lane, REG_RX_FFE(PRE, 1), cm1));
    ERR_PROP(common_dsp_set_lms_clk(slice, lane, true));

    ERR_PROP(common_dsp_set_lms_clk(slice, lane, false));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_FFE_ADAPT_PHASE_OWEN, 0));
    ERR_PROP(common_dsp_set_lms_clk(slice, lane, true));
    return CR_OK;
}

CredoError_t common_dsp_set_rx_ffe_cm1(CredoSlice_t* slice, int lane, int cm1) {
    ERR_PROP(common_dsp_set_lms_clk(slice, lane, false));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_FFE_PRE1_OW, cm1));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_FFE_ADAPT_PH_CMN, 1));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_FFE_PRE_OWEN, 1));
    ERR_PROP(common_dsp_set_lms_clk(slice, lane, true));

    ERR_PROP(common_dsp_set_lms_clk(slice, lane, false));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_FFE_PRE_OWEN, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_FFE_ADAPT_PH_CMN, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_FFE_ADAPT_PHASE_OWEN, 0));
    ERR_PROP(common_dsp_set_lms_clk(slice, lane, true));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_lane_ready(CredoSlice_t* slice, int lane, int* ready) {
    ERR_PROPS(readRegLane(slice, lane, REG_PHY_READY, (unsigned*)ready));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_dfe_nonlinear_mode(CredoSlice_t* slice, int lane, int* en) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_DFE_NL_MODE, (unsigned*)en));
    return CR_OK;
}

CredoError_t common_dsp_set_rx_dfe_nonlinear_mode(CredoSlice_t* slice, int lane, int en) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_DFE_NL_MODE, en));
    return CR_OK;
}

CredoError_t common_dsp_set_fec_clk(CredoSlice_t* slice, int lane, bool enable) {
    ERR_PROP(writeRegLane(slice, lane, REG_FEC_CLKB_EN, enable));
    return CR_OK;
}

CredoError_t common_dsp_get_fec_clk(CredoSlice_t* slice, int lane, bool* enable) {
    unsigned enable_u;
    ERR_PROP(readRegLane(slice, lane, REG_FEC_CLKB_EN, &enable_u));
    *enable = enable_u;
    return CR_OK;
}

CredoError_t common_dsp_set_lms_clk(CredoSlice_t* slice, int lane, bool enable) {
    ERR_PROP(writeRegLane(slice, lane, REG_LMS_CLKB_EN, enable));
    return CR_OK;
}

CredoError_t common_dsp_get_lms_clk(CredoSlice_t* slice, int lane, bool* enable) {
    unsigned enable_u;
    ERR_PROP(readRegLane(slice, lane, REG_LMS_CLKB_EN, &enable_u));
    *enable = enable_u;
    return CR_OK;
}

CredoError_t common_dsp_get_rx_kp(CredoSlice_t* slice, int lane, unsigned* kp) {
    bool bb_en = false;
    ERR_PROPS(common_dsp_get_dtl_bb_en(slice, lane, &bb_en));

    if (bb_en) {
        ERR_PROPS(readRegLane(slice, lane, REG_RX_LOW_SPEED_KP, kp));
    } else {
        ERR_PROPS(readRegLane(slice, lane, REG_RX_KP, kp));
    }
    return CR_OK;
}

CredoError_t common_dsp_set_rx_kp(CredoSlice_t* slice, int lane, unsigned kp) {
    bool bb_en = false;
    ERR_PROPS(common_dsp_get_dtl_bb_en(slice, lane, &bb_en));

    if (bb_en) {
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_LOW_SPEED_KP, kp));
    } else {
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_KP, kp));
    }
    return CR_OK;
}

CredoError_t common_dsp_get_rx_kf(CredoSlice_t* slice, int lane, unsigned* kf) {
    bool bb_en = false;
    ERR_PROPS(common_dsp_get_dtl_bb_en(slice, lane, &bb_en));

    if (bb_en) {
        ERR_PROPS(readRegLane(slice, lane, REG_RX_LOW_SPEED_KF, kf));
    } else {
        ERR_PROPS(readRegLane(slice, lane, REG_RX_KF, kf));
    }
    return CR_OK;
}

CredoError_t common_dsp_set_rx_kf(CredoSlice_t* slice, int lane, unsigned kf) {
    bool bb_en = false;
    ERR_PROPS(common_dsp_get_dtl_bb_en(slice, lane, &bb_en));

    if (bb_en) {
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_LOW_SPEED_KF, kf));
    } else {
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_KF, kf));
    }
    return CR_OK;
}

CredoError_t common_dsp_get_rx_th_ud_ph_enable(CredoSlice_t* slice, int lane, unsigned* th_ud_ph_en) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_TH_UD_PH_EN, th_ud_ph_en));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_flt_sel(CredoSlice_t* slice, int lane, unsigned flt_sel[]) {
    ERR_PROPS(common_dsp_set_lms_clk(slice, lane, false));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_FLT_SEL0, flt_sel + 0));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_FLT_SEL1, flt_sel + 1));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_FLT_SEL2, flt_sel + 2));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_FLT_SEL3, flt_sel + 3));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_FLT_SEL4, flt_sel + 4));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_FLT_SEL5, flt_sel + 5));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_FLT_SEL6, flt_sel + 6));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_FLT_SEL7, flt_sel + 7));
    ERR_PROPS(common_dsp_set_lms_clk(slice, lane, true));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_flt_location_by_sel(CredoSlice_t* slice, int lane, unsigned flt_sel[],
                                                   unsigned flt_loc[]) {
    for (int i = 0; i < 4; i++) {
        flt_loc[i] = FLT_LOC1[i][flt_sel[i]];
    }
    for (int i = 4; i < 8; i++) {
        flt_loc[i] = FLT_LOC2[i - 4][flt_sel[i]];
    }
    return CR_OK;
}

CredoError_t common_dsp_get_rx_flt_location(CredoSlice_t* slice, int lane, unsigned flt_loc[]) {
    unsigned flt_sel[DSP_FLT_COUNT] = {0};
    ERR_PROPS(common_dsp_get_rx_flt_sel(slice, lane, flt_sel));
    ERR_PROPS(common_dsp_get_rx_flt_location_by_sel(slice, lane, flt_sel, flt_loc));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_thdly(CredoSlice_t* slice, int lane, unsigned thdly[]) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_READ_THETA_TH_OUT0, thdly + 0));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_READ_THETA_TH_OUT1, thdly + 1));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_READ_THETA_TH_OUT2, thdly + 2));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_READ_THETA_TH_OUT3, thdly + 3));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_READ_THETA_TH_OUT4, thdly + 4));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_READ_THETA_TH_OUT5, thdly + 5));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_READ_THETA_TH_OUT6, thdly + 6));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_READ_THETA_TH_OUT7, thdly + 7));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_READ_THETA_TH_OUT8, thdly + 8));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_READ_THETA_TH_OUT9, thdly + 9));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_READ_THETA_TH_OUT10, thdly + 10));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_READ_THETA_TH_OUT11, thdly + 11));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_READ_THETA_TH_OUT12, thdly + 12));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_READ_THETA_TH_OUT13, thdly + 13));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_READ_THETA_TH_OUT14, thdly + 14));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_READ_THETA_TH_OUT15, thdly + 15));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_th_ktheta(CredoSlice_t* slice, int lane, unsigned* ktheta) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_TH_KTHETA, ktheta));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_ktheta(CredoSlice_t* slice, int lane, unsigned kt[]) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_KTHETA1, kt + 0));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_KTHETA2, kt + 1));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_KTHETA3, kt + 2));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_ktheta_flip(CredoSlice_t* slice, int lane, unsigned kflip[]) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_THETA1_UPDATE_FLIP, kflip + 0));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_THETA2_UPDATE_FLIP, kflip + 1));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_THETA3_UPDATE_FLIP, kflip + 2));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_clk_comp_flip(CredoSlice_t* slice, int lane, unsigned* clk_comp_flip) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_CLK_COMP_FLIP, clk_comp_flip));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_thdly_acc_in_sel(CredoSlice_t* slice, int lane, unsigned* thdly_acc_in_sel) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_THDLY_ACC_IN_SEL, thdly_acc_in_sel));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_phase_fast_rotate(CredoSlice_t* slice, int lane, unsigned* ph_fast_rotate) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_PHASE_FAST_ROTATE, ph_fast_rotate));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_ted_slope_decision(CredoSlice_t* slice, int lane, unsigned* ted_slope_decision) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_TED_SLOPE_DECISION, ted_slope_decision));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_delayloop_freeze(CredoSlice_t* slice, int lane, unsigned* delayloop_freeze) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_DELAYLOOP_FREEZE, delayloop_freeze));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_dtl_phase0(CredoSlice_t* slice, int lane, unsigned* dtl_phase0) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_READ_PHASE0, dtl_phase0));
    return CR_OK;
}

CredoError_t common_dsp_set_rx_dtl_phase0(CredoSlice_t* slice, int lane, unsigned dtl_phase0) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_OW_PHASE0_ACC, (unsigned)(dtl_phase0 / 2)));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_OWEN_PHASE0_ACC, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_OWEN_PHASE0_ACC, 0));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_dtl_theta(CredoSlice_t* slice, int lane, int dtl_theta[]) {
    ERR_PROPS(readRegSignedLane(slice, lane, REG_RX_READ_THETA1_ACC, dtl_theta + 0));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_RX_READ_THETA2_ACC, dtl_theta + 1));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_RX_READ_THETA3_ACC, dtl_theta + 2));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_vga(CredoSlice_t* slice, int lane, unsigned* vga) {
    ERR_PROPS(common_dsp_set_lms_clk(slice, lane, false));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_VGA_COE_RD, vga));
    ERR_PROPS(common_dsp_set_lms_clk(slice, lane, true));
    return CR_OK;
}

CredoError_t common_dsp_set_rx_vga(CredoSlice_t* slice, int lane, unsigned vga) {
    ERR_PROPS(common_dsp_set_lms_clk(slice, lane, false));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_VGA_COE_OW, vga));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_VGA_COE_OWEN, 1));
    ERR_PROPS(common_dsp_set_lms_clk(slice, lane, true));

    ERR_PROPS(common_dsp_set_lms_clk(slice, lane, false));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_VGA_COE_OWEN, 0));
    ERR_PROPS(common_dsp_set_lms_clk(slice, lane, true));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_vga_mode(CredoSlice_t* slice, int lane, unsigned vga_mode[]) {
    ERR_PROPS(common_dsp_set_lms_clk(slice, lane, false));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_VGA0_MODE, vga_mode + 0));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_VGA1_MODE, vga_mode + 1));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_VGA2_MODE, vga_mode + 2));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_VGA3_MODE, vga_mode + 3));
    ERR_PROPS(common_dsp_set_lms_clk(slice, lane, true));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_dc_cmn(CredoSlice_t* slice, int lane, int* dc_cmn) {
    ERR_PROPS(readRegSignedLane(slice, lane, REG_RX_DC_COE_CMN_RD, dc_cmn));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_dc_sar(CredoSlice_t* slice, int lane, int* dc_sar) {
    for (int i = 0; i < DSP_SAR_COUNT; i++) {
        ERR_PROPS(common_dsp_set_lms_clk(slice, lane, false));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_DC_COE_SAR_SEL, i));
        ERR_PROPS(readRegSignedLane(slice, lane, REG_RX_DC_COE_SAR_RD, dc_sar + i));
        ERR_PROPS(common_dsp_set_lms_clk(slice, lane, true));
    }
    return CR_OK;
}

CredoError_t common_dsp_get_rx_gain_sar(CredoSlice_t* slice, int lane, unsigned* gain_sar) {
    unsigned en = 0;
    ERR_PROPS(common_dsp_set_lms_clk(slice, lane, false));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_GAIN_ADAPT_EN, &en));
    ERR_PROPS(common_dsp_set_lms_clk(slice, lane, true));

    for (int i = 0; i < DSP_SAR_COUNT; i++) {
        ERR_PROPS(common_dsp_set_lms_clk(slice, lane, false));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_GAIN_ADAPT_EN, 0));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_GAIN_ADAPT_PATH_FB_OW, i));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_GAIN_ADAPT_PATH_IN_OW, i));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_GAIN_ADAPT_PATH_OWEN, 1));
        ERR_PROPS(readRegLane(slice, lane, REG_RX_GAIN_COE_RD, gain_sar + i));
        ERR_PROPS(common_dsp_set_lms_clk(slice, lane, true));

        ERR_PROPS(common_dsp_set_lms_clk(slice, lane, false));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_GAIN_ADAPT_PATH_OWEN, 0));
        ERR_PROPS(common_dsp_set_lms_clk(slice, lane, true));
    }

    ERR_PROPS(common_dsp_set_lms_clk(slice, lane, false));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_GAIN_ADAPT_EN, en));
    ERR_PROPS(common_dsp_set_lms_clk(slice, lane, true));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_halfrate_en(CredoSlice_t* slice, int lane, bool* en) {
    unsigned val = 0;
    ERR_PROPS(readRegLane(slice, lane, REG_HALFRATE_EN, &val));
    *en = (val) ? true : false;
    return CR_OK;
}

CredoError_t common_dsp_get_rx_ictrl(CredoSlice_t* slice, int lane, unsigned ictrl[2]) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_ICTRL_S1, &ictrl[0]));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_ICTRL_S2, &ictrl[1]));
    return CR_OK;
}
