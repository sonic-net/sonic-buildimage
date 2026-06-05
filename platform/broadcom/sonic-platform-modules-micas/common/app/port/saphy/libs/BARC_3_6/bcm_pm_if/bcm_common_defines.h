/*
 *
 * $Id: bcm_common_defines.h $
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 *
 */
#ifndef BCM_COMMON_DEFINES_H
#define BCM_COMMON_DEFINES_H
#define BCM_LINE_SIDE 0
#define BCM_SYSTEM_SIDE 1

#define BCM_NUM_DFE_TAPS 18
#define MAX_LANES_PER_CORE  16
#define _PLP_API_UNAVAILABLE 0xdead
#define BCM_PLP_COLD_BOOT (0)
#define BCM_PLP_WARM_BOOT (1 << 0)
#define BCM_PLP_AN_MODE_CL37 (1 << 1)
#define BCM_PLP_CL37_SGMII_MASTER_MODE (1 << 2)
#define BCM_PLP_SGMII_MODE_RECONFIG (1 << 14)
#define BCM_PLP_PCS_LOGICAL_LANES 5
#define BCM_PLP_PCS_ENCODED_BIT_STREAMS 20 /* 20 lanes per 100G are logical lanes per IEEE clause 82 */
#ifdef _STDINT_H
typedef uint64_t        plp_uint64_t;
#else
typedef unsigned long long int plp_uint64_t;
#endif


/* Serdes debug dump levels */

/*
 * This displays the core state and lane state information
 */
#define BCM_PLP_INTERNAL_DUMP (1 << 3)
/*
 * This displays the Chip configurations if applicable to chip
 */
#define BCM_PLP_INTERNAL_CHIP_DUMP (1 << 4)
/*
 * This displays the core state and lane state configurations
 */
#define BCM_PLP_INTERNAL_CORE_LANE_DUMP (1 << 5)
#define BCM_PLP_INTERNAL_CORE_LANE_DUMP_DUMP BCM_PLP_INTERNAL_CORE_LANE_DUMP 
/*
 * This displays the chip specific DSC dump if applicable to chip
 */
#define BCM_PLP_INTERNAL_CHIP_DSC_DUMP (1 << 6)
/*
 * This displays the serdes Diag and register dump if applicable to chip
 */
#define BCM_PLP_INTERNAL_DIAG_REG_DUMP (1 << 7)
/*
 * This displays the serdes event and uc info
 */
#define BCM_PLP_INTERNAL_EVENT_UC_DUMP (1 << 8)
/*
 * This displays the core state, the lane state, the extended
 * lane state and the event log
 */
#define BCM_PLP_INTERNAL_DUMP_L1 (1 << 9)
/*
 * This displays the core state, the lane state, the extended
 * lane state, the event log, lane and core registers as well
 * as lane and core uc variables.
 */
#define BCM_PLP_INTERNAL_DUMP_L2 (1 << 10)
/*
 * This displays the core state, the lane state, the extended
 * lane state, the event log, fast eye scan, lane and core
 * registers as well as lane and core uc variables.
 */
#define BCM_PLP_INTERNAL_DUMP_L3 (1 << 11)

/*This displays minimal PCS MAC and MACSEC registers*/
#define BCM_PLP_PCS_DIAG_DUMP_L1 (1 << 12)

/*This displays extensive PCS MAC and MACSEC registers*/
#define BCM_PLP_PCS_DIAG_DUMP_L2 (1 << 13)

/*
 * This displays customized diag parameters like
 * pcs_sync, pcs_block_lock, pcs_rx_fault, pcs_tx_fault,
 * pmd_rx_fault, pmd_tx_fault, hi_ber, ber_count, delay_skew
 */
#define BCM_PLP_CUSTOM_DIAG_DUMP (1 << 14)

/* BCM layer error codes */
#define BCM_PM_IF_SUCCESS                0
#define BCM_PM_NOT_FOUND                -20
#define BCM_PM_IF_MEMORY                -21
#define SOC_CORE_NOT_FOUND              -22
#define BCM_PM_IF_INTERNAL              -23
#define BCM_PM_IF_PHY_EXISTING          -24
#define BCM_PM_IF_PHY_NA                -25
#define BCM_PM_IF_INVALID_PHY           -26
#define BCM_PM_IF_UNAVAIL               -27
#define BCM_PM_IF_PARAM                 -28

#ifdef SERDES_API_FLOATING_POINT
   typedef float  plp_float_t;
#else
   typedef int  plp_float_t;
#endif

typedef int (*bcm_plp_mutex_take_f)(unsigned int phy_id, void* platform_ctxt);

typedef int (*bcm_plp_mutex_give_f)(unsigned int phy_id, void* platform_ctxt);

typedef struct bcm_plp_mutex_s {
    bcm_plp_mutex_take_f mutex_take; /* bcm mutex take*/
    bcm_plp_mutex_give_f mutex_give; /* bcm mutex give*/
} bcm_plp_mutex_info_t;

typedef enum bcm_pm_interface_e {
    bcm_pm_InterfaceBypass = 0,
    bcm_pm_InterfaceSR,
    bcm_pm_InterfaceSR4,
    bcm_pm_InterfaceKX,
    bcm_pm_InterfaceKX4,
    bcm_pm_InterfaceKR,
    bcm_pm_InterfaceKR2,
    bcm_pm_InterfaceKR4,
    bcm_pm_InterfaceCX,
    bcm_pm_InterfaceCX2,
    bcm_pm_InterfaceCX4,
    bcm_pm_InterfaceCR,
    bcm_pm_InterfaceCR2,
    bcm_pm_InterfaceCR4,
    bcm_pm_InterfaceCR10,
    bcm_pm_InterfaceXFI,
    bcm_pm_InterfaceSFI,
    bcm_pm_InterfaceSFPDAC,
    bcm_pm_InterfaceXGMII,
    bcm_pm_Interface1000X,
    bcm_pm_InterfaceSGMII,
    bcm_pm_InterfaceXAUI,
    bcm_pm_InterfaceRXAUI,
    bcm_pm_InterfaceX2,
    bcm_pm_InterfaceXLAUI,
    bcm_pm_InterfaceXLAUI2,
    bcm_pm_InterfaceCAUI,
    bcm_pm_InterfaceQSGMII,
    bcm_pm_InterfaceLR4,
    bcm_pm_InterfaceLR,
    bcm_pm_InterfaceLR2,
    bcm_pm_InterfaceER,
    bcm_pm_InterfaceER2,
    bcm_pm_InterfaceER4,
    bcm_pm_InterfaceSR2,
    bcm_pm_InterfaceSR10,
    bcm_pm_InterfaceCAUI4,
    bcm_pm_InterfaceVSR,
    bcm_pm_InterfaceLR10,
    bcm_pm_InterfaceKR10,
    bcm_pm_InterfaceCAUI4_C2C,
    bcm_pm_InterfaceCAUI4_C2M,
    bcm_pm_InterfaceZR,
    bcm_pm_InterfaceLRM,
    bcm_pm_InterfaceXLPPI,
    bcm_pm_InterfaceOTN,
    bcm_pm_InterfaceAUI_C2C,
    bcm_pm_InterfaceAUI_C2M,
    bcm_pm_Interface2500R,
    bcm_pm_Interface2500X,
    bcm_pm_Interface5000R,
    bcm_pm_Interface5000X,
    bcm_pm_InterfaceUSXGMII,
    bcm_pm_InterfaceKRUSXGMII,
    bcm_pm_InterfaceCEIMR,
    bcm_pm_InterfaceCEILR,
    bcm_pm_InterfaceCount
} bcm_pm_interface_t;
typedef bcm_pm_interface_t bcm_plp_pm_interface_t;

typedef enum bcm_pm_ref_clk_e {
    bcm_pm_RefClk156Mhz = 0, /**< 156.25MHz */
    bcm_pm_RefClk125Mhz, /**< 125MHz */
    bcm_pm_RefClk106Mhz, /**< 106Mhz */
    bcm_pm_RefClk161Mhz, /**< 161Mhz */
    bcm_pm_RefClk174Mhz, /**< 174Mhz */
    bcm_pm_RefClk312Mhz, /**< 312Mhz */
    bcm_pm_RefClk322Mhz, /**< 322Mhz */
    bcm_pm_RefClk349Mhz, /**< 349Mhz */
    bcm_pm_RefClk644Mhz, /**< 644Mhz */
    bcm_pm_RefClk698Mhz, /**< 698Mhz */
    bcm_pm_RefClk155Mhz, /**< 155Mhz */
    bcm_pm_RefClk156P6Mhz, /**< 156P6Mhz */
    bcm_pm_RefClk157Mhz, /**< 157Mhz */
    bcm_pm_RefClk158Mhz, /**< 158Mhz */
    bcm_pm_RefClk159Mhz, /**< 159Mhz */
    bcm_pm_RefClk168Mhz, /**< 168Mhz */
    bcm_pm_RefClk172Mhz, /**< 172Mhz */
    bcm_pm_RefClk173Mhz, /**< 173Mhz */
    bcm_pm_RefClk169P409Mhz, /**< 169P409Mhz */
    bcm_pm_RefClk348P125Mhz, /**< 348P125Mhz */
    bcm_pm_RefClk162P948Mhz, /**< 162P948Mhz */
    bcm_pm_RefClk336P094Mhz, /**< 336P094Mhz */
    bcm_pm_RefClk168P12Mhz, /**< 168P12Mhz */
    bcm_pm_RefClk346P74Mhz, /**< 346P74Mhz */
    bcm_pm_RefClk167P41Mhz, /**< 167P41Mhz */
    bcm_pm_RefClk345P28Mhz, /**< 345P28Mhz */
    bcm_pm_RefClk162P26Mhz, /**< 162P26Mhz */
    bcm_pm_RefClk334P66Mhz, /**< 334P66Mhz */
    bcm_pm_RefClk212P5Mhz, /**< 212.5Mhz */
    bcm_pm_RefClk311P04Mhz, /**< 311.04Mhz */
    bcm_pm_RefClk425Mhz, /**< 425Mhz */
    bcm_pm_RefClk174P70Mhz, /**< 174P70Mhz */
    bcm_pm_RefClk122P88Mhz, /**< 122P88Mhz */
    bcm_pm_RefClk106P5Mhz, /**< 106P5Mhz */
    bcm_pm_RefClk167P38Mhz, /**< 167P38Mhz */
    bcm_pm_RefClk106P25Mhz, /**< 106P25Mhz */
    bcm_pm_RefClk155P52Mhz, /**< 155P52Mhz */
    bcm_pm_RefClk156P637Mhz, /**< 156P637Mhz */
    bcm_pm_RefClk157P844Mhz, /**< 157P844Mhz */
    bcm_pm_RefClk158P51Mhz, /**< 158P51Mhz */
    bcm_pm_RefClk159P375Mhz, /**< 159P375Mhz */
    bcm_pm_RefClk167P331Mhz, /**< 167P331Mhz */
    bcm_pm_RefClk168P04Mhz, /**< 168P04Mhz */
    bcm_pm_RefClk173P37Mhz, /**< 173P37Mhz */
    bcm_pm_RefClkCount
} bcm_pm_ref_clk_t;
typedef bcm_pm_ref_clk_t bcm_plp_pm_ref_clk_t;

typedef enum bcm_pm_interface_mode_e {
    bcm_pm_Interface_mode_IEEE = 0,
    bcm_pm_Interface_mode_HIGIG,
    bcm_pm_Interface_mode_OTN,
    bcm_pm_Interface_mode_FIBER,
    bcm_pm_Interface_mode_proprietary=11,
    bcm_pm_Interface_mode_count
} bcm_pm_interface_mode_t;
typedef bcm_pm_interface_mode_t bcm_plp_pm_interface_mode_t;

typedef enum bcm_pm_usxgmii_mode_e {
    bcmpmUsxgmiiModeDisable = 0x0,
    bcmpmUsxgmiiModeSingle  = 0x1,
    bcmpmUsxgmiiModeDual    = 0x2,
    bcmpmUsxgmiiModeQuad    = 0x4,
    bcmpmUsxgmiiModeOctal   = 0x8,
    bcmpmUsxgmiiModeAutoNeg = 0x10,
    bcmpmUsxgmiiModeCount,
} bcm_pm_usxgmii_mode_t;
typedef bcm_pm_usxgmii_mode_t bcm_plp_pm_usxgmii_mode_t;

typedef enum bcm_pm_prbs_poly_e {
    bcm_pm_PrbsPoly7 = 0,
    bcm_pm_PrbsPoly9,
    bcm_pm_PrbsPoly11,
    bcm_pm_PrbsPoly15,
    bcm_pm_PrbsPoly23,
    bcm_pm_PrbsPoly31,
    bcm_pm_PrbsPoly58,
    bcm_pm_PrbsPoly49,
    bcm_pm_PrbsPoly10,
    bcm_pm_PrbsPoly20,
    bcm_pm_PrbsPoly13,
    bcm_pm_PrbsPolyCount
} bcm_pm_prbs_poly_t;
typedef bcm_pm_prbs_poly_t bcm_plp_pm_prbs_poly_t;

typedef enum bcm_pm_osr_mode_e {
    bcmpmOversampleMode1 = 0,
    bcmpmOversampleMode2,
    bcmpmOversampleMode3,
    bcmpmOversampleMode3P3,
    bcmpmOversampleMode4,
    bcmpmOversampleMode5,
    bcmpmOversampleMode7P5,
    bcmpmOversampleMode8,
    bcmpmOversampleMode8P25,
    bcmpmOversampleMode10,
    bcmpmOversampleMode16P5,
    bcmpmOversampleMode20P625,
    bcmpmOversampleModeCount
} bcm_pm_osr_mode_t;
typedef bcm_pm_osr_mode_t bcm_plp_pm_osr_mode_t;

typedef enum bcm_pm_pmd_mode_e {
    bcmpmPmdModeOs = 0,
    bcmpmPmdModeOsDfe,
    bcmpmPmdModeBrDfe,
    bcmpmPmdModeCount
} bcm_pm_pmd_mode_t;
typedef bcm_pm_pmd_mode_t bcm_plp_pm_pmd_mode_t;

typedef enum bcm_pm_sequencer_operation_e {
    bcmpmSeqOpStop = 0, /* Stop Sequencer */
    bcmpmSeqOpStart, /* Start Sequencer */
    bcmpmSeqOpRestart, /* Toggle Sequencer */
    bcmpmSeqOpCount
} bcm_pm_sequencer_operation_t;
typedef bcm_pm_sequencer_operation_t bcm_plp_pm_sequencer_operation_t;

typedef enum bcm_pm_phy_tx_lane_control_e {
    bcmpmTxTrafficDisable = 0, /* disable tx traffic */
    bcmpmTxTrafficEnable, /* enable tx traffic */
    bcmpmTxReset, /* reset tx data path */
    bcmpmTxSquelchOn, /* squelch tx */
    bcmpmTxSquelchOff, /* squelch tx off */
    bcmpmTxCount
} bcm_pm_phy_tx_lane_control_t;
typedef bcm_pm_phy_tx_lane_control_t bcm_plp_pm_phy_tx_lane_control_t;

typedef enum bcm_pm_phy_rx_lane_control_e {
    bcmpmRxReset, /* reset rx data path */
    bcmpmRxSquelchOn, /* squelch rx */
    bcmpmRxSquelchOff, /* squelch rx off */
    bcmpmRxCount
} bcm_pm_phy_rx_lane_control_t;
typedef bcm_pm_phy_rx_lane_control_t bcm_plp_pm_phy_rx_lane_control_t;

typedef enum bcm_pm_firmware_broadcast_method_e {
	bcmpmFirmwareBroadcastNone=0,          /* Firmware downloaded as unicast for each PHY device on MDIO bus */
    bcmpmFirmwareBroadcastCoreReset,       /* Reset the core for all PHY IDs on MDIO bus */
    bcmpmFirmwareBroadcastEnable,          /* Enable the broadcast for all PHY IDs on MDIO bus */
    bcmpmFirmwareBroadcastFirmwareExecute, /* Load the FW for only one PHY of similar type of PHYs on MDIO bus */
    bcmpmFirmwareBroadcastFirmwareVerify,  /* FW load verify for all PHY IDs on MDIO bus */
    bcmpmFirmwareBroadcastEnd,             /* Disable the broadcast for all PHY IDs on MDIO bus */
    bcmpmFirmwareBroadcastCount
} bcm_pm_firmware_broadcast_method_t;
typedef bcm_pm_firmware_broadcast_method_t bcm_plp_firmware_broadcast_method_t;

typedef enum bcm_pm_firmware_load_method_e {
    bcmpmFirmwareLoadMethodNone = 0,   /* Do not download FW.  Use already downloaded FW instead */
    bcmpmFirmwareLoadMethodInternal,   /* Download FW internally via MDIO */
    bcmpmFirmwareLoadMethodExternal,   /* Load FW using a user registered function */
    bcmpmFirmwareLoadMethodProgEEPROM, /* Load FW and flash it into EEPROM */
    bcmpmFirmwareLoadMethodAuto,       /* Auto download in case of firmware change */
    bcmpmFirmwareLoadMethodCount
} bcm_pm_firmware_load_method_t;
typedef bcm_pm_firmware_load_method_t bcm_plp_pm_firmware_load_method_t;

/*!
 * @enum bcm_pm_firmware_load_force_e
 * @brief Firmware load force
 */
typedef enum bcm_pm_firmware_load_force_e {
    bcmpmFirmwareLoadSkip = 0, /* Skip firmware download if firmware is already present */
    bcmpmFirmwareLoadForce,    /* Always download the firmware specified by firmware load method */
    bcmpmFirmwareLoadAuto,     /* Check the firmware version. If it is different from current version download the firmware */
    bcmpmFirmwareLoadEEPROMUpgrade,  /* EEPROM Upgrade FW without disrupting the traffic */
    bcmpmFirmwareLoadCount
} bcm_pm_firmware_load_force_t;
typedef bcm_pm_firmware_load_force_t bcm_plp_firmware_load_force_t;

typedef enum bcm_pm_reset_direction_e {
    bcmpmResetDirectionIn = 0, /* In Reset */
    bcmpmResetDirectionOut, /* Out of Reset */
    bcmpmResetDirectionInOut, /* Toggle Reset */
    bcmpmResetDirectionNone, /* No Reset */
    bcmpmResetDirectionCount
} bcm_pm_reset_direction_t;
typedef bcm_pm_reset_direction_t bcm_plp_pm_reset_direction_t;

typedef enum bcm_pm_firmware_mode_e {
    bcm_pm_fw_default = 0,
    bcm_pm_fw_dfe,
    bcm_pm_fw_osdfe,
    bcm_pm_fw_br_dfe,
    bcm_pm_fw_lp_dfe,
    bcm_pm_fw_sfp_dac,
    bcm_pm_fw_xlaui,
    bcm_pm_fw_sfp_opt_sr4,
    bcm_pm_fw_firmware_mode_none,
    bcm_pm_firmware_mode_Count
} bcm_pm_firmware_mode_t;
typedef bcm_pm_firmware_mode_t bcm_plp_pm_firmware_mode_t;

typedef enum bcm_pm_firmware_media_type_e {
    bcmplpFirmwareMediaTypePcbTraceBackPlane = 0, /* back plane */
    bcmplpFirmwareMediaTypeCopperCable, /* copper cable */
    bcmplpFirmwareMediaTypeOptics, /* optical */
    bcmplpFirmwareMediaTypeCount
} bcm_pm_firmware_media_type_t;
typedef bcm_pm_firmware_media_type_t bcm_plp_pm_firmware_media_type_t;
/*!
 * @enum bcm_plp_failover_mode_e
 * @brief Failover configuration
 */
 typedef enum bcm_plp_failover_mode_e {
    bcmplpFailovermodeNone,
    bcmplpFailovermodeEnable, /* enable Failover mode */
    /* For Millenio Family Chips only */
    bcmplpFailovermodePrimary = 0,
    bcmplpFailovermodeSecondary = 1,
    bcmplpFailovermodeCount
} bcm_plp_failover_mode_t;

/*!
 * @enum bcm_plp_edc_config_method_e
 * @brief Configuration method for Electronic Dispersion Compensation (EDC)
 */
typedef enum bcm_plp_edc_config_method_e {
    bcmplpEdcConfigMethodNone,
    bcmplpEdcConfigMethodHardware, /* EDC mode is set automatically by hardware */
    bcmplpEdcConfigMethodSoftware, /* EDC mode is selected by driver software */
    bcmplpEdcConfigMethodCount
} bcm_plp_edc_config_method_t;

/*!
 * @enum bcm_plp_data_path_direction_e
 * @brief Direction of Datapath
 */
typedef enum bcm_plp_data_path_direction_e {
    bcmplpDataPathDirectionNONE = 0,      /* Default NONE */
    bcmplpDataPathDirectionEgress,        /* Egress */
    bcmplpDataPathDirectionIngress,       /* Ingress */
    bcmplpDataPathDirectionEgressIngress /* Both Egress and Ingress */
} bcm_plp_data_path_direction_t;


/*! PHY Access Information
 *
 * \arg void *platform_ctxt \n
 *             Represents user data. This member is passed to\n
 *             register read/write APIs. It can be NULL if not used.
 *
 * \arg unsigned int phy_addr \n
 *             Represents PHY-ID\n
 *
 * \arg unsigned int if_side \n
 *             Represents the interface side \n
 *                    0 - line side of the PHY device\n
 *                    1 - system side of the PHY device\n
 *
 * \arg unsigned int lane_map \n
 *             Represents the Lane mapping of a port\n
 *             LSB Bit 0 represents lane 0 of the specified PHY-ID.\n
 *             LSB Bit 1 represents lane 1 of the specified PHY-ID\n
 *             and similarly for lane 2 to lane N,\n
 *             where N is the maximum number of lanes on a PHY.
 *             It also supports multicast\n
 *             Eg:
 *                   0x3 represents lane 0 and 1 \n
 *                   0xF represents lane 0 to lane 3\n
 *
 * \arg unsigned int flags \n
 *             BCM_PLP_WARM_BOOT (1 << 0)
 *             BCM_PLP_AN_MODE_CL37 (1 << 1)
 *             BCM_PLP_CL37_SGMII_MASTER_MODE (1 << 2)
 *             BCM_PLP_INTERNAL_DUMP (1 << 3)
 *             BCM_PLP_INTERNAL_CHIP_DUMP (1 << 4)
 *             BCM_PLP_INTERNAL_CORE_LANE_DUMP (1 << 5)
 *             BCM_PLP_INTERNAL_CHIP_DSC_DUMP (1 << 6)
 *             BCM_PLP_INTERNAL_DIAG_REG_DUMP (1 << 7)
 *             BCM_PLP_INTERNAL_EVENT_UC_DUMP (1 << 8)
 *             BCM_PLP_INTERNAL_DUMP_L1 (1 << 9)
 *             BCM_PLP_INTERNAL_DUMP_L2 (1 << 10)
 *             BCM_PLP_INTERNAL_DUMP_L3 (1 << 11)
 *             BCM_PHY_MASTER_PORT (1 << 30)
 *             BCM_PHY_PHYSICAL_ADDR_GIVEN_BY_APP (1 << 31)
 *             Flags usage will be extended in the future as needed.
 *
 * \arg data_path_dir \n
 *           Represents the data path direction of a port \n
 *           Eg: For Egress               : bcmplpDataPathDirectionEgress \n
 *               for Ingress              : bcmplpDataPathDirectionIngress \n
 *               for Both(Egress/Ingress  : bcmplpDataPathDirectionEgressIngress \n
 */
typedef struct bcm_plp_access_s {
    void *platform_ctxt;
    unsigned int phy_addr;
    unsigned int if_side;
    unsigned int lane_map;
    unsigned int flags;
    bcm_plp_data_path_direction_t data_path_dir;
}bcm_plp_access_t;

/*! Loopback mode enum */
typedef enum bcm_plp_loopback_mode_e {
    bcmplpLpbkModeDigitalLoopback = 1,               /* Digital PMD loopback (not supported by all PHYs) */
    bcmplpLpbkModeRemoteLoopback = 2,                /* Remote PMD loopback (not supported by all PHYs) */
    bcmplpLpbkModeAnalogInternalLoopback = 3,        /* Analog internal loopback (not supported by all PHYs) */
                                                     /* Below are for 10GBase copper PHYs only */
    bcmplpLpbkModeDigitalLoopbackMatchLineSpeed = 1, /* Set digital loopback matching line side speed */
    bcmplpLpbkModeDigitalLoopback100M = 2,           /* Set digital loopback for 100M */
    bcmplpLpbkModeDigitalLoopback1G = 3,             /* Set digital loopback for 1G */
    bcmplpLpbkModeDigitalLoopback10G = 4,            /* Set digital loopback for 10G */
    bcmplpLpbkModeDigitalLoopback2p5G = 5,           /* Set digital loopback for 2.5G */
    bcmplpLpbkModeDigitalLoopback5G = 6,             /* Set digital loopback for 5G */
    bcmplpLpbkModeDigitalLoopback7p5G = 7,           /* Set digital loopback for 7.5G */
    bcmplpLpbkModeDigitalLoopback25G = 8,            /* Set digital loopback for 25G */
    bcmplpLpbkModeRemoteLoopbackXgBaseT = 10         /* Remote loopback */
}bcm_plp_loopback_mode_t;

typedef struct bcm_plp_value_override_s {
    unsigned int enable;
    int value;
} bcm_plp_value_override_t;

typedef struct bcm_plp_rx_s {
    bcm_plp_value_override_t vga;
    unsigned int num_of_dfe_taps; /*number of elements in DFE array*/
    bcm_plp_value_override_t dfe[BCM_NUM_DFE_TAPS];
    bcm_plp_value_override_t peaking_filter;
    bcm_plp_value_override_t low_freq_peaking_filter;
    bcm_plp_value_override_t high_freq_peaking_filter;
    bcm_plp_value_override_t ffe1; /* Feed Forward Equalization */
    bcm_plp_value_override_t ffe2; /* Feed Forward Equalization */
    bcm_plp_value_override_t kp_sweep; /* PAM4 KP sweep for Copper and PAM4 mode */
    bcm_plp_value_override_t fga;
    int rx_symbol_swap;/** Rx symbol_swap for PAM4 mode: 0- No swap  1- Swap   */
    int rx_graycode;   /** Rx gray code enable for PAM4 mode: 0- Disable; 1- Enable */
} bcm_plp_rx_t;

typedef enum bcm_plp_modulation_mode_e {
    bcmplpModulationNONE = 0,
    bcmplpModulationNRZ,
    bcmplpModulationPAM4,
    bcmplpModulationCount
} bcm_plp_modulation_mode_t;

typedef enum bcm_plp_fec_mode_sel_e {
    bcmplpNoFEC = 0,
    bcmplpSFEC,
    bcmplpKP4FEC, /* RS544 KP4 FEC */
    bcmplpRS544 = bcmplpKP4FEC, /* RS544 FEC */
    bcmplpJFEC,   /* Prop FEC */
    bcmplpPCSFEC, /* 100G, 50G, 25G, 10gG 40G PCS, depending upon mode different IEEE clause is applicable */
    bcmplpFCFEC,  /* FC_FEC CL74 */
    bcmplpKR4FEC, /* RS528 KR4 FEC */
    bcmplpRS528 = bcmplpKR4FEC, /* RS528 FEC */
    bcmplpFecCount
} bcm_plp_fec_mode_sel_t;

/*! Enum for AN capability advertisement */
typedef enum bcm_plp_tech_ability_e {
    bcmplpAnCap1G_KX            = 1,             /* AN_CAP_1G_KX[IEEE]                   */
    bcmplpAnCap10G_KX4          = 2,             /* AN_CAP_10G_KX4[IEEE]                 */
    bcmplpAnCap10G_KR           = 4,             /* AN_CAP_10G_KR[IEEE]                  */
    bcmplpAnCap40G_KR4          = 8,             /* AN_CAP_40G_KR4[IEEE]                 */
    bcmplpAnCap40G_CR4          = 0x10,          /* AN_CAP_40G_CR4[IEEE]                 */
    bcmplpAnCap100G_CR10        = 0x20,          /* AN_CAP_100G_CR10[IEEE]               */
    bcmplpAnCap100G_CR4         = 0x40,          /* AN_CAP_100G_CR4[IEEE]                */
    bcmplpAnCap100G_KR4         = 0x80,          /* AN_CAP_100G_KR4[IEEE]                */
    bcmplpAnCap25G_KRS1         = 0x1000,        /* AN_CAP_25G_KRS1[IEEE]                */
    bcmplpAnCap25G_CRS1         = 0x2000,        /* AN_CAP_25G_CRS1[IEEE]                */
    bcmplpAnCap25G_KR           = 0x4000,        /* AN_CAP_25G_KR[IEEE]                  */
    bcmplpAnCap25G_CR           = 0x8000,        /* AN_CAP_25G_CR[IEEE]                  */
    bcmplpAnCap50G_KR2          = 0x100,         /* AN_CAP_50G_KR2[Consortium]           */
    bcmplpAnCap50G_CR2          = 0x200,         /* AN_CAP_50G_CR2[Consortium]           */
    bcmplpAnCap25G_KR1          = 0x400,         /* AN_CAP_25G_KR1[Consortium]           */
    bcmplpAnCap25G_CR1          = 0x800,         /* AN_CAP_25G_CR1[Consortium]           */
    bcmplpAnCap50G_CR_KR        = 0x10000,       /* AN_CAP_50G_CR_KR[IEEE]               */
    bcmplpAnCap100G_CR2_KR2     = 0x20000,       /* AN_CAP_100G_CR2_KR2[IEEE]            */
    bcmplpAnCap200G_CR4_KR4     = 0x40000,       /* AN_CAP_200G_CR4_KR4[IEEE]            */
    bcmplpAnCap50G_KP           = 0x80000,       /* AN_CAP_50G_KP[Proprietary]           */
    bcmplpAnCap100G_KP2         = 0x100000,      /* AN_CAP_100G_KP2[Proprietary]         */
    bcmplpAnCap200G_KP4         = 0x200000,      /* AN_CAP_200G_KP4[Proprietary]         */
    bcmplpAnCap400G_CR8_KR8     = 0x400000,      /* AN_CAP_400G_CR8_KR8[Consortium]      */
    bcmplpAnCap50G_CR1_KR1      = 0x800000,      /* AN_CAP_50G_CR1_KR1[Consortium]       */
    bcmplpAnCapBAM_50G_CR1_KR1  = 0x1000000,     /* AN_CAP_BAM_50G_CR1_KR1[Proprietary]  */
    bcmplpAnCapBAM_50G_CR2_KR2  = 0x2000000,     /* AN_CAP_BAM_50G_CR2_KR2[Proprietary]  */
    bcmplpAnCapBAM_100G_CR2_KR2 = 0x4000000,     /* AN_CAP_BAM_100G_CR2_KR2[Proprietary] */
    bcmplpAnCapBAM_100G_CR4_KR4 = 0x8000000,     /* AN_CAP_BAM_100G_CR4_KR4[Proprietary] */
    bcmplpAnCapBAM_200G_CR4_KR4 = 0x10000000,    /* AN_CAP_BAM_200G_CR4_KR4[Proprietary] */
    bcmplpAnCapBAM_400G_CR8_KR8 = 0x20000000,    /* AN_CAP_BAM_400G_CR8_KR8[Proprietary] */
} bcm_plp_tech_ability_t;


/*! Enum for AN capability extension advertisement BAM AN Modes*/
typedef enum bcm_plp_tech_ability_ext_e {
    bcmplpAnCapBAM_20G_CR1      = 0x1,
    bcmplpAnCapBAM_20G_KR1      = 0x2,
    bcmplpAnCapBAM_20G_CR2      = 0x4,
    bcmplpAnCapBAM_20G_KR2      = 0x8,
    bcmplpAnCapBAM_25G_CR1      = 0x10,
    bcmplpAnCapBAM_25G_KR1      = 0x20,
    bcmplpAnCapBAM_40G_CR2      = 0x40,
    bcmplpAnCapBAM_40G_KR2      = 0x80,
    bcmplpAnCapBAM_50G_CR4      = 0x100,
    bcmplpAnCapBAM_50G_KR4      = 0x200,
    bcmplpAnCapBAM_100G_CR1_KR1 = 0x400,    /* AN_CAP_BAM_100G_CR1_KR1[Proprietary] */
    bcmplpAnCap100G_CR1_KR1     = 0x800,    /* AN_CAP_100G_CR1_KR1 [IEEE]           */
    bcmplpAnCap200G_CR2_KR2     = 0x1000,   /* AN_CAP_200G_CR2_KR2 [IEEE]           */
    bcmplpAnCap400G_CR4_KR4     = 0x2000,   /* AN_CAP_400G_CR4_KR4 [IEEE]           */
    bcmplpAnCapMSA_100G_CR2_KR2 = 0x4000,   /* AN_CAP_MSA_100G_CR2_KR2[Consortium]  */
    bcmplpAnCapMSA_200G_CR4_KR4 = 0x8000,   /* AN_CAP_MSA_200G_CR4_KR4[Consortium]  */
} bcm_plp_tech_ability_ext_t;


typedef enum bcm_plp_pause_ability_s {
    bcmplpNoPause = 0,          /** No Pause */
    bcmplpSymmPause = 0x40,     /** Symmetric Pause */
    bcmplpAsymPause = 0x80,     /** Asymmetric Pause */
    bcmplpAsymSymmPause = 0x100 /** Asymmetric and Symmetric Pause */
} bcm_plp_pause_ability_t;

typedef enum bcm_plp_fec_ability_s {
    bcmplpHardwareDefault = 0,         /** Hardware default */
    bcmplpFecAbility = 0x1,            /** FEC Ability/Capable */
    bcmplpFecRequested = 0x2,          /** FEC requested */
    bcmplpFec91RSFecAbility = 0x4,     /** FEC 91/RS FEC Ability/Capable */
    bcmplpFec91RSFecRequested = 0x8,   /** FEC 91/RS FEC requested */
    bcmplpFecRS528 = 0x10,             /** RS FEC 528 */
    bcmplpFecRS544 = 0x20,             /** RS FEC 544 */
    bcmplpFecRS272 = 0x40,             /** RS FEC 272 */
    bcmplpFecNone = 0x8000             /** NONE or NO Fec */
} bcm_plp_fec_ability_t;

typedef struct bcm_plp_autoneg_ability_s {
   bcm_plp_tech_ability_t tech_ability; /**< This indicates the CL73 tech ability. For instance, CL73 40G KR4 */
   bcm_plp_fec_ability_t fec_ability; /**< Represent FEC ability: Whether to enable FEC or not */
   bcm_plp_pause_ability_t pause_ability; /**< Pause ability */
   unsigned int an_training; /**< Whether CL72/CL93/802.3.CD should be turned on when system is in AN mode.
                                1 - enable, 0 - disable \n
                                User needs to construct "enable" as follows\n
                                bit 3:0 represents enable(1)/disable(0) training\n
                                bit 7:4 represents training frame size, used only in case of PAM4\n
                                  0 : 4K Frame\n
                                  1 : 16K Frame \n
                                bit 11:8 represents training init condition applicable for both NRZ and PAM4,\n
                                  0 : No link training init condition \n
                                  1 : Link training init condition enable\n
                                bit 15:12 represents preset type for NRZ mode,\n
                                  0: No link training Preset Normal: normal operation\n
                                  1: Link training Preset Enable: Preset coefficients \n
                                bit 19:16 represents restart,\n
                                  0: No restart\n
                                  1: restart training\n
                                bit 31:20 : Reserved for future use  */
   unsigned int an_master_lane; /**< Master lane belongs to port. for 4 lane port [0-3], for 2 lane port [0-1]. This paramter is ignored for single lane port */
} bcm_plp_autoneg_ability_t;

typedef struct bcm_plp_autoneg_control_s {
    unsigned int flags;   /** Flags usage will be extended in future as and when required */
    unsigned int enable;  /** Autoneg enable */
} bcm_plp_autoneg_control_t;

typedef struct bcm_plp_autoneg_status_s {
    unsigned int enabled;        /** Autoneg enable status */
    unsigned int an_status;      /** Autoneg status */
    unsigned int an_data_rate;   /** Auto Negotiated data rate */
    unsigned int an_neg_fec;     /** Auto Negotiated Fec */
} bcm_plp_autoneg_status_t;

typedef enum bcm_plp_barchetta_lane_data_rate_e {
    bcmpLplaneDataRateNone        = (0),
    bcmpLplaneDataRate_1P0625G    = (1062),
    bcmpLplaneDataRate_1P25G      = (1250),
    bcmpLplaneDataRate_2P125G     = (2125),
    bcmpLplaneDataRate_2P4576G    = (2457),
    bcmpLplaneDataRate_4P25G      = (4250),
    bcmpLplaneDataRate_4P9152G    = (4915),
    bcmpLplaneDataRate_6P144G     = (6144),
    bcmpLplaneDataRate_6P25G      = (6250),
    bcmpLplaneDataRate_7P5G       = (7500),
    bcmpLplaneDataRate_8P5G       = (8500),
    bcmpLplaneDataRate_9P95328G   = (9953),
    bcmpLplaneDataRate_9P8304G    = (9830),
    bcmpLplaneDataRate_10P1376G   = (10137),
    bcmpLplaneDataRate_10P3125G   = (10312),
    bcmpLplaneDataRate_10P51875G  = (10518),
    bcmpLplaneDataRate_10P52581G  = (10525),
    bcmpLplaneDataRate_10P70922G  = (10709),
    bcmpLplaneDataRate_10P75460G  = (10754),
    bcmpLplaneDataRate_10P75500G  = (10755),
    bcmpLplaneDataRate_10P9375G   = (10937),
    bcmpLplaneDataRate_11P04908G  = (11049),
    bcmpLplaneDataRate_11P09568G  = (11095),
    bcmpLplaneDataRate_11P09592G  = (11095),
    bcmpLplaneDataRate_11P181G    = (11181),
    bcmpLplaneDataRate_11P197G    = (11197),
    bcmpLplaneDataRate_11P25G     = (11250),
    bcmpLplaneDataRate_11P5G      = (11500),
    bcmpLplaneDataRate_12P16512G  = (12165),
    bcmpLplaneDataRate_12P5G      = (12500),
    bcmpLplaneDataRate_14P025G    = (14025),
    bcmpLplaneDataRate_15G        = (15000),
    bcmpLplaneDataRate_20P625G    = (20625),
    bcmpLplaneDataRate_21P875G    = (21875),
    bcmpLplaneDataRate_22P5G      = (22500),
    bcmpLplaneDataRate_23G        = (23000),
    bcmpLplaneDataRate_24P33024G  = (24330),
    bcmpLplaneDataRate_25G        = (25000),
    bcmpLplaneDataRate_25P6608G   = (25660),
    bcmpLplaneDataRate_25P78125G  = (25781),
    bcmpLplaneDataRate_26P25G     = (26250),
    bcmpLplaneDataRate_26P5625G   = (26562),
    bcmpLplaneDataRate_27P34375G  = (27343),
    bcmpLplaneDataRate_27P85G     = (27850),
    bcmpLplaneDataRate_27P9525G   = (27952),
    bcmpLplaneDataRate_28P05G     = (28050),
    bcmpLplaneDataRate_28P125G    = (28125),
    bcmpLplaneDataRate_32P5G      = (32500),
    bcmpLplaneDataRate_33P75G     = (33750),
    bcmpLplaneDataRate_46P25G     = (46250),
    bcmpLplaneDataRate_50G        = (50000),
    bcmpLplaneDataRate_51P5625G   = (51562),
    bcmpLplaneDataRate_53P125G    = (53125),
    bcmpLplaneDataRate_56P1G      = (56100),
    bcmpLplaneDataRate_56P25G     = (56250)

} bcm_plp_barchetta_lane_data_rate_t ;

typedef enum bcm_plp_barchetta_tx_driver_mode_e {
    bcmplptxdriveHWdefaults = 0,  /* Tx drive HW defaults*/
    bcmplptxdrive0P8Volt = 1,     /* Tx drive .8V*/
    bcmplptxdrive1P2Volt = 2      /* tx driver as 1.2V*/

}bcm_plp_barchetta_tx_driver_mode_t;

/*! Barchetta device aux mode
*
*  This structure can be used to set up multiple modes that barchetta supports. \n
*  This structure is used in bcm_plp_mode_config_set/bcm_plp_mode_config_get APIs. \n
*  Gearbox or Inverse or Passthrough modes are created based on lane_map combinations
*  on respective sides using bcm_plp_mode_config_set API.
*
* \arg lane_data_rate \n
*   Barchetta supports multiple lane data rates on System and Line side based on
*   FEC modes selected (bcm_plp_fec_mode_sel_t). \n
*   Per lane data rate of the port. Ex: For 56.25G. lane_data_rate = bcmpLplaneDataRate_56P25G \n
*
*   When port is created of more than one lane; port speed is same as PCS/MAC port speed
*   and lane data rate will be different based on number of lanes and FEC operations. \n
*   Please refer the chip specific data sheet for supported modes \n
*
* \arg modulation_mode \n
*   Type of modulation used to configure the port. Modulation mode can be PAM4 or NRZ.\n
*   This enum bcm_plp_modulation_mode_t has supported modes. \n
*
* \arg clock_mode \n
*   Type of clock mode used for clock configuration.\n
*   clock_mode = 0, Represents Recovered clock(Lane 0 is used as clock source). \n
*   clock_mode = 1, Represents Reference clock.\n
*
* \arg ll_mode \n
*   Low latency mode. \n
*   ll_mode.BIT[0] = 0, Represents Ingress low latency disabled. \n
*   ll_mode.BIT[0] = 1, Represents Ingress low latency enabled. Set this for all PAM4 and 1G mode. \n
*   ll_mode.BIT[1] = 0, Represents Egress low latency disabled
*   ll_mode.BIT[1] = 1, Represents Egress low latency enabled. Set this for all PAM4 and 1G mode. \n
*
* \arg failover_lane_map \n
*   Lane map of the secondary port in case of fail over mode.\n
*   In case of RETIMER, GB and RGB modes the failover_lane_map should be zero for both sides(Line and System). \n
*   In case of fail over mode the failover_lane_map should be non-zero in either of the sides(Line or System) .\n
*
* \arg phy_mode \n
*   Mode of the PHY .\n
*   0 : General PHY
*   1 : Local PHY
*   2 : Remote PHY
*
* \arg tx_driver_mode \n
* Configures tx to drive high voltage.\n
*    0 : bcmplptxdriveHWdefaults
*    1 : bcmplptxdrive0P8Volt
*    2 : bcmplptxdrive1P2Volt
*
*
*  Not all the combinations are supported. For supported combinations; please refer to product specific data sheet\n
*/
typedef struct bcm_plp_barchetta_device_aux_modes_s {
    bcm_plp_barchetta_lane_data_rate_t  lane_data_rate    ;
    bcm_plp_modulation_mode_t           modulation_mode   ;
    unsigned char                       clock_mode        ;
    unsigned char                       ll_mode           ;
    unsigned int                        failover_lane_map ;
    unsigned int                        phy_mode          ;
    bcm_plp_barchetta_tx_driver_mode_t  tx_driver_mode;
} bcm_plp_barchetta_device_aux_modes_t;

/*! Estoque device aux mode
 *
 *  This structure can be used to set up multiple modes that estoque supports. \n
 *  This structure is used in bcm_plp_mode_config_set/bcm_plp_mode_config_get APIs. \n
 *  Gearbox or Inverse or Passthrough modes are created based on lane_map combinations
 *  on respective sides using bcm_plp_mode_config_set API.
 *
 * \arg lane_data_rate \n
 *   estoque supports multiple lane data rates on System and Line side based on
 *   FEC modes selected (bcm_plp_fec_mode_sel_t). \n
 *   Per lane data rate of the port. Ex: For 53G. lane_data_rate = 53000 \n
 *
 *   When port is created of more than one lane; port speed is same as PCS/MAC port speed
 *   and lane data rate will be different based on number of lanes and FEC operations. \n
 *   Please refer the chip specific data sheet for supported modes \n
 *
 * \arg modulation_mode \n
 *   Type of modulation used to configure the port. Modulation mode can be PAM4 or NRZ.\n
 *   This enum bcm_plp_modulation_mode_t has supported modes. \n
 *
 * \arg fec_mode_sel \n
 *   Fec mode used to configure the port. This enum bcm_plp_fec_mode_sel_t has supported mode. \n
 *   User can choose different type of FECs on System/Line side of Estoque based on the system requirements \n
 *
 * \arg an_ability \n
 *   Auto Negotiation ability. Used to set the Tech Ability, Fec Ability and etc\n
 *
 * \arg an_control \n
 *   Auto Negotiation enable and disable, This param is ignored for bcm_plp_mode_config_get API\n
 *
 * \arg an_status \n
 *   Auto Negotiation status. This param is ignored for bcm_plp_mode_config_ret API \n
 *     AN_DISABLE            = 0x0, \n
 *     AN_ENABLE             = 0x1, \n
 *     TRANSMIT_DISABLE      = 0x2, \n
 *     ABILITY_DETECT        = 0x4, \n
 *     ACKNOWLEDGE_DETECT    = 0x8, \n
 *     COMPLETE_ACKNOWLEDGE  = 0x10, \n
 *     NEXT_PAGE_WAIT        = 0x20, \n
 *     AN_GOOD_CHECK         = 0x40, \n
 *     AN_GOOD               = 0x80 \n
 *
 *  Auto Negotiated Fec
 *     NoFec                 = 0x0\n
 *     BaseRFec              = 0x1\n
 *     BaseRSFec             = 0x2\n
 *
 *  Not all the combinations are supported. For supported combinations; please refer to product specific data sheet\n
 */

typedef struct bcm_plp_estoque_device_aux_modes_s {
    unsigned int lane_data_rate;
    bcm_plp_modulation_mode_t modulation_mode;
    bcm_plp_fec_mode_sel_t fec_mode_sel;
    bcm_plp_autoneg_ability_t *an_ability;
    bcm_plp_autoneg_control_t *an_control;
    bcm_plp_autoneg_status_t *an_status;
} bcm_plp_estoque_device_aux_modes_t;
typedef bcm_plp_estoque_device_aux_modes_t bcm_plp_estoquej_device_aux_modes_t;


typedef enum bcm_plp_lane_data_rate_e {
    bcmplpLaneDataRateNone        = (0),
    bcmplpLaneDataRate_1P0625G    = (1062),
    bcmplpLaneDataRate_1P25G      = (1250),
    bcmplpLaneDataRate_1P5625G    = (1562),
    bcmplpLaneDataRate_2P125G     = (2125),
    bcmplpLaneDataRate_2P4576G    = (2457),
    bcmplpLaneDataRate_3P072G     = (3072),
    bcmplpLaneDataRate_3P125G     = (3125),
    bcmplpLaneDataRate_4P25G      = (4250),
    bcmplpLaneDataRate_4P9152G    = (4915),
    bcmplpLaneDataRate_6P144G     = (6144),
    bcmplpLaneDataRate_6P25G      = (6250),
    bcmplpLaneDataRate_7P5G       = (7500),
    bcmplpLaneDataRate_8P11G      = (8110),
    bcmplpLaneDataRate_8P5G       = (8500),
    bcmplpLaneDataRate_9P800G     = (9800),
    bcmplpLaneDataRate_9P8304G    = (9830),
    bcmplpLaneDataRate_9P95328G   = (9953),
    bcmplpLaneDataRate_10P1376G   = (10137),
    bcmplpLaneDataRate_10P3125G   = (10312),
    bcmplpLaneDataRate_10P51875G  = (10518),
    bcmplpLaneDataRate_10P52581G  = (10525),
    bcmplpLaneDataRate_10P70922G  = (10709),
    bcmplpLaneDataRate_10P75460G  = (10754),
    bcmplpLaneDataRate_10P75500G  = (10755),
    bcmplpLaneDataRate_10P9375G   = (10937),
    bcmplpLaneDataRate_11P04908G  = (11049),
    bcmplpLaneDataRate_11P09568G  = (11095),
    bcmplpLaneDataRate_11P09592G  = (11095),
    bcmplpLaneDataRate_11P181G    = (11181),
    bcmplpLaneDataRate_11P197G    = (11197),
    bcmplpLaneDataRate_11P25G     = (11250),
    bcmplpLaneDataRate_11P5G      = (11500),
    bcmplpLaneDataRate_12P16512G  = (12165),
    bcmplpLaneDataRate_12P5G      = (12500),
    bcmplpLaneDataRate_14P025G    = (14025),
    bcmplpLaneDataRate_15G        = (15000),
    bcmplpLaneDataRate_20P625G    = (20625),
    bcmplpLaneDataRate_21P875G    = (21875),
    bcmplpLaneDataRate_22P5G      = (22500),
    bcmplpLaneDataRate_23G        = (23000),
    bcmplpLaneDataRate_24P33024G  = (24330),
    bcmplpLaneDataRate_25G        = (25000),
    bcmplpLaneDataRate_25P6608G   = (25660),
    bcmplpLaneDataRate_25P78125G  = (25781),
    bcmplpLaneDataRate_26P25G     = (26250),
    bcmplpLaneDataRate_26P5625G   = (26562),
    bcmplpLaneDataRate_27P076G    = (27076),
    bcmplpLaneDataRate_27P34375G  = (27343),
    bcmplpLaneDataRate_27P75G     = (27750),
    bcmplpLaneDataRate_27P8125G   = (27812),
    bcmplpLaneDataRate_27P85G     = (27850),
    bcmplpLaneDataRate_27P85125G  = (27851),
    bcmplpLaneDataRate_27P9512G   = (27951),
    bcmplpLaneDataRate_27P9525G   = (27952),
    bcmplpLaneDataRate_28G        = (28000),
    bcmplpLaneDataRate_28P05G     = (28050),
    bcmplpLaneDataRate_28P0762G   = (28076),
    bcmplpLaneDataRate_28P125G    = (28125),
    bcmplpLaneDataRate_32P5G      = (32500),
    bcmplpLaneDataRate_33P75G     = (33750),
    bcmplpLaneDataRate_46P25G     = (46250),
    bcmplpLaneDataRate_50G        = (50000),
    bcmplpLaneDataRate_51P5625G   = (51562),
    bcmplpLaneDataRate_53P125G    = (53125),
    bcmplpLaneDataRate_53P6585G   = (53658),
    bcmplpLaneDataRate_54G        = (54000),
    bcmplpLaneDataRate_55P5G      = (55500),
    bcmplpLaneDataRate_55P625G    = (55625),
    bcmplpLaneDataRate_55P9025G   = (55902),
    bcmplpLaneDataRate_55P9049G   = (55904),
    bcmplpLaneDataRate_55P90474G  = (55905),
    bcmplpLaneDataRate_56G        = (56000),
    bcmplpLaneDataRate_56P1G      = (56100),
    bcmplpLaneDataRate_56P1523G   = (56152),
    bcmplpLaneDataRate_56P25G     = (56250),
    bcmplpLaneDataRate_57G        = (57000),
    bcmplpLaneDataRate_106P25G    = (106250),
    bcmplpLaneDataRate_112P2G     = (112200),
    bcmplpLaneDataRate_112P5G     = (112500)
} bcm_plp_lane_data_rate_t;

/**
 * The enumeration of FEC Encoder per lane
 */
typedef enum bcm_plp_fec_enc_type_e {
    bcmplpFecEncTypeDefault, /* FEC Encoder Default */
    bcmplpFecEncTypeBypass,  /* Enable FEC Encoder Bypass */
    bcmplpFecEncTypeENC,     /* Enable FEC Encoder */
    bcmplpFecEncTypeXENC,    /* Enable FEC Transencoder */
    bcmplpFecEncTypeXENC_ENC /* Enable FEC Transencoder and Encoder */
} bcm_plp_fec_enc_type_t;


/**
 * The enumeration of FEC Decoder per lane
 */
typedef enum bcm_plp_fec_dec_type_e {
    bcmplpFecDecTypeDefault,  /* FEC Decoder Default */
    bcmplpFecDecTypeBypass,   /* Enable FEC Decoder Bypass */
    bcmplpFecDecTypeDEC,      /* Enable FEC Decoder */
    bcmplpFecDecTypeXDEC,     /* Enable FEC Transdecoder */
    bcmplpFecDecTypeDEC_XDEC  /* Enable FEC Decoder and Transdecoder */
} bcm_plp_fec_dec_type_t;

/*! Millenio and Milleniob aux mode
 *
 *  This structure can be used to set up multiple modes that millenio supports. \n
 *  This structure is used in bcm_plp_mode_config_set/bcm_plp_mode_config_get APIs. \n
 *  Gearbox or Inverse or Passthrough modes are created based on lane_map combinations
 *  on respective sides using bcm_plp_mode_config_set API.
 *
 * \arg lane_data_rate \n
 *   Millenio supports multiple lane data rates on System and Line side based on
 *   FEC modes selected (bcm_plp_fec_mode_sel_t). \n
 *
 *   When port is created of more than one lane; port speed is same as PCS/MAC port speed
 *   and lane data rate will be different based on number of lanes and FEC operations. \n
 *   Please refer the chip specific data sheet for supported modes \n
 *
 * \arg modulation_mode \n
 *   Type of modulation used to configure the port. Modulation mode can be PAM4 or NRZ.\n
 *   This enum bcm_plp_modulation_mode_t has supported modes. \n
 *
 * \arg fec_mode_sel \n
 *   Fec mode used to configure the port. This enum bcm_plp_fec_mode_sel_t has supported mode. \n
 *   User can choose different type of FECs on System/Line side of millenio based on the system requirements \n
 *
 * \arg hitless_mux_mode \n
 *   To select the hitless mux mode feature on the chip  0: non-hitless mux mode, 1: Hitless mux mode,\n
 *       2: Hybrid hitless mux mode, 3: PMD hitless mux mode \n
 *
 * \arg hybrid_port_mode \n
 *   To select hybrid port mode for dual-die and non-contagious lanemap \n
 *
 */

typedef struct bcm_plp_millenio_device_aux_modes_s {
    bcm_plp_lane_data_rate_t lane_data_rate;
    bcm_plp_modulation_mode_t modulation_mode;
    bcm_plp_fec_mode_sel_t fec_mode_sel;
    unsigned char hitless_mux_mode;
    unsigned char hybrid_port_mode;
} bcm_plp_millenio_device_aux_modes_t;
typedef bcm_plp_millenio_device_aux_modes_t bcm_plp_milleniob_device_aux_modes_t;

/* Millenio and Milleniob FW init parameters set along with FW download */
typedef struct bcm_plp_millenio_fw_init_params_s {
    unsigned char avs_enable; /* Enable=1 / Disable=0 AVS */
    unsigned char phy_mode_reverse; /* Enable=1 / Disable=0 */
} bcm_plp_millenio_fw_init_params_t;

typedef bcm_plp_millenio_fw_init_params_t bcm_plp_milleniob_fw_init_params_t;


/*! Barchetta2 aux mode
 *
 *  This structure can be used to set up multiple modes that barchetta2 supports. \n
 *  This structure is used in bcm_plp_mode_config_set/bcm_plp_mode_config_get APIs. \n
 *  Gearbox or Inverse or Passthrough modes are created based on lane_map combinations
 *  on respective sides using bcm_plp_mode_config_set API.
 *
 * \arg lane_data_rate \n
 *   Barchetta2 supports multiple lane data rates on System and Line side based on
 *   FEC modes selected (bcm_plp_fec_mode_sel_t). \n
 *
 *   When port is created of more than one lane; port speed is same as PCS/MAC port speed
 *   and lane data rate will be different based on number of lanes and FEC operations. \n
 *   Please refer the chip specific data sheet for supported modes \n
 *
 * \arg modulation_mode \n
 *   Type of modulation used to configure the port. Modulation mode can be PAM4 or NRZ.\n
 *   This enum bcm_plp_modulation_mode_t has supported modes. \n
 *
 * \arg fec_mode_sel \n
 *   Fec mode used to configure the port. This enum bcm_plp_fec_mode_sel_t has supported mode. \n
 *   User can choose different type of FECs on System/Line side of Barchetta2 based on the system requirements. \n
 *
 * \arg hitless_mux_mode \n
 *   Select the hitless mux mode feature on the chip. \n
 *
 * \arg hybrid_port_mode \n
 *   Select hybrid port mode for dual-die and non-contagious lanemap. \n
 *
 * \arg fec_enc \n
 *   Select the FEC Encoder feature per lane. \n
 *
 * \arg fec_dec \n
 *   Select the FEC Decoder feature per lane. \n
 */

typedef struct bcm_plp_barchetta2_device_aux_modes_s {
    bcm_plp_lane_data_rate_t lane_data_rate;
    bcm_plp_modulation_mode_t modulation_mode;
    bcm_plp_fec_mode_sel_t fec_mode_sel;
    unsigned char hitless_mux_mode;
    unsigned char hybrid_port_mode;
    bcm_plp_fec_enc_type_t fec_enc;
    bcm_plp_fec_dec_type_t fec_dec;
} bcm_plp_barchetta2_device_aux_modes_t;

/*!bcm_plp_evora_fw_init_params_t
  * This structure is intended to initialize HW configuration after FW download
  * \arg tx_drv_supply\n
  * Forced value for tx driver supply.
  * 1 : TVDD1P25 supply is set to 1.0 V
  * 0 : TVDD1P25 supply is set to 1.25 V
  *
  */
typedef struct bcm_plp_evora_fw_init_params_s {
    unsigned int  tx_drv_supply;
} bcm_plp_evora_fw_init_params_t;

/*! Evora device aux mode
 *
 *  This structure can be used to set up low latency mode for evora. \n
 *  This structure is used in mode config APIs. \n
 *
 * \arg low_latency_variation \n
 *  1 : Applies optimized settings in Evora which will give the lowest \n
 *      latency variation for PTP application \n
 *
 */
typedef struct bcm_plp_evora_device_aux_modes_s {
    unsigned int low_latency_variation;
} bcm_plp_evora_device_aux_modes_t;


/*!bcm_plp_europa_fw_init_params_t
  * This structure is intended to initialize HW configuration after FW download
  * \arg tx_drv_supply\n
  * Forced value for tx driver supply.
  * 1 : TVDD1P25 supply is set to 1.0 V
  * 0 : TVDD1P25 supply is set to 1.25 V
  *
  */
typedef struct bcm_plp_europa_fw_init_params_s {
    unsigned int  tx_drv_supply;
} bcm_plp_europa_fw_init_params_t;

/*! Europa device aux mode
 *
 *  This structure can be used to set up low latency mode for europa. \n
 *  This structure is used in mode config APIs. \n
 *
 * \arg low_latency_variation \n
 *  1 : Applies optimized settings in Europa which will give the lowest \n
 *      latency variation for PTP application \n
 *
 */
typedef struct bcm_plp_europa_device_aux_modes_s {
    unsigned int low_latency_variation;
} bcm_plp_europa_device_aux_modes_t;

/*! Aperta Failover config
 *
 * \arg lane_map \n
 *  Lane map of the failover port
 *
 * \arg mux_location \n
 *  This parameter indicates whether mux needs be enabled before or after MACSEC,
 *  0 : After MACSEC
 *  1 : Before MACSEC
 *         
 */
typedef struct bcm_plp_aperta_failover_config_s {
    unsigned int lane_map;
    unsigned int mux_location;
}bcm_plp_aperta_failover_config_t;

/*! Aperta Fixed latency configuration
 *
 * This structure can be used to enable/disable fixed latency in aperta. 
 * It is also used to configure dp_ck_cycles
 *
 * \arg enable \n
 *     When set to "1" it enables the fixed latency, "0" Disables
 *
 * \arg igr_dp_ck_cycles \n
 *     This defines the number of dp_ck clock cycles that a packet
 *     should take from entering PM2MS interface to leaving MS2PM interface.
 *
 * \arg egr_dp_ck_cycles \n
 *     This defines the number of dp_ck clock cycles that a packet
 *     should take from entering MS2PM interface to leaving PM2MS interface.
 *
 *
 */
typedef struct bcm_plp_aperta_fixed_latency_config_s {
    unsigned int enable;
    unsigned int igr_dp_ck_cycles;
    unsigned int egr_dp_ck_cycles;
}bcm_plp_aperta_fixed_latency_config_t;

typedef enum bcm_plp_port_type_e {
    bcmplpPortTypePassthrough = 0,  /* Passthrough Port (one to one map between system and line sides)*/
    bcmplpPortTypeGearBox,          /* Gearbox port*/
    bcmplpPortTypeReverseGearBox    /* Reverse Gearbox port*/
}bcm_plp_port_type_t;

typedef enum bcm_plp_1588_config_e {
    bcmplpPTPDisabled = 0,
    bcmplpPTPNseEnabled, /*NSE TS for egress and Ingress*/
    bcmplpPTPNsePmEnabled, /*NSE TS for egress and PM-TS for Ingress*/
    bcmplpPTPPmEnabled, /*PM TS for egress and PM-TS for Ingress*/
    bcmplpPTPPmNseEnabled, /*PM TS for egress and NSE-TS for Ingress*/
    /* Value 5-7 is reserved for future use*/
    bcmplpPTPNse2SEnabled = 8,  /* NSE-TS (B0 Plan A) for Egress & Ingress (packets encrypted) - 2 Stage*/
    bcmplpPTPNsePm2SEnabled    /* NSE-TS (B0 Plan A) for Egress & PM-TS for Ingress (packets encrypted) - 2 Stage*/
} bcm_plp_1588_config_t;

typedef enum bcm_plp_aperta_fec_mode_select_e {
    bcmplpapertaNoFEC = 0,
    bcmplpapertaRS544,
    bcmplpapertaBaseR,
    bcmplpapertaRSFEC,
    bcmplpapertaRS272,
    bcmplpapertaRS544_2XN
} bcm_plp_aperta_fec_mode_select_t;

/*! Aperta device aux mode
*
*  This structure can be used to set up multiple modes that aperta supports. \n
*  This structure is used in bcm_plp_mode_config_set/bcm_plp_mode_config_get APIs. \n
*  Gearbox or Inverse or Passthrough modes are created based on lane_map combinations
*  on respective sides using bcm_plp_mode_config_set API.
*
* \arg lane_data_rate \n
*   Aperta supports multiple lane data rates on System and Line side based on
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
*  \arg egr_ptp_fixed_latency \n
*     This defines the number of dp_ck clock cycles. Applicable only for chip revision B0.
*
*  \arg port_type \n
*     This defines the type of a port (Passthrough, Gearbox, Reverse Gearbox).
*
*  Not all the combinations are supported. For supported combinations; please refer to product specific data sheet\n
*/
typedef struct bcm_plp_aperta_device_aux_modes_s {
    bcm_plp_lane_data_rate_t                lane_data_rate;
    bcm_plp_modulation_mode_t               modulation_mode;
    bcm_plp_aperta_fec_mode_select_t        fec_mode_sel;
    bcm_plp_aperta_failover_config_t        failover_config;
    bcm_plp_aperta_fixed_latency_config_t   fixed_latency_config;
    bcm_plp_1588_config_t                   ts_config;
    int                                     egr_ptp_fixed_latency;
    bcm_plp_port_type_t                     port_type;
} bcm_plp_aperta_device_aux_modes_t;

/*! Agera device aux mode
*
*  This structure can be used to set up multiple modes that agera supports. \n
*  This structure is used in bcm_plp_mode_config_set/bcm_plp_mode_config_get APIs.
*
* \arg lane_data_rate \n
*   Agera supports multiple lane data rates on System and Line side.
*   Per lane data rate of the port. Ex: For 56.25G. lane_data_rate = bcmpLplaneDataRate_56P25G \n
*
*   When port is created of more than one lane; port speed is same as PCS/MAC port speed
*   and lane data rate will be different based on number of lanes. \n
*   Please refer the chip specific data sheet for supported modes \n
*
* \arg modulation_mode \n
*   Type of modulation used to configure the port. Modulation mode can be PAM4 or NRZ.\n
*   This enum bcm_plp_modulation_mode_t has supported modes. \n
*
* \arg failover_lane_map \n
*   Lane map of the secondary port in case of fail over mode.\n
*   In case of RETIMER modes the failover_lane_map should be zero for both sides(Line and System). \n
*   In case of FAILOVER modes the failover_lane_map should be non-zero in either of the sides(Line or System) .\n
*
* \arg clock_mode \n
*   Type of clock mode used for clock configuration.\n
*   clock_mode = 0, Represents Recovered clock(Lane 0 is used as clock source). \n
*   clock_mode = 1, Represents Reference clock.\n
*
*  For supported lane_data_rate and modulation_mode, please refer to product specific data sheet\n
*/
typedef struct bcm_plp_agera_device_aux_modes_s {
    bcm_plp_lane_data_rate_t   lane_data_rate;
    bcm_plp_modulation_mode_t  modulation_mode;
    unsigned int               failover_lane_map;
    unsigned char              clock_mode;
} bcm_plp_agera_device_aux_modes_t;

/*! Ageralite device aux mode
*
*  This structure can be used to set up multiple modes that ageralite supports. \n
*  This structure is used in bcm_plp_mode_config_set/bcm_plp_mode_config_get APIs.
*
* \arg lane_data_rate \n
*   Ageralite supports multiple lane data rates on System and Line side.
*   Per lane data rate of the port. Ex: For 53.125G. lane_data_rate = bcmplpLaneDataRate_53P125G \n
*
*   When port is created of more than one lane; port speed is same as PCS/MAC port speed
*   and lane data rate will be different based on number of lanes. \n
*   Please refer the chip specific data sheet for supported modes \n
*
* \arg modulation_mode \n
*   Type of modulation used to configure the port. Modulation mode can be PAM4 or NRZ.\n
*   This enum bcm_plp_modulation_mode_t has supported modes. \n
*
* \arg clock_mode \n
*   Type of clock mode used for clock configuration.\n
*   clock_mode = 0, Represents Recovered clock(Lane 0 is used as clock source). \n
*   clock_mode = 1, Represents Reference clock.\n
*
*  For supported lane_data_rate and modulation_mode, please refer to product specific data sheet\n
*/
typedef struct bcm_plp_ageralite_device_aux_modes_s {
	bcm_plp_lane_data_rate_t   lane_data_rate;
	bcm_plp_modulation_mode_t  modulation_mode;
	unsigned char              clock_mode;
} bcm_plp_ageralite_device_aux_modes_t;

typedef struct bcm_plp_device_aux_modes_s {
    unsigned int pass_thru;
} bcm_plp_device_aux_modes_t;

/** TXFIR Tap Enable Enum */
typedef enum bcm_plp_serdes_tx_tap_mode_e {
    bcmplpTapModeNRZ_LP_3TAP = 1,
    bcmplpTapModeNRZ_6TAP,
    bcmplpTapModePAM4_LP_3TAP,
    bcmplpTapModePAM4_6TAP
} bcm_plp_serdes_tx_tap_mode_t;

/** Independent mode, TX PRECODE option */
typedef enum bcm_plp_indep_tx_precode_e {
    bcmplpIndepTxPrecodeDefault = 0,
    bcmplpIndepTxPrecodeOff,
    bcmplpIndepTxPrecodeOn
} bcm_plp_indep_tx_precode_t;

typedef struct bcm_plp_tx_s {
    char pre;
    char main;
    char post;
    char post2;
    char post3;
    char amp;
    short tx_hpf;
    short serdes_speed;
}bcm_plp_tx_t;

typedef struct bcm_plp_pam4_tx_s {
    int pre;  /** Pretap value */
    int main; /** Maintap value */
    int post; /** Posttap value */
    int post2; /** Post2tap value */
    int post3; /** Post3tap value */
    int amp; /** current value */
    int drivermode; /** drivermode value */
    int pre2; /** Pre2tap value */
    bcm_plp_serdes_tx_tap_mode_t serdes_tx_tap_mode; /*Number of elements need to be selected based on chip mode */
    bcm_plp_indep_tx_precode_t tx_precode; /* Independent mode, TX PRECODE option */
    int pre3; /** Pre3tap value */
    int tx_symbol_swap;/** Tx symbol_swap for PAM4 mode: 0- No swap  1- Swap   */
    int tx_graycode;   /** Tx gray code enable for PAM4 mode: 0- Disable; 1- Enable */
}bcm_plp_pam4_tx_t;

typedef struct bcm_plp_pm_diag_slicer_offset_s {
    unsigned int offset_pe;
    unsigned int offset_ze;
    unsigned int offset_me;
    unsigned int offset_po;
    unsigned int offset_zo;
    unsigned int offset_mo;
} bcm_plp_pm_diag_slicer_offset_t;

typedef struct bcm_plp_pm_diag_eyescan_s {
    unsigned int heye_left;
    unsigned int heye_right;
    unsigned int veye_upper;
    unsigned int veye_lower;
} bcm_plp_pm_diag_eyescan_t;

typedef struct bcm_plp_pm_phy_diagnostics_s {
    unsigned int signal_detect;
    unsigned int vga_bias_reduced;
    unsigned int postc_metric;
    bcm_plp_pm_osr_mode_t osr_mode;
    bcm_plp_pm_pmd_mode_t pmd_mode;
    unsigned int rx_lock;
    unsigned int rx_ppm;
    unsigned int tx_ppm;
    unsigned int clk90_offset;
    unsigned int clkp1_offset;
    unsigned int p1_lvl;
    unsigned int m1_lvl;
    unsigned int dfe1_dcd;
    unsigned int dfe2_dcd;
    unsigned int slicer_target;
    bcm_plp_pm_diag_slicer_offset_t slicer_offset;
    bcm_plp_pm_diag_eyescan_t eyescan;
    unsigned int state_machine_status;
    unsigned int link_time; /* Added as required by Falcon core */
    int pf_main;
    int pf_hiz;
    int pf_bst;
    int pf_low;
    int pf2_ctrl;
    int vga;
    int dc_offset;
    int p1_lvl_ctrl;
    int dfe1;
    int dfe2;
    int dfe3;
    int dfe4;
    int dfe5;
    int dfe6;
    int txfir_pre;
    int txfir_main;
    int txfir_post1;
    int txfir_post2;
    int txfir_post3;
    int tx_amp_ctrl;
    unsigned char br_pd_en;
    unsigned int state_mfg_diag_inst; /* Specfic device block */
    int state_mfg_diag_op_type; /* Operation type */
    int state_mfg_diag_op_cmd; /* Operation type */
    void* state_mfg_diag_arg; /* Argument pointer */
    int txfir_pre2;
    int pf3_ctrl;
    int ffe_enable;
    int ffe1;
    int ffe2;
    unsigned char lane_reset_state;
    unsigned char tx_reset_state;
    unsigned char uc_stop_state;
    unsigned char ucv_lane_status;
    unsigned char tx_pll_select;
    unsigned char rx_pll_select;
    int  pcs_sync;
    int  pcs_blk_lock;
    int  pcs_rx_fault;
    int  pcs_tx_fault;
    int  pmd_rx_fault;
    int  pmd_tx_fault;
    int  hi_ber;
    int  ber_cnt;
    int  delay_skew;
} bcm_plp_pm_phy_diagnostics_t;

typedef struct bcm_plp_pm_phy_pam4_diagnostics_s {
    unsigned int signal_detect;              /* Signal detect of lane*/
    unsigned int vga_bias_reduced;
    unsigned int postc_metric;
    bcm_plp_pm_osr_mode_t osr_mode;          /* Over Sample Rate mode*/
    bcm_plp_pm_pmd_mode_t pmd_mode;
    unsigned int rx_lock;                    /* PMD Rx lock*/
    unsigned int rx_ppm;                     /* Frequency offset of local reference clock with respect to TX data in ppm */
    unsigned int tx_ppm;                     /* Frequency offset of local reference clock with respect to RX data in ppm */
    unsigned int clk90_offset;               /* Delay of zero crossing slicer, m1, wrt to data in PI codes */
    unsigned int clkp1_offset;               /* Delay of diagnostic/lms slicer, p1, wrt to data in PI codes */
    unsigned int p1_lvl;                     /* Vertical threshold voltage of p1 slicer (mV) */
    unsigned int m1_lvl;                     /* Vertical threshold voltage of m1 slicer (mV) */
    unsigned int dfe1_dcd;                   /* DFE tap 1 Duty Cycle Distortion */
    unsigned int dfe2_dcd;                   /* DFE tap 2 Duty Cycle Distortion */
    unsigned int slicer_target;              /* Slicer target*/
    bcm_plp_pm_diag_slicer_offset_t slicer_offset;  /* Slicer calibration control codes*/
    bcm_plp_pm_diag_eyescan_t eyescan;              /*internal diagnostic slicer in mUI and mV*/
    unsigned int state_machine_status;
    unsigned int link_time;                  /*  Link time in milliseconds */
    int pf_main;                             /* Peaking Filter Main Settings */
    int pf_hiz;                              /* Peaking Filter Hiz mode enable */
    int pf_bst;                              /* Peaking Filter DC gain adjustment for CTLE */
    int pf_low;
    int pf2_ctrl;                            /* Low Frequency Peaking filter control */
    int vga;                                 /* Variable gain amplifier setting  */
    int dc_offset;                           /* DC offset DAC control value */
    int p1_lvl_ctrl;                         /* P1 eyediag status */
    int dfe1;                                /* DFE tap 1 value */
    int dfe2;                                /* DFE tap 2 value */
    int dfe3;                                /* DFE tap 3 value */
    int dfe4;                                /* DFE tap 4 value */
    int dfe5;                                /* DFE tap 5 value */
    int dfe6;                                /* DFE tap 6 value */
    int txfir_pre;                           /* TX equalization FIR pre tap weight */
    int txfir_main;                          /* TX equalization FIR main tap weight */
    int txfir_post1;                         /* TX equalization FIR post1 tap weight */
    int txfir_post2;                         /* TX equalization FIR post2 tap weight */
    int txfir_post3;                         /* TX equalization FIR post3 tap weight */
    int tx_amp_ctrl;                         /* TX_AMP **/
    unsigned char br_pd_en;                  /* Baud Rate Phase Detector enable */
    unsigned int state_mfg_diag_inst;        /* Specfic device block */
    int state_mfg_diag_op_type;              /* Operation type */
    int state_mfg_diag_op_cmd;               /* Operation type */
    void* state_mfg_diag_arg;                /* Argument pointer */
    int txfir_pre2;                          /* TX equalization FIR pre2 tap weight */
    int pf3_ctrl;                            /* high Frequency Peaking filter control */
    int ffe_enable;                          /* Feed feedback equalizer enable*/
    int ffe1;                                /* Feed feedback equalizer1*/
    int ffe2;                                /* Feed feedback equalizer2*/
    unsigned char lane_reset_state;          /* Lane reset state*/
    unsigned char tx_reset_state;            /* Tx reset state*/
    unsigned char uc_stop_state;             /* state of UC stop */
    unsigned char ucv_lane_status;           /* state of uc lane status*/
    unsigned char tx_pll_select;             /* Tx Pll select*/
    unsigned char rx_pll_select;             /* Rx Pll select*/
    plp_float_t rx_ppm_real;                 /* Frequency offset of local reference clock with respect to TX data in ppm, Applicable for DSP serdes */
    plp_float_t tx_ppm_real;                 /* Frequency offset of local reference clock with respect to RX data in ppm, applicable for DSP serdes */
    unsigned char modulation;                /* Modulation mode NRZ/PAM4*/
    char dc_offset_array[48];                /* DC offset array*/
    plp_float_t snr;                         /* Signal to noise ratio*/
    plp_float_t slice_historgram;            /* Slice histogram*/
    plp_float_t ffe_real[24];                /* Feed feedback equalizer value*/
    unsigned char tr_skew[8];                /* PMD skew state*/
    unsigned char peaking_filter;            /* Peaking filter*/
    char capture_mem_adc[576];               /* Capture mem for DSP serdes*/
    char capture_mem_eq[576];                /* Capture mem for DSP serdes*/
    unsigned short tr_phase;                 /* Tr phase for DSP serdes*/
    plp_float_t dc_offset_real[48];          /* DC offset real */
    unsigned char dpga_array[48];            /* DPGA Array for DSP serdes */
    plp_float_t dpga_real[48];               /* DPGA Real for DSP serdes */
    char eye_size;                           /* Eye Size */
    int txfir_pre3;                          /* TX equalization FIR pre3 tap weight */
    int fga;                                 /* Fixed gain amplifier setting  */
    unsigned char signal_detect_latch;       /* Signal detect latch status of a lane*/
    unsigned char cdr_lock_status_latch;     /* CDR lock latch status of a lane*/
} bcm_plp_pm_phy_pam4_diagnostics_t;

/*! Core diagnostics
 *
 * \arg temperature \n
 *     Core Temperature in Celcius.\n
 * \arg voltage \n
 *     Core voltage in Volts for applicable PHYs.\n
 * \arg pll_range \n
 *     PLL Range value for applicable PHYs.  Refer to respective user guide for more details.\n
 */

typedef struct bcm_plp_core_diagnostics_s {
    int temperature;
    unsigned int voltage;
    unsigned int pll_range;
} bcm_plp_core_diagnostics_t;

/*! Lane swap configuration using TX/RX map
 *
 * \arg num_of_lanes \n
 *     Number of elements in lane_map_rx/tx arrays \n
 *
 * \arg lane_map_rx \n
 *     lane_map_rx[x]=y means that rx lane x is mapped to rx lane y \n
 *
 * \arg lane_map_tx \n
 *     lane_map_tx[x]=y means that tx lane x is mapped to tx lane y \n
 */
typedef struct bcm_plp_laneswap_map_s {
    /* Number of elements in lane_map_rx/tx arrays */
    unsigned int num_of_lanes;
    union {
        /* MDI pair swap doesn't differentiate Tx/Rx directions,  *\
        |* lanswap_map[] shares the same space with lane_map_rx[] *|
        |* don't care lane_map_tx[] when setting MDI Pair Swap.   *|
        \* lanswap_map[x]=y means that pair x is mapped to pair y */
        unsigned int lanswap_map[MAX_LANES_PER_CORE];
        /* lane_map_rx[x]=y means that rx lane x is mapped to rx lane y */
        unsigned int lane_map_rx[MAX_LANES_PER_CORE];
    };
    /* lane_map_tx[x]=y means that tx lane x is mapped to tx lane y */
    unsigned int lane_map_tx[MAX_LANES_PER_CORE];
} bcm_plp_laneswap_map_t;
typedef bcm_plp_laneswap_map_t bcm_laneswap_map_t;

typedef struct bcm_plp_pm_phy_reset_s {
    bcm_plp_pm_reset_direction_t rx;
    bcm_plp_pm_reset_direction_t tx;
} bcm_plp_pm_phy_reset_t;

typedef struct plp_static_config_s {
    unsigned int ull_dp;
} plp_static_config_t;

typedef struct bcm_plp_phy_static_config_s {
    unsigned int phy_id;
	plp_static_config_t* bcm_static_config;
} bcm_plp_phy_static_config_t;

typedef struct bcm_plp_pm_firmware_lane_config_s {
    bcm_plp_pm_firmware_mode_t firmware_mode;
    unsigned int ena_dis;
    unsigned int LaneConfigFromPCS;
    unsigned int AnEnabled; /**< Autoneg */
    bcm_plp_pm_firmware_media_type_t MediaType; /**< Media Type */
    unsigned int UnreliableLos; /**< For optical use */
    unsigned int ScramblingDisable; /**< disable scrambling */
    unsigned int Cl72AutoPolEn; /**< Forced CL72 */
    unsigned int Cl72RestTO; /**< Forced CL72 */
    unsigned int ForceES; /**< Force ES Mode */
    unsigned int ForceNS; /**< Force NS Mode */
    unsigned int LinkPartnerPrecoderEnable; /* Link Partner has Precoder Enable */
    unsigned int ForcePAM4Mode; /**< Override the PAM4 mode pin with PAM4_MODE*/
    unsigned int ForceNRZMode; /**< Override the PAM4 mode pin with NRZ_MODE*/
    unsigned int DbLoss; /**< DB loss */
    unsigned int LinkTrainingReStartTimeOut; /**< Link Training restart timeout */
    unsigned int OppositeCdrFirst; /**< 1:Wait for opposite cdr_lock is ready before link training start 0: Start link training firstly, then check opposite cdr_lock status */
    unsigned int AnTimer; /** 0:IEEE standard 3s link inhibit timer, 1:6s  link inhibit timer: Applicable only PAM4 modes */
    unsigned int DfeOn; /**< Enable DFE */
    unsigned int ForceBrDfe; /**< Force Baud rate DFE */
    unsigned int LpDfeOn; /**< Enable low power DFE */
    unsigned int TxSquelchOrPrbs; /**< Squelch TX or send PRBS58 pattern if a fault occurs.  0: Squelch TX, 1: Send PRBS58 */
} bcm_plp_pm_firmware_lane_config_t;


typedef struct bcm_plp_dsrds_firmware_lane_config_s {
    unsigned int DfeOn; /**< Enable DFE */
    unsigned int AnEnabled; /**< Autoneg */
    bcm_plp_pm_firmware_media_type_t MediaType; /**< Media Type */
    unsigned int AutoPeakingEnable; /**< 1:Enable, 0:Disable:Auto peaking algorithm in copper mode link training */
    unsigned int OppositeCdrFirst; /**< 1:Wait for opposite cdr_lock is ready before link training start, 0: Start link training first, then check opposite cdr_lock status */
    int DcWanderMu; /**< RX value dc_wander_mu */
    int ExSlicer; /**< RX Extented Slicer */
    unsigned int LinkTrainingReStartTimeOut; /**< Link Training restart timeout */
    unsigned int LinkTrainingCL72_CL93PresetEn; /**< LT RX send cl72/cl93 preset request: 0:Disable (non-IEEE compatible mode); 1: Enable(IEEE compatible mode, default value) */
    unsigned int LinkTraining802_3CDPresetEn; /**< LT RX send 802.3cd preset1 request: 0:Disable (non-IEEE compatible mode); 1: Enable (IEEE compatible mode, default value) */
    unsigned int LinkTrainingTxAmpTuning; /**< Copper PAM4 Link training, 0: RX adjust LP's TX TXFIR with fixed 7 steps; 1: Copper PAM4 Link training, RX adjust LP's TX TXFIR dynamically */
    unsigned int LpDfeOn; /**< Enable Low poer DFE */
    unsigned int AnTimer; /** 0:IEEE standard 3s link inhibit timer, 1:6s  link inhibit timer: Applicable only PAM4 modes */
    unsigned int TxSquelchOrPrbs;  /**< Squelch TX or send PRBS58 pattern if a fault occurs.  0: Squelch TX, 1: Send PRBS58 */
} bcm_plp_dsrds_firmware_lane_config_t;


/*!bcm_plp_ext_fw_params_t
  * This structure is used to get the external firmware information from user
  * \arg firmware_address \n
  *  Firmware Buffer address provided by the application
  * \arg fw_length \n
  * Total size of the firmware microcode to download
  * \arg transfer_size \n
  *  Transfer size is the size of the buffer that the application to provide for a single transfer.\n
  * If total firmaware size is greater than the transfer size, the firmware loader (bcm_firmware_loader_f) function will be called multiple times until total size
  * of firmware ucode is downloaded.  Minimum transfer size is 32 bytes.   The transfer size has to be an even number, otherwise an error will be returned
  *
  */
typedef struct bcm_plp_ext_fw_params_s{
    unsigned char *firmware_address;
    unsigned int  fw_length;
    unsigned int  transfer_size;
}bcm_plp_ext_fw_params_t;

/*!
 * bcm_firmware_loader_f
 * Purpose: To provide application a callback mechanism to register their own firmware loader/
 * helper function \n
 * @brief function definition for firmware loading \n
 *
 * @param [in]  ext_fw_phy_info      - reserved for future use \n
 * @param [out]  ext_fw_params  - External firmware information that needs to be filled by the application: firmware_address, fw_length and transfer_size \n
 * If transfer size is less than total size this function will be called multiple times until it reaches to the total size of the firmware.
 * Minimum transfer size is 32 bytes.  The transfer size has to be an even number, otherwise an error will be returned.\n
 *
 */

typedef int (*bcm_firmware_loader_f)(void *ext_fw_phy_info, bcm_plp_ext_fw_params_t *ext_fw_params);

typedef enum bcm_plp_aperta_pll1_vco_e{
    bcmplpVco20p625G = 0,    /* 20p625 VCO*/
    bcmplpVco25p781G = 1,    /* 25p781 VCO*/
    bcmplpVco26p562G = 2     /* 26p562 VCO*/
}bcm_plp_aperta_pll1_vco_t;

/*!bcm_plp_aperta_fw_init_params_t
  * This structure is intended to initialize HW configuration after FW download
  * \arg macsec_static_bypass\n
  * When set to 1, macsec(EIP 163 & EIP 164) is bypassed otherwise user needs to 
  * initialize MACSEC.
  *
  * \arg pll1_vco_rate\n
  * Used to choose the max vco rate for PLL1.
  *
  * \arg tx_drv_supply\n
  * Forced value for tx driver supply.
  * 1 : TVDD1P25 supply is set to 1.0 V 
  * 0 : TVDD1P25 supply is set to 1.25 V 
  * 
  */
typedef struct bcm_plp_aperta_fw_init_params_s {
    unsigned int  macsec_static_bypass;
    bcm_plp_aperta_pll1_vco_t  pll1_vco_rate;
    unsigned int  tx_drv_supply;
} bcm_plp_aperta_fw_init_params_t;

/*!bcm_plp_agera_fw_init_params_t
  * This structure is intended to configure agera PHY after FW download
  * \arg ref_clk\n
  * Reference clock information required for PHY configuration.
  */
typedef struct bcm_plp_agera_fw_init_params_s {
    bcm_plp_pm_ref_clk_t  ref_clk;

} bcm_plp_agera_fw_init_params_t;

typedef bcm_plp_agera_fw_init_params_t bcm_plp_ageralite_fw_init_params_t;

/*! bcm_plp_firmware_load_type_t \n
  * This structure is used to select firmware load method and firmware load type.  It i salso used
  * to register external firmware load function if external firmware load method is selected \n
  * \arg firmware_load_method \n
  *  Select the firmware load method \n
  * \arg force_load_method  \n
  *   Select the firmware load type as force, auto or none \n
  * \arg firmware_loader   \n
  * If external firmware load method is selected, application has to register a firmware load function. \n
  * \arg fw_init_params \n
  * void pointer is used to pass the firmware init params \n
  *
 */
typedef struct bcm_plp_firmware_load_type_s{
    bcm_pm_firmware_load_method_t firmware_load_method;
    bcm_pm_firmware_load_force_t  force_load_method;
    bcm_firmware_loader_f firmware_loader;
    void *fw_init_params;
}bcm_plp_firmware_load_type_t;

typedef struct bcm_plp_an_config_s{
    unsigned int master_lane;
    unsigned int cl72_en;
    unsigned int tech_ability;
    unsigned int tech_ability_ext;
}bcm_plp_an_config_t;

typedef enum bcm_plp_interface_mode_e {
    bcmplpInterfaceModeIEEE=0x100000,
    bcmplpInterfaceModeHIGIG=0x10,
    bcmplpInterfaceModeOS2=0x20,
    bcmplpInterfaceModeSCR=0x40,
    bcmplpInterfaceModeHALFDUPLEX=0x80,
    bcmplpInterfaceModeFIBER=0x100,
    bcmplpInterfaceModeTRIPLECORE=0x200,
    bcmplpInterfaceModeTC343=0x400,
    bcmplpInterfaceModeTC442=0x800,
    bcmplpInterfaceModeTC244=0x1000,
    bcmplpInterfaceModeBACKPLANE=0x2000,
    bcmplpInterfaceModeCOPPER=0x4000,
    bcmplpInterfaceModeOTN=0x8000,
    bcmplpInterfaceModeSIMPLEX=0x10000,
    bcmplpInterfaceModeUNRELIABLELOS=0x20000,
}bcm_plp_interface_mode_t;

/* MAC */
typedef enum bcm_plp_mac_lb_type_e {
    bcmplpMacOuterloopback = 0,
    bcmplpMaxloopbackCnt
}bcm_plp_mac_lb_type_t;

typedef enum bcm_plp_mac_flow_control_e {
    bcmplpFlowcontrolTerminateGenerate = 0,
    bcmplpFlowcontrolPassthrough
} bcm_plp_mac_flow_control_t;

typedef enum bcm_mac_fault_option_e {
    bcmplpFaultoptionTerminateGenerate = 0,
    bcmplpFaultoptionPassthrough
} bcm_plp_mac_fault_option_t;

typedef enum bcm_plp_mac_frame_select_e {
    bcmplpFrameNONE = 0,
    bcmplpFrameControl,
    bcmplpFramePause,
    bcmplpFramePFC
} bcm_plp_mac_frame_select_t;

typedef struct bcm_plp_mac_pause_control_s {
    unsigned char rx_enable;
    unsigned char tx_enable;
    int refresh_timer; /* use -1 for disable this feature; Threshold for pause timer to cause XOFF to be resent */
    int xoff_timer; /* Time value sent in the Timer Field for classes in XOFF state */
} bcm_plp_mac_pause_control_t;

typedef struct bcm_plp_mac_pfc_control_s {
    unsigned char rx_enable;
    unsigned char tx_enable;
    unsigned char stats_en; /* enable PFC counters */
    unsigned char force_xon; /* Instructs MAC to send Xon message to all classes of service */
    int refresh_timer; /* use -1 for disable this feature; Threshold for pause timer to cause XOFF to be resent */
    int xoff_timer; /* Time value sent in the Timer Field for classes in XOFF state */
} bcm_plp_mac_pfc_control_t;

typedef struct bcm_plp_mac_access_s {
    bcm_plp_access_t phy_info;
}bcm_plp_mac_access_t;

typedef enum bcm_plp_fec_type_e {
    bcmplpFecType91 = 0,
    bcmplpFecType74,
    bcmplpFecType108
} bcm_plp_fec_type_t;

typedef struct bcm_plp_l1_intr_status_s {
    unsigned int l1_intr_sts;
    unsigned int reserved;
}bcm_plp_l1_intr_status_t;

/*struct for 1588 tx info*/
typedef struct bcm_plp_pm_ts_tx_info_s {
    unsigned int ts_in_fifo_lo; /**< low 32bit of Timestamp in Fifo */
    unsigned int ts_in_fifo_hi; /**< high 32bit of Timestamp in Fifo */
    unsigned int ts_seq_id; /**< sequence id of tx 1588 packet */
    unsigned int ts_sub_nanosec; /**< sub nanoseconds of tx 1588 packet */
} bcm_plp_pm_ts_tx_info_t;

/**
 * JFEC bit flip enable control
 */
typedef enum bcm_plp_jfec_bit_flip_ena_e {
    bcmplpBitFlipDisable =0,
	bcmplpBitFlipEnable
} bcm_plp_jfec_bit_flip_ena_t;


/**
 * JFEC bit swap enable control
 */
typedef enum bcm_plp_jfec_bit2_swap_ena_e {
    bcmplpBit2SwapDisable =0,
	bcmplpBit2SwapEnable
} bcm_plp_jfec_bit2_swap_ena_t;

/**
 * JFEC word swap disable control
 */
typedef enum bcm_plp_jfec_word_swap_dis_e {

	bcmplpWordSwapEnable =0,
	bcmplpWordSwapDisable =1,
} bcm_plp_jfec_word_swap_dis_t;

/**
 * JFEC data swap enable control
 */
typedef enum bcm_plp_jfec_data_swap_ena_e {
    bcmplpDataSwapDisable =0,
	bcmplpDataSwapEnable
} bcm_plp_jfec_data_swap_ena_t;


/**
 * JFEC data path mode selection structure
 */
typedef enum bcm_plp_jfec_dp_mode_sel_e {
    bcmplpDpTraffic=0,
	bcmplpDpPrbs =2,
	bcmplpDpPrbsGen =3,
	bcmplpDpPrbsMon =4,
	bcmplpDpMaxCount
} bcm_plp_jfec_dp_mode_sel_t;

/**
 * JFEC decoding bypass enable/disable structure
 */
typedef enum bcm_plp_jfec_dec_bypass_ena_e {
    bcmplpFecDecBypassDisable =0,
	bcmplpFecDecBypassEnable
} bcm_plp_jfec_dec_bypass_ena_t;

/**
 * JFEC encoding bypass enable/disable structure
 */
typedef enum bcm_plp_jfec_enc_bypass_ena_e {
    bcmplpFecEncBypassDisable =0,
	bcmplpFecEncBypassEnable
} bcm_plp_jfec_enc_bypass_ena_t;


/**
 * JFEC mode selection
 */
typedef enum bcm_plp_jfec_mode_sel_e {
    bcmplpFecModeT12 =0,
	bcmplpFecModeT8
} bcm_plp_jfec_mode_sel_t;

/**
 * FEC uncorrectable error enable structure
 */
typedef enum bcm_plp_jfec_uncorr_err_ena_e {
    bcmplpUncorrErrorCreateDisable=0,
	bcmplpUncorrErrorCreateEnable,
} bcm_plp_jfec_uncorr_err_ena_t;

/**
 * JFEC control structure
 */
typedef struct bcm_plp_jfec_config_s{
    bcm_plp_jfec_bit_flip_ena_t bit_flip_ena; /*bcm_plp_jfec_bit_flip_ena_t */
    bcm_plp_jfec_bit2_swap_ena_t bit2_swap_ena; /*bcm_plp_jfec_bit2_swap_ena_t  */
    bcm_plp_jfec_word_swap_dis_t word_swap_dis; /*bcm_plp_jfec_word_swap_dis */
    bcm_plp_jfec_data_swap_ena_t data_swap_ena; /*bcm_plp_jfec_data_swap_ena_t */
    bcm_plp_jfec_dp_mode_sel_t dp_mode_sel; /*bcm_plp_jfec_dp_mode_sel_t*/
    bcm_plp_jfec_dec_bypass_ena_t fec_dec_bypass_ena; /*bcm_plp_jfec_dec_bypass_ena */
    bcm_plp_jfec_enc_bypass_ena_t fec_enc_bypass_ena; /*bcm_plp_jfec_enc_bypass_ena_t */
    bcm_plp_jfec_mode_sel_t fec_mode_sel; /*fec_mode_sel */
    unsigned short align_lock_chk_wdw; /*align_lock_chk_wdw */
    unsigned short align_lock_chk_thr; /*align_lock_chk_thr  */
    unsigned short align_unlock_thr; /*align_unlock_thr */
    unsigned short fec_cerr_sel; /*fec_cerr_sel */
    bcm_plp_jfec_uncorr_err_ena_t fec_uerr_ena      ; /*jfec_uncorr_err_ena_t */
} bcm_plp_jfec_config_t;

typedef enum bcm_plp_kp4_50g_mode_e {
    bcmplpKP450GCompModeIEEE802p3CD50G = 0, /**< 802.3CD 50G mode */
    bcmplpKP450GCompModeConsortiumRs54450G = 1, /**< Consortium RS544 50g mode */
    bcmplpKP450GCompModeCount
} bcm_plp_kp4_50g_mode_t;

typedef enum bcm_plp_kp4_fec_format_e {
    bcmplpKP4FecFormatIEEE802p3CD = 0, /**< 802.3CD Format */
    bcmplpKP4FecFormatNicholsProposal = 1, /**< NICHOLS_PROPOSAL */
    bcmplpKP4FecFormatCount
} bcm_plp_kp4_fec_format_t;

typedef enum bcm_plp_kp4_message_mode_e {
    bcmplpKP4MessageModeNoLegacy = 0, /**< Not legacy mode */
    bcmplpKP4MessageModeLegacy = 1, /**< Legacy mode */
    bcmplpKP4MessageModeCount
} bcm_plp_kp4_message_mode_t;


typedef enum bcm_plp_kp4_term_encoder_bypass_e {
    bcmplpKP4TermEncoderNoBypass = 0, /**< No Bypass term encoder */
    bcmplpKP4TermEncoderBypass = 1, /**< Bypass term encoder */
    bcmplpKP4TermEncoderCount
} bcm_plp_kp4_term_encoder_bypass_t;

typedef struct bcm_plp_kp4_prbs_config_s {
    bcm_pm_prbs_poly_t poly; /** < PRBS polynomial */
    unsigned int enable;        /**< PRBS enable */
    unsigned int invert;        /**< PRBS invert */
    unsigned int tx_rx;        /**< 0 = Both transmit and receive PRBS \n 1 = Receive side PRBS\n 2 = Transmit side PRBS. Input only */
} bcm_plp_kp4_prbs_config_t;

typedef struct bcm_plp_kp4_prbs_status_s {
    unsigned int prbs_lock; /**< Whether PRBS is currently locked */
    unsigned int prbs_lock_loss; /**< Whether PRBS was unlocked since last call */
    unsigned int error_count; /**< PRBS errors count */
} bcm_plp_kp4_prbs_status_t;


typedef enum bcm_plp_kp4_prbs_e {
    bcmplpKP4PrbsDisable = 0, /**< Disable  */
    bcmplpKP4PrbsEnable = 1, /**< Enable  */
    bcmplpKP4PrbsCount
} bcm_plp_kp4_prbs_t;

typedef enum bcm_plp_kp4_fec_lane_convertor_e {
    bcmplpKP4FecLaneConverterDisable = 0, /**< Disable lane convertor */
    bcmplpKP4FecLaneConverterEnable = 1, /**< Enable lane convertor */
    bcmplpKP4FecLaneConverterCount
} bcm_plp_kp4_fec_lane_converter_t;

typedef enum bcm_plp_kp4_fec_convert_format_e {
    bcmplpKP4FecConvertFormatByIEEE = 0,
    bcmplpKP4FecConvertFormatLegacy = 1,
    bcmplpKP4FecConvertFormatCount
} bcm_plp_kp4_fec_convert_format_t;

/**
 * KP4 Output FEC Lane number
 */
typedef enum bcm_plp_kp4_output_fec_lane_num_e  {
    bcmplpKp4OutputFec4Lanes =0,
    bcmplpKp4OutputFec2Lanes
} bcm_plp_kp4_output_fec_lane_num_t;

/**
 * KP4 Freeze Alignment Lock
 */
typedef enum bcm_plp_kp4_freeze_align_lock_e  {
    bcmplpAlignLockByIeee =0,
    bcmplpFreezeAlignLock
} bcm_plp_kp4_freeze_align_lock_t;


typedef struct bcm_plp_kp4_fec_config_s {
    unsigned int kp4_fec_config_enable;  /** KP4 FEC config enable. This field must be set 1 to configure KP4 FEC. Input only, Output is ignored*/
    bcm_plp_kp4_50g_mode_t kp4_50g_mode; /**< KP4 50G mode */
    bcm_plp_kp4_fec_format_t kp4_fec_format; /**< KP4 FEC format */
    bcm_plp_kp4_message_mode_t kp4_message_mode; /**< KP4 message mode */
    bcm_plp_kp4_term_encoder_bypass_t kp4_term_encoder_bypass; /**< KP4 term encoder */
    bcm_plp_kp4_fec_lane_converter_t fec_lane_converter_enable; /**< FEC lane convertor */
    bcm_plp_kp4_fec_convert_format_t kp4_fec_lane_converter; /**< KP4 FEC convertor format */
    bcm_plp_kp4_output_fec_lane_num_t kp4_output_fec_lane_num; /**< This field has been obsoleted, do not use */
    bcm_plp_kp4_freeze_align_lock_t kp4_freeze_align_lock; /**< This field has been obsoleted, do not use */
    bcm_plp_kp4_prbs_t kp4_prbs; /**< KP4 FEC Prbs: Note: This field must be set to 1 to configure KP4 PRBS */
    bcm_plp_kp4_prbs_config_t kp4_prbs_config; /**< KP4 FEC Prbs config */
    bcm_plp_kp4_prbs_status_t kp4_prbs_status; /**< KP4 FEC Prbs status. Output only */
    unsigned int kp4_prbs_align_tx; /**< KP4 FEC Prbs tx align, Input only, Output is ignored*/
    unsigned int kp4_prbs_align_rx; /**< KP4 FEC Prbs rx align Input only, Output is ignored*/
    unsigned int kp4_prbs_init_tx; /**< KP4 FEC Prbs tx init Input only, Output is ignored*/
    unsigned int kp4_prbs_init_rx; /**< KP4 FEC Prbs rx init Input only, Output is ignored*/
} bcm_plp_kp4_fec_config_t;

/*********************************FEC get information ***********************************/


#define BCM_PLP_FEC_TOT_BITS_CORR_NUM   2
#define BCM_PLP_FEC_TOT_FRAMES_ERR_NUM 16                     /*JFEC is 13, KP4 is 16 */


/**
 * FEC status structure to support both KP4 and JFEC
 */

typedef struct bcm_plp_fec_err_cnt_s{
	unsigned int ieee_uncorr_cnt;
	unsigned int ieee_symbols_corr_cnt;
	unsigned int sys_am_lock;
	unsigned int line_am_lock;
	unsigned int ieee_kp4_dec_ctrl;
	unsigned int ieee_kp4_stat_ctrl;
	unsigned int ieee_fec_lane_mapping;
	unsigned int ieee_kp4_hi_ser_th_sp;
	plp_uint64_t  tot_frame_rev_cnt;
	plp_uint64_t  tot_frame_corr_cnt;
    plp_uint64_t  tot_frame_uncorr_cnt;
    plp_uint64_t  tot_symbols_corr_cnt;
    plp_uint64_t  tot_bits_corr_cnt[BCM_PLP_FEC_TOT_BITS_CORR_NUM];
    plp_uint64_t  bcm_plp_tot_frames_err_cnt[BCM_PLP_FEC_TOT_FRAMES_ERR_NUM];
}bcm_plp_fec_err_cnt_t;

/**
 * The enumeration of lol sticky
 */
typedef enum bcm_plp_align_lol_e {
    bcmplpFecNoLolFticky,               /* fec lol sticky is not set   */
    bcmplpFecLolFticky,                  /* fec lol sticky is set  */
} bcm_plp_align_lol_t;


/**
 * The enumeration of align lock
 */
typedef enum bcm_plp_align_lock_e {
    bcmplpFecAlignUnlock,               /* fec align is not locked   */
    bcmplpFecAlignLocked,                  /* fec align is locked   */
} bcm_plp_align_lock_t;


typedef struct bcm_plp_fec_rx_err_cnt_s {
    plp_uint64_t tot_frame_rev_cnt; /**< Total frame count received */
    plp_uint64_t tot_frame_corr_cnt; /**< Total frame corrected count */
    plp_uint64_t tot_frame_uncorr_cnt; /**< Total frame un-corrected count */
    plp_uint64_t tot_symbols_corr_cnt; /**< based on tot_frames_err_cnt[0..15] */
    plp_uint64_t tot_bits_corr_cnt[2]; /**< [0] for correctable 0's; [1] for correctable 1's */
    plp_uint64_t tot_frames_err_cnt[16]; /**< [n] for number of cw with correstable symbol error=n */
} bcm_plp_fec_rx_err_cnt_t;

typedef struct bcm_plp_ieee_fec_rx_status_s {
    unsigned short ieee_rsfec_ctrl; /**< 1.200 for 50G/100G ; 3.800 for 200G */
    unsigned short ieee_rsfec_stat; /**< 1.201 for 50G/100G ; 3.801 for 200G */
    unsigned short ieee_fec_lane_mapping; /**< ln0_map[1:0]; ln1_map[3:2]; ln2_map[5:4]; ln3_map[7:6] 1.206 for 100G/50G only */
    unsigned short ieee_corr_cw_cnt; /**< 1.202+1.203 - RS-FEC corrected codewords counter */
    unsigned short ieee_uncorr_cw_cnt; /**< 1.204+1.205 - RS-FEC uncorrected codewords counter */
    unsigned short ieee_symbols_corr_cnt_fln[8]; /**< 1.210+1.211..1.216+1.217 - Symbol error per fec lane for 50G(2ln)/100G(4ln); 3.600+3.601..3.614+3.615 for 200G(8ln) */
    unsigned short ieee_fecpcs_alignment_status_1; /**< 3.50 - pcs lane align @12 for 200g only */
    unsigned short ieee_fecpcs_alignment_status_3; /**< 3.52 - align_status @0..7 for 200g only */
    unsigned short ieee_fecpcs_lane_mapping[8]; /**< 3.400..3.407 - pcs lane mapping [4:0]; for 200g only */
} bcm_plp_ieee_fec_rx_status_t;

typedef struct bcm_plp_fec_rx_status_s {
    /* NOTE: Not all the members of this structure applicable to all PHY families.  
       Please refer to corresponding user guides for more information */
    unsigned char igbox_clsn_sticky; /**< one-hot encoded intf-gbox clsn per fec lane - read followed by clear */
    unsigned char am_lolock_sticky; /**< one-hot encoded am_lolock per fec lane - read followed by clear */
    unsigned char dgbox_clsn_sticky; /**< dgbox clsn for aggr fec traffic - read followed by clear */
    unsigned char hi_ser_sticky; /**< high SER for aggr fec traffic - read followed by clear */
    unsigned char xdec_err_sticky; /**< Xdecoder error for aggr fec traffic - read followed by clear */
    unsigned char fec_link_stat; /**< 1: link up; 0: link down */
    unsigned char fec_link_stat_sticky; /**< 1: link up; 0: link down - read followed by clear */
    unsigned int intf_gbox_clsn_sticky; /**< one-hot encoded intf-gbox clsn per fec lane - read followed by clear */
    unsigned int am_lo_lock_sticky; /**< one-hot encoded am_lolock per fec lane - read followed by clear */
    unsigned int xdec_gbox_clsn_sticky; /**< xdec gearbox collison per fec port - read followed by clear */
    unsigned int xenc_gbox_clsn_sticky; /**< xenc gearbox collison per fec port - read followed by clear */
    unsigned int ogbox_clsn_sticky; /**< xdec gearbox collison per fec port - read followed by clear */
    unsigned int fec_algn_lol_sticky; /**< alignment lol sticky per fec port - read followed by clear */
    unsigned int fec_algn_stat; /**< alignment lock live status */
} bcm_plp_fec_rx_status_t;

typedef struct bcm_plp_fec_ber_s {
    plp_float_t pre_fec_ber; /**< pre fec BER */
    plp_float_t post_fec_ber; /**< post fec BER */
    plp_float_t post_fec_ber_proj; /**< projected post fec BER */
} bcm_plp_fec_ber_t;

typedef struct bcm_plp_fec_dump_status_s {
    bcm_plp_ieee_fec_rx_status_t ieee_fec_sts; /**< for 25g/50g/100g/200g Note: Applicable only for millenio */
    bcm_plp_fec_rx_status_t fec_sts;           /**< fec status*/
    bcm_plp_fec_rx_err_cnt_t fec_a_err_cnt;    /**< fec err counter struct - should latch/clear/update counters separately */
    bcm_plp_fec_rx_err_cnt_t fec_b_err_cnt;    /**< fec err counter struct - only for 200g */
    bcm_plp_fec_ber_t fec_ber;                 /**< fec ber */
} bcm_plp_fec_dump_status_t;

/*!
 * @enum bcm_plp_fec_status_init_e
 * @brief FEC status counters enable/disable
 */
typedef enum bcm_plp_fec_status_init_e {
    bcmplpFecStatusInitDefault = 0, /**< FEC status init default */
    bcmplpFecStatusInitEnable, /**< Enable the FEC status counters */
    bcmplpFecStatusInitDisable, /**< Disable the FEC status counters */
} bcm_plp_fec_status_init_t;

/**
 * The struct of fec status supports both Kp4 and Jfec
 */

typedef struct bcm_plp_fec_status_s {
    /* NOTE: Not all the members of this structure applicable to all PHY families.  
       Please refer to corresponding user guides for more information */
    bcm_plp_align_lol_t align_lol_sticky;            /*whether align LOL sticky set or not */
    bcm_plp_align_lock_t align_lock;                 /*whether align is locked or not*/
    bcm_plp_fec_err_cnt_t fec_err_cnt;               /*fec err counter struct */
    bcm_plp_fec_dump_status_t fec_dump_status;       /* Fec status along with IEEE status dump*/
    unsigned char fec_status_clear;                  /* used clear the fec status */
    bcm_plp_fec_status_init_t fec_status_init;       /* used enable/disable the fec status counters*/
} bcm_plp_fec_status_t;


/*
 * Below enums can be used to set the ctrl_value in
 * bcm_plp_base_t_device_aux_modes_t, when passing
 * bcmplpBaseTCtrlMacsecEn as the ctrl_select.
 * Follow the sequences below to configure MACSEC
 * during startup or after a link toggle using
 * the bcm_plp_mode_config_set() API.

 * Sequence to disable MACSEC:
        Startup:
            - bcmplpMacsecEnableOff(port status: MACSEC-disabled)
        After a link toggle:
            - Do nothing(port status: MACSEC-disabled)

 * Sequence to enable MACSEC packet transformation:
        Startup:
            - bcmplpMacsecEnableSet
              Wait till the link is up(port status: MACSEC-ready-for-enable, Bypass-disabled)
            - bcmplpMacsecEnableOn(port status: MACSEC-enabled, Bypass-disabled
        After a link toggle:
              Wait till the link is up(port status: MACSEC-ready-for-enable, Bypass-disabled)
            - bcmplpMacsecEnableOn(port status: MACSEC-enabled, Bypass-disabled)

 * Sequence to enable MACSEC for Bypass traffic:
        Startup:
            - bcmplpMacsecEnableSetBypass(port status: MACSEC-enabled, Bypass-enabled)
        After a link toggle:
            - Do nothing(port status: MACSEC-enabled, Bypass-enabled)

* Sequence to disable Bypass, if port is in Bypass when MACSEC is enabled:
        Startup (or) After a link toggle:
              Wait till the link is up(port status: MACSEC-enabled, Bypass-enabled)
            - bcmplpMacsecEnableClearBypass(port status: MACSEC-enabled, Bypass-disabled)
*/
typedef enum  bcm_plp_macsec_enable_mode_e {
    bcmplpMacsecEnableOff = 0,
    bcmplpMacsecEnableOn = (1 << 0),
    bcmplpMacsecEnableSet = (1 << 1),
    bcmplpMacsecEnableSetBypass = (1 << 2),
    bcmplpMacsecEnableClearBypass = (1 << 3),
} bcm_plp_macsec_enable_mode_t;

/*************************************\
** BASE_T Specific Defines
*************************************/

typedef enum  bcm_plp_base_t_interrupt_e {
    bcmplpBaseTIntrPtpPic    = (1 << 0),  /* IEEE1588 Interrupt INT_PIC_EN  */
    bcmplpBaseTIntrPtpFsync  = (1 << 1),  /* IEEE1588 Interrupt INT_FYNC_EN */
    bcmplpBaseTIntrPtpSop    = (1 << 2),  /* IEEE1588 Interrupt INT_SOP     */
    bcmplpBaseTIntrPtpSyncIn = (1 << 3),  /* IEEE1588 Interrupt PCH Sync-In */
    bcmplpBaseTIntrPtpIpg    = (1 << 4),  /* IEEE1588 Interrupt Preamble or IPG error */
    bcmplpBaseTIntrCount
} bcm_plp_base_t_interrupt_e;
typedef bcm_plp_base_t_interrupt_e  bcm_plp_base_t_interrupt_t;

typedef enum  bcm_plp_base_t_speed_e {
    bcmplpBaseTSpeed0       = 0x0,         /* Not used */
    bcmplpBaseTSpeed10      = (1 << 0),    /*  10M     */
    bcmplpBaseTSpeed100     = (1 << 5),    /* 100M     */
    bcmplpBaseTSpeed1000    = (1 << 6),    /*   1G     */
    bcmplpBaseTSpeed2500    = (1 << 7),    /* 2.5G     */
    bcmplpBaseTSpeed5000    = (1 << 9),    /*   5G     */
    bcmplpBaseTSpeed7500    = (1 << 10),   /* 7.5G     */
    bcmplpBaseTSpeed10000   = (1 << 11),   /*  10G     */
    bcmplpBaseTSpeed25000   = (1 << 13),   /*  25G     */
    bcmplpBaseTSpeedNonIEEE = (1 << 24),   /* Non-IEEE */
    bcmplpBaseTFastRetrain2500  = (1 << 25), /* 2.5G FR  */
    bcmplpBaseTFastRetrain5000  = (1 << 26), /*   5G FR  */
    bcmplpBaseTFastRetrain10000 = (1 << 27), /*  10G FR  */
    bcmplpBaseTFastRetrain25000 = (1 << 28), /*  25G FR  */
    bcmplpBaseTFastRetrain7500  = (1 << 29), /* 7.5G FR  */
    bcmplpBaseTRepeaterDTE      = (1 << 30), /*  repeater DTE  */
    bcmplpBaseTSpeedCount
} bcm_plp_base_t_speed_t;   /* This structure needs to be in sync *\
                            \*  with its SDK counterpart          */

typedef enum  bcm_plp_base_t_pause_e {
    bcmplpBaseTPauseNone   = 0x0,           /* no pause */
    bcmplpBaseTPauseTx     = (1 << 0),      /* Tx pause */
    bcmplpBaseTPauseRx     = (1 << 1),      /* Rx pause */
    bcmplpBaseTPauseRxTx   =((1 << 1) | (1 << 0)), /* Tx/Rx pause      */
    bcmplpBaseTPauseCount
} bcm_plp_base_t_pause_t;   /* This structure needs to be in sync *\
                            \*  with its SDK counterpart          */

typedef enum  bcm_plp_base_t_eee_ability_e {
    bcmplpBaseTEee100mBaseTX = (1 << 0),
    bcmplpBaseTEee1gBaseT    = (1 << 1),
    bcmplpBaseTEee10gBaseT   = (1 << 2),
    bcmplpBaseTEeeCount

} bcm_plp_base_t_eee_ability_t;

typedef enum bcm_plp_base_t_mdix_mode_e {
    bcmplpBaseTMdixAuto = 0,
    bcmplpBaseTMdixForceAuto,
    bcmplpBaseTMdixStraight,
    bcmplpBaseTMdixCrossover,
    bcmplpBaseTMdixCount
} bcm_plp_base_t_mdix_mode_t;   /* This structure needs to be in sync *\
                                \*  with its SDK counterpart          */

typedef enum bcm_plp_base_t_ms_mode_e {
    bcmplpBaseTMsModeSlave = 0,
    bcmplpBaseTMsModeMaster,
    bcmplpBaseTMsModeAuto,
    bcmplpBaseTMsModeNone,
    bcmplpBaseTMsModeCount
} bcm_plp_base_t_ms_mode_e;     /* This structure needs to be in sync *\
                                \*  with its SDK counterpart          */
typedef bcm_plp_base_t_ms_mode_e  bcm_plp_base_t_ms_mode_t;

typedef enum bcm_plp_base_t_power_mode_e { /* AutoNeg master/slave mode */
    bcmplpBaseTPowerModeNoChange = -1,   /* stay where you are */
    bcmplpBaseTPowerModeFull = 0,        /* full power */
    bcmplpBaseTPowerModeLow,             /* low power */
    bcmplpBaseTPowerModeAuto,            /* auto power */
    bcmplpBaseTPowerModeCount
} bcm_plp_base_t_power_mode_t;  /* This structure needs to be in sync *\
                                \*  with its SDK counterpart          */

/*!
 * @enum bcm_plp_base_t_ctrl_e
 * @brief Aux PHY Controls from setting BASE-T features thru bcm_plp_barchetta_mode_config_set()/_get()
 */
typedef enum  bcm_plp_base_t_ctrl_e {
    bcmplpBaseTCtrlNone   = -1,
    bcmplpBaseTCtrlDuplex =  0,                     /* Full or half duplex */
    bcmplpBaseTCtrlMdix,                            /* Auto MDIX setting */
    bcmplpBaseTCtrlMdixStatus,                      /* Get MDIX status after AN completes */
    bcmplpBaseTCtrlMasterSlave,                     /* Used to set AN mode (master, slave or auto) */
    plpBASETCtrlMacsecEn,                           /* Used to enable/disable MACSEC */
    bcmplpBaseTCtrlMacsecEn = plpBASETCtrlMacsecEn, /* Same as plpBASETCtrlMacsecEn */
    bcmplpBaseTCtrlSuperIsolate,                    /* Used to configure super isolate */
    bcmplpBaseTCtrlSgmii1GAutodetect,               /* used for auto detecting SGMII slave or 1000X (fiber) */
    bcmplpBaseTCtrlForceFiberIface,                 /* Used to set fiber interface without auto medium */
    bcmplpBaseTCtrlSerdesAutoNeg,                   /* Used to set USXGMII AN */
    bcmplpBaseTCtrlPreambleAlignment,               /* Control preamble alignment to interoperate with
                                                     * non-IEEE-standard PHYs */
    bcmplpBaseTCtrlCount
} bcm_plp_base_t_ctrl_t;

typedef enum bcm_plp_base_t_eee_mode_e {
    bcmplpBaseTEeeModeNoChange = -1, /* stay where you are */
    bcmplpBaseTEeeModeNone = 0,      /* EEE off */
    bcmplpBaseTEeeModeAuto,          /* AutogrEEEn mode */
    bcmplpBaseTEeeModeNative,        /* native IEEE EEE mode */
    bcmplpBaseTEeeModeGetStats,      /* for getting stats info, no change on EEE mode */
    bcmplpBaseTEeeModeClearStats,    /* for clearing stats info, no change on EEE mode */
    bcmplpBaseTEeeModeCount
} bcm_plp_base_t_eee_mode_t;

typedef enum bcm_plp_base_t_eee_latency_e {
    bcmplpBaseTEeeLatencyNoChange = -1,  /* stay where you are    */
    bcmplpBaseTEeeLatencyVariable = 0,   /* variable latency mode */
    bcmplpBaseTEeeLatencyFixed,          /* fixed latency mode    */
    bcmplpBaseTEeeLatencyCount
} bcm_plp_base_t_eee_latency_t;

typedef struct bcm_plp_base_t_eee_stats_s {
    unsigned int  tx_events;     /* Tx event counts */
    unsigned int  tx_duration;   /* Tx LPI duration */
    unsigned int  rx_events;     /* Rx event counts */
    unsigned int  rx_duration;   /* Rx LPI duration */
} bcm_plp_base_t_eee_stats_t;

typedef struct bcm_plp_base_t_eee_s {
    bcm_plp_base_t_eee_mode_t  mode;         /* EEE mode               */
    bcm_plp_base_t_eee_latency_t  latency;   /* EEE latency            */
    bcm_plp_base_t_eee_stats_t  stats;       /* EEE statistic info     */
    unsigned int  latency_value;             /* Constant latency value */
    unsigned int  idle_threshold;            /* Idle threshold         */
} bcm_plp_base_t_eee_t;

typedef enum bcm_plp_base_t_automedium_e {
    bcmplpBaseTAutomediumDefault = 0,  /* Keep software defaults for backward compatibility */
                                       /* Automedium is enabled by default */
    bcmplpBaseTAutomediumEnable  = 1,  /* Enable automedium */
    bcmplpBaseTAutomediumDisable = 2   /* Disable automedium */
} bcm_plp_base_t_automedium_t;


typedef struct bcm_plp_base_t_device_aux_modes_s {
    unsigned int  ctrl_select;
    unsigned int  ctrl_value;
    void         *ctrl_data_ptr;
    unsigned int  automedium;          /* Use bcm_plp_base_t_automedium_t to configure whether both */
                                       /* copper and fiber media types are selected or not */
    unsigned int  media_preferred;     /* Configure media precedence, Copper = 0, Fiber = 1 */
} bcm_plp_base_t_device_aux_modes_t;

/*! Seahawks device aux mode
 *
 *  This structure can be used to set chip specific feature. \n
 *  This structure is used in bcm_plp_mode_config_set/bcm_plp_mode_config_get APIs. \n
 *
 * \arg ctrl_select \n
 *  Optional:  Seahawks duplex, mdix control and master select control setting can be selected \n
 *  Control parameter can be selected by the control select enum bcm_plp_base_t_ctrl_t. \n
 *  bcmplpBaseTCtrlDuplex = 0, bcmplpBaseTCtrlMdix = 1, bcmplpBaseTCtrlMdixStatus = 2, \n
 *  bcmplpBaseTCtrlMasterSlave = 3, plpBASETCtrlMacsecEn = 4, bcmplpBaseTCtrlCount = 5. \n
 * \arg ctrl_value \n
 *   value to be passed for ctrl_select parameter.
 *
 * \arg ctrl_data_ptr \n
 *   control data memory if required.
 * \arg macsec_enable \n
 *    If macsec feature is enabled \n
 *    1 or plpBASETCtrlMacsecEn - Enable \n
 *    0 - Disable \n
 * \arg macsec_dev_addr \n
 *  MDIO address for MACSEC device \n
 *  This is calculated based on phy chip base address and number of lanes supported \n
 *  Like for BCM54190: 
 *  macsec_dev_addr = BASE_PHY_ADDR[0] + 9    \n
 *  For BCM54194:   
 *  macsec_dev_addr = BASE_PHY_ADDR[0] + 5  \n
 *  There is one exception for BCM54194 chip if system side interface is QSGMII
 *  macsec_dev_addr = BASE_PHY_ADDR[0] + 9    \n
 * 
 * \arg macsec_mac_policy \n
 *  Mac policy for switch side \n
 *  1 - MACSEC_SWITCH_FOLLOWS_LINE \n
 *  2 - PHY_MAC_SWITCH_DUPLEX_GATEWAY \n
 */
typedef struct bcm_plp_base_t_seahawks_aux_modes_s {
    unsigned int  ctrl_select;
    unsigned int  ctrl_value;
    void         *ctrl_data_ptr;
    int macsec_enable;
    int macsec_dev_addr;
    int macsec_mac_policy;
    unsigned int  automedium;          /* Use bcm_plp_base_t_automedium_t to configure whether both */
                                       /*  copper and fiber media types are selected or not */
    unsigned int  media_preferred;     /* Configure media precedence, Copper = 0, Fiber = 1 */
} bcm_plp_base_t_seahawks_aux_modes_t;

/*!
 *  Selecting datapath for Gallardo28 chip specific settings
 *  \arg bcmplpGallardo28Datapath20 \n
 *   Selecting 20 bit datapath \n
 *  \arg bcmplpGallardo28Datapath4Depth1 \n
 *  Selecting 4 bit datapath with latency depth1 \n
 *  \arg bcmplpGallardo28Datapath4Depth2 \n
 *  Selecting 4 bit datapath with latency depth2 \n
 *
 */
typedef enum bcm_plp_gallardo28_datapath_e{
    bcmplpGallardo28Datapath20,
    bcmplpGallardo28Datapath4Depth1,
    bcmplpGallardo28Datapath4Depth2,
    bcmplpGallardo28DatapathCount
}bcm_plp_gallardo28_datapath_t;

/*! Gallardo28 device aux mode
 *
 *  This structure is used to set chip specific features in  bcm_plp_mode_config_set/get APIs. \n
 *
 * \arg datapath \n
 *  Select datapath, e.g. bcm_plp_gallardo28_DATAPATH_20 etc. \n
 *  Note: 20 bit datapath is used during initialization (e.g. CL72 training, loopback mode and PRBS).
 *  20 bit datapath is enabled after device reset, thus, 4 bit datapath needs to be \n
 *  explicitly enabled at the end of initialization procedure before data traffic commences. \n
 *
 * \arg module_auto_detect_en \n
 * Enable auto module detection for SFP and QSFP modules \n
 *   1         enable auto module detect  \n
 *   0         Disable auto module detect  \n
 * \arg rx_los_en \n
 * Enable RX LOS feature \n
 *   1         enable RX LOS  \n
 *   0         Disable RX LOS  \n
 *
  */

typedef struct _bcm_plp_gallardo28_device_aux_modes_s{
    bcm_plp_gallardo28_datapath_t datapath;
    unsigned int          module_auto_detect_en;
    unsigned int rx_los_en;
}bcm_plp_gallardo28_device_aux_modes_t;

typedef struct bcm_plp_base_t_ability_s {
    bcm_plp_base_t_speed_t  speed_half_duplex;   /* speed abilities for HD */
    bcm_plp_base_t_speed_t  speed_full_duplex;   /* speed abilities for FD */
    bcm_plp_base_t_pause_t  pause;               /* flow control ability   */
    bcm_plp_base_t_eee_ability_t  eee;           /* EEE abilities          */
    bcm_plp_base_t_speed_t  speed_2pair;         /* 2-pair ethernet mode */
    int an_side;                                 /* AN side, 0 - Get local ability */
                                                 /*          1 - Get remote ability*/
} bcm_plp_base_t_ability_t;

/* Cable Diagnostics */

typedef enum bcm_plp_base_t_cable_state_e {
    bcmplpBaseTCableStateOk,
    bcmplpBaseTCableStateOpen,
    bcmplpBaseTCableStateShort,
    bcmplpBaseTCableStateOpenShort,
    bcmplpBaseTCableStateCrosstalk,
    bcmplpBaseTCableStateUnknown,
    bcmplpBaseTCableStateCount      /* last, as always */
} bcm_plp_base_t_cable_state_t;

typedef struct bcm_plp_base_t_cable_diag_s {
    bcm_plp_base_t_cable_state_t    state;          /* state of all pairs        */
    int                             npairs;         /* number of pairs           */
    bcm_plp_base_t_cable_state_t    pair_state[4];  /* pair state                */
    int                             pair_len[4];    /* pair length in meters     */
    int                             fuzz_len;       /* len values +/- this       */
    int                             ecd_unit;       /* cable length unit         */
    int                             cable_type;     /* Cable type for applicable PHYs
                                                       0 = cat6a cable and 10G link partner
                                                       1 = cat5e cable and 10G link partner
                                                       2 = cat6 cable and 10G link partner
                                                       3 = cat6a cable and Gphy link partner
                                                       4 = cat5e and Gphy link partner
                                                     */
    int                             partner_power;  /* link partner power status for applicable PHYs
                                                       For all applicable PHYs
                                                       0 = link partner powered UP
                                                       2 = link partner powered DOWN
                                                     */
} bcm_plp_base_t_cable_diag_t;

/* State/Manufacturing Diag Control  op_type  */
typedef enum bcm_plp_base_t_diag_ctrl_type_e {
    bcmplpBaseTDiagCtrlTypeGet  =  0,    /* a get action */
    bcmplpBaseTDiagCtrlTypeSet  =  1,    /* a set action */
    bcmplpBaseTDiagCtrltypeCmd  =  2,    /* a sequence of command actions */
    bcmplpBaseTDiagCtrlTypeCount
} bcm_plp_base_t_diag_ctrl_type_t;

/* State/Manufacturing Diag Control  op_cmd  */
typedef enum bcm_plp_base_t_diag_ctrl_cmd_e {
    bcmplpBaseTDiagCtrlCmdCableDiag  = 0,
    bcmplpBaseTDiagCtrlCmdDsc        = 0x08000005,
    bcmplpBaseTDiagCtrlCmdMfgHybCanc = 0x0800001B,
    bcmplpBaseTDiagCtrlCmdMfgDenc,
    bcmplpBaseTDiagCtrlCmdMfgTxOn,
    bcmplpBaseTDiagCtrlCmdMfgExit,
    bcmplpBaseTDiagCtrlCmdStateTrace1,
    bcmplpBaseTDiagCtrlCmdStateTrace2,
    bcmplpBaseTDiagCtrlCmdStateWhereAmI,
    bcmplpBaseTDiagCtrlCmdStateTemp,
    bcmplpBaseTDiagCtrlCmdStateGeneric,
    bcmplpBaseTDiagCtrlCmdCount
} bcm_plp_base_t_diag_ctrl_cmd_t;

/*************************************\
** End of BASE_T Specific Defines   **|
\*************************************/


/****************************************\
**  Generic Traffic/Packet properties  **|
\****************************************/

/** Traffic direction **/
typedef enum bcm_plp_traffic_direction_e {
    bcmplpTrafficTxRx         = 0,
    bcmplpTrafficRx           = 0x1,
    bcmplpTrafficTx           = 0x2,
    bcmplpTrafficCount
} bcm_plp_traffic_direction_t;

/** Packet types **/
typedef enum bcm_plp_packet_type_e {
    bcmplpPktTypeNone          = 0x0,       /* none */
    bcmplpPktTypeAll           = 0x1,       /* all type of packets   */
    bcmplpPktTypeCrc           = 0x10,      /* packet with CRC error */
    bcmplpPktTypePtpSync       = 0x100,     /* IEEE1588 PTP Sync            message */
    bcmplpPktTypePtpDelayReq   = 0x200,     /* IEEE1588 PTP Delay  Request  message */
    bcmplpPktTypePtpPdelayReq  = 0x400,     /* IEEE1588 PTP Pdelay Request  message */
    bcmplpPktTypePtpPdelayResp = 0x800,     /* IEEE1588 PTP Pdelay Response message */
    bcmplpPktTypeCount
} bcm_plp_packet_type_t;

/** Math operations for timeers/counters **/
typedef enum bcm_plp_math_operator_e {
    bcmplpMathOperatorNone,
    bcmplpMathOperatorAdd,
    bcmplpMathOperatorSubtract,
    bcmplpMathOperatorAddPre,    /*      Add preset/predefined/previous values */
    bcmplpMathOperatorSubPre,    /* Subtract preset/predefined/previous values */
    bcmplpMathOperatorCount
} bcm_plp_math_operator_t;

/**************************************\
|**  I/O Device Control              **|
\**************************************/

typedef enum  bcm_plp_peripheral_device_e {
    bcmplpPeriDevI2c  = 0x10000     /* I2C  bus */
} bcm_plp_peripheral_device_t;

typedef enum  bcm_plp_peripheral_device_control_e {
    bcmplpPeriDevCtrlDataFlush,    /* flush data            */
    bcmplpPeriDevCtrlBusSpeedGet,  /* get bus speed         */
    bcmplpPeriDevCtrlBusSpeedSet   /* set bus speed         */
} bcm_plp_peripheral_device_control_t;

typedef enum  bcm_plp_peripheral_device_speed_e {
    bcmplpPeriDevI2cBusSpeed100kbps,   /* I2C bus speed 100Kbps  */
    bcmplpPeriDevI2cBusSpeed400kbps    /* I2C bus speed 400Kbps  */
} bcm_plp_peripheral_device_speed_t;

typedef struct bcm_plp_peripheral_dev_ctrl_data_s {
    int   value;
} bcm_plp_peripheral_dev_ctrl_data_t;

/**************************************\
|**  1588 TimeSync Specific Defines  **|
\**************************************/

typedef enum  bcm_plp_timesync_mpls_option_e {
    bcmplpTimesyncMplsNone     = 0x0,
    bcmplpTimesyncMplsEnable   = 0x1,
    bcmplpTimesyncMplsEntropy  = 0x2,
    bcmplpTimesyncMplsSpecial  = 0x4,
    bcmplpTimesyncMplsCtrlWord = 0x8
} bcm_plp_timesync_mpls_option_t;

typedef struct bcm_plp_timesync_mpls_label_s {
    unsigned int value;     /* MPLS label bits [19:0] */
    unsigned int mask;      /* MPLS label mask bits [19:0] */
    unsigned int flags;     /* MPLS label flags */
} bcm_plp_timesync_mpls_label_t;

typedef struct bcm_plp_timesync_mpls_ctrl_s {
    unsigned int flags;                             /* Flags */
    unsigned int special_label;                     /* bits [19:0] */
    bcm_plp_timesync_mpls_label_t labels[10];   /* Timesync MPLS labels */
    int size;                           /* Number of elements in label array */
} bcm_plp_timesync_mpls_ctrl_t;

typedef enum bcm_plp_timesync_inband_filter_flags_e {
    bcmplpTimesyncFilterFlagMatchSrc     = (1U <<  0),
    bcmplpTimesyncFilterFlagMatchDst     = (1U <<  1),
    bcmplpTimesyncFilterFlagMatchPtpver  = (1U <<  2),
    bcmplpTimesyncFilterFlagMatchMacIp   = (1U <<  3)
} bcm_plp_timesync_inband_filter_flags_t;

typedef enum bcm_plp_timesync_inband_filter_action_e {
    bcmplpTimesyncFilterActionNone                 =  0,
    bcmplpTimesyncFilterActionInband36             =  1,
    bcmplpTimesyncFilterActionInband32             =  2,
    bcmplpTimesyncFilterActionMdioNoInband         =  3,
    bcmplpTimesyncFilterActionInband32PtpverEgress =  4
} bcm_plp_timesync_inband_filter_action_t;

typedef struct bcm_plp_mac_ip_addr_s {
    unsigned char ip_addr[4];   /* ipv4 address */
    unsigned char mac_addr[6];  /* mac address */
} bcm_plp_mac_ip_addr_t;

/*! Inband Filter Configuration
 *
 * \arg match_addr \n
 *  mac address or ipv4 address to match and apply filter
 *  set using bcm_plp_mac_ip_addr_t
 *
 * \arg msg_type \n
 *  unused
 *
 * \arg flags \n
 *  flags to enable to apply filter on
 *  set using bcm_plp_timesync_inband_filter_flags_t
 *  bcmplpTimesyncFilterFlagMatchPtpver is for applicable PHYs only
 *
 * \arg reserved \n
 *  reserved
 *
 * \arg action \n
 *  Timesync action after matching filter criteria
 *  set using bcm_plp_timesync_inband_filter_action_t
 *  bcmplpTimesyncFilterActionInband32PtpverEgress is for applicable PHYs only
 *
 * \arg valid \n
 *  validate or invalidate filter entry
 *   0 : invalid/null filter entry
 *   1 : valid filter entry
 *
 */

typedef struct bcm_plp_timesync_inband_filter_ctrl_s {
    bcm_plp_mac_ip_addr_t match_addr;
    unsigned short msg_type;
    unsigned int flags;
    unsigned short reserved;
    unsigned short action;
    unsigned short valid;
} bcm_plp_timesync_inband_filter_ctrl_t;

/*
 *   TimeSync enums
 */

typedef enum  bcm_plp_timesync_interrupt_e {    /* IEEE1588 TimeSync Interrupts */
    bcmplpTimesyncIntrMASK   = 0x7F000000,
    bcmplpTimesyncIntrPic    = bcmplpBaseTIntrPtpPic    | bcmplpTimesyncIntrMASK,
    bcmplpTimesyncIntrFsync  = bcmplpBaseTIntrPtpFsync  | bcmplpTimesyncIntrMASK,
    bcmplpTimesyncIntrSop    = bcmplpBaseTIntrPtpSop    | bcmplpTimesyncIntrMASK,
    bcmplpTimesyncIntrSyncIn = bcmplpBaseTIntrPtpSyncIn | bcmplpTimesyncIntrMASK,
    bcmplpTimesyncIntrIpg    = bcmplpBaseTIntrPtpIpg    | bcmplpTimesyncIntrMASK,
    bcmplpTimesyncIntrCount
} bcm_plp_timesync_interrupt_t;

typedef enum bcm_plp_timesync_valid_e {
    bcmplpTimesyncValidFlags                 = (1U <<  0),
    bcmplpTimesyncValidItpid                 = (1U <<  1),
    bcmplpTimesyncValidOtpid                 = (1U <<  2),
    bcmplpTimesyncValidOtpid2                = (1U <<  3),
    bcmplpTimesyncValidGmode                 = (1U <<  4),
    bcmplpTimesyncValidFramesyncMode         = (1U <<  5),
    bcmplpTimesyncValidSyncOutMode           = (1U <<  6),
    bcmplpTimesyncValidTsDivider             = (1U <<  7),
    bcmplpTimesyncValidOriginalTimecode      = (1U <<  8),
    bcmplpTimesyncValidTxTimestampOffset     = (1U <<  9),
    bcmplpTimesyncValidRxTimestampOffset     = (1U << 10),
    bcmplpTimesyncValidTxSyncMode            = (1U << 11),
    bcmplpTimesyncValidTxDelayRequestMode    = (1U << 12),
    bcmplpTimesyncValidTxPdelayRequestMode   = (1U << 13),
    bcmplpTimesyncValidTxPdelayResponseMode  = (1U << 14),
    bcmplpTimesyncValidRxSyncMode            = (1U << 15),
    bcmplpTimesyncValidRxDelayRequestMode    = (1U << 16),
    bcmplpTimesyncValidRxPdelayRequestMode   = (1U << 17),
    bcmplpTimesyncValidRxPdelayResponseMode  = (1U << 18),
    bcmplpTimesyncValidMplsControl           = (1U << 19),
    bcmplpTimesyncValidRxLinkDelay           = (1U << 20),
    bcmplpTimesyncValid1588SyncFreq          = (1U << 21),
    bcmplpTimesyncValid1588DpllK1            = (1U << 22),
    bcmplpTimesyncValid1588DpllK2            = (1U << 23),
    bcmplpTimesyncValid1588DpllK3            = (1U << 24),
    bcmplpTimesyncValid1588DpllLoopFilter    = (1U << 25),
    bcmplpTimesyncValid1588DpllRefPhase      = (1U << 26),
    bcmplpTimesyncValid1588DpllRefPhaseDelta = (1U << 27),
    bcmplpTimesyncValid1588InbandTsControl   = (1U << 28),
    bcmplpTimesyncValidCount
} bcm_plp_timesync_valid_t;

typedef enum bcm_plp_timesync_flag_e {
    bcmplpTimesyncFlagCaptureStampEnable            = (1U<<0),  /* Enable TimeStamp capturing */
    bcmplpTimesyncFlagHeartbeatStampEnable          = (1U<<1),  /* Capture HeartBeat on next FrameSync event */
    bcmplpTimesyncFlagRXCrcEnable                   = (1U<<2),  /* Enable the CRC check in ingress PTP messages */
    bcmplpTimesyncFlag8021asEnable                  = (1U<<3),  /* Enable 802.1AS MAC DA check for PTP messages */
    bcmplpTimesyncFlagL2Enable                      = (1U<<4),  /* Enable the L2  MAC DA check for PTP messages */
    bcmplpTimesyncFlagIp4Enable                     = (1U<<5),  /* Enable the L4 IPv4 DA check for PTP messages */
    bcmplpTimesyncFlagIp6Enable                     = (1U<<6),  /* Enable the L4 IPv6 DA check for PTP messages */
    bcmplpTimesyncFlagClockSrcExt                   = (1U<<7),  /* Reference clock source:  0=internal  1=external */
    bcmplpTimesyncFlagClockSrcExtMode               = (1U<<8),  /* External Reference clock source: 0=PHY_REFCLK 1=PLP_XTAL_CLK */
    bcmplpTimesyncFlag1588EncryptedMode             = (1U<<9),  /* Enable IEEE1588 Encrypted Mode */
    bcmplpTimesyncFlagFollowUpAssistEnable          = (1U<<10), /* Enable Follow-Up Assist */
    bcmplpTimesyncFlagDelayRespAssistEnable         = (1U<<11), /* Enable Delay-Response Assist */
    bcmplpTimesyncFlag64BitTimestampEnable          = (1U<<12), /* Enable 64-bit TimeStamp */
    bcmplpTimesyncFlag1588OverHsrEnable             = (1U<<13), /* Enable HSR Parser & LLC Check */
    bcmplpTimesyncFlagCaptureTimestampTxSync        = (1U<<14), /* Capture TimeStamp for egress PTP Sync messages */
    bcmplpTimesyncFlagCaptureTimestampTxDelayReq    = (1U<<15), /* Capture TimeStamp for egress PTP DelayReq messages */
    bcmplpTimesyncFlagCaptureTimestampTxPdelayReq   = (1U<<16), /* Capture TimeStamp for egress PTP PDelayReq messages */
    bcmplpTimesyncFlagCaptureTimestampTxPdelayResp  = (1U<<17), /* Capture TimeStamp for egress PTP PDelayResp messages */
    bcmplpTimesyncFlagCaptureTimestampRxSync        = (1U<<18), /* Capture TimeStamp for ingress PTP Sync messages */
    bcmplpTimesyncFlagCaptureTimestampRxDelayReq    = (1U<<19), /* Capture TimeStamp for ingress PTP DelayReq messages */
    bcmplpTimesyncFlagCaptureTimestampRxPdelayReq   = (1U<<20), /* Capture TimeStamp for ingress PTP PDelayReq messages */
    bcmplpTimesyncFlagCaptureTimestampRxPdelayResp  = (1U<<21), /* Capture TimeStamp for ingress PTP PDelayResp messages */
    bcmplpTimesyncFlagTXCrcEnable                   = (1U<<22), /* Enable the CRC check in egress PTP messages */
    bcmplpTimesyncDisabled                          = (1U<<23), /* NSE/PM TimeStamping disabled */
    bcmplpTimesyncNseEnabled                        = (1U<<24), /* NSE-TS for both egress and Ingress */
    bcmplpTimesyncNsePmEnabled                      = (1U<<25), /* NSE-TS for egress and  PM-TS for Ingress */
    bcmplpTimesyncPmEnabled                         = (1U<<26), /* PM-TS for egress and  PM-TS for Ingress */
    bcmplpTimesyncPmNseEnabled                      = (1U<<27), /* PM-TS for egress and NSE-TS for Ingress */
    bcmplpTimesyncNse2SEnabled                      = (1U<<28), /* NSE-TS (B0 Plan A) for Egress & Ingress (packets encrypted) - 2 Stage */
    bcmplpTimesyncNsePm2SEnabled                    = (1U<<29), /* NSE-TS (B0 Plan A) for Egress & PM-TS for Ingress (packets encrypted) - 2 Stage */
    bcmplpTimesyncNseE2EEnabled                     = (1U<<30), /* NSE-TS (B0 Plan B) for Egress & Ingress (packets encrypted) - E2E */
    bcmplpTimesyncNsePmE2EEnabled                   = (1U<<31), /* NSE-TS (B0 Plan B) for Egress, PM-TS for Ingress (packets encrypted)- E2E */
    bcmplpTimesyncFlagCount
} bcm_plp_timesync_flag_t;

/* Traffic direction */
typedef enum bcm_plp_timesync_txrx_e {
    bcmplpTimesyncTxRx                       = 0,    /* both ingress & egress */
    bcmplpTimesyncRx                         = (1U <<  0),  /* ingress direction */
    bcmplpTimesyncTx                         = (1U <<  1),  /* egress  direction */
    bcmplpTimesyncTxRxCount
} bcm_plp_timesync_txrx_t;

/* TimeSync enable control option */
typedef enum bcm_plp_timesync_enable_ctrl_e {
    bcmplpTimesyncEnCtrlTxSliceEn            = (1U <<  0), /* enable TimeSync-1588 Tx */
    bcmplpTimesyncEnCtrlRxSliceEn            = (1U <<  1), /* enable TimeSync-1588 Rx */
    bcmplpTimesyncEnCtrlNseTimerClkEn        = (1U <<  2), /* enable 1588 timer clock */
    bcmplpTimesyncEnCtrlPchTimerClkEn        = (1U <<  3), /* enable PCH timer clock  */
    bcmplpTimesyncEnCtrlSelI2ccfgSync        = (1U <<  4), /* SyncIn/Out definition   */
    bcmplpTimesyncEnCtrlMode1gOvrd           = (1U <<  5), /* Enables 1G speed mode override  */
    bcmplpTimesyncEnCtrlMode1gOv             = (1U <<  6), /* 1G speed mode overrride value   */
    bcmplpTimesyncEnCtrlMode10gOvrd          = (1U <<  7), /* Enables 10G speed mode override */
    bcmplpTimesyncEnCtrlMode10gOv            = (1U <<  8), /* 10G speed mode overrride value  */
    bcmplpTimesyncEnCtrlOtpDis               = (1U <<  9), /* disable 1588 otp   */
    bcmplpTimesyncEnCtrlNseTimerNewClk       = (1U << 10), /* Select timer clock */
    bcmplpTimesyncEnCtrlUsxgmiiCarrierExtDis = (1U << 11), /* Disable Carrier Extend marker */
    bcmplpTimesyncEnCtrlReserved             = (1U << 12), /* reserved */
    bcmplpTimesyncEnCtrlSwRstbNse            = (1U << 13), /* Soft reset for 1588 timer block only */
    bcmplpTimesyncEnCtrlSwRstbReg            = (1U << 14), /* Soft reset for 1588 register block   */
    bcmplpTimesyncEnCtrlSwRstb               = (1U << 15), /* Soft reset for 1588 block,datapath & state machine */
    bcmplpTimesyncEnCtrlFlag                 = (1U << 31), /* flag to indicate enabling control    */
    bcmplpTimesyncEnCtrlCount
} bcm_plp_timesync_enable_ctrl_t;

typedef enum bcm_plp_timesync_timer_mode_e {
    bcmplpTimesyncTimerModeNone    = 0x0,
    bcmplpTimesyncTimerModeDefault = 0x1,
    bcmplpTimesyncTimerMode32Bit   = 0x2,
    bcmplpTimesyncTimerMode48Bit   = 0x4,
    bcmplpTimesyncTimerMode64Bit   = 0x8,
    bcmplpTimesyncTimerMode80Bit   = 0x10,
    bcmplpTimesyncTimerModeCount
} bcm_plp_timesync_timer_mode_t;

typedef enum bcm_plp_timesync_global_mode_e {
    bcmplpTimesyncGLobalModeFree   = 0x0,
    bcmplpTimesyncGLobalModeSyncIn = 0x1,
    bcmplpTimesyncGLobalModeCpu    = 0x2,
    bcmplpTimesyncGLobalModeCount
} bcm_plp_timesync_global_mode_t;

typedef enum bcm_plp_timesync_syncmode_ctrl_e {
    bcmplpTimesyncSyncmodeCtrlNone           = 0x0,
    bcmplpTimesyncSyncmodeCtrlNseSyncOutPort = (1U << 6),  /* NSE to drive its associated SynOut port */
    bcmplpTimesyncSyncmodeCtrlResetSyncIn    = (1U << 7),  /* Reset SyncIn  FSM back to Idle state    */
    bcmplpTimesyncSyncmodeCtrlResetSyncOut   = (1U << 8),  /* Reset SyncOut FSM back to Idle state    */
    bcmplpTimesyncSyncmodeCtrlResetLock      = (1U << 9),  /* Reset lock FSM back to Idle state       */
    bcmplpTimesyncSyncmodeCtrlReserved       = (1U << 10), /* reserved */
    bcmplpTimesyncSyncmodeCtrlLocalSyncDis   = (1U << 11), /* Disable SyncOut and treat as local SyncIn */
    bcmplpTimesyncSyncmodeCtrlNseInit        = (1U << 12), /* Initial NSE block                         */
    bcmplpTimesyncSyncmodeCtrlTsCapture      = (1U << 13), /* Timestamp to be captured by on FrameSync  */
    bcmplpTimesyncSyncmodeCtrlCount
} bcm_plp_timesync_syncmode_ctrl_t;

typedef enum bcm_plp_timesync_syncout_mode_s {
    bcmplpTimesyncSyncoutModeDisable        = 0, /* SynIn/SynOut pins are input  */
    bcmplpTimesyncSyncoutModeOneTime        = 1, /* One time pulse on a match with timestamp reg */
    bcmplpTimesyncSyncoutModePulseTrain     = 2, /* Generate a pulse train  */
    bcmplpTimesyncSyncoutModePulseTrainSync = 3, /* Both one time pulse & pulse train */
    bcmplpTimesyncSyncoutModeCount
} bcm_plp_timesync_syncout_mode_t;

typedef enum bcm_plp_timesync_framesync_mode_e {
    bcmplpTimesyncFramsyncModeNone    = 0x0,
    bcmplpTimesyncFramsyncModeSyncIn0 = 0x1,
    bcmplpTimesyncFramsyncModeSyncIn1 = 0x2,
    bcmplpTimesyncFramsyncModeSyncOut = 0x3,
    bcmplpTimesyncFramsyncModeCpu     = 0x4,
    bcmplpTimesyncFramsyncModeCount
} bcm_plp_timesync_framesync_mode_t;

typedef enum bcm_plp_timesync_event_msg_action_e {
    bcmplpTimesyncEventMsgActionNone,
    bcmplpTimesyncEventMsgActionEgrModeUpdateCorrectionField,
    bcmplpTimesyncEventMsgActionEgrModeReplaceCorrectionFieldOrigin,
    bcmplpTimesyncEventMsgActionEgrModeCaptureTimestamp,
    bcmplpTimesyncEventMsgActionIngModeUpdateCorrectionField,
    bcmplpTimesyncEventMsgActionIngModeInsertTimestamp,
    bcmplpTimesyncEventMsgActionIngModeInsertDelaytime,
    bcmplpTimesyncEventMsgActionCount
} bcm_plp_timesync_event_msg_action_t;

typedef enum bcm_plp_timesync_inband_option_e {
    bcmplpTimesyncInbandResv0IdCheck     = (1U << 0),
    bcmplpTimesyncInbandSyncEnable       = (1U << 1),
    bcmplpTimesyncInbandDelayReqEnable   = (1U << 2),
    bcmplpTimesyncInbandPdelayReqEnable  = (1U << 3),
    bcmplpTimesyncInbandPdelaYRespEnable = (1U << 4),
    bcmplpTimesyncInbandResv0IdUpdate    = (1U << 5),
    bcmplpTimesyncInbandCapSrcPortClkId  = (1U << 6),
    bcmplpTimesyncInbandMatchVlanId      = (1U << 7),
    bcmplpTimesyncInbandMatchSrcPortNum  = (1U << 8),
    bcmplpTimesyncInbandMatchMacAddr     = (1U << 9),
    bcmplpTimesyncInbandMatchIpAddr      = (1U << 10),
    bcmplpTimesyncInbandFollowUpAssit    = (1U << 11),
    bcmplpTimesyncInbandDelayRespAssit   = (1U << 12),
    bcmplpTimesyncInbandTimerModeSelect  = (1U << 13),
    bcmplpTimesyncInbandOptCount
} bcm_plp_timesync_inband_option_t;

/* Parse fields in packet to write to or compare from SOPmem*/
typedef enum bcm_plp_timesync_inband_parse_e {
    bcmplpTimesyncInbandParseSrcPort   = (1U << 0),  /* parse source port */
    bcmplpTimesyncInbandParseMplsLabel = (1U << 1),  /* parse MPLS label  */
    bcmplpTimesyncInbandParseMacDa     = (1U << 2),  /* parse MAC DA */
    bcmplpTimesyncInbandParseMacSa     = (1U << 3),  /* parse MAC SA */
    bcmplpTimesyncInbandParseIpv4DstIp = (1U << 4),  /* parse IPv4 dest IP   */
    bcmplpTimesyncInbandParseIpv4SrcIp = (1U << 5),  /* parse IPv4 Source IP */
    bcmplpTimesyncInbandParseResv2     = (1U << 6),  /* parse Resv2 field    */
    bcmplpTimesyncInbandParseIpv6DstIp = (1U << 7),  /* parse IPv6 Dest IP   */
    bcmplpTimesyncInbandParseCount
} bcm_plp_timesync_inband_parse_t;


/* PTP 1588 Shadow Registers Load Control bits */
typedef enum bcm_plp_timesync_load_ctrl_e {
    bcmplpTimesyncLoadCtrlTsCorrect      = (1U << 0),  /* Timestamp correction */
    bcmplpTimesyncLoadCtrlTimeCode       = (1U << 1),  /* Time code control */
    bcmplpTimesyncLoadCtrlSyncOut        = (1U << 2),  /* Sync out          */
    bcmplpTimesyncLoadCtrlNcoDivider     = (1U << 3),  /* NCO divider       */
    bcmplpTimesyncLoadCtrlLocalTime      = (1U << 4),  /* 48-bit Local time */
    bcmplpTimesyncLoadCtrlNcoAddend      = (1U << 5),  /* NCO addend        */
    bcmplpTimesyncLoadCtrlDpllLoopFilter = (1U << 6),  /* DPLL loop filter  */
    bcmplpTimesyncLoadCtrlDpllRefPhase   = (1U << 7),  /* DPLL ref phase    */
    bcmplpTimesyncLoadCtrlDpllRefDelta   = (1U << 8),  /* DPLL ref phase delta  */
    bcmplpTimesyncLoadCtrlDpllK3         = (1U << 9),  /* DPLL K3 */
    bcmplpTimesyncLoadCtrlDpllK2         = (1U << 10), /* DPLL K2 */
    bcmplpTimesyncLoadCtrlDpllK1         = (1U << 11)  /* DPLL K1 */
} bcm_plp_timesync_load_ctrl_t;

/* PTP 1588 Heartbeat Capture Modes */
typedef enum bcm_plp_timesync_heartbeat_capture_e {
    bcmplpTimesyncHeartbeatCaptureLc,    /* select nse_reg_ts_captime for read back */
    bcmplpTimesyncHeartbeatCaptureTc,    /* select time_code_captured for read back */
    bcmplpTimesyncHeartbeatCaptureTcNtp, /* select time_code_ntp_captured for read back */
    bcmplpTimesyncHeartbeatCaptureLt,    /* select nco_reg_ts_captime for read back */
    bcmplpTimesyncHeartbeatCaptureCount
} bcm_plp_timesync_heartbeat_capture_t;

/* PTP 1588 TimeSync Control timers/counters */
typedef enum bcm_plp_timesync_timing_control_e {
    bcmplpTimesyncTimerTypeDpllRefPhase         = 0x0021,  /* NSE DPLL Reference Phase             */
    bcmplpTimesyncTimerTypeDpllRefDelta         = 0x0024,  /* NSE DPLL Reference Delta Phase       */
    bcmplpTimesyncTimerTypeDpllK3K2K1           = 0x0026,  /* NSE DPLL K3 / K2 /K1                 */
    bcmplpTimesyncTimerTypeDpllInitLpFlt        = 0x0029,  /* NSE DPLL initial loopfilter          */
    bcmplpTimesyncTimerTypeNcoFreqStep          = 0x002E,  /* NCO Frequency Stepping control       */
    bcmplpTimesyncTimerTypeSyncLocalTimer       = 0x0030,  /* NCO Sync value of 48-bit local timer */
    bcmplpTimesyncTimerTypeNtpFreqCtrl          = 0x00A8,  /* NTP Frequency control word           */
    bcmplpTimesyncTimerTypeNTPDownCntr          = 0x00AE,  /* NTP Down Counter control word        */
    bcmplpTimesyncTimerTypeNseCtrOffsetLo       = 0x00EB,  /* NSE counter offset bit[79-64]        */
    bcmplpTimesyncTimerTypeNseCtrOffsetHi       = 0x00EF,  /* NSE counter offset bit[63-00]        */
    bcmplpTimesyncTimerTypeRefClkMode           = 0x60000B09,  /*  PTP reference clock for 1G PHYs */
    bcmplpTimesyncTimerTypeTimeOfDay48          = 0x600070D6,  /*  48-bit Time of Day (ToD)        */
    bcmplpTimesyncTimerTypeTimeOfDayTSDpll80    = 0x600070D7,  /*  80-bit Time of Day TimeStamp (ToD) and TOD DPLL  */
    bcmplpTimesyncTimerTypeTimeOfDayTSDpll48    = 0x600070D8,  /*  48-bit Time of Day TimeStamp (ToD) and TOD DPLL  */
    bcmplpTimesyncTimerTypeCount
} bcm_plp_timesync_timing_control_t;

/* PTP 1588 TimeSync reference clock mode */
typedef enum bcm_plp_timesync_ref_clk_mode_e {
    bcmplpTimesyncRefClkModeNone        = 0,
    bcmplpTimesyncRefClkModeInternal    = (1U << 0),    /* internal PTP reference clock */
    bcmplpTimesyncRefClkModeExternal    = (1U << 1),    /* external PTP reference clock */
    bcmplpTimesyncRefClkModeActiveHigh  = (1U << 2),    /* PTP clock is active-high     */
    bcmplpTimesyncRefClkModeAsync       = (1U << 3)     /* Asynchronous clocks          */
} bcm_plp_timesync_ref_clk_mode_t;

/* PTP 1588 SOPmem (start of packet memory) */
typedef enum bcm_plp_timesync_sopmem_field_e {
    bcmplpTimesyncSOPmemValid          = (1U << 0),  /* entry is valid or not    */
    bcmplpTimesyncSOPmemDirection      = (1U << 1),  /* Rx/Tx direction          */
    bcmplpTimesyncSOPmemMsgType        = (1U << 2),  /* PTP message type         */
    bcmplpTimesyncSOPmemSeqId          = (1U << 3),  /* sequence ID              */
    bcmplpTimesyncSOPmemDomainNum      = (1U << 4),  /* domain number            */
    bcmplpTimesyncSOPmemVlanId         = (1U << 5),  /* virtual LAN ID           */
    bcmplpTimesyncSOPmemSrcPort        = (1U << 6),  /* source TCP/UDP port      */
    bcmplpTimesyncSOPmemClassifiedData = (1U << 7),  /* classified data such as  *\
                                                     \*  MAC or IP addr or MPLS  */
    bcmplpTimesyncSOPmemSrcIP          = bcmplpTimesyncSOPmemClassifiedData,
    bcmplpTimesyncSOPmemTimestamp      = (1U << 8)   /* timestamp                */
} bcm_plp_timesync_sopmem_field_t;

typedef enum bcm_plp_timesync_sopmem_option_e {
    bcmplpTimesyncSOPmemOpPchFifo         =  0,         /* PCH FIFO access             */
    bcmplpTimesyncSOPmemOpNoSkipInvalid   = (1U << 0),  /* not to skip invalid entries */
    bcmplpTimesyncSOPmemOpClearOne        = (1U << 1),  /* clear the SOPmem entry hit  */
    bcmplpTimesyncSOPmemOpClearAll        = (1U << 2),  /* clear all SOPmem entries    */
    bcmplpTimesyncSOPmemOpRegular         = (1U << 7),  /* regular  SOPmem operations  */
    bcmplpTimesyncSOPmemOpClassify        = (1U << 8)   /* classify SOPmem data        */
} bcm_plp_timesync_sopmem_option_t;

typedef enum bcm_plp_timesync_stamping_e {
    bcmplpTimesyncStampingNse    =  0       ,  /* NSE datapath timestamping */
    bcmplpTimesyncStampingPmTx   = (1U << 0),  /* Tx PortMacro timestamping */
    bcmplpTimesyncStampingPmRx   = (1U << 1),  /* Rx PortMacro timestamping */
    bcmplpTimesyncStampingBypass = (1U << 3)   /* bypass P1588 timestamping */
} bcm_plp_timesync_stamping_t;

typedef enum  bcm_plp_timesync_univ_timer_e {
    bcmplpTimesyncUnivTimerReserved   =  0,          /* reserved timer type */
    bcmplpTimesyncUnivPchSystemTime   =  (1U << 1),  /* PCH system timers   */
    bcmplpTimesyncUnivHeartbeat80bit  =  (1U << 2)   /* 80-bit HeartBeat    */
} bcm_plp_timesync_univ_timer_t;

typedef enum  bcm_plp_timesync_pch_enable_e {
    bcmplpTimesyncPchNone    = 0,           /* disable PCH               */
    bcmplpTimesyncPchRx      = (1U << 0),   /* ingress direction         */
    bcmplpTimesyncPchTx      = (1U << 1),   /* egress  direction         */
    bcmplpTimesyncPchGen1    = (1U << 2),   /* PCH Gen1 support          */
    bcmplpTimesyncPchIet     = (1U << 3)    /* IET (preemption) support  */
} bcm_plp_timesync_pch_enable_t;

typedef enum  bcm_plp_timesync_pch_mode_e {
    bcmplpTimesyncPchModeTsCap       = (1U << 0),  /* Egress SOP timestamp capture   */
    bcmplpTimesyncPchModeTypeChk     = (1U << 1),  /* Packet_Type check in egress    */
    bcmplpTimesyncPchModeExtFieldChk = (1U << 2)   /* Ext_Field_Type check in egress */
} bcm_plp_timesync_pch_mode_t;

typedef enum  bcm_plp_timesync_pch_pktype_e {
    bcmplpTimesyncPchPktPch = 0,   /* with    PCH */
    bcmplpTimesyncPchPktNone,      /* without PCH */
    bcmplpTimesyncPchPktIdle,      /* idle packet */
    bcmplpTimesyncPchPktIet        /* IET (preemption) frame */
} bcm_plp_timesync_pch_pktype_t;

typedef enum  bcm_plp_timesync_pch_extension_e {
    bcmplpTimesyncPchExtIgnore = 0,   /* ignore extension field */
    bcmplpTimesyncPchExtTs32,         /* 32-bit timestamp       */
    bcmplpTimesyncPchExtTs24          /* 24-bit timestamp       */
} bcm_plp_timesync_pch_extension_t;

typedef enum  bcm_plp_pch_systime_type_e {
    bcmplpTimesyncPchSystimeNone    = 0,
    bcmplpTimesyncPchSystimeBase    = (1U << 0),  /* base system time       */
    bcmplpTimesyncPchSystimeOffset  = (1U << 1),  /* offset adjust          */
    bcmplpTimesyncPchSystimeDrift   = (1U << 2),  /* drift  adjust          */
    bcmplpTimesyncPchSystimeCapture = (1U << 3)   /* snapshot system time   */
} bcm_plp_pch_systime_type_t;

/*
 *   TimeSync data structures
 */
#ifdef PHYMOD_TIMESYNC_PROPERTY_USE_UINT32
typedef unsigned int     timesync_property_t;
#else
typedef unsigned char    timesync_property_t;
#endif

typedef struct  bcm_plp_timesync_pch_s {
    bcm_plp_timesync_pch_enable_t     en;         /* enable Rx/Tx/IET PCH */
    bcm_plp_timesync_pch_mode_t       mode;       /* PCH mode             */
    bcm_plp_timesync_pch_pktype_t     pkt_type;   /* PCH frame type       */
    bcm_plp_timesync_pch_extension_t  ext_field;  /* PCH extension field  */
    unsigned int                      subport;    /* channel number       */
} bcm_plp_timesync_pch_t;

typedef struct  bcm_plp_timestamp_univ_s {
    int           op;                   /* operations for the timer                   */
    plp_uint64_t  nanosecond;           /* low bits [63:00] of the nanosecond value   */
    plp_uint64_t  subnanosecond;        /* sub-nanosecond value                       */
    plp_uint64_t  hi_nanosecond;        /* high bits [127:64] of the nanosecond value */
} bcm_plp_timestamp_univ_t;

typedef struct bcm_plp_timesync_sop_timestamp_capture_s {
    timesync_property_t         dp_ts_wclk;  /* update datapath timestamp from wall clock */
    timesync_property_t         err_ecc2pkt; /* contribute ECC error to packet error      */
    bcm_plp_timesync_stamping_t stamping;    /* NSE/Tx_PM/Rx_PM/bypass timestamping       */
    timesync_property_t         lov;         /* low variation (LOV) mode on Rx            */
    timesync_property_t         ts_cap;      /* enable the timestamp capture on Tx/Rx     */
}  bcm_plp_timesync_sop_timestamp_capture_t;

/* TimeSync 1588 inband properties */
typedef struct bcm_plp_timesync_inband_property_s {
    timesync_property_t  spare;             /* Spare */
    timesync_property_t  clear_rsv012;      /* clear all reserved fields */
    timesync_property_t  ins_4bytes;        /* insert {2-bit sec, 30-bit ns} after 1588 header */
    timesync_property_t  ts32_format;       /* inband using 32 bit format {2-bit sec, 30-bit ns} */
    timesync_property_t  strict;            /*  strict mode */
    timesync_property_t  mdio_sopmem;       /* revert back to Gen1 mode */
    timesync_property_t  ts_80bits;         /* support only 80-bit timestamp */
    timesync_property_t  partial_tc_mode;   /* partial TC mode - compute CF with RXSOP from reserved field */
    timesync_property_t  tc_mode;           /* TC mode */
    timesync_property_t  update_resv0;      /* update Resv0_ID into Resv0 field in PTP header */
    timesync_property_t  check_resv0;       /* check Resv0_ID with Resv0 field in PTP header */
    timesync_property_t  inband_on;         /* enable Inband operation */
    timesync_property_t  resv0_id;          /* Resv0_ID */
    bcm_plp_timesync_inband_parse_t  write_sopmem;   /* select field to write to SOPmem for classification */
    bcm_plp_timesync_inband_parse_t  compare_sopmem; /* select which field to compare when read SOPmem */
    timesync_property_t  cmp_domain_num;    /* enable compare domain_num */
    timesync_property_t  cmp_seq_num;       /* enable compare seq_num */
    timesync_property_t  cmp_src_port;      /* enable compare source port */
    timesync_property_t  cmp_field_sel;     /* enable compare fields selected */
    timesync_property_t  cmp_vlan_id;       /* enable compare VLAN ID */
} bcm_plp_timesync_inband_property_t;

/* TimeSync 1588 inband control data structure */
typedef struct bcm_plp_timesync_inband_ctrl_s {
    unsigned int flags;                         /* Flags */
    int resv0_id;                               /* Reserved ID */
    bcm_plp_timesync_timer_mode_t timer_mode;   /* Timer mode */
} bcm_plp_timesync_inband_ctrl_t;

/* TimeSync 1588 Tx/Rx option select */
typedef struct bcm_plp_timesync_option_s {
    timesync_property_t  force_ts_to_sopmem;    /* Capture TS into SOPmem regardless CRC */
    timesync_property_t  ptpv2_chk_dis;         /* Disable the TX PTP version 2 check    */
    timesync_property_t  timecode_to_insertion; /* Add timecode into the insertion field */
    timesync_property_t  keep_ori_crc;          /* Keep the original CRC on Tx/Rx        */
} bcm_plp_timesync_option_t;

/* TimeSync 1588 FrameSync control data structure */
typedef struct bcm_plp_timesync_framesync_s {
    bcm_plp_timesync_framesync_mode_t mode;     /* Framesync Mode */
    unsigned int length_threshold;              /* Threshold */
    unsigned int event_offset;                  /* Even offset */
    unsigned int flags;                         /* NSE options */
    bcm_plp_timesync_global_mode_t    gmode;    /* NSE global mode */
    bcm_plp_timesync_syncout_mode_t   syncout_mode; /* SyncOut mode */
} bcm_plp_timesync_framesync_t;

/* TimeSync 1588 SyncOut control data structure */
typedef struct bcm_plp_timesync_syncout_s {
    bcm_plp_timesync_syncout_mode_t mode;
    unsigned short pulse_1_length;    /* Pulse 1 length in nanoseconds */
    unsigned short pulse_2_length;    /* Pulse 2 length in nanoseconds */
    unsigned int interval;            /* Interval in nanoseconds */
    plp_uint64_t timestamp;     /* Syncout timestamp */
} bcm_plp_timesync_syncout_t;

/* TimeSync 1588 time code data structure */
typedef struct bcm_plp_timesync_timespec_s {
    unsigned char      isnegative;    /* Sign identifier */
    plp_uint64_t seconds;       /* Seconds absolute value     */
    unsigned int       nanoseconds;   /* Nanoseconds absolute value */
} bcm_plp_timesync_timespec_t;

/* TimeSync 1588 time value data structure */
typedef struct bcm_plp_timesync_time_value_s {
    unsigned int        direction;          /* Tx or Rx direction       */
    unsigned int        op;                 /* addition or subtraction  */
    unsigned int        type;               /* PTP message/counter/timer type */
    plp_uint64_t        time_value;         /* Time value   */
    plp_uint64_t        time_value_hi;      /* Time value high(bit64-bit80) for 80-bit   */
    plp_uint64_t        dpll_time_value;    /* DPLL time value */
    plp_uint64_t        dpll_time_value_hi; /* DPLL time high(bit64-bit80) for 80-bit   */
} bcm_plp_timesync_time_value_t;

/* bcm_plp_timesync_event_msg_action_t validation */
typedef struct bcm_plp_timesync_timer_adjust_s {
    bcm_plp_timesync_timer_mode_t mode;         /* Timer mode */
    int delta;                                  /* Delta */
} bcm_plp_timesync_timer_adjust_t;

/* PTP 1588 packet count selection */
typedef struct bcm_plp_timesync_pkt_cnt_sel_s {
    unsigned char  crc_cnt_sel;     /* count CRC error packets */
    unsigned int   tx_pkt_cnt_sel;  /* select packet type to be counted on Tx */
    unsigned int   rx_pkt_cnt_sel;  /* select packet type to ve counted on Rx */
} bcm_plp_timesync_pkt_cnt_sel_t;

#define BCMPLP_TIMESYNC_SOPMEM_CLASSIFIED_DATA_SIZE         8
#define BCMPLP_TIMESYNC_SOPMEM_TO_BE_READ_BYTE              0xFF

/* PTP 1588 SOPmem (start of packet memory) */
typedef struct bcm_plp_timesync_sopmem_s {
    unsigned int   lookup_key_mask;/* which fields below are lookup keys */
    unsigned char  valid;          /* entry is valid or not    */
    unsigned char  direction;      /* Rx/Tx direction          */
    unsigned char  msg_type;       /* PTP message type         */
    unsigned char  mask_enable;    /* mask                     */
    unsigned int   seq_id;         /* sequence ID              */
    unsigned int   domain_num;     /* domain number            */
    unsigned int   vlan_id;        /* Virtual LAN ID           */
    unsigned int   src_port;       /* source TCP/UDP port ID   */
    unsigned char  src_ip[BCMPLP_TIMESYNC_SOPMEM_CLASSIFIED_DATA_SIZE];
                                   /* source IP address deprecated by classified_data *\
                                   \*        Left here for backward compatibility     */
    unsigned char  timestamp[10];  /* 10-octet timestamp       */
    unsigned char  classified_data[BCMPLP_TIMESYNC_SOPMEM_CLASSIFIED_DATA_SIZE];
                                   /* captured data classified by       *\
                                   |*   tx/rx_inband_prop.write_sopmem  *|
                                   \*   in  bcm_plp_timesync_config_t   */
} bcm_plp_timesync_sopmem_t;

/* PTP 1588 TimeSync configuration type. */
typedef struct bcm_plp_timesync_config_s {
    unsigned int validity_mask;       /* Validity mask */
    unsigned int flags;               /* Flags bcm_plp_timesync_flag_t */
    unsigned short itpid;             /* 1588 inner tag */
    unsigned short otpid;             /* 1588 outer tag */
    unsigned short otpid2;            /* 1588 outer tag2 */
    bcm_plp_timesync_timer_adjust_t timer_adjust;   /* Timer adjustment */
    bcm_plp_timesync_inband_ctrl_t inband_ctrl;     /* Inband TS control */
    bcm_plp_timesync_global_mode_t gmode;           /* Global mode */
    bcm_plp_timesync_syncout_t syncout;             /* Syncout */
    unsigned short ts_divider;                      /* TS divider */
    bcm_plp_timesync_timespec_t original_timecode;  /* Original timecode to be inserted */
    unsigned int rx_link_delay;         /* RX link delay */
    bcm_plp_timesync_event_msg_action_t tx_sync_mode;        /* sync */
    bcm_plp_timesync_event_msg_action_t tx_delay_req_mode;   /* delay request */
    bcm_plp_timesync_event_msg_action_t tx_pdelay_req_mode;  /* pdelay request */
    bcm_plp_timesync_event_msg_action_t tx_pdelay_resp_mode; /* pdelay response */
    bcm_plp_timesync_event_msg_action_t rx_sync_mode;        /* sync */
    bcm_plp_timesync_event_msg_action_t rx_delay_req_mode;   /* delay request */
    bcm_plp_timesync_event_msg_action_t rx_pdelay_req_mode;  /* pdelay request */
    bcm_plp_timesync_event_msg_action_t rx_pdelay_resp_mode; /* pdelay response */
    bcm_plp_timesync_mpls_ctrl_t mpls_ctrl;                  /* MPLS control */
    unsigned int sync_freq;                       /* sync frequency */
    unsigned short phy_1588_dpll_k1;              /* DPLL K1 */
    unsigned short phy_1588_dpll_k2;              /* DPLL K2 */
    unsigned short phy_1588_dpll_k3;              /* DPLL K3 */
    plp_uint64_t phy_1588_dpll_loop_filter; /* DPLL loop filter */
    plp_uint64_t phy_1588_dpll_ref_phase;   /* DPLL ref phase */
    unsigned int phy_1588_dpll_ref_phase_delta;   /* DPLL ref phase delta */
    unsigned int tx_inband_spare;               /* Tx inband spare */
    unsigned int tx_inband_clear;               /* Tx inband clear */
    unsigned int tx_inband32_format;            /* tx inband using 32-bit format */
    unsigned int tx_inband_strict;              /* Packet action based in inband only */
    unsigned int tx_mdio_sop_mem;               /* SOP MEM based on GEN1 */
    unsigned int tx_partial_tc;                 /* Compute CF with TXSOP from RSRD field */
    unsigned int tx_mode_tc;                    /* Set node as TC node */
    unsigned int rx_inband_spare;               /* Rx inband spare */
    unsigned int rx_inband_insert_4byt;         /* Insert 32-bit after 1588 header */
    unsigned int rx_inband32_format;            /* Rx inband using 32-bit format */
    unsigned int rx_inband_strict;              /* Packet action based in inband only */
    unsigned int rx_mdio_sop_mem;               /* SOP MEM based on GEN1*/
    unsigned int rx_partial_tc;                 /* Compute CF with RXSOP from RSRD field */
    unsigned int rx_mode_tc;                    /* Set node as TC node */
    bcm_plp_timesync_mpls_ctrl_t rx_mpls_ctrl;               /* RX MPLS control */
    bcm_plp_timesync_option_t          tx_option;       /* Tx timesync options  */
    bcm_plp_timesync_option_t          rx_option;       /* Rx timesync options  */
    bcm_plp_timesync_inband_property_t tx_inband_prop;  /* Tx inband properties */
    bcm_plp_timesync_inband_property_t rx_inband_prop;  /* Rx inband properties */
    unsigned int tx_pkt_count_sel;      /* Tx PTP/CRC packet counting select */
    unsigned int rx_pkt_count_sel;      /* Rx PTP/CRC packet counting select */
    unsigned int fifo_level_intr_thold; /* threshold value to trigger packet timestamp interrupt */
    unsigned int hb_capture_mode;       /* heartbeat capture mode */
    unsigned int nco_sync0_pulse ;      /* edge detect for ext_syncIn0 */
    bcm_plp_timesync_sop_timestamp_capture_t  sop_ts_cap;  /* SOP timestamp capture modes */
} bcm_plp_timesync_config_t;

/*******************************************\
|** End of 1588 TimeSync Specific Defines **|
\*******************************************/

/*!
 * @enum bcm_plp_avs_control_e
 * @brief AVS Control Type
 */
typedef enum bcm_plp_avs_control_e {
    bcmplpAvsControlInternal = 0, /**< Internal AVS Control */
    bcmplpAvsControlExternal, /**< External Chip controlls the AVS */
    bcmplpAvsControlCount
} bcm_plp_avs_control_t;

/*!
 * @enum bcm_plp_avs_pkg_share_e
 * @brief AVS number of packages share AVS
 */
typedef enum bcm_plp_avs_pkg_share_e {
    bcmplpAvsPackageShare1 = 0, /**< 1:Package share AVS */
    bcmplpAvsPackageShare2, /**< 2:Package share AVS */
    bcmplpAvsPackageShare3, /**< 3:Package share AVS */
    bcmplpAvsPackageShare4, /**< 4:Package share AVS */
    bcmplpAvsPackageShare5, /**< 5:Package share AVS */
    bcmplpAvsPackageShare6, /**< 6:Package share AVS */
    bcmplpAvsPackageShare7, /**< 7:Package share AVS */
    bcmplpAvsPackageShare8, /**< 8:Package share AVS */
    bcmplpAvsPackageShareCount
} bcm_plp_avs_pkg_share_t;

/*!
 * @enum bcm_plp_avs_board_dc_margin_e
 * @brief AVS Board DC Margin type
 */
typedef enum bcm_plp_avs_board_dc_margin_e {
    bcmplpAvsBoardDcMargin0mV = 0, /**< 0 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin5mV, /**< 5 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin10mV, /**< 10 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin15mV, /**< 15 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin20mV, /**< 20 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin25mV, /**< 25 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin30mV, /**< 30 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin35mV, /**< 35 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin40mV, /**< 40 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin45mV, /**< 45 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin50mV, /**< 50 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin55mV, /**< 55 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin60mV, /**< 60 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin65mV, /**< 65 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin70mV, /**< 70 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin75mV, /**< 75 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin80mV, /**< 80 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin85mV, /**< 85 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin90mV, /**< 90 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin95mV, /**< 95 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin100mV, /**< 100 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin105mV, /**< 105 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin110mV, /**< 110 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin115mV, /**< 115 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin120mV, /**< 120 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin125mV, /**< 125 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin130mV, /**< 130 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin135mV, /**< 135 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin140mV, /**< 140 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin145mV, /**< 145 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin150mV, /**< 150 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMargin155mV, /**< 155 mV margin added on top for board IR drop */
    bcmplpAvsBoardDcMarginCount
} bcm_plp_avs_board_dc_margin_t;

/*!
 * @enum bcm_plp_avs_status_e
 * @brief AVS Control Type
 */
typedef enum bcm_plp_avs_status_e {
    bcmplpAvsStatusNotStarted = 0, /**< AVS Not Started */
    bcmplpAvsAliveAndInitSuccess, /**< AVS Alive and init Success */
    bcmplpAvsStatusInitError, /**< AVS Init Error */
    bcmplpAvsAliveSuccess, /**< AVS Alive Success */
    bcmplpAvsStatusCount
} bcm_plp_avs_status_t;


/*!
 * @enum bcm_plp_avs_track_status_e
 * @brief AVS track status type
 */
typedef enum bcm_plp_avs_track_status_e {
    bcmplpAvsTrackStatusNotStarted = 0, /**< AVS Not Started */
    bcmplpAvsTrackStatusNormal,         /**< AVS track normal */
    bcmplpAvsTrackStatusError           /**< AVS track Error */
} bcm_plp_avs_track_status_t;

/*!
 * @enum bcm_plp_avs_regulator_e
 * @brief AVS Regulator Type
 */
typedef enum bcm_plp_avs_regulator_e {
    bcmplpAvsRegulator4677 = 0,    /**< 4677 regulator */
    bcmplpAvsRegulator7219,        /**< 7219 regulator */
    bcmplpAvsRegulator7180,        /**< 7180 regulator */
    bcmplpAvsRegulatorTPS544,      /**< TPS544 regulator */
    bcmplpAvsRegulatorMAX77812,    /**< MAX77812 regulator */
    bcmplpAvsRegulator20730,       /**< 20730 regulator */
    bcmplpAvsRegulator4678,        /**< 4678 regulator */
    bcmplpAvsRegulatorTPS546,      /**< TPS546 regulator */
    bcmplpAvsRegulatorCount
} bcm_plp_avs_regulator_t; /**< AVS Regulator Type */

/*!
 * @enum bcm_plp_avs_disable_type_e
 * @brief AVS disable either no firmware control or firmware control
 */
typedef enum bcm_plp_avs_disable_type_e {
    bcmplpAvsDisableTypeNoFirmwareControl = 0, /**< No firmware control */
    bcmplpAvsDisableTypeFirmwareControl, /**< Firmware set back volatge */
    bcmplpAvsDisableTypeCount
} bcm_plp_avs_disable_type_t;

/*!
 * @enum bcm_plp_avs_external_ctrl_step_e
 * @brief AVS External control step 1 or N 
 */ 
typedef enum bcm_plp_avs_external_ctrl_step_e {
    bcmplpAvsExternalCtrlStep1 = 0, /**< External control step 1 */
    bcmplpAvsExternalCtrlStepN, /**< External control step N */
    bcmplpAvsExternalCtrlStepCount
} bcm_plp_avs_external_ctrl_step_t;

/*! bcm_plp_avs_config_status structure \n
  * \arg enable \n
  *  enable/disable AVS configuration\n
  * \arg avs_status  \n
  *   AVS Status
 */
typedef struct bcm_plp_avs_config_status_s {
    unsigned int enable;
    bcm_plp_avs_status_t avs_status;
    bcm_plp_avs_track_status_t avs_track_status;
} bcm_plp_avs_config_status_t;

typedef struct bcm_plp_avs_config_s {
    unsigned int enable; /**< AVS enable/disable */
    bcm_plp_avs_disable_type_t avs_disable_type; /**< AVS disable either no firmware control or firmware control */
    bcm_plp_avs_control_t avs_ctrl; /**< AVS control */
    bcm_plp_avs_pkg_share_t avs_pkg_share; /**<  AVS number of package share AVS */
    bcm_plp_avs_board_dc_margin_t avs_dc_margin; /**< Board DC Margin */
    bcm_plp_avs_regulator_t avs_regulator; /**< Different type of regulator */
    bcm_plp_pm_ref_clk_t ref_clk; /**< Reference clock */
    unsigned char i2c_slave_address[3]; /**< i2c slave address in case of external avs regulator as master */
    bcm_plp_avs_external_ctrl_step_t external_ctrl_step; /**< AVS external control step control */
    unsigned char regulator_i2c_address;  /**< Set Regulator i2c slave address, if not set, keep it 0 to use default regulator slave address */
    unsigned char regulator_legs; /**< Regulator legs set to 0 by default. Enables user to use any of the two legs for supplying DVDD */
    unsigned char internal_master_slave_package; /** < 0:slave package; 1:master package */
} bcm_plp_avs_config_t;


typedef struct bcm_plp_prbs_error_inject_s {
    unsigned int no_of_error_bits;
} bcm_plp_prbs_error_inject_t;

typedef struct bcm_plp_logical_lane_map_s {
    unsigned char num_of_lanes;     /**< Number of elements in lane_map_rx/tx arrays */
    unsigned char rx_lane_list[16]; /**< rx_lane_list[x]=y means that logical lane x Rx mapped to package lane y Rx  */
    unsigned char tx_lane_list[16]; /**< tx_lane_list[x]=y means that logical lane x Tx mapped to package lane y Tx  */
} bcm_plp_logical_lane_map_t;

/****************************************************/
/************** SyncE Definitions Start *************/
/**********************************************/
/*!
 * @enum bcm_plp_clkgen_squelch_cfg_e
 * @brief Recovered Clock Generation and Squelch Mode Configruation
 */
typedef enum bcm_plp_clkgen_squelch_cfg_e {
    bcmplpClkGenSquelchDisable = 0, /**< Disable Clk Gen and Squelch */
    bcmplpClkGenEnSquelchNone, /**< Enable Clk Gen only, no squelch needed (Clock is always sent out) */
    bcmplpClkGenEnSquelchLOS, /**< Squelch clock on Loss of Signal (LOS) */
    bcmplpClkGenEnSquelchLOL, /**< Squelch clock on Loss of Lock (LOL) */
    bcmplpClkGenEnSquelchLOSOrLOL, /**< Squelch clock on Loss of Signal(LOS) or Loss of Lock(LOL) */
    bcmplpClkGenEnSquelchForce /**< Force Squelch */
} bcm_plp_clkgen_squelch_cfg_t;

/*!
 * @enum bcm_plp_recovered_clk_lane_e
 * @brief Recovered Clock Lane selection.
 */
typedef enum bcm_plp_recovered_clk_lane_e {
    bcmplpRecoveredClkLane0 = 0,  /**< Select Lane 0 for recovered Clock Generation. */
    bcmplpRecoveredClkLane1,  /**< Select Lane 1 for recovered Clock Generation. */
    bcmplpRecoveredClkLane2,  /**< Select Lane 2 for recovered Clock Generation. */
    bcmplpRecoveredClkLane3,  /**< Select Lane 3 for recovered Clock Generation. */
    bcmplpRecoveredClkLane4,  /**< Select Lane 4 for recovered Clock Generation. */
    bcmplpRecoveredClkLane5,  /**< Select Lane 5 for recovered Clock Generation. */
    bcmplpRecoveredClkLane6,  /**< Select Lane 6 for recovered Clock Generation. */
    bcmplpRecoveredClkLane7,  /**< Select Lane 7 for recovered Clock Generation. */
    bcmplpRecoveredClkLane8,  /**< Select Lane 8 for recovered Clock Generation. */
    bcmplpRecoveredClkLane9,  /**< Select Lane 9 for recovered Clock Generation. */
    bcmplpRecoveredClkLane10, /**< Select Lane 10 for recovered Clock Generation. */
    bcmplpRecoveredClkLane11, /**< Select Lane 11 for recovered Clock Generation. */
    bcmplpRecoveredClkLane12, /**< Select Lane 12 for recovered Clock Generation. */
    bcmplpRecoveredClkLane13, /**< Select Lane 13 for recovered Clock Generation. */
    bcmplpRecoveredClkLane14, /**< Select Lane 14 for recovered Clock Generation. */
    bcmplpRecoveredClkLane15  /**< Select Lane 15 for recovered Clock Generation. */
} bcm_plp_recovered_clk_lane_t;

/*!
 * @enum bcm_plp_synce_divider_e
 * @brief Divider selection to apply on Line/Lane rate before outputting clock.
 * Not all dividers are supported on each phy.
 * - Dividers supported by Europa family are:
 *    20, 40, 80, 160, 400, 1000
 *
 * - Dividers supported by Millenio family are:
 *    Single-ended ports -> 512, 1024, 4096, 8192
 *    Differential ports -> 32, 64, 128, 256, 512, 1024, 2048, 4096
 *
 * - Dividers supported by Aperta family are:
 *    Single-ended ports -> 80, 120, 240, 520, 1000, 2040, 4080
 *    Differential ports -> 32, 64, 128, 256, 512, 1024, 2048, 4096
 *
 * - Dividers supported by Miura family are:
 *    10G LRM Interface    -> 66, 82.5, 528
 *    All other Interfaces -> 16, 64, 66, 82.5, 528
 */
typedef enum bcm_plp_synce_divider_e {
    bcmplpDivider20 = 0,/**< Divide by 20 */
    bcmplpDivider40,    /**< Divide by 40*/
    bcmplpDivider80,    /**< Divide by 80 */
    bcmplpDivider160,   /**< Divide by 160 */
    bcmplpDivider400,   /**< Divide by 400 */
    bcmplpDivider1000,  /**< Divide by 1000 */
    bcmplpDivider64,    /**< Divide by 64 */
    bcmplpDivider128,   /**< Divide by 128 */
    bcmplpDivider256,   /**< Divide by 256 */
    bcmplpDivider512,   /**< Divide by 512 */
    bcmplpDivider1024,  /**< Divide by 1024 */
    bcmplpDivider2048,  /**< Divide by 2048 */
    bcmplpDivider4096,  /**< Divide by 4096 */
    bcmplpDivider8192,  /**< Divide by 8192 */
    bcmplpDivider32,    /**< Divide by 32 */
    bcmplpDivider120,   /**< Divide by 120 */
    bcmplpDivider240,   /**< Divide by 240 */
    bcmplpDivider520,   /**< Divide by 520 */
    bcmplpDivider2040,  /**< Divide by 2040 */
    bcmplpDivider4080,  /**< Divide by 4080 */
    bcmplpDivider1,     /**< Divide by 1. */
    bcmplpDivider2,     /**< Divide by 2. */
    bcmplpDivider4,     /**< Divide by 4. */
    bcmplpDivider8,      /**< Divide by 8. */
    bcmplpDivider16,     /**< Divide by 16. */
    bcmplpDivider66,     /**< Divide by 66. */
    bcmplpDivider82p5,   /**< Divide by 82.5. */
    bcmplpDivider528    /**< Divide by 528. */
} bcm_plp_synce_divider_t;

/*!
 * @enum bcm_plp_rclk_pin_sel_e
 * @brief Recovered Clock output pin selection.
 */

typedef enum bcm_plp_rclk_pin_sel_e {
    bcmplpRclkPin0 = 0,  /**< rclk pin 0 */
    bcmplpRclkPin1,      /**< rclk pin 1 */
    bcmplpRclkPin2       /**< rclk pin 2 */
} bcm_plp_rclk_pin_sel_t;


/*! SyncE configuration
 *
 * \arg clkGenSquelchCfg \n
 *     Recovered Clock Generation enable/disable and Squelch Mode Configuration. Use bcm_plp_clkgen_squelch_cfg_t enum. \n
 *
 * \arg recoveredClkLane \n
 *     Recovered Clock Lane selection. Use bcm_plp_recovered_clk_lane_t enum. \n
 *
 * \arg squelchMonitorLanemap \n
 *     Package Lanes that need to be monitored for Loss of Signal or Loss of Line. \n
 *
 * \arg divider \n
 *     Divider selection to apply on Line/Lane rate before outputting clock. Use bcm_plp_synce_divider_t enum\n
 *
 * \arg rclk_out_pin_sel \n
 *     Recovered clock output pins are rclk0, rclk1 and rclk2. Use bcm_plp_rclk_pin_sel_t enum\n
 *     User must select one of the output pins. This is always input\n
 *
 * \arg rclk_if_side \n
 *     [Output only] Indicates if the recovered clock lane is on line side or system side\n
 *     0 : Line side, 1 : System side lanes are used for source of the divider which will be output as one of the rclks\n
 */
typedef struct bcm_plp_synce_cfg_s {
    unsigned int clkGenSquelchCfg;
    unsigned int recoveredClkLane;
    unsigned int squelchMonitorLanemap;
    unsigned int divider;
    unsigned int rclk_out_pin_sel;
    unsigned int rclk_if_side;
} bcm_plp_synce_cfg_t;

/**************************************************/
/************** SyncE Definitions End *************/
/**************************************************/

typedef struct bcm_plp_ieee_pcs_rx_status_s {
    unsigned short ieee_baser_pcs_status_1;           /*!< 3.32 - block_lock @0 ; high_ber @1 ; receive_link_status @12 */
    unsigned short ieee_baser_pcs_status_2;           /*!< 3.33 - ber_low_cnt @8..13 (count[0:5]) ; latch_high_ber @14 ; latch_block_lock @15 */
    unsigned short ieee_ber_high_order_counter;       /*!< 3.44 - ber_high_cnt @0..15 (count [6:21]) */
    unsigned short ieee_pcs_alignment_status_1;       /*!< 3.50 - lock_status @0..7 (lanes 0..7) ; pcs_lane_align_status @12 */
    unsigned short ieee_pcs_alignment_status_2;       /*!< 3.51 - lock_status @0..11 (lanes 8..19) */
    unsigned short ieee_pcs_alignment_status_3;       /*!< 3.52 - align_status @0..7 (lanes 0..7) */
    unsigned short ieee_pcs_alignment_status_4;       /*!< 3.53 - align_status @0..11 (lanes 8..19) */
    unsigned short ieee_bip_err_count_pcsln[BCM_PLP_PCS_ENCODED_BIT_STREAMS];      /*!< 3.200..3.219 - bip_error_counter @0..15 ; per pcs lane */
    unsigned short ieee_pcs_lane_mapping[BCM_PLP_PCS_ENCODED_BIT_STREAMS];         /*!< 3.400..3.419 - pcs_lane_mapping @0..4 ; per pcs lane */
} bcm_plp_ieee_pcs_rx_status_t;

typedef struct bcm_plp_pcs_rx_status_phylane_s {
    unsigned char  pcs_block_lock_stat;           /*!< per pcs lane for each phy lane - read followed by clear; Applicable to all modes 10GE, 40GE, 25GE, 50GE and 100GE */
    unsigned char  pcs_block_lolock_sticky;       /*!< per pcs lane for each phy lane - read followed by clear; Applicable to all modes 10GE, 40GE, 25GE, 50GE and 100GE */
    unsigned char  pcs_am_lock_stat;              /*!< per pcs lane for each phy lane - read followed by clear; Not applicable to single lane speeds of 10GE and 25GE */
    unsigned char  pcs_am_lolock_sticky;          /*!< per pcs lane for each phy lane - read followed by clear; Not applicable to single lane speeds of 10GE and 25GE */
    unsigned char  pcs_dskw_error_sticky;         /*!< per pcs lane for each phy lane - read followed by clear; Not applicable to single lane speeds of 10GE and 25GE */
    unsigned char  pcs_lane_mapping[BCM_PLP_PCS_LOGICAL_LANES];           /*!< per pcs lane for each phy lane - valid only if corresponding am_lolock=0;
                                                                               Not applicable to single lane speeds of 10GE and 25GE */
    unsigned short pcs_bip_err_cnt[BCM_PLP_PCS_LOGICAL_LANES];            /*!< per pcs lane for each phy lane - valid only if corresponding am_lolock=0;
                                                                               Not applicable to single lane speeds of 10GE and 25GE */
    unsigned char pcs_skew_stat[BCM_PLP_PCS_LOGICAL_LANES];               /*!< per pcs lane for each phy lane - valid only if corresponding am_lolock=0;
                                                                               Not applicable to single lane speeds of 10GE and 25GE */
    unsigned int fcfec_corrected_block_cnt[BCM_PLP_PCS_LOGICAL_LANES];    /*!< per pcs lane for each phy lane - valid only if corresponding am_lolock=0;
                                                                               Applicable for 25GE and 50GE */
    unsigned int fcfec_uncorrected_block_cnt[BCM_PLP_PCS_LOGICAL_LANES];  /*!< per pcs lane for each phy lane - valid only if corresponding am_lolock=0;
                                                                               Applicable for 25GE and 50GE */
} bcm_plp_pcs_rx_status_phylane_t;

typedef struct bcm_plp_pcs_rx_status_s {
    unsigned char pcs_igbox_clsn_sticky;         /*!< per phy lane (one hot encoded) - read followed by clear */
    bcm_plp_pcs_rx_status_phylane_t pcs_status_phylane [4];    /*!< per pcs core (max 100G) */
    unsigned char pcs_dskw_align_stat;           /*!< per pcs core (deskew alignment) */
    unsigned char pcs_dskw_align_loss_sticky;    /*!< per pcs core (deskew alignment) - read followed by clear */
    unsigned char pcs_hiber_stat;                /*!< per pcs core (high ber) */
    unsigned char pcs_hiber_sticky;              /*!< per pcs core (high ber) - read followed by clear */
    unsigned int pcs_ber_cnt;                    /*!< per pcs core (high ber counter) - latch and read */
    unsigned char pcs_link_stat;                 /*!< 1: link up; 0: link down */
    unsigned char pcs_link_stat_sticky;          /*!< 1: link up; 0: link down - read followed by clear */
    unsigned char pcs_skew_stat_pcsln[BCM_PLP_PCS_ENCODED_BIT_STREAMS];       /*!< per pcs lane ; valid from idx 0 till (pcsl_num - 1)*/
} bcm_plp_pcs_rx_status_t;

typedef struct bcm_plp_pcs_status_s {
    bcm_plp_ieee_pcs_rx_status_t ieee_pcs_sts;
    bcm_plp_pcs_rx_status_t pcs_sts;
} bcm_plp_pcs_status_t;

typedef enum bcm_plp_pattern_type_e {
    bcmplpPatternTypeSSPRQ = 0,   /**< SSPRQ pattern gen */
    bcmplpPatternTypeQPRBS13,     /**< QPRBS13 pattern gen */
    bcmplpPatternTypeSquareWave,        /**< SQUARE WAVE pattern gen. Supported pattern length modes are 2, 4, 8 and 16 consecutive 1s */
    bcmplpPatternTypePCSScrambleIdle,   /**< PCS Scrambled IDLE pattern */
    bcmplpPatternTypeKP4PRBS            /**< KP4 PRBS pattern */
} bcm_plp_pattern_type_t;


typedef enum bcm_plp_pattern_len_e {
    bcmplpPatternLengthMode2 = 0,
    bcmplpPatternLengthMode4,
    bcmplpPatternLengthMode8,
    bcmplpPatternLengthMode16
} bcm_plp_pattern_len_t;


typedef struct bcm_plp_pattern_s {
    unsigned int enable;   /*!< enable/disable pattern generation.  For set API this is an input and for get API it is an output*/
    unsigned int pattern_len; /*!< Pattern length or mode if applicable. Currently only supported for SquareWave. Use enum bcm_plp_pattern_len_t */
    unsigned int* pattern; /*!< Pattern type to set use bcm_plp_pattern_type_t.  This is always input for set and get APIs*/
} bcm_plp_pattern_t;

/*!
 * @enum bcm_plp_srds_diag_access_e
 * @brief Configures the type of access needed
*/
typedef enum bcm_plp_srds_diag_access_e {
    bcmplpSrdsRegRead = 0x0,  /** Register Read (param becomes count)*/
    bcmplpSrdsRegRmw, /** Register Read-Modify-Write (param becomes mask)*/
    bcmplpSrdsCoreRamRdByte, /** CORE RAM Read byte  (data becomes count)*/
    bcmplpSrdsCoreRamRmwByte, /** CORE RAM Read-Modify-Write byte (param becomes mask)*/
    bcmplpSrdsCoreRamRdWord, /** CORE RAM Read word  (data becomes count)*/
    bcmplpSrdsCoreRamRmwWord, /** CORE RAM Read-Modify-Write word (param becomes mask)*/
    bcmplpSrdsLaneRamRdByte, /** LANE RAM Read byte    (data becomes count)*/
    bcmplpSrdsLaneRamRmwByte, /** LANE RAM Read-Modify-Write byte (param becomes mask)*/
    bcmplpSrdsLaneRamRdWord, /** LANE RAM Read word  (data becomes count)*/
    bcmplpSrdsLaneRamRmwWord, /** LANE RAM Read-Modify-Write word (param becomes mask)*/
    bcmplpSrdsGlobRamRdByte, /** Global RAM Read byte  (data becomes count)*/
    bcmplpSrdsGlobRamRmwByte, /** Global RAM Read-Modify-Write byte(param becomes mask)*/
    bcmplpSrdsGlobRamRdWord, /** Global RAM Read word  (data becomes count)*/
    bcmplpSrdsGlobRamRmwWord, /** Global RAM Read-Modify-Write word (param becomes mask)*/
    bcmplpSrdsProgRamRdByte, /** Prog RAM Read byte (data becomes count) */
    bcmplpSrdsProgRamRdWord, /** Prog RAM Read word (data becomes count) */
    bcmplpSrdsUcCmd, /** uC Command (addr becomes command; param becomes supp_info)*/
} bcm_plp_srds_diag_access_t;

typedef struct bcm_plp_srds_diag_access_cfg_s {
    unsigned short addr;  /** In most cases is the address of the register or RAM location */
    unsigned short data;  /** In most cases is the data to be written */
    unsigned short param; /** It is a multipurpose parameter which can be a mask or other data */
    bcm_plp_srds_diag_access_t access_type; /** Controls the type of access requested for diagnostics purposes */
} bcm_plp_srds_diag_access_cfg_t;

typedef enum bcm_plp_pai_phy_op_e {
    BCM_PLP_OP_PHY_TO_PAI = 1, /* for PHY to PAI conversion*/
    BCM_PLP_OP_PAI_TO_PHY      /* For PAI to PHY conversion*/
} bcm_plp_pai_phy_op_t;

typedef enum bcm_plp_pai_phy_config_type_e {
    PAI_PHY_CONFIG_TYPE_FW_INIT = 1,    /* FW Init operation*/
    PAI_PHY_CONFIG_TYPE_PORT_CONFIG,    /* Portconfig operation*/
    PAI_PHY_CONFIG_TYPE_TRAINING,       /* training operation*/
    PAI_PHY_INIT                        /* PLP initialization*/    
} bcm_plp_pai_phy_config_type_t;

typedef struct bcm_plp_pai_fw_init_config_s {
    int phy_reverse_mode;         /* Configure normal or reverse mode to select line wrapper or serdes side */
    int macsec_static_bypass;     /* Configure MACSEC enable or bypass mode */
    int pll1_vco;                 /* Configure maximum VCO rate for PLL1 */
    int tx_drv_supply;            /* Configure TX driver support voltage */
    bcm_plp_pm_ref_clk_t ref_clk; /* Configure reference clock during firmware initialization */
} bcm_plp_pai_fw_init_config_t;

typedef struct bcm_plp_pai_port_config_s {
    unsigned int speed;
    bcm_pm_interface_t interface_type;
    unsigned int fec_mode;
    unsigned int low_latency_variation;
    unsigned int hitless_mux_mode;
    unsigned int failover_lane_map;
    unsigned int tx_drv_supply;
} bcm_plp_pai_port_config_t;

typedef struct bcm_plp_pai_tx_training_s {
    unsigned int training_enable;
} bcm_plp_pai_tx_training_t;

typedef struct bcm_plp_pai_init_s {
    bcm_plp_firmware_load_type_t *firmware_load_type;
    bcm_pm_firmware_broadcast_method_t broadcast_method;
} bcm_plp_pai_init_t;

typedef struct bcm_plp_pai_phy_config_s {
    bcm_plp_pai_phy_config_type_t config_type;
    void *config_data;
} bcm_plp_pai_phy_config_t;

/**
 * The enumeration of Failover config mode 
 */
typedef enum bcm_plp_failover_config_mode_e {
    bcmplpFailoverConfigModeNoHitless = 0,   /**< Failover ports are configured but do not operate in hitless mode */
    bcmplpFailoverConfigModeHitless,         /**< Failover ports are configured and operate in hitless mode  */
    bcmplpFailoverConfigModeHitlessAuto,     /**< Failover ports are configured and operate in hitless mode and switching of context is automatic */
    bcmplpFailoverConfigModeHitlessDiffPPM   /**< Failover ports are configured and operate in hitless mode with two contexts with different PPM */
} bcm_plp_failover_config_mode_t;

/**
 * The enumeration of Failover standby modes 
 */
typedef enum bcm_plp_failover_standby_mode_e {
    bcmplpFailoverStandbyNoLowPower = 0,    /**< Standby port is not in low power mode */
    bcmplpFailoverStandbyLowPowerRcvXmit,   /**< Standby port both recieve and transmit is in low power mode */
    bcmplpFailoverStandbyLowPowerRcv,       /**< Standby port recieve is in low power mode */
    bcmplpFailoverStandbyLowPowerXmit       /**< Standby port transmit is in low power mode */
} bcm_plp_failover_standby_mode_t;

/**
 * The enumeration of Failover switch modes 
 */
typedef enum bcm_plp_failover_switch_mode_e {
    bcmplpFailoverSwitchModeGlobal = 0, /**< active contexts are switched and the scope is global use bcm_plp_barchetta_failover_mode_set to switch context */
    bcmplpFailoverSwitchModePerPort,    /**< active contexts are switched and the scope is per port determined by API bcm_plp_barchetta_failover_mode_set */
    bcmplpFailoverSwitchModePinBased    /**< active contexts are switched and the scope is HMUX_GLOBAL by pin call to API bcm_plp_barchetta_failover_mode_set returns error */
} bcm_plp_failover_switch_mode_t;

    
typedef struct bcm_plp_failover_config_s {
    bcm_plp_failover_config_mode_t failover_config_mode;  /**< Failover hitless mux mode */
    bcm_plp_failover_standby_mode_t standby_mode;         /**< Failover hitless mux standby mode */
    bcm_plp_failover_switch_mode_t switch_mode;           /**< Failover hitless mux switch operation mode */
} bcm_plp_failover_config_t;

typedef struct bcm_plp_pai_info_s {
    bcm_plp_pai_phy_op_t operation;   /**<Indicates whether to convert info from PAI to PHY or vice versa*/
    bcm_plp_pai_phy_config_t *pai_phy_config; /**<Structure with chip information*/ 
    void* epdm_data; /**Pointer to chip specific data*/ 
    int (*read)(void* user_acc, unsigned int core_addr, unsigned int reg_addr, unsigned int* val); /* read Pointer*/
    int (*write)(void* user_acc, unsigned int core_addr, unsigned int reg_addr, unsigned int val); /*Write pointer*/
} bcm_plp_pai_info_t;


typedef enum bcm_plp_mib_stat_type_e {
    BCM_PLP_MIB_STAT_R64 = 0,           /* Receive 64 Byte Frame Counter*/
    BCM_PLP_MIB_STAT_R127,              /* Receive 65 to 127 Byte Frame Counter*/
    BCM_PLP_MIB_STAT_R255,              /* Receive 128 to 255 Byte Frame Counter*/
    BCM_PLP_MIB_STAT_R511,              /* Receive 256 to 511 Byte Frame Counter*/
    BCM_PLP_MIB_STAT_R1023,             /* Receive 512 to 1023 Byte Frame Counter*/
    BCM_PLP_MIB_STAT_R1518,             /* Receive 1024 to 1518 Byte Frame Counter*/
    BCM_PLP_MIB_STAT_RMGV,              /* Receive 1519 to 1522 Byte Good VLAN Frame Counter*/
    BCM_PLP_MIB_STAT_R2047,             /* Receive 1519 to 2047 Byte Frame Counter*/
    BCM_PLP_MIB_STAT_R4095,             /* Receive 2048 to 4095 Byte Frame Counter*/
    BCM_PLP_MIB_STAT_R9216,             /* Receive 4096 to 9216 Byte Frame Counter*/
    BCM_PLP_MIB_STAT_R16383,            /* Receive 9217 to 16383 Byte Frame Counter*/
    BCM_PLP_MIB_STAT_RBCA,              /* Receive Broadcast Frame Counter*/
    BCM_PLP_MIB_STAT_RPROG0,            /* Programmable Range Counter 0*/
    BCM_PLP_MIB_STAT_RPROG1,            /* Programmable Range Counter 1*/
    BCM_PLP_MIB_STAT_RPROG2,            /* Programmable Range Counter 2*/ 
    BCM_PLP_MIB_STAT_RPROG3,            /* Programmable Range Counter 3*/
    BCM_PLP_MIB_STAT_RPKT,              /* Receive Frame/Packet Counter*/
    BCM_PLP_MIB_STAT_RPOK,              /* Receive Good Packet Counter*/
    BCM_PLP_MIB_STAT_RUCA,              /* Receive Unicast Frame Counter*/
    BCM_PLP_MIB_STAT_RESERVED0,         /* Reserved -0 */
    BCM_PLP_MIB_STAT_RMCA,              /* Receive Multicast Frame Counter*/
    BCM_PLP_MIB_STAT_RXPF,              /* Receive PAUSE Frame Counter*/
    BCM_PLP_MIB_STAT_RXPP,              /* Receive PFC (Per-Priority Pause) Frame Counter*/
    BCM_PLP_MIB_STAT_RXCF,              /* Receive Control Frame Counter*/
    BCM_PLP_MIB_STAT_RFCS,              /* Receive FCS Error Frame Counter*/
    BCM_PLP_MIB_STAT_RERPKT,            /* Receive Code Error Frame Counter*/
    BCM_PLP_MIB_STAT_RFLR,              /* Receive Length Out of Range Frame Counter*/
    BCM_PLP_MIB_STAT_RJBR,              /* Receive Jabber Frame Counter*/
    BCM_PLP_MIB_STAT_RMTUE,             /* Receive MTU Check Error Frame Counter*/
    BCM_PLP_MIB_STAT_ROVR,              /* Receive Oversized Frame Counter*/
    BCM_PLP_MIB_STAT_RVLN,              /* Receive VLAN Tag Frame Counter*/
    BCM_PLP_MIB_STAT_RDVLN,             /* Receive Double VLAN Tag Frame Counter*/
    BCM_PLP_MIB_STAT_RXUO,              /* Receive Unsupported Opcode Frame Counter*/
    BCM_PLP_MIB_STAT_RXUDA,             /* Receive Unsupported DA for PAUSE/PFC Frame Counter*/
    BCM_PLP_MIB_STAT_RXWSA,             /* Receive Wrong SA Frame Counter*/
    BCM_PLP_MIB_STAT_RPRM,              /* Receive Promiscuous Frame Counter*/
    BCM_PLP_MIB_STAT_RPFC0,             /* Receive PFC Frame Priority 0*/
    BCM_PLP_MIB_STAT_RPFCOFF0,          /* Receive PFC Frame Priority 0 XON to XOFF*/
    BCM_PLP_MIB_STAT_RPFC1,             /* Receive PFC Frame Priority 1*/
    BCM_PLP_MIB_STAT_RPFCOFF1,          /* Receive PFC Frame Priority 1 XON to XOFF*/
    BCM_PLP_MIB_STAT_RPFC2,             /* Receive PFC Frame Priority 2*/
    BCM_PLP_MIB_STAT_RPFCOFF2,          /* Receive PFC Frame Priority 2 XON to XOFF*/
    BCM_PLP_MIB_STAT_RPFC3,             /* Receive PFC Frame Priority 3*/ 
    BCM_PLP_MIB_STAT_RPFCOFF3,          /* Receive PFC Frame Priority 3 XON to XOFF*/
    BCM_PLP_MIB_STAT_RPFC4,             /* Receive PFC Frame Priority 4*/
    BCM_PLP_MIB_STAT_RPFCOFF4,          /* Receive PFC Frame Priority 4 XON to XOFF*/
    BCM_PLP_MIB_STAT_RPFC5,             /* Receive PFC Frame Priority 5*/
    BCM_PLP_MIB_STAT_RPFCOFF5,          /* Receive PFC Frame Priority 5 XON to XOFF*/
    BCM_PLP_MIB_STAT_RPFC6,             /* Receive PFC Frame Priority 6*/ 
    BCM_PLP_MIB_STAT_RPFCOFF6,          /* Receive PFC Frame Priority 6 XON to XOFF*/
    BCM_PLP_MIB_STAT_RPFC7,             /* Receive PFC Frame Priority 7*/
    BCM_PLP_MIB_STAT_RPFCOFF7,          /* Receive PFC Frame Priority 7 XON to XOFF*/
    BCM_PLP_MIB_STAT_RUND,              /* Receive Undersize Frame Counter*/
    BCM_PLP_MIB_STAT_RFRG,              /* Receive Fragment Counter*/
    BCM_PLP_MIB_STAT_RRPKT,             /* Receive RUNT Frame Counter*/
    BCM_PLP_MIB_STAT_RESERVED1,         /* RESERVED - 1*/
    BCM_PLP_MIB_STAT_T64,               /* Transmit 64 Byte Frame Counter*/
    BCM_PLP_MIB_STAT_T127,              /* Transmit 65 to 127 Byte Frame Counter*/
    BCM_PLP_MIB_STAT_T255,              /* Transmit 128 to 255 Byte Frame Counter*/
    BCM_PLP_MIB_STAT_T511,              /* Transmit 256 to 511 Byte Frame Counter*/
    BCM_PLP_MIB_STAT_T1023,             /* Transmit 512 to 1023 Byte Frame Counter*/
    BCM_PLP_MIB_STAT_T1518,             /* Transmit 1024 to 1518 Byte Frame Counter*/
    BCM_PLP_MIB_STAT_TMGV,              /* Transmit 1519 to 1522 Byte Good VLAN Frame Counter*/
    BCM_PLP_MIB_STAT_T2047,             /* Transmit 1519 to 2047 Byte Frame Counter*/
    BCM_PLP_MIB_STAT_T4095,             /* Transmit 2048 to 4095 Byte Frame Counter*/
    BCM_PLP_MIB_STAT_T9216,             /* Transmit 4096 to 9216 Byte Frame Counter*/
    BCM_PLP_MIB_STAT_T16383,            /* Transmit 9217 to 16383 Byte Frame Counter*/
    BCM_PLP_MIB_STAT_TBC,               /* Transmit Broadcast Frame Counter*/
    BCM_PLP_MIB_STAT_TPFC0,             /* Transmit PFC Frame Priority 0*/
    BCM_PLP_MIB_STAT_TPFCOFF0,          /* Transmit PFC Frame Priority 0 XON to XOFF*/
    BCM_PLP_MIB_STAT_TPFC1 ,            /* Transmit PFC Frame Priority 1*/
    BCM_PLP_MIB_STAT_TPFCOFF1,          /* Transmit PFC Frame Priority 1 XON to XOFF*/
    BCM_PLP_MIB_STAT_TPFC2 ,            /* Transmit PFC Frame Priority 2*/
    BCM_PLP_MIB_STAT_TPFCOFF2,          /* Transmit PFC Frame Priority 2 XON to XOFF*/
    BCM_PLP_MIB_STAT_TPFC3 ,            /* Transmit PFC Frame Priority 3*/
    BCM_PLP_MIB_STAT_TPFCOFF3,          /* Transmit PFC Frame Priority 3 XON to XOFF*/
    BCM_PLP_MIB_STAT_TPFC4 ,            /* Transmit PFC Frame Priority 4*/ 
    BCM_PLP_MIB_STAT_TPFCOFF4,          /* Transmit PFC Frame Priority 4 XON to XOFF*/
    BCM_PLP_MIB_STAT_TPFC5 ,            /* Transmit PFC Frame Priority 5*/   
    BCM_PLP_MIB_STAT_TPFCOFF5,          /* Transmit PFC Frame Priority 5 XON to XOFF*/
    BCM_PLP_MIB_STAT_TPFC6 ,            /* Transmit PFC Frame Priority 6*/
    BCM_PLP_MIB_STAT_TPFCOFF6,          /* Transmit PFC Frame Priority 6 XON to XOFF*/
    BCM_PLP_MIB_STAT_TPFC7 ,            /* Transmit PFC Frame Priority 7*/
    BCM_PLP_MIB_STAT_TPFCOFF7,          /* Transmit PFC Frame Priority 7 XON to XOFF*/
    BCM_PLP_MIB_STAT_TPKT,              /* Transmit frame/packet Counter*/ 
    BCM_PLP_MIB_STAT_TPOK,              /* Transmit Good Packet Counter*/
    BCM_PLP_MIB_STAT_TUCA,              /* Transmit Unicast Frame Counter*/
    BCM_PLP_MIB_STAT_TUF,               /* Transmit FIFO Underrun Counter*/
    BCM_PLP_MIB_STAT_TMCA,              /* Transmit Multicast Frame Counter*/
    BCM_PLP_MIB_STAT_TXPF,              /* Transmit Pause Control Frame Counter*/
    BCM_PLP_MIB_STAT_TXPP,              /* Transmit PFC/Per-Priority Pause Control Frame Counter*/
    BCM_PLP_MIB_STAT_TXCF,              /* Transmit Control Frame Counter*/
    BCM_PLP_MIB_STAT_TFCS,              /* Transmit FCS Error Counter*/
    BCM_PLP_MIB_STAT_TERR,              /* Transmit Error Counter*/
    BCM_PLP_MIB_STAT_TOVR,              /* Transmit Oversize Packet Counter*/
    BCM_PLP_MIB_STAT_TJBR,              /* Transmit Jabber Counter*/
    BCM_PLP_MIB_STAT_TRPKT,             /* Transmit RUNT Frame Counter*/
    BCM_PLP_MIB_STAT_TFRG ,             /* Transmit Fragment Counter*/
    BCM_PLP_MIB_STAT_TVLN ,             /* Transmit VLAN Tag Frame Counter*/
    BCM_PLP_MIB_STAT_TDVLN,             /* Transmit Double VLAN Tag Frame Counter*/
    BCM_PLP_MIB_STAT_RBYT,              /* Receive Byte Counter*/
    BCM_PLP_MIB_STAT_RRBYT,             /* Receive Runt Byte Counter*/
    BCM_PLP_MIB_STAT_TBYT               /* Transmit Byte Counter*/
} bcm_plp_mib_stat_type_t;

typedef struct bcm_plp_aperta_port_config_s {
    bcm_plp_aperta_fixed_latency_config_t    fixed_latency_config;
    int egr_ptp_fixed_latency; /*Applicable for Chip Rev B0*/
} bcm_plp_aperta_port_config_t;



#endif /* BCM_COMMON_DEFINES_H */
