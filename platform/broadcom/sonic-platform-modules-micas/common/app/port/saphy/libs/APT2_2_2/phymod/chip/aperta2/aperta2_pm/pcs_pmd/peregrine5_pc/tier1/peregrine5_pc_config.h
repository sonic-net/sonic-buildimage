/***********************************************************************************
 *                                                                                 *
 * Copyright: (c) 2021 Broadcom.                                                   *
 * Broadcom Proprietary and Confidential. All rights reserved.                     *
 *                                                                                 *
 ***********************************************************************************/

/***********************************************************************************
 ***********************************************************************************
 *                                                                                 *
 *  Revision    :      *
 *                                                                                 *
 *  Description :  Config functions targeted to IP user                            *
 *                                                                                 *
 ***********************************************************************************
 ***********************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PEREGRINE5_PC_API_CONFIG_H
#define PEREGRINE5_PC_API_CONFIG_H

#include "peregrine5_pc_ipconfig.h"
#include "common/srds_api_enum.h"
#include "common/srds_api_err_code.h"
#include "common/srds_api_types.h"
#include "peregrine5_pc_enum.h"
#include "peregrine5_pc_types.h"
#include "peregrine5_pc_select_defns.h"
#include "peregrine5_pc_access.h"

/*! @file
 *  @brief Configuration functions provided to IP User
 */

/*! @addtogroup APITag
 * @{
 */

/*! @defgroup SerdesAPIConfigTag Configuration APIs
 * Serdes API functions which can be used to get and/or set
 * Serdes configuration settings, as well as enable/disable modes.
 * Also includes APIs for loading and verifying uCode.
 */

/*! @addtogroup SerdesAPIConfigTag
 * @{
 */

#define GRACEFUL_STOP_TIME (2000)

#define IF_API_ID_CODES_DONT_MATCH_CORE_ID_CODES      \
uint16_t revid, revid2, ipversion;                    \
ESTM(revid     = rdc_revid_model());                  \
ESTM(revid2    = rdc_revid2());                       \
ESTM(ipversion = rd_tx_sts_version_id());             \
if (PEREGRINE5_PC_REVID_MODEL_DEFAULT != revid || ((PEREGRINE5_PC_REVID2_DEFAULT_A0_1 != revid2) && (PEREGRINE5_PC_REVID2_DEFAULT_A0_2 != revid2) && (PEREGRINE5_PC_REVID2_DEFAULT_A0_6 != revid2)) || PEREGRINE5_PC_AFE_IPVERSION_ID_DEFAULT != ipversion) { \
    EFUN_PRINTF(("ERROR: IP Hardware ID Mismatch. Expected: 0x%08X (A0_1), 0x%08X (A0_2), or 0x%08X (A0_6). Actual: 0x%08X.\n",                                                     \
                (uint32_t)((PEREGRINE5_PC_REVID_MODEL_DEFAULT << 16) | (PEREGRINE5_PC_REVID2_DEFAULT_A0_1 << 8) | (PEREGRINE5_PC_AFE_IPVERSION_ID_DEFAULT)),          \
                (uint32_t)((PEREGRINE5_PC_REVID_MODEL_DEFAULT << 16) | (PEREGRINE5_PC_REVID2_DEFAULT_A0_2 << 8) | (PEREGRINE5_PC_AFE_IPVERSION_ID_DEFAULT)),          \
                (uint32_t)((PEREGRINE5_PC_REVID_MODEL_DEFAULT << 16) | (PEREGRINE5_PC_REVID2_DEFAULT_A0_6 << 8) | (PEREGRINE5_PC_AFE_IPVERSION_ID_DEFAULT)),          \
                (uint32_t)((revid << 16) | (revid2 << 8) | (ipversion))));


#define CLOSING_BRACE }

#define RELEASE_LOCK_AND_RETURN(__x__) \
    USR_RELEASE_LOCK; \
    return (__x__);

/****************************************************/
/*  CORE Based APIs - Required to be used per Core  */
/****************************************************/
#ifndef SMALL_FOOTPRINT
/* Returns API Version Number */
/** API Version Number.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[out] api_version API Version Number returned by the API
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_version(srds_access_t *sa__, uint32_t *api_version);
#endif /* SMALL_FOOTPRINT */

/*------------------------------------------------*/
/*  APIs to Read Core Config variables in uC RAM  */
/*------------------------------------------------*/
/** Read value of core_config uC RAM variable.
 *  Note that various API configuration functions can modify core config.
 *  Since the value returned by this can become stale, re-read after modifying core configuration.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[out] struct_val core_config RAM variable values read by the API
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_get_uc_core_config(srds_access_t *sa__, struct peregrine5_pc_uc_core_config_st *struct_val);
/*------------------------------------------------*/
/*  APIs to get physical Tx addr for given lane   */
/*------------------------------------------------*/
/** Read value of physical Tx address for a mapped lane.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] lane Logical lane address for the lane
 * @param[out] physical_tx_lane get the physical address of the tx logical lane
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_get_physical_tx_addr(srds_access_t *sa__, uint8_t lane, uint8_t *physical_tx_lane);

/*------------------------------------------------*/
/*  APIs to get physical Rx addr for given lane   */
/*------------------------------------------------*/
/** Read value of physical Rx address for a mapped lane.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] lane Logical lane address for the lane
 * @param[out] physical_rx_lane get the physical address of the rx logical lane
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_get_physical_rx_addr(srds_access_t *sa__, uint8_t lane, uint8_t *physical_rx_lane);


/*------------------------------------------------*/
/*  APIs to get number of uc cores  */
/*------------------------------------------------*/
/** Get the number of uc cores.
 *  Note that various API configuration functions can modify core config.
 *  Since the value returned by this can become stale, re-read after modifying core configuration.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[out] num_micros Value containing the number of uc cores returned by the API.
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_get_micro_num_uc_cores(srds_access_t *sa__, uint8_t *num_micros);

/*-----------------------------------*/
/*  Microcode Load/Verify Functions  */
/*-----------------------------------*/
/** Performs the initial register writes which are required before loading microcode.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] pram indicates if the microcode is loaded through the PRAM interface (1 = yes; 0 = no )
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_ucode_load_init(srds_access_t *sa__, uint8_t pram);

/** Writes all or part of the microcode image into Micro.
 * Note: If microcode is being loaded with multiple writes, then ucode_len must be divisble by 4 (32 bit aligned)
 * for all writes, except for the final write, which does not need to be 32 bit aligned.
 * Note: plp_aperta2_peregrine5_pc_ucode_load_init() must be called before the first call to plp_aperta2_peregrine5_pc_ucode_load_write(),
 * and plp_aperta2_peregrine5_pc_ucode_load_close() must be called after the last call to plp_aperta2_peregrine5_pc_ucode_load_write().
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] ucode_image pointer to the Microcode image organized in bytes.
 * @param[in] ucode_len Length of Microcode Image (number of bytes).
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_ucode_load_write(srds_access_t *sa__, uint8_t *ucode_image, uint32_t ucode_len);

/** Ends Microcode loading state. Call after the last plp_aperta2_peregrine5_pc_ucode_load_write().
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] pram indicates if the microcode is loaded through the PRAM interface (1 = yes; 0 = no )
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_ucode_load_close(srds_access_t *sa__, uint8_t pram);

/** Load Microcode into current one core through Register (MDIO) Interface.
 * Once the microcode is loaded, de-assert reset to micro to start executing microcode "wrc_micro_core_rstb(0x1)".
 * Note: Micro should be loaded only after issuing a plp_aperta2_peregrine5_pc_uc_reset(1) followed by asserting and de-asserting
 * core_s_reset. Information table should be intialized with plp_aperta2_peregrine5_pc_init_peregrine5_pc_info after microcode load.
 * See relevant Programmers guide for more details.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] ucode_image pointer to the Microcode image organized in bytes.
 * @param[in] ucode_len Length of Microcode Image (number of bytes).
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_ucode_load(srds_access_t *sa__, uint8_t *ucode_image, uint32_t ucode_len);


#ifndef SMALL_FOOTPRINT
/** To verify the Microcode image loaded in the Micro.
 * Read back the microcode from Micro and check against expected microcode image.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] ucode_image pointer to the expected Microcode image organized in bytes.
 * @param[in] ucode_len Length of Microcode Image (number of bytes).
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_ucode_load_verify(srds_access_t *sa__, uint8_t *ucode_image, uint32_t ucode_len);

/** To verify the CRC of the microcode loaded in the Micro.
 * Instruct uC to read image and calculate CRC and check against expected CRC.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] ucode_len Length of Microcode Image (number of bytes)
 * @param[in] expected_crc_value Expected CRC value of the microcode
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_ucode_crc_verify(srds_access_t *sa__, uint32_t ucode_len, uint16_t expected_crc_value);

#endif /* SMALL_FOOTPRINT */

/** Load the Micro through the PRAM bus.
 * NOTE: Information table should be intialized with plp_aperta2_peregrine5_pc_init_peregrine5_pc_info after microcode load.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] ucode_image Microcode Image to be written.
 * @param[in] ucode_len Length of Microcode Image (number of bytes).
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_ucode_pram_load(srds_access_t *sa__, char const * ucode_image, uint32_t ucode_len);

/** Enable or Disable the uC reset.
 * Dummy function to maintain compatibility with BHK7 APIs [calls plp_aperta2_peregrine5_pc_uc_reset(..) internally].
 * Note: Micro should be reset using the API everytime before reloading the microcode.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] enable Enable/Disable uC reset (1 = Enable; 0 = Disable).
 * @param[in] ucode_info struct has information regarding stack size.
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_uc_reset_with_info(srds_access_t *sa__, uint8_t enable, ucode_info_t ucode_info);

/** Enable or Disable the uC reset.
 * Note: Micro should be reset using the API everytime before reloading the microcode.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] enable Enable/Disable uC reset (1 = Enable; 0 = Disable).
 * @param[in] ucode_info struct has information regarding stack size.
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_uc_reset(srds_access_t *sa__, uint8_t enable, ucode_info_t ucode_info);

/** API to assert Serdes core reset.
 * Issues plp_aperta2_peregrine5_pc_uc_reset followed by peregrine5_pc core reset (core_s_rstb) after selecting com_clk as micro_clk.
 * NOTE: This API should be used to issue a core reset instead of directly using the core_s_rstb register.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] ucode_info struct has information regarding stack size.
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_core_reset(srds_access_t *sa__, ucode_info_t ucode_info);

/** Wait for the uC to become active.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @return Error Code generated if uC does not become active (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_wait_uc_active(srds_access_t *sa__);

/* cleanup */
#define INFO_TABLE_END INFO_TABLE_END_COMMON_BLOCK

/** Initialize the plp_aperta2_peregrine5_pc_info for the uC.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @return Error Code generated if uC does not become active (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_init_peregrine5_pc_info(srds_access_t *sa__);

/** Clear the plp_aperta2_peregrine5_pc_info for the uC.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_clear_peregrine5_pc_info(srds_access_t *sa__);

#ifndef SMALL_FOOTPRINT

/** Set the core_cfg_from_pcs flag in the core configuration.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] core_cfg_from_pcs The value to set.
 * @return Error Code generated if uC does not become active (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_set_core_config_from_pcs(srds_access_t *sa__, uint8_t core_cfg_from_pcs);

/** Set the osr_5_en flag in the core configuration.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] osr_5_en The value to set.
 * @return Error Code generated if uC does not become active (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_set_osr_5_en(srds_access_t *sa__, uint8_t osr_5_en);

/** Configure the lane address mapping.
 *  Note: Micro and core data path must be in reset when this function is called.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] num_lanes The number of entries in tx_lane_map and rx_lane_map.
 *                  This must match the number of lanes associated with the core.
 * @param[in] tx_lane_map A num_lanes-sized array of lane indexes to map to the TX lanes.
 *                     Each entry must be from 0 to num_lanes-1, and each entry must be unique.
 * @param[in] rx_lane_map A num_lanes-sized array of lane indexes to map to the RX lanes.
 *                     Each entry must be from 0 to num_lanes-1, and each entry must be unique.
 *                     If independent TX / RX lane mapping is not enabled for a core, then this must match tx_lane_map.
 *
 * @b Example (2 lane core)
 * @code

    uint8_t const tx_lane_map[2] = {1,0};
    uint8_t const rx_lane_map[2] = {1,0};
    EFUN(wrc_micro_core_rstb(0));
    EFUN(plp_aperta2_peregrine5_pc_core_dp_reset(sa__, 1));
    EFUN(plp_aperta2_peregrine5_pc_map_lanes(sa__, 2, tx_lane_map, rx_lane_map));
 * @endcode
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_map_lanes(srds_access_t *sa__, const uint8_t num_lanes, uint8_t const *tx_lane_map, uint8_t const *rx_lane_map);
/** Gets the current lane address mapping.
 *
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] num_lanes The size of tx_lane_map and rx_lane_map, which must also match the
 *                   number of lanes associated with the core.
 * @param[out] tx_lane_map A num_lanes-sized array. This array will get populated with logical TX lane indexes.
 *                     Each returned index should be from 0 to num_lanes-1, and should be unique.
 * @param[out] rx_lane_map A num_lanes-sized array. This array will get populated with logical RX lane indexes.
 *                     Each returned index should be from 0 to num_lanes-1, and should be unique.
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_get_lane_map(srds_access_t *sa__, const uint8_t num_lanes, uint8_t *tx_lane_map, uint8_t *rx_lane_map);

/** Reconfigures the lane address mapping.
 *
 *  Reads the current lane mapping from hardware and compares it with the user provided lane mapping. Only lanes which 
 *  differ are required to be put in lane datapath reset.
 *  Note: Before this function is called, TX and/or RX lane data path(s) must be in reset for lanes which are being remapped.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] num_lanes The number of entries in tx_lane_map and rx_lane_map.
 *                  This must match the number of lanes associated with the core.
 * @param[in] tx_lane_map A num_lanes-sized array of lane indexes to map to the TX lanes.
 *                     Each entry must be from 0 to num_lanes-1, and each entry must be unique.
 * @param[in] rx_lane_map A num_lanes-sized array of lane indexes to map to the RX lanes.
 *                     Each entry must be from 0 to num_lanes-1, and each entry must be unique.
 *                     If independent TX / RX lane mapping is not enabled for a core, then this must match tx_lane_map.
 *
 * @b Example Update/swap mapping of 2 lanes (4 lane core)
 * @code

    uint8_t tx_lane_map[4];
    uint8_t rx_lane_map[4];
    uint8_t temp;

    EFUN(plp_aperta2_peregrine5_pc_get_lane_map(sa__, 4, tx_lane_map, rx_lane_map));
    temp = tx_lane_map[0];
    tx_lane_map[0] = tx_lane_map[1];
    tx_lane_map[1] = temp;

    temp = rx_lane_map[0];
    rx_lane_map[0] = rx_lane_map[1];
    rx_lane_map[1] = temp;

    EFUN(plp_aperta2_peregrine5_pc_set_lane(sa__, 0)); EFUN(plp_aperta2_peregrine5_pc_ln_dp_reset(sa__, 1));
    EFUN(plp_aperta2_peregrine5_pc_set_lane(sa__, 1)); EFUN(plp_aperta2_peregrine5_pc_ln_dp_reset(sa__, 1));
    EFUN(plp_aperta2_peregrine5_pc_remap_lanes(sa__, 4, tx_lane_map, rx_lane_map));
 * @endcode
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_remap_lanes(srds_access_t *sa__, const uint8_t num_lanes, uint8_t const *tx_lane_map, uint8_t const *rx_lane_map);
#endif /* SMALL_FOOTPRINT */

/*-----------------*/
/*  Configure PLL  */
/*-----------------*/

/** The pll can be configured by providing all four parameters:  refclk frequency, divider value, VCO output frequency and the PLL options.
 *  This is the only option in SMALL_FOOTPRINT
 */
#define peregrine5_pc_configure_pll_refclk_div_vco plp_aperta2_peregrine5_pc_INTERNAL_configure_pll

#ifndef SMALL_FOOTPRINT
/** PLL Powerdown.
 *
 * Powers down the selected PLL. To power up the PLL, reconfigure the PLL by using
 * one of the "peregrine5_pc_configure_pll_" APIs.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_configure_pll_powerdown(srds_access_t *sa__);

/** Configure PLL.
 *
 * Use core_dp_s_rstb to re-initialize the PLL to its default configuration before calling this function.
 * For single PLL SerDes, use core_s_rstb instead of core_dp_s_rstb to also re-initialize all registers to default.
 *
 * Configures PLL registers to obtain the required configuration.
 * PLL configuration for this function is based on refclk frequency and divider value.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] refclk Reference clock frequency (enumerated).
 * @param[in] srds_div Divider value (enumerated).
 *
 * @b Example (refclk = 156.25 MHz, Divider = 170, VCO output frequency = 26.5625 GHz)
 * @code
    EFUN(plp_aperta2_peregrine5_pc_core_dp_reset(sa__, 1));
    EFUN(plp_aperta2_peregrine5_pc_configure_pll_refclk_div(sa__, PEREGRINE5_PC_PLL_REFCLK_156P25MHZ, PEREGRINE5_PC_PLL_DIV_170));
    EFUN(plp_aperta2_peregrine5_pc_core_dp_reset(sa__, 0));
 * @endcode
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_configure_pll_refclk_div(srds_access_t *sa__,
                                           enum peregrine5_pc_pll_refclk_enum refclk,
                                           enum peregrine5_pc_pll_div_enum srds_div);

/** Configure PLL.
 *
 * Use core_dp_s_rstb to re-initialize the PLL to its default configuration before calling this function.
 * For single PLL SerDes, use core_s_rstb instead of core_dp_s_rstb to also re-initialize all registers to default.
 *
 * Configures PLL registers to obtain the required configuration.
 * PLL configuration for this function is based on refclk frequency and VCO output frequency.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] refclk Reference clock frequency (enumerated).
 * @param[in] vco_freq_khz VCO output frequency, in kHz.
 *
 * @b Example (VCO output frequency = 25 GHz, refclk = 156.25 MHz, Divider = 160)
 * @code
    EFUN(plp_aperta2_peregrine5_pc_core_dp_reset(sa__, 1));
    EFUN(plp_aperta2_peregrine5_pc_configure_pll_refclk_vco(sa__, PEREGRINE5_PC_PLL_REFCLK_156P25MHZ, 25000000));
    EFUN(plp_aperta2_peregrine5_pc_core_dp_reset(sa__, 0));
 * @endcode
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_configure_pll_refclk_vco(srds_access_t *sa__,
                                           enum peregrine5_pc_pll_refclk_enum refclk,
                                           uint32_t vco_freq_khz);

/** Configure PLL.
 *
 * Use core_dp_s_rstb to re-initialize the PLL to its default configuration before calling this function.
 * For single PLL SerDes, use core_s_rstb instead of core_dp_s_rstb to also re-initialize all registers to default.
 *
 * Configures PLL registers to obtain the required configuration.
 * PLL configuration for this function is based on divider value and VCO output frequency.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] srds_div Divider value (enumerated).
 * @param[in] vco_freq_khz VCO output frequency, in kHz.
 *
 * @b Example (VCO output frequency = 25 GHz, Divider = 170, refclk = 147.05 MHz)
 * @code
    EFUN(plp_aperta2_peregrine5_pc_core_dp_reset(sa__, 1));
    EFUN(plp_aperta2_peregrine5_pc_configure_pll_refclk_vco(sa__, PEREGRINE5_PC_PLL_DIV_170, 25000000));
    EFUN(plp_aperta2_peregrine5_pc_core_dp_reset(sa__, 0));
 * @endcode
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_configure_pll_div_vco(srds_access_t *sa__,
                                        enum peregrine5_pc_pll_div_enum srds_div,
                                        uint32_t vco_freq_khz);

/** Configure PLL.
 *
 * Use core_dp_s_rstb to re-initialize the PLL to its default configuration before calling this function.
 * For single PLL SerDes, use core_s_rstb instead of core_dp_s_rstb to also re-initialize all registers to default.
 *
 * Configures PLL registers to obtain the required configuration.
 * PLL configuration for this fucntion is based on input refclk frequency and divider value.
 * but first divides the input refclk by 2 and adjusts parameters accordingly.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] refclk Input Reference clock frequency before refclk divide by 2  (enumerated).
 * @param[in] srds_div Divider value based on input reference clock frequency (enumerated).
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_configure_pll_refclk_div_div2refclk(srds_access_t *sa__,
                                           enum peregrine5_pc_pll_refclk_enum refclk,
                                           enum peregrine5_pc_pll_div_enum srds_div);

/** Configure PLL.
 *
 * Use core_dp_s_rstb to re-initialize the PLL to its default configuration before calling this function.
 * For single PLL SerDes, use core_s_rstb instead of core_dp_s_rstb to also re-initialize all registers to default.
 *
 * Configures PLL registers to obtain the required configuration.
 * PLL configuration for this function is based on input refclk frequency and VCO output frequency.
 * but first divides the input refclk by 2 and adjusts parameters accordingly.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] refclk Input Reference clock frequency before refclk divide by 2 (enumerated).
 * @param[in] vco_freq_khz VCO output frequency, in kHz.
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_configure_pll_refclk_vco_div2refclk(srds_access_t *sa__,
                                           enum peregrine5_pc_pll_refclk_enum refclk,
                                           uint32_t vco_freq_khz);

/** Configure PLL.
 *
 * Use core_dp_s_rstb to re-initialize the PLL to its default configuration before calling this function.
 * For single PLL SerDes, use core_s_rstb instead of core_dp_s_rstb to also re-initialize all registers to default.
 *
 * Configures PLL registers to obtain the required configuration.
 * PLL configuration for this function is based on divider value and VCO output frequency.
 * but first divides the input refclk by 2 and adjusts parameters accordingly.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] srds_div Divider value based on input refclk frequency before refclk divide by 2 (enumerated).
 * @param[in] vco_freq_khz VCO output frequency, in kHz.
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_configure_pll_div_vco_div2refclk(srds_access_t *sa__,
                                        enum peregrine5_pc_pll_div_enum srds_div,
                                        uint32_t vco_freq_khz);
/** Configure PLL.
 *
 * Use core_dp_s_rstb to re-initialize the PLL to its default configuration before calling this function.
 * For single PLL SerDes, use core_s_rstb instead of core_dp_s_rstb to also re-initialize all registers to default.
 *
 * Configures PLL registers to obtain the required configuration.
 * PLL configuration for this fucntion is based on input refclk frequency and divider value.
 * but first divides the input refclk by 4 and adjusts parameters accordingly.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] refclk Input Reference clock frequency before refclk divide by 4  (enumerated).
 * @param[in] srds_div Divider value based on input reference clock frequency (enumerated).
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_configure_pll_refclk_div_div4refclk(srds_access_t *sa__,
                                           enum peregrine5_pc_pll_refclk_enum refclk,
                                           enum peregrine5_pc_pll_div_enum srds_div);

/** Configure PLL.
 *
 * Use core_dp_s_rstb to re-initialize the PLL to its default configuration before calling this function.
 * For single PLL SerDes, use core_s_rstb instead of core_dp_s_rstb to also re-initialize all registers to default.
 *
 * Configures PLL registers to obtain the required configuration.
 * PLL configuration for this function is based on input refclk frequency and VCO output frequency.
 * but first divides the input refclk by 4 and adjusts parameters accordingly.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] refclk Input Reference clock frequency before refclk divide by 4 (enumerated).
 * @param[in] vco_freq_khz VCO output frequency, in kHz.
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_configure_pll_refclk_vco_div4refclk(srds_access_t *sa__,
                                           enum peregrine5_pc_pll_refclk_enum refclk,
                                           uint32_t vco_freq_khz);

/** Configure PLL.
 *
 * Use core_dp_s_rstb to re-initialize the PLL to its default configuration before calling this function.
 * For single PLL SerDes, use core_s_rstb instead of core_dp_s_rstb to also re-initialize all registers to default.
 *
 * Configures PLL registers to obtain the required configuration.
 * PLL configuration for this function is based on divider value and VCO output frequency.
 * but first divides the input refclk by 4 and adjusts parameters accordingly.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] srds_div Divider value based on input refclk frequency before refclk divide by 4 (enumerated).
 * @param[in] vco_freq_khz VCO output frequency, in kHz.
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_configure_pll_div_vco_div4refclk(srds_access_t *sa__,
                                        enum peregrine5_pc_pll_div_enum srds_div,
                                        uint32_t vco_freq_khz);


/** Get the VCO frequency in kHz, based on the reference clock frequency and divider value.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] refclk_freq_hz Reference clock frequency, in Hz.
 * @param[in] srds_div Divider value, in the same encoding as enum #peregrine5_pc_pll_div_enum.
 * @param[out] vco_freq_khz VCO output frequency, in kHz, obtained based on reference clock frequency and dic value.
 * @param[in] pll_option Select PLL configuration option from enum #peregrine5_pc_pll_option_enum.
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_get_vco_from_refclk_div(srds_access_t *sa__, uint32_t refclk_freq_hz, enum peregrine5_pc_pll_div_enum srds_div, uint32_t *vco_freq_khz, enum peregrine5_pc_pll_option_enum pll_option);
#endif /* SMALL_FOOTPRINT */

/**************************************************/
/* LANE Based APIs - Required to be used per Lane */
/**************************************************/

/*------------------------------------------------------------*/
/*  APIs to Write Lane Config and User variables into uC RAM  */
/*------------------------------------------------------------*/
/** Write to lane_config uC RAM variable.
 * Note: This API should be used only during configuration under dp_reset (plp_aperta2_peregrine5_pc_ln_dp_reset()).
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] struct_val Value to be written into lane_config RAM variable.
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_set_uc_lane_cfg(srds_access_t *sa__, struct peregrine5_pc_uc_lane_config_st struct_val);

/** Get force OS/BR/ER CDR mode
 * Note: ER-CDR Mode does not apply to 7nm Serdes cores
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[out] mode An enable/disable option from the enum peregrine5_pc_force_cdr_mode_enum.
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_get_force_lane_cdr_mode(srds_access_t *sa__, enum peregrine5_pc_force_cdr_mode_enum *mode);

/** Enable or disable force OS/BR/ER CDR mode
 * Note: This API should be used only during configuration under dp reset (plp_aperta2_peregrine5_pc_ln_dp_reset()).
 *       When enabling one mode, this API will disable the other mode.
 * Note: ER-CDR Mode does not apply to 7nm Serdes cores
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] mode An enable/disable option from the enum peregrine5_pc_force_cdr_mode_enum.
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_force_lane_cdr_mode(srds_access_t *sa__, enum peregrine5_pc_force_cdr_mode_enum mode);

#ifndef SMALL_FOOTPRINT
/*-----------------------------------------------------------*/
/*  APIs to Read Lane Config and User variables from uC RAM  */
/*-----------------------------------------------------------*/
/** Read value of lane_config uC RAM variable.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[out] struct_val Value read from lane_config RAM variable.
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_get_uc_lane_cfg(srds_access_t *sa__, struct peregrine5_pc_uc_lane_config_st *struct_val);
#endif /* SMALL_FOOTPRINT */

/*--------------------------------------------*/
/*  APIs to Reset Lane to Default             */
/*--------------------------------------------*/

/** Resets TX Lane to Default Configuration.
 * Toggles TX ln_s_rstb and leaves the lane TX in datapath reset.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_reset_tx_lane_to_default(srds_access_t *sa__);

/** Resets RX Lane to Default Configuration.
 * Toggles RX ln_s_rstb and leaves the lane RX in datapath reset.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_reset_rx_lane_to_default(srds_access_t *sa__);

/** Resets Lane to Default Configuration.
 * Toggles ln_s_rstb and leaves the lane in datapath reset.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_reset_lane_to_default(srds_access_t *sa__);

/*--------------------------------------------*/
/*  APIs to Enable or Disable datapath reset  */
/*--------------------------------------------*/

/** Enable or Disable TX datapath reset.
 * Asserts handshake signals upon disable.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] enable Enable/Disable TX datapath reset (1 = Enable; 0 = Disable).
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_tx_dp_reset(srds_access_t *sa__, uint8_t enable);

/** Enable or Disable RX datapath reset.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] enable Enable/Disable RX datapath reset (1 = Enable; 0 = Disable).
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_rx_dp_reset(srds_access_t *sa__, uint8_t enable);

/** Enable or Disable Core datapath reset.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] enable Enable/Disable Core datapath reset (1 = Enable; 0 = Disable).
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_core_dp_reset(srds_access_t *sa__, uint8_t enable);

/** Enable or Disable Lane datapath reset.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] enable Enable/Disable Lane datapath reset (1 = Enable; 0 = Disable).
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_ln_dp_reset(srds_access_t *sa__, uint8_t enable);

/*-----------------------------------------*/
/*  APIs for setting and getting OSR mode  */
/*-----------------------------------------*/

/** Configure if the TX and RX osr mode pins should only be used to configure the TX and RX osr modes.
 * Note: Calling this API will overwrite any OSR mode value previously programmed through plp_aperta2_peregrine5_pc_set_osr_mode(),
 *       plp_aperta2_peregrine5_pc_set_tx_osr_mode(), or plp_aperta2_peregrine5_pc_set_rx_osr_mode().
 * Note: This API requires accessing microcode RAM variables.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] use_pins_only Allow the use of TX and RX osr mode pins (1 = Yes; 0 = No).
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_set_use_osr_mode_pins_only(srds_access_t *sa__, uint8_t use_pins_only);

/** Get the TX and RX osr mode pins only status.
 * Note: This API requires accessing microcode RAM variables.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[out] tx_use_pins_only Returns whether the TX OSR mode pins only should be used (1 = Yes; 0 = No).
 * @param[out] rx_use_pins_only Returns whether the RX OSR mode pins only should be used (1 = Yes; 0 = No).
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_get_use_osr_mode_pins_only(srds_access_t *sa__, uint8_t *tx_use_pins_only, uint8_t *rx_use_pins_only);

/** Configure if the RX osr mode pins should only be used to configure the RX osr mode.
 * Note: Calling this API will overwrite any OSR mode value previously programmed through plp_aperta2_peregrine5_pc_set_osr_mode()
 *        or plp_aperta2_peregrine5_pc_set_rx_osr_mode().
 * Note: This API requires accessing microcode RAM variables.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] rx_use_pins_only Allow the use of RX osr mode pins (1 = Yes; 0 = No).
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_set_use_rx_osr_mode_pins_only(srds_access_t *sa__, uint8_t rx_use_pins_only);

/** Get the RX osr mode pins only status.
 * Note: This API requires accessing microcode RAM variables.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[out] rx_use_pins_only Returns whether the RX OSR mode pins only should be used (1 = Yes; 0 = No).
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_get_use_rx_osr_mode_pins_only(srds_access_t *sa__, uint8_t *rx_use_pins_only);

/** Configure if the RX osr mode pins should only be used to configure the TX osr mode.
 * Note: Calling this API will overwrite any OSR mode value previously programmed through plp_aperta2_peregrine5_pc_set_osr_mode()
 *        or plp_aperta2_peregrine5_pc_set_tx_osr_mode().
 * Note: This API requires accessing microcode RAM variables.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] tx_use_pins_only Allow the use of TX osr mode pins (1 = Yes; 0 = No).
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_set_use_tx_osr_mode_pins_only(srds_access_t *sa__, uint8_t tx_use_pins_only);

/** Get the TX osr mode pins only status.
 * Note: This API requires accessing microcode RAM variables.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[out] tx_use_pins_only Returns whether the TX OSR mode pins only should be used (1 = Yes; 0 = No).
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_get_use_tx_osr_mode_pins_only(srds_access_t *sa__, uint8_t *tx_use_pins_only);

/** Set both the TX and RX osr mode.
 * Note: Calling this API will overwrite the plp_aperta2_peregrine5_pc_set_use_osr_mode_pins_only() setting if it was previously set.
 * Note: This API should be used for configuring TX and RX osr modes when pins aren't being used. Do not write directly to the following
 *       register fields: rx_osr_mode_frc, rx_osr_mode_frc_val, tx_osr_mode_frc, tx_osr_mode_frc_val, osr_mode_frc, and osr_mode_frc_val.
 * Note: This API requires accessing microcode RAM variables.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] osr_mode The desired osr mode value that will be programmed.
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_set_osr_mode(srds_access_t *sa__, enum peregrine5_pc_osr_mode_enum osr_mode);

/** Get both the TX and RX osr mode.
 * Note: A return value of 0x3f is uninitialized.
 * Note: This API returns the control status, not the actual hardware status. Use plp_aperta2_peregrine5_pc_INTERNAL_get_osr_mode()
 *       if the actual hardware status is needed.
 * Note: This API requires accessing microcode RAM variables. If using pin only mode, then the osr_mode_pin
 *       status register can be directly read from rd_osr_mode_pin() instead of using this API.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[out] tx_osr_mode The returned TX osr mode.
 * @param[out] rx_osr_mode The returned RX osr mode.
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_get_osr_mode(srds_access_t *sa__, enum peregrine5_pc_osr_mode_enum *tx_osr_mode, enum peregrine5_pc_osr_mode_enum *rx_osr_mode);

/** Set the RX osr mode.
 * Note: Calling this API will overwrite the plp_aperta2_peregrine5_pc_set_use_rx_osr_mode_pins_only() setting if it was previously set.
 * Note: This API should be used for configuring TX and RX osr modes when pins aren't being used. Do not write directly to the following
 *       register fields: rx_osr_mode_frc, rx_osr_mode_frc_val, tx_osr_mode_frc, tx_osr_mode_frc_val, osr_mode_frc, and osr_mode_frc_val.
 * Note: This API requires accessing microcode RAM variables.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] rx_osr_mode The desired RX osr mode value that will be programmed.
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_set_rx_osr_mode(srds_access_t *sa__, enum peregrine5_pc_osr_mode_enum rx_osr_mode);

/** Get the RX osr mode.
 * Note: A return value of 0x3f is uninitialized.
 * Note: This API returns the control status, not the actual hardware status. Use plp_aperta2_peregrine5_pc_INTERNAL_get_osr_mode()
 *       if the actual hardware status is needed.
 * Note: This API requires accessing microcode RAM variables. If using pin only mode, then the osr_mode_pin
 *       status register can be directly read from rd_rx_osr_mode_pin() instead of using this API.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[out] rx_osr_mode The returned RX osr mode.
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_get_rx_osr_mode(srds_access_t *sa__, enum peregrine5_pc_osr_mode_enum *rx_osr_mode);

/** Set the TX osr mode.
 * Note: Calling this API will overwrite the plp_aperta2_peregrine5_pc_set_use_tx_osr_mode_pins_only() setting if it was previously set.
 * Note: This API should be used for configuring TX and RX osr modes when pins aren't being used. Do not write directly to the following
 *       register fields: rx_osr_mode_frc, rx_osr_mode_frc_val, tx_osr_mode_frc, tx_osr_mode_frc_val, osr_mode_frc, and osr_mode_frc_val.
 * Note: This API requires accessing microcode RAM variables.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] tx_osr_mode The desired TX osr mode value that will be programmed.
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_set_tx_osr_mode(srds_access_t *sa__, enum peregrine5_pc_osr_mode_enum tx_osr_mode);

/** Get the TX osr mode.
 * Note: A return value of 0x3f is uninitialized.
 * Note: This API returns the control status, not the actual hardware status. Use plp_aperta2_peregrine5_pc_INTERNAL_get_osr_mode()
 *       if the actual hardware status is needed.
 * Note: This API requires accessing microcode RAM variables. If using pin only mode, then the osr_mode_pin
 *       status register can be directly read from rd_tx_osr_mode_pin() instead of using this API.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[out] tx_osr_mode The returned TX osr mode.
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_get_tx_osr_mode(srds_access_t *sa__, enum peregrine5_pc_osr_mode_enum *tx_osr_mode);


/*---------------------------*/
/* TX Analog APIs            */
/*---------------------------*/
#ifndef SMALL_FOOTPRINT
/** Write TX AFE parameters.
 * NOTE: This API should be used only after lane_dp_reset is released.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] param selects the parameter to write based on #peregrine5_pc_tx_afe_settings_enum.
 * @param[in] val is the signed input value to the parameter.
 * @return Error Code generated by invalid tap settings (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_write_tx_afe(srds_access_t *sa__, enum peregrine5_pc_tx_afe_settings_enum param, int16_t val);
#endif /* SMALL_FOOTPRINT */

/** Read TX AFE parameters.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] param selects the parameter to read based on #peregrine5_pc_tx_afe_settings_enum.
 * @param[out] val is the returned signed value of the parameter.
 * @return Error Code generated by invalid tap settings (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_read_tx_afe(srds_access_t *sa__, enum peregrine5_pc_tx_afe_settings_enum param, int16_t *val);

/** Validates TXFIR tap settings.
 * Returns failcodes if TXFIR settings are invalid.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] enable_taps Enable TXFIR TAPs based on #peregrine5_pc_txfir_tap_enable_enum.
 * @param[in] pre3    TXFIR pre3  tap value (-8 to 0).
 * @param[in] pre2    TXFIR pre2  tap value (0 to +16 when in PAM4 mode, otherwise 0 to +8).
 * @param[in] pre1    TXFIR pre1  tap value (-40 to 0 when in PAM4 mode, otherwise -30 to 0).
 * @param[in] main    TXFIR main  tap value (0 to +168 when in PAM4 mode, otherwise +1 to +127).
 * @param[in] post1   TXFIR post  tap value (-64 to 0 when in PAM4 mode, otherwise -62 to 0).
 * @param[in] post2   TXFIR post2 tap value (-16 to +16).
 * @return Error Code generated by invalid tap settings (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_validate_txfir_cfg(srds_access_t *sa__, enum peregrine5_pc_txfir_tap_enable_enum enable_taps, int16_t pre3, int16_t pre2, int16_t pre1, int16_t main, int16_t post1, int16_t post2);

/** Writes Serdes TXFIR tap settings.
 * Returns failcodes if TXFIR settings are invalid.
 * NOTE: Dynamically switching between enable_6tap and !enable_6tap will cause a 1UI slip in TX.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] enable_taps Enable TXFIR TAPs based on #peregrine5_pc_txfir_tap_enable_enum.
 * @param[in] pre3    TXFIR pre3  tap value (-8 to 0).
 * @param[in] pre2    TXFIR pre2  tap value (0 to +16 when in PAM4 mode, otherwise 0 to +8).
 * @param[in] pre1    TXFIR pre1  tap value (-40 to 0 when in PAM4 mode, otherwise -30 to 0).
 * @param[in] main    TXFIR main  tap value (0 to +168 when in PAM4 mode, otherwise +1 to +127).
 * @param[in] post1   TXFIR post  tap value (-64 to 0 when in PAM4 mode, otherwise -62 to 0).
 * @param[in] post2   TXFIR post2 tap value (-16 to +16).
 * @return Error Code generated by invalid tap settings (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_apply_txfir_cfg(srds_access_t *sa__, enum peregrine5_pc_txfir_tap_enable_enum enable_taps, int16_t pre3, int16_t pre2, int16_t pre1, int16_t main, int16_t post1, int16_t post2);

/** Validates TXFIR tap settings with nonlinear compensation.
 * Returns failcodes if TXFIR settings are invalid.
 * NOTE: This API only supports PAM4 6 tap mode.
 * NOTE: Users are expected to adjust the TXFIR tap values themselves based on  nlc_upper_eye_pct and nlc_lower_eye_pct:
 *       nlc_tap_value = original_tap_value * (3 / (3 + (nlc_upper_eye_pct + nlc_lower_eye_pct) / 100))
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] enable_taps Enable TXFIR TAPs based on #peregrine5_pc_txfir_tap_enable_enum.
 * @param[in] nlc_upper_eye_pct Upper pam4 eye nonlinear compensation percentage (-100 to +100) relative to the inner eye
 * @param[in] nlc_lower_eye_pct Lower pam4 eye nonlinear compensation percentage (-100 to +100) relative to the inner eye
 * @param[in] pre3    TXFIR pre3  tap value
 * @param[in] pre2    TXFIR pre2  tap value
 * @param[in] pre1    TXFIR pre1  tap value
 * @param[in] main    TXFIR main  tap value
 * @param[in] post1   TXFIR post  tap value
 * @param[in] post2   TXFIR post2 tap value
 * @return Error Code generated by invalid tap settings (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_validate_txfir_cfg_with_nlc(srds_access_t *sa__, enum peregrine5_pc_txfir_tap_enable_enum enable_taps, int8_t nlc_upper_eye_pct, int8_t nlc_lower_eye_pct, int16_t pre3, int16_t pre2, int16_t pre1, int16_t main, int16_t post1, int16_t post2);

/** Writes Serdes TXFIR tap settings with nonlinear compensation.
 * Returns failcodes if TXFIR settings are invalid.
 * NOTE: This API only supports PAM4 6 tap mode.
 * NOTE: Users are expected to adjust the TXFIR tap values themselves based on nlc_pct:
 *       new_tap_value = old_tap_value * (3 / (3 + 2 * nlc_pct / 100))
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] enable_taps Enable TXFIR TAPs based on #peregrine5_pc_txfir_tap_enable_enum. Only PEREGRINE5_PC_PAM4_6TAP is supported.
 * @param[in] nlc_pct nonlinear compensation percentage (-100 to +100)
 * @param[in] pre3    TXFIR pre3  tap value
 * @param[in] pre2    TXFIR pre2  tap value
 * @param[in] pre1    TXFIR pre1  tap value
 * @param[in] main    TXFIR main  tap value
 * @param[in] post1   TXFIR post  tap value
 * @param[in] post2   TXFIR post2 tap value
 * @return Error Code generated by invalid tap settings (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_apply_txfir_cfg_with_nlc(srds_access_t *sa__, enum peregrine5_pc_txfir_tap_enable_enum enable_taps, int8_t nlc_pct, int16_t pre3, int16_t pre2, int16_t pre1, int16_t main, int16_t post1, int16_t post2);
/** Writes Serdes Asymmetric TXFIR tap settings with nonlinear compensation.
 * Returns failcodes if TXFIR settings are invalid.
 * NOTE: This API only supports PAM4 6 tap mode.
 * NOTE: Users are expected to adjust the TXFIR tap values themselves based on  nlc_upper_eye_pct and nlc_lower_eye_pct:
 *       nlc_tap_value = original_tap_value * (3 / (3 + (nlc_upper_eye_pct + nlc_lower_eye_pct) / 100))
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] enable_taps Enable TXFIR TAPs based on #peregrine5_pc_txfir_tap_enable_enum. Only PEREGRINE5_PC_PAM4_6TAP is supported.
 * @param[in] nlc_upper_eye_pct Upper pam4 eye nonlinear compensation percentage (-100 to +100) relative to the inner eye
 * @param[in] nlc_lower_eye_pct Lower pam4 eye nonlinear compensation percentage (-100 to +100) relative to the inner eye
 * @param[in] pre3    TXFIR pre3  tap value
 * @param[in] pre2    TXFIR pre2  tap value
 * @param[in] pre1    TXFIR pre1  tap value
 * @param[in] main    TXFIR main  tap value
 * @param[in] post1   TXFIR post  tap value
 * @param[in] post2   TXFIR post2 tap value
 * @return Error Code generated by invalid tap settings (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_apply_txfir_cfg_with_asymm_nlc(srds_access_t *sa__, enum peregrine5_pc_txfir_tap_enable_enum enable_taps, int8_t nlc_upper_eye_pct, int8_t nlc_lower_eye_pct, int16_t pre3, int16_t pre2, int16_t pre1, int16_t main, int16_t post1, int16_t post2);

/** Gets TxFIR nonlinear compensation percentage.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[out] nlc_pct nonlinear compensation percentage (-100 to +100)
 * @return Error Code generated by invalid tap settings (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_get_tx_nlc_pct(srds_access_t *sa__, int8_t *nlc_pct);
/** Gets TxFIR asymmetric nonlinear compensation percentages.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] nlc_upper_eye_pct Upper pam4 eye nonlinear compensation percentage (-100 to +100) relative to the inner eye
 * @param[in] nlc_lower_eye_pct Lower pam4 eye nonlinear compensation percentage (-100 to +100) relative to the inner eye
 * @return Error Code generated by invalid tap settings (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_get_tx_asymm_nlc_pct(srds_access_t *sa__, int8_t *nlc_upper_eye_pct, int8_t *nlc_lower_eye_pct);


/*----------------*/
/*   PMD_RX_LOCK  */
/*----------------*/

/** PMD rx lock status of current lane.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[out] *pmd_rx_lock PMD_RX_LOCK status of current lane returned by API.
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_pmd_lock_status(srds_access_t *sa__, uint8_t *pmd_rx_lock);

/*--------------------------------*/
/*  Serdes TX disable/RX Restart  */
/*--------------------------------*/
/** TX Disable.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] enable Enable/Disable TX disable (1 = TX Disable asserted; 0 = TX Disable removed).
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_tx_disable(srds_access_t *sa__, uint8_t enable);

/** Enable/disable Restart RX and hold.
 * (Reset DSC state machine into RESTART State and hold it till disabled)
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] enable Enable/Disable Restart RX and hold (1 = RX restart and hold; 0 = Release hold in restart state)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_rx_restart(srds_access_t *sa__, uint8_t enable);


/*----------------------*/
/*  Fast Link Recovery  */
/*----------------------*/

/** Configure Fast Link Recovery (FLR) for the lane.
 * This may only be performed while the lane is in reset.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] enable Enables the FLR feature.
 * @param[in] enable_timeout Enables timeout.
 * @param[in] timeout_in_ms The timeout time in milliseconds, only valid if enable_timeout is set. timeout_in_ms must be <= 511.
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_configure_fast_link_recovery(srds_access_t *sa__, uint8_t enable, uint8_t enable_timeout, uint32_t timeout_in_ms);

/** Get Fast Link Recovery (FLR) Configuration for the lane.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[out] enable Returns if FLR is enabled (1 = Enabled; 0 = Disabled).
 * @param[out] enable_timeout  Returns if timeouts are enabled (1 = Enabled; 0 = Disabled).
 * @param[out] timeout_in_ms The timeout time in milliseconds, only valid if enable_timeout is set. timeout_in_ms must be <= 511.
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_get_configure_fast_link_recovery(srds_access_t *sa__, uint8_t *enable, uint8_t *enable_timeout, uint32_t *timeout_in_ms);

/** Get Fast Link Recovery (FLR) ready status for the lane.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[out] ready Returns if FLR is ready to recover from short LOS events (1 = Ready; 0 = Not Ready).
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_get_flr_ready_status(srds_access_t *sa__, uint8_t *ready);


#ifndef SMALL_FOOTPRINT
/*-----------------------------*/
/*  Stop/Resume RX Adaptation  */
/*-----------------------------*/
/** Stop RX Adaptation on a Lane. Control is returned only after attempting to stop adaptation.
 * RX Adaptation needs to be stopped before modifying any of the VGA, PF or DFE taps.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] enable Enable RX Adaptation stop (1 = Stop RX Adaptation on lane; 0 = Resume RX Adaptation on lane)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_stop_rx_adaptation(srds_access_t *sa__, uint8_t enable);

/** Request to stop RX Adaptation on a Lane.
 * Control will be returned immediately before adaptaion is completely stopped.
 * RX Adaptation needs to be stopped before modifying any of the VGA, PF or DFE taps.
 * To resume RX adaptation, use the plp_aperta2_peregrine5_pc_stop_rx_adaptation() API.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_request_stop_rx_adaptation(srds_access_t *sa__);
#endif /* SMALL_FOOTPRINT */

#if !defined(SMALL_FOOTPRINT) 
/*------------------------------------*/
/*  Read/Write all RX AFE parameters  */
/*------------------------------------*/

/** Write to RX AFE settings.
 * Note: RX Adaptation needs to be stopped before modifying any of the VGA, PF or DFE taps.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] param Enum (#plp_aperta2_srds_rx_afe_settings_enum) to select the required RX AFE setting to be modified
 * @param[in] val Value to be written to the selected AFE setting
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_write_rx_afe(srds_access_t *sa__, enum plp_aperta2_srds_rx_afe_settings_enum param, int8_t val);
#endif /* !SMALL_FOOTPRINT */

#ifndef SMALL_FOOTPRINT
/** Read from RX AFE settings.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] param Enum (#plp_aperta2_srds_rx_afe_settings_enum) to select the required RX AFE setting to be read.
 * @param[out] val RX AFE value returned from API.
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors).
 */
err_code_t plp_aperta2_peregrine5_pc_read_rx_afe(srds_access_t *sa__, enum plp_aperta2_srds_rx_afe_settings_enum param, int8_t *val);
#endif /* SMALL_FOOTPRINT */

/*-----------------------------*/
/*  TX_PI Fixed Frequency Mode */
/*-----------------------------*/

/** TX_PI Fixed Frequency Mode.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] enable Enable/Disable TX_PI (1 = Enable; 0 = Disable)
 * @param[in] freq_override_val Fixed Frequency Override value (freq_override_val = desired_ppm*8192/781.25; Range: -8192 to + 8192);
 * @return Error Code generated by invalid TX_PI settings (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_tx_pi_freq_override(srds_access_t *sa__, uint8_t enable, int16_t freq_override_val);

#ifndef SMALL_FOOTPRINT
#endif /* SMALL_FOOTPRINT */

/*--------------------------------------------*/
/*  Loopback and Ultra-Low Latency Functions  */
/*--------------------------------------------*/

/** Locks TX_PI to Loop timing from any Rx.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] timing_src_lane timing source Rx lane
 * @param[in] enable Enable TX_PI lock to loop timing (1 = Enable Lock; 0 = Disable Lock)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_asymmetric_loop_timing(srds_access_t *sa__, uint8_t timing_src_lane, uint8_t enable);
/** Locks TX_PI to Loop timing.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] enable Enable TX_PI lock to loop timing (1 = Enable Lock; 0 = Disable Lock)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_loop_timing(srds_access_t *sa__, uint8_t enable);

/** Enable/Disable Remote Loopback.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in] enable Enable Remote Loopback (1 = Enable rmt_lpbk; 0 = Disable rmt_lpbk)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t plp_aperta2_peregrine5_pc_rmt_lpbk(srds_access_t *sa__, uint8_t enable);

/*! @} SerdesAPIConfigTag */
/*! @} APITag */
#endif
#ifdef __cplusplus
}
#endif
