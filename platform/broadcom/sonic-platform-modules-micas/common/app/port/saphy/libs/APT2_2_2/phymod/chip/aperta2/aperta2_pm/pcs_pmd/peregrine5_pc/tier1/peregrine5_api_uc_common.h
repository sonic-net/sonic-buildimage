/*************************************************************************************
 *                                                                                   *
 * Copyright: (c) 2021 Broadcom.                                                     *
 * Broadcom Proprietary and Confidential. All rights reserved.                       *
 *                                                                                   *
 *************************************************************************************/


/** @file peregrine5_api_uc_common.h
 * Defines and Enumerations shared by Peregrine5 IP Specific API and Microcode
 */

#ifndef PEREGRINE5_API_UC_COMMON_H
#define PEREGRINE5_API_UC_COMMON_H

/* Add Peregrine5 specific items below this */

enum srds_lane_timer_enum {
  PEREGRINE5_PC_LANE_TIMER_UNKNOWN = 0,
  PEREGRINE5_PC_LANE_TIMER_SHIFT   = 5
};

/* Event Log Group Mask Default */
#define EVENT_GROUP_DEFAULT        0x02E0FFFF   /* bits 0..15, 21..23, 25 */
/* Event Log Group Masks that try to mimic older priority level settings. */
#define EVENT_GROUP_PRIORITY_0      0x00000000
#define EVENT_GROUP_PRIORITY_1      EVENT_GROUP_DEFAULT
#define EVENT_GROUP_PRIORITY_2     (0x00208000 | EVENT_GROUP_PRIORITY_1)
#define EVENT_GROUP_PRIORITY_3     (0x00020000 | EVENT_GROUP_PRIORITY_2)
#define EVENT_GROUP_PRIORITY_4     (0x00D80000 | EVENT_GROUP_PRIORITY_3)
#define EVENT_GROUP_PRIORITY_5      0x00FFFFFF

/* Please note that when adding entries here you should update the #defines in the peregrine5_pc_common.h */

/** DSC_STATES Enum **/
/** These state values must match the dsc_state bitfield in the register map. **/
/** If these values change, then consider changing the test functions below.  **/
enum plp_aperta2_srds_dsc_state_enum {
  DSC_STATE_UNKNOWN      = 255,
  DSC_STATE_RESET        = 0,
  DSC_STATE_RESTART      = 1,
  DSC_STATE_CONFIG       = 2,
  DSC_STATE_WAIT_FOR_SIG = 3,
  DSC_STATE_ACQ_CDR      = 4,
  DSC_STATE_CDR_SETTLE   = 5,
  DSC_STATE_HW_TUNE      = 6,
  DSC_STATE_UC_TUNE      = 7,
  DSC_STATE_MEASURE      = 8,
  DSC_STATE_DONE         = 9,
  DSC_STATE_FLR_LOS      = 17,
  DSC_STATE_FLR_CONFIG   = 18
};

/** Is the state between DSC_STATE_RESET and DSC_STATE_WAIT_FOR_SIG? **/
#define is_dsc_state_at_or_before_wait_for_sig(state) (state <= DSC_STATE_WAIT_FOR_SIG)


#define EYE_SCAN_MODE_BER_PROJ 1
#define EYE_SCAN_MODE_NOISE    2

/** Serdes eye scan struct */
struct peregrine5_pc_eye_scan_st {
    uint16_t eye_scan_mode      : 2;
    uint16_t step_size          : 4;
    uint16_t ber_proj_top0_bot1 : 1;
    uint16_t reserved           : 9;
};

/** Serdes eye scan config union */
union peregrine5_pc_eye_scan_config_un {
    struct  peregrine5_pc_eye_scan_st field;
    uint16_t word;
};


#define EYE_SCAN_NRZ_VERTICAL_IDX_MAX  (191)
#define EYE_SCAN_NRZ_VERTICAL_STEP       (2)
#define EYE_SCAN_PAM_VERTICAL_IDX_MAX  (191)
#define EYE_SCAN_PAM_VERTICAL_STEP       (2)

#define EYE_SCAN_NRZ_HORIZONTAL_WIDTH  (128)
#define EYE_SCAN_NRZ_HORIZONTAL_STEP     (2)
#define EYE_SCAN_PAM_HORIZONTAL_WIDTH   (64)
#define EYE_SCAN_PAM_HORIZONTAL_STEP     (1)

#define BER_PROJ_NRZ_VERTICAL_IDX_MAX  (127)
#define BER_PROJ_NRZ_VERTICAL_STEP       (2)
#define BER_PROJ_NRZ_HORIZONTAL_WIDTH   (64)
#define BER_PROJ_NRZ_HORIZONTAL_STEP     (2)

/** OSR_MODES Enum */
enum peregrine5_pc_osr_mode_enum {
  /* If the enumerations change, then consider updating OSR_MODE_SUPPORTS_EYE_TESTS(). */
  /*  NOTE:  If adding or removing ANY of these you MUST change the initializer below. */
  PEREGRINE5_PC_OSX1              = 0x0,
  PEREGRINE5_PC_OSX2              = 0x1,
  PEREGRINE5_PC_OSX4              = 0x2,
  PEREGRINE5_PC_OSX5              = 0x3,
  PEREGRINE5_PC_OSX33             = 0x11,
  PEREGRINE5_PC_OSX41P25          = 0x19,
  PEREGRINE5_PC_OSX42P5           = 0x21,
  PEREGRINE5_PC_OSR_UNINITIALIZED = 0x3f
};

/* UNIQUIFY_PUBLIC_END  - Marker used by API Uniquify script */

/*This is an initializer to sort the OSR modes in order so that EVAL can walk*/
/*through them proper and choose them in order.*/
#define INIT_OSR_ASSOC_TABLE                                                                                        \
        {{PEREGRINE5_PC_OSX1, 1000}, {PEREGRINE5_PC_OSX2, 2000}, {PEREGRINE5_PC_OSX4, 4000}, {PEREGRINE5_PC_OSX5, 5000},    \
         {PEREGRINE5_PC_OSX33, 33000}, {PEREGRINE5_PC_OSX41P25, 41250}, {PEREGRINE5_PC_OSX42P5, 42500}}

#define INIT_OSR_MODE_ENUM {PEREGRINE5_PC_OSX1, PEREGRINE5_PC_OSX2, PEREGRINE5_PC_OSX4, PEREGRINE5_PC_OSX5, \
                            PEREGRINE5_PC_OSX33, PEREGRINE5_PC_OSX41P25, PEREGRINE5_PC_OSX42P5, 0xff}

#define INIT_OSR_MODE_INT {1000, 2000, 4000, 5000, 33000, 41250, 42500}

/** CDR mode Enum **/
enum peregrine5_pc_cdr_mode_enum {
  PEREGRINE5_PC_CDR_MODE_AUTO = 0,
  PEREGRINE5_PC_CDR_MODE_OS   = 1,
  PEREGRINE5_PC_CDR_MODE_BR   = 2,
  PEREGRINE5_PC_CDR_MODE_ER   = 3
};

/** Lane User Control Clause93/72 Force Value **/
enum peregrine5_pc_cl93n72_frc_val_enum {
  PEREGRINE5_PC_CL93N72_FORCE_OS  = 0,
  PEREGRINE5_PC_CL93N72_FORCE_BR  = 1
};

/** AFE Override Slicer Selection Value
    Notation indicates functional slicer name **/
typedef enum {
  INVALID_SLICER = 0,
  DATA23_SLICER  = 1,
  DATA05_SLICER  = 3,
  PHASE1_SLICER  = 4,
  PHASE02_SLICER = 5,
  DATA14_SLICER  = 2,
  DFE_TAPS_2_3   = 6,
  LMS_SLICER     = 8
} afe_override_slicer_sel_t;

#if defined(INCLUDE_LIB_TEST) 
struct adc_data_st {
 int16_t    val          :8;
 int16_t    ilv          :4;
 int16_t    bank         :4;
 };

union adc_data_un  {
 struct adc_data_st  field;
 int16_t   word;
};

struct dfe_data_st {
 int16_t   val_e       :8;
 int16_t   slicer      :4;
 int16_t   dfe_mode    :2;
 int16_t   idx         :2;
};

union dfe_data_un  {
 struct dfe_data_st  field;
 int16_t   word;
};

struct ffe_data_st {
 int16_t val       :11;
 int16_t  idx      :4;
 int16_t bank      :1;
};

union ffe_data_un  {
 struct ffe_data_st  field;
 int16_t   word;
};

struct rcfir_data_st {
 int16_t tap_idx :5;
 int16_t tap_loc :8;
 int16_t dummy   :3;
};

union rcfir_data_un {
 struct rcfir_data_st field;
 int16_t word;
};
#endif

/* The following functions translate between a VCO frequency in MHz and the
 * vco_rate that is found in the Core Config Variable Structure using the
 * formula:
 *
 *     vco_rate = (frequency_in_ghz * 8.0) - 232.0
 *
 * The valid VCO ranges from 41GHz to 57GHz
 *
 * Both functions round to the nearest resulting value.  This
 * provides the highest accuracy possible, and ensures that:
 *
 *     vco_rate == MHZ_TO_VCO_RATE(VCO_RATE_TO_MHZ(vco_rate))
 *
 * In the microcode, these functions should only be called with a numeric
 * literal parameter.
 */
#define MHZ_TO_VCO_RATE(mhz) ((uint8_t)((((uint16_t)(mhz) + 62) / 125) - 232))
#define VCO_RATE_TO_MHZ(vco_rate) (((uint16_t)(vco_rate) + 232) * 125)

/* BOUNDARIES FOR FIR TAP VALUES
 *
 * Hardware limits the sum of the taps to be TXFIR_SUM_LIMIT or TXFIR_PAM4_SUM_LIMIT:
 *
 *     sum(n=0..11, abs(tap[n])) <= TXFIR_NRZ_SUM_LIMIT, if in NRZ mode
 *     sum(n=0..11, abs(tap[n])) <= TXFIR_PAM4_UC_SUM_LIMIT, if in PAM4 mode
 */
#define TXFIR_NRZ_SUM_LIMIT     (127)
#define TXFIR_PAM4_UC_SUM_LIMIT (168)

/*
 * All taps have bitfield limits:
 */
#define TXFIR_NRZ_TAP_MIN     (-127)
#define TXFIR_NRZ_TAP_MAX     ( 127)
#define TXFIR_PAM4_UC_TAP_MIN (-168)
#define TXFIR_PAM4_UC_TAP_MAX ( 168)

/*
 * Peaking Filter Boundaries and Lookup Tables
 */

/*
 * PFMID
 */
#define PF_MIN_VALUE          ( 0)
#define PF_MAX_VALUE          (30)

/*
 * PFLO
 */
#define PF2_MIN_VALUE         ( 0)
#define PF2_MAX_VALUE         (18)

/*
 * PFHI
 */
#define PF3_MIN_VALUE         ( 0)
#define PF3_MAX_VALUE         (23)

/*
 * AFE BW Boundaries and Lookup Tables
 */
#define VGABW_LUT_MIN_VALUE     ( 0)

#define VGABW_LUT_MAX_VALUE     (19)
                                                /* {pga_bw_cl,fga_bw_cl} */
#define VGABW_LUT_INIT const uint8_t vgabw_lut[VGABW_LUT_MAX_VALUE+1][2] =  \
                                                          {{2,0},   \
                                                           {2,1},   \
                                                           {2,2},   \
                                                           {2,3},   \
                                                           {2,4},   \
                                                           {2,5},   \
                                                           {2,6},   \
                                                           {3,6},   \
                                                           {4,6},   \
                                                           {5,6},   \
                                                           {6,6},   \
                                                           {7,6},   \
                                                           {8,6},   \
                                                           {9,6},   \
                                                          {10,6},   \
                                                          {11,6},   \
                                                          {12,6},   \
                                                          {13,6},   \
                                                          {14,6},   \
                                                          {15,6}};

#define SSA_MODE                        (1)

/* defined(__arm__) || defined(EMULATION_EN) */
/* TODO: below comments? */
/*
 * Note (c) under Table 72-8 of IEEE 802.3 states that V2 must be greater than
 * 40 mV peak-to-zero in all situations.  We test this using the equation:
 *
 *     abs(main) - abs(pre) - abs(post1) >=
 *         TXFIR_V2_LIMIT
 *
 * 40 mV peak-to-zero corresponds to 80 mV peak-to-peak.
 */
#define TXFIR_NRZ_V2_LIMIT  (12)
#define TXFIR_PAM4_V2_LIMIT (16)

/* Maximum values for rx_vga_ctrl_val */
#define RX_VGA_CTRL_VAL_MAX (76)

/**************************************************************************
 *               PVTMON code generated by script                          *
 **************************************************************************/

/* BEGIN_GENERATED_TEMPERATURE_CODE */
/*
 * The formula 1 for PVTMON >= ML3.5.0.XX (HW_VER >= 1) is:
 *
 *     T = 504.41600 - 0.32936 * reg_bin
 */
#define _bin_to_degC_double_1(bin_) (504.41600 - (0.32936 * (USR_DOUBLE)(bin_)))


/* Identify the temperature from the PVTMON output. */
#define _bin_to_degC_1(bin_) (((((int32_t)(  528918512L) +           \
                               ((int32_t)(bin_) * (    -345357L))) \
                              >> 19) + 1) >> 1)

/* Identify the PVTMON output corresponding to the temperature. */
#define _degC_to_bin_1(degc_) (((((int32_t)(  401476897L) +           \
                                ((int32_t)(degc_) * (    -795924L))) \
                               >> 17) + 1) >> 1)

/* Scale temperature change in C to equivalent change in PVTMON output. */
#define _scale_degC_to_bin_1(degc_) (((((int32_t)(degc_) * (    -795924L)) \
                               >> 17) + 1) >> 1)
/* END_GENERATED_TEMPERATURE_CODE */

/**************************************************************************
 *               PVTMON code generated by script                          *
 **************************************************************************/

/* BEGIN_GENERATED_TEMPERATURE_CODE */
/*
 * The formula 0 for PVTMON >=ML3.4.0.XX is:
 *
 *     T = 476.35900 - 0.31770 * reg_bin
 */
#define _bin_to_degC_double_0(bin_) (476.35900 - (0.31770 * (USR_DOUBLE)(bin_)))


/* Identify the temperature from the PVTMON output. */
#define _bin_to_degC_0(bin_) (((((int32_t)(  499498615L) +           \
                               ((int32_t)(bin_) * (    -333137L))) \
                              >> 19) + 1) >> 1)

/* Identify the PVTMON output corresponding to the temperature. */
#define _degC_to_bin_0(degc_) (((((int32_t)(  393053451L) +           \
                                ((int32_t)(degc_) * (    -825120L))) \
                               >> 17) + 1) >> 1)

/* Scale temperature change in C to equivalent change in PVTMON output. */
#define _scale_degC_to_bin_0(degc_) (((((int32_t)(degc_) * (    -825120L)) \
                               >> 17) + 1) >> 1)

/* END_GENERATED_TEMPERATURE_CODE */
#define  TEMP_SELECTOR rdc_temp_select()

#define _bin_to_degC_double(bin_)      ((TEMP_SELECTOR) ? _bin_to_degC_double_1(bin_)     : _bin_to_degC_double_0(bin_))
#define _bin_to_degC(bin_)             ((TEMP_SELECTOR) ? _bin_to_degC_1(bin_)            : _bin_to_degC_0(bin_))
#define _degC_to_bin(degc_)            ((TEMP_SELECTOR) ? _degC_to_bin_1(degc_)           : _degC_to_bin_0(degc_))
#define _scale_degC_to_bin(degc_)      ((TEMP_SELECTOR) ? _scale_degC_to_bin_1(degc_)     : _scale_degC_to_bin_0(degc_))

#endif
