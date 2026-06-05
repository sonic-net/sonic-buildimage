/***********************************************************************************
 *                                                                                 *
 * Copyright: (c) 2021 Broadcom.                                                   *
 * Broadcom Proprietary and Confidential. All rights reserved.                     *
 *                                                                                 *
 ***********************************************************************************/

/*****************************************************************************************
 *****************************************************************************************
 *                                                                                       *
 *  Revision      :   *
 *                                                                                       *
 *  Description   :  Common types used by Serdes API functions                           *
 *                                                                                       *
 *****************************************************************************************
 *****************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PEREGRINE5_PC_API_TYPES_H
#define PEREGRINE5_PC_API_TYPES_H

#include "peregrine5_pc_ipconfig.h"
#include "peregrine5_pc_common.h"
#include "peregrine5_pc_select_defns.h"

/*! @file
 *  @brief Common types used by Serdes API functions
 */

/*! @addtogroup APITag
 * @{
 */

/*! @defgroup SerdesAPITypesTag Common Core Types
 * Contains structs and typedefs which are used throughout
 * Serdes APIs.
 */

/*! @addtogroup SerdesAPITypesTag
 * @{
 */

/*! Used for error reporting APIs
 *
 */
typedef struct peregrine5_pc_triage_info_st {
   uint32_t     api_ver;
   uint32_t     ucode_ver;
   srds_core_t  core;
   uint8_t      lane;

   err_code_t   error;
   uint16_t     line;

   uint16_t     stop_status;
   uint16_t     sw_exception;
   uint16_t     hw_exception;
   uint16_t     stack_overflow;
   uint16_t     overflow_lane_id;
   uint16_t     cmd_info;

   uint8_t      pmd_lock;
   uint8_t      pmd_lock_chg;
   uint8_t      sig_det;
   uint8_t      sig_det_chg;
   uint16_t     dsc_one_hot[2];
} peregrine5_pc_triage_info;



/*! Used for debug feedback APIs
 *
 */
typedef struct peregrine5_pc_dbgfb_cfg_s {
    uint8_t y;
    uint8_t x;
    uint32_t time_in_us;
    int32_t data1;
    int32_t data2;
    int32_t data3;
} peregrine5_pc_dbgfb_cfg_st;

#define DBGFB_CFG_ST_INIT {0,0,0,0,0,0}

/*----------------------------------------*/
/*  Lane/Core structs (without bitfields) */
/*----------------------------------------*/

/*! Lane Config Variable Structure in Microcode
 *
 */
struct peregrine5_pc_uc_lane_config_field_st {
  uint8_t  lane_cfg_from_pcs ;
  uint8_t  an_enabled        ;
  uint8_t  dfe_on            ;
  uint8_t  rx_low_power      ;
  uint8_t  force_cdr_mode    ;
  uint8_t  media_type        ;
  uint8_t  unreliable_los    ;
  uint8_t  linktrn_auto_polarity_en ;
  uint8_t  linktrn_restart_timeout_en;
  uint8_t  force_er          ;
  uint8_t  force_nr          ;
  uint8_t  lp_has_prec_en    ;
  uint8_t  force_pam4_mode   ; /*!< Used to override the pam4mode pin */
  uint8_t  force_nrz_mode    ; /*!< Used to override the pam4mode pin */
};
#define UC_LANE_CONFIG_FIELD_INIT {0,0,0,0,0,0,0,0,0,0,0,0,0,0}

/*! Core Config Variable Structure in Microcode
 *
 */
struct peregrine5_pc_uc_core_config_field_st {
  uint8_t  vco_rate          ;
  uint8_t  core_cfg_from_pcs ;
  uint8_t  osr_5_en          ;
  uint8_t  reserved          ;
};
#define UC_CORE_CONFIG_FIELD_INIT {0,0,0,0}

/*! Lane Config Struct
 *
 */
struct  peregrine5_pc_uc_lane_config_st {
  struct peregrine5_pc_uc_lane_config_field_st field;
  uint16_t word;
};
#define UC_LANE_CONFIG_INIT {UC_LANE_CONFIG_FIELD_INIT, 0}

/*! Core Config Struct
 *
 */
struct  peregrine5_pc_uc_core_config_st {
  struct peregrine5_pc_uc_core_config_field_st field;
  uint16_t word;
  int32_t vco_rate_in_Mhz; /*!< if > 0 then will get converted and replace field.vco_rate when update is called */
};
#define UC_CORE_CONFIG_INIT {UC_CORE_CONFIG_FIELD_INIT, 0, 0}

/*! Lane User Control Function Structure in Microcode
 *
 */
struct peregrine5_pc_usr_ctrl_func_st {
  uint32_t all_adaptation           ; /*!< All Adaptation */ 
  uint32_t adc_hw_calibration       ; /*!< ADC HW Calibration */
  uint32_t adc_gain_calibration     ; /*!< ADC Gain Calibration */
  uint32_t adc_skew_calibration     ; /*!< ADC Skew Calibration */
  uint32_t c2d_coupling_calibration ; /*!< C2D Coupling Calibration */
  uint32_t pga_adaptation           ; /*!< PGA Adaptation */
  uint32_t pfhi_adaptation          ; /*!< High Frequency Peaking Filter Adaptation */
  uint32_t pfmid_adaptation         ; /*!< Mid Frequency Peaking Filter Adaptation */
  uint32_t pflow_adaptation         ; /*!< Low Frequency Peaking Filter Adaptation */
  uint32_t afebw_adaptation         ; /*!< AFE b/w Adaptation */
  uint32_t dc_adaptation            ; /*!< DC Offset Adaptation */
  uint32_t blw_adaptation           ; /*!< BLW Adaptation */
  uint32_t snr_estimation           ; /*!< SNR Estimation */
  uint32_t eye_margin_estimation    ; /*!< Eye Margin Estimation */
  uint32_t ffe_adaptation           ; /*!< FFE Adaptation */
  uint32_t ffe_offset_adaptation    ; /*!< FFE Offset Adaptation*/
  uint32_t rcfir_tap_adaptation     ; /*!< RC FIR Tap Adaptation */
  uint32_t dfe_tap_adaptation       ; /*!< DFE Tap Adaptation */
  uint32_t dfe_offset_adaptation    ; /*!< DFE Offset Adaptation */
  uint32_t channel_id_estimation    ; /*!< Channel ID Estimation */
  uint32_t freq_offset              ; /*!< Freq Offset */
  uint32_t generic_disable13        ; /*!< Generic Disable 13 */
};


/*! Lane User Control Disable Function Struct
 *
 */
struct peregrine5_pc_usr_ctrl_disable_functions_st {
  struct peregrine5_pc_usr_ctrl_func_st field;
  uint32_t dword;
};


/*! Used in BER APIs
 *
 */
struct peregrine5_pc_ber_data_st {
    uint64_t num_errs;
    uint64_t num_bits;
    uint8_t lcklost;
    uint8_t prbs_chk_en;
    uint8_t cdrlcklost;
    uint8_t prbs_lck_state;
};


/*! Used for checking platform-specific padding
 *
 */
struct peregrine5_pc_check_platform_info_st {
    char c;
    uint32_t d;
};

#define BER_DATA_ST_INIT {0, 0, 0, 0, 0, 0}

/*! @def NUM_ILV
 * The number of interleaves per lane
 */
#define NUM_ILV 8

#define DATA_ARR_SIZE (6)
#define PHASE_ARR_SIZE (3)

#define DFE_ARR_SIZE (38)



#ifndef SMALL_FOOTPRINT
/*! Used for maintaining correctness of SIZE_OF_EXTENDED_LANE_STATE
 *
 */

#endif /* !SMALL_FOOTPRINT */

/****************************************************************************
 * @name Diagnostic Sampling
 *
 ****************************************************************************/
/***/

#ifdef STANDALONE_EVENT
#define DIAG_MAX_SAMPLES (64)
#else
/**
 * Diagnostic sample set size, DSP variants.
 * Applies to collections of BER measurements, eye margins, etc.
 */
#define DIAG_MAX_SAMPLES (128)
#endif



/*! @} SerdesAPITypesTag */
/*! @} APITag */
#endif
#ifdef __cplusplus
}
#endif

