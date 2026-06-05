/***********************************************************************************
 *                                                                                 *
 * Copyright: (c) 2021 Broadcom.                                                   *
 * Broadcom Proprietary and Confidential. All rights reserved.                     *
 *                                                                                 *
 ***********************************************************************************/

/********************************************************************************
 ********************************************************************************
 *                                                                              *
 *  Revision      :   *
 *                                                                              *
 *  Description   :  Defines and Enumerations required by Serdes APIs           *
 *                                                                              *
 ********************************************************************************
 ********************************************************************************/

/** @file peregrine5_pc_common.h
 * Defines and Enumerations shared across MER7/BHK7/OSP7/PER5 APIs but NOT uCode
 */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef PEREGRINE5_PC_API_COMMON_H
#define PEREGRINE5_PC_API_COMMON_H

#include "peregrine5_pc_ipconfig.h"
#include "peregrine5_pc_usr_includes.h"

/** Macro to determine sign of a value */
#define sign(x) ((x>=0) ? 1 : -1)

#define SRDS_MAX_BER_STR_LEN        (32)
#define SRDS_MAX_OSR_STR_LEN        (16)

#define SRDS_MAX_UC_STS_STR_LEN     (512)
#define SRDS_MAX_UC_STS_EXT_STR_LEN (512)

/*
 * IP-Specific Iteration Bounds
 */
# define NUM_PLLS (1)

#define TXFIR_ST_NUM_TAPS (6)

/*! TX FIR tap values
 *
 */
typedef struct peregrine5_pc_txfir {
  int16_t pre3;
  int16_t pre2;
  int16_t pre1;
  int16_t main;
  int16_t post1;
  int16_t post2;
} peregrine5_pc_txfir_st;

#define SRDS_NUM_ADCS           (80)
#define SRDS_NUM_FFE_TAPS       (12)
#define SRDS_NUM_RCFIRS         (12)
#define SRDS_NUM_ADC_SKEW_CTRL  (8)
#define SRDS_NUM_ANA_ADC_GAIN   (80)
#define SRDS_NUM_DFE_OFFSETS    (12)
#define SRDS_NUM_VEYE_CENTERS   (12)
#define SRDS_NUM_VEYE_RADIUS    (12)
#define SRDS_NUM_FFE_ILVS       (2)

/*! RC FIR struct
 *
 */
typedef struct peregrine5_pc_rcfir {
  uint8_t tap_loc;
  int32_t  tap_val;
} peregrine5_pc_rcfir_st;
/*------------------------------*/
/** Serdes OSR Mode Structure   */
/*------------------------------*/
typedef struct {
  /** TX OSR Mode */
  uint8_t tx;
  /** RX OSR Mode */
  uint8_t rx;
  /** OSR Mode for TX and RX (used when both TX and RX should have same OSR Mode) */
  uint8_t tx_rx;
} peregrine5_pc_osr_mode_st;

#define srds_Y_width (6)
#define srds_X_width (9)
/*! Debug feedback status
 *
 */
typedef struct peregrine5_pc_dbgfb_stats_s {
    int32_t  data1[srds_X_width][srds_Y_width];
    int32_t  data2[srds_X_width][srds_Y_width];
    int32_t  data3[srds_X_width][srds_Y_width];
    int32_t  data4[srds_X_width][srds_Y_width];
} peregrine5_pc_dbgfb_data_st;


/*! Debug feedback data
 *
 */
typedef struct peregrine5_pc_dbgfb_data_s {
    uint32_t time_in_us;
    peregrine5_pc_dbgfb_data_st data;
    uint8_t pam4es_en;
    uint8_t temp[3];
} peregrine5_pc_dbgfb_stats_st;



#endif
#ifdef __cplusplus
}
#endif
