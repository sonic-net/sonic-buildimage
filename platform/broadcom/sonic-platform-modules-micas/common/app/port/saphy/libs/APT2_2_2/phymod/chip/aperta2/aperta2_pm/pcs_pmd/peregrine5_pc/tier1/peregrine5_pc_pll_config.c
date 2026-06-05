/***********************************************************************************
 *                                                                                 *
 * Copyright: (c) 2021 Broadcom.                                                   *
 * Broadcom Proprietary and Confidential. All rights reserved.                     *
 *                                                                                 *
 ***********************************************************************************/

/**********************************************************************************
 **********************************************************************************
 *  File Name     :  peregrine5_pll_config.c                                      *
 *  Created On    :  5 Oct 2020                                                   *
 *  Created By    :  Shayan Daryoush                                              *
 *  Description   :  Peregrine5 PLL Configuration APIs                            *
 *                                                                                *
 **********************************************************************************
 **********************************************************************************/


/** @file peregrine5_pll_config.c
 * Peregrine5 PLL Configuration APIs
 */


#include "peregrine5_pc_config.h"
#include "peregrine5_pc_functions.h"
#include "peregrine5_pc_internal.h"
#include "peregrine5_pc_internal_error.h"
#include "peregrine5_pc_select_defns.h"

#define _ndiv_frac_l(x) ((x)&0x7FFF)
#define _ndiv_frac_h(x) ((x)>>15)

#define _ndiv_frac_decode(l_, h_) (((l_) & 0x7FFF) | (((h_) & 0x7) << 15))

#define VCO_CUTOFF_FREQ_KHZ    (50000000)

static err_code_t _plp_aperta2_peregrine5_pc_restore_pll_defaults(srds_access_t *sa__);
    static err_code_t _plp_aperta2_peregrine5_pc_vco_enable(srds_access_t *sa__, uint8_t vco_band);

/* The pll_fracn_ndiv_int and pll_fracn_frac bitfields have this many bits. */
static const uint32_t plp_aperta2_pll_fracn_ndiv_int_bits = 10;
static const uint32_t plp_aperta2_pll_fracn_frac_bits     = 18;

static err_code_t _plp_aperta2_peregrine5_pc_restore_pll_defaults(srds_access_t *sa__) {
    INIT_SRDS_ERR_CODE
    EFUN(wrc_ams_pll_fracn_ndiv_int(0x3B));
    EFUN(wrc_ams_pll_fracn_ndiv_frac_l(0x0));
    EFUN(wrc_ams_pll_fracn_ndiv_frac_h(0x0));
    EFUN(wrc_ams_pll_refdiv2(0x0));
    EFUN(wrc_ams_pll_refdiv4(0x0));
    EFUN(wrc_ams_pll_div4_2_sel(0x0));
    EFUN(wrc_ams_pll_pll_frac_mode(0x1));
    EFUN(wrc_ams_pll_resetb_mmd(0x1));
    EFUN(wrc_ams_pll_en_lb_res(0x0));
    EFUN(wrc_ams_pll_en_lb_ind(0x0));
    EFUN(wrc_ams_pll_lb_sel_pll(0x0));
    EFUN(wrc_ams_pll_vcobuf_bandcntrl(0x4));
    EFUN(wrc_ams_pll_rz_sel(0x1));
    EFUN(wrc_ams_pll_cap_l_pll(0x0));
    EFUN(wrc_ams_pll_cap_r_pll(0x0));
    EFUN(wrc_ams_pll_ictrl_drv_l_pll(0x4));
    EFUN(wrc_ams_pll_ictrl_drv_r_pll(0x4));
    EFUN(wrc_ams_pll_ictrl_buf_pll(0x5));
    EFUN(wrc_ams_pll_cm_ctrl_buf_pll(0x5));
    EFUN(wrc_ams_pll_div2_itail_ctrl(0x3));
    EFUN(wrc_ams_pll_icp_sel(0xf));
    EFUN(wrc_ams_pll_bmax(0x0));
    EFUN(wrc_ams_pll_imode(0x0));
    EFUN(wrc_ams_pll_imax(0x0));
    EFUN(wrc_ams_pll_rz_band(0x0));
    EFUN(wrc_ams_pll_cp_sel(0x1));
    EFUN(wrc_ams_pll_cp_sel_extra(0x0));
    EFUN(wrc_ams_pll_fp3_c_sel(0x0));
    EFUN(wrc_ams_pll_fracn_enable(0x0));
    return (ERR_CODE_NONE);
}

static err_code_t _plp_aperta2_peregrine5_pc_vco_enable(srds_access_t *sa__, uint8_t vco_band)
{
    INIT_SRDS_ERR_CODE
    uint8_t en_lb_res, en_lb_ind, ictrl_buf_pll, cm_ctrl_buf_pll, div2_itail_ctrl, lb_sel_pll, pwrdn_hbvco, pwrdn_lbvco;

    if(vco_band == SRDS_HBVCO) {
        pwrdn_hbvco = 0;
        pwrdn_lbvco = 1;
        en_lb_res = 0;
        en_lb_ind = 0;
        ictrl_buf_pll = 5;
        cm_ctrl_buf_pll = 5;
        div2_itail_ctrl = 3;
        lb_sel_pll = 0;
    }
    else {
        pwrdn_hbvco = 1;
        pwrdn_lbvco = 0;
        en_lb_res = 1;
        en_lb_ind = 1;
        ictrl_buf_pll = 2;
        cm_ctrl_buf_pll = 2;
        div2_itail_ctrl = 1;
        lb_sel_pll = 1;
    }

    EFUN(wrc_ams_pll_pwrdn_hbvco(pwrdn_hbvco));
    EFUN(wrc_ams_pll_pwrdn_lbvco(pwrdn_lbvco));
    EFUN(wrc_ams_pll_en_lb_res(en_lb_res));
    EFUN(wrc_ams_pll_en_lb_ind(en_lb_ind));
    EFUN(wrc_ams_pll_ictrl_buf_pll(ictrl_buf_pll));
    EFUN(wrc_ams_pll_cm_ctrl_buf_pll(cm_ctrl_buf_pll));
    EFUN(wrc_ams_pll_div2_itail_ctrl(div2_itail_ctrl));
    EFUN(wrc_ams_pll_lb_sel_pll(lb_sel_pll));

    return ERR_CODE_NONE;
}
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_configure_pll(srds_access_t *sa__,
                                         enum peregrine5_pc_pll_refclk_enum refclk,
                                         enum peregrine5_pc_pll_div_enum srds_div,
                                         uint32_t vco_freq_khz,
                                         enum peregrine5_pc_pll_option_enum pll_option) {
    INIT_SRDS_ERR_CODE
    uint32_t refclk_freq_hz=0;
    uint32_t pll_vco_cutoff_in_khz=VCO_CUTOFF_FREQ_KHZ;

    if (pll_option & PEREGRINE5_PC_PLL_OPTION_POWERDOWN) {
        UNUSED(refclk_freq_hz);
        EFUN(wrc_ams_pll_pwrdn(0x1));
        return (ERR_CODE_NONE);
    }

#ifdef SMALL_FOOTPRINT
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_refclk_in_hz(sa__, refclk, &refclk_freq_hz));
#else
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_resolve_pll_parameters(sa__, refclk, &refclk_freq_hz, &srds_div, &vco_freq_khz, pll_option));
#endif

    /* Use this to restore defaults if reprogramming the PLL under dp-reset (typically Auto-Neg FW) */
    EFUN(_plp_aperta2_peregrine5_pc_restore_pll_defaults(sa__));

    {
        uint8_t reset_state;
        /* Use core_s_rstb to re-initialize all registers to default before calling this function. */
        ESTM(reset_state = rdc_core_dp_reset_state());

        if(reset_state < 7) {
            EFUN_PRINTF(("ERROR: peregrine5_pc_configure_pll(..) called without core_dp_s_rstb=0\n"));
            return (peregrine5_pc_error(sa__, ERR_CODE_CORE_DP_NOT_RESET));
        }
    }

    pll_option = (enum peregrine5_pc_pll_option_enum)(pll_option & PEREGRINE5_PC_PLL_OPTION_REFCLK_MASK);

    /* Handle refclk PLL options */
    if (pll_option == PEREGRINE5_PC_PLL_OPTION_REFCLK_DOUBLER_EN) {
        EFUN_PRINTF(("ERROR: REFCLK_DOUBLER_EN option not supported."));
        return (peregrine5_pc_error(sa__, ERR_CODE_INVALID_PLL_CFG));
    } else if (pll_option == PEREGRINE5_PC_PLL_OPTION_REFCLK_DIV2_EN) {
        EFUN(wrc_ams_pll_refdiv2(1));
        EFUN(wrc_ams_pll_div4_2_sel(1));
        refclk_freq_hz >>= 1;
    } else if (pll_option == PEREGRINE5_PC_PLL_OPTION_REFCLK_DIV4_EN) {
        EFUN(wrc_ams_pll_refdiv4(1));
        EFUN(wrc_ams_pll_div4_2_sel(1));
        refclk_freq_hz >>= 2;
    }
    UNUSED(refclk_freq_hz);

    /* Clear PLL powerdown */
    EFUN(wrc_ams_pll_pwrdn(0x0));

    /* Use HBVCO for 50.0 GHz and above */
    if (vco_freq_khz >= pll_vco_cutoff_in_khz) {
        EFUN(_plp_aperta2_peregrine5_pc_vco_enable(sa__, SRDS_HBVCO)); /* Enable HBVCO */
    }
    else {
        EFUN(_plp_aperta2_peregrine5_pc_vco_enable(sa__, SRDS_LBVCO)); /* Enable LBVCO */
    }


    {
        uint8_t cap_pll = 0;
        if (vco_freq_khz > 55333330) {
            cap_pll = 0;
        }
        else if (vco_freq_khz > 54666660) {
            cap_pll = 1;
        }
        else if (vco_freq_khz > 53999990) {
            cap_pll = 2;
        }
        else if (vco_freq_khz > 53333320) {
            cap_pll = 3;
        }
        else if (vco_freq_khz > 52666650) {
            cap_pll = 4;
        }
        else if (vco_freq_khz > 51999980) {
            cap_pll = 5;
        }
        else if (vco_freq_khz > 51333310) {
            cap_pll = 6;
        }
        else if (vco_freq_khz > 50666640) {
            cap_pll = 7;
        }
        else if (vco_freq_khz > 49999970) {
            cap_pll = 8;
        }
        else if (vco_freq_khz > 49400000) {
            cap_pll = 0;
        }
        else if (vco_freq_khz > 48800000) {
            cap_pll = 1;
        }
        else if (vco_freq_khz > 48200000) {
            cap_pll = 2;
        }
        else if (vco_freq_khz > 47600000) {
            cap_pll = 3;
        }
        else if (vco_freq_khz > 47000000) {
            cap_pll = 4;
        }
        else if (vco_freq_khz > 46400000) {
            cap_pll = 5;
        }
        else if (vco_freq_khz > 45800000) {
            cap_pll = 6;
        }
        else if (vco_freq_khz > 45200000) {
            cap_pll = 7;
        }
        else if (vco_freq_khz > 44600000) {
            cap_pll = 8;
        }
        else if (vco_freq_khz > 44000000) {
            cap_pll = 9;
        }
        else if (vco_freq_khz > 43400000) {
            cap_pll = 10;
        }
        else if (vco_freq_khz > 42800000) {
            cap_pll = 11;
        }
        else if (vco_freq_khz > 42200000) {
            cap_pll = 12;
        }
        else if (vco_freq_khz > 41600000) {
            cap_pll = 13;
        }
        else {
            cap_pll = 14;
        }
        EFUN(wrc_ams_pll_cap_l_pll(cap_pll));
        EFUN(wrc_ams_pll_cap_r_pll(cap_pll));
    }
    {
        uint8_t drv_pll = 0;
        if (vco_freq_khz > 54000000) {
            drv_pll = 4;
        }
        else if (vco_freq_khz > 52000000) {
            drv_pll = 5;
        }
        else if (vco_freq_khz > 49900000) {
            drv_pll = 6;
        }
        else if (vco_freq_khz > 47750000) {
            drv_pll = 1;
        }
        else if (vco_freq_khz > 45500000) {
            drv_pll = 2;
        }
        else if (vco_freq_khz > 43250000) {
            drv_pll = 3;
        }
        else {
            drv_pll = 4;
        }
        EFUN(wrc_ams_pll_ictrl_drv_l_pll(drv_pll));
        EFUN(wrc_ams_pll_ictrl_drv_r_pll(drv_pll));
    }
    {
        /* Get information needed for fractional mode configuration.
         * Start with the div value composed of an integer and a wide fractional value.
         *
         * The value programmed into the pll_fracn_* bitfields which must account for the
         * initial div2 stage after the VCO.  For instance, a divide by 147.2 must be
         * programmed with an integer of 73 and a fraction of 0.6.
         *
         * Start with the div value, divided by 2, composed of an integer and a wide fractional value.
         */
        const uint8_t  div_fraction_width = 28; /* Must be less than 32 due to overflow detection below. */
        const uint16_t div_integer        = (uint16_t)(SRDS_INTERNAL_GET_PLL_DIV_INTEGER(srds_div) >> 1);
        const uint32_t div_fraction       = (((SRDS_INTERNAL_GET_PLL_DIV_INTEGER(srds_div) & 1) << (div_fraction_width-1))
                                             | SRDS_INTERNAL_GET_PLL_DIV_FRACTION_NUM(srds_div, div_fraction_width-1));

        /* The div_fraction may have more precision than our pll_fracn_frac bitfield.
         * So round it.  Start by adding 1/2 LSB of the fraction div_fraction.
         */
        const uint32_t div_fraction_0p5 = 1 << (div_fraction_width - plp_aperta2_pll_fracn_frac_bits - 1);
        const uint32_t div_fraction_plus_0p5 = div_fraction + div_fraction_0p5;

        /* Did div_fraction_plus_p5 have a carry bit? */
        const uint32_t div_fraction_plus_p5_carry = div_fraction_plus_0p5 >> div_fraction_width;

        /* The final rounded div_fraction, including carry up to div_integer.
         * This removes the carry and implements the fixed point truncation.
         */
        const uint16_t pll_fracn_ndiv_int  = (uint16_t)(div_integer + div_fraction_plus_p5_carry);
        const uint32_t pll_fracn_div = ((div_fraction_plus_0p5 & ((1U << div_fraction_width)-1))
                                        >> (div_fraction_width - plp_aperta2_pll_fracn_frac_bits));

        if (pll_fracn_ndiv_int != (pll_fracn_ndiv_int & ((1 << plp_aperta2_pll_fracn_ndiv_int_bits)-1))) {
            EFUN_PRINTF(("ERROR:  PLL divide is too large for div value 0x%08X\n", srds_div));
            return (peregrine5_pc_error(sa__, ERR_CODE_PLL_DIV_INVALID));
        }

        /* fracn_ndiv_int restrcited to 12 to 511) */
        if ((pll_fracn_ndiv_int < 12) || (pll_fracn_ndiv_int > 511)) {
            return (peregrine5_pc_error(sa__, ERR_CODE_INVALID_PLL_CFG));
        }

        {
            uint8_t rz_sel, imode, imax, cp_sel, cp_sel_extra, fp3_c_sel, fracn_enable;

            if (pll_fracn_div > 0) { /* Fractional mode settings */
                rz_sel = 0x4;
                imode = 0x0;
                imax = 0x0;
                cp_sel = 0x7;
                cp_sel_extra = 0x1;
                fp3_c_sel = 0xf;
                fracn_enable = 0x1;
            }
            else {                   /* Integer mode settings */
                rz_sel = 0x1;
                imode = 0x3;
                imax = 0x3;
                cp_sel = 0x1;
                cp_sel_extra = 0x0;
                fp3_c_sel = 0x0;
                fracn_enable = 0x0;
            }

            EFUN(wrc_ams_pll_rz_sel(rz_sel));
            EFUN(wrc_ams_pll_imode(imode));
            EFUN(wrc_ams_pll_imax(imax));
            EFUN(wrc_ams_pll_cp_sel(cp_sel));
            EFUN(wrc_ams_pll_cp_sel_extra(cp_sel_extra));
            EFUN(wrc_ams_pll_fp3_c_sel(fp3_c_sel));
            EFUN(wrc_ams_pll_fracn_enable(fracn_enable));
        }

        /* To ensure glitch-free operation - toggle ndiv_valid low; load ndiv values; toggle ndiv_valid high. */
        EFUN(wrc_ams_pll_resetb_mmd(0x0));                                /* reset PLL mmd */
        EFUN(wrc_ams_pll_resetb_mmd(0x1));                                /* release reset */
        EFUN(wrc_ams_pll_i_ndiv_valid(0x0));                              /* assert low before programming fracn PLL div value */
        if (pll_fracn_ndiv_int < 60) {
            EFUN(wrc_ams_pll_pll_frac_mode(0x2)); /* MMD 4/5 mode (pll_frac_mode = 2) => [12 <= Ndiv < 60] */
        }
        else {
            EFUN(wrc_ams_pll_pll_frac_mode(0x1)); /* MMD 8/9 mode (pll_frac_mode = 1) => [60 <= Ndiv < 512] */
        }
        EFUN(wrc_ams_pll_fracn_ndiv_int   (pll_fracn_ndiv_int));          /* interger part of fracn PLL div */
        EFUN(wrc_ams_pll_fracn_ndiv_frac_l(_ndiv_frac_l(pll_fracn_div))); /* set lower 15 bits of fractional part of fracn PLL div */
        EFUN(wrc_ams_pll_fracn_ndiv_frac_h((uint8_t)(_ndiv_frac_h(pll_fracn_div)))); /* set upper  3 bits of fractional part of fracn PLL div */
        EFUN(wrc_ams_pll_rstb_rtl_div(0));
        EFUN(wrc_ams_pll_rstb_rtl_div(1));
        EFUN(wrc_ams_pll_i_ndiv_valid(0x1));                              /* to load fracn_ndiv and ndiv_int */
        EFUN(USR_DELAY_US(5));                                            /* delay of atleast 8 Refclk cycles */
        EFUN(wrc_ams_pll_i_ndiv_valid(0x0));                              /* to latch fracn_ndiv and ndiv_int */
    }

    /* NOTE: Might have to add some optimized PLL control settings post-DVT*/

    /* Update core variables with the VCO rate. */
    {
        struct peregrine5_pc_uc_core_config_st core_config = UC_CORE_CONFIG_INIT;
        EFUN(plp_aperta2_peregrine5_pc_get_uc_core_config(sa__, &core_config));
        core_config.vco_rate_in_Mhz = (int32_t)((vco_freq_khz + 500) / 1000);
        core_config.field.vco_rate = MHZ_TO_VCO_RATE(core_config.vco_rate_in_Mhz);
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_set_uc_core_config(sa__, core_config));
    }

    return (ERR_CODE_NONE);
} /* peregrine5_pc_configure_pll */

#ifndef SMALL_FOOTPRINT
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_read_pll_div(srds_access_t *sa__, uint32_t *srds_div) {
    INIT_SRDS_ERR_CODE
    uint16_t pll_fracn_ndiv_int;
    uint32_t pll_fracn_div;
    ESTM(pll_fracn_ndiv_int = rdc_ams_pll_fracn_ndiv_int());
    ESTM(pll_fracn_div = (uint32_t)(_ndiv_frac_decode(rdc_ams_pll_fracn_ndiv_frac_l(), rdc_ams_pll_fracn_ndiv_frac_h())));

    /* The value programmed into the pll_fracn_* bitfields which must
     * account for the initial div2 stage after the VCO.  For instance, a
     * divide by 147.2 must be programmed with an integer of 73 and a
     * fraction of 0.6.
     *
     * So multiply the bitfield reads by 2.
     */

      pll_fracn_ndiv_int   = (uint16_t)(pll_fracn_ndiv_int << 1);
      pll_fracn_div      <<= 1;

      {
          /* If the post-multiplied fractional part overflows, then apply the carry to
           * the integer part.
           */
          const uint32_t pll_fracn_div_masked = pll_fracn_div & ((1U << plp_aperta2_pll_fracn_frac_bits)-1);
          if (pll_fracn_div_masked != pll_fracn_div) {
              ++pll_fracn_ndiv_int;
              pll_fracn_div = pll_fracn_div_masked;
          }
      }

    *srds_div = SRDS_INTERNAL_COMPOSE_PLL_DIV(pll_fracn_ndiv_int, pll_fracn_div, plp_aperta2_pll_fracn_frac_bits);
    return (ERR_CODE_NONE);
}
#endif /* SMALL_FOOTPRINT */
