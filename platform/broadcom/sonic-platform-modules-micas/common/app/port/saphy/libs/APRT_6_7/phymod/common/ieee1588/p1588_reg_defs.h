/*
 * $Id: $
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 */

#ifndef __P1588_REG_DEFS__H__
#define __P1588_REG_DEFS__H__

#if defined(PHYMOD_XGBASET_SUPPORT) || defined(PHYMOD_MGAUTO_SUPPORT)
  #define CAP_SEL_MASK    0x07    /* bit[2:0] to select PTPsrcPort/MacDA/SA/DstIP/SrcIP/MPLS/IPv6 */
#else          /*  Aperta / Miura / Evora / Europa / Quadra28 / Orca  */
  #define CAP_SEL_MASK    0xFF    /* bit[7:0] to select PTPsrcPort/MacDA/SA/DstIP/SrcIP/MPLS/IPv6 */
#endif


/*******************************************************************************
 *
 * REGISTER:  P1588_SLICE_ENABLE_CONTROL
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7000
 * DESC:     SLICE ENABLE CONTROL Register
 * RESETVAL: 0x4 (4)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_SLICE_1588_EN Enables the 1588 slice[0] - enable TX port
 *     RX_SLICE_1588_EN Enables the 1588 slice[1] - enable RX port
 *     NSE_TIMER_CLOCK_ENABLE 1= enable timer clock
 *     PCH_TIMER_CLOCK_ENABLE 1= enable timer clockif(nse_timer_clock_enable) PCH function will operate NSE ck_1p03125g/8 clockelse PCH function will operate PTP_ref clock from PAD
 *     SEL_I2CCFG_FOR_SYNC 0 - SYNC_IN/SYNC_OUT uses GPIO[1:0] pins1 - SYNC_IN/SYNC_OUT uses CONFIG[1:0] pins
 *     MODE_1G_OVRD     Enables 1G speed mode override
 *     MODE_1G_OV       1G speed mode overrride value
 *     MODE_10G_OVRD    Enables 10G speed mode override
 *     MODE_10G_OV      10G speed mode overrride value
 *     P1588_OTP_EN_OVERRIDE 1= disable 1588 otp enable bit
 *     VCO_RATE_CHANGE_ENABLE TBD
 *     SW_RSTB_NSE_P1588 Soft reset for 1588 timer block onlyWriting a 1 will cause a reset and this bit will be cleared by hw
 *     SW_RSTB_REG_P1588 Soft reset for 1588 register blockWriting a 1 will cause a reset and this bit will be cleared by hw
 *     SW_RSTB_P1588    Soft reset for 1588 block. datapath and statem achineWriting a 1 will cause a reset and this bit will be cleared by hw
 *
 ******************************************************************************/
#define PLP_P1588_SLICE_ENABLE_CONTROLr    0x00000000

#define PLP_P1588_SLICE_ENABLE_CONTROLr_SIZE 4

/*
 * This structure should be used to declare and program P1588_SLICE_ENABLE_CONTROL.
 *
 */
typedef union PLP_P1588_SLICE_ENABLE_CONTROLr_s {
	uint32_t v[1];
	uint32_t p1588_slice_enable_control[1];
	uint32_t _p1588_slice_enable_control;
} PLP_P1588_SLICE_ENABLE_CONTROLr_t;

#define PLP_P1588_SLICE_ENABLE_CONTROLr_CLR(r) (r).p1588_slice_enable_control[0] = 0
#define PLP_P1588_SLICE_ENABLE_CONTROLr_SET(r,d) (r).p1588_slice_enable_control[0] = d
#define PLP_P1588_SLICE_ENABLE_CONTROLr_GET(r) (r).p1588_slice_enable_control[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SLICE_ENABLE_CONTROLr_SW_RSTB_P1588f_GET(r) ((((r).p1588_slice_enable_control[0]) >> 15) & 0x1)
#define PLP_P1588_SLICE_ENABLE_CONTROLr_SW_RSTB_P1588f_SET(r,f) (r).p1588_slice_enable_control[0]=(((r).p1588_slice_enable_control[0] & ~((uint32_t)0x1 << 15)) | ((((uint32_t)f) & 0x1) << 15)) | (1 << (16 + 15))
#define PLP_P1588_SLICE_ENABLE_CONTROLr_SW_RSTB_REG_P1588f_GET(r) ((((r).p1588_slice_enable_control[0]) >> 14) & 0x1)
#define PLP_P1588_SLICE_ENABLE_CONTROLr_SW_RSTB_REG_P1588f_SET(r,f) (r).p1588_slice_enable_control[0]=(((r).p1588_slice_enable_control[0] & ~((uint32_t)0x1 << 14)) | ((((uint32_t)f) & 0x1) << 14)) | (1 << (16 + 14))
#define PLP_P1588_SLICE_ENABLE_CONTROLr_SW_RSTB_NSE_P1588f_GET(r) ((((r).p1588_slice_enable_control[0]) >> 13) & 0x1)
#define PLP_P1588_SLICE_ENABLE_CONTROLr_SW_RSTB_NSE_P1588f_SET(r,f) (r).p1588_slice_enable_control[0]=(((r).p1588_slice_enable_control[0] & ~((uint32_t)0x1 << 13)) | ((((uint32_t)f) & 0x1) << 13)) | (1 << (16 + 13))
#define PLP_P1588_SLICE_ENABLE_CONTROLr_VCO_RATE_CHANGE_ENABLEf_GET(r) ((((r).p1588_slice_enable_control[0]) >> 11) & 0x1)
#define PLP_P1588_SLICE_ENABLE_CONTROLr_VCO_RATE_CHANGE_ENABLEf_SET(r,f) (r).p1588_slice_enable_control[0]=(((r).p1588_slice_enable_control[0] & ~((uint32_t)0x1 << 11)) | ((((uint32_t)f) & 0x1) << 11)) | (1 << (16 + 11))
#define PLP_P1588_SLICE_ENABLE_CONTROLr_NSE_TIMER_NEW_CLOCK_ENABLEf_GET(r) ((((r).p1588_slice_enable_control[0]) >> 10) & 0x1)
#define PLP_P1588_SLICE_ENABLE_CONTROLr_NSE_TIMER_NEW_CLOCK_ENABLEf_SET(r,f) (r).p1588_slice_enable_control[0]=(((r).p1588_slice_enable_control[0] & ~((uint32_t)0x1 << 10)) | ((((uint32_t)f) & 0x1) << 10)) | (1 << (16 + 10))
#define PLP_P1588_SLICE_ENABLE_CONTROLr_P1588_OTP_EN_OVERRIDEf_GET(r) ((((r).p1588_slice_enable_control[0]) >> 9) & 0x1)
#define PLP_P1588_SLICE_ENABLE_CONTROLr_P1588_OTP_EN_OVERRIDEf_SET(r,f) (r).p1588_slice_enable_control[0]=(((r).p1588_slice_enable_control[0] & ~((uint32_t)0x1 << 9)) | ((((uint32_t)f) & 0x1) << 9)) | (1 << (16 + 9))
#define PLP_P1588_SLICE_ENABLE_CONTROLr_MODE_10G_OVf_GET(r) ((((r).p1588_slice_enable_control[0]) >> 8) & 0x1)
#define PLP_P1588_SLICE_ENABLE_CONTROLr_MODE_10G_OVf_SET(r,f) (r).p1588_slice_enable_control[0]=(((r).p1588_slice_enable_control[0] & ~((uint32_t)0x1 << 8)) | ((((uint32_t)f) & 0x1) << 8)) | (1 << (16 + 8))
#define PLP_P1588_SLICE_ENABLE_CONTROLr_MODE_10G_OVRDf_GET(r) ((((r).p1588_slice_enable_control[0]) >> 7) & 0x1)
#define PLP_P1588_SLICE_ENABLE_CONTROLr_MODE_10G_OVRDf_SET(r,f) (r).p1588_slice_enable_control[0]=(((r).p1588_slice_enable_control[0] & ~((uint32_t)0x1 << 7)) | ((((uint32_t)f) & 0x1) << 7)) | (1 << (16 + 7))
#define PLP_P1588_SLICE_ENABLE_CONTROLr_MODE_1G_OVf_GET(r) ((((r).p1588_slice_enable_control[0]) >> 6) & 0x1)
#define PLP_P1588_SLICE_ENABLE_CONTROLr_MODE_1G_OVf_SET(r,f) (r).p1588_slice_enable_control[0]=(((r).p1588_slice_enable_control[0] & ~((uint32_t)0x1 << 6)) | ((((uint32_t)f) & 0x1) << 6)) | (1 << (16 + 6))
#define PLP_P1588_SLICE_ENABLE_CONTROLr_MODE_1G_OVRDf_GET(r) ((((r).p1588_slice_enable_control[0]) >> 5) & 0x1)
#define PLP_P1588_SLICE_ENABLE_CONTROLr_MODE_1G_OVRDf_SET(r,f) (r).p1588_slice_enable_control[0]=(((r).p1588_slice_enable_control[0] & ~((uint32_t)0x1 << 5)) | ((((uint32_t)f) & 0x1) << 5)) | (1 << (16 + 5))
#define PLP_P1588_SLICE_ENABLE_CONTROLr_PTP_VER_2_1f_GET(r) ((((r).p1588_slice_enable_control[0]) >> 4) & 0x1)
#define PLP_P1588_SLICE_ENABLE_CONTROLr_PTP_VER_2_1f_SET(r,f)  (r).p1588_slice_enable_control[0]=(((r).p1588_slice_enable_control[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4)) | (1 << (16 + 4))
#define PLP_P1588_SLICE_ENABLE_CONTROLr_SEL_I2CCFG_FOR_SYNCf_GET(r) ((((r).p1588_slice_enable_control[0]) >> 4) & 0x1)
#define PLP_P1588_SLICE_ENABLE_CONTROLr_SEL_I2CCFG_FOR_SYNCf_SET(r,f) (r).p1588_slice_enable_control[0]=(((r).p1588_slice_enable_control[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4)) | (1 << (16 + 4))
#define PLP_P1588_SLICE_ENABLE_CONTROLr_PIM_TIMER_CLOCK_ENABLEf_GET(r) ((((r).p1588_slice_enable_control[0]) >> 3) & 0x1)
#define PLP_P1588_SLICE_ENABLE_CONTROLr_PIM_TIMER_CLOCK_ENABLEf_SET(r,f) (r).p1588_slice_enable_control[0]=(((r).p1588_slice_enable_control[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3)) | (1 << (16 + 3))
#define PLP_P1588_SLICE_ENABLE_CONTROLr_NSE_TIMER_CLOCK_ENABLEf_GET(r) ((((r).p1588_slice_enable_control[0]) >> 2) & 0x1)
#define PLP_P1588_SLICE_ENABLE_CONTROLr_NSE_TIMER_CLOCK_ENABLEf_SET(r,f) (r).p1588_slice_enable_control[0]=(((r).p1588_slice_enable_control[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2)) | (1 << (16 + 2))
#define PLP_P1588_SLICE_ENABLE_CONTROLr_RX_SLICE_1588_ENf_GET(r) ((((r).p1588_slice_enable_control[0]) >> 1) & 0x1)
#define PLP_P1588_SLICE_ENABLE_CONTROLr_RX_SLICE_1588_ENf_SET(r,f) (r).p1588_slice_enable_control[0]=(((r).p1588_slice_enable_control[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))
#define PLP_P1588_SLICE_ENABLE_CONTROLr_TX_SLICE_1588_ENf_GET(r) (((r).p1588_slice_enable_control[0]) & 0x1)
#define PLP_P1588_SLICE_ENABLE_CONTROLr_TX_SLICE_1588_ENf_SET(r,f) (r).p1588_slice_enable_control[0]=(((r).p1588_slice_enable_control[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1)) | (0x1 << 16)

#define PLP_P1588_SLICE_ENABLE_CONTROLr_RXTX_SLICE_1588_ENf_GET(r) (((r).p1588_slice_enable_control[0]) & 0x3)

/*
 * These macros can be used to access P1588_SLICE_ENABLE_CONTROL.
 *
 */
#define PLP_READ_P1588_SLICE_ENABLE_CONTROLr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SLICE_ENABLE_CONTROLr,(_r._p1588_slice_enable_control))
#define PLP_WRITE_P1588_SLICE_ENABLE_CONTROLr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SLICE_ENABLE_CONTROLr,(_r._p1588_slice_enable_control))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SLICE_ENABLE_CONTROLr PLP_P1588_SLICE_ENABLE_CONTROLr
#define P1588_SLICE_ENABLE_CONTROLr_SIZE PLP_P1588_SLICE_ENABLE_CONTROLr_SIZE
typedef PLP_P1588_SLICE_ENABLE_CONTROLr_t P1588_SLICE_ENABLE_CONTROLr_t;
#define P1588_SLICE_ENABLE_CONTROLr_CLR PLP_P1588_SLICE_ENABLE_CONTROLr_CLR
#define P1588_SLICE_ENABLE_CONTROLr_SET PLP_P1588_SLICE_ENABLE_CONTROLr_SET
#define P1588_SLICE_ENABLE_CONTROLr_GET PLP_P1588_SLICE_ENABLE_CONTROLr_GET
#define P1588_SLICE_ENABLE_CONTROLr_SW_RSTB_P1588f_GET PLP_P1588_SLICE_ENABLE_CONTROLr_SW_RSTB_P1588f_GET
#define P1588_SLICE_ENABLE_CONTROLr_SW_RSTB_P1588f_SET PLP_P1588_SLICE_ENABLE_CONTROLr_SW_RSTB_P1588f_SET
#define P1588_SLICE_ENABLE_CONTROLr_SW_RSTB_REG_P1588f_GET PLP_P1588_SLICE_ENABLE_CONTROLr_SW_RSTB_REG_P1588f_GET
#define P1588_SLICE_ENABLE_CONTROLr_SW_RSTB_REG_P1588f_SET PLP_P1588_SLICE_ENABLE_CONTROLr_SW_RSTB_REG_P1588f_SET
#define P1588_SLICE_ENABLE_CONTROLr_SW_RSTB_NSE_P1588f_GET PLP_P1588_SLICE_ENABLE_CONTROLr_SW_RSTB_NSE_P1588f_GET
#define P1588_SLICE_ENABLE_CONTROLr_SW_RSTB_NSE_P1588f_SET PLP_P1588_SLICE_ENABLE_CONTROLr_SW_RSTB_NSE_P1588f_SET
#define P1588_SLICE_ENABLE_CONTROLr_VCO_RATE_CHANGE_ENABLEf_GET PLP_P1588_SLICE_ENABLE_CONTROLr_VCO_RATE_CHANGE_ENABLEf_GET
#define P1588_SLICE_ENABLE_CONTROLr_VCO_RATE_CHANGE_ENABLEf_SET PLP_P1588_SLICE_ENABLE_CONTROLr_VCO_RATE_CHANGE_ENABLEf_SET
#define P1588_SLICE_ENABLE_CONTROLr_NSE_TIMER_NEW_CLOCK_ENABLEf_GET PLP_P1588_SLICE_ENABLE_CONTROLr_NSE_TIMER_NEW_CLOCK_ENABLEf_GET
#define P1588_SLICE_ENABLE_CONTROLr_NSE_TIMER_NEW_CLOCK_ENABLEf_SET PLP_P1588_SLICE_ENABLE_CONTROLr_NSE_TIMER_NEW_CLOCK_ENABLEf_SET
#define P1588_SLICE_ENABLE_CONTROLr_P1588_OTP_EN_OVERRIDEf_GET PLP_P1588_SLICE_ENABLE_CONTROLr_P1588_OTP_EN_OVERRIDEf_GET
#define P1588_SLICE_ENABLE_CONTROLr_P1588_OTP_EN_OVERRIDEf_SET PLP_P1588_SLICE_ENABLE_CONTROLr_P1588_OTP_EN_OVERRIDEf_SET
#define P1588_SLICE_ENABLE_CONTROLr_MODE_10G_OVf_GET PLP_P1588_SLICE_ENABLE_CONTROLr_MODE_10G_OVf_GET
#define P1588_SLICE_ENABLE_CONTROLr_MODE_10G_OVf_SET PLP_P1588_SLICE_ENABLE_CONTROLr_MODE_10G_OVf_SET
#define P1588_SLICE_ENABLE_CONTROLr_MODE_10G_OVRDf_GET PLP_P1588_SLICE_ENABLE_CONTROLr_MODE_10G_OVRDf_GET
#define P1588_SLICE_ENABLE_CONTROLr_MODE_10G_OVRDf_SET PLP_P1588_SLICE_ENABLE_CONTROLr_MODE_10G_OVRDf_SET
#define P1588_SLICE_ENABLE_CONTROLr_MODE_1G_OVf_GET PLP_P1588_SLICE_ENABLE_CONTROLr_MODE_1G_OVf_GET
#define P1588_SLICE_ENABLE_CONTROLr_MODE_1G_OVf_SET PLP_P1588_SLICE_ENABLE_CONTROLr_MODE_1G_OVf_SET
#define P1588_SLICE_ENABLE_CONTROLr_MODE_1G_OVRDf_GET PLP_P1588_SLICE_ENABLE_CONTROLr_MODE_1G_OVRDf_GET
#define P1588_SLICE_ENABLE_CONTROLr_MODE_1G_OVRDf_SET PLP_P1588_SLICE_ENABLE_CONTROLr_MODE_1G_OVRDf_SET
#define P1588_SLICE_ENABLE_CONTROLr_PTP_VER_2_1f_GET PLP_P1588_SLICE_ENABLE_CONTROLr_PTP_VER_2_1f_GET
#define P1588_SLICE_ENABLE_CONTROLr_PTP_VER_2_1f_SET PLP_P1588_SLICE_ENABLE_CONTROLr_PTP_VER_2_1f_SET
#define P1588_SLICE_ENABLE_CONTROLr_SEL_I2CCFG_FOR_SYNCf_GET PLP_P1588_SLICE_ENABLE_CONTROLr_SEL_I2CCFG_FOR_SYNCf_GET
#define P1588_SLICE_ENABLE_CONTROLr_SEL_I2CCFG_FOR_SYNCf_SET PLP_P1588_SLICE_ENABLE_CONTROLr_SEL_I2CCFG_FOR_SYNCf_SET
#define P1588_SLICE_ENABLE_CONTROLr_PCH_TIMER_CLOCK_ENABLEf_GET PLP_P1588_SLICE_ENABLE_CONTROLr_PIM_TIMER_CLOCK_ENABLEf_GET
#define P1588_SLICE_ENABLE_CONTROLr_PCH_TIMER_CLOCK_ENABLEf_SET PLP_P1588_SLICE_ENABLE_CONTROLr_PIM_TIMER_CLOCK_ENABLEf_SET
#define P1588_SLICE_ENABLE_CONTROLr_EXT_TIMER_CLOCK_ENABLEf_GET PLP_P1588_SLICE_ENABLE_CONTROLr_PIM_TIMER_CLOCK_ENABLEf_GET
#define P1588_SLICE_ENABLE_CONTROLr_EXT_TIMER_CLOCK_ENABLEf_SET PLP_P1588_SLICE_ENABLE_CONTROLr_PIM_TIMER_CLOCK_ENABLEf_SET
#define P1588_SLICE_ENABLE_CONTROLr_NSE_TIMER_CLOCK_ENABLEf_GET PLP_P1588_SLICE_ENABLE_CONTROLr_NSE_TIMER_CLOCK_ENABLEf_GET
#define P1588_SLICE_ENABLE_CONTROLr_NSE_TIMER_CLOCK_ENABLEf_SET PLP_P1588_SLICE_ENABLE_CONTROLr_NSE_TIMER_CLOCK_ENABLEf_SET
#define P1588_SLICE_ENABLE_CONTROLr_RX_SLICE_1588_ENf_GET PLP_P1588_SLICE_ENABLE_CONTROLr_RX_SLICE_1588_ENf_GET
#define P1588_SLICE_ENABLE_CONTROLr_RX_SLICE_1588_ENf_SET PLP_P1588_SLICE_ENABLE_CONTROLr_RX_SLICE_1588_ENf_SET
#define P1588_SLICE_ENABLE_CONTROLr_TX_SLICE_1588_ENf_GET PLP_P1588_SLICE_ENABLE_CONTROLr_TX_SLICE_1588_ENf_GET
#define P1588_SLICE_ENABLE_CONTROLr_TX_SLICE_1588_ENf_SET PLP_P1588_SLICE_ENABLE_CONTROLr_TX_SLICE_1588_ENf_SET
#define P1588_SLICE_ENABLE_CONTROLr_RXTX_SLICE_1588_ENf_GET PLP_P1588_SLICE_ENABLE_CONTROLr_RXTX_SLICE_1588_ENf_GET
#define READ_P1588_SLICE_ENABLE_CONTROLr PLP_READ_P1588_SLICE_ENABLE_CONTROLr
#define WRITE_P1588_SLICE_ENABLE_CONTROLr PLP_WRITE_P1588_SLICE_ENABLE_CONTROLr
#define MODIFY_P1588_SLICE_ENABLE_CONTROLr PLP_MODIFY_P1588_SLICE_ENABLE_CONTROLr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SLICE_ENABLE_CONTROLr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TX_EVENT_MESSAGE_MODE_SEL
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7001
 * DESC:     TX EVENT MESSAGE MODE SEL Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_MODE1         TX Port mode selection bit[15:14] - LED enable option2'b00 = LED off2'b01 = CRCTAG Mode LED on2'b20 = PCH Mode LED on2'b11 = 1588 Mode LED[13:12] - event message 10 selection[11:10] - event message 9 selection[9:8] - event message 8 selection[7:6] - event message 3 selection[5:4] - event message 2 selection[3:2] - event message 1 selection[1:0] - event message 0 selectionExample, applies to each one of the event message types2'b00 - NA2'b01 - Update correction field2'b10 - Replace CF based on the NCO TS2'b11 - Replace CF based on the NCO TS and time_code_reg
 *
 ******************************************************************************/
#define PLP_P1588_TX_EVENT_MESSAGE_MODE_SELr    0x00000001

#define PLP_P1588_TX_EVENT_MESSAGE_MODE_SELr_SIZE 4

/*
 * This structure should be used to declare and program P1588_TX_EVENT_MESSAGE_MODE_SEL.
 *
 */
typedef union PLP_P1588_TX_EVENT_MESSAGE_MODE_SELr_s {
	uint32_t v[1];
	uint32_t p1588_tx_event_message_mode_sel[1];
	uint32_t _p1588_tx_event_message_mode_sel;
} PLP_P1588_TX_EVENT_MESSAGE_MODE_SELr_t;

#define PLP_P1588_TX_EVENT_MESSAGE_MODE_SELr_CLR(r) (r).p1588_tx_event_message_mode_sel[0] = 0
#define PLP_P1588_TX_EVENT_MESSAGE_MODE_SELr_SET(r,d) (r).p1588_tx_event_message_mode_sel[0] = d
#define PLP_P1588_TX_EVENT_MESSAGE_MODE_SELr_GET(r) (r).p1588_tx_event_message_mode_sel[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TX_EVENT_MESSAGE_MODE_SELr_TX_MODE1f_GET(r) (((r).p1588_tx_event_message_mode_sel[0]) & 0xffff)
#define PLP_P1588_TX_EVENT_MESSAGE_MODE_SELr_TX_MODE1f_SET(r,f) (r).p1588_tx_event_message_mode_sel[0]=(((r).p1588_tx_event_message_mode_sel[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TX_EVENT_MESSAGE_MODE_SEL.
 *
 */
#define PLP_READ_P1588_TX_EVENT_MESSAGE_MODE_SELr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TX_EVENT_MESSAGE_MODE_SELr,(_r._p1588_tx_event_message_mode_sel))
#define PLP_WRITE_P1588_TX_EVENT_MESSAGE_MODE_SELr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TX_EVENT_MESSAGE_MODE_SELr,(_r._p1588_tx_event_message_mode_sel))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TX_EVENT_MESSAGE_MODE_SELr PLP_P1588_TX_EVENT_MESSAGE_MODE_SELr
#define P1588_TX_EVENT_MESSAGE_MODE_SELr_SIZE PLP_P1588_TX_EVENT_MESSAGE_MODE_SELr_SIZE
typedef PLP_P1588_TX_EVENT_MESSAGE_MODE_SELr_t P1588_TX_EVENT_MESSAGE_MODE_SELr_t;
#define P1588_TX_EVENT_MESSAGE_MODE_SELr_CLR PLP_P1588_TX_EVENT_MESSAGE_MODE_SELr_CLR
#define P1588_TX_EVENT_MESSAGE_MODE_SELr_SET PLP_P1588_TX_EVENT_MESSAGE_MODE_SELr_SET
#define P1588_TX_EVENT_MESSAGE_MODE_SELr_GET PLP_P1588_TX_EVENT_MESSAGE_MODE_SELr_GET
#define P1588_TX_EVENT_MESSAGE_MODE_SELr_TX_MODE1f_GET PLP_P1588_TX_EVENT_MESSAGE_MODE_SELr_TX_MODE1f_GET
#define P1588_TX_EVENT_MESSAGE_MODE_SELr_TX_MODE1f_SET PLP_P1588_TX_EVENT_MESSAGE_MODE_SELr_TX_MODE1f_SET
#define READ_P1588_TX_EVENT_MESSAGE_MODE_SELr PLP_READ_P1588_TX_EVENT_MESSAGE_MODE_SELr
#define WRITE_P1588_TX_EVENT_MESSAGE_MODE_SELr PLP_WRITE_P1588_TX_EVENT_MESSAGE_MODE_SELr
#define MODIFY_P1588_TX_EVENT_MESSAGE_MODE_SELr PLP_MODIFY_P1588_TX_EVENT_MESSAGE_MODE_SELr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TX_EVENT_MESSAGE_MODE_SELr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TX_MODE2_SEL
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7002
 * DESC:     TX MODE2 SEL Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_MODE2         Bits 6 to 4 are reserved for future useBits 7 - 1 = enable read packet counter for current channelBits 7 - 0 = disable read packet counter for current channel[3] - Capture enable for PDelay resp message[2] - Capture enable for PDelay req message[1] - Capture enable for Delay req message[0] - Capture enable for Sync message
 *
 ******************************************************************************/
#define PLP_P1588_TX_MODE2_SELr    0x00000002

#define PLP_P1588_TX_MODE2_SELr_SIZE 4

/*
 * This structure should be used to declare and program P1588_TX_MODE2_SEL.
 *
 */
typedef union PLP_P1588_TX_MODE2_SELr_s {
	uint32_t v[1];
	uint32_t p1588_tx_mode2_sel[1];
	uint32_t _p1588_tx_mode2_sel;
} PLP_P1588_TX_MODE2_SELr_t;

#define PLP_P1588_TX_MODE2_SELr_CLR(r) (r).p1588_tx_mode2_sel[0] = 0
#define PLP_P1588_TX_MODE2_SELr_SET(r,d) (r).p1588_tx_mode2_sel[0] = d
#define PLP_P1588_TX_MODE2_SELr_GET(r) (r).p1588_tx_mode2_sel[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TX_MODE2_SELr_TX_MODE2f_GET(r) (((r).p1588_tx_mode2_sel[0]) & 0xff)
#define PLP_P1588_TX_MODE2_SELr_TX_MODE2f_SET(r,f) (r).p1588_tx_mode2_sel[0]=(((r).p1588_tx_mode2_sel[0] & ~((uint32_t)0xff)) | (((uint32_t)f) & 0xff)) | (0xff << 16)

/*
 * These macros can be used to access P1588_TX_MODE2_SEL.
 *
 */
#define PLP_READ_P1588_TX_MODE2_SELr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TX_MODE2_SELr,(_r._p1588_tx_mode2_sel))
#define PLP_WRITE_P1588_TX_MODE2_SELr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TX_MODE2_SELr,(_r._p1588_tx_mode2_sel))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TX_MODE2_SELr PLP_P1588_TX_MODE2_SELr
#define P1588_TX_MODE2_SELr_SIZE PLP_P1588_TX_MODE2_SELr_SIZE
typedef PLP_P1588_TX_MODE2_SELr_t P1588_TX_MODE2_SELr_t;
#define P1588_TX_MODE2_SELr_CLR PLP_P1588_TX_MODE2_SELr_CLR
#define P1588_TX_MODE2_SELr_SET PLP_P1588_TX_MODE2_SELr_SET
#define P1588_TX_MODE2_SELr_GET PLP_P1588_TX_MODE2_SELr_GET
#define P1588_TX_MODE2_SELr_TX_MODE2f_GET PLP_P1588_TX_MODE2_SELr_TX_MODE2f_GET
#define P1588_TX_MODE2_SELr_TX_MODE2f_SET PLP_P1588_TX_MODE2_SELr_TX_MODE2f_SET
#define READ_P1588_TX_MODE2_SELr PLP_READ_P1588_TX_MODE2_SELr
#define WRITE_P1588_TX_MODE2_SELr PLP_WRITE_P1588_TX_MODE2_SELr
#define MODIFY_P1588_TX_MODE2_SELr PLP_MODIFY_P1588_TX_MODE2_SELr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TX_MODE2_SELr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_RX_EVENT_MESSAGE_MODE_SEL
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7003
 * DESC:     RX EVENT MESSAGE MODE SEL Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_MODE1         RX Port mode selection bit[15:14] - LED enable option2'b00 = LED off2'b01 = CRCTAG Mode LED on2'b20 = PCH Mode LED on2'b11 = 1588 Mode LED[13:12] - event message 10 selection[11:10] - event message 9 selection[9:8] - event message 8 selection[7:6] - event message 3 selection[5:4] - event message 2 selection[3:2] - event message 1 selection[1:0] - event message 0 selectionExample, applies to each one of the event message types2'b00 - NA2'b01 - Update correction field2'b10 - Insert 80b timestamp(Ts) field using time_code_reg value2'b11 - Insert 80b timestamp(Ts-Tn) field using the 80b NCO
 *
 ******************************************************************************/
#define PLP_P1588_RX_EVENT_MESSAGE_MODE_SELr    0x00000003

#define PLP_P1588_RX_EVENT_MESSAGE_MODE_SELr_SIZE 4

/*
 * This structure should be used to declare and program P1588_RX_EVENT_MESSAGE_MODE_SEL.
 *
 */
typedef union PLP_P1588_RX_EVENT_MESSAGE_MODE_SELr_s {
	uint32_t v[1];
	uint32_t p1588_rx_event_message_mode_sel[1];
	uint32_t _p1588_rx_event_message_mode_sel;
} PLP_P1588_RX_EVENT_MESSAGE_MODE_SELr_t;

#define PLP_P1588_RX_EVENT_MESSAGE_MODE_SELr_CLR(r) (r).p1588_rx_event_message_mode_sel[0] = 0
#define PLP_P1588_RX_EVENT_MESSAGE_MODE_SELr_SET(r,d) (r).p1588_rx_event_message_mode_sel[0] = d
#define PLP_P1588_RX_EVENT_MESSAGE_MODE_SELr_GET(r) (r).p1588_rx_event_message_mode_sel[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_RX_EVENT_MESSAGE_MODE_SELr_RX_MODE1f_GET(r) (((r).p1588_rx_event_message_mode_sel[0]) & 0xffff)
#define PLP_P1588_RX_EVENT_MESSAGE_MODE_SELr_RX_MODE1f_SET(r,f) (r).p1588_rx_event_message_mode_sel[0]=(((r).p1588_rx_event_message_mode_sel[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_RX_EVENT_MESSAGE_MODE_SEL.
 *
 */
#define PLP_READ_P1588_RX_EVENT_MESSAGE_MODE_SELr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_RX_EVENT_MESSAGE_MODE_SELr,(_r._p1588_rx_event_message_mode_sel))
#define PLP_WRITE_P1588_RX_EVENT_MESSAGE_MODE_SELr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_RX_EVENT_MESSAGE_MODE_SELr,(_r._p1588_rx_event_message_mode_sel))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_RX_EVENT_MESSAGE_MODE_SELr PLP_P1588_RX_EVENT_MESSAGE_MODE_SELr
#define P1588_RX_EVENT_MESSAGE_MODE_SELr_SIZE PLP_P1588_RX_EVENT_MESSAGE_MODE_SELr_SIZE
typedef PLP_P1588_RX_EVENT_MESSAGE_MODE_SELr_t P1588_RX_EVENT_MESSAGE_MODE_SELr_t;
#define P1588_RX_EVENT_MESSAGE_MODE_SELr_CLR PLP_P1588_RX_EVENT_MESSAGE_MODE_SELr_CLR
#define P1588_RX_EVENT_MESSAGE_MODE_SELr_SET PLP_P1588_RX_EVENT_MESSAGE_MODE_SELr_SET
#define P1588_RX_EVENT_MESSAGE_MODE_SELr_GET PLP_P1588_RX_EVENT_MESSAGE_MODE_SELr_GET
#define P1588_RX_EVENT_MESSAGE_MODE_SELr_RX_MODE1f_GET PLP_P1588_RX_EVENT_MESSAGE_MODE_SELr_RX_MODE1f_GET
#define P1588_RX_EVENT_MESSAGE_MODE_SELr_RX_MODE1f_SET PLP_P1588_RX_EVENT_MESSAGE_MODE_SELr_RX_MODE1f_SET
#define READ_P1588_RX_EVENT_MESSAGE_MODE_SELr PLP_READ_P1588_RX_EVENT_MESSAGE_MODE_SELr
#define WRITE_P1588_RX_EVENT_MESSAGE_MODE_SELr PLP_WRITE_P1588_RX_EVENT_MESSAGE_MODE_SELr
#define MODIFY_P1588_RX_EVENT_MESSAGE_MODE_SELr PLP_MODIFY_P1588_RX_EVENT_MESSAGE_MODE_SELr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_RX_EVENT_MESSAGE_MODE_SELr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_RX_MODE2_SEL
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7004
 * DESC:     RX MODE2 SEL Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_MODE2         Bits 7 to 4 are reserved for future use[3] - Capture enable for PDelay resp message[2] - Capture enable for PDelay req message[1] - Capture enable for Delay req message[0] - Capture enable for Sync message
 *
 ******************************************************************************/
#define PLP_P1588_RX_MODE2_SELr    0x00000004

#define PLP_P1588_RX_MODE2_SELr_SIZE 4

/*
 * This structure should be used to declare and program P1588_RX_MODE2_SEL.
 *
 */
typedef union PLP_P1588_RX_MODE2_SELr_s {
	uint32_t v[1];
	uint32_t p1588_rx_mode2_sel[1];
	uint32_t _p1588_rx_mode2_sel;
} PLP_P1588_RX_MODE2_SELr_t;

#define PLP_P1588_RX_MODE2_SELr_CLR(r) (r).p1588_rx_mode2_sel[0] = 0
#define PLP_P1588_RX_MODE2_SELr_SET(r,d) (r).p1588_rx_mode2_sel[0] = d
#define PLP_P1588_RX_MODE2_SELr_GET(r) (r).p1588_rx_mode2_sel[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_RX_MODE2_SELr_RX_MODE2f_GET(r) (((r).p1588_rx_mode2_sel[0]) & 0xff)
#define PLP_P1588_RX_MODE2_SELr_RX_MODE2f_SET(r,f) (r).p1588_rx_mode2_sel[0]=(((r).p1588_rx_mode2_sel[0] & ~((uint32_t)0xff)) | (((uint32_t)f) & 0xff)) | (0xff << 16)

/*
 * These macros can be used to access P1588_RX_MODE2_SEL.
 *
 */
#define PLP_READ_P1588_RX_MODE2_SELr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_RX_MODE2_SELr,(_r._p1588_rx_mode2_sel))
#define PLP_WRITE_P1588_RX_MODE2_SELr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_RX_MODE2_SELr,(_r._p1588_rx_mode2_sel))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_RX_MODE2_SELr PLP_P1588_RX_MODE2_SELr
#define P1588_RX_MODE2_SELr_SIZE PLP_P1588_RX_MODE2_SELr_SIZE
typedef PLP_P1588_RX_MODE2_SELr_t P1588_RX_MODE2_SELr_t;
#define P1588_RX_MODE2_SELr_CLR PLP_P1588_RX_MODE2_SELr_CLR
#define P1588_RX_MODE2_SELr_SET PLP_P1588_RX_MODE2_SELr_SET
#define P1588_RX_MODE2_SELr_GET PLP_P1588_RX_MODE2_SELr_GET
#define P1588_RX_MODE2_SELr_RX_MODE2f_GET PLP_P1588_RX_MODE2_SELr_RX_MODE2f_GET
#define P1588_RX_MODE2_SELr_RX_MODE2f_SET PLP_P1588_RX_MODE2_SELr_RX_MODE2f_SET
#define READ_P1588_RX_MODE2_SELr PLP_READ_P1588_RX_MODE2_SELr
#define WRITE_P1588_RX_MODE2_SELr PLP_WRITE_P1588_RX_MODE2_SELr
#define MODIFY_P1588_RX_MODE2_SELr PLP_MODIFY_P1588_RX_MODE2_SELr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_RX_MODE2_SELr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_RX_LINK_DELAY_SEL
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7005
 * DESC:     RX LINK DELAY SEL Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_LINK_DELAY    Link delay for Rx
 *
 ******************************************************************************/
#define PLP_P1588_RX_LINK_DELAY_SELr    0x00000005

#define PLP_P1588_RX_LINK_DELAY_SELr_SIZE 4

/*
 * This structure should be used to declare and program P1588_RX_LINK_DELAY_SEL.
 *
 */
typedef union PLP_P1588_RX_LINK_DELAY_SELr_s {
	uint32_t v[1];
	uint32_t p1588_rx_link_delay_sel[1];
	uint32_t _p1588_rx_link_delay_sel;
} PLP_P1588_RX_LINK_DELAY_SELr_t;

#define PLP_P1588_RX_LINK_DELAY_SELr_CLR(r) (r).p1588_rx_link_delay_sel[0] = 0
#define PLP_P1588_RX_LINK_DELAY_SELr_SET(r,d) (r).p1588_rx_link_delay_sel[0] = d
#define PLP_P1588_RX_LINK_DELAY_SELr_GET(r) (r).p1588_rx_link_delay_sel[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_RX_LINK_DELAY_SELr_RX_LINK_DELAYf_GET(r) (((r).p1588_rx_link_delay_sel[0]) & 0xffff)
#define PLP_P1588_RX_LINK_DELAY_SELr_RX_LINK_DELAYf_SET(r,f) (r).p1588_rx_link_delay_sel[0]=(((r).p1588_rx_link_delay_sel[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_RX_LINK_DELAY_SEL.
 *
 */
#define PLP_READ_P1588_RX_LINK_DELAY_SELr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_RX_LINK_DELAY_SELr,(_r._p1588_rx_link_delay_sel))
#define PLP_WRITE_P1588_RX_LINK_DELAY_SELr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_RX_LINK_DELAY_SELr,(_r._p1588_rx_link_delay_sel))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_RX_LINK_DELAY_SELr PLP_P1588_RX_LINK_DELAY_SELr
#define P1588_RX_LINK_DELAY_SELr_SIZE PLP_P1588_RX_LINK_DELAY_SELr_SIZE
typedef PLP_P1588_RX_LINK_DELAY_SELr_t P1588_RX_LINK_DELAY_SELr_t;
#define P1588_RX_LINK_DELAY_SELr_CLR PLP_P1588_RX_LINK_DELAY_SELr_CLR
#define P1588_RX_LINK_DELAY_SELr_SET PLP_P1588_RX_LINK_DELAY_SELr_SET
#define P1588_RX_LINK_DELAY_SELr_GET PLP_P1588_RX_LINK_DELAY_SELr_GET
#define P1588_RX_LINK_DELAY_SELr_RX_LINK_DELAYf_GET PLP_P1588_RX_LINK_DELAY_SELr_RX_LINK_DELAYf_GET
#define P1588_RX_LINK_DELAY_SELr_RX_LINK_DELAYf_SET PLP_P1588_RX_LINK_DELAY_SELr_RX_LINK_DELAYf_SET
#define READ_P1588_RX_LINK_DELAY_SELr PLP_READ_P1588_RX_LINK_DELAY_SELr
#define WRITE_P1588_RX_LINK_DELAY_SELr PLP_WRITE_P1588_RX_LINK_DELAY_SELr
#define MODIFY_P1588_RX_LINK_DELAY_SELr PLP_MODIFY_P1588_RX_LINK_DELAY_SELr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_RX_LINK_DELAY_SELr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_RX_LINK_DELAY_MSB_SEL
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7006
 * DESC:     RX LINK DELAY MSB SEL Register
 * RESETVAL: 0xa (10)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_LINK_DELAY_MSB Link delay MSB for Rx
 *
 ******************************************************************************/
#define PLP_P1588_RX_LINK_DELAY_MSB_SELr    0x00000006

#define PLP_P1588_RX_LINK_DELAY_MSB_SELr_SIZE 4

/*
 * This structure should be used to declare and program P1588_RX_LINK_DELAY_MSB_SEL.
 *
 */
typedef union PLP_P1588_RX_LINK_DELAY_MSB_SELr_s {
	uint32_t v[1];
	uint32_t p1588_rx_link_delay_msb_sel[1];
	uint32_t _p1588_rx_link_delay_msb_sel;
} PLP_P1588_RX_LINK_DELAY_MSB_SELr_t;

#define PLP_P1588_RX_LINK_DELAY_MSB_SELr_CLR(r) (r).p1588_rx_link_delay_msb_sel[0] = 0
#define PLP_P1588_RX_LINK_DELAY_MSB_SELr_SET(r,d) (r).p1588_rx_link_delay_msb_sel[0] = d
#define PLP_P1588_RX_LINK_DELAY_MSB_SELr_GET(r) (r).p1588_rx_link_delay_msb_sel[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_RX_LINK_DELAY_MSB_SELr_RX_LINK_DELAY_MSBf_GET(r) (((r).p1588_rx_link_delay_msb_sel[0]) & 0xffff)
#define PLP_P1588_RX_LINK_DELAY_MSB_SELr_RX_LINK_DELAY_MSBf_SET(r,f) (r).p1588_rx_link_delay_msb_sel[0]=(((r).p1588_rx_link_delay_msb_sel[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_RX_LINK_DELAY_MSB_SEL.
 *
 */
#define PLP_READ_P1588_RX_LINK_DELAY_MSB_SELr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_RX_LINK_DELAY_MSB_SELr,(_r._p1588_rx_link_delay_msb_sel))
#define PLP_WRITE_P1588_RX_LINK_DELAY_MSB_SELr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_RX_LINK_DELAY_MSB_SELr,(_r._p1588_rx_link_delay_msb_sel))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_RX_LINK_DELAY_MSB_SELr PLP_P1588_RX_LINK_DELAY_MSB_SELr
#define P1588_RX_LINK_DELAY_MSB_SELr_SIZE PLP_P1588_RX_LINK_DELAY_MSB_SELr_SIZE
typedef PLP_P1588_RX_LINK_DELAY_MSB_SELr_t P1588_RX_LINK_DELAY_MSB_SELr_t;
#define P1588_RX_LINK_DELAY_MSB_SELr_CLR PLP_P1588_RX_LINK_DELAY_MSB_SELr_CLR
#define P1588_RX_LINK_DELAY_MSB_SELr_SET PLP_P1588_RX_LINK_DELAY_MSB_SELr_SET
#define P1588_RX_LINK_DELAY_MSB_SELr_GET PLP_P1588_RX_LINK_DELAY_MSB_SELr_GET
#define P1588_RX_LINK_DELAY_MSB_SELr_RX_LINK_DELAY_MSBf_GET PLP_P1588_RX_LINK_DELAY_MSB_SELr_RX_LINK_DELAY_MSBf_GET
#define P1588_RX_LINK_DELAY_MSB_SELr_RX_LINK_DELAY_MSBf_SET PLP_P1588_RX_LINK_DELAY_MSB_SELr_RX_LINK_DELAY_MSBf_SET
#define READ_P1588_RX_LINK_DELAY_MSB_SELr PLP_READ_P1588_RX_LINK_DELAY_MSB_SELr
#define WRITE_P1588_RX_LINK_DELAY_MSB_SELr PLP_WRITE_P1588_RX_LINK_DELAY_MSB_SELr
#define MODIFY_P1588_RX_LINK_DELAY_MSB_SELr PLP_MODIFY_P1588_RX_LINK_DELAY_MSB_SELr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_RX_LINK_DELAY_MSB_SELr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TXRX_SOP_TS_CAPTURE_ENABLE
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7007
 * DESC:     TXRX SOP TS CAPTURE ENABLE Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_TS_CAP        Enable the timestamp capture on RX
 *     RX_TS_CAP        Enable the timestamp capture on RX
 *
 ******************************************************************************/
#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr    0x00000007

#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_SIZE 4

/*
 * This structure should be used to declare and program P1588_TXRX_SOP_TS_CAPTURE_ENABLE.
 *
 */
typedef union PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_s {
    uint32_t v[1];
    uint32_t p1588_txrx_sop_ts_capture_enable[1];
    uint32_t _p1588_txrx_sop_ts_capture_enable;
} PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_t;

#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_CLR(r) (r).p1588_txrx_sop_ts_capture_enable[0] = 0
#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_SET(r,d) (r).p1588_txrx_sop_ts_capture_enable[0] = d
#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_GET(r) (r).p1588_txrx_sop_ts_capture_enable[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TXRX_UPDATE_DP_TSf_GET(r) ((((r).p1588_txrx_sop_ts_capture_enable[0]) >> 7) & 0x3)
#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TXRX_UPDATE_DP_TSf_SET(r,f) (r).p1588_txrx_sop_ts_capture_enable[0]=(((r).p1588_txrx_sop_ts_capture_enable[0] & ~((uint32_t)0x3 << 7)) | ((((uint32_t)f) & 0x3) << 7)) | (3 << (16 + 7))

#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_UPDATE_DP_TSf_GET(r) ((((r).p1588_txrx_sop_ts_capture_enable[0]) >> 8) & 0x1)
#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_UPDATE_DP_TSf_SET(r,f) (r).p1588_txrx_sop_ts_capture_enable[0]=(((r).p1588_txrx_sop_ts_capture_enable[0] & ~((uint32_t)0x1 << 8)) | ((((uint32_t)f) & 0x1) << 8)) | (1 << (16 + 8))
#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_UPDATE_DP_TSf_GET(r) ((((r).p1588_txrx_sop_ts_capture_enable[0]) >> 7) & 0x1)
#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_UPDATE_DP_TSf_SET(r,f) (r).p1588_txrx_sop_ts_capture_enable[0]=(((r).p1588_txrx_sop_ts_capture_enable[0] & ~((uint32_t)0x1 << 7)) | ((((uint32_t)f) & 0x1) << 7)) | (1 << (16 + 7))
#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_ECCERR_ENAf_GET(r) ((((r).p1588_txrx_sop_ts_capture_enable[0]) >> 6) & 0x1)
#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_ECCERR_ENAf_SET(r,f) (r).p1588_txrx_sop_ts_capture_enable[0]=(((r).p1588_txrx_sop_ts_capture_enable[0] & ~((uint32_t)0x1 << 6)) | ((((uint32_t)f) & 0x1) << 6)) | (1 << (16 + 6))
#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_PM_P1588_ENAf_GET(r) ((((r).p1588_txrx_sop_ts_capture_enable[0]) >> 5) & 0x1)
#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_PM_P1588_ENAf_SET(r,f) (r).p1588_txrx_sop_ts_capture_enable[0]=(((r).p1588_txrx_sop_ts_capture_enable[0] & ~((uint32_t)0x1 << 5)) | ((((uint32_t)f) & 0x1) << 5)) | (1 << (16 + 5))
#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_PM_P1588_ENAf_GET(r) ((((r).p1588_txrx_sop_ts_capture_enable[0]) >> 4) & 0x1)
#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_PM_P1588_ENAf_SET(r,f) (r).p1588_txrx_sop_ts_capture_enable[0]=(((r).p1588_txrx_sop_ts_capture_enable[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4)) | (1 << (16 + 4))
#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_P1588_BYPASSf_GET(r) ((((r).p1588_txrx_sop_ts_capture_enable[0]) >> 3) & 0x1)
#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_P1588_BYPASSf_SET(r,f) (r).p1588_txrx_sop_ts_capture_enable[0]=(((r).p1588_txrx_sop_ts_capture_enable[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3)) | (1 << (16 + 3))
#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_TS_LOV_ENAf_GET(r) ((((r).p1588_txrx_sop_ts_capture_enable[0]) >> 2) & 0x1)
#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_TS_LOV_ENAf_SET(r,f) (r).p1588_txrx_sop_ts_capture_enable[0]=(((r).p1588_txrx_sop_ts_capture_enable[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2)) | (1 << (16 + 2))
#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_TS_CAPf_GET(r) ((((r).p1588_txrx_sop_ts_capture_enable[0]) >> 1) & 0x1)
#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_TS_CAPf_SET(r,f) (r).p1588_txrx_sop_ts_capture_enable[0]=(((r).p1588_txrx_sop_ts_capture_enable[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))
#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_TS_CAPf_GET(r) (((r).p1588_txrx_sop_ts_capture_enable[0]) & 0x1)
#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_TS_CAPf_SET(r,f) (r).p1588_txrx_sop_ts_capture_enable[0]=(((r).p1588_txrx_sop_ts_capture_enable[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1)) | (0x1 << 16)
#define PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TXRX_TS_CAPf_SET(r,f) (r).p1588_txrx_sop_ts_capture_enable[0]=(((r).p1588_txrx_sop_ts_capture_enable[0] & ~((uint32_t)0x3)) | (((uint32_t)f) & 0x3))

/*
 * These macros can be used to access P1588_TXRX_SOP_TS_CAPTURE_ENABLE.
 *
 */
#define PLP_READ_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr,(_r._p1588_txrx_sop_ts_capture_enable))
#define PLP_WRITE_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr,(_r._p1588_txrx_sop_ts_capture_enable))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr
#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_SIZE PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_SIZE
typedef PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_t P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_t;
#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_CLR PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_CLR
#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_SET PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_SET
#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_GET PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_GET
#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_UPDATE_DP_TSf_GET PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_UPDATE_DP_TSf_GET
#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_UPDATE_DP_TSf_SET PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_UPDATE_DP_TSf_SET
#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_UPDATE_DP_TSf_GET PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_UPDATE_DP_TSf_GET
#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_UPDATE_DP_TSf_SET PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_UPDATE_DP_TSf_SET
#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_ECCERR_ENAf_GET PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_ECCERR_ENAf_GET
#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_ECCERR_ENAf_SET PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_ECCERR_ENAf_SET
#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_PM_P1588_ENAf_GET PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_PM_P1588_ENAf_GET
#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_PM_P1588_ENAf_SET PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_PM_P1588_ENAf_SET
#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_PM_P1588_ENAf_GET PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_PM_P1588_ENAf_GET
#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_PM_P1588_ENAf_SET PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_PM_P1588_ENAf_SET
#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_P1588_BYPASSf_GET PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_P1588_BYPASSf_GET
#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_P1588_BYPASSf_SET PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_P1588_BYPASSf_SET
#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_TS_LOV_ENAf_GET PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_TS_LOV_ENAf_GET
#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_TS_LOV_ENAf_SET PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_TS_LOV_ENAf_SET
#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_TS_CAPf_GET PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_TS_CAPf_GET
#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_TS_CAPf_SET PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_TS_CAPf_SET
#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_TS_CAPf_GET PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_TS_CAPf_GET
#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_TS_CAPf_SET PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_TS_CAPf_SET
#define P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TXRX_TS_CAPf_SET PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TXRX_TS_CAPf_SET
#define READ_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr PLP_READ_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr
#define WRITE_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr PLP_WRITE_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr
#define MODIFY_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr PLP_MODIFY_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TX_OPTION_SEL
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7008
 * DESC:     TX OPTION SEL Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_OPTION        [7:3] - Reserved for future use[4] : 1- Capture TS into SOPmem regardless packet CRC error or not0- Capture TS into SOPmem only if no packet CRC error[2] : 1- Disable the TX PTP version 2 check0- Enable the TX PTP version 2 check[1] : 1- Add timecode into the insertion field0- Do not Add timecode into the insertion field[0] : 1- Keep the original CRC on Tx0- Update CRC on Tx
 *
 ******************************************************************************/
#define PLP_P1588_TX_OPTION_SELr    0x00000008

#define PLP_P1588_TX_OPTION_SELr_SIZE 4

/*
 * This structure should be used to declare and program P1588_TX_OPTION_SEL.
 *
 */
typedef union PLP_P1588_TX_OPTION_SELr_s {
	uint32_t v[1];
	uint32_t p1588_tx_option_sel[1];
	uint32_t _p1588_tx_option_sel;
} PLP_P1588_TX_OPTION_SELr_t;

#define PLP_P1588_TX_OPTION_SELr_CLR(r) (r).p1588_tx_option_sel[0] = 0
#define PLP_P1588_TX_OPTION_SELr_SET(r,d) (r).p1588_tx_option_sel[0] = d
#define PLP_P1588_TX_OPTION_SELr_GET(r) (r).p1588_tx_option_sel[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TX_OPTION_SELr_TX_OPTIONf_GET(r) (((r).p1588_tx_option_sel[0]) & 0xff)
#define PLP_P1588_TX_OPTION_SELr_TX_OPTIONf_SET(r,f) (r).p1588_tx_option_sel[0]=(((r).p1588_tx_option_sel[0] & ~((uint32_t)0xff)) | (((uint32_t)f) & 0xff)) | (0xff << 16)

#define P1588_TX_OPTION_SELr_TX_OPTION_3_0f_GET(r) (((r).p1588_tx_option_sel[0]) & 0xf)
#define P1588_TX_OPTION_SELr_TX_OPTION_3_0f_SET(r,f) (r).p1588_tx_option_sel[0]=(((r).p1588_tx_option_sel[0] & ~((uint32_t)0xf)) | (((uint32_t)f) & 0xf)) | (0xf << 16)

#define P1588_TX_OPTION_SELr_TX_OPTION_PTPv2p1_CHK_DISf_GET(r) ((((r).p1588_tx_option_sel[0]) >> 6) & 0x1)
#define P1588_TX_OPTION_SELr_TX_OPTION_PTPv2p1_CHK_DISf_SET(r,f) (r).p1588_tx_option_sel[0]=(((r).p1588_tx_option_sel[0] & ~((uint32_t)0x1 << 6)) | ((((uint32_t)f) & 0x1) << 6)) | (1 << (16 + 6))
#define P1588_TX_OPTION_SELr_TX_OPTION_TS_TO_SOPMEMf_GET(r) ((((r).p1588_tx_option_sel[0]) >> 4) & 0x1)
#define P1588_TX_OPTION_SELr_TX_OPTION_TS_TO_SOPMEMf_SET(r,f) (r).p1588_tx_option_sel[0]=(((r).p1588_tx_option_sel[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4)) | (1 << (16 + 4))
#define P1588_TX_OPTION_SELr_TX_OPTION_80BIT_TIMER_TCf_GET(r) ((((r).p1588_tx_option_sel[0]) >> 3) & 0x1)
#define P1588_TX_OPTION_SELr_TX_OPTION_80BIT_TIMER_TCf_SET(r,f) (r).p1588_tx_option_sel[0]=(((r).p1588_tx_option_sel[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3)) | (1 << (16 + 3))
#define P1588_TX_OPTION_SELr_TX_OPTION_PTPv2_CHK_DISf_GET(r) ((((r).p1588_tx_option_sel[0]) >> 2) & 0x1)
#define P1588_TX_OPTION_SELr_TX_OPTION_PTPv2_CHK_DISf_SET(r,f) (r).p1588_tx_option_sel[0]=(((r).p1588_tx_option_sel[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2)) | (1 << (16 + 2))
#define P1588_TX_OPTION_SELr_TX_OPTION_TC_TO_INSf_GET(r) ((((r).p1588_tx_option_sel[0]) >> 1) & 0x1)
#define P1588_TX_OPTION_SELr_TX_OPTION_TC_TO_INSf_SET(r,f) (r).p1588_tx_option_sel[0]=(((r).p1588_tx_option_sel[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))
#define P1588_TX_OPTION_SELr_TX_OPTION_KEEP_CRCf_GET(r) ((((r).p1588_tx_option_sel[0]) >> 0) & 0x1)
#define P1588_TX_OPTION_SELr_TX_OPTION_KEEP_CRCf_SET(r,f) (r).p1588_tx_option_sel[0]=(((r).p1588_tx_option_sel[0] & ~((uint32_t)0x1 << 0)) | ((((uint32_t)f) & 0x1) << 0)) | (1 << (16 + 0))

/*
 * These macros can be used to access P1588_TX_OPTION_SEL.
 *
 */
#define PLP_READ_P1588_TX_OPTION_SELr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TX_OPTION_SELr,(_r._p1588_tx_option_sel))
#define PLP_WRITE_P1588_TX_OPTION_SELr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TX_OPTION_SELr,(_r._p1588_tx_option_sel))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TX_OPTION_SELr PLP_P1588_TX_OPTION_SELr
#define P1588_TX_OPTION_SELr_SIZE PLP_P1588_TX_OPTION_SELr_SIZE
typedef PLP_P1588_TX_OPTION_SELr_t P1588_TX_OPTION_SELr_t;
#define P1588_TX_OPTION_SELr_CLR PLP_P1588_TX_OPTION_SELr_CLR
#define P1588_TX_OPTION_SELr_SET PLP_P1588_TX_OPTION_SELr_SET
#define P1588_TX_OPTION_SELr_GET PLP_P1588_TX_OPTION_SELr_GET
#define P1588_TX_OPTION_SELr_TX_OPTIONf_GET PLP_P1588_TX_OPTION_SELr_TX_OPTIONf_GET
#define P1588_TX_OPTION_SELr_TX_OPTIONf_SET PLP_P1588_TX_OPTION_SELr_TX_OPTIONf_SET
#define READ_P1588_TX_OPTION_SELr PLP_READ_P1588_TX_OPTION_SELr
#define WRITE_P1588_TX_OPTION_SELr PLP_WRITE_P1588_TX_OPTION_SELr
#define MODIFY_P1588_TX_OPTION_SELr PLP_MODIFY_P1588_TX_OPTION_SELr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TX_OPTION_SELr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_RX_OPTION_SEL
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7009
 * DESC:     RX OPTION SEL Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_OPTION        [7:3] - Reserved for future use[2] : 1- Disable the RX PTP version 2 check0- Enable the RX PTP version 2 check[1] : 1- Add timecode into the insertion field0- Do not Add timecode into the insertion field[0] : 1- Keep the original CRC on Rx0- Update CRC on Rx
 *
 ******************************************************************************/
#define PLP_P1588_RX_OPTION_SELr    0x00000009

#define PLP_P1588_RX_OPTION_SELr_SIZE 4

/*
 * This structure should be used to declare and program P1588_RX_OPTION_SEL.
 *
 */
typedef union PLP_P1588_RX_OPTION_SELr_s {
	uint32_t v[1];
	uint32_t p1588_rx_option_sel[1];
	uint32_t _p1588_rx_option_sel;
} PLP_P1588_RX_OPTION_SELr_t;

#define PLP_P1588_RX_OPTION_SELr_CLR(r) (r).p1588_rx_option_sel[0] = 0
#define PLP_P1588_RX_OPTION_SELr_SET(r,d) (r).p1588_rx_option_sel[0] = d
#define PLP_P1588_RX_OPTION_SELr_GET(r) (r).p1588_rx_option_sel[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_RX_OPTION_SELr_RX_OPTIONf_GET(r) (((r).p1588_rx_option_sel[0]) & 0xff)
#define PLP_P1588_RX_OPTION_SELr_RX_OPTIONf_SET(r,f) (r).p1588_rx_option_sel[0]=(((r).p1588_rx_option_sel[0] & ~((uint32_t)0xff)) | (((uint32_t)f) & 0xff)) | (0xff << 16)

#define P1588_RX_OPTION_SELr_RX_OPTION_3_0f_GET(r) (((r).p1588_rx_option_sel[0]) & 0xf)
#define P1588_RX_OPTION_SELr_RX_OPTION_3_0f_SET(r,f) (r).p1588_rx_option_sel[0]=(((r).p1588_rx_option_sel[0] & ~((uint32_t)0xf)) | (((uint32_t)f) & 0xf)) | (0xf << 16)

#define P1588_RX_OPTION_SELr_RX_OPTION_PTPv2p1_CHK_DISf_GET(r) ((((r).p1588_rx_option_sel[0]) >> 6) & 0x1)
#define P1588_RX_OPTION_SELr_RX_OPTION_PTPv2p1_CHK_DISf_SET(r,f) (r).p1588_rx_option_sel[0]=(((r).p1588_rx_option_sel[0] & ~((uint32_t)0x1 << 6)) | ((((uint32_t)f) & 0x1) << 6)) | (1 << (16 + 6))
#define P1588_RX_OPTION_SELr_RX_OPTION_TS_TO_SOPMEMf_GET(r) ((((r).p1588_rx_option_sel[0]) >> 4) & 0x1)
#define P1588_RX_OPTION_SELr_RX_OPTION_TS_TO_SOPMEMf_SET(r,f) (r).p1588_rx_option_sel[0]=(((r).p1588_rx_option_sel[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4)) | (1 << (16 + 4))
#define P1588_RX_OPTION_SELr_RX_OPTION_80BIT_TIMER_TCf_GET(r) ((((r).p1588_rx_option_sel[0]) >> 3) & 0x1)
#define P1588_RX_OPTION_SELr_RX_OPTION_80BIT_TIMER_TCf_SET(r,f) (r).p1588_rx_option_sel[0]=(((r).p1588_rx_option_sel[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3)) | (1 << (16 + 3))
#define P1588_RX_OPTION_SELr_RX_OPTION_PTPv2_CHK_DISf_GET(r) ((((r).p1588_rx_option_sel[0]) >> 2) & 0x1)
#define P1588_RX_OPTION_SELr_RX_OPTION_PTPv2_CHK_DISf_SET(r,f) (r).p1588_rx_option_sel[0]=(((r).p1588_rx_option_sel[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2)) | (1 << (16 + 2))
#define P1588_RX_OPTION_SELr_RX_OPTION_TC_TO_INSf_GET(r) ((((r).p1588_rx_option_sel[0]) >> 1) & 0x1)
#define P1588_RX_OPTION_SELr_RX_OPTION_TC_TO_INSf_SET(r,f) (r).p1588_rx_option_sel[0]=(((r).p1588_rx_option_sel[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))
#define P1588_RX_OPTION_SELr_RX_OPTION_KEEP_CRCf_GET(r) ((((r).p1588_rx_option_sel[0]) >> 0) & 0x1)
#define P1588_RX_OPTION_SELr_RX_OPTION_KEEP_CRCf_SET(r,f) (r).p1588_rx_option_sel[0]=(((r).p1588_rx_option_sel[0] & ~((uint32_t)0x1 << 0)) | ((((uint32_t)f) & 0x1) << 0)) | (1 << (16 + 0))

/*
 * These macros can be used to access P1588_RX_OPTION_SEL.
 *
 */
#define PLP_READ_P1588_RX_OPTION_SELr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_RX_OPTION_SELr,(_r._p1588_rx_option_sel))
#define PLP_WRITE_P1588_RX_OPTION_SELr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_RX_OPTION_SELr,(_r._p1588_rx_option_sel))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_RX_OPTION_SELr PLP_P1588_RX_OPTION_SELr
#define P1588_RX_OPTION_SELr_SIZE PLP_P1588_RX_OPTION_SELr_SIZE
typedef PLP_P1588_RX_OPTION_SELr_t P1588_RX_OPTION_SELr_t;
#define P1588_RX_OPTION_SELr_CLR PLP_P1588_RX_OPTION_SELr_CLR
#define P1588_RX_OPTION_SELr_SET PLP_P1588_RX_OPTION_SELr_SET
#define P1588_RX_OPTION_SELr_GET PLP_P1588_RX_OPTION_SELr_GET
#define P1588_RX_OPTION_SELr_RX_OPTIONf_GET PLP_P1588_RX_OPTION_SELr_RX_OPTIONf_GET
#define P1588_RX_OPTION_SELr_RX_OPTIONf_SET PLP_P1588_RX_OPTION_SELr_RX_OPTIONf_SET
#define READ_P1588_RX_OPTION_SELr PLP_READ_P1588_RX_OPTION_SELr
#define WRITE_P1588_RX_OPTION_SELr PLP_WRITE_P1588_RX_OPTION_SELr
#define MODIFY_P1588_RX_OPTION_SELr PLP_MODIFY_P1588_RX_OPTION_SELr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_RX_OPTION_SELr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TX_TS_OFFSET
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x700a
 * DESC:     TXRX TS OFFSET Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TS_OFFSET_TX0_11_0 Tx Timestamp offset registerThe unit is signed ns. This register compensatesthe delay of AFE. The Tx timestamp is given byNCO timestamp + {ts_offset_tx0_msb,ts_offset_tx0}
 *     TS_OFFSET_TX0_15_12 Tx Timestamp offset registerThe unit is signed 2^16 ns. This register compensatesthe delay of AFE. The Rx timestamp is given byNCO timestamp + {ts_offset_tx0_15_12,ts_offset_rx0}
 *
 ******************************************************************************/
#define PLP_P1588_TX_TS_OFFSETr    0x0000000a

#define PLP_P1588_TX_TS_OFFSETr_SIZE 4

/*
 * This structure should be used to declare and program P1588_TX_TS_OFFSET.
 *
 */
typedef union PLP_P1588_TX_TS_OFFSETr_s {
	uint32_t v[1];
	uint32_t P1588_TX_TS_OFFSET[1];
	uint32_t _P1588_TX_TS_OFFSET;
} PLP_P1588_TX_TS_OFFSETr_t;

#define PLP_P1588_TX_TS_OFFSETr_CLR(r) (r).P1588_TX_TS_OFFSET[0] = 0
#define PLP_P1588_TX_TS_OFFSETr_SET(r,d) (r).P1588_TX_TS_OFFSET[0] = d
#define PLP_P1588_TX_TS_OFFSETr_GET(r) (r).P1588_TX_TS_OFFSET[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TX_TS_OFFSETr_TS_OFFSET_TX0_15_12f_GET(r) ((((r).P1588_TX_TS_OFFSET[0]) >> 12) & 0xf)
#define PLP_P1588_TX_TS_OFFSETr_TS_OFFSET_TX0_15_12f_SET(r,f) (r).P1588_TX_TS_OFFSET[0]=(((r).P1588_TX_TS_OFFSET[0] & ~((uint32_t)0xf << 12)) | ((((uint32_t)f) & 0xf) << 12)) | (15 << (16 + 12))
#define PLP_P1588_TX_TS_OFFSETr_TS_OFFSET_TX0_11_0f_GET(r) (((r).P1588_TX_TS_OFFSET[0]) & 0xfff)
#define PLP_P1588_TX_TS_OFFSETr_TS_OFFSET_TX0_11_0f_SET(r,f) (r).P1588_TX_TS_OFFSET[0]=(((r).P1588_TX_TS_OFFSET[0] & ~((uint32_t)0xfff)) | (((uint32_t)f) & 0xfff)) | (0xfff << 16)

/*
 * These macros can be used to access P1588_TX_TS_OFFSET.
 *
 */
#define PLP_READ_P1588_TX_TS_OFFSETr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TX_TS_OFFSETr,(_r._P1588_TX_TS_OFFSET))
#define PLP_WRITE_P1588_TX_TS_OFFSETr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TX_TS_OFFSETr,(_r._P1588_TX_TS_OFFSET))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TX_TS_OFFSETr PLP_P1588_TX_TS_OFFSETr
#define P1588_TX_TS_OFFSETr_SIZE PLP_P1588_TX_TS_OFFSETr_SIZE
typedef PLP_P1588_TX_TS_OFFSETr_t P1588_TX_TS_OFFSETr_t;
#define P1588_TX_TS_OFFSETr_CLR PLP_P1588_TX_TS_OFFSETr_CLR
#define P1588_TX_TS_OFFSETr_SET PLP_P1588_TX_TS_OFFSETr_SET
#define P1588_TX_TS_OFFSETr_GET PLP_P1588_TX_TS_OFFSETr_GET
#define P1588_TX_TS_OFFSETr_TS_OFFSET_TX0_15_12f_GET PLP_P1588_TX_TS_OFFSETr_TS_OFFSET_TX0_15_12f_GET
#define P1588_TX_TS_OFFSETr_TS_OFFSET_TX0_15_12f_SET PLP_P1588_TX_TS_OFFSETr_TS_OFFSET_TX0_15_12f_SET
#define P1588_TX_TS_OFFSETr_TS_OFFSET_TX0_11_0f_GET PLP_P1588_TX_TS_OFFSETr_TS_OFFSET_TX0_11_0f_GET
#define P1588_TX_TS_OFFSETr_TS_OFFSET_TX0_11_0f_SET PLP_P1588_TX_TS_OFFSETr_TS_OFFSET_TX0_11_0f_SET
#define READ_P1588_TX_TS_OFFSETr PLP_READ_P1588_TX_TS_OFFSETr
#define WRITE_P1588_TX_TS_OFFSETr PLP_WRITE_P1588_TX_TS_OFFSETr
#define MODIFY_P1588_TX_TS_OFFSETr PLP_MODIFY_P1588_TX_TS_OFFSETr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TX_TS_OFFSETr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_RX_TS_OFFSET
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x700b
 * DESC:     RX TS OFFSET Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TS_OFFSET_RX0    Rx Timestamp offset registerThe unit is signed ns. This register compensatesthe delay of AFE. The Rx timestamp is given byNCO timestamp + {ts_offset_rx0_msb,ts_offset_rx0}
 *
 ******************************************************************************/
#define PLP_P1588_RX_TS_OFFSETr    0x0000000b

#define PLP_P1588_RX_TS_OFFSETr_SIZE 4

/*
 * This structure should be used to declare and program P1588_RX_TS_OFFSET.
 *
 */
typedef union PLP_P1588_RX_TS_OFFSETr_s {
	uint32_t v[1];
	uint32_t p1588_rx_ts_offset[1];
	uint32_t _p1588_rx_ts_offset;
} PLP_P1588_RX_TS_OFFSETr_t;

#define PLP_P1588_RX_TS_OFFSETr_CLR(r) (r).p1588_rx_ts_offset[0] = 0
#define PLP_P1588_RX_TS_OFFSETr_SET(r,d) (r).p1588_rx_ts_offset[0] = d
#define PLP_P1588_RX_TS_OFFSETr_GET(r) (r).p1588_rx_ts_offset[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_RX_TS_OFFSETr_TS_OFFSET_RX0f_GET(r) (((r).p1588_rx_ts_offset[0]) & 0xffff)
#define PLP_P1588_RX_TS_OFFSETr_TS_OFFSET_RX0f_SET(r,f) (r).p1588_rx_ts_offset[0]=(((r).p1588_rx_ts_offset[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_RX_TS_OFFSET.
 *
 */
#define PLP_READ_P1588_RX_TS_OFFSETr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_RX_TS_OFFSETr,(_r._p1588_rx_ts_offset))
#define PLP_WRITE_P1588_RX_TS_OFFSETr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_RX_TS_OFFSETr,(_r._p1588_rx_ts_offset))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_RX_TS_OFFSETr PLP_P1588_RX_TS_OFFSETr
#define P1588_RX_TS_OFFSETr_SIZE PLP_P1588_RX_TS_OFFSETr_SIZE
typedef PLP_P1588_RX_TS_OFFSETr_t P1588_RX_TS_OFFSETr_t;
#define P1588_RX_TS_OFFSETr_CLR PLP_P1588_RX_TS_OFFSETr_CLR
#define P1588_RX_TS_OFFSETr_SET PLP_P1588_RX_TS_OFFSETr_SET
#define P1588_RX_TS_OFFSETr_GET PLP_P1588_RX_TS_OFFSETr_GET
#define P1588_RX_TS_OFFSETr_TS_OFFSET_RX0f_GET PLP_P1588_RX_TS_OFFSETr_TS_OFFSET_RX0f_GET
#define P1588_RX_TS_OFFSETr_TS_OFFSET_RX0f_SET PLP_P1588_RX_TS_OFFSETr_TS_OFFSET_RX0f_SET
#define READ_P1588_RX_TS_OFFSETr PLP_READ_P1588_RX_TS_OFFSETr
#define WRITE_P1588_RX_TS_OFFSETr PLP_WRITE_P1588_RX_TS_OFFSETr
#define MODIFY_P1588_RX_TS_OFFSETr PLP_MODIFY_P1588_RX_TS_OFFSETr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_RX_TS_OFFSETr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TIME_CODE_1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x700c
 * DESC:     TIME CODE 1 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_TIME_CODE_79_64 Original time code value that will beused in egress port for Sync, Delay_Req andPdelay_Req messages. covers bits 79:64.
 *
 ******************************************************************************/
#define PLP_P1588_TIME_CODE_1r    0x0000000c

#define PLP_P1588_TIME_CODE_1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_TIME_CODE_1.
 *
 */
typedef union PLP_P1588_TIME_CODE_1r_s {
	uint32_t v[1];
	uint32_t p1588_time_code_1[1];
	uint32_t _p1588_time_code_1;
} PLP_P1588_TIME_CODE_1r_t;

#define PLP_P1588_TIME_CODE_1r_CLR(r) (r).p1588_time_code_1[0] = 0
#define PLP_P1588_TIME_CODE_1r_SET(r,d) (r).p1588_time_code_1[0] = d
#define PLP_P1588_TIME_CODE_1r_GET(r) (r).p1588_time_code_1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TIME_CODE_1r_NSE_REG_TIME_CODE_79_64f_GET(r) (((r).p1588_time_code_1[0]) & 0xffff)
#define PLP_P1588_TIME_CODE_1r_NSE_REG_TIME_CODE_79_64f_SET(r,f) (r).p1588_time_code_1[0]=(((r).p1588_time_code_1[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TIME_CODE_1.
 *
 */
#define PLP_READ_P1588_TIME_CODE_1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TIME_CODE_1r,(_r._p1588_time_code_1))
#define PLP_WRITE_P1588_TIME_CODE_1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TIME_CODE_1r,(_r._p1588_time_code_1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TIME_CODE_1r PLP_P1588_TIME_CODE_1r
#define P1588_TIME_CODE_1r_SIZE PLP_P1588_TIME_CODE_1r_SIZE
typedef PLP_P1588_TIME_CODE_1r_t P1588_TIME_CODE_1r_t;
#define P1588_TIME_CODE_1r_CLR PLP_P1588_TIME_CODE_1r_CLR
#define P1588_TIME_CODE_1r_SET PLP_P1588_TIME_CODE_1r_SET
#define P1588_TIME_CODE_1r_GET PLP_P1588_TIME_CODE_1r_GET
#define P1588_TIME_CODE_1r_NSE_REG_TIME_CODE_79_64f_GET PLP_P1588_TIME_CODE_1r_NSE_REG_TIME_CODE_79_64f_GET
#define P1588_TIME_CODE_1r_NSE_REG_TIME_CODE_79_64f_SET PLP_P1588_TIME_CODE_1r_NSE_REG_TIME_CODE_79_64f_SET
#define READ_P1588_TIME_CODE_1r PLP_READ_P1588_TIME_CODE_1r
#define WRITE_P1588_TIME_CODE_1r PLP_WRITE_P1588_TIME_CODE_1r
#define MODIFY_P1588_TIME_CODE_1r PLP_MODIFY_P1588_TIME_CODE_1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TIME_CODE_1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TIME_CODE_2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x700d
 * DESC:     TIME CODE 2 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_TIME_CODE_63_48 Original time code value that will beused in egress port for Sync, Delay_Req andPdelay_Req messages. covers bits 63:48.
 *
 ******************************************************************************/
#define PLP_P1588_TIME_CODE_2r    0x0000000d

#define PLP_P1588_TIME_CODE_2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_TIME_CODE_2.
 *
 */
typedef union PLP_P1588_TIME_CODE_2r_s {
	uint32_t v[1];
	uint32_t p1588_time_code_2[1];
	uint32_t _p1588_time_code_2;
} PLP_P1588_TIME_CODE_2r_t;

#define PLP_P1588_TIME_CODE_2r_CLR(r) (r).p1588_time_code_2[0] = 0
#define PLP_P1588_TIME_CODE_2r_SET(r,d) (r).p1588_time_code_2[0] = d
#define PLP_P1588_TIME_CODE_2r_GET(r) (r).p1588_time_code_2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TIME_CODE_2r_NSE_REG_TIME_CODE_63_48f_GET(r) (((r).p1588_time_code_2[0]) & 0xffff)
#define PLP_P1588_TIME_CODE_2r_NSE_REG_TIME_CODE_63_48f_SET(r,f) (r).p1588_time_code_2[0]=(((r).p1588_time_code_2[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TIME_CODE_2.
 *
 */
#define PLP_READ_P1588_TIME_CODE_2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TIME_CODE_2r,(_r._p1588_time_code_2))
#define PLP_WRITE_P1588_TIME_CODE_2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TIME_CODE_2r,(_r._p1588_time_code_2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TIME_CODE_2r PLP_P1588_TIME_CODE_2r
#define P1588_TIME_CODE_2r_SIZE PLP_P1588_TIME_CODE_2r_SIZE
typedef PLP_P1588_TIME_CODE_2r_t P1588_TIME_CODE_2r_t;
#define P1588_TIME_CODE_2r_CLR PLP_P1588_TIME_CODE_2r_CLR
#define P1588_TIME_CODE_2r_SET PLP_P1588_TIME_CODE_2r_SET
#define P1588_TIME_CODE_2r_GET PLP_P1588_TIME_CODE_2r_GET
#define P1588_TIME_CODE_2r_NSE_REG_TIME_CODE_63_48f_GET PLP_P1588_TIME_CODE_2r_NSE_REG_TIME_CODE_63_48f_GET
#define P1588_TIME_CODE_2r_NSE_REG_TIME_CODE_63_48f_SET PLP_P1588_TIME_CODE_2r_NSE_REG_TIME_CODE_63_48f_SET
#define READ_P1588_TIME_CODE_2r PLP_READ_P1588_TIME_CODE_2r
#define WRITE_P1588_TIME_CODE_2r PLP_WRITE_P1588_TIME_CODE_2r
#define MODIFY_P1588_TIME_CODE_2r PLP_MODIFY_P1588_TIME_CODE_2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TIME_CODE_2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TIME_CODE_3
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x700e
 * DESC:     TIME CODE 3 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_TIME_CODE_47_32 Original time code value that will beused in egress port for Sync, Delay_Req andPdelay_Req messages. covers bits 47:32.
 *
 ******************************************************************************/
#define PLP_P1588_TIME_CODE_3r    0x0000000e

#define PLP_P1588_TIME_CODE_3r_SIZE 4

/*
 * This structure should be used to declare and program P1588_TIME_CODE_3.
 *
 */
typedef union PLP_P1588_TIME_CODE_3r_s {
	uint32_t v[1];
	uint32_t p1588_time_code_3[1];
	uint32_t _p1588_time_code_3;
} PLP_P1588_TIME_CODE_3r_t;

#define PLP_P1588_TIME_CODE_3r_CLR(r) (r).p1588_time_code_3[0] = 0
#define PLP_P1588_TIME_CODE_3r_SET(r,d) (r).p1588_time_code_3[0] = d
#define PLP_P1588_TIME_CODE_3r_GET(r) (r).p1588_time_code_3[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TIME_CODE_3r_NSE_REG_TIME_CODE_47_32f_GET(r) (((r).p1588_time_code_3[0]) & 0xffff)
#define PLP_P1588_TIME_CODE_3r_NSE_REG_TIME_CODE_47_32f_SET(r,f) (r).p1588_time_code_3[0]=(((r).p1588_time_code_3[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TIME_CODE_3.
 *
 */
#define PLP_READ_P1588_TIME_CODE_3r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TIME_CODE_3r,(_r._p1588_time_code_3))
#define PLP_WRITE_P1588_TIME_CODE_3r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TIME_CODE_3r,(_r._p1588_time_code_3))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TIME_CODE_3r PLP_P1588_TIME_CODE_3r
#define P1588_TIME_CODE_3r_SIZE PLP_P1588_TIME_CODE_3r_SIZE
typedef PLP_P1588_TIME_CODE_3r_t P1588_TIME_CODE_3r_t;
#define P1588_TIME_CODE_3r_CLR PLP_P1588_TIME_CODE_3r_CLR
#define P1588_TIME_CODE_3r_SET PLP_P1588_TIME_CODE_3r_SET
#define P1588_TIME_CODE_3r_GET PLP_P1588_TIME_CODE_3r_GET
#define P1588_TIME_CODE_3r_NSE_REG_TIME_CODE_47_32f_GET PLP_P1588_TIME_CODE_3r_NSE_REG_TIME_CODE_47_32f_GET
#define P1588_TIME_CODE_3r_NSE_REG_TIME_CODE_47_32f_SET PLP_P1588_TIME_CODE_3r_NSE_REG_TIME_CODE_47_32f_SET
#define READ_P1588_TIME_CODE_3r PLP_READ_P1588_TIME_CODE_3r
#define WRITE_P1588_TIME_CODE_3r PLP_WRITE_P1588_TIME_CODE_3r
#define MODIFY_P1588_TIME_CODE_3r PLP_MODIFY_P1588_TIME_CODE_3r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TIME_CODE_3r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TIME_CODE_4
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x700f
 * DESC:     TIME CODE 4 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_TIME_CODE_31_16 Original time code value that will beused in egress port for Sync, Delay_Req andPdelay_Req messages. covers bits 31:16.
 *
 ******************************************************************************/
#define PLP_P1588_TIME_CODE_4r    0x0000000f

#define PLP_P1588_TIME_CODE_4r_SIZE 4

/*
 * This structure should be used to declare and program P1588_TIME_CODE_4.
 *
 */
typedef union PLP_P1588_TIME_CODE_4r_s {
	uint32_t v[1];
	uint32_t p1588_time_code_4[1];
	uint32_t _p1588_time_code_4;
} PLP_P1588_TIME_CODE_4r_t;

#define PLP_P1588_TIME_CODE_4r_CLR(r) (r).p1588_time_code_4[0] = 0
#define PLP_P1588_TIME_CODE_4r_SET(r,d) (r).p1588_time_code_4[0] = d
#define PLP_P1588_TIME_CODE_4r_GET(r) (r).p1588_time_code_4[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TIME_CODE_4r_NSE_REG_TIME_CODE_31_16f_GET(r) (((r).p1588_time_code_4[0]) & 0xffff)
#define PLP_P1588_TIME_CODE_4r_NSE_REG_TIME_CODE_31_16f_SET(r,f) (r).p1588_time_code_4[0]=(((r).p1588_time_code_4[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TIME_CODE_4.
 *
 */
#define PLP_READ_P1588_TIME_CODE_4r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TIME_CODE_4r,(_r._p1588_time_code_4))
#define PLP_WRITE_P1588_TIME_CODE_4r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TIME_CODE_4r,(_r._p1588_time_code_4))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TIME_CODE_4r PLP_P1588_TIME_CODE_4r
#define P1588_TIME_CODE_4r_SIZE PLP_P1588_TIME_CODE_4r_SIZE
typedef PLP_P1588_TIME_CODE_4r_t P1588_TIME_CODE_4r_t;
#define P1588_TIME_CODE_4r_CLR PLP_P1588_TIME_CODE_4r_CLR
#define P1588_TIME_CODE_4r_SET PLP_P1588_TIME_CODE_4r_SET
#define P1588_TIME_CODE_4r_GET PLP_P1588_TIME_CODE_4r_GET
#define P1588_TIME_CODE_4r_NSE_REG_TIME_CODE_31_16f_GET PLP_P1588_TIME_CODE_4r_NSE_REG_TIME_CODE_31_16f_GET
#define P1588_TIME_CODE_4r_NSE_REG_TIME_CODE_31_16f_SET PLP_P1588_TIME_CODE_4r_NSE_REG_TIME_CODE_31_16f_SET
#define READ_P1588_TIME_CODE_4r PLP_READ_P1588_TIME_CODE_4r
#define WRITE_P1588_TIME_CODE_4r PLP_WRITE_P1588_TIME_CODE_4r
#define MODIFY_P1588_TIME_CODE_4r PLP_MODIFY_P1588_TIME_CODE_4r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TIME_CODE_4r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TIME_CODE_5
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7010
 * DESC:     TIME CODE 5 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_TIME_CODE_15_0 Original time code value that will beused in egress port for Sync, Delay_Req andPdelay_Req messages. covers bits 15:0.
 *
 ******************************************************************************/
#define PLP_P1588_TIME_CODE_5r    0x00000010

#define PLP_P1588_TIME_CODE_5r_SIZE 4

/*
 * This structure should be used to declare and program P1588_TIME_CODE_5.
 *
 */
typedef union PLP_P1588_TIME_CODE_5r_s {
	uint32_t v[1];
	uint32_t p1588_time_code_5[1];
	uint32_t _p1588_time_code_5;
} PLP_P1588_TIME_CODE_5r_t;

#define PLP_P1588_TIME_CODE_5r_CLR(r) (r).p1588_time_code_5[0] = 0
#define PLP_P1588_TIME_CODE_5r_SET(r,d) (r).p1588_time_code_5[0] = d
#define PLP_P1588_TIME_CODE_5r_GET(r) (r).p1588_time_code_5[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TIME_CODE_5r_NSE_REG_TIME_CODE_15_0f_GET(r) (((r).p1588_time_code_5[0]) & 0xffff)
#define PLP_P1588_TIME_CODE_5r_NSE_REG_TIME_CODE_15_0f_SET(r,f) (r).p1588_time_code_5[0]=(((r).p1588_time_code_5[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TIME_CODE_5.
 *
 */
#define PLP_READ_P1588_TIME_CODE_5r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TIME_CODE_5r,(_r._p1588_time_code_5))
#define PLP_WRITE_P1588_TIME_CODE_5r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TIME_CODE_5r,(_r._p1588_time_code_5))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TIME_CODE_5r PLP_P1588_TIME_CODE_5r
#define P1588_TIME_CODE_5r_SIZE PLP_P1588_TIME_CODE_5r_SIZE
typedef PLP_P1588_TIME_CODE_5r_t P1588_TIME_CODE_5r_t;
#define P1588_TIME_CODE_5r_CLR PLP_P1588_TIME_CODE_5r_CLR
#define P1588_TIME_CODE_5r_SET PLP_P1588_TIME_CODE_5r_SET
#define P1588_TIME_CODE_5r_GET PLP_P1588_TIME_CODE_5r_GET
#define P1588_TIME_CODE_5r_NSE_REG_TIME_CODE_15_0f_GET PLP_P1588_TIME_CODE_5r_NSE_REG_TIME_CODE_15_0f_GET
#define P1588_TIME_CODE_5r_NSE_REG_TIME_CODE_15_0f_SET PLP_P1588_TIME_CODE_5r_NSE_REG_TIME_CODE_15_0f_SET
#define READ_P1588_TIME_CODE_5r PLP_READ_P1588_TIME_CODE_5r
#define WRITE_P1588_TIME_CODE_5r PLP_WRITE_P1588_TIME_CODE_5r
#define MODIFY_P1588_TIME_CODE_5r PLP_MODIFY_P1588_TIME_CODE_5r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TIME_CODE_5r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_DPLL_DB_1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7011
 * DESC:     DPLL DB 1 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     DPLL_DB_31_16    DPLL NCO 32bits or Delta LSB 32bUpper 16b of the value
 *
 ******************************************************************************/
#define PLP_P1588_DPLL_DB_1r    0x00000011

#define PLP_P1588_DPLL_DB_1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_DPLL_DB_1.
 *
 */
typedef union PLP_P1588_DPLL_DB_1r_s {
	uint32_t v[1];
	uint32_t p1588_dpll_db_1[1];
	uint32_t _p1588_dpll_db_1;
} PLP_P1588_DPLL_DB_1r_t;

#define PLP_P1588_DPLL_DB_1r_CLR(r) (r).p1588_dpll_db_1[0] = 0
#define PLP_P1588_DPLL_DB_1r_SET(r,d) (r).p1588_dpll_db_1[0] = d
#define PLP_P1588_DPLL_DB_1r_GET(r) (r).p1588_dpll_db_1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_DPLL_DB_1r_DPLL_DB_31_16f_GET(r) (((r).p1588_dpll_db_1[0]) & 0xffff)
#define PLP_P1588_DPLL_DB_1r_DPLL_DB_31_16f_SET(r,f) (r).p1588_dpll_db_1[0]=(((r).p1588_dpll_db_1[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_DPLL_DB_1.
 *
 */
#define PLP_READ_P1588_DPLL_DB_1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_DPLL_DB_1r,(_r._p1588_dpll_db_1))
#define PLP_WRITE_P1588_DPLL_DB_1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_DPLL_DB_1r,(_r._p1588_dpll_db_1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_DPLL_DB_1r PLP_P1588_DPLL_DB_1r
#define P1588_DPLL_DB_1r_SIZE PLP_P1588_DPLL_DB_1r_SIZE
typedef PLP_P1588_DPLL_DB_1r_t P1588_DPLL_DB_1r_t;
#define P1588_DPLL_DB_1r_CLR PLP_P1588_DPLL_DB_1r_CLR
#define P1588_DPLL_DB_1r_SET PLP_P1588_DPLL_DB_1r_SET
#define P1588_DPLL_DB_1r_GET PLP_P1588_DPLL_DB_1r_GET
#define P1588_DPLL_DB_1r_DPLL_DB_31_16f_GET PLP_P1588_DPLL_DB_1r_DPLL_DB_31_16f_GET
#define P1588_DPLL_DB_1r_DPLL_DB_31_16f_SET PLP_P1588_DPLL_DB_1r_DPLL_DB_31_16f_SET
#define READ_P1588_DPLL_DB_1r PLP_READ_P1588_DPLL_DB_1r
#define WRITE_P1588_DPLL_DB_1r PLP_WRITE_P1588_DPLL_DB_1r
#define MODIFY_P1588_DPLL_DB_1r PLP_MODIFY_P1588_DPLL_DB_1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_DPLL_DB_1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_DPLL_DB_2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7012
 * DESC:     DPLL DB 2 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     DPLL_DB_15_0     DPLL NCO 32bits or Delta LSB 32bLower 16b of the value
 *
 ******************************************************************************/
#define PLP_P1588_DPLL_DB_2r    0x00000012

#define PLP_P1588_DPLL_DB_2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_DPLL_DB_2.
 *
 */
typedef union PLP_P1588_DPLL_DB_2r_s {
	uint32_t v[1];
	uint32_t p1588_dpll_db_2[1];
	uint32_t _p1588_dpll_db_2;
} PLP_P1588_DPLL_DB_2r_t;

#define PLP_P1588_DPLL_DB_2r_CLR(r) (r).p1588_dpll_db_2[0] = 0
#define PLP_P1588_DPLL_DB_2r_SET(r,d) (r).p1588_dpll_db_2[0] = d
#define PLP_P1588_DPLL_DB_2r_GET(r) (r).p1588_dpll_db_2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_DPLL_DB_2r_DPLL_DB_15_0f_GET(r) (((r).p1588_dpll_db_2[0]) & 0xffff)
#define PLP_P1588_DPLL_DB_2r_DPLL_DB_15_0f_SET(r,f) (r).p1588_dpll_db_2[0]=(((r).p1588_dpll_db_2[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_DPLL_DB_2.
 *
 */
#define PLP_READ_P1588_DPLL_DB_2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_DPLL_DB_2r,(_r._p1588_dpll_db_2))
#define PLP_WRITE_P1588_DPLL_DB_2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_DPLL_DB_2r,(_r._p1588_dpll_db_2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_DPLL_DB_2r PLP_P1588_DPLL_DB_2r
#define P1588_DPLL_DB_2r_SIZE PLP_P1588_DPLL_DB_2r_SIZE
typedef PLP_P1588_DPLL_DB_2r_t P1588_DPLL_DB_2r_t;
#define P1588_DPLL_DB_2r_CLR PLP_P1588_DPLL_DB_2r_CLR
#define P1588_DPLL_DB_2r_SET PLP_P1588_DPLL_DB_2r_SET
#define P1588_DPLL_DB_2r_GET PLP_P1588_DPLL_DB_2r_GET
#define P1588_DPLL_DB_2r_DPLL_DB_15_0f_GET PLP_P1588_DPLL_DB_2r_DPLL_DB_15_0f_GET
#define P1588_DPLL_DB_2r_DPLL_DB_15_0f_SET PLP_P1588_DPLL_DB_2r_DPLL_DB_15_0f_SET
#define READ_P1588_DPLL_DB_2r PLP_READ_P1588_DPLL_DB_2r
#define WRITE_P1588_DPLL_DB_2r PLP_WRITE_P1588_DPLL_DB_2r
#define MODIFY_P1588_DPLL_DB_2r PLP_MODIFY_P1588_DPLL_DB_2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_DPLL_DB_2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_DPLL_DEBUG_SEL
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7013
 * DESC:     DPLL DEBUG SEL Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     DPLL_DB_SEL      Select the DPLL NCO 32b or Delta LSB 32b0 - Delta Phase[31:0]1 - Loopfilter MSB 32b2 - Delta Phase[47:16]3 - Loopfilter LSB 32b4 - Timestamp[31:0]5 - Timestamp[47:16]6 - Timestamp[15:0], Ref Phase[15:0]7 - Timestamp[31:16], Ref Phase[31:16]8 - Timestamp[47:32], Ref Phase[47:32]
 *     FREQ_MDIO_SEL    1 - Use nco_freqcntrl_reg as adder input for 48-bit and 80-bit timers0 - Use dpll output as adder input
 *
 ******************************************************************************/
#define PLP_P1588_DPLL_DEBUG_SELr    0x00000013

#define PLP_P1588_DPLL_DEBUG_SELr_SIZE 4

/*
 * This structure should be used to declare and program P1588_DPLL_DEBUG_SEL.
 *
 */
typedef union PLP_P1588_DPLL_DEBUG_SELr_s {
	uint32_t v[1];
	uint32_t p1588_dpll_debug_sel[1];
	uint32_t _p1588_dpll_debug_sel;
} PLP_P1588_DPLL_DEBUG_SELr_t;

#define PLP_P1588_DPLL_DEBUG_SELr_CLR(r) (r).p1588_dpll_debug_sel[0] = 0
#define PLP_P1588_DPLL_DEBUG_SELr_SET(r,d) (r).p1588_dpll_debug_sel[0] = d
#define PLP_P1588_DPLL_DEBUG_SELr_GET(r) (r).p1588_dpll_debug_sel[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_DPLL_DEBUG_SELr_FREQ_MDIO_SELf_GET(r) ((((r).p1588_dpll_debug_sel[0]) >> 5) & 0x1)
#define PLP_P1588_DPLL_DEBUG_SELr_FREQ_MDIO_SELf_SET(r,f) (r).p1588_dpll_debug_sel[0]=(((r).p1588_dpll_debug_sel[0] & ~((uint32_t)0x1 << 5)) | ((((uint32_t)f) & 0x1) << 5)) | (1 << (16 + 5))
#define PLP_P1588_DPLL_DEBUG_SELr_DPLL_DB_SELf_GET(r) (((r).p1588_dpll_debug_sel[0]) & 0x1f)
#define PLP_P1588_DPLL_DEBUG_SELr_DPLL_DB_SELf_SET(r,f) (r).p1588_dpll_debug_sel[0]=(((r).p1588_dpll_debug_sel[0] & ~((uint32_t)0x1f)) | (((uint32_t)f) & 0x1f)) | (0x1f << 16)

/*
 * These macros can be used to access P1588_DPLL_DEBUG_SEL.
 *
 */
#define PLP_READ_P1588_DPLL_DEBUG_SELr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_DPLL_DEBUG_SELr,(_r._p1588_dpll_debug_sel))
#define PLP_WRITE_P1588_DPLL_DEBUG_SELr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_DPLL_DEBUG_SELr,(_r._p1588_dpll_debug_sel))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_DPLL_DEBUG_SELr PLP_P1588_DPLL_DEBUG_SELr
#define P1588_DPLL_DEBUG_SELr_SIZE PLP_P1588_DPLL_DEBUG_SELr_SIZE
typedef PLP_P1588_DPLL_DEBUG_SELr_t P1588_DPLL_DEBUG_SELr_t;
#define P1588_DPLL_DEBUG_SELr_CLR PLP_P1588_DPLL_DEBUG_SELr_CLR
#define P1588_DPLL_DEBUG_SELr_SET PLP_P1588_DPLL_DEBUG_SELr_SET
#define P1588_DPLL_DEBUG_SELr_GET PLP_P1588_DPLL_DEBUG_SELr_GET
#define P1588_DPLL_DEBUG_SELr_FREQ_MDIO_SELf_GET PLP_P1588_DPLL_DEBUG_SELr_FREQ_MDIO_SELf_GET
#define P1588_DPLL_DEBUG_SELr_FREQ_MDIO_SELf_SET PLP_P1588_DPLL_DEBUG_SELr_FREQ_MDIO_SELf_SET
#define P1588_DPLL_DEBUG_SELr_DPLL_DB_SELf_GET PLP_P1588_DPLL_DEBUG_SELr_DPLL_DB_SELf_GET
#define P1588_DPLL_DEBUG_SELr_DPLL_DB_SELf_SET PLP_P1588_DPLL_DEBUG_SELr_DPLL_DB_SELf_SET
#define READ_P1588_DPLL_DEBUG_SELr PLP_READ_P1588_DPLL_DEBUG_SELr
#define WRITE_P1588_DPLL_DEBUG_SELr PLP_WRITE_P1588_DPLL_DEBUG_SELr
#define MODIFY_P1588_DPLL_DEBUG_SELr PLP_MODIFY_P1588_DPLL_DEBUG_SELr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_DPLL_DEBUG_SELr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_REGS_SHADOW_CONTROL_1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7014
 * DESC:     SHADOW REG CONTROL 1 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_CTL_31_16 [15] -  Reserved for future[14] -  Reserved for future[13] -  Reserved for future[12] -  1 = reset 64b NTP Control load bit[11] -  1 = reset Timestamp Correction control bit[10] -  1 = reset 80b Timecode control load bit[9]  -  1 = reset Syncout control load bit[8]  -  1 = reset NCO divider control load bit[7]  -  1 = reset 48b Local time control load bit[6]  -  1 = reset NCO 48b Frequency control load bit[5]  -  1 = reset DPLL Loop Filter control bit[4]  -  1 = reset DPLL Ref Phase control load bit[3]  -  1 = reset DPLL Ref Phase Delta control load bit[2]  -  1 = reset DPLL K3 control load bit[1]  -  1 = reset DPLL K2 control load bit[0]  -  1 = reset DPLL K1 control load bitNOTE: THESE BITS ARE SC TYPE but RTL IMPLEMENTS TO BEHAVE AS WO TYPE.NEED TO CLEAR MANUALY BY MDIO TRANSACTION
 *
 ******************************************************************************/
#define PLP_P1588_REGS_SHADOW_CONTROL_1r    0x00000014

#define PLP_P1588_REGS_SHADOW_CONTROL_1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_REGS_SHADOW_CONTROL_1.
 *
 */
typedef union PLP_P1588_REGS_SHADOW_CONTROL_1r_s {
	uint32_t v[1];
	uint32_t p1588_regs_shadow_control_1[1];
	uint32_t _p1588_regs_shadow_control_1;
} PLP_P1588_REGS_SHADOW_CONTROL_1r_t;

#define PLP_P1588_REGS_SHADOW_CONTROL_1r_CLR(r) (r).p1588_regs_shadow_control_1[0] = 0
#define PLP_P1588_REGS_SHADOW_CONTROL_1r_SET(r,d) (r).p1588_regs_shadow_control_1[0] = d
#define PLP_P1588_REGS_SHADOW_CONTROL_1r_GET(r) (r).p1588_regs_shadow_control_1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_REGS_SHADOW_CONTROL_1r_NSE_REG_CTL_31_16f_GET(r) (((r).p1588_regs_shadow_control_1[0]) & 0xffff)
#define PLP_P1588_REGS_SHADOW_CONTROL_1r_NSE_REG_CTL_31_16f_SET(r,f) (r).p1588_regs_shadow_control_1[0]=(((r).p1588_regs_shadow_control_1[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_REGS_SHADOW_CONTROL_1.
 *
 */
#define PLP_READ_P1588_REGS_SHADOW_CONTROL_1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_REGS_SHADOW_CONTROL_1r,(_r._p1588_regs_shadow_control_1))
#define PLP_WRITE_P1588_REGS_SHADOW_CONTROL_1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_REGS_SHADOW_CONTROL_1r,(_r._p1588_regs_shadow_control_1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_REGS_SHADOW_CONTROL_1r PLP_P1588_REGS_SHADOW_CONTROL_1r
#define P1588_REGS_SHADOW_CONTROL_1r_SIZE PLP_P1588_REGS_SHADOW_CONTROL_1r_SIZE
typedef PLP_P1588_REGS_SHADOW_CONTROL_1r_t P1588_REGS_SHADOW_CONTROL_1r_t;
#define P1588_REGS_SHADOW_CONTROL_1r_CLR PLP_P1588_REGS_SHADOW_CONTROL_1r_CLR
#define P1588_REGS_SHADOW_CONTROL_1r_SET PLP_P1588_REGS_SHADOW_CONTROL_1r_SET
#define P1588_REGS_SHADOW_CONTROL_1r_GET PLP_P1588_REGS_SHADOW_CONTROL_1r_GET
#define P1588_REGS_SHADOW_CONTROL_1r_NSE_REG_CTL_31_16f_GET PLP_P1588_REGS_SHADOW_CONTROL_1r_NSE_REG_CTL_31_16f_GET
#define P1588_REGS_SHADOW_CONTROL_1r_NSE_REG_CTL_31_16f_SET PLP_P1588_REGS_SHADOW_CONTROL_1r_NSE_REG_CTL_31_16f_SET
#define READ_P1588_REGS_SHADOW_CONTROL_1r PLP_READ_P1588_REGS_SHADOW_CONTROL_1r
#define WRITE_P1588_REGS_SHADOW_CONTROL_1r PLP_WRITE_P1588_REGS_SHADOW_CONTROL_1r
#define MODIFY_P1588_REGS_SHADOW_CONTROL_1r PLP_MODIFY_P1588_REGS_SHADOW_CONTROL_1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_REGS_SHADOW_CONTROL_1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_REGS_SHADOW_CONTROL_2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7015
 * DESC:     SHADOW REG CONTROL 2 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_CTL_15_0 [15] -  Reserved for future[14] -  Reserved for future[13] -  Reserved for future[12] -  1 = set 64b NTP Control load bit[11] -  1 = set Timestamp Correction control load bit[10] -  1 = set 80b Timecode control load bit[9]  -  1 = set Syncout control load bit[8]  -  1 = set NCO divider control load bit[7]  -  1 = set 48b Local time control load bit[6]  -  1 = set NCO 48b Frequency control load bit[5]  -  1 = set DPLL Loop Filter control load bit[4]  -  1 = set DPLL Ref Phase control load bit[3]  -  1 = set DPLL Ref Phase Delta control load bit[2]  -  1 = set DPLL K3 control load bit[1]  -  1 = set DPLL K2 control load bit[0]  -  1 = set DPLL K1 control load bitNOTE: THESE BITS ARE SELF-CLEARING AND NEED TO BEHANDLED OUTSIDE THE REGISTER BLOCK
 *
 ******************************************************************************/
#define PLP_P1588_REGS_SHADOW_CONTROL_2r    0x00000015

#define PLP_P1588_REGS_SHADOW_CONTROL_2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_REGS_SHADOW_CONTROL_2.
 *
 */
typedef union PLP_P1588_REGS_SHADOW_CONTROL_2r_s {
	uint32_t v[1];
	uint32_t p1588_regs_shadow_control_2[1];
	uint32_t _p1588_regs_shadow_control_2;
} PLP_P1588_REGS_SHADOW_CONTROL_2r_t;

#define PLP_P1588_REGS_SHADOW_CONTROL_2r_CLR(r) (r).p1588_regs_shadow_control_2[0] = 0
#define PLP_P1588_REGS_SHADOW_CONTROL_2r_SET(r,d) (r).p1588_regs_shadow_control_2[0] = d
#define PLP_P1588_REGS_SHADOW_CONTROL_2r_GET(r) (r).p1588_regs_shadow_control_2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_REGS_SHADOW_CONTROL_2r_NSE_REG_CTL_15_0f_GET(r) (((r).p1588_regs_shadow_control_2[0]) & 0xffff)
#define PLP_P1588_REGS_SHADOW_CONTROL_2r_NSE_REG_CTL_15_0f_SET(r,f) (r).p1588_regs_shadow_control_2[0]=(((r).p1588_regs_shadow_control_2[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_REGS_SHADOW_CONTROL_2.
 *
 */
#define PLP_READ_P1588_REGS_SHADOW_CONTROL_2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_REGS_SHADOW_CONTROL_2r,(_r._p1588_regs_shadow_control_2))
#define PLP_WRITE_P1588_REGS_SHADOW_CONTROL_2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_REGS_SHADOW_CONTROL_2r,(_r._p1588_regs_shadow_control_2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_REGS_SHADOW_CONTROL_2r PLP_P1588_REGS_SHADOW_CONTROL_2r
#define P1588_REGS_SHADOW_CONTROL_2r_SIZE PLP_P1588_REGS_SHADOW_CONTROL_2r_SIZE
typedef PLP_P1588_REGS_SHADOW_CONTROL_2r_t P1588_REGS_SHADOW_CONTROL_2r_t;
#define P1588_REGS_SHADOW_CONTROL_2r_CLR PLP_P1588_REGS_SHADOW_CONTROL_2r_CLR
#define P1588_REGS_SHADOW_CONTROL_2r_SET PLP_P1588_REGS_SHADOW_CONTROL_2r_SET
#define P1588_REGS_SHADOW_CONTROL_2r_GET PLP_P1588_REGS_SHADOW_CONTROL_2r_GET
#define P1588_REGS_SHADOW_CONTROL_2r_NSE_REG_CTL_15_0f_GET PLP_P1588_REGS_SHADOW_CONTROL_2r_NSE_REG_CTL_15_0f_GET
#define P1588_REGS_SHADOW_CONTROL_2r_NSE_REG_CTL_15_0f_SET PLP_P1588_REGS_SHADOW_CONTROL_2r_NSE_REG_CTL_15_0f_SET
#define READ_P1588_REGS_SHADOW_CONTROL_2r PLP_READ_P1588_REGS_SHADOW_CONTROL_2r
#define WRITE_P1588_REGS_SHADOW_CONTROL_2r PLP_WRITE_P1588_REGS_SHADOW_CONTROL_2r
#define MODIFY_P1588_REGS_SHADOW_CONTROL_2r PLP_MODIFY_P1588_REGS_SHADOW_CONTROL_2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_REGS_SHADOW_CONTROL_2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_INTERRUPT_MASK
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7016
 * DESC:     INTERRUPT MASK Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     INTC_FSYNC_MASK  Framesync Timestamp interrupt maskUser need to disable TS_CAPTURE register
 *     INTC_SOP_MASK    Packet Timestamp interrupt maskUser need to disable RX_CAP and RX_CAP registers also
 *     INTC_MASK_7_2    Bits 7:3 are reserved for future useBits 2 is PCH sync-in interrupt mask enable
 *
 ******************************************************************************/
#define PLP_P1588_INTERRUPT_MASKr    0x00000016

#define PLP_P1588_INTERRUPT_MASKr_SIZE 4

/*
 * This structure should be used to declare and program P1588_INTERRUPT_MASK.
 *
 */
typedef union PLP_P1588_INTERRUPT_MASKr_s {
	uint32_t v[1];
	uint32_t p1588_interrupt_mask[1];
	uint32_t _p1588_interrupt_mask;
} PLP_P1588_INTERRUPT_MASKr_t;

#define PLP_P1588_INTERRUPT_MASKr_CLR(r) (r).p1588_interrupt_mask[0] = 0
#define PLP_P1588_INTERRUPT_MASKr_SET(r,d) (r).p1588_interrupt_mask[0] = d
#define PLP_P1588_INTERRUPT_MASKr_GET(r) (r).p1588_interrupt_mask[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_INTERRUPT_MASKr_INTC_MASK_7_2f_GET(r) ((((r).p1588_interrupt_mask[0]) >> 2) & 0x3f)
#define PLP_P1588_INTERRUPT_MASKr_INTC_MASK_7_2f_SET(r,f) (r).p1588_interrupt_mask[0]=(((r).p1588_interrupt_mask[0] & ~((uint32_t)0x3f << 2)) | ((((uint32_t)f) & 0x3f) << 2)) | (63 << (16 + 2))
#define PLP_P1588_INTERRUPT_MASKr_INTC_IPG_MASKf_GET(r) ((((r).p1588_interrupt_mask[0]) >> 3) & 0x1)
#define PLP_P1588_INTERRUPT_MASKr_INTC_IPG_MASKf_SET(r,f) (r).p1588_interrupt_mask[0]=(((r).p1588_interrupt_mask[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3)) | (1 << (16 + 3))
#define PLP_P1588_INTERRUPT_MASKr_INTC_SYNCIN_MASKf_GET(r) ((((r).p1588_interrupt_mask[0]) >> 2) & 0x1)
#define PLP_P1588_INTERRUPT_MASKr_INTC_SYNCIN_MASKf_SET(r,f) (r).p1588_interrupt_mask[0]=(((r).p1588_interrupt_mask[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2)) | (1 << (16 + 2))
#define PLP_P1588_INTERRUPT_MASKr_INTC_SOP_MASKf_GET(r) ((((r).p1588_interrupt_mask[0]) >> 1) & 0x1)
#define PLP_P1588_INTERRUPT_MASKr_INTC_SOP_MASKf_SET(r,f) (r).p1588_interrupt_mask[0]=(((r).p1588_interrupt_mask[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))
#define PLP_P1588_INTERRUPT_MASKr_INTC_FSYNC_MASKf_GET(r) (((r).p1588_interrupt_mask[0]) & 0x1)
#define PLP_P1588_INTERRUPT_MASKr_INTC_FSYNC_MASKf_SET(r,f) (r).p1588_interrupt_mask[0]=(((r).p1588_interrupt_mask[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1)) | (0x1 << 16)

#define PLP_P1588_INTERRUPT_MASKr_INTC_IPG_PCH_SOP_FSYNC_SET(r,f) (r).p1588_interrupt_mask[0]=(((r).p1588_interrupt_mask[0] & ~((uint32_t)0xf)) | (((uint32_t)f) & 0xf))

/*
 * These macros can be used to access P1588_INTERRUPT_MASK.
 *
 */
#define PLP_READ_P1588_INTERRUPT_MASKr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_INTERRUPT_MASKr,(_r._p1588_interrupt_mask))
#define PLP_WRITE_P1588_INTERRUPT_MASKr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_INTERRUPT_MASKr,(_r._p1588_interrupt_mask))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_INTERRUPT_MASKr PLP_P1588_INTERRUPT_MASKr
#define P1588_INTERRUPT_MASKr_SIZE PLP_P1588_INTERRUPT_MASKr_SIZE
typedef PLP_P1588_INTERRUPT_MASKr_t P1588_INTERRUPT_MASKr_t;
#define P1588_INTERRUPT_MASKr_CLR PLP_P1588_INTERRUPT_MASKr_CLR
#define P1588_INTERRUPT_MASKr_SET PLP_P1588_INTERRUPT_MASKr_SET
#define P1588_INTERRUPT_MASKr_GET PLP_P1588_INTERRUPT_MASKr_GET
#define P1588_INTERRUPT_MASKr_INTC_MASK_7_2f_GET PLP_P1588_INTERRUPT_MASKr_INTC_MASK_7_2f_GET
#define P1588_INTERRUPT_MASKr_INTC_MASK_7_2f_SET PLP_P1588_INTERRUPT_MASKr_INTC_MASK_7_2f_SET
#define P1588_INTERRUPT_MASKr_INTC_IPG_MASKf_GET PLP_P1588_INTERRUPT_MASKr_INTC_IPG_MASKf_GET
#define P1588_INTERRUPT_MASKr_INTC_IPG_MASKf_SET PLP_P1588_INTERRUPT_MASKr_INTC_IPG_MASKf_SET
#define P1588_INTERRUPT_MASKr_INTC_SYNCIN_MASKf_GET PLP_P1588_INTERRUPT_MASKr_INTC_SYNCIN_MASKf_GET
#define P1588_INTERRUPT_MASKr_INTC_SYNCIN_MASKf_SET PLP_P1588_INTERRUPT_MASKr_INTC_SYNCIN_MASKf_SET
#define P1588_INTERRUPT_MASKr_INTC_SOP_MASKf_GET PLP_P1588_INTERRUPT_MASKr_INTC_SOP_MASKf_GET
#define P1588_INTERRUPT_MASKr_INTC_SOP_MASKf_SET PLP_P1588_INTERRUPT_MASKr_INTC_SOP_MASKf_SET
#define P1588_INTERRUPT_MASKr_INTC_FSYNC_MASKf_GET PLP_P1588_INTERRUPT_MASKr_INTC_FSYNC_MASKf_GET
#define P1588_INTERRUPT_MASKr_INTC_FSYNC_MASKf_SET PLP_P1588_INTERRUPT_MASKr_INTC_FSYNC_MASKf_SET
#define P1588_INTERRUPT_MASKr_INTC_IPG_PCH_SOP_FSYNC_SET PLP_P1588_INTERRUPT_MASKr_INTC_IPG_PCH_SOP_FSYNC_SET
#define READ_P1588_INTERRUPT_MASKr PLP_READ_P1588_INTERRUPT_MASKr
#define WRITE_P1588_INTERRUPT_MASKr PLP_WRITE_P1588_INTERRUPT_MASKr
#define MODIFY_P1588_INTERRUPT_MASKr PLP_MODIFY_P1588_INTERRUPT_MASKr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_INTERRUPT_MASKr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_INTERRUPT_STATUS
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7017
 * DESC:     INTERRUPT STATUS Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     INTC_STAT_0      Framesync Timestamp interrupt statusNOTE: THIS BIT IS CLEAR ON READ AND NEED TO BEHANDLED OUTSIDE THE REGISTER BLOCK
 *     INTC_STAT_1      Packet Timestamp interrupt statusNOTE: THIS BIT IS CLEAR ON READING and CLEAR THE ENTRY FROM SOPMEM
 *     INTC_STAT_7_2    Bits 7:3 are reserved for future useBits 2 is PCH sync-in interrupt
 *
 ******************************************************************************/
#define PLP_P1588_INTERRUPT_STATUSr    0x00000017

#define PLP_P1588_INTERRUPT_STATUSr_SIZE 4

/*
 * This structure should be used to declare and program P1588_INTERRUPT_STATUS.
 *
 */
typedef union PLP_P1588_INTERRUPT_STATUSr_s {
	uint32_t v[1];
	uint32_t p1588_interrupt_status[1];
	uint32_t _p1588_interrupt_status;
} PLP_P1588_INTERRUPT_STATUSr_t;

#define PLP_P1588_INTERRUPT_STATUSr_CLR(r) (r).p1588_interrupt_status[0] = 0
#define PLP_P1588_INTERRUPT_STATUSr_SET(r,d) (r).p1588_interrupt_status[0] = d
#define PLP_P1588_INTERRUPT_STATUSr_GET(r) (r).p1588_interrupt_status[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_INTERRUPT_STATUSr_INTC_STAT_7_2f_GET(r) ((((r).p1588_interrupt_status[0]) >> 2) & 0x3f)
#define PLP_P1588_INTERRUPT_STATUSr_INTC_STAT_7_2f_SET(r,f) (r).p1588_interrupt_status[0]=(((r).p1588_interrupt_status[0] & ~((uint32_t)0x3f << 2)) | ((((uint32_t)f) & 0x3f) << 2)) | (63 << (16 + 2))
#define PLP_P1588_INTERRUPT_STATUSr_INTC_STAT_1f_GET(r) ((((r).p1588_interrupt_status[0]) >> 1) & 0x1)
#define PLP_P1588_INTERRUPT_STATUSr_INTC_STAT_1f_SET(r,f) (r).p1588_interrupt_status[0]=(((r).p1588_interrupt_status[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))
#define PLP_P1588_INTERRUPT_STATUSr_INTC_STAT_0f_GET(r) (((r).p1588_interrupt_status[0]) & 0x1)
#define PLP_P1588_INTERRUPT_STATUSr_INTC_STAT_0f_SET(r,f) (r).p1588_interrupt_status[0]=(((r).p1588_interrupt_status[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1)) | (0x1 << 16)

#define PLP_P1588_INTERRUPT_STATUSr_INTC_IPG_STATf_GET(r) ((((r).p1588_interrupt_status[0]) >> 3) & 0x1)
#define PLP_P1588_INTERRUPT_STATUSr_INTC_IPG_STATf_SET(r,f) (r).p1588_interrupt_status[0]=(((r).p1588_interrupt_status[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3)) | (1 << (16 + 3))
#define PLP_P1588_INTERRUPT_STATUSr_INTC_SYNCIN_STATf_GET(r) ((((r).p1588_interrupt_status[0]) >> 2) & 0x1)
#define PLP_P1588_INTERRUPT_STATUSr_INTC_SYNCIN_STATf_SET(r,f) (r).p1588_interrupt_status[0]=(((r).p1588_interrupt_status[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2)) | (1 << (16 + 2))
#define PLP_P1588_INTERRUPT_STATUSr_INTC_SOP_STATf_GET(r) ((((r).p1588_interrupt_status[0]) >> 1) & 0x1)
#define PLP_P1588_INTERRUPT_STATUSr_INTC_SOP_STATf_SET(r,f) (r).p1588_interrupt_status[0]=(((r).p1588_interrupt_status[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))
#define PLP_P1588_INTERRUPT_STATUSr_INTC_FSYNC_STATf_GET(r) (((r).p1588_interrupt_status[0]) & 0x1)
#define PLP_P1588_INTERRUPT_STATUSr_INTC_FSYNC_STATf_SET(r,f) (r).p1588_interrupt_status[0]=(((r).p1588_interrupt_status[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1)) | (0x1 << 16)

/*
 * These macros can be used to access P1588_INTERRUPT_STATUS.
 *
 */
#define PLP_READ_P1588_INTERRUPT_STATUSr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_INTERRUPT_STATUSr,(_r._p1588_interrupt_status))
#define PLP_WRITE_P1588_INTERRUPT_STATUSr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_INTERRUPT_STATUSr,(_r._p1588_interrupt_status))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_INTERRUPT_STATUSr PLP_P1588_INTERRUPT_STATUSr
#define P1588_INTERRUPT_STATUSr_SIZE PLP_P1588_INTERRUPT_STATUSr_SIZE
typedef PLP_P1588_INTERRUPT_STATUSr_t P1588_INTERRUPT_STATUSr_t;
#define P1588_INTERRUPT_STATUSr_CLR PLP_P1588_INTERRUPT_STATUSr_CLR
#define P1588_INTERRUPT_STATUSr_SET PLP_P1588_INTERRUPT_STATUSr_SET
#define P1588_INTERRUPT_STATUSr_GET PLP_P1588_INTERRUPT_STATUSr_GET
#define P1588_INTERRUPT_STATUSr_INTC_STAT_7_2f_GET PLP_P1588_INTERRUPT_STATUSr_INTC_STAT_7_2f_GET
#define P1588_INTERRUPT_STATUSr_INTC_STAT_7_2f_SET PLP_P1588_INTERRUPT_STATUSr_INTC_STAT_7_2f_SET
#define P1588_INTERRUPT_STATUSr_INTC_STAT_1f_GET PLP_P1588_INTERRUPT_STATUSr_INTC_STAT_1f_GET
#define P1588_INTERRUPT_STATUSr_INTC_STAT_1f_SET PLP_P1588_INTERRUPT_STATUSr_INTC_STAT_1f_SET
#define P1588_INTERRUPT_STATUSr_INTC_STAT_0f_GET PLP_P1588_INTERRUPT_STATUSr_INTC_STAT_0f_GET
#define P1588_INTERRUPT_STATUSr_INTC_STAT_0f_SET PLP_P1588_INTERRUPT_STATUSr_INTC_STAT_0f_SET

#define P1588_INTERRUPT_STATUSr_INTC_IPG_STATf_GET PLP_P1588_INTERRUPT_STATUSr_INTC_IPG_STATf_GET
#define P1588_INTERRUPT_STATUSr_INTC_IPG_STATf_SET PLP_P1588_INTERRUPT_STATUSr_INTC_IPG_STATf_SET
#define P1588_INTERRUPT_STATUSr_INTC_SYNCIN_STATf_GET PLP_P1588_INTERRUPT_STATUSr_INTC_SYNCIN_STATf_GET
#define P1588_INTERRUPT_STATUSr_INTC_SYNCIN_STATf_SET PLP_P1588_INTERRUPT_STATUSr_INTC_SYNCIN_STATf_SET
#define P1588_INTERRUPT_STATUSr_INTC_SOP_STATf_GET PLP_P1588_INTERRUPT_STATUSr_INTC_SOP_STATf_GET
#define P1588_INTERRUPT_STATUSr_INTC_SOP_STATf_SET PLP_P1588_INTERRUPT_STATUSr_INTC_SOP_STATf_SET
#define P1588_INTERRUPT_STATUSr_INTC_FSYNC_STATf_GET PLP_P1588_INTERRUPT_STATUSr_INTC_FSYNC_STATf_GET
#define P1588_INTERRUPT_STATUSr_INTC_FSYNC_STATf_SET PLP_P1588_INTERRUPT_STATUSr_INTC_FSYNC_STATf_SET

#define READ_P1588_INTERRUPT_STATUSr PLP_READ_P1588_INTERRUPT_STATUSr
#define WRITE_P1588_INTERRUPT_STATUSr PLP_WRITE_P1588_INTERRUPT_STATUSr
#define MODIFY_P1588_INTERRUPT_STATUSr PLP_MODIFY_P1588_INTERRUPT_STATUSr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_INTERRUPT_STATUSr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TX_CONTROL
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7018
 * DESC:     TX CONTROL Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_L4_IPV6_UDP_EN Enables the 1588 L4/UDP IPV6 packet check on Tx
 *     TX_L4_IPV4_UDP_EN Enables the 1588 L4/UDP IPV4 packet check on Tx
 *     TX_L2_EN         Enables the 1588 L2 packet check on Tx
 *     TX_AS_EN         Enables the 802.1AS packet check on Tx
 *     TX_L4_IPV6_ADDRESS_EN Enables the L4 IPv6 DA check for 1588 message packet on Tx
 *     TX_L4_IP_ADDRESS_EN Enables the L4 IPv4 DA check for 1588 message packet on Tx
 *     TX_L2_DS_EN      Enables the L2 MAC DA check for 1588 message packet on Tx
 *     TX_AS_DS_EN      Enables the 802.1AS MAC DA check for 1588 message packet on Tx
 *
 ******************************************************************************/
#define PLP_P1588_TX_CONTROLr    0x00000018

#define PLP_P1588_TX_CONTROLr_SIZE 4

/*
 * This structure should be used to declare and program P1588_TX_CONTROL.
 *
 */
typedef union PLP_P1588_TX_CONTROLr_s {
	uint32_t v[1];
	uint32_t p1588_tx_control[1];
	uint32_t _p1588_tx_control;
} PLP_P1588_TX_CONTROLr_t;

#define PLP_P1588_TX_CONTROLr_CLR(r) (r).p1588_tx_control[0] = 0
#define PLP_P1588_TX_CONTROLr_SET(r,d) (r).p1588_tx_control[0] = d
#define PLP_P1588_TX_CONTROLr_GET(r) (r).p1588_tx_control[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TX_CONTROLr_TX_AS_DS_ENf_GET(r) ((((r).p1588_tx_control[0]) >> 7) & 0x1)
#define PLP_P1588_TX_CONTROLr_TX_AS_DS_ENf_SET(r,f) (r).p1588_tx_control[0]=(((r).p1588_tx_control[0] & ~((uint32_t)0x1 << 7)) | ((((uint32_t)f) & 0x1) << 7)) | (1 << (16 + 7))
#define PLP_P1588_TX_CONTROLr_TX_L2_DS_ENf_GET(r) ((((r).p1588_tx_control[0]) >> 6) & 0x1)
#define PLP_P1588_TX_CONTROLr_TX_L2_DS_ENf_SET(r,f) (r).p1588_tx_control[0]=(((r).p1588_tx_control[0] & ~((uint32_t)0x1 << 6)) | ((((uint32_t)f) & 0x1) << 6)) | (1 << (16 + 6))
#define PLP_P1588_TX_CONTROLr_TX_L4_IP_ADDRESS_ENf_GET(r) ((((r).p1588_tx_control[0]) >> 5) & 0x1)
#define PLP_P1588_TX_CONTROLr_TX_L4_IP_ADDRESS_ENf_SET(r,f) (r).p1588_tx_control[0]=(((r).p1588_tx_control[0] & ~((uint32_t)0x1 << 5)) | ((((uint32_t)f) & 0x1) << 5)) | (1 << (16 + 5))
#define PLP_P1588_TX_CONTROLr_TX_L4_IPV6_ADDRESS_ENf_GET(r) ((((r).p1588_tx_control[0]) >> 4) & 0x1)
#define PLP_P1588_TX_CONTROLr_TX_L4_IPV6_ADDRESS_ENf_SET(r,f) (r).p1588_tx_control[0]=(((r).p1588_tx_control[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4)) | (1 << (16 + 4))
#define PLP_P1588_TX_CONTROLr_TX_AS_ENf_GET(r) ((((r).p1588_tx_control[0]) >> 3) & 0x1)
#define PLP_P1588_TX_CONTROLr_TX_AS_ENf_SET(r,f) (r).p1588_tx_control[0]=(((r).p1588_tx_control[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3)) | (1 << (16 + 3))
#define PLP_P1588_TX_CONTROLr_TX_L2_ENf_GET(r) ((((r).p1588_tx_control[0]) >> 2) & 0x1)
#define PLP_P1588_TX_CONTROLr_TX_L2_ENf_SET(r,f) (r).p1588_tx_control[0]=(((r).p1588_tx_control[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2)) | (1 << (16 + 2))
#define PLP_P1588_TX_CONTROLr_TX_L4_IPV4_UDP_ENf_GET(r) ((((r).p1588_tx_control[0]) >> 1) & 0x1)
#define PLP_P1588_TX_CONTROLr_TX_L4_IPV4_UDP_ENf_SET(r,f) (r).p1588_tx_control[0]=(((r).p1588_tx_control[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))
#define PLP_P1588_TX_CONTROLr_TX_L4_IPV6_UDP_ENf_GET(r) (((r).p1588_tx_control[0]) & 0x1)
#define PLP_P1588_TX_CONTROLr_TX_L4_IPV6_UDP_ENf_SET(r,f) (r).p1588_tx_control[0]=(((r).p1588_tx_control[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1)) | (0x1 << 16)

#define PLP_P1588_TX_CONTROLr_TX_AS_L2_L4_ENf_SET(r,f) (r).p1588_tx_control[0]=(((r).p1588_tx_control[0] & ~((uint32_t)0xf)) | (((uint32_t)f) & 0xf))

/*
 * These macros can be used to access P1588_TX_CONTROL.
 *
 */
#define PLP_READ_P1588_TX_CONTROLr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TX_CONTROLr,(_r._p1588_tx_control))
#define PLP_WRITE_P1588_TX_CONTROLr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TX_CONTROLr,(_r._p1588_tx_control))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TX_CONTROLr PLP_P1588_TX_CONTROLr
#define P1588_TX_CONTROLr_SIZE PLP_P1588_TX_CONTROLr_SIZE
typedef PLP_P1588_TX_CONTROLr_t P1588_TX_CONTROLr_t;
#define P1588_TX_CONTROLr_CLR PLP_P1588_TX_CONTROLr_CLR
#define P1588_TX_CONTROLr_SET PLP_P1588_TX_CONTROLr_SET
#define P1588_TX_CONTROLr_GET PLP_P1588_TX_CONTROLr_GET
#define P1588_TX_CONTROLr_TX_AS_DS_ENf_GET PLP_P1588_TX_CONTROLr_TX_AS_DS_ENf_GET
#define P1588_TX_CONTROLr_TX_AS_DS_ENf_SET PLP_P1588_TX_CONTROLr_TX_AS_DS_ENf_SET
#define P1588_TX_CONTROLr_TX_L2_DS_ENf_GET PLP_P1588_TX_CONTROLr_TX_L2_DS_ENf_GET
#define P1588_TX_CONTROLr_TX_L2_DS_ENf_SET PLP_P1588_TX_CONTROLr_TX_L2_DS_ENf_SET
#define P1588_TX_CONTROLr_TX_L4_IP_ADDRESS_ENf_GET PLP_P1588_TX_CONTROLr_TX_L4_IP_ADDRESS_ENf_GET
#define P1588_TX_CONTROLr_TX_L4_IP_ADDRESS_ENf_SET PLP_P1588_TX_CONTROLr_TX_L4_IP_ADDRESS_ENf_SET
#define P1588_TX_CONTROLr_TX_L4_IPV6_ADDRESS_ENf_GET PLP_P1588_TX_CONTROLr_TX_L4_IPV6_ADDRESS_ENf_GET
#define P1588_TX_CONTROLr_TX_L4_IPV6_ADDRESS_ENf_SET PLP_P1588_TX_CONTROLr_TX_L4_IPV6_ADDRESS_ENf_SET
#define P1588_TX_CONTROLr_TX_AS_ENf_GET PLP_P1588_TX_CONTROLr_TX_AS_ENf_GET
#define P1588_TX_CONTROLr_TX_AS_ENf_SET PLP_P1588_TX_CONTROLr_TX_AS_ENf_SET
#define P1588_TX_CONTROLr_TX_L2_ENf_GET PLP_P1588_TX_CONTROLr_TX_L2_ENf_GET
#define P1588_TX_CONTROLr_TX_L2_ENf_SET PLP_P1588_TX_CONTROLr_TX_L2_ENf_SET
#define P1588_TX_CONTROLr_TX_L4_IPV4_UDP_ENf_GET PLP_P1588_TX_CONTROLr_TX_L4_IPV4_UDP_ENf_GET
#define P1588_TX_CONTROLr_TX_L4_IPV4_UDP_ENf_SET PLP_P1588_TX_CONTROLr_TX_L4_IPV4_UDP_ENf_SET
#define P1588_TX_CONTROLr_TX_L4_IPV6_UDP_ENf_GET PLP_P1588_TX_CONTROLr_TX_L4_IPV6_UDP_ENf_GET
#define P1588_TX_CONTROLr_TX_L4_IPV6_UDP_ENf_SET PLP_P1588_TX_CONTROLr_TX_L4_IPV6_UDP_ENf_SET
#define P1588_TX_CONTROLr_TX_AS_L2_L4_ENf_SET PLP_P1588_TX_CONTROLr_TX_AS_L2_L4_ENf_SET
#define READ_P1588_TX_CONTROLr PLP_READ_P1588_TX_CONTROLr
#define WRITE_P1588_TX_CONTROLr PLP_WRITE_P1588_TX_CONTROLr
#define MODIFY_P1588_TX_CONTROLr PLP_MODIFY_P1588_TX_CONTROLr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TX_CONTROLr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TX_DEBUG
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7019
 * DESC:     TX DEBUG Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_OFFSET_0      0 - Enable the correction field update1 - Disable the correction field update
 *     TX_OFFSET_1      0 - Checksum computation of 48b TS and 16b Checksum without one's complement1 - One's complement checksum computation of 48b TS and 16b Checksum
 *     TX_OFFSET_2      0 - Update the UDP Checksum to the new computed value1 - Force UDP Checksum to 0
 *     TX_OFFSET_3      0 - Update UDP Checksum1 - Disable the update of UDP Checksum
 *     TX_OFFSET_4      0 - Ignore the 1588 event message type check1 - Perform the 1588 event message type check
 *     TX_OFFSET_5      0 - Update CF, ignore the overflow1 - Update CF to 64'h7FFFFFFF_FFFFFFFF when an overflow occurs
 *     TX_OFFSET_7_6    Select which 16b of MAC DA address is read out to MDIO00 - Select MAC DA[15:0] to be read out to MDIO01 - Select MAC DA[31:16] to be read out to MDIO10 - Select MAC DA[47:32] to be read out to MDIO11 - Select MAC DA[15:0] to be read out to MDIO
 *     TX_CS_DIS        0 - Enable Checksum update1 - Disable Checksum update
 *
 ******************************************************************************/
#define PLP_P1588_TX_DEBUGr    0x00000019

#define PLP_P1588_TX_DEBUGr_SIZE 4

/*
 * This structure should be used to declare and program P1588_TX_DEBUG.
 *
 */
typedef union PLP_P1588_TX_DEBUGr_s {
	uint32_t v[1];
	uint32_t p1588_tx_debug[1];
	uint32_t _p1588_tx_debug;
} PLP_P1588_TX_DEBUGr_t;

#define PLP_P1588_TX_DEBUGr_CLR(r) (r).p1588_tx_debug[0] = 0
#define PLP_P1588_TX_DEBUGr_SET(r,d) (r).p1588_tx_debug[0] = d
#define PLP_P1588_TX_DEBUGr_GET(r) (r).p1588_tx_debug[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TX_DEBUGr_TX_CS_DISf_GET(r) ((((r).p1588_tx_debug[0]) >> 8) & 0x1)
#define PLP_P1588_TX_DEBUGr_TX_CS_DISf_SET(r,f) (r).p1588_tx_debug[0]=(((r).p1588_tx_debug[0] & ~((uint32_t)0x1 << 8)) | ((((uint32_t)f) & 0x1) << 8)) | (1 << (16 + 8))
#define PLP_P1588_TX_DEBUGr_TX_OFFSET_7_6f_GET(r) ((((r).p1588_tx_debug[0]) >> 6) & 0x3)
#define PLP_P1588_TX_DEBUGr_TX_OFFSET_7_6f_SET(r,f) (r).p1588_tx_debug[0]=(((r).p1588_tx_debug[0] & ~((uint32_t)0x3 << 6)) | ((((uint32_t)f) & 0x3) << 6)) | (3 << (16 + 6))
#define PLP_P1588_TX_DEBUGr_TX_OFFSET_5f_GET(r) ((((r).p1588_tx_debug[0]) >> 5) & 0x1)
#define PLP_P1588_TX_DEBUGr_TX_OFFSET_5f_SET(r,f) (r).p1588_tx_debug[0]=(((r).p1588_tx_debug[0] & ~((uint32_t)0x1 << 5)) | ((((uint32_t)f) & 0x1) << 5)) | (1 << (16 + 5))
#define PLP_P1588_TX_DEBUGr_TX_OFFSET_4f_GET(r) ((((r).p1588_tx_debug[0]) >> 4) & 0x1)
#define PLP_P1588_TX_DEBUGr_TX_OFFSET_4f_SET(r,f) (r).p1588_tx_debug[0]=(((r).p1588_tx_debug[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4)) | (1 << (16 + 4))
#define PLP_P1588_TX_DEBUGr_TX_OFFSET_3f_GET(r) ((((r).p1588_tx_debug[0]) >> 3) & 0x1)
#define PLP_P1588_TX_DEBUGr_TX_OFFSET_3f_SET(r,f) (r).p1588_tx_debug[0]=(((r).p1588_tx_debug[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3)) | (1 << (16 + 3))
#define PLP_P1588_TX_DEBUGr_TX_OFFSET_2f_GET(r) ((((r).p1588_tx_debug[0]) >> 2) & 0x1)
#define PLP_P1588_TX_DEBUGr_TX_OFFSET_2f_SET(r,f) (r).p1588_tx_debug[0]=(((r).p1588_tx_debug[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2)) | (1 << (16 + 2))
#define PLP_P1588_TX_DEBUGr_TX_OFFSET_1f_GET(r) ((((r).p1588_tx_debug[0]) >> 1) & 0x1)
#define PLP_P1588_TX_DEBUGr_TX_OFFSET_1f_SET(r,f) (r).p1588_tx_debug[0]=(((r).p1588_tx_debug[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))
#define PLP_P1588_TX_DEBUGr_TX_OFFSET_0f_GET(r) (((r).p1588_tx_debug[0]) & 0x1)
#define PLP_P1588_TX_DEBUGr_TX_OFFSET_0f_SET(r,f) (r).p1588_tx_debug[0]=(((r).p1588_tx_debug[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1)) | (0x1 << 16)

/*
 * These macros can be used to access P1588_TX_DEBUG.
 *
 */
#define PLP_READ_P1588_TX_DEBUGr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TX_DEBUGr,(_r._p1588_tx_debug))
#define PLP_WRITE_P1588_TX_DEBUGr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TX_DEBUGr,(_r._p1588_tx_debug))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TX_DEBUGr PLP_P1588_TX_DEBUGr
#define P1588_TX_DEBUGr_SIZE PLP_P1588_TX_DEBUGr_SIZE
typedef PLP_P1588_TX_DEBUGr_t P1588_TX_DEBUGr_t;
#define P1588_TX_DEBUGr_CLR PLP_P1588_TX_DEBUGr_CLR
#define P1588_TX_DEBUGr_SET PLP_P1588_TX_DEBUGr_SET
#define P1588_TX_DEBUGr_GET PLP_P1588_TX_DEBUGr_GET
#define P1588_TX_DEBUGr_TX_CS_DISf_GET PLP_P1588_TX_DEBUGr_TX_CS_DISf_GET
#define P1588_TX_DEBUGr_TX_CS_DISf_SET PLP_P1588_TX_DEBUGr_TX_CS_DISf_SET
#define P1588_TX_DEBUGr_TX_OFFSET_7_6f_GET PLP_P1588_TX_DEBUGr_TX_OFFSET_7_6f_GET
#define P1588_TX_DEBUGr_TX_OFFSET_7_6f_SET PLP_P1588_TX_DEBUGr_TX_OFFSET_7_6f_SET
#define P1588_TX_DEBUGr_TX_OFFSET_5f_GET PLP_P1588_TX_DEBUGr_TX_OFFSET_5f_GET
#define P1588_TX_DEBUGr_TX_OFFSET_5f_SET PLP_P1588_TX_DEBUGr_TX_OFFSET_5f_SET
#define P1588_TX_DEBUGr_TX_OFFSET_4f_GET PLP_P1588_TX_DEBUGr_TX_OFFSET_4f_GET
#define P1588_TX_DEBUGr_TX_OFFSET_4f_SET PLP_P1588_TX_DEBUGr_TX_OFFSET_4f_SET
#define P1588_TX_DEBUGr_TX_OFFSET_3f_GET PLP_P1588_TX_DEBUGr_TX_OFFSET_3f_GET
#define P1588_TX_DEBUGr_TX_OFFSET_3f_SET PLP_P1588_TX_DEBUGr_TX_OFFSET_3f_SET
#define P1588_TX_DEBUGr_TX_OFFSET_2f_GET PLP_P1588_TX_DEBUGr_TX_OFFSET_2f_GET
#define P1588_TX_DEBUGr_TX_OFFSET_2f_SET PLP_P1588_TX_DEBUGr_TX_OFFSET_2f_SET
#define P1588_TX_DEBUGr_TX_OFFSET_1f_GET PLP_P1588_TX_DEBUGr_TX_OFFSET_1f_GET
#define P1588_TX_DEBUGr_TX_OFFSET_1f_SET PLP_P1588_TX_DEBUGr_TX_OFFSET_1f_SET
#define P1588_TX_DEBUGr_TX_OFFSET_0f_GET PLP_P1588_TX_DEBUGr_TX_OFFSET_0f_GET
#define P1588_TX_DEBUGr_TX_OFFSET_0f_SET PLP_P1588_TX_DEBUGr_TX_OFFSET_0f_SET
#define READ_P1588_TX_DEBUGr PLP_READ_P1588_TX_DEBUGr
#define WRITE_P1588_TX_DEBUGr PLP_WRITE_P1588_TX_DEBUGr
#define MODIFY_P1588_TX_DEBUGr PLP_MODIFY_P1588_TX_DEBUGr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TX_DEBUGr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_RX_CONTROL
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x701a
 * DESC:     RX CONTROL Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_L4_IPV6_UDP_EN Enables the 1588 L4/UDP IPV6 packet check on Rx
 *     RX_L4_IPV4_UDP_EN Enables the 1588 L4/UDP IPV4 packet check on Rx
 *     RX_L2_EN         Enables the 1588 L2 packet check on Rx
 *     RX_AS_EN         Enables the 802.1AS packet check on Rx
 *     RX_L4_IPV6_ADDRESS_EN Enables the L4 IPv6 DA check for 1588 message packet on Rx
 *     RX_L4_IP_ADDRESS_EN Enables the L4 IPv4 DA check for 1588 message packet on Rx
 *     RX_L2_DS_EN      Enables the L2 MAC DA check for 1588 message packet on Rx
 *     RX_AS_DS_EN      Enables the 802.1AS MAC DA check for 1588 message packet on Rx
 *
 ******************************************************************************/
#define PLP_P1588_RX_CONTROLr    0x0000001a

#define PLP_P1588_RX_CONTROLr_SIZE 4

/*
 * This structure should be used to declare and program P1588_RX_CONTROL.
 *
 */
typedef union PLP_P1588_RX_CONTROLr_s {
	uint32_t v[1];
	uint32_t p1588_rx_control[1];
	uint32_t _p1588_rx_control;
} PLP_P1588_RX_CONTROLr_t;

#define PLP_P1588_RX_CONTROLr_CLR(r) (r).p1588_rx_control[0] = 0
#define PLP_P1588_RX_CONTROLr_SET(r,d) (r).p1588_rx_control[0] = d
#define PLP_P1588_RX_CONTROLr_GET(r) (r).p1588_rx_control[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_RX_CONTROLr_RX_AS_DS_ENf_GET(r) ((((r).p1588_rx_control[0]) >> 7) & 0x1)
#define PLP_P1588_RX_CONTROLr_RX_AS_DS_ENf_SET(r,f) (r).p1588_rx_control[0]=(((r).p1588_rx_control[0] & ~((uint32_t)0x1 << 7)) | ((((uint32_t)f) & 0x1) << 7)) | (1 << (16 + 7))
#define PLP_P1588_RX_CONTROLr_RX_L2_DS_ENf_GET(r) ((((r).p1588_rx_control[0]) >> 6) & 0x1)
#define PLP_P1588_RX_CONTROLr_RX_L2_DS_ENf_SET(r,f) (r).p1588_rx_control[0]=(((r).p1588_rx_control[0] & ~((uint32_t)0x1 << 6)) | ((((uint32_t)f) & 0x1) << 6)) | (1 << (16 + 6))
#define PLP_P1588_RX_CONTROLr_RX_L4_IP_ADDRESS_ENf_GET(r) ((((r).p1588_rx_control[0]) >> 5) & 0x1)
#define PLP_P1588_RX_CONTROLr_RX_L4_IP_ADDRESS_ENf_SET(r,f) (r).p1588_rx_control[0]=(((r).p1588_rx_control[0] & ~((uint32_t)0x1 << 5)) | ((((uint32_t)f) & 0x1) << 5)) | (1 << (16 + 5))
#define PLP_P1588_RX_CONTROLr_RX_L4_IPV6_ADDRESS_ENf_GET(r) ((((r).p1588_rx_control[0]) >> 4) & 0x1)
#define PLP_P1588_RX_CONTROLr_RX_L4_IPV6_ADDRESS_ENf_SET(r,f) (r).p1588_rx_control[0]=(((r).p1588_rx_control[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4)) | (1 << (16 + 4))
#define PLP_P1588_RX_CONTROLr_RX_AS_ENf_GET(r) ((((r).p1588_rx_control[0]) >> 3) & 0x1)
#define PLP_P1588_RX_CONTROLr_RX_AS_ENf_SET(r,f) (r).p1588_rx_control[0]=(((r).p1588_rx_control[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3)) | (1 << (16 + 3))
#define PLP_P1588_RX_CONTROLr_RX_L2_ENf_GET(r) ((((r).p1588_rx_control[0]) >> 2) & 0x1)
#define PLP_P1588_RX_CONTROLr_RX_L2_ENf_SET(r,f) (r).p1588_rx_control[0]=(((r).p1588_rx_control[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2)) | (1 << (16 + 2))
#define PLP_P1588_RX_CONTROLr_RX_L4_IPV4_UDP_ENf_GET(r) ((((r).p1588_rx_control[0]) >> 1) & 0x1)
#define PLP_P1588_RX_CONTROLr_RX_L4_IPV4_UDP_ENf_SET(r,f) (r).p1588_rx_control[0]=(((r).p1588_rx_control[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))
#define PLP_P1588_RX_CONTROLr_RX_L4_IPV6_UDP_ENf_GET(r) (((r).p1588_rx_control[0]) & 0x1)
#define PLP_P1588_RX_CONTROLr_RX_L4_IPV6_UDP_ENf_SET(r,f) (r).p1588_rx_control[0]=(((r).p1588_rx_control[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1)) | (0x1 << 16)

#define PLP_P1588_RX_CONTROLr_RX_AS_L2_L4_ENf_SET(r,f) (r).p1588_rx_control[0]=(((r).p1588_rx_control[0] & ~((uint32_t)0xf)) | (((uint32_t)f) & 0xf))

/*
 * These macros can be used to access P1588_RX_CONTROL.
 *
 */
#define PLP_READ_P1588_RX_CONTROLr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_RX_CONTROLr,(_r._p1588_rx_control))
#define PLP_WRITE_P1588_RX_CONTROLr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_RX_CONTROLr,(_r._p1588_rx_control))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_RX_CONTROLr PLP_P1588_RX_CONTROLr
#define P1588_RX_CONTROLr_SIZE PLP_P1588_RX_CONTROLr_SIZE
typedef PLP_P1588_RX_CONTROLr_t P1588_RX_CONTROLr_t;
#define P1588_RX_CONTROLr_CLR PLP_P1588_RX_CONTROLr_CLR
#define P1588_RX_CONTROLr_SET PLP_P1588_RX_CONTROLr_SET
#define P1588_RX_CONTROLr_GET PLP_P1588_RX_CONTROLr_GET
#define P1588_RX_CONTROLr_RX_AS_DS_ENf_GET PLP_P1588_RX_CONTROLr_RX_AS_DS_ENf_GET
#define P1588_RX_CONTROLr_RX_AS_DS_ENf_SET PLP_P1588_RX_CONTROLr_RX_AS_DS_ENf_SET
#define P1588_RX_CONTROLr_RX_L2_DS_ENf_GET PLP_P1588_RX_CONTROLr_RX_L2_DS_ENf_GET
#define P1588_RX_CONTROLr_RX_L2_DS_ENf_SET PLP_P1588_RX_CONTROLr_RX_L2_DS_ENf_SET
#define P1588_RX_CONTROLr_RX_L4_IP_ADDRESS_ENf_GET PLP_P1588_RX_CONTROLr_RX_L4_IP_ADDRESS_ENf_GET
#define P1588_RX_CONTROLr_RX_L4_IP_ADDRESS_ENf_SET PLP_P1588_RX_CONTROLr_RX_L4_IP_ADDRESS_ENf_SET
#define P1588_RX_CONTROLr_RX_L4_IPV6_ADDRESS_ENf_GET PLP_P1588_RX_CONTROLr_RX_L4_IPV6_ADDRESS_ENf_GET
#define P1588_RX_CONTROLr_RX_L4_IPV6_ADDRESS_ENf_SET PLP_P1588_RX_CONTROLr_RX_L4_IPV6_ADDRESS_ENf_SET
#define P1588_RX_CONTROLr_RX_AS_ENf_GET PLP_P1588_RX_CONTROLr_RX_AS_ENf_GET
#define P1588_RX_CONTROLr_RX_AS_ENf_SET PLP_P1588_RX_CONTROLr_RX_AS_ENf_SET
#define P1588_RX_CONTROLr_RX_L2_ENf_GET PLP_P1588_RX_CONTROLr_RX_L2_ENf_GET
#define P1588_RX_CONTROLr_RX_L2_ENf_SET PLP_P1588_RX_CONTROLr_RX_L2_ENf_SET
#define P1588_RX_CONTROLr_RX_L4_IPV4_UDP_ENf_GET PLP_P1588_RX_CONTROLr_RX_L4_IPV4_UDP_ENf_GET
#define P1588_RX_CONTROLr_RX_L4_IPV4_UDP_ENf_SET PLP_P1588_RX_CONTROLr_RX_L4_IPV4_UDP_ENf_SET
#define P1588_RX_CONTROLr_RX_L4_IPV6_UDP_ENf_GET PLP_P1588_RX_CONTROLr_RX_L4_IPV6_UDP_ENf_GET
#define P1588_RX_CONTROLr_RX_L4_IPV6_UDP_ENf_SET PLP_P1588_RX_CONTROLr_RX_L4_IPV6_UDP_ENf_SET
#define P1588_RX_CONTROLr_RX_AS_L2_L4_ENf_SET PLP_P1588_RX_CONTROLr_RX_AS_L2_L4_ENf_SET
#define READ_P1588_RX_CONTROLr PLP_READ_P1588_RX_CONTROLr
#define WRITE_P1588_RX_CONTROLr PLP_WRITE_P1588_RX_CONTROLr
#define MODIFY_P1588_RX_CONTROLr PLP_MODIFY_P1588_RX_CONTROLr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_RX_CONTROLr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_RX_DEBUG
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x701b
 * DESC:     RX DEBUG Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_OFFSET_0      0 - Enable the correction field update1 - Disable the correction field update
 *     RX_OFFSET_1      0 - Checksum computation of 48b TS and 16b Checksum without one's complement1 - One's complement checksum computation of 48b TS and 16b Checksum
 *     RX_OFFSET_2      0 - Update the UDP Checksum to the new computed value1 - Force UDP Checksum to 0
 *     RX_OFFSET_3      0 - Update UDP Checksum1 - Disable the update of UDP Checksum
 *     RX_OFFSET_4      0 - Ignore the 1588 event message type check1 - Perform the 1588 event message type check
 *     RX_OFFSET_5      0 - Update CF, ignore the overflow1 - Update CF to 64'h7FFFFFFF_FFFFFFFF when an overflow occurs
 *     RX_OFFSET_7_6    Select which 16b of MAC DA address is read out to MDIO00 - Select MAC DA[15:0] to be read out to MDIO01 - Select MAC DA[31:16] to be read out to MDIO10 - Select MAC DA[47:32] to be read out to MDIO11 - Select MAC DA[15:0] to be read out to MDIO
 *     RX_CS_DIS        0 - Enable Checksum update1 - Disable Checksum update
 *
 ******************************************************************************/
#define PLP_P1588_RX_DEBUGr    0x0000001b

#define PLP_P1588_RX_DEBUGr_SIZE 4

/*
 * This structure should be used to declare and program P1588_RX_DEBUG.
 *
 */
typedef union PLP_P1588_RX_DEBUGr_s {
	uint32_t v[1];
	uint32_t p1588_rx_debug[1];
	uint32_t _p1588_rx_debug;
} PLP_P1588_RX_DEBUGr_t;

#define PLP_P1588_RX_DEBUGr_CLR(r) (r).p1588_rx_debug[0] = 0
#define PLP_P1588_RX_DEBUGr_SET(r,d) (r).p1588_rx_debug[0] = d
#define PLP_P1588_RX_DEBUGr_GET(r) (r).p1588_rx_debug[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_RX_DEBUGr_RX_CS_DISf_GET(r) ((((r).p1588_rx_debug[0]) >> 8) & 0x1)
#define PLP_P1588_RX_DEBUGr_RX_CS_DISf_SET(r,f) (r).p1588_rx_debug[0]=(((r).p1588_rx_debug[0] & ~((uint32_t)0x1 << 8)) | ((((uint32_t)f) & 0x1) << 8)) | (1 << (16 + 8))
#define PLP_P1588_RX_DEBUGr_RX_OFFSET_7_6f_GET(r) ((((r).p1588_rx_debug[0]) >> 6) & 0x3)
#define PLP_P1588_RX_DEBUGr_RX_OFFSET_7_6f_SET(r,f) (r).p1588_rx_debug[0]=(((r).p1588_rx_debug[0] & ~((uint32_t)0x3 << 6)) | ((((uint32_t)f) & 0x3) << 6)) | (3 << (16 + 6))
#define PLP_P1588_RX_DEBUGr_RX_OFFSET_5f_GET(r) ((((r).p1588_rx_debug[0]) >> 5) & 0x1)
#define PLP_P1588_RX_DEBUGr_RX_OFFSET_5f_SET(r,f) (r).p1588_rx_debug[0]=(((r).p1588_rx_debug[0] & ~((uint32_t)0x1 << 5)) | ((((uint32_t)f) & 0x1) << 5)) | (1 << (16 + 5))
#define PLP_P1588_RX_DEBUGr_RX_OFFSET_4f_GET(r) ((((r).p1588_rx_debug[0]) >> 4) & 0x1)
#define PLP_P1588_RX_DEBUGr_RX_OFFSET_4f_SET(r,f) (r).p1588_rx_debug[0]=(((r).p1588_rx_debug[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4)) | (1 << (16 + 4))
#define PLP_P1588_RX_DEBUGr_RX_OFFSET_3f_GET(r) ((((r).p1588_rx_debug[0]) >> 3) & 0x1)
#define PLP_P1588_RX_DEBUGr_RX_OFFSET_3f_SET(r,f) (r).p1588_rx_debug[0]=(((r).p1588_rx_debug[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3)) | (1 << (16 + 3))
#define PLP_P1588_RX_DEBUGr_RX_OFFSET_2f_GET(r) ((((r).p1588_rx_debug[0]) >> 2) & 0x1)
#define PLP_P1588_RX_DEBUGr_RX_OFFSET_2f_SET(r,f) (r).p1588_rx_debug[0]=(((r).p1588_rx_debug[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2)) | (1 << (16 + 2))
#define PLP_P1588_RX_DEBUGr_RX_OFFSET_1f_GET(r) ((((r).p1588_rx_debug[0]) >> 1) & 0x1)
#define PLP_P1588_RX_DEBUGr_RX_OFFSET_1f_SET(r,f) (r).p1588_rx_debug[0]=(((r).p1588_rx_debug[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))
#define PLP_P1588_RX_DEBUGr_RX_OFFSET_0f_GET(r) (((r).p1588_rx_debug[0]) & 0x1)
#define PLP_P1588_RX_DEBUGr_RX_OFFSET_0f_SET(r,f) (r).p1588_rx_debug[0]=(((r).p1588_rx_debug[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1)) | (0x1 << 16)

/*
 * These macros can be used to access P1588_RX_DEBUG.
 *
 */
#define PLP_READ_P1588_RX_DEBUGr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_RX_DEBUGr,(_r._p1588_rx_debug))
#define PLP_WRITE_P1588_RX_DEBUGr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_RX_DEBUGr,(_r._p1588_rx_debug))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_RX_DEBUGr PLP_P1588_RX_DEBUGr
#define P1588_RX_DEBUGr_SIZE PLP_P1588_RX_DEBUGr_SIZE
typedef PLP_P1588_RX_DEBUGr_t P1588_RX_DEBUGr_t;
#define P1588_RX_DEBUGr_CLR PLP_P1588_RX_DEBUGr_CLR
#define P1588_RX_DEBUGr_SET PLP_P1588_RX_DEBUGr_SET
#define P1588_RX_DEBUGr_GET PLP_P1588_RX_DEBUGr_GET
#define P1588_RX_DEBUGr_RX_CS_DISf_GET PLP_P1588_RX_DEBUGr_RX_CS_DISf_GET
#define P1588_RX_DEBUGr_RX_CS_DISf_SET PLP_P1588_RX_DEBUGr_RX_CS_DISf_SET
#define P1588_RX_DEBUGr_RX_OFFSET_7_6f_GET PLP_P1588_RX_DEBUGr_RX_OFFSET_7_6f_GET
#define P1588_RX_DEBUGr_RX_OFFSET_7_6f_SET PLP_P1588_RX_DEBUGr_RX_OFFSET_7_6f_SET
#define P1588_RX_DEBUGr_RX_OFFSET_5f_GET PLP_P1588_RX_DEBUGr_RX_OFFSET_5f_GET
#define P1588_RX_DEBUGr_RX_OFFSET_5f_SET PLP_P1588_RX_DEBUGr_RX_OFFSET_5f_SET
#define P1588_RX_DEBUGr_RX_OFFSET_4f_GET PLP_P1588_RX_DEBUGr_RX_OFFSET_4f_GET
#define P1588_RX_DEBUGr_RX_OFFSET_4f_SET PLP_P1588_RX_DEBUGr_RX_OFFSET_4f_SET
#define P1588_RX_DEBUGr_RX_OFFSET_3f_GET PLP_P1588_RX_DEBUGr_RX_OFFSET_3f_GET
#define P1588_RX_DEBUGr_RX_OFFSET_3f_SET PLP_P1588_RX_DEBUGr_RX_OFFSET_3f_SET
#define P1588_RX_DEBUGr_RX_OFFSET_2f_GET PLP_P1588_RX_DEBUGr_RX_OFFSET_2f_GET
#define P1588_RX_DEBUGr_RX_OFFSET_2f_SET PLP_P1588_RX_DEBUGr_RX_OFFSET_2f_SET
#define P1588_RX_DEBUGr_RX_OFFSET_1f_GET PLP_P1588_RX_DEBUGr_RX_OFFSET_1f_GET
#define P1588_RX_DEBUGr_RX_OFFSET_1f_SET PLP_P1588_RX_DEBUGr_RX_OFFSET_1f_SET
#define P1588_RX_DEBUGr_RX_OFFSET_0f_GET PLP_P1588_RX_DEBUGr_RX_OFFSET_0f_GET
#define P1588_RX_DEBUGr_RX_OFFSET_0f_SET PLP_P1588_RX_DEBUGr_RX_OFFSET_0f_SET
#define READ_P1588_RX_DEBUGr PLP_READ_P1588_RX_DEBUGr
#define WRITE_P1588_RX_DEBUGr PLP_WRITE_P1588_RX_DEBUGr
#define MODIFY_P1588_RX_DEBUGr PLP_MODIFY_P1588_RX_DEBUGr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_RX_DEBUGr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_RX_TX_CONTROL
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x701c
 * DESC:     RX TX CONTROL Register
 * RESETVAL: 0x80 (128)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_L4_IP_ADDRESS_SEL Selects the Layer4 IP address check for 1588 messages on Rx[2] - Check for 224.0.1.129[1] - Check for 224.0.1.130(131)(132)[0] - Check for 224.0.0.107
 *     RX_CRC_EN        Enable the CRC check in PTP messages on Rx0 - Ignore the CRC check1 - Check the incoming CRC
 *     TX_L4_IP_ADDRESS_SEL Selects the Layer4 IP address check for 1588 messages on Tx[6] - Check for 224.0.1.129[5] - Check for 224.0.1.130(131)(132)[4] - Check for 224.0.0.107
 *     TX_CRC_EN        Enable the CRC check in PTP messages on Tx0 - Ignore the CRC check1 - Check the incoming CRC
 *
 ******************************************************************************/
#define PLP_P1588_RX_TX_CONTROLr    0x0000001c

#define PLP_P1588_RX_TX_CONTROLr_SIZE 4

/*
 * This structure should be used to declare and program P1588_RX_TX_CONTROL.
 *
 */
typedef union PLP_P1588_RX_TX_CONTROLr_s {
	uint32_t v[1];
	uint32_t p1588_rx_tx_control[1];
	uint32_t _p1588_rx_tx_control;
} PLP_P1588_RX_TX_CONTROLr_t;

#define PLP_P1588_RX_TX_CONTROLr_CLR(r) (r).p1588_rx_tx_control[0] = 0
#define PLP_P1588_RX_TX_CONTROLr_SET(r,d) (r).p1588_rx_tx_control[0] = d
#define PLP_P1588_RX_TX_CONTROLr_GET(r) (r).p1588_rx_tx_control[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_RX_TX_CONTROLr_TX_CRC_ENf_GET(r) ((((r).p1588_rx_tx_control[0]) >> 7) & 0x1)
#define PLP_P1588_RX_TX_CONTROLr_TX_CRC_ENf_SET(r,f) (r).p1588_rx_tx_control[0]=(((r).p1588_rx_tx_control[0] & ~((uint32_t)0x1 << 7)) | ((((uint32_t)f) & 0x1) << 7)) | (1 << (16 + 7))
#define PLP_P1588_RX_TX_CONTROLr_TX_L4_IP_ADDRESS_SELf_GET(r) ((((r).p1588_rx_tx_control[0]) >> 4) & 0x7)
#define PLP_P1588_RX_TX_CONTROLr_TX_L4_IP_ADDRESS_SELf_SET(r,f) (r).p1588_rx_tx_control[0]=(((r).p1588_rx_tx_control[0] & ~((uint32_t)0x7 << 4)) | ((((uint32_t)f) & 0x7) << 4)) | (7 << (16 + 4))
#define PLP_P1588_RX_TX_CONTROLr_RX_CRC_ENf_GET(r) ((((r).p1588_rx_tx_control[0]) >> 3) & 0x1)
#define PLP_P1588_RX_TX_CONTROLr_RX_CRC_ENf_SET(r,f) (r).p1588_rx_tx_control[0]=(((r).p1588_rx_tx_control[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3)) | (1 << (16 + 3))
#define PLP_P1588_RX_TX_CONTROLr_RX_L4_IP_ADDRESS_SELf_GET(r) (((r).p1588_rx_tx_control[0]) & 0x7)
#define PLP_P1588_RX_TX_CONTROLr_RX_L4_IP_ADDRESS_SELf_SET(r,f) (r).p1588_rx_tx_control[0]=(((r).p1588_rx_tx_control[0] & ~((uint32_t)0x7)) | (((uint32_t)f) & 0x7)) | (0x7 << 16)

/*
 * These macros can be used to access P1588_RX_TX_CONTROL.
 *
 */
#define PLP_READ_P1588_RX_TX_CONTROLr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_RX_TX_CONTROLr,(_r._p1588_rx_tx_control))
#define PLP_WRITE_P1588_RX_TX_CONTROLr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_RX_TX_CONTROLr,(_r._p1588_rx_tx_control))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_RX_TX_CONTROLr PLP_P1588_RX_TX_CONTROLr
#define P1588_RX_TX_CONTROLr_SIZE PLP_P1588_RX_TX_CONTROLr_SIZE
typedef PLP_P1588_RX_TX_CONTROLr_t P1588_RX_TX_CONTROLr_t;
#define P1588_RX_TX_CONTROLr_CLR PLP_P1588_RX_TX_CONTROLr_CLR
#define P1588_RX_TX_CONTROLr_SET PLP_P1588_RX_TX_CONTROLr_SET
#define P1588_RX_TX_CONTROLr_GET PLP_P1588_RX_TX_CONTROLr_GET
#define P1588_RX_TX_CONTROLr_TX_CRC_ENf_GET PLP_P1588_RX_TX_CONTROLr_TX_CRC_ENf_GET
#define P1588_RX_TX_CONTROLr_TX_CRC_ENf_SET PLP_P1588_RX_TX_CONTROLr_TX_CRC_ENf_SET
#define P1588_RX_TX_CONTROLr_TX_L4_IP_ADDRESS_SELf_GET PLP_P1588_RX_TX_CONTROLr_TX_L4_IP_ADDRESS_SELf_GET
#define P1588_RX_TX_CONTROLr_TX_L4_IP_ADDRESS_SELf_SET PLP_P1588_RX_TX_CONTROLr_TX_L4_IP_ADDRESS_SELf_SET
#define P1588_RX_TX_CONTROLr_RX_CRC_ENf_GET PLP_P1588_RX_TX_CONTROLr_RX_CRC_ENf_GET
#define P1588_RX_TX_CONTROLr_RX_CRC_ENf_SET PLP_P1588_RX_TX_CONTROLr_RX_CRC_ENf_SET
#define P1588_RX_TX_CONTROLr_RX_L4_IP_ADDRESS_SELf_GET PLP_P1588_RX_TX_CONTROLr_RX_L4_IP_ADDRESS_SELf_GET
#define P1588_RX_TX_CONTROLr_RX_L4_IP_ADDRESS_SELf_SET PLP_P1588_RX_TX_CONTROLr_RX_L4_IP_ADDRESS_SELf_SET
#define READ_P1588_RX_TX_CONTROLr PLP_READ_P1588_RX_TX_CONTROLr
#define WRITE_P1588_RX_TX_CONTROLr PLP_WRITE_P1588_RX_TX_CONTROLr
#define MODIFY_P1588_RX_TX_CONTROLr PLP_MODIFY_P1588_RX_TX_CONTROLr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_RX_TX_CONTROLr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_VLAN_TAG
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x701d
 * DESC:     VLAN TAG Register
 * RESETVAL: 0x8100 (33024)
 * ACCESS:   R/W
 * FIELDS:
 *     ITPID            VLAN Tag to match for single tagged packets and Inner VLAN tag to match for double tagged packets
 *
 ******************************************************************************/
#define PLP_P1588_VLAN_TAGr    0x0000001d

#define PLP_P1588_VLAN_TAGr_SIZE 4

/*
 * This structure should be used to declare and program P1588_VLAN_TAG.
 *
 */
typedef union PLP_P1588_VLAN_TAGr_s {
	uint32_t v[1];
	uint32_t p1588_vlan_tag[1];
	uint32_t _p1588_vlan_tag;
} PLP_P1588_VLAN_TAGr_t;

#define PLP_P1588_VLAN_TAGr_CLR(r) (r).p1588_vlan_tag[0] = 0
#define PLP_P1588_VLAN_TAGr_SET(r,d) (r).p1588_vlan_tag[0] = d
#define PLP_P1588_VLAN_TAGr_GET(r) (r).p1588_vlan_tag[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_VLAN_TAGr_ITPIDf_GET(r) (((r).p1588_vlan_tag[0]) & 0xffff)
#define PLP_P1588_VLAN_TAGr_ITPIDf_SET(r,f) (r).p1588_vlan_tag[0]=(((r).p1588_vlan_tag[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_VLAN_TAG.
 *
 */
#define PLP_READ_P1588_VLAN_TAGr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_VLAN_TAGr,(_r._p1588_vlan_tag))
#define PLP_WRITE_P1588_VLAN_TAGr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_VLAN_TAGr,(_r._p1588_vlan_tag))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_VLAN_TAGr PLP_P1588_VLAN_TAGr
#define P1588_VLAN_TAGr_SIZE PLP_P1588_VLAN_TAGr_SIZE
typedef PLP_P1588_VLAN_TAGr_t P1588_VLAN_TAGr_t;
#define P1588_VLAN_TAGr_CLR PLP_P1588_VLAN_TAGr_CLR
#define P1588_VLAN_TAGr_SET PLP_P1588_VLAN_TAGr_SET
#define P1588_VLAN_TAGr_GET PLP_P1588_VLAN_TAGr_GET
#define P1588_VLAN_TAGr_ITPIDf_GET PLP_P1588_VLAN_TAGr_ITPIDf_GET
#define P1588_VLAN_TAGr_ITPIDf_SET PLP_P1588_VLAN_TAGr_ITPIDf_SET
#define READ_P1588_VLAN_TAGr PLP_READ_P1588_VLAN_TAGr
#define WRITE_P1588_VLAN_TAGr PLP_WRITE_P1588_VLAN_TAGr
#define MODIFY_P1588_VLAN_TAGr PLP_MODIFY_P1588_VLAN_TAGr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_VLAN_TAGr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_OUTER_VLAN_TAG
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x701e
 * DESC:     OUTER VLAN TAG Register
 * RESETVAL: 0x9100 (37120)
 * ACCESS:   R/W
 * FIELDS:
 *     OTPID            Outer VLAN Tag to match for double tagged packets
 *
 ******************************************************************************/
#define PLP_P1588_OUTER_VLAN_TAGr    0x0000001e

#define PLP_P1588_OUTER_VLAN_TAGr_SIZE 4

/*
 * This structure should be used to declare and program P1588_OUTER_VLAN_TAG.
 *
 */
typedef union PLP_P1588_OUTER_VLAN_TAGr_s {
	uint32_t v[1];
	uint32_t p1588_outer_vlan_tag[1];
	uint32_t _p1588_outer_vlan_tag;
} PLP_P1588_OUTER_VLAN_TAGr_t;

#define PLP_P1588_OUTER_VLAN_TAGr_CLR(r) (r).p1588_outer_vlan_tag[0] = 0
#define PLP_P1588_OUTER_VLAN_TAGr_SET(r,d) (r).p1588_outer_vlan_tag[0] = d
#define PLP_P1588_OUTER_VLAN_TAGr_GET(r) (r).p1588_outer_vlan_tag[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_OUTER_VLAN_TAGr_OTPIDf_GET(r) (((r).p1588_outer_vlan_tag[0]) & 0xffff)
#define PLP_P1588_OUTER_VLAN_TAGr_OTPIDf_SET(r,f) (r).p1588_outer_vlan_tag[0]=(((r).p1588_outer_vlan_tag[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_OUTER_VLAN_TAG.
 *
 */
#define PLP_READ_P1588_OUTER_VLAN_TAGr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_OUTER_VLAN_TAGr,(_r._p1588_outer_vlan_tag))
#define PLP_WRITE_P1588_OUTER_VLAN_TAGr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_OUTER_VLAN_TAGr,(_r._p1588_outer_vlan_tag))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_OUTER_VLAN_TAGr PLP_P1588_OUTER_VLAN_TAGr
#define P1588_OUTER_VLAN_TAGr_SIZE PLP_P1588_OUTER_VLAN_TAGr_SIZE
typedef PLP_P1588_OUTER_VLAN_TAGr_t P1588_OUTER_VLAN_TAGr_t;
#define P1588_OUTER_VLAN_TAGr_CLR PLP_P1588_OUTER_VLAN_TAGr_CLR
#define P1588_OUTER_VLAN_TAGr_SET PLP_P1588_OUTER_VLAN_TAGr_SET
#define P1588_OUTER_VLAN_TAGr_GET PLP_P1588_OUTER_VLAN_TAGr_GET
#define P1588_OUTER_VLAN_TAGr_OTPIDf_GET PLP_P1588_OUTER_VLAN_TAGr_OTPIDf_GET
#define P1588_OUTER_VLAN_TAGr_OTPIDf_SET PLP_P1588_OUTER_VLAN_TAGr_OTPIDf_SET
#define READ_P1588_OUTER_VLAN_TAGr PLP_READ_P1588_OUTER_VLAN_TAGr
#define WRITE_P1588_OUTER_VLAN_TAGr PLP_WRITE_P1588_OUTER_VLAN_TAGr
#define MODIFY_P1588_OUTER_VLAN_TAGr PLP_MODIFY_P1588_OUTER_VLAN_TAGr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_OUTER_VLAN_TAGr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_INNER_VLAN_TAG
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x701f
 * DESC:     INNER VLAN TAG Register
 * RESETVAL: 0x8100 (33024)
 * ACCESS:   R/W
 * FIELDS:
 *     OTPID2           Alternate Outer VLAN Tag to match for double tagged packets
 *
 ******************************************************************************/
#define PLP_P1588_INNER_VLAN_TAGr    0x0000001f

#define PLP_P1588_INNER_VLAN_TAGr_SIZE 4

/*
 * This structure should be used to declare and program P1588_INNER_VLAN_TAG.
 *
 */
typedef union PLP_P1588_INNER_VLAN_TAGr_s {
	uint32_t v[1];
	uint32_t p1588_inner_vlan_tag[1];
	uint32_t _p1588_inner_vlan_tag;
} PLP_P1588_INNER_VLAN_TAGr_t;

#define PLP_P1588_INNER_VLAN_TAGr_CLR(r) (r).p1588_inner_vlan_tag[0] = 0
#define PLP_P1588_INNER_VLAN_TAGr_SET(r,d) (r).p1588_inner_vlan_tag[0] = d
#define PLP_P1588_INNER_VLAN_TAGr_GET(r) (r).p1588_inner_vlan_tag[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_INNER_VLAN_TAGr_OTPID2f_GET(r) (((r).p1588_inner_vlan_tag[0]) & 0xffff)
#define PLP_P1588_INNER_VLAN_TAGr_OTPID2f_SET(r,f) (r).p1588_inner_vlan_tag[0]=(((r).p1588_inner_vlan_tag[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_INNER_VLAN_TAG.
 *
 */
#define PLP_READ_P1588_INNER_VLAN_TAGr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_INNER_VLAN_TAGr,(_r._p1588_inner_vlan_tag))
#define PLP_WRITE_P1588_INNER_VLAN_TAGr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_INNER_VLAN_TAGr,(_r._p1588_inner_vlan_tag))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_INNER_VLAN_TAGr PLP_P1588_INNER_VLAN_TAGr
#define P1588_INNER_VLAN_TAGr_SIZE PLP_P1588_INNER_VLAN_TAGr_SIZE
typedef PLP_P1588_INNER_VLAN_TAGr_t P1588_INNER_VLAN_TAGr_t;
#define P1588_INNER_VLAN_TAGr_CLR PLP_P1588_INNER_VLAN_TAGr_CLR
#define P1588_INNER_VLAN_TAGr_SET PLP_P1588_INNER_VLAN_TAGr_SET
#define P1588_INNER_VLAN_TAGr_GET PLP_P1588_INNER_VLAN_TAGr_GET
#define P1588_INNER_VLAN_TAGr_OTPID2f_GET PLP_P1588_INNER_VLAN_TAGr_OTPID2f_GET
#define P1588_INNER_VLAN_TAGr_OTPID2f_SET PLP_P1588_INNER_VLAN_TAGr_OTPID2f_SET
#define READ_P1588_INNER_VLAN_TAGr PLP_READ_P1588_INNER_VLAN_TAGr
#define WRITE_P1588_INNER_VLAN_TAGr PLP_WRITE_P1588_INNER_VLAN_TAGr
#define MODIFY_P1588_INNER_VLAN_TAGr PLP_MODIFY_P1588_INNER_VLAN_TAGr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_INNER_VLAN_TAGr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_DPLL_1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7020
 * DESC:     NSE DPLL 1 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_TESTBUS_SEL_1_0 00 - Select bits[15:0] of the ones selected in 4:2 above01 - Select bits[31:16] of the ones selected in 4:2 above10 - Select bits[47:32] of the ones selected in 4:2 above
 *     NSE_REG_TESTBUS_SEL_4_2 000 - Select timestamp to bring out to the testbus001 - Select loopfilter to bring out to the testbus010 - Select delta phase to bring out to the testbus011 - Select sync-in and sync-out FSM to bring out to the testbus
 *     NSE_REG_TESTBUS_SEL_5 currently not used in the design
 *     NSE_REG_TESTBUS_SEL_6 0 - Select the Rx slice output to testport1 - Select the Rx slice input to testportThis is the rx_test_sel field
 *     NSE_REG_TESTBUS_SEL_7 Enable timestamp or timecode debug modeThis is the ts_debug_en field
 *     NSE_REG_TESTBUS_SEL_10_8 Fixed value, used for debug, for 48b timestamp field or 80b timecode field or 64b NTP timestamp000 - 48'h0000_0000_aaaa or 80'h1234_5678_9abc_def8_8888 or 64'h1234_5678_9abc_1731001 - 48'hffff_ffff_0001 or 80'hffff_ffff_ffff_ffff_1588 or 64'hffff_ffff_ffff_0001010 - 48'hffff_ffff_ffff or 80'h1588_ffff_ffff_ffff_ffff or 64'hffff_ffff_ffff_ffff011 - 48'hffff_5555_aaaa or 80'hffff_ffff_ffff_5555_aaaa or 64'hffff_ffff_5555_aaaa100 - 48'h8000_0000_aaaa or 80'h8000_0000_0000_3b9a_c9ff or 64'h8000_1234_0000_aaaa101 - 48'h8000_5555_0000 or 80'h8000_5555_0000_0000_0000 or 64'h8000_ffff_5555_0000110 - 48'h7fff_ffff_ffff or 80'h7fff_ffff_ffff_ffff_1588 or 64'h7fff_0000_ffff_ffff111 - 48'h7fff_5555_aaaa or 80'h7fff_ffff_ffff_5555_1588 or 64'h7fff_0000_5555_aaaaThis is the ts_debug field
 *     NSE_REG_TESTBUS_SEL_14_11 currently not used in the design
 *     NSE_REG_MODE     Selects between Frequency loop and Phase loop in the DPLL0 - Select Delta Phase for DPLL1 - Select Delta Frequency for DPLL
 *
 ******************************************************************************/
#define PLP_P1588_NSE_DPLL_1r    0x00000020

#define PLP_P1588_NSE_DPLL_1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_DPLL_1.
 *
 */
typedef union PLP_P1588_NSE_DPLL_1r_s {
	uint32_t v[1];
	uint32_t p1588_nse_dpll_1[1];
	uint32_t _p1588_nse_dpll_1;
} PLP_P1588_NSE_DPLL_1r_t;

#define PLP_P1588_NSE_DPLL_1r_CLR(r) (r).p1588_nse_dpll_1[0] = 0
#define PLP_P1588_NSE_DPLL_1r_SET(r,d) (r).p1588_nse_dpll_1[0] = d
#define PLP_P1588_NSE_DPLL_1r_GET(r) (r).p1588_nse_dpll_1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define P1588_NSE_DPLL_1r_NSE_HB_CAPTURE_MODEf_GET(r) ((((r).p1588_nse_dpll_1[0]) >> 12) & 0x3)
#define P1588_NSE_DPLL_1r_NSE_HB_CAPTURE_MODEf_SET(r,f) (r).p1588_nse_dpll_1[0]=(((r).p1588_nse_dpll_1[0] & ~((uint32_t)0x3 << 12)) | ((((uint32_t)f) & 0x3) << 12)) | (15 << (16 + 12))

#define PLP_P1588_NSE_DPLL_1r_NSE_REG_MODEf_GET(r) ((((r).p1588_nse_dpll_1[0]) >> 15) & 0x1)
#define PLP_P1588_NSE_DPLL_1r_NSE_REG_MODEf_SET(r,f) (r).p1588_nse_dpll_1[0]=(((r).p1588_nse_dpll_1[0] & ~((uint32_t)0x1 << 15)) | ((((uint32_t)f) & 0x1) << 15)) | (1 << (16 + 15))
#define PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_14_11f_GET(r) ((((r).p1588_nse_dpll_1[0]) >> 11) & 0xf)
#define PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_14_11f_SET(r,f) (r).p1588_nse_dpll_1[0]=(((r).p1588_nse_dpll_1[0] & ~((uint32_t)0xf << 11)) | ((((uint32_t)f) & 0xf) << 11)) | (15 << (16 + 11))
#define PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_10_8f_GET(r) ((((r).p1588_nse_dpll_1[0]) >> 8) & 0x7)
#define PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_10_8f_SET(r,f) (r).p1588_nse_dpll_1[0]=(((r).p1588_nse_dpll_1[0] & ~((uint32_t)0x7 << 8)) | ((((uint32_t)f) & 0x7) << 8)) | (7 << (16 + 8))
#define PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_7f_GET(r) ((((r).p1588_nse_dpll_1[0]) >> 7) & 0x1)
#define PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_7f_SET(r,f) (r).p1588_nse_dpll_1[0]=(((r).p1588_nse_dpll_1[0] & ~((uint32_t)0x1 << 7)) | ((((uint32_t)f) & 0x1) << 7)) | (1 << (16 + 7))
#define PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_6f_GET(r) ((((r).p1588_nse_dpll_1[0]) >> 6) & 0x1)
#define PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_6f_SET(r,f) (r).p1588_nse_dpll_1[0]=(((r).p1588_nse_dpll_1[0] & ~((uint32_t)0x1 << 6)) | ((((uint32_t)f) & 0x1) << 6)) | (1 << (16 + 6))
#define PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_5f_GET(r) ((((r).p1588_nse_dpll_1[0]) >> 5) & 0x1)
#define PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_5f_SET(r,f) (r).p1588_nse_dpll_1[0]=(((r).p1588_nse_dpll_1[0] & ~((uint32_t)0x1 << 5)) | ((((uint32_t)f) & 0x1) << 5)) | (1 << (16 + 5))
#define PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_4_2f_GET(r) ((((r).p1588_nse_dpll_1[0]) >> 2) & 0x7)
#define PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_4_2f_SET(r,f) (r).p1588_nse_dpll_1[0]=(((r).p1588_nse_dpll_1[0] & ~((uint32_t)0x7 << 2)) | ((((uint32_t)f) & 0x7) << 2)) | (7 << (16 + 2))
#define PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_1_0f_GET(r) (((r).p1588_nse_dpll_1[0]) & 0x3)
#define PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_1_0f_SET(r,f) (r).p1588_nse_dpll_1[0]=(((r).p1588_nse_dpll_1[0] & ~((uint32_t)0x3)) | (((uint32_t)f) & 0x3)) | (0x3 << 16)

/*
 * These macros can be used to access P1588_NSE_DPLL_1.
 *
 */
#define PLP_READ_P1588_NSE_DPLL_1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_DPLL_1r,(_r._p1588_nse_dpll_1))
#define PLP_WRITE_P1588_NSE_DPLL_1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_DPLL_1r,(_r._p1588_nse_dpll_1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_DPLL_1r PLP_P1588_NSE_DPLL_1r
#define P1588_NSE_DPLL_1r_SIZE PLP_P1588_NSE_DPLL_1r_SIZE
typedef PLP_P1588_NSE_DPLL_1r_t P1588_NSE_DPLL_1r_t;
#define P1588_NSE_DPLL_1r_CLR PLP_P1588_NSE_DPLL_1r_CLR
#define P1588_NSE_DPLL_1r_SET PLP_P1588_NSE_DPLL_1r_SET
#define P1588_NSE_DPLL_1r_GET PLP_P1588_NSE_DPLL_1r_GET
#define P1588_NSE_DPLL_1r_NSE_REG_MODEf_GET PLP_P1588_NSE_DPLL_1r_NSE_REG_MODEf_GET
#define P1588_NSE_DPLL_1r_NSE_REG_MODEf_SET PLP_P1588_NSE_DPLL_1r_NSE_REG_MODEf_SET
#define P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_14_11f_GET PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_14_11f_GET
#define P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_14_11f_SET PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_14_11f_SET
#define P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_10_8f_GET PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_10_8f_GET
#define P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_10_8f_SET PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_10_8f_SET
#define P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_7f_GET PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_7f_GET
#define P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_7f_SET PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_7f_SET
#define P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_6f_GET PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_6f_GET
#define P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_6f_SET PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_6f_SET
#define P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_5f_GET PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_5f_GET
#define P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_5f_SET PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_5f_SET
#define P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_4_2f_GET PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_4_2f_GET
#define P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_4_2f_SET PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_4_2f_SET
#define P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_1_0f_GET PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_1_0f_GET
#define P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_1_0f_SET PLP_P1588_NSE_DPLL_1r_NSE_REG_TESTBUS_SEL_1_0f_SET
#define READ_P1588_NSE_DPLL_1r PLP_READ_P1588_NSE_DPLL_1r
#define WRITE_P1588_NSE_DPLL_1r PLP_WRITE_P1588_NSE_DPLL_1r
#define MODIFY_P1588_NSE_DPLL_1r PLP_MODIFY_P1588_NSE_DPLL_1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_DPLL_1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_DPLL_2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7021
 * DESC:     NSE DPLL 2 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_REF_PHASE_47_32 DPLL Initial reference phase[47:32]
 *
 ******************************************************************************/
#define PLP_P1588_NSE_DPLL_2r    0x00000021
#define P1588_DPLL_Ref_Phase_47             PLP_P1588_NSE_DPLL_2r

#define PLP_P1588_NSE_DPLL_2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_DPLL_2.
 *
 */
typedef union PLP_P1588_NSE_DPLL_2r_s {
	uint32_t v[1];
	uint32_t p1588_nse_dpll_2[1];
	uint32_t _p1588_nse_dpll_2;
} PLP_P1588_NSE_DPLL_2r_t;

#define PLP_P1588_NSE_DPLL_2r_CLR(r) (r).p1588_nse_dpll_2[0] = 0
#define PLP_P1588_NSE_DPLL_2r_SET(r,d) (r).p1588_nse_dpll_2[0] = d
#define PLP_P1588_NSE_DPLL_2r_GET(r) (r).p1588_nse_dpll_2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_DPLL_2r_NSE_REG_REF_PHASE_47_32f_GET(r) (((r).p1588_nse_dpll_2[0]) & 0xffff)
#define PLP_P1588_NSE_DPLL_2r_NSE_REG_REF_PHASE_47_32f_SET(r,f) (r).p1588_nse_dpll_2[0]=(((r).p1588_nse_dpll_2[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_DPLL_2.
 *
 */
#define PLP_READ_P1588_NSE_DPLL_2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_DPLL_2r,(_r._p1588_nse_dpll_2))
#define PLP_WRITE_P1588_NSE_DPLL_2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_DPLL_2r,(_r._p1588_nse_dpll_2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_DPLL_2r PLP_P1588_NSE_DPLL_2r
#define P1588_NSE_DPLL_2r_SIZE PLP_P1588_NSE_DPLL_2r_SIZE
typedef PLP_P1588_NSE_DPLL_2r_t P1588_NSE_DPLL_2r_t;
#define P1588_NSE_DPLL_2r_CLR PLP_P1588_NSE_DPLL_2r_CLR
#define P1588_NSE_DPLL_2r_SET PLP_P1588_NSE_DPLL_2r_SET
#define P1588_NSE_DPLL_2r_GET PLP_P1588_NSE_DPLL_2r_GET
#define P1588_NSE_DPLL_2r_NSE_REG_REF_PHASE_47_32f_GET PLP_P1588_NSE_DPLL_2r_NSE_REG_REF_PHASE_47_32f_GET
#define P1588_NSE_DPLL_2r_NSE_REG_REF_PHASE_47_32f_SET PLP_P1588_NSE_DPLL_2r_NSE_REG_REF_PHASE_47_32f_SET
#define READ_P1588_NSE_DPLL_2r PLP_READ_P1588_NSE_DPLL_2r
#define WRITE_P1588_NSE_DPLL_2r PLP_WRITE_P1588_NSE_DPLL_2r
#define MODIFY_P1588_NSE_DPLL_2r PLP_MODIFY_P1588_NSE_DPLL_2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_DPLL_2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_DPLL_3
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7022
 * DESC:     NSE DPLL 3 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_REF_PHASE_31_16 DPLL Initial reference phase[31:16]
 *
 ******************************************************************************/
#define PLP_P1588_NSE_DPLL_3r    0x00000022

#define PLP_P1588_NSE_DPLL_3r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_DPLL_3.
 *
 */
typedef union PLP_P1588_NSE_DPLL_3r_s {
	uint32_t v[1];
	uint32_t p1588_nse_dpll_3[1];
	uint32_t _p1588_nse_dpll_3;
} PLP_P1588_NSE_DPLL_3r_t;

#define PLP_P1588_NSE_DPLL_3r_CLR(r) (r).p1588_nse_dpll_3[0] = 0
#define PLP_P1588_NSE_DPLL_3r_SET(r,d) (r).p1588_nse_dpll_3[0] = d
#define PLP_P1588_NSE_DPLL_3r_GET(r) (r).p1588_nse_dpll_3[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_DPLL_3r_NSE_REG_REF_PHASE_31_16f_GET(r) (((r).p1588_nse_dpll_3[0]) & 0xffff)
#define PLP_P1588_NSE_DPLL_3r_NSE_REG_REF_PHASE_31_16f_SET(r,f) (r).p1588_nse_dpll_3[0]=(((r).p1588_nse_dpll_3[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_DPLL_3.
 *
 */
#define PLP_READ_P1588_NSE_DPLL_3r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_DPLL_3r,(_r._p1588_nse_dpll_3))
#define PLP_WRITE_P1588_NSE_DPLL_3r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_DPLL_3r,(_r._p1588_nse_dpll_3))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_DPLL_3r PLP_P1588_NSE_DPLL_3r
#define P1588_NSE_DPLL_3r_SIZE PLP_P1588_NSE_DPLL_3r_SIZE
typedef PLP_P1588_NSE_DPLL_3r_t P1588_NSE_DPLL_3r_t;
#define P1588_NSE_DPLL_3r_CLR PLP_P1588_NSE_DPLL_3r_CLR
#define P1588_NSE_DPLL_3r_SET PLP_P1588_NSE_DPLL_3r_SET
#define P1588_NSE_DPLL_3r_GET PLP_P1588_NSE_DPLL_3r_GET
#define P1588_NSE_DPLL_3r_NSE_REG_REF_PHASE_31_16f_GET PLP_P1588_NSE_DPLL_3r_NSE_REG_REF_PHASE_31_16f_GET
#define P1588_NSE_DPLL_3r_NSE_REG_REF_PHASE_31_16f_SET PLP_P1588_NSE_DPLL_3r_NSE_REG_REF_PHASE_31_16f_SET
#define READ_P1588_NSE_DPLL_3r PLP_READ_P1588_NSE_DPLL_3r
#define WRITE_P1588_NSE_DPLL_3r PLP_WRITE_P1588_NSE_DPLL_3r
#define MODIFY_P1588_NSE_DPLL_3r PLP_MODIFY_P1588_NSE_DPLL_3r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_DPLL_3r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_DPLL_4
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7023
 * DESC:     NSE DPLL 4 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_REF_PHASE_15_0 DPLL Initial reference phase[15:0]
 *
 ******************************************************************************/
#define PLP_P1588_NSE_DPLL_4r    0x00000023
#define P1588_DPLL_Ref_Phase_0              PLP_P1588_NSE_DPLL_4r

#define PLP_P1588_NSE_DPLL_4r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_DPLL_4.
 *
 */
typedef union PLP_P1588_NSE_DPLL_4r_s {
	uint32_t v[1];
	uint32_t p1588_nse_dpll_4[1];
	uint32_t _p1588_nse_dpll_4;
} PLP_P1588_NSE_DPLL_4r_t;

#define PLP_P1588_NSE_DPLL_4r_CLR(r) (r).p1588_nse_dpll_4[0] = 0
#define PLP_P1588_NSE_DPLL_4r_SET(r,d) (r).p1588_nse_dpll_4[0] = d
#define PLP_P1588_NSE_DPLL_4r_GET(r) (r).p1588_nse_dpll_4[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_DPLL_4r_NSE_REG_REF_PHASE_15_0f_GET(r) (((r).p1588_nse_dpll_4[0]) & 0xffff)
#define PLP_P1588_NSE_DPLL_4r_NSE_REG_REF_PHASE_15_0f_SET(r,f) (r).p1588_nse_dpll_4[0]=(((r).p1588_nse_dpll_4[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_DPLL_4.
 *
 */
#define PLP_READ_P1588_NSE_DPLL_4r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_DPLL_4r,(_r._p1588_nse_dpll_4))
#define PLP_WRITE_P1588_NSE_DPLL_4r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_DPLL_4r,(_r._p1588_nse_dpll_4))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_DPLL_4r PLP_P1588_NSE_DPLL_4r
#define P1588_NSE_DPLL_4r_SIZE PLP_P1588_NSE_DPLL_4r_SIZE
typedef PLP_P1588_NSE_DPLL_4r_t P1588_NSE_DPLL_4r_t;
#define P1588_NSE_DPLL_4r_CLR PLP_P1588_NSE_DPLL_4r_CLR
#define P1588_NSE_DPLL_4r_SET PLP_P1588_NSE_DPLL_4r_SET
#define P1588_NSE_DPLL_4r_GET PLP_P1588_NSE_DPLL_4r_GET
#define P1588_NSE_DPLL_4r_NSE_REG_REF_PHASE_15_0f_GET PLP_P1588_NSE_DPLL_4r_NSE_REG_REF_PHASE_15_0f_GET
#define P1588_NSE_DPLL_4r_NSE_REG_REF_PHASE_15_0f_SET PLP_P1588_NSE_DPLL_4r_NSE_REG_REF_PHASE_15_0f_SET
#define READ_P1588_NSE_DPLL_4r PLP_READ_P1588_NSE_DPLL_4r
#define WRITE_P1588_NSE_DPLL_4r PLP_WRITE_P1588_NSE_DPLL_4r
#define MODIFY_P1588_NSE_DPLL_4r PLP_MODIFY_P1588_NSE_DPLL_4r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_DPLL_4r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_DPLL_5
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7024
 * DESC:     NSE DPLL 5 Register
 * RESETVAL: 0xf (15)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_REF_PHASE_DELTA_31_16 DPLL Initial reference delta phase[31:16]Set for a sync-in frequency of 1Khz, determinedby 1e9/sync-in freq, which is 1e9/1e3=1e61e6 in hex is 0xF4240
 *
 ******************************************************************************/
#define PLP_P1588_NSE_DPLL_5r    0x00000024
#define P1588_DPLL_Ref_PhaseDelta_31        PLP_P1588_NSE_DPLL_5r

#define PLP_P1588_NSE_DPLL_5r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_DPLL_5.
 *
 */
typedef union PLP_P1588_NSE_DPLL_5r_s {
	uint32_t v[1];
	uint32_t p1588_nse_dpll_5[1];
	uint32_t _p1588_nse_dpll_5;
} PLP_P1588_NSE_DPLL_5r_t;

#define PLP_P1588_NSE_DPLL_5r_CLR(r) (r).p1588_nse_dpll_5[0] = 0
#define PLP_P1588_NSE_DPLL_5r_SET(r,d) (r).p1588_nse_dpll_5[0] = d
#define PLP_P1588_NSE_DPLL_5r_GET(r) (r).p1588_nse_dpll_5[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_DPLL_5r_NSE_REG_REF_PHASE_DELTA_31_16f_GET(r) (((r).p1588_nse_dpll_5[0]) & 0xffff)
#define PLP_P1588_NSE_DPLL_5r_NSE_REG_REF_PHASE_DELTA_31_16f_SET(r,f) (r).p1588_nse_dpll_5[0]=(((r).p1588_nse_dpll_5[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_DPLL_5.
 *
 */
#define PLP_READ_P1588_NSE_DPLL_5r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_DPLL_5r,(_r._p1588_nse_dpll_5))
#define PLP_WRITE_P1588_NSE_DPLL_5r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_DPLL_5r,(_r._p1588_nse_dpll_5))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_DPLL_5r PLP_P1588_NSE_DPLL_5r
#define P1588_NSE_DPLL_5r_SIZE PLP_P1588_NSE_DPLL_5r_SIZE
typedef PLP_P1588_NSE_DPLL_5r_t P1588_NSE_DPLL_5r_t;
#define P1588_NSE_DPLL_5r_CLR PLP_P1588_NSE_DPLL_5r_CLR
#define P1588_NSE_DPLL_5r_SET PLP_P1588_NSE_DPLL_5r_SET
#define P1588_NSE_DPLL_5r_GET PLP_P1588_NSE_DPLL_5r_GET
#define P1588_NSE_DPLL_5r_NSE_REG_REF_PHASE_DELTA_31_16f_GET PLP_P1588_NSE_DPLL_5r_NSE_REG_REF_PHASE_DELTA_31_16f_GET
#define P1588_NSE_DPLL_5r_NSE_REG_REF_PHASE_DELTA_31_16f_SET PLP_P1588_NSE_DPLL_5r_NSE_REG_REF_PHASE_DELTA_31_16f_SET
#define READ_P1588_NSE_DPLL_5r PLP_READ_P1588_NSE_DPLL_5r
#define WRITE_P1588_NSE_DPLL_5r PLP_WRITE_P1588_NSE_DPLL_5r
#define MODIFY_P1588_NSE_DPLL_5r PLP_MODIFY_P1588_NSE_DPLL_5r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_DPLL_5r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_DPLL_6
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7025
 * DESC:     NSE DPLL 6 Register
 * RESETVAL: 0x4240 (16960)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_REF_PHASE_DELTA_15_0 DPLL Initial reference delta phase[15:0]Set for a sync-in frequency of 1Khz, determinedby 1e9/sync-in freq, which is 1e9/1e3=1e61e6 in hex is 0xF4240
 *
 ******************************************************************************/
#define PLP_P1588_NSE_DPLL_6r    0x00000025
#define P1588_DPLL_Ref_PhaseDelta_0         PLP_P1588_NSE_DPLL_6r

#define PLP_P1588_NSE_DPLL_6r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_DPLL_6.
 *
 */
typedef union PLP_P1588_NSE_DPLL_6r_s {
	uint32_t v[1];
	uint32_t p1588_nse_dpll_6[1];
	uint32_t _p1588_nse_dpll_6;
} PLP_P1588_NSE_DPLL_6r_t;

#define PLP_P1588_NSE_DPLL_6r_CLR(r) (r).p1588_nse_dpll_6[0] = 0
#define PLP_P1588_NSE_DPLL_6r_SET(r,d) (r).p1588_nse_dpll_6[0] = d
#define PLP_P1588_NSE_DPLL_6r_GET(r) (r).p1588_nse_dpll_6[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_DPLL_6r_NSE_REG_REF_PHASE_DELTA_15_0f_GET(r) (((r).p1588_nse_dpll_6[0]) & 0xffff)
#define PLP_P1588_NSE_DPLL_6r_NSE_REG_REF_PHASE_DELTA_15_0f_SET(r,f) (r).p1588_nse_dpll_6[0]=(((r).p1588_nse_dpll_6[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_DPLL_6.
 *
 */
#define PLP_READ_P1588_NSE_DPLL_6r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_DPLL_6r,(_r._p1588_nse_dpll_6))
#define PLP_WRITE_P1588_NSE_DPLL_6r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_DPLL_6r,(_r._p1588_nse_dpll_6))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_DPLL_6r PLP_P1588_NSE_DPLL_6r
#define P1588_NSE_DPLL_6r_SIZE PLP_P1588_NSE_DPLL_6r_SIZE
typedef PLP_P1588_NSE_DPLL_6r_t P1588_NSE_DPLL_6r_t;
#define P1588_NSE_DPLL_6r_CLR PLP_P1588_NSE_DPLL_6r_CLR
#define P1588_NSE_DPLL_6r_SET PLP_P1588_NSE_DPLL_6r_SET
#define P1588_NSE_DPLL_6r_GET PLP_P1588_NSE_DPLL_6r_GET
#define P1588_NSE_DPLL_6r_NSE_REG_REF_PHASE_DELTA_15_0f_GET PLP_P1588_NSE_DPLL_6r_NSE_REG_REF_PHASE_DELTA_15_0f_GET
#define P1588_NSE_DPLL_6r_NSE_REG_REF_PHASE_DELTA_15_0f_SET PLP_P1588_NSE_DPLL_6r_NSE_REG_REF_PHASE_DELTA_15_0f_SET
#define READ_P1588_NSE_DPLL_6r PLP_READ_P1588_NSE_DPLL_6r
#define WRITE_P1588_NSE_DPLL_6r PLP_WRITE_P1588_NSE_DPLL_6r
#define MODIFY_P1588_NSE_DPLL_6r PLP_MODIFY_P1588_NSE_DPLL_6r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_DPLL_6r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_DPLL_7
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7026
 * DESC:     NSE DPLL 7 Register
 * RESETVAL: 0x2a (42)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_K1       DPLL K1[7:0]
 *
 ******************************************************************************/
#define PLP_P1588_NSE_DPLL_7r    0x00000026
#define P1588_DPLL_K1            PLP_P1588_NSE_DPLL_7r

#define PLP_P1588_NSE_DPLL_7r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_DPLL_7.
 *
 */
typedef union PLP_P1588_NSE_DPLL_7r_s {
	uint32_t v[1];
	uint32_t p1588_nse_dpll_7[1];
	uint32_t _p1588_nse_dpll_7;
} PLP_P1588_NSE_DPLL_7r_t;

#define PLP_P1588_NSE_DPLL_7r_CLR(r) (r).p1588_nse_dpll_7[0] = 0
#define PLP_P1588_NSE_DPLL_7r_SET(r,d) (r).p1588_nse_dpll_7[0] = d
#define PLP_P1588_NSE_DPLL_7r_GET(r) (r).p1588_nse_dpll_7[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_DPLL_7r_NSE_REG_K1f_GET(r) (((r).p1588_nse_dpll_7[0]) & 0xff)
#define PLP_P1588_NSE_DPLL_7r_NSE_REG_K1f_SET(r,f) (r).p1588_nse_dpll_7[0]=(((r).p1588_nse_dpll_7[0] & ~((uint32_t)0xff)) | (((uint32_t)f) & 0xff)) | (0xff << 16)

/*
 * These macros can be used to access P1588_NSE_DPLL_7.
 *
 */
#define PLP_READ_P1588_NSE_DPLL_7r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_DPLL_7r,(_r._p1588_nse_dpll_7))
#define PLP_WRITE_P1588_NSE_DPLL_7r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_DPLL_7r,(_r._p1588_nse_dpll_7))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_DPLL_7r PLP_P1588_NSE_DPLL_7r
#define P1588_NSE_DPLL_7r_SIZE PLP_P1588_NSE_DPLL_7r_SIZE
typedef PLP_P1588_NSE_DPLL_7r_t P1588_NSE_DPLL_7r_t;
#define P1588_NSE_DPLL_7r_CLR PLP_P1588_NSE_DPLL_7r_CLR
#define P1588_NSE_DPLL_7r_SET PLP_P1588_NSE_DPLL_7r_SET
#define P1588_NSE_DPLL_7r_GET PLP_P1588_NSE_DPLL_7r_GET
#define P1588_NSE_DPLL_7r_NSE_REG_K1f_GET PLP_P1588_NSE_DPLL_7r_NSE_REG_K1f_GET
#define P1588_NSE_DPLL_7r_NSE_REG_K1f_SET PLP_P1588_NSE_DPLL_7r_NSE_REG_K1f_SET
#define READ_P1588_NSE_DPLL_7r PLP_READ_P1588_NSE_DPLL_7r
#define WRITE_P1588_NSE_DPLL_7r PLP_WRITE_P1588_NSE_DPLL_7r
#define MODIFY_P1588_NSE_DPLL_7r PLP_MODIFY_P1588_NSE_DPLL_7r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_DPLL_7r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_DPLL_8
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7027
 * DESC:     NSE DPLL 8 Register
 * RESETVAL: 0x28 (40)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_K2       DPLL K2[7:0]
 *
 ******************************************************************************/
#define PLP_P1588_NSE_DPLL_8r    0x00000027
#define P1588_DPLL_K2            PLP_P1588_NSE_DPLL_8r

#define PLP_P1588_NSE_DPLL_8r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_DPLL_8.
 *
 */
typedef union PLP_P1588_NSE_DPLL_8r_s {
	uint32_t v[1];
	uint32_t p1588_nse_dpll_8[1];
	uint32_t _p1588_nse_dpll_8;
} PLP_P1588_NSE_DPLL_8r_t;

#define PLP_P1588_NSE_DPLL_8r_CLR(r) (r).p1588_nse_dpll_8[0] = 0
#define PLP_P1588_NSE_DPLL_8r_SET(r,d) (r).p1588_nse_dpll_8[0] = d
#define PLP_P1588_NSE_DPLL_8r_GET(r) (r).p1588_nse_dpll_8[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_DPLL_8r_NSE_REG_K2f_GET(r) (((r).p1588_nse_dpll_8[0]) & 0xff)
#define PLP_P1588_NSE_DPLL_8r_NSE_REG_K2f_SET(r,f) (r).p1588_nse_dpll_8[0]=(((r).p1588_nse_dpll_8[0] & ~((uint32_t)0xff)) | (((uint32_t)f) & 0xff)) | (0xff << 16)

/*
 * These macros can be used to access P1588_NSE_DPLL_8.
 *
 */
#define PLP_READ_P1588_NSE_DPLL_8r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_DPLL_8r,(_r._p1588_nse_dpll_8))
#define PLP_WRITE_P1588_NSE_DPLL_8r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_DPLL_8r,(_r._p1588_nse_dpll_8))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_DPLL_8r PLP_P1588_NSE_DPLL_8r
#define P1588_NSE_DPLL_8r_SIZE PLP_P1588_NSE_DPLL_8r_SIZE
typedef PLP_P1588_NSE_DPLL_8r_t P1588_NSE_DPLL_8r_t;
#define P1588_NSE_DPLL_8r_CLR PLP_P1588_NSE_DPLL_8r_CLR
#define P1588_NSE_DPLL_8r_SET PLP_P1588_NSE_DPLL_8r_SET
#define P1588_NSE_DPLL_8r_GET PLP_P1588_NSE_DPLL_8r_GET
#define P1588_NSE_DPLL_8r_NSE_REG_K2f_GET PLP_P1588_NSE_DPLL_8r_NSE_REG_K2f_GET
#define P1588_NSE_DPLL_8r_NSE_REG_K2f_SET PLP_P1588_NSE_DPLL_8r_NSE_REG_K2f_SET
#define READ_P1588_NSE_DPLL_8r PLP_READ_P1588_NSE_DPLL_8r
#define WRITE_P1588_NSE_DPLL_8r PLP_WRITE_P1588_NSE_DPLL_8r
#define MODIFY_P1588_NSE_DPLL_8r PLP_MODIFY_P1588_NSE_DPLL_8r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_DPLL_8r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_DPLL_9
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7028
 * DESC:     NSE DPLL 9 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_K3       DPLL K3[7:0]
 *
 ******************************************************************************/
#define PLP_P1588_NSE_DPLL_9r    0x00000028
#define P1588_DPLL_K3            PLP_P1588_NSE_DPLL_9r

#define PLP_P1588_NSE_DPLL_9r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_DPLL_9.
 *
 */
typedef union PLP_P1588_NSE_DPLL_9r_s {
	uint32_t v[1];
	uint32_t p1588_nse_dpll_9[1];
	uint32_t _p1588_nse_dpll_9;
} PLP_P1588_NSE_DPLL_9r_t;

#define PLP_P1588_NSE_DPLL_9r_CLR(r) (r).p1588_nse_dpll_9[0] = 0
#define PLP_P1588_NSE_DPLL_9r_SET(r,d) (r).p1588_nse_dpll_9[0] = d
#define PLP_P1588_NSE_DPLL_9r_GET(r) (r).p1588_nse_dpll_9[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_DPLL_9r_NSE_REG_K3f_GET(r) (((r).p1588_nse_dpll_9[0]) & 0xff)
#define PLP_P1588_NSE_DPLL_9r_NSE_REG_K3f_SET(r,f) (r).p1588_nse_dpll_9[0]=(((r).p1588_nse_dpll_9[0] & ~((uint32_t)0xff)) | (((uint32_t)f) & 0xff)) | (0xff << 16)

/*
 * These macros can be used to access P1588_NSE_DPLL_9.
 *
 */
#define PLP_READ_P1588_NSE_DPLL_9r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_DPLL_9r,(_r._p1588_nse_dpll_9))
#define PLP_WRITE_P1588_NSE_DPLL_9r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_DPLL_9r,(_r._p1588_nse_dpll_9))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_DPLL_9r PLP_P1588_NSE_DPLL_9r
#define P1588_NSE_DPLL_9r_SIZE PLP_P1588_NSE_DPLL_9r_SIZE
typedef PLP_P1588_NSE_DPLL_9r_t P1588_NSE_DPLL_9r_t;
#define P1588_NSE_DPLL_9r_CLR PLP_P1588_NSE_DPLL_9r_CLR
#define P1588_NSE_DPLL_9r_SET PLP_P1588_NSE_DPLL_9r_SET
#define P1588_NSE_DPLL_9r_GET PLP_P1588_NSE_DPLL_9r_GET
#define P1588_NSE_DPLL_9r_NSE_REG_K3f_GET PLP_P1588_NSE_DPLL_9r_NSE_REG_K3f_GET
#define P1588_NSE_DPLL_9r_NSE_REG_K3f_SET PLP_P1588_NSE_DPLL_9r_NSE_REG_K3f_SET
#define READ_P1588_NSE_DPLL_9r PLP_READ_P1588_NSE_DPLL_9r
#define WRITE_P1588_NSE_DPLL_9r PLP_WRITE_P1588_NSE_DPLL_9r
#define MODIFY_P1588_NSE_DPLL_9r PLP_MODIFY_P1588_NSE_DPLL_9r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_DPLL_9r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_DPLL_10
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7029
 * DESC:     NSE DPLL 10 Register
 * RESETVAL: 0x6666 (26214)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_LOOPFILTER_63_48 DPLL initial loopfilter[63:48]note to programmer:  override default value with this value 0x7c1b for 128.92 MHz
 *
 ******************************************************************************/
#define PLP_P1588_NSE_DPLL_10r    0x00000029
#define P1588_DPLL_Init_LoopFilter_63           PLP_P1588_NSE_DPLL_10r

#define PLP_P1588_NSE_DPLL_10r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_DPLL_10.
 *
 */
typedef union PLP_P1588_NSE_DPLL_10r_s {
	uint32_t v[1];
	uint32_t p1588_nse_dpll_10[1];
	uint32_t _p1588_nse_dpll_10;
} PLP_P1588_NSE_DPLL_10r_t;

#define PLP_P1588_NSE_DPLL_10r_CLR(r) (r).p1588_nse_dpll_10[0] = 0
#define PLP_P1588_NSE_DPLL_10r_SET(r,d) (r).p1588_nse_dpll_10[0] = d
#define PLP_P1588_NSE_DPLL_10r_GET(r) (r).p1588_nse_dpll_10[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_DPLL_10r_NSE_REG_LOOPFILTER_63_48f_GET(r) (((r).p1588_nse_dpll_10[0]) & 0xffff)
#define PLP_P1588_NSE_DPLL_10r_NSE_REG_LOOPFILTER_63_48f_SET(r,f) (r).p1588_nse_dpll_10[0]=(((r).p1588_nse_dpll_10[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_DPLL_10.
 *
 */
#define PLP_READ_P1588_NSE_DPLL_10r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_DPLL_10r,(_r._p1588_nse_dpll_10))
#define PLP_WRITE_P1588_NSE_DPLL_10r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_DPLL_10r,(_r._p1588_nse_dpll_10))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_DPLL_10r PLP_P1588_NSE_DPLL_10r
#define P1588_NSE_DPLL_10r_SIZE PLP_P1588_NSE_DPLL_10r_SIZE
typedef PLP_P1588_NSE_DPLL_10r_t P1588_NSE_DPLL_10r_t;
#define P1588_NSE_DPLL_10r_CLR PLP_P1588_NSE_DPLL_10r_CLR
#define P1588_NSE_DPLL_10r_SET PLP_P1588_NSE_DPLL_10r_SET
#define P1588_NSE_DPLL_10r_GET PLP_P1588_NSE_DPLL_10r_GET
#define P1588_NSE_DPLL_10r_NSE_REG_LOOPFILTER_63_48f_GET PLP_P1588_NSE_DPLL_10r_NSE_REG_LOOPFILTER_63_48f_GET
#define P1588_NSE_DPLL_10r_NSE_REG_LOOPFILTER_63_48f_SET PLP_P1588_NSE_DPLL_10r_NSE_REG_LOOPFILTER_63_48f_SET
#define READ_P1588_NSE_DPLL_10r PLP_READ_P1588_NSE_DPLL_10r
#define WRITE_P1588_NSE_DPLL_10r PLP_WRITE_P1588_NSE_DPLL_10r
#define MODIFY_P1588_NSE_DPLL_10r PLP_MODIFY_P1588_NSE_DPLL_10r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_DPLL_10r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_DPLL_11
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x702a
 * DESC:     NSE DPLL 11 Register
 * RESETVAL: 0x6666 (26214)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_LOOPFILTER_47_32 DPLL initial loopfilter[47:32]note to programmer:  override default value with this value 0xe345 for 128.92 MHz
 *
 ******************************************************************************/
#define PLP_P1588_NSE_DPLL_11r    0x0000002a

#define PLP_P1588_NSE_DPLL_11r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_DPLL_11.
 *
 */
typedef union PLP_P1588_NSE_DPLL_11r_s {
	uint32_t v[1];
	uint32_t p1588_nse_dpll_11[1];
	uint32_t _p1588_nse_dpll_11;
} PLP_P1588_NSE_DPLL_11r_t;

#define PLP_P1588_NSE_DPLL_11r_CLR(r) (r).p1588_nse_dpll_11[0] = 0
#define PLP_P1588_NSE_DPLL_11r_SET(r,d) (r).p1588_nse_dpll_11[0] = d
#define PLP_P1588_NSE_DPLL_11r_GET(r) (r).p1588_nse_dpll_11[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_DPLL_11r_NSE_REG_LOOPFILTER_47_32f_GET(r) (((r).p1588_nse_dpll_11[0]) & 0xffff)
#define PLP_P1588_NSE_DPLL_11r_NSE_REG_LOOPFILTER_47_32f_SET(r,f) (r).p1588_nse_dpll_11[0]=(((r).p1588_nse_dpll_11[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_DPLL_11.
 *
 */
#define PLP_READ_P1588_NSE_DPLL_11r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_DPLL_11r,(_r._p1588_nse_dpll_11))
#define PLP_WRITE_P1588_NSE_DPLL_11r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_DPLL_11r,(_r._p1588_nse_dpll_11))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_DPLL_11r PLP_P1588_NSE_DPLL_11r
#define P1588_NSE_DPLL_11r_SIZE PLP_P1588_NSE_DPLL_11r_SIZE
typedef PLP_P1588_NSE_DPLL_11r_t P1588_NSE_DPLL_11r_t;
#define P1588_NSE_DPLL_11r_CLR PLP_P1588_NSE_DPLL_11r_CLR
#define P1588_NSE_DPLL_11r_SET PLP_P1588_NSE_DPLL_11r_SET
#define P1588_NSE_DPLL_11r_GET PLP_P1588_NSE_DPLL_11r_GET
#define P1588_NSE_DPLL_11r_NSE_REG_LOOPFILTER_47_32f_GET PLP_P1588_NSE_DPLL_11r_NSE_REG_LOOPFILTER_47_32f_GET
#define P1588_NSE_DPLL_11r_NSE_REG_LOOPFILTER_47_32f_SET PLP_P1588_NSE_DPLL_11r_NSE_REG_LOOPFILTER_47_32f_SET
#define READ_P1588_NSE_DPLL_11r PLP_READ_P1588_NSE_DPLL_11r
#define WRITE_P1588_NSE_DPLL_11r PLP_WRITE_P1588_NSE_DPLL_11r
#define MODIFY_P1588_NSE_DPLL_11r PLP_MODIFY_P1588_NSE_DPLL_11r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_DPLL_11r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_DPLL_12
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x702b
 * DESC:     NSE DPLL 12 Register
 * RESETVAL: 0x6666 (26214)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_LOOPFILTER_31_16 DPLL initial loopfilter[31:16]note to programmer:  override default value with this value 0x3072 for 128.92 MHz
 *
 ******************************************************************************/
#define PLP_P1588_NSE_DPLL_12r    0x0000002b

#define PLP_P1588_NSE_DPLL_12r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_DPLL_12.
 *
 */
typedef union PLP_P1588_NSE_DPLL_12r_s {
	uint32_t v[1];
	uint32_t p1588_nse_dpll_12[1];
	uint32_t _p1588_nse_dpll_12;
} PLP_P1588_NSE_DPLL_12r_t;

#define PLP_P1588_NSE_DPLL_12r_CLR(r) (r).p1588_nse_dpll_12[0] = 0
#define PLP_P1588_NSE_DPLL_12r_SET(r,d) (r).p1588_nse_dpll_12[0] = d
#define PLP_P1588_NSE_DPLL_12r_GET(r) (r).p1588_nse_dpll_12[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_DPLL_12r_NSE_REG_LOOPFILTER_31_16f_GET(r) (((r).p1588_nse_dpll_12[0]) & 0xffff)
#define PLP_P1588_NSE_DPLL_12r_NSE_REG_LOOPFILTER_31_16f_SET(r,f) (r).p1588_nse_dpll_12[0]=(((r).p1588_nse_dpll_12[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_DPLL_12.
 *
 */
#define PLP_READ_P1588_NSE_DPLL_12r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_DPLL_12r,(_r._p1588_nse_dpll_12))
#define PLP_WRITE_P1588_NSE_DPLL_12r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_DPLL_12r,(_r._p1588_nse_dpll_12))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_DPLL_12r PLP_P1588_NSE_DPLL_12r
#define P1588_NSE_DPLL_12r_SIZE PLP_P1588_NSE_DPLL_12r_SIZE
typedef PLP_P1588_NSE_DPLL_12r_t P1588_NSE_DPLL_12r_t;
#define P1588_NSE_DPLL_12r_CLR PLP_P1588_NSE_DPLL_12r_CLR
#define P1588_NSE_DPLL_12r_SET PLP_P1588_NSE_DPLL_12r_SET
#define P1588_NSE_DPLL_12r_GET PLP_P1588_NSE_DPLL_12r_GET
#define P1588_NSE_DPLL_12r_NSE_REG_LOOPFILTER_31_16f_GET PLP_P1588_NSE_DPLL_12r_NSE_REG_LOOPFILTER_31_16f_GET
#define P1588_NSE_DPLL_12r_NSE_REG_LOOPFILTER_31_16f_SET PLP_P1588_NSE_DPLL_12r_NSE_REG_LOOPFILTER_31_16f_SET
#define READ_P1588_NSE_DPLL_12r PLP_READ_P1588_NSE_DPLL_12r
#define WRITE_P1588_NSE_DPLL_12r PLP_WRITE_P1588_NSE_DPLL_12r
#define MODIFY_P1588_NSE_DPLL_12r PLP_MODIFY_P1588_NSE_DPLL_12r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_DPLL_12r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_DPLL_13
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x702c
 * DESC:     NSE DPLL 13 Register
 * RESETVAL: 0x6666 (26214)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_LOOPFILTER_15_0 DPLL initial loopfilter[15:0]note to programmer:  override default value with this value 0x384d for 128.92 MHz
 *
 ******************************************************************************/
#define PLP_P1588_NSE_DPLL_13r    0x0000002c
#define P1588_DPLL_Init_LoopFilter_0            PLP_P1588_NSE_DPLL_13r

#define PLP_P1588_NSE_DPLL_13r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_DPLL_13.
 *
 */
typedef union PLP_P1588_NSE_DPLL_13r_s {
	uint32_t v[1];
	uint32_t p1588_nse_dpll_13[1];
	uint32_t _p1588_nse_dpll_13;
} PLP_P1588_NSE_DPLL_13r_t;

#define PLP_P1588_NSE_DPLL_13r_CLR(r) (r).p1588_nse_dpll_13[0] = 0
#define PLP_P1588_NSE_DPLL_13r_SET(r,d) (r).p1588_nse_dpll_13[0] = d
#define PLP_P1588_NSE_DPLL_13r_GET(r) (r).p1588_nse_dpll_13[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_DPLL_13r_NSE_REG_LOOPFILTER_15_0f_GET(r) (((r).p1588_nse_dpll_13[0]) & 0xffff)
#define PLP_P1588_NSE_DPLL_13r_NSE_REG_LOOPFILTER_15_0f_SET(r,f) (r).p1588_nse_dpll_13[0]=(((r).p1588_nse_dpll_13[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_DPLL_13.
 *
 */
#define PLP_READ_P1588_NSE_DPLL_13r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_DPLL_13r,(_r._p1588_nse_dpll_13))
#define PLP_WRITE_P1588_NSE_DPLL_13r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_DPLL_13r,(_r._p1588_nse_dpll_13))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_DPLL_13r PLP_P1588_NSE_DPLL_13r
#define P1588_NSE_DPLL_13r_SIZE PLP_P1588_NSE_DPLL_13r_SIZE
typedef PLP_P1588_NSE_DPLL_13r_t P1588_NSE_DPLL_13r_t;
#define P1588_NSE_DPLL_13r_CLR PLP_P1588_NSE_DPLL_13r_CLR
#define P1588_NSE_DPLL_13r_SET PLP_P1588_NSE_DPLL_13r_SET
#define P1588_NSE_DPLL_13r_GET PLP_P1588_NSE_DPLL_13r_GET
#define P1588_NSE_DPLL_13r_NSE_REG_LOOPFILTER_15_0f_GET PLP_P1588_NSE_DPLL_13r_NSE_REG_LOOPFILTER_15_0f_GET
#define P1588_NSE_DPLL_13r_NSE_REG_LOOPFILTER_15_0f_SET PLP_P1588_NSE_DPLL_13r_NSE_REG_LOOPFILTER_15_0f_SET
#define READ_P1588_NSE_DPLL_13r PLP_READ_P1588_NSE_DPLL_13r
#define WRITE_P1588_NSE_DPLL_13r PLP_WRITE_P1588_NSE_DPLL_13r
#define MODIFY_P1588_NSE_DPLL_13r PLP_MODIFY_P1588_NSE_DPLL_13r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_DPLL_13r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_DPLL_14
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x702d
 * DESC:     NSE DPLL 14 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TESTBUS_P1588    Testbus read data. The read select is controlledby nse_reg_testbus_sel[4:0]
 *
 ******************************************************************************/
#define PLP_P1588_NSE_DPLL_14r    0x0000002d

#define PLP_P1588_NSE_DPLL_14r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_DPLL_14.
 *
 */
typedef union PLP_P1588_NSE_DPLL_14r_s {
	uint32_t v[1];
	uint32_t p1588_nse_dpll_14[1];
	uint32_t _p1588_nse_dpll_14;
} PLP_P1588_NSE_DPLL_14r_t;

#define PLP_P1588_NSE_DPLL_14r_CLR(r) (r).p1588_nse_dpll_14[0] = 0
#define PLP_P1588_NSE_DPLL_14r_SET(r,d) (r).p1588_nse_dpll_14[0] = d
#define PLP_P1588_NSE_DPLL_14r_GET(r) (r).p1588_nse_dpll_14[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_DPLL_14r_TESTBUS_P1588f_GET(r) (((r).p1588_nse_dpll_14[0]) & 0xffff)
#define PLP_P1588_NSE_DPLL_14r_TESTBUS_P1588f_SET(r,f) (r).p1588_nse_dpll_14[0]=(((r).p1588_nse_dpll_14[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_DPLL_14.
 *
 */
#define PLP_READ_P1588_NSE_DPLL_14r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_DPLL_14r,(_r._p1588_nse_dpll_14))
#define PLP_WRITE_P1588_NSE_DPLL_14r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_DPLL_14r,(_r._p1588_nse_dpll_14))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_DPLL_14r PLP_P1588_NSE_DPLL_14r
#define P1588_NSE_DPLL_14r_SIZE PLP_P1588_NSE_DPLL_14r_SIZE
typedef PLP_P1588_NSE_DPLL_14r_t P1588_NSE_DPLL_14r_t;
#define P1588_NSE_DPLL_14r_CLR PLP_P1588_NSE_DPLL_14r_CLR
#define P1588_NSE_DPLL_14r_SET PLP_P1588_NSE_DPLL_14r_SET
#define P1588_NSE_DPLL_14r_GET PLP_P1588_NSE_DPLL_14r_GET
#define P1588_NSE_DPLL_14r_TESTBUS_P1588f_GET PLP_P1588_NSE_DPLL_14r_TESTBUS_P1588f_GET
#define P1588_NSE_DPLL_14r_TESTBUS_P1588f_SET PLP_P1588_NSE_DPLL_14r_TESTBUS_P1588f_SET
#define READ_P1588_NSE_DPLL_14r PLP_READ_P1588_NSE_DPLL_14r
#define WRITE_P1588_NSE_DPLL_14r PLP_WRITE_P1588_NSE_DPLL_14r
#define MODIFY_P1588_NSE_DPLL_14r PLP_MODIFY_P1588_NSE_DPLL_14r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_DPLL_14r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_NCO_1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x702e
 * DESC:     NSE NCO 1 Register
 * RESETVAL: 0x7c1f (31775)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_NCO_FREQCNTRL_31_16 NCO Frequency Stepping control[31:16]note to programmer:  override default value with this value 0x7c1b for 128.92 MHzValid when freq_mdio_sel is 1
 *
 ******************************************************************************/
#define PLP_P1588_NSE_NCO_1r    0x0000002e
#define P1588_NCO_Freq_Step_Ctrl_31         PLP_P1588_NSE_NCO_1r

#define PLP_P1588_NSE_NCO_1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_NCO_1.
 *
 */
typedef union PLP_P1588_NSE_NCO_1r_s {
	uint32_t v[1];
	uint32_t p1588_nse_nco_1[1];
	uint32_t _p1588_nse_nco_1;
} PLP_P1588_NSE_NCO_1r_t;

#define PLP_P1588_NSE_NCO_1r_CLR(r) (r).p1588_nse_nco_1[0] = 0
#define PLP_P1588_NSE_NCO_1r_SET(r,d) (r).p1588_nse_nco_1[0] = d
#define PLP_P1588_NSE_NCO_1r_GET(r) (r).p1588_nse_nco_1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_NCO_1r_NSE_REG_NCO_FREQCNTRL_31_16f_GET(r) (((r).p1588_nse_nco_1[0]) & 0xffff)
#define PLP_P1588_NSE_NCO_1r_NSE_REG_NCO_FREQCNTRL_31_16f_SET(r,f) (r).p1588_nse_nco_1[0]=(((r).p1588_nse_nco_1[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_NCO_1.
 *
 */
#define PLP_READ_P1588_NSE_NCO_1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_NCO_1r,(_r._p1588_nse_nco_1))
#define PLP_WRITE_P1588_NSE_NCO_1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_NCO_1r,(_r._p1588_nse_nco_1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_NCO_1r PLP_P1588_NSE_NCO_1r
#define P1588_NSE_NCO_1r_SIZE PLP_P1588_NSE_NCO_1r_SIZE
typedef PLP_P1588_NSE_NCO_1r_t P1588_NSE_NCO_1r_t;
#define P1588_NSE_NCO_1r_CLR PLP_P1588_NSE_NCO_1r_CLR
#define P1588_NSE_NCO_1r_SET PLP_P1588_NSE_NCO_1r_SET
#define P1588_NSE_NCO_1r_GET PLP_P1588_NSE_NCO_1r_GET
#define P1588_NSE_NCO_1r_NSE_REG_NCO_FREQCNTRL_31_16f_GET PLP_P1588_NSE_NCO_1r_NSE_REG_NCO_FREQCNTRL_31_16f_GET
#define P1588_NSE_NCO_1r_NSE_REG_NCO_FREQCNTRL_31_16f_SET PLP_P1588_NSE_NCO_1r_NSE_REG_NCO_FREQCNTRL_31_16f_SET
#define READ_P1588_NSE_NCO_1r PLP_READ_P1588_NSE_NCO_1r
#define WRITE_P1588_NSE_NCO_1r PLP_WRITE_P1588_NSE_NCO_1r
#define MODIFY_P1588_NSE_NCO_1r PLP_MODIFY_P1588_NSE_NCO_1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_NCO_1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_NCO_2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x702f
 * DESC:     NSE NCO 2 Register
 * RESETVAL: 0x7c2 (1986)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_NCO_FREQCNTRL_15_0 NCO Frequency Stepping control[15:0]note to programmer:  override default value with this value 0xe345 for 128.92 MHzValid when freq_mdio_sel is 1
 *
 ******************************************************************************/
#define PLP_P1588_NSE_NCO_2r    0x0000002f
#define P1588_NCO_Freq_Step_Ctrl_0          PLP_P1588_NSE_NCO_2r

#define PLP_P1588_NSE_NCO_2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_NCO_2.
 *
 */
typedef union PLP_P1588_NSE_NCO_2r_s {
	uint32_t v[1];
	uint32_t p1588_nse_nco_2[1];
	uint32_t _p1588_nse_nco_2;
} PLP_P1588_NSE_NCO_2r_t;

#define PLP_P1588_NSE_NCO_2r_CLR(r) (r).p1588_nse_nco_2[0] = 0
#define PLP_P1588_NSE_NCO_2r_SET(r,d) (r).p1588_nse_nco_2[0] = d
#define PLP_P1588_NSE_NCO_2r_GET(r) (r).p1588_nse_nco_2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_NCO_2r_NSE_REG_NCO_FREQCNTRL_15_0f_GET(r) (((r).p1588_nse_nco_2[0]) & 0xffff)
#define PLP_P1588_NSE_NCO_2r_NSE_REG_NCO_FREQCNTRL_15_0f_SET(r,f) (r).p1588_nse_nco_2[0]=(((r).p1588_nse_nco_2[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_NCO_2.
 *
 */
#define PLP_READ_P1588_NSE_NCO_2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_NCO_2r,(_r._p1588_nse_nco_2))
#define PLP_WRITE_P1588_NSE_NCO_2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_NCO_2r,(_r._p1588_nse_nco_2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_NCO_2r PLP_P1588_NSE_NCO_2r
#define P1588_NSE_NCO_2r_SIZE PLP_P1588_NSE_NCO_2r_SIZE
typedef PLP_P1588_NSE_NCO_2r_t P1588_NSE_NCO_2r_t;
#define P1588_NSE_NCO_2r_CLR PLP_P1588_NSE_NCO_2r_CLR
#define P1588_NSE_NCO_2r_SET PLP_P1588_NSE_NCO_2r_SET
#define P1588_NSE_NCO_2r_GET PLP_P1588_NSE_NCO_2r_GET
#define P1588_NSE_NCO_2r_NSE_REG_NCO_FREQCNTRL_15_00f_GET PLP_P1588_NSE_NCO_2r_NSE_REG_NCO_FREQCNTRL_15_0f_GET
#define P1588_NSE_NCO_2r_NSE_REG_NCO_FREQCNTRL_15_00f_SET PLP_P1588_NSE_NCO_2r_NSE_REG_NCO_FREQCNTRL_15_0f_SET
#define READ_P1588_NSE_NCO_2r PLP_READ_P1588_NSE_NCO_2r
#define WRITE_P1588_NSE_NCO_2r PLP_WRITE_P1588_NSE_NCO_2r
#define MODIFY_P1588_NSE_NCO_2r PLP_MODIFY_P1588_NSE_NCO_2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_NCO_2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_NCO_3
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7030
 * DESC:     NSE NCO 3 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_NCO_LT_CNTRL_47_32 Register containing sync value of 48-bit local timer bits [47:32]
 *
 ******************************************************************************/
#define PLP_P1588_NSE_NCO_3r    0x00000030

#define PLP_P1588_NSE_NCO_3r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_NCO_3.
 *
 */
typedef union PLP_P1588_NSE_NCO_3r_s {
	uint32_t v[1];
	uint32_t p1588_nse_nco_3[1];
	uint32_t _p1588_nse_nco_3;
} PLP_P1588_NSE_NCO_3r_t;

#define PLP_P1588_NSE_NCO_3r_CLR(r) (r).p1588_nse_nco_3[0] = 0
#define PLP_P1588_NSE_NCO_3r_SET(r,d) (r).p1588_nse_nco_3[0] = d
#define PLP_P1588_NSE_NCO_3r_GET(r) (r).p1588_nse_nco_3[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_NCO_3r_NSE_REG_NCO_LT_CNTRL_47_32f_GET(r) (((r).p1588_nse_nco_3[0]) & 0xffff)
#define PLP_P1588_NSE_NCO_3r_NSE_REG_NCO_LT_CNTRL_47_32f_SET(r,f) (r).p1588_nse_nco_3[0]=(((r).p1588_nse_nco_3[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_NCO_3.
 *
 */
#define PLP_READ_P1588_NSE_NCO_3r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_NCO_3r,(_r._p1588_nse_nco_3))
#define PLP_WRITE_P1588_NSE_NCO_3r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_NCO_3r,(_r._p1588_nse_nco_3))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_NCO_3r PLP_P1588_NSE_NCO_3r
#define P1588_NSE_NCO_3r_SIZE PLP_P1588_NSE_NCO_3r_SIZE
typedef PLP_P1588_NSE_NCO_3r_t P1588_NSE_NCO_3r_t;
#define P1588_NSE_NCO_3r_CLR PLP_P1588_NSE_NCO_3r_CLR
#define P1588_NSE_NCO_3r_SET PLP_P1588_NSE_NCO_3r_SET
#define P1588_NSE_NCO_3r_GET PLP_P1588_NSE_NCO_3r_GET
#define P1588_NSE_NCO_3r_NSE_REG_NCO_LT_CNTRL_47_32f_GET PLP_P1588_NSE_NCO_3r_NSE_REG_NCO_LT_CNTRL_47_32f_GET
#define P1588_NSE_NCO_3r_NSE_REG_NCO_LT_CNTRL_47_32f_SET PLP_P1588_NSE_NCO_3r_NSE_REG_NCO_LT_CNTRL_47_32f_SET
#define READ_P1588_NSE_NCO_3r PLP_READ_P1588_NSE_NCO_3r
#define WRITE_P1588_NSE_NCO_3r PLP_WRITE_P1588_NSE_NCO_3r
#define MODIFY_P1588_NSE_NCO_3r PLP_MODIFY_P1588_NSE_NCO_3r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_NCO_3r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_NCO_4
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7031
 * DESC:     NSE NCO 4 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_NCO_LT_CNTRL_31_16 Register containing sync value of 48-bit local timer bits [31:16]
 *
 ******************************************************************************/
#define PLP_P1588_NSE_NCO_4r    0x00000031

#define PLP_P1588_NSE_NCO_4r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_NCO_4.
 *
 */
typedef union PLP_P1588_NSE_NCO_4r_s {
	uint32_t v[1];
	uint32_t p1588_nse_nco_4[1];
	uint32_t _p1588_nse_nco_4;
} PLP_P1588_NSE_NCO_4r_t;

#define PLP_P1588_NSE_NCO_4r_CLR(r) (r).p1588_nse_nco_4[0] = 0
#define PLP_P1588_NSE_NCO_4r_SET(r,d) (r).p1588_nse_nco_4[0] = d
#define PLP_P1588_NSE_NCO_4r_GET(r) (r).p1588_nse_nco_4[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_NCO_4r_NSE_REG_NCO_LT_CNTRL_31_16f_GET(r) (((r).p1588_nse_nco_4[0]) & 0xffff)
#define PLP_P1588_NSE_NCO_4r_NSE_REG_NCO_LT_CNTRL_31_16f_SET(r,f) (r).p1588_nse_nco_4[0]=(((r).p1588_nse_nco_4[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_NCO_4.
 *
 */
#define PLP_READ_P1588_NSE_NCO_4r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_NCO_4r,(_r._p1588_nse_nco_4))
#define PLP_WRITE_P1588_NSE_NCO_4r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_NCO_4r,(_r._p1588_nse_nco_4))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_NCO_4r PLP_P1588_NSE_NCO_4r
#define P1588_NSE_NCO_4r_SIZE PLP_P1588_NSE_NCO_4r_SIZE
typedef PLP_P1588_NSE_NCO_4r_t P1588_NSE_NCO_4r_t;
#define P1588_NSE_NCO_4r_CLR PLP_P1588_NSE_NCO_4r_CLR
#define P1588_NSE_NCO_4r_SET PLP_P1588_NSE_NCO_4r_SET
#define P1588_NSE_NCO_4r_GET PLP_P1588_NSE_NCO_4r_GET
#define P1588_NSE_NCO_4r_NSE_REG_NCO_LT_CNTRL_31_16f_GET PLP_P1588_NSE_NCO_4r_NSE_REG_NCO_LT_CNTRL_31_16f_GET
#define P1588_NSE_NCO_4r_NSE_REG_NCO_LT_CNTRL_31_16f_SET PLP_P1588_NSE_NCO_4r_NSE_REG_NCO_LT_CNTRL_31_16f_SET
#define READ_P1588_NSE_NCO_4r PLP_READ_P1588_NSE_NCO_4r
#define WRITE_P1588_NSE_NCO_4r PLP_WRITE_P1588_NSE_NCO_4r
#define MODIFY_P1588_NSE_NCO_4r PLP_MODIFY_P1588_NSE_NCO_4r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_NCO_4r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_NCO_5
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7032
 * DESC:     NSE NCO 5 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_NCO_LT_CNTRL_15_0 Register containing sync value of 48-bit local timer bits [15:0]
 *
 ******************************************************************************/
#define PLP_P1588_NSE_NCO_5r    0x00000032

#define PLP_P1588_NSE_NCO_5r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_NCO_5.
 *
 */
typedef union PLP_P1588_NSE_NCO_5r_s {
	uint32_t v[1];
	uint32_t p1588_nse_nco_5[1];
	uint32_t _p1588_nse_nco_5;
} PLP_P1588_NSE_NCO_5r_t;

#define PLP_P1588_NSE_NCO_5r_CLR(r) (r).p1588_nse_nco_5[0] = 0
#define PLP_P1588_NSE_NCO_5r_SET(r,d) (r).p1588_nse_nco_5[0] = d
#define PLP_P1588_NSE_NCO_5r_GET(r) (r).p1588_nse_nco_5[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_NCO_5r_NSE_REG_NCO_LT_CNTRL_15_0f_GET(r) (((r).p1588_nse_nco_5[0]) & 0xffff)
#define PLP_P1588_NSE_NCO_5r_NSE_REG_NCO_LT_CNTRL_15_0f_SET(r,f) (r).p1588_nse_nco_5[0]=(((r).p1588_nse_nco_5[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_NCO_5.
 *
 */
#define PLP_READ_P1588_NSE_NCO_5r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_NCO_5r,(_r._p1588_nse_nco_5))
#define PLP_WRITE_P1588_NSE_NCO_5r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_NCO_5r,(_r._p1588_nse_nco_5))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_NCO_5r PLP_P1588_NSE_NCO_5r
#define P1588_NSE_NCO_5r_SIZE PLP_P1588_NSE_NCO_5r_SIZE
typedef PLP_P1588_NSE_NCO_5r_t P1588_NSE_NCO_5r_t;
#define P1588_NSE_NCO_5r_CLR PLP_P1588_NSE_NCO_5r_CLR
#define P1588_NSE_NCO_5r_SET PLP_P1588_NSE_NCO_5r_SET
#define P1588_NSE_NCO_5r_GET PLP_P1588_NSE_NCO_5r_GET
#define P1588_NSE_NCO_5r_NSE_REG_NCO_LT_CNTRL_15_0f_GET PLP_P1588_NSE_NCO_5r_NSE_REG_NCO_LT_CNTRL_15_0f_GET
#define P1588_NSE_NCO_5r_NSE_REG_NCO_LT_CNTRL_15_0f_SET PLP_P1588_NSE_NCO_5r_NSE_REG_NCO_LT_CNTRL_15_0f_SET
#define READ_P1588_NSE_NCO_5r PLP_READ_P1588_NSE_NCO_5r
#define WRITE_P1588_NSE_NCO_5r PLP_WRITE_P1588_NSE_NCO_5r
#define MODIFY_P1588_NSE_NCO_5r PLP_MODIFY_P1588_NSE_NCO_5r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_NCO_5r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_SC_1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7033
 * DESC:     NSE SC 1 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_SC_SYNCOUT_CNTRL_38_32 Pulse train length[8:2] register
 *     NSE_REG_SC_SYNCOUT_CNTRL_47_39 Framesync pulse length[8:0] registereach count is ~7.7568 ns
 *
 ******************************************************************************/
#define PLP_P1588_NSE_SC_1r    0x00000033

#define PLP_P1588_NSE_SC_1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_SC_1.
 *
 */
typedef union PLP_P1588_NSE_SC_1r_s {
	uint32_t v[1];
	uint32_t p1588_nse_sc_1[1];
	uint32_t _p1588_nse_sc_1;
} PLP_P1588_NSE_SC_1r_t;

#define PLP_P1588_NSE_SC_1r_CLR(r) (r).p1588_nse_sc_1[0] = 0
#define PLP_P1588_NSE_SC_1r_SET(r,d) (r).p1588_nse_sc_1[0] = d
#define PLP_P1588_NSE_SC_1r_GET(r) (r).p1588_nse_sc_1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_SC_1r_NSE_REG_SC_SYNCOUT_CNTRL_47_39f_GET(r) ((((r).p1588_nse_sc_1[0]) >> 7) & 0x1ff)
#define PLP_P1588_NSE_SC_1r_NSE_REG_SC_SYNCOUT_CNTRL_47_39f_SET(r,f) (r).p1588_nse_sc_1[0]=(((r).p1588_nse_sc_1[0] & ~((uint32_t)0x1ff << 7)) | ((((uint32_t)f) & 0x1ff) << 7)) | (511 << (16 + 7))
#define PLP_P1588_NSE_SC_1r_NSE_REG_SC_SYNCOUT_CNTRL_38_32f_GET(r) (((r).p1588_nse_sc_1[0]) & 0x7f)
#define PLP_P1588_NSE_SC_1r_NSE_REG_SC_SYNCOUT_CNTRL_38_32f_SET(r,f) (r).p1588_nse_sc_1[0]=(((r).p1588_nse_sc_1[0] & ~((uint32_t)0x7f)) | (((uint32_t)f) & 0x7f)) | (0x7f << 16)

/*
 * These macros can be used to access P1588_NSE_SC_1.
 *
 */
#define PLP_READ_P1588_NSE_SC_1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_SC_1r,(_r._p1588_nse_sc_1))
#define PLP_WRITE_P1588_NSE_SC_1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_SC_1r,(_r._p1588_nse_sc_1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_SC_1r PLP_P1588_NSE_SC_1r
#define P1588_NSE_SC_1r_SIZE PLP_P1588_NSE_SC_1r_SIZE
typedef PLP_P1588_NSE_SC_1r_t P1588_NSE_SC_1r_t;
#define P1588_NSE_SC_1r_CLR PLP_P1588_NSE_SC_1r_CLR
#define P1588_NSE_SC_1r_SET PLP_P1588_NSE_SC_1r_SET
#define P1588_NSE_SC_1r_GET PLP_P1588_NSE_SC_1r_GET
#define P1588_NSE_SC_1r_NSE_REG_SC_SYNCOUT_CNTRL_47_39f_GET PLP_P1588_NSE_SC_1r_NSE_REG_SC_SYNCOUT_CNTRL_47_39f_GET
#define P1588_NSE_SC_1r_NSE_REG_SC_SYNCOUT_CNTRL_47_39f_SET PLP_P1588_NSE_SC_1r_NSE_REG_SC_SYNCOUT_CNTRL_47_39f_SET
#define P1588_NSE_SC_1r_NSE_REG_SC_SYNCOUT_CNTRL_38_32f_GET PLP_P1588_NSE_SC_1r_NSE_REG_SC_SYNCOUT_CNTRL_38_32f_GET
#define P1588_NSE_SC_1r_NSE_REG_SC_SYNCOUT_CNTRL_38_32f_SET PLP_P1588_NSE_SC_1r_NSE_REG_SC_SYNCOUT_CNTRL_38_32f_SET
#define READ_P1588_NSE_SC_1r PLP_READ_P1588_NSE_SC_1r
#define WRITE_P1588_NSE_SC_1r PLP_WRITE_P1588_NSE_SC_1r
#define MODIFY_P1588_NSE_SC_1r PLP_MODIFY_P1588_NSE_SC_1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_SC_1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_SC_2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7034
 * DESC:     NSE SC 2 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_SC_SYNCOUT_CNTRL_29_16 Interval length[29:16] register
 *     NSE_REG_SC_SYNCOUT_CNTRL_31_30 Pulse train length[1:0] register
 *
 ******************************************************************************/
#define PLP_P1588_NSE_SC_2r    0x00000034

#define PLP_P1588_NSE_SC_2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_SC_2.
 *
 */
typedef union PLP_P1588_NSE_SC_2r_s {
	uint32_t v[1];
	uint32_t p1588_nse_sc_2[1];
	uint32_t _p1588_nse_sc_2;
} PLP_P1588_NSE_SC_2r_t;

#define PLP_P1588_NSE_SC_2r_CLR(r) (r).p1588_nse_sc_2[0] = 0
#define PLP_P1588_NSE_SC_2r_SET(r,d) (r).p1588_nse_sc_2[0] = d
#define PLP_P1588_NSE_SC_2r_GET(r) (r).p1588_nse_sc_2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_SC_2r_NSE_REG_SC_SYNCOUT_CNTRL_31_30f_GET(r) ((((r).p1588_nse_sc_2[0]) >> 14) & 0x3)
#define PLP_P1588_NSE_SC_2r_NSE_REG_SC_SYNCOUT_CNTRL_31_30f_SET(r,f) (r).p1588_nse_sc_2[0]=(((r).p1588_nse_sc_2[0] & ~((uint32_t)0x3 << 14)) | ((((uint32_t)f) & 0x3) << 14)) | (3 << (16 + 14))
#define PLP_P1588_NSE_SC_2r_NSE_REG_SC_SYNCOUT_CNTRL_29_16f_GET(r) (((r).p1588_nse_sc_2[0]) & 0x3fff)
#define PLP_P1588_NSE_SC_2r_NSE_REG_SC_SYNCOUT_CNTRL_29_16f_SET(r,f) (r).p1588_nse_sc_2[0]=(((r).p1588_nse_sc_2[0] & ~((uint32_t)0x3fff)) | (((uint32_t)f) & 0x3fff)) | (0x3fff << 16)

/*
 * These macros can be used to access P1588_NSE_SC_2.
 *
 */
#define PLP_READ_P1588_NSE_SC_2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_SC_2r,(_r._p1588_nse_sc_2))
#define PLP_WRITE_P1588_NSE_SC_2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_SC_2r,(_r._p1588_nse_sc_2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_SC_2r PLP_P1588_NSE_SC_2r
#define P1588_NSE_SC_2r_SIZE PLP_P1588_NSE_SC_2r_SIZE
typedef PLP_P1588_NSE_SC_2r_t P1588_NSE_SC_2r_t;
#define P1588_NSE_SC_2r_CLR PLP_P1588_NSE_SC_2r_CLR
#define P1588_NSE_SC_2r_SET PLP_P1588_NSE_SC_2r_SET
#define P1588_NSE_SC_2r_GET PLP_P1588_NSE_SC_2r_GET
#define P1588_NSE_SC_2r_NSE_REG_SC_SYNCOUT_CNTRL_31_30f_GET PLP_P1588_NSE_SC_2r_NSE_REG_SC_SYNCOUT_CNTRL_31_30f_GET
#define P1588_NSE_SC_2r_NSE_REG_SC_SYNCOUT_CNTRL_31_30f_SET PLP_P1588_NSE_SC_2r_NSE_REG_SC_SYNCOUT_CNTRL_31_30f_SET
#define P1588_NSE_SC_2r_NSE_REG_SC_SYNCOUT_CNTRL_29_16f_GET PLP_P1588_NSE_SC_2r_NSE_REG_SC_SYNCOUT_CNTRL_29_16f_GET
#define P1588_NSE_SC_2r_NSE_REG_SC_SYNCOUT_CNTRL_29_16f_SET PLP_P1588_NSE_SC_2r_NSE_REG_SC_SYNCOUT_CNTRL_29_16f_SET
#define READ_P1588_NSE_SC_2r PLP_READ_P1588_NSE_SC_2r
#define WRITE_P1588_NSE_SC_2r PLP_WRITE_P1588_NSE_SC_2r
#define MODIFY_P1588_NSE_SC_2r PLP_MODIFY_P1588_NSE_SC_2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_SC_2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_SC_3
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7035
 * DESC:     NSE SC 3 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_SC_SYNCOUT_CNTRL_15_0 Interval length[15:0] register
 *
 ******************************************************************************/
#define PLP_P1588_NSE_SC_3r    0x00000035

#define PLP_P1588_NSE_SC_3r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_SC_3.
 *
 */
typedef union PLP_P1588_NSE_SC_3r_s {
	uint32_t v[1];
	uint32_t p1588_nse_sc_3[1];
	uint32_t _p1588_nse_sc_3;
} PLP_P1588_NSE_SC_3r_t;

#define PLP_P1588_NSE_SC_3r_CLR(r) (r).p1588_nse_sc_3[0] = 0
#define PLP_P1588_NSE_SC_3r_SET(r,d) (r).p1588_nse_sc_3[0] = d
#define PLP_P1588_NSE_SC_3r_GET(r) (r).p1588_nse_sc_3[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_SC_3r_NSE_REG_SC_SYNCOUT_CNTRL_15_0f_GET(r) (((r).p1588_nse_sc_3[0]) & 0xffff)
#define PLP_P1588_NSE_SC_3r_NSE_REG_SC_SYNCOUT_CNTRL_15_0f_SET(r,f) (r).p1588_nse_sc_3[0]=(((r).p1588_nse_sc_3[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_SC_3.
 *
 */
#define PLP_READ_P1588_NSE_SC_3r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_SC_3r,(_r._p1588_nse_sc_3))
#define PLP_WRITE_P1588_NSE_SC_3r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_SC_3r,(_r._p1588_nse_sc_3))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_SC_3r PLP_P1588_NSE_SC_3r
#define P1588_NSE_SC_3r_SIZE PLP_P1588_NSE_SC_3r_SIZE
typedef PLP_P1588_NSE_SC_3r_t P1588_NSE_SC_3r_t;
#define P1588_NSE_SC_3r_CLR PLP_P1588_NSE_SC_3r_CLR
#define P1588_NSE_SC_3r_SET PLP_P1588_NSE_SC_3r_SET
#define P1588_NSE_SC_3r_GET PLP_P1588_NSE_SC_3r_GET
#define P1588_NSE_SC_3r_NSE_REG_SC_SYNCOUT_CNTRL_15_0f_GET PLP_P1588_NSE_SC_3r_NSE_REG_SC_SYNCOUT_CNTRL_15_0f_GET
#define P1588_NSE_SC_3r_NSE_REG_SC_SYNCOUT_CNTRL_15_0f_SET PLP_P1588_NSE_SC_3r_NSE_REG_SC_SYNCOUT_CNTRL_15_0f_SET
#define READ_P1588_NSE_SC_3r PLP_READ_P1588_NSE_SC_3r
#define WRITE_P1588_NSE_SC_3r PLP_WRITE_P1588_NSE_SC_3r
#define MODIFY_P1588_NSE_SC_3r PLP_MODIFY_P1588_NSE_SC_3r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_SC_3r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_SC_4
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7036
 * DESC:     NSE SC 4 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_TS_DIVIDER Divider for sync-in. If it is set to 4, Timestamp willgenerate one pulse to latch local time into ts_sync_time_regfor every 4 sync-in pulse
 *
 ******************************************************************************/
#define PLP_P1588_NSE_SC_4r    0x00000036

#define PLP_P1588_NSE_SC_4r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_SC_4.
 *
 */
typedef union PLP_P1588_NSE_SC_4r_s {
	uint32_t v[1];
	uint32_t p1588_nse_sc_4[1];
	uint32_t _p1588_nse_sc_4;
} PLP_P1588_NSE_SC_4r_t;

#define PLP_P1588_NSE_SC_4r_CLR(r) (r).p1588_nse_sc_4[0] = 0
#define PLP_P1588_NSE_SC_4r_SET(r,d) (r).p1588_nse_sc_4[0] = d
#define PLP_P1588_NSE_SC_4r_GET(r) (r).p1588_nse_sc_4[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_SC_4r_NSE_REG_TS_DIVIDERf_GET(r) (((r).p1588_nse_sc_4[0]) & 0xfff)
#define PLP_P1588_NSE_SC_4r_NSE_REG_TS_DIVIDERf_SET(r,f) (r).p1588_nse_sc_4[0]=(((r).p1588_nse_sc_4[0] & ~((uint32_t)0xfff)) | (((uint32_t)f) & 0xfff)) | (0xfff << 16)

/*
 * These macros can be used to access P1588_NSE_SC_4.
 *
 */
#define PLP_READ_P1588_NSE_SC_4r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_SC_4r,(_r._p1588_nse_sc_4))
#define PLP_WRITE_P1588_NSE_SC_4r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_SC_4r,(_r._p1588_nse_sc_4))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_SC_4r PLP_P1588_NSE_SC_4r
#define P1588_NSE_SC_4r_SIZE PLP_P1588_NSE_SC_4r_SIZE
typedef PLP_P1588_NSE_SC_4r_t P1588_NSE_SC_4r_t;
#define P1588_NSE_SC_4r_CLR PLP_P1588_NSE_SC_4r_CLR
#define P1588_NSE_SC_4r_SET PLP_P1588_NSE_SC_4r_SET
#define P1588_NSE_SC_4r_GET PLP_P1588_NSE_SC_4r_GET
#define P1588_NSE_SC_4r_NSE_REG_TS_DIVIDERf_GET PLP_P1588_NSE_SC_4r_NSE_REG_TS_DIVIDERf_GET
#define P1588_NSE_SC_4r_NSE_REG_TS_DIVIDERf_SET PLP_P1588_NSE_SC_4r_NSE_REG_TS_DIVIDERf_SET
#define READ_P1588_NSE_SC_4r PLP_READ_P1588_NSE_SC_4r
#define WRITE_P1588_NSE_SC_4r PLP_WRITE_P1588_NSE_SC_4r
#define MODIFY_P1588_NSE_SC_4r PLP_MODIFY_P1588_NSE_SC_4r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_SC_4r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_SC_5
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7037
 * DESC:     NSE SC 5 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_SC_TSCOMP_47_32 When local timer equal to synout_ts_reg, a one time pulse willbe generated on syncout. Only [47:4] are used for comparisonThis carries [47:32] for the comparison
 *
 ******************************************************************************/
#define PLP_P1588_NSE_SC_5r    0x00000037

#define PLP_P1588_NSE_SC_5r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_SC_5.
 *
 */
typedef union PLP_P1588_NSE_SC_5r_s {
	uint32_t v[1];
	uint32_t p1588_nse_sc_5[1];
	uint32_t _p1588_nse_sc_5;
} PLP_P1588_NSE_SC_5r_t;

#define PLP_P1588_NSE_SC_5r_CLR(r) (r).p1588_nse_sc_5[0] = 0
#define PLP_P1588_NSE_SC_5r_SET(r,d) (r).p1588_nse_sc_5[0] = d
#define PLP_P1588_NSE_SC_5r_GET(r) (r).p1588_nse_sc_5[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_SC_5r_NSE_REG_SC_TSCOMP_47_32f_GET(r) (((r).p1588_nse_sc_5[0]) & 0xffff)
#define PLP_P1588_NSE_SC_5r_NSE_REG_SC_TSCOMP_47_32f_SET(r,f) (r).p1588_nse_sc_5[0]=(((r).p1588_nse_sc_5[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_SC_5.
 *
 */
#define PLP_READ_P1588_NSE_SC_5r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_SC_5r,(_r._p1588_nse_sc_5))
#define PLP_WRITE_P1588_NSE_SC_5r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_SC_5r,(_r._p1588_nse_sc_5))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_SC_5r PLP_P1588_NSE_SC_5r
#define P1588_NSE_SC_5r_SIZE PLP_P1588_NSE_SC_5r_SIZE
typedef PLP_P1588_NSE_SC_5r_t P1588_NSE_SC_5r_t;
#define P1588_NSE_SC_5r_CLR PLP_P1588_NSE_SC_5r_CLR
#define P1588_NSE_SC_5r_SET PLP_P1588_NSE_SC_5r_SET
#define P1588_NSE_SC_5r_GET PLP_P1588_NSE_SC_5r_GET
#define P1588_NSE_SC_5r_NSE_REG_SC_TSCOMP_47_32f_GET PLP_P1588_NSE_SC_5r_NSE_REG_SC_TSCOMP_47_32f_GET
#define P1588_NSE_SC_5r_NSE_REG_SC_TSCOMP_47_32f_SET PLP_P1588_NSE_SC_5r_NSE_REG_SC_TSCOMP_47_32f_SET
#define READ_P1588_NSE_SC_5r PLP_READ_P1588_NSE_SC_5r
#define WRITE_P1588_NSE_SC_5r PLP_WRITE_P1588_NSE_SC_5r
#define MODIFY_P1588_NSE_SC_5r PLP_MODIFY_P1588_NSE_SC_5r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_SC_5r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_SC_6
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7038
 * DESC:     NSE SC 6 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_SC_TSCOMP_31_16 When local timer equal to synout_ts_reg, a one time pulse willbe generated on syncout. Only [47:4] are used for comparisonThis carries [31:16] for the comparison
 *
 ******************************************************************************/
#define PLP_P1588_NSE_SC_6r    0x00000038

#define PLP_P1588_NSE_SC_6r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_SC_6.
 *
 */
typedef union PLP_P1588_NSE_SC_6r_s {
	uint32_t v[1];
	uint32_t p1588_nse_sc_6[1];
	uint32_t _p1588_nse_sc_6;
} PLP_P1588_NSE_SC_6r_t;

#define PLP_P1588_NSE_SC_6r_CLR(r) (r).p1588_nse_sc_6[0] = 0
#define PLP_P1588_NSE_SC_6r_SET(r,d) (r).p1588_nse_sc_6[0] = d
#define PLP_P1588_NSE_SC_6r_GET(r) (r).p1588_nse_sc_6[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_SC_6r_NSE_REG_SC_TSCOMP_31_16f_GET(r) (((r).p1588_nse_sc_6[0]) & 0xffff)
#define PLP_P1588_NSE_SC_6r_NSE_REG_SC_TSCOMP_31_16f_SET(r,f) (r).p1588_nse_sc_6[0]=(((r).p1588_nse_sc_6[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_SC_6.
 *
 */
#define PLP_READ_P1588_NSE_SC_6r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_SC_6r,(_r._p1588_nse_sc_6))
#define PLP_WRITE_P1588_NSE_SC_6r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_SC_6r,(_r._p1588_nse_sc_6))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_SC_6r PLP_P1588_NSE_SC_6r
#define P1588_NSE_SC_6r_SIZE PLP_P1588_NSE_SC_6r_SIZE
typedef PLP_P1588_NSE_SC_6r_t P1588_NSE_SC_6r_t;
#define P1588_NSE_SC_6r_CLR PLP_P1588_NSE_SC_6r_CLR
#define P1588_NSE_SC_6r_SET PLP_P1588_NSE_SC_6r_SET
#define P1588_NSE_SC_6r_GET PLP_P1588_NSE_SC_6r_GET
#define P1588_NSE_SC_6r_NSE_REG_SC_TSCOMP_31_16f_GET PLP_P1588_NSE_SC_6r_NSE_REG_SC_TSCOMP_31_16f_GET
#define P1588_NSE_SC_6r_NSE_REG_SC_TSCOMP_31_16f_SET PLP_P1588_NSE_SC_6r_NSE_REG_SC_TSCOMP_31_16f_SET
#define READ_P1588_NSE_SC_6r PLP_READ_P1588_NSE_SC_6r
#define WRITE_P1588_NSE_SC_6r PLP_WRITE_P1588_NSE_SC_6r
#define MODIFY_P1588_NSE_SC_6r PLP_MODIFY_P1588_NSE_SC_6r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_SC_6r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_SC_7
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7039
 * DESC:     NSE SC 7 Register
 * RESETVAL: 0x10 (16)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_SC_TSCOMP_3_0 These are reserved for future use
 *     NSE_REG_SC_TSCOMP_15_4 When local timer equal to synout_ts_reg, a one time pulse willbe generated on syncout. Only [47:4] are used for comparisonThis carries [15:4] for the comparison
 *
 ******************************************************************************/
#define PLP_P1588_NSE_SC_7r    0x00000039

#define PLP_P1588_NSE_SC_7r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_SC_7.
 *
 */
typedef union PLP_P1588_NSE_SC_7r_s {
	uint32_t v[1];
	uint32_t p1588_nse_sc_7[1];
	uint32_t _p1588_nse_sc_7;
} PLP_P1588_NSE_SC_7r_t;

#define PLP_P1588_NSE_SC_7r_CLR(r) (r).p1588_nse_sc_7[0] = 0
#define PLP_P1588_NSE_SC_7r_SET(r,d) (r).p1588_nse_sc_7[0] = d
#define PLP_P1588_NSE_SC_7r_GET(r) (r).p1588_nse_sc_7[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_SC_7r_NSE_REG_SC_TSCOMP_15_4f_GET(r) ((((r).p1588_nse_sc_7[0]) >> 4) & 0xfff)
#define PLP_P1588_NSE_SC_7r_NSE_REG_SC_TSCOMP_15_4f_SET(r,f) (r).p1588_nse_sc_7[0]=(((r).p1588_nse_sc_7[0] & ~((uint32_t)0xfff << 4)) | ((((uint32_t)f) & 0xfff) << 4)) | (4095 << (16 + 4))
#define PLP_P1588_NSE_SC_7r_NSE_REG_SC_TSCOMP_3_0f_GET(r) (((r).p1588_nse_sc_7[0]) & 0xf)
#define PLP_P1588_NSE_SC_7r_NSE_REG_SC_TSCOMP_3_0f_SET(r,f) (r).p1588_nse_sc_7[0]=(((r).p1588_nse_sc_7[0] & ~((uint32_t)0xf)) | (((uint32_t)f) & 0xf)) | (0xf << 16)

/*
 * These macros can be used to access P1588_NSE_SC_7.
 *
 */
#define PLP_READ_P1588_NSE_SC_7r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_SC_7r,(_r._p1588_nse_sc_7))
#define PLP_WRITE_P1588_NSE_SC_7r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_SC_7r,(_r._p1588_nse_sc_7))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_SC_7r PLP_P1588_NSE_SC_7r
#define P1588_NSE_SC_7r_SIZE PLP_P1588_NSE_SC_7r_SIZE
typedef PLP_P1588_NSE_SC_7r_t P1588_NSE_SC_7r_t;
#define P1588_NSE_SC_7r_CLR PLP_P1588_NSE_SC_7r_CLR
#define P1588_NSE_SC_7r_SET PLP_P1588_NSE_SC_7r_SET
#define P1588_NSE_SC_7r_GET PLP_P1588_NSE_SC_7r_GET
#define P1588_NSE_SC_7r_NSE_REG_SC_TSCOMP_15_4f_GET PLP_P1588_NSE_SC_7r_NSE_REG_SC_TSCOMP_15_4f_GET
#define P1588_NSE_SC_7r_NSE_REG_SC_TSCOMP_15_4f_SET PLP_P1588_NSE_SC_7r_NSE_REG_SC_TSCOMP_15_4f_SET
#define P1588_NSE_SC_7r_NSE_REG_SC_TSCOMP_3_0f_GET PLP_P1588_NSE_SC_7r_NSE_REG_SC_TSCOMP_3_0f_GET
#define P1588_NSE_SC_7r_NSE_REG_SC_TSCOMP_3_0f_SET PLP_P1588_NSE_SC_7r_NSE_REG_SC_TSCOMP_3_0f_SET
#define READ_P1588_NSE_SC_7r PLP_READ_P1588_NSE_SC_7r
#define WRITE_P1588_NSE_SC_7r PLP_WRITE_P1588_NSE_SC_7r
#define MODIFY_P1588_NSE_SC_7r PLP_MODIFY_P1588_NSE_SC_7r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_SC_7r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_SC_8
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x703a
 * DESC:     NSE SC 8 Register
 * RESETVAL: 0x4004 (16388)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_SC_SYNMODE_CNTRL_1_0 synout_mode[1:0].2'b01 - Generate a one time pulse output on a match withsynout_ts_reg2'b10 - Generate a pulse train. Detail pulse train specificationis described in NSE NCO register2'b11 - Generate a pulse train and insert a one time frame syncevent, under syncout mode1 condition.
 *     NSE_REG_SC_SYNMODE_CNTRL_5_2 synin_mode[3:0]. This is valid when gmode is 2'b11.[2] - Use long pulse on syncin0 for frame sync[3] - Use syncin1 for frame sync[4] - Use internal syncout for frame sync[5] - CPU trigger immediate for frame syncnote to programmer:  need to reprogram the defualt value again!!!
 *     NSE_REG_SC_SYNMODE_CNTRL_6 Enables the current NSE to drive its associated synout port0: The current NSE does not drive its associate synout port1: The current drives its associate synout port
 *     NSE_REG_SC_SYNMODE_CNTRL_7 reset_syncin _state: 1 - Reset sync in FSM back to Idle state
 *     NSE_REG_SC_SYNMODE_CNTRL_8 reset_syncout_state: 1 - Reset sync out FSM back to Idle state
 *     NSE_REG_SC_SYNMODE_CNTRL_9 reset_lock_state: 1 - Reset lock FSM back to Idle state
 *     NSE_REG_SC_SYNMODE_CNTRL_10 This bit is reserved for future use
 *     NSE_REG_SC_SYNMODE_CNTRL_11 m34_local_sync_dis: 1 - Disable sync out and treat aslocal sync in when synin_mode is equal to 3 or 4
 *     NSE_REG_SC_SYNMODE_CNTRL_12 nse_init: 1 - Initial NSE block
 *     NSE_REG_SC_SYNMODE_CNTRL_13 ts_capture: 1 - Enable the timestamp to be captured by ts_capture_timeon the next framesync eventts_capture: 0 - Disable the timestamp to be captured by ts_capture_timeon the next framesync event
 *     NSE_REG_SC_SYNMODE_CNTRL_15_14 gmode[1:0] = 'b01 - When syn in0 is connected to resetsignal on the board. System is set to freerunning state. Assume all PHYs in the systemshare the same RX clock, and DPLL disablegmode[1:0] = 'b10 - When syn in0 is connected to clockno CPU involved. Assume all PHY do not sharesame RX lcok. DPLL is used to lock to syncin0signalgmode[1:0] = 'b11 - When system has CPU involved. Used togetherwith SYNIN_MODE. See more detail for synin_modenote to programmer:  need to reprogram the defualt value again!!!
 *
 ******************************************************************************/
#define PLP_P1588_NSE_SC_8r    0x0000003a

#define PLP_P1588_NSE_SC_8r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_SC_8.
 *
 */
typedef union PLP_P1588_NSE_SC_8r_s {
	uint32_t v[1];
	uint32_t p1588_nse_sc_8[1];
	uint32_t _p1588_nse_sc_8;
} PLP_P1588_NSE_SC_8r_t;

#define PLP_P1588_NSE_SC_8r_CLR(r) (r).p1588_nse_sc_8[0] = 0
#define PLP_P1588_NSE_SC_8r_SET(r,d) (r).p1588_nse_sc_8[0] = d
#define PLP_P1588_NSE_SC_8r_GET(r) (r).p1588_nse_sc_8[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_15_14f_GET(r) ((((r).p1588_nse_sc_8[0]) >> 14) & 0x3)
#define PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_15_14f_SET(r,f) (r).p1588_nse_sc_8[0]=(((r).p1588_nse_sc_8[0] & ~((uint32_t)0x3 << 14)) | ((((uint32_t)f) & 0x3) << 14)) | (3 << (16 + 14))
#define PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_13f_GET(r) ((((r).p1588_nse_sc_8[0]) >> 13) & 0x1)
#define PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_13f_SET(r,f) (r).p1588_nse_sc_8[0]=(((r).p1588_nse_sc_8[0] & ~((uint32_t)0x1 << 13)) | ((((uint32_t)f) & 0x1) << 13)) | (1 << (16 + 13))
#define PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_12f_GET(r) ((((r).p1588_nse_sc_8[0]) >> 12) & 0x1)
#define PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_12f_SET(r,f) (r).p1588_nse_sc_8[0]=(((r).p1588_nse_sc_8[0] & ~((uint32_t)0x1 << 12)) | ((((uint32_t)f) & 0x1) << 12)) | (1 << (16 + 12))
#define PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_11f_GET(r) ((((r).p1588_nse_sc_8[0]) >> 11) & 0x1)
#define PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_11f_SET(r,f) (r).p1588_nse_sc_8[0]=(((r).p1588_nse_sc_8[0] & ~((uint32_t)0x1 << 11)) | ((((uint32_t)f) & 0x1) << 11)) | (1 << (16 + 11))
#define PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_10f_GET(r) ((((r).p1588_nse_sc_8[0]) >> 10) & 0x1)
#define PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_10f_SET(r,f) (r).p1588_nse_sc_8[0]=(((r).p1588_nse_sc_8[0] & ~((uint32_t)0x1 << 10)) | ((((uint32_t)f) & 0x1) << 10)) | (1 << (16 + 10))
#define PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_9f_GET(r) ((((r).p1588_nse_sc_8[0]) >> 9) & 0x1)
#define PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_9f_SET(r,f) (r).p1588_nse_sc_8[0]=(((r).p1588_nse_sc_8[0] & ~((uint32_t)0x1 << 9)) | ((((uint32_t)f) & 0x1) << 9)) | (1 << (16 + 9))
#define PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_8f_GET(r) ((((r).p1588_nse_sc_8[0]) >> 8) & 0x1)
#define PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_8f_SET(r,f) (r).p1588_nse_sc_8[0]=(((r).p1588_nse_sc_8[0] & ~((uint32_t)0x1 << 8)) | ((((uint32_t)f) & 0x1) << 8)) | (1 << (16 + 8))
#define PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_7f_GET(r) ((((r).p1588_nse_sc_8[0]) >> 7) & 0x1)
#define PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_7f_SET(r,f) (r).p1588_nse_sc_8[0]=(((r).p1588_nse_sc_8[0] & ~((uint32_t)0x1 << 7)) | ((((uint32_t)f) & 0x1) << 7)) | (1 << (16 + 7))
#define PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_6f_GET(r) ((((r).p1588_nse_sc_8[0]) >> 6) & 0x1)
#define PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_6f_SET(r,f) (r).p1588_nse_sc_8[0]=(((r).p1588_nse_sc_8[0] & ~((uint32_t)0x1 << 6)) | ((((uint32_t)f) & 0x1) << 6)) | (1 << (16 + 6))
#define PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_5_2f_GET(r) ((((r).p1588_nse_sc_8[0]) >> 2) & 0xf)
#define PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_5_2f_SET(r,f) (r).p1588_nse_sc_8[0]=(((r).p1588_nse_sc_8[0] & ~((uint32_t)0xf << 2)) | ((((uint32_t)f) & 0xf) << 2)) | (15 << (16 + 2))
#define PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_1_0f_GET(r) (((r).p1588_nse_sc_8[0]) & 0x3)
#define PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_1_0f_SET(r,f) (r).p1588_nse_sc_8[0]=(((r).p1588_nse_sc_8[0] & ~((uint32_t)0x3)) | (((uint32_t)f) & 0x3)) | (0x3 << 16)

/*
 * These macros can be used to access P1588_NSE_SC_8.
 *
 */
#define PLP_READ_P1588_NSE_SC_8r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_SC_8r,(_r._p1588_nse_sc_8))
#define PLP_WRITE_P1588_NSE_SC_8r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_SC_8r,(_r._p1588_nse_sc_8))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_SC_8r PLP_P1588_NSE_SC_8r
#define P1588_NSE_SC_8r_SIZE PLP_P1588_NSE_SC_8r_SIZE
typedef PLP_P1588_NSE_SC_8r_t P1588_NSE_SC_8r_t;
#define P1588_NSE_SC_8r_CLR PLP_P1588_NSE_SC_8r_CLR
#define P1588_NSE_SC_8r_SET PLP_P1588_NSE_SC_8r_SET
#define P1588_NSE_SC_8r_GET PLP_P1588_NSE_SC_8r_GET
#define P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_15_14f_GET PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_15_14f_GET
#define P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_15_14f_SET PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_15_14f_SET
#define P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_13f_GET PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_13f_GET
#define P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_13f_SET PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_13f_SET
#define P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_12f_GET PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_12f_GET
#define P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_12f_SET PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_12f_SET
#define P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_11f_GET PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_11f_GET
#define P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_11f_SET PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_11f_SET
#define P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_10f_GET PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_10f_GET
#define P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_10f_SET PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_10f_SET
#define P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_9f_GET PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_9f_GET
#define P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_9f_SET PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_9f_SET
#define P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_8f_GET PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_8f_GET
#define P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_8f_SET PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_8f_SET
#define P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_7f_GET PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_7f_GET
#define P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_7f_SET PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_7f_SET
#define P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_6f_GET PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_6f_GET
#define P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_6f_SET PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_6f_SET
#define P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_5_2f_GET PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_5_2f_GET
#define P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_5_2f_SET PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_5_2f_SET
#define P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_1_0f_GET PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_1_0f_GET
#define P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_1_0f_SET PLP_P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_1_0f_SET
#define READ_P1588_NSE_SC_8r PLP_READ_P1588_NSE_SC_8r
#define WRITE_P1588_NSE_SC_8r PLP_WRITE_P1588_NSE_SC_8r
#define MODIFY_P1588_NSE_SC_8r PLP_MODIFY_P1588_NSE_SC_8r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_SC_8r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_SC_9
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x703b
 * DESC:     NSE SC 9 Register
 * RESETVAL: 0x8 (8)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_SC_SYNCIN_CNTRL_31_16 Event_offset[15:0]: Offset timer for frame sync to kick off
 *
 ******************************************************************************/
#define PLP_P1588_NSE_SC_9r    0x0000003b

#define PLP_P1588_NSE_SC_9r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_SC_9.
 *
 */
typedef union PLP_P1588_NSE_SC_9r_s {
	uint32_t v[1];
	uint32_t p1588_nse_sc_9[1];
	uint32_t _p1588_nse_sc_9;
} PLP_P1588_NSE_SC_9r_t;

#define PLP_P1588_NSE_SC_9r_CLR(r) (r).p1588_nse_sc_9[0] = 0
#define PLP_P1588_NSE_SC_9r_SET(r,d) (r).p1588_nse_sc_9[0] = d
#define PLP_P1588_NSE_SC_9r_GET(r) (r).p1588_nse_sc_9[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_SC_9r_NSE_REG_SC_SYNCIN_CNTRL_31_16f_GET(r) (((r).p1588_nse_sc_9[0]) & 0xffff)
#define PLP_P1588_NSE_SC_9r_NSE_REG_SC_SYNCIN_CNTRL_31_16f_SET(r,f) (r).p1588_nse_sc_9[0]=(((r).p1588_nse_sc_9[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_SC_9.
 *
 */
#define PLP_READ_P1588_NSE_SC_9r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_SC_9r,(_r._p1588_nse_sc_9))
#define PLP_WRITE_P1588_NSE_SC_9r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_SC_9r,(_r._p1588_nse_sc_9))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_SC_9r PLP_P1588_NSE_SC_9r
#define P1588_NSE_SC_9r_SIZE PLP_P1588_NSE_SC_9r_SIZE
typedef PLP_P1588_NSE_SC_9r_t P1588_NSE_SC_9r_t;
#define P1588_NSE_SC_9r_CLR PLP_P1588_NSE_SC_9r_CLR
#define P1588_NSE_SC_9r_SET PLP_P1588_NSE_SC_9r_SET
#define P1588_NSE_SC_9r_GET PLP_P1588_NSE_SC_9r_GET
#define P1588_NSE_SC_9r_NSE_REG_SC_SYNCIN_CNTRL_31_16f_GET PLP_P1588_NSE_SC_9r_NSE_REG_SC_SYNCIN_CNTRL_31_16f_GET
#define P1588_NSE_SC_9r_NSE_REG_SC_SYNCIN_CNTRL_31_16f_SET PLP_P1588_NSE_SC_9r_NSE_REG_SC_SYNCIN_CNTRL_31_16f_SET
#define READ_P1588_NSE_SC_9r PLP_READ_P1588_NSE_SC_9r
#define WRITE_P1588_NSE_SC_9r PLP_WRITE_P1588_NSE_SC_9r
#define MODIFY_P1588_NSE_SC_9r PLP_MODIFY_P1588_NSE_SC_9r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_SC_9r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_SC_10
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x703c
 * DESC:     NSE SC 10 Register
 * RESETVAL: 0x8 (8)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_SC_SYNCIN_CNTRL_15_0 Length_threshold[15:0]: Length to specify frame sync conditionOnce sync width is longer than Length_threshold, NCO willtreat it as frame sync
 *
 ******************************************************************************/
#define PLP_P1588_NSE_SC_10r    0x0000003c

#define PLP_P1588_NSE_SC_10r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_SC_10.
 *
 */
typedef union PLP_P1588_NSE_SC_10r_s {
	uint32_t v[1];
	uint32_t p1588_nse_sc_10[1];
	uint32_t _p1588_nse_sc_10;
} PLP_P1588_NSE_SC_10r_t;

#define PLP_P1588_NSE_SC_10r_CLR(r) (r).p1588_nse_sc_10[0] = 0
#define PLP_P1588_NSE_SC_10r_SET(r,d) (r).p1588_nse_sc_10[0] = d
#define PLP_P1588_NSE_SC_10r_GET(r) (r).p1588_nse_sc_10[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_SC_10r_NSE_REG_SC_SYNCIN_CNTRL_15_0f_GET(r) (((r).p1588_nse_sc_10[0]) & 0xffff)
#define PLP_P1588_NSE_SC_10r_NSE_REG_SC_SYNCIN_CNTRL_15_0f_SET(r,f) (r).p1588_nse_sc_10[0]=(((r).p1588_nse_sc_10[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_SC_10.
 *
 */
#define PLP_READ_P1588_NSE_SC_10r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_SC_10r,(_r._p1588_nse_sc_10))
#define PLP_WRITE_P1588_NSE_SC_10r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_SC_10r,(_r._p1588_nse_sc_10))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_SC_10r PLP_P1588_NSE_SC_10r
#define P1588_NSE_SC_10r_SIZE PLP_P1588_NSE_SC_10r_SIZE
typedef PLP_P1588_NSE_SC_10r_t P1588_NSE_SC_10r_t;
#define P1588_NSE_SC_10r_CLR PLP_P1588_NSE_SC_10r_CLR
#define P1588_NSE_SC_10r_SET PLP_P1588_NSE_SC_10r_SET
#define P1588_NSE_SC_10r_GET PLP_P1588_NSE_SC_10r_GET
#define P1588_NSE_SC_10r_NSE_REG_SC_SYNCIN_CNTRL_15_0f_GET PLP_P1588_NSE_SC_10r_NSE_REG_SC_SYNCIN_CNTRL_15_0f_GET
#define P1588_NSE_SC_10r_NSE_REG_SC_SYNCIN_CNTRL_15_0f_SET PLP_P1588_NSE_SC_10r_NSE_REG_SC_SYNCIN_CNTRL_15_0f_SET
#define READ_P1588_NSE_SC_10r PLP_READ_P1588_NSE_SC_10r
#define WRITE_P1588_NSE_SC_10r PLP_WRITE_P1588_NSE_SC_10r
#define MODIFY_P1588_NSE_SC_10r PLP_MODIFY_P1588_NSE_SC_10r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_SC_10r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TS_HB_SEL_1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x703d
 * DESC:     TS HB SEL 1 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     EXPANSION_REG_F3_1_0 not used
 *     EXPANSION_REG_F3_1_1 not used
 *     EXPANSION_REG_F3_1_2 1= start read SOPmem entry
 *     EXPANSION_REG_F3_1_3 1= clear current SOPmem entry
 *     EXPANSION_REG_F3_1_4 1 - 48b TS will be treated as 47b TS with msb of 00 - Normal 48b TS value
 *     EXPANSION_REG_F3_1_6_5 [6] - leap59, increment 47b counter and not the 1b in the 80b TS block[5] - leap61, no increment to 47b counter and 1b in the 80b TS block
 *     EXPANSION_REG_F3_1_15_7 bit 15: 1= select TS80 to capture to 1.c6f4-1.c6f8bit 15: 0= select TS48 to capture to 1.c6f4-1.c6f6
 *
 ******************************************************************************/
#define PLP_P1588_TS_HB_SEL_1r    0x0000003d

#define PLP_P1588_TS_HB_SEL_1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_TS_HB_SEL_1.
 *
 */
typedef union PLP_P1588_TS_HB_SEL_1r_s {
	uint32_t v[1];
	uint32_t p1588_ts_hb_sel_1[1];
	uint32_t _p1588_ts_hb_sel_1;
} PLP_P1588_TS_HB_SEL_1r_t;

#define PLP_P1588_TS_HB_SEL_1r_CLR(r) (r).p1588_ts_hb_sel_1[0] = 0
#define PLP_P1588_TS_HB_SEL_1r_SET(r,d) (r).p1588_ts_hb_sel_1[0] = d
#define PLP_P1588_TS_HB_SEL_1r_GET(r) (r).p1588_ts_hb_sel_1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_15_7f_GET(r) ((((r).p1588_ts_hb_sel_1[0]) >> 7) & 0x1ff)
#define PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_15_7f_SET(r,f) (r).p1588_ts_hb_sel_1[0]=(((r).p1588_ts_hb_sel_1[0] & ~((uint32_t)0x1ff << 7)) | ((((uint32_t)f) & 0x1ff) << 7)) | (511 << (16 + 7))
#define PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_6_5f_GET(r) ((((r).p1588_ts_hb_sel_1[0]) >> 5) & 0x3)
#define PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_6_5f_SET(r,f) (r).p1588_ts_hb_sel_1[0]=(((r).p1588_ts_hb_sel_1[0] & ~((uint32_t)0x3 << 5)) | ((((uint32_t)f) & 0x3) << 5)) | (3 << (16 + 5))
#define PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_4f_GET(r) ((((r).p1588_ts_hb_sel_1[0]) >> 4) & 0x1)
#define PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_4f_SET(r,f) (r).p1588_ts_hb_sel_1[0]=(((r).p1588_ts_hb_sel_1[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4)) | (1 << (16 + 4))
#define PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_3f_GET(r) ((((r).p1588_ts_hb_sel_1[0]) >> 3) & 0x1)
#define PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_3f_SET(r,f) (r).p1588_ts_hb_sel_1[0]=(((r).p1588_ts_hb_sel_1[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3)) | (1 << (16 + 3))
#define PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_2f_GET(r) ((((r).p1588_ts_hb_sel_1[0]) >> 2) & 0x1)
#define PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_2f_SET(r,f) (r).p1588_ts_hb_sel_1[0]=(((r).p1588_ts_hb_sel_1[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2)) | (1 << (16 + 2))
#define PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_1f_GET(r) ((((r).p1588_ts_hb_sel_1[0]) >> 1) & 0x1)
#define PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_1f_SET(r,f) (r).p1588_ts_hb_sel_1[0]=(((r).p1588_ts_hb_sel_1[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))
#define PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_0f_GET(r) (((r).p1588_ts_hb_sel_1[0]) & 0x1)
#define PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_0f_SET(r,f) (r).p1588_ts_hb_sel_1[0]=(((r).p1588_ts_hb_sel_1[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1)) | (0x1 << 16)

/*
 * These macros can be used to access P1588_TS_HB_SEL_1.
 *
 */
#define PLP_READ_P1588_TS_HB_SEL_1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TS_HB_SEL_1r,(_r._p1588_ts_hb_sel_1))
#define PLP_WRITE_P1588_TS_HB_SEL_1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TS_HB_SEL_1r,(_r._p1588_ts_hb_sel_1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TS_HB_SEL_1r PLP_P1588_TS_HB_SEL_1r
#define P1588_TS_HB_SEL_1r_SIZE PLP_P1588_TS_HB_SEL_1r_SIZE
typedef PLP_P1588_TS_HB_SEL_1r_t P1588_TS_HB_SEL_1r_t;
#define P1588_TS_HB_SEL_1r_CLR PLP_P1588_TS_HB_SEL_1r_CLR
#define P1588_TS_HB_SEL_1r_SET PLP_P1588_TS_HB_SEL_1r_SET
#define P1588_TS_HB_SEL_1r_GET PLP_P1588_TS_HB_SEL_1r_GET
#define P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_15_7f_GET PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_15_7f_GET
#define P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_15_7f_SET PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_15_7f_SET
#define P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_6_5f_GET PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_6_5f_GET
#define P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_6_5f_SET PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_6_5f_SET
#define P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_4f_GET PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_4f_GET
#define P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_4f_SET PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_4f_SET
#define P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_3f_GET PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_3f_GET
#define P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_3f_SET PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_3f_SET
#define P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_2f_GET PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_2f_GET
#define P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_2f_SET PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_2f_SET
#define P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_1f_GET PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_1f_GET
#define P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_1f_SET PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_1f_SET
#define P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_0f_GET PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_0f_GET
#define P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_0f_SET PLP_P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_0f_SET
#define READ_P1588_TS_HB_SEL_1r PLP_READ_P1588_TS_HB_SEL_1r
#define WRITE_P1588_TS_HB_SEL_1r PLP_WRITE_P1588_TS_HB_SEL_1r
#define MODIFY_P1588_TS_HB_SEL_1r PLP_MODIFY_P1588_TS_HB_SEL_1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TS_HB_SEL_1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TS_HB_SEL_2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x703e
 * DESC:     TS HB SEL 2 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOP_TS_47_32     Timestamp[47:32] stored in the FIFO
 *
 ******************************************************************************/
#define PLP_P1588_TS_HB_SEL_2r    0x0000003e

#define PLP_P1588_TS_HB_SEL_2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_TS_HB_SEL_2.
 *
 */
typedef union PLP_P1588_TS_HB_SEL_2r_s {
	uint32_t v[1];
	uint32_t p1588_ts_hb_sel_2[1];
	uint32_t _p1588_ts_hb_sel_2;
} PLP_P1588_TS_HB_SEL_2r_t;

#define PLP_P1588_TS_HB_SEL_2r_CLR(r) (r).p1588_ts_hb_sel_2[0] = 0
#define PLP_P1588_TS_HB_SEL_2r_SET(r,d) (r).p1588_ts_hb_sel_2[0] = d
#define PLP_P1588_TS_HB_SEL_2r_GET(r) (r).p1588_ts_hb_sel_2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TS_HB_SEL_2r_SOP_TS_47_32f_GET(r) (((r).p1588_ts_hb_sel_2[0]) & 0xffff)
#define PLP_P1588_TS_HB_SEL_2r_SOP_TS_47_32f_SET(r,f) (r).p1588_ts_hb_sel_2[0]=(((r).p1588_ts_hb_sel_2[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TS_HB_SEL_2.
 *
 */
#define PLP_READ_P1588_TS_HB_SEL_2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TS_HB_SEL_2r,(_r._p1588_ts_hb_sel_2))
#define PLP_WRITE_P1588_TS_HB_SEL_2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TS_HB_SEL_2r,(_r._p1588_ts_hb_sel_2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TS_HB_SEL_2r PLP_P1588_TS_HB_SEL_2r
#define P1588_TS_HB_SEL_2r_SIZE PLP_P1588_TS_HB_SEL_2r_SIZE
typedef PLP_P1588_TS_HB_SEL_2r_t P1588_TS_HB_SEL_2r_t;
#define P1588_TS_HB_SEL_2r_CLR PLP_P1588_TS_HB_SEL_2r_CLR
#define P1588_TS_HB_SEL_2r_SET PLP_P1588_TS_HB_SEL_2r_SET
#define P1588_TS_HB_SEL_2r_GET PLP_P1588_TS_HB_SEL_2r_GET
#define P1588_TS_HB_SEL_2r_SOP_TS_47_32f_GET PLP_P1588_TS_HB_SEL_2r_SOP_TS_47_32f_GET
#define P1588_TS_HB_SEL_2r_SOP_TS_47_32f_SET PLP_P1588_TS_HB_SEL_2r_SOP_TS_47_32f_SET
#define READ_P1588_TS_HB_SEL_2r PLP_READ_P1588_TS_HB_SEL_2r
#define WRITE_P1588_TS_HB_SEL_2r PLP_WRITE_P1588_TS_HB_SEL_2r
#define MODIFY_P1588_TS_HB_SEL_2r PLP_MODIFY_P1588_TS_HB_SEL_2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TS_HB_SEL_2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TS_HB_SEL_3
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x703f
 * DESC:     TS HB SEL 3 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOP_TS_31_16     Timestamp[31:16] stored in the FIFO
 *
 ******************************************************************************/
#define PLP_P1588_TS_HB_SEL_3r    0x0000003f

#define PLP_P1588_TS_HB_SEL_3r_SIZE 4

/*
 * This structure should be used to declare and program P1588_TS_HB_SEL_3.
 *
 */
typedef union PLP_P1588_TS_HB_SEL_3r_s {
	uint32_t v[1];
	uint32_t p1588_ts_hb_sel_3[1];
	uint32_t _p1588_ts_hb_sel_3;
} PLP_P1588_TS_HB_SEL_3r_t;

#define PLP_P1588_TS_HB_SEL_3r_CLR(r) (r).p1588_ts_hb_sel_3[0] = 0
#define PLP_P1588_TS_HB_SEL_3r_SET(r,d) (r).p1588_ts_hb_sel_3[0] = d
#define PLP_P1588_TS_HB_SEL_3r_GET(r) (r).p1588_ts_hb_sel_3[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TS_HB_SEL_3r_SOP_TS_31_16f_GET(r) (((r).p1588_ts_hb_sel_3[0]) & 0xffff)
#define PLP_P1588_TS_HB_SEL_3r_SOP_TS_31_16f_SET(r,f) (r).p1588_ts_hb_sel_3[0]=(((r).p1588_ts_hb_sel_3[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TS_HB_SEL_3.
 *
 */
#define PLP_READ_P1588_TS_HB_SEL_3r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TS_HB_SEL_3r,(_r._p1588_ts_hb_sel_3))
#define PLP_WRITE_P1588_TS_HB_SEL_3r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TS_HB_SEL_3r,(_r._p1588_ts_hb_sel_3))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TS_HB_SEL_3r PLP_P1588_TS_HB_SEL_3r
#define P1588_TS_HB_SEL_3r_SIZE PLP_P1588_TS_HB_SEL_3r_SIZE
typedef PLP_P1588_TS_HB_SEL_3r_t P1588_TS_HB_SEL_3r_t;
#define P1588_TS_HB_SEL_3r_CLR PLP_P1588_TS_HB_SEL_3r_CLR
#define P1588_TS_HB_SEL_3r_SET PLP_P1588_TS_HB_SEL_3r_SET
#define P1588_TS_HB_SEL_3r_GET PLP_P1588_TS_HB_SEL_3r_GET
#define P1588_TS_HB_SEL_3r_SOP_TS_31_16f_GET PLP_P1588_TS_HB_SEL_3r_SOP_TS_31_16f_GET
#define P1588_TS_HB_SEL_3r_SOP_TS_31_16f_SET PLP_P1588_TS_HB_SEL_3r_SOP_TS_31_16f_SET
#define READ_P1588_TS_HB_SEL_3r PLP_READ_P1588_TS_HB_SEL_3r
#define WRITE_P1588_TS_HB_SEL_3r PLP_WRITE_P1588_TS_HB_SEL_3r
#define MODIFY_P1588_TS_HB_SEL_3r PLP_MODIFY_P1588_TS_HB_SEL_3r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TS_HB_SEL_3r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TS_HB_SEL_4
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7040
 * DESC:     TS HB SEL 4 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOP_TS_15_0      Timestamp[15:0] stored in the FIFO
 *
 ******************************************************************************/
#define PLP_P1588_TS_HB_SEL_4r    0x00000040

#define PLP_P1588_TS_HB_SEL_4r_SIZE 4

/*
 * This structure should be used to declare and program P1588_TS_HB_SEL_4.
 *
 */
typedef union PLP_P1588_TS_HB_SEL_4r_s {
	uint32_t v[1];
	uint32_t p1588_ts_hb_sel_4[1];
	uint32_t _p1588_ts_hb_sel_4;
} PLP_P1588_TS_HB_SEL_4r_t;

#define PLP_P1588_TS_HB_SEL_4r_CLR(r) (r).p1588_ts_hb_sel_4[0] = 0
#define PLP_P1588_TS_HB_SEL_4r_SET(r,d) (r).p1588_ts_hb_sel_4[0] = d
#define PLP_P1588_TS_HB_SEL_4r_GET(r) (r).p1588_ts_hb_sel_4[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TS_HB_SEL_4r_SOP_TS_15_0f_GET(r) (((r).p1588_ts_hb_sel_4[0]) & 0xffff)
#define PLP_P1588_TS_HB_SEL_4r_SOP_TS_15_0f_SET(r,f) (r).p1588_ts_hb_sel_4[0]=(((r).p1588_ts_hb_sel_4[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TS_HB_SEL_4.
 *
 */
#define PLP_READ_P1588_TS_HB_SEL_4r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TS_HB_SEL_4r,(_r._p1588_ts_hb_sel_4))
#define PLP_WRITE_P1588_TS_HB_SEL_4r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TS_HB_SEL_4r,(_r._p1588_ts_hb_sel_4))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TS_HB_SEL_4r PLP_P1588_TS_HB_SEL_4r
#define P1588_TS_HB_SEL_4r_SIZE PLP_P1588_TS_HB_SEL_4r_SIZE
typedef PLP_P1588_TS_HB_SEL_4r_t P1588_TS_HB_SEL_4r_t;
#define P1588_TS_HB_SEL_4r_CLR PLP_P1588_TS_HB_SEL_4r_CLR
#define P1588_TS_HB_SEL_4r_SET PLP_P1588_TS_HB_SEL_4r_SET
#define P1588_TS_HB_SEL_4r_GET PLP_P1588_TS_HB_SEL_4r_GET
#define P1588_TS_HB_SEL_4r_SOP_TS_15_0f_GET PLP_P1588_TS_HB_SEL_4r_SOP_TS_15_0f_GET
#define P1588_TS_HB_SEL_4r_SOP_TS_15_0f_SET PLP_P1588_TS_HB_SEL_4r_SOP_TS_15_0f_SET
#define READ_P1588_TS_HB_SEL_4r PLP_READ_P1588_TS_HB_SEL_4r
#define WRITE_P1588_TS_HB_SEL_4r PLP_WRITE_P1588_TS_HB_SEL_4r
#define MODIFY_P1588_TS_HB_SEL_4r PLP_MODIFY_P1588_TS_HB_SEL_4r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TS_HB_SEL_4r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TS_HB_SEL_5
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7041
 * DESC:     TS HB SEL 5 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOP_TS_INFO_55_40 [15:0] - Contains MAC DA info16b of 48b selected by tx_offset[7:6]
 *
 ******************************************************************************/
#define PLP_P1588_TS_HB_SEL_5r    0x00000041

#define PLP_P1588_TS_HB_SEL_5r_SIZE 4

/*
 * This structure should be used to declare and program P1588_TS_HB_SEL_5.
 *
 */
typedef union PLP_P1588_TS_HB_SEL_5r_s {
	uint32_t v[1];
	uint32_t p1588_ts_hb_sel_5[1];
	uint32_t _p1588_ts_hb_sel_5;
} PLP_P1588_TS_HB_SEL_5r_t;

#define PLP_P1588_TS_HB_SEL_5r_CLR(r) (r).p1588_ts_hb_sel_5[0] = 0
#define PLP_P1588_TS_HB_SEL_5r_SET(r,d) (r).p1588_ts_hb_sel_5[0] = d
#define PLP_P1588_TS_HB_SEL_5r_GET(r) (r).p1588_ts_hb_sel_5[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TS_HB_SEL_5r_SOP_TS_INFO_55_40f_GET(r) (((r).p1588_ts_hb_sel_5[0]) & 0xffff)
#define PLP_P1588_TS_HB_SEL_5r_SOP_TS_INFO_55_40f_SET(r,f) (r).p1588_ts_hb_sel_5[0]=(((r).p1588_ts_hb_sel_5[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TS_HB_SEL_5.
 *
 */
#define PLP_READ_P1588_TS_HB_SEL_5r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TS_HB_SEL_5r,(_r._p1588_ts_hb_sel_5))
#define PLP_WRITE_P1588_TS_HB_SEL_5r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TS_HB_SEL_5r,(_r._p1588_ts_hb_sel_5))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TS_HB_SEL_5r PLP_P1588_TS_HB_SEL_5r
#define P1588_TS_HB_SEL_5r_SIZE PLP_P1588_TS_HB_SEL_5r_SIZE
typedef PLP_P1588_TS_HB_SEL_5r_t P1588_TS_HB_SEL_5r_t;
#define P1588_TS_HB_SEL_5r_CLR PLP_P1588_TS_HB_SEL_5r_CLR
#define P1588_TS_HB_SEL_5r_SET PLP_P1588_TS_HB_SEL_5r_SET
#define P1588_TS_HB_SEL_5r_GET PLP_P1588_TS_HB_SEL_5r_GET
#define P1588_TS_HB_SEL_5r_SOP_TS_INFO_55_40f_GET PLP_P1588_TS_HB_SEL_5r_SOP_TS_INFO_55_40f_GET
#define P1588_TS_HB_SEL_5r_SOP_TS_INFO_55_40f_SET PLP_P1588_TS_HB_SEL_5r_SOP_TS_INFO_55_40f_SET
#define READ_P1588_TS_HB_SEL_5r PLP_READ_P1588_TS_HB_SEL_5r
#define WRITE_P1588_TS_HB_SEL_5r PLP_WRITE_P1588_TS_HB_SEL_5r
#define MODIFY_P1588_TS_HB_SEL_5r PLP_MODIFY_P1588_TS_HB_SEL_5r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TS_HB_SEL_5r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TS_HB_SEL_6
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7042
 * DESC:     TS HB SEL 6 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOP_TS_INFO_39_24 Contains db info[15:15] - 1588 Packet found[14:14] - UDP match for 1588[13:13] - DA match for 1588[12:12] - Ethertype match for 1588/IPv4/IPv6[11:6]  - FSM max[5:0]   - FSM min
 *
 ******************************************************************************/
#define PLP_P1588_TS_HB_SEL_6r    0x00000042

#define PLP_P1588_TS_HB_SEL_6r_SIZE 4

/*
 * This structure should be used to declare and program P1588_TS_HB_SEL_6.
 *
 */
typedef union PLP_P1588_TS_HB_SEL_6r_s {
	uint32_t v[1];
	uint32_t p1588_ts_hb_sel_6[1];
	uint32_t _p1588_ts_hb_sel_6;
} PLP_P1588_TS_HB_SEL_6r_t;

#define PLP_P1588_TS_HB_SEL_6r_CLR(r) (r).p1588_ts_hb_sel_6[0] = 0
#define PLP_P1588_TS_HB_SEL_6r_SET(r,d) (r).p1588_ts_hb_sel_6[0] = d
#define PLP_P1588_TS_HB_SEL_6r_GET(r) (r).p1588_ts_hb_sel_6[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TS_HB_SEL_6r_SOP_TS_INFO_39_24f_GET(r) (((r).p1588_ts_hb_sel_6[0]) & 0xffff)
#define PLP_P1588_TS_HB_SEL_6r_SOP_TS_INFO_39_24f_SET(r,f) (r).p1588_ts_hb_sel_6[0]=(((r).p1588_ts_hb_sel_6[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TS_HB_SEL_6.
 *
 */
#define PLP_READ_P1588_TS_HB_SEL_6r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TS_HB_SEL_6r,(_r._p1588_ts_hb_sel_6))
#define PLP_WRITE_P1588_TS_HB_SEL_6r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TS_HB_SEL_6r,(_r._p1588_ts_hb_sel_6))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TS_HB_SEL_6r PLP_P1588_TS_HB_SEL_6r
#define P1588_TS_HB_SEL_6r_SIZE PLP_P1588_TS_HB_SEL_6r_SIZE
typedef PLP_P1588_TS_HB_SEL_6r_t P1588_TS_HB_SEL_6r_t;
#define P1588_TS_HB_SEL_6r_CLR PLP_P1588_TS_HB_SEL_6r_CLR
#define P1588_TS_HB_SEL_6r_SET PLP_P1588_TS_HB_SEL_6r_SET
#define P1588_TS_HB_SEL_6r_GET PLP_P1588_TS_HB_SEL_6r_GET
#define P1588_TS_HB_SEL_6r_SOP_TS_INFO_39_24f_GET PLP_P1588_TS_HB_SEL_6r_SOP_TS_INFO_39_24f_GET
#define P1588_TS_HB_SEL_6r_SOP_TS_INFO_39_24f_SET PLP_P1588_TS_HB_SEL_6r_SOP_TS_INFO_39_24f_SET
#define READ_P1588_TS_HB_SEL_6r PLP_READ_P1588_TS_HB_SEL_6r
#define WRITE_P1588_TS_HB_SEL_6r PLP_WRITE_P1588_TS_HB_SEL_6r
#define MODIFY_P1588_TS_HB_SEL_6r PLP_MODIFY_P1588_TS_HB_SEL_6r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TS_HB_SEL_6r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TS_HB_SEL_7
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7043
 * DESC:     TS HB SEL 7 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOP_TS_INFO_23_16 [7:5] - Contains Port No[4:4] - Contains Tx/Rx info[3:0] - Contains Message Type info
 *
 ******************************************************************************/
#define PLP_P1588_TS_HB_SEL_7r    0x00000043

#define PLP_P1588_TS_HB_SEL_7r_SIZE 4

/*
 * This structure should be used to declare and program P1588_TS_HB_SEL_7.
 *
 */
typedef union PLP_P1588_TS_HB_SEL_7r_s {
	uint32_t v[1];
	uint32_t p1588_ts_hb_sel_7[1];
	uint32_t _p1588_ts_hb_sel_7;
} PLP_P1588_TS_HB_SEL_7r_t;

#define PLP_P1588_TS_HB_SEL_7r_CLR(r) (r).p1588_ts_hb_sel_7[0] = 0
#define PLP_P1588_TS_HB_SEL_7r_SET(r,d) (r).p1588_ts_hb_sel_7[0] = d
#define PLP_P1588_TS_HB_SEL_7r_GET(r) (r).p1588_ts_hb_sel_7[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TS_HB_SEL_7r_SOP_TS_INFO_23_16f_GET(r) (((r).p1588_ts_hb_sel_7[0]) & 0xff)
#define PLP_P1588_TS_HB_SEL_7r_SOP_TS_INFO_23_16f_SET(r,f) (r).p1588_ts_hb_sel_7[0]=(((r).p1588_ts_hb_sel_7[0] & ~((uint32_t)0xff)) | (((uint32_t)f) & 0xff)) | (0xff << 16)

/*
 * These macros can be used to access P1588_TS_HB_SEL_7.
 *
 */
#define PLP_READ_P1588_TS_HB_SEL_7r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TS_HB_SEL_7r,(_r._p1588_ts_hb_sel_7))
#define PLP_WRITE_P1588_TS_HB_SEL_7r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TS_HB_SEL_7r,(_r._p1588_ts_hb_sel_7))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TS_HB_SEL_7r PLP_P1588_TS_HB_SEL_7r
#define P1588_TS_HB_SEL_7r_SIZE PLP_P1588_TS_HB_SEL_7r_SIZE
typedef PLP_P1588_TS_HB_SEL_7r_t P1588_TS_HB_SEL_7r_t;
#define P1588_TS_HB_SEL_7r_CLR PLP_P1588_TS_HB_SEL_7r_CLR
#define P1588_TS_HB_SEL_7r_SET PLP_P1588_TS_HB_SEL_7r_SET
#define P1588_TS_HB_SEL_7r_GET PLP_P1588_TS_HB_SEL_7r_GET
#define P1588_TS_HB_SEL_7r_SOP_TS_INFO_23_16f_GET PLP_P1588_TS_HB_SEL_7r_SOP_TS_INFO_23_16f_GET
#define P1588_TS_HB_SEL_7r_SOP_TS_INFO_23_16f_SET PLP_P1588_TS_HB_SEL_7r_SOP_TS_INFO_23_16f_SET
#define READ_P1588_TS_HB_SEL_7r PLP_READ_P1588_TS_HB_SEL_7r
#define WRITE_P1588_TS_HB_SEL_7r PLP_WRITE_P1588_TS_HB_SEL_7r
#define MODIFY_P1588_TS_HB_SEL_7r PLP_MODIFY_P1588_TS_HB_SEL_7r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TS_HB_SEL_7r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TS_HB_SEL_8
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7044
 * DESC:     TS HB SEL 8 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOP_TS_INFO_15_0 [15:0] - Contains SeqID info
 *
 ******************************************************************************/
#define PLP_P1588_TS_HB_SEL_8r    0x00000044

#define PLP_P1588_TS_HB_SEL_8r_SIZE 4

/*
 * This structure should be used to declare and program P1588_TS_HB_SEL_8.
 *
 */
typedef union PLP_P1588_TS_HB_SEL_8r_s {
	uint32_t v[1];
	uint32_t p1588_ts_hb_sel_8[1];
	uint32_t _p1588_ts_hb_sel_8;
} PLP_P1588_TS_HB_SEL_8r_t;

#define PLP_P1588_TS_HB_SEL_8r_CLR(r) (r).p1588_ts_hb_sel_8[0] = 0
#define PLP_P1588_TS_HB_SEL_8r_SET(r,d) (r).p1588_ts_hb_sel_8[0] = d
#define PLP_P1588_TS_HB_SEL_8r_GET(r) (r).p1588_ts_hb_sel_8[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TS_HB_SEL_8r_SOP_TS_INFO_15_0f_GET(r) (((r).p1588_ts_hb_sel_8[0]) & 0xffff)
#define PLP_P1588_TS_HB_SEL_8r_SOP_TS_INFO_15_0f_SET(r,f) (r).p1588_ts_hb_sel_8[0]=(((r).p1588_ts_hb_sel_8[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TS_HB_SEL_8.
 *
 */
#define PLP_READ_P1588_TS_HB_SEL_8r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TS_HB_SEL_8r,(_r._p1588_ts_hb_sel_8))
#define PLP_WRITE_P1588_TS_HB_SEL_8r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TS_HB_SEL_8r,(_r._p1588_ts_hb_sel_8))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TS_HB_SEL_8r PLP_P1588_TS_HB_SEL_8r
#define P1588_TS_HB_SEL_8r_SIZE PLP_P1588_TS_HB_SEL_8r_SIZE
typedef PLP_P1588_TS_HB_SEL_8r_t P1588_TS_HB_SEL_8r_t;
#define P1588_TS_HB_SEL_8r_CLR PLP_P1588_TS_HB_SEL_8r_CLR
#define P1588_TS_HB_SEL_8r_SET PLP_P1588_TS_HB_SEL_8r_SET
#define P1588_TS_HB_SEL_8r_GET PLP_P1588_TS_HB_SEL_8r_GET
#define P1588_TS_HB_SEL_8r_SOP_TS_INFO_15_0f_GET PLP_P1588_TS_HB_SEL_8r_SOP_TS_INFO_15_0f_GET
#define P1588_TS_HB_SEL_8r_SOP_TS_INFO_15_0f_SET PLP_P1588_TS_HB_SEL_8r_SOP_TS_INFO_15_0f_SET
#define READ_P1588_TS_HB_SEL_8r PLP_READ_P1588_TS_HB_SEL_8r
#define WRITE_P1588_TS_HB_SEL_8r PLP_WRITE_P1588_TS_HB_SEL_8r
#define MODIFY_P1588_TS_HB_SEL_8r PLP_MODIFY_P1588_TS_HB_SEL_8r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TS_HB_SEL_8r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TS_HB_SEL_9
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7045
 * DESC:     TS HB SEL 9 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     HB_TS_47_32      Local time NCO[47:32] snapshot with write to nse_reg_sc_synmode_cntrl_13
 *
 ******************************************************************************/
#define PLP_P1588_TS_HB_SEL_9r    0x00000045

#define PLP_P1588_TS_HB_SEL_9r_SIZE 4

/*
 * This structure should be used to declare and program P1588_TS_HB_SEL_9.
 *
 */
typedef union PLP_P1588_TS_HB_SEL_9r_s {
	uint32_t v[1];
	uint32_t p1588_ts_hb_sel_9[1];
	uint32_t _p1588_ts_hb_sel_9;
} PLP_P1588_TS_HB_SEL_9r_t;

#define PLP_P1588_TS_HB_SEL_9r_CLR(r) (r).p1588_ts_hb_sel_9[0] = 0
#define PLP_P1588_TS_HB_SEL_9r_SET(r,d) (r).p1588_ts_hb_sel_9[0] = d
#define PLP_P1588_TS_HB_SEL_9r_GET(r) (r).p1588_ts_hb_sel_9[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TS_HB_SEL_9r_HB_TS_47_32f_GET(r) (((r).p1588_ts_hb_sel_9[0]) & 0xffff)
#define PLP_P1588_TS_HB_SEL_9r_HB_TS_47_32f_SET(r,f) (r).p1588_ts_hb_sel_9[0]=(((r).p1588_ts_hb_sel_9[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TS_HB_SEL_9.
 *
 */
#define PLP_READ_P1588_TS_HB_SEL_9r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TS_HB_SEL_9r,(_r._p1588_ts_hb_sel_9))
#define PLP_WRITE_P1588_TS_HB_SEL_9r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TS_HB_SEL_9r,(_r._p1588_ts_hb_sel_9))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TS_HB_SEL_9r PLP_P1588_TS_HB_SEL_9r
#define P1588_TS_HB_SEL_9r_SIZE PLP_P1588_TS_HB_SEL_9r_SIZE
typedef PLP_P1588_TS_HB_SEL_9r_t P1588_TS_HB_SEL_9r_t;
#define P1588_TS_HB_SEL_9r_CLR PLP_P1588_TS_HB_SEL_9r_CLR
#define P1588_TS_HB_SEL_9r_SET PLP_P1588_TS_HB_SEL_9r_SET
#define P1588_TS_HB_SEL_9r_GET PLP_P1588_TS_HB_SEL_9r_GET
#define P1588_TS_HB_SEL_9r_HB_TS_47_32f_GET PLP_P1588_TS_HB_SEL_9r_HB_TS_47_32f_GET
#define P1588_TS_HB_SEL_9r_HB_TS_47_32f_SET PLP_P1588_TS_HB_SEL_9r_HB_TS_47_32f_SET
#define READ_P1588_TS_HB_SEL_9r PLP_READ_P1588_TS_HB_SEL_9r
#define WRITE_P1588_TS_HB_SEL_9r PLP_WRITE_P1588_TS_HB_SEL_9r
#define MODIFY_P1588_TS_HB_SEL_9r PLP_MODIFY_P1588_TS_HB_SEL_9r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TS_HB_SEL_9r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TS_HB_SEL_10
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7046
 * DESC:     TS HB SEL 10 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     HB_TS_31_16      Local time NCO[31:16] snapshot with write to nse_reg_sc_synmode_cntrl_13
 *
 ******************************************************************************/
#define PLP_P1588_TS_HB_SEL_10r    0x00000046

#define PLP_P1588_TS_HB_SEL_10r_SIZE 4

/*
 * This structure should be used to declare and program P1588_TS_HB_SEL_10.
 *
 */
typedef union PLP_P1588_TS_HB_SEL_10r_s {
	uint32_t v[1];
	uint32_t p1588_ts_hb_sel_10[1];
	uint32_t _p1588_ts_hb_sel_10;
} PLP_P1588_TS_HB_SEL_10r_t;

#define PLP_P1588_TS_HB_SEL_10r_CLR(r) (r).p1588_ts_hb_sel_10[0] = 0
#define PLP_P1588_TS_HB_SEL_10r_SET(r,d) (r).p1588_ts_hb_sel_10[0] = d
#define PLP_P1588_TS_HB_SEL_10r_GET(r) (r).p1588_ts_hb_sel_10[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TS_HB_SEL_10r_HB_TS_31_16f_GET(r) (((r).p1588_ts_hb_sel_10[0]) & 0xffff)
#define PLP_P1588_TS_HB_SEL_10r_HB_TS_31_16f_SET(r,f) (r).p1588_ts_hb_sel_10[0]=(((r).p1588_ts_hb_sel_10[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TS_HB_SEL_10.
 *
 */
#define PLP_READ_P1588_TS_HB_SEL_10r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TS_HB_SEL_10r,(_r._p1588_ts_hb_sel_10))
#define PLP_WRITE_P1588_TS_HB_SEL_10r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TS_HB_SEL_10r,(_r._p1588_ts_hb_sel_10))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TS_HB_SEL_10r PLP_P1588_TS_HB_SEL_10r
#define P1588_TS_HB_SEL_10r_SIZE PLP_P1588_TS_HB_SEL_10r_SIZE
typedef PLP_P1588_TS_HB_SEL_10r_t P1588_TS_HB_SEL_10r_t;
#define P1588_TS_HB_SEL_10r_CLR PLP_P1588_TS_HB_SEL_10r_CLR
#define P1588_TS_HB_SEL_10r_SET PLP_P1588_TS_HB_SEL_10r_SET
#define P1588_TS_HB_SEL_10r_GET PLP_P1588_TS_HB_SEL_10r_GET
#define P1588_TS_HB_SEL_10r_HB_TS_31_16f_GET PLP_P1588_TS_HB_SEL_10r_HB_TS_31_16f_GET
#define P1588_TS_HB_SEL_10r_HB_TS_31_16f_SET PLP_P1588_TS_HB_SEL_10r_HB_TS_31_16f_SET
#define READ_P1588_TS_HB_SEL_10r PLP_READ_P1588_TS_HB_SEL_10r
#define WRITE_P1588_TS_HB_SEL_10r PLP_WRITE_P1588_TS_HB_SEL_10r
#define MODIFY_P1588_TS_HB_SEL_10r PLP_MODIFY_P1588_TS_HB_SEL_10r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TS_HB_SEL_10r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TS_HB_SEL_11
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7047
 * DESC:     TS HB SEL 11 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     HB_TS_15_0       Local time NCO[15:0] snapshot with write to nse_reg_sc_synmode_cntrl_13
 *
 ******************************************************************************/
#define PLP_P1588_TS_HB_SEL_11r    0x00000047

#define PLP_P1588_TS_HB_SEL_11r_SIZE 4

/*
 * This structure should be used to declare and program P1588_TS_HB_SEL_11.
 *
 */
typedef union PLP_P1588_TS_HB_SEL_11r_s {
	uint32_t v[1];
	uint32_t p1588_ts_hb_sel_11[1];
	uint32_t _p1588_ts_hb_sel_11;
} PLP_P1588_TS_HB_SEL_11r_t;

#define PLP_P1588_TS_HB_SEL_11r_CLR(r) (r).p1588_ts_hb_sel_11[0] = 0
#define PLP_P1588_TS_HB_SEL_11r_SET(r,d) (r).p1588_ts_hb_sel_11[0] = d
#define PLP_P1588_TS_HB_SEL_11r_GET(r) (r).p1588_ts_hb_sel_11[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TS_HB_SEL_11r_HB_TS_15_0f_GET(r) (((r).p1588_ts_hb_sel_11[0]) & 0xffff)
#define PLP_P1588_TS_HB_SEL_11r_HB_TS_15_0f_SET(r,f) (r).p1588_ts_hb_sel_11[0]=(((r).p1588_ts_hb_sel_11[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TS_HB_SEL_11.
 *
 */
#define PLP_READ_P1588_TS_HB_SEL_11r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TS_HB_SEL_11r,(_r._p1588_ts_hb_sel_11))
#define PLP_WRITE_P1588_TS_HB_SEL_11r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TS_HB_SEL_11r,(_r._p1588_ts_hb_sel_11))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TS_HB_SEL_11r PLP_P1588_TS_HB_SEL_11r
#define P1588_TS_HB_SEL_11r_SIZE PLP_P1588_TS_HB_SEL_11r_SIZE
typedef PLP_P1588_TS_HB_SEL_11r_t P1588_TS_HB_SEL_11r_t;
#define P1588_TS_HB_SEL_11r_CLR PLP_P1588_TS_HB_SEL_11r_CLR
#define P1588_TS_HB_SEL_11r_SET PLP_P1588_TS_HB_SEL_11r_SET
#define P1588_TS_HB_SEL_11r_GET PLP_P1588_TS_HB_SEL_11r_GET
#define P1588_TS_HB_SEL_11r_HB_TS_15_0f_GET PLP_P1588_TS_HB_SEL_11r_HB_TS_15_0f_GET
#define P1588_TS_HB_SEL_11r_HB_TS_15_0f_SET PLP_P1588_TS_HB_SEL_11r_HB_TS_15_0f_SET
#define READ_P1588_TS_HB_SEL_11r PLP_READ_P1588_TS_HB_SEL_11r
#define WRITE_P1588_TS_HB_SEL_11r PLP_WRITE_P1588_TS_HB_SEL_11r
#define MODIFY_P1588_TS_HB_SEL_11r PLP_MODIFY_P1588_TS_HB_SEL_11r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TS_HB_SEL_11r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_RX_SOP_COUNTER
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7048
 * DESC:     RX SOP COUNTER Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_COUNTER       Ingress 1G SOP counter. NOTE: UNUSEDNOTE: THESE BITS ARE CLEAR ON READ AND NEED TO BEHANDLED OUTSIDE THE REGISTER BLOCK
 *
 ******************************************************************************/
#define PLP_P1588_RX_SOP_COUNTERr    0x00000048

#define PLP_P1588_RX_SOP_COUNTERr_SIZE 4

/*
 * This structure should be used to declare and program P1588_RX_SOP_COUNTER.
 *
 */
typedef union PLP_P1588_RX_SOP_COUNTERr_s {
	uint32_t v[1];
	uint32_t p1588_rx_sop_counter[1];
	uint32_t _p1588_rx_sop_counter;
} PLP_P1588_RX_SOP_COUNTERr_t;

#define PLP_P1588_RX_SOP_COUNTERr_CLR(r) (r).p1588_rx_sop_counter[0] = 0
#define PLP_P1588_RX_SOP_COUNTERr_SET(r,d) (r).p1588_rx_sop_counter[0] = d
#define PLP_P1588_RX_SOP_COUNTERr_GET(r) (r).p1588_rx_sop_counter[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_RX_SOP_COUNTERr_RX_COUNTERf_GET(r) (((r).p1588_rx_sop_counter[0]) & 0xffff)
#define PLP_P1588_RX_SOP_COUNTERr_RX_COUNTERf_SET(r,f) (r).p1588_rx_sop_counter[0]=(((r).p1588_rx_sop_counter[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_RX_SOP_COUNTER.
 *
 */
#define PLP_READ_P1588_RX_SOP_COUNTERr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_RX_SOP_COUNTERr,(_r._p1588_rx_sop_counter))
#define PLP_WRITE_P1588_RX_SOP_COUNTERr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_RX_SOP_COUNTERr,(_r._p1588_rx_sop_counter))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_RX_SOP_COUNTERr PLP_P1588_RX_SOP_COUNTERr
#define P1588_RX_SOP_COUNTERr_SIZE PLP_P1588_RX_SOP_COUNTERr_SIZE
typedef PLP_P1588_RX_SOP_COUNTERr_t P1588_RX_SOP_COUNTERr_t;
#define P1588_RX_SOP_COUNTERr_CLR PLP_P1588_RX_SOP_COUNTERr_CLR
#define P1588_RX_SOP_COUNTERr_SET PLP_P1588_RX_SOP_COUNTERr_SET
#define P1588_RX_SOP_COUNTERr_GET PLP_P1588_RX_SOP_COUNTERr_GET
#define P1588_RX_SOP_COUNTERr_RX_COUNTERf_GET PLP_P1588_RX_SOP_COUNTERr_RX_COUNTERf_GET
#define P1588_RX_SOP_COUNTERr_RX_COUNTERf_SET PLP_P1588_RX_SOP_COUNTERr_RX_COUNTERf_SET
#define READ_P1588_RX_SOP_COUNTERr PLP_READ_P1588_RX_SOP_COUNTERr
#define WRITE_P1588_RX_SOP_COUNTERr PLP_WRITE_P1588_RX_SOP_COUNTERr
#define MODIFY_P1588_RX_SOP_COUNTERr PLP_MODIFY_P1588_RX_SOP_COUNTERr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_RX_SOP_COUNTERr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_RX_EOP_COUNTER
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7049
 * DESC:     RX EOP COUNTER Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_1588_COUNTER  Ingress 1G EOP counter. NOTE: UNUSEDNOTE: THESE BITS ARE CLEAR ON READ AND NEED TO BEHANDLED OUTSIDE THE REGISTER BLOCK
 *
 ******************************************************************************/
#define PLP_P1588_RX_EOP_COUNTERr    0x00000049

#define PLP_P1588_RX_EOP_COUNTERr_SIZE 4

/*
 * This structure should be used to declare and program P1588_RX_EOP_COUNTER.
 *
 */
typedef union PLP_P1588_RX_EOP_COUNTERr_s {
	uint32_t v[1];
	uint32_t p1588_rx_eop_counter[1];
	uint32_t _p1588_rx_eop_counter;
} PLP_P1588_RX_EOP_COUNTERr_t;

#define PLP_P1588_RX_EOP_COUNTERr_CLR(r) (r).p1588_rx_eop_counter[0] = 0
#define PLP_P1588_RX_EOP_COUNTERr_SET(r,d) (r).p1588_rx_eop_counter[0] = d
#define PLP_P1588_RX_EOP_COUNTERr_GET(r) (r).p1588_rx_eop_counter[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_RX_EOP_COUNTERr_RX_1588_COUNTERf_GET(r) (((r).p1588_rx_eop_counter[0]) & 0xff)
#define PLP_P1588_RX_EOP_COUNTERr_RX_1588_COUNTERf_SET(r,f) (r).p1588_rx_eop_counter[0]=(((r).p1588_rx_eop_counter[0] & ~((uint32_t)0xff)) | (((uint32_t)f) & 0xff)) | (0xff << 16)

/*
 * These macros can be used to access P1588_RX_EOP_COUNTER.
 *
 */
#define PLP_READ_P1588_RX_EOP_COUNTERr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_RX_EOP_COUNTERr,(_r._p1588_rx_eop_counter))
#define PLP_WRITE_P1588_RX_EOP_COUNTERr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_RX_EOP_COUNTERr,(_r._p1588_rx_eop_counter))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_RX_EOP_COUNTERr PLP_P1588_RX_EOP_COUNTERr
#define P1588_RX_EOP_COUNTERr_SIZE PLP_P1588_RX_EOP_COUNTERr_SIZE
typedef PLP_P1588_RX_EOP_COUNTERr_t P1588_RX_EOP_COUNTERr_t;
#define P1588_RX_EOP_COUNTERr_CLR PLP_P1588_RX_EOP_COUNTERr_CLR
#define P1588_RX_EOP_COUNTERr_SET PLP_P1588_RX_EOP_COUNTERr_SET
#define P1588_RX_EOP_COUNTERr_GET PLP_P1588_RX_EOP_COUNTERr_GET
#define P1588_RX_EOP_COUNTERr_RX_1588_COUNTERf_GET PLP_P1588_RX_EOP_COUNTERr_RX_1588_COUNTERf_GET
#define P1588_RX_EOP_COUNTERr_RX_1588_COUNTERf_SET PLP_P1588_RX_EOP_COUNTERr_RX_1588_COUNTERf_SET
#define READ_P1588_RX_EOP_COUNTERr PLP_READ_P1588_RX_EOP_COUNTERr
#define WRITE_P1588_RX_EOP_COUNTERr PLP_WRITE_P1588_RX_EOP_COUNTERr
#define MODIFY_P1588_RX_EOP_COUNTERr PLP_MODIFY_P1588_RX_EOP_COUNTERr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_RX_EOP_COUNTERr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TX_SOP_COUNTER
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x704a
 * DESC:     TX SOP COUNTER Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_COUNTER       Egress 1G SOP counter. NOTE: NOT USEDNOTE: THESE BITS ARE CLEAR ON READ AND NEED TO BEHANDLED OUTSIDE THE REGISTER BLOCK
 *
 ******************************************************************************/
#define PLP_P1588_TX_SOP_COUNTERr    0x0000004a

#define PLP_P1588_TX_SOP_COUNTERr_SIZE 4

/*
 * This structure should be used to declare and program P1588_TX_SOP_COUNTER.
 *
 */
typedef union PLP_P1588_TX_SOP_COUNTERr_s {
	uint32_t v[1];
	uint32_t p1588_tx_sop_counter[1];
	uint32_t _p1588_tx_sop_counter;
} PLP_P1588_TX_SOP_COUNTERr_t;

#define PLP_P1588_TX_SOP_COUNTERr_CLR(r) (r).p1588_tx_sop_counter[0] = 0
#define PLP_P1588_TX_SOP_COUNTERr_SET(r,d) (r).p1588_tx_sop_counter[0] = d
#define PLP_P1588_TX_SOP_COUNTERr_GET(r) (r).p1588_tx_sop_counter[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TX_SOP_COUNTERr_TX_COUNTERf_GET(r) (((r).p1588_tx_sop_counter[0]) & 0xffff)
#define PLP_P1588_TX_SOP_COUNTERr_TX_COUNTERf_SET(r,f) (r).p1588_tx_sop_counter[0]=(((r).p1588_tx_sop_counter[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TX_SOP_COUNTER.
 *
 */
#define PLP_READ_P1588_TX_SOP_COUNTERr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TX_SOP_COUNTERr,(_r._p1588_tx_sop_counter))
#define PLP_WRITE_P1588_TX_SOP_COUNTERr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TX_SOP_COUNTERr,(_r._p1588_tx_sop_counter))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TX_SOP_COUNTERr PLP_P1588_TX_SOP_COUNTERr
#define P1588_TX_SOP_COUNTERr_SIZE PLP_P1588_TX_SOP_COUNTERr_SIZE
typedef PLP_P1588_TX_SOP_COUNTERr_t P1588_TX_SOP_COUNTERr_t;
#define P1588_TX_SOP_COUNTERr_CLR PLP_P1588_TX_SOP_COUNTERr_CLR
#define P1588_TX_SOP_COUNTERr_SET PLP_P1588_TX_SOP_COUNTERr_SET
#define P1588_TX_SOP_COUNTERr_GET PLP_P1588_TX_SOP_COUNTERr_GET
#define P1588_TX_SOP_COUNTERr_TX_COUNTERf_GET PLP_P1588_TX_SOP_COUNTERr_TX_COUNTERf_GET
#define P1588_TX_SOP_COUNTERr_TX_COUNTERf_SET PLP_P1588_TX_SOP_COUNTERr_TX_COUNTERf_SET
#define READ_P1588_TX_SOP_COUNTERr PLP_READ_P1588_TX_SOP_COUNTERr
#define WRITE_P1588_TX_SOP_COUNTERr PLP_WRITE_P1588_TX_SOP_COUNTERr
#define MODIFY_P1588_TX_SOP_COUNTERr PLP_MODIFY_P1588_TX_SOP_COUNTERr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TX_SOP_COUNTERr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TX_EOP_COUNTER
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x704b
 * DESC:     TX EOP COUNTER Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_1588_COUNTER  Egress 1G EOP counter. NOTE: NOT USEDNOTE: THESE BITS ARE CLEAR ON READ AND NEED TO BEHANDLED OUTSIDE THE REGISTER BLOCK
 *
 ******************************************************************************/
#define PLP_P1588_TX_EOP_COUNTERr    0x0000004b

#define PLP_P1588_TX_EOP_COUNTERr_SIZE 4

/*
 * This structure should be used to declare and program P1588_TX_EOP_COUNTER.
 *
 */
typedef union PLP_P1588_TX_EOP_COUNTERr_s {
	uint32_t v[1];
	uint32_t p1588_tx_eop_counter[1];
	uint32_t _p1588_tx_eop_counter;
} PLP_P1588_TX_EOP_COUNTERr_t;

#define PLP_P1588_TX_EOP_COUNTERr_CLR(r) (r).p1588_tx_eop_counter[0] = 0
#define PLP_P1588_TX_EOP_COUNTERr_SET(r,d) (r).p1588_tx_eop_counter[0] = d
#define PLP_P1588_TX_EOP_COUNTERr_GET(r) (r).p1588_tx_eop_counter[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TX_EOP_COUNTERr_TX_1588_COUNTERf_GET(r) (((r).p1588_tx_eop_counter[0]) & 0xff)
#define PLP_P1588_TX_EOP_COUNTERr_TX_1588_COUNTERf_SET(r,f) (r).p1588_tx_eop_counter[0]=(((r).p1588_tx_eop_counter[0] & ~((uint32_t)0xff)) | (((uint32_t)f) & 0xff)) | (0xff << 16)

/*
 * These macros can be used to access P1588_TX_EOP_COUNTER.
 *
 */
#define PLP_READ_P1588_TX_EOP_COUNTERr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TX_EOP_COUNTERr,(_r._p1588_tx_eop_counter))
#define PLP_WRITE_P1588_TX_EOP_COUNTERr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TX_EOP_COUNTERr,(_r._p1588_tx_eop_counter))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TX_EOP_COUNTERr PLP_P1588_TX_EOP_COUNTERr
#define P1588_TX_EOP_COUNTERr_SIZE PLP_P1588_TX_EOP_COUNTERr_SIZE
typedef PLP_P1588_TX_EOP_COUNTERr_t P1588_TX_EOP_COUNTERr_t;
#define P1588_TX_EOP_COUNTERr_CLR PLP_P1588_TX_EOP_COUNTERr_CLR
#define P1588_TX_EOP_COUNTERr_SET PLP_P1588_TX_EOP_COUNTERr_SET
#define P1588_TX_EOP_COUNTERr_GET PLP_P1588_TX_EOP_COUNTERr_GET
#define P1588_TX_EOP_COUNTERr_TX_1588_COUNTERf_GET PLP_P1588_TX_EOP_COUNTERr_TX_1588_COUNTERf_GET
#define P1588_TX_EOP_COUNTERr_TX_1588_COUNTERf_SET PLP_P1588_TX_EOP_COUNTERr_TX_1588_COUNTERf_SET
#define READ_P1588_TX_EOP_COUNTERr PLP_READ_P1588_TX_EOP_COUNTERr
#define WRITE_P1588_TX_EOP_COUNTERr PLP_WRITE_P1588_TX_EOP_COUNTERr
#define MODIFY_P1588_TX_EOP_COUNTERr PLP_MODIFY_P1588_TX_EOP_COUNTERr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TX_EOP_COUNTERr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_RXPKT_SOP_10G_COUNTER
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x704c
 * DESC:     RXPKT SOP 10G COUNTER Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_SOP_10G_COUNTER Ingress SOP counter for all speedsNOTE: THESE BITS ARE CLEAR ON READ AND NEED TO BEHANDLED OUTSIDE THE REGISTER BLOCK
 *
 ******************************************************************************/
#define PLP_P1588_RXPKT_SOP_10G_COUNTERr    0x0000004c

#define PLP_P1588_RXPKT_SOP_10G_COUNTERr_SIZE 4

/*
 * This structure should be used to declare and program P1588_RXPKT_SOP_10G_COUNTER.
 *
 */
typedef union PLP_P1588_RXPKT_SOP_10G_COUNTERr_s {
	uint32_t v[1];
	uint32_t p1588_rxpkt_sop_10g_counter[1];
	uint32_t _p1588_rxpkt_sop_10g_counter;
} PLP_P1588_RXPKT_SOP_10G_COUNTERr_t;

#define PLP_P1588_RXPKT_SOP_10G_COUNTERr_CLR(r) (r).p1588_rxpkt_sop_10g_counter[0] = 0
#define PLP_P1588_RXPKT_SOP_10G_COUNTERr_SET(r,d) (r).p1588_rxpkt_sop_10g_counter[0] = d
#define PLP_P1588_RXPKT_SOP_10G_COUNTERr_GET(r) (r).p1588_rxpkt_sop_10g_counter[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_RXPKT_SOP_10G_COUNTERr_RX_SOP_10G_COUNTERf_GET(r) (((r).p1588_rxpkt_sop_10g_counter[0]) & 0xffff)
#define PLP_P1588_RXPKT_SOP_10G_COUNTERr_RX_SOP_10G_COUNTERf_SET(r,f) (r).p1588_rxpkt_sop_10g_counter[0]=(((r).p1588_rxpkt_sop_10g_counter[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_RXPKT_SOP_10G_COUNTER.
 *
 */
#define PLP_READ_P1588_RXPKT_SOP_10G_COUNTERr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_RXPKT_SOP_10G_COUNTERr,(_r._p1588_rxpkt_sop_10g_counter))
#define PLP_WRITE_P1588_RXPKT_SOP_10G_COUNTERr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_RXPKT_SOP_10G_COUNTERr,(_r._p1588_rxpkt_sop_10g_counter))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_RXPKT_SOP_10G_COUNTERr PLP_P1588_RXPKT_SOP_10G_COUNTERr
#define P1588_RXPKT_SOP_10G_COUNTERr_SIZE PLP_P1588_RXPKT_SOP_10G_COUNTERr_SIZE
typedef PLP_P1588_RXPKT_SOP_10G_COUNTERr_t P1588_RXPKT_SOP_10G_COUNTERr_t;
#define P1588_RXPKT_SOP_10G_COUNTERr_CLR PLP_P1588_RXPKT_SOP_10G_COUNTERr_CLR
#define P1588_RXPKT_SOP_10G_COUNTERr_SET PLP_P1588_RXPKT_SOP_10G_COUNTERr_SET
#define P1588_RXPKT_SOP_10G_COUNTERr_GET PLP_P1588_RXPKT_SOP_10G_COUNTERr_GET
#define P1588_RXPKT_SOP_10G_COUNTERr_RX_SOP_10G_COUNTERf_GET PLP_P1588_RXPKT_SOP_10G_COUNTERr_RX_SOP_10G_COUNTERf_GET
#define P1588_RXPKT_SOP_10G_COUNTERr_RX_SOP_10G_COUNTERf_SET PLP_P1588_RXPKT_SOP_10G_COUNTERr_RX_SOP_10G_COUNTERf_SET
#define READ_P1588_RXPKT_SOP_10G_COUNTERr PLP_READ_P1588_RXPKT_SOP_10G_COUNTERr
#define WRITE_P1588_RXPKT_SOP_10G_COUNTERr PLP_WRITE_P1588_RXPKT_SOP_10G_COUNTERr
#define MODIFY_P1588_RXPKT_SOP_10G_COUNTERr PLP_MODIFY_P1588_RXPKT_SOP_10G_COUNTERr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_RXPKT_SOP_10G_COUNTERr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_RXPKT_EOP_10G_COUNTER
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x704d
 * DESC:     RXPKT EOP 10G COUNTER Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_EOP_10G_COUNTER Ingress EOP counter for all speedsCount is dependent on type of packet and type of 1588 packetDefault is to count all incoming packetsNOTE: THESE BITS ARE CLEAR ON READ AND NEED TO BEHANDLED OUTSIDE THE REGISTER BLOCK
 *
 ******************************************************************************/
#define PLP_P1588_RXPKT_EOP_10G_COUNTERr    0x0000004d

#define PLP_P1588_RXPKT_EOP_10G_COUNTERr_SIZE 4

/*
 * This structure should be used to declare and program P1588_RXPKT_EOP_10G_COUNTER.
 *
 */
typedef union PLP_P1588_RXPKT_EOP_10G_COUNTERr_s {
	uint32_t v[1];
	uint32_t p1588_rxpkt_eop_10g_counter[1];
	uint32_t _p1588_rxpkt_eop_10g_counter;
} PLP_P1588_RXPKT_EOP_10G_COUNTERr_t;

#define PLP_P1588_RXPKT_EOP_10G_COUNTERr_CLR(r) (r).p1588_rxpkt_eop_10g_counter[0] = 0
#define PLP_P1588_RXPKT_EOP_10G_COUNTERr_SET(r,d) (r).p1588_rxpkt_eop_10g_counter[0] = d
#define PLP_P1588_RXPKT_EOP_10G_COUNTERr_GET(r) (r).p1588_rxpkt_eop_10g_counter[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_RXPKT_EOP_10G_COUNTERr_RX_EOP_10G_COUNTERf_GET(r) (((r).p1588_rxpkt_eop_10g_counter[0]) & 0xffff)
#define PLP_P1588_RXPKT_EOP_10G_COUNTERr_RX_EOP_10G_COUNTERf_SET(r,f) (r).p1588_rxpkt_eop_10g_counter[0]=(((r).p1588_rxpkt_eop_10g_counter[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_RXPKT_EOP_10G_COUNTER.
 *
 */
#define PLP_READ_P1588_RXPKT_EOP_10G_COUNTERr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_RXPKT_EOP_10G_COUNTERr,(_r._p1588_rxpkt_eop_10g_counter))
#define PLP_WRITE_P1588_RXPKT_EOP_10G_COUNTERr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_RXPKT_EOP_10G_COUNTERr,(_r._p1588_rxpkt_eop_10g_counter))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_RXPKT_EOP_10G_COUNTERr PLP_P1588_RXPKT_EOP_10G_COUNTERr
#define P1588_RXPKT_EOP_10G_COUNTERr_SIZE PLP_P1588_RXPKT_EOP_10G_COUNTERr_SIZE
typedef PLP_P1588_RXPKT_EOP_10G_COUNTERr_t P1588_RXPKT_EOP_10G_COUNTERr_t;
#define P1588_RXPKT_EOP_10G_COUNTERr_CLR PLP_P1588_RXPKT_EOP_10G_COUNTERr_CLR
#define P1588_RXPKT_EOP_10G_COUNTERr_SET PLP_P1588_RXPKT_EOP_10G_COUNTERr_SET
#define P1588_RXPKT_EOP_10G_COUNTERr_GET PLP_P1588_RXPKT_EOP_10G_COUNTERr_GET
#define P1588_RXPKT_EOP_10G_COUNTERr_RX_EOP_10G_COUNTERf_GET PLP_P1588_RXPKT_EOP_10G_COUNTERr_RX_EOP_10G_COUNTERf_GET
#define P1588_RXPKT_EOP_10G_COUNTERr_RX_EOP_10G_COUNTERf_SET PLP_P1588_RXPKT_EOP_10G_COUNTERr_RX_EOP_10G_COUNTERf_SET
#define READ_P1588_RXPKT_EOP_10G_COUNTERr PLP_READ_P1588_RXPKT_EOP_10G_COUNTERr
#define WRITE_P1588_RXPKT_EOP_10G_COUNTERr PLP_WRITE_P1588_RXPKT_EOP_10G_COUNTERr
#define MODIFY_P1588_RXPKT_EOP_10G_COUNTERr PLP_MODIFY_P1588_RXPKT_EOP_10G_COUNTERr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_RXPKT_EOP_10G_COUNTERr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TXPKT_SOP_10G_COUNTER
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x704e
 * DESC:     TXPKT SOP 10G COUNTER Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_SOP_10G_COUNTER Egress SOP counter for all speedsNOTE: THESE BITS ARE CLEAR ON READ AND NEED TO BEHANDLED OUTSIDE THE REGISTER BLOCK
 *
 ******************************************************************************/
#define PLP_P1588_TXPKT_SOP_10G_COUNTERr    0x0000004e

#define PLP_P1588_TXPKT_SOP_10G_COUNTERr_SIZE 4

/*
 * This structure should be used to declare and program P1588_TXPKT_SOP_10G_COUNTER.
 *
 */
typedef union PLP_P1588_TXPKT_SOP_10G_COUNTERr_s {
	uint32_t v[1];
	uint32_t p1588_txpkt_sop_10g_counter[1];
	uint32_t _p1588_txpkt_sop_10g_counter;
} PLP_P1588_TXPKT_SOP_10G_COUNTERr_t;

#define PLP_P1588_TXPKT_SOP_10G_COUNTERr_CLR(r) (r).p1588_txpkt_sop_10g_counter[0] = 0
#define PLP_P1588_TXPKT_SOP_10G_COUNTERr_SET(r,d) (r).p1588_txpkt_sop_10g_counter[0] = d
#define PLP_P1588_TXPKT_SOP_10G_COUNTERr_GET(r) (r).p1588_txpkt_sop_10g_counter[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TXPKT_SOP_10G_COUNTERr_TX_SOP_10G_COUNTERf_GET(r) (((r).p1588_txpkt_sop_10g_counter[0]) & 0xffff)
#define PLP_P1588_TXPKT_SOP_10G_COUNTERr_TX_SOP_10G_COUNTERf_SET(r,f) (r).p1588_txpkt_sop_10g_counter[0]=(((r).p1588_txpkt_sop_10g_counter[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TXPKT_SOP_10G_COUNTER.
 *
 */
#define PLP_READ_P1588_TXPKT_SOP_10G_COUNTERr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TXPKT_SOP_10G_COUNTERr,(_r._p1588_txpkt_sop_10g_counter))
#define PLP_WRITE_P1588_TXPKT_SOP_10G_COUNTERr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TXPKT_SOP_10G_COUNTERr,(_r._p1588_txpkt_sop_10g_counter))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TXPKT_SOP_10G_COUNTERr PLP_P1588_TXPKT_SOP_10G_COUNTERr
#define P1588_TXPKT_SOP_10G_COUNTERr_SIZE PLP_P1588_TXPKT_SOP_10G_COUNTERr_SIZE
typedef PLP_P1588_TXPKT_SOP_10G_COUNTERr_t P1588_TXPKT_SOP_10G_COUNTERr_t;
#define P1588_TXPKT_SOP_10G_COUNTERr_CLR PLP_P1588_TXPKT_SOP_10G_COUNTERr_CLR
#define P1588_TXPKT_SOP_10G_COUNTERr_SET PLP_P1588_TXPKT_SOP_10G_COUNTERr_SET
#define P1588_TXPKT_SOP_10G_COUNTERr_GET PLP_P1588_TXPKT_SOP_10G_COUNTERr_GET
#define P1588_TXPKT_SOP_10G_COUNTERr_TX_SOP_10G_COUNTERf_GET PLP_P1588_TXPKT_SOP_10G_COUNTERr_TX_SOP_10G_COUNTERf_GET
#define P1588_TXPKT_SOP_10G_COUNTERr_TX_SOP_10G_COUNTERf_SET PLP_P1588_TXPKT_SOP_10G_COUNTERr_TX_SOP_10G_COUNTERf_SET
#define READ_P1588_TXPKT_SOP_10G_COUNTERr PLP_READ_P1588_TXPKT_SOP_10G_COUNTERr
#define WRITE_P1588_TXPKT_SOP_10G_COUNTERr PLP_WRITE_P1588_TXPKT_SOP_10G_COUNTERr
#define MODIFY_P1588_TXPKT_SOP_10G_COUNTERr PLP_MODIFY_P1588_TXPKT_SOP_10G_COUNTERr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TXPKT_SOP_10G_COUNTERr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TXPKT_EOP_10G_COUNTER
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x704f
 * DESC:     TXPKT EOP 10G COUNTER Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_EOP_10G_COUNTER Egress EOP counter for all speedsCount is dependent on type of packet and type of 1588 packetDefault is to count all incoming packetsNOTE: THESE BITS ARE CLEAR ON READ AND NEED TO BEHANDLED OUTSIDE THE REGISTER BLOCK
 *
 ******************************************************************************/
#define PLP_P1588_TXPKT_EOP_10G_COUNTERr    0x0000004f

#define PLP_P1588_TXPKT_EOP_10G_COUNTERr_SIZE 4

/*
 * This structure should be used to declare and program P1588_TXPKT_EOP_10G_COUNTER.
 *
 */
typedef union PLP_P1588_TXPKT_EOP_10G_COUNTERr_s {
	uint32_t v[1];
	uint32_t p1588_txpkt_eop_10g_counter[1];
	uint32_t _p1588_txpkt_eop_10g_counter;
} PLP_P1588_TXPKT_EOP_10G_COUNTERr_t;

#define PLP_P1588_TXPKT_EOP_10G_COUNTERr_CLR(r) (r).p1588_txpkt_eop_10g_counter[0] = 0
#define PLP_P1588_TXPKT_EOP_10G_COUNTERr_SET(r,d) (r).p1588_txpkt_eop_10g_counter[0] = d
#define PLP_P1588_TXPKT_EOP_10G_COUNTERr_GET(r) (r).p1588_txpkt_eop_10g_counter[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TXPKT_EOP_10G_COUNTERr_TX_EOP_10G_COUNTERf_GET(r) (((r).p1588_txpkt_eop_10g_counter[0]) & 0xffff)
#define PLP_P1588_TXPKT_EOP_10G_COUNTERr_TX_EOP_10G_COUNTERf_SET(r,f) (r).p1588_txpkt_eop_10g_counter[0]=(((r).p1588_txpkt_eop_10g_counter[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TXPKT_EOP_10G_COUNTER.
 *
 */
#define PLP_READ_P1588_TXPKT_EOP_10G_COUNTERr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TXPKT_EOP_10G_COUNTERr,(_r._p1588_txpkt_eop_10g_counter))
#define PLP_WRITE_P1588_TXPKT_EOP_10G_COUNTERr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TXPKT_EOP_10G_COUNTERr,(_r._p1588_txpkt_eop_10g_counter))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TXPKT_EOP_10G_COUNTERr PLP_P1588_TXPKT_EOP_10G_COUNTERr
#define P1588_TXPKT_EOP_10G_COUNTERr_SIZE PLP_P1588_TXPKT_EOP_10G_COUNTERr_SIZE
typedef PLP_P1588_TXPKT_EOP_10G_COUNTERr_t P1588_TXPKT_EOP_10G_COUNTERr_t;
#define P1588_TXPKT_EOP_10G_COUNTERr_CLR PLP_P1588_TXPKT_EOP_10G_COUNTERr_CLR
#define P1588_TXPKT_EOP_10G_COUNTERr_SET PLP_P1588_TXPKT_EOP_10G_COUNTERr_SET
#define P1588_TXPKT_EOP_10G_COUNTERr_GET PLP_P1588_TXPKT_EOP_10G_COUNTERr_GET
#define P1588_TXPKT_EOP_10G_COUNTERr_TX_EOP_10G_COUNTERf_GET PLP_P1588_TXPKT_EOP_10G_COUNTERr_TX_EOP_10G_COUNTERf_GET
#define P1588_TXPKT_EOP_10G_COUNTERr_TX_EOP_10G_COUNTERf_SET PLP_P1588_TXPKT_EOP_10G_COUNTERr_TX_EOP_10G_COUNTERf_SET
#define READ_P1588_TXPKT_EOP_10G_COUNTERr PLP_READ_P1588_TXPKT_EOP_10G_COUNTERr
#define WRITE_P1588_TXPKT_EOP_10G_COUNTERr PLP_WRITE_P1588_TXPKT_EOP_10G_COUNTERr
#define MODIFY_P1588_TXPKT_EOP_10G_COUNTERr PLP_MODIFY_P1588_TXPKT_EOP_10G_COUNTERr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TXPKT_EOP_10G_COUNTERr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PKT_COUNT_SEL
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7050
 * DESC:     PKT COUNT SEL Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_P1588_MSG_SEL Ingress EOP packet count select'b100 - counts all Sync message packets'b101 - counts all Delay Request message packets'b110 - counts all PDelay Request message packets'b111 - counts all PDelay Response message packets'b0xx - counts all 1588 message packets
 *     RX_P1588_PKT_SEL Packet type selection for count0 - All packets1 - Count only 1588 packets
 *     TX_P1588_MSG_SEL Ingress EOP packet count select'b100 - counts all Sync message packets'b101 - counts all Delay Request message packets'b110 - counts all PDelay Request message packets'b111 - counts all PDelay Response message packets'b0xx - counts all 1588 message packets
 *     TX_P1588_PKT_SEL Packet type selection for count0 - All packets1 - Count only 1588 packets
 *     RX_CRC_COUNT_EN  0 - Counts EOP packets in 1G and 10G1 - Counts CRC Error packets or EOP packets in dual clock, depending on mpls_rx_spare bit
 *     TX_CRC_COUNT_EN  0 - Counts EOP packets in 1G and 10G1 - Counts CRC Error packets or EOP packets in dual clock, depending on mpls_tx_spare bit
 *
 ******************************************************************************/
#define PLP_P1588_PKT_COUNT_SELr    0x00000050

#define PLP_P1588_PKT_COUNT_SELr_SIZE 4

/*
 * This structure should be used to declare and program P1588_PKT_COUNT_SEL.
 *
 */
typedef union PLP_P1588_PKT_COUNT_SELr_s {
	uint32_t v[1];
	uint32_t p1588_pkt_count_sel[1];
	uint32_t _p1588_pkt_count_sel;
} PLP_P1588_PKT_COUNT_SELr_t;

#define PLP_P1588_PKT_COUNT_SELr_CLR(r) (r).p1588_pkt_count_sel[0] = 0
#define PLP_P1588_PKT_COUNT_SELr_SET(r,d) (r).p1588_pkt_count_sel[0] = d
#define PLP_P1588_PKT_COUNT_SELr_GET(r) (r).p1588_pkt_count_sel[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PKT_COUNT_SELr_TX_CRC_COUNT_ENf_GET(r) ((((r).p1588_pkt_count_sel[0]) >> 15) & 0x1)
#define PLP_P1588_PKT_COUNT_SELr_TX_CRC_COUNT_ENf_SET(r,f) (r).p1588_pkt_count_sel[0]=(((r).p1588_pkt_count_sel[0] & ~((uint32_t)0x1 << 15)) | ((((uint32_t)f) & 0x1) << 15)) | (1 << (16 + 15))
#define PLP_P1588_PKT_COUNT_SELr_RX_CRC_COUNT_ENf_GET(r) ((((r).p1588_pkt_count_sel[0]) >> 14) & 0x1)
#define PLP_P1588_PKT_COUNT_SELr_RX_CRC_COUNT_ENf_SET(r,f) (r).p1588_pkt_count_sel[0]=(((r).p1588_pkt_count_sel[0] & ~((uint32_t)0x1 << 14)) | ((((uint32_t)f) & 0x1) << 14)) | (1 << (16 + 14))
#define PLP_P1588_PKT_COUNT_SELr_TX_P1588_PKT_SELf_GET(r) ((((r).p1588_pkt_count_sel[0]) >> 7) & 0x1)
#define PLP_P1588_PKT_COUNT_SELr_TX_P1588_PKT_SELf_SET(r,f) (r).p1588_pkt_count_sel[0]=(((r).p1588_pkt_count_sel[0] & ~((uint32_t)0x1 << 7)) | ((((uint32_t)f) & 0x1) << 7)) | (1 << (16 + 7))
#define PLP_P1588_PKT_COUNT_SELr_TX_P1588_MSG_SELf_GET(r) ((((r).p1588_pkt_count_sel[0]) >> 4) & 0x7)
#define PLP_P1588_PKT_COUNT_SELr_TX_P1588_MSG_SELf_SET(r,f) (r).p1588_pkt_count_sel[0]=(((r).p1588_pkt_count_sel[0] & ~((uint32_t)0x7 << 4)) | ((((uint32_t)f) & 0x7) << 4)) | (7 << (16 + 4))
#define PLP_P1588_PKT_COUNT_SELr_RX_P1588_PKT_SELf_GET(r) ((((r).p1588_pkt_count_sel[0]) >> 3) & 0x1)
#define PLP_P1588_PKT_COUNT_SELr_RX_P1588_PKT_SELf_SET(r,f) (r).p1588_pkt_count_sel[0]=(((r).p1588_pkt_count_sel[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3)) | (1 << (16 + 3))
#define PLP_P1588_PKT_COUNT_SELr_RX_P1588_MSG_SELf_GET(r) (((r).p1588_pkt_count_sel[0]) & 0x7)
#define PLP_P1588_PKT_COUNT_SELr_RX_P1588_MSG_SELf_SET(r,f) (r).p1588_pkt_count_sel[0]=(((r).p1588_pkt_count_sel[0] & ~((uint32_t)0x7)) | (((uint32_t)f) & 0x7)) | (0x7 << 16)

#define PLP_P1588_PKT_COUNT_SELr_TX_P1588_MSG_INDV_SELf_GET(r) ((((r).p1588_pkt_count_sel[0]) >> 6) & 0x1)
#define PLP_P1588_PKT_COUNT_SELr_TX_P1588_MSG_INDV_SELf_SET(r,f) (r).p1588_pkt_count_sel[0]=(((r).p1588_pkt_count_sel[0] & ~((uint32_t)0x1 << 6)) | ((((uint32_t)f) & 0x1) << 6)) | (7 << (16 + 6))
#define PLP_P1588_PKT_COUNT_SELr_RX_P1588_MSG_INDV_SELf_GET(r) ((((r).p1588_pkt_count_sel[0]) >> 2) & 0x1)
#define PLP_P1588_PKT_COUNT_SELr_RX_P1588_MSG_INDV_SELf_SET(r,f) (r).p1588_pkt_count_sel[0]=(((r).p1588_pkt_count_sel[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2)) | (7 << (16 + 2))
#define PLP_P1588_PKT_COUNT_SELr_TX_P1588_MSG_TYPE_SELf_GET(r) ((((r).p1588_pkt_count_sel[0]) >> 4) & 0x3)
#define PLP_P1588_PKT_COUNT_SELr_TX_P1588_MSG_TYPE_SELf_SET(r,f) (r).p1588_pkt_count_sel[0]=(((r).p1588_pkt_count_sel[0] & ~((uint32_t)0x3 << 4)) | ((((uint32_t)f) & 0x3) << 4)) | (7 << (16 + 4))
#define PLP_P1588_PKT_COUNT_SELr_RX_P1588_MSG_TYPE_SELf_GET(r) (((r).p1588_pkt_count_sel[0]) & 0x3)
#define PLP_P1588_PKT_COUNT_SELr_RX_P1588_MSG_TYPE_SELf_SET(r,f) (r).p1588_pkt_count_sel[0]=(((r).p1588_pkt_count_sel[0] & ~((uint32_t)0x3)) | (((uint32_t)f) & 0x3)) | (0x3 << 16)

/*
 * These macros can be used to access P1588_PKT_COUNT_SEL.
 *
 */
#define PLP_READ_P1588_PKT_COUNT_SELr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PKT_COUNT_SELr,(_r._p1588_pkt_count_sel))
#define PLP_WRITE_P1588_PKT_COUNT_SELr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PKT_COUNT_SELr,(_r._p1588_pkt_count_sel))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PKT_COUNT_SELr PLP_P1588_PKT_COUNT_SELr
#define P1588_PKT_COUNT_SELr_SIZE PLP_P1588_PKT_COUNT_SELr_SIZE
typedef PLP_P1588_PKT_COUNT_SELr_t P1588_PKT_COUNT_SELr_t;
#define P1588_PKT_COUNT_SELr_CLR PLP_P1588_PKT_COUNT_SELr_CLR
#define P1588_PKT_COUNT_SELr_SET PLP_P1588_PKT_COUNT_SELr_SET
#define P1588_PKT_COUNT_SELr_GET PLP_P1588_PKT_COUNT_SELr_GET
#define P1588_PKT_COUNT_SELr_TX_CRC_COUNT_ENf_GET PLP_P1588_PKT_COUNT_SELr_TX_CRC_COUNT_ENf_GET
#define P1588_PKT_COUNT_SELr_TX_CRC_COUNT_ENf_SET PLP_P1588_PKT_COUNT_SELr_TX_CRC_COUNT_ENf_SET
#define P1588_PKT_COUNT_SELr_RX_CRC_COUNT_ENf_GET PLP_P1588_PKT_COUNT_SELr_RX_CRC_COUNT_ENf_GET
#define P1588_PKT_COUNT_SELr_RX_CRC_COUNT_ENf_SET PLP_P1588_PKT_COUNT_SELr_RX_CRC_COUNT_ENf_SET
#define P1588_PKT_COUNT_SELr_TX_P1588_PKT_SELf_GET PLP_P1588_PKT_COUNT_SELr_TX_P1588_PKT_SELf_GET
#define P1588_PKT_COUNT_SELr_TX_P1588_PKT_SELf_SET PLP_P1588_PKT_COUNT_SELr_TX_P1588_PKT_SELf_SET
#define P1588_PKT_COUNT_SELr_TX_P1588_MSG_SELf_GET PLP_P1588_PKT_COUNT_SELr_TX_P1588_MSG_SELf_GET
#define P1588_PKT_COUNT_SELr_TX_P1588_MSG_SELf_SET PLP_P1588_PKT_COUNT_SELr_TX_P1588_MSG_SELf_SET
#define P1588_PKT_COUNT_SELr_RX_P1588_PKT_SELf_GET PLP_P1588_PKT_COUNT_SELr_RX_P1588_PKT_SELf_GET
#define P1588_PKT_COUNT_SELr_RX_P1588_PKT_SELf_SET PLP_P1588_PKT_COUNT_SELr_RX_P1588_PKT_SELf_SET
#define P1588_PKT_COUNT_SELr_RX_P1588_MSG_SELf_GET PLP_P1588_PKT_COUNT_SELr_RX_P1588_MSG_SELf_GET
#define P1588_PKT_COUNT_SELr_RX_P1588_MSG_SELf_SET PLP_P1588_PKT_COUNT_SELr_RX_P1588_MSG_SELf_SET

#define P1588_PKT_COUNT_SELr_TX_P1588_MSG_INDV_SELf_GET PLP_P1588_PKT_COUNT_SELr_TX_P1588_MSG_INDV_SELf_GET
#define P1588_PKT_COUNT_SELr_TX_P1588_MSG_INDV_SELf_SET PLP_P1588_PKT_COUNT_SELr_TX_P1588_MSG_INDV_SELf_SET
#define P1588_PKT_COUNT_SELr_RX_P1588_MSG_INDV_SELf_GET PLP_P1588_PKT_COUNT_SELr_RX_P1588_MSG_INDV_SELf_GET
#define P1588_PKT_COUNT_SELr_RX_P1588_MSG_INDV_SELf_SET PLP_P1588_PKT_COUNT_SELr_RX_P1588_MSG_INDV_SELf_SET
#define P1588_PKT_COUNT_SELr_TX_P1588_MSG_TYPE_SELf_GET PLP_P1588_PKT_COUNT_SELr_TX_P1588_MSG_TYPE_SELf_GET
#define P1588_PKT_COUNT_SELr_TX_P1588_MSG_TYPE_SELf_SET PLP_P1588_PKT_COUNT_SELr_TX_P1588_MSG_TYPE_SELf_SET
#define P1588_PKT_COUNT_SELr_RX_P1588_MSG_TYPE_SELf_GET PLP_P1588_PKT_COUNT_SELr_RX_P1588_MSG_TYPE_SELf_GET
#define P1588_PKT_COUNT_SELr_RX_P1588_MSG_TYPE_SELf_SET PLP_P1588_PKT_COUNT_SELr_RX_P1588_MSG_TYPE_SELf_SET

#define P1588_PKT_COUNT_SEL_MSG_TYPE_PDELAYRESP     0x11
#define P1588_PKT_COUNT_SEL_MSG_TYPE_PDELAYREQ      0x10
#define P1588_PKT_COUNT_SEL_MSG_TYPE_DELAYREQ       0x01
#define P1588_PKT_COUNT_SEL_MSG_TYPE_SYNC           0x00
#define P1588_PKT_COUNT_SEL_MSG_TYPE_ALLPTP         0x00
#define P1588_PKT_COUNT_SEL_MSG_TYPE_SINGLE         0x01

#define READ_P1588_PKT_COUNT_SELr PLP_READ_P1588_PKT_COUNT_SELr
#define WRITE_P1588_PKT_COUNT_SELr PLP_WRITE_P1588_PKT_COUNT_SELr
#define MODIFY_P1588_PKT_COUNT_SELr PLP_MODIFY_P1588_PKT_COUNT_SELr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PKT_COUNT_SELr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_CONTROL
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7051
 * DESC:     MPLS CONTROL Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_RX_RSVD_EN  MPLS Ingress enable, reserved for now
 *     MPLS_RX_CONTROL_WORD_EN MPLS Control word label enable for ingressWhen this bit is enabled, the parser expects to see a CW labelfollowing a PW label
 *     MPLS_RX_ENTROPY_EN MPLS Entropy label enable for ingressWhen this bit is enabled, the parser expects to see an Entropy labelfollowing a PW label
 *     MPLS_RX_EN       MPLS parsing enabled for ingress
 *     MPLS_TX_SPECIAL_LABEL_EN Egress MPLS Special label enable, refers to Tx Mode 2 in the MPLS specWhen this bit is enabled, the parser expects to see a Special labelas the first label on egress
 *     MPLS_TX_CONTROL_WORD_EN MPLS Control word label enable for egressWhen this bit is enabled, the parser expects to see a CW labelfollowing a PW label
 *     MPLS_TX_ENTROPY_EN MPLS Entropy label enable for egressWhen this bit is enabled, the parser expects to see an Entropy labelfollowing a PW label
 *     MPLS_TX_EN       MPLS parsing enabled for egress
 *     MPLS_RX_SPARE    1 - Count CRC Error packets0 - Count EOP when dual clock mode is enabled
 *     MPLS_TX_SPARE    1 - Count CRC Error packets0 - Count EOP when dual clock mode is enabled
 *     MPLS_RX_8848_DIS 1 - Disable Ethertype of 0x8848 for MPLS packets0 - Enable Ethertype of 0x8848 for MPLS packets
 *     MPLS_TX_8848_DIS 1 - Disable Ethertype of 0x8848 for MPLS packets0 - Enable Ethertype of 0x8848 for MPLS packets
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_CONTROLr    0x00000051

#define PLP_P1588_MPLS_CONTROLr_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_CONTROL.
 *
 */
typedef union PLP_P1588_MPLS_CONTROLr_s {
	uint32_t v[1];
	uint32_t p1588_mpls_control[1];
	uint32_t _p1588_mpls_control;
} PLP_P1588_MPLS_CONTROLr_t;

#define PLP_P1588_MPLS_CONTROLr_CLR(r) (r).p1588_mpls_control[0] = 0
#define PLP_P1588_MPLS_CONTROLr_SET(r,d) (r).p1588_mpls_control[0] = d
#define PLP_P1588_MPLS_CONTROLr_GET(r) (r).p1588_mpls_control[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_CONTROLr_MPLS_TX_8848_DISf_GET(r) ((((r).p1588_mpls_control[0]) >> 11) & 0x1)
#define PLP_P1588_MPLS_CONTROLr_MPLS_TX_8848_DISf_SET(r,f) (r).p1588_mpls_control[0]=(((r).p1588_mpls_control[0] & ~((uint32_t)0x1 << 11)) | ((((uint32_t)f) & 0x1) << 11)) | (1 << (16 + 11))
#define PLP_P1588_MPLS_CONTROLr_MPLS_RX_8848_DISf_GET(r) ((((r).p1588_mpls_control[0]) >> 10) & 0x1)
#define PLP_P1588_MPLS_CONTROLr_MPLS_RX_8848_DISf_SET(r,f) (r).p1588_mpls_control[0]=(((r).p1588_mpls_control[0] & ~((uint32_t)0x1 << 10)) | ((((uint32_t)f) & 0x1) << 10)) | (1 << (16 + 10))
#define PLP_P1588_MPLS_CONTROLr_MPLS_TX_SPAREf_GET(r) ((((r).p1588_mpls_control[0]) >> 9) & 0x1)
#define PLP_P1588_MPLS_CONTROLr_MPLS_TX_SPAREf_SET(r,f) (r).p1588_mpls_control[0]=(((r).p1588_mpls_control[0] & ~((uint32_t)0x1 << 9)) | ((((uint32_t)f) & 0x1) << 9)) | (1 << (16 + 9))
#define PLP_P1588_MPLS_CONTROLr_MPLS_RX_SPAREf_GET(r) ((((r).p1588_mpls_control[0]) >> 8) & 0x1)
#define PLP_P1588_MPLS_CONTROLr_MPLS_RX_SPAREf_SET(r,f) (r).p1588_mpls_control[0]=(((r).p1588_mpls_control[0] & ~((uint32_t)0x1 << 8)) | ((((uint32_t)f) & 0x1) << 8)) | (1 << (16 + 8))
#define PLP_P1588_MPLS_CONTROLr_MPLS_TX_ENf_GET(r) ((((r).p1588_mpls_control[0]) >> 7) & 0x1)
#define PLP_P1588_MPLS_CONTROLr_MPLS_TX_ENf_SET(r,f) (r).p1588_mpls_control[0]=(((r).p1588_mpls_control[0] & ~((uint32_t)0x1 << 7)) | ((((uint32_t)f) & 0x1) << 7)) | (1 << (16 + 7))
#define PLP_P1588_MPLS_CONTROLr_MPLS_TX_ENTROPY_ENf_GET(r) ((((r).p1588_mpls_control[0]) >> 6) & 0x1)
#define PLP_P1588_MPLS_CONTROLr_MPLS_TX_ENTROPY_ENf_SET(r,f) (r).p1588_mpls_control[0]=(((r).p1588_mpls_control[0] & ~((uint32_t)0x1 << 6)) | ((((uint32_t)f) & 0x1) << 6)) | (1 << (16 + 6))
#define PLP_P1588_MPLS_CONTROLr_MPLS_TX_CONTROL_WORD_ENf_GET(r) ((((r).p1588_mpls_control[0]) >> 5) & 0x1)
#define PLP_P1588_MPLS_CONTROLr_MPLS_TX_CONTROL_WORD_ENf_SET(r,f) (r).p1588_mpls_control[0]=(((r).p1588_mpls_control[0] & ~((uint32_t)0x1 << 5)) | ((((uint32_t)f) & 0x1) << 5)) | (1 << (16 + 5))
#define PLP_P1588_MPLS_CONTROLr_MPLS_TX_SPECIAL_LABEL_ENf_GET(r) ((((r).p1588_mpls_control[0]) >> 4) & 0x1)
#define PLP_P1588_MPLS_CONTROLr_MPLS_TX_SPECIAL_LABEL_ENf_SET(r,f) (r).p1588_mpls_control[0]=(((r).p1588_mpls_control[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4)) | (1 << (16 + 4))
#define PLP_P1588_MPLS_CONTROLr_MPLS_RX_ENf_GET(r) ((((r).p1588_mpls_control[0]) >> 3) & 0x1)
#define PLP_P1588_MPLS_CONTROLr_MPLS_RX_ENf_SET(r,f) (r).p1588_mpls_control[0]=(((r).p1588_mpls_control[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3)) | (1 << (16 + 3))
#define PLP_P1588_MPLS_CONTROLr_MPLS_RX_ENTROPY_ENf_GET(r) ((((r).p1588_mpls_control[0]) >> 2) & 0x1)
#define PLP_P1588_MPLS_CONTROLr_MPLS_RX_ENTROPY_ENf_SET(r,f) (r).p1588_mpls_control[0]=(((r).p1588_mpls_control[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2)) | (1 << (16 + 2))
#define PLP_P1588_MPLS_CONTROLr_MPLS_RX_CONTROL_WORD_ENf_GET(r) ((((r).p1588_mpls_control[0]) >> 1) & 0x1)
#define PLP_P1588_MPLS_CONTROLr_MPLS_RX_CONTROL_WORD_ENf_SET(r,f) (r).p1588_mpls_control[0]=(((r).p1588_mpls_control[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))
#define PLP_P1588_MPLS_CONTROLr_MPLS_RX_RSVD_ENf_GET(r) (((r).p1588_mpls_control[0]) & 0x1)
#define PLP_P1588_MPLS_CONTROLr_MPLS_RX_RSVD_ENf_SET(r,f) (r).p1588_mpls_control[0]=(((r).p1588_mpls_control[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1)) | (0x1 << 16)

/*
 * These macros can be used to access P1588_MPLS_CONTROL.
 *
 */
#define PLP_READ_P1588_MPLS_CONTROLr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_CONTROLr,(_r._p1588_mpls_control))
#define PLP_WRITE_P1588_MPLS_CONTROLr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_CONTROLr,(_r._p1588_mpls_control))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_CONTROLr PLP_P1588_MPLS_CONTROLr
#define P1588_MPLS_CONTROLr_SIZE PLP_P1588_MPLS_CONTROLr_SIZE
typedef PLP_P1588_MPLS_CONTROLr_t P1588_MPLS_CONTROLr_t;
#define P1588_MPLS_CONTROLr_CLR PLP_P1588_MPLS_CONTROLr_CLR
#define P1588_MPLS_CONTROLr_SET PLP_P1588_MPLS_CONTROLr_SET
#define P1588_MPLS_CONTROLr_GET PLP_P1588_MPLS_CONTROLr_GET
#define P1588_MPLS_CONTROLr_MPLS_TX_8848_DISf_GET PLP_P1588_MPLS_CONTROLr_MPLS_TX_8848_DISf_GET
#define P1588_MPLS_CONTROLr_MPLS_TX_8848_DISf_SET PLP_P1588_MPLS_CONTROLr_MPLS_TX_8848_DISf_SET
#define P1588_MPLS_CONTROLr_MPLS_RX_8848_DISf_GET PLP_P1588_MPLS_CONTROLr_MPLS_RX_8848_DISf_GET
#define P1588_MPLS_CONTROLr_MPLS_RX_8848_DISf_SET PLP_P1588_MPLS_CONTROLr_MPLS_RX_8848_DISf_SET
#define P1588_MPLS_CONTROLr_MPLS_TX_SPAREf_GET PLP_P1588_MPLS_CONTROLr_MPLS_TX_SPAREf_GET
#define P1588_MPLS_CONTROLr_MPLS_TX_SPAREf_SET PLP_P1588_MPLS_CONTROLr_MPLS_TX_SPAREf_SET
#define P1588_MPLS_CONTROLr_MPLS_RX_SPAREf_GET PLP_P1588_MPLS_CONTROLr_MPLS_RX_SPAREf_GET
#define P1588_MPLS_CONTROLr_MPLS_RX_SPAREf_SET PLP_P1588_MPLS_CONTROLr_MPLS_RX_SPAREf_SET
#define P1588_MPLS_CONTROLr_MPLS_TX_ENf_GET PLP_P1588_MPLS_CONTROLr_MPLS_TX_ENf_GET
#define P1588_MPLS_CONTROLr_MPLS_TX_ENf_SET PLP_P1588_MPLS_CONTROLr_MPLS_TX_ENf_SET
#define P1588_MPLS_CONTROLr_MPLS_TX_ENTROPY_ENf_GET PLP_P1588_MPLS_CONTROLr_MPLS_TX_ENTROPY_ENf_GET
#define P1588_MPLS_CONTROLr_MPLS_TX_ENTROPY_ENf_SET PLP_P1588_MPLS_CONTROLr_MPLS_TX_ENTROPY_ENf_SET
#define P1588_MPLS_CONTROLr_MPLS_TX_CONTROL_WORD_ENf_GET PLP_P1588_MPLS_CONTROLr_MPLS_TX_CONTROL_WORD_ENf_GET
#define P1588_MPLS_CONTROLr_MPLS_TX_CONTROL_WORD_ENf_SET PLP_P1588_MPLS_CONTROLr_MPLS_TX_CONTROL_WORD_ENf_SET
#define P1588_MPLS_CONTROLr_MPLS_TX_SPECIAL_LABEL_ENf_GET PLP_P1588_MPLS_CONTROLr_MPLS_TX_SPECIAL_LABEL_ENf_GET
#define P1588_MPLS_CONTROLr_MPLS_TX_SPECIAL_LABEL_ENf_SET PLP_P1588_MPLS_CONTROLr_MPLS_TX_SPECIAL_LABEL_ENf_SET
#define P1588_MPLS_CONTROLr_MPLS_RX_ENf_GET PLP_P1588_MPLS_CONTROLr_MPLS_RX_ENf_GET
#define P1588_MPLS_CONTROLr_MPLS_RX_ENf_SET PLP_P1588_MPLS_CONTROLr_MPLS_RX_ENf_SET
#define P1588_MPLS_CONTROLr_MPLS_RX_ENTROPY_ENf_GET PLP_P1588_MPLS_CONTROLr_MPLS_RX_ENTROPY_ENf_GET
#define P1588_MPLS_CONTROLr_MPLS_RX_ENTROPY_ENf_SET PLP_P1588_MPLS_CONTROLr_MPLS_RX_ENTROPY_ENf_SET
#define P1588_MPLS_CONTROLr_MPLS_RX_CONTROL_WORD_ENf_GET PLP_P1588_MPLS_CONTROLr_MPLS_RX_CONTROL_WORD_ENf_GET
#define P1588_MPLS_CONTROLr_MPLS_RX_CONTROL_WORD_ENf_SET PLP_P1588_MPLS_CONTROLr_MPLS_RX_CONTROL_WORD_ENf_SET
#define P1588_MPLS_CONTROLr_MPLS_RX_RSVD_ENf_GET PLP_P1588_MPLS_CONTROLr_MPLS_RX_RSVD_ENf_GET
#define P1588_MPLS_CONTROLr_MPLS_RX_RSVD_ENf_SET PLP_P1588_MPLS_CONTROLr_MPLS_RX_RSVD_ENf_SET
#define READ_P1588_MPLS_CONTROLr PLP_READ_P1588_MPLS_CONTROLr
#define WRITE_P1588_MPLS_CONTROLr PLP_WRITE_P1588_MPLS_CONTROLr
#define MODIFY_P1588_MPLS_CONTROLr PLP_MODIFY_P1588_MPLS_CONTROLr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_CONTROLr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_TX_SPECIAL_LABEL_1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7052
 * DESC:     MPLS TX SPECIAL LABEL 1 Register
 * RESETVAL: 0x3 (3)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_TX_SPECIAL_LABEL_VALUE_15_0 MPLS Egress Special label valueIn 1G code, connect to {mpls_spare6[3:0],mpls_spare5}
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_TX_SPECIAL_LABEL_1r    0x00000052

#define PLP_P1588_MPLS_TX_SPECIAL_LABEL_1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_TX_SPECIAL_LABEL_1.
 *
 */
typedef union PLP_P1588_MPLS_TX_SPECIAL_LABEL_1r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_tx_special_label_1[1];
	uint32_t _p1588_mpls_tx_special_label_1;
} PLP_P1588_MPLS_TX_SPECIAL_LABEL_1r_t;

#define PLP_P1588_MPLS_TX_SPECIAL_LABEL_1r_CLR(r) (r).p1588_mpls_tx_special_label_1[0] = 0
#define PLP_P1588_MPLS_TX_SPECIAL_LABEL_1r_SET(r,d) (r).p1588_mpls_tx_special_label_1[0] = d
#define PLP_P1588_MPLS_TX_SPECIAL_LABEL_1r_GET(r) (r).p1588_mpls_tx_special_label_1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_TX_SPECIAL_LABEL_1r_MPLS_TX_SPECIAL_LABEL_VALUE_15_0f_GET(r) (((r).p1588_mpls_tx_special_label_1[0]) & 0xffff)
#define PLP_P1588_MPLS_TX_SPECIAL_LABEL_1r_MPLS_TX_SPECIAL_LABEL_VALUE_15_0f_SET(r,f) (r).p1588_mpls_tx_special_label_1[0]=(((r).p1588_mpls_tx_special_label_1[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_MPLS_TX_SPECIAL_LABEL_1.
 *
 */
#define PLP_READ_P1588_MPLS_TX_SPECIAL_LABEL_1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_TX_SPECIAL_LABEL_1r,(_r._p1588_mpls_tx_special_label_1))
#define PLP_WRITE_P1588_MPLS_TX_SPECIAL_LABEL_1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_TX_SPECIAL_LABEL_1r,(_r._p1588_mpls_tx_special_label_1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_TX_SPECIAL_LABEL_1r PLP_P1588_MPLS_TX_SPECIAL_LABEL_1r
#define P1588_MPLS_TX_SPECIAL_LABEL_1r_SIZE PLP_P1588_MPLS_TX_SPECIAL_LABEL_1r_SIZE
typedef PLP_P1588_MPLS_TX_SPECIAL_LABEL_1r_t P1588_MPLS_TX_SPECIAL_LABEL_1r_t;
#define P1588_MPLS_TX_SPECIAL_LABEL_1r_CLR PLP_P1588_MPLS_TX_SPECIAL_LABEL_1r_CLR
#define P1588_MPLS_TX_SPECIAL_LABEL_1r_SET PLP_P1588_MPLS_TX_SPECIAL_LABEL_1r_SET
#define P1588_MPLS_TX_SPECIAL_LABEL_1r_GET PLP_P1588_MPLS_TX_SPECIAL_LABEL_1r_GET
#define P1588_MPLS_TX_SPECIAL_LABEL_1r_VAL_LO_GET  PLP_P1588_MPLS_TX_SPECIAL_LABEL_1r_MPLS_TX_SPECIAL_LABEL_VALUE_15_0f_GET
#define P1588_MPLS_TX_SPECIAL_LABEL_1r_MPLS_TX_SPECIAL_LABEL_VALUE_15_0f_GET PLP_P1588_MPLS_TX_SPECIAL_LABEL_1r_MPLS_TX_SPECIAL_LABEL_VALUE_15_0f_GET
#define P1588_MPLS_TX_SPECIAL_LABEL_1r_MPLS_TX_SPECIAL_LABEL_VALUE_15_0f_SET PLP_P1588_MPLS_TX_SPECIAL_LABEL_1r_MPLS_TX_SPECIAL_LABEL_VALUE_15_0f_SET
#define READ_P1588_MPLS_TX_SPECIAL_LABEL_1r PLP_READ_P1588_MPLS_TX_SPECIAL_LABEL_1r
#define WRITE_P1588_MPLS_TX_SPECIAL_LABEL_1r PLP_WRITE_P1588_MPLS_TX_SPECIAL_LABEL_1r
#define MODIFY_P1588_MPLS_TX_SPECIAL_LABEL_1r PLP_MODIFY_P1588_MPLS_TX_SPECIAL_LABEL_1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_TX_SPECIAL_LABEL_1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_TX_SPECIAL_LABEL_2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7053
 * DESC:     MPLS TX SPECIAL LABEL 2 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_TX_SPECIAL_LABEL_VALUE_19_16 MPLS Egress Special label valueIn 1G code, connect to {mpls_spare6[3:0],mpls_spare5}
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_TX_SPECIAL_LABEL_2r    0x00000053

#define PLP_P1588_MPLS_TX_SPECIAL_LABEL_2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_TX_SPECIAL_LABEL_2.
 *
 */
typedef union PLP_P1588_MPLS_TX_SPECIAL_LABEL_2r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_tx_special_label_2[1];
	uint32_t _p1588_mpls_tx_special_label_2;
} PLP_P1588_MPLS_TX_SPECIAL_LABEL_2r_t;

#define PLP_P1588_MPLS_TX_SPECIAL_LABEL_2r_CLR(r) (r).p1588_mpls_tx_special_label_2[0] = 0
#define PLP_P1588_MPLS_TX_SPECIAL_LABEL_2r_SET(r,d) (r).p1588_mpls_tx_special_label_2[0] = d
#define PLP_P1588_MPLS_TX_SPECIAL_LABEL_2r_GET(r) (r).p1588_mpls_tx_special_label_2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_TX_SPECIAL_LABEL_2r_MPLS_TX_SPECIAL_LABEL_VALUE_19_16f_GET(r) (((r).p1588_mpls_tx_special_label_2[0]) & 0xf)
#define PLP_P1588_MPLS_TX_SPECIAL_LABEL_2r_MPLS_TX_SPECIAL_LABEL_VALUE_19_16f_SET(r,f) (r).p1588_mpls_tx_special_label_2[0]=(((r).p1588_mpls_tx_special_label_2[0] & ~((uint32_t)0xf)) | (((uint32_t)f) & 0xf)) | (0xf << 16)

/*
 * These macros can be used to access P1588_MPLS_TX_SPECIAL_LABEL_2.
 *
 */
#define PLP_READ_P1588_MPLS_TX_SPECIAL_LABEL_2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_TX_SPECIAL_LABEL_2r,(_r._p1588_mpls_tx_special_label_2))
#define PLP_WRITE_P1588_MPLS_TX_SPECIAL_LABEL_2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_TX_SPECIAL_LABEL_2r,(_r._p1588_mpls_tx_special_label_2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_TX_SPECIAL_LABEL_2r PLP_P1588_MPLS_TX_SPECIAL_LABEL_2r
#define P1588_MPLS_TX_SPECIAL_LABEL_2r_SIZE PLP_P1588_MPLS_TX_SPECIAL_LABEL_2r_SIZE
typedef PLP_P1588_MPLS_TX_SPECIAL_LABEL_2r_t P1588_MPLS_TX_SPECIAL_LABEL_2r_t;
#define P1588_MPLS_TX_SPECIAL_LABEL_2r_CLR PLP_P1588_MPLS_TX_SPECIAL_LABEL_2r_CLR
#define P1588_MPLS_TX_SPECIAL_LABEL_2r_SET PLP_P1588_MPLS_TX_SPECIAL_LABEL_2r_SET
#define P1588_MPLS_TX_SPECIAL_LABEL_2r_GET PLP_P1588_MPLS_TX_SPECIAL_LABEL_2r_GET
#define P1588_MPLS_TX_SPECIAL_LABEL_2r_VAL_HI_GET  PLP_P1588_MPLS_TX_SPECIAL_LABEL_2r_MPLS_TX_SPECIAL_LABEL_VALUE_19_16f_GET
#define P1588_MPLS_TX_SPECIAL_LABEL_2r_MPLS_TX_SPECIAL_LABEL_VALUE_19_16f_GET PLP_P1588_MPLS_TX_SPECIAL_LABEL_2r_MPLS_TX_SPECIAL_LABEL_VALUE_19_16f_GET
#define P1588_MPLS_TX_SPECIAL_LABEL_2r_MPLS_TX_SPECIAL_LABEL_VALUE_19_16f_SET PLP_P1588_MPLS_TX_SPECIAL_LABEL_2r_MPLS_TX_SPECIAL_LABEL_VALUE_19_16f_SET
#define READ_P1588_MPLS_TX_SPECIAL_LABEL_2r PLP_READ_P1588_MPLS_TX_SPECIAL_LABEL_2r
#define WRITE_P1588_MPLS_TX_SPECIAL_LABEL_2r PLP_WRITE_P1588_MPLS_TX_SPECIAL_LABEL_2r
#define MODIFY_P1588_MPLS_TX_SPECIAL_LABEL_2r PLP_MODIFY_P1588_MPLS_TX_SPECIAL_LABEL_2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_TX_SPECIAL_LABEL_2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_VALUE_1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7054
 * DESC:     MPLS LABEL VALUE 1 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL1_VALUE_15_0 MPLS label 1 valueTx label match = (label dir == EGRESS)  ? (extracted label[19:0] & label_mask[19:0]) == label_value[19:0]) : 0Rx label match = (label dir == INGRESS) ? (extracted label[19:0] & label_mask[19:0]) == label_value[19:0]) : 0
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_VALUE_1r    0x00000054

#define PLP_P1588_MPLS_LABEL_VALUE_1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_VALUE_1.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_VALUE_1r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_value_1[1];
	uint32_t _p1588_mpls_label_value_1;
} PLP_P1588_MPLS_LABEL_VALUE_1r_t;

#define PLP_P1588_MPLS_LABEL_VALUE_1r_CLR(r) (r).p1588_mpls_label_value_1[0] = 0
#define PLP_P1588_MPLS_LABEL_VALUE_1r_SET(r,d) (r).p1588_mpls_label_value_1[0] = d
#define PLP_P1588_MPLS_LABEL_VALUE_1r_GET(r) (r).p1588_mpls_label_value_1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_VALUE_1r_MPLS_LABEL1_VALUE_15_0f_GET(r) (((r).p1588_mpls_label_value_1[0]) & 0xffff)
#define PLP_P1588_MPLS_LABEL_VALUE_1r_MPLS_LABEL1_VALUE_15_0f_SET(r,f) (r).p1588_mpls_label_value_1[0]=(((r).p1588_mpls_label_value_1[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_VALUE_1.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_VALUE_1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_VALUE_1r,(_r._p1588_mpls_label_value_1))
#define PLP_WRITE_P1588_MPLS_LABEL_VALUE_1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_VALUE_1r,(_r._p1588_mpls_label_value_1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_VALUE_1r PLP_P1588_MPLS_LABEL_VALUE_1r
#define P1588_MPLS_LABEL_VALUE_1r_SIZE PLP_P1588_MPLS_LABEL_VALUE_1r_SIZE
typedef PLP_P1588_MPLS_LABEL_VALUE_1r_t P1588_MPLS_LABEL_VALUE_1r_t;
#define P1588_MPLS_LABEL_VALUE_1r_CLR PLP_P1588_MPLS_LABEL_VALUE_1r_CLR
#define P1588_MPLS_LABEL_VALUE_1r_SET PLP_P1588_MPLS_LABEL_VALUE_1r_SET
#define P1588_MPLS_LABEL_VALUE_1r_GET PLP_P1588_MPLS_LABEL_VALUE_1r_GET
#define P1588_MPLS_LABEL_VALUE_1r_MPLS_LABEL1_VALUE_15_0f_GET PLP_P1588_MPLS_LABEL_VALUE_1r_MPLS_LABEL1_VALUE_15_0f_GET
#define P1588_MPLS_LABEL_VALUE_1r_MPLS_LABEL1_VALUE_15_0f_SET PLP_P1588_MPLS_LABEL_VALUE_1r_MPLS_LABEL1_VALUE_15_0f_SET
#define READ_P1588_MPLS_LABEL_VALUE_1r PLP_READ_P1588_MPLS_LABEL_VALUE_1r
#define WRITE_P1588_MPLS_LABEL_VALUE_1r PLP_WRITE_P1588_MPLS_LABEL_VALUE_1r
#define MODIFY_P1588_MPLS_LABEL_VALUE_1r PLP_MODIFY_P1588_MPLS_LABEL_VALUE_1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_VALUE_1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_VALUE_2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7055
 * DESC:     MPLS LABEL VALUE 2 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL2_VALUE_15_0 MPLS label 2 valueTx label match = (label dir == EGRESS)  ? (extracted label[19:0] & label_mask[19:0]) == label_value[19:0]) : 0Rx label match = (label dir == INGRESS) ? (extracted label[19:0] & label_mask[19:0]) == label_value[19:0]) : 0
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_VALUE_2r    0x00000055

#define PLP_P1588_MPLS_LABEL_VALUE_2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_VALUE_2.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_VALUE_2r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_value_2[1];
	uint32_t _p1588_mpls_label_value_2;
} PLP_P1588_MPLS_LABEL_VALUE_2r_t;

#define PLP_P1588_MPLS_LABEL_VALUE_2r_CLR(r) (r).p1588_mpls_label_value_2[0] = 0
#define PLP_P1588_MPLS_LABEL_VALUE_2r_SET(r,d) (r).p1588_mpls_label_value_2[0] = d
#define PLP_P1588_MPLS_LABEL_VALUE_2r_GET(r) (r).p1588_mpls_label_value_2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_VALUE_2r_MPLS_LABEL2_VALUE_15_0f_GET(r) (((r).p1588_mpls_label_value_2[0]) & 0xffff)
#define PLP_P1588_MPLS_LABEL_VALUE_2r_MPLS_LABEL2_VALUE_15_0f_SET(r,f) (r).p1588_mpls_label_value_2[0]=(((r).p1588_mpls_label_value_2[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_VALUE_2.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_VALUE_2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_VALUE_2r,(_r._p1588_mpls_label_value_2))
#define PLP_WRITE_P1588_MPLS_LABEL_VALUE_2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_VALUE_2r,(_r._p1588_mpls_label_value_2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_VALUE_2r PLP_P1588_MPLS_LABEL_VALUE_2r
#define P1588_MPLS_LABEL_VALUE_2r_SIZE PLP_P1588_MPLS_LABEL_VALUE_2r_SIZE
typedef PLP_P1588_MPLS_LABEL_VALUE_2r_t P1588_MPLS_LABEL_VALUE_2r_t;
#define P1588_MPLS_LABEL_VALUE_2r_CLR PLP_P1588_MPLS_LABEL_VALUE_2r_CLR
#define P1588_MPLS_LABEL_VALUE_2r_SET PLP_P1588_MPLS_LABEL_VALUE_2r_SET
#define P1588_MPLS_LABEL_VALUE_2r_GET PLP_P1588_MPLS_LABEL_VALUE_2r_GET
#define P1588_MPLS_LABEL_VALUE_2r_MPLS_LABEL2_VALUE_15_0f_GET PLP_P1588_MPLS_LABEL_VALUE_2r_MPLS_LABEL2_VALUE_15_0f_GET
#define P1588_MPLS_LABEL_VALUE_2r_MPLS_LABEL2_VALUE_15_0f_SET PLP_P1588_MPLS_LABEL_VALUE_2r_MPLS_LABEL2_VALUE_15_0f_SET
#define READ_P1588_MPLS_LABEL_VALUE_2r PLP_READ_P1588_MPLS_LABEL_VALUE_2r
#define WRITE_P1588_MPLS_LABEL_VALUE_2r PLP_WRITE_P1588_MPLS_LABEL_VALUE_2r
#define MODIFY_P1588_MPLS_LABEL_VALUE_2r PLP_MODIFY_P1588_MPLS_LABEL_VALUE_2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_VALUE_2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_VALUE_3
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7056
 * DESC:     MPLS LABEL VALUE 3 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL3_VALUE_15_0 MPLS label 3 valueTx label match = (label dir == EGRESS)  ? (extracted label[19:0] & label_mask[19:0]) == label_value[19:0]) : 0Rx label match = (label dir == INGRESS) ? (extracted label[19:0] & label_mask[19:0]) == label_value[19:0]) : 0
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_VALUE_3r    0x00000056

#define PLP_P1588_MPLS_LABEL_VALUE_3r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_VALUE_3.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_VALUE_3r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_value_3[1];
	uint32_t _p1588_mpls_label_value_3;
} PLP_P1588_MPLS_LABEL_VALUE_3r_t;

#define PLP_P1588_MPLS_LABEL_VALUE_3r_CLR(r) (r).p1588_mpls_label_value_3[0] = 0
#define PLP_P1588_MPLS_LABEL_VALUE_3r_SET(r,d) (r).p1588_mpls_label_value_3[0] = d
#define PLP_P1588_MPLS_LABEL_VALUE_3r_GET(r) (r).p1588_mpls_label_value_3[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_VALUE_3r_MPLS_LABEL3_VALUE_15_0f_GET(r) (((r).p1588_mpls_label_value_3[0]) & 0xffff)
#define PLP_P1588_MPLS_LABEL_VALUE_3r_MPLS_LABEL3_VALUE_15_0f_SET(r,f) (r).p1588_mpls_label_value_3[0]=(((r).p1588_mpls_label_value_3[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_VALUE_3.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_VALUE_3r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_VALUE_3r,(_r._p1588_mpls_label_value_3))
#define PLP_WRITE_P1588_MPLS_LABEL_VALUE_3r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_VALUE_3r,(_r._p1588_mpls_label_value_3))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_VALUE_3r PLP_P1588_MPLS_LABEL_VALUE_3r
#define P1588_MPLS_LABEL_VALUE_3r_SIZE PLP_P1588_MPLS_LABEL_VALUE_3r_SIZE
typedef PLP_P1588_MPLS_LABEL_VALUE_3r_t P1588_MPLS_LABEL_VALUE_3r_t;
#define P1588_MPLS_LABEL_VALUE_3r_CLR PLP_P1588_MPLS_LABEL_VALUE_3r_CLR
#define P1588_MPLS_LABEL_VALUE_3r_SET PLP_P1588_MPLS_LABEL_VALUE_3r_SET
#define P1588_MPLS_LABEL_VALUE_3r_GET PLP_P1588_MPLS_LABEL_VALUE_3r_GET
#define P1588_MPLS_LABEL_VALUE_3r_MPLS_LABEL3_VALUE_15_0f_GET PLP_P1588_MPLS_LABEL_VALUE_3r_MPLS_LABEL3_VALUE_15_0f_GET
#define P1588_MPLS_LABEL_VALUE_3r_MPLS_LABEL3_VALUE_15_0f_SET PLP_P1588_MPLS_LABEL_VALUE_3r_MPLS_LABEL3_VALUE_15_0f_SET
#define READ_P1588_MPLS_LABEL_VALUE_3r PLP_READ_P1588_MPLS_LABEL_VALUE_3r
#define WRITE_P1588_MPLS_LABEL_VALUE_3r PLP_WRITE_P1588_MPLS_LABEL_VALUE_3r
#define MODIFY_P1588_MPLS_LABEL_VALUE_3r PLP_MODIFY_P1588_MPLS_LABEL_VALUE_3r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_VALUE_3r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_VALUE_4
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7057
 * DESC:     MPLS LABEL VALUE 4 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL4_VALUE_15_0 MPLS label 4 valueTx label match = (label dir == EGRESS)  ? (extracted label[19:0] & label_mask[19:0]) == label_value[19:0]) : 0Rx label match = (label dir == INGRESS) ? (extracted label[19:0] & label_mask[19:0]) == label_value[19:0]) : 0
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_VALUE_4r    0x00000057

#define PLP_P1588_MPLS_LABEL_VALUE_4r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_VALUE_4.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_VALUE_4r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_value_4[1];
	uint32_t _p1588_mpls_label_value_4;
} PLP_P1588_MPLS_LABEL_VALUE_4r_t;

#define PLP_P1588_MPLS_LABEL_VALUE_4r_CLR(r) (r).p1588_mpls_label_value_4[0] = 0
#define PLP_P1588_MPLS_LABEL_VALUE_4r_SET(r,d) (r).p1588_mpls_label_value_4[0] = d
#define PLP_P1588_MPLS_LABEL_VALUE_4r_GET(r) (r).p1588_mpls_label_value_4[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_VALUE_4r_MPLS_LABEL4_VALUE_15_0f_GET(r) (((r).p1588_mpls_label_value_4[0]) & 0xffff)
#define PLP_P1588_MPLS_LABEL_VALUE_4r_MPLS_LABEL4_VALUE_15_0f_SET(r,f) (r).p1588_mpls_label_value_4[0]=(((r).p1588_mpls_label_value_4[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_VALUE_4.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_VALUE_4r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_VALUE_4r,(_r._p1588_mpls_label_value_4))
#define PLP_WRITE_P1588_MPLS_LABEL_VALUE_4r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_VALUE_4r,(_r._p1588_mpls_label_value_4))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_VALUE_4r PLP_P1588_MPLS_LABEL_VALUE_4r
#define P1588_MPLS_LABEL_VALUE_4r_SIZE PLP_P1588_MPLS_LABEL_VALUE_4r_SIZE
typedef PLP_P1588_MPLS_LABEL_VALUE_4r_t P1588_MPLS_LABEL_VALUE_4r_t;
#define P1588_MPLS_LABEL_VALUE_4r_CLR PLP_P1588_MPLS_LABEL_VALUE_4r_CLR
#define P1588_MPLS_LABEL_VALUE_4r_SET PLP_P1588_MPLS_LABEL_VALUE_4r_SET
#define P1588_MPLS_LABEL_VALUE_4r_GET PLP_P1588_MPLS_LABEL_VALUE_4r_GET
#define P1588_MPLS_LABEL_VALUE_4r_MPLS_LABEL4_VALUE_15_0f_GET PLP_P1588_MPLS_LABEL_VALUE_4r_MPLS_LABEL4_VALUE_15_0f_GET
#define P1588_MPLS_LABEL_VALUE_4r_MPLS_LABEL4_VALUE_15_0f_SET PLP_P1588_MPLS_LABEL_VALUE_4r_MPLS_LABEL4_VALUE_15_0f_SET
#define READ_P1588_MPLS_LABEL_VALUE_4r PLP_READ_P1588_MPLS_LABEL_VALUE_4r
#define WRITE_P1588_MPLS_LABEL_VALUE_4r PLP_WRITE_P1588_MPLS_LABEL_VALUE_4r
#define MODIFY_P1588_MPLS_LABEL_VALUE_4r PLP_MODIFY_P1588_MPLS_LABEL_VALUE_4r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_VALUE_4r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_VALUE_5
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7058
 * DESC:     MPLS LABEL VALUE 5 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL5_VALUE_15_0 MPLS label 5 valueTx label match = (label dir == EGRESS)  ? (extracted label[19:0] & label_mask[19:0]) == label_value[19:0]) : 0Rx label match = (label dir == INGRESS) ? (extracted label[19:0] & label_mask[19:0]) == label_value[19:0]) : 0
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_VALUE_5r    0x00000058

#define PLP_P1588_MPLS_LABEL_VALUE_5r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_VALUE_5.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_VALUE_5r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_value_5[1];
	uint32_t _p1588_mpls_label_value_5;
} PLP_P1588_MPLS_LABEL_VALUE_5r_t;

#define PLP_P1588_MPLS_LABEL_VALUE_5r_CLR(r) (r).p1588_mpls_label_value_5[0] = 0
#define PLP_P1588_MPLS_LABEL_VALUE_5r_SET(r,d) (r).p1588_mpls_label_value_5[0] = d
#define PLP_P1588_MPLS_LABEL_VALUE_5r_GET(r) (r).p1588_mpls_label_value_5[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_VALUE_5r_MPLS_LABEL5_VALUE_15_0f_GET(r) (((r).p1588_mpls_label_value_5[0]) & 0xffff)
#define PLP_P1588_MPLS_LABEL_VALUE_5r_MPLS_LABEL5_VALUE_15_0f_SET(r,f) (r).p1588_mpls_label_value_5[0]=(((r).p1588_mpls_label_value_5[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_VALUE_5.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_VALUE_5r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_VALUE_5r,(_r._p1588_mpls_label_value_5))
#define PLP_WRITE_P1588_MPLS_LABEL_VALUE_5r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_VALUE_5r,(_r._p1588_mpls_label_value_5))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_VALUE_5r PLP_P1588_MPLS_LABEL_VALUE_5r
#define P1588_MPLS_LABEL_VALUE_5r_SIZE PLP_P1588_MPLS_LABEL_VALUE_5r_SIZE
typedef PLP_P1588_MPLS_LABEL_VALUE_5r_t P1588_MPLS_LABEL_VALUE_5r_t;
#define P1588_MPLS_LABEL_VALUE_5r_CLR PLP_P1588_MPLS_LABEL_VALUE_5r_CLR
#define P1588_MPLS_LABEL_VALUE_5r_SET PLP_P1588_MPLS_LABEL_VALUE_5r_SET
#define P1588_MPLS_LABEL_VALUE_5r_GET PLP_P1588_MPLS_LABEL_VALUE_5r_GET
#define P1588_MPLS_LABEL_VALUE_5r_MPLS_LABEL5_VALUE_15_0f_GET PLP_P1588_MPLS_LABEL_VALUE_5r_MPLS_LABEL5_VALUE_15_0f_GET
#define P1588_MPLS_LABEL_VALUE_5r_MPLS_LABEL5_VALUE_15_0f_SET PLP_P1588_MPLS_LABEL_VALUE_5r_MPLS_LABEL5_VALUE_15_0f_SET
#define READ_P1588_MPLS_LABEL_VALUE_5r PLP_READ_P1588_MPLS_LABEL_VALUE_5r
#define WRITE_P1588_MPLS_LABEL_VALUE_5r PLP_WRITE_P1588_MPLS_LABEL_VALUE_5r
#define MODIFY_P1588_MPLS_LABEL_VALUE_5r PLP_MODIFY_P1588_MPLS_LABEL_VALUE_5r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_VALUE_5r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_VALUE_6
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7059
 * DESC:     MPLS LABEL VALUE 6 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL6_VALUE_15_0 MPLS label 6 valueTx label match = (label dir == EGRESS)  ? (extracted label[19:0] & label_mask[19:0]) == label_value[19:0]) : 0Rx label match = (label dir == INGRESS) ? (extracted label[19:0] & label_mask[19:0]) == label_value[19:0]) : 0
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_VALUE_6r    0x00000059

#define PLP_P1588_MPLS_LABEL_VALUE_6r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_VALUE_6.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_VALUE_6r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_value_6[1];
	uint32_t _p1588_mpls_label_value_6;
} PLP_P1588_MPLS_LABEL_VALUE_6r_t;

#define PLP_P1588_MPLS_LABEL_VALUE_6r_CLR(r) (r).p1588_mpls_label_value_6[0] = 0
#define PLP_P1588_MPLS_LABEL_VALUE_6r_SET(r,d) (r).p1588_mpls_label_value_6[0] = d
#define PLP_P1588_MPLS_LABEL_VALUE_6r_GET(r) (r).p1588_mpls_label_value_6[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_VALUE_6r_MPLS_LABEL6_VALUE_15_0f_GET(r) (((r).p1588_mpls_label_value_6[0]) & 0xffff)
#define PLP_P1588_MPLS_LABEL_VALUE_6r_MPLS_LABEL6_VALUE_15_0f_SET(r,f) (r).p1588_mpls_label_value_6[0]=(((r).p1588_mpls_label_value_6[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_VALUE_6.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_VALUE_6r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_VALUE_6r,(_r._p1588_mpls_label_value_6))
#define PLP_WRITE_P1588_MPLS_LABEL_VALUE_6r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_VALUE_6r,(_r._p1588_mpls_label_value_6))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_VALUE_6r PLP_P1588_MPLS_LABEL_VALUE_6r
#define P1588_MPLS_LABEL_VALUE_6r_SIZE PLP_P1588_MPLS_LABEL_VALUE_6r_SIZE
typedef PLP_P1588_MPLS_LABEL_VALUE_6r_t P1588_MPLS_LABEL_VALUE_6r_t;
#define P1588_MPLS_LABEL_VALUE_6r_CLR PLP_P1588_MPLS_LABEL_VALUE_6r_CLR
#define P1588_MPLS_LABEL_VALUE_6r_SET PLP_P1588_MPLS_LABEL_VALUE_6r_SET
#define P1588_MPLS_LABEL_VALUE_6r_GET PLP_P1588_MPLS_LABEL_VALUE_6r_GET
#define P1588_MPLS_LABEL_VALUE_6r_MPLS_LABEL6_VALUE_15_0f_GET PLP_P1588_MPLS_LABEL_VALUE_6r_MPLS_LABEL6_VALUE_15_0f_GET
#define P1588_MPLS_LABEL_VALUE_6r_MPLS_LABEL6_VALUE_15_0f_SET PLP_P1588_MPLS_LABEL_VALUE_6r_MPLS_LABEL6_VALUE_15_0f_SET
#define READ_P1588_MPLS_LABEL_VALUE_6r PLP_READ_P1588_MPLS_LABEL_VALUE_6r
#define WRITE_P1588_MPLS_LABEL_VALUE_6r PLP_WRITE_P1588_MPLS_LABEL_VALUE_6r
#define MODIFY_P1588_MPLS_LABEL_VALUE_6r PLP_MODIFY_P1588_MPLS_LABEL_VALUE_6r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_VALUE_6r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_VALUE_7
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x705a
 * DESC:     MPLS LABEL VALUE 7 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL7_VALUE_15_0 MPLS label 7 valueTx label match = (label dir == EGRESS)  ? (extracted label[19:0] & label_mask[19:0]) == label_value[19:0]) : 0Rx label match = (label dir == INGRESS) ? (extracted label[19:0] & label_mask[19:0]) == label_value[19:0]) : 0
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_VALUE_7r    0x0000005a

#define PLP_P1588_MPLS_LABEL_VALUE_7r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_VALUE_7.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_VALUE_7r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_value_7[1];
	uint32_t _p1588_mpls_label_value_7;
} PLP_P1588_MPLS_LABEL_VALUE_7r_t;

#define PLP_P1588_MPLS_LABEL_VALUE_7r_CLR(r) (r).p1588_mpls_label_value_7[0] = 0
#define PLP_P1588_MPLS_LABEL_VALUE_7r_SET(r,d) (r).p1588_mpls_label_value_7[0] = d
#define PLP_P1588_MPLS_LABEL_VALUE_7r_GET(r) (r).p1588_mpls_label_value_7[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_VALUE_7r_MPLS_LABEL7_VALUE_15_0f_GET(r) (((r).p1588_mpls_label_value_7[0]) & 0xffff)
#define PLP_P1588_MPLS_LABEL_VALUE_7r_MPLS_LABEL7_VALUE_15_0f_SET(r,f) (r).p1588_mpls_label_value_7[0]=(((r).p1588_mpls_label_value_7[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_VALUE_7.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_VALUE_7r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_VALUE_7r,(_r._p1588_mpls_label_value_7))
#define PLP_WRITE_P1588_MPLS_LABEL_VALUE_7r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_VALUE_7r,(_r._p1588_mpls_label_value_7))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_VALUE_7r PLP_P1588_MPLS_LABEL_VALUE_7r
#define P1588_MPLS_LABEL_VALUE_7r_SIZE PLP_P1588_MPLS_LABEL_VALUE_7r_SIZE
typedef PLP_P1588_MPLS_LABEL_VALUE_7r_t P1588_MPLS_LABEL_VALUE_7r_t;
#define P1588_MPLS_LABEL_VALUE_7r_CLR PLP_P1588_MPLS_LABEL_VALUE_7r_CLR
#define P1588_MPLS_LABEL_VALUE_7r_SET PLP_P1588_MPLS_LABEL_VALUE_7r_SET
#define P1588_MPLS_LABEL_VALUE_7r_GET PLP_P1588_MPLS_LABEL_VALUE_7r_GET
#define P1588_MPLS_LABEL_VALUE_7r_MPLS_LABEL7_VALUE_15_0f_GET PLP_P1588_MPLS_LABEL_VALUE_7r_MPLS_LABEL7_VALUE_15_0f_GET
#define P1588_MPLS_LABEL_VALUE_7r_MPLS_LABEL7_VALUE_15_0f_SET PLP_P1588_MPLS_LABEL_VALUE_7r_MPLS_LABEL7_VALUE_15_0f_SET
#define READ_P1588_MPLS_LABEL_VALUE_7r PLP_READ_P1588_MPLS_LABEL_VALUE_7r
#define WRITE_P1588_MPLS_LABEL_VALUE_7r PLP_WRITE_P1588_MPLS_LABEL_VALUE_7r
#define MODIFY_P1588_MPLS_LABEL_VALUE_7r PLP_MODIFY_P1588_MPLS_LABEL_VALUE_7r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_VALUE_7r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_VALUE_8
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x705b
 * DESC:     MPLS LABEL VALUE 8 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL8_VALUE_15_0 MPLS label 7 valueTx label match = (label dir == EGRESS)  ? (extracted label[19:0] & label_mask[19:0]) == label_value[19:0]) : 0Rx label match = (label dir == INGRESS) ? (extracted label[19:0] & label_mask[19:0]) == label_value[19:0]) : 0
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_VALUE_8r    0x0000005b

#define PLP_P1588_MPLS_LABEL_VALUE_8r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_VALUE_8.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_VALUE_8r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_value_8[1];
	uint32_t _p1588_mpls_label_value_8;
} PLP_P1588_MPLS_LABEL_VALUE_8r_t;

#define PLP_P1588_MPLS_LABEL_VALUE_8r_CLR(r) (r).p1588_mpls_label_value_8[0] = 0
#define PLP_P1588_MPLS_LABEL_VALUE_8r_SET(r,d) (r).p1588_mpls_label_value_8[0] = d
#define PLP_P1588_MPLS_LABEL_VALUE_8r_GET(r) (r).p1588_mpls_label_value_8[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_VALUE_8r_MPLS_LABEL8_VALUE_15_0f_GET(r) (((r).p1588_mpls_label_value_8[0]) & 0xffff)
#define PLP_P1588_MPLS_LABEL_VALUE_8r_MPLS_LABEL8_VALUE_15_0f_SET(r,f) (r).p1588_mpls_label_value_8[0]=(((r).p1588_mpls_label_value_8[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_VALUE_8.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_VALUE_8r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_VALUE_8r,(_r._p1588_mpls_label_value_8))
#define PLP_WRITE_P1588_MPLS_LABEL_VALUE_8r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_VALUE_8r,(_r._p1588_mpls_label_value_8))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_VALUE_8r PLP_P1588_MPLS_LABEL_VALUE_8r
#define P1588_MPLS_LABEL_VALUE_8r_SIZE PLP_P1588_MPLS_LABEL_VALUE_8r_SIZE
typedef PLP_P1588_MPLS_LABEL_VALUE_8r_t P1588_MPLS_LABEL_VALUE_8r_t;
#define P1588_MPLS_LABEL_VALUE_8r_CLR PLP_P1588_MPLS_LABEL_VALUE_8r_CLR
#define P1588_MPLS_LABEL_VALUE_8r_SET PLP_P1588_MPLS_LABEL_VALUE_8r_SET
#define P1588_MPLS_LABEL_VALUE_8r_GET PLP_P1588_MPLS_LABEL_VALUE_8r_GET
#define P1588_MPLS_LABEL_VALUE_8r_MPLS_LABEL8_VALUE_15_0f_GET PLP_P1588_MPLS_LABEL_VALUE_8r_MPLS_LABEL8_VALUE_15_0f_GET
#define P1588_MPLS_LABEL_VALUE_8r_MPLS_LABEL8_VALUE_15_0f_SET PLP_P1588_MPLS_LABEL_VALUE_8r_MPLS_LABEL8_VALUE_15_0f_SET
#define READ_P1588_MPLS_LABEL_VALUE_8r PLP_READ_P1588_MPLS_LABEL_VALUE_8r
#define WRITE_P1588_MPLS_LABEL_VALUE_8r PLP_WRITE_P1588_MPLS_LABEL_VALUE_8r
#define MODIFY_P1588_MPLS_LABEL_VALUE_8r PLP_MODIFY_P1588_MPLS_LABEL_VALUE_8r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_VALUE_8r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_VALUE_9
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x705c
 * DESC:     MPLS LABEL VALUE 9 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL9_VALUE_15_0 MPLS label 9 valueTx label match = (label dir == EGRESS)  ? (extracted label[19:0] & label_mask[19:0]) == label_value[19:0]) : 0Rx label match = (label dir == INGRESS) ? (extracted label[19:0] & label_mask[19:0]) == label_value[19:0]) : 0
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_VALUE_9r    0x0000005c

#define PLP_P1588_MPLS_LABEL_VALUE_9r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_VALUE_9.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_VALUE_9r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_value_9[1];
	uint32_t _p1588_mpls_label_value_9;
} PLP_P1588_MPLS_LABEL_VALUE_9r_t;

#define PLP_P1588_MPLS_LABEL_VALUE_9r_CLR(r) (r).p1588_mpls_label_value_9[0] = 0
#define PLP_P1588_MPLS_LABEL_VALUE_9r_SET(r,d) (r).p1588_mpls_label_value_9[0] = d
#define PLP_P1588_MPLS_LABEL_VALUE_9r_GET(r) (r).p1588_mpls_label_value_9[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_VALUE_9r_MPLS_LABEL9_VALUE_15_0f_GET(r) (((r).p1588_mpls_label_value_9[0]) & 0xffff)
#define PLP_P1588_MPLS_LABEL_VALUE_9r_MPLS_LABEL9_VALUE_15_0f_SET(r,f) (r).p1588_mpls_label_value_9[0]=(((r).p1588_mpls_label_value_9[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_VALUE_9.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_VALUE_9r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_VALUE_9r,(_r._p1588_mpls_label_value_9))
#define PLP_WRITE_P1588_MPLS_LABEL_VALUE_9r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_VALUE_9r,(_r._p1588_mpls_label_value_9))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_VALUE_9r PLP_P1588_MPLS_LABEL_VALUE_9r
#define P1588_MPLS_LABEL_VALUE_9r_SIZE PLP_P1588_MPLS_LABEL_VALUE_9r_SIZE
typedef PLP_P1588_MPLS_LABEL_VALUE_9r_t P1588_MPLS_LABEL_VALUE_9r_t;
#define P1588_MPLS_LABEL_VALUE_9r_CLR PLP_P1588_MPLS_LABEL_VALUE_9r_CLR
#define P1588_MPLS_LABEL_VALUE_9r_SET PLP_P1588_MPLS_LABEL_VALUE_9r_SET
#define P1588_MPLS_LABEL_VALUE_9r_GET PLP_P1588_MPLS_LABEL_VALUE_9r_GET
#define P1588_MPLS_LABEL_VALUE_9r_MPLS_LABEL9_VALUE_15_0f_GET PLP_P1588_MPLS_LABEL_VALUE_9r_MPLS_LABEL9_VALUE_15_0f_GET
#define P1588_MPLS_LABEL_VALUE_9r_MPLS_LABEL9_VALUE_15_0f_SET PLP_P1588_MPLS_LABEL_VALUE_9r_MPLS_LABEL9_VALUE_15_0f_SET
#define READ_P1588_MPLS_LABEL_VALUE_9r PLP_READ_P1588_MPLS_LABEL_VALUE_9r
#define WRITE_P1588_MPLS_LABEL_VALUE_9r PLP_WRITE_P1588_MPLS_LABEL_VALUE_9r
#define MODIFY_P1588_MPLS_LABEL_VALUE_9r PLP_MODIFY_P1588_MPLS_LABEL_VALUE_9r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_VALUE_9r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_VALUE_10
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x705d
 * DESC:     MPLS LABEL VALUE 10 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL10_VALUE_15_0 MPLS label 10 valueTx label match = (label dir == EGRESS)  ? (extracted label[19:0] & label_mask[19:0]) == label_value[19:0]) : 0Rx label match = (label dir == INGRESS) ? (extracted label[19:0] & label_mask[19:0]) == label_value[19:0]) : 0
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_VALUE_10r    0x0000005d

#define PLP_P1588_MPLS_LABEL_VALUE_10r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_VALUE_10.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_VALUE_10r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_value_10[1];
	uint32_t _p1588_mpls_label_value_10;
} PLP_P1588_MPLS_LABEL_VALUE_10r_t;

#define PLP_P1588_MPLS_LABEL_VALUE_10r_CLR(r) (r).p1588_mpls_label_value_10[0] = 0
#define PLP_P1588_MPLS_LABEL_VALUE_10r_SET(r,d) (r).p1588_mpls_label_value_10[0] = d
#define PLP_P1588_MPLS_LABEL_VALUE_10r_GET(r) (r).p1588_mpls_label_value_10[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_VALUE_10r_MPLS_LABEL10_VALUE_15_0f_GET(r) (((r).p1588_mpls_label_value_10[0]) & 0xffff)
#define PLP_P1588_MPLS_LABEL_VALUE_10r_MPLS_LABEL10_VALUE_15_0f_SET(r,f) (r).p1588_mpls_label_value_10[0]=(((r).p1588_mpls_label_value_10[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_VALUE_10.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_VALUE_10r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_VALUE_10r,(_r._p1588_mpls_label_value_10))
#define PLP_WRITE_P1588_MPLS_LABEL_VALUE_10r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_VALUE_10r,(_r._p1588_mpls_label_value_10))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_VALUE_10r PLP_P1588_MPLS_LABEL_VALUE_10r
#define P1588_MPLS_LABEL_VALUE_10r_SIZE PLP_P1588_MPLS_LABEL_VALUE_10r_SIZE
typedef PLP_P1588_MPLS_LABEL_VALUE_10r_t P1588_MPLS_LABEL_VALUE_10r_t;
#define P1588_MPLS_LABEL_VALUE_10r_CLR PLP_P1588_MPLS_LABEL_VALUE_10r_CLR
#define P1588_MPLS_LABEL_VALUE_10r_SET PLP_P1588_MPLS_LABEL_VALUE_10r_SET
#define P1588_MPLS_LABEL_VALUE_10r_GET PLP_P1588_MPLS_LABEL_VALUE_10r_GET
#define P1588_MPLS_LABEL_VALUE_10r_MPLS_LABEL10_VALUE_15_0f_GET PLP_P1588_MPLS_LABEL_VALUE_10r_MPLS_LABEL10_VALUE_15_0f_GET
#define P1588_MPLS_LABEL_VALUE_10r_MPLS_LABEL10_VALUE_15_0f_SET PLP_P1588_MPLS_LABEL_VALUE_10r_MPLS_LABEL10_VALUE_15_0f_SET
#define READ_P1588_MPLS_LABEL_VALUE_10r PLP_READ_P1588_MPLS_LABEL_VALUE_10r
#define WRITE_P1588_MPLS_LABEL_VALUE_10r PLP_WRITE_P1588_MPLS_LABEL_VALUE_10r
#define MODIFY_P1588_MPLS_LABEL_VALUE_10r PLP_MODIFY_P1588_MPLS_LABEL_VALUE_10r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_VALUE_10r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_VALUE_11
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x705e
 * DESC:     MPLS LABEL VALUE 11 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL1_VALUE_19_16 MPLS label 1 value
 *     MPLS_LABEL2_VALUE_19_16 MPLS label 2 value
 *     MPLS_LABEL3_VALUE_19_16 MPLS label 3 value
 *     MPLS_LABEL4_VALUE_19_16 MPLS label 4 value
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_VALUE_11r    0x0000005e

#define PLP_P1588_MPLS_LABEL_VALUE_11r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_VALUE_11.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_VALUE_11r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_value_11[1];
	uint32_t _p1588_mpls_label_value_11;
} PLP_P1588_MPLS_LABEL_VALUE_11r_t;

#define PLP_P1588_MPLS_LABEL_VALUE_11r_CLR(r) (r).p1588_mpls_label_value_11[0] = 0
#define PLP_P1588_MPLS_LABEL_VALUE_11r_SET(r,d) (r).p1588_mpls_label_value_11[0] = d
#define PLP_P1588_MPLS_LABEL_VALUE_11r_GET(r) (r).p1588_mpls_label_value_11[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL4_VALUE_19_16f_GET(r) ((((r).p1588_mpls_label_value_11[0]) >> 12) & 0xf)
#define PLP_P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL4_VALUE_19_16f_SET(r,f) (r).p1588_mpls_label_value_11[0]=(((r).p1588_mpls_label_value_11[0] & ~((uint32_t)0xf << 12)) | ((((uint32_t)f) & 0xf) << 12)) | (15 << (16 + 12))
#define PLP_P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL3_VALUE_19_16f_GET(r) ((((r).p1588_mpls_label_value_11[0]) >> 8) & 0xf)
#define PLP_P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL3_VALUE_19_16f_SET(r,f) (r).p1588_mpls_label_value_11[0]=(((r).p1588_mpls_label_value_11[0] & ~((uint32_t)0xf << 8)) | ((((uint32_t)f) & 0xf) << 8)) | (15 << (16 + 8))
#define PLP_P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL2_VALUE_19_16f_GET(r) ((((r).p1588_mpls_label_value_11[0]) >> 4) & 0xf)
#define PLP_P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL2_VALUE_19_16f_SET(r,f) (r).p1588_mpls_label_value_11[0]=(((r).p1588_mpls_label_value_11[0] & ~((uint32_t)0xf << 4)) | ((((uint32_t)f) & 0xf) << 4)) | (15 << (16 + 4))
#define PLP_P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL1_VALUE_19_16f_GET(r) (((r).p1588_mpls_label_value_11[0]) & 0xf)
#define PLP_P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL1_VALUE_19_16f_SET(r,f) (r).p1588_mpls_label_value_11[0]=(((r).p1588_mpls_label_value_11[0] & ~((uint32_t)0xf)) | (((uint32_t)f) & 0xf)) | (0xf << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_VALUE_11.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_VALUE_11r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_VALUE_11r,(_r._p1588_mpls_label_value_11))
#define PLP_WRITE_P1588_MPLS_LABEL_VALUE_11r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_VALUE_11r,(_r._p1588_mpls_label_value_11))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_VALUE_11r PLP_P1588_MPLS_LABEL_VALUE_11r
#define P1588_MPLS_LABEL_VALUE_11r_SIZE PLP_P1588_MPLS_LABEL_VALUE_11r_SIZE
typedef PLP_P1588_MPLS_LABEL_VALUE_11r_t P1588_MPLS_LABEL_VALUE_11r_t;
#define P1588_MPLS_LABEL_VALUE_11r_CLR PLP_P1588_MPLS_LABEL_VALUE_11r_CLR
#define P1588_MPLS_LABEL_VALUE_11r_SET PLP_P1588_MPLS_LABEL_VALUE_11r_SET
#define P1588_MPLS_LABEL_VALUE_11r_GET PLP_P1588_MPLS_LABEL_VALUE_11r_GET
#define P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL4_VALUE_19_16f_GET PLP_P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL4_VALUE_19_16f_GET
#define P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL4_VALUE_19_16f_SET PLP_P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL4_VALUE_19_16f_SET
#define P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL3_VALUE_19_16f_GET PLP_P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL3_VALUE_19_16f_GET
#define P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL3_VALUE_19_16f_SET PLP_P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL3_VALUE_19_16f_SET
#define P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL2_VALUE_19_16f_GET PLP_P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL2_VALUE_19_16f_GET
#define P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL2_VALUE_19_16f_SET PLP_P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL2_VALUE_19_16f_SET
#define P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL1_VALUE_19_16f_GET PLP_P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL1_VALUE_19_16f_GET
#define P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL1_VALUE_19_16f_SET PLP_P1588_MPLS_LABEL_VALUE_11r_MPLS_LABEL1_VALUE_19_16f_SET
#define READ_P1588_MPLS_LABEL_VALUE_11r PLP_READ_P1588_MPLS_LABEL_VALUE_11r
#define WRITE_P1588_MPLS_LABEL_VALUE_11r PLP_WRITE_P1588_MPLS_LABEL_VALUE_11r
#define MODIFY_P1588_MPLS_LABEL_VALUE_11r PLP_MODIFY_P1588_MPLS_LABEL_VALUE_11r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_VALUE_11r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_VALUE_12
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x705f
 * DESC:     MPLS LABEL VALUE 12 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL5_VALUE_19_16 MPLS label 5 value
 *     MPLS_LABEL6_VALUE_19_16 MPLS label 6 value
 *     MPLS_LABEL7_VALUE_19_16 MPLS label 7 value
 *     MPLS_LABEL8_VALUE_19_16 MPLS label 8 value
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_VALUE_12r    0x0000005f

#define PLP_P1588_MPLS_LABEL_VALUE_12r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_VALUE_12.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_VALUE_12r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_value_12[1];
	uint32_t _p1588_mpls_label_value_12;
} PLP_P1588_MPLS_LABEL_VALUE_12r_t;

#define PLP_P1588_MPLS_LABEL_VALUE_12r_CLR(r) (r).p1588_mpls_label_value_12[0] = 0
#define PLP_P1588_MPLS_LABEL_VALUE_12r_SET(r,d) (r).p1588_mpls_label_value_12[0] = d
#define PLP_P1588_MPLS_LABEL_VALUE_12r_GET(r) (r).p1588_mpls_label_value_12[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL8_VALUE_19_16f_GET(r) ((((r).p1588_mpls_label_value_12[0]) >> 12) & 0xf)
#define PLP_P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL8_VALUE_19_16f_SET(r,f) (r).p1588_mpls_label_value_12[0]=(((r).p1588_mpls_label_value_12[0] & ~((uint32_t)0xf << 12)) | ((((uint32_t)f) & 0xf) << 12)) | (15 << (16 + 12))
#define PLP_P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL7_VALUE_19_16f_GET(r) ((((r).p1588_mpls_label_value_12[0]) >> 8) & 0xf)
#define PLP_P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL7_VALUE_19_16f_SET(r,f) (r).p1588_mpls_label_value_12[0]=(((r).p1588_mpls_label_value_12[0] & ~((uint32_t)0xf << 8)) | ((((uint32_t)f) & 0xf) << 8)) | (15 << (16 + 8))
#define PLP_P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL6_VALUE_19_16f_GET(r) ((((r).p1588_mpls_label_value_12[0]) >> 4) & 0xf)
#define PLP_P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL6_VALUE_19_16f_SET(r,f) (r).p1588_mpls_label_value_12[0]=(((r).p1588_mpls_label_value_12[0] & ~((uint32_t)0xf << 4)) | ((((uint32_t)f) & 0xf) << 4)) | (15 << (16 + 4))
#define PLP_P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL5_VALUE_19_16f_GET(r) (((r).p1588_mpls_label_value_12[0]) & 0xf)
#define PLP_P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL5_VALUE_19_16f_SET(r,f) (r).p1588_mpls_label_value_12[0]=(((r).p1588_mpls_label_value_12[0] & ~((uint32_t)0xf)) | (((uint32_t)f) & 0xf)) | (0xf << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_VALUE_12.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_VALUE_12r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_VALUE_12r,(_r._p1588_mpls_label_value_12))
#define PLP_WRITE_P1588_MPLS_LABEL_VALUE_12r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_VALUE_12r,(_r._p1588_mpls_label_value_12))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_VALUE_12r PLP_P1588_MPLS_LABEL_VALUE_12r
#define P1588_MPLS_LABEL_VALUE_12r_SIZE PLP_P1588_MPLS_LABEL_VALUE_12r_SIZE
typedef PLP_P1588_MPLS_LABEL_VALUE_12r_t P1588_MPLS_LABEL_VALUE_12r_t;
#define P1588_MPLS_LABEL_VALUE_12r_CLR PLP_P1588_MPLS_LABEL_VALUE_12r_CLR
#define P1588_MPLS_LABEL_VALUE_12r_SET PLP_P1588_MPLS_LABEL_VALUE_12r_SET
#define P1588_MPLS_LABEL_VALUE_12r_GET PLP_P1588_MPLS_LABEL_VALUE_12r_GET
#define P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL8_VALUE_19_16f_GET PLP_P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL8_VALUE_19_16f_GET
#define P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL8_VALUE_19_16f_SET PLP_P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL8_VALUE_19_16f_SET
#define P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL7_VALUE_19_16f_GET PLP_P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL7_VALUE_19_16f_GET
#define P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL7_VALUE_19_16f_SET PLP_P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL7_VALUE_19_16f_SET
#define P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL6_VALUE_19_16f_GET PLP_P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL6_VALUE_19_16f_GET
#define P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL6_VALUE_19_16f_SET PLP_P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL6_VALUE_19_16f_SET
#define P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL5_VALUE_19_16f_GET PLP_P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL5_VALUE_19_16f_GET
#define P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL5_VALUE_19_16f_SET PLP_P1588_MPLS_LABEL_VALUE_12r_MPLS_LABEL5_VALUE_19_16f_SET
#define READ_P1588_MPLS_LABEL_VALUE_12r PLP_READ_P1588_MPLS_LABEL_VALUE_12r
#define WRITE_P1588_MPLS_LABEL_VALUE_12r PLP_WRITE_P1588_MPLS_LABEL_VALUE_12r
#define MODIFY_P1588_MPLS_LABEL_VALUE_12r PLP_MODIFY_P1588_MPLS_LABEL_VALUE_12r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_VALUE_12r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_VALUE_13
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7060
 * DESC:     MPLS LABEL VALUE 13 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL9_VALUE_19_16 MPLS label 9 value
 *     MPLS_LABEL10_VALUE_19_16 MPLS label 10 value
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_VALUE_13r    0x00000060

#define PLP_P1588_MPLS_LABEL_VALUE_13r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_VALUE_13.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_VALUE_13r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_value_13[1];
	uint32_t _p1588_mpls_label_value_13;
} PLP_P1588_MPLS_LABEL_VALUE_13r_t;

#define PLP_P1588_MPLS_LABEL_VALUE_13r_CLR(r) (r).p1588_mpls_label_value_13[0] = 0
#define PLP_P1588_MPLS_LABEL_VALUE_13r_SET(r,d) (r).p1588_mpls_label_value_13[0] = d
#define PLP_P1588_MPLS_LABEL_VALUE_13r_GET(r) (r).p1588_mpls_label_value_13[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_VALUE_13r_MPLS_LABEL10_VALUE_19_16f_GET(r) ((((r).p1588_mpls_label_value_13[0]) >> 4) & 0xf)
#define PLP_P1588_MPLS_LABEL_VALUE_13r_MPLS_LABEL10_VALUE_19_16f_SET(r,f) (r).p1588_mpls_label_value_13[0]=(((r).p1588_mpls_label_value_13[0] & ~((uint32_t)0xf << 4)) | ((((uint32_t)f) & 0xf) << 4)) | (15 << (16 + 4))
#define PLP_P1588_MPLS_LABEL_VALUE_13r_MPLS_LABEL9_VALUE_19_16f_GET(r) (((r).p1588_mpls_label_value_13[0]) & 0xf)
#define PLP_P1588_MPLS_LABEL_VALUE_13r_MPLS_LABEL9_VALUE_19_16f_SET(r,f) (r).p1588_mpls_label_value_13[0]=(((r).p1588_mpls_label_value_13[0] & ~((uint32_t)0xf)) | (((uint32_t)f) & 0xf)) | (0xf << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_VALUE_13.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_VALUE_13r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_VALUE_13r,(_r._p1588_mpls_label_value_13))
#define PLP_WRITE_P1588_MPLS_LABEL_VALUE_13r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_VALUE_13r,(_r._p1588_mpls_label_value_13))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_VALUE_13r PLP_P1588_MPLS_LABEL_VALUE_13r
#define P1588_MPLS_LABEL_VALUE_13r_SIZE PLP_P1588_MPLS_LABEL_VALUE_13r_SIZE
typedef PLP_P1588_MPLS_LABEL_VALUE_13r_t P1588_MPLS_LABEL_VALUE_13r_t;
#define P1588_MPLS_LABEL_VALUE_13r_CLR PLP_P1588_MPLS_LABEL_VALUE_13r_CLR
#define P1588_MPLS_LABEL_VALUE_13r_SET PLP_P1588_MPLS_LABEL_VALUE_13r_SET
#define P1588_MPLS_LABEL_VALUE_13r_GET PLP_P1588_MPLS_LABEL_VALUE_13r_GET
#define P1588_MPLS_LABEL_VALUE_13r_MPLS_LABEL10_VALUE_19_16f_GET PLP_P1588_MPLS_LABEL_VALUE_13r_MPLS_LABEL10_VALUE_19_16f_GET
#define P1588_MPLS_LABEL_VALUE_13r_MPLS_LABEL10_VALUE_19_16f_SET PLP_P1588_MPLS_LABEL_VALUE_13r_MPLS_LABEL10_VALUE_19_16f_SET
#define P1588_MPLS_LABEL_VALUE_13r_MPLS_LABEL9_VALUE_19_16f_GET PLP_P1588_MPLS_LABEL_VALUE_13r_MPLS_LABEL9_VALUE_19_16f_GET
#define P1588_MPLS_LABEL_VALUE_13r_MPLS_LABEL9_VALUE_19_16f_SET PLP_P1588_MPLS_LABEL_VALUE_13r_MPLS_LABEL9_VALUE_19_16f_SET
#define READ_P1588_MPLS_LABEL_VALUE_13r PLP_READ_P1588_MPLS_LABEL_VALUE_13r
#define WRITE_P1588_MPLS_LABEL_VALUE_13r PLP_WRITE_P1588_MPLS_LABEL_VALUE_13r
#define MODIFY_P1588_MPLS_LABEL_VALUE_13r PLP_MODIFY_P1588_MPLS_LABEL_VALUE_13r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_VALUE_13r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_MASK_1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7061
 * DESC:     MPLS LABEL MASK 1 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL1_MASK_15_0 MPLS label 1 mask0 - Ignore for label match1 - Enable for label matchThis is rightfully an enable and not a mask
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_MASK_1r    0x00000061

#define PLP_P1588_MPLS_LABEL_MASK_1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_MASK_1.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_MASK_1r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_mask_1[1];
	uint32_t _p1588_mpls_label_mask_1;
} PLP_P1588_MPLS_LABEL_MASK_1r_t;

#define PLP_P1588_MPLS_LABEL_MASK_1r_CLR(r) (r).p1588_mpls_label_mask_1[0] = 0
#define PLP_P1588_MPLS_LABEL_MASK_1r_SET(r,d) (r).p1588_mpls_label_mask_1[0] = d
#define PLP_P1588_MPLS_LABEL_MASK_1r_GET(r) (r).p1588_mpls_label_mask_1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_MASK_1r_MPLS_LABEL1_MASK_15_0f_GET(r) (((r).p1588_mpls_label_mask_1[0]) & 0xffff)
#define PLP_P1588_MPLS_LABEL_MASK_1r_MPLS_LABEL1_MASK_15_0f_SET(r,f) (r).p1588_mpls_label_mask_1[0]=(((r).p1588_mpls_label_mask_1[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_MASK_1.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_MASK_1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_MASK_1r,(_r._p1588_mpls_label_mask_1))
#define PLP_WRITE_P1588_MPLS_LABEL_MASK_1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_MASK_1r,(_r._p1588_mpls_label_mask_1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_MASK_1r PLP_P1588_MPLS_LABEL_MASK_1r
#define P1588_MPLS_LABEL_MASK_1r_SIZE PLP_P1588_MPLS_LABEL_MASK_1r_SIZE
typedef PLP_P1588_MPLS_LABEL_MASK_1r_t P1588_MPLS_LABEL_MASK_1r_t;
#define P1588_MPLS_LABEL_MASK_1r_CLR PLP_P1588_MPLS_LABEL_MASK_1r_CLR
#define P1588_MPLS_LABEL_MASK_1r_SET PLP_P1588_MPLS_LABEL_MASK_1r_SET
#define P1588_MPLS_LABEL_MASK_1r_GET PLP_P1588_MPLS_LABEL_MASK_1r_GET
#define P1588_MPLS_LABEL_MASK_1r_MPLS_LABEL1_MASK_15_0f_GET PLP_P1588_MPLS_LABEL_MASK_1r_MPLS_LABEL1_MASK_15_0f_GET
#define P1588_MPLS_LABEL_MASK_1r_MPLS_LABEL1_MASK_15_0f_SET PLP_P1588_MPLS_LABEL_MASK_1r_MPLS_LABEL1_MASK_15_0f_SET
#define READ_P1588_MPLS_LABEL_MASK_1r PLP_READ_P1588_MPLS_LABEL_MASK_1r
#define WRITE_P1588_MPLS_LABEL_MASK_1r PLP_WRITE_P1588_MPLS_LABEL_MASK_1r
#define MODIFY_P1588_MPLS_LABEL_MASK_1r PLP_MODIFY_P1588_MPLS_LABEL_MASK_1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_MASK_1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_MASK_2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7062
 * DESC:     MPLS LABEL MASK 2 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL2_MASK_15_0 MPLS label 2 mask0 - Ignore for label match1 - Enable for label matchThis is rightfully an enable and not a mask
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_MASK_2r    0x00000062

#define PLP_P1588_MPLS_LABEL_MASK_2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_MASK_2.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_MASK_2r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_mask_2[1];
	uint32_t _p1588_mpls_label_mask_2;
} PLP_P1588_MPLS_LABEL_MASK_2r_t;

#define PLP_P1588_MPLS_LABEL_MASK_2r_CLR(r) (r).p1588_mpls_label_mask_2[0] = 0
#define PLP_P1588_MPLS_LABEL_MASK_2r_SET(r,d) (r).p1588_mpls_label_mask_2[0] = d
#define PLP_P1588_MPLS_LABEL_MASK_2r_GET(r) (r).p1588_mpls_label_mask_2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_MASK_2r_MPLS_LABEL2_MASK_15_0f_GET(r) (((r).p1588_mpls_label_mask_2[0]) & 0xffff)
#define PLP_P1588_MPLS_LABEL_MASK_2r_MPLS_LABEL2_MASK_15_0f_SET(r,f) (r).p1588_mpls_label_mask_2[0]=(((r).p1588_mpls_label_mask_2[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_MASK_2.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_MASK_2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_MASK_2r,(_r._p1588_mpls_label_mask_2))
#define PLP_WRITE_P1588_MPLS_LABEL_MASK_2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_MASK_2r,(_r._p1588_mpls_label_mask_2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_MASK_2r PLP_P1588_MPLS_LABEL_MASK_2r
#define P1588_MPLS_LABEL_MASK_2r_SIZE PLP_P1588_MPLS_LABEL_MASK_2r_SIZE
typedef PLP_P1588_MPLS_LABEL_MASK_2r_t P1588_MPLS_LABEL_MASK_2r_t;
#define P1588_MPLS_LABEL_MASK_2r_CLR PLP_P1588_MPLS_LABEL_MASK_2r_CLR
#define P1588_MPLS_LABEL_MASK_2r_SET PLP_P1588_MPLS_LABEL_MASK_2r_SET
#define P1588_MPLS_LABEL_MASK_2r_GET PLP_P1588_MPLS_LABEL_MASK_2r_GET
#define P1588_MPLS_LABEL_MASK_2r_MPLS_LABEL2_MASK_15_0f_GET PLP_P1588_MPLS_LABEL_MASK_2r_MPLS_LABEL2_MASK_15_0f_GET
#define P1588_MPLS_LABEL_MASK_2r_MPLS_LABEL2_MASK_15_0f_SET PLP_P1588_MPLS_LABEL_MASK_2r_MPLS_LABEL2_MASK_15_0f_SET
#define READ_P1588_MPLS_LABEL_MASK_2r PLP_READ_P1588_MPLS_LABEL_MASK_2r
#define WRITE_P1588_MPLS_LABEL_MASK_2r PLP_WRITE_P1588_MPLS_LABEL_MASK_2r
#define MODIFY_P1588_MPLS_LABEL_MASK_2r PLP_MODIFY_P1588_MPLS_LABEL_MASK_2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_MASK_2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_MASK_3
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7063
 * DESC:     MPLS LABEL MASK 3 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL3_MASK_15_0 MPLS label 3 mask0 - Ignore for label match1 - Enable for label matchThis is rightfully an enable and not a mask
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_MASK_3r    0x00000063

#define PLP_P1588_MPLS_LABEL_MASK_3r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_MASK_3.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_MASK_3r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_mask_3[1];
	uint32_t _p1588_mpls_label_mask_3;
} PLP_P1588_MPLS_LABEL_MASK_3r_t;

#define PLP_P1588_MPLS_LABEL_MASK_3r_CLR(r) (r).p1588_mpls_label_mask_3[0] = 0
#define PLP_P1588_MPLS_LABEL_MASK_3r_SET(r,d) (r).p1588_mpls_label_mask_3[0] = d
#define PLP_P1588_MPLS_LABEL_MASK_3r_GET(r) (r).p1588_mpls_label_mask_3[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_MASK_3r_MPLS_LABEL3_MASK_15_0f_GET(r) (((r).p1588_mpls_label_mask_3[0]) & 0xffff)
#define PLP_P1588_MPLS_LABEL_MASK_3r_MPLS_LABEL3_MASK_15_0f_SET(r,f) (r).p1588_mpls_label_mask_3[0]=(((r).p1588_mpls_label_mask_3[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_MASK_3.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_MASK_3r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_MASK_3r,(_r._p1588_mpls_label_mask_3))
#define PLP_WRITE_P1588_MPLS_LABEL_MASK_3r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_MASK_3r,(_r._p1588_mpls_label_mask_3))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_MASK_3r PLP_P1588_MPLS_LABEL_MASK_3r
#define P1588_MPLS_LABEL_MASK_3r_SIZE PLP_P1588_MPLS_LABEL_MASK_3r_SIZE
typedef PLP_P1588_MPLS_LABEL_MASK_3r_t P1588_MPLS_LABEL_MASK_3r_t;
#define P1588_MPLS_LABEL_MASK_3r_CLR PLP_P1588_MPLS_LABEL_MASK_3r_CLR
#define P1588_MPLS_LABEL_MASK_3r_SET PLP_P1588_MPLS_LABEL_MASK_3r_SET
#define P1588_MPLS_LABEL_MASK_3r_GET PLP_P1588_MPLS_LABEL_MASK_3r_GET
#define P1588_MPLS_LABEL_MASK_3r_MPLS_LABEL3_MASK_15_0f_GET PLP_P1588_MPLS_LABEL_MASK_3r_MPLS_LABEL3_MASK_15_0f_GET
#define P1588_MPLS_LABEL_MASK_3r_MPLS_LABEL3_MASK_15_0f_SET PLP_P1588_MPLS_LABEL_MASK_3r_MPLS_LABEL3_MASK_15_0f_SET
#define READ_P1588_MPLS_LABEL_MASK_3r PLP_READ_P1588_MPLS_LABEL_MASK_3r
#define WRITE_P1588_MPLS_LABEL_MASK_3r PLP_WRITE_P1588_MPLS_LABEL_MASK_3r
#define MODIFY_P1588_MPLS_LABEL_MASK_3r PLP_MODIFY_P1588_MPLS_LABEL_MASK_3r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_MASK_3r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_MASK_4
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7064
 * DESC:     MPLS LABEL MASK 4 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL4_MASK_15_0 MPLS label 4 mask0 - Ignore for label match1 - Enable for label matchThis is rightfully an enable and not a mask
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_MASK_4r    0x00000064

#define PLP_P1588_MPLS_LABEL_MASK_4r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_MASK_4.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_MASK_4r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_mask_4[1];
	uint32_t _p1588_mpls_label_mask_4;
} PLP_P1588_MPLS_LABEL_MASK_4r_t;

#define PLP_P1588_MPLS_LABEL_MASK_4r_CLR(r) (r).p1588_mpls_label_mask_4[0] = 0
#define PLP_P1588_MPLS_LABEL_MASK_4r_SET(r,d) (r).p1588_mpls_label_mask_4[0] = d
#define PLP_P1588_MPLS_LABEL_MASK_4r_GET(r) (r).p1588_mpls_label_mask_4[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_MASK_4r_MPLS_LABEL4_MASK_15_0f_GET(r) (((r).p1588_mpls_label_mask_4[0]) & 0xffff)
#define PLP_P1588_MPLS_LABEL_MASK_4r_MPLS_LABEL4_MASK_15_0f_SET(r,f) (r).p1588_mpls_label_mask_4[0]=(((r).p1588_mpls_label_mask_4[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_MASK_4.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_MASK_4r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_MASK_4r,(_r._p1588_mpls_label_mask_4))
#define PLP_WRITE_P1588_MPLS_LABEL_MASK_4r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_MASK_4r,(_r._p1588_mpls_label_mask_4))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_MASK_4r PLP_P1588_MPLS_LABEL_MASK_4r
#define P1588_MPLS_LABEL_MASK_4r_SIZE PLP_P1588_MPLS_LABEL_MASK_4r_SIZE
typedef PLP_P1588_MPLS_LABEL_MASK_4r_t P1588_MPLS_LABEL_MASK_4r_t;
#define P1588_MPLS_LABEL_MASK_4r_CLR PLP_P1588_MPLS_LABEL_MASK_4r_CLR
#define P1588_MPLS_LABEL_MASK_4r_SET PLP_P1588_MPLS_LABEL_MASK_4r_SET
#define P1588_MPLS_LABEL_MASK_4r_GET PLP_P1588_MPLS_LABEL_MASK_4r_GET
#define P1588_MPLS_LABEL_MASK_4r_MPLS_LABEL4_MASK_15_0f_GET PLP_P1588_MPLS_LABEL_MASK_4r_MPLS_LABEL4_MASK_15_0f_GET
#define P1588_MPLS_LABEL_MASK_4r_MPLS_LABEL4_MASK_15_0f_SET PLP_P1588_MPLS_LABEL_MASK_4r_MPLS_LABEL4_MASK_15_0f_SET
#define READ_P1588_MPLS_LABEL_MASK_4r PLP_READ_P1588_MPLS_LABEL_MASK_4r
#define WRITE_P1588_MPLS_LABEL_MASK_4r PLP_WRITE_P1588_MPLS_LABEL_MASK_4r
#define MODIFY_P1588_MPLS_LABEL_MASK_4r PLP_MODIFY_P1588_MPLS_LABEL_MASK_4r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_MASK_4r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_MASK_5
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7065
 * DESC:     MPLS LABEL MASK 5 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL5_MASK_15_0 MPLS label 5 mask0 - Ignore for label match1 - Enable for label matchThis is rightfully an enable and not a mask
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_MASK_5r    0x00000065

#define PLP_P1588_MPLS_LABEL_MASK_5r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_MASK_5.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_MASK_5r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_mask_5[1];
	uint32_t _p1588_mpls_label_mask_5;
} PLP_P1588_MPLS_LABEL_MASK_5r_t;

#define PLP_P1588_MPLS_LABEL_MASK_5r_CLR(r) (r).p1588_mpls_label_mask_5[0] = 0
#define PLP_P1588_MPLS_LABEL_MASK_5r_SET(r,d) (r).p1588_mpls_label_mask_5[0] = d
#define PLP_P1588_MPLS_LABEL_MASK_5r_GET(r) (r).p1588_mpls_label_mask_5[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_MASK_5r_MPLS_LABEL5_MASK_15_0f_GET(r) (((r).p1588_mpls_label_mask_5[0]) & 0xffff)
#define PLP_P1588_MPLS_LABEL_MASK_5r_MPLS_LABEL5_MASK_15_0f_SET(r,f) (r).p1588_mpls_label_mask_5[0]=(((r).p1588_mpls_label_mask_5[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_MASK_5.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_MASK_5r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_MASK_5r,(_r._p1588_mpls_label_mask_5))
#define PLP_WRITE_P1588_MPLS_LABEL_MASK_5r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_MASK_5r,(_r._p1588_mpls_label_mask_5))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_MASK_5r PLP_P1588_MPLS_LABEL_MASK_5r
#define P1588_MPLS_LABEL_MASK_5r_SIZE PLP_P1588_MPLS_LABEL_MASK_5r_SIZE
typedef PLP_P1588_MPLS_LABEL_MASK_5r_t P1588_MPLS_LABEL_MASK_5r_t;
#define P1588_MPLS_LABEL_MASK_5r_CLR PLP_P1588_MPLS_LABEL_MASK_5r_CLR
#define P1588_MPLS_LABEL_MASK_5r_SET PLP_P1588_MPLS_LABEL_MASK_5r_SET
#define P1588_MPLS_LABEL_MASK_5r_GET PLP_P1588_MPLS_LABEL_MASK_5r_GET
#define P1588_MPLS_LABEL_MASK_5r_MPLS_LABEL5_MASK_15_0f_GET PLP_P1588_MPLS_LABEL_MASK_5r_MPLS_LABEL5_MASK_15_0f_GET
#define P1588_MPLS_LABEL_MASK_5r_MPLS_LABEL5_MASK_15_0f_SET PLP_P1588_MPLS_LABEL_MASK_5r_MPLS_LABEL5_MASK_15_0f_SET
#define READ_P1588_MPLS_LABEL_MASK_5r PLP_READ_P1588_MPLS_LABEL_MASK_5r
#define WRITE_P1588_MPLS_LABEL_MASK_5r PLP_WRITE_P1588_MPLS_LABEL_MASK_5r
#define MODIFY_P1588_MPLS_LABEL_MASK_5r PLP_MODIFY_P1588_MPLS_LABEL_MASK_5r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_MASK_5r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_MASK_6
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7066
 * DESC:     MPLS LABEL MASK 6 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL6_MASK_15_0 MPLS label 6 mask0 - Ignore for label match1 - Enable for label matchThis is rightfully an enable and not a mask
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_MASK_6r    0x00000066

#define PLP_P1588_MPLS_LABEL_MASK_6r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_MASK_6.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_MASK_6r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_mask_6[1];
	uint32_t _p1588_mpls_label_mask_6;
} PLP_P1588_MPLS_LABEL_MASK_6r_t;

#define PLP_P1588_MPLS_LABEL_MASK_6r_CLR(r) (r).p1588_mpls_label_mask_6[0] = 0
#define PLP_P1588_MPLS_LABEL_MASK_6r_SET(r,d) (r).p1588_mpls_label_mask_6[0] = d
#define PLP_P1588_MPLS_LABEL_MASK_6r_GET(r) (r).p1588_mpls_label_mask_6[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_MASK_6r_MPLS_LABEL6_MASK_15_0f_GET(r) (((r).p1588_mpls_label_mask_6[0]) & 0xffff)
#define PLP_P1588_MPLS_LABEL_MASK_6r_MPLS_LABEL6_MASK_15_0f_SET(r,f) (r).p1588_mpls_label_mask_6[0]=(((r).p1588_mpls_label_mask_6[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_MASK_6.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_MASK_6r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_MASK_6r,(_r._p1588_mpls_label_mask_6))
#define PLP_WRITE_P1588_MPLS_LABEL_MASK_6r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_MASK_6r,(_r._p1588_mpls_label_mask_6))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_MASK_6r PLP_P1588_MPLS_LABEL_MASK_6r
#define P1588_MPLS_LABEL_MASK_6r_SIZE PLP_P1588_MPLS_LABEL_MASK_6r_SIZE
typedef PLP_P1588_MPLS_LABEL_MASK_6r_t P1588_MPLS_LABEL_MASK_6r_t;
#define P1588_MPLS_LABEL_MASK_6r_CLR PLP_P1588_MPLS_LABEL_MASK_6r_CLR
#define P1588_MPLS_LABEL_MASK_6r_SET PLP_P1588_MPLS_LABEL_MASK_6r_SET
#define P1588_MPLS_LABEL_MASK_6r_GET PLP_P1588_MPLS_LABEL_MASK_6r_GET
#define P1588_MPLS_LABEL_MASK_6r_MPLS_LABEL6_MASK_15_0f_GET PLP_P1588_MPLS_LABEL_MASK_6r_MPLS_LABEL6_MASK_15_0f_GET
#define P1588_MPLS_LABEL_MASK_6r_MPLS_LABEL6_MASK_15_0f_SET PLP_P1588_MPLS_LABEL_MASK_6r_MPLS_LABEL6_MASK_15_0f_SET
#define READ_P1588_MPLS_LABEL_MASK_6r PLP_READ_P1588_MPLS_LABEL_MASK_6r
#define WRITE_P1588_MPLS_LABEL_MASK_6r PLP_WRITE_P1588_MPLS_LABEL_MASK_6r
#define MODIFY_P1588_MPLS_LABEL_MASK_6r PLP_MODIFY_P1588_MPLS_LABEL_MASK_6r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_MASK_6r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_MASK_7
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7067
 * DESC:     MPLS LABEL MASK 7 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL7_MASK_15_0 MPLS label 7 mask0 - Ignore for label match1 - Enable for label matchThis is rightfully an enable and not a mask
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_MASK_7r    0x00000067

#define PLP_P1588_MPLS_LABEL_MASK_7r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_MASK_7.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_MASK_7r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_mask_7[1];
	uint32_t _p1588_mpls_label_mask_7;
} PLP_P1588_MPLS_LABEL_MASK_7r_t;

#define PLP_P1588_MPLS_LABEL_MASK_7r_CLR(r) (r).p1588_mpls_label_mask_7[0] = 0
#define PLP_P1588_MPLS_LABEL_MASK_7r_SET(r,d) (r).p1588_mpls_label_mask_7[0] = d
#define PLP_P1588_MPLS_LABEL_MASK_7r_GET(r) (r).p1588_mpls_label_mask_7[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_MASK_7r_MPLS_LABEL7_MASK_15_0f_GET(r) (((r).p1588_mpls_label_mask_7[0]) & 0xffff)
#define PLP_P1588_MPLS_LABEL_MASK_7r_MPLS_LABEL7_MASK_15_0f_SET(r,f) (r).p1588_mpls_label_mask_7[0]=(((r).p1588_mpls_label_mask_7[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_MASK_7.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_MASK_7r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_MASK_7r,(_r._p1588_mpls_label_mask_7))
#define PLP_WRITE_P1588_MPLS_LABEL_MASK_7r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_MASK_7r,(_r._p1588_mpls_label_mask_7))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_MASK_7r PLP_P1588_MPLS_LABEL_MASK_7r
#define P1588_MPLS_LABEL_MASK_7r_SIZE PLP_P1588_MPLS_LABEL_MASK_7r_SIZE
typedef PLP_P1588_MPLS_LABEL_MASK_7r_t P1588_MPLS_LABEL_MASK_7r_t;
#define P1588_MPLS_LABEL_MASK_7r_CLR PLP_P1588_MPLS_LABEL_MASK_7r_CLR
#define P1588_MPLS_LABEL_MASK_7r_SET PLP_P1588_MPLS_LABEL_MASK_7r_SET
#define P1588_MPLS_LABEL_MASK_7r_GET PLP_P1588_MPLS_LABEL_MASK_7r_GET
#define P1588_MPLS_LABEL_MASK_7r_MPLS_LABEL7_MASK_15_0f_GET PLP_P1588_MPLS_LABEL_MASK_7r_MPLS_LABEL7_MASK_15_0f_GET
#define P1588_MPLS_LABEL_MASK_7r_MPLS_LABEL7_MASK_15_0f_SET PLP_P1588_MPLS_LABEL_MASK_7r_MPLS_LABEL7_MASK_15_0f_SET
#define READ_P1588_MPLS_LABEL_MASK_7r PLP_READ_P1588_MPLS_LABEL_MASK_7r
#define WRITE_P1588_MPLS_LABEL_MASK_7r PLP_WRITE_P1588_MPLS_LABEL_MASK_7r
#define MODIFY_P1588_MPLS_LABEL_MASK_7r PLP_MODIFY_P1588_MPLS_LABEL_MASK_7r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_MASK_7r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_MASK_8
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7068
 * DESC:     MPLS LABEL MASK 8 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL8_MASK_15_0 MPLS label 7 mask0 - Ignore for label match1 - Enable for label matchThis is rightfully an enable and not a mask
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_MASK_8r    0x00000068

#define PLP_P1588_MPLS_LABEL_MASK_8r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_MASK_8.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_MASK_8r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_mask_8[1];
	uint32_t _p1588_mpls_label_mask_8;
} PLP_P1588_MPLS_LABEL_MASK_8r_t;

#define PLP_P1588_MPLS_LABEL_MASK_8r_CLR(r) (r).p1588_mpls_label_mask_8[0] = 0
#define PLP_P1588_MPLS_LABEL_MASK_8r_SET(r,d) (r).p1588_mpls_label_mask_8[0] = d
#define PLP_P1588_MPLS_LABEL_MASK_8r_GET(r) (r).p1588_mpls_label_mask_8[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_MASK_8r_MPLS_LABEL8_MASK_15_0f_GET(r) (((r).p1588_mpls_label_mask_8[0]) & 0xffff)
#define PLP_P1588_MPLS_LABEL_MASK_8r_MPLS_LABEL8_MASK_15_0f_SET(r,f) (r).p1588_mpls_label_mask_8[0]=(((r).p1588_mpls_label_mask_8[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_MASK_8.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_MASK_8r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_MASK_8r,(_r._p1588_mpls_label_mask_8))
#define PLP_WRITE_P1588_MPLS_LABEL_MASK_8r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_MASK_8r,(_r._p1588_mpls_label_mask_8))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_MASK_8r PLP_P1588_MPLS_LABEL_MASK_8r
#define P1588_MPLS_LABEL_MASK_8r_SIZE PLP_P1588_MPLS_LABEL_MASK_8r_SIZE
typedef PLP_P1588_MPLS_LABEL_MASK_8r_t P1588_MPLS_LABEL_MASK_8r_t;
#define P1588_MPLS_LABEL_MASK_8r_CLR PLP_P1588_MPLS_LABEL_MASK_8r_CLR
#define P1588_MPLS_LABEL_MASK_8r_SET PLP_P1588_MPLS_LABEL_MASK_8r_SET
#define P1588_MPLS_LABEL_MASK_8r_GET PLP_P1588_MPLS_LABEL_MASK_8r_GET
#define P1588_MPLS_LABEL_MASK_8r_MPLS_LABEL8_MASK_15_0f_GET PLP_P1588_MPLS_LABEL_MASK_8r_MPLS_LABEL8_MASK_15_0f_GET
#define P1588_MPLS_LABEL_MASK_8r_MPLS_LABEL8_MASK_15_0f_SET PLP_P1588_MPLS_LABEL_MASK_8r_MPLS_LABEL8_MASK_15_0f_SET
#define READ_P1588_MPLS_LABEL_MASK_8r PLP_READ_P1588_MPLS_LABEL_MASK_8r
#define WRITE_P1588_MPLS_LABEL_MASK_8r PLP_WRITE_P1588_MPLS_LABEL_MASK_8r
#define MODIFY_P1588_MPLS_LABEL_MASK_8r PLP_MODIFY_P1588_MPLS_LABEL_MASK_8r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_MASK_8r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_MASK_9
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7069
 * DESC:     MPLS LABEL MASK 9 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL9_MASK_15_0 MPLS label 9 mask0 - Ignore for label match1 - Enable for label matchThis is rightfully an enable and not a mask
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_MASK_9r    0x00000069

#define PLP_P1588_MPLS_LABEL_MASK_9r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_MASK_9.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_MASK_9r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_mask_9[1];
	uint32_t _p1588_mpls_label_mask_9;
} PLP_P1588_MPLS_LABEL_MASK_9r_t;

#define PLP_P1588_MPLS_LABEL_MASK_9r_CLR(r) (r).p1588_mpls_label_mask_9[0] = 0
#define PLP_P1588_MPLS_LABEL_MASK_9r_SET(r,d) (r).p1588_mpls_label_mask_9[0] = d
#define PLP_P1588_MPLS_LABEL_MASK_9r_GET(r) (r).p1588_mpls_label_mask_9[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_MASK_9r_MPLS_LABEL9_MASK_15_0f_GET(r) (((r).p1588_mpls_label_mask_9[0]) & 0xffff)
#define PLP_P1588_MPLS_LABEL_MASK_9r_MPLS_LABEL9_MASK_15_0f_SET(r,f) (r).p1588_mpls_label_mask_9[0]=(((r).p1588_mpls_label_mask_9[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_MASK_9.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_MASK_9r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_MASK_9r,(_r._p1588_mpls_label_mask_9))
#define PLP_WRITE_P1588_MPLS_LABEL_MASK_9r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_MASK_9r,(_r._p1588_mpls_label_mask_9))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_MASK_9r PLP_P1588_MPLS_LABEL_MASK_9r
#define P1588_MPLS_LABEL_MASK_9r_SIZE PLP_P1588_MPLS_LABEL_MASK_9r_SIZE
typedef PLP_P1588_MPLS_LABEL_MASK_9r_t P1588_MPLS_LABEL_MASK_9r_t;
#define P1588_MPLS_LABEL_MASK_9r_CLR PLP_P1588_MPLS_LABEL_MASK_9r_CLR
#define P1588_MPLS_LABEL_MASK_9r_SET PLP_P1588_MPLS_LABEL_MASK_9r_SET
#define P1588_MPLS_LABEL_MASK_9r_GET PLP_P1588_MPLS_LABEL_MASK_9r_GET
#define P1588_MPLS_LABEL_MASK_9r_MPLS_LABEL9_MASK_15_0f_GET PLP_P1588_MPLS_LABEL_MASK_9r_MPLS_LABEL9_MASK_15_0f_GET
#define P1588_MPLS_LABEL_MASK_9r_MPLS_LABEL9_MASK_15_0f_SET PLP_P1588_MPLS_LABEL_MASK_9r_MPLS_LABEL9_MASK_15_0f_SET
#define READ_P1588_MPLS_LABEL_MASK_9r PLP_READ_P1588_MPLS_LABEL_MASK_9r
#define WRITE_P1588_MPLS_LABEL_MASK_9r PLP_WRITE_P1588_MPLS_LABEL_MASK_9r
#define MODIFY_P1588_MPLS_LABEL_MASK_9r PLP_MODIFY_P1588_MPLS_LABEL_MASK_9r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_MASK_9r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_MASK_10
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x706a
 * DESC:     MPLS LABEL MASK 10 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL10_MASK_15_0 MPLS label 10 mask0 - Ignore for label match1 - Enable for label matchThis is rightfully an enable and not a mask
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_MASK_10r    0x0000006a

#define PLP_P1588_MPLS_LABEL_MASK_10r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_MASK_10.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_MASK_10r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_mask_10[1];
	uint32_t _p1588_mpls_label_mask_10;
} PLP_P1588_MPLS_LABEL_MASK_10r_t;

#define PLP_P1588_MPLS_LABEL_MASK_10r_CLR(r) (r).p1588_mpls_label_mask_10[0] = 0
#define PLP_P1588_MPLS_LABEL_MASK_10r_SET(r,d) (r).p1588_mpls_label_mask_10[0] = d
#define PLP_P1588_MPLS_LABEL_MASK_10r_GET(r) (r).p1588_mpls_label_mask_10[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_MASK_10r_MPLS_LABEL10_MASK_15_0f_GET(r) (((r).p1588_mpls_label_mask_10[0]) & 0xffff)
#define PLP_P1588_MPLS_LABEL_MASK_10r_MPLS_LABEL10_MASK_15_0f_SET(r,f) (r).p1588_mpls_label_mask_10[0]=(((r).p1588_mpls_label_mask_10[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_MASK_10.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_MASK_10r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_MASK_10r,(_r._p1588_mpls_label_mask_10))
#define PLP_WRITE_P1588_MPLS_LABEL_MASK_10r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_MASK_10r,(_r._p1588_mpls_label_mask_10))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_MASK_10r PLP_P1588_MPLS_LABEL_MASK_10r
#define P1588_MPLS_LABEL_MASK_10r_SIZE PLP_P1588_MPLS_LABEL_MASK_10r_SIZE
typedef PLP_P1588_MPLS_LABEL_MASK_10r_t P1588_MPLS_LABEL_MASK_10r_t;
#define P1588_MPLS_LABEL_MASK_10r_CLR PLP_P1588_MPLS_LABEL_MASK_10r_CLR
#define P1588_MPLS_LABEL_MASK_10r_SET PLP_P1588_MPLS_LABEL_MASK_10r_SET
#define P1588_MPLS_LABEL_MASK_10r_GET PLP_P1588_MPLS_LABEL_MASK_10r_GET
#define P1588_MPLS_LABEL_MASK_10r_MPLS_LABEL10_MASK_15_0f_GET PLP_P1588_MPLS_LABEL_MASK_10r_MPLS_LABEL10_MASK_15_0f_GET
#define P1588_MPLS_LABEL_MASK_10r_MPLS_LABEL10_MASK_15_0f_SET PLP_P1588_MPLS_LABEL_MASK_10r_MPLS_LABEL10_MASK_15_0f_SET
#define READ_P1588_MPLS_LABEL_MASK_10r PLP_READ_P1588_MPLS_LABEL_MASK_10r
#define WRITE_P1588_MPLS_LABEL_MASK_10r PLP_WRITE_P1588_MPLS_LABEL_MASK_10r
#define MODIFY_P1588_MPLS_LABEL_MASK_10r PLP_MODIFY_P1588_MPLS_LABEL_MASK_10r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_MASK_10r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_MASK_11
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x706b
 * DESC:     MPLS LABEL MASK 11 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL1_MASK_19_16 MPLS label 1 mask0 - Ignore for label match1 - Enable for label matchThis is rightfully an enable and not a mask
 *     MPLS_LABEL2_MASK_19_16 MPLS label 2 mask0 - Ignore for label match1 - Enable for label matchThis is rightfully an enable and not a mask
 *     MPLS_LABEL3_MASK_19_16 MPLS label 3 mask0 - Ignore for label match1 - Enable for label matchThis is rightfully an enable and not a mask
 *     MPLS_LABEL4_MASK_19_16 MPLS label 4 mask0 - Ignore for label match1 - Enable for label matchThis is rightfully an enable and not a mask
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_MASK_11r    0x0000006b

#define PLP_P1588_MPLS_LABEL_MASK_11r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_MASK_11.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_MASK_11r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_mask_11[1];
	uint32_t _p1588_mpls_label_mask_11;
} PLP_P1588_MPLS_LABEL_MASK_11r_t;

#define PLP_P1588_MPLS_LABEL_MASK_11r_CLR(r) (r).p1588_mpls_label_mask_11[0] = 0
#define PLP_P1588_MPLS_LABEL_MASK_11r_SET(r,d) (r).p1588_mpls_label_mask_11[0] = d
#define PLP_P1588_MPLS_LABEL_MASK_11r_GET(r) (r).p1588_mpls_label_mask_11[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL4_MASK_19_16f_GET(r) ((((r).p1588_mpls_label_mask_11[0]) >> 12) & 0xf)
#define PLP_P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL4_MASK_19_16f_SET(r,f) (r).p1588_mpls_label_mask_11[0]=(((r).p1588_mpls_label_mask_11[0] & ~((uint32_t)0xf << 12)) | ((((uint32_t)f) & 0xf) << 12)) | (15 << (16 + 12))
#define PLP_P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL3_MASK_19_16f_GET(r) ((((r).p1588_mpls_label_mask_11[0]) >> 8) & 0xf)
#define PLP_P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL3_MASK_19_16f_SET(r,f) (r).p1588_mpls_label_mask_11[0]=(((r).p1588_mpls_label_mask_11[0] & ~((uint32_t)0xf << 8)) | ((((uint32_t)f) & 0xf) << 8)) | (15 << (16 + 8))
#define PLP_P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL2_MASK_19_16f_GET(r) ((((r).p1588_mpls_label_mask_11[0]) >> 4) & 0xf)
#define PLP_P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL2_MASK_19_16f_SET(r,f) (r).p1588_mpls_label_mask_11[0]=(((r).p1588_mpls_label_mask_11[0] & ~((uint32_t)0xf << 4)) | ((((uint32_t)f) & 0xf) << 4)) | (15 << (16 + 4))
#define PLP_P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL1_MASK_19_16f_GET(r) (((r).p1588_mpls_label_mask_11[0]) & 0xf)
#define PLP_P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL1_MASK_19_16f_SET(r,f) (r).p1588_mpls_label_mask_11[0]=(((r).p1588_mpls_label_mask_11[0] & ~((uint32_t)0xf)) | (((uint32_t)f) & 0xf)) | (0xf << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_MASK_11.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_MASK_11r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_MASK_11r,(_r._p1588_mpls_label_mask_11))
#define PLP_WRITE_P1588_MPLS_LABEL_MASK_11r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_MASK_11r,(_r._p1588_mpls_label_mask_11))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_MASK_11r PLP_P1588_MPLS_LABEL_MASK_11r
#define P1588_MPLS_LABEL_MASK_11r_SIZE PLP_P1588_MPLS_LABEL_MASK_11r_SIZE
typedef PLP_P1588_MPLS_LABEL_MASK_11r_t P1588_MPLS_LABEL_MASK_11r_t;
#define P1588_MPLS_LABEL_MASK_11r_CLR PLP_P1588_MPLS_LABEL_MASK_11r_CLR
#define P1588_MPLS_LABEL_MASK_11r_SET PLP_P1588_MPLS_LABEL_MASK_11r_SET
#define P1588_MPLS_LABEL_MASK_11r_GET PLP_P1588_MPLS_LABEL_MASK_11r_GET
#define P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL4_MASK_19_16f_GET PLP_P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL4_MASK_19_16f_GET
#define P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL4_MASK_19_16f_SET PLP_P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL4_MASK_19_16f_SET
#define P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL3_MASK_19_16f_GET PLP_P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL3_MASK_19_16f_GET
#define P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL3_MASK_19_16f_SET PLP_P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL3_MASK_19_16f_SET
#define P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL2_MASK_19_16f_GET PLP_P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL2_MASK_19_16f_GET
#define P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL2_MASK_19_16f_SET PLP_P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL2_MASK_19_16f_SET
#define P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL1_MASK_19_16f_GET PLP_P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL1_MASK_19_16f_GET
#define P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL1_MASK_19_16f_SET PLP_P1588_MPLS_LABEL_MASK_11r_MPLS_LABEL1_MASK_19_16f_SET
#define READ_P1588_MPLS_LABEL_MASK_11r PLP_READ_P1588_MPLS_LABEL_MASK_11r
#define WRITE_P1588_MPLS_LABEL_MASK_11r PLP_WRITE_P1588_MPLS_LABEL_MASK_11r
#define MODIFY_P1588_MPLS_LABEL_MASK_11r PLP_MODIFY_P1588_MPLS_LABEL_MASK_11r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_MASK_11r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_MASK_12
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x706c
 * DESC:     MPLS LABEL MASK 12 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL5_MASK_19_16 MPLS label 5 mask0 - Ignore for label match1 - Enable for label matchThis is rightfully an enable and not a mask
 *     MPLS_LABEL6_MASK_19_16 MPLS label 6 mask0 - Ignore for label match1 - Enable for label matchThis is rightfully an enable and not a mask
 *     MPLS_LABEL7_MASK_19_16 MPLS label 7 mask0 - Ignore for label match1 - Enable for label matchThis is rightfully an enable and not a mask
 *     MPLS_LABEL8_MASK_19_16 MPLS label 8 mask0 - Ignore for label match1 - Enable for label matchThis is rightfully an enable and not a mask
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_MASK_12r    0x0000006c

#define PLP_P1588_MPLS_LABEL_MASK_12r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_MASK_12.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_MASK_12r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_mask_12[1];
	uint32_t _p1588_mpls_label_mask_12;
} PLP_P1588_MPLS_LABEL_MASK_12r_t;

#define PLP_P1588_MPLS_LABEL_MASK_12r_CLR(r) (r).p1588_mpls_label_mask_12[0] = 0
#define PLP_P1588_MPLS_LABEL_MASK_12r_SET(r,d) (r).p1588_mpls_label_mask_12[0] = d
#define PLP_P1588_MPLS_LABEL_MASK_12r_GET(r) (r).p1588_mpls_label_mask_12[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL8_MASK_19_16f_GET(r) ((((r).p1588_mpls_label_mask_12[0]) >> 12) & 0xf)
#define PLP_P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL8_MASK_19_16f_SET(r,f) (r).p1588_mpls_label_mask_12[0]=(((r).p1588_mpls_label_mask_12[0] & ~((uint32_t)0xf << 12)) | ((((uint32_t)f) & 0xf) << 12)) | (15 << (16 + 12))
#define PLP_P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL7_MASK_19_16f_GET(r) ((((r).p1588_mpls_label_mask_12[0]) >> 8) & 0xf)
#define PLP_P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL7_MASK_19_16f_SET(r,f) (r).p1588_mpls_label_mask_12[0]=(((r).p1588_mpls_label_mask_12[0] & ~((uint32_t)0xf << 8)) | ((((uint32_t)f) & 0xf) << 8)) | (15 << (16 + 8))
#define PLP_P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL6_MASK_19_16f_GET(r) ((((r).p1588_mpls_label_mask_12[0]) >> 4) & 0xf)
#define PLP_P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL6_MASK_19_16f_SET(r,f) (r).p1588_mpls_label_mask_12[0]=(((r).p1588_mpls_label_mask_12[0] & ~((uint32_t)0xf << 4)) | ((((uint32_t)f) & 0xf) << 4)) | (15 << (16 + 4))
#define PLP_P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL5_MASK_19_16f_GET(r) (((r).p1588_mpls_label_mask_12[0]) & 0xf)
#define PLP_P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL5_MASK_19_16f_SET(r,f) (r).p1588_mpls_label_mask_12[0]=(((r).p1588_mpls_label_mask_12[0] & ~((uint32_t)0xf)) | (((uint32_t)f) & 0xf)) | (0xf << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_MASK_12.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_MASK_12r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_MASK_12r,(_r._p1588_mpls_label_mask_12))
#define PLP_WRITE_P1588_MPLS_LABEL_MASK_12r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_MASK_12r,(_r._p1588_mpls_label_mask_12))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_MASK_12r PLP_P1588_MPLS_LABEL_MASK_12r
#define P1588_MPLS_LABEL_MASK_12r_SIZE PLP_P1588_MPLS_LABEL_MASK_12r_SIZE
typedef PLP_P1588_MPLS_LABEL_MASK_12r_t P1588_MPLS_LABEL_MASK_12r_t;
#define P1588_MPLS_LABEL_MASK_12r_CLR PLP_P1588_MPLS_LABEL_MASK_12r_CLR
#define P1588_MPLS_LABEL_MASK_12r_SET PLP_P1588_MPLS_LABEL_MASK_12r_SET
#define P1588_MPLS_LABEL_MASK_12r_GET PLP_P1588_MPLS_LABEL_MASK_12r_GET
#define P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL8_MASK_19_16f_GET PLP_P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL8_MASK_19_16f_GET
#define P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL8_MASK_19_16f_SET PLP_P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL8_MASK_19_16f_SET
#define P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL7_MASK_19_16f_GET PLP_P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL7_MASK_19_16f_GET
#define P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL7_MASK_19_16f_SET PLP_P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL7_MASK_19_16f_SET
#define P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL6_MASK_19_16f_GET PLP_P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL6_MASK_19_16f_GET
#define P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL6_MASK_19_16f_SET PLP_P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL6_MASK_19_16f_SET
#define P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL5_MASK_19_16f_GET PLP_P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL5_MASK_19_16f_GET
#define P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL5_MASK_19_16f_SET PLP_P1588_MPLS_LABEL_MASK_12r_MPLS_LABEL5_MASK_19_16f_SET
#define READ_P1588_MPLS_LABEL_MASK_12r PLP_READ_P1588_MPLS_LABEL_MASK_12r
#define WRITE_P1588_MPLS_LABEL_MASK_12r PLP_WRITE_P1588_MPLS_LABEL_MASK_12r
#define MODIFY_P1588_MPLS_LABEL_MASK_12r PLP_MODIFY_P1588_MPLS_LABEL_MASK_12r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_MASK_12r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_MASK_13
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x706d
 * DESC:     MPLS LABEL MASK 13 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL9_MASK_19_16 MPLS label 9 mask0 - Ignore for label match1 - Enable for label matchThis is rightfully an enable and not a mask
 *     MPLS_LABEL10_MASK_19_16 MPLS label 10 mask0 - Ignore for label match1 - Enable for label matchThis is rightfully an enable and not a mask
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_MASK_13r    0x0000006d

#define PLP_P1588_MPLS_LABEL_MASK_13r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_MASK_13.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_MASK_13r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_mask_13[1];
	uint32_t _p1588_mpls_label_mask_13;
} PLP_P1588_MPLS_LABEL_MASK_13r_t;

#define PLP_P1588_MPLS_LABEL_MASK_13r_CLR(r) (r).p1588_mpls_label_mask_13[0] = 0
#define PLP_P1588_MPLS_LABEL_MASK_13r_SET(r,d) (r).p1588_mpls_label_mask_13[0] = d
#define PLP_P1588_MPLS_LABEL_MASK_13r_GET(r) (r).p1588_mpls_label_mask_13[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_MASK_13r_MPLS_LABEL10_MASK_19_16f_GET(r) ((((r).p1588_mpls_label_mask_13[0]) >> 4) & 0xf)
#define PLP_P1588_MPLS_LABEL_MASK_13r_MPLS_LABEL10_MASK_19_16f_SET(r,f) (r).p1588_mpls_label_mask_13[0]=(((r).p1588_mpls_label_mask_13[0] & ~((uint32_t)0xf << 4)) | ((((uint32_t)f) & 0xf) << 4)) | (15 << (16 + 4))
#define PLP_P1588_MPLS_LABEL_MASK_13r_MPLS_LABEL9_MASK_19_16f_GET(r) (((r).p1588_mpls_label_mask_13[0]) & 0xf)
#define PLP_P1588_MPLS_LABEL_MASK_13r_MPLS_LABEL9_MASK_19_16f_SET(r,f) (r).p1588_mpls_label_mask_13[0]=(((r).p1588_mpls_label_mask_13[0] & ~((uint32_t)0xf)) | (((uint32_t)f) & 0xf)) | (0xf << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_MASK_13.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_MASK_13r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_MASK_13r,(_r._p1588_mpls_label_mask_13))
#define PLP_WRITE_P1588_MPLS_LABEL_MASK_13r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_MASK_13r,(_r._p1588_mpls_label_mask_13))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_MASK_13r PLP_P1588_MPLS_LABEL_MASK_13r
#define P1588_MPLS_LABEL_MASK_13r_SIZE PLP_P1588_MPLS_LABEL_MASK_13r_SIZE
typedef PLP_P1588_MPLS_LABEL_MASK_13r_t P1588_MPLS_LABEL_MASK_13r_t;
#define P1588_MPLS_LABEL_MASK_13r_CLR PLP_P1588_MPLS_LABEL_MASK_13r_CLR
#define P1588_MPLS_LABEL_MASK_13r_SET PLP_P1588_MPLS_LABEL_MASK_13r_SET
#define P1588_MPLS_LABEL_MASK_13r_GET PLP_P1588_MPLS_LABEL_MASK_13r_GET
#define P1588_MPLS_LABEL_MASK_13r_MPLS_LABEL10_MASK_19_16f_GET PLP_P1588_MPLS_LABEL_MASK_13r_MPLS_LABEL10_MASK_19_16f_GET
#define P1588_MPLS_LABEL_MASK_13r_MPLS_LABEL10_MASK_19_16f_SET PLP_P1588_MPLS_LABEL_MASK_13r_MPLS_LABEL10_MASK_19_16f_SET
#define P1588_MPLS_LABEL_MASK_13r_MPLS_LABEL9_MASK_19_16f_GET PLP_P1588_MPLS_LABEL_MASK_13r_MPLS_LABEL9_MASK_19_16f_GET
#define P1588_MPLS_LABEL_MASK_13r_MPLS_LABEL9_MASK_19_16f_SET PLP_P1588_MPLS_LABEL_MASK_13r_MPLS_LABEL9_MASK_19_16f_SET
#define READ_P1588_MPLS_LABEL_MASK_13r PLP_READ_P1588_MPLS_LABEL_MASK_13r
#define WRITE_P1588_MPLS_LABEL_MASK_13r PLP_WRITE_P1588_MPLS_LABEL_MASK_13r
#define MODIFY_P1588_MPLS_LABEL_MASK_13r PLP_MODIFY_P1588_MPLS_LABEL_MASK_13r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_MASK_13r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_DIR_1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x706e
 * DESC:     MPLS LABEL DIR 1 Register
 * RESETVAL: 0xffff (65535)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL1_DIR  MPLS label 1 dir[1] - Applies to Egress label match[0] - Applies to Ingress label matchIn 1G code, this is mpls_value_enable
 *     MPLS_LABEL2_DIR  MPLS label 2 dir[3] - Applies to Egress label match[2] - Applies to Ingress label matchIn 1G code, this is mpls_value_enable
 *     MPLS_LABEL3_DIR  MPLS label 3 dir[5] - Applies to Egress label match[4] - Applies to Ingress label matchIn 1G code, this is mpls_value_enable
 *     MPLS_LABEL4_DIR  MPLS label 4 dir[7] - Applies to Egress label match[6] - Applies to Ingress label matchIn 1G code, this is mpls_value_enable
 *     MPLS_LABEL5_DIR  MPLS label 5 dir[9] - Applies to Egress label match[8] - Applies to Ingress label matchIn 1G code, this is mpls_value_enable
 *     MPLS_LABEL6_DIR  MPLS label 6 dir[11] - Applies to Egress label match[10] - Applies to Ingress label matchIn 1G code, this is mpls_value_enable
 *     MPLS_LABEL7_DIR  MPLS label 7 dir[13] - Applies to Egress label match[12] - Applies to Ingress label matchIn 1G code, this is mpls_value_enable
 *     MPLS_LABEL8_DIR  MPLS label 8 dir[15] - Applies to Egress label match[14] - Applies to Ingress label matchIn 1G code, this is mpls_value_enable
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_DIR_1r    0x0000006e

#define PLP_P1588_MPLS_LABEL_DIR_1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_DIR_1.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_DIR_1r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_dir_1[1];
	uint32_t _p1588_mpls_label_dir_1;
} PLP_P1588_MPLS_LABEL_DIR_1r_t;

#define PLP_P1588_MPLS_LABEL_DIR_1r_CLR(r) (r).p1588_mpls_label_dir_1[0] = 0
#define PLP_P1588_MPLS_LABEL_DIR_1r_SET(r,d) (r).p1588_mpls_label_dir_1[0] = d
#define PLP_P1588_MPLS_LABEL_DIR_1r_GET(r) (r).p1588_mpls_label_dir_1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL8_DIRf_GET(r) ((((r).p1588_mpls_label_dir_1[0]) >> 14) & 0x3)
#define PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL8_DIRf_SET(r,f) (r).p1588_mpls_label_dir_1[0]=(((r).p1588_mpls_label_dir_1[0] & ~((uint32_t)0x3 << 14)) | ((((uint32_t)f) & 0x3) << 14)) | (3 << (16 + 14))
#define PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL7_DIRf_GET(r) ((((r).p1588_mpls_label_dir_1[0]) >> 12) & 0x3)
#define PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL7_DIRf_SET(r,f) (r).p1588_mpls_label_dir_1[0]=(((r).p1588_mpls_label_dir_1[0] & ~((uint32_t)0x3 << 12)) | ((((uint32_t)f) & 0x3) << 12)) | (3 << (16 + 12))
#define PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL6_DIRf_GET(r) ((((r).p1588_mpls_label_dir_1[0]) >> 10) & 0x3)
#define PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL6_DIRf_SET(r,f) (r).p1588_mpls_label_dir_1[0]=(((r).p1588_mpls_label_dir_1[0] & ~((uint32_t)0x3 << 10)) | ((((uint32_t)f) & 0x3) << 10)) | (3 << (16 + 10))
#define PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL5_DIRf_GET(r) ((((r).p1588_mpls_label_dir_1[0]) >> 8) & 0x3)
#define PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL5_DIRf_SET(r,f) (r).p1588_mpls_label_dir_1[0]=(((r).p1588_mpls_label_dir_1[0] & ~((uint32_t)0x3 << 8)) | ((((uint32_t)f) & 0x3) << 8)) | (3 << (16 + 8))
#define PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL4_DIRf_GET(r) ((((r).p1588_mpls_label_dir_1[0]) >> 6) & 0x3)
#define PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL4_DIRf_SET(r,f) (r).p1588_mpls_label_dir_1[0]=(((r).p1588_mpls_label_dir_1[0] & ~((uint32_t)0x3 << 6)) | ((((uint32_t)f) & 0x3) << 6)) | (3 << (16 + 6))
#define PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL3_DIRf_GET(r) ((((r).p1588_mpls_label_dir_1[0]) >> 4) & 0x3)
#define PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL3_DIRf_SET(r,f) (r).p1588_mpls_label_dir_1[0]=(((r).p1588_mpls_label_dir_1[0] & ~((uint32_t)0x3 << 4)) | ((((uint32_t)f) & 0x3) << 4)) | (3 << (16 + 4))
#define PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL2_DIRf_GET(r) ((((r).p1588_mpls_label_dir_1[0]) >> 2) & 0x3)
#define PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL2_DIRf_SET(r,f) (r).p1588_mpls_label_dir_1[0]=(((r).p1588_mpls_label_dir_1[0] & ~((uint32_t)0x3 << 2)) | ((((uint32_t)f) & 0x3) << 2)) | (3 << (16 + 2))
#define PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL1_DIRf_GET(r) (((r).p1588_mpls_label_dir_1[0]) & 0x3)
#define PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL1_DIRf_SET(r,f) (r).p1588_mpls_label_dir_1[0]=(((r).p1588_mpls_label_dir_1[0] & ~((uint32_t)0x3)) | (((uint32_t)f) & 0x3)) | (0x3 << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_DIR_1.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_DIR_1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_DIR_1r,(_r._p1588_mpls_label_dir_1))
#define PLP_WRITE_P1588_MPLS_LABEL_DIR_1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_DIR_1r,(_r._p1588_mpls_label_dir_1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_DIR_1r PLP_P1588_MPLS_LABEL_DIR_1r
#define P1588_MPLS_LABEL_DIR_1r_SIZE PLP_P1588_MPLS_LABEL_DIR_1r_SIZE
typedef PLP_P1588_MPLS_LABEL_DIR_1r_t P1588_MPLS_LABEL_DIR_1r_t;
#define P1588_MPLS_LABEL_DIR_1r_CLR PLP_P1588_MPLS_LABEL_DIR_1r_CLR
#define P1588_MPLS_LABEL_DIR_1r_SET PLP_P1588_MPLS_LABEL_DIR_1r_SET
#define P1588_MPLS_LABEL_DIR_1r_GET PLP_P1588_MPLS_LABEL_DIR_1r_GET
#define P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL8_DIRf_GET PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL8_DIRf_GET
#define P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL8_DIRf_SET PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL8_DIRf_SET
#define P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL7_DIRf_GET PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL7_DIRf_GET
#define P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL7_DIRf_SET PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL7_DIRf_SET
#define P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL6_DIRf_GET PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL6_DIRf_GET
#define P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL6_DIRf_SET PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL6_DIRf_SET
#define P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL5_DIRf_GET PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL5_DIRf_GET
#define P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL5_DIRf_SET PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL5_DIRf_SET
#define P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL4_DIRf_GET PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL4_DIRf_GET
#define P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL4_DIRf_SET PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL4_DIRf_SET
#define P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL3_DIRf_GET PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL3_DIRf_GET
#define P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL3_DIRf_SET PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL3_DIRf_SET
#define P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL2_DIRf_GET PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL2_DIRf_GET
#define P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL2_DIRf_SET PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL2_DIRf_SET
#define P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL1_DIRf_GET PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL1_DIRf_GET
#define P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL1_DIRf_SET PLP_P1588_MPLS_LABEL_DIR_1r_MPLS_LABEL1_DIRf_SET
#define READ_P1588_MPLS_LABEL_DIR_1r PLP_READ_P1588_MPLS_LABEL_DIR_1r
#define WRITE_P1588_MPLS_LABEL_DIR_1r PLP_WRITE_P1588_MPLS_LABEL_DIR_1r
#define MODIFY_P1588_MPLS_LABEL_DIR_1r PLP_MODIFY_P1588_MPLS_LABEL_DIR_1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_DIR_1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MPLS_LABEL_DIR_2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x706f
 * DESC:     MPLS LABEL DIR 2 Register
 * RESETVAL: 0xf (15)
 * ACCESS:   R/W
 * FIELDS:
 *     MPLS_LABEL9_DIR  MPLS label 9 dir[1] - Applies to Egress label match[0] - Applies to Ingress label matchIn 1G code, this is mpls_value_enable
 *     MPLS_LABEL10_DIR MPLS label 10 dir[3] - Applies to Egress label match[2] - Applies to Ingress label matchIn 1G code, this is mpls_value_enable
 *
 ******************************************************************************/
#define PLP_P1588_MPLS_LABEL_DIR_2r    0x0000006f

#define PLP_P1588_MPLS_LABEL_DIR_2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_MPLS_LABEL_DIR_2.
 *
 */
typedef union PLP_P1588_MPLS_LABEL_DIR_2r_s {
	uint32_t v[1];
	uint32_t p1588_mpls_label_dir_2[1];
	uint32_t _p1588_mpls_label_dir_2;
} PLP_P1588_MPLS_LABEL_DIR_2r_t;

#define PLP_P1588_MPLS_LABEL_DIR_2r_CLR(r) (r).p1588_mpls_label_dir_2[0] = 0
#define PLP_P1588_MPLS_LABEL_DIR_2r_SET(r,d) (r).p1588_mpls_label_dir_2[0] = d
#define PLP_P1588_MPLS_LABEL_DIR_2r_GET(r) (r).p1588_mpls_label_dir_2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MPLS_LABEL_DIR_2r_MPLS_LABEL10_DIRf_GET(r) ((((r).p1588_mpls_label_dir_2[0]) >> 2) & 0x3)
#define PLP_P1588_MPLS_LABEL_DIR_2r_MPLS_LABEL10_DIRf_SET(r,f) (r).p1588_mpls_label_dir_2[0]=(((r).p1588_mpls_label_dir_2[0] & ~((uint32_t)0x3 << 2)) | ((((uint32_t)f) & 0x3) << 2)) | (3 << (16 + 2))
#define PLP_P1588_MPLS_LABEL_DIR_2r_MPLS_LABEL9_DIRf_GET(r) (((r).p1588_mpls_label_dir_2[0]) & 0x3)
#define PLP_P1588_MPLS_LABEL_DIR_2r_MPLS_LABEL9_DIRf_SET(r,f) (r).p1588_mpls_label_dir_2[0]=(((r).p1588_mpls_label_dir_2[0] & ~((uint32_t)0x3)) | (((uint32_t)f) & 0x3)) | (0x3 << 16)

/*
 * These macros can be used to access P1588_MPLS_LABEL_DIR_2.
 *
 */
#define PLP_READ_P1588_MPLS_LABEL_DIR_2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MPLS_LABEL_DIR_2r,(_r._p1588_mpls_label_dir_2))
#define PLP_WRITE_P1588_MPLS_LABEL_DIR_2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MPLS_LABEL_DIR_2r,(_r._p1588_mpls_label_dir_2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MPLS_LABEL_DIR_2r PLP_P1588_MPLS_LABEL_DIR_2r
#define P1588_MPLS_LABEL_DIR_2r_SIZE PLP_P1588_MPLS_LABEL_DIR_2r_SIZE
typedef PLP_P1588_MPLS_LABEL_DIR_2r_t P1588_MPLS_LABEL_DIR_2r_t;
#define P1588_MPLS_LABEL_DIR_2r_CLR PLP_P1588_MPLS_LABEL_DIR_2r_CLR
#define P1588_MPLS_LABEL_DIR_2r_SET PLP_P1588_MPLS_LABEL_DIR_2r_SET
#define P1588_MPLS_LABEL_DIR_2r_GET PLP_P1588_MPLS_LABEL_DIR_2r_GET
#define P1588_MPLS_LABEL_DIR_2r_MPLS_LABEL10_DIRf_GET PLP_P1588_MPLS_LABEL_DIR_2r_MPLS_LABEL10_DIRf_GET
#define P1588_MPLS_LABEL_DIR_2r_MPLS_LABEL10_DIRf_SET PLP_P1588_MPLS_LABEL_DIR_2r_MPLS_LABEL10_DIRf_SET
#define P1588_MPLS_LABEL_DIR_2r_MPLS_LABEL9_DIRf_GET PLP_P1588_MPLS_LABEL_DIR_2r_MPLS_LABEL9_DIRf_GET
#define P1588_MPLS_LABEL_DIR_2r_MPLS_LABEL9_DIRf_SET PLP_P1588_MPLS_LABEL_DIR_2r_MPLS_LABEL9_DIRf_SET
#define READ_P1588_MPLS_LABEL_DIR_2r PLP_READ_P1588_MPLS_LABEL_DIR_2r
#define WRITE_P1588_MPLS_LABEL_DIR_2r PLP_WRITE_P1588_MPLS_LABEL_DIR_2r
#define MODIFY_P1588_MPLS_LABEL_DIR_2r PLP_MODIFY_P1588_MPLS_LABEL_DIR_2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MPLS_LABEL_DIR_2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_DUAL_CLOCK_CONTROL_1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7070
 * DESC:     DUAL CLOCK CONTROL 1 Register
 * RESETVAL: 0x30 (48)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_MODE_CPU_ENABLE 1 - Enable Dual clock on RX0 - Disable Dual clock on RX
 *     TX_MODE_CPU_ENABLE 1 - Enable Dual clock on TX0 - Disable Dual clock on TX
 *     RX_MODE_CPU_TXRX_SEL 0 - Ingress Dest IP or MAC Address1 - Ingress Source IP or MAC Address
 *     TX_MODE_CPU_TXRX_SEL 0 - Egress Dest IP or MAC Address1 - Egress Source IP or MAC Address
 *     RX_MODE_CPU_MAC_IP_SEL 1 - Ingress MAC Address0 - Ingress IPv4 Address
 *     TX_MODE_CPU_MAC_IP_SEL 1 - Egress MAC Address0 - Egress IPv4 Address
 *     RX_MODE_47BIT    0 - Default correction field1 - Sign Extended CF, 64b
 *     TX_MODE_47BIT    0 - Default correction field1 - Sign Extended CF, 64b
 *
 ******************************************************************************/
#define PLP_P1588_DUAL_CLOCK_CONTROL_1r    0x00000070

#define PLP_P1588_DUAL_CLOCK_CONTROL_1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_DUAL_CLOCK_CONTROL_1.
 *
 */
typedef union PLP_P1588_DUAL_CLOCK_CONTROL_1r_s {
	uint32_t v[1];
	uint32_t p1588_dual_clock_control_1[1];
	uint32_t _p1588_dual_clock_control_1;
} PLP_P1588_DUAL_CLOCK_CONTROL_1r_t;

#define PLP_P1588_DUAL_CLOCK_CONTROL_1r_CLR(r) (r).p1588_dual_clock_control_1[0] = 0
#define PLP_P1588_DUAL_CLOCK_CONTROL_1r_SET(r,d) (r).p1588_dual_clock_control_1[0] = d
#define PLP_P1588_DUAL_CLOCK_CONTROL_1r_GET(r) (r).p1588_dual_clock_control_1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_47BITf_GET(r) ((((r).p1588_dual_clock_control_1[0]) >> 7) & 0x1)
#define PLP_P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_47BITf_SET(r,f) (r).p1588_dual_clock_control_1[0]=(((r).p1588_dual_clock_control_1[0] & ~((uint32_t)0x1 << 7)) | ((((uint32_t)f) & 0x1) << 7)) | (1 << (16 + 7))
#define PLP_P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_47BITf_GET(r) ((((r).p1588_dual_clock_control_1[0]) >> 6) & 0x1)
#define PLP_P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_47BITf_SET(r,f) (r).p1588_dual_clock_control_1[0]=(((r).p1588_dual_clock_control_1[0] & ~((uint32_t)0x1 << 6)) | ((((uint32_t)f) & 0x1) << 6)) | (1 << (16 + 6))
#define PLP_P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_CPU_MAC_IP_SELf_GET(r) ((((r).p1588_dual_clock_control_1[0]) >> 5) & 0x1)
#define PLP_P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_CPU_MAC_IP_SELf_SET(r,f) (r).p1588_dual_clock_control_1[0]=(((r).p1588_dual_clock_control_1[0] & ~((uint32_t)0x1 << 5)) | ((((uint32_t)f) & 0x1) << 5)) | (1 << (16 + 5))
#define PLP_P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_CPU_MAC_IP_SELf_GET(r) ((((r).p1588_dual_clock_control_1[0]) >> 4) & 0x1)
#define PLP_P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_CPU_MAC_IP_SELf_SET(r,f) (r).p1588_dual_clock_control_1[0]=(((r).p1588_dual_clock_control_1[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4)) | (1 << (16 + 4))
#define PLP_P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_CPU_TXRX_SELf_GET(r) ((((r).p1588_dual_clock_control_1[0]) >> 3) & 0x1)
#define PLP_P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_CPU_TXRX_SELf_SET(r,f) (r).p1588_dual_clock_control_1[0]=(((r).p1588_dual_clock_control_1[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3)) | (1 << (16 + 3))
#define PLP_P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_CPU_TXRX_SELf_GET(r) ((((r).p1588_dual_clock_control_1[0]) >> 2) & 0x1)
#define PLP_P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_CPU_TXRX_SELf_SET(r,f) (r).p1588_dual_clock_control_1[0]=(((r).p1588_dual_clock_control_1[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2)) | (1 << (16 + 2))
#define PLP_P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_CPU_ENABLEf_GET(r) ((((r).p1588_dual_clock_control_1[0]) >> 1) & 0x1)
#define PLP_P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_CPU_ENABLEf_SET(r,f) (r).p1588_dual_clock_control_1[0]=(((r).p1588_dual_clock_control_1[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))
#define PLP_P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_CPU_ENABLEf_GET(r) (((r).p1588_dual_clock_control_1[0]) & 0x1)
#define PLP_P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_CPU_ENABLEf_SET(r,f) (r).p1588_dual_clock_control_1[0]=(((r).p1588_dual_clock_control_1[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1)) | (0x1 << 16)

/*
 * These macros can be used to access P1588_DUAL_CLOCK_CONTROL_1.
 *
 */
#define PLP_READ_P1588_DUAL_CLOCK_CONTROL_1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_DUAL_CLOCK_CONTROL_1r,(_r._p1588_dual_clock_control_1))
#define PLP_WRITE_P1588_DUAL_CLOCK_CONTROL_1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_DUAL_CLOCK_CONTROL_1r,(_r._p1588_dual_clock_control_1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_DUAL_CLOCK_CONTROL_1r PLP_P1588_DUAL_CLOCK_CONTROL_1r
#define P1588_DUAL_CLOCK_CONTROL_1r_SIZE PLP_P1588_DUAL_CLOCK_CONTROL_1r_SIZE
typedef PLP_P1588_DUAL_CLOCK_CONTROL_1r_t P1588_DUAL_CLOCK_CONTROL_1r_t;
#define P1588_DUAL_CLOCK_CONTROL_1r_CLR PLP_P1588_DUAL_CLOCK_CONTROL_1r_CLR
#define P1588_DUAL_CLOCK_CONTROL_1r_SET PLP_P1588_DUAL_CLOCK_CONTROL_1r_SET
#define P1588_DUAL_CLOCK_CONTROL_1r_GET PLP_P1588_DUAL_CLOCK_CONTROL_1r_GET
#define P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_47BITf_GET PLP_P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_47BITf_GET
#define P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_47BITf_SET PLP_P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_47BITf_SET
#define P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_47BITf_GET PLP_P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_47BITf_GET
#define P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_47BITf_SET PLP_P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_47BITf_SET
#define P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_CPU_MAC_IP_SELf_GET PLP_P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_CPU_MAC_IP_SELf_GET
#define P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_CPU_MAC_IP_SELf_SET PLP_P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_CPU_MAC_IP_SELf_SET
#define P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_CPU_MAC_IP_SELf_GET PLP_P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_CPU_MAC_IP_SELf_GET
#define P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_CPU_MAC_IP_SELf_SET PLP_P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_CPU_MAC_IP_SELf_SET
#define P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_CPU_TXRX_SELf_GET PLP_P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_CPU_TXRX_SELf_GET
#define P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_CPU_TXRX_SELf_SET PLP_P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_CPU_TXRX_SELf_SET
#define P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_CPU_TXRX_SELf_GET PLP_P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_CPU_TXRX_SELf_GET
#define P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_CPU_TXRX_SELf_SET PLP_P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_CPU_TXRX_SELf_SET
#define P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_CPU_ENABLEf_GET PLP_P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_CPU_ENABLEf_GET
#define P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_CPU_ENABLEf_SET PLP_P1588_DUAL_CLOCK_CONTROL_1r_TX_MODE_CPU_ENABLEf_SET
#define P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_CPU_ENABLEf_GET PLP_P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_CPU_ENABLEf_GET
#define P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_CPU_ENABLEf_SET PLP_P1588_DUAL_CLOCK_CONTROL_1r_RX_MODE_CPU_ENABLEf_SET
#define READ_P1588_DUAL_CLOCK_CONTROL_1r PLP_READ_P1588_DUAL_CLOCK_CONTROL_1r
#define WRITE_P1588_DUAL_CLOCK_CONTROL_1r PLP_WRITE_P1588_DUAL_CLOCK_CONTROL_1r
#define MODIFY_P1588_DUAL_CLOCK_CONTROL_1r PLP_MODIFY_P1588_DUAL_CLOCK_CONTROL_1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_DUAL_CLOCK_CONTROL_1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_DUAL_CLOCK_CONTROL_2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7071
 * DESC:     DUAL CLOCK CONTROL 2 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_EVENT_SPARE   Reserved for Debug. Bit[0]=1 will turn on some detection of some packets in Ingress that may lead to corruption of data. Not to be used
 *     TX_EVENT_SPARE   Reserved for Debug. Bit[8]=1 will turn on some detection of some packets in Egress that may lead to corruption of data. Not to be used
 *
 ******************************************************************************/
#define PLP_P1588_DUAL_CLOCK_CONTROL_2r    0x00000071

#define PLP_P1588_DUAL_CLOCK_CONTROL_2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_DUAL_CLOCK_CONTROL_2.
 *
 */
typedef union PLP_P1588_DUAL_CLOCK_CONTROL_2r_s {
	uint32_t v[1];
	uint32_t p1588_dual_clock_control_2[1];
	uint32_t _p1588_dual_clock_control_2;
} PLP_P1588_DUAL_CLOCK_CONTROL_2r_t;

#define PLP_P1588_DUAL_CLOCK_CONTROL_2r_CLR(r) (r).p1588_dual_clock_control_2[0] = 0
#define PLP_P1588_DUAL_CLOCK_CONTROL_2r_SET(r,d) (r).p1588_dual_clock_control_2[0] = d
#define PLP_P1588_DUAL_CLOCK_CONTROL_2r_GET(r) (r).p1588_dual_clock_control_2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_DUAL_CLOCK_CONTROL_2r_TX_EVENT_SPAREf_GET(r) ((((r).p1588_dual_clock_control_2[0]) >> 8) & 0xff)
#define PLP_P1588_DUAL_CLOCK_CONTROL_2r_TX_EVENT_SPAREf_SET(r,f) (r).p1588_dual_clock_control_2[0]=(((r).p1588_dual_clock_control_2[0] & ~((uint32_t)0xff << 8)) | ((((uint32_t)f) & 0xff) << 8)) | (255 << (16 + 8))
#define PLP_P1588_DUAL_CLOCK_CONTROL_2r_RX_EVENT_SPAREf_GET(r) (((r).p1588_dual_clock_control_2[0]) & 0xff)
#define PLP_P1588_DUAL_CLOCK_CONTROL_2r_RX_EVENT_SPAREf_SET(r,f) (r).p1588_dual_clock_control_2[0]=(((r).p1588_dual_clock_control_2[0] & ~((uint32_t)0xff)) | (((uint32_t)f) & 0xff)) | (0xff << 16)

/*
 * These macros can be used to access P1588_DUAL_CLOCK_CONTROL_2.
 *
 */
#define PLP_READ_P1588_DUAL_CLOCK_CONTROL_2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_DUAL_CLOCK_CONTROL_2r,(_r._p1588_dual_clock_control_2))
#define PLP_WRITE_P1588_DUAL_CLOCK_CONTROL_2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_DUAL_CLOCK_CONTROL_2r,(_r._p1588_dual_clock_control_2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_DUAL_CLOCK_CONTROL_2r PLP_P1588_DUAL_CLOCK_CONTROL_2r
#define P1588_DUAL_CLOCK_CONTROL_2r_SIZE PLP_P1588_DUAL_CLOCK_CONTROL_2r_SIZE
typedef PLP_P1588_DUAL_CLOCK_CONTROL_2r_t P1588_DUAL_CLOCK_CONTROL_2r_t;
#define P1588_DUAL_CLOCK_CONTROL_2r_CLR PLP_P1588_DUAL_CLOCK_CONTROL_2r_CLR
#define P1588_DUAL_CLOCK_CONTROL_2r_SET PLP_P1588_DUAL_CLOCK_CONTROL_2r_SET
#define P1588_DUAL_CLOCK_CONTROL_2r_GET PLP_P1588_DUAL_CLOCK_CONTROL_2r_GET
#define P1588_DUAL_CLOCK_CONTROL_2r_TX_EVENT_SPAREf_GET PLP_P1588_DUAL_CLOCK_CONTROL_2r_TX_EVENT_SPAREf_GET
#define P1588_DUAL_CLOCK_CONTROL_2r_TX_EVENT_SPAREf_SET PLP_P1588_DUAL_CLOCK_CONTROL_2r_TX_EVENT_SPAREf_SET
#define P1588_DUAL_CLOCK_CONTROL_2r_RX_EVENT_SPAREf_GET PLP_P1588_DUAL_CLOCK_CONTROL_2r_RX_EVENT_SPAREf_GET
#define P1588_DUAL_CLOCK_CONTROL_2r_RX_EVENT_SPAREf_SET PLP_P1588_DUAL_CLOCK_CONTROL_2r_RX_EVENT_SPAREf_SET
#define READ_P1588_DUAL_CLOCK_CONTROL_2r PLP_READ_P1588_DUAL_CLOCK_CONTROL_2r
#define WRITE_P1588_DUAL_CLOCK_CONTROL_2r PLP_WRITE_P1588_DUAL_CLOCK_CONTROL_2r
#define MODIFY_P1588_DUAL_CLOCK_CONTROL_2r PLP_MODIFY_P1588_DUAL_CLOCK_CONTROL_2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_DUAL_CLOCK_CONTROL_2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_DUAL_CLOCK_CONTROL_3
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7072
 * DESC:     DUAL CLOCK CONTROL 3 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_TS_SPARE      [0] - Capture for Sync message[1] - Capture for Delay Req message[2] - Capture for PDelay Req message[3] - Capture for PDelay Resp message[4] - Capture for Sync message in Dual clock mode[5] - Capture for Delay Req message in Dual clock mode[6] - Capture for PDelay Req message in Dual clock mode[7] - Capture for PDelay Resp message in Dual clock mode
 *     TX_TS_SPARE      [8] - Capture for Sync message[9] - Capture for Delay Req message[10] - Capture for PDelay Req message[11] - Capture for PDelay Resp message[12] - Capture for Sync message in Dual clock mode[13] - Capture for Delay Req message in Dual clock mode[14] - Capture for PDelay Req message in Dual clock mode[15] - Capture for PDelay Resp message in Dual clock mode
 *
 ******************************************************************************/
#define PLP_P1588_DUAL_CLOCK_CONTROL_3r    0x00000072

#define PLP_P1588_DUAL_CLOCK_CONTROL_3r_SIZE 4

/*
 * This structure should be used to declare and program P1588_DUAL_CLOCK_CONTROL_3.
 *
 */
typedef union PLP_P1588_DUAL_CLOCK_CONTROL_3r_s {
	uint32_t v[1];
	uint32_t p1588_dual_clock_control_3[1];
	uint32_t _p1588_dual_clock_control_3;
} PLP_P1588_DUAL_CLOCK_CONTROL_3r_t;

#define PLP_P1588_DUAL_CLOCK_CONTROL_3r_CLR(r) (r).p1588_dual_clock_control_3[0] = 0
#define PLP_P1588_DUAL_CLOCK_CONTROL_3r_SET(r,d) (r).p1588_dual_clock_control_3[0] = d
#define PLP_P1588_DUAL_CLOCK_CONTROL_3r_GET(r) (r).p1588_dual_clock_control_3[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_DUAL_CLOCK_CONTROL_3r_TX_TS_SPAREf_GET(r) ((((r).p1588_dual_clock_control_3[0]) >> 8) & 0xff)
#define PLP_P1588_DUAL_CLOCK_CONTROL_3r_TX_TS_SPAREf_SET(r,f) (r).p1588_dual_clock_control_3[0]=(((r).p1588_dual_clock_control_3[0] & ~((uint32_t)0xff << 8)) | ((((uint32_t)f) & 0xff) << 8)) | (255 << (16 + 8))
#define PLP_P1588_DUAL_CLOCK_CONTROL_3r_RX_TS_SPAREf_GET(r) (((r).p1588_dual_clock_control_3[0]) & 0xff)
#define PLP_P1588_DUAL_CLOCK_CONTROL_3r_RX_TS_SPAREf_SET(r,f) (r).p1588_dual_clock_control_3[0]=(((r).p1588_dual_clock_control_3[0] & ~((uint32_t)0xff)) | (((uint32_t)f) & 0xff)) | (0xff << 16)

/*
 * These macros can be used to access P1588_DUAL_CLOCK_CONTROL_3.
 *
 */
#define PLP_READ_P1588_DUAL_CLOCK_CONTROL_3r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_DUAL_CLOCK_CONTROL_3r,(_r._p1588_dual_clock_control_3))
#define PLP_WRITE_P1588_DUAL_CLOCK_CONTROL_3r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_DUAL_CLOCK_CONTROL_3r,(_r._p1588_dual_clock_control_3))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_DUAL_CLOCK_CONTROL_3r PLP_P1588_DUAL_CLOCK_CONTROL_3r
#define P1588_DUAL_CLOCK_CONTROL_3r_SIZE PLP_P1588_DUAL_CLOCK_CONTROL_3r_SIZE
typedef PLP_P1588_DUAL_CLOCK_CONTROL_3r_t P1588_DUAL_CLOCK_CONTROL_3r_t;
#define P1588_DUAL_CLOCK_CONTROL_3r_CLR PLP_P1588_DUAL_CLOCK_CONTROL_3r_CLR
#define P1588_DUAL_CLOCK_CONTROL_3r_SET PLP_P1588_DUAL_CLOCK_CONTROL_3r_SET
#define P1588_DUAL_CLOCK_CONTROL_3r_GET PLP_P1588_DUAL_CLOCK_CONTROL_3r_GET
#define P1588_DUAL_CLOCK_CONTROL_3r_TX_TS_SPAREf_GET PLP_P1588_DUAL_CLOCK_CONTROL_3r_TX_TS_SPAREf_GET
#define P1588_DUAL_CLOCK_CONTROL_3r_TX_TS_SPAREf_SET PLP_P1588_DUAL_CLOCK_CONTROL_3r_TX_TS_SPAREf_SET
#define P1588_DUAL_CLOCK_CONTROL_3r_RX_TS_SPAREf_GET PLP_P1588_DUAL_CLOCK_CONTROL_3r_RX_TS_SPAREf_GET
#define P1588_DUAL_CLOCK_CONTROL_3r_RX_TS_SPAREf_SET PLP_P1588_DUAL_CLOCK_CONTROL_3r_RX_TS_SPAREf_SET
#define READ_P1588_DUAL_CLOCK_CONTROL_3r PLP_READ_P1588_DUAL_CLOCK_CONTROL_3r
#define WRITE_P1588_DUAL_CLOCK_CONTROL_3r PLP_WRITE_P1588_DUAL_CLOCK_CONTROL_3r
#define MODIFY_P1588_DUAL_CLOCK_CONTROL_3r PLP_MODIFY_P1588_DUAL_CLOCK_CONTROL_3r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_DUAL_CLOCK_CONTROL_3r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7073
 * DESC:     DUAL CLOCK RX MAC IP ADDR 1 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_MPLS_SPARE_47_32 48b MAC address used for match in Dual Clock modeThis covers 47:32 of the MAC addressThe MAC address to match is stable before the mode is enabledIn 1G code, rx_mpls_spare[47:0] connects to {mpls_spare4,mpls_spare3,mpls_spare2}
 *
 ******************************************************************************/
#define PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r    0x00000073

#define PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1.
 *
 */
typedef union PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r_s {
	uint32_t v[1];
	uint32_t p1588_dual_clock_rx_mac_ip_addr_1[1];
	uint32_t _p1588_dual_clock_rx_mac_ip_addr_1;
} PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r_t;

#define PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r_CLR(r) (r).p1588_dual_clock_rx_mac_ip_addr_1[0] = 0
#define PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r_SET(r,d) (r).p1588_dual_clock_rx_mac_ip_addr_1[0] = d
#define PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r_GET(r) (r).p1588_dual_clock_rx_mac_ip_addr_1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r_RX_MPLS_SPARE_47_32f_GET(r) (((r).p1588_dual_clock_rx_mac_ip_addr_1[0]) & 0xffff)
#define PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r_RX_MPLS_SPARE_47_32f_SET(r,f) (r).p1588_dual_clock_rx_mac_ip_addr_1[0]=(((r).p1588_dual_clock_rx_mac_ip_addr_1[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1.
 *
 */
#define PLP_READ_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r,(_r._p1588_dual_clock_rx_mac_ip_addr_1))
#define PLP_WRITE_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r,(_r._p1588_dual_clock_rx_mac_ip_addr_1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r
#define P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r_SIZE PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r_SIZE
typedef PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r_t P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r_t;
#define P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r_CLR PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r_CLR
#define P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r_SET PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r_SET
#define P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r_GET PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r_GET
#define P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r_RX_MPLS_SPARE_47_32f_GET PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r_RX_MPLS_SPARE_47_32f_GET
#define P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r_RX_MPLS_SPARE_47_32f_SET PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r_RX_MPLS_SPARE_47_32f_SET
#define READ_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r PLP_READ_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r
#define WRITE_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r PLP_WRITE_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r
#define MODIFY_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r PLP_MODIFY_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7074
 * DESC:     DUAL CLOCK RX MAC IP ADDR 2 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_MPLS_SPARE_31_16 48b MAC or 32b IP address used for match in Dual Clock modeThis covers 31:16 of the MAC or IP addressThe MAC address to match is stable before the mode is enabledIn 1G code, rx_mpls_spare[47:0] connects to {mpls_spare4,mpls_spare3,mpls_spare2}
 *
 ******************************************************************************/
#define PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r    0x00000074

#define PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2.
 *
 */
typedef union PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r_s {
	uint32_t v[1];
	uint32_t p1588_dual_clock_rx_mac_ip_addr_2[1];
	uint32_t _p1588_dual_clock_rx_mac_ip_addr_2;
} PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r_t;

#define PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r_CLR(r) (r).p1588_dual_clock_rx_mac_ip_addr_2[0] = 0
#define PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r_SET(r,d) (r).p1588_dual_clock_rx_mac_ip_addr_2[0] = d
#define PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r_GET(r) (r).p1588_dual_clock_rx_mac_ip_addr_2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r_RX_MPLS_SPARE_31_16f_GET(r) (((r).p1588_dual_clock_rx_mac_ip_addr_2[0]) & 0xffff)
#define PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r_RX_MPLS_SPARE_31_16f_SET(r,f) (r).p1588_dual_clock_rx_mac_ip_addr_2[0]=(((r).p1588_dual_clock_rx_mac_ip_addr_2[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2.
 *
 */
#define PLP_READ_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r,(_r._p1588_dual_clock_rx_mac_ip_addr_2))
#define PLP_WRITE_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r,(_r._p1588_dual_clock_rx_mac_ip_addr_2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r
#define P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r_SIZE PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r_SIZE
typedef PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r_t P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r_t;
#define P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r_CLR PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r_CLR
#define P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r_SET PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r_SET
#define P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r_GET PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r_GET
#define P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r_RX_MPLS_SPARE_31_16f_GET PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r_RX_MPLS_SPARE_31_16f_GET
#define P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r_RX_MPLS_SPARE_31_16f_SET PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r_RX_MPLS_SPARE_31_16f_SET
#define READ_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r PLP_READ_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r
#define WRITE_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r PLP_WRITE_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r
#define MODIFY_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r PLP_MODIFY_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7075
 * DESC:     DUAL CLOCK RX MAC IP ADDR 3 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_MPLS_SPARE_15_0 48b MAC or 32b IP address used for match in Dual Clock modeThis covers 15:0 of the MAC or IP addressThe MAC address to match is stable before the mode is enabledIn 1G code, rx_mpls_spare[47:0] connects to {mpls_spare4,mpls_spare3,mpls_spare2}
 *
 ******************************************************************************/
#define PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r    0x00000075

#define PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r_SIZE 4

/*
 * This structure should be used to declare and program P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3.
 *
 */
typedef union PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r_s {
	uint32_t v[1];
	uint32_t p1588_dual_clock_rx_mac_ip_addr_3[1];
	uint32_t _p1588_dual_clock_rx_mac_ip_addr_3;
} PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r_t;

#define PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r_CLR(r) (r).p1588_dual_clock_rx_mac_ip_addr_3[0] = 0
#define PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r_SET(r,d) (r).p1588_dual_clock_rx_mac_ip_addr_3[0] = d
#define PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r_GET(r) (r).p1588_dual_clock_rx_mac_ip_addr_3[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r_RX_MPLS_SPARE_15_0f_GET(r) (((r).p1588_dual_clock_rx_mac_ip_addr_3[0]) & 0xffff)
#define PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r_RX_MPLS_SPARE_15_0f_SET(r,f) (r).p1588_dual_clock_rx_mac_ip_addr_3[0]=(((r).p1588_dual_clock_rx_mac_ip_addr_3[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3.
 *
 */
#define PLP_READ_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r,(_r._p1588_dual_clock_rx_mac_ip_addr_3))
#define PLP_WRITE_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r,(_r._p1588_dual_clock_rx_mac_ip_addr_3))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r
#define P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r_SIZE PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r_SIZE
typedef PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r_t P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r_t;
#define P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r_CLR PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r_CLR
#define P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r_SET PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r_SET
#define P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r_GET PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r_GET
#define P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r_RX_MPLS_SPARE_15_0f_GET PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r_RX_MPLS_SPARE_15_0f_GET
#define P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r_RX_MPLS_SPARE_15_0f_SET PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r_RX_MPLS_SPARE_15_0f_SET
#define READ_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r PLP_READ_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r
#define WRITE_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r PLP_WRITE_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r
#define MODIFY_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r PLP_MODIFY_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_DUAL_CLOCK_RX_MAC_IP_ADDR_3r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7076
 * DESC:     DUAL CLOCK TX MAC IP ADDR 1 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_MPLS_SPARE_47_32 48b MAC or 32b IP address used for match in Dual Clock modeThis covers 47:32 of the MAC addressThe MAC address to match is stable before the mode is enabledIn 1G code, tx_mpls_spare[47:0] connects to {mpls_spare4,mpls_spare3,mpls_spare2}
 *
 ******************************************************************************/
#define PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r    0x00000076

#define PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1.
 *
 */
typedef union PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r_s {
	uint32_t v[1];
	uint32_t p1588_dual_clock_tx_mac_ip_addr_1[1];
	uint32_t _p1588_dual_clock_tx_mac_ip_addr_1;
} PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r_t;

#define PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r_CLR(r) (r).p1588_dual_clock_tx_mac_ip_addr_1[0] = 0
#define PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r_SET(r,d) (r).p1588_dual_clock_tx_mac_ip_addr_1[0] = d
#define PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r_GET(r) (r).p1588_dual_clock_tx_mac_ip_addr_1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r_TX_MPLS_SPARE_47_32f_GET(r) (((r).p1588_dual_clock_tx_mac_ip_addr_1[0]) & 0xffff)
#define PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r_TX_MPLS_SPARE_47_32f_SET(r,f) (r).p1588_dual_clock_tx_mac_ip_addr_1[0]=(((r).p1588_dual_clock_tx_mac_ip_addr_1[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1.
 *
 */
#define PLP_READ_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r,(_r._p1588_dual_clock_tx_mac_ip_addr_1))
#define PLP_WRITE_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r,(_r._p1588_dual_clock_tx_mac_ip_addr_1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r
#define P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r_SIZE PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r_SIZE
typedef PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r_t P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r_t;
#define P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r_CLR PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r_CLR
#define P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r_SET PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r_SET
#define P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r_GET PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r_GET
#define P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r_TX_MPLS_SPARE_47_32f_GET PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r_TX_MPLS_SPARE_47_32f_GET
#define P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r_TX_MPLS_SPARE_47_32f_SET PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r_TX_MPLS_SPARE_47_32f_SET
#define READ_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r PLP_READ_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r
#define WRITE_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r PLP_WRITE_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r
#define MODIFY_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r PLP_MODIFY_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7077
 * DESC:     DUAL CLOCK TX MAC IP ADDR 2 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_MPLS_SPARE_31_16 48b MAC or 32b IP address used for match in Dual Clock modeThis covers 31:16 of the MAC or IP addressThe MAC address to match is stable before the mode is enabledIn 1G code, tx_mpls_spare[47:0] connects to {mpls_spare4,mpls_spare3,mpls_spare2}
 *
 ******************************************************************************/
#define PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r    0x00000077

#define PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2.
 *
 */
typedef union PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r_s {
	uint32_t v[1];
	uint32_t p1588_dual_clock_tx_mac_ip_addr_2[1];
	uint32_t _p1588_dual_clock_tx_mac_ip_addr_2;
} PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r_t;

#define PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r_CLR(r) (r).p1588_dual_clock_tx_mac_ip_addr_2[0] = 0
#define PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r_SET(r,d) (r).p1588_dual_clock_tx_mac_ip_addr_2[0] = d
#define PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r_GET(r) (r).p1588_dual_clock_tx_mac_ip_addr_2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r_TX_MPLS_SPARE_31_16f_GET(r) (((r).p1588_dual_clock_tx_mac_ip_addr_2[0]) & 0xffff)
#define PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r_TX_MPLS_SPARE_31_16f_SET(r,f) (r).p1588_dual_clock_tx_mac_ip_addr_2[0]=(((r).p1588_dual_clock_tx_mac_ip_addr_2[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2.
 *
 */
#define PLP_READ_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r,(_r._p1588_dual_clock_tx_mac_ip_addr_2))
#define PLP_WRITE_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r,(_r._p1588_dual_clock_tx_mac_ip_addr_2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r
#define P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r_SIZE PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r_SIZE
typedef PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r_t P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r_t;
#define P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r_CLR PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r_CLR
#define P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r_SET PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r_SET
#define P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r_GET PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r_GET
#define P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r_TX_MPLS_SPARE_31_16f_GET PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r_TX_MPLS_SPARE_31_16f_GET
#define P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r_TX_MPLS_SPARE_31_16f_SET PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r_TX_MPLS_SPARE_31_16f_SET
#define READ_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r PLP_READ_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r
#define WRITE_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r PLP_WRITE_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r
#define MODIFY_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r PLP_MODIFY_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7078
 * DESC:     DUAL CLOCK TX MAC IP ADDR 3 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_MPLS_SPARE_15_0 48b MAC or 32b IP address used for match in Dual Clock modeThis covers 15:0 of the MAC or IP addressThe MAC address to match is stable before the mode is enabledIn 1G code, tx_mpls_spare[47:0] connects to {mpls_spare4,mpls_spare3,mpls_spare2}
 *
 ******************************************************************************/
#define PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r    0x00000078

#define PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r_SIZE 4

/*
 * This structure should be used to declare and program P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3.
 *
 */
typedef union PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r_s {
	uint32_t v[1];
	uint32_t p1588_dual_clock_tx_mac_ip_addr_3[1];
	uint32_t _p1588_dual_clock_tx_mac_ip_addr_3;
} PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r_t;

#define PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r_CLR(r) (r).p1588_dual_clock_tx_mac_ip_addr_3[0] = 0
#define PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r_SET(r,d) (r).p1588_dual_clock_tx_mac_ip_addr_3[0] = d
#define PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r_GET(r) (r).p1588_dual_clock_tx_mac_ip_addr_3[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r_TX_MPLS_SPARE_15_0f_GET(r) (((r).p1588_dual_clock_tx_mac_ip_addr_3[0]) & 0xffff)
#define PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r_TX_MPLS_SPARE_15_0f_SET(r,f) (r).p1588_dual_clock_tx_mac_ip_addr_3[0]=(((r).p1588_dual_clock_tx_mac_ip_addr_3[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3.
 *
 */
#define PLP_READ_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r,(_r._p1588_dual_clock_tx_mac_ip_addr_3))
#define PLP_WRITE_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r,(_r._p1588_dual_clock_tx_mac_ip_addr_3))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r
#define P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r_SIZE PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r_SIZE
typedef PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r_t P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r_t;
#define P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r_CLR PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r_CLR
#define P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r_SET PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r_SET
#define P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r_GET PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r_GET
#define P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r_TX_MPLS_SPARE_15_0f_GET PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r_TX_MPLS_SPARE_15_0f_GET
#define P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r_TX_MPLS_SPARE_15_0f_SET PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r_TX_MPLS_SPARE_15_0f_SET
#define READ_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r PLP_READ_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r
#define WRITE_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r PLP_WRITE_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r
#define MODIFY_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r PLP_MODIFY_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_DUAL_CLOCK_TX_MAC_IP_ADDR_3r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TX_TS_VAR_CONTROL
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7079
 * DESC:     TX TS VARIATION ADJUSTMENT CONTROL Register
 * RESETVAL: 0x400 (1024)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_TS_VAR_CONTROL this register control the variation adjustment relate to odd and even disparity in 1G trafficbit[0] - 1= add 1 clock cycles to the captured TS 80 bit when detect compensation is neccessarybit[1] - 1= sub 1 clock cycles to the captured TS 80 bit when detect compensation is neccessarybit[2] - 1= add 1 clock cycles to the captured TS 64 bit when detect compensation is neccessarybit[3] - 1= sub 1 clock cycles to the captured TS 64 bit when detect compensation is neccessarybit[4] - 1= add 1 clock cycles to the captured TS 48 bit when detect compensation is neccessarybit[5] - 1= sub 1 clock cycles to the captured TS 48 bit when detect compensation is neccessarybit[6] - 1= enable software compensation overridebit[15:7] - number of ns to compensate. default 8 ns
 *
 ******************************************************************************/
#define PLP_P1588_TX_TS_VAR_CONTROLr    0x00000079

#define PLP_P1588_TX_TS_VAR_CONTROLr_SIZE 4

/*
 * This structure should be used to declare and program P1588_TX_TS_VAR_CONTROL.
 *
 */
typedef union PLP_P1588_TX_TS_VAR_CONTROLr_s {
	uint32_t v[1];
	uint32_t p1588_tx_ts_var_control[1];
	uint32_t _p1588_tx_ts_var_control;
} PLP_P1588_TX_TS_VAR_CONTROLr_t;

#define PLP_P1588_TX_TS_VAR_CONTROLr_CLR(r) (r).p1588_tx_ts_var_control[0] = 0
#define PLP_P1588_TX_TS_VAR_CONTROLr_SET(r,d) (r).p1588_tx_ts_var_control[0] = d
#define PLP_P1588_TX_TS_VAR_CONTROLr_GET(r) (r).p1588_tx_ts_var_control[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TX_TS_VAR_CONTROLr_TX_TS_VAR_CONTROLf_GET(r) (((r).p1588_tx_ts_var_control[0]) & 0xffff)
#define PLP_P1588_TX_TS_VAR_CONTROLr_TX_TS_VAR_CONTROLf_SET(r,f) (r).p1588_tx_ts_var_control[0]=(((r).p1588_tx_ts_var_control[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TX_TS_VAR_CONTROL.
 *
 */
#define PLP_READ_P1588_TX_TS_VAR_CONTROLr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TX_TS_VAR_CONTROLr,(_r._p1588_tx_ts_var_control))
#define PLP_WRITE_P1588_TX_TS_VAR_CONTROLr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TX_TS_VAR_CONTROLr,(_r._p1588_tx_ts_var_control))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TX_TS_VAR_CONTROLr PLP_P1588_TX_TS_VAR_CONTROLr
#define P1588_TX_TS_VAR_CONTROLr_SIZE PLP_P1588_TX_TS_VAR_CONTROLr_SIZE
typedef PLP_P1588_TX_TS_VAR_CONTROLr_t P1588_TX_TS_VAR_CONTROLr_t;
#define P1588_TX_TS_VAR_CONTROLr_CLR PLP_P1588_TX_TS_VAR_CONTROLr_CLR
#define P1588_TX_TS_VAR_CONTROLr_SET PLP_P1588_TX_TS_VAR_CONTROLr_SET
#define P1588_TX_TS_VAR_CONTROLr_GET PLP_P1588_TX_TS_VAR_CONTROLr_GET
#define P1588_TX_TS_VAR_CONTROLr_TX_TS_VAR_CONTROLf_GET PLP_P1588_TX_TS_VAR_CONTROLr_TX_TS_VAR_CONTROLf_GET
#define P1588_TX_TS_VAR_CONTROLr_TX_TS_VAR_CONTROLf_SET PLP_P1588_TX_TS_VAR_CONTROLr_TX_TS_VAR_CONTROLf_SET
#define READ_P1588_TX_TS_VAR_CONTROLr PLP_READ_P1588_TX_TS_VAR_CONTROLr
#define WRITE_P1588_TX_TS_VAR_CONTROLr PLP_WRITE_P1588_TX_TS_VAR_CONTROLr
#define MODIFY_P1588_TX_TS_VAR_CONTROLr PLP_MODIFY_P1588_TX_TS_VAR_CONTROLr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TX_TS_VAR_CONTROLr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TIMESTAMP_DELTA
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x707a
 * DESC:     TS DELTA  Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TIMESTAMP_DELATA3 timestamp delta change, 2'comp
 *     TIMESTAMP_DELTA2 1 - 48 bit counter enable
 *     TIMESTAMP_DELTA1 write 1 - issue request for offset correctionwrite 0 - to clear request
 *
 ******************************************************************************/
#define PLP_P1588_TIMESTAMP_DELTAr    0x0000007a

#define PLP_P1588_TIMESTAMP_DELTAr_SIZE 4

/*
 * This structure should be used to declare and program P1588_TIMESTAMP_DELTA.
 *
 */
typedef union PLP_P1588_TIMESTAMP_DELTAr_s {
	uint32_t v[1];
	uint32_t p1588_timestamp_delta[1];
	uint32_t _p1588_timestamp_delta;
} PLP_P1588_TIMESTAMP_DELTAr_t;

#define PLP_P1588_TIMESTAMP_DELTAr_CLR(r) (r).p1588_timestamp_delta[0] = 0
#define PLP_P1588_TIMESTAMP_DELTAr_SET(r,d) (r).p1588_timestamp_delta[0] = d
#define PLP_P1588_TIMESTAMP_DELTAr_GET(r) (r).p1588_timestamp_delta[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TIMESTAMP_DELTAr_TIMESTAMP_DELTA1f_GET(r) ((((r).p1588_timestamp_delta[0]) >> 15) & 0x1)
#define PLP_P1588_TIMESTAMP_DELTAr_TIMESTAMP_DELTA1f_SET(r,f) (r).p1588_timestamp_delta[0]=(((r).p1588_timestamp_delta[0] & ~((uint32_t)0x1 << 15)) | ((((uint32_t)f) & 0x1) << 15)) | (1 << (16 + 15))
#define PLP_P1588_TIMESTAMP_DELTAr_TIMESTAMP_DELTA2f_GET(r) ((((r).p1588_timestamp_delta[0]) >> 14) & 0x1)
#define PLP_P1588_TIMESTAMP_DELTAr_TIMESTAMP_DELTA2f_SET(r,f) (r).p1588_timestamp_delta[0]=(((r).p1588_timestamp_delta[0] & ~((uint32_t)0x1 << 14)) | ((((uint32_t)f) & 0x1) << 14)) | (1 << (16 + 14))
#define PLP_P1588_TIMESTAMP_DELTAr_TIMESTAMP_DELATA3f_GET(r) (((r).p1588_timestamp_delta[0]) & 0x3fff)
#define PLP_P1588_TIMESTAMP_DELTAr_TIMESTAMP_DELATA3f_SET(r,f) (r).p1588_timestamp_delta[0]=(((r).p1588_timestamp_delta[0] & ~((uint32_t)0x3fff)) | (((uint32_t)f) & 0x3fff)) | (0x3fff << 16)

/*
 * These macros can be used to access P1588_TIMESTAMP_DELTA.
 *
 */
#define PLP_READ_P1588_TIMESTAMP_DELTAr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TIMESTAMP_DELTAr,(_r._p1588_timestamp_delta))
#define PLP_WRITE_P1588_TIMESTAMP_DELTAr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TIMESTAMP_DELTAr,(_r._p1588_timestamp_delta))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TIMESTAMP_DELTAr PLP_P1588_TIMESTAMP_DELTAr
#define P1588_TIMESTAMP_DELTAr_SIZE PLP_P1588_TIMESTAMP_DELTAr_SIZE
typedef PLP_P1588_TIMESTAMP_DELTAr_t P1588_TIMESTAMP_DELTAr_t;
#define P1588_TIMESTAMP_DELTAr_CLR PLP_P1588_TIMESTAMP_DELTAr_CLR
#define P1588_TIMESTAMP_DELTAr_SET PLP_P1588_TIMESTAMP_DELTAr_SET
#define P1588_TIMESTAMP_DELTAr_GET PLP_P1588_TIMESTAMP_DELTAr_GET
#define P1588_TIMESTAMP_DELTAr_TIMESTAMP_DELTA1f_GET PLP_P1588_TIMESTAMP_DELTAr_TIMESTAMP_DELTA1f_GET
#define P1588_TIMESTAMP_DELTAr_TIMESTAMP_DELTA1f_SET PLP_P1588_TIMESTAMP_DELTAr_TIMESTAMP_DELTA1f_SET
#define P1588_TIMESTAMP_DELTAr_TIMESTAMP_DELTA2f_GET PLP_P1588_TIMESTAMP_DELTAr_TIMESTAMP_DELTA2f_GET
#define P1588_TIMESTAMP_DELTAr_TIMESTAMP_DELTA2f_SET PLP_P1588_TIMESTAMP_DELTAr_TIMESTAMP_DELTA2f_SET
#define P1588_TIMESTAMP_DELTAr_TIMESTAMP_DELATA3f_GET PLP_P1588_TIMESTAMP_DELTAr_TIMESTAMP_DELATA3f_GET
#define P1588_TIMESTAMP_DELTAr_TIMESTAMP_DELATA3f_SET PLP_P1588_TIMESTAMP_DELTAr_TIMESTAMP_DELATA3f_SET
#define READ_P1588_TIMESTAMP_DELTAr PLP_READ_P1588_TIMESTAMP_DELTAr
#define WRITE_P1588_TIMESTAMP_DELTAr PLP_WRITE_P1588_TIMESTAMP_DELTAr
#define MODIFY_P1588_TIMESTAMP_DELTAr PLP_MODIFY_P1588_TIMESTAMP_DELTAr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TIMESTAMP_DELTAr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TXRX_TS_OFFSET_MSB
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x707b
 * DESC:     TXRX TS OFFSET MSB Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TS_OFFSET_TX0_MSB Tx Timestamp offset registerThe unit is signed ns. This register compensatesthe delay of AFE. The Tx timestamp is given byNCO timestamp + {ts_offset_tx0_msb,ts_offset_tx0}
 *     TS_OFFSET_RX0_MSB Rx Timestamp offset registerThe unit is signed ns. This register compensatesthe delay of AFE. The Rx timestamp is given byNCO timestamp + {ts_offset_rx0_msb,ts_offset_rx0}
 *
 ******************************************************************************/
#define PLP_P1588_TXRX_TS_OFFSET_MSBr    0x0000007b

#define PLP_P1588_TXRX_TS_OFFSET_MSBr_SIZE 4

/*
 * This structure should be used to declare and program P1588_TXRX_TS_OFFSET_MSB.
 *
 */
typedef union PLP_P1588_TXRX_TS_OFFSET_MSBr_s {
	uint32_t v[1];
	uint32_t P1588_TXRX_TS_OFFSET_MSB[1];
	uint32_t _P1588_TXRX_TS_OFFSET_MSB;
} PLP_P1588_TXRX_TS_OFFSET_MSBr_t;

#define PLP_P1588_TXRX_TS_OFFSET_MSBr_CLR(r) (r).P1588_TXRX_TS_OFFSET_MSB[0] = 0
#define PLP_P1588_TXRX_TS_OFFSET_MSBr_SET(r,d) (r).P1588_TXRX_TS_OFFSET_MSB[0] = d
#define PLP_P1588_TXRX_TS_OFFSET_MSBr_GET(r) (r).P1588_TXRX_TS_OFFSET_MSB[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TXRX_TS_OFFSET_MSBr_TS_OFFSET_RX0_MSBf_GET(r) ((((r).P1588_TXRX_TS_OFFSET_MSB[0]) >> 4) & 0xf)
#define PLP_P1588_TXRX_TS_OFFSET_MSBr_TS_OFFSET_RX0_MSBf_SET(r,f) (r).P1588_TXRX_TS_OFFSET_MSB[0]=(((r).P1588_TXRX_TS_OFFSET_MSB[0] & ~((uint32_t)0xf << 4)) | ((((uint32_t)f) & 0xf) << 4)) | (15 << (16 + 4))
#define PLP_P1588_TXRX_TS_OFFSET_MSBr_TS_OFFSET_TX0_MSBf_GET(r) (((r).P1588_TXRX_TS_OFFSET_MSB[0]) & 0xf)
#define PLP_P1588_TXRX_TS_OFFSET_MSBr_TS_OFFSET_TX0_MSBf_SET(r,f) (r).P1588_TXRX_TS_OFFSET_MSB[0]=(((r).P1588_TXRX_TS_OFFSET_MSB[0] & ~((uint32_t)0xf)) | (((uint32_t)f) & 0xf)) | (0xf << 16)

/*
 * These macros can be used to access P1588_TXRX_TS_OFFSET_MSB.
 *
 */
#define PLP_READ_P1588_TXRX_TS_OFFSET_MSBr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TXRX_TS_OFFSET_MSBr,(_r._P1588_TXRX_TS_OFFSET_MSB))
#define PLP_WRITE_P1588_TXRX_TS_OFFSET_MSBr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TXRX_TS_OFFSET_MSBr,(_r._P1588_TXRX_TS_OFFSET_MSB))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TXRX_TS_OFFSET_MSBr PLP_P1588_TXRX_TS_OFFSET_MSBr
#define P1588_TXRX_TS_OFFSET_MSBr_SIZE PLP_P1588_TXRX_TS_OFFSET_MSBr_SIZE
typedef PLP_P1588_TXRX_TS_OFFSET_MSBr_t P1588_TXRX_TS_OFFSET_MSBr_t;
#define P1588_TXRX_TS_OFFSET_MSBr_CLR PLP_P1588_TXRX_TS_OFFSET_MSBr_CLR
#define P1588_TXRX_TS_OFFSET_MSBr_SET PLP_P1588_TXRX_TS_OFFSET_MSBr_SET
#define P1588_TXRX_TS_OFFSET_MSBr_GET PLP_P1588_TXRX_TS_OFFSET_MSBr_GET
#define P1588_TXRX_TS_OFFSET_MSBr_TS_OFFSET_RX0_MSBf_GET PLP_P1588_TXRX_TS_OFFSET_MSBr_TS_OFFSET_RX0_MSBf_GET
#define P1588_TXRX_TS_OFFSET_MSBr_TS_OFFSET_RX0_MSBf_SET PLP_P1588_TXRX_TS_OFFSET_MSBr_TS_OFFSET_RX0_MSBf_SET
#define P1588_TXRX_TS_OFFSET_MSBr_TS_OFFSET_TX0_MSBf_GET PLP_P1588_TXRX_TS_OFFSET_MSBr_TS_OFFSET_TX0_MSBf_GET
#define P1588_TXRX_TS_OFFSET_MSBr_TS_OFFSET_TX0_MSBf_SET PLP_P1588_TXRX_TS_OFFSET_MSBr_TS_OFFSET_TX0_MSBf_SET
#define READ_P1588_TXRX_TS_OFFSET_MSBr PLP_READ_P1588_TXRX_TS_OFFSET_MSBr
#define WRITE_P1588_TXRX_TS_OFFSET_MSBr PLP_WRITE_P1588_TXRX_TS_OFFSET_MSBr
#define MODIFY_P1588_TXRX_TS_OFFSET_MSBr PLP_MODIFY_P1588_TXRX_TS_OFFSET_MSBr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TXRX_TS_OFFSET_MSBr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TX_PCH_COUNTER
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x707c
 * DESC:     TX PCH COUNTER Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_PCH_COUNTER   Egress PCH/HSR counterNOTE: THESE BITS ARE CLEAR ON READ AND NEED TO BEHANDLED OUTSIDE THE REGISTER BLOCK
 *
 ******************************************************************************/
#define PLP_P1588_TX_PCH_COUNTERr    0x0000007c

#define PLP_P1588_TX_PCH_COUNTERr_SIZE 4

/*
 * This structure should be used to declare and program P1588_TX_PCH_COUNTER.
 *
 */
typedef union PLP_P1588_TX_PCH_COUNTERr_s {
	uint32_t v[1];
	uint32_t p1588_tx_acch_counter[1];
	uint32_t _p1588_tx_acch_counter;
} PLP_P1588_TX_PCH_COUNTERr_t;

#define PLP_P1588_TX_PCH_COUNTERr_CLR(r) (r).p1588_tx_acch_counter[0] = 0
#define PLP_P1588_TX_PCH_COUNTERr_SET(r,d) (r).p1588_tx_acch_counter[0] = d
#define PLP_P1588_TX_PCH_COUNTERr_GET(r) (r).p1588_tx_acch_counter[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TX_PCH_COUNTERr_TX_PCH_COUNTERf_GET(r) (((r).p1588_tx_acch_counter[0]) & 0xffff)
#define PLP_P1588_TX_PCH_COUNTERr_TX_PCH_COUNTERf_SET(r,f) (r).p1588_tx_acch_counter[0]=(((r).p1588_tx_acch_counter[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TX_PCH_COUNTER.
 *
 */
#define PLP_READ_P1588_TX_PCH_COUNTERr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TX_PCH_COUNTERr,(_r._p1588_tx_acch_counter))
#define PLP_WRITE_P1588_TX_PCH_COUNTERr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TX_PCH_COUNTERr,(_r._p1588_tx_acch_counter))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TX_PCH_COUNTERr PLP_P1588_TX_PCH_COUNTERr
#define P1588_TX_PCH_COUNTERr_SIZE PLP_P1588_TX_PCH_COUNTERr_SIZE
typedef PLP_P1588_TX_PCH_COUNTERr_t P1588_TX_PCH_COUNTERr_t;
#define P1588_TX_PCH_COUNTERr_CLR PLP_P1588_TX_PCH_COUNTERr_CLR
#define P1588_TX_PCH_COUNTERr_SET PLP_P1588_TX_PCH_COUNTERr_SET
#define P1588_TX_PCH_COUNTERr_GET PLP_P1588_TX_PCH_COUNTERr_GET
#define P1588_TX_PCH_COUNTERr_TX_PCH_COUNTERf_GET PLP_P1588_TX_PCH_COUNTERr_TX_PCH_COUNTERf_GET
#define P1588_TX_PCH_COUNTERr_TX_PCH_COUNTERf_SET PLP_P1588_TX_PCH_COUNTERr_TX_PCH_COUNTERf_SET
#define READ_P1588_TX_PCH_COUNTERr PLP_READ_P1588_TX_PCH_COUNTERr
#define WRITE_P1588_TX_PCH_COUNTERr PLP_WRITE_P1588_TX_PCH_COUNTERr
#define MODIFY_P1588_TX_PCH_COUNTERr PLP_MODIFY_P1588_TX_PCH_COUNTERr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TX_PCH_COUNTERr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_RX_PCH_COUNTER
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x707d
 * DESC:     RX PCH COUNTER Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_PCH_COUNTER   Ingress PCH/HSR counterNOTE: THESE BITS ARE CLEAR ON READ AND NEED TO BEHANDLED OUTSIDE THE REGISTER BLOCK
 *
 ******************************************************************************/
#define PLP_P1588_RX_PCH_COUNTERr    0x0000007d

#define PLP_P1588_RX_PCH_COUNTERr_SIZE 4

/*
 * This structure should be used to declare and program P1588_RX_PCH_COUNTER.
 *
 */
typedef union PLP_P1588_RX_PCH_COUNTERr_s {
	uint32_t v[1];
	uint32_t p1588_rx_acch_counter[1];
	uint32_t _p1588_rx_acch_counter;
} PLP_P1588_RX_PCH_COUNTERr_t;

#define PLP_P1588_RX_PCH_COUNTERr_CLR(r) (r).p1588_rx_acch_counter[0] = 0
#define PLP_P1588_RX_PCH_COUNTERr_SET(r,d) (r).p1588_rx_acch_counter[0] = d
#define PLP_P1588_RX_PCH_COUNTERr_GET(r) (r).p1588_rx_acch_counter[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_RX_PCH_COUNTERr_RX_PCH_COUNTERf_GET(r) (((r).p1588_rx_acch_counter[0]) & 0xffff)
#define PLP_P1588_RX_PCH_COUNTERr_RX_PCH_COUNTERf_SET(r,f) (r).p1588_rx_acch_counter[0]=(((r).p1588_rx_acch_counter[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_RX_PCH_COUNTER.
 *
 */
#define PLP_READ_P1588_RX_PCH_COUNTERr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_RX_PCH_COUNTERr,(_r._p1588_rx_acch_counter))
#define PLP_WRITE_P1588_RX_PCH_COUNTERr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_RX_PCH_COUNTERr,(_r._p1588_rx_acch_counter))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_RX_PCH_COUNTERr PLP_P1588_RX_PCH_COUNTERr
#define P1588_RX_PCH_COUNTERr_SIZE PLP_P1588_RX_PCH_COUNTERr_SIZE
typedef PLP_P1588_RX_PCH_COUNTERr_t P1588_RX_PCH_COUNTERr_t;
#define P1588_RX_PCH_COUNTERr_CLR PLP_P1588_RX_PCH_COUNTERr_CLR
#define P1588_RX_PCH_COUNTERr_SET PLP_P1588_RX_PCH_COUNTERr_SET
#define P1588_RX_PCH_COUNTERr_GET PLP_P1588_RX_PCH_COUNTERr_GET
#define P1588_RX_PCH_COUNTERr_RX_PCH_COUNTERf_GET PLP_P1588_RX_PCH_COUNTERr_RX_PCH_COUNTERf_GET
#define P1588_RX_PCH_COUNTERr_RX_PCH_COUNTERf_SET PLP_P1588_RX_PCH_COUNTERr_RX_PCH_COUNTERf_SET
#define READ_P1588_RX_PCH_COUNTERr PLP_READ_P1588_RX_PCH_COUNTERr
#define WRITE_P1588_RX_PCH_COUNTERr PLP_WRITE_P1588_RX_PCH_COUNTERr
#define MODIFY_P1588_RX_PCH_COUNTERr PLP_MODIFY_P1588_RX_PCH_COUNTERr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_RX_PCH_COUNTERr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_FCMAC_PHY_STATUS_1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x707e
 * DESC:     FCMAC PHY STATUS 1 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     FCMAC_PHY_STATUS FCMAC to Phy Status register
 *
 ******************************************************************************/
#define PLP_P1588_FCMAC_PHY_STATUS_1r    0x0000007e

#define PLP_P1588_FCMAC_PHY_STATUS_1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_FCMAC_PHY_STATUS_1.
 *
 */
typedef union PLP_P1588_FCMAC_PHY_STATUS_1r_s {
	uint32_t v[1];
	uint32_t p1588_fcmac_phy_status_1[1];
	uint32_t _p1588_fcmac_phy_status_1;
} PLP_P1588_FCMAC_PHY_STATUS_1r_t;

#define PLP_P1588_FCMAC_PHY_STATUS_1r_CLR(r) (r).p1588_fcmac_phy_status_1[0] = 0
#define PLP_P1588_FCMAC_PHY_STATUS_1r_SET(r,d) (r).p1588_fcmac_phy_status_1[0] = d
#define PLP_P1588_FCMAC_PHY_STATUS_1r_GET(r) (r).p1588_fcmac_phy_status_1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_FCMAC_PHY_STATUS_1r_FCMAC_PHY_STATUSf_GET(r) (((r).p1588_fcmac_phy_status_1[0]) & 0xffff)
#define PLP_P1588_FCMAC_PHY_STATUS_1r_FCMAC_PHY_STATUSf_SET(r,f) (r).p1588_fcmac_phy_status_1[0]=(((r).p1588_fcmac_phy_status_1[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_FCMAC_PHY_STATUS_1.
 *
 */
#define PLP_READ_P1588_FCMAC_PHY_STATUS_1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_FCMAC_PHY_STATUS_1r,(_r._p1588_fcmac_phy_status_1))
#define PLP_WRITE_P1588_FCMAC_PHY_STATUS_1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_FCMAC_PHY_STATUS_1r,(_r._p1588_fcmac_phy_status_1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_FCMAC_PHY_STATUS_1r PLP_P1588_FCMAC_PHY_STATUS_1r
#define P1588_FCMAC_PHY_STATUS_1r_SIZE PLP_P1588_FCMAC_PHY_STATUS_1r_SIZE
typedef PLP_P1588_FCMAC_PHY_STATUS_1r_t P1588_FCMAC_PHY_STATUS_1r_t;
#define P1588_FCMAC_PHY_STATUS_1r_CLR PLP_P1588_FCMAC_PHY_STATUS_1r_CLR
#define P1588_FCMAC_PHY_STATUS_1r_SET PLP_P1588_FCMAC_PHY_STATUS_1r_SET
#define P1588_FCMAC_PHY_STATUS_1r_GET PLP_P1588_FCMAC_PHY_STATUS_1r_GET
#define P1588_FCMAC_PHY_STATUS_1r_FCMAC_PHY_STATUSf_GET PLP_P1588_FCMAC_PHY_STATUS_1r_FCMAC_PHY_STATUSf_GET
#define P1588_FCMAC_PHY_STATUS_1r_FCMAC_PHY_STATUSf_SET PLP_P1588_FCMAC_PHY_STATUS_1r_FCMAC_PHY_STATUSf_SET
#define READ_P1588_FCMAC_PHY_STATUS_1r PLP_READ_P1588_FCMAC_PHY_STATUS_1r
#define WRITE_P1588_FCMAC_PHY_STATUS_1r PLP_WRITE_P1588_FCMAC_PHY_STATUS_1r
#define MODIFY_P1588_FCMAC_PHY_STATUS_1r PLP_MODIFY_P1588_FCMAC_PHY_STATUS_1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_FCMAC_PHY_STATUS_1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PHY_FCMAC_CONTROL_1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x707f
 * DESC:     PHY FCMAC CONTROL 1 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PHY_FCMAC_CONTROL Phy to FCMAC Control register
 *
 ******************************************************************************/
#define PLP_P1588_PHY_FCMAC_CONTROL_1r    0x0000007f

#define PLP_P1588_PHY_FCMAC_CONTROL_1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PHY_FCMAC_CONTROL_1.
 *
 */
typedef union PLP_P1588_PHY_FCMAC_CONTROL_1r_s {
	uint32_t v[1];
	uint32_t p1588_phy_fcmac_control_1[1];
	uint32_t _p1588_phy_fcmac_control_1;
} PLP_P1588_PHY_FCMAC_CONTROL_1r_t;

#define PLP_P1588_PHY_FCMAC_CONTROL_1r_CLR(r) (r).p1588_phy_fcmac_control_1[0] = 0
#define PLP_P1588_PHY_FCMAC_CONTROL_1r_SET(r,d) (r).p1588_phy_fcmac_control_1[0] = d
#define PLP_P1588_PHY_FCMAC_CONTROL_1r_GET(r) (r).p1588_phy_fcmac_control_1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PHY_FCMAC_CONTROL_1r_PHY_FCMAC_CONTROLf_GET(r) (((r).p1588_phy_fcmac_control_1[0]) & 0xffff)
#define PLP_P1588_PHY_FCMAC_CONTROL_1r_PHY_FCMAC_CONTROLf_SET(r,f) (r).p1588_phy_fcmac_control_1[0]=(((r).p1588_phy_fcmac_control_1[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PHY_FCMAC_CONTROL_1.
 *
 */
#define PLP_READ_P1588_PHY_FCMAC_CONTROL_1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PHY_FCMAC_CONTROL_1r,(_r._p1588_phy_fcmac_control_1))
#define PLP_WRITE_P1588_PHY_FCMAC_CONTROL_1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PHY_FCMAC_CONTROL_1r,(_r._p1588_phy_fcmac_control_1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PHY_FCMAC_CONTROL_1r PLP_P1588_PHY_FCMAC_CONTROL_1r
#define P1588_PHY_FCMAC_CONTROL_1r_SIZE PLP_P1588_PHY_FCMAC_CONTROL_1r_SIZE
typedef PLP_P1588_PHY_FCMAC_CONTROL_1r_t P1588_PHY_FCMAC_CONTROL_1r_t;
#define P1588_PHY_FCMAC_CONTROL_1r_CLR PLP_P1588_PHY_FCMAC_CONTROL_1r_CLR
#define P1588_PHY_FCMAC_CONTROL_1r_SET PLP_P1588_PHY_FCMAC_CONTROL_1r_SET
#define P1588_PHY_FCMAC_CONTROL_1r_GET PLP_P1588_PHY_FCMAC_CONTROL_1r_GET
#define P1588_PHY_FCMAC_CONTROL_1r_PHY_FCMAC_CONTROLf_GET PLP_P1588_PHY_FCMAC_CONTROL_1r_PHY_FCMAC_CONTROLf_GET
#define P1588_PHY_FCMAC_CONTROL_1r_PHY_FCMAC_CONTROLf_SET PLP_P1588_PHY_FCMAC_CONTROL_1r_PHY_FCMAC_CONTROLf_SET
#define READ_P1588_PHY_FCMAC_CONTROL_1r PLP_READ_P1588_PHY_FCMAC_CONTROL_1r
#define WRITE_P1588_PHY_FCMAC_CONTROL_1r PLP_WRITE_P1588_PHY_FCMAC_CONTROL_1r
#define MODIFY_P1588_PHY_FCMAC_CONTROL_1r PLP_MODIFY_P1588_PHY_FCMAC_CONTROL_1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PHY_FCMAC_CONTROL_1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_MAC_PORT_ID
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7080
 * DESC:     MAC PORT ID Register
 * RESETVAL: 0x10 (16)
 * ACCESS:   R/W
 * FIELDS:
 *     LINE_PORT_ID     line port ID for line side MAC
 *     SWITCH_PORT_ID   Switch port ID for system side MAC
 *
 ******************************************************************************/
#define PLP_P1588_MAC_PORT_IDr    0x00000080

#define PLP_P1588_MAC_PORT_IDr_SIZE 4

/*
 * This structure should be used to declare and program P1588_MAC_PORT_ID.
 *
 */
typedef union PLP_P1588_MAC_PORT_IDr_s {
	uint32_t v[1];
	uint32_t p1588_mac_port_id[1];
	uint32_t _p1588_mac_port_id;
} PLP_P1588_MAC_PORT_IDr_t;

#define PLP_P1588_MAC_PORT_IDr_CLR(r) (r).p1588_mac_port_id[0] = 0
#define PLP_P1588_MAC_PORT_IDr_SET(r,d) (r).p1588_mac_port_id[0] = d
#define PLP_P1588_MAC_PORT_IDr_GET(r) (r).p1588_mac_port_id[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_MAC_PORT_IDr_SWITCH_PORT_IDf_GET(r) ((((r).p1588_mac_port_id[0]) >> 4) & 0x1f)
#define PLP_P1588_MAC_PORT_IDr_SWITCH_PORT_IDf_SET(r,f) (r).p1588_mac_port_id[0]=(((r).p1588_mac_port_id[0] & ~((uint32_t)0x1f << 4)) | ((((uint32_t)f) & 0x1f) << 4)) | (31 << (16 + 4))
#define PLP_P1588_MAC_PORT_IDr_LINE_PORT_IDf_GET(r) (((r).p1588_mac_port_id[0]) & 0xf)
#define PLP_P1588_MAC_PORT_IDr_LINE_PORT_IDf_SET(r,f) (r).p1588_mac_port_id[0]=(((r).p1588_mac_port_id[0] & ~((uint32_t)0xf)) | (((uint32_t)f) & 0xf)) | (0xf << 16)

/*
 * These macros can be used to access P1588_MAC_PORT_ID.
 *
 */
#define PLP_READ_P1588_MAC_PORT_IDr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_MAC_PORT_IDr,(_r._p1588_mac_port_id))
#define PLP_WRITE_P1588_MAC_PORT_IDr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_MAC_PORT_IDr,(_r._p1588_mac_port_id))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_MAC_PORT_IDr PLP_P1588_MAC_PORT_IDr
#define P1588_MAC_PORT_IDr_SIZE PLP_P1588_MAC_PORT_IDr_SIZE
typedef PLP_P1588_MAC_PORT_IDr_t P1588_MAC_PORT_IDr_t;
#define P1588_MAC_PORT_IDr_CLR PLP_P1588_MAC_PORT_IDr_CLR
#define P1588_MAC_PORT_IDr_SET PLP_P1588_MAC_PORT_IDr_SET
#define P1588_MAC_PORT_IDr_GET PLP_P1588_MAC_PORT_IDr_GET
#define P1588_MAC_PORT_IDr_SWITCH_PORT_IDf_GET PLP_P1588_MAC_PORT_IDr_SWITCH_PORT_IDf_GET
#define P1588_MAC_PORT_IDr_SWITCH_PORT_IDf_SET PLP_P1588_MAC_PORT_IDr_SWITCH_PORT_IDf_SET
#define P1588_MAC_PORT_IDr_LINE_PORT_IDf_GET PLP_P1588_MAC_PORT_IDr_LINE_PORT_IDf_GET
#define P1588_MAC_PORT_IDr_LINE_PORT_IDf_SET PLP_P1588_MAC_PORT_IDr_LINE_PORT_IDf_SET
#define READ_P1588_MAC_PORT_IDr PLP_READ_P1588_MAC_PORT_IDr
#define WRITE_P1588_MAC_PORT_IDr PLP_WRITE_P1588_MAC_PORT_IDr
#define MODIFY_P1588_MAC_PORT_IDr PLP_MODIFY_P1588_MAC_PORT_IDr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_MAC_PORT_IDr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_DEBUG_TYPE
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7081
 * DESC:     PIM Debug Type Register
 * RESETVAL: 0x1 (1)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_NCO_SEL
 *     PIM_NCO_FIX
 *     PIM_NCO_DB_SYNC_PULSE
 *     PIM_NCO_DB_MODE
 *     PIM_NCO_DB_FREQ_SEL
 *     PIM_RESERVED     Reserved bits for future use
 *
 ******************************************************************************/
#define PLP_P1588_PIM_DEBUG_TYPEr    0x00000081

#define PLP_P1588_PIM_DEBUG_TYPEr_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_DEBUG_TYPE.
 *
 */
typedef union PLP_P1588_PIM_DEBUG_TYPEr_s {
	uint32_t v[1];
	uint32_t p1588_pim_debug_type[1];
	uint32_t _p1588_pim_debug_type;
} PLP_P1588_PIM_DEBUG_TYPEr_t;

#define PLP_P1588_PIM_DEBUG_TYPEr_CLR(r) (r).p1588_pim_debug_type[0] = 0
#define PLP_P1588_PIM_DEBUG_TYPEr_SET(r,d) (r).p1588_pim_debug_type[0] = d
#define PLP_P1588_PIM_DEBUG_TYPEr_GET(r) (r).p1588_pim_debug_type[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_DEBUG_TYPEr_PIM_RESERVEDf_GET(r) ((((r).p1588_pim_debug_type[0]) >> 6) & 0x3ff)
#define PLP_P1588_PIM_DEBUG_TYPEr_PIM_RESERVEDf_SET(r,f) (r).p1588_pim_debug_type[0]=(((r).p1588_pim_debug_type[0] & ~((uint32_t)0x3ff << 6)) | ((((uint32_t)f) & 0x3ff) << 6)) | (1023 << (16 + 6))
#define PLP_P1588_PIM_DEBUG_TYPEr_PIM_SYNC0_PULSEf_GET(r) ((((r).p1588_pim_debug_type[0]) >> 6) & 0x7)
#define PLP_P1588_PIM_DEBUG_TYPEr_PIM_SYNC0_PULSEf_SET(r,f) (r).p1588_pim_debug_type[0]=(((r).p1588_pim_debug_type[0] & ~((uint32_t)0x7 << 6)) | ((((uint32_t)f) & 0x7) << 6)) | (1023 << (16 + 6))
#define PLP_P1588_PIM_DEBUG_TYPEr_PIM_NCO_DB_FREQ_SELf_GET(r) ((((r).p1588_pim_debug_type[0]) >> 5) & 0x1)
#define PLP_P1588_PIM_DEBUG_TYPEr_PIM_NCO_DB_FREQ_SELf_SET(r,f) (r).p1588_pim_debug_type[0]=(((r).p1588_pim_debug_type[0] & ~((uint32_t)0x1 << 5)) | ((((uint32_t)f) & 0x1) << 5)) | (1 << (16 + 5))
#define PLP_P1588_PIM_DEBUG_TYPEr_PIM_NCO_DB_MODEf_GET(r) ((((r).p1588_pim_debug_type[0]) >> 4) & 0x1)
#define PLP_P1588_PIM_DEBUG_TYPEr_PIM_NCO_DB_MODEf_SET(r,f) (r).p1588_pim_debug_type[0]=(((r).p1588_pim_debug_type[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4)) | (1 << (16 + 4))
#define PLP_P1588_PIM_DEBUG_TYPEr_PIM_NCO_DB_SYNC_PULSEf_GET(r) ((((r).p1588_pim_debug_type[0]) >> 3) & 0x1)
#define PLP_P1588_PIM_DEBUG_TYPEr_PIM_NCO_DB_SYNC_PULSEf_SET(r,f) (r).p1588_pim_debug_type[0]=(((r).p1588_pim_debug_type[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3)) | (1 << (16 + 3))
#define PLP_P1588_PIM_DEBUG_TYPEr_PIM_NCO_FIXf_GET(r) ((((r).p1588_pim_debug_type[0]) >> 2) & 0x1)
#define PLP_P1588_PIM_DEBUG_TYPEr_PIM_NCO_FIXf_SET(r,f) (r).p1588_pim_debug_type[0]=(((r).p1588_pim_debug_type[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2)) | (1 << (16 + 2))
#define PLP_P1588_PIM_DEBUG_TYPEr_PIM_NCO_SELf_GET(r) (((r).p1588_pim_debug_type[0]) & 0x3)
#define PLP_P1588_PIM_DEBUG_TYPEr_PIM_NCO_SELf_SET(r,f) (r).p1588_pim_debug_type[0]=(((r).p1588_pim_debug_type[0] & ~((uint32_t)0x3)) | (((uint32_t)f) & 0x3)) | (0x3 << 16)

/*
 * These macros can be used to access P1588_PIM_DEBUG_TYPE.
 *
 */
#define PLP_READ_P1588_PIM_DEBUG_TYPEr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_DEBUG_TYPEr,(_r._p1588_pim_debug_type))
#define PLP_WRITE_P1588_PIM_DEBUG_TYPEr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_DEBUG_TYPEr,(_r._p1588_pim_debug_type))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_DEBUG_TYPEr PLP_P1588_PIM_DEBUG_TYPEr
#define P1588_PIM_DEBUG_TYPEr_SIZE PLP_P1588_PIM_DEBUG_TYPEr_SIZE
typedef PLP_P1588_PIM_DEBUG_TYPEr_t P1588_PIM_DEBUG_TYPEr_t;
#define P1588_PIM_DEBUG_TYPEr_CLR PLP_P1588_PIM_DEBUG_TYPEr_CLR
#define P1588_PIM_DEBUG_TYPEr_SET PLP_P1588_PIM_DEBUG_TYPEr_SET
#define P1588_PIM_DEBUG_TYPEr_GET PLP_P1588_PIM_DEBUG_TYPEr_GET
#define P1588_PIM_DEBUG_TYPEr_PIM_RESERVEDf_GET PLP_P1588_PIM_DEBUG_TYPEr_PIM_RESERVEDf_GET
#define P1588_PIM_DEBUG_TYPEr_PIM_RESERVEDf_SET PLP_P1588_PIM_DEBUG_TYPEr_PIM_RESERVEDf_SET
#define P1588_PIM_DEBUG_TYPEr_PIM_SYNC0_PULSEf_GET  PLP_P1588_PIM_DEBUG_TYPEr_PIM_SYNC0_PULSEf_GET
#define P1588_PIM_DEBUG_TYPEr_PIM_SYNC0_PULSEf_SET  PLP_P1588_PIM_DEBUG_TYPEr_PIM_SYNC0_PULSEf_SET
#define P1588_PIM_DEBUG_TYPEr_PIM_NCO_DB_FREQ_SELf_GET PLP_P1588_PIM_DEBUG_TYPEr_PIM_NCO_DB_FREQ_SELf_GET
#define P1588_PIM_DEBUG_TYPEr_PIM_NCO_DB_FREQ_SELf_SET PLP_P1588_PIM_DEBUG_TYPEr_PIM_NCO_DB_FREQ_SELf_SET
#define P1588_PIM_DEBUG_TYPEr_PIM_NCO_DB_MODEf_GET PLP_P1588_PIM_DEBUG_TYPEr_PIM_NCO_DB_MODEf_GET
#define P1588_PIM_DEBUG_TYPEr_PIM_NCO_DB_MODEf_SET PLP_P1588_PIM_DEBUG_TYPEr_PIM_NCO_DB_MODEf_SET
#define P1588_PIM_DEBUG_TYPEr_PIM_NCO_DB_SYNC_PULSEf_GET PLP_P1588_PIM_DEBUG_TYPEr_PIM_NCO_DB_SYNC_PULSEf_GET
#define P1588_PIM_DEBUG_TYPEr_PIM_NCO_DB_SYNC_PULSEf_SET PLP_P1588_PIM_DEBUG_TYPEr_PIM_NCO_DB_SYNC_PULSEf_SET
#define P1588_PIM_DEBUG_TYPEr_PIM_NCO_FIXf_GET PLP_P1588_PIM_DEBUG_TYPEr_PIM_NCO_FIXf_GET
#define P1588_PIM_DEBUG_TYPEr_PIM_NCO_FIXf_SET PLP_P1588_PIM_DEBUG_TYPEr_PIM_NCO_FIXf_SET
#define P1588_PIM_DEBUG_TYPEr_PIM_NCO_SELf_GET PLP_P1588_PIM_DEBUG_TYPEr_PIM_NCO_SELf_GET
#define P1588_PIM_DEBUG_TYPEr_PIM_NCO_SELf_SET PLP_P1588_PIM_DEBUG_TYPEr_PIM_NCO_SELf_SET
#define READ_P1588_PIM_DEBUG_TYPEr PLP_READ_P1588_PIM_DEBUG_TYPEr
#define WRITE_P1588_PIM_DEBUG_TYPEr PLP_WRITE_P1588_PIM_DEBUG_TYPEr
#define MODIFY_P1588_PIM_DEBUG_TYPEr PLP_MODIFY_P1588_PIM_DEBUG_TYPEr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_DEBUG_TYPEr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_RX_Y1731_OPCODE45_OFFSET
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7082
 * DESC:     P1588 RX Y1731 Opcode45 Offset Register
 * RESETVAL: 0x8 (8)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_Y1731_OPCODE45_OFFSET Only bits [5:0] are used. Bits [5:0] indicate the byte number of the timestamp after the control word ending in flags+TLV offset.The numbering starts from 0.
 *
 ******************************************************************************/
#define PLP_P1588_RX_Y1731_OPCODE45_OFFSETr    0x00000082

#define PLP_P1588_RX_Y1731_OPCODE45_OFFSETr_SIZE 4

/*
 * This structure should be used to declare and program P1588_RX_Y1731_OPCODE45_OFFSET.
 *
 */
typedef union PLP_P1588_RX_Y1731_OPCODE45_OFFSETr_s {
	uint32_t v[1];
	uint32_t p1588_rx_y1731_opcode45_offset[1];
	uint32_t _p1588_rx_y1731_opcode45_offset;
} PLP_P1588_RX_Y1731_OPCODE45_OFFSETr_t;

#define PLP_P1588_RX_Y1731_OPCODE45_OFFSETr_CLR(r) (r).p1588_rx_y1731_opcode45_offset[0] = 0
#define PLP_P1588_RX_Y1731_OPCODE45_OFFSETr_SET(r,d) (r).p1588_rx_y1731_opcode45_offset[0] = d
#define PLP_P1588_RX_Y1731_OPCODE45_OFFSETr_GET(r) (r).p1588_rx_y1731_opcode45_offset[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_RX_Y1731_OPCODE45_OFFSETr_RX_Y1731_OPCODE45_OFFSETf_GET(r) (((r).p1588_rx_y1731_opcode45_offset[0]) & 0xffff)
#define PLP_P1588_RX_Y1731_OPCODE45_OFFSETr_RX_Y1731_OPCODE45_OFFSETf_SET(r,f) (r).p1588_rx_y1731_opcode45_offset[0]=(((r).p1588_rx_y1731_opcode45_offset[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_RX_Y1731_OPCODE45_OFFSET.
 *
 */
#define PLP_READ_P1588_RX_Y1731_OPCODE45_OFFSETr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_RX_Y1731_OPCODE45_OFFSETr,(_r._p1588_rx_y1731_opcode45_offset))
#define PLP_WRITE_P1588_RX_Y1731_OPCODE45_OFFSETr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_RX_Y1731_OPCODE45_OFFSETr,(_r._p1588_rx_y1731_opcode45_offset))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_RX_Y1731_OPCODE45_OFFSETr PLP_P1588_RX_Y1731_OPCODE45_OFFSETr
#define P1588_RX_Y1731_OPCODE45_OFFSETr_SIZE PLP_P1588_RX_Y1731_OPCODE45_OFFSETr_SIZE
typedef PLP_P1588_RX_Y1731_OPCODE45_OFFSETr_t P1588_RX_Y1731_OPCODE45_OFFSETr_t;
#define P1588_RX_Y1731_OPCODE45_OFFSETr_CLR PLP_P1588_RX_Y1731_OPCODE45_OFFSETr_CLR
#define P1588_RX_Y1731_OPCODE45_OFFSETr_SET PLP_P1588_RX_Y1731_OPCODE45_OFFSETr_SET
#define P1588_RX_Y1731_OPCODE45_OFFSETr_GET PLP_P1588_RX_Y1731_OPCODE45_OFFSETr_GET
#define P1588_RX_Y1731_OPCODE45_OFFSETr_RX_Y1731_OPCODE45_OFFSETf_GET PLP_P1588_RX_Y1731_OPCODE45_OFFSETr_RX_Y1731_OPCODE45_OFFSETf_GET
#define P1588_RX_Y1731_OPCODE45_OFFSETr_RX_Y1731_OPCODE45_OFFSETf_SET PLP_P1588_RX_Y1731_OPCODE45_OFFSETr_RX_Y1731_OPCODE45_OFFSETf_SET
#define READ_P1588_RX_Y1731_OPCODE45_OFFSETr PLP_READ_P1588_RX_Y1731_OPCODE45_OFFSETr
#define WRITE_P1588_RX_Y1731_OPCODE45_OFFSETr PLP_WRITE_P1588_RX_Y1731_OPCODE45_OFFSETr
#define MODIFY_P1588_RX_Y1731_OPCODE45_OFFSETr PLP_MODIFY_P1588_RX_Y1731_OPCODE45_OFFSETr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_RX_Y1731_OPCODE45_OFFSETr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_RX_Y1731_OPCODE46_OFFSET
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7083
 * DESC:     P1588 RX Y1731 Opcode46 Offset Register
 * RESETVAL: 0x18 (24)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_Y1731_OPCODE46_OFFSET Only bits [5:0] are used. Bits [5:0] indicate the byte number of the timestamp after the control word ending in flags+TLV offset.The numbering starts from 0.
 *
 ******************************************************************************/
#define PLP_P1588_RX_Y1731_OPCODE46_OFFSETr    0x00000083

#define PLP_P1588_RX_Y1731_OPCODE46_OFFSETr_SIZE 4

/*
 * This structure should be used to declare and program P1588_RX_Y1731_OPCODE46_OFFSET.
 *
 */
typedef union PLP_P1588_RX_Y1731_OPCODE46_OFFSETr_s {
	uint32_t v[1];
	uint32_t p1588_rx_y1731_opcode46_offset[1];
	uint32_t _p1588_rx_y1731_opcode46_offset;
} PLP_P1588_RX_Y1731_OPCODE46_OFFSETr_t;

#define PLP_P1588_RX_Y1731_OPCODE46_OFFSETr_CLR(r) (r).p1588_rx_y1731_opcode46_offset[0] = 0
#define PLP_P1588_RX_Y1731_OPCODE46_OFFSETr_SET(r,d) (r).p1588_rx_y1731_opcode46_offset[0] = d
#define PLP_P1588_RX_Y1731_OPCODE46_OFFSETr_GET(r) (r).p1588_rx_y1731_opcode46_offset[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_RX_Y1731_OPCODE46_OFFSETr_RX_Y1731_OPCODE46_OFFSETf_GET(r) (((r).p1588_rx_y1731_opcode46_offset[0]) & 0xffff)
#define PLP_P1588_RX_Y1731_OPCODE46_OFFSETr_RX_Y1731_OPCODE46_OFFSETf_SET(r,f) (r).p1588_rx_y1731_opcode46_offset[0]=(((r).p1588_rx_y1731_opcode46_offset[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_RX_Y1731_OPCODE46_OFFSET.
 *
 */
#define PLP_READ_P1588_RX_Y1731_OPCODE46_OFFSETr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_RX_Y1731_OPCODE46_OFFSETr,(_r._p1588_rx_y1731_opcode46_offset))
#define PLP_WRITE_P1588_RX_Y1731_OPCODE46_OFFSETr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_RX_Y1731_OPCODE46_OFFSETr,(_r._p1588_rx_y1731_opcode46_offset))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_RX_Y1731_OPCODE46_OFFSETr PLP_P1588_RX_Y1731_OPCODE46_OFFSETr
#define P1588_RX_Y1731_OPCODE46_OFFSETr_SIZE PLP_P1588_RX_Y1731_OPCODE46_OFFSETr_SIZE
typedef PLP_P1588_RX_Y1731_OPCODE46_OFFSETr_t P1588_RX_Y1731_OPCODE46_OFFSETr_t;
#define P1588_RX_Y1731_OPCODE46_OFFSETr_CLR PLP_P1588_RX_Y1731_OPCODE46_OFFSETr_CLR
#define P1588_RX_Y1731_OPCODE46_OFFSETr_SET PLP_P1588_RX_Y1731_OPCODE46_OFFSETr_SET
#define P1588_RX_Y1731_OPCODE46_OFFSETr_GET PLP_P1588_RX_Y1731_OPCODE46_OFFSETr_GET
#define P1588_RX_Y1731_OPCODE46_OFFSETr_RX_Y1731_OPCODE46_OFFSETf_GET PLP_P1588_RX_Y1731_OPCODE46_OFFSETr_RX_Y1731_OPCODE46_OFFSETf_GET
#define P1588_RX_Y1731_OPCODE46_OFFSETr_RX_Y1731_OPCODE46_OFFSETf_SET PLP_P1588_RX_Y1731_OPCODE46_OFFSETr_RX_Y1731_OPCODE46_OFFSETf_SET
#define READ_P1588_RX_Y1731_OPCODE46_OFFSETr PLP_READ_P1588_RX_Y1731_OPCODE46_OFFSETr
#define WRITE_P1588_RX_Y1731_OPCODE46_OFFSETr PLP_WRITE_P1588_RX_Y1731_OPCODE46_OFFSETr
#define MODIFY_P1588_RX_Y1731_OPCODE46_OFFSETr PLP_MODIFY_P1588_RX_Y1731_OPCODE46_OFFSETr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_RX_Y1731_OPCODE46_OFFSETr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_RX_Y1731_OPCODE47_OFFSET
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7084
 * DESC:     P1588 RX Y1731 Opcode47 Offset Register
 * RESETVAL: 0x8 (8)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_Y1731_OPCODE47_OFFSET Only bits [5:0] are used. Bits [5:0] indicate the byte number of the timestamp after the control word ending in flags+TLV offset.The numbering starts from 0.
 *
 ******************************************************************************/
#define PLP_P1588_RX_Y1731_OPCODE47_OFFSETr    0x00000084

#define PLP_P1588_RX_Y1731_OPCODE47_OFFSETr_SIZE 4

/*
 * This structure should be used to declare and program P1588_RX_Y1731_OPCODE47_OFFSET.
 *
 */
typedef union PLP_P1588_RX_Y1731_OPCODE47_OFFSETr_s {
	uint32_t v[1];
	uint32_t p1588_rx_y1731_opcode47_offset[1];
	uint32_t _p1588_rx_y1731_opcode47_offset;
} PLP_P1588_RX_Y1731_OPCODE47_OFFSETr_t;

#define PLP_P1588_RX_Y1731_OPCODE47_OFFSETr_CLR(r) (r).p1588_rx_y1731_opcode47_offset[0] = 0
#define PLP_P1588_RX_Y1731_OPCODE47_OFFSETr_SET(r,d) (r).p1588_rx_y1731_opcode47_offset[0] = d
#define PLP_P1588_RX_Y1731_OPCODE47_OFFSETr_GET(r) (r).p1588_rx_y1731_opcode47_offset[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_RX_Y1731_OPCODE47_OFFSETr_RX_Y1731_OPCODE47_OFFSETf_GET(r) (((r).p1588_rx_y1731_opcode47_offset[0]) & 0xffff)
#define PLP_P1588_RX_Y1731_OPCODE47_OFFSETr_RX_Y1731_OPCODE47_OFFSETf_SET(r,f) (r).p1588_rx_y1731_opcode47_offset[0]=(((r).p1588_rx_y1731_opcode47_offset[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_RX_Y1731_OPCODE47_OFFSET.
 *
 */
#define PLP_READ_P1588_RX_Y1731_OPCODE47_OFFSETr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_RX_Y1731_OPCODE47_OFFSETr,(_r._p1588_rx_y1731_opcode47_offset))
#define PLP_WRITE_P1588_RX_Y1731_OPCODE47_OFFSETr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_RX_Y1731_OPCODE47_OFFSETr,(_r._p1588_rx_y1731_opcode47_offset))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_RX_Y1731_OPCODE47_OFFSETr PLP_P1588_RX_Y1731_OPCODE47_OFFSETr
#define P1588_RX_Y1731_OPCODE47_OFFSETr_SIZE PLP_P1588_RX_Y1731_OPCODE47_OFFSETr_SIZE
typedef PLP_P1588_RX_Y1731_OPCODE47_OFFSETr_t P1588_RX_Y1731_OPCODE47_OFFSETr_t;
#define P1588_RX_Y1731_OPCODE47_OFFSETr_CLR PLP_P1588_RX_Y1731_OPCODE47_OFFSETr_CLR
#define P1588_RX_Y1731_OPCODE47_OFFSETr_SET PLP_P1588_RX_Y1731_OPCODE47_OFFSETr_SET
#define P1588_RX_Y1731_OPCODE47_OFFSETr_GET PLP_P1588_RX_Y1731_OPCODE47_OFFSETr_GET
#define P1588_RX_Y1731_OPCODE47_OFFSETr_RX_Y1731_OPCODE47_OFFSETf_GET PLP_P1588_RX_Y1731_OPCODE47_OFFSETr_RX_Y1731_OPCODE47_OFFSETf_GET
#define P1588_RX_Y1731_OPCODE47_OFFSETr_RX_Y1731_OPCODE47_OFFSETf_SET PLP_P1588_RX_Y1731_OPCODE47_OFFSETr_RX_Y1731_OPCODE47_OFFSETf_SET
#define READ_P1588_RX_Y1731_OPCODE47_OFFSETr PLP_READ_P1588_RX_Y1731_OPCODE47_OFFSETr
#define WRITE_P1588_RX_Y1731_OPCODE47_OFFSETr PLP_WRITE_P1588_RX_Y1731_OPCODE47_OFFSETr
#define MODIFY_P1588_RX_Y1731_OPCODE47_OFFSETr PLP_MODIFY_P1588_RX_Y1731_OPCODE47_OFFSETr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_RX_Y1731_OPCODE47_OFFSETr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_ETHERTYPE14
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7087
 * DESC:     ETHERTYPE14 Register
 * RESETVAL: 0x110e (4366)
 * ACCESS:   R/W
 * FIELDS:
 *     ETHERTYPE14      ethertype14 used in new ieft register
 *
 ******************************************************************************/
#define PLP_P1588_ETHERTYPE14r    0x00000087

#define PLP_P1588_ETHERTYPE14r_SIZE 4

/*
 * This structure should be used to declare and program P1588_ETHERTYPE14.
 *
 */
typedef union PLP_P1588_ETHERTYPE14r_s {
	uint32_t v[1];
	uint32_t p1588_ethertype14[1];
	uint32_t _p1588_ethertype14;
} PLP_P1588_ETHERTYPE14r_t;

#define PLP_P1588_ETHERTYPE14r_CLR(r) (r).p1588_ethertype14[0] = 0
#define PLP_P1588_ETHERTYPE14r_SET(r,d) (r).p1588_ethertype14[0] = d
#define PLP_P1588_ETHERTYPE14r_GET(r) (r).p1588_ethertype14[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_ETHERTYPE14r_ETHERTYPE14f_GET(r) (((r).p1588_ethertype14[0]) & 0xffff)
#define PLP_P1588_ETHERTYPE14r_ETHERTYPE14f_SET(r,f) (r).p1588_ethertype14[0]=(((r).p1588_ethertype14[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_ETHERTYPE14.
 *
 */
#define PLP_READ_P1588_ETHERTYPE14r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_ETHERTYPE14r,(_r._p1588_ethertype14))
#define PLP_WRITE_P1588_ETHERTYPE14r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_ETHERTYPE14r,(_r._p1588_ethertype14))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_ETHERTYPE14r PLP_P1588_ETHERTYPE14r
#define P1588_ETHERTYPE14r_SIZE PLP_P1588_ETHERTYPE14r_SIZE
typedef PLP_P1588_ETHERTYPE14r_t P1588_ETHERTYPE14r_t;
#define P1588_ETHERTYPE14r_CLR PLP_P1588_ETHERTYPE14r_CLR
#define P1588_ETHERTYPE14r_SET PLP_P1588_ETHERTYPE14r_SET
#define P1588_ETHERTYPE14r_GET PLP_P1588_ETHERTYPE14r_GET
#define P1588_ETHERTYPE14r_ETHERTYPE14f_GET PLP_P1588_ETHERTYPE14r_ETHERTYPE14f_GET
#define P1588_ETHERTYPE14r_ETHERTYPE14f_SET PLP_P1588_ETHERTYPE14r_ETHERTYPE14f_SET
#define READ_P1588_ETHERTYPE14r PLP_READ_P1588_ETHERTYPE14r
#define WRITE_P1588_ETHERTYPE14r PLP_WRITE_P1588_ETHERTYPE14r
#define MODIFY_P1588_ETHERTYPE14r PLP_MODIFY_P1588_ETHERTYPE14r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_ETHERTYPE14r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_ETHERTYPE15
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7088
 * DESC:     ETHERTYPE15 Register
 * RESETVAL: 0x110f (4367)
 * ACCESS:   R/W
 * FIELDS:
 *     ETHERTYPE15      ethertype15 used in new ieft register
 *
 ******************************************************************************/
#define PLP_P1588_ETHERTYPE15r    0x00000088

#define PLP_P1588_ETHERTYPE15r_SIZE 4

/*
 * This structure should be used to declare and program P1588_ETHERTYPE15.
 *
 */
typedef union PLP_P1588_ETHERTYPE15r_s {
	uint32_t v[1];
	uint32_t p1588_ethertype15[1];
	uint32_t _p1588_ethertype15;
} PLP_P1588_ETHERTYPE15r_t;

#define PLP_P1588_ETHERTYPE15r_CLR(r) (r).p1588_ethertype15[0] = 0
#define PLP_P1588_ETHERTYPE15r_SET(r,d) (r).p1588_ethertype15[0] = d
#define PLP_P1588_ETHERTYPE15r_GET(r) (r).p1588_ethertype15[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_ETHERTYPE15r_ETHERTYPE15f_GET(r) (((r).p1588_ethertype15[0]) & 0xffff)
#define PLP_P1588_ETHERTYPE15r_ETHERTYPE15f_SET(r,f) (r).p1588_ethertype15[0]=(((r).p1588_ethertype15[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_ETHERTYPE15.
 *
 */
#define PLP_READ_P1588_ETHERTYPE15r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_ETHERTYPE15r,(_r._p1588_ethertype15))
#define PLP_WRITE_P1588_ETHERTYPE15r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_ETHERTYPE15r,(_r._p1588_ethertype15))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_ETHERTYPE15r PLP_P1588_ETHERTYPE15r
#define P1588_ETHERTYPE15r_SIZE PLP_P1588_ETHERTYPE15r_SIZE
typedef PLP_P1588_ETHERTYPE15r_t P1588_ETHERTYPE15r_t;
#define P1588_ETHERTYPE15r_CLR PLP_P1588_ETHERTYPE15r_CLR
#define P1588_ETHERTYPE15r_SET PLP_P1588_ETHERTYPE15r_SET
#define P1588_ETHERTYPE15r_GET PLP_P1588_ETHERTYPE15r_GET
#define P1588_ETHERTYPE15r_ETHERTYPE15f_GET PLP_P1588_ETHERTYPE15r_ETHERTYPE15f_GET
#define P1588_ETHERTYPE15r_ETHERTYPE15f_SET PLP_P1588_ETHERTYPE15r_ETHERTYPE15f_SET
#define READ_P1588_ETHERTYPE15r PLP_READ_P1588_ETHERTYPE15r
#define WRITE_P1588_ETHERTYPE15r PLP_WRITE_P1588_ETHERTYPE15r
#define MODIFY_P1588_ETHERTYPE15r PLP_MODIFY_P1588_ETHERTYPE15r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_ETHERTYPE15r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_ETHERTYPE0
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7089
 * DESC:     Y1731 ETHERTYPE0 Register
 * RESETVAL: 0x1100 (4352)
 * ACCESS:   R/W
 * FIELDS:
 *     ETHTYPE0         EtherType for Tx(Y.1731 OAM) PTP
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_ETHERTYPE0r    0x00000089

#define PLP_P1588_Y1731_ETHERTYPE0r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_ETHERTYPE0.
 *
 */
typedef union PLP_P1588_Y1731_ETHERTYPE0r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_ethertype0[1];
	uint32_t _p1588_y1731_ethertype0;
} PLP_P1588_Y1731_ETHERTYPE0r_t;

#define PLP_P1588_Y1731_ETHERTYPE0r_CLR(r) (r).p1588_y1731_ethertype0[0] = 0
#define PLP_P1588_Y1731_ETHERTYPE0r_SET(r,d) (r).p1588_y1731_ethertype0[0] = d
#define PLP_P1588_Y1731_ETHERTYPE0r_GET(r) (r).p1588_y1731_ethertype0[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_ETHERTYPE0r_ETHTYPE0f_GET(r) (((r).p1588_y1731_ethertype0[0]) & 0xffff)
#define PLP_P1588_Y1731_ETHERTYPE0r_ETHTYPE0f_SET(r,f) (r).p1588_y1731_ethertype0[0]=(((r).p1588_y1731_ethertype0[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_ETHERTYPE0.
 *
 */
#define PLP_READ_P1588_Y1731_ETHERTYPE0r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_ETHERTYPE0r,(_r._p1588_y1731_ethertype0))
#define PLP_WRITE_P1588_Y1731_ETHERTYPE0r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_ETHERTYPE0r,(_r._p1588_y1731_ethertype0))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_ETHERTYPE0r PLP_P1588_Y1731_ETHERTYPE0r
#define P1588_Y1731_ETHERTYPE0r_SIZE PLP_P1588_Y1731_ETHERTYPE0r_SIZE
typedef PLP_P1588_Y1731_ETHERTYPE0r_t P1588_Y1731_ETHERTYPE0r_t;
#define P1588_Y1731_ETHERTYPE0r_CLR PLP_P1588_Y1731_ETHERTYPE0r_CLR
#define P1588_Y1731_ETHERTYPE0r_SET PLP_P1588_Y1731_ETHERTYPE0r_SET
#define P1588_Y1731_ETHERTYPE0r_GET PLP_P1588_Y1731_ETHERTYPE0r_GET
#define P1588_Y1731_ETHERTYPE0r_ETHTYPE0f_GET PLP_P1588_Y1731_ETHERTYPE0r_ETHTYPE0f_GET
#define P1588_Y1731_ETHERTYPE0r_ETHTYPE0f_SET PLP_P1588_Y1731_ETHERTYPE0r_ETHTYPE0f_SET
#define READ_P1588_Y1731_ETHERTYPE0r PLP_READ_P1588_Y1731_ETHERTYPE0r
#define WRITE_P1588_Y1731_ETHERTYPE0r PLP_WRITE_P1588_Y1731_ETHERTYPE0r
#define MODIFY_P1588_Y1731_ETHERTYPE0r PLP_MODIFY_P1588_Y1731_ETHERTYPE0r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_ETHERTYPE0r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_ETHERTYPE1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x708a
 * DESC:     Y1731 ETHERTYPE1 Register
 * RESETVAL: 0x1101 (4353)
 * ACCESS:   R/W
 * FIELDS:
 *     ETHTYPE1         EtherType for Tx(Y.1731 OAM) PTP
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_ETHERTYPE1r    0x0000008a

#define PLP_P1588_Y1731_ETHERTYPE1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_ETHERTYPE1.
 *
 */
typedef union PLP_P1588_Y1731_ETHERTYPE1r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_ethertype1[1];
	uint32_t _p1588_y1731_ethertype1;
} PLP_P1588_Y1731_ETHERTYPE1r_t;

#define PLP_P1588_Y1731_ETHERTYPE1r_CLR(r) (r).p1588_y1731_ethertype1[0] = 0
#define PLP_P1588_Y1731_ETHERTYPE1r_SET(r,d) (r).p1588_y1731_ethertype1[0] = d
#define PLP_P1588_Y1731_ETHERTYPE1r_GET(r) (r).p1588_y1731_ethertype1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_ETHERTYPE1r_ETHTYPE1f_GET(r) (((r).p1588_y1731_ethertype1[0]) & 0xffff)
#define PLP_P1588_Y1731_ETHERTYPE1r_ETHTYPE1f_SET(r,f) (r).p1588_y1731_ethertype1[0]=(((r).p1588_y1731_ethertype1[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_ETHERTYPE1.
 *
 */
#define PLP_READ_P1588_Y1731_ETHERTYPE1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_ETHERTYPE1r,(_r._p1588_y1731_ethertype1))
#define PLP_WRITE_P1588_Y1731_ETHERTYPE1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_ETHERTYPE1r,(_r._p1588_y1731_ethertype1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_ETHERTYPE1r PLP_P1588_Y1731_ETHERTYPE1r
#define P1588_Y1731_ETHERTYPE1r_SIZE PLP_P1588_Y1731_ETHERTYPE1r_SIZE
typedef PLP_P1588_Y1731_ETHERTYPE1r_t P1588_Y1731_ETHERTYPE1r_t;
#define P1588_Y1731_ETHERTYPE1r_CLR PLP_P1588_Y1731_ETHERTYPE1r_CLR
#define P1588_Y1731_ETHERTYPE1r_SET PLP_P1588_Y1731_ETHERTYPE1r_SET
#define P1588_Y1731_ETHERTYPE1r_GET PLP_P1588_Y1731_ETHERTYPE1r_GET
#define P1588_Y1731_ETHERTYPE1r_ETHTYPE1f_GET PLP_P1588_Y1731_ETHERTYPE1r_ETHTYPE1f_GET
#define P1588_Y1731_ETHERTYPE1r_ETHTYPE1f_SET PLP_P1588_Y1731_ETHERTYPE1r_ETHTYPE1f_SET
#define READ_P1588_Y1731_ETHERTYPE1r PLP_READ_P1588_Y1731_ETHERTYPE1r
#define WRITE_P1588_Y1731_ETHERTYPE1r PLP_WRITE_P1588_Y1731_ETHERTYPE1r
#define MODIFY_P1588_Y1731_ETHERTYPE1r PLP_MODIFY_P1588_Y1731_ETHERTYPE1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_ETHERTYPE1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_ETHERTYPE2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x708b
 * DESC:     Y1731 ETHERTYPE2 Register
 * RESETVAL: 0x1102 (4354)
 * ACCESS:   R/W
 * FIELDS:
 *     ETHTYPE2         EtherType for Tx(Y.1731 OAM) NTP
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_ETHERTYPE2r    0x0000008b

#define PLP_P1588_Y1731_ETHERTYPE2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_ETHERTYPE2.
 *
 */
typedef union PLP_P1588_Y1731_ETHERTYPE2r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_ethertype2[1];
	uint32_t _p1588_y1731_ethertype2;
} PLP_P1588_Y1731_ETHERTYPE2r_t;

#define PLP_P1588_Y1731_ETHERTYPE2r_CLR(r) (r).p1588_y1731_ethertype2[0] = 0
#define PLP_P1588_Y1731_ETHERTYPE2r_SET(r,d) (r).p1588_y1731_ethertype2[0] = d
#define PLP_P1588_Y1731_ETHERTYPE2r_GET(r) (r).p1588_y1731_ethertype2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_ETHERTYPE2r_ETHTYPE2f_GET(r) (((r).p1588_y1731_ethertype2[0]) & 0xffff)
#define PLP_P1588_Y1731_ETHERTYPE2r_ETHTYPE2f_SET(r,f) (r).p1588_y1731_ethertype2[0]=(((r).p1588_y1731_ethertype2[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_ETHERTYPE2.
 *
 */
#define PLP_READ_P1588_Y1731_ETHERTYPE2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_ETHERTYPE2r,(_r._p1588_y1731_ethertype2))
#define PLP_WRITE_P1588_Y1731_ETHERTYPE2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_ETHERTYPE2r,(_r._p1588_y1731_ethertype2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_ETHERTYPE2r PLP_P1588_Y1731_ETHERTYPE2r
#define P1588_Y1731_ETHERTYPE2r_SIZE PLP_P1588_Y1731_ETHERTYPE2r_SIZE
typedef PLP_P1588_Y1731_ETHERTYPE2r_t P1588_Y1731_ETHERTYPE2r_t;
#define P1588_Y1731_ETHERTYPE2r_CLR PLP_P1588_Y1731_ETHERTYPE2r_CLR
#define P1588_Y1731_ETHERTYPE2r_SET PLP_P1588_Y1731_ETHERTYPE2r_SET
#define P1588_Y1731_ETHERTYPE2r_GET PLP_P1588_Y1731_ETHERTYPE2r_GET
#define P1588_Y1731_ETHERTYPE2r_ETHTYPE2f_GET PLP_P1588_Y1731_ETHERTYPE2r_ETHTYPE2f_GET
#define P1588_Y1731_ETHERTYPE2r_ETHTYPE2f_SET PLP_P1588_Y1731_ETHERTYPE2r_ETHTYPE2f_SET
#define READ_P1588_Y1731_ETHERTYPE2r PLP_READ_P1588_Y1731_ETHERTYPE2r
#define WRITE_P1588_Y1731_ETHERTYPE2r PLP_WRITE_P1588_Y1731_ETHERTYPE2r
#define MODIFY_P1588_Y1731_ETHERTYPE2r PLP_MODIFY_P1588_Y1731_ETHERTYPE2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_ETHERTYPE2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_ETHERTYPE3
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x708c
 * DESC:     Y1731 ETHERTYPE3 Register
 * RESETVAL: 0x1103 (4355)
 * ACCESS:   R/W
 * FIELDS:
 *     ETHTYPE3         EtherType for Tx(BHH OAM) PTP
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_ETHERTYPE3r    0x0000008c

#define PLP_P1588_Y1731_ETHERTYPE3r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_ETHERTYPE3.
 *
 */
typedef union PLP_P1588_Y1731_ETHERTYPE3r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_ethertype3[1];
	uint32_t _p1588_y1731_ethertype3;
} PLP_P1588_Y1731_ETHERTYPE3r_t;

#define PLP_P1588_Y1731_ETHERTYPE3r_CLR(r) (r).p1588_y1731_ethertype3[0] = 0
#define PLP_P1588_Y1731_ETHERTYPE3r_SET(r,d) (r).p1588_y1731_ethertype3[0] = d
#define PLP_P1588_Y1731_ETHERTYPE3r_GET(r) (r).p1588_y1731_ethertype3[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_ETHERTYPE3r_ETHTYPE3f_GET(r) (((r).p1588_y1731_ethertype3[0]) & 0xffff)
#define PLP_P1588_Y1731_ETHERTYPE3r_ETHTYPE3f_SET(r,f) (r).p1588_y1731_ethertype3[0]=(((r).p1588_y1731_ethertype3[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_ETHERTYPE3.
 *
 */
#define PLP_READ_P1588_Y1731_ETHERTYPE3r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_ETHERTYPE3r,(_r._p1588_y1731_ethertype3))
#define PLP_WRITE_P1588_Y1731_ETHERTYPE3r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_ETHERTYPE3r,(_r._p1588_y1731_ethertype3))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_ETHERTYPE3r PLP_P1588_Y1731_ETHERTYPE3r
#define P1588_Y1731_ETHERTYPE3r_SIZE PLP_P1588_Y1731_ETHERTYPE3r_SIZE
typedef PLP_P1588_Y1731_ETHERTYPE3r_t P1588_Y1731_ETHERTYPE3r_t;
#define P1588_Y1731_ETHERTYPE3r_CLR PLP_P1588_Y1731_ETHERTYPE3r_CLR
#define P1588_Y1731_ETHERTYPE3r_SET PLP_P1588_Y1731_ETHERTYPE3r_SET
#define P1588_Y1731_ETHERTYPE3r_GET PLP_P1588_Y1731_ETHERTYPE3r_GET
#define P1588_Y1731_ETHERTYPE3r_ETHTYPE3f_GET PLP_P1588_Y1731_ETHERTYPE3r_ETHTYPE3f_GET
#define P1588_Y1731_ETHERTYPE3r_ETHTYPE3f_SET PLP_P1588_Y1731_ETHERTYPE3r_ETHTYPE3f_SET
#define READ_P1588_Y1731_ETHERTYPE3r PLP_READ_P1588_Y1731_ETHERTYPE3r
#define WRITE_P1588_Y1731_ETHERTYPE3r PLP_WRITE_P1588_Y1731_ETHERTYPE3r
#define MODIFY_P1588_Y1731_ETHERTYPE3r PLP_MODIFY_P1588_Y1731_ETHERTYPE3r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_ETHERTYPE3r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_ETHERTYPE4
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x708d
 * DESC:     Y1731 ETHERTYPE4 Register
 * RESETVAL: 0x1104 (4356)
 * ACCESS:   R/W
 * FIELDS:
 *     ETHTYPE4         EtherType for Tx(BHH OAM) NTP
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_ETHERTYPE4r    0x0000008d

#define PLP_P1588_Y1731_ETHERTYPE4r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_ETHERTYPE4.
 *
 */
typedef union PLP_P1588_Y1731_ETHERTYPE4r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_ethertype4[1];
	uint32_t _p1588_y1731_ethertype4;
} PLP_P1588_Y1731_ETHERTYPE4r_t;

#define PLP_P1588_Y1731_ETHERTYPE4r_CLR(r) (r).p1588_y1731_ethertype4[0] = 0
#define PLP_P1588_Y1731_ETHERTYPE4r_SET(r,d) (r).p1588_y1731_ethertype4[0] = d
#define PLP_P1588_Y1731_ETHERTYPE4r_GET(r) (r).p1588_y1731_ethertype4[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_ETHERTYPE4r_ETHTYPE4f_GET(r) (((r).p1588_y1731_ethertype4[0]) & 0xffff)
#define PLP_P1588_Y1731_ETHERTYPE4r_ETHTYPE4f_SET(r,f) (r).p1588_y1731_ethertype4[0]=(((r).p1588_y1731_ethertype4[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_ETHERTYPE4.
 *
 */
#define PLP_READ_P1588_Y1731_ETHERTYPE4r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_ETHERTYPE4r,(_r._p1588_y1731_ethertype4))
#define PLP_WRITE_P1588_Y1731_ETHERTYPE4r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_ETHERTYPE4r,(_r._p1588_y1731_ethertype4))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_ETHERTYPE4r PLP_P1588_Y1731_ETHERTYPE4r
#define P1588_Y1731_ETHERTYPE4r_SIZE PLP_P1588_Y1731_ETHERTYPE4r_SIZE
typedef PLP_P1588_Y1731_ETHERTYPE4r_t P1588_Y1731_ETHERTYPE4r_t;
#define P1588_Y1731_ETHERTYPE4r_CLR PLP_P1588_Y1731_ETHERTYPE4r_CLR
#define P1588_Y1731_ETHERTYPE4r_SET PLP_P1588_Y1731_ETHERTYPE4r_SET
#define P1588_Y1731_ETHERTYPE4r_GET PLP_P1588_Y1731_ETHERTYPE4r_GET
#define P1588_Y1731_ETHERTYPE4r_ETHTYPE4f_GET PLP_P1588_Y1731_ETHERTYPE4r_ETHTYPE4f_GET
#define P1588_Y1731_ETHERTYPE4r_ETHTYPE4f_SET PLP_P1588_Y1731_ETHERTYPE4r_ETHTYPE4f_SET
#define READ_P1588_Y1731_ETHERTYPE4r PLP_READ_P1588_Y1731_ETHERTYPE4r
#define WRITE_P1588_Y1731_ETHERTYPE4r PLP_WRITE_P1588_Y1731_ETHERTYPE4r
#define MODIFY_P1588_Y1731_ETHERTYPE4r PLP_MODIFY_P1588_Y1731_ETHERTYPE4r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_ETHERTYPE4r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_ETHERTYPE5
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x708e
 * DESC:     Y1731 ETHERTYPE5 Register
 * RESETVAL: 0x8902 (35074)
 * ACCESS:   R/W
 * FIELDS:
 *     ETHTYPE5         EtherType for Rx(Y.1731 OAM)
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_ETHERTYPE5r    0x0000008e

#define PLP_P1588_Y1731_ETHERTYPE5r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_ETHERTYPE5.
 *
 */
typedef union PLP_P1588_Y1731_ETHERTYPE5r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_ethertype5[1];
	uint32_t _p1588_y1731_ethertype5;
} PLP_P1588_Y1731_ETHERTYPE5r_t;

#define PLP_P1588_Y1731_ETHERTYPE5r_CLR(r) (r).p1588_y1731_ethertype5[0] = 0
#define PLP_P1588_Y1731_ETHERTYPE5r_SET(r,d) (r).p1588_y1731_ethertype5[0] = d
#define PLP_P1588_Y1731_ETHERTYPE5r_GET(r) (r).p1588_y1731_ethertype5[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_ETHERTYPE5r_ETHTYPE5f_GET(r) (((r).p1588_y1731_ethertype5[0]) & 0xffff)
#define PLP_P1588_Y1731_ETHERTYPE5r_ETHTYPE5f_SET(r,f) (r).p1588_y1731_ethertype5[0]=(((r).p1588_y1731_ethertype5[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_ETHERTYPE5.
 *
 */
#define PLP_READ_P1588_Y1731_ETHERTYPE5r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_ETHERTYPE5r,(_r._p1588_y1731_ethertype5))
#define PLP_WRITE_P1588_Y1731_ETHERTYPE5r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_ETHERTYPE5r,(_r._p1588_y1731_ethertype5))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_ETHERTYPE5r PLP_P1588_Y1731_ETHERTYPE5r
#define P1588_Y1731_ETHERTYPE5r_SIZE PLP_P1588_Y1731_ETHERTYPE5r_SIZE
typedef PLP_P1588_Y1731_ETHERTYPE5r_t P1588_Y1731_ETHERTYPE5r_t;
#define P1588_Y1731_ETHERTYPE5r_CLR PLP_P1588_Y1731_ETHERTYPE5r_CLR
#define P1588_Y1731_ETHERTYPE5r_SET PLP_P1588_Y1731_ETHERTYPE5r_SET
#define P1588_Y1731_ETHERTYPE5r_GET PLP_P1588_Y1731_ETHERTYPE5r_GET
#define P1588_Y1731_ETHERTYPE5r_ETHTYPE5f_GET PLP_P1588_Y1731_ETHERTYPE5r_ETHTYPE5f_GET
#define P1588_Y1731_ETHERTYPE5r_ETHTYPE5f_SET PLP_P1588_Y1731_ETHERTYPE5r_ETHTYPE5f_SET
#define READ_P1588_Y1731_ETHERTYPE5r PLP_READ_P1588_Y1731_ETHERTYPE5r
#define WRITE_P1588_Y1731_ETHERTYPE5r PLP_WRITE_P1588_Y1731_ETHERTYPE5r
#define MODIFY_P1588_Y1731_ETHERTYPE5r PLP_MODIFY_P1588_Y1731_ETHERTYPE5r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_ETHERTYPE5r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_ETHERTYPE6
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x708f
 * DESC:     Y1731 ETHERTYPE6 Register
 * RESETVAL: 0x8847 (34887)
 * ACCESS:   R/W
 * FIELDS:
 *     ETHTYPE6         EtherType for MPLS Unicast
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_ETHERTYPE6r    0x0000008f

#define PLP_P1588_Y1731_ETHERTYPE6r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_ETHERTYPE6.
 *
 */
typedef union PLP_P1588_Y1731_ETHERTYPE6r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_ethertype6[1];
	uint32_t _p1588_y1731_ethertype6;
} PLP_P1588_Y1731_ETHERTYPE6r_t;

#define PLP_P1588_Y1731_ETHERTYPE6r_CLR(r) (r).p1588_y1731_ethertype6[0] = 0
#define PLP_P1588_Y1731_ETHERTYPE6r_SET(r,d) (r).p1588_y1731_ethertype6[0] = d
#define PLP_P1588_Y1731_ETHERTYPE6r_GET(r) (r).p1588_y1731_ethertype6[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_ETHERTYPE6r_ETHTYPE6f_GET(r) (((r).p1588_y1731_ethertype6[0]) & 0xffff)
#define PLP_P1588_Y1731_ETHERTYPE6r_ETHTYPE6f_SET(r,f) (r).p1588_y1731_ethertype6[0]=(((r).p1588_y1731_ethertype6[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_ETHERTYPE6.
 *
 */
#define PLP_READ_P1588_Y1731_ETHERTYPE6r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_ETHERTYPE6r,(_r._p1588_y1731_ethertype6))
#define PLP_WRITE_P1588_Y1731_ETHERTYPE6r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_ETHERTYPE6r,(_r._p1588_y1731_ethertype6))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_ETHERTYPE6r PLP_P1588_Y1731_ETHERTYPE6r
#define P1588_Y1731_ETHERTYPE6r_SIZE PLP_P1588_Y1731_ETHERTYPE6r_SIZE
typedef PLP_P1588_Y1731_ETHERTYPE6r_t P1588_Y1731_ETHERTYPE6r_t;
#define P1588_Y1731_ETHERTYPE6r_CLR PLP_P1588_Y1731_ETHERTYPE6r_CLR
#define P1588_Y1731_ETHERTYPE6r_SET PLP_P1588_Y1731_ETHERTYPE6r_SET
#define P1588_Y1731_ETHERTYPE6r_GET PLP_P1588_Y1731_ETHERTYPE6r_GET
#define P1588_Y1731_ETHERTYPE6r_ETHTYPE6f_GET PLP_P1588_Y1731_ETHERTYPE6r_ETHTYPE6f_GET
#define P1588_Y1731_ETHERTYPE6r_ETHTYPE6f_SET PLP_P1588_Y1731_ETHERTYPE6r_ETHTYPE6f_SET
#define READ_P1588_Y1731_ETHERTYPE6r PLP_READ_P1588_Y1731_ETHERTYPE6r
#define WRITE_P1588_Y1731_ETHERTYPE6r PLP_WRITE_P1588_Y1731_ETHERTYPE6r
#define MODIFY_P1588_Y1731_ETHERTYPE6r PLP_MODIFY_P1588_Y1731_ETHERTYPE6r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_ETHERTYPE6r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_ETHERTYPE7
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7090
 * DESC:     Y1731 ETHERTYPE7 Register
 * RESETVAL: 0x8848 (34888)
 * ACCESS:   R/W
 * FIELDS:
 *     ETHTYPE7         EtherType for MPLS Multicast
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_ETHERTYPE7r    0x00000090

#define PLP_P1588_Y1731_ETHERTYPE7r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_ETHERTYPE7.
 *
 */
typedef union PLP_P1588_Y1731_ETHERTYPE7r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_ethertype7[1];
	uint32_t _p1588_y1731_ethertype7;
} PLP_P1588_Y1731_ETHERTYPE7r_t;

#define PLP_P1588_Y1731_ETHERTYPE7r_CLR(r) (r).p1588_y1731_ethertype7[0] = 0
#define PLP_P1588_Y1731_ETHERTYPE7r_SET(r,d) (r).p1588_y1731_ethertype7[0] = d
#define PLP_P1588_Y1731_ETHERTYPE7r_GET(r) (r).p1588_y1731_ethertype7[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_ETHERTYPE7r_ETHTYPE7f_GET(r) (((r).p1588_y1731_ethertype7[0]) & 0xffff)
#define PLP_P1588_Y1731_ETHERTYPE7r_ETHTYPE7f_SET(r,f) (r).p1588_y1731_ethertype7[0]=(((r).p1588_y1731_ethertype7[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_ETHERTYPE7.
 *
 */
#define PLP_READ_P1588_Y1731_ETHERTYPE7r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_ETHERTYPE7r,(_r._p1588_y1731_ethertype7))
#define PLP_WRITE_P1588_Y1731_ETHERTYPE7r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_ETHERTYPE7r,(_r._p1588_y1731_ethertype7))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_ETHERTYPE7r PLP_P1588_Y1731_ETHERTYPE7r
#define P1588_Y1731_ETHERTYPE7r_SIZE PLP_P1588_Y1731_ETHERTYPE7r_SIZE
typedef PLP_P1588_Y1731_ETHERTYPE7r_t P1588_Y1731_ETHERTYPE7r_t;
#define P1588_Y1731_ETHERTYPE7r_CLR PLP_P1588_Y1731_ETHERTYPE7r_CLR
#define P1588_Y1731_ETHERTYPE7r_SET PLP_P1588_Y1731_ETHERTYPE7r_SET
#define P1588_Y1731_ETHERTYPE7r_GET PLP_P1588_Y1731_ETHERTYPE7r_GET
#define P1588_Y1731_ETHERTYPE7r_ETHTYPE7f_GET PLP_P1588_Y1731_ETHERTYPE7r_ETHTYPE7f_GET
#define P1588_Y1731_ETHERTYPE7r_ETHTYPE7f_SET PLP_P1588_Y1731_ETHERTYPE7r_ETHTYPE7f_SET
#define READ_P1588_Y1731_ETHERTYPE7r PLP_READ_P1588_Y1731_ETHERTYPE7r
#define WRITE_P1588_Y1731_ETHERTYPE7r PLP_WRITE_P1588_Y1731_ETHERTYPE7r
#define MODIFY_P1588_Y1731_ETHERTYPE7r PLP_MODIFY_P1588_Y1731_ETHERTYPE7r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_ETHERTYPE7r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_ETHERTYPE8
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7091
 * DESC:     Y1731 ETHERTYPE8 Register
 * RESETVAL: 0x1105 (4357)
 * ACCESS:   R/W
 * FIELDS:
 *     ETHTYPE8         EtherType for Rx(BHH DM)
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_ETHERTYPE8r    0x00000091

#define PLP_P1588_Y1731_ETHERTYPE8r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_ETHERTYPE8.
 *
 */
typedef union PLP_P1588_Y1731_ETHERTYPE8r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_ethertype8[1];
	uint32_t _p1588_y1731_ethertype8;
} PLP_P1588_Y1731_ETHERTYPE8r_t;

#define PLP_P1588_Y1731_ETHERTYPE8r_CLR(r) (r).p1588_y1731_ethertype8[0] = 0
#define PLP_P1588_Y1731_ETHERTYPE8r_SET(r,d) (r).p1588_y1731_ethertype8[0] = d
#define PLP_P1588_Y1731_ETHERTYPE8r_GET(r) (r).p1588_y1731_ethertype8[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_ETHERTYPE8r_ETHTYPE8f_GET(r) (((r).p1588_y1731_ethertype8[0]) & 0xffff)
#define PLP_P1588_Y1731_ETHERTYPE8r_ETHTYPE8f_SET(r,f) (r).p1588_y1731_ethertype8[0]=(((r).p1588_y1731_ethertype8[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_ETHERTYPE8.
 *
 */
#define PLP_READ_P1588_Y1731_ETHERTYPE8r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_ETHERTYPE8r,(_r._p1588_y1731_ethertype8))
#define PLP_WRITE_P1588_Y1731_ETHERTYPE8r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_ETHERTYPE8r,(_r._p1588_y1731_ethertype8))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_ETHERTYPE8r PLP_P1588_Y1731_ETHERTYPE8r
#define P1588_Y1731_ETHERTYPE8r_SIZE PLP_P1588_Y1731_ETHERTYPE8r_SIZE
typedef PLP_P1588_Y1731_ETHERTYPE8r_t P1588_Y1731_ETHERTYPE8r_t;
#define P1588_Y1731_ETHERTYPE8r_CLR PLP_P1588_Y1731_ETHERTYPE8r_CLR
#define P1588_Y1731_ETHERTYPE8r_SET PLP_P1588_Y1731_ETHERTYPE8r_SET
#define P1588_Y1731_ETHERTYPE8r_GET PLP_P1588_Y1731_ETHERTYPE8r_GET
#define P1588_Y1731_ETHERTYPE8r_ETHTYPE8f_GET PLP_P1588_Y1731_ETHERTYPE8r_ETHTYPE8f_GET
#define P1588_Y1731_ETHERTYPE8r_ETHTYPE8f_SET PLP_P1588_Y1731_ETHERTYPE8r_ETHTYPE8f_SET
#define READ_P1588_Y1731_ETHERTYPE8r PLP_READ_P1588_Y1731_ETHERTYPE8r
#define WRITE_P1588_Y1731_ETHERTYPE8r PLP_WRITE_P1588_Y1731_ETHERTYPE8r
#define MODIFY_P1588_Y1731_ETHERTYPE8r PLP_MODIFY_P1588_Y1731_ETHERTYPE8r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_ETHERTYPE8r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_ETHERTYPE9
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7092
 * DESC:     Y1731 ETHERTYPE9 Register
 * RESETVAL: 0x1106 (4358)
 * ACCESS:   R/W
 * FIELDS:
 *     ETHTYPE9         EtherType for Tx(IETF DM)
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_ETHERTYPE9r    0x00000092

#define PLP_P1588_Y1731_ETHERTYPE9r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_ETHERTYPE9.
 *
 */
typedef union PLP_P1588_Y1731_ETHERTYPE9r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_ethertype9[1];
	uint32_t _p1588_y1731_ethertype9;
} PLP_P1588_Y1731_ETHERTYPE9r_t;

#define PLP_P1588_Y1731_ETHERTYPE9r_CLR(r) (r).p1588_y1731_ethertype9[0] = 0
#define PLP_P1588_Y1731_ETHERTYPE9r_SET(r,d) (r).p1588_y1731_ethertype9[0] = d
#define PLP_P1588_Y1731_ETHERTYPE9r_GET(r) (r).p1588_y1731_ethertype9[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_ETHERTYPE9r_ETHTYPE9f_GET(r) (((r).p1588_y1731_ethertype9[0]) & 0xffff)
#define PLP_P1588_Y1731_ETHERTYPE9r_ETHTYPE9f_SET(r,f) (r).p1588_y1731_ethertype9[0]=(((r).p1588_y1731_ethertype9[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_ETHERTYPE9.
 *
 */
#define PLP_READ_P1588_Y1731_ETHERTYPE9r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_ETHERTYPE9r,(_r._p1588_y1731_ethertype9))
#define PLP_WRITE_P1588_Y1731_ETHERTYPE9r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_ETHERTYPE9r,(_r._p1588_y1731_ethertype9))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_ETHERTYPE9r PLP_P1588_Y1731_ETHERTYPE9r
#define P1588_Y1731_ETHERTYPE9r_SIZE PLP_P1588_Y1731_ETHERTYPE9r_SIZE
typedef PLP_P1588_Y1731_ETHERTYPE9r_t P1588_Y1731_ETHERTYPE9r_t;
#define P1588_Y1731_ETHERTYPE9r_CLR PLP_P1588_Y1731_ETHERTYPE9r_CLR
#define P1588_Y1731_ETHERTYPE9r_SET PLP_P1588_Y1731_ETHERTYPE9r_SET
#define P1588_Y1731_ETHERTYPE9r_GET PLP_P1588_Y1731_ETHERTYPE9r_GET
#define P1588_Y1731_ETHERTYPE9r_ETHTYPE9f_GET PLP_P1588_Y1731_ETHERTYPE9r_ETHTYPE9f_GET
#define P1588_Y1731_ETHERTYPE9r_ETHTYPE9f_SET PLP_P1588_Y1731_ETHERTYPE9r_ETHTYPE9f_SET
#define READ_P1588_Y1731_ETHERTYPE9r PLP_READ_P1588_Y1731_ETHERTYPE9r
#define WRITE_P1588_Y1731_ETHERTYPE9r PLP_WRITE_P1588_Y1731_ETHERTYPE9r
#define MODIFY_P1588_Y1731_ETHERTYPE9r PLP_MODIFY_P1588_Y1731_ETHERTYPE9r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_ETHERTYPE9r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_ETHERTYPE10
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7093
 * DESC:     Y1731 ETHERTYPE10 Register
 * RESETVAL: 0x1107 (4359)
 * ACCESS:   R/W
 * FIELDS:
 *     ETHTYPE10        EtherType for Rx(IETF DM)
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_ETHERTYPE10r    0x00000093

#define PLP_P1588_Y1731_ETHERTYPE10r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_ETHERTYPE10.
 *
 */
typedef union PLP_P1588_Y1731_ETHERTYPE10r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_ethertype10[1];
	uint32_t _p1588_y1731_ethertype10;
} PLP_P1588_Y1731_ETHERTYPE10r_t;

#define PLP_P1588_Y1731_ETHERTYPE10r_CLR(r) (r).p1588_y1731_ethertype10[0] = 0
#define PLP_P1588_Y1731_ETHERTYPE10r_SET(r,d) (r).p1588_y1731_ethertype10[0] = d
#define PLP_P1588_Y1731_ETHERTYPE10r_GET(r) (r).p1588_y1731_ethertype10[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_ETHERTYPE10r_ETHTYPE10f_GET(r) (((r).p1588_y1731_ethertype10[0]) & 0xffff)
#define PLP_P1588_Y1731_ETHERTYPE10r_ETHTYPE10f_SET(r,f) (r).p1588_y1731_ethertype10[0]=(((r).p1588_y1731_ethertype10[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_ETHERTYPE10.
 *
 */
#define PLP_READ_P1588_Y1731_ETHERTYPE10r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_ETHERTYPE10r,(_r._p1588_y1731_ethertype10))
#define PLP_WRITE_P1588_Y1731_ETHERTYPE10r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_ETHERTYPE10r,(_r._p1588_y1731_ethertype10))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_ETHERTYPE10r PLP_P1588_Y1731_ETHERTYPE10r
#define P1588_Y1731_ETHERTYPE10r_SIZE PLP_P1588_Y1731_ETHERTYPE10r_SIZE
typedef PLP_P1588_Y1731_ETHERTYPE10r_t P1588_Y1731_ETHERTYPE10r_t;
#define P1588_Y1731_ETHERTYPE10r_CLR PLP_P1588_Y1731_ETHERTYPE10r_CLR
#define P1588_Y1731_ETHERTYPE10r_SET PLP_P1588_Y1731_ETHERTYPE10r_SET
#define P1588_Y1731_ETHERTYPE10r_GET PLP_P1588_Y1731_ETHERTYPE10r_GET
#define P1588_Y1731_ETHERTYPE10r_ETHTYPE10f_GET PLP_P1588_Y1731_ETHERTYPE10r_ETHTYPE10f_GET
#define P1588_Y1731_ETHERTYPE10r_ETHTYPE10f_SET PLP_P1588_Y1731_ETHERTYPE10r_ETHTYPE10f_SET
#define READ_P1588_Y1731_ETHERTYPE10r PLP_READ_P1588_Y1731_ETHERTYPE10r
#define WRITE_P1588_Y1731_ETHERTYPE10r PLP_WRITE_P1588_Y1731_ETHERTYPE10r
#define MODIFY_P1588_Y1731_ETHERTYPE10r PLP_MODIFY_P1588_Y1731_ETHERTYPE10r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_ETHERTYPE10r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_ETHERTYPE11
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7094
 * DESC:     Y1731 ETHERTYPE11 Register
 * RESETVAL: 0x1108 (4360)
 * ACCESS:   R/W
 * FIELDS:
 *     ETHTYPE11        EtherType for Tx(BHH OAM) PTP
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_ETHERTYPE11r    0x00000094

#define PLP_P1588_Y1731_ETHERTYPE11r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_ETHERTYPE11.
 *
 */
typedef union PLP_P1588_Y1731_ETHERTYPE11r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_ethertype11[1];
	uint32_t _p1588_y1731_ethertype11;
} PLP_P1588_Y1731_ETHERTYPE11r_t;

#define PLP_P1588_Y1731_ETHERTYPE11r_CLR(r) (r).p1588_y1731_ethertype11[0] = 0
#define PLP_P1588_Y1731_ETHERTYPE11r_SET(r,d) (r).p1588_y1731_ethertype11[0] = d
#define PLP_P1588_Y1731_ETHERTYPE11r_GET(r) (r).p1588_y1731_ethertype11[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_ETHERTYPE11r_ETHTYPE11f_GET(r) (((r).p1588_y1731_ethertype11[0]) & 0xffff)
#define PLP_P1588_Y1731_ETHERTYPE11r_ETHTYPE11f_SET(r,f) (r).p1588_y1731_ethertype11[0]=(((r).p1588_y1731_ethertype11[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_ETHERTYPE11.
 *
 */
#define PLP_READ_P1588_Y1731_ETHERTYPE11r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_ETHERTYPE11r,(_r._p1588_y1731_ethertype11))
#define PLP_WRITE_P1588_Y1731_ETHERTYPE11r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_ETHERTYPE11r,(_r._p1588_y1731_ethertype11))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_ETHERTYPE11r PLP_P1588_Y1731_ETHERTYPE11r
#define P1588_Y1731_ETHERTYPE11r_SIZE PLP_P1588_Y1731_ETHERTYPE11r_SIZE
typedef PLP_P1588_Y1731_ETHERTYPE11r_t P1588_Y1731_ETHERTYPE11r_t;
#define P1588_Y1731_ETHERTYPE11r_CLR PLP_P1588_Y1731_ETHERTYPE11r_CLR
#define P1588_Y1731_ETHERTYPE11r_SET PLP_P1588_Y1731_ETHERTYPE11r_SET
#define P1588_Y1731_ETHERTYPE11r_GET PLP_P1588_Y1731_ETHERTYPE11r_GET
#define P1588_Y1731_ETHERTYPE11r_ETHTYPE11f_GET PLP_P1588_Y1731_ETHERTYPE11r_ETHTYPE11f_GET
#define P1588_Y1731_ETHERTYPE11r_ETHTYPE11f_SET PLP_P1588_Y1731_ETHERTYPE11r_ETHTYPE11f_SET
#define READ_P1588_Y1731_ETHERTYPE11r PLP_READ_P1588_Y1731_ETHERTYPE11r
#define WRITE_P1588_Y1731_ETHERTYPE11r PLP_WRITE_P1588_Y1731_ETHERTYPE11r
#define MODIFY_P1588_Y1731_ETHERTYPE11r PLP_MODIFY_P1588_Y1731_ETHERTYPE11r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_ETHERTYPE11r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_ETHERTYPE12
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7095
 * DESC:     Y1731 ETHERTYPE12 Register
 * RESETVAL: 0x1109 (4361)
 * ACCESS:   R/W
 * FIELDS:
 *     ETHTYPE12        EtherType for Tx(BHH OAM) NTP
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_ETHERTYPE12r    0x00000095

#define PLP_P1588_Y1731_ETHERTYPE12r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_ETHERTYPE12.
 *
 */
typedef union PLP_P1588_Y1731_ETHERTYPE12r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_ethertype12[1];
	uint32_t _p1588_y1731_ethertype12;
} PLP_P1588_Y1731_ETHERTYPE12r_t;

#define PLP_P1588_Y1731_ETHERTYPE12r_CLR(r) (r).p1588_y1731_ethertype12[0] = 0
#define PLP_P1588_Y1731_ETHERTYPE12r_SET(r,d) (r).p1588_y1731_ethertype12[0] = d
#define PLP_P1588_Y1731_ETHERTYPE12r_GET(r) (r).p1588_y1731_ethertype12[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_ETHERTYPE12r_ETHTYPE12f_GET(r) (((r).p1588_y1731_ethertype12[0]) & 0xffff)
#define PLP_P1588_Y1731_ETHERTYPE12r_ETHTYPE12f_SET(r,f) (r).p1588_y1731_ethertype12[0]=(((r).p1588_y1731_ethertype12[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_ETHERTYPE12.
 *
 */
#define PLP_READ_P1588_Y1731_ETHERTYPE12r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_ETHERTYPE12r,(_r._p1588_y1731_ethertype12))
#define PLP_WRITE_P1588_Y1731_ETHERTYPE12r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_ETHERTYPE12r,(_r._p1588_y1731_ethertype12))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_ETHERTYPE12r PLP_P1588_Y1731_ETHERTYPE12r
#define P1588_Y1731_ETHERTYPE12r_SIZE PLP_P1588_Y1731_ETHERTYPE12r_SIZE
typedef PLP_P1588_Y1731_ETHERTYPE12r_t P1588_Y1731_ETHERTYPE12r_t;
#define P1588_Y1731_ETHERTYPE12r_CLR PLP_P1588_Y1731_ETHERTYPE12r_CLR
#define P1588_Y1731_ETHERTYPE12r_SET PLP_P1588_Y1731_ETHERTYPE12r_SET
#define P1588_Y1731_ETHERTYPE12r_GET PLP_P1588_Y1731_ETHERTYPE12r_GET
#define P1588_Y1731_ETHERTYPE12r_ETHTYPE12f_GET PLP_P1588_Y1731_ETHERTYPE12r_ETHTYPE12f_GET
#define P1588_Y1731_ETHERTYPE12r_ETHTYPE12f_SET PLP_P1588_Y1731_ETHERTYPE12r_ETHTYPE12f_SET
#define READ_P1588_Y1731_ETHERTYPE12r PLP_READ_P1588_Y1731_ETHERTYPE12r
#define WRITE_P1588_Y1731_ETHERTYPE12r PLP_WRITE_P1588_Y1731_ETHERTYPE12r
#define MODIFY_P1588_Y1731_ETHERTYPE12r PLP_MODIFY_P1588_Y1731_ETHERTYPE12r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_ETHERTYPE12r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_ETHERTYPE13
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7096
 * DESC:     Y1731 ETHERTYPE13 Register
 * RESETVAL: 0x110a (4362)
 * ACCESS:   R/W
 * FIELDS:
 *     ETHTYPE13        EtherType for Tx(IETF DM)
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_ETHERTYPE13r    0x00000096

#define PLP_P1588_Y1731_ETHERTYPE13r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_ETHERTYPE13.
 *
 */
typedef union PLP_P1588_Y1731_ETHERTYPE13r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_ethertype13[1];
	uint32_t _p1588_y1731_ethertype13;
} PLP_P1588_Y1731_ETHERTYPE13r_t;

#define PLP_P1588_Y1731_ETHERTYPE13r_CLR(r) (r).p1588_y1731_ethertype13[0] = 0
#define PLP_P1588_Y1731_ETHERTYPE13r_SET(r,d) (r).p1588_y1731_ethertype13[0] = d
#define PLP_P1588_Y1731_ETHERTYPE13r_GET(r) (r).p1588_y1731_ethertype13[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_ETHERTYPE13r_ETHTYPE13f_GET(r) (((r).p1588_y1731_ethertype13[0]) & 0xffff)
#define PLP_P1588_Y1731_ETHERTYPE13r_ETHTYPE13f_SET(r,f) (r).p1588_y1731_ethertype13[0]=(((r).p1588_y1731_ethertype13[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_ETHERTYPE13.
 *
 */
#define PLP_READ_P1588_Y1731_ETHERTYPE13r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_ETHERTYPE13r,(_r._p1588_y1731_ethertype13))
#define PLP_WRITE_P1588_Y1731_ETHERTYPE13r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_ETHERTYPE13r,(_r._p1588_y1731_ethertype13))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_ETHERTYPE13r PLP_P1588_Y1731_ETHERTYPE13r
#define P1588_Y1731_ETHERTYPE13r_SIZE PLP_P1588_Y1731_ETHERTYPE13r_SIZE
typedef PLP_P1588_Y1731_ETHERTYPE13r_t P1588_Y1731_ETHERTYPE13r_t;
#define P1588_Y1731_ETHERTYPE13r_CLR PLP_P1588_Y1731_ETHERTYPE13r_CLR
#define P1588_Y1731_ETHERTYPE13r_SET PLP_P1588_Y1731_ETHERTYPE13r_SET
#define P1588_Y1731_ETHERTYPE13r_GET PLP_P1588_Y1731_ETHERTYPE13r_GET
#define P1588_Y1731_ETHERTYPE13r_ETHTYPE13f_GET PLP_P1588_Y1731_ETHERTYPE13r_ETHTYPE13f_GET
#define P1588_Y1731_ETHERTYPE13r_ETHTYPE13f_SET PLP_P1588_Y1731_ETHERTYPE13r_ETHTYPE13f_SET
#define READ_P1588_Y1731_ETHERTYPE13r PLP_READ_P1588_Y1731_ETHERTYPE13r
#define WRITE_P1588_Y1731_ETHERTYPE13r PLP_WRITE_P1588_Y1731_ETHERTYPE13r
#define MODIFY_P1588_Y1731_ETHERTYPE13r PLP_MODIFY_P1588_Y1731_ETHERTYPE13r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_ETHERTYPE13r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_OPCODE1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7097
 * DESC:     Y1731 OPCODE1 Register
 * RESETVAL: 0x2f2d (12077)
 * ACCESS:   R/W
 * FIELDS:
 *     OPCODE1          Opcode for 1DM packets
 *     OPCODE2          Opcode for DMM packets
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_OPCODE1r    0x00000097

#define PLP_P1588_Y1731_OPCODE1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_OPCODE1.
 *
 */
typedef union PLP_P1588_Y1731_OPCODE1r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_opcode1[1];
	uint32_t _p1588_y1731_opcode1;
} PLP_P1588_Y1731_OPCODE1r_t;

#define PLP_P1588_Y1731_OPCODE1r_CLR(r) (r).p1588_y1731_opcode1[0] = 0
#define PLP_P1588_Y1731_OPCODE1r_SET(r,d) (r).p1588_y1731_opcode1[0] = d
#define PLP_P1588_Y1731_OPCODE1r_GET(r) (r).p1588_y1731_opcode1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_OPCODE1r_OPCODE2f_GET(r) ((((r).p1588_y1731_opcode1[0]) >> 8) & 0xff)
#define PLP_P1588_Y1731_OPCODE1r_OPCODE2f_SET(r,f) (r).p1588_y1731_opcode1[0]=(((r).p1588_y1731_opcode1[0] & ~((uint32_t)0xff << 8)) | ((((uint32_t)f) & 0xff) << 8)) | (255 << (16 + 8))
#define PLP_P1588_Y1731_OPCODE1r_OPCODE1f_GET(r) (((r).p1588_y1731_opcode1[0]) & 0xff)
#define PLP_P1588_Y1731_OPCODE1r_OPCODE1f_SET(r,f) (r).p1588_y1731_opcode1[0]=(((r).p1588_y1731_opcode1[0] & ~((uint32_t)0xff)) | (((uint32_t)f) & 0xff)) | (0xff << 16)

/*
 * These macros can be used to access P1588_Y1731_OPCODE1.
 *
 */
#define PLP_READ_P1588_Y1731_OPCODE1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_OPCODE1r,(_r._p1588_y1731_opcode1))
#define PLP_WRITE_P1588_Y1731_OPCODE1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_OPCODE1r,(_r._p1588_y1731_opcode1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_OPCODE1r PLP_P1588_Y1731_OPCODE1r
#define P1588_Y1731_OPCODE1r_SIZE PLP_P1588_Y1731_OPCODE1r_SIZE
typedef PLP_P1588_Y1731_OPCODE1r_t P1588_Y1731_OPCODE1r_t;
#define P1588_Y1731_OPCODE1r_CLR PLP_P1588_Y1731_OPCODE1r_CLR
#define P1588_Y1731_OPCODE1r_SET PLP_P1588_Y1731_OPCODE1r_SET
#define P1588_Y1731_OPCODE1r_GET PLP_P1588_Y1731_OPCODE1r_GET
#define P1588_Y1731_OPCODE1r_OPCODE2f_GET PLP_P1588_Y1731_OPCODE1r_OPCODE2f_GET
#define P1588_Y1731_OPCODE1r_OPCODE2f_SET PLP_P1588_Y1731_OPCODE1r_OPCODE2f_SET
#define P1588_Y1731_OPCODE1r_OPCODE1f_GET PLP_P1588_Y1731_OPCODE1r_OPCODE1f_GET
#define P1588_Y1731_OPCODE1r_OPCODE1f_SET PLP_P1588_Y1731_OPCODE1r_OPCODE1f_SET
#define READ_P1588_Y1731_OPCODE1r PLP_READ_P1588_Y1731_OPCODE1r
#define WRITE_P1588_Y1731_OPCODE1r PLP_WRITE_P1588_Y1731_OPCODE1r
#define MODIFY_P1588_Y1731_OPCODE1r PLP_MODIFY_P1588_Y1731_OPCODE1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_OPCODE1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_OPCODE2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7098
 * DESC:     Y1731 OPCODE2 Register
 * RESETVAL: 0x2e (46)
 * ACCESS:   R/W
 * FIELDS:
 *     OPCODE3          Opcode for DMR packets
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_OPCODE2r    0x00000098

#define PLP_P1588_Y1731_OPCODE2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_OPCODE2.
 *
 */
typedef union PLP_P1588_Y1731_OPCODE2r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_opcode2[1];
	uint32_t _p1588_y1731_opcode2;
} PLP_P1588_Y1731_OPCODE2r_t;

#define PLP_P1588_Y1731_OPCODE2r_CLR(r) (r).p1588_y1731_opcode2[0] = 0
#define PLP_P1588_Y1731_OPCODE2r_SET(r,d) (r).p1588_y1731_opcode2[0] = d
#define PLP_P1588_Y1731_OPCODE2r_GET(r) (r).p1588_y1731_opcode2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_OPCODE2r_OPCODE3f_GET(r) (((r).p1588_y1731_opcode2[0]) & 0xff)
#define PLP_P1588_Y1731_OPCODE2r_OPCODE3f_SET(r,f) (r).p1588_y1731_opcode2[0]=(((r).p1588_y1731_opcode2[0] & ~((uint32_t)0xff)) | (((uint32_t)f) & 0xff)) | (0xff << 16)

/*
 * These macros can be used to access P1588_Y1731_OPCODE2.
 *
 */
#define PLP_READ_P1588_Y1731_OPCODE2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_OPCODE2r,(_r._p1588_y1731_opcode2))
#define PLP_WRITE_P1588_Y1731_OPCODE2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_OPCODE2r,(_r._p1588_y1731_opcode2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_OPCODE2r PLP_P1588_Y1731_OPCODE2r
#define P1588_Y1731_OPCODE2r_SIZE PLP_P1588_Y1731_OPCODE2r_SIZE
typedef PLP_P1588_Y1731_OPCODE2r_t P1588_Y1731_OPCODE2r_t;
#define P1588_Y1731_OPCODE2r_CLR PLP_P1588_Y1731_OPCODE2r_CLR
#define P1588_Y1731_OPCODE2r_SET PLP_P1588_Y1731_OPCODE2r_SET
#define P1588_Y1731_OPCODE2r_GET PLP_P1588_Y1731_OPCODE2r_GET
#define P1588_Y1731_OPCODE2r_OPCODE3f_GET PLP_P1588_Y1731_OPCODE2r_OPCODE3f_GET
#define P1588_Y1731_OPCODE2r_OPCODE3f_SET PLP_P1588_Y1731_OPCODE2r_OPCODE3f_SET
#define READ_P1588_Y1731_OPCODE2r PLP_READ_P1588_Y1731_OPCODE2r
#define WRITE_P1588_Y1731_OPCODE2r PLP_WRITE_P1588_Y1731_OPCODE2r
#define MODIFY_P1588_Y1731_OPCODE2r PLP_MODIFY_P1588_Y1731_OPCODE2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_OPCODE2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_MAC_ADDR1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x7099
 * DESC:     Y1731 MAC ADDR1 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MAC_ADDR1_15_0   [15:0] of Mac Address1 to match for DA/SA
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_MAC_ADDR1r    0x00000099

#define PLP_P1588_Y1731_MAC_ADDR1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_MAC_ADDR1.
 *
 */
typedef union PLP_P1588_Y1731_MAC_ADDR1r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_mac_addr1[1];
	uint32_t _p1588_y1731_mac_addr1;
} PLP_P1588_Y1731_MAC_ADDR1r_t;

#define PLP_P1588_Y1731_MAC_ADDR1r_CLR(r) (r).p1588_y1731_mac_addr1[0] = 0
#define PLP_P1588_Y1731_MAC_ADDR1r_SET(r,d) (r).p1588_y1731_mac_addr1[0] = d
#define PLP_P1588_Y1731_MAC_ADDR1r_GET(r) (r).p1588_y1731_mac_addr1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_MAC_ADDR1r_MAC_ADDR1_15_0f_GET(r) (((r).p1588_y1731_mac_addr1[0]) & 0xffff)
#define PLP_P1588_Y1731_MAC_ADDR1r_MAC_ADDR1_15_0f_SET(r,f) (r).p1588_y1731_mac_addr1[0]=(((r).p1588_y1731_mac_addr1[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_MAC_ADDR1.
 *
 */
#define PLP_READ_P1588_Y1731_MAC_ADDR1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_MAC_ADDR1r,(_r._p1588_y1731_mac_addr1))
#define PLP_WRITE_P1588_Y1731_MAC_ADDR1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_MAC_ADDR1r,(_r._p1588_y1731_mac_addr1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_MAC_ADDR1r PLP_P1588_Y1731_MAC_ADDR1r
#define P1588_Y1731_MAC_ADDR1r_SIZE PLP_P1588_Y1731_MAC_ADDR1r_SIZE
typedef PLP_P1588_Y1731_MAC_ADDR1r_t P1588_Y1731_MAC_ADDR1r_t;
#define P1588_Y1731_MAC_ADDR1r_CLR PLP_P1588_Y1731_MAC_ADDR1r_CLR
#define P1588_Y1731_MAC_ADDR1r_SET PLP_P1588_Y1731_MAC_ADDR1r_SET
#define P1588_Y1731_MAC_ADDR1r_GET PLP_P1588_Y1731_MAC_ADDR1r_GET
#define P1588_Y1731_MAC_ADDR1r_MAC_ADDR1_15_0f_GET PLP_P1588_Y1731_MAC_ADDR1r_MAC_ADDR1_15_0f_GET
#define P1588_Y1731_MAC_ADDR1r_MAC_ADDR1_15_0f_SET PLP_P1588_Y1731_MAC_ADDR1r_MAC_ADDR1_15_0f_SET
#define READ_P1588_Y1731_MAC_ADDR1r PLP_READ_P1588_Y1731_MAC_ADDR1r
#define WRITE_P1588_Y1731_MAC_ADDR1r PLP_WRITE_P1588_Y1731_MAC_ADDR1r
#define MODIFY_P1588_Y1731_MAC_ADDR1r PLP_MODIFY_P1588_Y1731_MAC_ADDR1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_MAC_ADDR1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_MAC_ADDR2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x709a
 * DESC:     Y1731 MAC ADDR2 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MAC_ADDR1_31_16  [31:16] of Mac Address1 to match for DA/SA
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_MAC_ADDR2r    0x0000009a

#define PLP_P1588_Y1731_MAC_ADDR2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_MAC_ADDR2.
 *
 */
typedef union PLP_P1588_Y1731_MAC_ADDR2r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_mac_addr2[1];
	uint32_t _p1588_y1731_mac_addr2;
} PLP_P1588_Y1731_MAC_ADDR2r_t;

#define PLP_P1588_Y1731_MAC_ADDR2r_CLR(r) (r).p1588_y1731_mac_addr2[0] = 0
#define PLP_P1588_Y1731_MAC_ADDR2r_SET(r,d) (r).p1588_y1731_mac_addr2[0] = d
#define PLP_P1588_Y1731_MAC_ADDR2r_GET(r) (r).p1588_y1731_mac_addr2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_MAC_ADDR2r_MAC_ADDR1_31_16f_GET(r) (((r).p1588_y1731_mac_addr2[0]) & 0xffff)
#define PLP_P1588_Y1731_MAC_ADDR2r_MAC_ADDR1_31_16f_SET(r,f) (r).p1588_y1731_mac_addr2[0]=(((r).p1588_y1731_mac_addr2[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_MAC_ADDR2.
 *
 */
#define PLP_READ_P1588_Y1731_MAC_ADDR2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_MAC_ADDR2r,(_r._p1588_y1731_mac_addr2))
#define PLP_WRITE_P1588_Y1731_MAC_ADDR2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_MAC_ADDR2r,(_r._p1588_y1731_mac_addr2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_MAC_ADDR2r PLP_P1588_Y1731_MAC_ADDR2r
#define P1588_Y1731_MAC_ADDR2r_SIZE PLP_P1588_Y1731_MAC_ADDR2r_SIZE
typedef PLP_P1588_Y1731_MAC_ADDR2r_t P1588_Y1731_MAC_ADDR2r_t;
#define P1588_Y1731_MAC_ADDR2r_CLR PLP_P1588_Y1731_MAC_ADDR2r_CLR
#define P1588_Y1731_MAC_ADDR2r_SET PLP_P1588_Y1731_MAC_ADDR2r_SET
#define P1588_Y1731_MAC_ADDR2r_GET PLP_P1588_Y1731_MAC_ADDR2r_GET
#define P1588_Y1731_MAC_ADDR2r_MAC_ADDR1_31_16f_GET PLP_P1588_Y1731_MAC_ADDR2r_MAC_ADDR1_31_16f_GET
#define P1588_Y1731_MAC_ADDR2r_MAC_ADDR1_31_16f_SET PLP_P1588_Y1731_MAC_ADDR2r_MAC_ADDR1_31_16f_SET
#define READ_P1588_Y1731_MAC_ADDR2r PLP_READ_P1588_Y1731_MAC_ADDR2r
#define WRITE_P1588_Y1731_MAC_ADDR2r PLP_WRITE_P1588_Y1731_MAC_ADDR2r
#define MODIFY_P1588_Y1731_MAC_ADDR2r PLP_MODIFY_P1588_Y1731_MAC_ADDR2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_MAC_ADDR2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_MAC_ADDR3
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x709b
 * DESC:     Y1731 MAC ADDR3 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MAC_ADDR1_47_32  [47:32] of Mac Address1 to match for DA/SA
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_MAC_ADDR3r    0x0000009b

#define PLP_P1588_Y1731_MAC_ADDR3r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_MAC_ADDR3.
 *
 */
typedef union PLP_P1588_Y1731_MAC_ADDR3r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_mac_addr3[1];
	uint32_t _p1588_y1731_mac_addr3;
} PLP_P1588_Y1731_MAC_ADDR3r_t;

#define PLP_P1588_Y1731_MAC_ADDR3r_CLR(r) (r).p1588_y1731_mac_addr3[0] = 0
#define PLP_P1588_Y1731_MAC_ADDR3r_SET(r,d) (r).p1588_y1731_mac_addr3[0] = d
#define PLP_P1588_Y1731_MAC_ADDR3r_GET(r) (r).p1588_y1731_mac_addr3[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_MAC_ADDR3r_MAC_ADDR1_47_32f_GET(r) (((r).p1588_y1731_mac_addr3[0]) & 0xffff)
#define PLP_P1588_Y1731_MAC_ADDR3r_MAC_ADDR1_47_32f_SET(r,f) (r).p1588_y1731_mac_addr3[0]=(((r).p1588_y1731_mac_addr3[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_MAC_ADDR3.
 *
 */
#define PLP_READ_P1588_Y1731_MAC_ADDR3r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_MAC_ADDR3r,(_r._p1588_y1731_mac_addr3))
#define PLP_WRITE_P1588_Y1731_MAC_ADDR3r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_MAC_ADDR3r,(_r._p1588_y1731_mac_addr3))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_MAC_ADDR3r PLP_P1588_Y1731_MAC_ADDR3r
#define P1588_Y1731_MAC_ADDR3r_SIZE PLP_P1588_Y1731_MAC_ADDR3r_SIZE
typedef PLP_P1588_Y1731_MAC_ADDR3r_t P1588_Y1731_MAC_ADDR3r_t;
#define P1588_Y1731_MAC_ADDR3r_CLR PLP_P1588_Y1731_MAC_ADDR3r_CLR
#define P1588_Y1731_MAC_ADDR3r_SET PLP_P1588_Y1731_MAC_ADDR3r_SET
#define P1588_Y1731_MAC_ADDR3r_GET PLP_P1588_Y1731_MAC_ADDR3r_GET
#define P1588_Y1731_MAC_ADDR3r_MAC_ADDR1_47_32f_GET PLP_P1588_Y1731_MAC_ADDR3r_MAC_ADDR1_47_32f_GET
#define P1588_Y1731_MAC_ADDR3r_MAC_ADDR1_47_32f_SET PLP_P1588_Y1731_MAC_ADDR3r_MAC_ADDR1_47_32f_SET
#define READ_P1588_Y1731_MAC_ADDR3r PLP_READ_P1588_Y1731_MAC_ADDR3r
#define WRITE_P1588_Y1731_MAC_ADDR3r PLP_WRITE_P1588_Y1731_MAC_ADDR3r
#define MODIFY_P1588_Y1731_MAC_ADDR3r PLP_MODIFY_P1588_Y1731_MAC_ADDR3r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_MAC_ADDR3r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_MAC_ADDR4
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x709c
 * DESC:     Y1731 MAC ADDR4 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MAC_ADDR2_15_0   [15:0] of Mac Address2 to match for DA/SA
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_MAC_ADDR4r    0x0000009c

#define PLP_P1588_Y1731_MAC_ADDR4r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_MAC_ADDR4.
 *
 */
typedef union PLP_P1588_Y1731_MAC_ADDR4r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_mac_addr4[1];
	uint32_t _p1588_y1731_mac_addr4;
} PLP_P1588_Y1731_MAC_ADDR4r_t;

#define PLP_P1588_Y1731_MAC_ADDR4r_CLR(r) (r).p1588_y1731_mac_addr4[0] = 0
#define PLP_P1588_Y1731_MAC_ADDR4r_SET(r,d) (r).p1588_y1731_mac_addr4[0] = d
#define PLP_P1588_Y1731_MAC_ADDR4r_GET(r) (r).p1588_y1731_mac_addr4[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_MAC_ADDR4r_MAC_ADDR2_15_0f_GET(r) (((r).p1588_y1731_mac_addr4[0]) & 0xffff)
#define PLP_P1588_Y1731_MAC_ADDR4r_MAC_ADDR2_15_0f_SET(r,f) (r).p1588_y1731_mac_addr4[0]=(((r).p1588_y1731_mac_addr4[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_MAC_ADDR4.
 *
 */
#define PLP_READ_P1588_Y1731_MAC_ADDR4r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_MAC_ADDR4r,(_r._p1588_y1731_mac_addr4))
#define PLP_WRITE_P1588_Y1731_MAC_ADDR4r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_MAC_ADDR4r,(_r._p1588_y1731_mac_addr4))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_MAC_ADDR4r PLP_P1588_Y1731_MAC_ADDR4r
#define P1588_Y1731_MAC_ADDR4r_SIZE PLP_P1588_Y1731_MAC_ADDR4r_SIZE
typedef PLP_P1588_Y1731_MAC_ADDR4r_t P1588_Y1731_MAC_ADDR4r_t;
#define P1588_Y1731_MAC_ADDR4r_CLR PLP_P1588_Y1731_MAC_ADDR4r_CLR
#define P1588_Y1731_MAC_ADDR4r_SET PLP_P1588_Y1731_MAC_ADDR4r_SET
#define P1588_Y1731_MAC_ADDR4r_GET PLP_P1588_Y1731_MAC_ADDR4r_GET
#define P1588_Y1731_MAC_ADDR4r_MAC_ADDR2_15_0f_GET PLP_P1588_Y1731_MAC_ADDR4r_MAC_ADDR2_15_0f_GET
#define P1588_Y1731_MAC_ADDR4r_MAC_ADDR2_15_0f_SET PLP_P1588_Y1731_MAC_ADDR4r_MAC_ADDR2_15_0f_SET
#define READ_P1588_Y1731_MAC_ADDR4r PLP_READ_P1588_Y1731_MAC_ADDR4r
#define WRITE_P1588_Y1731_MAC_ADDR4r PLP_WRITE_P1588_Y1731_MAC_ADDR4r
#define MODIFY_P1588_Y1731_MAC_ADDR4r PLP_MODIFY_P1588_Y1731_MAC_ADDR4r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_MAC_ADDR4r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_MAC_ADDR5
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x709d
 * DESC:     Y1731 MAC ADDR5 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MAC_ADDR2_31_16  [31:16] of Mac Address2 to match for DA/SA
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_MAC_ADDR5r    0x0000009d

#define PLP_P1588_Y1731_MAC_ADDR5r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_MAC_ADDR5.
 *
 */
typedef union PLP_P1588_Y1731_MAC_ADDR5r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_mac_addr5[1];
	uint32_t _p1588_y1731_mac_addr5;
} PLP_P1588_Y1731_MAC_ADDR5r_t;

#define PLP_P1588_Y1731_MAC_ADDR5r_CLR(r) (r).p1588_y1731_mac_addr5[0] = 0
#define PLP_P1588_Y1731_MAC_ADDR5r_SET(r,d) (r).p1588_y1731_mac_addr5[0] = d
#define PLP_P1588_Y1731_MAC_ADDR5r_GET(r) (r).p1588_y1731_mac_addr5[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_MAC_ADDR5r_MAC_ADDR2_31_16f_GET(r) (((r).p1588_y1731_mac_addr5[0]) & 0xffff)
#define PLP_P1588_Y1731_MAC_ADDR5r_MAC_ADDR2_31_16f_SET(r,f) (r).p1588_y1731_mac_addr5[0]=(((r).p1588_y1731_mac_addr5[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_MAC_ADDR5.
 *
 */
#define PLP_READ_P1588_Y1731_MAC_ADDR5r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_MAC_ADDR5r,(_r._p1588_y1731_mac_addr5))
#define PLP_WRITE_P1588_Y1731_MAC_ADDR5r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_MAC_ADDR5r,(_r._p1588_y1731_mac_addr5))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_MAC_ADDR5r PLP_P1588_Y1731_MAC_ADDR5r
#define P1588_Y1731_MAC_ADDR5r_SIZE PLP_P1588_Y1731_MAC_ADDR5r_SIZE
typedef PLP_P1588_Y1731_MAC_ADDR5r_t P1588_Y1731_MAC_ADDR5r_t;
#define P1588_Y1731_MAC_ADDR5r_CLR PLP_P1588_Y1731_MAC_ADDR5r_CLR
#define P1588_Y1731_MAC_ADDR5r_SET PLP_P1588_Y1731_MAC_ADDR5r_SET
#define P1588_Y1731_MAC_ADDR5r_GET PLP_P1588_Y1731_MAC_ADDR5r_GET
#define P1588_Y1731_MAC_ADDR5r_MAC_ADDR2_31_16f_GET PLP_P1588_Y1731_MAC_ADDR5r_MAC_ADDR2_31_16f_GET
#define P1588_Y1731_MAC_ADDR5r_MAC_ADDR2_31_16f_SET PLP_P1588_Y1731_MAC_ADDR5r_MAC_ADDR2_31_16f_SET
#define READ_P1588_Y1731_MAC_ADDR5r PLP_READ_P1588_Y1731_MAC_ADDR5r
#define WRITE_P1588_Y1731_MAC_ADDR5r PLP_WRITE_P1588_Y1731_MAC_ADDR5r
#define MODIFY_P1588_Y1731_MAC_ADDR5r PLP_MODIFY_P1588_Y1731_MAC_ADDR5r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_MAC_ADDR5r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_MAC_ADDR6
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x709e
 * DESC:     Y1731 MAC ADDR6 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MAC_ADDR2_47_32  [47:32] of Mac Address2 to match for DA/SA
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_MAC_ADDR6r    0x0000009e

#define PLP_P1588_Y1731_MAC_ADDR6r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_MAC_ADDR6.
 *
 */
typedef union PLP_P1588_Y1731_MAC_ADDR6r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_mac_addr6[1];
	uint32_t _p1588_y1731_mac_addr6;
} PLP_P1588_Y1731_MAC_ADDR6r_t;

#define PLP_P1588_Y1731_MAC_ADDR6r_CLR(r) (r).p1588_y1731_mac_addr6[0] = 0
#define PLP_P1588_Y1731_MAC_ADDR6r_SET(r,d) (r).p1588_y1731_mac_addr6[0] = d
#define PLP_P1588_Y1731_MAC_ADDR6r_GET(r) (r).p1588_y1731_mac_addr6[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_MAC_ADDR6r_MAC_ADDR2_47_32f_GET(r) (((r).p1588_y1731_mac_addr6[0]) & 0xffff)
#define PLP_P1588_Y1731_MAC_ADDR6r_MAC_ADDR2_47_32f_SET(r,f) (r).p1588_y1731_mac_addr6[0]=(((r).p1588_y1731_mac_addr6[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_MAC_ADDR6.
 *
 */
#define PLP_READ_P1588_Y1731_MAC_ADDR6r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_MAC_ADDR6r,(_r._p1588_y1731_mac_addr6))
#define PLP_WRITE_P1588_Y1731_MAC_ADDR6r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_MAC_ADDR6r,(_r._p1588_y1731_mac_addr6))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_MAC_ADDR6r PLP_P1588_Y1731_MAC_ADDR6r
#define P1588_Y1731_MAC_ADDR6r_SIZE PLP_P1588_Y1731_MAC_ADDR6r_SIZE
typedef PLP_P1588_Y1731_MAC_ADDR6r_t P1588_Y1731_MAC_ADDR6r_t;
#define P1588_Y1731_MAC_ADDR6r_CLR PLP_P1588_Y1731_MAC_ADDR6r_CLR
#define P1588_Y1731_MAC_ADDR6r_SET PLP_P1588_Y1731_MAC_ADDR6r_SET
#define P1588_Y1731_MAC_ADDR6r_GET PLP_P1588_Y1731_MAC_ADDR6r_GET
#define P1588_Y1731_MAC_ADDR6r_MAC_ADDR2_47_32f_GET PLP_P1588_Y1731_MAC_ADDR6r_MAC_ADDR2_47_32f_GET
#define P1588_Y1731_MAC_ADDR6r_MAC_ADDR2_47_32f_SET PLP_P1588_Y1731_MAC_ADDR6r_MAC_ADDR2_47_32f_SET
#define READ_P1588_Y1731_MAC_ADDR6r PLP_READ_P1588_Y1731_MAC_ADDR6r
#define WRITE_P1588_Y1731_MAC_ADDR6r PLP_WRITE_P1588_Y1731_MAC_ADDR6r
#define MODIFY_P1588_Y1731_MAC_ADDR6r PLP_MODIFY_P1588_Y1731_MAC_ADDR6r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_MAC_ADDR6r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_MAC_ADDR7
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x709f
 * DESC:     Y1731 MAC ADDR7 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MAC_ADDR3_15_0   [15:0] of Mac Address3 to match for DA/SA
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_MAC_ADDR7r    0x0000009f

#define PLP_P1588_Y1731_MAC_ADDR7r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_MAC_ADDR7.
 *
 */
typedef union PLP_P1588_Y1731_MAC_ADDR7r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_mac_addr7[1];
	uint32_t _p1588_y1731_mac_addr7;
} PLP_P1588_Y1731_MAC_ADDR7r_t;

#define PLP_P1588_Y1731_MAC_ADDR7r_CLR(r) (r).p1588_y1731_mac_addr7[0] = 0
#define PLP_P1588_Y1731_MAC_ADDR7r_SET(r,d) (r).p1588_y1731_mac_addr7[0] = d
#define PLP_P1588_Y1731_MAC_ADDR7r_GET(r) (r).p1588_y1731_mac_addr7[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_MAC_ADDR7r_MAC_ADDR3_15_0f_GET(r) (((r).p1588_y1731_mac_addr7[0]) & 0xffff)
#define PLP_P1588_Y1731_MAC_ADDR7r_MAC_ADDR3_15_0f_SET(r,f) (r).p1588_y1731_mac_addr7[0]=(((r).p1588_y1731_mac_addr7[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_MAC_ADDR7.
 *
 */
#define PLP_READ_P1588_Y1731_MAC_ADDR7r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_MAC_ADDR7r,(_r._p1588_y1731_mac_addr7))
#define PLP_WRITE_P1588_Y1731_MAC_ADDR7r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_MAC_ADDR7r,(_r._p1588_y1731_mac_addr7))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_MAC_ADDR7r PLP_P1588_Y1731_MAC_ADDR7r
#define P1588_Y1731_MAC_ADDR7r_SIZE PLP_P1588_Y1731_MAC_ADDR7r_SIZE
typedef PLP_P1588_Y1731_MAC_ADDR7r_t P1588_Y1731_MAC_ADDR7r_t;
#define P1588_Y1731_MAC_ADDR7r_CLR PLP_P1588_Y1731_MAC_ADDR7r_CLR
#define P1588_Y1731_MAC_ADDR7r_SET PLP_P1588_Y1731_MAC_ADDR7r_SET
#define P1588_Y1731_MAC_ADDR7r_GET PLP_P1588_Y1731_MAC_ADDR7r_GET
#define P1588_Y1731_MAC_ADDR7r_MAC_ADDR3_15_0f_GET PLP_P1588_Y1731_MAC_ADDR7r_MAC_ADDR3_15_0f_GET
#define P1588_Y1731_MAC_ADDR7r_MAC_ADDR3_15_0f_SET PLP_P1588_Y1731_MAC_ADDR7r_MAC_ADDR3_15_0f_SET
#define READ_P1588_Y1731_MAC_ADDR7r PLP_READ_P1588_Y1731_MAC_ADDR7r
#define WRITE_P1588_Y1731_MAC_ADDR7r PLP_WRITE_P1588_Y1731_MAC_ADDR7r
#define MODIFY_P1588_Y1731_MAC_ADDR7r PLP_MODIFY_P1588_Y1731_MAC_ADDR7r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_MAC_ADDR7r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_MAC_ADDR8
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70a0
 * DESC:     Y1731 MAC ADDR8 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MAC_ADDR3_31_16  [31:16] of Mac Address3 to match for DA/SA
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_MAC_ADDR8r    0x000000a0

#define PLP_P1588_Y1731_MAC_ADDR8r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_MAC_ADDR8.
 *
 */
typedef union PLP_P1588_Y1731_MAC_ADDR8r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_mac_addr8[1];
	uint32_t _p1588_y1731_mac_addr8;
} PLP_P1588_Y1731_MAC_ADDR8r_t;

#define PLP_P1588_Y1731_MAC_ADDR8r_CLR(r) (r).p1588_y1731_mac_addr8[0] = 0
#define PLP_P1588_Y1731_MAC_ADDR8r_SET(r,d) (r).p1588_y1731_mac_addr8[0] = d
#define PLP_P1588_Y1731_MAC_ADDR8r_GET(r) (r).p1588_y1731_mac_addr8[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_MAC_ADDR8r_MAC_ADDR3_31_16f_GET(r) (((r).p1588_y1731_mac_addr8[0]) & 0xffff)
#define PLP_P1588_Y1731_MAC_ADDR8r_MAC_ADDR3_31_16f_SET(r,f) (r).p1588_y1731_mac_addr8[0]=(((r).p1588_y1731_mac_addr8[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_MAC_ADDR8.
 *
 */
#define PLP_READ_P1588_Y1731_MAC_ADDR8r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_MAC_ADDR8r,(_r._p1588_y1731_mac_addr8))
#define PLP_WRITE_P1588_Y1731_MAC_ADDR8r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_MAC_ADDR8r,(_r._p1588_y1731_mac_addr8))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_MAC_ADDR8r PLP_P1588_Y1731_MAC_ADDR8r
#define P1588_Y1731_MAC_ADDR8r_SIZE PLP_P1588_Y1731_MAC_ADDR8r_SIZE
typedef PLP_P1588_Y1731_MAC_ADDR8r_t P1588_Y1731_MAC_ADDR8r_t;
#define P1588_Y1731_MAC_ADDR8r_CLR PLP_P1588_Y1731_MAC_ADDR8r_CLR
#define P1588_Y1731_MAC_ADDR8r_SET PLP_P1588_Y1731_MAC_ADDR8r_SET
#define P1588_Y1731_MAC_ADDR8r_GET PLP_P1588_Y1731_MAC_ADDR8r_GET
#define P1588_Y1731_MAC_ADDR8r_MAC_ADDR3_31_16f_GET PLP_P1588_Y1731_MAC_ADDR8r_MAC_ADDR3_31_16f_GET
#define P1588_Y1731_MAC_ADDR8r_MAC_ADDR3_31_16f_SET PLP_P1588_Y1731_MAC_ADDR8r_MAC_ADDR3_31_16f_SET
#define READ_P1588_Y1731_MAC_ADDR8r PLP_READ_P1588_Y1731_MAC_ADDR8r
#define WRITE_P1588_Y1731_MAC_ADDR8r PLP_WRITE_P1588_Y1731_MAC_ADDR8r
#define MODIFY_P1588_Y1731_MAC_ADDR8r PLP_MODIFY_P1588_Y1731_MAC_ADDR8r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_MAC_ADDR8r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_MAC_ADDR9
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70a1
 * DESC:     Y1731 MAC ADDR9 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MAC_ADDR3_47_32  [47:32] of Mac Address3 to match for DA/SA
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_MAC_ADDR9r    0x000000a1

#define PLP_P1588_Y1731_MAC_ADDR9r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_MAC_ADDR9.
 *
 */
typedef union PLP_P1588_Y1731_MAC_ADDR9r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_mac_addr9[1];
	uint32_t _p1588_y1731_mac_addr9;
} PLP_P1588_Y1731_MAC_ADDR9r_t;

#define PLP_P1588_Y1731_MAC_ADDR9r_CLR(r) (r).p1588_y1731_mac_addr9[0] = 0
#define PLP_P1588_Y1731_MAC_ADDR9r_SET(r,d) (r).p1588_y1731_mac_addr9[0] = d
#define PLP_P1588_Y1731_MAC_ADDR9r_GET(r) (r).p1588_y1731_mac_addr9[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_MAC_ADDR9r_MAC_ADDR3_47_32f_GET(r) (((r).p1588_y1731_mac_addr9[0]) & 0xffff)
#define PLP_P1588_Y1731_MAC_ADDR9r_MAC_ADDR3_47_32f_SET(r,f) (r).p1588_y1731_mac_addr9[0]=(((r).p1588_y1731_mac_addr9[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_MAC_ADDR9.
 *
 */
#define PLP_READ_P1588_Y1731_MAC_ADDR9r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_MAC_ADDR9r,(_r._p1588_y1731_mac_addr9))
#define PLP_WRITE_P1588_Y1731_MAC_ADDR9r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_MAC_ADDR9r,(_r._p1588_y1731_mac_addr9))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_MAC_ADDR9r PLP_P1588_Y1731_MAC_ADDR9r
#define P1588_Y1731_MAC_ADDR9r_SIZE PLP_P1588_Y1731_MAC_ADDR9r_SIZE
typedef PLP_P1588_Y1731_MAC_ADDR9r_t P1588_Y1731_MAC_ADDR9r_t;
#define P1588_Y1731_MAC_ADDR9r_CLR PLP_P1588_Y1731_MAC_ADDR9r_CLR
#define P1588_Y1731_MAC_ADDR9r_SET PLP_P1588_Y1731_MAC_ADDR9r_SET
#define P1588_Y1731_MAC_ADDR9r_GET PLP_P1588_Y1731_MAC_ADDR9r_GET
#define P1588_Y1731_MAC_ADDR9r_MAC_ADDR3_47_32f_GET PLP_P1588_Y1731_MAC_ADDR9r_MAC_ADDR3_47_32f_GET
#define P1588_Y1731_MAC_ADDR9r_MAC_ADDR3_47_32f_SET PLP_P1588_Y1731_MAC_ADDR9r_MAC_ADDR3_47_32f_SET
#define READ_P1588_Y1731_MAC_ADDR9r PLP_READ_P1588_Y1731_MAC_ADDR9r
#define WRITE_P1588_Y1731_MAC_ADDR9r PLP_WRITE_P1588_Y1731_MAC_ADDR9r
#define MODIFY_P1588_Y1731_MAC_ADDR9r PLP_MODIFY_P1588_Y1731_MAC_ADDR9r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_MAC_ADDR9r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_MAC_ADDR10
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70a2
 * DESC:     Y1731 MAC ADDR10 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MAC_ADDR4_15_0   [15:0] of Mac Address4 to match for DA/SA
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_MAC_ADDR10r    0x000000a2

#define PLP_P1588_Y1731_MAC_ADDR10r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_MAC_ADDR10.
 *
 */
typedef union PLP_P1588_Y1731_MAC_ADDR10r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_mac_addr10[1];
	uint32_t _p1588_y1731_mac_addr10;
} PLP_P1588_Y1731_MAC_ADDR10r_t;

#define PLP_P1588_Y1731_MAC_ADDR10r_CLR(r) (r).p1588_y1731_mac_addr10[0] = 0
#define PLP_P1588_Y1731_MAC_ADDR10r_SET(r,d) (r).p1588_y1731_mac_addr10[0] = d
#define PLP_P1588_Y1731_MAC_ADDR10r_GET(r) (r).p1588_y1731_mac_addr10[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_MAC_ADDR10r_MAC_ADDR4_15_0f_GET(r) (((r).p1588_y1731_mac_addr10[0]) & 0xffff)
#define PLP_P1588_Y1731_MAC_ADDR10r_MAC_ADDR4_15_0f_SET(r,f) (r).p1588_y1731_mac_addr10[0]=(((r).p1588_y1731_mac_addr10[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_MAC_ADDR10.
 *
 */
#define PLP_READ_P1588_Y1731_MAC_ADDR10r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_MAC_ADDR10r,(_r._p1588_y1731_mac_addr10))
#define PLP_WRITE_P1588_Y1731_MAC_ADDR10r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_MAC_ADDR10r,(_r._p1588_y1731_mac_addr10))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_MAC_ADDR10r PLP_P1588_Y1731_MAC_ADDR10r
#define P1588_Y1731_MAC_ADDR10r_SIZE PLP_P1588_Y1731_MAC_ADDR10r_SIZE
typedef PLP_P1588_Y1731_MAC_ADDR10r_t P1588_Y1731_MAC_ADDR10r_t;
#define P1588_Y1731_MAC_ADDR10r_CLR PLP_P1588_Y1731_MAC_ADDR10r_CLR
#define P1588_Y1731_MAC_ADDR10r_SET PLP_P1588_Y1731_MAC_ADDR10r_SET
#define P1588_Y1731_MAC_ADDR10r_GET PLP_P1588_Y1731_MAC_ADDR10r_GET
#define P1588_Y1731_MAC_ADDR10r_MAC_ADDR4_15_0f_GET PLP_P1588_Y1731_MAC_ADDR10r_MAC_ADDR4_15_0f_GET
#define P1588_Y1731_MAC_ADDR10r_MAC_ADDR4_15_0f_SET PLP_P1588_Y1731_MAC_ADDR10r_MAC_ADDR4_15_0f_SET
#define READ_P1588_Y1731_MAC_ADDR10r PLP_READ_P1588_Y1731_MAC_ADDR10r
#define WRITE_P1588_Y1731_MAC_ADDR10r PLP_WRITE_P1588_Y1731_MAC_ADDR10r
#define MODIFY_P1588_Y1731_MAC_ADDR10r PLP_MODIFY_P1588_Y1731_MAC_ADDR10r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_MAC_ADDR10r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_MAC_ADDR11
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70a3
 * DESC:     Y1731 MAC ADDR11 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MAC_ADDR4_31_16  [31:16] of Mac Address4 to match for DA/SA
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_MAC_ADDR11r    0x000000a3

#define PLP_P1588_Y1731_MAC_ADDR11r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_MAC_ADDR11.
 *
 */
typedef union PLP_P1588_Y1731_MAC_ADDR11r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_mac_addr11[1];
	uint32_t _p1588_y1731_mac_addr11;
} PLP_P1588_Y1731_MAC_ADDR11r_t;

#define PLP_P1588_Y1731_MAC_ADDR11r_CLR(r) (r).p1588_y1731_mac_addr11[0] = 0
#define PLP_P1588_Y1731_MAC_ADDR11r_SET(r,d) (r).p1588_y1731_mac_addr11[0] = d
#define PLP_P1588_Y1731_MAC_ADDR11r_GET(r) (r).p1588_y1731_mac_addr11[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_MAC_ADDR11r_MAC_ADDR4_31_16f_GET(r) (((r).p1588_y1731_mac_addr11[0]) & 0xffff)
#define PLP_P1588_Y1731_MAC_ADDR11r_MAC_ADDR4_31_16f_SET(r,f) (r).p1588_y1731_mac_addr11[0]=(((r).p1588_y1731_mac_addr11[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_MAC_ADDR11.
 *
 */
#define PLP_READ_P1588_Y1731_MAC_ADDR11r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_MAC_ADDR11r,(_r._p1588_y1731_mac_addr11))
#define PLP_WRITE_P1588_Y1731_MAC_ADDR11r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_MAC_ADDR11r,(_r._p1588_y1731_mac_addr11))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_MAC_ADDR11r PLP_P1588_Y1731_MAC_ADDR11r
#define P1588_Y1731_MAC_ADDR11r_SIZE PLP_P1588_Y1731_MAC_ADDR11r_SIZE
typedef PLP_P1588_Y1731_MAC_ADDR11r_t P1588_Y1731_MAC_ADDR11r_t;
#define P1588_Y1731_MAC_ADDR11r_CLR PLP_P1588_Y1731_MAC_ADDR11r_CLR
#define P1588_Y1731_MAC_ADDR11r_SET PLP_P1588_Y1731_MAC_ADDR11r_SET
#define P1588_Y1731_MAC_ADDR11r_GET PLP_P1588_Y1731_MAC_ADDR11r_GET
#define P1588_Y1731_MAC_ADDR11r_MAC_ADDR4_31_16f_GET PLP_P1588_Y1731_MAC_ADDR11r_MAC_ADDR4_31_16f_GET
#define P1588_Y1731_MAC_ADDR11r_MAC_ADDR4_31_16f_SET PLP_P1588_Y1731_MAC_ADDR11r_MAC_ADDR4_31_16f_SET
#define READ_P1588_Y1731_MAC_ADDR11r PLP_READ_P1588_Y1731_MAC_ADDR11r
#define WRITE_P1588_Y1731_MAC_ADDR11r PLP_WRITE_P1588_Y1731_MAC_ADDR11r
#define MODIFY_P1588_Y1731_MAC_ADDR11r PLP_MODIFY_P1588_Y1731_MAC_ADDR11r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_MAC_ADDR11r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_MAC_ADDR12
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70a4
 * DESC:     Y1731 MAC ADDR12 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MAC_ADDR4_47_32  [47:32] of Mac Address4 to match for DA/SA
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_MAC_ADDR12r    0x000000a4

#define PLP_P1588_Y1731_MAC_ADDR12r_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_MAC_ADDR12.
 *
 */
typedef union PLP_P1588_Y1731_MAC_ADDR12r_s {
	uint32_t v[1];
	uint32_t p1588_y1731_mac_addr12[1];
	uint32_t _p1588_y1731_mac_addr12;
} PLP_P1588_Y1731_MAC_ADDR12r_t;

#define PLP_P1588_Y1731_MAC_ADDR12r_CLR(r) (r).p1588_y1731_mac_addr12[0] = 0
#define PLP_P1588_Y1731_MAC_ADDR12r_SET(r,d) (r).p1588_y1731_mac_addr12[0] = d
#define PLP_P1588_Y1731_MAC_ADDR12r_GET(r) (r).p1588_y1731_mac_addr12[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_MAC_ADDR12r_MAC_ADDR4_47_32f_GET(r) (((r).p1588_y1731_mac_addr12[0]) & 0xffff)
#define PLP_P1588_Y1731_MAC_ADDR12r_MAC_ADDR4_47_32f_SET(r,f) (r).p1588_y1731_mac_addr12[0]=(((r).p1588_y1731_mac_addr12[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_Y1731_MAC_ADDR12.
 *
 */
#define PLP_READ_P1588_Y1731_MAC_ADDR12r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_MAC_ADDR12r,(_r._p1588_y1731_mac_addr12))
#define PLP_WRITE_P1588_Y1731_MAC_ADDR12r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_MAC_ADDR12r,(_r._p1588_y1731_mac_addr12))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_MAC_ADDR12r PLP_P1588_Y1731_MAC_ADDR12r
#define P1588_Y1731_MAC_ADDR12r_SIZE PLP_P1588_Y1731_MAC_ADDR12r_SIZE
typedef PLP_P1588_Y1731_MAC_ADDR12r_t P1588_Y1731_MAC_ADDR12r_t;
#define P1588_Y1731_MAC_ADDR12r_CLR PLP_P1588_Y1731_MAC_ADDR12r_CLR
#define P1588_Y1731_MAC_ADDR12r_SET PLP_P1588_Y1731_MAC_ADDR12r_SET
#define P1588_Y1731_MAC_ADDR12r_GET PLP_P1588_Y1731_MAC_ADDR12r_GET
#define P1588_Y1731_MAC_ADDR12r_MAC_ADDR4_47_32f_GET PLP_P1588_Y1731_MAC_ADDR12r_MAC_ADDR4_47_32f_GET
#define P1588_Y1731_MAC_ADDR12r_MAC_ADDR4_47_32f_SET PLP_P1588_Y1731_MAC_ADDR12r_MAC_ADDR4_47_32f_SET
#define READ_P1588_Y1731_MAC_ADDR12r PLP_READ_P1588_Y1731_MAC_ADDR12r
#define WRITE_P1588_Y1731_MAC_ADDR12r PLP_WRITE_P1588_Y1731_MAC_ADDR12r
#define MODIFY_P1588_Y1731_MAC_ADDR12r PLP_MODIFY_P1588_Y1731_MAC_ADDR12r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_MAC_ADDR12r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_Y1731_TS_FORMAT
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70a5
 * DESC:     Y1731 TS FORMAT Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_Y1731_TS_TYPE 0 - Use PTP Timestamp for Y.1731 DM1 - Use NTP Timestamp for Y.1731 DM
 *     RX_BHH_TS_TYPE   0 - Use PTP Timestamp for BHH DM1 - Use NTP Timestamp for BHH DM
 *     RX_USE_IETF_QTF_CFG 0 - Use QTF field to determine timestamp format1 - Use rx_ietf_qtf_ts_type to determine timestamp formatOver-ride for Query packets
 *     RX_IETF_QTF_TS_TYPE 0 - Use PTP Timestamp for Y.1731/BHH Query packets1 - Use NTP Timestamp for Y.1731/BHH Query packets
 *     RX_USE_IETF_RTF_CFG 0 - Use RPTF field to determine timestamp format1 - Use rx_ietf_rtf_ts_type to determine timestamp formatOver-ride for Response packets
 *     RX_IETF_RTF_TS_TYPE 0 - Use PTP Timestamp for Y.1731/BHH Response packet1 - Use NTP Timestamp for Y.1731/BHH Response packet
 *     LOCAL_MAC_DA_CHECK_EN 0 - Ignore MAC DA check for Rx direction1 - Enable MAC DA check for Rx direction
 *     TX_Y1731_TS_TYPE Reserved for future
 *     TX_BHH_TS_TYPE   reserved for future
 *     TX_USE_IETF_QTF_CFG 0 - Use QTF field to determine timestamp format1 - Use tx_ietf_qtf_ts_type to determine timestamp formatOver-ride for Query packets
 *     TX_IETF_QTF_TS_TYPE 0 - Use PTP Timestamp for Y.1731/BHH Query packets1 - Use NTP Timestamp for Y.1731/BHH Query packets
 *     TX_USE_IETF_RTF_CFG 0 - Use RPTF field to determine timestamp format1 - Use tx_ietf_rtf_ts_type to determine timestamp formatOver-ride for Response packets
 *     TX_IETF_RTF_TS_TYPE reserved for future use
 *     LOCAL_MAC_SA_CHECK_EN 0 - Ignore MAC SA check for Tx direction1 - Enable MAC SA check for Tx direction
 *
 ******************************************************************************/
#define PLP_P1588_Y1731_TS_FORMATr    0x000000a5

#define PLP_P1588_Y1731_TS_FORMATr_SIZE 4

/*
 * This structure should be used to declare and program P1588_Y1731_TS_FORMAT.
 *
 */
typedef union PLP_P1588_Y1731_TS_FORMATr_s {
	uint32_t v[1];
	uint32_t p1588_y1731_ts_format[1];
	uint32_t _p1588_y1731_ts_format;
} PLP_P1588_Y1731_TS_FORMATr_t;

#define PLP_P1588_Y1731_TS_FORMATr_CLR(r) (r).p1588_y1731_ts_format[0] = 0
#define PLP_P1588_Y1731_TS_FORMATr_SET(r,d) (r).p1588_y1731_ts_format[0] = d
#define PLP_P1588_Y1731_TS_FORMATr_GET(r) (r).p1588_y1731_ts_format[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_Y1731_TS_FORMATr_LOCAL_MAC_SA_CHECK_ENf_GET(r) ((((r).p1588_y1731_ts_format[0]) >> 15) & 0x1)
#define PLP_P1588_Y1731_TS_FORMATr_LOCAL_MAC_SA_CHECK_ENf_SET(r,f) (r).p1588_y1731_ts_format[0]=(((r).p1588_y1731_ts_format[0] & ~((uint32_t)0x1 << 15)) | ((((uint32_t)f) & 0x1) << 15)) | (1 << (16 + 15))
#define PLP_P1588_Y1731_TS_FORMATr_TX_IETF_RTF_TS_TYPEf_GET(r) ((((r).p1588_y1731_ts_format[0]) >> 13) & 0x1)
#define PLP_P1588_Y1731_TS_FORMATr_TX_IETF_RTF_TS_TYPEf_SET(r,f) (r).p1588_y1731_ts_format[0]=(((r).p1588_y1731_ts_format[0] & ~((uint32_t)0x1 << 13)) | ((((uint32_t)f) & 0x1) << 13)) | (1 << (16 + 13))
#define PLP_P1588_Y1731_TS_FORMATr_TX_USE_IETF_RTF_CFGf_GET(r) ((((r).p1588_y1731_ts_format[0]) >> 12) & 0x1)
#define PLP_P1588_Y1731_TS_FORMATr_TX_USE_IETF_RTF_CFGf_SET(r,f) (r).p1588_y1731_ts_format[0]=(((r).p1588_y1731_ts_format[0] & ~((uint32_t)0x1 << 12)) | ((((uint32_t)f) & 0x1) << 12)) | (1 << (16 + 12))
#define PLP_P1588_Y1731_TS_FORMATr_TX_IETF_QTF_TS_TYPEf_GET(r) ((((r).p1588_y1731_ts_format[0]) >> 11) & 0x1)
#define PLP_P1588_Y1731_TS_FORMATr_TX_IETF_QTF_TS_TYPEf_SET(r,f) (r).p1588_y1731_ts_format[0]=(((r).p1588_y1731_ts_format[0] & ~((uint32_t)0x1 << 11)) | ((((uint32_t)f) & 0x1) << 11)) | (1 << (16 + 11))
#define PLP_P1588_Y1731_TS_FORMATr_TX_USE_IETF_QTF_CFGf_GET(r) ((((r).p1588_y1731_ts_format[0]) >> 10) & 0x1)
#define PLP_P1588_Y1731_TS_FORMATr_TX_USE_IETF_QTF_CFGf_SET(r,f) (r).p1588_y1731_ts_format[0]=(((r).p1588_y1731_ts_format[0] & ~((uint32_t)0x1 << 10)) | ((((uint32_t)f) & 0x1) << 10)) | (1 << (16 + 10))
#define PLP_P1588_Y1731_TS_FORMATr_TX_BHH_TS_TYPEf_GET(r) ((((r).p1588_y1731_ts_format[0]) >> 9) & 0x1)
#define PLP_P1588_Y1731_TS_FORMATr_TX_BHH_TS_TYPEf_SET(r,f) (r).p1588_y1731_ts_format[0]=(((r).p1588_y1731_ts_format[0] & ~((uint32_t)0x1 << 9)) | ((((uint32_t)f) & 0x1) << 9)) | (1 << (16 + 9))
#define PLP_P1588_Y1731_TS_FORMATr_TX_Y1731_TS_TYPEf_GET(r) ((((r).p1588_y1731_ts_format[0]) >> 8) & 0x1)
#define PLP_P1588_Y1731_TS_FORMATr_TX_Y1731_TS_TYPEf_SET(r,f) (r).p1588_y1731_ts_format[0]=(((r).p1588_y1731_ts_format[0] & ~((uint32_t)0x1 << 8)) | ((((uint32_t)f) & 0x1) << 8)) | (1 << (16 + 8))
#define PLP_P1588_Y1731_TS_FORMATr_LOCAL_MAC_DA_CHECK_ENf_GET(r) ((((r).p1588_y1731_ts_format[0]) >> 7) & 0x1)
#define PLP_P1588_Y1731_TS_FORMATr_LOCAL_MAC_DA_CHECK_ENf_SET(r,f) (r).p1588_y1731_ts_format[0]=(((r).p1588_y1731_ts_format[0] & ~((uint32_t)0x1 << 7)) | ((((uint32_t)f) & 0x1) << 7)) | (1 << (16 + 7))
#define PLP_P1588_Y1731_TS_FORMATr_RX_IETF_RTF_TS_TYPEf_GET(r) ((((r).p1588_y1731_ts_format[0]) >> 5) & 0x1)
#define PLP_P1588_Y1731_TS_FORMATr_RX_IETF_RTF_TS_TYPEf_SET(r,f) (r).p1588_y1731_ts_format[0]=(((r).p1588_y1731_ts_format[0] & ~((uint32_t)0x1 << 5)) | ((((uint32_t)f) & 0x1) << 5)) | (1 << (16 + 5))
#define PLP_P1588_Y1731_TS_FORMATr_RX_USE_IETF_RTF_CFGf_GET(r) ((((r).p1588_y1731_ts_format[0]) >> 4) & 0x1)
#define PLP_P1588_Y1731_TS_FORMATr_RX_USE_IETF_RTF_CFGf_SET(r,f) (r).p1588_y1731_ts_format[0]=(((r).p1588_y1731_ts_format[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4)) | (1 << (16 + 4))
#define PLP_P1588_Y1731_TS_FORMATr_RX_IETF_QTF_TS_TYPEf_GET(r) ((((r).p1588_y1731_ts_format[0]) >> 3) & 0x1)
#define PLP_P1588_Y1731_TS_FORMATr_RX_IETF_QTF_TS_TYPEf_SET(r,f) (r).p1588_y1731_ts_format[0]=(((r).p1588_y1731_ts_format[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3)) | (1 << (16 + 3))
#define PLP_P1588_Y1731_TS_FORMATr_RX_USE_IETF_QTF_CFGf_GET(r) ((((r).p1588_y1731_ts_format[0]) >> 2) & 0x1)
#define PLP_P1588_Y1731_TS_FORMATr_RX_USE_IETF_QTF_CFGf_SET(r,f) (r).p1588_y1731_ts_format[0]=(((r).p1588_y1731_ts_format[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2)) | (1 << (16 + 2))
#define PLP_P1588_Y1731_TS_FORMATr_RX_BHH_TS_TYPEf_GET(r) ((((r).p1588_y1731_ts_format[0]) >> 1) & 0x1)
#define PLP_P1588_Y1731_TS_FORMATr_RX_BHH_TS_TYPEf_SET(r,f) (r).p1588_y1731_ts_format[0]=(((r).p1588_y1731_ts_format[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))
#define PLP_P1588_Y1731_TS_FORMATr_RX_Y1731_TS_TYPEf_GET(r) (((r).p1588_y1731_ts_format[0]) & 0x1)
#define PLP_P1588_Y1731_TS_FORMATr_RX_Y1731_TS_TYPEf_SET(r,f) (r).p1588_y1731_ts_format[0]=(((r).p1588_y1731_ts_format[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1)) | (0x1 << 16)

/*
 * These macros can be used to access P1588_Y1731_TS_FORMAT.
 *
 */
#define PLP_READ_P1588_Y1731_TS_FORMATr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_Y1731_TS_FORMATr,(_r._p1588_y1731_ts_format))
#define PLP_WRITE_P1588_Y1731_TS_FORMATr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_Y1731_TS_FORMATr,(_r._p1588_y1731_ts_format))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_Y1731_TS_FORMATr PLP_P1588_Y1731_TS_FORMATr
#define P1588_Y1731_TS_FORMATr_SIZE PLP_P1588_Y1731_TS_FORMATr_SIZE
typedef PLP_P1588_Y1731_TS_FORMATr_t P1588_Y1731_TS_FORMATr_t;
#define P1588_Y1731_TS_FORMATr_CLR PLP_P1588_Y1731_TS_FORMATr_CLR
#define P1588_Y1731_TS_FORMATr_SET PLP_P1588_Y1731_TS_FORMATr_SET
#define P1588_Y1731_TS_FORMATr_GET PLP_P1588_Y1731_TS_FORMATr_GET
#define P1588_Y1731_TS_FORMATr_LOCAL_MAC_SA_CHECK_ENf_GET PLP_P1588_Y1731_TS_FORMATr_LOCAL_MAC_SA_CHECK_ENf_GET
#define P1588_Y1731_TS_FORMATr_LOCAL_MAC_SA_CHECK_ENf_SET PLP_P1588_Y1731_TS_FORMATr_LOCAL_MAC_SA_CHECK_ENf_SET
#define P1588_Y1731_TS_FORMATr_TX_IETF_RTF_TS_TYPEf_GET PLP_P1588_Y1731_TS_FORMATr_TX_IETF_RTF_TS_TYPEf_GET
#define P1588_Y1731_TS_FORMATr_TX_IETF_RTF_TS_TYPEf_SET PLP_P1588_Y1731_TS_FORMATr_TX_IETF_RTF_TS_TYPEf_SET
#define P1588_Y1731_TS_FORMATr_TX_USE_IETF_RTF_CFGf_GET PLP_P1588_Y1731_TS_FORMATr_TX_USE_IETF_RTF_CFGf_GET
#define P1588_Y1731_TS_FORMATr_TX_USE_IETF_RTF_CFGf_SET PLP_P1588_Y1731_TS_FORMATr_TX_USE_IETF_RTF_CFGf_SET
#define P1588_Y1731_TS_FORMATr_TX_IETF_QTF_TS_TYPEf_GET PLP_P1588_Y1731_TS_FORMATr_TX_IETF_QTF_TS_TYPEf_GET
#define P1588_Y1731_TS_FORMATr_TX_IETF_QTF_TS_TYPEf_SET PLP_P1588_Y1731_TS_FORMATr_TX_IETF_QTF_TS_TYPEf_SET
#define P1588_Y1731_TS_FORMATr_TX_USE_IETF_QTF_CFGf_GET PLP_P1588_Y1731_TS_FORMATr_TX_USE_IETF_QTF_CFGf_GET
#define P1588_Y1731_TS_FORMATr_TX_USE_IETF_QTF_CFGf_SET PLP_P1588_Y1731_TS_FORMATr_TX_USE_IETF_QTF_CFGf_SET
#define P1588_Y1731_TS_FORMATr_TX_BHH_TS_TYPEf_GET PLP_P1588_Y1731_TS_FORMATr_TX_BHH_TS_TYPEf_GET
#define P1588_Y1731_TS_FORMATr_TX_BHH_TS_TYPEf_SET PLP_P1588_Y1731_TS_FORMATr_TX_BHH_TS_TYPEf_SET
#define P1588_Y1731_TS_FORMATr_TX_Y1731_TS_TYPEf_GET PLP_P1588_Y1731_TS_FORMATr_TX_Y1731_TS_TYPEf_GET
#define P1588_Y1731_TS_FORMATr_TX_Y1731_TS_TYPEf_SET PLP_P1588_Y1731_TS_FORMATr_TX_Y1731_TS_TYPEf_SET
#define P1588_Y1731_TS_FORMATr_LOCAL_MAC_DA_CHECK_ENf_GET PLP_P1588_Y1731_TS_FORMATr_LOCAL_MAC_DA_CHECK_ENf_GET
#define P1588_Y1731_TS_FORMATr_LOCAL_MAC_DA_CHECK_ENf_SET PLP_P1588_Y1731_TS_FORMATr_LOCAL_MAC_DA_CHECK_ENf_SET
#define P1588_Y1731_TS_FORMATr_RX_IETF_RTF_TS_TYPEf_GET PLP_P1588_Y1731_TS_FORMATr_RX_IETF_RTF_TS_TYPEf_GET
#define P1588_Y1731_TS_FORMATr_RX_IETF_RTF_TS_TYPEf_SET PLP_P1588_Y1731_TS_FORMATr_RX_IETF_RTF_TS_TYPEf_SET
#define P1588_Y1731_TS_FORMATr_RX_USE_IETF_RTF_CFGf_GET PLP_P1588_Y1731_TS_FORMATr_RX_USE_IETF_RTF_CFGf_GET
#define P1588_Y1731_TS_FORMATr_RX_USE_IETF_RTF_CFGf_SET PLP_P1588_Y1731_TS_FORMATr_RX_USE_IETF_RTF_CFGf_SET
#define P1588_Y1731_TS_FORMATr_RX_IETF_QTF_TS_TYPEf_GET PLP_P1588_Y1731_TS_FORMATr_RX_IETF_QTF_TS_TYPEf_GET
#define P1588_Y1731_TS_FORMATr_RX_IETF_QTF_TS_TYPEf_SET PLP_P1588_Y1731_TS_FORMATr_RX_IETF_QTF_TS_TYPEf_SET
#define P1588_Y1731_TS_FORMATr_RX_USE_IETF_QTF_CFGf_GET PLP_P1588_Y1731_TS_FORMATr_RX_USE_IETF_QTF_CFGf_GET
#define P1588_Y1731_TS_FORMATr_RX_USE_IETF_QTF_CFGf_SET PLP_P1588_Y1731_TS_FORMATr_RX_USE_IETF_QTF_CFGf_SET
#define P1588_Y1731_TS_FORMATr_RX_BHH_TS_TYPEf_GET PLP_P1588_Y1731_TS_FORMATr_RX_BHH_TS_TYPEf_GET
#define P1588_Y1731_TS_FORMATr_RX_BHH_TS_TYPEf_SET PLP_P1588_Y1731_TS_FORMATr_RX_BHH_TS_TYPEf_SET
#define P1588_Y1731_TS_FORMATr_RX_Y1731_TS_TYPEf_GET PLP_P1588_Y1731_TS_FORMATr_RX_Y1731_TS_TYPEf_GET
#define P1588_Y1731_TS_FORMATr_RX_Y1731_TS_TYPEf_SET PLP_P1588_Y1731_TS_FORMATr_RX_Y1731_TS_TYPEf_SET
#define READ_P1588_Y1731_TS_FORMATr PLP_READ_P1588_Y1731_TS_FORMATr
#define WRITE_P1588_Y1731_TS_FORMATr PLP_WRITE_P1588_Y1731_TS_FORMATr
#define MODIFY_P1588_Y1731_TS_FORMATr PLP_MODIFY_P1588_Y1731_TS_FORMATr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_Y1731_TS_FORMATr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_INTERRUPT_CONTROL
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70a7
 * DESC:     INTERRUPT CONTROL Register
 * RESETVAL: 0xfffe (65534)
 * ACCESS:   R/W
 * FIELDS:
 *     INTERRUPT_CTRL   LED to GPIO mapping[15] 0 = drive RX SOP LED to OPTXFLT PAD[14] 0 = drive RX SOP LED to GPIO_1 PAD[13] 0 = drive RX SOP LED to GPIO_0 PAD[11] 0 = drive TX SOP LED to OPTXFLT PAD[10] 0 = drive TX SOP LED to GPIO_1 PAD[9]  0 = drive TX SOP LED to GPIO_0 PAD1588 Interrupt control registerUsed to drive 1588 interrupt onto any of theavailable pins[7] - reserved for future use will read 1[6] - 1: Disable 1588 Interrupt to GPIO1 pad0: Enable  1588 Interrupt to GPIO1 pad[5] - 1: Disable 1588 Interrupt to GPIO0 pad0: Enable  1588 Interrupt to GPIO0 pad[4] - reserved for future use will read 1[3] - reserved for future use will read 1[2] - reserved for future use will read 1[1] - reserved for future use will read 1[0] - 1: Disable 1588 Interrupt to LASI pad0: Enable  1588 Interrupt to LASI pad
 *
 ******************************************************************************/
#define PLP_P1588_INTERRUPT_CONTROLr    0x000000a7

#define PLP_P1588_INTERRUPT_CONTROLr_SIZE 4

/*
 * This structure should be used to declare and program P1588_INTERRUPT_CONTROL.
 *
 */
typedef union PLP_P1588_INTERRUPT_CONTROLr_s {
	uint32_t v[1];
	uint32_t p1588_interrupt_control[1];
	uint32_t _p1588_interrupt_control;
} PLP_P1588_INTERRUPT_CONTROLr_t;

#define PLP_P1588_INTERRUPT_CONTROLr_CLR(r) (r).p1588_interrupt_control[0] = 0
#define PLP_P1588_INTERRUPT_CONTROLr_SET(r,d) (r).p1588_interrupt_control[0] = d
#define PLP_P1588_INTERRUPT_CONTROLr_GET(r) (r).p1588_interrupt_control[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_INTERRUPT_CONTROLr_INTERRUPT_CTRLf_GET(r) (((r).p1588_interrupt_control[0]) & 0xffff)
#define PLP_P1588_INTERRUPT_CONTROLr_INTERRUPT_CTRLf_SET(r,f) (r).p1588_interrupt_control[0]=(((r).p1588_interrupt_control[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_INTERRUPT_CONTROL.
 *
 */
#define PLP_READ_P1588_INTERRUPT_CONTROLr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_INTERRUPT_CONTROLr,(_r._p1588_interrupt_control))
#define PLP_WRITE_P1588_INTERRUPT_CONTROLr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_INTERRUPT_CONTROLr,(_r._p1588_interrupt_control))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_INTERRUPT_CONTROLr PLP_P1588_INTERRUPT_CONTROLr
#define P1588_INTERRUPT_CONTROLr_SIZE PLP_P1588_INTERRUPT_CONTROLr_SIZE
typedef PLP_P1588_INTERRUPT_CONTROLr_t P1588_INTERRUPT_CONTROLr_t;
#define P1588_INTERRUPT_CONTROLr_CLR PLP_P1588_INTERRUPT_CONTROLr_CLR
#define P1588_INTERRUPT_CONTROLr_SET PLP_P1588_INTERRUPT_CONTROLr_SET
#define P1588_INTERRUPT_CONTROLr_GET PLP_P1588_INTERRUPT_CONTROLr_GET
#define P1588_INTERRUPT_CONTROLr_INTERRUPT_CTRLf_GET PLP_P1588_INTERRUPT_CONTROLr_INTERRUPT_CTRLf_GET
#define P1588_INTERRUPT_CONTROLr_INTERRUPT_CTRLf_SET PLP_P1588_INTERRUPT_CONTROLr_INTERRUPT_CTRLf_SET
#define READ_P1588_INTERRUPT_CONTROLr PLP_READ_P1588_INTERRUPT_CONTROLr
#define WRITE_P1588_INTERRUPT_CONTROLr PLP_WRITE_P1588_INTERRUPT_CONTROLr
#define MODIFY_P1588_INTERRUPT_CONTROLr PLP_MODIFY_P1588_INTERRUPT_CONTROLr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_INTERRUPT_CONTROLr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_NTP_1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70a8
 * DESC:     NSE NTP 1 Register
 * RESETVAL: 0x5463 (21603)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_NTP_FREQCNTRL_15_0 NTP Frequency control wordlower 16b of 20b NTP counter initial valuenote to programmer:  override default value with this value 0x542d for 128.92 MHz
 *
 ******************************************************************************/
#define PLP_P1588_NSE_NTP_1r    0x000000a8
#define P1588_NTP_Freq_Ctrl_Word_0           PLP_P1588_NSE_NTP_1r

#define PLP_P1588_NSE_NTP_1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_NTP_1.
 *
 */
typedef union PLP_P1588_NSE_NTP_1r_s {
	uint32_t v[1];
	uint32_t p1588_nse_ntp_1[1];
	uint32_t _p1588_nse_ntp_1;
} PLP_P1588_NSE_NTP_1r_t;

#define PLP_P1588_NSE_NTP_1r_CLR(r) (r).p1588_nse_ntp_1[0] = 0
#define PLP_P1588_NSE_NTP_1r_SET(r,d) (r).p1588_nse_ntp_1[0] = d
#define PLP_P1588_NSE_NTP_1r_GET(r) (r).p1588_nse_ntp_1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_NTP_1r_NSE_REG_NTP_FREQCNTRL_15_0f_GET(r) (((r).p1588_nse_ntp_1[0]) & 0xffff)
#define PLP_P1588_NSE_NTP_1r_NSE_REG_NTP_FREQCNTRL_15_0f_SET(r,f) (r).p1588_nse_ntp_1[0]=(((r).p1588_nse_ntp_1[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_NTP_1.
 *
 */
#define PLP_READ_P1588_NSE_NTP_1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_NTP_1r,(_r._p1588_nse_ntp_1))
#define PLP_WRITE_P1588_NSE_NTP_1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_NTP_1r,(_r._p1588_nse_ntp_1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_NTP_1r PLP_P1588_NSE_NTP_1r
#define P1588_NSE_NTP_1r_SIZE PLP_P1588_NSE_NTP_1r_SIZE
typedef PLP_P1588_NSE_NTP_1r_t P1588_NSE_NTP_1r_t;
#define P1588_NSE_NTP_1r_CLR PLP_P1588_NSE_NTP_1r_CLR
#define P1588_NSE_NTP_1r_SET PLP_P1588_NSE_NTP_1r_SET
#define P1588_NSE_NTP_1r_GET PLP_P1588_NSE_NTP_1r_GET
#define P1588_NSE_NTP_1r_NSE_REG_NTP_FREQCNTRL_15_0f_GET PLP_P1588_NSE_NTP_1r_NSE_REG_NTP_FREQCNTRL_15_0f_GET
#define P1588_NSE_NTP_1r_NSE_REG_NTP_FREQCNTRL_15_0f_SET PLP_P1588_NSE_NTP_1r_NSE_REG_NTP_FREQCNTRL_15_0f_SET
#define READ_P1588_NSE_NTP_1r PLP_READ_P1588_NSE_NTP_1r
#define WRITE_P1588_NSE_NTP_1r PLP_WRITE_P1588_NSE_NTP_1r
#define MODIFY_P1588_NSE_NTP_1r PLP_MODIFY_P1588_NSE_NTP_1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_NTP_1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_NTP_2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70a9
 * DESC:     NSE NTP 2 Register
 * RESETVAL: 0x8 (8)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_NTP_FREQCNTRL_19_16 NTP Frequency control wordupper 4b of 20b NTP counter initial value
 *
 ******************************************************************************/
#define PLP_P1588_NSE_NTP_2r    0x000000a9
#define P1588_NTP_Freq_Ctrl_Word_19         PLP_P1588_NSE_NTP_2r

#define PLP_P1588_NSE_NTP_2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_NTP_2.
 *
 */
typedef union PLP_P1588_NSE_NTP_2r_s {
	uint32_t v[1];
	uint32_t p1588_nse_ntp_2[1];
	uint32_t _p1588_nse_ntp_2;
} PLP_P1588_NSE_NTP_2r_t;

#define PLP_P1588_NSE_NTP_2r_CLR(r) (r).p1588_nse_ntp_2[0] = 0
#define PLP_P1588_NSE_NTP_2r_SET(r,d) (r).p1588_nse_ntp_2[0] = d
#define PLP_P1588_NSE_NTP_2r_GET(r) (r).p1588_nse_ntp_2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_NTP_2r_NSE_REG_NTP_FREQCNTRL_19_16f_GET(r) (((r).p1588_nse_ntp_2[0]) & 0xf)
#define PLP_P1588_NSE_NTP_2r_NSE_REG_NTP_FREQCNTRL_19_16f_SET(r,f) (r).p1588_nse_ntp_2[0]=(((r).p1588_nse_ntp_2[0] & ~((uint32_t)0xf)) | (((uint32_t)f) & 0xf)) | (0xf << 16)

/*
 * These macros can be used to access P1588_NSE_NTP_2.
 *
 */
#define PLP_READ_P1588_NSE_NTP_2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_NTP_2r,(_r._p1588_nse_ntp_2))
#define PLP_WRITE_P1588_NSE_NTP_2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_NTP_2r,(_r._p1588_nse_ntp_2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_NTP_2r PLP_P1588_NSE_NTP_2r
#define P1588_NSE_NTP_2r_SIZE PLP_P1588_NSE_NTP_2r_SIZE
typedef PLP_P1588_NSE_NTP_2r_t P1588_NSE_NTP_2r_t;
#define P1588_NSE_NTP_2r_CLR PLP_P1588_NSE_NTP_2r_CLR
#define P1588_NSE_NTP_2r_SET PLP_P1588_NSE_NTP_2r_SET
#define P1588_NSE_NTP_2r_GET PLP_P1588_NSE_NTP_2r_GET
#define P1588_NSE_NTP_2r_NSE_REG_NTP_FREQCNTRL_19_16f_GET PLP_P1588_NSE_NTP_2r_NSE_REG_NTP_FREQCNTRL_19_16f_GET
#define P1588_NSE_NTP_2r_NSE_REG_NTP_FREQCNTRL_19_16f_SET PLP_P1588_NSE_NTP_2r_NSE_REG_NTP_FREQCNTRL_19_16f_SET
#define READ_P1588_NSE_NTP_2r PLP_READ_P1588_NSE_NTP_2r
#define WRITE_P1588_NSE_NTP_2r PLP_WRITE_P1588_NSE_NTP_2r
#define MODIFY_P1588_NSE_NTP_2r PLP_MODIFY_P1588_NSE_NTP_2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_NTP_2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_NTP_3
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70aa
 * DESC:     NSE NTP 3 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NTP_TIME_STAMP_REG_15_0 NTP Second and Fractional second control wordbits[15:0] of 58b NTP counter initial value
 *
 ******************************************************************************/
#define PLP_P1588_NSE_NTP_3r    0x000000aa

#define PLP_P1588_NSE_NTP_3r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_NTP_3.
 *
 */
typedef union PLP_P1588_NSE_NTP_3r_s {
	uint32_t v[1];
	uint32_t p1588_nse_ntp_3[1];
	uint32_t _p1588_nse_ntp_3;
} PLP_P1588_NSE_NTP_3r_t;

#define PLP_P1588_NSE_NTP_3r_CLR(r) (r).p1588_nse_ntp_3[0] = 0
#define PLP_P1588_NSE_NTP_3r_SET(r,d) (r).p1588_nse_ntp_3[0] = d
#define PLP_P1588_NSE_NTP_3r_GET(r) (r).p1588_nse_ntp_3[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_NTP_3r_NTP_TIME_STAMP_REG_15_0f_GET(r) (((r).p1588_nse_ntp_3[0]) & 0xffff)
#define PLP_P1588_NSE_NTP_3r_NTP_TIME_STAMP_REG_15_0f_SET(r,f) (r).p1588_nse_ntp_3[0]=(((r).p1588_nse_ntp_3[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_NTP_3.
 *
 */
#define PLP_READ_P1588_NSE_NTP_3r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_NTP_3r,(_r._p1588_nse_ntp_3))
#define PLP_WRITE_P1588_NSE_NTP_3r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_NTP_3r,(_r._p1588_nse_ntp_3))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_NTP_3r PLP_P1588_NSE_NTP_3r
#define P1588_NSE_NTP_3r_SIZE PLP_P1588_NSE_NTP_3r_SIZE
typedef PLP_P1588_NSE_NTP_3r_t P1588_NSE_NTP_3r_t;
#define P1588_NSE_NTP_3r_CLR PLP_P1588_NSE_NTP_3r_CLR
#define P1588_NSE_NTP_3r_SET PLP_P1588_NSE_NTP_3r_SET
#define P1588_NSE_NTP_3r_GET PLP_P1588_NSE_NTP_3r_GET
#define P1588_NSE_NTP_3r_NTP_TIME_STAMP_REG_15_0f_GET PLP_P1588_NSE_NTP_3r_NTP_TIME_STAMP_REG_15_0f_GET
#define P1588_NSE_NTP_3r_NTP_TIME_STAMP_REG_15_0f_SET PLP_P1588_NSE_NTP_3r_NTP_TIME_STAMP_REG_15_0f_SET
#define READ_P1588_NSE_NTP_3r PLP_READ_P1588_NSE_NTP_3r
#define WRITE_P1588_NSE_NTP_3r PLP_WRITE_P1588_NSE_NTP_3r
#define MODIFY_P1588_NSE_NTP_3r PLP_MODIFY_P1588_NSE_NTP_3r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_NTP_3r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_NTP_4
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70ab
 * DESC:     NSE NTP 4 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NTP_TIME_STAMP_REG_31_16 NTP Second and Fractional second control wordbits[31:16] of 58b NTP counter initial value
 *
 ******************************************************************************/
#define PLP_P1588_NSE_NTP_4r    0x000000ab

#define PLP_P1588_NSE_NTP_4r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_NTP_4.
 *
 */
typedef union PLP_P1588_NSE_NTP_4r_s {
	uint32_t v[1];
	uint32_t p1588_nse_ntp_4[1];
	uint32_t _p1588_nse_ntp_4;
} PLP_P1588_NSE_NTP_4r_t;

#define PLP_P1588_NSE_NTP_4r_CLR(r) (r).p1588_nse_ntp_4[0] = 0
#define PLP_P1588_NSE_NTP_4r_SET(r,d) (r).p1588_nse_ntp_4[0] = d
#define PLP_P1588_NSE_NTP_4r_GET(r) (r).p1588_nse_ntp_4[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_NTP_4r_NTP_TIME_STAMP_REG_31_16f_GET(r) (((r).p1588_nse_ntp_4[0]) & 0xffff)
#define PLP_P1588_NSE_NTP_4r_NTP_TIME_STAMP_REG_31_16f_SET(r,f) (r).p1588_nse_ntp_4[0]=(((r).p1588_nse_ntp_4[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_NTP_4.
 *
 */
#define PLP_READ_P1588_NSE_NTP_4r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_NTP_4r,(_r._p1588_nse_ntp_4))
#define PLP_WRITE_P1588_NSE_NTP_4r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_NTP_4r,(_r._p1588_nse_ntp_4))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_NTP_4r PLP_P1588_NSE_NTP_4r
#define P1588_NSE_NTP_4r_SIZE PLP_P1588_NSE_NTP_4r_SIZE
typedef PLP_P1588_NSE_NTP_4r_t P1588_NSE_NTP_4r_t;
#define P1588_NSE_NTP_4r_CLR PLP_P1588_NSE_NTP_4r_CLR
#define P1588_NSE_NTP_4r_SET PLP_P1588_NSE_NTP_4r_SET
#define P1588_NSE_NTP_4r_GET PLP_P1588_NSE_NTP_4r_GET
#define P1588_NSE_NTP_4r_NTP_TIME_STAMP_REG_31_16f_GET PLP_P1588_NSE_NTP_4r_NTP_TIME_STAMP_REG_31_16f_GET
#define P1588_NSE_NTP_4r_NTP_TIME_STAMP_REG_31_16f_SET PLP_P1588_NSE_NTP_4r_NTP_TIME_STAMP_REG_31_16f_SET
#define READ_P1588_NSE_NTP_4r PLP_READ_P1588_NSE_NTP_4r
#define WRITE_P1588_NSE_NTP_4r PLP_WRITE_P1588_NSE_NTP_4r
#define MODIFY_P1588_NSE_NTP_4r PLP_MODIFY_P1588_NSE_NTP_4r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_NTP_4r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_NTP_5
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70ac
 * DESC:     NSE NTP 5 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NTP_TIME_STAMP_REG_47_32 NTP Second and Fractional second control wordbits[47:32] of 58b NTP counter initial value
 *
 ******************************************************************************/
#define PLP_P1588_NSE_NTP_5r    0x000000ac

#define PLP_P1588_NSE_NTP_5r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_NTP_5.
 *
 */
typedef union PLP_P1588_NSE_NTP_5r_s {
	uint32_t v[1];
	uint32_t p1588_nse_ntp_5[1];
	uint32_t _p1588_nse_ntp_5;
} PLP_P1588_NSE_NTP_5r_t;

#define PLP_P1588_NSE_NTP_5r_CLR(r) (r).p1588_nse_ntp_5[0] = 0
#define PLP_P1588_NSE_NTP_5r_SET(r,d) (r).p1588_nse_ntp_5[0] = d
#define PLP_P1588_NSE_NTP_5r_GET(r) (r).p1588_nse_ntp_5[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_NTP_5r_NTP_TIME_STAMP_REG_47_32f_GET(r) (((r).p1588_nse_ntp_5[0]) & 0xffff)
#define PLP_P1588_NSE_NTP_5r_NTP_TIME_STAMP_REG_47_32f_SET(r,f) (r).p1588_nse_ntp_5[0]=(((r).p1588_nse_ntp_5[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_NTP_5.
 *
 */
#define PLP_READ_P1588_NSE_NTP_5r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_NTP_5r,(_r._p1588_nse_ntp_5))
#define PLP_WRITE_P1588_NSE_NTP_5r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_NTP_5r,(_r._p1588_nse_ntp_5))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_NTP_5r PLP_P1588_NSE_NTP_5r
#define P1588_NSE_NTP_5r_SIZE PLP_P1588_NSE_NTP_5r_SIZE
typedef PLP_P1588_NSE_NTP_5r_t P1588_NSE_NTP_5r_t;
#define P1588_NSE_NTP_5r_CLR PLP_P1588_NSE_NTP_5r_CLR
#define P1588_NSE_NTP_5r_SET PLP_P1588_NSE_NTP_5r_SET
#define P1588_NSE_NTP_5r_GET PLP_P1588_NSE_NTP_5r_GET
#define P1588_NSE_NTP_5r_NTP_TIME_STAMP_REG_47_32f_GET PLP_P1588_NSE_NTP_5r_NTP_TIME_STAMP_REG_47_32f_GET
#define P1588_NSE_NTP_5r_NTP_TIME_STAMP_REG_47_32f_SET PLP_P1588_NSE_NTP_5r_NTP_TIME_STAMP_REG_47_32f_SET
#define READ_P1588_NSE_NTP_5r PLP_READ_P1588_NSE_NTP_5r
#define WRITE_P1588_NSE_NTP_5r PLP_WRITE_P1588_NSE_NTP_5r
#define MODIFY_P1588_NSE_NTP_5r PLP_MODIFY_P1588_NSE_NTP_5r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_NTP_5r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_NTP_6
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70ad
 * DESC:     NSE NTP 6 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NTP_TIME_STAMP_REG_57_48 NTP Second and Fractional second control wordbits[63:48] of 64b NTP counter initial value
 *
 ******************************************************************************/
#define PLP_P1588_NSE_NTP_6r    0x000000ad

#define PLP_P1588_NSE_NTP_6r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_NTP_6.
 *
 */
typedef union PLP_P1588_NSE_NTP_6r_s {
	uint32_t v[1];
	uint32_t p1588_nse_ntp_6[1];
	uint32_t _p1588_nse_ntp_6;
} PLP_P1588_NSE_NTP_6r_t;

#define PLP_P1588_NSE_NTP_6r_CLR(r) (r).p1588_nse_ntp_6[0] = 0
#define PLP_P1588_NSE_NTP_6r_SET(r,d) (r).p1588_nse_ntp_6[0] = d
#define PLP_P1588_NSE_NTP_6r_GET(r) (r).p1588_nse_ntp_6[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_NTP_6r_NTP_TIME_STAMP_REG_57_48f_GET(r) (((r).p1588_nse_ntp_6[0]) & 0xffff)
#define PLP_P1588_NSE_NTP_6r_NTP_TIME_STAMP_REG_57_48f_SET(r,f) (r).p1588_nse_ntp_6[0]=(((r).p1588_nse_ntp_6[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_NTP_6.
 *
 */
#define PLP_READ_P1588_NSE_NTP_6r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_NTP_6r,(_r._p1588_nse_ntp_6))
#define PLP_WRITE_P1588_NSE_NTP_6r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_NTP_6r,(_r._p1588_nse_ntp_6))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_NTP_6r PLP_P1588_NSE_NTP_6r
#define P1588_NSE_NTP_6r_SIZE PLP_P1588_NSE_NTP_6r_SIZE
typedef PLP_P1588_NSE_NTP_6r_t P1588_NSE_NTP_6r_t;
#define P1588_NSE_NTP_6r_CLR PLP_P1588_NSE_NTP_6r_CLR
#define P1588_NSE_NTP_6r_SET PLP_P1588_NSE_NTP_6r_SET
#define P1588_NSE_NTP_6r_GET PLP_P1588_NSE_NTP_6r_GET
#define P1588_NSE_NTP_6r_NTP_TIME_STAMP_REG_57_48f_GET PLP_P1588_NSE_NTP_6r_NTP_TIME_STAMP_REG_57_48f_GET
#define P1588_NSE_NTP_6r_NTP_TIME_STAMP_REG_57_48f_SET PLP_P1588_NSE_NTP_6r_NTP_TIME_STAMP_REG_57_48f_SET
#define READ_P1588_NSE_NTP_6r PLP_READ_P1588_NSE_NTP_6r
#define WRITE_P1588_NSE_NTP_6r PLP_WRITE_P1588_NSE_NTP_6r
#define MODIFY_P1588_NSE_NTP_6r PLP_MODIFY_P1588_NSE_NTP_6r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_NTP_6r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_NTP_7
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70ae
 * DESC:     NSE NTP 7 Register
 * RESETVAL: 0xcd65 (52581)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_NTP_DN_CNTRL_15_0 NTP Down Counter control wordlower 16b of 21b NTP counter initial valueDefault set to 1953125
 *
 ******************************************************************************/
#define PLP_P1588_NSE_NTP_7r    0x000000ae
#define P1588_NTP_Down_Counter_Ctrl_0      PLP_P1588_NSE_NTP_7r

#define PLP_P1588_NSE_NTP_7r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_NTP_7.
 *
 */
typedef union PLP_P1588_NSE_NTP_7r_s {
	uint32_t v[1];
	uint32_t p1588_nse_ntp_7[1];
	uint32_t _p1588_nse_ntp_7;
} PLP_P1588_NSE_NTP_7r_t;

#define PLP_P1588_NSE_NTP_7r_CLR(r) (r).p1588_nse_ntp_7[0] = 0
#define PLP_P1588_NSE_NTP_7r_SET(r,d) (r).p1588_nse_ntp_7[0] = d
#define PLP_P1588_NSE_NTP_7r_GET(r) (r).p1588_nse_ntp_7[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_NTP_7r_NSE_REG_NTP_DN_CNTRL_15_0f_GET(r) (((r).p1588_nse_ntp_7[0]) & 0xffff)
#define PLP_P1588_NSE_NTP_7r_NSE_REG_NTP_DN_CNTRL_15_0f_SET(r,f) (r).p1588_nse_ntp_7[0]=(((r).p1588_nse_ntp_7[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_NTP_7.
 *
 */
#define PLP_READ_P1588_NSE_NTP_7r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_NTP_7r,(_r._p1588_nse_ntp_7))
#define PLP_WRITE_P1588_NSE_NTP_7r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_NTP_7r,(_r._p1588_nse_ntp_7))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_NTP_7r PLP_P1588_NSE_NTP_7r
#define P1588_NSE_NTP_7r_SIZE PLP_P1588_NSE_NTP_7r_SIZE
typedef PLP_P1588_NSE_NTP_7r_t P1588_NSE_NTP_7r_t;
#define P1588_NSE_NTP_7r_CLR PLP_P1588_NSE_NTP_7r_CLR
#define P1588_NSE_NTP_7r_SET PLP_P1588_NSE_NTP_7r_SET
#define P1588_NSE_NTP_7r_GET PLP_P1588_NSE_NTP_7r_GET
#define P1588_NSE_NTP_7r_NSE_REG_NTP_DN_CNTRL_15_0f_GET PLP_P1588_NSE_NTP_7r_NSE_REG_NTP_DN_CNTRL_15_0f_GET
#define P1588_NSE_NTP_7r_NSE_REG_NTP_DN_CNTRL_15_0f_SET PLP_P1588_NSE_NTP_7r_NSE_REG_NTP_DN_CNTRL_15_0f_SET
#define READ_P1588_NSE_NTP_7r PLP_READ_P1588_NSE_NTP_7r
#define WRITE_P1588_NSE_NTP_7r PLP_WRITE_P1588_NSE_NTP_7r
#define MODIFY_P1588_NSE_NTP_7r PLP_MODIFY_P1588_NSE_NTP_7r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_NTP_7r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_NTP_8
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70af
 * DESC:     NSE NTP 8 Register
 * RESETVAL: 0x1d (29)
 * ACCESS:   R/W
 * FIELDS:
 *     NSE_REG_NTP_DN_CNTRL_20_16 NTP Down Counter control wordupper 5b of 21b NTP counter initial valueDefault set to 1953125
 *     TC_64_LEAP       [15] - leap59, increment 31b counter and not the 1b in the 64b NTP block[14] - leap61, no increment to 31b counter and 1b in the 64b NTP block
 *
 ******************************************************************************/
#define PLP_P1588_NSE_NTP_8r    0x000000af
#define P1588_NTP_Down_Counter_Ctrl_20      PLP_P1588_NSE_NTP_8r

#define PLP_P1588_NSE_NTP_8r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_NTP_8.
 *
 */
typedef union PLP_P1588_NSE_NTP_8r_s {
	uint32_t v[1];
	uint32_t p1588_nse_ntp_8[1];
	uint32_t _p1588_nse_ntp_8;
} PLP_P1588_NSE_NTP_8r_t;

#define PLP_P1588_NSE_NTP_8r_CLR(r) (r).p1588_nse_ntp_8[0] = 0
#define PLP_P1588_NSE_NTP_8r_SET(r,d) (r).p1588_nse_ntp_8[0] = d
#define PLP_P1588_NSE_NTP_8r_GET(r) (r).p1588_nse_ntp_8[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_NTP_8r_TC_64_LEAPf_GET(r) ((((r).p1588_nse_ntp_8[0]) >> 14) & 0x3)
#define PLP_P1588_NSE_NTP_8r_TC_64_LEAPf_SET(r,f) (r).p1588_nse_ntp_8[0]=(((r).p1588_nse_ntp_8[0] & ~((uint32_t)0x3 << 14)) | ((((uint32_t)f) & 0x3) << 14)) | (3 << (16 + 14))
#define PLP_P1588_NSE_NTP_8r_NSE_REG_NTP_DN_CNTRL_20_16f_GET(r) (((r).p1588_nse_ntp_8[0]) & 0x1f)
#define PLP_P1588_NSE_NTP_8r_NSE_REG_NTP_DN_CNTRL_20_16f_SET(r,f) (r).p1588_nse_ntp_8[0]=(((r).p1588_nse_ntp_8[0] & ~((uint32_t)0x1f)) | (((uint32_t)f) & 0x1f)) | (0x1f << 16)

/*
 * These macros can be used to access P1588_NSE_NTP_8.
 *
 */
#define PLP_READ_P1588_NSE_NTP_8r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_NTP_8r,(_r._p1588_nse_ntp_8))
#define PLP_WRITE_P1588_NSE_NTP_8r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_NTP_8r,(_r._p1588_nse_ntp_8))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_NTP_8r PLP_P1588_NSE_NTP_8r
#define P1588_NSE_NTP_8r_SIZE PLP_P1588_NSE_NTP_8r_SIZE
typedef PLP_P1588_NSE_NTP_8r_t P1588_NSE_NTP_8r_t;
#define P1588_NSE_NTP_8r_CLR PLP_P1588_NSE_NTP_8r_CLR
#define P1588_NSE_NTP_8r_SET PLP_P1588_NSE_NTP_8r_SET
#define P1588_NSE_NTP_8r_GET PLP_P1588_NSE_NTP_8r_GET
#define P1588_NSE_NTP_8r_TC_64_LEAPf_GET PLP_P1588_NSE_NTP_8r_TC_64_LEAPf_GET
#define P1588_NSE_NTP_8r_TC_64_LEAPf_SET PLP_P1588_NSE_NTP_8r_TC_64_LEAPf_SET
#define P1588_NSE_NTP_8r_NSE_REG_NTP_DN_CNTRL_20_16f_GET PLP_P1588_NSE_NTP_8r_NSE_REG_NTP_DN_CNTRL_20_16f_GET
#define P1588_NSE_NTP_8r_NSE_REG_NTP_DN_CNTRL_20_16f_SET PLP_P1588_NSE_NTP_8r_NSE_REG_NTP_DN_CNTRL_20_16f_SET
#define READ_P1588_NSE_NTP_8r PLP_READ_P1588_NSE_NTP_8r
#define WRITE_P1588_NSE_NTP_8r PLP_WRITE_P1588_NSE_NTP_8r
#define MODIFY_P1588_NSE_NTP_8r PLP_MODIFY_P1588_NSE_NTP_8r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_NTP_8r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_NTP_9
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70b0
 * DESC:     NSE NTP 9 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NTP_ERROR_CAP_15_0 NTP Error Capture registerLower 16b of 17b Error Capture value
 *
 ******************************************************************************/
#define PLP_P1588_NSE_NTP_9r    0x000000b0

#define PLP_P1588_NSE_NTP_9r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_NTP_9.
 *
 */
typedef union PLP_P1588_NSE_NTP_9r_s {
	uint32_t v[1];
	uint32_t p1588_nse_ntp_9[1];
	uint32_t _p1588_nse_ntp_9;
} PLP_P1588_NSE_NTP_9r_t;

#define PLP_P1588_NSE_NTP_9r_CLR(r) (r).p1588_nse_ntp_9[0] = 0
#define PLP_P1588_NSE_NTP_9r_SET(r,d) (r).p1588_nse_ntp_9[0] = d
#define PLP_P1588_NSE_NTP_9r_GET(r) (r).p1588_nse_ntp_9[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_NTP_9r_NTP_ERROR_CAP_15_0f_GET(r) (((r).p1588_nse_ntp_9[0]) & 0xffff)
#define PLP_P1588_NSE_NTP_9r_NTP_ERROR_CAP_15_0f_SET(r,f) (r).p1588_nse_ntp_9[0]=(((r).p1588_nse_ntp_9[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_NSE_NTP_9.
 *
 */
#define PLP_READ_P1588_NSE_NTP_9r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_NTP_9r,(_r._p1588_nse_ntp_9))
#define PLP_WRITE_P1588_NSE_NTP_9r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_NTP_9r,(_r._p1588_nse_ntp_9))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_NTP_9r PLP_P1588_NSE_NTP_9r
#define P1588_NSE_NTP_9r_SIZE PLP_P1588_NSE_NTP_9r_SIZE
typedef PLP_P1588_NSE_NTP_9r_t P1588_NSE_NTP_9r_t;
#define P1588_NSE_NTP_9r_CLR PLP_P1588_NSE_NTP_9r_CLR
#define P1588_NSE_NTP_9r_SET PLP_P1588_NSE_NTP_9r_SET
#define P1588_NSE_NTP_9r_GET PLP_P1588_NSE_NTP_9r_GET
#define P1588_NSE_NTP_9r_NTP_ERROR_CAP_15_0f_GET PLP_P1588_NSE_NTP_9r_NTP_ERROR_CAP_15_0f_GET
#define P1588_NSE_NTP_9r_NTP_ERROR_CAP_15_0f_SET PLP_P1588_NSE_NTP_9r_NTP_ERROR_CAP_15_0f_SET
#define READ_P1588_NSE_NTP_9r PLP_READ_P1588_NSE_NTP_9r
#define WRITE_P1588_NSE_NTP_9r PLP_WRITE_P1588_NSE_NTP_9r
#define MODIFY_P1588_NSE_NTP_9r PLP_MODIFY_P1588_NSE_NTP_9r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_NTP_9r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_NSE_NTP_10
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70b1
 * DESC:     NSE NTP 10 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     NTP_ERROR_CAP_16 NTP Error Capture registerUpper 1b of 17b Error Capture value
 *
 ******************************************************************************/
#define PLP_P1588_NSE_NTP_10r    0x000000b1

#define PLP_P1588_NSE_NTP_10r_SIZE 4

/*
 * This structure should be used to declare and program P1588_NSE_NTP_10.
 *
 */
typedef union PLP_P1588_NSE_NTP_10r_s {
	uint32_t v[1];
	uint32_t p1588_nse_ntp_10[1];
	uint32_t _p1588_nse_ntp_10;
} PLP_P1588_NSE_NTP_10r_t;

#define PLP_P1588_NSE_NTP_10r_CLR(r) (r).p1588_nse_ntp_10[0] = 0
#define PLP_P1588_NSE_NTP_10r_SET(r,d) (r).p1588_nse_ntp_10[0] = d
#define PLP_P1588_NSE_NTP_10r_GET(r) (r).p1588_nse_ntp_10[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_NSE_NTP_10r_NTP_ERROR_CAP_16f_GET(r) (((r).p1588_nse_ntp_10[0]) & 0x1)
#define PLP_P1588_NSE_NTP_10r_NTP_ERROR_CAP_16f_SET(r,f) (r).p1588_nse_ntp_10[0]=(((r).p1588_nse_ntp_10[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1)) | (0x1 << 16)

/*
 * These macros can be used to access P1588_NSE_NTP_10.
 *
 */
#define PLP_READ_P1588_NSE_NTP_10r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_NSE_NTP_10r,(_r._p1588_nse_ntp_10))
#define PLP_WRITE_P1588_NSE_NTP_10r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_NSE_NTP_10r,(_r._p1588_nse_ntp_10))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_NSE_NTP_10r PLP_P1588_NSE_NTP_10r
#define P1588_NSE_NTP_10r_SIZE PLP_P1588_NSE_NTP_10r_SIZE
typedef PLP_P1588_NSE_NTP_10r_t P1588_NSE_NTP_10r_t;
#define P1588_NSE_NTP_10r_CLR PLP_P1588_NSE_NTP_10r_CLR
#define P1588_NSE_NTP_10r_SET PLP_P1588_NSE_NTP_10r_SET
#define P1588_NSE_NTP_10r_GET PLP_P1588_NSE_NTP_10r_GET
#define P1588_NSE_NTP_10r_NTP_ERROR_CAP_16f_GET PLP_P1588_NSE_NTP_10r_NTP_ERROR_CAP_16f_GET
#define P1588_NSE_NTP_10r_NTP_ERROR_CAP_16f_SET PLP_P1588_NSE_NTP_10r_NTP_ERROR_CAP_16f_SET
#define READ_P1588_NSE_NTP_10r PLP_READ_P1588_NSE_NTP_10r
#define WRITE_P1588_NSE_NTP_10r PLP_WRITE_P1588_NSE_NTP_10r
#define MODIFY_P1588_NSE_NTP_10r PLP_MODIFY_P1588_NSE_NTP_10r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_NSE_NTP_10r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TXPKT_Y1731_COUNTER
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70b2
 * DESC:     TX Y1731 COUNTER Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_Y1731_COUNTER Egress Y1731 counterCounts all Y1731 packetsNOTE: THESE BITS ARE CLEAR ON READ AND NEED TO BEHANDLED OUTSIDE THE REGISTER BLOCK
 *
 ******************************************************************************/
#define PLP_P1588_TXPKT_Y1731_COUNTERr    0x000000b2

#define PLP_P1588_TXPKT_Y1731_COUNTERr_SIZE 4

/*
 * This structure should be used to declare and program P1588_TXPKT_Y1731_COUNTER.
 *
 */
typedef union PLP_P1588_TXPKT_Y1731_COUNTERr_s {
	uint32_t v[1];
	uint32_t p1588_txpkt_y1731_counter[1];
	uint32_t _p1588_txpkt_y1731_counter;
} PLP_P1588_TXPKT_Y1731_COUNTERr_t;

#define PLP_P1588_TXPKT_Y1731_COUNTERr_CLR(r) (r).p1588_txpkt_y1731_counter[0] = 0
#define PLP_P1588_TXPKT_Y1731_COUNTERr_SET(r,d) (r).p1588_txpkt_y1731_counter[0] = d
#define PLP_P1588_TXPKT_Y1731_COUNTERr_GET(r) (r).p1588_txpkt_y1731_counter[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TXPKT_Y1731_COUNTERr_TX_Y1731_COUNTERf_GET(r) (((r).p1588_txpkt_y1731_counter[0]) & 0xffff)
#define PLP_P1588_TXPKT_Y1731_COUNTERr_TX_Y1731_COUNTERf_SET(r,f) (r).p1588_txpkt_y1731_counter[0]=(((r).p1588_txpkt_y1731_counter[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TXPKT_Y1731_COUNTER.
 *
 */
#define PLP_READ_P1588_TXPKT_Y1731_COUNTERr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TXPKT_Y1731_COUNTERr,(_r._p1588_txpkt_y1731_counter))
#define PLP_WRITE_P1588_TXPKT_Y1731_COUNTERr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TXPKT_Y1731_COUNTERr,(_r._p1588_txpkt_y1731_counter))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TXPKT_Y1731_COUNTERr PLP_P1588_TXPKT_Y1731_COUNTERr
#define P1588_TXPKT_Y1731_COUNTERr_SIZE PLP_P1588_TXPKT_Y1731_COUNTERr_SIZE
typedef PLP_P1588_TXPKT_Y1731_COUNTERr_t P1588_TXPKT_Y1731_COUNTERr_t;
#define P1588_TXPKT_Y1731_COUNTERr_CLR PLP_P1588_TXPKT_Y1731_COUNTERr_CLR
#define P1588_TXPKT_Y1731_COUNTERr_SET PLP_P1588_TXPKT_Y1731_COUNTERr_SET
#define P1588_TXPKT_Y1731_COUNTERr_GET PLP_P1588_TXPKT_Y1731_COUNTERr_GET
#define P1588_TXPKT_Y1731_COUNTERr_TX_Y1731_COUNTERf_GET PLP_P1588_TXPKT_Y1731_COUNTERr_TX_Y1731_COUNTERf_GET
#define P1588_TXPKT_Y1731_COUNTERr_TX_Y1731_COUNTERf_SET PLP_P1588_TXPKT_Y1731_COUNTERr_TX_Y1731_COUNTERf_SET
#define READ_P1588_TXPKT_Y1731_COUNTERr PLP_READ_P1588_TXPKT_Y1731_COUNTERr
#define WRITE_P1588_TXPKT_Y1731_COUNTERr PLP_WRITE_P1588_TXPKT_Y1731_COUNTERr
#define MODIFY_P1588_TXPKT_Y1731_COUNTERr PLP_MODIFY_P1588_TXPKT_Y1731_COUNTERr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TXPKT_Y1731_COUNTERr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_RXPKT_Y1731_COUNTER
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70b3
 * DESC:     RX Y1731 COUNTER Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_Y1731_COUNTER Ingress Y1731 counterCounts all Y1731 packetsNOTE: THESE BITS ARE CLEAR ON READ AND NEED TO BEHANDLED OUTSIDE THE REGISTER BLOCK
 *
 ******************************************************************************/
#define PLP_P1588_RXPKT_Y1731_COUNTERr    0x000000b3

#define PLP_P1588_RXPKT_Y1731_COUNTERr_SIZE 4

/*
 * This structure should be used to declare and program P1588_RXPKT_Y1731_COUNTER.
 *
 */
typedef union PLP_P1588_RXPKT_Y1731_COUNTERr_s {
	uint32_t v[1];
	uint32_t p1588_rxpkt_y1731_counter[1];
	uint32_t _p1588_rxpkt_y1731_counter;
} PLP_P1588_RXPKT_Y1731_COUNTERr_t;

#define PLP_P1588_RXPKT_Y1731_COUNTERr_CLR(r) (r).p1588_rxpkt_y1731_counter[0] = 0
#define PLP_P1588_RXPKT_Y1731_COUNTERr_SET(r,d) (r).p1588_rxpkt_y1731_counter[0] = d
#define PLP_P1588_RXPKT_Y1731_COUNTERr_GET(r) (r).p1588_rxpkt_y1731_counter[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_RXPKT_Y1731_COUNTERr_RX_Y1731_COUNTERf_GET(r) (((r).p1588_rxpkt_y1731_counter[0]) & 0xffff)
#define PLP_P1588_RXPKT_Y1731_COUNTERr_RX_Y1731_COUNTERf_SET(r,f) (r).p1588_rxpkt_y1731_counter[0]=(((r).p1588_rxpkt_y1731_counter[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_RXPKT_Y1731_COUNTER.
 *
 */
#define PLP_READ_P1588_RXPKT_Y1731_COUNTERr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_RXPKT_Y1731_COUNTERr,(_r._p1588_rxpkt_y1731_counter))
#define PLP_WRITE_P1588_RXPKT_Y1731_COUNTERr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_RXPKT_Y1731_COUNTERr,(_r._p1588_rxpkt_y1731_counter))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_RXPKT_Y1731_COUNTERr PLP_P1588_RXPKT_Y1731_COUNTERr
#define P1588_RXPKT_Y1731_COUNTERr_SIZE PLP_P1588_RXPKT_Y1731_COUNTERr_SIZE
typedef PLP_P1588_RXPKT_Y1731_COUNTERr_t P1588_RXPKT_Y1731_COUNTERr_t;
#define P1588_RXPKT_Y1731_COUNTERr_CLR PLP_P1588_RXPKT_Y1731_COUNTERr_CLR
#define P1588_RXPKT_Y1731_COUNTERr_SET PLP_P1588_RXPKT_Y1731_COUNTERr_SET
#define P1588_RXPKT_Y1731_COUNTERr_GET PLP_P1588_RXPKT_Y1731_COUNTERr_GET
#define P1588_RXPKT_Y1731_COUNTERr_RX_Y1731_COUNTERf_GET PLP_P1588_RXPKT_Y1731_COUNTERr_RX_Y1731_COUNTERf_GET
#define P1588_RXPKT_Y1731_COUNTERr_RX_Y1731_COUNTERf_SET PLP_P1588_RXPKT_Y1731_COUNTERr_RX_Y1731_COUNTERf_SET
#define READ_P1588_RXPKT_Y1731_COUNTERr PLP_READ_P1588_RXPKT_Y1731_COUNTERr
#define WRITE_P1588_RXPKT_Y1731_COUNTERr PLP_WRITE_P1588_RXPKT_Y1731_COUNTERr
#define MODIFY_P1588_RXPKT_Y1731_COUNTERr PLP_MODIFY_P1588_RXPKT_Y1731_COUNTERr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_RXPKT_Y1731_COUNTERr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TS_HB_SEL_12
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70b4
 * DESC:     TS HB SEL 12 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOP_TS_INFO_63_56 [7:0] - Contains Domain number info
 *
 ******************************************************************************/
#define PLP_P1588_TS_HB_SEL_12r    0x000000b4

#define PLP_P1588_TS_HB_SEL_12r_SIZE 4

/*
 * This structure should be used to declare and program P1588_TS_HB_SEL_12.
 *
 */
typedef union PLP_P1588_TS_HB_SEL_12r_s {
	uint32_t v[1];
	uint32_t p1588_ts_hb_sel_12[1];
	uint32_t _p1588_ts_hb_sel_12;
} PLP_P1588_TS_HB_SEL_12r_t;

#define PLP_P1588_TS_HB_SEL_12r_CLR(r) (r).p1588_ts_hb_sel_12[0] = 0
#define PLP_P1588_TS_HB_SEL_12r_SET(r,d) (r).p1588_ts_hb_sel_12[0] = d
#define PLP_P1588_TS_HB_SEL_12r_GET(r) (r).p1588_ts_hb_sel_12[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TS_HB_SEL_12r_SOP_TS_INFO_63_56f_GET(r) (((r).p1588_ts_hb_sel_12[0]) & 0xff)
#define PLP_P1588_TS_HB_SEL_12r_SOP_TS_INFO_63_56f_SET(r,f) (r).p1588_ts_hb_sel_12[0]=(((r).p1588_ts_hb_sel_12[0] & ~((uint32_t)0xff)) | (((uint32_t)f) & 0xff)) | (0xff << 16)

/*
 * These macros can be used to access P1588_TS_HB_SEL_12.
 *
 */
#define PLP_READ_P1588_TS_HB_SEL_12r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TS_HB_SEL_12r,(_r._p1588_ts_hb_sel_12))
#define PLP_WRITE_P1588_TS_HB_SEL_12r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TS_HB_SEL_12r,(_r._p1588_ts_hb_sel_12))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TS_HB_SEL_12r PLP_P1588_TS_HB_SEL_12r
#define P1588_TS_HB_SEL_12r_SIZE PLP_P1588_TS_HB_SEL_12r_SIZE
typedef PLP_P1588_TS_HB_SEL_12r_t P1588_TS_HB_SEL_12r_t;
#define P1588_TS_HB_SEL_12r_CLR PLP_P1588_TS_HB_SEL_12r_CLR
#define P1588_TS_HB_SEL_12r_SET PLP_P1588_TS_HB_SEL_12r_SET
#define P1588_TS_HB_SEL_12r_GET PLP_P1588_TS_HB_SEL_12r_GET
#define P1588_TS_HB_SEL_12r_SOP_TS_INFO_63_56f_GET PLP_P1588_TS_HB_SEL_12r_SOP_TS_INFO_63_56f_GET
#define P1588_TS_HB_SEL_12r_SOP_TS_INFO_63_56f_SET PLP_P1588_TS_HB_SEL_12r_SOP_TS_INFO_63_56f_SET
#define READ_P1588_TS_HB_SEL_12r PLP_READ_P1588_TS_HB_SEL_12r
#define WRITE_P1588_TS_HB_SEL_12r PLP_WRITE_P1588_TS_HB_SEL_12r
#define MODIFY_P1588_TS_HB_SEL_12r PLP_MODIFY_P1588_TS_HB_SEL_12r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TS_HB_SEL_12r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TS_HB_SEL_13
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70b5
 * DESC:     TS HB SEL 13 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOP_TS_63_48     [15:0] - Contains [63:48] of the timestamp capture
 *
 ******************************************************************************/
#define PLP_P1588_TS_HB_SEL_13r    0x000000b5

#define PLP_P1588_TS_HB_SEL_13r_SIZE 4

/*
 * This structure should be used to declare and program P1588_TS_HB_SEL_13.
 *
 */
typedef union PLP_P1588_TS_HB_SEL_13r_s {
	uint32_t v[1];
	uint32_t p1588_ts_hb_sel_13[1];
	uint32_t _p1588_ts_hb_sel_13;
} PLP_P1588_TS_HB_SEL_13r_t;

#define PLP_P1588_TS_HB_SEL_13r_CLR(r) (r).p1588_ts_hb_sel_13[0] = 0
#define PLP_P1588_TS_HB_SEL_13r_SET(r,d) (r).p1588_ts_hb_sel_13[0] = d
#define PLP_P1588_TS_HB_SEL_13r_GET(r) (r).p1588_ts_hb_sel_13[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TS_HB_SEL_13r_SOP_TS_63_48f_GET(r) (((r).p1588_ts_hb_sel_13[0]) & 0xffff)
#define PLP_P1588_TS_HB_SEL_13r_SOP_TS_63_48f_SET(r,f) (r).p1588_ts_hb_sel_13[0]=(((r).p1588_ts_hb_sel_13[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TS_HB_SEL_13.
 *
 */
#define PLP_READ_P1588_TS_HB_SEL_13r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TS_HB_SEL_13r,(_r._p1588_ts_hb_sel_13))
#define PLP_WRITE_P1588_TS_HB_SEL_13r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TS_HB_SEL_13r,(_r._p1588_ts_hb_sel_13))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TS_HB_SEL_13r PLP_P1588_TS_HB_SEL_13r
#define P1588_TS_HB_SEL_13r_SIZE PLP_P1588_TS_HB_SEL_13r_SIZE
typedef PLP_P1588_TS_HB_SEL_13r_t P1588_TS_HB_SEL_13r_t;
#define P1588_TS_HB_SEL_13r_CLR PLP_P1588_TS_HB_SEL_13r_CLR
#define P1588_TS_HB_SEL_13r_SET PLP_P1588_TS_HB_SEL_13r_SET
#define P1588_TS_HB_SEL_13r_GET PLP_P1588_TS_HB_SEL_13r_GET
#define P1588_TS_HB_SEL_13r_SOP_TS_63_48f_GET PLP_P1588_TS_HB_SEL_13r_SOP_TS_63_48f_GET
#define P1588_TS_HB_SEL_13r_SOP_TS_63_48f_SET PLP_P1588_TS_HB_SEL_13r_SOP_TS_63_48f_SET
#define READ_P1588_TS_HB_SEL_13r PLP_READ_P1588_TS_HB_SEL_13r
#define WRITE_P1588_TS_HB_SEL_13r PLP_WRITE_P1588_TS_HB_SEL_13r
#define MODIFY_P1588_TS_HB_SEL_13r PLP_MODIFY_P1588_TS_HB_SEL_13r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TS_HB_SEL_13r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TS_HB_SEL_14
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70b6
 * DESC:     TS HB SEL 14 Register
 * RESETVAL: 0x10 (16)
 * ACCESS:   R/W
 * FIELDS:
 *     FIFO_LEVEL       No of TS FIFO entries that have been written to trigger the packet timestamp interruptMin value is 0 ie more than 1 entry will trigger the packet timestamp interruptMax value is 32 ie when the FIFO is full
 *
 ******************************************************************************/
#define PLP_P1588_TS_HB_SEL_14r    0x000000b6

#define PLP_P1588_TS_HB_SEL_14r_SIZE 4

/*
 * This structure should be used to declare and program P1588_TS_HB_SEL_14.
 *
 */
typedef union PLP_P1588_TS_HB_SEL_14r_s {
	uint32_t v[1];
	uint32_t p1588_ts_hb_sel_14[1];
	uint32_t _p1588_ts_hb_sel_14;
} PLP_P1588_TS_HB_SEL_14r_t;

#define PLP_P1588_TS_HB_SEL_14r_CLR(r) (r).p1588_ts_hb_sel_14[0] = 0
#define PLP_P1588_TS_HB_SEL_14r_SET(r,d) (r).p1588_ts_hb_sel_14[0] = d
#define PLP_P1588_TS_HB_SEL_14r_GET(r) (r).p1588_ts_hb_sel_14[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TS_HB_SEL_14r_FIFO_LEVELf_GET(r) (((r).p1588_ts_hb_sel_14[0]) & 0x3f)
#define PLP_P1588_TS_HB_SEL_14r_FIFO_LEVELf_SET(r,f) (r).p1588_ts_hb_sel_14[0]=(((r).p1588_ts_hb_sel_14[0] & ~((uint32_t)0x3f)) | (((uint32_t)f) & 0x3f)) | (0x3f << 16)

/*
 * These macros can be used to access P1588_TS_HB_SEL_14.
 *
 */
#define PLP_READ_P1588_TS_HB_SEL_14r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TS_HB_SEL_14r,(_r._p1588_ts_hb_sel_14))
#define PLP_WRITE_P1588_TS_HB_SEL_14r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TS_HB_SEL_14r,(_r._p1588_ts_hb_sel_14))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TS_HB_SEL_14r PLP_P1588_TS_HB_SEL_14r
#define P1588_TS_HB_SEL_14r_SIZE PLP_P1588_TS_HB_SEL_14r_SIZE
typedef PLP_P1588_TS_HB_SEL_14r_t P1588_TS_HB_SEL_14r_t;
#define P1588_TS_HB_SEL_14r_CLR PLP_P1588_TS_HB_SEL_14r_CLR
#define P1588_TS_HB_SEL_14r_SET PLP_P1588_TS_HB_SEL_14r_SET
#define P1588_TS_HB_SEL_14r_GET PLP_P1588_TS_HB_SEL_14r_GET
#define P1588_TS_HB_SEL_14r_FIFO_LEVELf_GET PLP_P1588_TS_HB_SEL_14r_FIFO_LEVELf_GET
#define P1588_TS_HB_SEL_14r_FIFO_LEVELf_SET PLP_P1588_TS_HB_SEL_14r_FIFO_LEVELf_SET
#define READ_P1588_TS_HB_SEL_14r PLP_READ_P1588_TS_HB_SEL_14r
#define WRITE_P1588_TS_HB_SEL_14r PLP_WRITE_P1588_TS_HB_SEL_14r
#define MODIFY_P1588_TS_HB_SEL_14r PLP_MODIFY_P1588_TS_HB_SEL_14r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TS_HB_SEL_14r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_INBAND_TX_CONTROL1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70b7
 * DESC:     inband tx control1 Register
 * RESETVAL: 0x2b3b (11067)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_RESV0_ID      Identifier for BRCM 1588 architecture using inband timestamp operation method.
 *     TX_INBAND_ON     1 = enable inband when check_resv0_id is low.
 *     TX_CHECK_RESV0_ID 1 = enable checking tx_resv0_id with reserve id from message.
 *     TX_UPDATE_RESV0_ID Not Applicable on TX.  Ignore
 *     TX_MODE_TC       1 = set this node as TC node.
 *     TX_MODE_PARTIAL_TC 1 = compute CF with RXSOP from reserved field.
 *     TX_TS_USE_80BITS function is disabled. support only ts80 bit
 *     TX_MDIO_SOPMEM_OPTION 1 = revert back to gen1 mode.
 *     TX_INBAND_STRICT_OPTION 1 = strict mode. packet action based on inband command only0 = packet action based on IEEE Standard with inband feature
 *     TX_INBAND_TS32_FORMAT 1 = inband using 32 bit format {2-bit sec, 30-bit ns}.
 *     TX_INBAND_CLEAR_RSV012_RG 1 = clear all 3 rsv fields  0= clear only rsv 0 and 1.
 *     TX_INBAND_SPARE  spare
 *
 ******************************************************************************/
#define PLP_P1588_INBAND_TX_CONTROL1r    0x000000b7

#define PLP_P1588_INBAND_TX_CONTROL1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_INBAND_TX_CONTROL1.
 *
 */
typedef union PLP_P1588_INBAND_TX_CONTROL1r_s {
	uint32_t v[1];
	uint32_t p1588_inband_tx_control1[1];
	uint32_t _p1588_inband_tx_control1;
} PLP_P1588_INBAND_TX_CONTROL1r_t;

#define PLP_P1588_INBAND_TX_CONTROL1r_CLR(r) (r).p1588_inband_tx_control1[0] = 0
#define PLP_P1588_INBAND_TX_CONTROL1r_SET(r,d) (r).p1588_inband_tx_control1[0] = d
#define PLP_P1588_INBAND_TX_CONTROL1r_GET(r) (r).p1588_inband_tx_control1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_INBAND_SPAREf_GET(r) ((((r).p1588_inband_tx_control1[0]) >> 14) & 0x3)
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_INBAND_SPAREf_SET(r,f) (r).p1588_inband_tx_control1[0]=(((r).p1588_inband_tx_control1[0] & ~((uint32_t)0x3 << 14)) | ((((uint32_t)f) & 0x3) << 14)) | (3 << (16 + 14))
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_INBAND_INSERT_4BYTESf_GET(r) ((((r).p1588_inband_tx_control1[0]) >> 13) & 0x1)
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_INBAND_INSERT_4BYTESf_SET(r,f) (r).p1588_inband_tx_control1[0]=(((r).p1588_inband_tx_control1[0] & ~((uint32_t)0x1 << 13)) | ((((uint32_t)f) & 0x1) << 13)) | (1 << (16 + 13))
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_INBAND_CLEAR_RSV012_RGf_GET(r) ((((r).p1588_inband_tx_control1[0]) >> 13) & 0x1)
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_INBAND_CLEAR_RSV012_RGf_SET(r,f) (r).p1588_inband_tx_control1[0]=(((r).p1588_inband_tx_control1[0] & ~((uint32_t)0x1 << 13)) | ((((uint32_t)f) & 0x1) << 13)) | (1 << (16 + 13))
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_INBAND_TS32_FORMATf_GET(r) ((((r).p1588_inband_tx_control1[0]) >> 12) & 0x1)
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_INBAND_TS32_FORMATf_SET(r,f) (r).p1588_inband_tx_control1[0]=(((r).p1588_inband_tx_control1[0] & ~((uint32_t)0x1 << 12)) | ((((uint32_t)f) & 0x1) << 12)) | (1 << (16 + 12))
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_INBAND_STRICT_OPTIONf_GET(r) ((((r).p1588_inband_tx_control1[0]) >> 11) & 0x1)
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_INBAND_STRICT_OPTIONf_SET(r,f) (r).p1588_inband_tx_control1[0]=(((r).p1588_inband_tx_control1[0] & ~((uint32_t)0x1 << 11)) | ((((uint32_t)f) & 0x1) << 11)) | (1 << (16 + 11))
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_MDIO_SOPMEM_OPTIONf_GET(r) ((((r).p1588_inband_tx_control1[0]) >> 10) & 0x1)
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_MDIO_SOPMEM_OPTIONf_SET(r,f) (r).p1588_inband_tx_control1[0]=(((r).p1588_inband_tx_control1[0] & ~((uint32_t)0x1 << 10)) | ((((uint32_t)f) & 0x1) << 10)) | (1 << (16 + 10))
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_TS_USE_80BITSf_GET(r) ((((r).p1588_inband_tx_control1[0]) >> 9) & 0x1)
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_TS_USE_80BITSf_SET(r,f) (r).p1588_inband_tx_control1[0]=(((r).p1588_inband_tx_control1[0] & ~((uint32_t)0x1 << 9)) | ((((uint32_t)f) & 0x1) << 9)) | (1 << (16 + 9))
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_MODE_PARTIAL_TCf_GET(r) ((((r).p1588_inband_tx_control1[0]) >> 8) & 0x1)
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_MODE_PARTIAL_TCf_SET(r,f) (r).p1588_inband_tx_control1[0]=(((r).p1588_inband_tx_control1[0] & ~((uint32_t)0x1 << 8)) | ((((uint32_t)f) & 0x1) << 8)) | (1 << (16 + 8))
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_MODE_TCf_GET(r) ((((r).p1588_inband_tx_control1[0]) >> 7) & 0x1)
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_MODE_TCf_SET(r,f) (r).p1588_inband_tx_control1[0]=(((r).p1588_inband_tx_control1[0] & ~((uint32_t)0x1 << 7)) | ((((uint32_t)f) & 0x1) << 7)) | (1 << (16 + 7))
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_UPDATE_RESV0_IDf_GET(r) ((((r).p1588_inband_tx_control1[0]) >> 6) & 0x1)
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_UPDATE_RESV0_IDf_SET(r,f) (r).p1588_inband_tx_control1[0]=(((r).p1588_inband_tx_control1[0] & ~((uint32_t)0x1 << 6)) | ((((uint32_t)f) & 0x1) << 6)) | (1 << (16 + 6))
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_CHECK_RESV0_IDf_GET(r) ((((r).p1588_inband_tx_control1[0]) >> 5) & 0x1)
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_CHECK_RESV0_IDf_SET(r,f) (r).p1588_inband_tx_control1[0]=(((r).p1588_inband_tx_control1[0] & ~((uint32_t)0x1 << 5)) | ((((uint32_t)f) & 0x1) << 5)) | (1 << (16 + 5))
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_INBAND_ONf_GET(r) ((((r).p1588_inband_tx_control1[0]) >> 4) & 0x1)
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_INBAND_ONf_SET(r,f) (r).p1588_inband_tx_control1[0]=(((r).p1588_inband_tx_control1[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4)) | (1 << (16 + 4))
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_RESV0_IDf_GET(r) (((r).p1588_inband_tx_control1[0]) & 0xf)
#define PLP_P1588_INBAND_TX_CONTROL1r_TX_RESV0_IDf_SET(r,f) (r).p1588_inband_tx_control1[0]=(((r).p1588_inband_tx_control1[0] & ~((uint32_t)0xf)) | (((uint32_t)f) & 0xf)) | (0xf << 16)

/*
 * These macros can be used to access P1588_INBAND_TX_CONTROL1.
 *
 */
#define PLP_READ_P1588_INBAND_TX_CONTROL1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_INBAND_TX_CONTROL1r,(_r._p1588_inband_tx_control1))
#define PLP_WRITE_P1588_INBAND_TX_CONTROL1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_INBAND_TX_CONTROL1r,(_r._p1588_inband_tx_control1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_INBAND_TX_CONTROL1r PLP_P1588_INBAND_TX_CONTROL1r
#define P1588_INBAND_TX_CONTROL1r_SIZE PLP_P1588_INBAND_TX_CONTROL1r_SIZE
typedef PLP_P1588_INBAND_TX_CONTROL1r_t P1588_INBAND_TX_CONTROL1r_t;
#define P1588_INBAND_TX_CONTROL1r_CLR PLP_P1588_INBAND_TX_CONTROL1r_CLR
#define P1588_INBAND_TX_CONTROL1r_SET PLP_P1588_INBAND_TX_CONTROL1r_SET
#define P1588_INBAND_TX_CONTROL1r_GET PLP_P1588_INBAND_TX_CONTROL1r_GET
#define P1588_INBAND_TX_CONTROL1r_TX_INBAND_SPAREf_GET PLP_P1588_INBAND_TX_CONTROL1r_TX_INBAND_SPAREf_GET
#define P1588_INBAND_TX_CONTROL1r_TX_INBAND_SPAREf_SET PLP_P1588_INBAND_TX_CONTROL1r_TX_INBAND_SPAREf_SET
#define P1588_INBAND_TX_CONTROL1r_TX_INBAND_CLEAR_RSV012_RGf_GET PLP_P1588_INBAND_TX_CONTROL1r_TX_INBAND_CLEAR_RSV012_RGf_GET
#define P1588_INBAND_TX_CONTROL1r_TX_INBAND_CLEAR_RSV012_RGf_SET PLP_P1588_INBAND_TX_CONTROL1r_TX_INBAND_CLEAR_RSV012_RGf_SET
#define P1588_INBAND_TX_CONTROL1r_TX_INBAND_TS32_FORMATf_GET PLP_P1588_INBAND_TX_CONTROL1r_TX_INBAND_TS32_FORMATf_GET
#define P1588_INBAND_TX_CONTROL1r_TX_INBAND_TS32_FORMATf_SET PLP_P1588_INBAND_TX_CONTROL1r_TX_INBAND_TS32_FORMATf_SET
#define P1588_INBAND_TX_CONTROL1r_TX_INBAND_STRICT_OPTIONf_GET PLP_P1588_INBAND_TX_CONTROL1r_TX_INBAND_STRICT_OPTIONf_GET
#define P1588_INBAND_TX_CONTROL1r_TX_INBAND_STRICT_OPTIONf_SET PLP_P1588_INBAND_TX_CONTROL1r_TX_INBAND_STRICT_OPTIONf_SET
#define P1588_INBAND_TX_CONTROL1r_TX_MDIO_SOPMEM_OPTIONf_GET PLP_P1588_INBAND_TX_CONTROL1r_TX_MDIO_SOPMEM_OPTIONf_GET
#define P1588_INBAND_TX_CONTROL1r_TX_MDIO_SOPMEM_OPTIONf_SET PLP_P1588_INBAND_TX_CONTROL1r_TX_MDIO_SOPMEM_OPTIONf_SET
#define P1588_INBAND_TX_CONTROL1r_TX_TS_USE_80BITSf_GET PLP_P1588_INBAND_TX_CONTROL1r_TX_TS_USE_80BITSf_GET
#define P1588_INBAND_TX_CONTROL1r_TX_TS_USE_80BITSf_SET PLP_P1588_INBAND_TX_CONTROL1r_TX_TS_USE_80BITSf_SET
#define P1588_INBAND_TX_CONTROL1r_TX_MODE_PARTIAL_TCf_GET PLP_P1588_INBAND_TX_CONTROL1r_TX_MODE_PARTIAL_TCf_GET
#define P1588_INBAND_TX_CONTROL1r_TX_MODE_PARTIAL_TCf_SET PLP_P1588_INBAND_TX_CONTROL1r_TX_MODE_PARTIAL_TCf_SET
#define P1588_INBAND_TX_CONTROL1r_TX_MODE_TCf_GET PLP_P1588_INBAND_TX_CONTROL1r_TX_MODE_TCf_GET
#define P1588_INBAND_TX_CONTROL1r_TX_MODE_TCf_SET PLP_P1588_INBAND_TX_CONTROL1r_TX_MODE_TCf_SET
#define P1588_INBAND_TX_CONTROL1r_TX_UPDATE_RESV0_IDf_GET PLP_P1588_INBAND_TX_CONTROL1r_TX_UPDATE_RESV0_IDf_GET
#define P1588_INBAND_TX_CONTROL1r_TX_UPDATE_RESV0_IDf_SET PLP_P1588_INBAND_TX_CONTROL1r_TX_UPDATE_RESV0_IDf_SET
#define P1588_INBAND_TX_CONTROL1r_TX_CHECK_RESV0_IDf_GET PLP_P1588_INBAND_TX_CONTROL1r_TX_CHECK_RESV0_IDf_GET
#define P1588_INBAND_TX_CONTROL1r_TX_CHECK_RESV0_IDf_SET PLP_P1588_INBAND_TX_CONTROL1r_TX_CHECK_RESV0_IDf_SET
#define P1588_INBAND_TX_CONTROL1r_TX_INBAND_ONf_GET PLP_P1588_INBAND_TX_CONTROL1r_TX_INBAND_ONf_GET
#define P1588_INBAND_TX_CONTROL1r_TX_INBAND_ONf_SET PLP_P1588_INBAND_TX_CONTROL1r_TX_INBAND_ONf_SET
#define P1588_INBAND_TX_CONTROL1r_TX_RESV0_IDf_GET PLP_P1588_INBAND_TX_CONTROL1r_TX_RESV0_IDf_GET
#define P1588_INBAND_TX_CONTROL1r_TX_RESV0_IDf_SET PLP_P1588_INBAND_TX_CONTROL1r_TX_RESV0_IDf_SET
#define READ_P1588_INBAND_TX_CONTROL1r PLP_READ_P1588_INBAND_TX_CONTROL1r
#define WRITE_P1588_INBAND_TX_CONTROL1r PLP_WRITE_P1588_INBAND_TX_CONTROL1r
#define MODIFY_P1588_INBAND_TX_CONTROL1r PLP_MODIFY_P1588_INBAND_TX_CONTROL1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_INBAND_TX_CONTROL1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_INBAND_TX_CONTROL2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70b8
 * DESC:     inband tx control2 Register
 * RESETVAL: 0x702 (1794)
 * ACCESS:   R/W
 * FIELDS:
 *     INBAND_TX_CONTROL2 bit 0 : 1= select parser_source_port[79:16] to write to SOPMEMbit 1 : 1= select parser_mac_da to write to SOPMEMbit 2 : 1= selectparser_mac_sa to write to SOPMEMbit 3 : 1= selectparser_ipv4_dstIP to write to SOPMEMbit 4 : 1= select parser_ipv4_srcIP to write to SOPMEMbit 8 : 1= enable compare domain_num.  default: check-onbit 9 : 1= enable compare check seq_num.  default: check-onbit 10 : 1= enable compare check source_port.  default: check-onbit 11 : 1= enable compare field select in bit [4:0] abovebit 14:12 select which field to compare when read SOPmem3'b001 = mac_da3'b010 = mac_sa3'b011 = ivp4_dstIP3'b100 = ivp4_srcIPbit 15 : 1= enable compare vlan id
 *
 ******************************************************************************/
#define PLP_P1588_INBAND_TX_CONTROL2r    0x000000b8

#define PLP_P1588_INBAND_TX_CONTROL2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_INBAND_TX_CONTROL2.
 *
 */
typedef union PLP_P1588_INBAND_TX_CONTROL2r_s {
	uint32_t v[1];
	uint32_t p1588_inband_tx_control2[1];
	uint32_t _p1588_inband_tx_control2;
} PLP_P1588_INBAND_TX_CONTROL2r_t;

#define PLP_P1588_INBAND_TX_CONTROL2r_CLR(r) (r).p1588_inband_tx_control2[0] = 0
#define PLP_P1588_INBAND_TX_CONTROL2r_SET(r,d) (r).p1588_inband_tx_control2[0] = d
#define PLP_P1588_INBAND_TX_CONTROL2r_GET(r) (r).p1588_inband_tx_control2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_INBAND_TX_CONTROL2r_INBAND_TX_CONTROL2f_GET(r) (((r).p1588_inband_tx_control2[0]) & 0xffff)
#define PLP_P1588_INBAND_TX_CONTROL2r_INBAND_TX_CONTROL2f_SET(r,f) (r).p1588_inband_tx_control2[0]=(((r).p1588_inband_tx_control2[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

#define P1588_INBAND_TX_CONTROL2r_CMP_VLAN_IDf_GET(r) ((((r).p1588_inband_tx_control2[0]) >> 15) & 0x1)
#define P1588_INBAND_TX_CONTROL2r_CMP_VLAN_IDf_SET(r,f) (r).p1588_inband_tx_control2[0]=(((r).p1588_inband_tx_control2[0] & ~((uint32_t)0x1 << 15)) | ((((uint32_t)f) & 0x1) << 15)) | (1 << (16 + 15))
#define P1588_INBAND_TX_CONTROL2r_CMP_SOPMEMf_GET(r) ((((r).p1588_inband_tx_control2[0]) >> 12) & 0x7)
#define P1588_INBAND_TX_CONTROL2r_CMP_SOPMEMf_SET(r,f) (r).p1588_inband_tx_control2[0]=(((r).p1588_inband_tx_control2[0] & ~((uint32_t)0x7 << 12)) | ((((uint32_t)f) & 0x7) << 12)) | (7 << (16 + 12))
#define P1588_INBAND_TX_CONTROL2r_CMP_FIELD_SELf_GET(r) ((((r).p1588_inband_tx_control2[0]) >> 11) & 0x1)
#define P1588_INBAND_TX_CONTROL2r_CMP_FIELD_SELf_SET(r,f) (r).p1588_inband_tx_control2[0]=(((r).p1588_inband_tx_control2[0] & ~((uint32_t)0x1 << 11)) | ((((uint32_t)f) & 0x1) << 11)) | (1 << (16 + 11))
#define P1588_INBAND_TX_CONTROL2r_CMP_SRC_PORTf_GET(r) ((((r).p1588_inband_tx_control2[0]) >> 10) & 0x1)
#define P1588_INBAND_TX_CONTROL2r_CMP_SRC_PORTf_SET(r,f) (r).p1588_inband_tx_control2[0]=(((r).p1588_inband_tx_control2[0] & ~((uint32_t)0x1 << 10)) | ((((uint32_t)f) & 0x1) << 10)) | (1 << (16 + 10))
#define P1588_INBAND_TX_CONTROL2r_CMP_SEQ_NUMf_GET(r) ((((r).p1588_inband_tx_control2[0]) >> 9) & 0x1)
#define P1588_INBAND_TX_CONTROL2r_CMP_SEQ_NUMf_GSET(r,f) (r).p1588_inband_tx_control2[0]=(((r).p1588_inband_tx_control2[0] & ~((uint32_t)0x1 << 9)) | ((((uint32_t)f) & 0x1) << 9)) | (1 << (16 + 9))
#define P1588_INBAND_TX_CONTROL2r_CMP_DOMAIN_NUMf_GET(r) ((((r).p1588_inband_tx_control2[0]) >> 8) & 0x1)
#define P1588_INBAND_TX_CONTROL2r_CMP_DOMAIN_NUMf_SET(r,f) (r).p1588_inband_tx_control2[0]=(((r).p1588_inband_tx_control2[0] & ~((uint32_t)0x1 << 8)) | ((((uint32_t)f) & 0x1) << 8)) | (1 << (16 + 8))
#define P1588_INBAND_TX_CONTROL2r_WRITE_SOPMEMf_GET(r) ((((r).p1588_inband_tx_control2[0]) >> 0) & CAP_SEL_MASK)
#define P1588_INBAND_TX_CONTROL2r_WRITE_SOPMEMf_SET(r,f) (r).p1588_inband_tx_control2[0]=(((r).p1588_inband_tx_control2[0] & ~((uint32_t)CAP_SEL_MASK << 0)) | ((((uint32_t)f) & CAP_SEL_MASK) << 0)) | (CAP_SEL_MASK << (16 + 0))

/*
 * These macros can be used to access P1588_INBAND_TX_CONTROL2.
 *
 */
#define PLP_READ_P1588_INBAND_TX_CONTROL2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_INBAND_TX_CONTROL2r,(_r._p1588_inband_tx_control2))
#define PLP_WRITE_P1588_INBAND_TX_CONTROL2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_INBAND_TX_CONTROL2r,(_r._p1588_inband_tx_control2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_INBAND_TX_CONTROL2r PLP_P1588_INBAND_TX_CONTROL2r
#define P1588_INBAND_TX_CONTROL2r_SIZE PLP_P1588_INBAND_TX_CONTROL2r_SIZE
typedef PLP_P1588_INBAND_TX_CONTROL2r_t P1588_INBAND_TX_CONTROL2r_t;
#define P1588_INBAND_TX_CONTROL2r_CLR PLP_P1588_INBAND_TX_CONTROL2r_CLR
#define P1588_INBAND_TX_CONTROL2r_SET PLP_P1588_INBAND_TX_CONTROL2r_SET
#define P1588_INBAND_TX_CONTROL2r_GET PLP_P1588_INBAND_TX_CONTROL2r_GET
#define P1588_INBAND_TX_CONTROL2r_INBAND_TX_CONTROL2f_GET PLP_P1588_INBAND_TX_CONTROL2r_INBAND_TX_CONTROL2f_GET
#define P1588_INBAND_TX_CONTROL2r_INBAND_TX_CONTROL2f_SET PLP_P1588_INBAND_TX_CONTROL2r_INBAND_TX_CONTROL2f_SET
#define READ_P1588_INBAND_TX_CONTROL2r PLP_READ_P1588_INBAND_TX_CONTROL2r
#define WRITE_P1588_INBAND_TX_CONTROL2r PLP_WRITE_P1588_INBAND_TX_CONTROL2r
#define MODIFY_P1588_INBAND_TX_CONTROL2r PLP_MODIFY_P1588_INBAND_TX_CONTROL2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_INBAND_TX_CONTROL2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_INBAND_RX_CONTROL1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70b9
 * DESC:     inband rx control1 Register
 * RESETVAL: 0xb7b (2939)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_RESV0_ID      Identifier for BRCM 1588 architecture using inband timestamp operation method.
 *     RX_INBAND_ON     1 = enable inband when check_resv0_id is low.
 *     RX_CHECK_RESV0_ID Not applicable on RX.  Ignore
 *     RX_UPDATE_RESV0_ID 1 = update rx_resv0_id into reseverved field 0 of RX.
 *     RX_MODE_TC       1 = set this node as TC node.
 *     RX_MODE_PARTIAL_TC 1 = compute CF with RXSOP from reserved field.
 *     RX_TS_USE_80BITS function is disable.  support only ts80 bit.
 *     RX_MDIO_SOPMEM_OPTION 1 = revert back to gen1 mode.
 *     RX_INBAND_STRICT_OPTION 1 = strict mode. packet action based on inband command only0 = packet action based on IEEE Standard with inband feature
 *     RX_INBAND_TS32_FORMAT 1 = inband using 32 bit format {2-bit sec, 30-bit ns}.
 *     RX_INBAND_INSERT_4BYTES 1 =  insert {2-bit sec, 30-bit ns}. after 1588 header
 *     RX_INBAND_SPARE_RG spare bits.
 *
 ******************************************************************************/
#define PLP_P1588_INBAND_RX_CONTROL1r    0x000000b9

#define PLP_P1588_INBAND_RX_CONTROL1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_INBAND_RX_CONTROL1.
 *
 */
typedef union PLP_P1588_INBAND_RX_CONTROL1r_s {
	uint32_t v[1];
	uint32_t p1588_inband_rx_control1[1];
	uint32_t _p1588_inband_rx_control1;
} PLP_P1588_INBAND_RX_CONTROL1r_t;

#define PLP_P1588_INBAND_RX_CONTROL1r_CLR(r) (r).p1588_inband_rx_control1[0] = 0
#define PLP_P1588_INBAND_RX_CONTROL1r_SET(r,d) (r).p1588_inband_rx_control1[0] = d
#define PLP_P1588_INBAND_RX_CONTROL1r_GET(r) (r).p1588_inband_rx_control1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_INBAND_SPARE_RGf_GET(r) ((((r).p1588_inband_rx_control1[0]) >> 14) & 0x3)
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_INBAND_SPARE_RGf_SET(r,f) (r).p1588_inband_rx_control1[0]=(((r).p1588_inband_rx_control1[0] & ~((uint32_t)0x3 << 14)) | ((((uint32_t)f) & 0x3) << 14)) | (3 << (16 + 14))
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_INBAND_INSERT_4BYTESf_GET(r) ((((r).p1588_inband_rx_control1[0]) >> 13) & 0x1)
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_INBAND_INSERT_4BYTESf_SET(r,f) (r).p1588_inband_rx_control1[0]=(((r).p1588_inband_rx_control1[0] & ~((uint32_t)0x1 << 13)) | ((((uint32_t)f) & 0x1) << 13)) | (1 << (16 + 13))
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_INBAND_TS32_FORMATf_GET(r) ((((r).p1588_inband_rx_control1[0]) >> 12) & 0x1)
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_INBAND_TS32_FORMATf_SET(r,f) (r).p1588_inband_rx_control1[0]=(((r).p1588_inband_rx_control1[0] & ~((uint32_t)0x1 << 12)) | ((((uint32_t)f) & 0x1) << 12)) | (1 << (16 + 12))
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_INBAND_STRICT_OPTIONf_GET(r) ((((r).p1588_inband_rx_control1[0]) >> 11) & 0x1)
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_INBAND_STRICT_OPTIONf_SET(r,f) (r).p1588_inband_rx_control1[0]=(((r).p1588_inband_rx_control1[0] & ~((uint32_t)0x1 << 11)) | ((((uint32_t)f) & 0x1) << 11)) | (1 << (16 + 11))
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_MDIO_SOPMEM_OPTIONf_GET(r) ((((r).p1588_inband_rx_control1[0]) >> 10) & 0x1)
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_MDIO_SOPMEM_OPTIONf_SET(r,f) (r).p1588_inband_rx_control1[0]=(((r).p1588_inband_rx_control1[0] & ~((uint32_t)0x1 << 10)) | ((((uint32_t)f) & 0x1) << 10)) | (1 << (16 + 10))
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_TS_USE_80BITSf_GET(r) ((((r).p1588_inband_rx_control1[0]) >> 9) & 0x1)
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_TS_USE_80BITSf_SET(r,f) (r).p1588_inband_rx_control1[0]=(((r).p1588_inband_rx_control1[0] & ~((uint32_t)0x1 << 9)) | ((((uint32_t)f) & 0x1) << 9)) | (1 << (16 + 9))
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_MODE_PARTIAL_TCf_GET(r) ((((r).p1588_inband_rx_control1[0]) >> 8) & 0x1)
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_MODE_PARTIAL_TCf_SET(r,f) (r).p1588_inband_rx_control1[0]=(((r).p1588_inband_rx_control1[0] & ~((uint32_t)0x1 << 8)) | ((((uint32_t)f) & 0x1) << 8)) | (1 << (16 + 8))
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_MODE_TCf_GET(r) ((((r).p1588_inband_rx_control1[0]) >> 7) & 0x1)
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_MODE_TCf_SET(r,f) (r).p1588_inband_rx_control1[0]=(((r).p1588_inband_rx_control1[0] & ~((uint32_t)0x1 << 7)) | ((((uint32_t)f) & 0x1) << 7)) | (1 << (16 + 7))
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_UPDATE_RESV0_IDf_GET(r) ((((r).p1588_inband_rx_control1[0]) >> 6) & 0x1)
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_UPDATE_RESV0_IDf_SET(r,f) (r).p1588_inband_rx_control1[0]=(((r).p1588_inband_rx_control1[0] & ~((uint32_t)0x1 << 6)) | ((((uint32_t)f) & 0x1) << 6)) | (1 << (16 + 6))
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_CHECK_RESV0_IDf_GET(r) ((((r).p1588_inband_rx_control1[0]) >> 5) & 0x1)
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_CHECK_RESV0_IDf_SET(r,f) (r).p1588_inband_rx_control1[0]=(((r).p1588_inband_rx_control1[0] & ~((uint32_t)0x1 << 5)) | ((((uint32_t)f) & 0x1) << 5)) | (1 << (16 + 5))
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_INBAND_ONf_GET(r) ((((r).p1588_inband_rx_control1[0]) >> 4) & 0x1)
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_INBAND_ONf_SET(r,f) (r).p1588_inband_rx_control1[0]=(((r).p1588_inband_rx_control1[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4)) | (1 << (16 + 4))
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_RESV0_IDf_GET(r) (((r).p1588_inband_rx_control1[0]) & 0xf)
#define PLP_P1588_INBAND_RX_CONTROL1r_RX_RESV0_IDf_SET(r,f) (r).p1588_inband_rx_control1[0]=(((r).p1588_inband_rx_control1[0] & ~((uint32_t)0xf)) | (((uint32_t)f) & 0xf)) | (0xf << 16)

/*
 * These macros can be used to access P1588_INBAND_RX_CONTROL1.
 *
 */
#define PLP_READ_P1588_INBAND_RX_CONTROL1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_INBAND_RX_CONTROL1r,(_r._p1588_inband_rx_control1))
#define PLP_WRITE_P1588_INBAND_RX_CONTROL1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_INBAND_RX_CONTROL1r,(_r._p1588_inband_rx_control1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_INBAND_RX_CONTROL1r PLP_P1588_INBAND_RX_CONTROL1r
#define P1588_INBAND_RX_CONTROL1r_SIZE PLP_P1588_INBAND_RX_CONTROL1r_SIZE
typedef PLP_P1588_INBAND_RX_CONTROL1r_t P1588_INBAND_RX_CONTROL1r_t;
#define P1588_INBAND_RX_CONTROL1r_CLR PLP_P1588_INBAND_RX_CONTROL1r_CLR
#define P1588_INBAND_RX_CONTROL1r_SET PLP_P1588_INBAND_RX_CONTROL1r_SET
#define P1588_INBAND_RX_CONTROL1r_GET PLP_P1588_INBAND_RX_CONTROL1r_GET
#define P1588_INBAND_RX_CONTROL1r_RX_INBAND_SPARE_RGf_GET PLP_P1588_INBAND_RX_CONTROL1r_RX_INBAND_SPARE_RGf_GET
#define P1588_INBAND_RX_CONTROL1r_RX_INBAND_SPARE_RGf_SET PLP_P1588_INBAND_RX_CONTROL1r_RX_INBAND_SPARE_RGf_SET
#define P1588_INBAND_RX_CONTROL1r_RX_INBAND_INSERT_4BYTESf_GET PLP_P1588_INBAND_RX_CONTROL1r_RX_INBAND_INSERT_4BYTESf_GET
#define P1588_INBAND_RX_CONTROL1r_RX_INBAND_INSERT_4BYTESf_SET PLP_P1588_INBAND_RX_CONTROL1r_RX_INBAND_INSERT_4BYTESf_SET
#define P1588_INBAND_RX_CONTROL1r_RX_INBAND_TS32_FORMATf_GET PLP_P1588_INBAND_RX_CONTROL1r_RX_INBAND_TS32_FORMATf_GET
#define P1588_INBAND_RX_CONTROL1r_RX_INBAND_TS32_FORMATf_SET PLP_P1588_INBAND_RX_CONTROL1r_RX_INBAND_TS32_FORMATf_SET
#define P1588_INBAND_RX_CONTROL1r_RX_INBAND_STRICT_OPTIONf_GET PLP_P1588_INBAND_RX_CONTROL1r_RX_INBAND_STRICT_OPTIONf_GET
#define P1588_INBAND_RX_CONTROL1r_RX_INBAND_STRICT_OPTIONf_SET PLP_P1588_INBAND_RX_CONTROL1r_RX_INBAND_STRICT_OPTIONf_SET
#define P1588_INBAND_RX_CONTROL1r_RX_MDIO_SOPMEM_OPTIONf_GET PLP_P1588_INBAND_RX_CONTROL1r_RX_MDIO_SOPMEM_OPTIONf_GET
#define P1588_INBAND_RX_CONTROL1r_RX_MDIO_SOPMEM_OPTIONf_SET PLP_P1588_INBAND_RX_CONTROL1r_RX_MDIO_SOPMEM_OPTIONf_SET
#define P1588_INBAND_RX_CONTROL1r_RX_TS_USE_80BITSf_GET PLP_P1588_INBAND_RX_CONTROL1r_RX_TS_USE_80BITSf_GET
#define P1588_INBAND_RX_CONTROL1r_RX_TS_USE_80BITSf_SET PLP_P1588_INBAND_RX_CONTROL1r_RX_TS_USE_80BITSf_SET
#define P1588_INBAND_RX_CONTROL1r_RX_MODE_PARTIAL_TCf_GET PLP_P1588_INBAND_RX_CONTROL1r_RX_MODE_PARTIAL_TCf_GET
#define P1588_INBAND_RX_CONTROL1r_RX_MODE_PARTIAL_TCf_SET PLP_P1588_INBAND_RX_CONTROL1r_RX_MODE_PARTIAL_TCf_SET
#define P1588_INBAND_RX_CONTROL1r_RX_MODE_TCf_GET PLP_P1588_INBAND_RX_CONTROL1r_RX_MODE_TCf_GET
#define P1588_INBAND_RX_CONTROL1r_RX_MODE_TCf_SET PLP_P1588_INBAND_RX_CONTROL1r_RX_MODE_TCf_SET
#define P1588_INBAND_RX_CONTROL1r_RX_UPDATE_RESV0_IDf_GET PLP_P1588_INBAND_RX_CONTROL1r_RX_UPDATE_RESV0_IDf_GET
#define P1588_INBAND_RX_CONTROL1r_RX_UPDATE_RESV0_IDf_SET PLP_P1588_INBAND_RX_CONTROL1r_RX_UPDATE_RESV0_IDf_SET
#define P1588_INBAND_RX_CONTROL1r_RX_CHECK_RESV0_IDf_GET PLP_P1588_INBAND_RX_CONTROL1r_RX_CHECK_RESV0_IDf_GET
#define P1588_INBAND_RX_CONTROL1r_RX_CHECK_RESV0_IDf_SET PLP_P1588_INBAND_RX_CONTROL1r_RX_CHECK_RESV0_IDf_SET
#define P1588_INBAND_RX_CONTROL1r_RX_INBAND_ONf_GET PLP_P1588_INBAND_RX_CONTROL1r_RX_INBAND_ONf_GET
#define P1588_INBAND_RX_CONTROL1r_RX_INBAND_ONf_SET PLP_P1588_INBAND_RX_CONTROL1r_RX_INBAND_ONf_SET
#define P1588_INBAND_RX_CONTROL1r_RX_RESV0_IDf_GET PLP_P1588_INBAND_RX_CONTROL1r_RX_RESV0_IDf_GET
#define P1588_INBAND_RX_CONTROL1r_RX_RESV0_IDf_SET PLP_P1588_INBAND_RX_CONTROL1r_RX_RESV0_IDf_SET
#define READ_P1588_INBAND_RX_CONTROL1r PLP_READ_P1588_INBAND_RX_CONTROL1r
#define WRITE_P1588_INBAND_RX_CONTROL1r PLP_WRITE_P1588_INBAND_RX_CONTROL1r
#define MODIFY_P1588_INBAND_RX_CONTROL1r PLP_MODIFY_P1588_INBAND_RX_CONTROL1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_INBAND_RX_CONTROL1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_INBAND_RX_CONTROL2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70ba
 * DESC:     inband rx control2 Register
 * RESETVAL: 0x702 (1794)
 * ACCESS:   R/W
 * FIELDS:
 *     INBAND_RX_CONTROL2 bit 0 : 1= select parser_source_port[79:16] to write to SOPMEMbit 1 : 1= select parser_mac_da to write to SOPMEMbit 2 : 1= selectparser_mac_sa to write to SOPMEMbit 3 : 1= selectparser_ipv4_dstIP to write to SOPMEMbit 4 : 1= select parser_ipv4_srcIP to write to SOPMEMbit 8 : 1= enable compare domain_num.  default: check-onbit 9 : 1= enable compare check seq_num.  default: check-onbit 10 : 1= enable compare check source_port.  default: check-onbit 11 : 1= enable compare field select in bit [4:0] abovebit 14:12 select which field to compare when read SOPmem3'b001 = mac_da3'b010 = mac_sa3'b011 = ivp4_dstIP3'b100 = ivp4_srcIPbit 15 : 1= enable compare vlan id
 *
 ******************************************************************************/
#define PLP_P1588_INBAND_RX_CONTROL2r    0x000000ba

#define PLP_P1588_INBAND_RX_CONTROL2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_INBAND_RX_CONTROL2.
 *
 */
typedef union PLP_P1588_INBAND_RX_CONTROL2r_s {
	uint32_t v[1];
	uint32_t p1588_inband_rx_control2[1];
	uint32_t _p1588_inband_rx_control2;
} PLP_P1588_INBAND_RX_CONTROL2r_t;

#define PLP_P1588_INBAND_RX_CONTROL2r_CLR(r) (r).p1588_inband_rx_control2[0] = 0
#define PLP_P1588_INBAND_RX_CONTROL2r_SET(r,d) (r).p1588_inband_rx_control2[0] = d
#define PLP_P1588_INBAND_RX_CONTROL2r_GET(r) (r).p1588_inband_rx_control2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_INBAND_RX_CONTROL2r_INBAND_RX_CONTROL2f_GET(r) (((r).p1588_inband_rx_control2[0]) & 0xffff)
#define PLP_P1588_INBAND_RX_CONTROL2r_INBAND_RX_CONTROL2f_SET(r,f) (r).p1588_inband_rx_control2[0]=(((r).p1588_inband_rx_control2[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

#define P1588_INBAND_RX_CONTROL2r_CMP_VLAN_IDf_GET(r) ((((r).p1588_inband_rx_control2[0]) >> 15) & 0x1)
#define P1588_INBAND_RX_CONTROL2r_CMP_VLAN_IDf_SET(r,f) (r).p1588_inband_rx_control2[0]=(((r).p1588_inband_rx_control2[0] & ~((uint32_t)0x1 << 15)) | ((((uint32_t)f) & 0x1) << 15)) | (1 << (16 + 15))
#define P1588_INBAND_RX_CONTROL2r_CMP_SOPMEMf_GET(r) ((((r).p1588_inband_rx_control2[0]) >> 12) & 0x7)
#define P1588_INBAND_RX_CONTROL2r_CMP_SOPMEMf_SET(r,f) (r).p1588_inband_rx_control2[0]=(((r).p1588_inband_rx_control2[0] & ~((uint32_t)0x7 << 12)) | ((((uint32_t)f) & 0x7) << 12)) | (7 << (16 + 12))
#define P1588_INBAND_RX_CONTROL2r_CMP_FIELD_SELf_GET(r) ((((r).p1588_inband_rx_control2[0]) >> 11) & 0x1)
#define P1588_INBAND_RX_CONTROL2r_CMP_FIELD_SELf_SET(r,f) (r).p1588_inband_rx_control2[0]=(((r).p1588_inband_rx_control2[0] & ~((uint32_t)0x1 << 11)) | ((((uint32_t)f) & 0x1) << 11)) | (1 << (16 + 11))
#define P1588_INBAND_RX_CONTROL2r_CMP_SRC_PORTf_GET(r) ((((r).p1588_inband_rx_control2[0]) >> 10) & 0x1)
#define P1588_INBAND_RX_CONTROL2r_CMP_SRC_PORTf_SET(r,f) (r).p1588_inband_rx_control2[0]=(((r).p1588_inband_rx_control2[0] & ~((uint32_t)0x1 << 10)) | ((((uint32_t)f) & 0x1) << 10)) | (1 << (16 + 10))
#define P1588_INBAND_RX_CONTROL2r_CMP_SEQ_NUMf_GET(r) ((((r).p1588_inband_rx_control2[0]) >> 9) & 0x1)
#define P1588_INBAND_RX_CONTROL2r_CMP_SEQ_NUMf_GSET(r,f) (r).p1588_inband_rx_control2[0]=(((r).p1588_inband_rx_control2[0] & ~((uint32_t)0x1 << 9)) | ((((uint32_t)f) & 0x1) << 9)) | (1 << (16 + 9))
#define P1588_INBAND_RX_CONTROL2r_CMP_DOMAIN_NUMf_GET(r) ((((r).p1588_inband_rx_control2[0]) >> 8) & 0x1)
#define P1588_INBAND_RX_CONTROL2r_CMP_DOMAIN_NUMf_SET(r,f) (r).p1588_inband_rx_control2[0]=(((r).p1588_inband_rx_control2[0] & ~((uint32_t)0x1 << 8)) | ((((uint32_t)f) & 0x1) << 8)) | (1 << (16 + 8))
#define P1588_INBAND_RX_CONTROL2r_WRITE_SOPMEMf_GET(r) ((((r).p1588_inband_rx_control2[0]) >> 0) & CAP_SEL_MASK)
#define P1588_INBAND_RX_CONTROL2r_WRITE_SOPMEMf_SET(r,f) (r).p1588_inband_rx_control2[0]=(((r).p1588_inband_rx_control2[0] & ~((uint32_t)CAP_SEL_MASK << 0)) | ((((uint32_t)f) & CAP_SEL_MASK) << 0)) | (CAP_SEL_MASK << (16 + 0))

/*
 * These macros can be used to access P1588_INBAND_RX_CONTROL2.
 *
 */
#define PLP_READ_P1588_INBAND_RX_CONTROL2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_INBAND_RX_CONTROL2r,(_r._p1588_inband_rx_control2))
#define PLP_WRITE_P1588_INBAND_RX_CONTROL2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_INBAND_RX_CONTROL2r,(_r._p1588_inband_rx_control2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_INBAND_RX_CONTROL2r PLP_P1588_INBAND_RX_CONTROL2r
#define P1588_INBAND_RX_CONTROL2r_SIZE PLP_P1588_INBAND_RX_CONTROL2r_SIZE
typedef PLP_P1588_INBAND_RX_CONTROL2r_t P1588_INBAND_RX_CONTROL2r_t;
#define P1588_INBAND_RX_CONTROL2r_CLR PLP_P1588_INBAND_RX_CONTROL2r_CLR
#define P1588_INBAND_RX_CONTROL2r_SET PLP_P1588_INBAND_RX_CONTROL2r_SET
#define P1588_INBAND_RX_CONTROL2r_GET PLP_P1588_INBAND_RX_CONTROL2r_GET
#define P1588_INBAND_RX_CONTROL2r_INBAND_RX_CONTROL2f_GET PLP_P1588_INBAND_RX_CONTROL2r_INBAND_RX_CONTROL2f_GET
#define P1588_INBAND_RX_CONTROL2r_INBAND_RX_CONTROL2f_SET PLP_P1588_INBAND_RX_CONTROL2r_INBAND_RX_CONTROL2f_SET
#define READ_P1588_INBAND_RX_CONTROL2r PLP_READ_P1588_INBAND_RX_CONTROL2r
#define WRITE_P1588_INBAND_RX_CONTROL2r PLP_WRITE_P1588_INBAND_RX_CONTROL2r
#define MODIFY_P1588_INBAND_RX_CONTROL2r PLP_MODIFY_P1588_INBAND_RX_CONTROL2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_INBAND_RX_CONTROL2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_CONTROL
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70bb
 * DESC:     sopmem control type Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_INDEX     index to 32 sopmem entries.
 *     SOPMEM_SUB_INDEX index to 32 sopmem sub entries and packets counters.
 *     SOPMEM_STATUS_CAPTURE 1 = enable capture and hold all status counter show in sub_index before read.
 *     SOPMEM_AUTO_INCREMENT 1 = enable auto increment sub_index.
 *     SOPMEM_LOAD_INDEX 1 = load both index and sub_index.
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_CONTROLr    0x000000bb

#define PLP_P1588_SOPMEM_CONTROLr_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_CONTROL.
 *
 */
typedef union PLP_P1588_SOPMEM_CONTROLr_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_control[1];
	uint32_t _p1588_sopmem_control;
} PLP_P1588_SOPMEM_CONTROLr_t;

#define PLP_P1588_SOPMEM_CONTROLr_CLR(r) (r).p1588_sopmem_control[0] = 0
#define PLP_P1588_SOPMEM_CONTROLr_SET(r,d) (r).p1588_sopmem_control[0] = d
#define PLP_P1588_SOPMEM_CONTROLr_GET(r) (r).p1588_sopmem_control[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_CONTROLr_SOPMEM_LOAD_INDEXf_GET(r) ((((r).p1588_sopmem_control[0]) >> 15) & 0x1)
#define PLP_P1588_SOPMEM_CONTROLr_SOPMEM_LOAD_INDEXf_SET(r,f) (r).p1588_sopmem_control[0]=(((r).p1588_sopmem_control[0] & ~((uint32_t)0x1 << 15)) | ((((uint32_t)f) & 0x1) << 15)) | (1 << (16 + 15))
#define PLP_P1588_SOPMEM_CONTROLr_SOPMEM_AUTO_INCREMENTf_GET(r) ((((r).p1588_sopmem_control[0]) >> 14) & 0x1)
#define PLP_P1588_SOPMEM_CONTROLr_SOPMEM_AUTO_INCREMENTf_SET(r,f) (r).p1588_sopmem_control[0]=(((r).p1588_sopmem_control[0] & ~((uint32_t)0x1 << 14)) | ((((uint32_t)f) & 0x1) << 14)) | (1 << (16 + 14))
#define PLP_P1588_SOPMEM_CONTROLr_SOPMEM_WRITE_ACCESS_GET(r) ((((r).p1588_sopmem_control[0]) >> 13) & 0x1)
#define PLP_P1588_SOPMEM_CONTROLr_SOPMEM_WRITE_ACCESS_SET(r,f) (r).p1588_sopmem_control[0]=(((r).p1588_sopmem_control[0] & ~((uint32_t)0x1 << 13)) | ((((uint32_t)f) & 0x1) << 13)) | (1 << (16 + 13))
#define PLP_P1588_SOPMEM_CONTROLr_SOPMEM_READ_ACCESS_GET(r)  ((((r).p1588_sopmem_control[0]) >> 12) & 0x1)
#define PLP_P1588_SOPMEM_CONTROLr_SOPMEM_READ_ACCESS_SET(r,f)  (r).p1588_sopmem_control[0]=(((r).p1588_sopmem_control[0] & ~((uint32_t)0x1 << 12)) | ((((uint32_t)f) & 0x1) << 12)) | (1 << (16 + 12))
#define PLP_P1588_SOPMEM_CONTROLr_SOPMEM_STATUS_CAPTUREf_GET(r) ((((r).p1588_sopmem_control[0]) >> 11) & 0x1)
#define PLP_P1588_SOPMEM_CONTROLr_SOPMEM_STATUS_CAPTUREf_SET(r,f) (r).p1588_sopmem_control[0]=(((r).p1588_sopmem_control[0] & ~((uint32_t)0x1 << 11)) | ((((uint32_t)f) & 0x1) << 11)) | (1 << (16 + 11))
#define PLP_P1588_SOPMEM_CONTROLr_SOPMEM_SUB_INDEXf_GET(r) ((((r).p1588_sopmem_control[0]) >> 5) & 0x3f)
#define PLP_P1588_SOPMEM_CONTROLr_SOPMEM_SUB_INDEXf_SET(r,f) (r).p1588_sopmem_control[0]=(((r).p1588_sopmem_control[0] & ~((uint32_t)0x3f << 5)) | ((((uint32_t)f) & 0x3f) << 5)) | (63 << (16 + 5))
#define PLP_P1588_SOPMEM_CONTROLr_SOPMEM_INDEXf_GET(r) (((r).p1588_sopmem_control[0]) & 0x1f)
#define PLP_P1588_SOPMEM_CONTROLr_SOPMEM_INDEXf_SET(r,f) (r).p1588_sopmem_control[0]=(((r).p1588_sopmem_control[0] & ~((uint32_t)0x1f)) | (((uint32_t)f) & 0x1f)) | (0x1f << 16)

/*
 * These macros can be used to access P1588_SOPMEM_CONTROL.
 *
 */
#define PLP_READ_P1588_SOPMEM_CONTROLr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_CONTROLr,(_r._p1588_sopmem_control))
#define PLP_WRITE_P1588_SOPMEM_CONTROLr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_CONTROLr,(_r._p1588_sopmem_control))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_CONTROLr PLP_P1588_SOPMEM_CONTROLr
#define P1588_SOPMEM_CONTROLr_SIZE PLP_P1588_SOPMEM_CONTROLr_SIZE
typedef PLP_P1588_SOPMEM_CONTROLr_t P1588_SOPMEM_CONTROLr_t;
#define P1588_SOPMEM_CONTROLr_CLR PLP_P1588_SOPMEM_CONTROLr_CLR
#define P1588_SOPMEM_CONTROLr_SET PLP_P1588_SOPMEM_CONTROLr_SET
#define P1588_SOPMEM_CONTROLr_GET PLP_P1588_SOPMEM_CONTROLr_GET
#define P1588_SOPMEM_CONTROLr_SOPMEM_LOAD_INDEXf_GET PLP_P1588_SOPMEM_CONTROLr_SOPMEM_LOAD_INDEXf_GET
#define P1588_SOPMEM_CONTROLr_SOPMEM_LOAD_INDEXf_SET PLP_P1588_SOPMEM_CONTROLr_SOPMEM_LOAD_INDEXf_SET
#define P1588_SOPMEM_CONTROLr_SOPMEM_AUTO_INCREMENTf_GET PLP_P1588_SOPMEM_CONTROLr_SOPMEM_AUTO_INCREMENTf_GET
#define P1588_SOPMEM_CONTROLr_SOPMEM_AUTO_INCREMENTf_SET PLP_P1588_SOPMEM_CONTROLr_SOPMEM_AUTO_INCREMENTf_SET
#define P1588_SOPMEM_CONTROLr_SOPMEM_WRITE_ACCESS_GET PLP_P1588_SOPMEM_CONTROLr_SOPMEM_WRITE_ACCESS_GET
#define P1588_SOPMEM_CONTROLr_SOPMEM_WRITE_ACCESS_SET PLP_P1588_SOPMEM_CONTROLr_SOPMEM_WRITE_ACCESS_SET
#define P1588_SOPMEM_CONTROLr_SOPMEM_READ_ACCESS_GET  PLP_P1588_SOPMEM_CONTROLr_SOPMEM_READ_ACCESS_GET
#define P1588_SOPMEM_CONTROLr_SOPMEM_READ_ACCESS_SET  PLP_P1588_SOPMEM_CONTROLr_SOPMEM_READ_ACCESS_SET
#define P1588_SOPMEM_CONTROLr_SOPMEM_STATUS_CAPTUREf_GET PLP_P1588_SOPMEM_CONTROLr_SOPMEM_STATUS_CAPTUREf_GET
#define P1588_SOPMEM_CONTROLr_SOPMEM_STATUS_CAPTUREf_SET PLP_P1588_SOPMEM_CONTROLr_SOPMEM_STATUS_CAPTUREf_SET
#define P1588_SOPMEM_CONTROLr_SOPMEM_SUB_INDEXf_GET PLP_P1588_SOPMEM_CONTROLr_SOPMEM_SUB_INDEXf_GET
#define P1588_SOPMEM_CONTROLr_SOPMEM_SUB_INDEXf_SET PLP_P1588_SOPMEM_CONTROLr_SOPMEM_SUB_INDEXf_SET
#define P1588_SOPMEM_CONTROLr_SOPMEM_INDEXf_GET PLP_P1588_SOPMEM_CONTROLr_SOPMEM_INDEXf_GET
#define P1588_SOPMEM_CONTROLr_SOPMEM_INDEXf_SET PLP_P1588_SOPMEM_CONTROLr_SOPMEM_INDEXf_SET
#define READ_P1588_SOPMEM_CONTROLr PLP_READ_P1588_SOPMEM_CONTROLr
#define WRITE_P1588_SOPMEM_CONTROLr PLP_WRITE_P1588_SOPMEM_CONTROLr
#define MODIFY_P1588_SOPMEM_CONTROLr PLP_MODIFY_P1588_SOPMEM_CONTROLr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_CONTROLr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_CONTROL2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70bc
 * DESC:     sopmem control type Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_CONTROL2  bit 0: 1 = enable MDIO write access sopmem;  0 = disasble MDIO write access to SOPMEM.  default 0.bit 1:  1= enable inband filter override. 0= disable filter override and use action from filter table entrybit 6:4 - override inband filter actions- 0= no action.- 1= ts inband36 .- 2= ts inband32 no command.- 3= mdio gen1 mode.- 4= ts inband32 + ptpver.bit 7: 1= inband filter enable.  the override action above will get executedbit 11: 1 = select nse48 as timer for gen2 TC and partial TC mode.  default 0 - used nse80 bit
 *     SOPMEM_ALL_ENTRIES_CLEAR 1 = clear all 16 entries at once.
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_CONTROL2r    0x000000bc

#define PLP_P1588_SOPMEM_CONTROL2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_CONTROL2.
 *
 */
typedef union PLP_P1588_SOPMEM_CONTROL2r_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_control2[1];
	uint32_t _p1588_sopmem_control2;
} PLP_P1588_SOPMEM_CONTROL2r_t;

#define PLP_P1588_SOPMEM_CONTROL2r_CLR(r) (r).p1588_sopmem_control2[0] = 0
#define PLP_P1588_SOPMEM_CONTROL2r_SET(r,d) (r).p1588_sopmem_control2[0] = d
#define PLP_P1588_SOPMEM_CONTROL2r_GET(r) (r).p1588_sopmem_control2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_CONTROL2r_SOPMEM_ALL_ENTRIES_CLEARf_GET(r) ((((r).p1588_sopmem_control2[0]) >> 15) & 0x1)
#define PLP_P1588_SOPMEM_CONTROL2r_SOPMEM_ALL_ENTRIES_CLEARf_SET(r,f) (r).p1588_sopmem_control2[0]=(((r).p1588_sopmem_control2[0] & ~((uint32_t)0x1 << 15)) | ((((uint32_t)f) & 0x1) << 15)) | (1 << (16 + 15))
#define PLP_P1588_SOPMEM_CONTROL2r_NSE_48BIT_TIMERf_GET(r) ((((r).p1588_sopmem_control2[0]) >> 11) & 0x1)
#define PLP_P1588_SOPMEM_CONTROL2r_NSE_48BIT_TIMERf_SET(r,f) (r).p1588_sopmem_control2[0]=(((r).p1588_sopmem_control2[0] & ~((uint32_t)0x1 << 11)) | ((((uint32_t)f) & 0x1) << 11)) | (1 << (16 + 11))

#define PLP_P1588_SOPMEM_CONTROL2r_SOPMEM_CONTROL2f_GET(r) (((r).p1588_sopmem_control2[0]) & 0x7fff)
#define PLP_P1588_SOPMEM_CONTROL2r_SOPMEM_CONTROL2f_SET(r,f) (r).p1588_sopmem_control2[0]=(((r).p1588_sopmem_control2[0] & ~((uint32_t)0x7fff)) | (((uint32_t)f) & 0x7fff)) | (0x7fff << 16)

/*
 * These macros can be used to access P1588_SOPMEM_CONTROL2.
 *
 */
#define PLP_READ_P1588_SOPMEM_CONTROL2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_CONTROL2r,(_r._p1588_sopmem_control2))
#define PLP_WRITE_P1588_SOPMEM_CONTROL2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_CONTROL2r,(_r._p1588_sopmem_control2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_CONTROL2r PLP_P1588_SOPMEM_CONTROL2r
#define P1588_SOPMEM_CONTROL2r_SIZE PLP_P1588_SOPMEM_CONTROL2r_SIZE
typedef PLP_P1588_SOPMEM_CONTROL2r_t P1588_SOPMEM_CONTROL2r_t;
#define P1588_SOPMEM_CONTROL2r_CLR PLP_P1588_SOPMEM_CONTROL2r_CLR
#define P1588_SOPMEM_CONTROL2r_SET PLP_P1588_SOPMEM_CONTROL2r_SET
#define P1588_SOPMEM_CONTROL2r_GET PLP_P1588_SOPMEM_CONTROL2r_GET
#define P1588_SOPMEM_CONTROL2r_SOPMEM_ALL_ENTRIES_CLEARf_GET PLP_P1588_SOPMEM_CONTROL2r_SOPMEM_ALL_ENTRIES_CLEARf_GET
#define P1588_SOPMEM_CONTROL2r_SOPMEM_ALL_ENTRIES_CLEARf_SET PLP_P1588_SOPMEM_CONTROL2r_SOPMEM_ALL_ENTRIES_CLEARf_SET
#define P1588_SOPMEM_CONTROL2r_SOPMEM_CONTROL2f_GET PLP_P1588_SOPMEM_CONTROL2r_SOPMEM_CONTROL2f_GET
#define P1588_SOPMEM_CONTROL2r_SOPMEM_CONTROL2f_SET PLP_P1588_SOPMEM_CONTROL2r_SOPMEM_CONTROL2f_SET
#define READ_P1588_SOPMEM_CONTROL2r PLP_READ_P1588_SOPMEM_CONTROL2r
#define WRITE_P1588_SOPMEM_CONTROL2r PLP_WRITE_P1588_SOPMEM_CONTROL2r
#define MODIFY_P1588_SOPMEM_CONTROL2r PLP_MODIFY_P1588_SOPMEM_CONTROL2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_CONTROL2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_WDATA
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70bd
 * DESC:     sopmem wdata Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_WDATA     indirect write data.
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_WDATAr    0x000000bd

#define PLP_P1588_SOPMEM_WDATAr_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_WDATA.
 *
 */
typedef union PLP_P1588_SOPMEM_WDATAr_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_wdata[1];
	uint32_t _p1588_sopmem_wdata;
} PLP_P1588_SOPMEM_WDATAr_t;

#define PLP_P1588_SOPMEM_WDATAr_CLR(r) (r).p1588_sopmem_wdata[0] = 0
#define PLP_P1588_SOPMEM_WDATAr_SET(r,d) (r).p1588_sopmem_wdata[0] = d
#define PLP_P1588_SOPMEM_WDATAr_GET(r) (r).p1588_sopmem_wdata[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_WDATAr_SOPMEM_WDATAf_GET(r) (((r).p1588_sopmem_wdata[0]) & 0xffff)
#define PLP_P1588_SOPMEM_WDATAr_SOPMEM_WDATAf_SET(r,f) (r).p1588_sopmem_wdata[0]=(((r).p1588_sopmem_wdata[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_SOPMEM_WDATA.
 *
 */
#define PLP_READ_P1588_SOPMEM_WDATAr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_WDATAr,(_r._p1588_sopmem_wdata))
#define PLP_WRITE_P1588_SOPMEM_WDATAr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_WDATAr,(_r._p1588_sopmem_wdata))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_WDATAr PLP_P1588_SOPMEM_WDATAr
#define P1588_SOPMEM_WDATAr_SIZE PLP_P1588_SOPMEM_WDATAr_SIZE
typedef PLP_P1588_SOPMEM_WDATAr_t P1588_SOPMEM_WDATAr_t;
#define P1588_SOPMEM_WDATAr_CLR PLP_P1588_SOPMEM_WDATAr_CLR
#define P1588_SOPMEM_WDATAr_SET PLP_P1588_SOPMEM_WDATAr_SET
#define P1588_SOPMEM_WDATAr_GET PLP_P1588_SOPMEM_WDATAr_GET
#define P1588_SOPMEM_WDATAr_SOPMEM_WDATAf_GET PLP_P1588_SOPMEM_WDATAr_SOPMEM_WDATAf_GET
#define P1588_SOPMEM_WDATAr_SOPMEM_WDATAf_SET PLP_P1588_SOPMEM_WDATAr_SOPMEM_WDATAf_SET
#define READ_P1588_SOPMEM_WDATAr PLP_READ_P1588_SOPMEM_WDATAr
#define WRITE_P1588_SOPMEM_WDATAr PLP_WRITE_P1588_SOPMEM_WDATAr
#define MODIFY_P1588_SOPMEM_WDATAr PLP_MODIFY_P1588_SOPMEM_WDATAr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_WDATAr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_RDATA
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70be
 * DESC:     sopmem rdata Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_RDATA     indirect read data.
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_RDATAr    0x000000be

#define PLP_P1588_SOPMEM_RDATAr_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_RDATA.
 *
 */
typedef union PLP_P1588_SOPMEM_RDATAr_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_rdata[1];
	uint32_t _p1588_sopmem_rdata;
} PLP_P1588_SOPMEM_RDATAr_t;

#define PLP_P1588_SOPMEM_RDATAr_CLR(r) (r).p1588_sopmem_rdata[0] = 0
#define PLP_P1588_SOPMEM_RDATAr_SET(r,d) (r).p1588_sopmem_rdata[0] = d
#define PLP_P1588_SOPMEM_RDATAr_GET(r) (r).p1588_sopmem_rdata[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_RDATAr_SOPMEM_RDATAf_GET(r) (((r).p1588_sopmem_rdata[0]) & 0xffff)
#define PLP_P1588_SOPMEM_RDATAr_SOPMEM_RDATAf_SET(r,f) (r).p1588_sopmem_rdata[0]=(((r).p1588_sopmem_rdata[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_SOPMEM_RDATA.
 *
 */
#define PLP_READ_P1588_SOPMEM_RDATAr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_RDATAr,(_r._p1588_sopmem_rdata))
#define PLP_WRITE_P1588_SOPMEM_RDATAr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_RDATAr,(_r._p1588_sopmem_rdata))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_RDATAr PLP_P1588_SOPMEM_RDATAr
#define P1588_SOPMEM_RDATAr_SIZE PLP_P1588_SOPMEM_RDATAr_SIZE
typedef PLP_P1588_SOPMEM_RDATAr_t P1588_SOPMEM_RDATAr_t;
#define P1588_SOPMEM_RDATAr_CLR PLP_P1588_SOPMEM_RDATAr_CLR
#define P1588_SOPMEM_RDATAr_SET PLP_P1588_SOPMEM_RDATAr_SET
#define P1588_SOPMEM_RDATAr_GET PLP_P1588_SOPMEM_RDATAr_GET
#define P1588_SOPMEM_RDATAr_SOPMEM_RDATAf_GET PLP_P1588_SOPMEM_RDATAr_SOPMEM_RDATAf_GET
#define P1588_SOPMEM_RDATAr_SOPMEM_RDATAf_SET PLP_P1588_SOPMEM_RDATAr_SOPMEM_RDATAf_SET
#define READ_P1588_SOPMEM_RDATAr PLP_READ_P1588_SOPMEM_RDATAr
#define WRITE_P1588_SOPMEM_RDATAr PLP_WRITE_P1588_SOPMEM_RDATAr
#define MODIFY_P1588_SOPMEM_RDATAr PLP_MODIFY_P1588_SOPMEM_RDATAr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_RDATAr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_CLEAR_LO_ENTRY
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70bf
 * DESC:     sopmem clear low entry Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_CLEAR_LO_ENTRY bitwise clear individual entry 15:0 .
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_CLEAR_LO_ENTRYr    0x000000bf

#define PLP_P1588_SOPMEM_CLEAR_LO_ENTRYr_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_CLEAR_LO_ENTRY.
 *
 */
typedef union PLP_P1588_SOPMEM_CLEAR_LO_ENTRYr_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_clear_lo_entry[1];
	uint32_t _p1588_sopmem_clear_lo_entry;
} PLP_P1588_SOPMEM_CLEAR_LO_ENTRYr_t;

#define PLP_P1588_SOPMEM_CLEAR_LO_ENTRYr_CLR(r) (r).p1588_sopmem_clear_lo_entry[0] = 0
#define PLP_P1588_SOPMEM_CLEAR_LO_ENTRYr_SET(r,d) (r).p1588_sopmem_clear_lo_entry[0] = d
#define PLP_P1588_SOPMEM_CLEAR_LO_ENTRYr_GET(r) (r).p1588_sopmem_clear_lo_entry[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_CLEAR_LO_ENTRYr_SOPMEM_CLEAR_LO_ENTRYf_GET(r) (((r).p1588_sopmem_clear_lo_entry[0]) & 0xffff)
#define PLP_P1588_SOPMEM_CLEAR_LO_ENTRYr_SOPMEM_CLEAR_LO_ENTRYf_SET(r,f) (r).p1588_sopmem_clear_lo_entry[0]=(((r).p1588_sopmem_clear_lo_entry[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_SOPMEM_CLEAR_LO_ENTRY.
 *
 */
#define PLP_READ_P1588_SOPMEM_CLEAR_LO_ENTRYr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_CLEAR_LO_ENTRYr,(_r._p1588_sopmem_clear_lo_entry))
#define PLP_WRITE_P1588_SOPMEM_CLEAR_LO_ENTRYr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_CLEAR_LO_ENTRYr,(_r._p1588_sopmem_clear_lo_entry))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_CLEAR_LO_ENTRYr PLP_P1588_SOPMEM_CLEAR_LO_ENTRYr
#define P1588_SOPMEM_CLEAR_LO_ENTRYr_SIZE PLP_P1588_SOPMEM_CLEAR_LO_ENTRYr_SIZE
typedef PLP_P1588_SOPMEM_CLEAR_LO_ENTRYr_t P1588_SOPMEM_CLEAR_LO_ENTRYr_t;
#define P1588_SOPMEM_CLEAR_LO_ENTRYr_CLR PLP_P1588_SOPMEM_CLEAR_LO_ENTRYr_CLR
#define P1588_SOPMEM_CLEAR_LO_ENTRYr_SET PLP_P1588_SOPMEM_CLEAR_LO_ENTRYr_SET
#define P1588_SOPMEM_CLEAR_LO_ENTRYr_GET PLP_P1588_SOPMEM_CLEAR_LO_ENTRYr_GET
#define P1588_SOPMEM_CLEAR_LO_ENTRYr_SOPMEM_CLEAR_LO_ENTRYf_GET PLP_P1588_SOPMEM_CLEAR_LO_ENTRYr_SOPMEM_CLEAR_LO_ENTRYf_GET
#define P1588_SOPMEM_CLEAR_LO_ENTRYr_SOPMEM_CLEAR_LO_ENTRYf_SET PLP_P1588_SOPMEM_CLEAR_LO_ENTRYr_SOPMEM_CLEAR_LO_ENTRYf_SET
#define READ_P1588_SOPMEM_CLEAR_LO_ENTRYr PLP_READ_P1588_SOPMEM_CLEAR_LO_ENTRYr
#define WRITE_P1588_SOPMEM_CLEAR_LO_ENTRYr PLP_WRITE_P1588_SOPMEM_CLEAR_LO_ENTRYr
#define MODIFY_P1588_SOPMEM_CLEAR_LO_ENTRYr PLP_MODIFY_P1588_SOPMEM_CLEAR_LO_ENTRYr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_CLEAR_LO_ENTRYr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_CLEAR_HI_ENTRY
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70c0
 * DESC:     sopmem clear high entry Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_CLEAR_HI_ENTRY bitwise clear individual entry 31:16.
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_CLEAR_HI_ENTRYr    0x000000c0

#define PLP_P1588_SOPMEM_CLEAR_HI_ENTRYr_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_CLEAR_HI_ENTRY.
 *
 */
typedef union PLP_P1588_SOPMEM_CLEAR_HI_ENTRYr_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_clear_hi_entry[1];
	uint32_t _p1588_sopmem_clear_hi_entry;
} PLP_P1588_SOPMEM_CLEAR_HI_ENTRYr_t;

#define PLP_P1588_SOPMEM_CLEAR_HI_ENTRYr_CLR(r) (r).p1588_sopmem_clear_hi_entry[0] = 0
#define PLP_P1588_SOPMEM_CLEAR_HI_ENTRYr_SET(r,d) (r).p1588_sopmem_clear_hi_entry[0] = d
#define PLP_P1588_SOPMEM_CLEAR_HI_ENTRYr_GET(r) (r).p1588_sopmem_clear_hi_entry[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_CLEAR_HI_ENTRYr_SOPMEM_CLEAR_HI_ENTRYf_GET(r) (((r).p1588_sopmem_clear_hi_entry[0]) & 0xffff)
#define PLP_P1588_SOPMEM_CLEAR_HI_ENTRYr_SOPMEM_CLEAR_HI_ENTRYf_SET(r,f) (r).p1588_sopmem_clear_hi_entry[0]=(((r).p1588_sopmem_clear_hi_entry[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_SOPMEM_CLEAR_HI_ENTRY.
 *
 */
#define PLP_READ_P1588_SOPMEM_CLEAR_HI_ENTRYr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_CLEAR_HI_ENTRYr,(_r._p1588_sopmem_clear_hi_entry))
#define PLP_WRITE_P1588_SOPMEM_CLEAR_HI_ENTRYr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_CLEAR_HI_ENTRYr,(_r._p1588_sopmem_clear_hi_entry))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_CLEAR_HI_ENTRYr PLP_P1588_SOPMEM_CLEAR_HI_ENTRYr
#define P1588_SOPMEM_CLEAR_HI_ENTRYr_SIZE PLP_P1588_SOPMEM_CLEAR_HI_ENTRYr_SIZE
typedef PLP_P1588_SOPMEM_CLEAR_HI_ENTRYr_t P1588_SOPMEM_CLEAR_HI_ENTRYr_t;
#define P1588_SOPMEM_CLEAR_HI_ENTRYr_CLR PLP_P1588_SOPMEM_CLEAR_HI_ENTRYr_CLR
#define P1588_SOPMEM_CLEAR_HI_ENTRYr_SET PLP_P1588_SOPMEM_CLEAR_HI_ENTRYr_SET
#define P1588_SOPMEM_CLEAR_HI_ENTRYr_GET PLP_P1588_SOPMEM_CLEAR_HI_ENTRYr_GET
#define P1588_SOPMEM_CLEAR_HI_ENTRYr_SOPMEM_CLEAR_HI_ENTRYf_GET PLP_P1588_SOPMEM_CLEAR_HI_ENTRYr_SOPMEM_CLEAR_HI_ENTRYf_GET
#define P1588_SOPMEM_CLEAR_HI_ENTRYr_SOPMEM_CLEAR_HI_ENTRYf_SET PLP_P1588_SOPMEM_CLEAR_HI_ENTRYr_SOPMEM_CLEAR_HI_ENTRYf_SET
#define READ_P1588_SOPMEM_CLEAR_HI_ENTRYr PLP_READ_P1588_SOPMEM_CLEAR_HI_ENTRYr
#define WRITE_P1588_SOPMEM_CLEAR_HI_ENTRYr PLP_WRITE_P1588_SOPMEM_CLEAR_HI_ENTRYr
#define MODIFY_P1588_SOPMEM_CLEAR_HI_ENTRYr PLP_MODIFY_P1588_SOPMEM_CLEAR_HI_ENTRYr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_CLEAR_HI_ENTRYr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_VALID_LO_ENTRY
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70c1
 * DESC:     sopmem valid low entry Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_VALID_LO_ENTRY valid entry status - read only.
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_VALID_LO_ENTRYr    0x000000c1

#define PLP_P1588_SOPMEM_VALID_LO_ENTRYr_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_VALID_LO_ENTRY.
 *
 */
typedef union PLP_P1588_SOPMEM_VALID_LO_ENTRYr_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_valid_lo_entry[1];
	uint32_t _p1588_sopmem_valid_lo_entry;
} PLP_P1588_SOPMEM_VALID_LO_ENTRYr_t;

#define PLP_P1588_SOPMEM_VALID_LO_ENTRYr_CLR(r) (r).p1588_sopmem_valid_lo_entry[0] = 0
#define PLP_P1588_SOPMEM_VALID_LO_ENTRYr_SET(r,d) (r).p1588_sopmem_valid_lo_entry[0] = d
#define PLP_P1588_SOPMEM_VALID_LO_ENTRYr_GET(r) (r).p1588_sopmem_valid_lo_entry[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_VALID_LO_ENTRYr_SOPMEM_VALID_LO_ENTRYf_GET(r) (((r).p1588_sopmem_valid_lo_entry[0]) & 0xffff)
#define PLP_P1588_SOPMEM_VALID_LO_ENTRYr_SOPMEM_VALID_LO_ENTRYf_SET(r,f) (r).p1588_sopmem_valid_lo_entry[0]=(((r).p1588_sopmem_valid_lo_entry[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_SOPMEM_VALID_LO_ENTRY.
 *
 */
#define PLP_READ_P1588_SOPMEM_VALID_LO_ENTRYr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_VALID_LO_ENTRYr,(_r._p1588_sopmem_valid_lo_entry))
#define PLP_WRITE_P1588_SOPMEM_VALID_LO_ENTRYr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_VALID_LO_ENTRYr,(_r._p1588_sopmem_valid_lo_entry))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_VALID_LO_ENTRYr PLP_P1588_SOPMEM_VALID_LO_ENTRYr
#define P1588_SOPMEM_VALID_LO_ENTRYr_SIZE PLP_P1588_SOPMEM_VALID_LO_ENTRYr_SIZE
typedef PLP_P1588_SOPMEM_VALID_LO_ENTRYr_t P1588_SOPMEM_VALID_LO_ENTRYr_t;
#define P1588_SOPMEM_VALID_LO_ENTRYr_CLR PLP_P1588_SOPMEM_VALID_LO_ENTRYr_CLR
#define P1588_SOPMEM_VALID_LO_ENTRYr_SET PLP_P1588_SOPMEM_VALID_LO_ENTRYr_SET
#define P1588_SOPMEM_VALID_LO_ENTRYr_GET PLP_P1588_SOPMEM_VALID_LO_ENTRYr_GET
#define P1588_SOPMEM_VALID_LO_ENTRYr_SOPMEM_VALID_LO_ENTRYf_GET PLP_P1588_SOPMEM_VALID_LO_ENTRYr_SOPMEM_VALID_LO_ENTRYf_GET
#define P1588_SOPMEM_VALID_LO_ENTRYr_SOPMEM_VALID_LO_ENTRYf_SET PLP_P1588_SOPMEM_VALID_LO_ENTRYr_SOPMEM_VALID_LO_ENTRYf_SET
#define READ_P1588_SOPMEM_VALID_LO_ENTRYr PLP_READ_P1588_SOPMEM_VALID_LO_ENTRYr
#define WRITE_P1588_SOPMEM_VALID_LO_ENTRYr PLP_WRITE_P1588_SOPMEM_VALID_LO_ENTRYr
#define MODIFY_P1588_SOPMEM_VALID_LO_ENTRYr PLP_MODIFY_P1588_SOPMEM_VALID_LO_ENTRYr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_VALID_LO_ENTRYr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_VALID_HI_ENTRY
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70c2
 * DESC:     sopmem valid high entry Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_VALID_HI_ENTRY valid entry status - read only.
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_VALID_HI_ENTRYr    0x000000c2

#define PLP_P1588_SOPMEM_VALID_HI_ENTRYr_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_VALID_HI_ENTRY.
 *
 */
typedef union PLP_P1588_SOPMEM_VALID_HI_ENTRYr_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_valid_hi_entry[1];
	uint32_t _p1588_sopmem_valid_hi_entry;
} PLP_P1588_SOPMEM_VALID_HI_ENTRYr_t;

#define PLP_P1588_SOPMEM_VALID_HI_ENTRYr_CLR(r) (r).p1588_sopmem_valid_hi_entry[0] = 0
#define PLP_P1588_SOPMEM_VALID_HI_ENTRYr_SET(r,d) (r).p1588_sopmem_valid_hi_entry[0] = d
#define PLP_P1588_SOPMEM_VALID_HI_ENTRYr_GET(r) (r).p1588_sopmem_valid_hi_entry[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_VALID_HI_ENTRYr_SOPMEM_VALID_HI_ENTRYf_GET(r) (((r).p1588_sopmem_valid_hi_entry[0]) & 0xffff)
#define PLP_P1588_SOPMEM_VALID_HI_ENTRYr_SOPMEM_VALID_HI_ENTRYf_SET(r,f) (r).p1588_sopmem_valid_hi_entry[0]=(((r).p1588_sopmem_valid_hi_entry[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_SOPMEM_VALID_HI_ENTRY.
 *
 */
#define PLP_READ_P1588_SOPMEM_VALID_HI_ENTRYr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_VALID_HI_ENTRYr,(_r._p1588_sopmem_valid_hi_entry))
#define PLP_WRITE_P1588_SOPMEM_VALID_HI_ENTRYr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_VALID_HI_ENTRYr,(_r._p1588_sopmem_valid_hi_entry))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_VALID_HI_ENTRYr PLP_P1588_SOPMEM_VALID_HI_ENTRYr
#define P1588_SOPMEM_VALID_HI_ENTRYr_SIZE PLP_P1588_SOPMEM_VALID_HI_ENTRYr_SIZE
typedef PLP_P1588_SOPMEM_VALID_HI_ENTRYr_t P1588_SOPMEM_VALID_HI_ENTRYr_t;
#define P1588_SOPMEM_VALID_HI_ENTRYr_CLR PLP_P1588_SOPMEM_VALID_HI_ENTRYr_CLR
#define P1588_SOPMEM_VALID_HI_ENTRYr_SET PLP_P1588_SOPMEM_VALID_HI_ENTRYr_SET
#define P1588_SOPMEM_VALID_HI_ENTRYr_GET PLP_P1588_SOPMEM_VALID_HI_ENTRYr_GET
#define P1588_SOPMEM_VALID_HI_ENTRYr_SOPMEM_VALID_HI_ENTRYf_GET PLP_P1588_SOPMEM_VALID_HI_ENTRYr_SOPMEM_VALID_HI_ENTRYf_GET
#define P1588_SOPMEM_VALID_HI_ENTRYr_SOPMEM_VALID_HI_ENTRYf_SET PLP_P1588_SOPMEM_VALID_HI_ENTRYr_SOPMEM_VALID_HI_ENTRYf_SET
#define READ_P1588_SOPMEM_VALID_HI_ENTRYr PLP_READ_P1588_SOPMEM_VALID_HI_ENTRYr
#define WRITE_P1588_SOPMEM_VALID_HI_ENTRYr PLP_WRITE_P1588_SOPMEM_VALID_HI_ENTRYr
#define MODIFY_P1588_SOPMEM_VALID_HI_ENTRYr PLP_MODIFY_P1588_SOPMEM_VALID_HI_ENTRYr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_VALID_HI_ENTRYr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_HSR_CONTROL
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70c3
 * DESC:     HSR Control Register
 * RESETVAL: 0x4 (4)
 * ACCESS:   R/W
 * FIELDS:
 *     HSR_PARSER_ENABLE 1 = hsr parser enable.
 *     HSR_LLC_ENABLE   1 = hsr llc enable.
 *     HSR_LLC_DSAP_SSAP_CHECK_ENABLE 1 = hsr LLC dsap and ssap check enable.
 *     HSR_LLC_CTRL_CHECK_ENABLE 1 = hsr llc ctrl check enable.
 *     HSR_LLC_CTRL_SIZE 1 = hsr llc ctrl size is 2 bytes.   0 = hsr control size is 1 byteHW support 1 byte ctrl only
 *
 ******************************************************************************/
#define PLP_P1588_HSR_CONTROLr    0x000000c3

#define PLP_P1588_HSR_CONTROLr_SIZE 4

/*
 * This structure should be used to declare and program P1588_HSR_CONTROL.
 *
 */
typedef union PLP_P1588_HSR_CONTROLr_s {
	uint32_t v[1];
	uint32_t p1588_hsr_control[1];
	uint32_t _p1588_hsr_control;
} PLP_P1588_HSR_CONTROLr_t;

#define PLP_P1588_HSR_CONTROLr_CLR(r) (r).p1588_hsr_control[0] = 0
#define PLP_P1588_HSR_CONTROLr_SET(r,d) (r).p1588_hsr_control[0] = d
#define PLP_P1588_HSR_CONTROLr_GET(r) (r).p1588_hsr_control[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_HSR_CONTROLr_HSR_LLC_CTRL_SIZEf_GET(r) ((((r).p1588_hsr_control[0]) >> 4) & 0x1)
#define PLP_P1588_HSR_CONTROLr_HSR_LLC_CTRL_SIZEf_SET(r,f) (r).p1588_hsr_control[0]=(((r).p1588_hsr_control[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4)) | (1 << (16 + 4))
#define PLP_P1588_HSR_CONTROLr_HSR_LLC_CTRL_CHECK_ENABLEf_GET(r) ((((r).p1588_hsr_control[0]) >> 3) & 0x1)
#define PLP_P1588_HSR_CONTROLr_HSR_LLC_CTRL_CHECK_ENABLEf_SET(r,f) (r).p1588_hsr_control[0]=(((r).p1588_hsr_control[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3)) | (1 << (16 + 3))
#define PLP_P1588_HSR_CONTROLr_HSR_LLC_DSAP_SSAP_CHECK_ENABLEf_GET(r) ((((r).p1588_hsr_control[0]) >> 2) & 0x1)
#define PLP_P1588_HSR_CONTROLr_HSR_LLC_DSAP_SSAP_CHECK_ENABLEf_SET(r,f) (r).p1588_hsr_control[0]=(((r).p1588_hsr_control[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2)) | (1 << (16 + 2))
#define PLP_P1588_HSR_CONTROLr_HSR_LLC_ENABLEf_GET(r) ((((r).p1588_hsr_control[0]) >> 1) & 0x1)
#define PLP_P1588_HSR_CONTROLr_HSR_LLC_ENABLEf_SET(r,f) (r).p1588_hsr_control[0]=(((r).p1588_hsr_control[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))
#define PLP_P1588_HSR_CONTROLr_HSR_PARSER_ENABLEf_GET(r) (((r).p1588_hsr_control[0]) & 0x1)
#define PLP_P1588_HSR_CONTROLr_HSR_PARSER_ENABLEf_SET(r,f) (r).p1588_hsr_control[0]=(((r).p1588_hsr_control[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1)) | (0x1 << 16)

/*
 * These macros can be used to access P1588_HSR_CONTROL.
 *
 */
#define PLP_READ_P1588_HSR_CONTROLr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_HSR_CONTROLr,(_r._p1588_hsr_control))
#define PLP_WRITE_P1588_HSR_CONTROLr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_HSR_CONTROLr,(_r._p1588_hsr_control))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_HSR_CONTROLr PLP_P1588_HSR_CONTROLr
#define P1588_HSR_CONTROLr_SIZE PLP_P1588_HSR_CONTROLr_SIZE
typedef PLP_P1588_HSR_CONTROLr_t P1588_HSR_CONTROLr_t;
#define P1588_HSR_CONTROLr_CLR PLP_P1588_HSR_CONTROLr_CLR
#define P1588_HSR_CONTROLr_SET PLP_P1588_HSR_CONTROLr_SET
#define P1588_HSR_CONTROLr_GET PLP_P1588_HSR_CONTROLr_GET
#define P1588_HSR_CONTROLr_HSR_LLC_CTRL_SIZEf_GET PLP_P1588_HSR_CONTROLr_HSR_LLC_CTRL_SIZEf_GET
#define P1588_HSR_CONTROLr_HSR_LLC_CTRL_SIZEf_SET PLP_P1588_HSR_CONTROLr_HSR_LLC_CTRL_SIZEf_SET
#define P1588_HSR_CONTROLr_HSR_LLC_CTRL_CHECK_ENABLEf_GET PLP_P1588_HSR_CONTROLr_HSR_LLC_CTRL_CHECK_ENABLEf_GET
#define P1588_HSR_CONTROLr_HSR_LLC_CTRL_CHECK_ENABLEf_SET PLP_P1588_HSR_CONTROLr_HSR_LLC_CTRL_CHECK_ENABLEf_SET
#define P1588_HSR_CONTROLr_HSR_LLC_DSAP_SSAP_CHECK_ENABLEf_GET PLP_P1588_HSR_CONTROLr_HSR_LLC_DSAP_SSAP_CHECK_ENABLEf_GET
#define P1588_HSR_CONTROLr_HSR_LLC_DSAP_SSAP_CHECK_ENABLEf_SET PLP_P1588_HSR_CONTROLr_HSR_LLC_DSAP_SSAP_CHECK_ENABLEf_SET
#define P1588_HSR_CONTROLr_HSR_LLC_ENABLEf_GET PLP_P1588_HSR_CONTROLr_HSR_LLC_ENABLEf_GET
#define P1588_HSR_CONTROLr_HSR_LLC_ENABLEf_SET PLP_P1588_HSR_CONTROLr_HSR_LLC_ENABLEf_SET
#define P1588_HSR_CONTROLr_HSR_PARSER_ENABLEf_GET PLP_P1588_HSR_CONTROLr_HSR_PARSER_ENABLEf_GET
#define P1588_HSR_CONTROLr_HSR_PARSER_ENABLEf_SET PLP_P1588_HSR_CONTROLr_HSR_PARSER_ENABLEf_SET
#define READ_P1588_HSR_CONTROLr PLP_READ_P1588_HSR_CONTROLr
#define WRITE_P1588_HSR_CONTROLr PLP_WRITE_P1588_HSR_CONTROLr
#define MODIFY_P1588_HSR_CONTROLr PLP_MODIFY_P1588_HSR_CONTROLr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_HSR_CONTROLr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_HSR_ETYPE
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70c4
 * DESC:     HSR Ethertype Register
 * RESETVAL: 0x892f (35119)
 * ACCESS:   R/W
 * FIELDS:
 *     HSR_ETYPE        HSR ETYPE.
 *
 ******************************************************************************/
#define PLP_P1588_HSR_ETYPEr    0x000000c4

#define PLP_P1588_HSR_ETYPEr_SIZE 4

/*
 * This structure should be used to declare and program P1588_HSR_ETYPE.
 *
 */
typedef union PLP_P1588_HSR_ETYPEr_s {
	uint32_t v[1];
	uint32_t p1588_hsr_etype[1];
	uint32_t _p1588_hsr_etype;
} PLP_P1588_HSR_ETYPEr_t;

#define PLP_P1588_HSR_ETYPEr_CLR(r) (r).p1588_hsr_etype[0] = 0
#define PLP_P1588_HSR_ETYPEr_SET(r,d) (r).p1588_hsr_etype[0] = d
#define PLP_P1588_HSR_ETYPEr_GET(r) (r).p1588_hsr_etype[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_HSR_ETYPEr_HSR_ETYPEf_GET(r) (((r).p1588_hsr_etype[0]) & 0xffff)
#define PLP_P1588_HSR_ETYPEr_HSR_ETYPEf_SET(r,f) (r).p1588_hsr_etype[0]=(((r).p1588_hsr_etype[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_HSR_ETYPE.
 *
 */
#define PLP_READ_P1588_HSR_ETYPEr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_HSR_ETYPEr,(_r._p1588_hsr_etype))
#define PLP_WRITE_P1588_HSR_ETYPEr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_HSR_ETYPEr,(_r._p1588_hsr_etype))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_HSR_ETYPEr PLP_P1588_HSR_ETYPEr
#define P1588_HSR_ETYPEr_SIZE PLP_P1588_HSR_ETYPEr_SIZE
typedef PLP_P1588_HSR_ETYPEr_t P1588_HSR_ETYPEr_t;
#define P1588_HSR_ETYPEr_CLR PLP_P1588_HSR_ETYPEr_CLR
#define P1588_HSR_ETYPEr_SET PLP_P1588_HSR_ETYPEr_SET
#define P1588_HSR_ETYPEr_GET PLP_P1588_HSR_ETYPEr_GET
#define P1588_HSR_ETYPEr_HSR_ETYPEf_GET PLP_P1588_HSR_ETYPEr_HSR_ETYPEf_GET
#define P1588_HSR_ETYPEr_HSR_ETYPEf_SET PLP_P1588_HSR_ETYPEr_HSR_ETYPEf_SET
#define READ_P1588_HSR_ETYPEr PLP_READ_P1588_HSR_ETYPEr
#define WRITE_P1588_HSR_ETYPEr PLP_WRITE_P1588_HSR_ETYPEr
#define MODIFY_P1588_HSR_ETYPEr PLP_MODIFY_P1588_HSR_ETYPEr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_HSR_ETYPEr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_HSR_LLC_DSAP_SSAP_FIELD
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70c5
 * DESC:     HSR LLC DSAP SSAP Register
 * RESETVAL: 0xaaaa (43690)
 * ACCESS:   R/W
 * FIELDS:
 *     HSR_LLC_DSAP_SSAP_FIELD ddsap [7:0]ssap [7:0]
 *
 ******************************************************************************/
#define PLP_P1588_HSR_LLC_DSAP_SSAP_FIELDr    0x000000c5

#define PLP_P1588_HSR_LLC_DSAP_SSAP_FIELDr_SIZE 4

/*
 * This structure should be used to declare and program P1588_HSR_LLC_DSAP_SSAP_FIELD.
 *
 */
typedef union PLP_P1588_HSR_LLC_DSAP_SSAP_FIELDr_s {
	uint32_t v[1];
	uint32_t p1588_hsr_llc_dsap_ssap_field[1];
	uint32_t _p1588_hsr_llc_dsap_ssap_field;
} PLP_P1588_HSR_LLC_DSAP_SSAP_FIELDr_t;

#define PLP_P1588_HSR_LLC_DSAP_SSAP_FIELDr_CLR(r) (r).p1588_hsr_llc_dsap_ssap_field[0] = 0
#define PLP_P1588_HSR_LLC_DSAP_SSAP_FIELDr_SET(r,d) (r).p1588_hsr_llc_dsap_ssap_field[0] = d
#define PLP_P1588_HSR_LLC_DSAP_SSAP_FIELDr_GET(r) (r).p1588_hsr_llc_dsap_ssap_field[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_HSR_LLC_DSAP_SSAP_FIELDr_HSR_LLC_DSAP_SSAP_FIELDf_GET(r) (((r).p1588_hsr_llc_dsap_ssap_field[0]) & 0xffff)
#define PLP_P1588_HSR_LLC_DSAP_SSAP_FIELDr_HSR_LLC_DSAP_SSAP_FIELDf_SET(r,f) (r).p1588_hsr_llc_dsap_ssap_field[0]=(((r).p1588_hsr_llc_dsap_ssap_field[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_HSR_LLC_DSAP_SSAP_FIELD.
 *
 */
#define PLP_READ_P1588_HSR_LLC_DSAP_SSAP_FIELDr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_HSR_LLC_DSAP_SSAP_FIELDr,(_r._p1588_hsr_llc_dsap_ssap_field))
#define PLP_WRITE_P1588_HSR_LLC_DSAP_SSAP_FIELDr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_HSR_LLC_DSAP_SSAP_FIELDr,(_r._p1588_hsr_llc_dsap_ssap_field))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_HSR_LLC_DSAP_SSAP_FIELDr PLP_P1588_HSR_LLC_DSAP_SSAP_FIELDr
#define P1588_HSR_LLC_DSAP_SSAP_FIELDr_SIZE PLP_P1588_HSR_LLC_DSAP_SSAP_FIELDr_SIZE
typedef PLP_P1588_HSR_LLC_DSAP_SSAP_FIELDr_t P1588_HSR_LLC_DSAP_SSAP_FIELDr_t;
#define P1588_HSR_LLC_DSAP_SSAP_FIELDr_CLR PLP_P1588_HSR_LLC_DSAP_SSAP_FIELDr_CLR
#define P1588_HSR_LLC_DSAP_SSAP_FIELDr_SET PLP_P1588_HSR_LLC_DSAP_SSAP_FIELDr_SET
#define P1588_HSR_LLC_DSAP_SSAP_FIELDr_GET PLP_P1588_HSR_LLC_DSAP_SSAP_FIELDr_GET
#define P1588_HSR_LLC_DSAP_SSAP_FIELDr_HSR_LLC_DSAP_SSAP_FIELDf_GET PLP_P1588_HSR_LLC_DSAP_SSAP_FIELDr_HSR_LLC_DSAP_SSAP_FIELDf_GET
#define P1588_HSR_LLC_DSAP_SSAP_FIELDr_HSR_LLC_DSAP_SSAP_FIELDf_SET PLP_P1588_HSR_LLC_DSAP_SSAP_FIELDr_HSR_LLC_DSAP_SSAP_FIELDf_SET
#define READ_P1588_HSR_LLC_DSAP_SSAP_FIELDr PLP_READ_P1588_HSR_LLC_DSAP_SSAP_FIELDr
#define WRITE_P1588_HSR_LLC_DSAP_SSAP_FIELDr PLP_WRITE_P1588_HSR_LLC_DSAP_SSAP_FIELDr
#define MODIFY_P1588_HSR_LLC_DSAP_SSAP_FIELDr PLP_MODIFY_P1588_HSR_LLC_DSAP_SSAP_FIELDr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_HSR_LLC_DSAP_SSAP_FIELDr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_HSR_LLC_CTRL_FIELD
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70c6
 * DESC:     HSR LLC CTRL Register
 * RESETVAL: 0x3 (3)
 * ACCESS:   R/W
 * FIELDS:
 *     HSR_LLC_CTRL_FIELD hsr llc ctrl field
 *
 ******************************************************************************/
#define PLP_P1588_HSR_LLC_CTRL_FIELDr    0x000000c6

#define PLP_P1588_HSR_LLC_CTRL_FIELDr_SIZE 4

/*
 * This structure should be used to declare and program P1588_HSR_LLC_CTRL_FIELD.
 *
 */
typedef union PLP_P1588_HSR_LLC_CTRL_FIELDr_s {
	uint32_t v[1];
	uint32_t p1588_hsr_llc_ctrl_field[1];
	uint32_t _p1588_hsr_llc_ctrl_field;
} PLP_P1588_HSR_LLC_CTRL_FIELDr_t;

#define PLP_P1588_HSR_LLC_CTRL_FIELDr_CLR(r) (r).p1588_hsr_llc_ctrl_field[0] = 0
#define PLP_P1588_HSR_LLC_CTRL_FIELDr_SET(r,d) (r).p1588_hsr_llc_ctrl_field[0] = d
#define PLP_P1588_HSR_LLC_CTRL_FIELDr_GET(r) (r).p1588_hsr_llc_ctrl_field[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_HSR_LLC_CTRL_FIELDr_HSR_LLC_CTRL_FIELDf_GET(r) (((r).p1588_hsr_llc_ctrl_field[0]) & 0xffff)
#define PLP_P1588_HSR_LLC_CTRL_FIELDr_HSR_LLC_CTRL_FIELDf_SET(r,f) (r).p1588_hsr_llc_ctrl_field[0]=(((r).p1588_hsr_llc_ctrl_field[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_HSR_LLC_CTRL_FIELD.
 *
 */
#define PLP_READ_P1588_HSR_LLC_CTRL_FIELDr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_HSR_LLC_CTRL_FIELDr,(_r._p1588_hsr_llc_ctrl_field))
#define PLP_WRITE_P1588_HSR_LLC_CTRL_FIELDr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_HSR_LLC_CTRL_FIELDr,(_r._p1588_hsr_llc_ctrl_field))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_HSR_LLC_CTRL_FIELDr PLP_P1588_HSR_LLC_CTRL_FIELDr
#define P1588_HSR_LLC_CTRL_FIELDr_SIZE PLP_P1588_HSR_LLC_CTRL_FIELDr_SIZE
typedef PLP_P1588_HSR_LLC_CTRL_FIELDr_t P1588_HSR_LLC_CTRL_FIELDr_t;
#define P1588_HSR_LLC_CTRL_FIELDr_CLR PLP_P1588_HSR_LLC_CTRL_FIELDr_CLR
#define P1588_HSR_LLC_CTRL_FIELDr_SET PLP_P1588_HSR_LLC_CTRL_FIELDr_SET
#define P1588_HSR_LLC_CTRL_FIELDr_GET PLP_P1588_HSR_LLC_CTRL_FIELDr_GET
#define P1588_HSR_LLC_CTRL_FIELDr_HSR_LLC_CTRL_FIELDf_GET PLP_P1588_HSR_LLC_CTRL_FIELDr_HSR_LLC_CTRL_FIELDf_GET
#define P1588_HSR_LLC_CTRL_FIELDr_HSR_LLC_CTRL_FIELDf_SET PLP_P1588_HSR_LLC_CTRL_FIELDr_HSR_LLC_CTRL_FIELDf_SET
#define READ_P1588_HSR_LLC_CTRL_FIELDr PLP_READ_P1588_HSR_LLC_CTRL_FIELDr
#define WRITE_P1588_HSR_LLC_CTRL_FIELDr PLP_WRITE_P1588_HSR_LLC_CTRL_FIELDr
#define MODIFY_P1588_HSR_LLC_CTRL_FIELDr PLP_MODIFY_P1588_HSR_LLC_CTRL_FIELDr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_HSR_LLC_CTRL_FIELDr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PKGEN_CONTROL
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70c7
 * DESC:     pkgen control type Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PKGEN_INDEX      index to field registers for packet gen or inband filter entry.0:	packet_num_rg[15:0]] 		- number of packet to send.1:	packet_idle_cnt_rg[15:0]] 	- number of idle byte to send.2:	tx_pkgen_enable_rg[0]] 		- 1= enable packet gen   0= filter mode for Egress.3:	rx_pkgen_enable_rg[0]] 		- 1= enable packet gen   0= filter mode for Ingress.4:  	packet_byte_cnt_rg[7:0] 	- number of byte in packet.5:	packet_control_char_rg[7:0]]- control characters.10:	fld00 - packet data or filter data.11:	fld01 - packet data or filter data.12:	fld02 - packet data or filter data.13:	fld03 - packet data or filter data.......252:	fld252 - packet data or filter data.263:	fld253 - packet data or filter data.264:	fld254 - packet data or filter data.265:	fld255 - packet data or filter data.in filter mode, there are 32 entry filter for Egress and 32 entry filter for Ingress.entry filters are assigned as follow:.tx_entry00 = {fld03, fld02, fld01, fld00};.tx_entry01 = {fld07, fld06, fld05, fld04};.......rx_entry30 = {fld123, fld122, fld121, fld120};.rx_entry31 = {fld127, fld126, fld125, fld124};.rx_entry00 = {fld131, fld130, fld129, fld128};.rx_entry01 = {fld135, fld134, fld133, fld132};.......rx_entry30 = {fld251, fld250, fld249, fld248};.rx_entry31 = {fld255, fld254, fld253, fld252};.Entry definition.valid_rg        = *x_entry*[63];     - 1= valid.act_rg          = *x_entry*[62:61]; .- 0= no action.- 1= ts inband36 .- 2= ts inband32 with inband filter.- 3= mdio gen1 mode.macip_flag_rg   = *x_entry*[54];     - 1=indicate mac address 0=indicate ip address.dst_flag_rg     = *x_entry*[53];     - 1=check dest address.src_flag_rg     = *x_entry*[52];     - 1=check src address.msgtype_rg      = *x_entry*[51:48];  - use used.ip_addr_rg      = *x_entry*[47:0];   - MAC or IP address..
 *     PKGEN_REPEAT     1 = enable to continously send packets
 *     PKGEN_ENABLE     1 = switch traffic mux to select traffic from packet generator.
 *     PKGEN_AUTOINC    1 = enable auto increment.
 *     PKGEN_SEND       1 = trigger to send packet.
 *     PKGEN_LOAD_INDEX 1 = load both index for indirector mode .
 *
 ******************************************************************************/
#define PLP_P1588_PKGEN_CONTROLr    0x000000c7

#define PLP_P1588_PKGEN_CONTROLr_SIZE 4

/*
 * This structure should be used to declare and program P1588_PKGEN_CONTROL.
 *
 */
typedef union PLP_P1588_PKGEN_CONTROLr_s {
	uint32_t v[1];
	uint32_t p1588_pkgen_control[1];
	uint32_t _p1588_pkgen_control;
} PLP_P1588_PKGEN_CONTROLr_t;

#define PLP_P1588_PKGEN_CONTROLr_CLR(r) (r).p1588_pkgen_control[0] = 0
#define PLP_P1588_PKGEN_CONTROLr_SET(r,d) (r).p1588_pkgen_control[0] = d
#define PLP_P1588_PKGEN_CONTROLr_GET(r) (r).p1588_pkgen_control[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PKGEN_CONTROLr_PKGEN_LOAD_INDEXf_GET(r) ((((r).p1588_pkgen_control[0]) >> 15) & 0x1)
#define PLP_P1588_PKGEN_CONTROLr_PKGEN_LOAD_INDEXf_SET(r,f) (r).p1588_pkgen_control[0]=(((r).p1588_pkgen_control[0] & ~((uint32_t)0x1 << 15)) | ((((uint32_t)f) & 0x1) << 15)) | (1 << (16 + 15))
#define PLP_P1588_PKGEN_CONTROLr_PKGEN_SENDf_GET(r) ((((r).p1588_pkgen_control[0]) >> 14) & 0x1)
#define PLP_P1588_PKGEN_CONTROLr_PKGEN_SENDf_SET(r,f) (r).p1588_pkgen_control[0]=(((r).p1588_pkgen_control[0] & ~((uint32_t)0x1 << 14)) | ((((uint32_t)f) & 0x1) << 14)) | (1 << (16 + 14))
#define PLP_P1588_PKGEN_CONTROLr_PKGEN_AUTOINCf_GET(r) ((((r).p1588_pkgen_control[0]) >> 13) & 0x1)
#define PLP_P1588_PKGEN_CONTROLr_PKGEN_AUTOINCf_SET(r,f) (r).p1588_pkgen_control[0]=(((r).p1588_pkgen_control[0] & ~((uint32_t)0x1 << 13)) | ((((uint32_t)f) & 0x1) << 13)) | (1 << (16 + 13))
#define PLP_P1588_PKGEN_CONTROLr_PKGEN_ENABLEf_GET(r) ((((r).p1588_pkgen_control[0]) >> 12) & 0x1)
#define PLP_P1588_PKGEN_CONTROLr_PKGEN_ENABLEf_SET(r,f) (r).p1588_pkgen_control[0]=(((r).p1588_pkgen_control[0] & ~((uint32_t)0x1 << 12)) | ((((uint32_t)f) & 0x1) << 12)) | (1 << (16 + 12))
#define PLP_P1588_PKGEN_CONTROLr_PKGEN_REPEATf_GET(r) ((((r).p1588_pkgen_control[0]) >> 11) & 0x1)
#define PLP_P1588_PKGEN_CONTROLr_PKGEN_REPEATf_SET(r,f) (r).p1588_pkgen_control[0]=(((r).p1588_pkgen_control[0] & ~((uint32_t)0x1 << 11)) | ((((uint32_t)f) & 0x1) << 11)) | (1 << (16 + 11))
#define PLP_P1588_PKGEN_CONTROLr_PKGEN_INDEXf_GET(r) (((r).p1588_pkgen_control[0]) & 0x1ff)
#define PLP_P1588_PKGEN_CONTROLr_PKGEN_INDEXf_SET(r,f) (r).p1588_pkgen_control[0]=(((r).p1588_pkgen_control[0] & ~((uint32_t)0x1ff)) | (((uint32_t)f) & 0x1ff)) | (0x1ff << 16)

/*
 * These macros can be used to access P1588_PKGEN_CONTROL.
 *
 */
#define PLP_READ_P1588_PKGEN_CONTROLr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PKGEN_CONTROLr,(_r._p1588_pkgen_control))
#define PLP_WRITE_P1588_PKGEN_CONTROLr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PKGEN_CONTROLr,(_r._p1588_pkgen_control))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PKGEN_CONTROLr PLP_P1588_PKGEN_CONTROLr
#define P1588_PKGEN_CONTROLr_SIZE PLP_P1588_PKGEN_CONTROLr_SIZE
typedef PLP_P1588_PKGEN_CONTROLr_t P1588_PKGEN_CONTROLr_t;
#define P1588_PKGEN_CONTROLr_CLR PLP_P1588_PKGEN_CONTROLr_CLR
#define P1588_PKGEN_CONTROLr_SET PLP_P1588_PKGEN_CONTROLr_SET
#define P1588_PKGEN_CONTROLr_GET PLP_P1588_PKGEN_CONTROLr_GET
#define P1588_PKGEN_CONTROLr_PKGEN_LOAD_INDEXf_GET PLP_P1588_PKGEN_CONTROLr_PKGEN_LOAD_INDEXf_GET
#define P1588_PKGEN_CONTROLr_PKGEN_LOAD_INDEXf_SET PLP_P1588_PKGEN_CONTROLr_PKGEN_LOAD_INDEXf_SET
#define P1588_PKGEN_CONTROLr_PKGEN_SENDf_GET PLP_P1588_PKGEN_CONTROLr_PKGEN_SENDf_GET
#define P1588_PKGEN_CONTROLr_PKGEN_SENDf_SET PLP_P1588_PKGEN_CONTROLr_PKGEN_SENDf_SET
#define P1588_PKGEN_CONTROLr_PKGEN_AUTOINCf_GET PLP_P1588_PKGEN_CONTROLr_PKGEN_AUTOINCf_GET
#define P1588_PKGEN_CONTROLr_PKGEN_AUTOINCf_SET PLP_P1588_PKGEN_CONTROLr_PKGEN_AUTOINCf_SET
#define P1588_PKGEN_CONTROLr_PKGEN_ENABLEf_GET PLP_P1588_PKGEN_CONTROLr_PKGEN_ENABLEf_GET
#define P1588_PKGEN_CONTROLr_PKGEN_ENABLEf_SET PLP_P1588_PKGEN_CONTROLr_PKGEN_ENABLEf_SET
#define P1588_PKGEN_CONTROLr_PKGEN_REPEATf_GET PLP_P1588_PKGEN_CONTROLr_PKGEN_REPEATf_GET
#define P1588_PKGEN_CONTROLr_PKGEN_REPEATf_SET PLP_P1588_PKGEN_CONTROLr_PKGEN_REPEATf_SET
#define P1588_PKGEN_CONTROLr_PKGEN_INDEXf_GET PLP_P1588_PKGEN_CONTROLr_PKGEN_INDEXf_GET
#define P1588_PKGEN_CONTROLr_PKGEN_INDEXf_SET PLP_P1588_PKGEN_CONTROLr_PKGEN_INDEXf_SET
#define READ_P1588_PKGEN_CONTROLr PLP_READ_P1588_PKGEN_CONTROLr
#define WRITE_P1588_PKGEN_CONTROLr PLP_WRITE_P1588_PKGEN_CONTROLr
#define MODIFY_P1588_PKGEN_CONTROLr PLP_MODIFY_P1588_PKGEN_CONTROLr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PKGEN_CONTROLr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PKGEN_WDATA
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70c8
 * DESC:     pkgen wdata Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PKGEN_WDATA      indirect write data.
 *
 ******************************************************************************/
#define PLP_P1588_PKGEN_WDATAr    0x000000c8

#define PLP_P1588_PKGEN_WDATAr_SIZE 4

/*
 * This structure should be used to declare and program P1588_PKGEN_WDATA.
 *
 */
typedef union PLP_P1588_PKGEN_WDATAr_s {
	uint32_t v[1];
	uint32_t p1588_pkgen_wdata[1];
	uint32_t _p1588_pkgen_wdata;
} PLP_P1588_PKGEN_WDATAr_t;

#define PLP_P1588_PKGEN_WDATAr_CLR(r) (r).p1588_pkgen_wdata[0] = 0
#define PLP_P1588_PKGEN_WDATAr_SET(r,d) (r).p1588_pkgen_wdata[0] = d
#define PLP_P1588_PKGEN_WDATAr_GET(r) (r).p1588_pkgen_wdata[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PKGEN_WDATAr_PKGEN_WDATAf_GET(r) (((r).p1588_pkgen_wdata[0]) & 0xffff)
#define PLP_P1588_PKGEN_WDATAr_PKGEN_WDATAf_SET(r,f) (r).p1588_pkgen_wdata[0]=(((r).p1588_pkgen_wdata[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PKGEN_WDATA.
 *
 */
#define PLP_READ_P1588_PKGEN_WDATAr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PKGEN_WDATAr,(_r._p1588_pkgen_wdata))
#define PLP_WRITE_P1588_PKGEN_WDATAr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PKGEN_WDATAr,(_r._p1588_pkgen_wdata))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PKGEN_WDATAr PLP_P1588_PKGEN_WDATAr
#define P1588_PKGEN_WDATAr_SIZE PLP_P1588_PKGEN_WDATAr_SIZE
typedef PLP_P1588_PKGEN_WDATAr_t P1588_PKGEN_WDATAr_t;
#define P1588_PKGEN_WDATAr_CLR PLP_P1588_PKGEN_WDATAr_CLR
#define P1588_PKGEN_WDATAr_SET PLP_P1588_PKGEN_WDATAr_SET
#define P1588_PKGEN_WDATAr_GET PLP_P1588_PKGEN_WDATAr_GET
#define P1588_PKGEN_WDATAr_PKGEN_WDATAf_GET PLP_P1588_PKGEN_WDATAr_PKGEN_WDATAf_GET
#define P1588_PKGEN_WDATAr_PKGEN_WDATAf_SET PLP_P1588_PKGEN_WDATAr_PKGEN_WDATAf_SET
#define READ_P1588_PKGEN_WDATAr PLP_READ_P1588_PKGEN_WDATAr
#define WRITE_P1588_PKGEN_WDATAr PLP_WRITE_P1588_PKGEN_WDATAr
#define MODIFY_P1588_PKGEN_WDATAr PLP_MODIFY_P1588_PKGEN_WDATAr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PKGEN_WDATAr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PKGEN_RDATA
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70c9
 * DESC:     pkgen rdata Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PKGEN_RDATA      indirect read data.
 *
 ******************************************************************************/
#define PLP_P1588_PKGEN_RDATAr    0x000000c9

#define PLP_P1588_PKGEN_RDATAr_SIZE 4

/*
 * This structure should be used to declare and program P1588_PKGEN_RDATA.
 *
 */
typedef union PLP_P1588_PKGEN_RDATAr_s {
	uint32_t v[1];
	uint32_t p1588_pkgen_rdata[1];
	uint32_t _p1588_pkgen_rdata;
} PLP_P1588_PKGEN_RDATAr_t;

#define PLP_P1588_PKGEN_RDATAr_CLR(r) (r).p1588_pkgen_rdata[0] = 0
#define PLP_P1588_PKGEN_RDATAr_SET(r,d) (r).p1588_pkgen_rdata[0] = d
#define PLP_P1588_PKGEN_RDATAr_GET(r) (r).p1588_pkgen_rdata[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PKGEN_RDATAr_PKGEN_RDATAf_GET(r) (((r).p1588_pkgen_rdata[0]) & 0xffff)
#define PLP_P1588_PKGEN_RDATAr_PKGEN_RDATAf_SET(r,f) (r).p1588_pkgen_rdata[0]=(((r).p1588_pkgen_rdata[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PKGEN_RDATA.
 *
 */
#define PLP_READ_P1588_PKGEN_RDATAr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PKGEN_RDATAr,(_r._p1588_pkgen_rdata))
#define PLP_WRITE_P1588_PKGEN_RDATAr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PKGEN_RDATAr,(_r._p1588_pkgen_rdata))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PKGEN_RDATAr PLP_P1588_PKGEN_RDATAr
#define P1588_PKGEN_RDATAr_SIZE PLP_P1588_PKGEN_RDATAr_SIZE
typedef PLP_P1588_PKGEN_RDATAr_t P1588_PKGEN_RDATAr_t;
#define P1588_PKGEN_RDATAr_CLR PLP_P1588_PKGEN_RDATAr_CLR
#define P1588_PKGEN_RDATAr_SET PLP_P1588_PKGEN_RDATAr_SET
#define P1588_PKGEN_RDATAr_GET PLP_P1588_PKGEN_RDATAr_GET
#define P1588_PKGEN_RDATAr_PKGEN_RDATAf_GET PLP_P1588_PKGEN_RDATAr_PKGEN_RDATAf_GET
#define P1588_PKGEN_RDATAr_PKGEN_RDATAf_SET PLP_P1588_PKGEN_RDATAr_PKGEN_RDATAf_SET
#define READ_P1588_PKGEN_RDATAr PLP_READ_P1588_PKGEN_RDATAr
#define WRITE_P1588_PKGEN_RDATAr PLP_WRITE_P1588_PKGEN_RDATAr
#define MODIFY_P1588_PKGEN_RDATAr PLP_MODIFY_P1588_PKGEN_RDATAr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PKGEN_RDATAr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TX_IETF_ETHERTYPE10_OFFSET
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70ca
 * DESC:     P1588 TX IETF Ethertype10 Offset Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_IETF_ETHERTYPE10_OFFSET Only bits [5:0] are used. Bits [5:0] indicate the byte number of the timestamp after the IETF header.The numbering starts from 0.
 *
 ******************************************************************************/
#define PLP_P1588_TX_IETF_ETHERTYPE10_OFFSETr    0x000000ca

#define PLP_P1588_TX_IETF_ETHERTYPE10_OFFSETr_SIZE 4

/*
 * This structure should be used to declare and program P1588_TX_IETF_ETHERTYPE10_OFFSET.
 *
 */
typedef union PLP_P1588_TX_IETF_ETHERTYPE10_OFFSETr_s {
	uint32_t v[1];
	uint32_t p1588_tx_ietf_ethertype10_offset[1];
	uint32_t _p1588_tx_ietf_ethertype10_offset;
} PLP_P1588_TX_IETF_ETHERTYPE10_OFFSETr_t;

#define PLP_P1588_TX_IETF_ETHERTYPE10_OFFSETr_CLR(r) (r).p1588_tx_ietf_ethertype10_offset[0] = 0
#define PLP_P1588_TX_IETF_ETHERTYPE10_OFFSETr_SET(r,d) (r).p1588_tx_ietf_ethertype10_offset[0] = d
#define PLP_P1588_TX_IETF_ETHERTYPE10_OFFSETr_GET(r) (r).p1588_tx_ietf_ethertype10_offset[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TX_IETF_ETHERTYPE10_OFFSETr_TX_IETF_ETHERTYPE10_OFFSETf_GET(r) (((r).p1588_tx_ietf_ethertype10_offset[0]) & 0xffff)
#define PLP_P1588_TX_IETF_ETHERTYPE10_OFFSETr_TX_IETF_ETHERTYPE10_OFFSETf_SET(r,f) (r).p1588_tx_ietf_ethertype10_offset[0]=(((r).p1588_tx_ietf_ethertype10_offset[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TX_IETF_ETHERTYPE10_OFFSET.
 *
 */
#define PLP_READ_P1588_TX_IETF_ETHERTYPE10_OFFSETr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TX_IETF_ETHERTYPE10_OFFSETr,(_r._p1588_tx_ietf_ethertype10_offset))
#define PLP_WRITE_P1588_TX_IETF_ETHERTYPE10_OFFSETr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TX_IETF_ETHERTYPE10_OFFSETr,(_r._p1588_tx_ietf_ethertype10_offset))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TX_IETF_ETHERTYPE10_OFFSETr PLP_P1588_TX_IETF_ETHERTYPE10_OFFSETr
#define P1588_TX_IETF_ETHERTYPE10_OFFSETr_SIZE PLP_P1588_TX_IETF_ETHERTYPE10_OFFSETr_SIZE
typedef PLP_P1588_TX_IETF_ETHERTYPE10_OFFSETr_t P1588_TX_IETF_ETHERTYPE10_OFFSETr_t;
#define P1588_TX_IETF_ETHERTYPE10_OFFSETr_CLR PLP_P1588_TX_IETF_ETHERTYPE10_OFFSETr_CLR
#define P1588_TX_IETF_ETHERTYPE10_OFFSETr_SET PLP_P1588_TX_IETF_ETHERTYPE10_OFFSETr_SET
#define P1588_TX_IETF_ETHERTYPE10_OFFSETr_GET PLP_P1588_TX_IETF_ETHERTYPE10_OFFSETr_GET
#define P1588_TX_IETF_ETHERTYPE10_OFFSETr_TX_IETF_ETHERTYPE10_OFFSETf_GET PLP_P1588_TX_IETF_ETHERTYPE10_OFFSETr_TX_IETF_ETHERTYPE10_OFFSETf_GET
#define P1588_TX_IETF_ETHERTYPE10_OFFSETr_TX_IETF_ETHERTYPE10_OFFSETf_SET PLP_P1588_TX_IETF_ETHERTYPE10_OFFSETr_TX_IETF_ETHERTYPE10_OFFSETf_SET
#define READ_P1588_TX_IETF_ETHERTYPE10_OFFSETr PLP_READ_P1588_TX_IETF_ETHERTYPE10_OFFSETr
#define WRITE_P1588_TX_IETF_ETHERTYPE10_OFFSETr PLP_WRITE_P1588_TX_IETF_ETHERTYPE10_OFFSETr
#define MODIFY_P1588_TX_IETF_ETHERTYPE10_OFFSETr PLP_MODIFY_P1588_TX_IETF_ETHERTYPE10_OFFSETr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TX_IETF_ETHERTYPE10_OFFSETr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TX_IETF_ETHERTYPE14_OFFSET
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70cb
 * DESC:     P1588 TX IETF Ethertype14 Offset Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_IETF_ETHERTYPE14_OFFSET Only bits [5:0] are used. Bits [5:0] indicate the byte number of the timestamp after the IETF header.The numbering starts from 0.
 *
 ******************************************************************************/
#define PLP_P1588_TX_IETF_ETHERTYPE14_OFFSETr    0x000000cb

#define PLP_P1588_TX_IETF_ETHERTYPE14_OFFSETr_SIZE 4

/*
 * This structure should be used to declare and program P1588_TX_IETF_ETHERTYPE14_OFFSET.
 *
 */
typedef union PLP_P1588_TX_IETF_ETHERTYPE14_OFFSETr_s {
	uint32_t v[1];
	uint32_t p1588_tx_ietf_ethertype14_offset[1];
	uint32_t _p1588_tx_ietf_ethertype14_offset;
} PLP_P1588_TX_IETF_ETHERTYPE14_OFFSETr_t;

#define PLP_P1588_TX_IETF_ETHERTYPE14_OFFSETr_CLR(r) (r).p1588_tx_ietf_ethertype14_offset[0] = 0
#define PLP_P1588_TX_IETF_ETHERTYPE14_OFFSETr_SET(r,d) (r).p1588_tx_ietf_ethertype14_offset[0] = d
#define PLP_P1588_TX_IETF_ETHERTYPE14_OFFSETr_GET(r) (r).p1588_tx_ietf_ethertype14_offset[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TX_IETF_ETHERTYPE14_OFFSETr_TX_IETF_ETHERTYPE14_OFFSETf_GET(r) (((r).p1588_tx_ietf_ethertype14_offset[0]) & 0xffff)
#define PLP_P1588_TX_IETF_ETHERTYPE14_OFFSETr_TX_IETF_ETHERTYPE14_OFFSETf_SET(r,f) (r).p1588_tx_ietf_ethertype14_offset[0]=(((r).p1588_tx_ietf_ethertype14_offset[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TX_IETF_ETHERTYPE14_OFFSET.
 *
 */
#define PLP_READ_P1588_TX_IETF_ETHERTYPE14_OFFSETr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TX_IETF_ETHERTYPE14_OFFSETr,(_r._p1588_tx_ietf_ethertype14_offset))
#define PLP_WRITE_P1588_TX_IETF_ETHERTYPE14_OFFSETr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TX_IETF_ETHERTYPE14_OFFSETr,(_r._p1588_tx_ietf_ethertype14_offset))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TX_IETF_ETHERTYPE14_OFFSETr PLP_P1588_TX_IETF_ETHERTYPE14_OFFSETr
#define P1588_TX_IETF_ETHERTYPE14_OFFSETr_SIZE PLP_P1588_TX_IETF_ETHERTYPE14_OFFSETr_SIZE
typedef PLP_P1588_TX_IETF_ETHERTYPE14_OFFSETr_t P1588_TX_IETF_ETHERTYPE14_OFFSETr_t;
#define P1588_TX_IETF_ETHERTYPE14_OFFSETr_CLR PLP_P1588_TX_IETF_ETHERTYPE14_OFFSETr_CLR
#define P1588_TX_IETF_ETHERTYPE14_OFFSETr_SET PLP_P1588_TX_IETF_ETHERTYPE14_OFFSETr_SET
#define P1588_TX_IETF_ETHERTYPE14_OFFSETr_GET PLP_P1588_TX_IETF_ETHERTYPE14_OFFSETr_GET
#define P1588_TX_IETF_ETHERTYPE14_OFFSETr_TX_IETF_ETHERTYPE14_OFFSETf_GET PLP_P1588_TX_IETF_ETHERTYPE14_OFFSETr_TX_IETF_ETHERTYPE14_OFFSETf_GET
#define P1588_TX_IETF_ETHERTYPE14_OFFSETr_TX_IETF_ETHERTYPE14_OFFSETf_SET PLP_P1588_TX_IETF_ETHERTYPE14_OFFSETr_TX_IETF_ETHERTYPE14_OFFSETf_SET
#define READ_P1588_TX_IETF_ETHERTYPE14_OFFSETr PLP_READ_P1588_TX_IETF_ETHERTYPE14_OFFSETr
#define WRITE_P1588_TX_IETF_ETHERTYPE14_OFFSETr PLP_WRITE_P1588_TX_IETF_ETHERTYPE14_OFFSETr
#define MODIFY_P1588_TX_IETF_ETHERTYPE14_OFFSETr PLP_MODIFY_P1588_TX_IETF_ETHERTYPE14_OFFSETr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TX_IETF_ETHERTYPE14_OFFSETr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TX_IETF_ETHERTYPE15_OFFSET
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70cc
 * DESC:     P1588 TX IETF Ethertype15 Offset Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_IETF_ETHERTYPE15_OFFSET Only bits [5:0] are used. Bits [5:0] indicate the byte number of the timestamp after the IETF header.The numbering starts from 0.
 *
 ******************************************************************************/
#define PLP_P1588_TX_IETF_ETHERTYPE15_OFFSETr    0x000000cc

#define PLP_P1588_TX_IETF_ETHERTYPE15_OFFSETr_SIZE 4

/*
 * This structure should be used to declare and program P1588_TX_IETF_ETHERTYPE15_OFFSET.
 *
 */
typedef union PLP_P1588_TX_IETF_ETHERTYPE15_OFFSETr_s {
	uint32_t v[1];
	uint32_t p1588_tx_ietf_ethertype15_offset[1];
	uint32_t _p1588_tx_ietf_ethertype15_offset;
} PLP_P1588_TX_IETF_ETHERTYPE15_OFFSETr_t;

#define PLP_P1588_TX_IETF_ETHERTYPE15_OFFSETr_CLR(r) (r).p1588_tx_ietf_ethertype15_offset[0] = 0
#define PLP_P1588_TX_IETF_ETHERTYPE15_OFFSETr_SET(r,d) (r).p1588_tx_ietf_ethertype15_offset[0] = d
#define PLP_P1588_TX_IETF_ETHERTYPE15_OFFSETr_GET(r) (r).p1588_tx_ietf_ethertype15_offset[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TX_IETF_ETHERTYPE15_OFFSETr_TX_IETF_ETHERTYPE15_OFFSETf_GET(r) (((r).p1588_tx_ietf_ethertype15_offset[0]) & 0xffff)
#define PLP_P1588_TX_IETF_ETHERTYPE15_OFFSETr_TX_IETF_ETHERTYPE15_OFFSETf_SET(r,f) (r).p1588_tx_ietf_ethertype15_offset[0]=(((r).p1588_tx_ietf_ethertype15_offset[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TX_IETF_ETHERTYPE15_OFFSET.
 *
 */
#define PLP_READ_P1588_TX_IETF_ETHERTYPE15_OFFSETr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TX_IETF_ETHERTYPE15_OFFSETr,(_r._p1588_tx_ietf_ethertype15_offset))
#define PLP_WRITE_P1588_TX_IETF_ETHERTYPE15_OFFSETr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TX_IETF_ETHERTYPE15_OFFSETr,(_r._p1588_tx_ietf_ethertype15_offset))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TX_IETF_ETHERTYPE15_OFFSETr PLP_P1588_TX_IETF_ETHERTYPE15_OFFSETr
#define P1588_TX_IETF_ETHERTYPE15_OFFSETr_SIZE PLP_P1588_TX_IETF_ETHERTYPE15_OFFSETr_SIZE
typedef PLP_P1588_TX_IETF_ETHERTYPE15_OFFSETr_t P1588_TX_IETF_ETHERTYPE15_OFFSETr_t;
#define P1588_TX_IETF_ETHERTYPE15_OFFSETr_CLR PLP_P1588_TX_IETF_ETHERTYPE15_OFFSETr_CLR
#define P1588_TX_IETF_ETHERTYPE15_OFFSETr_SET PLP_P1588_TX_IETF_ETHERTYPE15_OFFSETr_SET
#define P1588_TX_IETF_ETHERTYPE15_OFFSETr_GET PLP_P1588_TX_IETF_ETHERTYPE15_OFFSETr_GET
#define P1588_TX_IETF_ETHERTYPE15_OFFSETr_TX_IETF_ETHERTYPE15_OFFSETf_GET PLP_P1588_TX_IETF_ETHERTYPE15_OFFSETr_TX_IETF_ETHERTYPE15_OFFSETf_GET
#define P1588_TX_IETF_ETHERTYPE15_OFFSETr_TX_IETF_ETHERTYPE15_OFFSETf_SET PLP_P1588_TX_IETF_ETHERTYPE15_OFFSETr_TX_IETF_ETHERTYPE15_OFFSETf_SET
#define READ_P1588_TX_IETF_ETHERTYPE15_OFFSETr PLP_READ_P1588_TX_IETF_ETHERTYPE15_OFFSETr
#define WRITE_P1588_TX_IETF_ETHERTYPE15_OFFSETr PLP_WRITE_P1588_TX_IETF_ETHERTYPE15_OFFSETr
#define MODIFY_P1588_TX_IETF_ETHERTYPE15_OFFSETr PLP_MODIFY_P1588_TX_IETF_ETHERTYPE15_OFFSETr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TX_IETF_ETHERTYPE15_OFFSETr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_RX_IETF_ETHERTYPE10_OFFSET
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70cd
 * DESC:     P1588 RX IETF Ethertype10 Offset Register
 * RESETVAL: 0x8 (8)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_IETF_ETHERTYPE10_OFFSET Only bits [5:0] are used. Bits [5:0] indicate the byte number of the timestamp after the IETF header.The numbering starts from 0.
 *
 ******************************************************************************/
#define PLP_P1588_RX_IETF_ETHERTYPE10_OFFSETr    0x000000cd

#define PLP_P1588_RX_IETF_ETHERTYPE10_OFFSETr_SIZE 4

/*
 * This structure should be used to declare and program P1588_RX_IETF_ETHERTYPE10_OFFSET.
 *
 */
typedef union PLP_P1588_RX_IETF_ETHERTYPE10_OFFSETr_s {
	uint32_t v[1];
	uint32_t p1588_rx_ietf_ethertype10_offset[1];
	uint32_t _p1588_rx_ietf_ethertype10_offset;
} PLP_P1588_RX_IETF_ETHERTYPE10_OFFSETr_t;

#define PLP_P1588_RX_IETF_ETHERTYPE10_OFFSETr_CLR(r) (r).p1588_rx_ietf_ethertype10_offset[0] = 0
#define PLP_P1588_RX_IETF_ETHERTYPE10_OFFSETr_SET(r,d) (r).p1588_rx_ietf_ethertype10_offset[0] = d
#define PLP_P1588_RX_IETF_ETHERTYPE10_OFFSETr_GET(r) (r).p1588_rx_ietf_ethertype10_offset[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_RX_IETF_ETHERTYPE10_OFFSETr_RX_IETF_ETHERTYPE10_OFFSETf_GET(r) (((r).p1588_rx_ietf_ethertype10_offset[0]) & 0xffff)
#define PLP_P1588_RX_IETF_ETHERTYPE10_OFFSETr_RX_IETF_ETHERTYPE10_OFFSETf_SET(r,f) (r).p1588_rx_ietf_ethertype10_offset[0]=(((r).p1588_rx_ietf_ethertype10_offset[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_RX_IETF_ETHERTYPE10_OFFSET.
 *
 */
#define PLP_READ_P1588_RX_IETF_ETHERTYPE10_OFFSETr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_RX_IETF_ETHERTYPE10_OFFSETr,(_r._p1588_rx_ietf_ethertype10_offset))
#define PLP_WRITE_P1588_RX_IETF_ETHERTYPE10_OFFSETr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_RX_IETF_ETHERTYPE10_OFFSETr,(_r._p1588_rx_ietf_ethertype10_offset))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_RX_IETF_ETHERTYPE10_OFFSETr PLP_P1588_RX_IETF_ETHERTYPE10_OFFSETr
#define P1588_RX_IETF_ETHERTYPE10_OFFSETr_SIZE PLP_P1588_RX_IETF_ETHERTYPE10_OFFSETr_SIZE
typedef PLP_P1588_RX_IETF_ETHERTYPE10_OFFSETr_t P1588_RX_IETF_ETHERTYPE10_OFFSETr_t;
#define P1588_RX_IETF_ETHERTYPE10_OFFSETr_CLR PLP_P1588_RX_IETF_ETHERTYPE10_OFFSETr_CLR
#define P1588_RX_IETF_ETHERTYPE10_OFFSETr_SET PLP_P1588_RX_IETF_ETHERTYPE10_OFFSETr_SET
#define P1588_RX_IETF_ETHERTYPE10_OFFSETr_GET PLP_P1588_RX_IETF_ETHERTYPE10_OFFSETr_GET
#define P1588_RX_IETF_ETHERTYPE10_OFFSETr_RX_IETF_ETHERTYPE10_OFFSETf_GET PLP_P1588_RX_IETF_ETHERTYPE10_OFFSETr_RX_IETF_ETHERTYPE10_OFFSETf_GET
#define P1588_RX_IETF_ETHERTYPE10_OFFSETr_RX_IETF_ETHERTYPE10_OFFSETf_SET PLP_P1588_RX_IETF_ETHERTYPE10_OFFSETr_RX_IETF_ETHERTYPE10_OFFSETf_SET
#define READ_P1588_RX_IETF_ETHERTYPE10_OFFSETr PLP_READ_P1588_RX_IETF_ETHERTYPE10_OFFSETr
#define WRITE_P1588_RX_IETF_ETHERTYPE10_OFFSETr PLP_WRITE_P1588_RX_IETF_ETHERTYPE10_OFFSETr
#define MODIFY_P1588_RX_IETF_ETHERTYPE10_OFFSETr PLP_MODIFY_P1588_RX_IETF_ETHERTYPE10_OFFSETr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_RX_IETF_ETHERTYPE10_OFFSETr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_RX_IETF_ETHERTYPE14_OFFSET
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70ce
 * DESC:     P1588 RX IETF Ethertype14 Offset Register
 * RESETVAL: 0x8 (8)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_IETF_ETHERTYPE14_OFFSET Only bits [5:0] are used. Bits [5:0] indicate the byte number of the timestamp after the IETF header.The numbering starts from 0.
 *
 ******************************************************************************/
#define PLP_P1588_RX_IETF_ETHERTYPE14_OFFSETr    0x000000ce

#define PLP_P1588_RX_IETF_ETHERTYPE14_OFFSETr_SIZE 4

/*
 * This structure should be used to declare and program P1588_RX_IETF_ETHERTYPE14_OFFSET.
 *
 */
typedef union PLP_P1588_RX_IETF_ETHERTYPE14_OFFSETr_s {
	uint32_t v[1];
	uint32_t p1588_rx_ietf_ethertype14_offset[1];
	uint32_t _p1588_rx_ietf_ethertype14_offset;
} PLP_P1588_RX_IETF_ETHERTYPE14_OFFSETr_t;

#define PLP_P1588_RX_IETF_ETHERTYPE14_OFFSETr_CLR(r) (r).p1588_rx_ietf_ethertype14_offset[0] = 0
#define PLP_P1588_RX_IETF_ETHERTYPE14_OFFSETr_SET(r,d) (r).p1588_rx_ietf_ethertype14_offset[0] = d
#define PLP_P1588_RX_IETF_ETHERTYPE14_OFFSETr_GET(r) (r).p1588_rx_ietf_ethertype14_offset[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_RX_IETF_ETHERTYPE14_OFFSETr_RX_IETF_ETHERTYPE14_OFFSETf_GET(r) (((r).p1588_rx_ietf_ethertype14_offset[0]) & 0xffff)
#define PLP_P1588_RX_IETF_ETHERTYPE14_OFFSETr_RX_IETF_ETHERTYPE14_OFFSETf_SET(r,f) (r).p1588_rx_ietf_ethertype14_offset[0]=(((r).p1588_rx_ietf_ethertype14_offset[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_RX_IETF_ETHERTYPE14_OFFSET.
 *
 */
#define PLP_READ_P1588_RX_IETF_ETHERTYPE14_OFFSETr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_RX_IETF_ETHERTYPE14_OFFSETr,(_r._p1588_rx_ietf_ethertype14_offset))
#define PLP_WRITE_P1588_RX_IETF_ETHERTYPE14_OFFSETr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_RX_IETF_ETHERTYPE14_OFFSETr,(_r._p1588_rx_ietf_ethertype14_offset))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_RX_IETF_ETHERTYPE14_OFFSETr PLP_P1588_RX_IETF_ETHERTYPE14_OFFSETr
#define P1588_RX_IETF_ETHERTYPE14_OFFSETr_SIZE PLP_P1588_RX_IETF_ETHERTYPE14_OFFSETr_SIZE
typedef PLP_P1588_RX_IETF_ETHERTYPE14_OFFSETr_t P1588_RX_IETF_ETHERTYPE14_OFFSETr_t;
#define P1588_RX_IETF_ETHERTYPE14_OFFSETr_CLR PLP_P1588_RX_IETF_ETHERTYPE14_OFFSETr_CLR
#define P1588_RX_IETF_ETHERTYPE14_OFFSETr_SET PLP_P1588_RX_IETF_ETHERTYPE14_OFFSETr_SET
#define P1588_RX_IETF_ETHERTYPE14_OFFSETr_GET PLP_P1588_RX_IETF_ETHERTYPE14_OFFSETr_GET
#define P1588_RX_IETF_ETHERTYPE14_OFFSETr_RX_IETF_ETHERTYPE14_OFFSETf_GET PLP_P1588_RX_IETF_ETHERTYPE14_OFFSETr_RX_IETF_ETHERTYPE14_OFFSETf_GET
#define P1588_RX_IETF_ETHERTYPE14_OFFSETr_RX_IETF_ETHERTYPE14_OFFSETf_SET PLP_P1588_RX_IETF_ETHERTYPE14_OFFSETr_RX_IETF_ETHERTYPE14_OFFSETf_SET
#define READ_P1588_RX_IETF_ETHERTYPE14_OFFSETr PLP_READ_P1588_RX_IETF_ETHERTYPE14_OFFSETr
#define WRITE_P1588_RX_IETF_ETHERTYPE14_OFFSETr PLP_WRITE_P1588_RX_IETF_ETHERTYPE14_OFFSETr
#define MODIFY_P1588_RX_IETF_ETHERTYPE14_OFFSETr PLP_MODIFY_P1588_RX_IETF_ETHERTYPE14_OFFSETr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_RX_IETF_ETHERTYPE14_OFFSETr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_RX_IETF_ETHERTYPE15_OFFSET
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70cf
 * DESC:     P1588 RX IETF Ethertype15 Offset Register
 * RESETVAL: 0x8 (8)
 * ACCESS:   R/W
 * FIELDS:
 *     RX_IETF_ETHERTYPE15_OFFSET Only bits [5:0] are used. Bits [5:0] indicate the byte number of the timestamp after the IETF header.The numbering starts from 0.
 *
 ******************************************************************************/
#define PLP_P1588_RX_IETF_ETHERTYPE15_OFFSETr    0x000000cf

#define PLP_P1588_RX_IETF_ETHERTYPE15_OFFSETr_SIZE 4

/*
 * This structure should be used to declare and program P1588_RX_IETF_ETHERTYPE15_OFFSET.
 *
 */
typedef union PLP_P1588_RX_IETF_ETHERTYPE15_OFFSETr_s {
	uint32_t v[1];
	uint32_t p1588_rx_ietf_ethertype15_offset[1];
	uint32_t _p1588_rx_ietf_ethertype15_offset;
} PLP_P1588_RX_IETF_ETHERTYPE15_OFFSETr_t;

#define PLP_P1588_RX_IETF_ETHERTYPE15_OFFSETr_CLR(r) (r).p1588_rx_ietf_ethertype15_offset[0] = 0
#define PLP_P1588_RX_IETF_ETHERTYPE15_OFFSETr_SET(r,d) (r).p1588_rx_ietf_ethertype15_offset[0] = d
#define PLP_P1588_RX_IETF_ETHERTYPE15_OFFSETr_GET(r) (r).p1588_rx_ietf_ethertype15_offset[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_RX_IETF_ETHERTYPE15_OFFSETr_RX_IETF_ETHERTYPE15_OFFSETf_GET(r) (((r).p1588_rx_ietf_ethertype15_offset[0]) & 0xffff)
#define PLP_P1588_RX_IETF_ETHERTYPE15_OFFSETr_RX_IETF_ETHERTYPE15_OFFSETf_SET(r,f) (r).p1588_rx_ietf_ethertype15_offset[0]=(((r).p1588_rx_ietf_ethertype15_offset[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_RX_IETF_ETHERTYPE15_OFFSET.
 *
 */
#define PLP_READ_P1588_RX_IETF_ETHERTYPE15_OFFSETr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_RX_IETF_ETHERTYPE15_OFFSETr,(_r._p1588_rx_ietf_ethertype15_offset))
#define PLP_WRITE_P1588_RX_IETF_ETHERTYPE15_OFFSETr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_RX_IETF_ETHERTYPE15_OFFSETr,(_r._p1588_rx_ietf_ethertype15_offset))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_RX_IETF_ETHERTYPE15_OFFSETr PLP_P1588_RX_IETF_ETHERTYPE15_OFFSETr
#define P1588_RX_IETF_ETHERTYPE15_OFFSETr_SIZE PLP_P1588_RX_IETF_ETHERTYPE15_OFFSETr_SIZE
typedef PLP_P1588_RX_IETF_ETHERTYPE15_OFFSETr_t P1588_RX_IETF_ETHERTYPE15_OFFSETr_t;
#define P1588_RX_IETF_ETHERTYPE15_OFFSETr_CLR PLP_P1588_RX_IETF_ETHERTYPE15_OFFSETr_CLR
#define P1588_RX_IETF_ETHERTYPE15_OFFSETr_SET PLP_P1588_RX_IETF_ETHERTYPE15_OFFSETr_SET
#define P1588_RX_IETF_ETHERTYPE15_OFFSETr_GET PLP_P1588_RX_IETF_ETHERTYPE15_OFFSETr_GET
#define P1588_RX_IETF_ETHERTYPE15_OFFSETr_RX_IETF_ETHERTYPE15_OFFSETf_GET PLP_P1588_RX_IETF_ETHERTYPE15_OFFSETr_RX_IETF_ETHERTYPE15_OFFSETf_GET
#define P1588_RX_IETF_ETHERTYPE15_OFFSETr_RX_IETF_ETHERTYPE15_OFFSETf_SET PLP_P1588_RX_IETF_ETHERTYPE15_OFFSETr_RX_IETF_ETHERTYPE15_OFFSETf_SET
#define READ_P1588_RX_IETF_ETHERTYPE15_OFFSETr PLP_READ_P1588_RX_IETF_ETHERTYPE15_OFFSETr
#define WRITE_P1588_RX_IETF_ETHERTYPE15_OFFSETr PLP_WRITE_P1588_RX_IETF_ETHERTYPE15_OFFSETr
#define MODIFY_P1588_RX_IETF_ETHERTYPE15_OFFSETr PLP_MODIFY_P1588_RX_IETF_ETHERTYPE15_OFFSETr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_RX_IETF_ETHERTYPE15_OFFSETr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_ENTRY_WORD00
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70d0
 * DESC:     SOPMEM ENTRY WORD 0 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_ENTRY_WORD00 SOPmem Capture - {ts[15:0]}
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_ENTRY_WORD00r    0x000000d0

#define PLP_P1588_SOPMEM_ENTRY_WORD00r_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_ENTRY_WORD00.
 *
 */
typedef union PLP_P1588_SOPMEM_ENTRY_WORD00r_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_entry_word00[1];
	uint32_t _p1588_sopmem_entry_word00;
} PLP_P1588_SOPMEM_ENTRY_WORD00r_t;

#define PLP_P1588_SOPMEM_ENTRY_WORD00r_CLR(r) (r).p1588_sopmem_entry_word00[0] = 0
#define PLP_P1588_SOPMEM_ENTRY_WORD00r_SET(r,d) (r).p1588_sopmem_entry_word00[0] = d
#define PLP_P1588_SOPMEM_ENTRY_WORD00r_GET(r) (r).p1588_sopmem_entry_word00[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_ENTRY_WORD00r_SOPMEM_ENTRY_WORD00f_GET(r) (((r).p1588_sopmem_entry_word00[0]) & 0xffff)
#define PLP_P1588_SOPMEM_ENTRY_WORD00r_SOPMEM_ENTRY_WORD00f_SET(r,f) (r).p1588_sopmem_entry_word00[0]=(((r).p1588_sopmem_entry_word00[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_SOPMEM_ENTRY_WORD00.
 *
 */
#define PLP_READ_P1588_SOPMEM_ENTRY_WORD00r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_ENTRY_WORD00r,(_r._p1588_sopmem_entry_word00))
#define PLP_WRITE_P1588_SOPMEM_ENTRY_WORD00r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_ENTRY_WORD00r,(_r._p1588_sopmem_entry_word00))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_ENTRY_WORD00r PLP_P1588_SOPMEM_ENTRY_WORD00r
#define P1588_SOPMEM_ENTRY_WORD00r_SIZE PLP_P1588_SOPMEM_ENTRY_WORD00r_SIZE
typedef PLP_P1588_SOPMEM_ENTRY_WORD00r_t P1588_SOPMEM_ENTRY_WORD00r_t;
#define P1588_SOPMEM_ENTRY_WORD00r_CLR PLP_P1588_SOPMEM_ENTRY_WORD00r_CLR
#define P1588_SOPMEM_ENTRY_WORD00r_SET PLP_P1588_SOPMEM_ENTRY_WORD00r_SET
#define P1588_SOPMEM_ENTRY_WORD00r_GET PLP_P1588_SOPMEM_ENTRY_WORD00r_GET
#define P1588_SOPMEM_ENTRY_WORD00r_SOPMEM_ENTRY_WORD00f_GET PLP_P1588_SOPMEM_ENTRY_WORD00r_SOPMEM_ENTRY_WORD00f_GET
#define P1588_SOPMEM_ENTRY_WORD00r_SOPMEM_ENTRY_WORD00f_SET PLP_P1588_SOPMEM_ENTRY_WORD00r_SOPMEM_ENTRY_WORD00f_SET
#define READ_P1588_SOPMEM_ENTRY_WORD00r PLP_READ_P1588_SOPMEM_ENTRY_WORD00r
#define WRITE_P1588_SOPMEM_ENTRY_WORD00r PLP_WRITE_P1588_SOPMEM_ENTRY_WORD00r
#define MODIFY_P1588_SOPMEM_ENTRY_WORD00r PLP_MODIFY_P1588_SOPMEM_ENTRY_WORD00r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_ENTRY_WORD00r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_ENTRY_WORD01
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70d1
 * DESC:     SOPMEM ENTRY WORD 1 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_ENTRY_WORD01 SOPmem Capture - {ts[31:16]}
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_ENTRY_WORD01r    0x000000d1

#define PLP_P1588_SOPMEM_ENTRY_WORD01r_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_ENTRY_WORD01.
 *
 */
typedef union PLP_P1588_SOPMEM_ENTRY_WORD01r_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_entry_word01[1];
	uint32_t _p1588_sopmem_entry_word01;
} PLP_P1588_SOPMEM_ENTRY_WORD01r_t;

#define PLP_P1588_SOPMEM_ENTRY_WORD01r_CLR(r) (r).p1588_sopmem_entry_word01[0] = 0
#define PLP_P1588_SOPMEM_ENTRY_WORD01r_SET(r,d) (r).p1588_sopmem_entry_word01[0] = d
#define PLP_P1588_SOPMEM_ENTRY_WORD01r_GET(r) (r).p1588_sopmem_entry_word01[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_ENTRY_WORD01r_SOPMEM_ENTRY_WORD01f_GET(r) (((r).p1588_sopmem_entry_word01[0]) & 0xffff)
#define PLP_P1588_SOPMEM_ENTRY_WORD01r_SOPMEM_ENTRY_WORD01f_SET(r,f) (r).p1588_sopmem_entry_word01[0]=(((r).p1588_sopmem_entry_word01[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_SOPMEM_ENTRY_WORD01.
 *
 */
#define PLP_READ_P1588_SOPMEM_ENTRY_WORD01r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_ENTRY_WORD01r,(_r._p1588_sopmem_entry_word01))
#define PLP_WRITE_P1588_SOPMEM_ENTRY_WORD01r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_ENTRY_WORD01r,(_r._p1588_sopmem_entry_word01))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_ENTRY_WORD01r PLP_P1588_SOPMEM_ENTRY_WORD01r
#define P1588_SOPMEM_ENTRY_WORD01r_SIZE PLP_P1588_SOPMEM_ENTRY_WORD01r_SIZE
typedef PLP_P1588_SOPMEM_ENTRY_WORD01r_t P1588_SOPMEM_ENTRY_WORD01r_t;
#define P1588_SOPMEM_ENTRY_WORD01r_CLR PLP_P1588_SOPMEM_ENTRY_WORD01r_CLR
#define P1588_SOPMEM_ENTRY_WORD01r_SET PLP_P1588_SOPMEM_ENTRY_WORD01r_SET
#define P1588_SOPMEM_ENTRY_WORD01r_GET PLP_P1588_SOPMEM_ENTRY_WORD01r_GET
#define P1588_SOPMEM_ENTRY_WORD01r_SOPMEM_ENTRY_WORD01f_GET PLP_P1588_SOPMEM_ENTRY_WORD01r_SOPMEM_ENTRY_WORD01f_GET
#define P1588_SOPMEM_ENTRY_WORD01r_SOPMEM_ENTRY_WORD01f_SET PLP_P1588_SOPMEM_ENTRY_WORD01r_SOPMEM_ENTRY_WORD01f_SET
#define READ_P1588_SOPMEM_ENTRY_WORD01r PLP_READ_P1588_SOPMEM_ENTRY_WORD01r
#define WRITE_P1588_SOPMEM_ENTRY_WORD01r PLP_WRITE_P1588_SOPMEM_ENTRY_WORD01r
#define MODIFY_P1588_SOPMEM_ENTRY_WORD01r PLP_MODIFY_P1588_SOPMEM_ENTRY_WORD01r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_ENTRY_WORD01r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_ENTRY_WORD02
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70d2
 * DESC:     SOPMEM ENTRY WORD 2 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_ENTRY_WORD02 SOPmem Capture - {ts[47:32]}
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_ENTRY_WORD02r    0x000000d2

#define PLP_P1588_SOPMEM_ENTRY_WORD02r_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_ENTRY_WORD02.
 *
 */
typedef union PLP_P1588_SOPMEM_ENTRY_WORD02r_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_entry_word02[1];
	uint32_t _p1588_sopmem_entry_word02;
} PLP_P1588_SOPMEM_ENTRY_WORD02r_t;

#define PLP_P1588_SOPMEM_ENTRY_WORD02r_CLR(r) (r).p1588_sopmem_entry_word02[0] = 0
#define PLP_P1588_SOPMEM_ENTRY_WORD02r_SET(r,d) (r).p1588_sopmem_entry_word02[0] = d
#define PLP_P1588_SOPMEM_ENTRY_WORD02r_GET(r) (r).p1588_sopmem_entry_word02[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_ENTRY_WORD02r_SOPMEM_ENTRY_WORD02f_GET(r) (((r).p1588_sopmem_entry_word02[0]) & 0xffff)
#define PLP_P1588_SOPMEM_ENTRY_WORD02r_SOPMEM_ENTRY_WORD02f_SET(r,f) (r).p1588_sopmem_entry_word02[0]=(((r).p1588_sopmem_entry_word02[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_SOPMEM_ENTRY_WORD02.
 *
 */
#define PLP_READ_P1588_SOPMEM_ENTRY_WORD02r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_ENTRY_WORD02r,(_r._p1588_sopmem_entry_word02))
#define PLP_WRITE_P1588_SOPMEM_ENTRY_WORD02r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_ENTRY_WORD02r,(_r._p1588_sopmem_entry_word02))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_ENTRY_WORD02r PLP_P1588_SOPMEM_ENTRY_WORD02r
#define P1588_SOPMEM_ENTRY_WORD02r_SIZE PLP_P1588_SOPMEM_ENTRY_WORD02r_SIZE
typedef PLP_P1588_SOPMEM_ENTRY_WORD02r_t P1588_SOPMEM_ENTRY_WORD02r_t;
#define P1588_SOPMEM_ENTRY_WORD02r_CLR PLP_P1588_SOPMEM_ENTRY_WORD02r_CLR
#define P1588_SOPMEM_ENTRY_WORD02r_SET PLP_P1588_SOPMEM_ENTRY_WORD02r_SET
#define P1588_SOPMEM_ENTRY_WORD02r_GET PLP_P1588_SOPMEM_ENTRY_WORD02r_GET
#define P1588_SOPMEM_ENTRY_WORD02r_SOPMEM_ENTRY_WORD02f_GET PLP_P1588_SOPMEM_ENTRY_WORD02r_SOPMEM_ENTRY_WORD02f_GET
#define P1588_SOPMEM_ENTRY_WORD02r_SOPMEM_ENTRY_WORD02f_SET PLP_P1588_SOPMEM_ENTRY_WORD02r_SOPMEM_ENTRY_WORD02f_SET
#define READ_P1588_SOPMEM_ENTRY_WORD02r PLP_READ_P1588_SOPMEM_ENTRY_WORD02r
#define WRITE_P1588_SOPMEM_ENTRY_WORD02r PLP_WRITE_P1588_SOPMEM_ENTRY_WORD02r
#define MODIFY_P1588_SOPMEM_ENTRY_WORD02r PLP_MODIFY_P1588_SOPMEM_ENTRY_WORD02r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_ENTRY_WORD02r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_ENTRY_WORD03
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70d3
 * DESC:     SOPMEM ENTRY WORD 3 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_ENTRY_WORD03 SOPmem Capture - {ts[63:48]}
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_ENTRY_WORD03r    0x000000d3

#define PLP_P1588_SOPMEM_ENTRY_WORD03r_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_ENTRY_WORD03.
 *
 */
typedef union PLP_P1588_SOPMEM_ENTRY_WORD03r_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_entry_word03[1];
	uint32_t _p1588_sopmem_entry_word03;
} PLP_P1588_SOPMEM_ENTRY_WORD03r_t;

#define PLP_P1588_SOPMEM_ENTRY_WORD03r_CLR(r) (r).p1588_sopmem_entry_word03[0] = 0
#define PLP_P1588_SOPMEM_ENTRY_WORD03r_SET(r,d) (r).p1588_sopmem_entry_word03[0] = d
#define PLP_P1588_SOPMEM_ENTRY_WORD03r_GET(r) (r).p1588_sopmem_entry_word03[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_ENTRY_WORD03r_SOPMEM_ENTRY_WORD03f_GET(r) (((r).p1588_sopmem_entry_word03[0]) & 0xffff)
#define PLP_P1588_SOPMEM_ENTRY_WORD03r_SOPMEM_ENTRY_WORD03f_SET(r,f) (r).p1588_sopmem_entry_word03[0]=(((r).p1588_sopmem_entry_word03[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_SOPMEM_ENTRY_WORD03.
 *
 */
#define PLP_READ_P1588_SOPMEM_ENTRY_WORD03r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_ENTRY_WORD03r,(_r._p1588_sopmem_entry_word03))
#define PLP_WRITE_P1588_SOPMEM_ENTRY_WORD03r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_ENTRY_WORD03r,(_r._p1588_sopmem_entry_word03))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_ENTRY_WORD03r PLP_P1588_SOPMEM_ENTRY_WORD03r
#define P1588_SOPMEM_ENTRY_WORD03r_SIZE PLP_P1588_SOPMEM_ENTRY_WORD03r_SIZE
typedef PLP_P1588_SOPMEM_ENTRY_WORD03r_t P1588_SOPMEM_ENTRY_WORD03r_t;
#define P1588_SOPMEM_ENTRY_WORD03r_CLR PLP_P1588_SOPMEM_ENTRY_WORD03r_CLR
#define P1588_SOPMEM_ENTRY_WORD03r_SET PLP_P1588_SOPMEM_ENTRY_WORD03r_SET
#define P1588_SOPMEM_ENTRY_WORD03r_GET PLP_P1588_SOPMEM_ENTRY_WORD03r_GET
#define P1588_SOPMEM_ENTRY_WORD03r_SOPMEM_ENTRY_WORD03f_GET PLP_P1588_SOPMEM_ENTRY_WORD03r_SOPMEM_ENTRY_WORD03f_GET
#define P1588_SOPMEM_ENTRY_WORD03r_SOPMEM_ENTRY_WORD03f_SET PLP_P1588_SOPMEM_ENTRY_WORD03r_SOPMEM_ENTRY_WORD03f_SET
#define READ_P1588_SOPMEM_ENTRY_WORD03r PLP_READ_P1588_SOPMEM_ENTRY_WORD03r
#define WRITE_P1588_SOPMEM_ENTRY_WORD03r PLP_WRITE_P1588_SOPMEM_ENTRY_WORD03r
#define MODIFY_P1588_SOPMEM_ENTRY_WORD03r PLP_MODIFY_P1588_SOPMEM_ENTRY_WORD03r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_ENTRY_WORD03r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_ENTRY_WORD04
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70d4
 * DESC:     SOPMEM ENTRY WORD 4 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_ENTRY_WORD04 SOPmem Capture - {ts[79:64]}
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_ENTRY_WORD04r    0x000000d4

#define PLP_P1588_SOPMEM_ENTRY_WORD04r_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_ENTRY_WORD04.
 *
 */
typedef union PLP_P1588_SOPMEM_ENTRY_WORD04r_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_entry_word04[1];
	uint32_t _p1588_sopmem_entry_word04;
} PLP_P1588_SOPMEM_ENTRY_WORD04r_t;

#define PLP_P1588_SOPMEM_ENTRY_WORD04r_CLR(r) (r).p1588_sopmem_entry_word04[0] = 0
#define PLP_P1588_SOPMEM_ENTRY_WORD04r_SET(r,d) (r).p1588_sopmem_entry_word04[0] = d
#define PLP_P1588_SOPMEM_ENTRY_WORD04r_GET(r) (r).p1588_sopmem_entry_word04[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_ENTRY_WORD04r_SOPMEM_ENTRY_WORD04f_GET(r) (((r).p1588_sopmem_entry_word04[0]) & 0xffff)
#define PLP_P1588_SOPMEM_ENTRY_WORD04r_SOPMEM_ENTRY_WORD04f_SET(r,f) (r).p1588_sopmem_entry_word04[0]=(((r).p1588_sopmem_entry_word04[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_SOPMEM_ENTRY_WORD04.
 *
 */
#define PLP_READ_P1588_SOPMEM_ENTRY_WORD04r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_ENTRY_WORD04r,(_r._p1588_sopmem_entry_word04))
#define PLP_WRITE_P1588_SOPMEM_ENTRY_WORD04r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_ENTRY_WORD04r,(_r._p1588_sopmem_entry_word04))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_ENTRY_WORD04r PLP_P1588_SOPMEM_ENTRY_WORD04r
#define P1588_SOPMEM_ENTRY_WORD04r_SIZE PLP_P1588_SOPMEM_ENTRY_WORD04r_SIZE
typedef PLP_P1588_SOPMEM_ENTRY_WORD04r_t P1588_SOPMEM_ENTRY_WORD04r_t;
#define P1588_SOPMEM_ENTRY_WORD04r_CLR PLP_P1588_SOPMEM_ENTRY_WORD04r_CLR
#define P1588_SOPMEM_ENTRY_WORD04r_SET PLP_P1588_SOPMEM_ENTRY_WORD04r_SET
#define P1588_SOPMEM_ENTRY_WORD04r_GET PLP_P1588_SOPMEM_ENTRY_WORD04r_GET
#define P1588_SOPMEM_ENTRY_WORD04r_SOPMEM_ENTRY_WORD04f_GET PLP_P1588_SOPMEM_ENTRY_WORD04r_SOPMEM_ENTRY_WORD04f_GET
#define P1588_SOPMEM_ENTRY_WORD04r_SOPMEM_ENTRY_WORD04f_SET PLP_P1588_SOPMEM_ENTRY_WORD04r_SOPMEM_ENTRY_WORD04f_SET
#define READ_P1588_SOPMEM_ENTRY_WORD04r PLP_READ_P1588_SOPMEM_ENTRY_WORD04r
#define WRITE_P1588_SOPMEM_ENTRY_WORD04r PLP_WRITE_P1588_SOPMEM_ENTRY_WORD04r
#define MODIFY_P1588_SOPMEM_ENTRY_WORD04r PLP_MODIFY_P1588_SOPMEM_ENTRY_WORD04r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_ENTRY_WORD04r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_ENTRY_WORD05
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70d5
 * DESC:     SOPMEM ENTRY WORD 5 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_ENTRY_WORD05 SOPmem Capture - {sequenceID[3:0], domainNumber[7:0], msgType[3:0]}
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_ENTRY_WORD05r    0x000000d5

#define PLP_P1588_SOPMEM_ENTRY_WORD05r_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_ENTRY_WORD05.
 *
 */
typedef union PLP_P1588_SOPMEM_ENTRY_WORD05r_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_entry_word05[1];
	uint32_t _p1588_sopmem_entry_word05;
} PLP_P1588_SOPMEM_ENTRY_WORD05r_t;

#define PLP_P1588_SOPMEM_ENTRY_WORD05r_CLR(r) (r).p1588_sopmem_entry_word05[0] = 0
#define PLP_P1588_SOPMEM_ENTRY_WORD05r_SET(r,d) (r).p1588_sopmem_entry_word05[0] = d
#define PLP_P1588_SOPMEM_ENTRY_WORD05r_GET(r) (r).p1588_sopmem_entry_word05[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_ENTRY_WORD05r_SOPMEM_ENTRY_WORD05f_GET(r) (((r).p1588_sopmem_entry_word05[0]) & 0xffff)
#define PLP_P1588_SOPMEM_ENTRY_WORD05r_SOPMEM_ENTRY_WORD05f_SET(r,f) (r).p1588_sopmem_entry_word05[0]=(((r).p1588_sopmem_entry_word05[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_SOPMEM_ENTRY_WORD05.
 *
 */
#define PLP_READ_P1588_SOPMEM_ENTRY_WORD05r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_ENTRY_WORD05r,(_r._p1588_sopmem_entry_word05))
#define PLP_WRITE_P1588_SOPMEM_ENTRY_WORD05r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_ENTRY_WORD05r,(_r._p1588_sopmem_entry_word05))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_ENTRY_WORD05r PLP_P1588_SOPMEM_ENTRY_WORD05r
#define P1588_SOPMEM_ENTRY_WORD05r_SIZE PLP_P1588_SOPMEM_ENTRY_WORD05r_SIZE
typedef PLP_P1588_SOPMEM_ENTRY_WORD05r_t P1588_SOPMEM_ENTRY_WORD05r_t;
#define P1588_SOPMEM_ENTRY_WORD05r_CLR PLP_P1588_SOPMEM_ENTRY_WORD05r_CLR
#define P1588_SOPMEM_ENTRY_WORD05r_SET PLP_P1588_SOPMEM_ENTRY_WORD05r_SET
#define P1588_SOPMEM_ENTRY_WORD05r_GET PLP_P1588_SOPMEM_ENTRY_WORD05r_GET
#define P1588_SOPMEM_ENTRY_WORD05r_SOPMEM_ENTRY_WORD05f_GET PLP_P1588_SOPMEM_ENTRY_WORD05r_SOPMEM_ENTRY_WORD05f_GET
#define P1588_SOPMEM_ENTRY_WORD05r_SOPMEM_ENTRY_WORD05f_SET PLP_P1588_SOPMEM_ENTRY_WORD05r_SOPMEM_ENTRY_WORD05f_SET
#define READ_P1588_SOPMEM_ENTRY_WORD05r PLP_READ_P1588_SOPMEM_ENTRY_WORD05r
#define WRITE_P1588_SOPMEM_ENTRY_WORD05r PLP_WRITE_P1588_SOPMEM_ENTRY_WORD05r
#define MODIFY_P1588_SOPMEM_ENTRY_WORD05r PLP_MODIFY_P1588_SOPMEM_ENTRY_WORD05r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_ENTRY_WORD05r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_ENTRY_WORD06
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70d6
 * DESC:     SOPMEM ENTRY WORD 6 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_ENTRY_WORD06 SOPmem Capture - {sourcePortID[3:0], sequenceID[15:4]}
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_ENTRY_WORD06r    0x000000d6

#define PLP_P1588_SOPMEM_ENTRY_WORD06r_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_ENTRY_WORD06.
 *
 */
typedef union PLP_P1588_SOPMEM_ENTRY_WORD06r_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_entry_word06[1];
	uint32_t _p1588_sopmem_entry_word06;
} PLP_P1588_SOPMEM_ENTRY_WORD06r_t;

#define PLP_P1588_SOPMEM_ENTRY_WORD06r_CLR(r) (r).p1588_sopmem_entry_word06[0] = 0
#define PLP_P1588_SOPMEM_ENTRY_WORD06r_SET(r,d) (r).p1588_sopmem_entry_word06[0] = d
#define PLP_P1588_SOPMEM_ENTRY_WORD06r_GET(r) (r).p1588_sopmem_entry_word06[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_ENTRY_WORD06r_SOPMEM_ENTRY_WORD06f_GET(r) (((r).p1588_sopmem_entry_word06[0]) & 0xffff)
#define PLP_P1588_SOPMEM_ENTRY_WORD06r_SOPMEM_ENTRY_WORD06f_SET(r,f) (r).p1588_sopmem_entry_word06[0]=(((r).p1588_sopmem_entry_word06[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_SOPMEM_ENTRY_WORD06.
 *
 */
#define PLP_READ_P1588_SOPMEM_ENTRY_WORD06r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_ENTRY_WORD06r,(_r._p1588_sopmem_entry_word06))
#define PLP_WRITE_P1588_SOPMEM_ENTRY_WORD06r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_ENTRY_WORD06r,(_r._p1588_sopmem_entry_word06))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_ENTRY_WORD06r PLP_P1588_SOPMEM_ENTRY_WORD06r
#define P1588_SOPMEM_ENTRY_WORD06r_SIZE PLP_P1588_SOPMEM_ENTRY_WORD06r_SIZE
typedef PLP_P1588_SOPMEM_ENTRY_WORD06r_t P1588_SOPMEM_ENTRY_WORD06r_t;
#define P1588_SOPMEM_ENTRY_WORD06r_CLR PLP_P1588_SOPMEM_ENTRY_WORD06r_CLR
#define P1588_SOPMEM_ENTRY_WORD06r_SET PLP_P1588_SOPMEM_ENTRY_WORD06r_SET
#define P1588_SOPMEM_ENTRY_WORD06r_GET PLP_P1588_SOPMEM_ENTRY_WORD06r_GET
#define P1588_SOPMEM_ENTRY_WORD06r_SOPMEM_ENTRY_WORD06f_GET PLP_P1588_SOPMEM_ENTRY_WORD06r_SOPMEM_ENTRY_WORD06f_GET
#define P1588_SOPMEM_ENTRY_WORD06r_SOPMEM_ENTRY_WORD06f_SET PLP_P1588_SOPMEM_ENTRY_WORD06r_SOPMEM_ENTRY_WORD06f_SET
#define READ_P1588_SOPMEM_ENTRY_WORD06r PLP_READ_P1588_SOPMEM_ENTRY_WORD06r
#define WRITE_P1588_SOPMEM_ENTRY_WORD06r PLP_WRITE_P1588_SOPMEM_ENTRY_WORD06r
#define MODIFY_P1588_SOPMEM_ENTRY_WORD06r PLP_MODIFY_P1588_SOPMEM_ENTRY_WORD06r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_ENTRY_WORD06r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_ENTRY_WORD07
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70d7
 * DESC:     SOPMEM ENTRY WORD 7 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_ENTRY_WORD07 SOPmem Capture - {sourceIP[3:0], sourcePortID[15:4]}
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_ENTRY_WORD07r    0x000000d7

#define PLP_P1588_SOPMEM_ENTRY_WORD07r_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_ENTRY_WORD07.
 *
 */
typedef union PLP_P1588_SOPMEM_ENTRY_WORD07r_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_entry_word07[1];
	uint32_t _p1588_sopmem_entry_word07;
} PLP_P1588_SOPMEM_ENTRY_WORD07r_t;

#define PLP_P1588_SOPMEM_ENTRY_WORD07r_CLR(r) (r).p1588_sopmem_entry_word07[0] = 0
#define PLP_P1588_SOPMEM_ENTRY_WORD07r_SET(r,d) (r).p1588_sopmem_entry_word07[0] = d
#define PLP_P1588_SOPMEM_ENTRY_WORD07r_GET(r) (r).p1588_sopmem_entry_word07[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_ENTRY_WORD07r_SOPMEM_ENTRY_WORD07f_GET(r) (((r).p1588_sopmem_entry_word07[0]) & 0xffff)
#define PLP_P1588_SOPMEM_ENTRY_WORD07r_SOPMEM_ENTRY_WORD07f_SET(r,f) (r).p1588_sopmem_entry_word07[0]=(((r).p1588_sopmem_entry_word07[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_SOPMEM_ENTRY_WORD07.
 *
 */
#define PLP_READ_P1588_SOPMEM_ENTRY_WORD07r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_ENTRY_WORD07r,(_r._p1588_sopmem_entry_word07))
#define PLP_WRITE_P1588_SOPMEM_ENTRY_WORD07r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_ENTRY_WORD07r,(_r._p1588_sopmem_entry_word07))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_ENTRY_WORD07r PLP_P1588_SOPMEM_ENTRY_WORD07r
#define P1588_SOPMEM_ENTRY_WORD07r_SIZE PLP_P1588_SOPMEM_ENTRY_WORD07r_SIZE
typedef PLP_P1588_SOPMEM_ENTRY_WORD07r_t P1588_SOPMEM_ENTRY_WORD07r_t;
#define P1588_SOPMEM_ENTRY_WORD07r_CLR PLP_P1588_SOPMEM_ENTRY_WORD07r_CLR
#define P1588_SOPMEM_ENTRY_WORD07r_SET PLP_P1588_SOPMEM_ENTRY_WORD07r_SET
#define P1588_SOPMEM_ENTRY_WORD07r_GET PLP_P1588_SOPMEM_ENTRY_WORD07r_GET
#define P1588_SOPMEM_ENTRY_WORD07r_SOPMEM_ENTRY_WORD07f_GET PLP_P1588_SOPMEM_ENTRY_WORD07r_SOPMEM_ENTRY_WORD07f_GET
#define P1588_SOPMEM_ENTRY_WORD07r_SOPMEM_ENTRY_WORD07f_SET PLP_P1588_SOPMEM_ENTRY_WORD07r_SOPMEM_ENTRY_WORD07f_SET
#define READ_P1588_SOPMEM_ENTRY_WORD07r PLP_READ_P1588_SOPMEM_ENTRY_WORD07r
#define WRITE_P1588_SOPMEM_ENTRY_WORD07r PLP_WRITE_P1588_SOPMEM_ENTRY_WORD07r
#define MODIFY_P1588_SOPMEM_ENTRY_WORD07r PLP_MODIFY_P1588_SOPMEM_ENTRY_WORD07r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_ENTRY_WORD07r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_ENTRY_WORD08
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70d8
 * DESC:     SOPMEM ENTRY WORD 8 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_ENTRY_WORD08 SOPmem Capture - {sourceIP[19:4]}
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_ENTRY_WORD08r    0x000000d8

#define PLP_P1588_SOPMEM_ENTRY_WORD08r_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_ENTRY_WORD08.
 *
 */
typedef union PLP_P1588_SOPMEM_ENTRY_WORD08r_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_entry_word08[1];
	uint32_t _p1588_sopmem_entry_word08;
} PLP_P1588_SOPMEM_ENTRY_WORD08r_t;

#define PLP_P1588_SOPMEM_ENTRY_WORD08r_CLR(r) (r).p1588_sopmem_entry_word08[0] = 0
#define PLP_P1588_SOPMEM_ENTRY_WORD08r_SET(r,d) (r).p1588_sopmem_entry_word08[0] = d
#define PLP_P1588_SOPMEM_ENTRY_WORD08r_GET(r) (r).p1588_sopmem_entry_word08[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_ENTRY_WORD08r_SOPMEM_ENTRY_WORD08f_GET(r) (((r).p1588_sopmem_entry_word08[0]) & 0xffff)
#define PLP_P1588_SOPMEM_ENTRY_WORD08r_SOPMEM_ENTRY_WORD08f_SET(r,f) (r).p1588_sopmem_entry_word08[0]=(((r).p1588_sopmem_entry_word08[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_SOPMEM_ENTRY_WORD08.
 *
 */
#define PLP_READ_P1588_SOPMEM_ENTRY_WORD08r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_ENTRY_WORD08r,(_r._p1588_sopmem_entry_word08))
#define PLP_WRITE_P1588_SOPMEM_ENTRY_WORD08r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_ENTRY_WORD08r,(_r._p1588_sopmem_entry_word08))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_ENTRY_WORD08r PLP_P1588_SOPMEM_ENTRY_WORD08r
#define P1588_SOPMEM_ENTRY_WORD08r_SIZE PLP_P1588_SOPMEM_ENTRY_WORD08r_SIZE
typedef PLP_P1588_SOPMEM_ENTRY_WORD08r_t P1588_SOPMEM_ENTRY_WORD08r_t;
#define P1588_SOPMEM_ENTRY_WORD08r_CLR PLP_P1588_SOPMEM_ENTRY_WORD08r_CLR
#define P1588_SOPMEM_ENTRY_WORD08r_SET PLP_P1588_SOPMEM_ENTRY_WORD08r_SET
#define P1588_SOPMEM_ENTRY_WORD08r_GET PLP_P1588_SOPMEM_ENTRY_WORD08r_GET
#define P1588_SOPMEM_ENTRY_WORD08r_SOPMEM_ENTRY_WORD08f_GET PLP_P1588_SOPMEM_ENTRY_WORD08r_SOPMEM_ENTRY_WORD08f_GET
#define P1588_SOPMEM_ENTRY_WORD08r_SOPMEM_ENTRY_WORD08f_SET PLP_P1588_SOPMEM_ENTRY_WORD08r_SOPMEM_ENTRY_WORD08f_SET
#define READ_P1588_SOPMEM_ENTRY_WORD08r PLP_READ_P1588_SOPMEM_ENTRY_WORD08r
#define WRITE_P1588_SOPMEM_ENTRY_WORD08r PLP_WRITE_P1588_SOPMEM_ENTRY_WORD08r
#define MODIFY_P1588_SOPMEM_ENTRY_WORD08r PLP_MODIFY_P1588_SOPMEM_ENTRY_WORD08r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_ENTRY_WORD08r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_ENTRY_WORD09
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70d9
 * DESC:     SOPMEM ENTRY WORD 9 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_ENTRY_WORD09 SOPmem Capture - {sourceIP[35:20]}
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_ENTRY_WORD09r    0x000000d9

#define PLP_P1588_SOPMEM_ENTRY_WORD09r_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_ENTRY_WORD09.
 *
 */
typedef union PLP_P1588_SOPMEM_ENTRY_WORD09r_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_entry_word09[1];
	uint32_t _p1588_sopmem_entry_word09;
} PLP_P1588_SOPMEM_ENTRY_WORD09r_t;

#define PLP_P1588_SOPMEM_ENTRY_WORD09r_CLR(r) (r).p1588_sopmem_entry_word09[0] = 0
#define PLP_P1588_SOPMEM_ENTRY_WORD09r_SET(r,d) (r).p1588_sopmem_entry_word09[0] = d
#define PLP_P1588_SOPMEM_ENTRY_WORD09r_GET(r) (r).p1588_sopmem_entry_word09[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_ENTRY_WORD09r_SOPMEM_ENTRY_WORD09f_GET(r) (((r).p1588_sopmem_entry_word09[0]) & 0xffff)
#define PLP_P1588_SOPMEM_ENTRY_WORD09r_SOPMEM_ENTRY_WORD09f_SET(r,f) (r).p1588_sopmem_entry_word09[0]=(((r).p1588_sopmem_entry_word09[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_SOPMEM_ENTRY_WORD09.
 *
 */
#define PLP_READ_P1588_SOPMEM_ENTRY_WORD09r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_ENTRY_WORD09r,(_r._p1588_sopmem_entry_word09))
#define PLP_WRITE_P1588_SOPMEM_ENTRY_WORD09r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_ENTRY_WORD09r,(_r._p1588_sopmem_entry_word09))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_ENTRY_WORD09r PLP_P1588_SOPMEM_ENTRY_WORD09r
#define P1588_SOPMEM_ENTRY_WORD09r_SIZE PLP_P1588_SOPMEM_ENTRY_WORD09r_SIZE
typedef PLP_P1588_SOPMEM_ENTRY_WORD09r_t P1588_SOPMEM_ENTRY_WORD09r_t;
#define P1588_SOPMEM_ENTRY_WORD09r_CLR PLP_P1588_SOPMEM_ENTRY_WORD09r_CLR
#define P1588_SOPMEM_ENTRY_WORD09r_SET PLP_P1588_SOPMEM_ENTRY_WORD09r_SET
#define P1588_SOPMEM_ENTRY_WORD09r_GET PLP_P1588_SOPMEM_ENTRY_WORD09r_GET
#define P1588_SOPMEM_ENTRY_WORD09r_SOPMEM_ENTRY_WORD09f_GET PLP_P1588_SOPMEM_ENTRY_WORD09r_SOPMEM_ENTRY_WORD09f_GET
#define P1588_SOPMEM_ENTRY_WORD09r_SOPMEM_ENTRY_WORD09f_SET PLP_P1588_SOPMEM_ENTRY_WORD09r_SOPMEM_ENTRY_WORD09f_SET
#define READ_P1588_SOPMEM_ENTRY_WORD09r PLP_READ_P1588_SOPMEM_ENTRY_WORD09r
#define WRITE_P1588_SOPMEM_ENTRY_WORD09r PLP_WRITE_P1588_SOPMEM_ENTRY_WORD09r
#define MODIFY_P1588_SOPMEM_ENTRY_WORD09r PLP_MODIFY_P1588_SOPMEM_ENTRY_WORD09r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_ENTRY_WORD09r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_ENTRY_WORD10
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70da
 * DESC:     SOPMEM ENTRY WORD 10 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_ENTRY_WORD10 SOPmem Capture - {sourceIP[51:36]}
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_ENTRY_WORD10r    0x000000da

#define PLP_P1588_SOPMEM_ENTRY_WORD10r_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_ENTRY_WORD10.
 *
 */
typedef union PLP_P1588_SOPMEM_ENTRY_WORD10r_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_entry_word10[1];
	uint32_t _p1588_sopmem_entry_word10;
} PLP_P1588_SOPMEM_ENTRY_WORD10r_t;

#define PLP_P1588_SOPMEM_ENTRY_WORD10r_CLR(r) (r).p1588_sopmem_entry_word10[0] = 0
#define PLP_P1588_SOPMEM_ENTRY_WORD10r_SET(r,d) (r).p1588_sopmem_entry_word10[0] = d
#define PLP_P1588_SOPMEM_ENTRY_WORD10r_GET(r) (r).p1588_sopmem_entry_word10[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_ENTRY_WORD10r_SOPMEM_ENTRY_WORD10f_GET(r) (((r).p1588_sopmem_entry_word10[0]) & 0xffff)
#define PLP_P1588_SOPMEM_ENTRY_WORD10r_SOPMEM_ENTRY_WORD10f_SET(r,f) (r).p1588_sopmem_entry_word10[0]=(((r).p1588_sopmem_entry_word10[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_SOPMEM_ENTRY_WORD10.
 *
 */
#define PLP_READ_P1588_SOPMEM_ENTRY_WORD10r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_ENTRY_WORD10r,(_r._p1588_sopmem_entry_word10))
#define PLP_WRITE_P1588_SOPMEM_ENTRY_WORD10r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_ENTRY_WORD10r,(_r._p1588_sopmem_entry_word10))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_ENTRY_WORD10r PLP_P1588_SOPMEM_ENTRY_WORD10r
#define P1588_SOPMEM_ENTRY_WORD10r_SIZE PLP_P1588_SOPMEM_ENTRY_WORD10r_SIZE
typedef PLP_P1588_SOPMEM_ENTRY_WORD10r_t P1588_SOPMEM_ENTRY_WORD10r_t;
#define P1588_SOPMEM_ENTRY_WORD10r_CLR PLP_P1588_SOPMEM_ENTRY_WORD10r_CLR
#define P1588_SOPMEM_ENTRY_WORD10r_SET PLP_P1588_SOPMEM_ENTRY_WORD10r_SET
#define P1588_SOPMEM_ENTRY_WORD10r_GET PLP_P1588_SOPMEM_ENTRY_WORD10r_GET
#define P1588_SOPMEM_ENTRY_WORD10r_SOPMEM_ENTRY_WORD10f_GET PLP_P1588_SOPMEM_ENTRY_WORD10r_SOPMEM_ENTRY_WORD10f_GET
#define P1588_SOPMEM_ENTRY_WORD10r_SOPMEM_ENTRY_WORD10f_SET PLP_P1588_SOPMEM_ENTRY_WORD10r_SOPMEM_ENTRY_WORD10f_SET
#define READ_P1588_SOPMEM_ENTRY_WORD10r PLP_READ_P1588_SOPMEM_ENTRY_WORD10r
#define WRITE_P1588_SOPMEM_ENTRY_WORD10r PLP_WRITE_P1588_SOPMEM_ENTRY_WORD10r
#define MODIFY_P1588_SOPMEM_ENTRY_WORD10r PLP_MODIFY_P1588_SOPMEM_ENTRY_WORD10r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_ENTRY_WORD10r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_ENTRY_WORD11
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70db
 * DESC:     SOPMEM ENTRY WORD 11 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_ENTRY_WORD11 SOPmem Capture - {vlanID[3:0], sourceIP[63:52]}
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_ENTRY_WORD11r    0x000000db

#define PLP_P1588_SOPMEM_ENTRY_WORD11r_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_ENTRY_WORD11.
 *
 */
typedef union PLP_P1588_SOPMEM_ENTRY_WORD11r_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_entry_word11[1];
	uint32_t _p1588_sopmem_entry_word11;
} PLP_P1588_SOPMEM_ENTRY_WORD11r_t;

#define PLP_P1588_SOPMEM_ENTRY_WORD11r_CLR(r) (r).p1588_sopmem_entry_word11[0] = 0
#define PLP_P1588_SOPMEM_ENTRY_WORD11r_SET(r,d) (r).p1588_sopmem_entry_word11[0] = d
#define PLP_P1588_SOPMEM_ENTRY_WORD11r_GET(r) (r).p1588_sopmem_entry_word11[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_ENTRY_WORD11r_SOPMEM_ENTRY_WORD11f_GET(r) (((r).p1588_sopmem_entry_word11[0]) & 0xffff)
#define PLP_P1588_SOPMEM_ENTRY_WORD11r_SOPMEM_ENTRY_WORD11f_SET(r,f) (r).p1588_sopmem_entry_word11[0]=(((r).p1588_sopmem_entry_word11[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_SOPMEM_ENTRY_WORD11.
 *
 */
#define PLP_READ_P1588_SOPMEM_ENTRY_WORD11r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_ENTRY_WORD11r,(_r._p1588_sopmem_entry_word11))
#define PLP_WRITE_P1588_SOPMEM_ENTRY_WORD11r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_ENTRY_WORD11r,(_r._p1588_sopmem_entry_word11))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_ENTRY_WORD11r PLP_P1588_SOPMEM_ENTRY_WORD11r
#define P1588_SOPMEM_ENTRY_WORD11r_SIZE PLP_P1588_SOPMEM_ENTRY_WORD11r_SIZE
typedef PLP_P1588_SOPMEM_ENTRY_WORD11r_t P1588_SOPMEM_ENTRY_WORD11r_t;
#define P1588_SOPMEM_ENTRY_WORD11r_CLR PLP_P1588_SOPMEM_ENTRY_WORD11r_CLR
#define P1588_SOPMEM_ENTRY_WORD11r_SET PLP_P1588_SOPMEM_ENTRY_WORD11r_SET
#define P1588_SOPMEM_ENTRY_WORD11r_GET PLP_P1588_SOPMEM_ENTRY_WORD11r_GET
#define P1588_SOPMEM_ENTRY_WORD11r_SOPMEM_ENTRY_WORD11f_GET PLP_P1588_SOPMEM_ENTRY_WORD11r_SOPMEM_ENTRY_WORD11f_GET
#define P1588_SOPMEM_ENTRY_WORD11r_SOPMEM_ENTRY_WORD11f_SET PLP_P1588_SOPMEM_ENTRY_WORD11r_SOPMEM_ENTRY_WORD11f_SET
#define READ_P1588_SOPMEM_ENTRY_WORD11r PLP_READ_P1588_SOPMEM_ENTRY_WORD11r
#define WRITE_P1588_SOPMEM_ENTRY_WORD11r PLP_WRITE_P1588_SOPMEM_ENTRY_WORD11r
#define MODIFY_P1588_SOPMEM_ENTRY_WORD11r PLP_MODIFY_P1588_SOPMEM_ENTRY_WORD11r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_ENTRY_WORD11r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_ENTRY_WORD12
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70dc
 * DESC:     SOPMEM ENTRY WORD 12 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_ENTRY_WORD12 SOPmem Capture - {mask_enable[5:0], 1'b0,  direction, vlanID[11:4]}
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_ENTRY_WORD12r    0x000000dc

#define PLP_P1588_SOPMEM_ENTRY_WORD12r_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_ENTRY_WORD12.
 *
 */
typedef union PLP_P1588_SOPMEM_ENTRY_WORD12r_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_entry_word12[1];
	uint32_t _p1588_sopmem_entry_word12;
} PLP_P1588_SOPMEM_ENTRY_WORD12r_t;

#define PLP_P1588_SOPMEM_ENTRY_WORD12r_CLR(r) (r).p1588_sopmem_entry_word12[0] = 0
#define PLP_P1588_SOPMEM_ENTRY_WORD12r_SET(r,d) (r).p1588_sopmem_entry_word12[0] = d
#define PLP_P1588_SOPMEM_ENTRY_WORD12r_GET(r) (r).p1588_sopmem_entry_word12[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_ENTRY_WORD12r_SOPMEM_ENTRY_WORD12f_GET(r) (((r).p1588_sopmem_entry_word12[0]) & 0xffff)
#define PLP_P1588_SOPMEM_ENTRY_WORD12r_SOPMEM_ENTRY_WORD12f_SET(r,f) (r).p1588_sopmem_entry_word12[0]=(((r).p1588_sopmem_entry_word12[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_SOPMEM_ENTRY_WORD12.
 *
 */
#define PLP_READ_P1588_SOPMEM_ENTRY_WORD12r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_ENTRY_WORD12r,(_r._p1588_sopmem_entry_word12))
#define PLP_WRITE_P1588_SOPMEM_ENTRY_WORD12r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_ENTRY_WORD12r,(_r._p1588_sopmem_entry_word12))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_ENTRY_WORD12r PLP_P1588_SOPMEM_ENTRY_WORD12r
#define P1588_SOPMEM_ENTRY_WORD12r_SIZE PLP_P1588_SOPMEM_ENTRY_WORD12r_SIZE
typedef PLP_P1588_SOPMEM_ENTRY_WORD12r_t P1588_SOPMEM_ENTRY_WORD12r_t;
#define P1588_SOPMEM_ENTRY_WORD12r_CLR PLP_P1588_SOPMEM_ENTRY_WORD12r_CLR
#define P1588_SOPMEM_ENTRY_WORD12r_SET PLP_P1588_SOPMEM_ENTRY_WORD12r_SET
#define P1588_SOPMEM_ENTRY_WORD12r_GET PLP_P1588_SOPMEM_ENTRY_WORD12r_GET
#define P1588_SOPMEM_ENTRY_WORD12r_SOPMEM_ENTRY_WORD12f_GET PLP_P1588_SOPMEM_ENTRY_WORD12r_SOPMEM_ENTRY_WORD12f_GET
#define P1588_SOPMEM_ENTRY_WORD12r_SOPMEM_ENTRY_WORD12f_SET PLP_P1588_SOPMEM_ENTRY_WORD12r_SOPMEM_ENTRY_WORD12f_SET
#define READ_P1588_SOPMEM_ENTRY_WORD12r PLP_READ_P1588_SOPMEM_ENTRY_WORD12r
#define WRITE_P1588_SOPMEM_ENTRY_WORD12r PLP_WRITE_P1588_SOPMEM_ENTRY_WORD12r
#define MODIFY_P1588_SOPMEM_ENTRY_WORD12r PLP_MODIFY_P1588_SOPMEM_ENTRY_WORD12r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_ENTRY_WORD12r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_ENTRY_WORD13
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70dd
 * DESC:     SOPMEM ENTRY WORD 13 Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_ENTRY_WORD13 SOPmem Capture - {13'h0, valid, mask_enable[7:6]}
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_ENTRY_WORD13r    0x000000dd

#define PLP_P1588_SOPMEM_ENTRY_WORD13r_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_ENTRY_WORD13.
 *
 */
typedef union PLP_P1588_SOPMEM_ENTRY_WORD13r_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_entry_word13[1];
	uint32_t _p1588_sopmem_entry_word13;
} PLP_P1588_SOPMEM_ENTRY_WORD13r_t;

#define PLP_P1588_SOPMEM_ENTRY_WORD13r_CLR(r) (r).p1588_sopmem_entry_word13[0] = 0
#define PLP_P1588_SOPMEM_ENTRY_WORD13r_SET(r,d) (r).p1588_sopmem_entry_word13[0] = d
#define PLP_P1588_SOPMEM_ENTRY_WORD13r_GET(r) (r).p1588_sopmem_entry_word13[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_ENTRY_WORD13r_SOPMEM_ENTRY_WORD13f_GET(r) (((r).p1588_sopmem_entry_word13[0]) & 0xffff)
#define PLP_P1588_SOPMEM_ENTRY_WORD13r_SOPMEM_ENTRY_WORD13f_SET(r,f) (r).p1588_sopmem_entry_word13[0]=(((r).p1588_sopmem_entry_word13[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_SOPMEM_ENTRY_WORD13.
 *
 */
#define PLP_READ_P1588_SOPMEM_ENTRY_WORD13r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_ENTRY_WORD13r,(_r._p1588_sopmem_entry_word13))
#define PLP_WRITE_P1588_SOPMEM_ENTRY_WORD13r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_ENTRY_WORD13r,(_r._p1588_sopmem_entry_word13))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_ENTRY_WORD13r PLP_P1588_SOPMEM_ENTRY_WORD13r
#define P1588_SOPMEM_ENTRY_WORD13r_SIZE PLP_P1588_SOPMEM_ENTRY_WORD13r_SIZE
typedef PLP_P1588_SOPMEM_ENTRY_WORD13r_t P1588_SOPMEM_ENTRY_WORD13r_t;
#define P1588_SOPMEM_ENTRY_WORD13r_CLR PLP_P1588_SOPMEM_ENTRY_WORD13r_CLR
#define P1588_SOPMEM_ENTRY_WORD13r_SET PLP_P1588_SOPMEM_ENTRY_WORD13r_SET
#define P1588_SOPMEM_ENTRY_WORD13r_GET PLP_P1588_SOPMEM_ENTRY_WORD13r_GET
#define P1588_SOPMEM_ENTRY_WORD13r_SOPMEM_ENTRY_WORD13f_GET PLP_P1588_SOPMEM_ENTRY_WORD13r_SOPMEM_ENTRY_WORD13f_GET
#define P1588_SOPMEM_ENTRY_WORD13r_SOPMEM_ENTRY_WORD13f_SET PLP_P1588_SOPMEM_ENTRY_WORD13r_SOPMEM_ENTRY_WORD13f_SET
#define READ_P1588_SOPMEM_ENTRY_WORD13r PLP_READ_P1588_SOPMEM_ENTRY_WORD13r
#define WRITE_P1588_SOPMEM_ENTRY_WORD13r PLP_WRITE_P1588_SOPMEM_ENTRY_WORD13r
#define MODIFY_P1588_SOPMEM_ENTRY_WORD13r PLP_MODIFY_P1588_SOPMEM_ENTRY_WORD13r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_ENTRY_WORD13r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_READ_MISS_COUNTER
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70de
 * DESC:     SOPMEM TX READ MISS COUNTER Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_READ_MISS_COUNTER SOPmem inband mode- count number of read to SOPmem result in classfication missed or while SOPmem is emptySOPmem PCH mode- count number of CRC error in PCH header for TX pathThis register is read-on-clear (ROC)
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_READ_MISS_COUNTERr    0x000000de

#define PLP_P1588_SOPMEM_READ_MISS_COUNTERr_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_READ_MISS_COUNTER.
 *
 */
typedef union PLP_P1588_SOPMEM_READ_MISS_COUNTERr_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_read_miss_counter[1];
	uint32_t _p1588_sopmem_read_miss_counter;
} PLP_P1588_SOPMEM_READ_MISS_COUNTERr_t;

#define PLP_P1588_SOPMEM_READ_MISS_COUNTERr_CLR(r) (r).p1588_sopmem_read_miss_counter[0] = 0
#define PLP_P1588_SOPMEM_READ_MISS_COUNTERr_SET(r,d) (r).p1588_sopmem_read_miss_counter[0] = d
#define PLP_P1588_SOPMEM_READ_MISS_COUNTERr_GET(r) (r).p1588_sopmem_read_miss_counter[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_READ_MISS_COUNTERr_SOPMEM_READ_MISS_COUNTERf_GET(r) (((r).p1588_sopmem_read_miss_counter[0]) & 0xffff)
#define PLP_P1588_SOPMEM_READ_MISS_COUNTERr_SOPMEM_READ_MISS_COUNTERf_SET(r,f) (r).p1588_sopmem_read_miss_counter[0]=(((r).p1588_sopmem_read_miss_counter[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_SOPMEM_READ_MISS_COUNTER.
 *
 */
#define PLP_READ_P1588_SOPMEM_READ_MISS_COUNTERr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_READ_MISS_COUNTERr,(_r._p1588_sopmem_read_miss_counter))
#define PLP_WRITE_P1588_SOPMEM_READ_MISS_COUNTERr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_READ_MISS_COUNTERr,(_r._p1588_sopmem_read_miss_counter))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_READ_MISS_COUNTERr PLP_P1588_SOPMEM_READ_MISS_COUNTERr
#define P1588_SOPMEM_READ_MISS_COUNTERr_SIZE PLP_P1588_SOPMEM_READ_MISS_COUNTERr_SIZE
typedef PLP_P1588_SOPMEM_READ_MISS_COUNTERr_t P1588_SOPMEM_READ_MISS_COUNTERr_t;
#define P1588_SOPMEM_READ_MISS_COUNTERr_CLR PLP_P1588_SOPMEM_READ_MISS_COUNTERr_CLR
#define P1588_SOPMEM_READ_MISS_COUNTERr_SET PLP_P1588_SOPMEM_READ_MISS_COUNTERr_SET
#define P1588_SOPMEM_READ_MISS_COUNTERr_GET PLP_P1588_SOPMEM_READ_MISS_COUNTERr_GET
#define P1588_SOPMEM_READ_MISS_COUNTERr_SOPMEM_READ_MISS_COUNTERf_GET PLP_P1588_SOPMEM_READ_MISS_COUNTERr_SOPMEM_READ_MISS_COUNTERf_GET
#define P1588_SOPMEM_READ_MISS_COUNTERr_SOPMEM_READ_MISS_COUNTERf_SET PLP_P1588_SOPMEM_READ_MISS_COUNTERr_SOPMEM_READ_MISS_COUNTERf_SET
#define READ_P1588_SOPMEM_READ_MISS_COUNTERr PLP_READ_P1588_SOPMEM_READ_MISS_COUNTERr
#define WRITE_P1588_SOPMEM_READ_MISS_COUNTERr PLP_WRITE_P1588_SOPMEM_READ_MISS_COUNTERr
#define MODIFY_P1588_SOPMEM_READ_MISS_COUNTERr PLP_MODIFY_P1588_SOPMEM_READ_MISS_COUNTERr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_READ_MISS_COUNTERr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_NUM_VALID_ENTRY
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70df
 * DESC:     SOPMEM NUMBER OF VALID ENTRY Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_NUMBER_VALID_ENTRY SOPmem - number of currently valid entry
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_NUM_VALID_ENTRYr    0x000000df

#define PLP_P1588_SOPMEM_NUM_VALID_ENTRYr_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_NUM_VALID_ENTRY.
 *
 */
typedef union PLP_P1588_SOPMEM_NUM_VALID_ENTRYr_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_num_valid_entry[1];
	uint32_t _p1588_sopmem_num_valid_entry;
} PLP_P1588_SOPMEM_NUM_VALID_ENTRYr_t;

#define PLP_P1588_SOPMEM_NUM_VALID_ENTRYr_CLR(r) (r).p1588_sopmem_num_valid_entry[0] = 0
#define PLP_P1588_SOPMEM_NUM_VALID_ENTRYr_SET(r,d) (r).p1588_sopmem_num_valid_entry[0] = d
#define PLP_P1588_SOPMEM_NUM_VALID_ENTRYr_GET(r) (r).p1588_sopmem_num_valid_entry[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_NUM_VALID_ENTRYr_SOPMEM_NUMBER_VALID_ENTRYf_GET(r) (((r).p1588_sopmem_num_valid_entry[0]) & 0xffff)
#define PLP_P1588_SOPMEM_NUM_VALID_ENTRYr_SOPMEM_NUMBER_VALID_ENTRYf_SET(r,f) (r).p1588_sopmem_num_valid_entry[0]=(((r).p1588_sopmem_num_valid_entry[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)
#define PLP_P1588_SOPMEM_NUM_VALID_ENTRYr_SOPMEM_NUMBER_CURRENT_VALID_ENTRYf_GET(r) (((r).p1588_sopmem_num_valid_entry[0]) & 0xff)

/*
 * These macros can be used to access P1588_SOPMEM_NUM_VALID_ENTRY.
 *
 */
#define PLP_READ_P1588_SOPMEM_NUM_VALID_ENTRYr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_NUM_VALID_ENTRYr,(_r._p1588_sopmem_num_valid_entry))
#define PLP_WRITE_P1588_SOPMEM_NUM_VALID_ENTRYr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_NUM_VALID_ENTRYr,(_r._p1588_sopmem_num_valid_entry))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_NUM_VALID_ENTRYr PLP_P1588_SOPMEM_NUM_VALID_ENTRYr
#define P1588_SOPMEM_NUM_VALID_ENTRYr_SIZE PLP_P1588_SOPMEM_NUM_VALID_ENTRYr_SIZE
typedef PLP_P1588_SOPMEM_NUM_VALID_ENTRYr_t P1588_SOPMEM_NUM_VALID_ENTRYr_t;
#define P1588_SOPMEM_NUM_VALID_ENTRYr_CLR PLP_P1588_SOPMEM_NUM_VALID_ENTRYr_CLR
#define P1588_SOPMEM_NUM_VALID_ENTRYr_SET PLP_P1588_SOPMEM_NUM_VALID_ENTRYr_SET
#define P1588_SOPMEM_NUM_VALID_ENTRYr_GET PLP_P1588_SOPMEM_NUM_VALID_ENTRYr_GET
#define P1588_SOPMEM_NUM_VALID_ENTRYr_SOPMEM_NUMBER_VALID_ENTRYf_GET PLP_P1588_SOPMEM_NUM_VALID_ENTRYr_SOPMEM_NUMBER_VALID_ENTRYf_GET
#define P1588_SOPMEM_NUM_VALID_ENTRYr_SOPMEM_NUMBER_VALID_ENTRYf_SET PLP_P1588_SOPMEM_NUM_VALID_ENTRYr_SOPMEM_NUMBER_VALID_ENTRYf_SET
#define P1588_SOPMEM_NUM_VALID_ENTRYr_SOPMEM_NUMBER_CURRENTVALID_ENTRYf_GET PLP_P1588_SOPMEM_NUM_VALID_ENTRYr_SOPMEM_NUMBER_CURRENT_VALID_ENTRYf_GET
#define READ_P1588_SOPMEM_NUM_VALID_ENTRYr PLP_READ_P1588_SOPMEM_NUM_VALID_ENTRYr
#define WRITE_P1588_SOPMEM_NUM_VALID_ENTRYr PLP_WRITE_P1588_SOPMEM_NUM_VALID_ENTRYr
#define MODIFY_P1588_SOPMEM_NUM_VALID_ENTRYr PLP_MODIFY_P1588_SOPMEM_NUM_VALID_ENTRYr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_NUM_VALID_ENTRYr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_SOPMEM_READ_HIT_COUNTER
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70e0
 * DESC:     SOPMEM READ HIT COUNTER Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SOPMEM_READ_HIT_COUNTER SOPmem - count number of read to SOPmem read result in classfication hit or successfulSOPmem PCH mode- count number of CRC good in PCH header for TX pathThis register is read-on-clear (ROC)
 *
 ******************************************************************************/
#define PLP_P1588_SOPMEM_READ_HIT_COUNTERr    0x000000e0

#define PLP_P1588_SOPMEM_READ_HIT_COUNTERr_SIZE 4

/*
 * This structure should be used to declare and program P1588_SOPMEM_READ_HIT_COUNTER.
 *
 */
typedef union PLP_P1588_SOPMEM_READ_HIT_COUNTERr_s {
	uint32_t v[1];
	uint32_t p1588_sopmem_read_hit_counter[1];
	uint32_t _p1588_sopmem_read_hit_counter;
} PLP_P1588_SOPMEM_READ_HIT_COUNTERr_t;

#define PLP_P1588_SOPMEM_READ_HIT_COUNTERr_CLR(r) (r).p1588_sopmem_read_hit_counter[0] = 0
#define PLP_P1588_SOPMEM_READ_HIT_COUNTERr_SET(r,d) (r).p1588_sopmem_read_hit_counter[0] = d
#define PLP_P1588_SOPMEM_READ_HIT_COUNTERr_GET(r) (r).p1588_sopmem_read_hit_counter[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_SOPMEM_READ_HIT_COUNTERr_SOPMEM_READ_HIT_COUNTERf_GET(r) (((r).p1588_sopmem_read_hit_counter[0]) & 0xffff)
#define PLP_P1588_SOPMEM_READ_HIT_COUNTERr_SOPMEM_READ_HIT_COUNTERf_SET(r,f) (r).p1588_sopmem_read_hit_counter[0]=(((r).p1588_sopmem_read_hit_counter[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_SOPMEM_READ_HIT_COUNTER.
 *
 */
#define PLP_READ_P1588_SOPMEM_READ_HIT_COUNTERr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_SOPMEM_READ_HIT_COUNTERr,(_r._p1588_sopmem_read_hit_counter))
#define PLP_WRITE_P1588_SOPMEM_READ_HIT_COUNTERr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_SOPMEM_READ_HIT_COUNTERr,(_r._p1588_sopmem_read_hit_counter))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_SOPMEM_READ_HIT_COUNTERr PLP_P1588_SOPMEM_READ_HIT_COUNTERr
#define P1588_SOPMEM_READ_HIT_COUNTERr_SIZE PLP_P1588_SOPMEM_READ_HIT_COUNTERr_SIZE
typedef PLP_P1588_SOPMEM_READ_HIT_COUNTERr_t P1588_SOPMEM_READ_HIT_COUNTERr_t;
#define P1588_SOPMEM_READ_HIT_COUNTERr_CLR PLP_P1588_SOPMEM_READ_HIT_COUNTERr_CLR
#define P1588_SOPMEM_READ_HIT_COUNTERr_SET PLP_P1588_SOPMEM_READ_HIT_COUNTERr_SET
#define P1588_SOPMEM_READ_HIT_COUNTERr_GET PLP_P1588_SOPMEM_READ_HIT_COUNTERr_GET
#define P1588_SOPMEM_READ_HIT_COUNTERr_SOPMEM_READ_HIT_COUNTERf_GET PLP_P1588_SOPMEM_READ_HIT_COUNTERr_SOPMEM_READ_HIT_COUNTERf_GET
#define P1588_SOPMEM_READ_HIT_COUNTERr_SOPMEM_READ_HIT_COUNTERf_SET PLP_P1588_SOPMEM_READ_HIT_COUNTERr_SOPMEM_READ_HIT_COUNTERf_SET
#define READ_P1588_SOPMEM_READ_HIT_COUNTERr PLP_READ_P1588_SOPMEM_READ_HIT_COUNTERr
#define WRITE_P1588_SOPMEM_READ_HIT_COUNTERr PLP_WRITE_P1588_SOPMEM_READ_HIT_COUNTERr
#define MODIFY_P1588_SOPMEM_READ_HIT_COUNTERr PLP_MODIFY_P1588_SOPMEM_READ_HIT_COUNTERr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_SOPMEM_READ_HIT_COUNTERr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_ENABLE
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70e2
 * DESC:     pim mode slice enable register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_ENABLE       Enables the 1588 RX and TX Cio mode function.Bit 1 -- enable RX port 1Bit 0 -- enable TX port 0Reset value is 0x0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_ENABLEr    0x000000e2

#define PLP_P1588_PIM_ENABLEr_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_ENABLE.
 *
 */
typedef union PLP_P1588_PIM_ENABLEr_s {
	uint32_t v[1];
	uint32_t p1588_pim_enable[1];
	uint32_t _p1588_pim_enable;
} PLP_P1588_PIM_ENABLEr_t;

#define PLP_P1588_PIM_ENABLEr_CLR(r) (r).p1588_pim_enable[0] = 0
#define PLP_P1588_PIM_ENABLEr_SET(r,d) (r).p1588_pim_enable[0] = d
#define PLP_P1588_PIM_ENABLEr_GET(r) (r).p1588_pim_enable[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_ENABLEr_PIM_ENABLEf_GET(r) (((r).p1588_pim_enable[0]) & 0xffff)
#define PLP_P1588_PIM_ENABLEr_PIM_ENABLEf_SET(r,f) (r).p1588_pim_enable[0]=(((r).p1588_pim_enable[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

#define P1588_PIM_ENABLEr_TXRX_1588PCH_BYPASS_GET(r) ((((r).p1588_pim_enable[0]) >> 6) & 0x3)
#define P1588_PIM_ENABLEr_TXRX_1588PCH_BYPASS_SET(r,f) (r).p1588_pim_enable[0]=(((r).p1588_pim_enable[0] & ~((uint32_t)0x3 << 6)) | ((((uint32_t)f) & 0x3) << 6)) | (3 << (16 + 6))

/*
 * These macros can be used to access P1588_PIM_ENABLE.
 *
 */
#define PLP_READ_P1588_PIM_ENABLEr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_ENABLEr,(_r._p1588_pim_enable))
#define PLP_WRITE_P1588_PIM_ENABLEr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_ENABLEr,(_r._p1588_pim_enable))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_ENABLEr PLP_P1588_PIM_ENABLEr
#define P1588_PIM_ENABLEr_SIZE PLP_P1588_PIM_ENABLEr_SIZE
typedef PLP_P1588_PIM_ENABLEr_t P1588_PIM_ENABLEr_t;
#define P1588_PIM_ENABLEr_CLR PLP_P1588_PIM_ENABLEr_CLR
#define P1588_PIM_ENABLEr_SET PLP_P1588_PIM_ENABLEr_SET
#define P1588_PIM_ENABLEr_GET PLP_P1588_PIM_ENABLEr_GET
#define P1588_PIM_ENABLEr_PIM_ENABLEf_GET PLP_P1588_PIM_ENABLEr_PIM_ENABLEf_GET
#define P1588_PIM_ENABLEr_PIM_ENABLEf_SET PLP_P1588_PIM_ENABLEr_PIM_ENABLEf_SET
#define READ_P1588_PIM_ENABLEr PLP_READ_P1588_PIM_ENABLEr
#define WRITE_P1588_PIM_ENABLEr PLP_WRITE_P1588_PIM_ENABLEr
#define MODIFY_P1588_PIM_ENABLEr PLP_MODIFY_P1588_PIM_ENABLEr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_ENABLEr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_CNTL_P0
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70e3
 * DESC:     pim port 0 control register
 * RESETVAL: 0x201 (513)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_CNTL_P0      bit[15]    1 = Disable TS capture in egress sidebit[14]    1 = override extension field 2'b01 check on egress sidebit[13]    1 = override extension field 2'b10 check on egress sidebit[12]    0 = enable CRC check on egress sidebit[10]    1 = caputre TS for Idle Packet  Contains status data for a port  no packet data. packet type 2'b10bit[9]      1 = caputre TS for Ethernet packet, no Status Header (packet information).  packet type 2'b01.bit[8]      1 = caputre TS for Ethernet Packet with status header. packet type 2'b00bit[7:6]   Packet typebit[5:2]   Subport IDbit[1:0]   Extension field type for 1588 function, default:2'b01Reset value is 0x1.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_CNTL_P0r    0x000000e3

#define PLP_P1588_PIM_CNTL_P0r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_CNTL_P0.
 *
 */
typedef union PLP_P1588_PIM_CNTL_P0r_s {
	uint32_t v[1];
	uint32_t p1588_pim_cntl_p0[1];
	uint32_t _p1588_pim_cntl_p0;
} PLP_P1588_PIM_CNTL_P0r_t;

#define PLP_P1588_PIM_CNTL_P0r_CLR(r) (r).p1588_pim_cntl_p0[0] = 0
#define PLP_P1588_PIM_CNTL_P0r_SET(r,d) (r).p1588_pim_cntl_p0[0] = d
#define PLP_P1588_PIM_CNTL_P0r_GET(r) (r).p1588_pim_cntl_p0[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_CNTL_P0r_PIM_CNTL_P0f_GET(r) (((r).p1588_pim_cntl_p0[0]) & 0xffff)
#define PLP_P1588_PIM_CNTL_P0r_PIM_CNTL_P0f_SET(r,f) (r).p1588_pim_cntl_p0[0]=(((r).p1588_pim_cntl_p0[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PIM_CNTL_P0.
 *
 */
#define PLP_READ_P1588_PIM_CNTL_P0r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_CNTL_P0r,(_r._p1588_pim_cntl_p0))
#define PLP_WRITE_P1588_PIM_CNTL_P0r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_CNTL_P0r,(_r._p1588_pim_cntl_p0))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_CNTL_P0r PLP_P1588_PIM_CNTL_P0r
#define P1588_PIM_CNTL_P0r_SIZE PLP_P1588_PIM_CNTL_P0r_SIZE
typedef PLP_P1588_PIM_CNTL_P0r_t P1588_PIM_CNTL_P0r_t;
#define P1588_PIM_CNTL_P0r_CLR PLP_P1588_PIM_CNTL_P0r_CLR
#define P1588_PIM_CNTL_P0r_SET PLP_P1588_PIM_CNTL_P0r_SET
#define P1588_PIM_CNTL_P0r_GET PLP_P1588_PIM_CNTL_P0r_GET
#define P1588_PIM_CNTL_P0r_PIM_CNTL_P0f_GET PLP_P1588_PIM_CNTL_P0r_PIM_CNTL_P0f_GET
#define P1588_PIM_CNTL_P0r_PIM_CNTL_P0f_SET PLP_P1588_PIM_CNTL_P0r_PIM_CNTL_P0f_SET
#define READ_P1588_PIM_CNTL_P0r PLP_READ_P1588_PIM_CNTL_P0r
#define WRITE_P1588_PIM_CNTL_P0r PLP_WRITE_P1588_PIM_CNTL_P0r
#define MODIFY_P1588_PIM_CNTL_P0r PLP_MODIFY_P1588_PIM_CNTL_P0r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_CNTL_P0r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_NSE_SYNC_CNTL
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70e4
 * DESC:     pim nse sync control register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_NSE_SYNC_CNTL bit[15:4]    Reservedbit[3]   Enable capture registerbit[2]   Enable drift adjust registerbit[1]   Enable offset adjust registerbit[0]   Enable set time registerReset value is 0x0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_NSE_SYNC_CNTLr    0x000000e4

#define PLP_P1588_PIM_NSE_SYNC_CNTLr_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_NSE_SYNC_CNTL.
 *
 */
typedef union PLP_P1588_PIM_NSE_SYNC_CNTLr_s {
	uint32_t v[1];
	uint32_t p1588_pim_nse_sync_cntl[1];
	uint32_t _p1588_pim_nse_sync_cntl;
} PLP_P1588_PIM_NSE_SYNC_CNTLr_t;

#define PLP_P1588_PIM_NSE_SYNC_CNTLr_CLR(r) (r).p1588_pim_nse_sync_cntl[0] = 0
#define PLP_P1588_PIM_NSE_SYNC_CNTLr_SET(r,d) (r).p1588_pim_nse_sync_cntl[0] = d
#define PLP_P1588_PIM_NSE_SYNC_CNTLr_GET(r) (r).p1588_pim_nse_sync_cntl[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_NSE_SYNC_CNTLr_PIM_NSE_SYNC_CNTLf_GET(r) (((r).p1588_pim_nse_sync_cntl[0]) & 0xffff)
#define PLP_P1588_PIM_NSE_SYNC_CNTLr_PIM_NSE_SYNC_CNTLf_SET(r,f) (r).p1588_pim_nse_sync_cntl[0]=(((r).p1588_pim_nse_sync_cntl[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PIM_NSE_SYNC_CNTL.
 *
 */
#define PLP_READ_P1588_PIM_NSE_SYNC_CNTLr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_NSE_SYNC_CNTLr,(_r._p1588_pim_nse_sync_cntl))
#define PLP_WRITE_P1588_PIM_NSE_SYNC_CNTLr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_NSE_SYNC_CNTLr,(_r._p1588_pim_nse_sync_cntl))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_NSE_SYNC_CNTLr PLP_P1588_PIM_NSE_SYNC_CNTLr
#define P1588_PIM_NSE_SYNC_CNTLr_SIZE PLP_P1588_PIM_NSE_SYNC_CNTLr_SIZE
typedef PLP_P1588_PIM_NSE_SYNC_CNTLr_t P1588_PIM_NSE_SYNC_CNTLr_t;
#define P1588_PIM_NSE_SYNC_CNTLr_CLR PLP_P1588_PIM_NSE_SYNC_CNTLr_CLR
#define P1588_PIM_NSE_SYNC_CNTLr_SET PLP_P1588_PIM_NSE_SYNC_CNTLr_SET
#define P1588_PIM_NSE_SYNC_CNTLr_GET PLP_P1588_PIM_NSE_SYNC_CNTLr_GET
#define P1588_PIM_NSE_SYNC_CNTLr_PIM_NSE_SYNC_CNTLf_GET PLP_P1588_PIM_NSE_SYNC_CNTLr_PIM_NSE_SYNC_CNTLf_GET
#define P1588_PIM_NSE_SYNC_CNTLr_PIM_NSE_SYNC_CNTLf_SET PLP_P1588_PIM_NSE_SYNC_CNTLr_PIM_NSE_SYNC_CNTLf_SET
#define READ_P1588_PIM_NSE_SYNC_CNTLr PLP_READ_P1588_PIM_NSE_SYNC_CNTLr
#define WRITE_P1588_PIM_NSE_SYNC_CNTLr PLP_WRITE_P1588_PIM_NSE_SYNC_CNTLr
#define MODIFY_P1588_PIM_NSE_SYNC_CNTLr PLP_MODIFY_P1588_PIM_NSE_SYNC_CNTLr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_NSE_SYNC_CNTLr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_NSE_OFFSET_0
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70e5
 * DESC:     pim nse offset register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_NSE_OFFSET_0 The PIM NSE counter offset registerPIM_NSE_OFFSET = {PIM_NSE_OFFSET_5, PIM_NSE_OFFSET_4, PIM_NSE_OFFSET_3, PIM_NSE_OFFSET_2, PIM_NSE_OFFSET_1, PIM_NSE_OFFSET_0}Reset value is 0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_NSE_OFFSET_0r    0x000000e5

#define PLP_P1588_PIM_NSE_OFFSET_0r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_NSE_OFFSET_0.
 *
 */
typedef union PLP_P1588_PIM_NSE_OFFSET_0r_s {
	uint32_t v[1];
	uint32_t p1588_pim_nse_offset_0[1];
	uint32_t _p1588_pim_nse_offset_0;
} PLP_P1588_PIM_NSE_OFFSET_0r_t;

#define PLP_P1588_PIM_NSE_OFFSET_0r_CLR(r) (r).p1588_pim_nse_offset_0[0] = 0
#define PLP_P1588_PIM_NSE_OFFSET_0r_SET(r,d) (r).p1588_pim_nse_offset_0[0] = d
#define PLP_P1588_PIM_NSE_OFFSET_0r_GET(r) (r).p1588_pim_nse_offset_0[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_NSE_OFFSET_0r_PIM_NSE_OFFSET_0f_GET(r) (((r).p1588_pim_nse_offset_0[0]) & 0xffff)
#define PLP_P1588_PIM_NSE_OFFSET_0r_PIM_NSE_OFFSET_0f_SET(r,f) (r).p1588_pim_nse_offset_0[0]=(((r).p1588_pim_nse_offset_0[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PIM_NSE_OFFSET_0.
 *
 */
#define PLP_READ_P1588_PIM_NSE_OFFSET_0r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_NSE_OFFSET_0r,(_r._p1588_pim_nse_offset_0))
#define PLP_WRITE_P1588_PIM_NSE_OFFSET_0r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_NSE_OFFSET_0r,(_r._p1588_pim_nse_offset_0))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_NSE_OFFSET_0r PLP_P1588_PIM_NSE_OFFSET_0r
#define P1588_PIM_NSE_OFFSET_0r_SIZE PLP_P1588_PIM_NSE_OFFSET_0r_SIZE
typedef PLP_P1588_PIM_NSE_OFFSET_0r_t P1588_PIM_NSE_OFFSET_0r_t;
#define P1588_PIM_NSE_OFFSET_0r_CLR PLP_P1588_PIM_NSE_OFFSET_0r_CLR
#define P1588_PIM_NSE_OFFSET_0r_SET PLP_P1588_PIM_NSE_OFFSET_0r_SET
#define P1588_PIM_NSE_OFFSET_0r_GET PLP_P1588_PIM_NSE_OFFSET_0r_GET
#define P1588_PIM_NSE_OFFSET_0r_PIM_NSE_OFFSET_0f_GET PLP_P1588_PIM_NSE_OFFSET_0r_PIM_NSE_OFFSET_0f_GET
#define P1588_PIM_NSE_OFFSET_0r_PIM_NSE_OFFSET_0f_SET PLP_P1588_PIM_NSE_OFFSET_0r_PIM_NSE_OFFSET_0f_SET
#define READ_P1588_PIM_NSE_OFFSET_0r PLP_READ_P1588_PIM_NSE_OFFSET_0r
#define WRITE_P1588_PIM_NSE_OFFSET_0r PLP_WRITE_P1588_PIM_NSE_OFFSET_0r
#define MODIFY_P1588_PIM_NSE_OFFSET_0r PLP_MODIFY_P1588_PIM_NSE_OFFSET_0r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_NSE_OFFSET_0r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_NSE_OFFSET_1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70e6
 * DESC:     pim nse offset register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_NSE_OFFSET_1 The PIM NSE counter offset registerPIM_NSE_OFFSET = {PIM_NSE_OFFSET_5, PIM_NSE_OFFSET_4, PIM_NSE_OFFSET_3, PIM_NSE_OFFSET_2, PIM_NSE_OFFSET_1, PIM_NSE_OFFSET_0}Reset value is 0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_NSE_OFFSET_1r    0x000000e6

#define PLP_P1588_PIM_NSE_OFFSET_1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_NSE_OFFSET_1.
 *
 */
typedef union PLP_P1588_PIM_NSE_OFFSET_1r_s {
	uint32_t v[1];
	uint32_t p1588_pim_nse_offset_1[1];
	uint32_t _p1588_pim_nse_offset_1;
} PLP_P1588_PIM_NSE_OFFSET_1r_t;

#define PLP_P1588_PIM_NSE_OFFSET_1r_CLR(r) (r).p1588_pim_nse_offset_1[0] = 0
#define PLP_P1588_PIM_NSE_OFFSET_1r_SET(r,d) (r).p1588_pim_nse_offset_1[0] = d
#define PLP_P1588_PIM_NSE_OFFSET_1r_GET(r) (r).p1588_pim_nse_offset_1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_NSE_OFFSET_1r_PIM_NSE_OFFSET_1f_GET(r) (((r).p1588_pim_nse_offset_1[0]) & 0xffff)
#define PLP_P1588_PIM_NSE_OFFSET_1r_PIM_NSE_OFFSET_1f_SET(r,f) (r).p1588_pim_nse_offset_1[0]=(((r).p1588_pim_nse_offset_1[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PIM_NSE_OFFSET_1.
 *
 */
#define PLP_READ_P1588_PIM_NSE_OFFSET_1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_NSE_OFFSET_1r,(_r._p1588_pim_nse_offset_1))
#define PLP_WRITE_P1588_PIM_NSE_OFFSET_1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_NSE_OFFSET_1r,(_r._p1588_pim_nse_offset_1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_NSE_OFFSET_1r PLP_P1588_PIM_NSE_OFFSET_1r
#define P1588_PIM_NSE_OFFSET_1r_SIZE PLP_P1588_PIM_NSE_OFFSET_1r_SIZE
typedef PLP_P1588_PIM_NSE_OFFSET_1r_t P1588_PIM_NSE_OFFSET_1r_t;
#define P1588_PIM_NSE_OFFSET_1r_CLR PLP_P1588_PIM_NSE_OFFSET_1r_CLR
#define P1588_PIM_NSE_OFFSET_1r_SET PLP_P1588_PIM_NSE_OFFSET_1r_SET
#define P1588_PIM_NSE_OFFSET_1r_GET PLP_P1588_PIM_NSE_OFFSET_1r_GET
#define P1588_PIM_NSE_OFFSET_1r_PIM_NSE_OFFSET_1f_GET PLP_P1588_PIM_NSE_OFFSET_1r_PIM_NSE_OFFSET_1f_GET
#define P1588_PIM_NSE_OFFSET_1r_PIM_NSE_OFFSET_1f_SET PLP_P1588_PIM_NSE_OFFSET_1r_PIM_NSE_OFFSET_1f_SET
#define READ_P1588_PIM_NSE_OFFSET_1r PLP_READ_P1588_PIM_NSE_OFFSET_1r
#define WRITE_P1588_PIM_NSE_OFFSET_1r PLP_WRITE_P1588_PIM_NSE_OFFSET_1r
#define MODIFY_P1588_PIM_NSE_OFFSET_1r PLP_MODIFY_P1588_PIM_NSE_OFFSET_1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_NSE_OFFSET_1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_NSE_OFFSET_2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70e7
 * DESC:     pim nse offset register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_NSE_OFFSET_2 The PIM NSE counter offset registerPIM_NSE_OFFSET = {PIM_NSE_OFFSET_5, PIM_NSE_OFFSET_4, PIM_NSE_OFFSET_3, PIM_NSE_OFFSET_2, PIM_NSE_OFFSET_1, PIM_NSE_OFFSET_0}Reset value is 0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_NSE_OFFSET_2r    0x000000e7

#define PLP_P1588_PIM_NSE_OFFSET_2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_NSE_OFFSET_2.
 *
 */
typedef union PLP_P1588_PIM_NSE_OFFSET_2r_s {
	uint32_t v[1];
	uint32_t p1588_pim_nse_offset_2[1];
	uint32_t _p1588_pim_nse_offset_2;
} PLP_P1588_PIM_NSE_OFFSET_2r_t;

#define PLP_P1588_PIM_NSE_OFFSET_2r_CLR(r) (r).p1588_pim_nse_offset_2[0] = 0
#define PLP_P1588_PIM_NSE_OFFSET_2r_SET(r,d) (r).p1588_pim_nse_offset_2[0] = d
#define PLP_P1588_PIM_NSE_OFFSET_2r_GET(r) (r).p1588_pim_nse_offset_2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_NSE_OFFSET_2r_PIM_NSE_OFFSET_2f_GET(r) (((r).p1588_pim_nse_offset_2[0]) & 0xffff)
#define PLP_P1588_PIM_NSE_OFFSET_2r_PIM_NSE_OFFSET_2f_SET(r,f) (r).p1588_pim_nse_offset_2[0]=(((r).p1588_pim_nse_offset_2[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PIM_NSE_OFFSET_2.
 *
 */
#define PLP_READ_P1588_PIM_NSE_OFFSET_2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_NSE_OFFSET_2r,(_r._p1588_pim_nse_offset_2))
#define PLP_WRITE_P1588_PIM_NSE_OFFSET_2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_NSE_OFFSET_2r,(_r._p1588_pim_nse_offset_2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_NSE_OFFSET_2r PLP_P1588_PIM_NSE_OFFSET_2r
#define P1588_PIM_NSE_OFFSET_2r_SIZE PLP_P1588_PIM_NSE_OFFSET_2r_SIZE
typedef PLP_P1588_PIM_NSE_OFFSET_2r_t P1588_PIM_NSE_OFFSET_2r_t;
#define P1588_PIM_NSE_OFFSET_2r_CLR PLP_P1588_PIM_NSE_OFFSET_2r_CLR
#define P1588_PIM_NSE_OFFSET_2r_SET PLP_P1588_PIM_NSE_OFFSET_2r_SET
#define P1588_PIM_NSE_OFFSET_2r_GET PLP_P1588_PIM_NSE_OFFSET_2r_GET
#define P1588_PIM_NSE_OFFSET_2r_PIM_NSE_OFFSET_2f_GET PLP_P1588_PIM_NSE_OFFSET_2r_PIM_NSE_OFFSET_2f_GET
#define P1588_PIM_NSE_OFFSET_2r_PIM_NSE_OFFSET_2f_SET PLP_P1588_PIM_NSE_OFFSET_2r_PIM_NSE_OFFSET_2f_SET
#define READ_P1588_PIM_NSE_OFFSET_2r PLP_READ_P1588_PIM_NSE_OFFSET_2r
#define WRITE_P1588_PIM_NSE_OFFSET_2r PLP_WRITE_P1588_PIM_NSE_OFFSET_2r
#define MODIFY_P1588_PIM_NSE_OFFSET_2r PLP_MODIFY_P1588_PIM_NSE_OFFSET_2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_NSE_OFFSET_2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_NSE_OFFSET_3
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70e8
 * DESC:     pim nse offset register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_NSE_OFFSET_3 The PIM NSE counter offset registerPIM_NSE_OFFSET = {PIM_NSE_OFFSET_5, PIM_NSE_OFFSET_4, PIM_NSE_OFFSET_3, PIM_NSE_OFFSET_2, PIM_NSE_OFFSET_1, PIM_NSE_OFFSET_0}Reset value is 0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_NSE_OFFSET_3r    0x000000e8

#define PLP_P1588_PIM_NSE_OFFSET_3r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_NSE_OFFSET_3.
 *
 */
typedef union PLP_P1588_PIM_NSE_OFFSET_3r_s {
	uint32_t v[1];
	uint32_t p1588_pim_nse_offset_3[1];
	uint32_t _p1588_pim_nse_offset_3;
} PLP_P1588_PIM_NSE_OFFSET_3r_t;

#define PLP_P1588_PIM_NSE_OFFSET_3r_CLR(r) (r).p1588_pim_nse_offset_3[0] = 0
#define PLP_P1588_PIM_NSE_OFFSET_3r_SET(r,d) (r).p1588_pim_nse_offset_3[0] = d
#define PLP_P1588_PIM_NSE_OFFSET_3r_GET(r) (r).p1588_pim_nse_offset_3[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_NSE_OFFSET_3r_PIM_NSE_OFFSET_3f_GET(r) (((r).p1588_pim_nse_offset_3[0]) & 0xffff)
#define PLP_P1588_PIM_NSE_OFFSET_3r_PIM_NSE_OFFSET_3f_SET(r,f) (r).p1588_pim_nse_offset_3[0]=(((r).p1588_pim_nse_offset_3[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PIM_NSE_OFFSET_3.
 *
 */
#define PLP_READ_P1588_PIM_NSE_OFFSET_3r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_NSE_OFFSET_3r,(_r._p1588_pim_nse_offset_3))
#define PLP_WRITE_P1588_PIM_NSE_OFFSET_3r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_NSE_OFFSET_3r,(_r._p1588_pim_nse_offset_3))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_NSE_OFFSET_3r PLP_P1588_PIM_NSE_OFFSET_3r
#define P1588_PIM_NSE_OFFSET_3r_SIZE PLP_P1588_PIM_NSE_OFFSET_3r_SIZE
typedef PLP_P1588_PIM_NSE_OFFSET_3r_t P1588_PIM_NSE_OFFSET_3r_t;
#define P1588_PIM_NSE_OFFSET_3r_CLR PLP_P1588_PIM_NSE_OFFSET_3r_CLR
#define P1588_PIM_NSE_OFFSET_3r_SET PLP_P1588_PIM_NSE_OFFSET_3r_SET
#define P1588_PIM_NSE_OFFSET_3r_GET PLP_P1588_PIM_NSE_OFFSET_3r_GET
#define P1588_PIM_NSE_OFFSET_3r_PIM_NSE_OFFSET_3f_GET PLP_P1588_PIM_NSE_OFFSET_3r_PIM_NSE_OFFSET_3f_GET
#define P1588_PIM_NSE_OFFSET_3r_PIM_NSE_OFFSET_3f_SET PLP_P1588_PIM_NSE_OFFSET_3r_PIM_NSE_OFFSET_3f_SET
#define READ_P1588_PIM_NSE_OFFSET_3r PLP_READ_P1588_PIM_NSE_OFFSET_3r
#define WRITE_P1588_PIM_NSE_OFFSET_3r PLP_WRITE_P1588_PIM_NSE_OFFSET_3r
#define MODIFY_P1588_PIM_NSE_OFFSET_3r PLP_MODIFY_P1588_PIM_NSE_OFFSET_3r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_NSE_OFFSET_3r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_NSE_OFFSET_4
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70e9
 * DESC:     pim nse offset register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_NSE_OFFSET_4 The PIM NSE counter offset registerPIM_NSE_OFFSET = {PIM_NSE_OFFSET_5, PIM_NSE_OFFSET_4, PIM_NSE_OFFSET_3, PIM_NSE_OFFSET_2, PIM_NSE_OFFSET_1, PIM_NSE_OFFSET_0}Reset value is 0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_NSE_OFFSET_4r    0x000000e9

#define PLP_P1588_PIM_NSE_OFFSET_4r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_NSE_OFFSET_4.
 *
 */
typedef union PLP_P1588_PIM_NSE_OFFSET_4r_s {
	uint32_t v[1];
	uint32_t p1588_pim_nse_offset_4[1];
	uint32_t _p1588_pim_nse_offset_4;
} PLP_P1588_PIM_NSE_OFFSET_4r_t;

#define PLP_P1588_PIM_NSE_OFFSET_4r_CLR(r) (r).p1588_pim_nse_offset_4[0] = 0
#define PLP_P1588_PIM_NSE_OFFSET_4r_SET(r,d) (r).p1588_pim_nse_offset_4[0] = d
#define PLP_P1588_PIM_NSE_OFFSET_4r_GET(r) (r).p1588_pim_nse_offset_4[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_NSE_OFFSET_4r_PIM_NSE_OFFSET_4f_GET(r) (((r).p1588_pim_nse_offset_4[0]) & 0xffff)
#define PLP_P1588_PIM_NSE_OFFSET_4r_PIM_NSE_OFFSET_4f_SET(r,f) (r).p1588_pim_nse_offset_4[0]=(((r).p1588_pim_nse_offset_4[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PIM_NSE_OFFSET_4.
 *
 */
#define PLP_READ_P1588_PIM_NSE_OFFSET_4r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_NSE_OFFSET_4r,(_r._p1588_pim_nse_offset_4))
#define PLP_WRITE_P1588_PIM_NSE_OFFSET_4r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_NSE_OFFSET_4r,(_r._p1588_pim_nse_offset_4))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_NSE_OFFSET_4r PLP_P1588_PIM_NSE_OFFSET_4r
#define P1588_PIM_NSE_OFFSET_4r_SIZE PLP_P1588_PIM_NSE_OFFSET_4r_SIZE
typedef PLP_P1588_PIM_NSE_OFFSET_4r_t P1588_PIM_NSE_OFFSET_4r_t;
#define P1588_PIM_NSE_OFFSET_4r_CLR PLP_P1588_PIM_NSE_OFFSET_4r_CLR
#define P1588_PIM_NSE_OFFSET_4r_SET PLP_P1588_PIM_NSE_OFFSET_4r_SET
#define P1588_PIM_NSE_OFFSET_4r_GET PLP_P1588_PIM_NSE_OFFSET_4r_GET
#define P1588_PIM_NSE_OFFSET_4r_PIM_NSE_OFFSET_4f_GET PLP_P1588_PIM_NSE_OFFSET_4r_PIM_NSE_OFFSET_4f_GET
#define P1588_PIM_NSE_OFFSET_4r_PIM_NSE_OFFSET_4f_SET PLP_P1588_PIM_NSE_OFFSET_4r_PIM_NSE_OFFSET_4f_SET
#define READ_P1588_PIM_NSE_OFFSET_4r PLP_READ_P1588_PIM_NSE_OFFSET_4r
#define WRITE_P1588_PIM_NSE_OFFSET_4r PLP_WRITE_P1588_PIM_NSE_OFFSET_4r
#define MODIFY_P1588_PIM_NSE_OFFSET_4r PLP_MODIFY_P1588_PIM_NSE_OFFSET_4r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_NSE_OFFSET_4r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_NSE_OFFSET_5
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70ea
 * DESC:     pim nse offset register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_NSE_OFFSET_5 The PIM NSE counter offset registerPIM_NSE_OFFSET = {PIM_NSE_OFFSET_5, PIM_NSE_OFFSET_4, PIM_NSE_OFFSET_3, PIM_NSE_OFFSET_2, PIM_NSE_OFFSET_1, PIM_NSE_OFFSET_0}Reset value is 0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_NSE_OFFSET_5r    0x000000ea

#define PLP_P1588_PIM_NSE_OFFSET_5r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_NSE_OFFSET_5.
 *
 */
typedef union PLP_P1588_PIM_NSE_OFFSET_5r_s {
	uint32_t v[1];
	uint32_t p1588_pim_nse_offset_5[1];
	uint32_t _p1588_pim_nse_offset_5;
} PLP_P1588_PIM_NSE_OFFSET_5r_t;

#define PLP_P1588_PIM_NSE_OFFSET_5r_CLR(r) (r).p1588_pim_nse_offset_5[0] = 0
#define PLP_P1588_PIM_NSE_OFFSET_5r_SET(r,d) (r).p1588_pim_nse_offset_5[0] = d
#define PLP_P1588_PIM_NSE_OFFSET_5r_GET(r) (r).p1588_pim_nse_offset_5[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_NSE_OFFSET_5r_PIM_NSE_OFFSET_5f_GET(r) (((r).p1588_pim_nse_offset_5[0]) & 0xffff)
#define PLP_P1588_PIM_NSE_OFFSET_5r_PIM_NSE_OFFSET_5f_SET(r,f) (r).p1588_pim_nse_offset_5[0]=(((r).p1588_pim_nse_offset_5[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PIM_NSE_OFFSET_5.
 *
 */
#define PLP_READ_P1588_PIM_NSE_OFFSET_5r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_NSE_OFFSET_5r,(_r._p1588_pim_nse_offset_5))
#define PLP_WRITE_P1588_PIM_NSE_OFFSET_5r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_NSE_OFFSET_5r,(_r._p1588_pim_nse_offset_5))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_NSE_OFFSET_5r PLP_P1588_PIM_NSE_OFFSET_5r
#define P1588_PIM_NSE_OFFSET_5r_SIZE PLP_P1588_PIM_NSE_OFFSET_5r_SIZE
typedef PLP_P1588_PIM_NSE_OFFSET_5r_t P1588_PIM_NSE_OFFSET_5r_t;
#define P1588_PIM_NSE_OFFSET_5r_CLR PLP_P1588_PIM_NSE_OFFSET_5r_CLR
#define P1588_PIM_NSE_OFFSET_5r_SET PLP_P1588_PIM_NSE_OFFSET_5r_SET
#define P1588_PIM_NSE_OFFSET_5r_GET PLP_P1588_PIM_NSE_OFFSET_5r_GET
#define P1588_PIM_NSE_OFFSET_5r_PIM_NSE_OFFSET_5f_GET PLP_P1588_PIM_NSE_OFFSET_5r_PIM_NSE_OFFSET_5f_GET
#define P1588_PIM_NSE_OFFSET_5r_PIM_NSE_OFFSET_5f_SET PLP_P1588_PIM_NSE_OFFSET_5r_PIM_NSE_OFFSET_5f_SET
#define READ_P1588_PIM_NSE_OFFSET_5r PLP_READ_P1588_PIM_NSE_OFFSET_5r
#define WRITE_P1588_PIM_NSE_OFFSET_5r PLP_WRITE_P1588_PIM_NSE_OFFSET_5r
#define MODIFY_P1588_PIM_NSE_OFFSET_5r PLP_MODIFY_P1588_PIM_NSE_OFFSET_5r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_NSE_OFFSET_5r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_NSE_TIME_0
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70eb
 * DESC:     pim nse offset register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_NSE_TIME_0   The PIM NSE counter offset registerPIM_NSE_OFFSET = {PIM_NSE_TIME_5, PIM_NSE_TIME_4, PIM_NSE_TIME_3, PIM_NSE_TIME_2, PIM_NSE_TIME_1, PIM_NSE_TIME_0}Reset value is 0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_NSE_TIME_0r    0x000000eb

#define PLP_P1588_PIM_NSE_TIME_0r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_NSE_TIME_0.
 *
 */
typedef union PLP_P1588_PIM_NSE_TIME_0r_s {
	uint32_t v[1];
	uint32_t p1588_pim_nse_time_0[1];
	uint32_t _p1588_pim_nse_time_0;
} PLP_P1588_PIM_NSE_TIME_0r_t;

#define PLP_P1588_PIM_NSE_TIME_0r_CLR(r) (r).p1588_pim_nse_time_0[0] = 0
#define PLP_P1588_PIM_NSE_TIME_0r_SET(r,d) (r).p1588_pim_nse_time_0[0] = d
#define PLP_P1588_PIM_NSE_TIME_0r_GET(r) (r).p1588_pim_nse_time_0[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_NSE_TIME_0r_PIM_NSE_TIME_0f_GET(r) (((r).p1588_pim_nse_time_0[0]) & 0xffff)
#define PLP_P1588_PIM_NSE_TIME_0r_PIM_NSE_TIME_0f_SET(r,f) (r).p1588_pim_nse_time_0[0]=(((r).p1588_pim_nse_time_0[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PIM_NSE_TIME_0.
 *
 */
#define PLP_READ_P1588_PIM_NSE_TIME_0r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_NSE_TIME_0r,(_r._p1588_pim_nse_time_0))
#define PLP_WRITE_P1588_PIM_NSE_TIME_0r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_NSE_TIME_0r,(_r._p1588_pim_nse_time_0))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_NSE_TIME_0r PLP_P1588_PIM_NSE_TIME_0r
#define P1588_PIM_NSE_TIME_0r_SIZE PLP_P1588_PIM_NSE_TIME_0r_SIZE
typedef PLP_P1588_PIM_NSE_TIME_0r_t P1588_PIM_NSE_TIME_0r_t;
#define P1588_PIM_NSE_TIME_0r_CLR PLP_P1588_PIM_NSE_TIME_0r_CLR
#define P1588_PIM_NSE_TIME_0r_SET PLP_P1588_PIM_NSE_TIME_0r_SET
#define P1588_PIM_NSE_TIME_0r_GET PLP_P1588_PIM_NSE_TIME_0r_GET
#define P1588_PIM_NSE_TIME_0r_PIM_NSE_TIME_0f_GET PLP_P1588_PIM_NSE_TIME_0r_PIM_NSE_TIME_0f_GET
#define P1588_PIM_NSE_TIME_0r_PIM_NSE_TIME_0f_SET PLP_P1588_PIM_NSE_TIME_0r_PIM_NSE_TIME_0f_SET
#define READ_P1588_PIM_NSE_TIME_0r PLP_READ_P1588_PIM_NSE_TIME_0r
#define WRITE_P1588_PIM_NSE_TIME_0r PLP_WRITE_P1588_PIM_NSE_TIME_0r
#define MODIFY_P1588_PIM_NSE_TIME_0r PLP_MODIFY_P1588_PIM_NSE_TIME_0r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_NSE_TIME_0r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_NSE_TIME_1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70ec
 * DESC:     pim nse offset register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_NSE_TIME_1   The PIM NSE counter offset registerPIM_NSE_OFFSET = {PIM_NSE_TIME_5, PIM_NSE_TIME_4, PIM_NSE_TIME_3, PIM_NSE_TIME_2, PIM_NSE_TIME_1, PIM_NSE_TIME_0}Reset value is 0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_NSE_TIME_1r    0x000000ec

#define PLP_P1588_PIM_NSE_TIME_1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_NSE_TIME_1.
 *
 */
typedef union PLP_P1588_PIM_NSE_TIME_1r_s {
	uint32_t v[1];
	uint32_t p1588_pim_nse_time_1[1];
	uint32_t _p1588_pim_nse_time_1;
} PLP_P1588_PIM_NSE_TIME_1r_t;

#define PLP_P1588_PIM_NSE_TIME_1r_CLR(r) (r).p1588_pim_nse_time_1[0] = 0
#define PLP_P1588_PIM_NSE_TIME_1r_SET(r,d) (r).p1588_pim_nse_time_1[0] = d
#define PLP_P1588_PIM_NSE_TIME_1r_GET(r) (r).p1588_pim_nse_time_1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_NSE_TIME_1r_PIM_NSE_TIME_1f_GET(r) (((r).p1588_pim_nse_time_1[0]) & 0xffff)
#define PLP_P1588_PIM_NSE_TIME_1r_PIM_NSE_TIME_1f_SET(r,f) (r).p1588_pim_nse_time_1[0]=(((r).p1588_pim_nse_time_1[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PIM_NSE_TIME_1.
 *
 */
#define PLP_READ_P1588_PIM_NSE_TIME_1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_NSE_TIME_1r,(_r._p1588_pim_nse_time_1))
#define PLP_WRITE_P1588_PIM_NSE_TIME_1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_NSE_TIME_1r,(_r._p1588_pim_nse_time_1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_NSE_TIME_1r PLP_P1588_PIM_NSE_TIME_1r
#define P1588_PIM_NSE_TIME_1r_SIZE PLP_P1588_PIM_NSE_TIME_1r_SIZE
typedef PLP_P1588_PIM_NSE_TIME_1r_t P1588_PIM_NSE_TIME_1r_t;
#define P1588_PIM_NSE_TIME_1r_CLR PLP_P1588_PIM_NSE_TIME_1r_CLR
#define P1588_PIM_NSE_TIME_1r_SET PLP_P1588_PIM_NSE_TIME_1r_SET
#define P1588_PIM_NSE_TIME_1r_GET PLP_P1588_PIM_NSE_TIME_1r_GET
#define P1588_PIM_NSE_TIME_1r_PIM_NSE_TIME_1f_GET PLP_P1588_PIM_NSE_TIME_1r_PIM_NSE_TIME_1f_GET
#define P1588_PIM_NSE_TIME_1r_PIM_NSE_TIME_1f_SET PLP_P1588_PIM_NSE_TIME_1r_PIM_NSE_TIME_1f_SET
#define READ_P1588_PIM_NSE_TIME_1r PLP_READ_P1588_PIM_NSE_TIME_1r
#define WRITE_P1588_PIM_NSE_TIME_1r PLP_WRITE_P1588_PIM_NSE_TIME_1r
#define MODIFY_P1588_PIM_NSE_TIME_1r PLP_MODIFY_P1588_PIM_NSE_TIME_1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_NSE_TIME_1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_NSE_TIME_2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70ed
 * DESC:     pim nse offset register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_NSE_TIME_2   The PIM NSE counter offset registerPIM_NSE_OFFSET = {PIM_NSE_TIME_5, PIM_NSE_TIME_4, PIM_NSE_TIME_3, PIM_NSE_TIME_2, PIM_NSE_TIME_1, PIM_NSE_TIME_0}Reset value is 0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_NSE_TIME_2r    0x000000ed

#define PLP_P1588_PIM_NSE_TIME_2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_NSE_TIME_2.
 *
 */
typedef union PLP_P1588_PIM_NSE_TIME_2r_s {
	uint32_t v[1];
	uint32_t p1588_pim_nse_time_2[1];
	uint32_t _p1588_pim_nse_time_2;
} PLP_P1588_PIM_NSE_TIME_2r_t;

#define PLP_P1588_PIM_NSE_TIME_2r_CLR(r) (r).p1588_pim_nse_time_2[0] = 0
#define PLP_P1588_PIM_NSE_TIME_2r_SET(r,d) (r).p1588_pim_nse_time_2[0] = d
#define PLP_P1588_PIM_NSE_TIME_2r_GET(r) (r).p1588_pim_nse_time_2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_NSE_TIME_2r_PIM_NSE_TIME_2f_GET(r) (((r).p1588_pim_nse_time_2[0]) & 0xffff)
#define PLP_P1588_PIM_NSE_TIME_2r_PIM_NSE_TIME_2f_SET(r,f) (r).p1588_pim_nse_time_2[0]=(((r).p1588_pim_nse_time_2[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PIM_NSE_TIME_2.
 *
 */
#define PLP_READ_P1588_PIM_NSE_TIME_2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_NSE_TIME_2r,(_r._p1588_pim_nse_time_2))
#define PLP_WRITE_P1588_PIM_NSE_TIME_2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_NSE_TIME_2r,(_r._p1588_pim_nse_time_2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_NSE_TIME_2r PLP_P1588_PIM_NSE_TIME_2r
#define P1588_PIM_NSE_TIME_2r_SIZE PLP_P1588_PIM_NSE_TIME_2r_SIZE
typedef PLP_P1588_PIM_NSE_TIME_2r_t P1588_PIM_NSE_TIME_2r_t;
#define P1588_PIM_NSE_TIME_2r_CLR PLP_P1588_PIM_NSE_TIME_2r_CLR
#define P1588_PIM_NSE_TIME_2r_SET PLP_P1588_PIM_NSE_TIME_2r_SET
#define P1588_PIM_NSE_TIME_2r_GET PLP_P1588_PIM_NSE_TIME_2r_GET
#define P1588_PIM_NSE_TIME_2r_PIM_NSE_TIME_2f_GET PLP_P1588_PIM_NSE_TIME_2r_PIM_NSE_TIME_2f_GET
#define P1588_PIM_NSE_TIME_2r_PIM_NSE_TIME_2f_SET PLP_P1588_PIM_NSE_TIME_2r_PIM_NSE_TIME_2f_SET
#define READ_P1588_PIM_NSE_TIME_2r PLP_READ_P1588_PIM_NSE_TIME_2r
#define WRITE_P1588_PIM_NSE_TIME_2r PLP_WRITE_P1588_PIM_NSE_TIME_2r
#define MODIFY_P1588_PIM_NSE_TIME_2r PLP_MODIFY_P1588_PIM_NSE_TIME_2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_NSE_TIME_2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_NSE_TIME_3
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70ee
 * DESC:     pim nse offset register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_NSE_TIME_3   The PIM NSE counter offset registerPIM_NSE_OFFSET = {PIM_NSE_TIME_5, PIM_NSE_TIME_4, PIM_NSE_TIME_3, PIM_NSE_TIME_2, PIM_NSE_TIME_1, PIM_NSE_TIME_0}Reset value is 0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_NSE_TIME_3r    0x000000ee

#define PLP_P1588_PIM_NSE_TIME_3r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_NSE_TIME_3.
 *
 */
typedef union PLP_P1588_PIM_NSE_TIME_3r_s {
	uint32_t v[1];
	uint32_t p1588_pim_nse_time_3[1];
	uint32_t _p1588_pim_nse_time_3;
} PLP_P1588_PIM_NSE_TIME_3r_t;

#define PLP_P1588_PIM_NSE_TIME_3r_CLR(r) (r).p1588_pim_nse_time_3[0] = 0
#define PLP_P1588_PIM_NSE_TIME_3r_SET(r,d) (r).p1588_pim_nse_time_3[0] = d
#define PLP_P1588_PIM_NSE_TIME_3r_GET(r) (r).p1588_pim_nse_time_3[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_NSE_TIME_3r_PIM_NSE_TIME_3f_GET(r) (((r).p1588_pim_nse_time_3[0]) & 0xffff)
#define PLP_P1588_PIM_NSE_TIME_3r_PIM_NSE_TIME_3f_SET(r,f) (r).p1588_pim_nse_time_3[0]=(((r).p1588_pim_nse_time_3[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PIM_NSE_TIME_3.
 *
 */
#define PLP_READ_P1588_PIM_NSE_TIME_3r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_NSE_TIME_3r,(_r._p1588_pim_nse_time_3))
#define PLP_WRITE_P1588_PIM_NSE_TIME_3r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_NSE_TIME_3r,(_r._p1588_pim_nse_time_3))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_NSE_TIME_3r PLP_P1588_PIM_NSE_TIME_3r
#define P1588_PIM_NSE_TIME_3r_SIZE PLP_P1588_PIM_NSE_TIME_3r_SIZE
typedef PLP_P1588_PIM_NSE_TIME_3r_t P1588_PIM_NSE_TIME_3r_t;
#define P1588_PIM_NSE_TIME_3r_CLR PLP_P1588_PIM_NSE_TIME_3r_CLR
#define P1588_PIM_NSE_TIME_3r_SET PLP_P1588_PIM_NSE_TIME_3r_SET
#define P1588_PIM_NSE_TIME_3r_GET PLP_P1588_PIM_NSE_TIME_3r_GET
#define P1588_PIM_NSE_TIME_3r_PIM_NSE_TIME_3f_GET PLP_P1588_PIM_NSE_TIME_3r_PIM_NSE_TIME_3f_GET
#define P1588_PIM_NSE_TIME_3r_PIM_NSE_TIME_3f_SET PLP_P1588_PIM_NSE_TIME_3r_PIM_NSE_TIME_3f_SET
#define READ_P1588_PIM_NSE_TIME_3r PLP_READ_P1588_PIM_NSE_TIME_3r
#define WRITE_P1588_PIM_NSE_TIME_3r PLP_WRITE_P1588_PIM_NSE_TIME_3r
#define MODIFY_P1588_PIM_NSE_TIME_3r PLP_MODIFY_P1588_PIM_NSE_TIME_3r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_NSE_TIME_3r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_NSE_TIME_4
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70ef
 * DESC:     pim nse offset register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_NSE_TIME_4   The PIM NSE counter offset registerPIM_NSE_OFFSET = {PIM_NSE_TIME_5, PIM_NSE_TIME_4, PIM_NSE_TIME_3, PIM_NSE_TIME_2, PIM_NSE_TIME_1, PIM_NSE_TIME_0}Reset value is 0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_NSE_TIME_4r    0x000000ef

#define PLP_P1588_PIM_NSE_TIME_4r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_NSE_TIME_4.
 *
 */
typedef union PLP_P1588_PIM_NSE_TIME_4r_s {
	uint32_t v[1];
	uint32_t p1588_pim_nse_time_4[1];
	uint32_t _p1588_pim_nse_time_4;
} PLP_P1588_PIM_NSE_TIME_4r_t;

#define PLP_P1588_PIM_NSE_TIME_4r_CLR(r) (r).p1588_pim_nse_time_4[0] = 0
#define PLP_P1588_PIM_NSE_TIME_4r_SET(r,d) (r).p1588_pim_nse_time_4[0] = d
#define PLP_P1588_PIM_NSE_TIME_4r_GET(r) (r).p1588_pim_nse_time_4[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_NSE_TIME_4r_PIM_NSE_TIME_4f_GET(r) (((r).p1588_pim_nse_time_4[0]) & 0xffff)
#define PLP_P1588_PIM_NSE_TIME_4r_PIM_NSE_TIME_4f_SET(r,f) (r).p1588_pim_nse_time_4[0]=(((r).p1588_pim_nse_time_4[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PIM_NSE_TIME_4.
 *
 */
#define PLP_READ_P1588_PIM_NSE_TIME_4r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_NSE_TIME_4r,(_r._p1588_pim_nse_time_4))
#define PLP_WRITE_P1588_PIM_NSE_TIME_4r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_NSE_TIME_4r,(_r._p1588_pim_nse_time_4))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_NSE_TIME_4r PLP_P1588_PIM_NSE_TIME_4r
#define P1588_PIM_NSE_TIME_4r_SIZE PLP_P1588_PIM_NSE_TIME_4r_SIZE
typedef PLP_P1588_PIM_NSE_TIME_4r_t P1588_PIM_NSE_TIME_4r_t;
#define P1588_PIM_NSE_TIME_4r_CLR PLP_P1588_PIM_NSE_TIME_4r_CLR
#define P1588_PIM_NSE_TIME_4r_SET PLP_P1588_PIM_NSE_TIME_4r_SET
#define P1588_PIM_NSE_TIME_4r_GET PLP_P1588_PIM_NSE_TIME_4r_GET
#define P1588_PIM_NSE_TIME_4r_PIM_NSE_TIME_4f_GET PLP_P1588_PIM_NSE_TIME_4r_PIM_NSE_TIME_4f_GET
#define P1588_PIM_NSE_TIME_4r_PIM_NSE_TIME_4f_SET PLP_P1588_PIM_NSE_TIME_4r_PIM_NSE_TIME_4f_SET
#define READ_P1588_PIM_NSE_TIME_4r PLP_READ_P1588_PIM_NSE_TIME_4r
#define WRITE_P1588_PIM_NSE_TIME_4r PLP_WRITE_P1588_PIM_NSE_TIME_4r
#define MODIFY_P1588_PIM_NSE_TIME_4r PLP_MODIFY_P1588_PIM_NSE_TIME_4r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_NSE_TIME_4r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_NSE_TIME_5
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70f0
 * DESC:     pim nse offset register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_NSE_TIME_5   The PIM NSE counter offset registerPIM_NSE_OFFSET = {PIM_NSE_TIME_5, PIM_NSE_TIME_4, PIM_NSE_TIME_3, PIM_NSE_TIME_2, PIM_NSE_TIME_1, PIM_NSE_TIME_0}Reset value is 0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_NSE_TIME_5r    0x000000f0

#define PLP_P1588_PIM_NSE_TIME_5r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_NSE_TIME_5.
 *
 */
typedef union PLP_P1588_PIM_NSE_TIME_5r_s {
	uint32_t v[1];
	uint32_t p1588_pim_nse_time_5[1];
	uint32_t _p1588_pim_nse_time_5;
} PLP_P1588_PIM_NSE_TIME_5r_t;

#define PLP_P1588_PIM_NSE_TIME_5r_CLR(r) (r).p1588_pim_nse_time_5[0] = 0
#define PLP_P1588_PIM_NSE_TIME_5r_SET(r,d) (r).p1588_pim_nse_time_5[0] = d
#define PLP_P1588_PIM_NSE_TIME_5r_GET(r) (r).p1588_pim_nse_time_5[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_NSE_TIME_5r_PIM_NSE_TIME_5f_GET(r) (((r).p1588_pim_nse_time_5[0]) & 0xffff)
#define PLP_P1588_PIM_NSE_TIME_5r_PIM_NSE_TIME_5f_SET(r,f) (r).p1588_pim_nse_time_5[0]=(((r).p1588_pim_nse_time_5[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

#define PLP_P1588_PIM_NSE_TIME_5r_CONVERT_80BIT_TO_48BITf_GET(r)     ((((r).p1588_pim_nse_time_5[0]) >> 12) & 0x1)
#define PLP_P1588_PIM_NSE_TIME_5r_CONVERT_80BIT_TO_48BITf_SET(r,f)   (r).p1588_pim_nse_time_5[0]=(((r).p1588_pim_nse_time_5[0] & ~((uint32_t)0x1 << 12)) | ((((uint32_t)f) & 0x1) << 12)) | (1 << (16 + 12))
#define PLP_P1588_PIM_NSE_TIME_5r_CONVERT_80BIT_INIT_48BITf_GET(r)   ((((r).p1588_pim_nse_time_5[0]) >> 11) & 0x1)
#define PLP_P1588_PIM_NSE_TIME_5r_CONVERT_80BIT_INIT_48BITf_SET(r,f) (r).p1588_pim_nse_time_5[0]=(((r).p1588_pim_nse_time_5[0] & ~((uint32_t)0x1 << 11)) | ((((uint32_t)f) & 0x1) << 11)) | (1 << (16 + 11))
#define PLP_P1588_PIM_NSE_TIME_5r_CPU_EMULATE_SYNC0f_GET(r)          ((((r).p1588_pim_nse_time_5[0]) >>  6) & 0x1)
#define PLP_P1588_PIM_NSE_TIME_5r_CPU_EMULATE_SYNC0f_SET(r,f)        (r).p1588_pim_nse_time_5[0]=(((r).p1588_pim_nse_time_5[0] & ~((uint32_t)0x1 <<  6)) | ((((uint32_t)f) & 0x1) <<  6)) | (1 << (16 +  6))
#define PLP_P1588_PIM_NSE_TIME_5r_ENABLE_TS_CAPTUREf_GET(r)          ((((r).p1588_pim_nse_time_5[0]) >>  3) & 0x1)
#define PLP_P1588_PIM_NSE_TIME_5r_ENABLE_TS_CAPTUREf_SET(r,f)        (r).p1588_pim_nse_time_5[0]=(((r).p1588_pim_nse_time_5[0] & ~((uint32_t)0x1 <<  3)) | ((((uint32_t)f) & 0x1) <<  3)) | (1 << (16 +  3))
#define PLP_P1588_PIM_NSE_TIME_5r_NSE_GEN2_MODEf_GET(r)              ((((r).p1588_pim_nse_time_5[0]) >>  1) & 0x3)
#define PLP_P1588_PIM_NSE_TIME_5r_NSE_GEN2_MODEf_SET(r,f)            (r).p1588_pim_nse_time_5[0]=(((r).p1588_pim_nse_time_5[0] & ~((uint32_t)0x3 <<  1)) | ((((uint32_t)f) & 0x3) <<  1)) | (3 << (16 +  1))
#define PLP_P1588_PIM_NSE_TIME_5r_ENABLE_NSE_GEN2f_GET(r)            (((r).p1588_pim_nse_time_5[0]) & 0x1)
#define PLP_P1588_PIM_NSE_TIME_5r_ENABLE_NSE_GEN2f_SET(r,f)          (r).p1588_pim_nse_time_5[0]=(((r).p1588_pim_nse_time_5[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1)) | (0x1 << 16)

/*
 * These macros can be used to access P1588_PIM_NSE_TIME_5.
 *
 */
#define PLP_READ_P1588_PIM_NSE_TIME_5r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_NSE_TIME_5r,(_r._p1588_pim_nse_time_5))
#define PLP_WRITE_P1588_PIM_NSE_TIME_5r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_NSE_TIME_5r,(_r._p1588_pim_nse_time_5))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_NSE_TIME_5r PLP_P1588_PIM_NSE_TIME_5r
#define P1588_PIM_NSE_TIME_5r_SIZE PLP_P1588_PIM_NSE_TIME_5r_SIZE
typedef PLP_P1588_PIM_NSE_TIME_5r_t P1588_PIM_NSE_TIME_5r_t;
#define P1588_PIM_NSE_TIME_5r_CLR PLP_P1588_PIM_NSE_TIME_5r_CLR
#define P1588_PIM_NSE_TIME_5r_SET PLP_P1588_PIM_NSE_TIME_5r_SET
#define P1588_PIM_NSE_TIME_5r_GET PLP_P1588_PIM_NSE_TIME_5r_GET
#define P1588_PIM_NSE_TIME_5r_PIM_NSE_TIME_5f_GET PLP_P1588_PIM_NSE_TIME_5r_PIM_NSE_TIME_5f_GET
#define P1588_PIM_NSE_TIME_5r_PIM_NSE_TIME_5f_SET PLP_P1588_PIM_NSE_TIME_5r_PIM_NSE_TIME_5f_SET
#define P1588_PIM_NSE_TIME_5r_80BIT_TO_48BITf_GET PLP_P1588_PIM_NSE_TIME_5r_CONVERT_80BIT_TO_48BITf_GET
#define P1588_PIM_NSE_TIME_5r_80BIT_TO_48BITf_SET PLP_P1588_PIM_NSE_TIME_5r_CONVERT_80BIT_TO_48BITf_SET
#define P1588_PIM_NSE_TIME_5r_80BIT_INIT_48BITf_GET PLP_P1588_PIM_NSE_TIME_5r_CONVERT_80BIT_INIT_48BITf_GET
#define P1588_PIM_NSE_TIME_5r_80BIT_INIT_48BITf_SET PLP_P1588_PIM_NSE_TIME_5r_CONVERT_80BIT_INIT_48BITf_SET
#define P1588_PIM_NSE_TIME_5r_CPU_EMULATE_SYNC0f_GET PLP_P1588_PIM_NSE_TIME_5r_CPU_EMULATE_SYNC0f_GET
#define P1588_PIM_NSE_TIME_5r_CPU_EMULATE_SYNC0f_SET PLP_P1588_PIM_NSE_TIME_5r_CPU_EMULATE_SYNC0f_SET
#define P1588_PIM_NSE_TIME_5r_ENABLE_TS_CAPTUREf_GET PLP_P1588_PIM_NSE_TIME_5r_ENABLE_TS_CAPTUREf_GET
#define P1588_PIM_NSE_TIME_5r_ENABLE_TS_CAPTUREf_SET PLP_P1588_PIM_NSE_TIME_5r_ENABLE_TS_CAPTUREf_SET
#define P1588_PIM_NSE_TIME_5r_NSE_GEN2_MODEf_GET PLP_P1588_PIM_NSE_TIME_5r_NSE_GEN2_MODEf_GET
#define P1588_PIM_NSE_TIME_5r_NSE_GEN2_MODEf_SET PLP_P1588_PIM_NSE_TIME_5r_NSE_GEN2_MODEf_SET
#define P1588_PIM_NSE_TIME_5r_ENABLE_NSE_GEN2f_GET PLP_P1588_PIM_NSE_TIME_5r_ENABLE_NSE_GEN2f_GET
#define P1588_PIM_NSE_TIME_5r_ENABLE_NSE_GEN2f_SET PLP_P1588_PIM_NSE_TIME_5r_ENABLE_NSE_GEN2f_SET

#define P1588_PIM_NSE_GEN2_MODE_IDLE                0x0
#define P1588_PIM_NSE_GEN2_MODE_1                   0x1
#define P1588_PIM_NSE_GEN2_MODE_2                   0x2
#define P1588_PIM_NSE_GEN2_MODE_3                   0x3

#define READ_P1588_PIM_NSE_TIME_5r PLP_READ_P1588_PIM_NSE_TIME_5r
#define WRITE_P1588_PIM_NSE_TIME_5r PLP_WRITE_P1588_PIM_NSE_TIME_5r
#define MODIFY_P1588_PIM_NSE_TIME_5r PLP_MODIFY_P1588_PIM_NSE_TIME_5r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_NSE_TIME_5r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_NSE_FREQ_0
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70f1
 * DESC:     pim nse offset register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_NSE_FREQ_0   The PIM NSE counter frequency registerPIM_NSE_OFFSET = {PIM_NSE_FREQ_2, PIM_NSE_FREQ_1, PIM_NSE_FREQ_0}Fractional NS bit 15:0Reset value is 0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_NSE_FREQ_0r    0x000000f1

#define PLP_P1588_PIM_NSE_FREQ_0r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_NSE_FREQ_0.
 *
 */
typedef union PLP_P1588_PIM_NSE_FREQ_0r_s {
	uint32_t v[1];
	uint32_t p1588_pim_nse_freq_0[1];
	uint32_t _p1588_pim_nse_freq_0;
} PLP_P1588_PIM_NSE_FREQ_0r_t;

#define PLP_P1588_PIM_NSE_FREQ_0r_CLR(r) (r).p1588_pim_nse_freq_0[0] = 0
#define PLP_P1588_PIM_NSE_FREQ_0r_SET(r,d) (r).p1588_pim_nse_freq_0[0] = d
#define PLP_P1588_PIM_NSE_FREQ_0r_GET(r) (r).p1588_pim_nse_freq_0[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_NSE_FREQ_0r_PIM_NSE_FREQ_0f_GET(r) (((r).p1588_pim_nse_freq_0[0]) & 0xffff)
#define PLP_P1588_PIM_NSE_FREQ_0r_PIM_NSE_FREQ_0f_SET(r,f) (r).p1588_pim_nse_freq_0[0]=(((r).p1588_pim_nse_freq_0[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PIM_NSE_FREQ_0.
 *
 */
#define PLP_READ_P1588_PIM_NSE_FREQ_0r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_NSE_FREQ_0r,(_r._p1588_pim_nse_freq_0))
#define PLP_WRITE_P1588_PIM_NSE_FREQ_0r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_NSE_FREQ_0r,(_r._p1588_pim_nse_freq_0))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_NSE_FREQ_0r PLP_P1588_PIM_NSE_FREQ_0r
#define P1588_PIM_NSE_FREQ_0r_SIZE PLP_P1588_PIM_NSE_FREQ_0r_SIZE
typedef PLP_P1588_PIM_NSE_FREQ_0r_t P1588_PIM_NSE_FREQ_0r_t;
#define P1588_PIM_NSE_FREQ_0r_CLR PLP_P1588_PIM_NSE_FREQ_0r_CLR
#define P1588_PIM_NSE_FREQ_0r_SET PLP_P1588_PIM_NSE_FREQ_0r_SET
#define P1588_PIM_NSE_FREQ_0r_GET PLP_P1588_PIM_NSE_FREQ_0r_GET
#define P1588_PIM_NSE_FREQ_0r_PIM_NSE_FREQ_0f_GET PLP_P1588_PIM_NSE_FREQ_0r_PIM_NSE_FREQ_0f_GET
#define P1588_PIM_NSE_FREQ_0r_PIM_NSE_FREQ_0f_SET PLP_P1588_PIM_NSE_FREQ_0r_PIM_NSE_FREQ_0f_SET
#define READ_P1588_PIM_NSE_FREQ_0r PLP_READ_P1588_PIM_NSE_FREQ_0r
#define WRITE_P1588_PIM_NSE_FREQ_0r PLP_WRITE_P1588_PIM_NSE_FREQ_0r
#define MODIFY_P1588_PIM_NSE_FREQ_0r PLP_MODIFY_P1588_PIM_NSE_FREQ_0r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_NSE_FREQ_0r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_NSE_FREQ_1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70f2
 * DESC:     pim nse offset register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_NSE_FREQ_1   The PIM NSE counter frequency registerPIM_NSE_OFFSET = {PIM_NSE_FREQ_2, PIM_NSE_FREQ_1, PIM_NSE_FREQ_0}Fractional NS bit 31:16Reset value is 0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_NSE_FREQ_1r    0x000000f2

#define PLP_P1588_PIM_NSE_FREQ_1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_NSE_FREQ_1.
 *
 */
typedef union PLP_P1588_PIM_NSE_FREQ_1r_s {
	uint32_t v[1];
	uint32_t p1588_pim_nse_freq_1[1];
	uint32_t _p1588_pim_nse_freq_1;
} PLP_P1588_PIM_NSE_FREQ_1r_t;

#define PLP_P1588_PIM_NSE_FREQ_1r_CLR(r) (r).p1588_pim_nse_freq_1[0] = 0
#define PLP_P1588_PIM_NSE_FREQ_1r_SET(r,d) (r).p1588_pim_nse_freq_1[0] = d
#define PLP_P1588_PIM_NSE_FREQ_1r_GET(r) (r).p1588_pim_nse_freq_1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_NSE_FREQ_1r_PIM_NSE_FREQ_1f_GET(r) (((r).p1588_pim_nse_freq_1[0]) & 0xffff)
#define PLP_P1588_PIM_NSE_FREQ_1r_PIM_NSE_FREQ_1f_SET(r,f) (r).p1588_pim_nse_freq_1[0]=(((r).p1588_pim_nse_freq_1[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PIM_NSE_FREQ_1.
 *
 */
#define PLP_READ_P1588_PIM_NSE_FREQ_1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_NSE_FREQ_1r,(_r._p1588_pim_nse_freq_1))
#define PLP_WRITE_P1588_PIM_NSE_FREQ_1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_NSE_FREQ_1r,(_r._p1588_pim_nse_freq_1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_NSE_FREQ_1r PLP_P1588_PIM_NSE_FREQ_1r
#define P1588_PIM_NSE_FREQ_1r_SIZE PLP_P1588_PIM_NSE_FREQ_1r_SIZE
typedef PLP_P1588_PIM_NSE_FREQ_1r_t P1588_PIM_NSE_FREQ_1r_t;
#define P1588_PIM_NSE_FREQ_1r_CLR PLP_P1588_PIM_NSE_FREQ_1r_CLR
#define P1588_PIM_NSE_FREQ_1r_SET PLP_P1588_PIM_NSE_FREQ_1r_SET
#define P1588_PIM_NSE_FREQ_1r_GET PLP_P1588_PIM_NSE_FREQ_1r_GET
#define P1588_PIM_NSE_FREQ_1r_PIM_NSE_FREQ_1f_GET PLP_P1588_PIM_NSE_FREQ_1r_PIM_NSE_FREQ_1f_GET
#define P1588_PIM_NSE_FREQ_1r_PIM_NSE_FREQ_1f_SET PLP_P1588_PIM_NSE_FREQ_1r_PIM_NSE_FREQ_1f_SET
#define READ_P1588_PIM_NSE_FREQ_1r PLP_READ_P1588_PIM_NSE_FREQ_1r
#define WRITE_P1588_PIM_NSE_FREQ_1r PLP_WRITE_P1588_PIM_NSE_FREQ_1r
#define MODIFY_P1588_PIM_NSE_FREQ_1r PLP_MODIFY_P1588_PIM_NSE_FREQ_1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_NSE_FREQ_1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_NSE_FREQ_2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70f3
 * DESC:     pim nse offset register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_NSE_FREQ_2   The PIM NSE counter frequency registerPIM_NSE_OFFSET = {PIM_NSE_FREQ_2, PIM_NSE_FREQ_1, PIM_NSE_FREQ_0}NS bit 7:0NS bit 15:8 are not usedReset value is 0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_NSE_FREQ_2r    0x000000f3

#define PLP_P1588_PIM_NSE_FREQ_2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_NSE_FREQ_2.
 *
 */
typedef union PLP_P1588_PIM_NSE_FREQ_2r_s {
	uint32_t v[1];
	uint32_t p1588_pim_nse_freq_2[1];
	uint32_t _p1588_pim_nse_freq_2;
} PLP_P1588_PIM_NSE_FREQ_2r_t;

#define PLP_P1588_PIM_NSE_FREQ_2r_CLR(r) (r).p1588_pim_nse_freq_2[0] = 0
#define PLP_P1588_PIM_NSE_FREQ_2r_SET(r,d) (r).p1588_pim_nse_freq_2[0] = d
#define PLP_P1588_PIM_NSE_FREQ_2r_GET(r) (r).p1588_pim_nse_freq_2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_NSE_FREQ_2r_PIM_NSE_FREQ_2f_GET(r) (((r).p1588_pim_nse_freq_2[0]) & 0xffff)
#define PLP_P1588_PIM_NSE_FREQ_2r_PIM_NSE_FREQ_2f_SET(r,f) (r).p1588_pim_nse_freq_2[0]=(((r).p1588_pim_nse_freq_2[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PIM_NSE_FREQ_2.
 *
 */
#define PLP_READ_P1588_PIM_NSE_FREQ_2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_NSE_FREQ_2r,(_r._p1588_pim_nse_freq_2))
#define PLP_WRITE_P1588_PIM_NSE_FREQ_2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_NSE_FREQ_2r,(_r._p1588_pim_nse_freq_2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_NSE_FREQ_2r PLP_P1588_PIM_NSE_FREQ_2r
#define P1588_PIM_NSE_FREQ_2r_SIZE PLP_P1588_PIM_NSE_FREQ_2r_SIZE
typedef PLP_P1588_PIM_NSE_FREQ_2r_t P1588_PIM_NSE_FREQ_2r_t;
#define P1588_PIM_NSE_FREQ_2r_CLR PLP_P1588_PIM_NSE_FREQ_2r_CLR
#define P1588_PIM_NSE_FREQ_2r_SET PLP_P1588_PIM_NSE_FREQ_2r_SET
#define P1588_PIM_NSE_FREQ_2r_GET PLP_P1588_PIM_NSE_FREQ_2r_GET
#define P1588_PIM_NSE_FREQ_2r_PIM_NSE_FREQ_2f_GET PLP_P1588_PIM_NSE_FREQ_2r_PIM_NSE_FREQ_2f_GET
#define P1588_PIM_NSE_FREQ_2r_PIM_NSE_FREQ_2f_SET PLP_P1588_PIM_NSE_FREQ_2r_PIM_NSE_FREQ_2f_SET
#define READ_P1588_PIM_NSE_FREQ_2r PLP_READ_P1588_PIM_NSE_FREQ_2r
#define WRITE_P1588_PIM_NSE_FREQ_2r PLP_WRITE_P1588_PIM_NSE_FREQ_2r
#define MODIFY_P1588_PIM_NSE_FREQ_2r PLP_MODIFY_P1588_PIM_NSE_FREQ_2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_NSE_FREQ_2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_HEARTBEAT_0
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70f4
 * DESC:     pim heartbeat register(0)
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_HEARTBEAT_0  Output of the snapshot of the time stampPIM_HEARTBEAT = {PIM_HEARTBEAT_5, PIM_HEARTBEAT_4, PIM_HEARTBEAT_3, PIM_HEARTBEAT_2, PIM_HEARTBEAT_1, PIM_HEARTBEAT_0}Reset value is 0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_HEARTBEAT_0r    0x000000f4

#define PLP_P1588_PIM_HEARTBEAT_0r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_HEARTBEAT_0.
 *
 */
typedef union PLP_P1588_PIM_HEARTBEAT_0r_s {
	uint32_t v[1];
	uint32_t p1588_pim_heartbeat_0[1];
	uint32_t _p1588_pim_heartbeat_0;
} PLP_P1588_PIM_HEARTBEAT_0r_t;

#define PLP_P1588_PIM_HEARTBEAT_0r_CLR(r) (r).p1588_pim_heartbeat_0[0] = 0
#define PLP_P1588_PIM_HEARTBEAT_0r_SET(r,d) (r).p1588_pim_heartbeat_0[0] = d
#define PLP_P1588_PIM_HEARTBEAT_0r_GET(r) (r).p1588_pim_heartbeat_0[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_HEARTBEAT_0r_PIM_HEARTBEAT_0f_GET(r) (((r).p1588_pim_heartbeat_0[0]) & 0xffff)
#define PLP_P1588_PIM_HEARTBEAT_0r_PIM_HEARTBEAT_0f_SET(r,f) (r).p1588_pim_heartbeat_0[0]=(((r).p1588_pim_heartbeat_0[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PIM_HEARTBEAT_0.
 *
 */
#define PLP_READ_P1588_PIM_HEARTBEAT_0r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_HEARTBEAT_0r,(_r._p1588_pim_heartbeat_0))
#define PLP_WRITE_P1588_PIM_HEARTBEAT_0r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_HEARTBEAT_0r,(_r._p1588_pim_heartbeat_0))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_HEARTBEAT_0r PLP_P1588_PIM_HEARTBEAT_0r
#define P1588_PIM_HEARTBEAT_0r_SIZE PLP_P1588_PIM_HEARTBEAT_0r_SIZE
typedef PLP_P1588_PIM_HEARTBEAT_0r_t P1588_PIM_HEARTBEAT_0r_t;
#define P1588_PIM_HEARTBEAT_0r_CLR PLP_P1588_PIM_HEARTBEAT_0r_CLR
#define P1588_PIM_HEARTBEAT_0r_SET PLP_P1588_PIM_HEARTBEAT_0r_SET
#define P1588_PIM_HEARTBEAT_0r_GET PLP_P1588_PIM_HEARTBEAT_0r_GET
#define P1588_PIM_HEARTBEAT_0r_PIM_HEARTBEAT_0f_GET PLP_P1588_PIM_HEARTBEAT_0r_PIM_HEARTBEAT_0f_GET
#define P1588_PIM_HEARTBEAT_0r_PIM_HEARTBEAT_0f_SET PLP_P1588_PIM_HEARTBEAT_0r_PIM_HEARTBEAT_0f_SET
#define READ_P1588_PIM_HEARTBEAT_0r PLP_READ_P1588_PIM_HEARTBEAT_0r
#define WRITE_P1588_PIM_HEARTBEAT_0r PLP_WRITE_P1588_PIM_HEARTBEAT_0r
#define MODIFY_P1588_PIM_HEARTBEAT_0r PLP_MODIFY_P1588_PIM_HEARTBEAT_0r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_HEARTBEAT_0r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_HEARTBEAT_1
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70f5
 * DESC:     pim heartbeat register(1)
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_HEARTBEAT_1  Output of the snapshot of the time stampPIM_HEARTBEAT = {PIM_HEARTBEAT_5, PIM_HEARTBEAT_4, PIM_HEARTBEAT_3, PIM_HEARTBEAT_2, PIM_HEARTBEAT_1, PIM_HEARTBEAT_0}Reset value is 0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_HEARTBEAT_1r    0x000000f5

#define PLP_P1588_PIM_HEARTBEAT_1r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_HEARTBEAT_1.
 *
 */
typedef union PLP_P1588_PIM_HEARTBEAT_1r_s {
	uint32_t v[1];
	uint32_t p1588_pim_heartbeat_1[1];
	uint32_t _p1588_pim_heartbeat_1;
} PLP_P1588_PIM_HEARTBEAT_1r_t;

#define PLP_P1588_PIM_HEARTBEAT_1r_CLR(r) (r).p1588_pim_heartbeat_1[0] = 0
#define PLP_P1588_PIM_HEARTBEAT_1r_SET(r,d) (r).p1588_pim_heartbeat_1[0] = d
#define PLP_P1588_PIM_HEARTBEAT_1r_GET(r) (r).p1588_pim_heartbeat_1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_HEARTBEAT_1r_PIM_HEARTBEAT_1f_GET(r) (((r).p1588_pim_heartbeat_1[0]) & 0xffff)
#define PLP_P1588_PIM_HEARTBEAT_1r_PIM_HEARTBEAT_1f_SET(r,f) (r).p1588_pim_heartbeat_1[0]=(((r).p1588_pim_heartbeat_1[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PIM_HEARTBEAT_1.
 *
 */
#define PLP_READ_P1588_PIM_HEARTBEAT_1r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_HEARTBEAT_1r,(_r._p1588_pim_heartbeat_1))
#define PLP_WRITE_P1588_PIM_HEARTBEAT_1r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_HEARTBEAT_1r,(_r._p1588_pim_heartbeat_1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_HEARTBEAT_1r PLP_P1588_PIM_HEARTBEAT_1r
#define P1588_PIM_HEARTBEAT_1r_SIZE PLP_P1588_PIM_HEARTBEAT_1r_SIZE
typedef PLP_P1588_PIM_HEARTBEAT_1r_t P1588_PIM_HEARTBEAT_1r_t;
#define P1588_PIM_HEARTBEAT_1r_CLR PLP_P1588_PIM_HEARTBEAT_1r_CLR
#define P1588_PIM_HEARTBEAT_1r_SET PLP_P1588_PIM_HEARTBEAT_1r_SET
#define P1588_PIM_HEARTBEAT_1r_GET PLP_P1588_PIM_HEARTBEAT_1r_GET
#define P1588_PIM_HEARTBEAT_1r_PIM_HEARTBEAT_1f_GET PLP_P1588_PIM_HEARTBEAT_1r_PIM_HEARTBEAT_1f_GET
#define P1588_PIM_HEARTBEAT_1r_PIM_HEARTBEAT_1f_SET PLP_P1588_PIM_HEARTBEAT_1r_PIM_HEARTBEAT_1f_SET
#define READ_P1588_PIM_HEARTBEAT_1r PLP_READ_P1588_PIM_HEARTBEAT_1r
#define WRITE_P1588_PIM_HEARTBEAT_1r PLP_WRITE_P1588_PIM_HEARTBEAT_1r
#define MODIFY_P1588_PIM_HEARTBEAT_1r PLP_MODIFY_P1588_PIM_HEARTBEAT_1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_HEARTBEAT_1r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_HEARTBEAT_2
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70f6
 * DESC:     pim heartbeat register(2)
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_HEARTBEAT_2  Output of the snapshot of the time stampPIM_HEARTBEAT = {PIM_HEARTBEAT_5, PIM_HEARTBEAT_4, PIM_HEARTBEAT_3, PIM_HEARTBEAT_2, PIM_HEARTBEAT_1, PIM_HEARTBEAT_0}Reset value is 0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_HEARTBEAT_2r    0x000000f6

#define PLP_P1588_PIM_HEARTBEAT_2r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_HEARTBEAT_2.
 *
 */
typedef union PLP_P1588_PIM_HEARTBEAT_2r_s {
	uint32_t v[1];
	uint32_t p1588_pim_heartbeat_2[1];
	uint32_t _p1588_pim_heartbeat_2;
} PLP_P1588_PIM_HEARTBEAT_2r_t;

#define PLP_P1588_PIM_HEARTBEAT_2r_CLR(r) (r).p1588_pim_heartbeat_2[0] = 0
#define PLP_P1588_PIM_HEARTBEAT_2r_SET(r,d) (r).p1588_pim_heartbeat_2[0] = d
#define PLP_P1588_PIM_HEARTBEAT_2r_GET(r) (r).p1588_pim_heartbeat_2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_HEARTBEAT_2r_PIM_HEARTBEAT_2f_GET(r) (((r).p1588_pim_heartbeat_2[0]) & 0xffff)
#define PLP_P1588_PIM_HEARTBEAT_2r_PIM_HEARTBEAT_2f_SET(r,f) (r).p1588_pim_heartbeat_2[0]=(((r).p1588_pim_heartbeat_2[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PIM_HEARTBEAT_2.
 *
 */
#define PLP_READ_P1588_PIM_HEARTBEAT_2r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_HEARTBEAT_2r,(_r._p1588_pim_heartbeat_2))
#define PLP_WRITE_P1588_PIM_HEARTBEAT_2r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_HEARTBEAT_2r,(_r._p1588_pim_heartbeat_2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_HEARTBEAT_2r PLP_P1588_PIM_HEARTBEAT_2r
#define P1588_PIM_HEARTBEAT_2r_SIZE PLP_P1588_PIM_HEARTBEAT_2r_SIZE
typedef PLP_P1588_PIM_HEARTBEAT_2r_t P1588_PIM_HEARTBEAT_2r_t;
#define P1588_PIM_HEARTBEAT_2r_CLR PLP_P1588_PIM_HEARTBEAT_2r_CLR
#define P1588_PIM_HEARTBEAT_2r_SET PLP_P1588_PIM_HEARTBEAT_2r_SET
#define P1588_PIM_HEARTBEAT_2r_GET PLP_P1588_PIM_HEARTBEAT_2r_GET
#define P1588_PIM_HEARTBEAT_2r_PIM_HEARTBEAT_2f_GET PLP_P1588_PIM_HEARTBEAT_2r_PIM_HEARTBEAT_2f_GET
#define P1588_PIM_HEARTBEAT_2r_PIM_HEARTBEAT_2f_SET PLP_P1588_PIM_HEARTBEAT_2r_PIM_HEARTBEAT_2f_SET
#define READ_P1588_PIM_HEARTBEAT_2r PLP_READ_P1588_PIM_HEARTBEAT_2r
#define WRITE_P1588_PIM_HEARTBEAT_2r PLP_WRITE_P1588_PIM_HEARTBEAT_2r
#define MODIFY_P1588_PIM_HEARTBEAT_2r PLP_MODIFY_P1588_PIM_HEARTBEAT_2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_HEARTBEAT_2r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_HEARTBEAT_3
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70f7
 * DESC:     pim heartbeat register(3)
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_HEARTBEAT_3  Output of the snapshot of the time stampPIM_HEARTBEAT = {PIM_HEARTBEAT_5, PIM_HEARTBEAT_4, PIM_HEARTBEAT_3, PIM_HEARTBEAT_2, PIM_HEARTBEAT_1, PIM_HEARTBEAT_0}Reset value is 0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_HEARTBEAT_3r    0x000000f7

#define PLP_P1588_PIM_HEARTBEAT_3r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_HEARTBEAT_3.
 *
 */
typedef union PLP_P1588_PIM_HEARTBEAT_3r_s {
	uint32_t v[1];
	uint32_t p1588_pim_heartbeat_3[1];
	uint32_t _p1588_pim_heartbeat_3;
} PLP_P1588_PIM_HEARTBEAT_3r_t;

#define PLP_P1588_PIM_HEARTBEAT_3r_CLR(r) (r).p1588_pim_heartbeat_3[0] = 0
#define PLP_P1588_PIM_HEARTBEAT_3r_SET(r,d) (r).p1588_pim_heartbeat_3[0] = d
#define PLP_P1588_PIM_HEARTBEAT_3r_GET(r) (r).p1588_pim_heartbeat_3[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_HEARTBEAT_3r_PIM_HEARTBEAT_3f_GET(r) (((r).p1588_pim_heartbeat_3[0]) & 0xffff)
#define PLP_P1588_PIM_HEARTBEAT_3r_PIM_HEARTBEAT_3f_SET(r,f) (r).p1588_pim_heartbeat_3[0]=(((r).p1588_pim_heartbeat_3[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PIM_HEARTBEAT_3.
 *
 */
#define PLP_READ_P1588_PIM_HEARTBEAT_3r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_HEARTBEAT_3r,(_r._p1588_pim_heartbeat_3))
#define PLP_WRITE_P1588_PIM_HEARTBEAT_3r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_HEARTBEAT_3r,(_r._p1588_pim_heartbeat_3))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_HEARTBEAT_3r PLP_P1588_PIM_HEARTBEAT_3r
#define P1588_PIM_HEARTBEAT_3r_SIZE PLP_P1588_PIM_HEARTBEAT_3r_SIZE
typedef PLP_P1588_PIM_HEARTBEAT_3r_t P1588_PIM_HEARTBEAT_3r_t;
#define P1588_PIM_HEARTBEAT_3r_CLR PLP_P1588_PIM_HEARTBEAT_3r_CLR
#define P1588_PIM_HEARTBEAT_3r_SET PLP_P1588_PIM_HEARTBEAT_3r_SET
#define P1588_PIM_HEARTBEAT_3r_GET PLP_P1588_PIM_HEARTBEAT_3r_GET
#define P1588_PIM_HEARTBEAT_3r_PIM_HEARTBEAT_3f_GET PLP_P1588_PIM_HEARTBEAT_3r_PIM_HEARTBEAT_3f_GET
#define P1588_PIM_HEARTBEAT_3r_PIM_HEARTBEAT_3f_SET PLP_P1588_PIM_HEARTBEAT_3r_PIM_HEARTBEAT_3f_SET
#define READ_P1588_PIM_HEARTBEAT_3r PLP_READ_P1588_PIM_HEARTBEAT_3r
#define WRITE_P1588_PIM_HEARTBEAT_3r PLP_WRITE_P1588_PIM_HEARTBEAT_3r
#define MODIFY_P1588_PIM_HEARTBEAT_3r PLP_MODIFY_P1588_PIM_HEARTBEAT_3r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_HEARTBEAT_3r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_HEARTBEAT_4
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70f8
 * DESC:     pim heartbeat register(4)
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_HEARTBEAT_4  Output of the snapshot of the time stampPIM_HEARTBEAT = {PIM_HEARTBEAT_5, PIM_HEARTBEAT_4, PIM_HEARTBEAT_3, PIM_HEARTBEAT_2, PIM_HEARTBEAT_1, PIM_HEARTBEAT_0}Reset value is 0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_HEARTBEAT_4r    0x000000f8

#define PLP_P1588_PIM_HEARTBEAT_4r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_HEARTBEAT_4.
 *
 */
typedef union PLP_P1588_PIM_HEARTBEAT_4r_s {
	uint32_t v[1];
	uint32_t p1588_pim_heartbeat_4[1];
	uint32_t _p1588_pim_heartbeat_4;
} PLP_P1588_PIM_HEARTBEAT_4r_t;

#define PLP_P1588_PIM_HEARTBEAT_4r_CLR(r) (r).p1588_pim_heartbeat_4[0] = 0
#define PLP_P1588_PIM_HEARTBEAT_4r_SET(r,d) (r).p1588_pim_heartbeat_4[0] = d
#define PLP_P1588_PIM_HEARTBEAT_4r_GET(r) (r).p1588_pim_heartbeat_4[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_HEARTBEAT_4r_PIM_HEARTBEAT_4f_GET(r) (((r).p1588_pim_heartbeat_4[0]) & 0xffff)
#define PLP_P1588_PIM_HEARTBEAT_4r_PIM_HEARTBEAT_4f_SET(r,f) (r).p1588_pim_heartbeat_4[0]=(((r).p1588_pim_heartbeat_4[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PIM_HEARTBEAT_4.
 *
 */
#define PLP_READ_P1588_PIM_HEARTBEAT_4r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_HEARTBEAT_4r,(_r._p1588_pim_heartbeat_4))
#define PLP_WRITE_P1588_PIM_HEARTBEAT_4r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_HEARTBEAT_4r,(_r._p1588_pim_heartbeat_4))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_HEARTBEAT_4r PLP_P1588_PIM_HEARTBEAT_4r
#define P1588_PIM_HEARTBEAT_4r_SIZE PLP_P1588_PIM_HEARTBEAT_4r_SIZE
typedef PLP_P1588_PIM_HEARTBEAT_4r_t P1588_PIM_HEARTBEAT_4r_t;
#define P1588_PIM_HEARTBEAT_4r_CLR PLP_P1588_PIM_HEARTBEAT_4r_CLR
#define P1588_PIM_HEARTBEAT_4r_SET PLP_P1588_PIM_HEARTBEAT_4r_SET
#define P1588_PIM_HEARTBEAT_4r_GET PLP_P1588_PIM_HEARTBEAT_4r_GET
#define P1588_PIM_HEARTBEAT_4r_PIM_HEARTBEAT_4f_GET PLP_P1588_PIM_HEARTBEAT_4r_PIM_HEARTBEAT_4f_GET
#define P1588_PIM_HEARTBEAT_4r_PIM_HEARTBEAT_4f_SET PLP_P1588_PIM_HEARTBEAT_4r_PIM_HEARTBEAT_4f_SET
#define READ_P1588_PIM_HEARTBEAT_4r PLP_READ_P1588_PIM_HEARTBEAT_4r
#define WRITE_P1588_PIM_HEARTBEAT_4r PLP_WRITE_P1588_PIM_HEARTBEAT_4r
#define MODIFY_P1588_PIM_HEARTBEAT_4r PLP_MODIFY_P1588_PIM_HEARTBEAT_4r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_HEARTBEAT_4r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_HEARTBEAT_5
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70f9
 * DESC:     pim heartbeat register(5)
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_HEARTBEAT_5  Output of the snapshot of the time stampPIM_HEARTBEAT = {PIM_HEARTBEAT_5, PIM_HEARTBEAT_4, PIM_HEARTBEAT_3, PIM_HEARTBEAT_2, PIM_HEARTBEAT_1, PIM_HEARTBEAT_0}Reset value is 0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_HEARTBEAT_5r    0x000000f9

#define PLP_P1588_PIM_HEARTBEAT_5r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_HEARTBEAT_5.
 *
 */
typedef union PLP_P1588_PIM_HEARTBEAT_5r_s {
	uint32_t v[1];
	uint32_t p1588_pim_heartbeat_5[1];
	uint32_t _p1588_pim_heartbeat_5;
} PLP_P1588_PIM_HEARTBEAT_5r_t;

#define PLP_P1588_PIM_HEARTBEAT_5r_CLR(r) (r).p1588_pim_heartbeat_5[0] = 0
#define PLP_P1588_PIM_HEARTBEAT_5r_SET(r,d) (r).p1588_pim_heartbeat_5[0] = d
#define PLP_P1588_PIM_HEARTBEAT_5r_GET(r) (r).p1588_pim_heartbeat_5[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_HEARTBEAT_5r_PIM_HEARTBEAT_5f_GET(r) (((r).p1588_pim_heartbeat_5[0]) & 0xffff)
#define PLP_P1588_PIM_HEARTBEAT_5r_PIM_HEARTBEAT_5f_SET(r,f) (r).p1588_pim_heartbeat_5[0]=(((r).p1588_pim_heartbeat_5[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PIM_HEARTBEAT_5.
 *
 */
#define PLP_READ_P1588_PIM_HEARTBEAT_5r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_HEARTBEAT_5r,(_r._p1588_pim_heartbeat_5))
#define PLP_WRITE_P1588_PIM_HEARTBEAT_5r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_HEARTBEAT_5r,(_r._p1588_pim_heartbeat_5))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_HEARTBEAT_5r PLP_P1588_PIM_HEARTBEAT_5r
#define P1588_PIM_HEARTBEAT_5r_SIZE PLP_P1588_PIM_HEARTBEAT_5r_SIZE
typedef PLP_P1588_PIM_HEARTBEAT_5r_t P1588_PIM_HEARTBEAT_5r_t;
#define P1588_PIM_HEARTBEAT_5r_CLR PLP_P1588_PIM_HEARTBEAT_5r_CLR
#define P1588_PIM_HEARTBEAT_5r_SET PLP_P1588_PIM_HEARTBEAT_5r_SET
#define P1588_PIM_HEARTBEAT_5r_GET PLP_P1588_PIM_HEARTBEAT_5r_GET
#define P1588_PIM_HEARTBEAT_5r_PIM_HEARTBEAT_5f_GET PLP_P1588_PIM_HEARTBEAT_5r_PIM_HEARTBEAT_5f_GET
#define P1588_PIM_HEARTBEAT_5r_PIM_HEARTBEAT_5f_SET PLP_P1588_PIM_HEARTBEAT_5r_PIM_HEARTBEAT_5f_SET
#define READ_P1588_PIM_HEARTBEAT_5r PLP_READ_P1588_PIM_HEARTBEAT_5r
#define WRITE_P1588_PIM_HEARTBEAT_5r PLP_WRITE_P1588_PIM_HEARTBEAT_5r
#define MODIFY_P1588_PIM_HEARTBEAT_5r PLP_MODIFY_P1588_PIM_HEARTBEAT_5r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_HEARTBEAT_5r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_PIM_CRC_CNTL_P0
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70fb
 * DESC:     crc tag port 0 control register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PIM_CRC_CNTL_P0  bit[15:13] Reservedbit[12:11] Select the ingress time stamp range2'b00: ts[27:0]2'b01: ts[28:1]2'b10: ts[29:2]2'b11: ts[30:3]bit[10:9]  BRCM ingress CRC TAG IDbit[8]     Enable ingress CRC TAG modebit[7:5]   Reservedbit[4]	 1 = allow capture on all packetsbit[3]	 1 = check egress BRCM IDbit[2:1]   BRCM egress CRC TAG IDbit[0]     Enable egress CRC TAG modeReset value is 0x0.
 *
 ******************************************************************************/
#define PLP_P1588_PIM_CRC_CNTL_P0r    0x000000fb

#define PLP_P1588_PIM_CRC_CNTL_P0r_SIZE 4

/*
 * This structure should be used to declare and program P1588_PIM_CRC_CNTL_P0.
 *
 */
typedef union PLP_P1588_PIM_CRC_CNTL_P0r_s {
	uint32_t v[1];
	uint32_t p1588_pim_crc_cntl_p0[1];
	uint32_t _p1588_pim_crc_cntl_p0;
} PLP_P1588_PIM_CRC_CNTL_P0r_t;

#define PLP_P1588_PIM_CRC_CNTL_P0r_CLR(r) (r).p1588_pim_crc_cntl_p0[0] = 0
#define PLP_P1588_PIM_CRC_CNTL_P0r_SET(r,d) (r).p1588_pim_crc_cntl_p0[0] = d
#define PLP_P1588_PIM_CRC_CNTL_P0r_GET(r) (r).p1588_pim_crc_cntl_p0[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_PIM_CRC_CNTL_P0r_PIM_CRC_CNTL_P0f_GET(r) (((r).p1588_pim_crc_cntl_p0[0]) & 0xffff)
#define PLP_P1588_PIM_CRC_CNTL_P0r_PIM_CRC_CNTL_P0f_SET(r,f) (r).p1588_pim_crc_cntl_p0[0]=(((r).p1588_pim_crc_cntl_p0[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_PIM_CRC_CNTL_P0.
 *
 */
#define PLP_READ_P1588_PIM_CRC_CNTL_P0r(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_PIM_CRC_CNTL_P0r,(_r._p1588_pim_crc_cntl_p0))
#define PLP_WRITE_P1588_PIM_CRC_CNTL_P0r(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_PIM_CRC_CNTL_P0r,(_r._p1588_pim_crc_cntl_p0))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_PIM_CRC_CNTL_P0r PLP_P1588_PIM_CRC_CNTL_P0r
#define P1588_PIM_CRC_CNTL_P0r_SIZE PLP_P1588_PIM_CRC_CNTL_P0r_SIZE
typedef PLP_P1588_PIM_CRC_CNTL_P0r_t P1588_PIM_CRC_CNTL_P0r_t;
#define P1588_PIM_CRC_CNTL_P0r_CLR PLP_P1588_PIM_CRC_CNTL_P0r_CLR
#define P1588_PIM_CRC_CNTL_P0r_SET PLP_P1588_PIM_CRC_CNTL_P0r_SET
#define P1588_PIM_CRC_CNTL_P0r_GET PLP_P1588_PIM_CRC_CNTL_P0r_GET
#define P1588_PIM_CRC_CNTL_P0r_PIM_CRC_CNTL_P0f_GET PLP_P1588_PIM_CRC_CNTL_P0r_PIM_CRC_CNTL_P0f_GET
#define P1588_PIM_CRC_CNTL_P0r_PIM_CRC_CNTL_P0f_SET PLP_P1588_PIM_CRC_CNTL_P0r_PIM_CRC_CNTL_P0f_SET
#define READ_P1588_PIM_CRC_CNTL_P0r PLP_READ_P1588_PIM_CRC_CNTL_P0r
#define WRITE_P1588_PIM_CRC_CNTL_P0r PLP_WRITE_P1588_PIM_CRC_CNTL_P0r
#define MODIFY_P1588_PIM_CRC_CNTL_P0r PLP_MODIFY_P1588_PIM_CRC_CNTL_P0r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_PIM_CRC_CNTL_P0r'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TX_Y1731_OPCODE45_OFFSET
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70fc
 * DESC:     P1588 TX Y1731 Opcode45 Offset Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_Y1731_OPCODE45_OFFSET Only bits [5:0] are used. Bits [5:0] indicate the byte number of the timestamp after the control word ending in flags+TLV offset.The numbering starts from 0.
 *
 ******************************************************************************/
#define PLP_P1588_TX_Y1731_OPCODE45_OFFSETr    0x000000fc

#define PLP_P1588_TX_Y1731_OPCODE45_OFFSETr_SIZE 4

/*
 * This structure should be used to declare and program P1588_TX_Y1731_OPCODE45_OFFSET.
 *
 */
typedef union PLP_P1588_TX_Y1731_OPCODE45_OFFSETr_s {
	uint32_t v[1];
	uint32_t p1588_tx_y1731_opcode45_offset[1];
	uint32_t _p1588_tx_y1731_opcode45_offset;
} PLP_P1588_TX_Y1731_OPCODE45_OFFSETr_t;

#define PLP_P1588_TX_Y1731_OPCODE45_OFFSETr_CLR(r) (r).p1588_tx_y1731_opcode45_offset[0] = 0
#define PLP_P1588_TX_Y1731_OPCODE45_OFFSETr_SET(r,d) (r).p1588_tx_y1731_opcode45_offset[0] = d
#define PLP_P1588_TX_Y1731_OPCODE45_OFFSETr_GET(r) (r).p1588_tx_y1731_opcode45_offset[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TX_Y1731_OPCODE45_OFFSETr_TX_Y1731_OPCODE45_OFFSETf_GET(r) (((r).p1588_tx_y1731_opcode45_offset[0]) & 0xffff)
#define PLP_P1588_TX_Y1731_OPCODE45_OFFSETr_TX_Y1731_OPCODE45_OFFSETf_SET(r,f) (r).p1588_tx_y1731_opcode45_offset[0]=(((r).p1588_tx_y1731_opcode45_offset[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TX_Y1731_OPCODE45_OFFSET.
 *
 */
#define PLP_READ_P1588_TX_Y1731_OPCODE45_OFFSETr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TX_Y1731_OPCODE45_OFFSETr,(_r._p1588_tx_y1731_opcode45_offset))
#define PLP_WRITE_P1588_TX_Y1731_OPCODE45_OFFSETr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TX_Y1731_OPCODE45_OFFSETr,(_r._p1588_tx_y1731_opcode45_offset))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TX_Y1731_OPCODE45_OFFSETr PLP_P1588_TX_Y1731_OPCODE45_OFFSETr
#define P1588_TX_Y1731_OPCODE45_OFFSETr_SIZE PLP_P1588_TX_Y1731_OPCODE45_OFFSETr_SIZE
typedef PLP_P1588_TX_Y1731_OPCODE45_OFFSETr_t P1588_TX_Y1731_OPCODE45_OFFSETr_t;
#define P1588_TX_Y1731_OPCODE45_OFFSETr_CLR PLP_P1588_TX_Y1731_OPCODE45_OFFSETr_CLR
#define P1588_TX_Y1731_OPCODE45_OFFSETr_SET PLP_P1588_TX_Y1731_OPCODE45_OFFSETr_SET
#define P1588_TX_Y1731_OPCODE45_OFFSETr_GET PLP_P1588_TX_Y1731_OPCODE45_OFFSETr_GET
#define P1588_TX_Y1731_OPCODE45_OFFSETr_TX_Y1731_OPCODE45_OFFSETf_GET PLP_P1588_TX_Y1731_OPCODE45_OFFSETr_TX_Y1731_OPCODE45_OFFSETf_GET
#define P1588_TX_Y1731_OPCODE45_OFFSETr_TX_Y1731_OPCODE45_OFFSETf_SET PLP_P1588_TX_Y1731_OPCODE45_OFFSETr_TX_Y1731_OPCODE45_OFFSETf_SET
#define READ_P1588_TX_Y1731_OPCODE45_OFFSETr PLP_READ_P1588_TX_Y1731_OPCODE45_OFFSETr
#define WRITE_P1588_TX_Y1731_OPCODE45_OFFSETr PLP_WRITE_P1588_TX_Y1731_OPCODE45_OFFSETr
#define MODIFY_P1588_TX_Y1731_OPCODE45_OFFSETr PLP_MODIFY_P1588_TX_Y1731_OPCODE45_OFFSETr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TX_Y1731_OPCODE45_OFFSETr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TX_Y1731_OPCODE46_OFFSET
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70fd
 * DESC:     P1588 TX Y1731 Opcode46 Offset Register
 * RESETVAL: 0x10 (16)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_Y1731_OPCODE46_OFFSET Only bits [5:0] are used. Bits [5:0] indicate the byte number of the timestamp after the control word ending in flags+TLV offset.The numbering starts from 0.
 *
 ******************************************************************************/
#define PLP_P1588_TX_Y1731_OPCODE46_OFFSETr    0x000000fd

#define PLP_P1588_TX_Y1731_OPCODE46_OFFSETr_SIZE 4

/*
 * This structure should be used to declare and program P1588_TX_Y1731_OPCODE46_OFFSET.
 *
 */
typedef union PLP_P1588_TX_Y1731_OPCODE46_OFFSETr_s {
	uint32_t v[1];
	uint32_t p1588_tx_y1731_opcode46_offset[1];
	uint32_t _p1588_tx_y1731_opcode46_offset;
} PLP_P1588_TX_Y1731_OPCODE46_OFFSETr_t;

#define PLP_P1588_TX_Y1731_OPCODE46_OFFSETr_CLR(r) (r).p1588_tx_y1731_opcode46_offset[0] = 0
#define PLP_P1588_TX_Y1731_OPCODE46_OFFSETr_SET(r,d) (r).p1588_tx_y1731_opcode46_offset[0] = d
#define PLP_P1588_TX_Y1731_OPCODE46_OFFSETr_GET(r) (r).p1588_tx_y1731_opcode46_offset[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TX_Y1731_OPCODE46_OFFSETr_TX_Y1731_OPCODE46_OFFSETf_GET(r) (((r).p1588_tx_y1731_opcode46_offset[0]) & 0xffff)
#define PLP_P1588_TX_Y1731_OPCODE46_OFFSETr_TX_Y1731_OPCODE46_OFFSETf_SET(r,f) (r).p1588_tx_y1731_opcode46_offset[0]=(((r).p1588_tx_y1731_opcode46_offset[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TX_Y1731_OPCODE46_OFFSET.
 *
 */
#define PLP_READ_P1588_TX_Y1731_OPCODE46_OFFSETr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TX_Y1731_OPCODE46_OFFSETr,(_r._p1588_tx_y1731_opcode46_offset))
#define PLP_WRITE_P1588_TX_Y1731_OPCODE46_OFFSETr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TX_Y1731_OPCODE46_OFFSETr,(_r._p1588_tx_y1731_opcode46_offset))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TX_Y1731_OPCODE46_OFFSETr PLP_P1588_TX_Y1731_OPCODE46_OFFSETr
#define P1588_TX_Y1731_OPCODE46_OFFSETr_SIZE PLP_P1588_TX_Y1731_OPCODE46_OFFSETr_SIZE
typedef PLP_P1588_TX_Y1731_OPCODE46_OFFSETr_t P1588_TX_Y1731_OPCODE46_OFFSETr_t;
#define P1588_TX_Y1731_OPCODE46_OFFSETr_CLR PLP_P1588_TX_Y1731_OPCODE46_OFFSETr_CLR
#define P1588_TX_Y1731_OPCODE46_OFFSETr_SET PLP_P1588_TX_Y1731_OPCODE46_OFFSETr_SET
#define P1588_TX_Y1731_OPCODE46_OFFSETr_GET PLP_P1588_TX_Y1731_OPCODE46_OFFSETr_GET
#define P1588_TX_Y1731_OPCODE46_OFFSETr_TX_Y1731_OPCODE46_OFFSETf_GET PLP_P1588_TX_Y1731_OPCODE46_OFFSETr_TX_Y1731_OPCODE46_OFFSETf_GET
#define P1588_TX_Y1731_OPCODE46_OFFSETr_TX_Y1731_OPCODE46_OFFSETf_SET PLP_P1588_TX_Y1731_OPCODE46_OFFSETr_TX_Y1731_OPCODE46_OFFSETf_SET
#define READ_P1588_TX_Y1731_OPCODE46_OFFSETr PLP_READ_P1588_TX_Y1731_OPCODE46_OFFSETr
#define WRITE_P1588_TX_Y1731_OPCODE46_OFFSETr PLP_WRITE_P1588_TX_Y1731_OPCODE46_OFFSETr
#define MODIFY_P1588_TX_Y1731_OPCODE46_OFFSETr PLP_MODIFY_P1588_TX_Y1731_OPCODE46_OFFSETr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TX_Y1731_OPCODE46_OFFSETr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_TX_Y1731_OPCODE47_OFFSET
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70fe
 * DESC:     P1588 TX Y1731 Opcode47 Offset Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     TX_Y1731_OPCODE47_OFFSET Only bits [5:0] are used. Bits [5:0] indicate the byte number of the timestamp after the control word ending in flags+TLV offset.The numbering starts from 0.
 *
 ******************************************************************************/
#define PLP_P1588_TX_Y1731_OPCODE47_OFFSETr    0x000000fe

#define PLP_P1588_TX_Y1731_OPCODE47_OFFSETr_SIZE 4

/*
 * This structure should be used to declare and program P1588_TX_Y1731_OPCODE47_OFFSET.
 *
 */
typedef union PLP_P1588_TX_Y1731_OPCODE47_OFFSETr_s {
	uint32_t v[1];
	uint32_t p1588_tx_y1731_opcode47_offset[1];
	uint32_t _p1588_tx_y1731_opcode47_offset;
} PLP_P1588_TX_Y1731_OPCODE47_OFFSETr_t;

#define PLP_P1588_TX_Y1731_OPCODE47_OFFSETr_CLR(r) (r).p1588_tx_y1731_opcode47_offset[0] = 0
#define PLP_P1588_TX_Y1731_OPCODE47_OFFSETr_SET(r,d) (r).p1588_tx_y1731_opcode47_offset[0] = d
#define PLP_P1588_TX_Y1731_OPCODE47_OFFSETr_GET(r) (r).p1588_tx_y1731_opcode47_offset[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_TX_Y1731_OPCODE47_OFFSETr_TX_Y1731_OPCODE47_OFFSETf_GET(r) (((r).p1588_tx_y1731_opcode47_offset[0]) & 0xffff)
#define PLP_P1588_TX_Y1731_OPCODE47_OFFSETr_TX_Y1731_OPCODE47_OFFSETf_SET(r,f) (r).p1588_tx_y1731_opcode47_offset[0]=(((r).p1588_tx_y1731_opcode47_offset[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_TX_Y1731_OPCODE47_OFFSET.
 *
 */
#define PLP_READ_P1588_TX_Y1731_OPCODE47_OFFSETr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_TX_Y1731_OPCODE47_OFFSETr,(_r._p1588_tx_y1731_opcode47_offset))
#define PLP_WRITE_P1588_TX_Y1731_OPCODE47_OFFSETr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_TX_Y1731_OPCODE47_OFFSETr,(_r._p1588_tx_y1731_opcode47_offset))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_TX_Y1731_OPCODE47_OFFSETr PLP_P1588_TX_Y1731_OPCODE47_OFFSETr
#define P1588_TX_Y1731_OPCODE47_OFFSETr_SIZE PLP_P1588_TX_Y1731_OPCODE47_OFFSETr_SIZE
typedef PLP_P1588_TX_Y1731_OPCODE47_OFFSETr_t P1588_TX_Y1731_OPCODE47_OFFSETr_t;
#define P1588_TX_Y1731_OPCODE47_OFFSETr_CLR PLP_P1588_TX_Y1731_OPCODE47_OFFSETr_CLR
#define P1588_TX_Y1731_OPCODE47_OFFSETr_SET PLP_P1588_TX_Y1731_OPCODE47_OFFSETr_SET
#define P1588_TX_Y1731_OPCODE47_OFFSETr_GET PLP_P1588_TX_Y1731_OPCODE47_OFFSETr_GET
#define P1588_TX_Y1731_OPCODE47_OFFSETr_TX_Y1731_OPCODE47_OFFSETf_GET PLP_P1588_TX_Y1731_OPCODE47_OFFSETr_TX_Y1731_OPCODE47_OFFSETf_GET
#define P1588_TX_Y1731_OPCODE47_OFFSETr_TX_Y1731_OPCODE47_OFFSETf_SET PLP_P1588_TX_Y1731_OPCODE47_OFFSETr_TX_Y1731_OPCODE47_OFFSETf_SET
#define READ_P1588_TX_Y1731_OPCODE47_OFFSETr PLP_READ_P1588_TX_Y1731_OPCODE47_OFFSETr
#define WRITE_P1588_TX_Y1731_OPCODE47_OFFSETr PLP_WRITE_P1588_TX_Y1731_OPCODE47_OFFSETr
#define MODIFY_P1588_TX_Y1731_OPCODE47_OFFSETr PLP_MODIFY_P1588_TX_Y1731_OPCODE47_OFFSETr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_TX_Y1731_OPCODE47_OFFSETr'
 ******************************************************************************/




/*******************************************************************************
 *
 * REGISTER:  P1588_INBAND_SOFT_COMMAND
 * BLOCKS:   P1588_REGS
 * REGADDR:  0x70ff
 * DESC:     inband soft command control register
 * RESETVAL: 0x4844 (18500)
 * ACCESS:   R/W
 * FIELDS:
 *     INBAND_SOFT_COMMAND used for inband soft commandnote: do not program this register.
 *
 ******************************************************************************/
#define PLP_P1588_INBAND_SOFT_COMMANDr    0x000000ff

#define PLP_P1588_INBAND_SOFT_COMMANDr_SIZE 4

/*
 * This structure should be used to declare and program P1588_INBAND_SOFT_COMMAND.
 *
 */
typedef union PLP_P1588_INBAND_SOFT_COMMANDr_s {
	uint32_t v[1];
	uint32_t p1588_inband_soft_command[1];
	uint32_t _p1588_inband_soft_command;
} PLP_P1588_INBAND_SOFT_COMMANDr_t;

#define PLP_P1588_INBAND_SOFT_COMMANDr_CLR(r) (r).p1588_inband_soft_command[0] = 0
#define PLP_P1588_INBAND_SOFT_COMMANDr_SET(r,d) (r).p1588_inband_soft_command[0] = d
#define PLP_P1588_INBAND_SOFT_COMMANDr_GET(r) (r).p1588_inband_soft_command[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define PLP_P1588_INBAND_SOFT_COMMANDr_INBAND_SOFT_COMMANDf_GET(r) (((r).p1588_inband_soft_command[0]) & 0xffff)
#define PLP_P1588_INBAND_SOFT_COMMANDr_INBAND_SOFT_COMMANDf_SET(r,f) (r).p1588_inband_soft_command[0]=(((r).p1588_inband_soft_command[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access P1588_INBAND_SOFT_COMMAND.
 *
 */
#define PLP_READ_P1588_INBAND_SOFT_COMMANDr(_acc,_r) plp_aperta_p1588_reg_read(_acc,PLP_P1588_INBAND_SOFT_COMMANDr,(_r._p1588_inband_soft_command))
#define PLP_WRITE_P1588_INBAND_SOFT_COMMANDr(_acc,_r) plp_aperta_p1588_reg_write(_acc,PLP_P1588_INBAND_SOFT_COMMANDr,(_r._p1588_inband_soft_command))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define P1588_INBAND_SOFT_COMMANDr PLP_P1588_INBAND_SOFT_COMMANDr
#define P1588_INBAND_SOFT_COMMANDr_SIZE PLP_P1588_INBAND_SOFT_COMMANDr_SIZE
typedef PLP_P1588_INBAND_SOFT_COMMANDr_t P1588_INBAND_SOFT_COMMANDr_t;
#define P1588_INBAND_SOFT_COMMANDr_CLR PLP_P1588_INBAND_SOFT_COMMANDr_CLR
#define P1588_INBAND_SOFT_COMMANDr_SET PLP_P1588_INBAND_SOFT_COMMANDr_SET
#define P1588_INBAND_SOFT_COMMANDr_GET PLP_P1588_INBAND_SOFT_COMMANDr_GET
#define P1588_INBAND_SOFT_COMMANDr_INBAND_SOFT_COMMANDf_GET PLP_P1588_INBAND_SOFT_COMMANDr_INBAND_SOFT_COMMANDf_GET
#define P1588_INBAND_SOFT_COMMANDr_INBAND_SOFT_COMMANDf_SET PLP_P1588_INBAND_SOFT_COMMANDr_INBAND_SOFT_COMMANDf_SET
#define READ_P1588_INBAND_SOFT_COMMANDr PLP_READ_P1588_INBAND_SOFT_COMMANDr
#define WRITE_P1588_INBAND_SOFT_COMMANDr PLP_WRITE_P1588_INBAND_SOFT_COMMANDr
#define MODIFY_P1588_INBAND_SOFT_COMMANDr PLP_MODIFY_P1588_INBAND_SOFT_COMMANDr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'PLP_P1588_INBAND_SOFT_COMMANDr'
 ******************************************************************************/


/******************************************************************************\
|* PTP Packet Lookup Action Registers                                          |
\******************************************************************************/

/************************************************************************************\
 * REGISTER: P1588_PREDEFINE_ACTION_SELECT registers
 * BLOCKS:   P1588_REGS
 * REGADDR:  RX: 0x00B9 ,  TX: 0x00B7   (P1588 direct address)
 * DESC:     Rx/Tx PTP packet Lookup Pre-defined Action select
 * ACCESS:   R/W
 *
 * P1588_PREDEFINE_ACTION_SELECT_0_RX/_TX   ( P1588 address: 0x00B9 / 0x00B7 )
 *      bit[9:7] -  3'bX00 = GM/SC E2E and P2P
 *                  3'b001 = Delay request-response mode (regular TC)
 *                  3'b011 = Delay request-response mode (partial TC)
 *                  3'b1X1 = 802.1AS mode TC
 *
\************************************************************************************/
#define P1588_PREDEFINE_ACTION_TX_SELECT        PLP_P1588_INBAND_TX_CONTROL1r
#define P1588_PREDEFINE_ACTION_RX_SELECT        PLP_P1588_INBAND_RX_CONTROL1r
#define P1588_PREDEFINE_ACTION_SELECT           P1588_PREDEFINE_ACTION_TX_SELECT

/* declare and program the indirect register P1588_PREDEFINE_ACTION_SELECT */
typedef union P1588_PREDEFINE_ACTION_SELECTr_s {
	uint32_t  v32[1];
	uint32_t  v;
} P1588_PREDEFINE_ACTION_SELECTr_t;

#define P1588_PREDEFINE_ACTION_SELECTr_CLR(r)         (r).v = 0
#define P1588_PREDEFINE_ACTION_SELECTr_SET(r,d)       (r).v = d
#define P1588_PREDEFINE_ACTION_SELECTr_GET(r)         (r).v

/* macros to access individual fields in P1588_PREDEFINE_ACTION_SELECT_RX/_TX registers  bit[10:7,4] */
#define P1588_PREDEFINE_ACTION_SEL_MODEf_GET(r)    ((((r).v) >> 7) & 0xfU)
#define P1588_PREDEFINE_ACTION_SEL_MODEf_SET(r,n)  (r).v=(((r).v & ~((uint32_t)0xfU << 7)) | ((((uint32_t)n) & 0xfU) << 7))
#define P1588_PREDEFINE_ACTION_PTPv2p1f_SET(r,n)  (r).v=(((r).v & ~((uint32_t)0x1U << 4)) | ((((uint32_t)n) & 0x1U) << 4))

/* macros to read/write P1588_PREDEFINE_ACTION_SELECT registers */
#define  READ_P1588_PREDEFINE_ACTION_SELECTr(_acc,_ad,_r)  plp_aperta_p1588_reg_read( _acc, (_ad), (_r.v))
#define WRITE_P1588_PREDEFINE_ACTION_SELECTr(_acc,_ad,_r)  plp_aperta_p1588_reg_write(_acc, (_ad), (_r.v))


/******************************************************************************\
|* Indirect Registers                                                          |
\******************************************************************************/

/************************************************************************************\
 * REGISTER: P1588_USER_ACTION_SELECT registers
 * BLOCKS:   P1588_REGS Indirect
 * REGADDR:  RX: 0x0017 - 0x0019 ,  TX: 0x001A - 0x001C  (indirect address)
 * DESC:     Rx/Tx PTP packet Lookup User-defined Action select
 * ACCESS:   R/W
 *
 * P1588_USER_ACTION_SELECT_0_RX/_TX   ( indirect address: 0x0017 / 0x001A )
 *      bit[03:00] - user defined action for PTP 1-step Sync messages
 *      bit[07:04] - user defined action for PTP 2-step Sync messages
 *      bit[11:08] - user defined action for PTP Delay Request messages
 *      bit[15:12] - user defined action for PTP Follow-Up Sync messages
 *
 * P1588_USER_ACTION_SELECT_1_RX/_TX   ( indirect address: 0x0018 / 0x001B )
 *      bit[03:00] - user defined action for PTP PDelay Request messages
 *      bit[07:04] - user defined action for PTP 1-step PDelay Response messages
 *      bit[11:08] - user defined action for PTP 2-step PDelay Response messages
 *      bit[15:12] - user defined action for PTP Follow-Up PDelay Response messages
 *
 * P1588_USER_ACTION_SELECT_2_RX/_TX   ( indirect address: 0x0019 / 0x001C )
 *      bit[03:00] - user defined action for PTP PDelay Response messages
 *      bit[4]     - 1: user defined mode ,  0: pre-defined action mode
 *
\************************************************************************************/
#define P1588_USER_ACTION_SELECT_0_RXr     0x0017
#define P1588_USER_ACTION_SELECT_1_RXr     0x0018
#define P1588_USER_ACTION_SELECT_2_RXr     0x0019
#define P1588_USER_ACTION_SELECT_0_TXr     0x001A
#define P1588_USER_ACTION_SELECT_1_TXr     0x001B
#define P1588_USER_ACTION_SELECT_2_TXr     0x001C
#define P1588_USER_ACTION_SELECTr          P1588_USER_ACTION_SELECT_0_RXr

/* declare and program the indirect register P1588_USER_ACTION_SELECT */
typedef union P1588_USER_ACTION_SELECTr_s {
	uint32_t  v32[1];
	uint32_t  v;
} P1588_USER_ACTION_SELECTr_t;

#define P1588_USER_ACTION_SELECTr_CLR(r) \
        do {       \
            int i;  for(i=0; i<NUM_USER_ACTION_SEL_REG; i++) (r)[i].v = 0U; \
        } while (0)

#define P1588_USER_ACTION_SELECTr_SET(r,d)       (r).v = d
#define P1588_USER_ACTION_SELECTr_GET(r)         (r).v

/* macros to access individual fields in P1588_USER_ACTION_SELECT_0_RX/_TX registers */
#define P1588_USER_ACTION_SEL_SYNC_1STEPf_GET(r)   ((((r).v) >> 0) & 0xfU)
#define P1588_USER_ACTION_SEL_SYNC_1STEPf_SET(r,n) (r).v=(((r).v & ~((uint32_t)0xfU << 0)) | ((((uint32_t)n) & 0xfU) << 0))
#define P1588_USER_ACTION_SEL_SYNC_2STEPf_GET(r)   ((((r).v) >> 4) & 0xfU)
#define P1588_USER_ACTION_SEL_SYNC_2STEPf_SET(r,n) (r).v=(((r).v & ~((uint32_t)0xfU << 4)) | ((((uint32_t)n) & 0xfU) << 4))
#define P1588_USER_ACTION_SEL_DELAY_REQf_GET( r)   ((((r).v) >> 8) & 0xfU)
#define P1588_USER_ACTION_SEL_DELAY_REQf_SET( r,n) (r).v=(((r).v & ~((uint32_t)0xfU << 8)) | ((((uint32_t)n) & 0xfU) << 8))
#define P1588_USER_ACTION_SEL_SYNC_FLUPf_GET( r)   ((((r).v) >>12) & 0xfU)
#define P1588_USER_ACTION_SEL_SYNC_FLUPf_SET( r,n) (r).v=(((r).v & ~((uint32_t)0xfU <<12)) | ((((uint32_t)n) & 0xfU) <<12))

/* macros to access individual fields in P1588_USER_ACTION_SELECT_1_RX/_TX registers */
#define P1588_USER_ACTION_SEL_PDLY_REQf_GET(r)          ((((r).v) >> 0) & 0xfU)
#define P1588_USER_ACTION_SEL_PDLY_REQf_SET(r,n)        (r).v=(((r).v & ~((uint32_t)0xfU << 0)) | ((((uint32_t)n) & 0xfU) << 0))
#define P1588_USER_ACTION_SEL_PDLY_RSP_1STEPf_GET(r)    ((((r).v) >> 4) & 0xfU)
#define P1588_USER_ACTION_SEL_PDLY_RSP_1STEPf_SET(r,n)  (r).v=(((r).v & ~((uint32_t)0xfU << 4)) | ((((uint32_t)n) & 0xfU) << 4))
#define P1588_USER_ACTION_SEL_PDLY_RSP_2STEPf_GET( r)   ((((r).v) >> 8) & 0xfU)
#define P1588_USER_ACTION_SEL_PDLY_RSP_2STEPf_SET( r,n) (r).v=(((r).v & ~((uint32_t)0xfU << 8)) | ((((uint32_t)n) & 0xfU) << 8))
#define P1588_USER_ACTION_SEL_PDLY_RSP_FLUPf_GET( r)    ((((r).v) >>12) & 0xfU)
#define P1588_USER_ACTION_SEL_PDLY_RSP_FLUPf_SET( r,n)  (r).v=(((r).v & ~((uint32_t)0xfU <<12)) | ((((uint32_t)n) & 0xfU) <<12))

/* macros to access individual fields in P1588_USER_ACTION_SELECT_2_RX/_TX registers */
#define P1588_USER_ACTION_SEL_DELAY_RESPf_GET(r)    ((((r).v) >> 0) & 0xfU)
#define P1588_USER_ACTION_SEL_DELAY_RESPf_SET(r,n)  (r).v=(((r).v & ~((uint32_t)0xfU << 0)) | ((((uint32_t)n) & 0xfU) << 0))
#define P1588_USER_ACTION_SEL_USER_DEFINEf_GET(r)   ((((r).v) >> 4) & 0x1U)
#define P1588_USER_ACTION_SEL_USER_DEFINEf_SET(r,n) (r).v=(((r).v & ~((uint32_t)0xfU << 4)) | ((((uint32_t)n) & 0x1U) << 4))

/* macros to read/write P1588_USER_ACTION_SELECT registers */
#define  READ_P1588_USER_ACTION_SELECTr(_acc,_ad,_r)  p1588_indir_reg_read( _acc, (_ad), (_r.v))
#define WRITE_P1588_USER_ACTION_SELECTr(_acc,_ad,_r)  p1588_indir_reg_write(_acc, (_ad), (_r.v))


#endif /* __P1588_REG_DEFS__H__ */

