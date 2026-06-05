/***********************************************************************************
 *                                                                                 *
 * Copyright: (c) 2021 Broadcom.                                                   *
 * Broadcom Proprietary and Confidential. All rights reserved.                     *
 *                                                                                 *
 ***********************************************************************************/

/**********************************************************************************
 **********************************************************************************
 *  File Name     :  blackhawk_pll_config.c                                       *
 *  Created On    :  11 Feb 2016                                                  *
 *  Created By    :  Brent Roberts                                                *
 *  Description   :  Blackhawk PLL Configuration API                              *
 *  Revision      :    *
 *                                                                                *
 **********************************************************************************
 **********************************************************************************/


/** @file blackhawk_pll_config.c
 * Blackhawk PLL Configuration
 */


#include "blackhawk_barchetta_config.h"
#include "blackhawk_barchetta_functions.h"
#include "blackhawk_barchetta_internal.h"
#include "blackhawk_barchetta_internal_error.h"
#include "blackhawk_barchetta_select_defns.h"
/* BLACKHAWK TODO:  This file was copied from Falcon16.
 * BLACKHAWK TODO:  vco_indicator removed, because it's reserved now.
 */

#define VCO_FREQ_KHZ_NEAR(vco_freq_khz_, near_val_) (   ((vco_freq_khz_) >= (near_val_) - 10000) \
                                                     || ((vco_freq_khz_) <= (near_val_) + 10000))

/* The pll_fracn_ndiv_int and pll_fracn_frac bitfields have this many bits. */
static const uint32_t plp_barchetta_pll_fracn_ndiv_int_bits = 10;
static const uint32_t plp_barchetta_pll_fracn_frac_bits     = 18;

err_code_t plp_barchetta_blackhawk_barchetta_INTERNAL_configure_pll(srds_access_t *sa__,
                                         enum blackhawk_barchetta_pll_refclk_enum refclk,
                                         enum blackhawk_barchetta_pll_div_enum srds_div,
                                         uint32_t vco_freq_khz,
                                         enum blackhawk_barchetta_pll_option_enum pll_option) {
    uint32_t refclk_freq_hz;
    uint8_t int_configured;

    EFUN(plp_barchetta_blackhawk_barchetta_INTERNAL_resolve_pll_parameters(sa__, refclk, &refclk_freq_hz, &srds_div, &vco_freq_khz, pll_option));


    /* Use this to restore defaults if reprogramming the PLL under dp-reset (typically Auto-Neg FW) */
    EFUN(wrc_pll_mode(0x7));
    EFUN(wrc_ams_pll_vco2_15g(0x0));
    EFUN(wrc_ams_pll_fracn_ndiv_int(0x0));
    EFUN(wrc_ams_pll_fracn_div(0x0000));
    EFUN(wrc_ams_pll_fracn_div_17_16(0x0));
    EFUN(wrc_ams_pll_fracn_bypass(0x0));
    EFUN(wrc_ams_pll_fracn_divrange(0x0));
    EFUN(wrc_ams_pll_ditheren(0x0));
    EFUN(wrc_ams_pll_fracn_sel(0x0));
    EFUN(wrc_ams_pll_kvh_force(0x0));
    EFUN(wrc_ams_pll_force_kvh_bw(0x0));
    EFUN(wrc_ams_pll_refclk_doubler(0x0));
    EFUN(wrc_ams_pll_doubler_res(0x0));
    EFUN(wrc_ams_pll_doubler_cap(0x0));
    EFUN(wrc_ams_pll_div2(0x0));
    EFUN(wrc_ams_pll_div4(0x0));
    EFUN(wrc_ams_pll_div4_2_sel(0x0));
    EFUN(wrc_ams_pll_ndiv_frac_valid(0x0));
    EFUN(wrc_ams_pll_pll2rx_clkbw(0x0));
    EFUN(wrc_refclk_divcnt(0x27));
    EFUN(wrc_vco_step_time(0x3));
    EFUN(wrc_ams_pll_fp3_rh(0x0));
    EFUN(wrc_ams_pll_fp3_ctrl(0x0));
    EFUN(wrc_ams_pll_iqp(0x4));

    {
        uint8_t reset_state;
        /* Use core_s_rstb to re-initialize all registers to default before calling this function. */
        ESTM(reset_state = rdc_core_dp_reset_state());

        if(reset_state < 7) {
            EFUN_PRINTF(("ERROR: blackhawk_barchetta_configure_pll(..) called without core_dp_s_rstb=0\n"));
            return (blackhawk_barchetta_error(sa__, ERR_CODE_CORE_DP_NOT_RESET));
        }
    }

    /* Clear PLL powerdown */
    EFUN(wrc_ams_pll_pwrdn(0));


    /* Per AB on 13 Nov 2015:  Use VCO2 for 22.6 GHz and below.
     * The next supported frequency is 23.0 GHz, so let's use 22.8 GHz as the threshold.
     */
    EFUN(wrc_ams_pll_vco2_15g((vco_freq_khz < 22800000) ? 1 : 0));

    int_configured = 0;
    if (!SRDS_INTERNAL_IS_PLL_DIV_FRACTIONAL(srds_div)) {
        uint8_t pll_mode;
        int_configured = 1;
        switch (SRDS_INTERNAL_GET_PLL_DIV_INTEGER(srds_div)) {
            case  64: pll_mode =  0; break;
            case  66: pll_mode =  1; break;
            case  80: pll_mode =  2; break;
            case 128: pll_mode =  3; break;
            case 132: pll_mode =  4; break;
            case 140: pll_mode =  5; break;
            case 160: pll_mode =  6; break;
            case 165: pll_mode =  7; break;
            case 168: pll_mode =  8; break;
            case 170: pll_mode =  9; break;
            case 175: pll_mode = 10; break;
            case 180: pll_mode = 11; break;
            case 184: pll_mode = 12; break;
            case 200: pll_mode = 13; break;
            case 224: pll_mode = 14; break;
            case 264: pll_mode = 15; break;
            case  96: pll_mode = 16; break;
            case 120: pll_mode = 17; break;
            case 144: pll_mode = 18; break;
            case 198: pll_mode = 19; break;
            default: int_configured = 0;
        }
    
        if (int_configured) {
            EFUN(wrc_pll_mode(pll_mode));
        }
    }
    
    EFUN(wrc_ams_pll_fracn_sel(int_configured ? 0 : 1));

    if (!int_configured) {
        /* Either fractional mode was requested, or integer mode doesn't support the divisor.
         * Get information needed for fractional mode configuration.
         *
         * The value programmed into the pll_fracn_* bitfields which must account for the
         * initial div2 stage after the VCO.  For instance, a divide by 147.2 must be
         * programmed with an integer of 73 and a fraction of 0.6.
         *
         * Start with the div value, divided by 2, composed of an integer and a wide fractional value.
         */
        const uint8_t  div_fraction_width = 28; /* Must be less than 32 due to overflow detection below. */
        const uint16_t div_integer        = (uint16_t)(SRDS_INTERNAL_GET_PLL_DIV_INTEGER(srds_div) >> 1);
        const uint32_t div_fraction       = (uint32_t)(((SRDS_INTERNAL_GET_PLL_DIV_INTEGER(srds_div) & 1) << (div_fraction_width-1))
                                             | SRDS_INTERNAL_GET_PLL_DIV_FRACTION_NUM(srds_div, div_fraction_width-1));

        /* The div_fraction may have more precision than our pll_fracn_frac bitfield.
         * So round it.  Start by adding 1/2 LSB of the fraction div_fraction.
         */
        const uint32_t div_fraction_0p5 = 1 << (div_fraction_width - plp_barchetta_pll_fracn_frac_bits - 1);
        const uint32_t div_fraction_plus_0p5 = div_fraction + div_fraction_0p5;

        /* Did div_fraction_plus_p5 have a carry bit? */
        const uint32_t div_fraction_plus_p5_carry = div_fraction_plus_0p5 >> div_fraction_width;

        /* The final rounded div_fraction, including carry up to div_integer.
         * This removes the carry and implements the fixed point truncation.
         */
        const uint16_t pll_fracn_ndiv_int  = (uint16_t)(div_integer + div_fraction_plus_p5_carry);
        const uint32_t pll_fracn_div = ((div_fraction_plus_0p5 & ((1U << div_fraction_width)-1))
                                        >> (div_fraction_width - plp_barchetta_pll_fracn_frac_bits));

        if (pll_fracn_ndiv_int != (pll_fracn_ndiv_int & ((1 << plp_barchetta_pll_fracn_ndiv_int_bits)-1))) {
            EFUN_PRINTF(("ERROR:  PLL divide is too large for div value 0x%08X\n", srds_div));
            return (blackhawk_barchetta_error(sa__, ERR_CODE_PLL_DIV_INVALID));
        }

        EFUN(wrc_pll_mode(0));
        if ((pll_fracn_ndiv_int < 12) || (pll_fracn_ndiv_int > 251)) {
            return (blackhawk_barchetta_error(sa__, ERR_CODE_INVALID_PLL_CFG));
        }
        EFUN(wrc_ams_pll_fracn_ndiv_int (pll_fracn_ndiv_int));
        EFUN(wrc_ams_pll_fracn_div      (pll_fracn_div & 0xFFFF));
        EFUN(wrc_ams_pll_fracn_div_17_16((uint8_t)(pll_fracn_div >> 16)));
        EFUN(wrc_ams_pll_fracn_bypass   (0));
        EFUN(wrc_ams_pll_fracn_divrange ((pll_fracn_ndiv_int < 91) ? 1 : 0));
        if (pll_fracn_div != 0) {
            {
                EFUN(wrc_ams_pll_fp3_rh(0x1));
                EFUN(wrc_ams_pll_fp3_ctrl(0xF));
            }
        }
        /* toggle ndiv_frac_valid high, then low to load in a new value for fracn_div. */
        EFUN(wrc_ams_pll_ndiv_frac_valid(1));
        EFUN(wrc_ams_pll_ndiv_frac_valid(0));
    }


    /* Determine KVH and pll2rx_clkbw_val per Falcon16 AMS v1.1 and comments in CRSRDSAPI-137 and CRSRDSFW-138 */
    {
        uint8_t kvh_force;
        uint8_t pll2rx_clkbw_val;

        if (vco_freq_khz > 27400000) {
            kvh_force = 0;
        } else if ((vco_freq_khz <= 27400000) && (vco_freq_khz > 18750000)) {
            kvh_force = 1;
        } else {
            kvh_force = 3;
        }

        if (vco_freq_khz > 25781250) {
            pll2rx_clkbw_val = 0;
        } else if ((vco_freq_khz <= 25781250) && (vco_freq_khz > 22600000)) {
            pll2rx_clkbw_val = 1;
        } else {
            pll2rx_clkbw_val = 3;
        }

        /* Per AB on 27 Oct 2015, vco_indicator is always forced to 0. */
#if defined(wrc_ams_pll_vco_indicator)
        EFUN(wrc_ams_pll_vco_indicator(0));
#endif
        EFUN(wrc_ams_pll_pll2rx_clkbw(pll2rx_clkbw_val));
        EFUN(wrc_ams_pll_kvh_force(kvh_force));
        EFUN(wrc_ams_pll_force_kvh_bw(1));

        /* Handle refclk PLL options */
        pll_option = (enum blackhawk_barchetta_pll_option_enum)(pll_option & BLACKHAWK_BARCHETTA_PLL_OPTION_REFCLK_MASK);
        if (pll_option == BLACKHAWK_BARCHETTA_PLL_OPTION_REFCLK_DOUBLER_EN) {
            EFUN(wrc_ams_pll_refclk_doubler(0x1));
            EFUN(wrc_ams_pll_doubler_res(0x6));
            EFUN(wrc_ams_pll_doubler_cap(0x3));
        } else if (pll_option == BLACKHAWK_BARCHETTA_PLL_OPTION_REFCLK_DIV2_EN) {
            EFUN(wrc_ams_pll_div2(1));
            EFUN(wrc_ams_pll_div4_2_sel(1));
        } else if (pll_option == BLACKHAWK_BARCHETTA_PLL_OPTION_REFCLK_DIV4_EN) {
            EFUN(wrc_ams_pll_div4(1));
            EFUN(wrc_ams_pll_div4_2_sel(1));
        }


        /* Force refclk_divcnt for refclk faster than 156.25 Mhz. See CRSRDSAPI-185/CRFALCON16-506. */
        if (refclk_freq_hz > 156250000) {
            uint16_t refclk_divcnt = (uint16_t)(39 * (refclk_freq_hz/15625000));
            EFUN(wrc_refclk_divcnt(refclk_divcnt/10));
        }

        /* Force vco_step_time. See CRSRDSAPI-229. */
        EFUN(wrc_vco_step_time(8));
    }

    /* Update core variables with the VCO rate. */
    {
        struct blackhawk_barchetta_uc_core_config_st core_config;
        EFUN(plp_barchetta_blackhawk_barchetta_get_uc_core_config(sa__, &core_config));
        core_config.vco_rate_in_Mhz = (int32_t)((vco_freq_khz + 500) / 1000);
        core_config.field.vco_rate = MHZ_TO_VCO_RATE(core_config.vco_rate_in_Mhz);
        EFUN(plp_barchetta_blackhawk_barchetta_INTERNAL_set_uc_core_config(sa__, core_config));
    }

    {
        uint8_t pll_idx;
        uint8_t other_pll_lock;
        ESTM(pll_idx = plp_barchetta_blackhawk_barchetta_get_pll_idx(sa__));
        EFUN(plp_barchetta_blackhawk_barchetta_set_pll_idx(sa__, ((pll_idx == 0) ? 1 : 0)));
        ESTM(other_pll_lock = rdc_pll_lock());
        if (!other_pll_lock) {
            uint8_t misc_ctrl_byte;
            ESTM(misc_ctrl_byte = rdcv_misc_ctrl_byte());
            if (pll_idx == 1) misc_ctrl_byte |= 1<<2;
            else              misc_ctrl_byte &= (uint8_t)(~(1<<2));
            EFUN(wrcv_misc_ctrl_byte(misc_ctrl_byte));
        }
        EFUN(plp_barchetta_blackhawk_barchetta_set_pll_idx(sa__, pll_idx));
    }

    return (ERR_CODE_NONE);
} /* blackhawk_barchetta_configure_pll */

err_code_t plp_barchetta_blackhawk_barchetta_INTERNAL_read_pll_div(srds_access_t *sa__, uint32_t *srds_div) {
    uint8_t fracn_sel;
    ESTM(fracn_sel = rdc_ams_pll_fracn_sel());

    if (fracn_sel) {
        uint16_t pll_fracn_ndiv_int;
        uint32_t pll_fracn_div;
        ESTM(pll_fracn_ndiv_int = rdc_ams_pll_fracn_ndiv_int());
        ESTM(pll_fracn_div = (uint32_t)(rdc_ams_pll_fracn_div() | (rdc_ams_pll_fracn_div_17_16() << 16)));

        /* The value programmed into the pll_fracn_* bitfields which must
         * account for the initial div2 stage after the VCO.  For instance, a
         * divide by 147.2 must be programmed with an integer of 73 and a
         * fraction of 0.6.
         *
         * So multiply the bitfield reads by 2.
         */
        pll_fracn_ndiv_int = (uint16_t)(pll_fracn_ndiv_int << 1);
        pll_fracn_div      <<= 1;
        
        {
            /* If the post-multiplied fractional part overflows, then apply the carry to
             * the integer part.
             */
            const uint32_t pll_fracn_div_masked = pll_fracn_div & ((1U << plp_barchetta_pll_fracn_frac_bits)-1);
            if (pll_fracn_div_masked != pll_fracn_div) {
                ++pll_fracn_ndiv_int;
                pll_fracn_div = pll_fracn_div_masked;
            }
        }
    
        *srds_div = (uint32_t)(SRDS_INTERNAL_COMPOSE_PLL_DIV(pll_fracn_ndiv_int, pll_fracn_div, plp_barchetta_pll_fracn_frac_bits));
    } else {
        uint8_t pll_mode;
        ESTM(pll_mode = rdc_pll_mode());
        switch (pll_mode) {
            case  0: *srds_div = SRDS_INTERNAL_COMPOSE_PLL_DIV(  64, 0, 18); break;
            case  1: *srds_div = SRDS_INTERNAL_COMPOSE_PLL_DIV(  66, 0, 18); break;
            case  2: *srds_div = SRDS_INTERNAL_COMPOSE_PLL_DIV(  80, 0, 18); break;
            case  3: *srds_div = SRDS_INTERNAL_COMPOSE_PLL_DIV( 128, 0, 18); break;
            case  4: *srds_div = SRDS_INTERNAL_COMPOSE_PLL_DIV( 132, 0, 18); break;
            case  5: *srds_div = SRDS_INTERNAL_COMPOSE_PLL_DIV( 140, 0, 18); break;
            case  6: *srds_div = SRDS_INTERNAL_COMPOSE_PLL_DIV( 160, 0, 18); break;
            case  7: *srds_div = SRDS_INTERNAL_COMPOSE_PLL_DIV( 165, 0, 18); break;
            case  8: *srds_div = SRDS_INTERNAL_COMPOSE_PLL_DIV( 168, 0, 18); break;
            case  9: *srds_div = SRDS_INTERNAL_COMPOSE_PLL_DIV( 170, 0, 18); break;
            case 10: *srds_div = SRDS_INTERNAL_COMPOSE_PLL_DIV( 175, 0, 18); break;
            case 11: *srds_div = SRDS_INTERNAL_COMPOSE_PLL_DIV( 180, 0, 18); break;
            case 12: *srds_div = SRDS_INTERNAL_COMPOSE_PLL_DIV( 184, 0, 18); break;
            case 13: *srds_div = SRDS_INTERNAL_COMPOSE_PLL_DIV( 200, 0, 18); break;
            case 14: *srds_div = SRDS_INTERNAL_COMPOSE_PLL_DIV( 224, 0, 18); break;
            case 15: *srds_div = SRDS_INTERNAL_COMPOSE_PLL_DIV( 264, 0, 18); break;
            case 16: *srds_div = SRDS_INTERNAL_COMPOSE_PLL_DIV(  96, 0, 18); break;
            case 17: *srds_div = SRDS_INTERNAL_COMPOSE_PLL_DIV( 120, 0, 18); break;
            case 18: *srds_div = SRDS_INTERNAL_COMPOSE_PLL_DIV( 144, 0, 18); break;
            case 19: *srds_div = SRDS_INTERNAL_COMPOSE_PLL_DIV( 198, 0, 18); break;
            default:
                EFUN_PRINTF(("ERROR: plp_barchetta_blackhawk_barchetta_INTERNAL_read_pll_div() found invalid pll_mode value:  %d\n", pll_mode));
                return (blackhawk_barchetta_error(sa__, ERR_CODE_PLL_DIV_INVALID));
        }
    }
    return (ERR_CODE_NONE);
}
