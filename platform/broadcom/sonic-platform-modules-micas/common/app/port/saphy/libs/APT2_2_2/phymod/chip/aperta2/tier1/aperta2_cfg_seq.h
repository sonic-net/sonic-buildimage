/*
 *
 * $Id: aperta2_pm_seq.h Exp $
 *
 *
 *
 *
 * $Copyright: (c) 2022 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 *
 *
 *
 *
 *
 */

#ifndef __APERTA2_CFG_SEQ_H__
#define __APERTA2_CFG_SEQ_H__

#include <phymod/phymod.h>
#define APERTA2_CHIP_85343              0x85343
#define APERTA2_CHIP_85344              0x85344
#define APERTA2_AGERA2_COMP_59611       0x59611

#define APERTA2_MICRO_RETRY_COUNT       200
#define APERTA2_HEADER_SIZE              64

#define APERTA2_IS_SYSTEM_SIDE(PHY)              (PHY->port_loc == phymodPortLocSys)
#define APERTA2_IS_LINE_SIDE(PHY)                (PHY->port_loc == phymodPortLocLine || PHY->port_loc == phymodPortLocDC)

/* The following defines are used to save interface type in SWGPREG */
/* Use 4 bits per lane in SWGPREG for saving SW interfaces*/
/* Totaly 8 register 4 for line and 4 for sys*/
#define APERTA2_IF_TYPE_SAVE_SWGPREG_BASE_ADDR        BCM_APERTA2_DIRECT_CTRL_SWGPREG01r
#define APERTA2_IF_TYPE_PER_LANE_STORAGE_MASK         0xF
#define APERTA2_LINE_SYS_IF_TYPE_SAVE_SWGPREG_OFFSET  4

/* Adding 2 more register to  interface type in SWGPREG(For Line and SYS - 16 lanes per side) */
/* Totaly 5 bits */
#define APERTA2_IF_TYPE_SAVE_SWGPREG_LINE_BIT4              BCM_APERTA2_DIRECT_CTRL_SWGPREG09r
#define APERTA2_IF_TYPE_SAVE_SWGPREG_SYS_BIT4               BCM_APERTA2_DIRECT_CTRL_SWGPREG0Ar

/* Initialization State*/
#define APERTA2_INIT_STATE_START                 1
#define APERTA2_INIT_STATE_COMPLETE              2


/*SW GPREG for storing number of lanes in systemside*/
#define APERTA2_SYSTEM_SIDE_NO_OF_LANES        BCM_APERTA2_DIRECT_CTRL_SWGPREG0Br

#define APERTA2_IF_TYPE_LIST_ELEMENTS         {                                       \
                                                    phymodInterfaceSR,                  \
                                                    phymodInterfaceKR,                  \
                                                    phymodInterfaceCR,                  \
                                                    phymodInterfaceXFI,                 \
                                                    phymodInterfaceSFI,                 \
                                                    phymodInterfaceXLAUI,               \
                                                    phymodInterfaceLR,                  \
                                                    phymodInterfaceER,                  \
                                                    phymodInterfaceVSR,                 \
                                                    phymodInterfaceCAUI4_C2C,           \
                                                    phymodInterfaceAUI_C2C,             \
                                                    phymodInterfaceAUI_C2M,             \
                                                    phymodInterfaceKR4,                 \
                                                    phymodInterfaceCR4,                 \
                                                    phymodInterfaceCAUI4_C2M,           \
                                                    phymodInterfaceXLPPI,               \
                                                    phymodInterfaceCEIMR,               \
                                                    phymodInterfaceCEILR                \
                                                }

/* Aperta2 interrupt Groups*/
/* PORT Group*/
#define APERTA2_INTR_PORT_PTP                       0x1              /*Port Ingress & Egress PTP time stamp interrupt*/
#define APERTA2_INTR_PORT_LINK_DOWN                 0x2              /*Port Link Down interrupt*/
#define APERTA2_INTR_PORT_SF_ERR_INTR               0x4              /*Port Egress Store and Forward(overflow/underflow) error interrupt*/
#define APERTA2_INTR_PORT_SF_DED_INTR               0x8              /*Port Egress Store and Forward(DED) interrupt*/
#define APERTA2_INTR_PORT_FC_ERR_INTR               0x10             /*Port Ingress & Egress Flow Control (overflow/underflow) error interrupt*/
#define APERTA2_INTR_PORT_FC_DED_INTR               0x20             /*Port Ingress & Egress Flow Control (DED) interrupt*/
#define APERTA2_INTR_PORT_INTF_ERR_INTR             0x40             /*Port Ingress & Egress Interface (overflow/underflow) error interrupt*/
#define APERTA2_INTR_PORT_INTF_DED_INTR             0x80             /*Port Ingress & Egress Interface (DED) interrupt*/

/* COMMON control1 Group*/
#define APERTA2_INTR_CMN_MOD_ABS_RISING             0x100            /*Module Absent Interrupt from GPIO Rising Edge*/
#define APERTA2_INTR_CMN_MOD_ABS_FALL               0x200            /*Module Absent Interrupt from GPIO Falling Edge*/
#define APERTA2_INTR_CMN_MOD_EXT_RAISING            0x400            /*Module External Rising Edge Interrupt */
#define APERTA2_INTR_CMN_MOD_EXT_FALL               0x800            /*Module External Falling Edge Interrupt */

/*Common control 0*/
#define APERTA2_INTR_CMN_LMI_ERR                    0x10000          /*LMI Error Interrupt Rising Edge*/
#define APERTA2_INTR_CMN_LMI_DED                    0x20000          /*LMI DED Interrupt */
#define APERTA2_INTR_CMN_PMIF_ERR                   0x40000          /*Port Macro Interface Error Interrupt */
#define APERTA2_INTR_CMN_PMIF_DED                   0x80000          /*Port Macro Interface DED Interrupt*/

/* COMMON DP Group*/
#define APERTA2_INTR_CMN_DP_CDC_FIFO_DED_INTR       0x100000         /*CDC FIFO DED Interrupt Rising */
/*Common control 0*/
#define APERTA2_INTR_CMN_PTP_PPS_RAISING            0x400000         /*PTP Pulse Per Second Rising Edge Interrupt*/
#define APERTA2_INTR_CMN_PTP_PPS_FALLING            0x800000         /*PTP Pulse Per Second Falling Edge Interrupt*/

/*M0 Group*/
#define APERTA2_INTR_M0_MST_FW_WDOG_EXP             0x1000000        /*Master Firmware Watchdog Timer Expired Interrupt*/
#define APERTA2_INTR_M0_MST_DRAM_DED                0x2000000        /*Master Data RAM DED Interrupt*/
#define APERTA2_INTR_M0_MST_DED                     0x4000000        /*Master Code RAM DED Interrupt*/

#define APERTA2_INTR_PORT                           0x00000FF        /* Mask for Port interrupt*/
#define APERTA2_INTR_COMMON                         0x0FFFF00        /* Mask for Common interrupt*/
#define APERTA2_INTR_COMMON_CTRL0                   0x0CF0000        /* Mask for Common control0 interrupt*/
#define APERTA2_INTR_COMMON_CTRL1                   0x0000F00        /* Mask for Common control1 interrupt*/
#define APERTA2_INTR_MO                             0x7000000        /* Mask for M0 interrupt*/

/*GPIO*/
#define APERTA2_GPIO_CTRL1_BASE_ADDR              BCM_APERTA2_DIRECT_PAD_CNTRL_GPIO_0_CONTROL_1r       /* GPIO Control 1 Base address*/
#define APERTA2_GPIO_CTRL0_BASE_ADDR              BCM_APERTA2_DIRECT_PAD_CNTRL_GPIO_0_CONTROL_0r       /* GPIO Control 0 Base address*/
#define APERTA2_GPIO_STS_BASE_ADDR                BCM_APERTA2_DIRECT_PAD_CNTRL_GPIO_0_STATUS_0r        /* GPIO Status Base address*/

/* Aperta2 Support 0-27 GPIO Pin*/
#define APERTA2_GPIO_MIN_PIN                      0                             
#define APERTA2_GPIO_MAX_PIN                      27

/* GPIO OEBF MASK*/
#define APERTA2_GPIO_CTRL_OEBF_MASK               1

/*Module controller specific defines */
#define APERTA2_MODULE_CNTRL_RAM_NVR0_ADR    0x8800                   /*NVRAM Base address*/

#define APERTA2_SRDS_DUMP_L1      0x3
#define APERTA2_SRDS_DUMP_L2      0x07
#define APERTA2_SRDS_DUMP_L3      0xFF 
#define APERTA2_SRDS_MIN_DUMP     0x2

typedef enum APERTA2_MSGOUT_E {
    APERTA2_MSGOUT_DONTCARE = 0x0000,
    APERTA2_MSGOUT_GET_CNT  = 0x8888,
    APERTA2_MSGOUT_GET_LSB  = 0xABCD,
    APERTA2_MSGOUT_GET_MSB  = 0x4321,
    APERTA2_MSGOUT_GET_2B   = 0xEEEE,
    APERTA2_MSGOUT_GET_B    = 0xF00D,
    APERTA2_MSGOUT_ERR      = 0x0BAD,
    APERTA2_MSGOUT_NEXT     = 0x2222, 
    APERTA2_MSGOUT_NOT_DWNLD     = 0x0101, 
    APERTA2_MSGOUT_DWNLD_ALREADY = 0x0202,
    APERTA2_MSGOUT_DWNLD_DONE    = 0x0303,
    APERTA2_MSGOUT_PRGRM_DONE    = 0x0404,
    APERTA2_MSGOUT_HEADER   = 0x0EAD,
    APERTA2_MSGOUT_HDR_ERR  = 0x0E0E,
    APERTA2_MSGOUT_FLASH    = 0xF1AC,
	APERTA2_MSGOUT_PLL_LOCK    = 0x0F0F
  } APERTA2_MSGOUT_T;

  typedef enum APERTA2_ACTION_TYPE_E {
      APERTA2_ACTION_TYPE_GET = 0,
      APERTA2_ACTION_TYPE_SET = 1,
  }APERTA2_ACTION_TYPE_T;

typedef enum aperta2_lane_data_rate_e {
    bcmAperta2LaneDataRateNone        = (0),
    bcmAperta2LaneDataRate_10P3125G   = (10312),
    bcmAperta2LaneDataRate_25P78125G  = (25781),
    bcmAperta2LaneDataRate_26P5625G   = (26562),
    bcmAperta2LaneDataRate_51P5625G   = (51562),
    bcmAperta2LaneDataRate_53P125G    = (53125),
    bcmAperta2LaneDataRate_106P25G    = (106250)
}aperta2_lane_data_rate_t ;

typedef enum aperta2_modulation_mode_e {
    bcmAperta2ModulationNONE = 0,
    bcmAperta2ModulationNRZ,
    bcmAperta2ModulationPAM4,
    bcmAperta2ModulationCount
} aperta2_modulation_mode_t;

typedef enum aperta2_fec_mode_sel_e {
    bcmAperta2NoFEC = 0,
    bcmAperta2RS544, /* RS544 */
    bcmAperta2RSFEC,
    bcmAperta2RS272,
    bcmAperta2RS272_2XN,
    bcmAperta2RS544_2XN,
    bcmAperta2FecCount
}aperta2_fec_mode_sel_t;

/*! Aperta2 Failover config
 *
 * \arg lane_map \n
 *  Lane map of the failover port
 *
 * \arg tx_back_pressure_mode \n
 *  This parameter indicates tx back pressure mode,
 *  0 : TX Back-pressure from active port
 *  1 : TX Back-pressure OR?ed from both ports(FO and active port)
 *         
 */
typedef struct aperta2_failover_configuration_s {
    unsigned int lane_map;
    unsigned int tx_back_pressure_mode;
}aperta2_failover_configuration_t;

/*! Fixed latency configuration
 *
 * This structure can be used to enable/disable fixed latency in aperta2. 
 * It is also used to configure dp_ck_cycles
 *
 * \arg enable \n
 *     When set to "1" it enables the fixed latency, "0" Disables
 *
 * \arg start_point
 *     When set to 1 the Start point is PM rx when 0 it is wall clock timer
 *
 * \arg dp_ck_cycles \n
 *     This defines the number of dp_ck clock cycles that a packet
 *     should take from entering PM2MS interface to leaving MS2PM interface.
 *
 */
typedef struct aperta2_fixed_latency_config_s {
    unsigned int enable;
    unsigned int start_point;     
    unsigned int dp_ck_cycles;
} aperta2_fixed_latency_config_t;

typedef enum aperta2_1588_config_e {
    bcmAperta2PTPDisabled = 0,
    bcmAperta2PTPNseEnabled, /*NSE TS for egress and Ingress (packets encrypted)*/
    bcmAperta2PTPNsePmEnabled, /*NSE TS for egress and PM-TS for Ingress(packets encrypted)*/
    bcmAperta2PTPPmEnabled, /*PM TS for egress and PM-TS for Ingress(packets not encrypted)*/
    bcmAperta2PTPPmNseEnabled, /*PM TS for egress and NSE-TS for Ingress(packets not encrypted)*/
    bcmAperta2PTPHybNseEnabled ,/* Hybrid mode for Egress & NSE for Ingress (packets not encrypted)*/
    bcmAperta2PTPHybPmEnabled ,/* Hybrid mode for Egress & PM for Ingress (packets encrypted)*/
} aperta2_1588_config_t;

typedef enum aperta2_port_type_e {
    bcmAperta2PortTypePassthrough = 0,  /* Passthrough Port*/
    bcmAperta2PortTypeGearBox ,         /* Gearbox port*/
    bcmAperta2PortTypeReverseGearBox    /* Reverse Gearbox port*/
}aperta2_port_type_t;

typedef enum aperta2_octal_crossing_e {
    bcmAperta2NoOctalCrossing = 0,       /* No Octal Crossing */
    bcmAperta2SysOctalCrossing ,         /* System Side Octal crossing*/
    bcmAperta2LineOctalCrossing          /* Line Side Octal Crossing*/
}aperta2_octal_crossing_t;

/* Module controller I2C master commands */
typedef enum aperta2_i2c_module_cmd_e{
    APERTA2_FLUSH = (0),
    APERTA2_RANDOM_ADDRESS_READ,
    APERTA2_CURRENT_ADDRESS_READ,
    APERTA2_I2C_WRITE
} aperta2_i2c_module_cmd_t;

/*! Aperta2 device aux mode
*
*  This structure can be used to set up multiple modes that aperta supports. \n
*  This structure is used in bcm_plp_mode_config_set/bcm_plp_mode_config_get APIs. \n
*  Gearbox or Inverse or Passthrough modes are created based on lane_map combinations
*  on respective sides using bcm_plp_mode_config_set API.
*
* \arg lane_data_rate \n
*   Aperta2 supports multiple lane data rates on System and Line side based on
*   FEC modes selected (bcm_plp_fec_mode_sel_t). \n
*   Per lane data rate of the port. Ex: For 56.25G. lane_data_rate = bcmplpLaneDataRate_56P25G \n
*
*   When port is created of more than one lane; port speed is same as PCS/MAC port speed
*   and lane data rate will be different based on number of lanes and FEC operations. \n
*   Please refer to the chip specific data sheet for supported modes \n
*
* \arg modulation_mode \n
*   Type of modulation used to configure the port. Modulation mode can be PAM4 or NRZ.\n
*   bcm_plp_modulation_mode_t enum has supported modes. \n
*
* \arg fec_mode_sel \n
*   Fec mode used to configure the port. bcm_plp_aperta_fec_mode_select_t has supported modes. \n
*   User can choose different types of FECs on System/Line side of Aperta based on the system requirements \n
*
*  \arg failover_config \n
*   Lane map of the secondary port in case of fail over mode.\n
*
*  \arg fixed_latency_config \n
*   fixed latency configuration\n
*
*  \arg ts_config \n
*   Time Sync configuration\n
*
*  \arg port_type \n
*     This defines the type of a port (Passthrough, Gearbox, Reverse Gearbox).
*
*  \arg octal_crossing \n
*     This defines the type of a octal crossing.
*
*  Not all the combinations are supported. For supported combinations; please refer to product specific data sheet\n
*/
typedef struct aperta2_device_aux_modes_s {
    aperta2_lane_data_rate_t                 lane_data_rate    ;
    aperta2_modulation_mode_t                modulation_mode   ;
    aperta2_fec_mode_sel_t                   fec_mode_sel      ;
    aperta2_failover_configuration_t         failover_config   ;
    aperta2_fixed_latency_config_t           ing_fixed_latency;
    aperta2_fixed_latency_config_t           egr_fixed_latency;
    aperta2_1588_config_t                    ts_config;
    aperta2_port_type_t                      port_type;
    aperta2_octal_crossing_t                 octal_crossing;
} aperta2_device_aux_modes_t;

typedef enum aperta2_tsc_clock_side_e {
    aperta2TscClockSelHWdefault = 0,
    aperta2TscClockSelSystemPM,
    aperta2TscClockSelLinePm
} aperta2_tsc_clock_side_t;

/* AVS FW registers */
/* AVS_FW_control_TYPE - 0x3A0 */
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_AVS_PKG_SHARE_GET(r) ((((r).fws_fwreg_3a0[0]) >> 8) & 0xf)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_AVS_PKG_SHARE_SET(r,f) (r).fws_fwreg_3a0[0]=(((r).fws_fwreg_3a0[0] & ~((uint32_t)0xf << 8)) | ((((uint32_t)f) & 0xf) << 8))
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_AVS_IS_PRI_GET(r) ((((r).fws_fwreg_3a0[0]) >> 3) & 0x1)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_AVS_IS_PRI_SET(r,f) (r).fws_fwreg_3a0[0]=(((r).fws_fwreg_3a0[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3))
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_AVS_CTRL_GET(r) ((((r).fws_fwreg_3a0[0]) >> 2) & 0x1)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_AVS_CTRL_SET(r,f) (r).fws_fwreg_3a0[0]=(((r).fws_fwreg_3a0[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2))
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_AVS_DIS_TYPE_GET(r) ((((r).fws_fwreg_3a0[0]) >> 1) & 0x1)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_AVS_DIS_TYPE_SET(r,f) (r).fws_fwreg_3a0[0]=(((r).fws_fwreg_3a0[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1))
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_AVS_EN_GET(r) (((r).fws_fwreg_3a0[0]) & 0x1)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_AVS_EN_SET(r,f) (r).fws_fwreg_3a0[0]=(((r).fws_fwreg_3a0[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1))

/* AVS_FW_status_TYPE - 0x3A1 */
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A1r_AVS_TRACK_STS_GET(r) ((((r).fws_fwreg_3a1[0]) >> 8) & 0xf)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A1r_AVS_TRACK_STS_SET(r,f) (r).fws_fwreg_3a1[0]=(((r).fws_fwreg_3a1[0] & ~((uint32_t)0xf << 8)) | ((((uint32_t)f) & 0xf) << 8))
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A1r_AVS_STS_GET(r) ((((r).fws_fwreg_3a1[0]) >> 4) & 0xf)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A1r_AVS_STS_SET(r,f) (r).fws_fwreg_3a1[0]=(((r).fws_fwreg_3a1[0] & ~((uint32_t)0xf << 4)) | ((((uint32_t)f) & 0xf) << 4))
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A1r_AVS_ENABLED_GET(r) (((r).fws_fwreg_3a1[0]) & 0x1)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A1r_AVS_ENABLED_SET(r,f) (r).fws_fwreg_3a1[0]=(((r).fws_fwreg_3a1[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1))

/* AVS_regulator0_config_TYPE - 0x3A2 */
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A2r_AVS_REGULATOR_ADDR_GET(r) (((r).fws_fwreg_3a2[0]) & 0x7f)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A2r_AVS_REGULATOR_ADDR_SET(r,f) (r).fws_fwreg_3a2[0]=(((r).fws_fwreg_3a2[0] & ~((uint32_t)0x7f)) | (((uint32_t)f) & 0x7f))
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A2r_AVS_I2C_SPEED_GET(r) ((((r).fws_fwreg_3a2[0]) >> 7) & 0x1)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A2r_AVS_I2C_SPEED_SET(r,f) (r).fws_fwreg_3a2[0]=(((r).fws_fwreg_3a2[0] & ~((uint32_t)0x1 << 7)) | ((((uint32_t)f) & 0x1) << 7))
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A2r_AVS_REGULATOR_TYPE_GET(r) ((((r).fws_fwreg_3a2[0]) >> 8) & 0xf)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A2r_AVS_REGULATOR_TYPE_SET(r,f) (r).fws_fwreg_3a2[0]=(((r).fws_fwreg_3a2[0] & ~((uint32_t)0xf << 8)) | ((((uint32_t)f) & 0xf) << 8))
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A2r_AVS_REGULATOR_CH0_INFO_GET(r) ((((r).fws_fwreg_3a2[0]) >> 12) & 0x3)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A2r_AVS_REGULATOR_CH0_INFO_SET(r,f) (r).fws_fwreg_3a2[0]=(((r).fws_fwreg_3a2[0] & ~((uint32_t)0x3 << 12)) | ((((uint32_t)f) & 0x3) << 12))
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A2r_AVS_REGULATOR_CH1_INFO_GET(r) ((((r).fws_fwreg_3a2[0]) >> 14) & 0x3)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A2r_AVS_REGULATOR_CH1_INFO_SET(r,f) (r).fws_fwreg_3a2[0]=(((r).fws_fwreg_3a2[0] & ~((uint32_t)0x3 << 14)) | ((((uint32_t)f) & 0x3) << 14))

/* AVS_regulator1_config_TYPE - 0x3A3 */
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A3r_AVS_REGULATOR_ADDR_GET(r) (((r).fws_fwreg_3a3[0]) & 0x7f)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A3r_AVS_REGULATOR_ADDR_SET(r,f) (r).fws_fwreg_3a3[0]=(((r).fws_fwreg_3a3[0] & ~((uint32_t)0x7f)) | (((uint32_t)f) & 0x7f))
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A3r_AVS_I2C_SPEED_GET(r) ((((r).fws_fwreg_3a3[0]) >> 7) & 0x1)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A3r_AVS_I2C_SPEED_SET(r,f) (r).fws_fwreg_3a3[0]=(((r).fws_fwreg_3a3[0] & ~((uint32_t)0x1 << 7)) | ((((uint32_t)f) & 0x1) << 7))
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A3r_AVS_REGULATOR_TYPE_GET(r) ((((r).fws_fwreg_3a3[0]) >> 8) & 0xf)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A3r_AVS_REGULATOR_TYPE_SET(r,f) (r).fws_fwreg_3a3[0]=(((r).fws_fwreg_3a3[0] & ~((uint32_t)0xf << 8)) | ((((uint32_t)f) & 0xf) << 8))
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A3r_AVS_REGULATOR_CH0_INFO_GET(r) ((((r).fws_fwreg_3a3[0]) >> 12) & 0x3)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A3r_AVS_REGULATOR_CH0_INFO_SET(r,f) (r).fws_fwreg_3a3[0]=(((r).fws_fwreg_3a3[0] & ~((uint32_t)0x3 << 12)) | ((((uint32_t)f) & 0x3) << 12))
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A3r_AVS_REGULATOR_CH1_INFO_GET(r) ((((r).fws_fwreg_3a3[0]) >> 14) & 0x3)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A3r_AVS_REGULATOR_CH1_INFO_SET(r,f) (r).fws_fwreg_3a3[0]=(((r).fws_fwreg_3a3[0] & ~((uint32_t)0x3 << 14)) | ((((uint32_t)f) & 0x3) << 14))

/* AVS_I2C_address0_TYPE - 0x3A4 */
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A4r_AVS_DVDD_RAIL_ADDR_GET(r) (((r).fws_fwreg_3a4[0]) & 0x7f)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A4r_AVS_DVDD_RAIL_ADDR_SET(r,f) (r).fws_fwreg_3a4[0]=(((r).fws_fwreg_3a4[0] & ~((uint32_t)0x7f)) | (((uint32_t)f) & 0x7f))
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A4r_AVS_RESPONDER0_ADDR_GET(r) ((((r).fws_fwreg_3a4[0]) >> 8) & 0x7f)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A4r_AVS_RESPONDER0_ADDR_SET(r,f) (r).fws_fwreg_3a4[0]=(((r).fws_fwreg_3a4[0] & ~((uint32_t)0x7f << 8)) | ((((uint32_t)f) & 0x7f) << 8))

/* AVS_I2C_address1_TYPE - 0x3A5 */
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A5r_AVS_RESPONDER1_ADDR_GET(r) (((r).fws_fwreg_3a5[0]) & 0x7f)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A5r_AVS_RESPONDER1_ADDR_SET(r,f) (r).fws_fwreg_3a5[0]=(((r).fws_fwreg_3a5[0] & ~((uint32_t)0x7f)) | (((uint32_t)f) & 0x7f))
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A5r_AVS_RESPONDER2_ADDR_GET(r) ((((r).fws_fwreg_3a5[0]) >> 8) & 0x7f)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A5r_AVS_RESPONDER2_ADDR_SET(r,f) (r).fws_fwreg_3a5[0]=(((r).fws_fwreg_3a5[0] & ~((uint32_t)0x7f << 8)) | ((((uint32_t)f) & 0x7f) << 8))

/* AVS_Vout_control_TYPE - 0x3A6 */
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A6r_AVS_BRD_DC_MARGIN_GET(r) (((r).fws_fwreg_3a6[0]) & 0x1f)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A6r_AVS_BRD_DC_MARGIN_SET(r,f) (r).fws_fwreg_3a6[0]=(((r).fws_fwreg_3a6[0] & ~((uint32_t)0x1f)) | (((uint32_t)f) & 0x1f))
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A6r_AVS_EXT_CTRL_STEP_GET(r) ((((r).fws_fwreg_3a6[0]) >> 5) & 0x1)
#define BCM_APERTA2_DIRECT_FWS_FWREG_3A6r_AVS_EXT_CTRL_STEP_SET(r,f) (r).fws_fwreg_3a6[0]=(((r).fws_fwreg_3a6[0] & ~((uint32_t)0x1 << 5)) | ((((uint32_t)f) & 0x1) << 5))

/* AVS Init done for SW and FW handshake (GEN_CNTRLS_gpreg_01 bit 4) */
#define BCM_APERTA2_DIRECT_GEN_CNTRLS_GPREG_01r_AVS_INIT_DONE_GET(r) ((((r).gen_cntrls_gpreg_01[0]) >> 4) & 0x1)
#define BCM_APERTA2_DIRECT_GEN_CNTRLS_GPREG_01r_AVS_INIT_DONE_SET(r,f) (r).gen_cntrls_gpreg_01[0]=(((r).gen_cntrls_gpreg_01[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4))

int plp_aperta2_get_chip_id (const plp_aperta2_phymod_access_t *pa, int *chip_id);
int plp_aperta2_get_chip_rev (const plp_aperta2_phymod_access_t *pa, uint32_t *chip_rev);
int _plp_aperta2_core_reset_set(const plp_aperta2_phymod_core_access_t* core, plp_aperta2_phymod_reset_mode_t reset_mode, plp_aperta2_phymod_reset_direction_t direction);
int _plp_aperta2_core_firmware_info_get(const plp_aperta2_phymod_access_t *acc, plp_aperta2_phymod_core_firmware_info_t* fw_info);
int _plp_aperta2_check_fw_download_status(const plp_aperta2_phymod_core_access_t *core_access, plp_aperta2_phymod_firmware_load_method_t load_method);
int _plp_aperta2_download_prog_eeprom(const plp_aperta2_phymod_core_access_t *core_access,
        uint8_t *plp_aperta2_ucode, uint32_t fw_length, uint16_t master_en,
        uint16_t mst_boot_addr, uint8_t prg_eeprom, const plp_aperta2_phymod_core_init_config_t* init_config);

int plp_aperta2_dload_fw(const plp_aperta2_phymod_core_access_t* core, const plp_aperta2_phymod_core_init_config_t* init_config);
plp_aperta2_phymod_interface_t _plp_aperta2_convert_numeric_value_to_interface_type(uint32_t numeric_val);
int plp_aperta2_sw_intf_set(const plp_aperta2_phymod_phy_access_t *phy, plp_aperta2_phymod_interface_t if_type);
int plp_aperta2_sw_intf_get(const plp_aperta2_phymod_phy_access_t *phy, uint8_t lane_index, plp_aperta2_phymod_interface_t *if_type);
int _plp_aperta2_phy_status_dump(const plp_aperta2_phymod_phy_access_t* phy);
int plp_aperta2_init_db(const plp_aperta2_phymod_core_access_t* core, const plp_aperta2_phymod_core_init_config_t* init_config) ;

int plp_aperta2_ded_wka(const plp_aperta2_phymod_core_access_t* core, const plp_aperta2_phymod_core_init_config_t* init_config);
int _plp_aperta2_intr_enable_set(const plp_aperta2_phymod_phy_access_t *phy, uint32_t port, uint32_t intr_type_enable);
int _plp_aperta2_intr_enable_get(const plp_aperta2_phymod_phy_access_t *phy, uint32_t port, uint32_t *intr_enable);
int _plp_aperta2_phy_intr_status_get(const plp_aperta2_phymod_phy_access_t* phy, unsigned int port, uint32_t *intr_sts);
int _plp_aperta2_phy_intr_status_clear(const plp_aperta2_phymod_phy_access_t* phy, unsigned int port, uint32_t intr_type);
int _plp_aperta2_phy_gpio_pin_value_get(const plp_aperta2_phymod_phy_access_t* phy, int pin_no, int* value);
int _plp_aperta2_phy_gpio_pin_value_set(const plp_aperta2_phymod_phy_access_t* phy, int pin_no, int value);
int _plp_aperta2_phy_gpio_config_get(const plp_aperta2_phymod_phy_access_t* phy, int pin_no, plp_aperta2_phymod_gpio_mode_t* gpio_mode);
int _plp_aperta2_phy_gpio_config_set(const plp_aperta2_phymod_phy_access_t* phy, int pin_no, plp_aperta2_phymod_gpio_mode_t gpio_mode);
int _plp_aperta2_phy_i2c_write(const plp_aperta2_phymod_phy_access_t* phy, uint32_t slv_dev_addr, uint32_t start_addr, uint32_t no_of_bytes, const uint8_t* write_data);
int _plp_aperta2_phy_i2c_read(const plp_aperta2_phymod_phy_access_t* phy, uint32_t slv_dev_addr, uint32_t start_addr, uint32_t no_of_bytes, uint8_t* read_data);
#endif /*__APERTA2_CFG_SEQ_H__*/
