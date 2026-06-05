/*
 *
 * $Id: aperta_cfg_seq.h Exp $
 *
 *
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 *
 *
 *
 *
 *
 */

#ifndef __APERTA_CFG_SEQ_H__
#define __APERTA_CFG_SEQ_H__

#include <phymod/phymod.h>


int plp_aperta_get_chip_id (const plp_aperta_phymod_access_t *pa, int *chip_id);
int plp_aperta_get_chip_rev (const plp_aperta_phymod_access_t *pa, uint32_t *chip_rev);
int _plp_aperta_check_fw_download_status(const plp_aperta_phymod_core_access_t *core_access,
        plp_aperta_phymod_firmware_load_method_t load_method);
int
_plp_aperta_download_prog_eeprom(const plp_aperta_phymod_core_access_t *core_access,
        uint8_t *plp_aperta_ucode, uint32_t fw_length, uint16_t master_en,
        uint16_t mst_boot_addr, uint8_t prg_eeprom, const plp_aperta_phymod_core_init_config_t* init_config);

#define APERTA_MICRO_RETRY_COUNT               1000
#define APERTA_HEADER_SIZE                     64
#define APERTA_FW_ALREADY_DOWNLOADED           0xFAD
#define APERTA_MODULE_CNTRL_RAM_NVR0_ADR       0x8800
#define APERTA_MAX_LANES                       8

/** Module controller I2C master commands */
typedef enum {
  APERTA_FLUSH   = 0,
  APERTA_RANDOM_ADDRESS_READ = 1,
  APERTA_CURRENT_ADDRESS_READ = 2,
  APERTA_I2C_WRITE = 3
} APERTA_I2CM_CMD_E;

typedef enum APERTA_MSGOUT_E {
    APERTA_MSGOUT_DONTCARE = 0x0000,
    APERTA_MSGOUT_GET_CNT  = 0x8888,
    APERTA_MSGOUT_GET_LSB  = 0xABCD,
    APERTA_MSGOUT_GET_MSB  = 0x4321,
    APERTA_MSGOUT_GET_2B   = 0xEEEE,
    APERTA_MSGOUT_GET_B    = 0xF00D,
    APERTA_MSGOUT_ERR      = 0x0BAD,
    APERTA_MSGOUT_NEXT     = 0x2222,
    APERTA_MSGOUT_NOT_DWNLD     = 0x0101,
    APERTA_MSGOUT_DWNLD_ALREADY = 0x0202,
    APERTA_MSGOUT_DWNLD_DONE    = 0x0303,
    APERTA_MSGOUT_PRGRM_DONE    = 0x0404,
    APERTA_MSGOUT_HEADER    = 0x0EAD,
    APERTA_MSGOUT_HDR_ERR  = 0x0E0E,
    APERTA_MSGOUT_FLASH    = 0xF1AC,
    APERTA_MSGOUT_PLL_LOCK = 0x0F0F
} APERTA_MSGOUT_T;

typedef enum APERTA_SW_SIDE_E {
    APERTA_SW_LINE_SIDE = 0,
    APERTA_SW_SYS_SIDE  = 1,
    APERTA_SW_BOTH_SIDE = 2,
    APERTA_SW_NO_SIDE   = 3,
    APERTA_SW_MAX_COUNT = 4
}APERTA_SW_SIDE_T;

int _plp_aperta_core_firmware_info_get(const plp_aperta_phymod_access_t *acc, plp_aperta_phymod_core_firmware_info_t* fw_info);

int plp_aperta_dload_fw(const plp_aperta_phymod_core_access_t* core, const plp_aperta_phymod_core_init_config_t* init_config);

int _plp_aperta_phy_status_dump(const plp_aperta_phymod_phy_access_t *phy);

int plp_aperta_module_read(const plp_aperta_phymod_phy_access_t *phy, uint32_t slv_addr, uint32_t start_addr, uint32_t no_of_bytes, uint8_t *read_data);

int plp_aperta_module_write(const plp_aperta_phymod_phy_access_t *phy, uint32_t slv_dev_addr, uint32_t start_addr, uint32_t no_of_bytes, const uint8_t *write_data);

int _plp_aperta_core_reset_set(const plp_aperta_phymod_core_access_t* core, plp_aperta_phymod_reset_mode_t reset_mode, plp_aperta_phymod_reset_direction_t direction);

int _plp_aperta_phy_gpio_config_set(const plp_aperta_phymod_phy_access_t* phy, int pin_number, plp_aperta_phymod_gpio_mode_t gpio_mode);

int _plp_aperta_phy_gpio_config_get(const plp_aperta_phymod_phy_access_t* phy, int pin_number, plp_aperta_phymod_gpio_mode_t* gpio_mode);

int _plp_aperta_phy_gpio_pin_value_set(const plp_aperta_phymod_phy_access_t* phy, int pin_number, int value);

int _plp_aperta_phy_gpio_pin_value_get(const plp_aperta_phymod_phy_access_t* phy, int pin_number, int* value);

int plp_aperta_sw_intf_set(const plp_aperta_phymod_phy_access_t *phy, plp_aperta_phymod_interface_t interface_type);

int plp_aperta_sw_intf_get(const plp_aperta_phymod_phy_access_t *phy, uint8_t lane_index, plp_aperta_phymod_interface_t *intf);

int _plp_aperta_phy_intr_enable_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t intr_type_enable);

int _plp_aperta_phy_intr_enable_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t *intr_enable);

int _plp_aperta_phy_intr_status_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t *intr_sts);

int _plp_aperta_phy_intr_status_clear(const plp_aperta_phymod_phy_access_t* phy, uint32_t intr_type);

#ifdef PHYMOD_TIMESYNC_SUPPORT /*---------------------------------------------------------------*/
    /* use the TimeSync functions declared in  phymod/common/ieee1588/timesync.h */

#else /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int _aperta_timesync_config_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_timesync_config_t* config);
int _aperta_timesync_config_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_timesync_config_t* config);

int _aperta_timesync_enable_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, uint32_t enable);
int _aperta_timesync_enable_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, uint32_t* enable);

int _aperta_timesync_nco_addend_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t freq_step);
int _aperta_timesync_nco_addend_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* freq_step);

int _aperta_timesync_framesync_mode_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_timesync_framesync_t* framesync);
int _aperta_timesync_framesync_mode_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_timesync_framesync_t* framesync);

int _aperta_timesync_local_time_set(const plp_aperta_phymod_phy_access_t* phy, uint64_t local_time);
int _aperta_timesync_local_time_get(const plp_aperta_phymod_phy_access_t* phy, uint64_t* local_time);

int _aperta_timesync_load_ctrl_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t load_once, uint32_t load_always);
int _aperta_timesync_load_ctrl_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* load_once, uint32_t* load_always);

int _aperta_timesync_tx_timestamp_offset_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t ts_offset);
int _aperta_timesync_tx_timestamp_offset_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* ts_offset);

int _aperta_timesync_rx_timestamp_offset_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t ts_offset);
int _aperta_timesync_rx_timestamp_offset_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* ts_offset);

int _aperta_timesync_capture_timestamp_get(const plp_aperta_phymod_phy_access_t* phy, uint64_t* cap_ts);
int _aperta_timesync_heartbeat_timestamp_get(const plp_aperta_phymod_phy_access_t* phy, uint64_t* hb_ts);

#endif /* PHYMOD_TIMESYNC_SUPPORT */ /*---------------------------------------------------------*/

/* Aperta intr Group*/
#define APERTA_INTR_PORT_PTP                    0x1
#define APERTA_INTR_PORT_LINK_DOWN              0x2
#define APERTA_INTR_PORT_SF_ERR_INTR            0x4
#define APERTA_INTR_PORT_SF_DED_INTR            0x8
#define APERTA_INTR_PORT_FC_ERR_INTR            0x10
#define APERTA_INTR_PORT_FC_DED_INTR            0x20
#define APERTA_INTR_PORT_INTF_ERR_INTR          0x40
#define APERTA_INTR_PORT_INTF_DED_INTR          0x80

#define APERTA_INTR_CMN_MOD_ABS_RISING          0x100
#define APERTA_INTR_CMN_MOD_ABS_FALL            0x200
#define APERTA_INTR_CMN_MOD_EXT_RAISING         0x400
#define APERTA_INTR_CMN_MOD_EXT_FALL            0x800
#define APERTA_INTR_CMN_LMI_ERR                 0x1000
#define APERTA_INTR_CMN_LMI_DED                 0x2000
#define APERTA_INTR_CMN_PMIF_ERR                0x4000
#define APERTA_INTR_CMN_PMIF_DED                0x8000

#define APERTA_INTR_M0_MST_FW_WDOG_EXP          0x10000
#define APERTA_INTR_M0_MST_DRAM_DED             0x20000
#define APERTA_INTR_M0_MST_DED                  0x40000

#define APERTA_INTR_PORT                        0xFF
#define APERTA_INTR_COMMON                      0xFF00
#define APERTA_INTR_MO                          0x70000
#define APERTA_INTR_TIMESYNC                    0x7F000000
#define APERTA_INTR_LEVEL1_TIMESYNC             APERTA_INTR_TIMESYNC

#define APERTA_GPIO_CTRL1_BASE_ADDR              BCMI_APERTA_D_PAD_CNTRL_GPIO_0_CONTROL_1r
#define APERTA_GPIO_CTRL0_BASE_ADDR              BCMI_APERTA_D_PAD_CNTRL_GPIO_0_CONTROL_0r
#define APERTA_GPIO_STS_BASE_ADDR                BCMI_APERTA_D_PAD_CNTRL_GPIO_0_STATUSr
#define APERTA_GPIO_MIN_PIN                     0
#define APERTA_GPIO_MAX_PIN                     15
#define APERTA_GPIO_CTRL_OEBF_MASK              1
#define APERTA_IS_SYSTEM_SIDE(PHY)              (PHY->port_loc == phymodPortLocSys)
#define APERTA_IS_LINE_SIDE(PHY)                (PHY->port_loc == phymodPortLocLine || PHY->port_loc == phymodPortLocDC)

/* SyncE registers to drive strength of the recovered clock */
#define APERTA_GPIO_12_CTRL1_BASE_ADDR           BCMI_APERTA_D_PAD_CNTRL_GPIO_12_CONTROL_1r
#define APERTA_GPIO_13_CTRL1_BASE_ADDR           BCMI_APERTA_D_PAD_CNTRL_GPIO_13_CONTROL_1r

/* The following defines are used to save interface type in SWGPREG */
/* Use 4 bits per lane in SWGPREG for saving SW interfaces*/
/* Totaly 4 register 2 for line and 2 for sys*/
#define APERTA_IF_TYPE_SAVE_SWGPREG_BASE_ADDR        BCMI_APERTA_D_CTRL_SWGPREG00r
#define APERTA_IF_TYPE_PER_LANE_STORAGE_MASK         0xF
#define APERTA_LINE_SYS_IF_TYPE_SAVE_SWGPREG_OFFSET  2
#define APERTA_IF_TYPE_SAVE_SWGPREG_BIT4             BCMI_APERTA_D_CTRL_SWGPREG1Er

/* Adding one more register to  interface type in SWGPREG */
/* Totaly 5 bits */


/*SW GPREG for storing number of lanes in systemside*/
#define APERTA_SYSTEM_SIDE_NO_OF_LANES        BCMI_APERTA_D_CTRL_SWGPREG04r

#define APERTA_IF_TYPE_LIST_ELEMENTS         {                                       \
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
#define APERTA_MAX_FW_SUPPORTED             6
#define APERTA_MAX_FW_ATT                   4

typedef enum aperta_lane_data_rate_e {
    bcmplpApertaLaneDataRateNone        = (0),
    bcmplpApertaLaneDataRate_1P25G      = (1250),
    bcmplpApertaLaneDataRate_10P3125G   = (10312),
    bcmplpApertaLaneDataRate_20P625G    = (20625),
    bcmplpApertaLaneDataRate_25P78125G  = (25781),
    bcmplpApertaLaneDataRate_26P5625G   = (26562),
    bcmplpApertaLaneDataRate_27P9525G   = (27952),
    bcmplpApertaLaneDataRate_28P125G    = (28125),
    bcmplpApertaLaneDataRate_51P5625G   = (51562),
    bcmplpApertaLaneDataRate_53P125G    = (53125),
    bcmplpApertaLaneDataRate_56P25G     = (56250)
}aperta_lane_data_rate_t ;

typedef enum aperta_modulation_mode_e {
    bcmplpApertaModulationNONE = 0,
    bcmplpApertaModulationNRZ,
    bcmplpApertaModulationPAM4,
    bcmplpApertaModulationCount
} aperta_modulation_mode_t;

typedef enum aperta_fec_mode_sel_e {
    bcmplpApertaNoFEC = 0,
    bcmplpApertaRS544, /* RS544 */
    bcmplpApertaBaseR,
    bcmplpApertaRSFEC,
    bcmplpApertaRS272,
    bcmplpApertaRS544_2XN,
    bcmplpApertaFecCount
}aperta_fec_mode_sel_t;

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
typedef struct aperta_failover_configuration_s {
    unsigned int lane_map;
    unsigned int mux_location;
}aperta_failover_configuration_t;

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
 *     should take from entering PM2MS interface to leaving MS2PM interface.
 *
 */
typedef struct aperta_fixed_latency_configuration_s {
    unsigned int enable;
    unsigned int igr_dp_ck_cycles;
    unsigned int egr_dp_ck_cycles;
}aperta_fixed_latency_configuration_t;

typedef enum aperta_1588_config_e {
    bcmplpApertaPTPDisabled = 0,
    bcmplpApertaPTPNseEnabled, /*NSE TS for egress and Ingress*/
    bcmplpApertaPTPNsePmEnabled, /*NSE TS for egress and PM-TS for Ingress*/
    bcmplpApertaPTPPmEnabled, /*PM TS for egress and PM-TS for Ingress*/
    bcmplpApertaPTPPmNseEnabled, /*PM TS for egress and NSE-TS for Ingress*/
    /* Value 5-7 is reserved for future use*/
    bcmplpApertaPTPNse2SEnabled = 8,  /* NSE-TS (B0 Plan A) for Egress & Ingress (packets encrypted) - 2 Stage*/
    bcmplpApertaPTPNsePm2SEnabled,    /* NSE-TS (B0 Plan A) for Egress & PM-TS for Ingress (packets encrypted) - 2 Stage*/
    bcmplpApertaPTPNseE2EEnabled,     /* NSE-TS (B0 Plan B) for Egress & Ingress (packets encrypted) - E2E*/
    bcmplpApertaPTPNsePmE2EEnabled    /* NSE-TS (B0 Plan B) for Egress, PM-TS for Ingress (packets encrypted)- E2E*/
} aperta_1588_config_t;

typedef enum aperta_port_type_e {
    bcmplpApertaPortTypePassthrough = 0,  /* Passthrough Port*/
    bcmplpApertaPortTypeGearBox ,         /* Gearbox port*/
    bcmplpApertaPortTypeReverseGearBox    /* Reverse Gearbox port*/
}aperta_port_type_t;

typedef struct aperta_device_aux_modes_s {
    aperta_lane_data_rate_t             lane_data_rate    ;
    aperta_modulation_mode_t            modulation_mode   ;
    aperta_fec_mode_sel_t               fec_mode_sel      ;
    aperta_failover_configuration_t     failover_config   ;
    aperta_fixed_latency_configuration_t    fixed_latency_config;
    aperta_1588_config_t                   ts_config;
    int                                 egr_ptp_fixed_latency;
    aperta_port_type_t                  port_type;
} aperta_device_aux_modes_t;

typedef struct aperta_update_port_config_s {
    aperta_fixed_latency_configuration_t    fixed_latency_config;
    int                                 egr_ptp_fixed_latency;
} aperta_update_port_config_t;

#endif

