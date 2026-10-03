/********************************************************************************
 * Copyright(C) 2026 Micas Network. All rights reserved.
 ********************************************************************************
 * PHY SDK Interface Definitions for BCM81394 APERTA
 *
 * (un-refactored) aperta_phy_sdk.c depended on.
 * enums and macros used by the ai_app sources (aperta_phy_sdk.c, aperta_bcm_sdk_stubs.c,
 * aperta_fw_dload.c, aperta_pm_port.c, aperta_phy.c, aperta_tbl.c).
 ********************************************************************************/
#ifndef APERTA_PHY_SDK_H
#define APERTA_PHY_SDK_H

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "common.h"      /* log_info / LOG_INFO / STATUS_* */
#include "mdio.h"        /* mdio_read_func_t / mdio_write_func_t */

/* ==========================================================================
 * Return codes
 * ========================================================================== */
#define PHYMOD_E_NONE       0
#define PHYMOD_E_FAIL       1
#define PHYMOD_E_INTERNAL   2
#define PHYMOD_E_CONFIG     3
#define PHYMOD_E_UNAVAIL    4
#define PHYMOD_E_TIMEOUT    5
#define PHYMOD_E_PARAM      6

/* ==========================================================================
 * Generic helpers
 * ========================================================================== */
#define PHYMOD_MEMSET(_m,_v,_s)  memset((_m),(_v),(_s))
#define PHYMOD_MEMCPY(_d,_s,_n)  memcpy((_d),(_s),(_n))
#define PHYMOD_USLEEP(_u)        usleep(_u)
#define PHYMOD_IF_ERR_RETURN(_expr) \
    do { int _phy_rc = (_expr); if (_phy_rc != PHYMOD_E_NONE) return _phy_rc; } while (0)
#define PHYMOD_CRIT_INFO(_args)  do { LOG_INFO _args; } while (0)

/* Base address for "direct" (BCMI chip-indirect / LMI) register space.
 * The BCMI_APERTA_D_* register macros in aperta_d_defs.h OR this into
 * the raw register offset. Must be defined before that header is included. */
#define PHYMOD_APERTA_DIRECT_BASE_ADR   0x1000000u

/* BCMI register macros (addresses + field GET/SET + READ/WRITE wrappers).
 * Must be included AFTER PHYMOD_APERTA_DIRECT_BASE_ADR is defined - the
 * register address macros OR the base into their offset at expansion time. */
#include "aperta_d_defs.h"

/* ==========================================================================
 * pm_info (per-PHY firmware state kept by the driver)
 * ========================================================================== */
#define APERTA_MAX_PM_INFO      1024
#define APERTA_PM_NUM_LANES     8
#define APERTA_UNINIT_PHYS      0xFFFF

typedef struct {
    uint32_t phy_id;
    uint32_t is_fw_dloaded;
    int speed[APERTA_PM_NUM_LANES];
    int sys_speed[APERTA_PM_NUM_LANES];
} aperta_pm_info_t;

extern aperta_pm_info_t _plp_aperta_pm_info[APERTA_MAX_PM_INFO];

typedef struct plp_aperta_phymod_access_s {
    void     *user_acc;             /**< Optional app data - used as bus ctx   */
    uint32_t  flags;                /**< PHYMOD_ACC_F_xxx flags                 */
    uint32_t  lane_mask;            /**< specific lanes bitmap                  */
    uint32_t  addr;                 /**< PHY address (PHYAD)                    */
    uint8_t   pll_idx;              /**< PLL number (micro index for TSC)       */
    uint8_t   tvco_pll_index;       /**< PLL to be configured for TVCO          */
} plp_aperta_phymod_access_t;

/* ==========================================================================
 * Bus access used by the aperta_d_defs.h READ/WRITE macros
 * ========================================================================== */
extern int _phy_sdk_bus_read(const plp_aperta_phymod_access_t *pa, uint32_t reg, uint32_t *data);
extern int _phy_sdk_bus_write(const plp_aperta_phymod_access_t *pa, uint32_t reg, uint32_t data);
#define PHYMOD_BUS_READ(_pa,_reg,_data)  _phy_sdk_bus_read((_pa),(_reg),&(_data))
#define PHYMOD_BUS_WRITE(_pa,_reg,_data) _phy_sdk_bus_write((_pa),(_reg),(_data))

typedef enum plp_aperta_phymod_port_loc_e {
    phymodPortLocDC = 0,            /**< Loc/side don't care */
    phymodPortLocLine,              /**< Line side */
    phymodPortLocSys,               /**< System side */
    phymodPortCount
} plp_aperta_phymod_port_loc_t;

typedef struct plp_aperta_phymod_phy_access_s {
    plp_aperta_phymod_port_loc_t  port_loc;
    plp_aperta_phymod_access_t    access;
} plp_aperta_phymod_phy_access_t;

typedef struct plp_aperta_phymod_mac_access_s {
    plp_aperta_phymod_phy_access_t phy_info;
} plp_aperta_phymod_mac_access_t;

#define PHYMOD_MAX_LANES_PER_CORE   8

typedef struct plp_aperta_phymod_lane_map_s {
    uint32_t num_of_lanes;                                      /**< #elements in rx/tx arrays */
    uint32_t lane_map_rx[PHYMOD_MAX_LANES_PER_CORE];            /**< rx lane x mapped to rx lane y */
    uint32_t lane_map_tx[PHYMOD_MAX_LANES_PER_CORE];            /**< tx lane x mapped to tx lane y */
} plp_aperta_phymod_lane_map_t;

typedef struct plp_aperta_phymod_polarity_s {
    uint32_t rx_polarity;   /**< RX polarity bitmap (per lane) */
    uint32_t tx_polarity;   /**< TX polarity bitmap (per lane) */
} plp_aperta_phymod_polarity_t;

typedef struct plp_aperta_phymod_core_firmware_info_s {
    uint32_t fw_version;
    uint32_t fw_crc;
} plp_aperta_phymod_core_firmware_info_t;

typedef enum plp_aperta_phymod_firmware_load_method_e {
    phymodFirmwareLoadMethodNone = 0,   /**< Don't load FW */
    phymodFirmwareLoadMethodInternal,   /**< Load FW internally */
    phymodFirmwareLoadMethodExternal,   /**< Load FW via MDIO by given function */
    phymodFirmwareLoadMethodProgEEPROM, /**< Program EEPROM */
    phymodFirmwareLoadMethodAuto,       /**< Auto download on firmware change */
    phymodFirmwareLoadMethodExternalNVM,/**< Load FW via MDIO to NVM */
    phymodFirmwareLoadMethodCount
} plp_aperta_phymod_firmware_load_method_t;

/* ==========================================================================
 * aperta device aux modes
 * ========================================================================== */
typedef enum aperta_lane_data_rate_e {
    bcmplpApertaLaneDataRateNone        = 0,
    bcmplpApertaLaneDataRate_1P25G      = (1250),
    bcmplpApertaLaneDataRate_10P3125G   = (10312),
    bcmplpApertaLaneDataRate_20P625G    = (20625),
    bcmplpApertaLaneDataRate_25P78125G  = (25781),
    bcmplpApertaLaneDataRate_26P5625G   = (26562),
    bcmplpApertaLaneDataRate_27P9525G   = (27952),
    bcmplpApertaLaneDataRate_28P125G    = (28125),
    bcmplpApertaLaneDataRate_51P5625G   = (51562),
    bcmplpApertaLaneDataRate_53P125G    = (53125),
    bcmplpApertaLaneDataRate_56P25G     = (56250),
    bcmplpApertaLaneDataRate_106P25G    = (106250)   /* ai_app extension */
} aperta_lane_data_rate_t;

typedef enum aperta_modulation_mode_e {
    bcmplpApertaModulationNONE = 0,
    bcmplpApertaModulationNRZ,
    bcmplpApertaModulationPAM4,
    bcmplpApertaModulationCount
} aperta_modulation_mode_t;

typedef enum aperta_fec_mode_sel_e {
    bcmplpApertaNoFEC = 0,   /* No FEC */
    bcmplpApertaRS544,       /* RS544 */
    bcmplpApertaBaseR,
    bcmplpApertaRSFEC,
    bcmplpApertaRS272,
    bcmplpApertaRS544_2XN,
    bcmplpApertaFecCount
} aperta_fec_mode_sel_t;

typedef enum aperta_port_type_e {
    bcmplpApertaPortTypePassthrough = 0,  /* Passthrough Port */
    bcmplpApertaPortTypeGearBox,          /* Gearbox port */
    bcmplpApertaPortTypeReverseGearBox    /* Reverse Gearbox port */
} aperta_port_type_t;

typedef struct aperta_failover_configuration_s {
    unsigned int lane_map;
    unsigned int mux_location;
} aperta_failover_configuration_t;

typedef struct aperta_fixed_latency_configuration_s {
    unsigned int enable;
    unsigned int igr_dp_ck_cycles;
    unsigned int egr_dp_ck_cycles;
} aperta_fixed_latency_configuration_t;

typedef enum aperta_1588_config_e {
    bcmplpApertaPTPDisabled = 0,
    bcmplpApertaPTPNseEnabled,
    bcmplpApertaPTPNsePmEnabled,
    bcmplpApertaPTPPmEnabled,
    bcmplpApertaPTPPmNseEnabled,
    bcmplpApertaPTPNse2SEnabled = 8,
    bcmplpApertaPTPNsePm2SEnabled,
    bcmplpApertaPTPNseE2EEnabled,
    bcmplpApertaPTPNsePmE2EEnabled
} aperta_1588_config_t;

typedef struct aperta_device_aux_modes_s {
    aperta_lane_data_rate_t             lane_data_rate;
    aperta_modulation_mode_t            modulation_mode;
    aperta_fec_mode_sel_t               fec_mode_sel;
    aperta_failover_configuration_t     failover_config;
    aperta_fixed_latency_configuration_t fixed_latency_config;
    aperta_1588_config_t                ts_config;
    int                                 egr_ptp_fixed_latency;
    aperta_port_type_t                  port_type;
} aperta_device_aux_modes_t;

/* ==========================================================================
 * PHY interface config
 * ========================================================================== */
typedef struct plp_aperta_phymod_phy_inf_config_s {
    int  data_rate;                  /**< Port speed (Mb/s) */
    int  interface_type;             /**< PHYMOD_APERTA_INTF_* */
    int  ref_clock;                  /**< Reference clock index */
    uint32_t interface_modes;        /**< PHYMOD_INTF_MODES_* */
    aperta_device_aux_modes_t *device_aux_modes;
} plp_aperta_phymod_phy_inf_config_t;

/* Interface types */
#define PHYMOD_APERTA_INTF_BYPASS      0
#define PHYMOD_APERTA_INTF_SR          1
#define PHYMOD_APERTA_INTF_KR          5
#define PHYMOD_APERTA_INTF_KR4         7
#define PHYMOD_APERTA_INTF_CR          11
#define PHYMOD_APERTA_INTF_CR4         13
#define PHYMOD_APERTA_INTF_XFI         15
#define PHYMOD_APERTA_INTF_SFI         16
#define PHYMOD_APERTA_INTF_XLAUI       24
#define PHYMOD_APERTA_INTF_LR          29
#define PHYMOD_APERTA_INTF_ER          31
#define PHYMOD_APERTA_INTF_VSR         37
#define PHYMOD_APERTA_INTF_CAUI4_C2C   40
#define PHYMOD_APERTA_INTF_CAUI4_C2M   41
#define PHYMOD_APERTA_INTF_XLPPI       44
#define PHYMOD_APERTA_INTF_AUI_C2C     46
#define PHYMOD_APERTA_INTF_AUI_C2M     47
#define PHYMOD_APERTA_INTF_CEIMR       54
#define PHYMOD_APERTA_INTF_CEILR       55

/* Interface mode flags */
#define PHYMOD_INTF_MODES_FIBER        0x10
#define PHYMOD_INTF_MODES_BACKPLANE    0x200
#define PHYMOD_INTF_MODES_COPPER       0x400

/* ==========================================================================
 * Core init flags
 * ========================================================================== */
#define PHYMOD_CORE_INIT_F_UNTIL_FW_LOAD           0x1
#define PHYMOD_CORE_INIT_F_RESUME_AFTER_FW_LOAD    0x2
#define PHYMOD_CORE_INIT_F_FIRMWARE_LOAD_VERIFY    0x4
#define PHYMOD_CORE_INIT_F_EXECUTE_PASS1           0x8
#define PHYMOD_CORE_INIT_F_EXECUTE_PASS2           0x10
#define PHYMOD_CORE_INIT_F_EXECUTE_FW_LOAD         0x80
#define PHYMOD_CORE_INIT_F_RESET_CORE_FOR_FW_LOAD  0x100
#define PHYMOD_CORE_INIT_F_FW_LOAD_END             0x200
#define PHYMOD_CORE_INIT_F_BROADCAST               0x400

#define PHYMOD_CORE_INIT_F_UNTIL_FW_LOAD_GET(c)        ((c)->flags & PHYMOD_CORE_INIT_F_UNTIL_FW_LOAD ? 1 : 0)
#define PHYMOD_CORE_INIT_F_RESUME_AFTER_FW_LOAD_GET(c) ((c)->flags & PHYMOD_CORE_INIT_F_RESUME_AFTER_FW_LOAD ? 1 : 0)
#define PHYMOD_CORE_INIT_F_EXECUTE_FW_LOAD_GET(c)      ((c)->flags & PHYMOD_CORE_INIT_F_EXECUTE_FW_LOAD ? 1 : 0)
#define PHYMOD_CORE_INIT_F_RESET_CORE_FOR_FW_LOAD_GET(c) ((c)->flags & PHYMOD_CORE_INIT_F_RESET_CORE_FOR_FW_LOAD ? 1 : 0)
#define PHYMOD_CORE_INIT_F_FW_LOAD_END_GET(c)          ((c)->flags & PHYMOD_CORE_INIT_F_FW_LOAD_END ? 1 : 0)

/* phy_sdk simplified init-mode constants (same numeric values as above) */
#define CORE_INIT_F_RESET_CORE           PHYMOD_CORE_INIT_F_RESET_CORE_FOR_FW_LOAD
#define CORE_INIT_F_UNTIL_FW_LOAD        PHYMOD_CORE_INIT_F_UNTIL_FW_LOAD
#define CORE_INIT_F_EXECUTE_FW_LOAD      PHYMOD_CORE_INIT_F_EXECUTE_FW_LOAD
#define CORE_INIT_F_RESUME_AFTER_FW_LOAD PHYMOD_CORE_INIT_F_RESUME_AFTER_FW_LOAD
#define CORE_INIT_F_FW_LOAD_END          PHYMOD_CORE_INIT_F_FW_LOAD_END

/* ==========================================================================
 * Core init config / core status
 * ========================================================================== */
typedef struct plp_aperta_phymod_core_init_config_s {
    plp_aperta_phymod_lane_map_t    lane_map;
    plp_aperta_phymod_polarity_t    polarity_map;
    plp_aperta_phymod_firmware_load_method_t firmware_load_method;
    plp_aperta_phymod_phy_inf_config_t interface;   /**< init values for all lanes */
    uint32_t flags;                 /**< PHYMOD_CORE_INIT_F_* */
    uint8_t  core_init_mode;        /**< requested init mode (mirrors flags) */
    uint8_t  core_mode;
    uint32_t pll0_div_init_value;
    uint32_t pll1_div_init_value;
} plp_aperta_phymod_core_init_config_t;

typedef struct plp_aperta_phymod_core_status_s {
    uint32_t pmd_active;
} plp_aperta_phymod_core_status_t;

/* ==========================================================================
 * APERTA constants
 * ========================================================================== */
#define APERTA_MAX_LANES            8
#define APERTA_MICRO_RETRY_COUNT    1000
#define APERTA_HEADER_SIZE          64
#define APERTA_FW_ALREADY_DOWNLOADED 0xFAD
#define APERTA_MSGOUT_HDR_ERR       0x0E0E
#define APERTA_MSGOUT_DWNLD_DONE    0x0303
#define APERTA_MSGOUT_HEADER        0x0EAD
#define APERTA_MSGOUT_FLASH         0xF1AC
#define APERTA_TVCO_PLL_INDEX       1
#define APERTA_TSCBH_PLL_DIVNONE    0xFFFFFFFFu

#define APERTA_SPEED_400G           400000
#define APERTA_SPEED_200G           200000
#define APERTA_SPEED_100G           100000
#define APERTA_SPEED_50G            50000
#define APERTA_SPEED_40G            40000
#define APERTA_SPEED_26G            26000
#define APERTA_SPEED_25G            25000
#define APERTA_SPEED_20G            20000
#define APERTA_SPEED_10G            10000
#define APERTA_SPEED_VCO_400G       400000   /* default VCO speed bookkeeping */

#define APERTA_W_FW                 1

#define PORTMOD_PORT_ADD_F_INIT_PASS1   0x1
#define PORTMOD_PORT_ADD_F_INIT_PASS2   0x2

/* ==========================================================================
 * MAC flow control
 * ========================================================================== */
typedef enum {
    bcmplpFlowcontrolTerminateGenerate = 1,
    bcmplpFlowcontrolPassthrough = 0
} bcm_plp_mac_flow_control_t;

/* ==========================================================================
 * phy_sdk access / bus / fw types
 * ========================================================================== */
#define PHY_SDK_MAX_LANE        8

typedef int (*phy_sdk_bus_read_t)(void *user_acc, uint32_t phy_addr, uint32_t reg, uint32_t *data);
typedef int (*phy_sdk_bus_write_t)(void *user_acc, uint32_t phy_addr, uint32_t reg, uint32_t data);

typedef struct {
    void     *platform_ctxt;   /**< user_acc passed to bus callbacks */
    uint32_t  phy_addr;        /**< PHY address (PHYAD) */
    uint32_t  if_side;         /**< PHY_SDK_SYSTEM_SIDE / PHY_SDK_LINE_SIDE */
    uint32_t  lane_map;        /**< lane bitmap */
} phy_sdk_access_t;

typedef struct {
    uint32_t num_of_lanes;
    uint32_t lane_map_rx[PHY_SDK_MAX_LANE];
    uint32_t lane_map_tx[PHY_SDK_MAX_LANE];
} phy_sdk_laneswap_map_t;

typedef struct {
    uint32_t fw_version;
    uint32_t fw_crc;
} phy_sdk_fw_info_t;

typedef struct {
    uint32_t pll1_vco_rate;     /**< 0=20.625G, 1=25.781G, 2=26.562G */
    uint32_t tx_drv_supply;     /**< TX driver supply select */
    uint32_t macsec_static_bypass;
} phy_sdk_aperta_fw_init_t;

typedef struct {
    void *fw_init_params;       /**< points to phy_sdk_aperta_fw_init_t */
    int   firmware_load_method;
} phy_sdk_fw_load_type_t;

typedef enum {
    FW_LOAD_NONE = 0,
    FW_LOAD_UNICAST,
    FW_LOAD_EEPROM,
    FW_LOAD_UPGRADE,
    FW_LOAD_SKIP
} fw_load_method_e;

typedef enum {
    FW_BCAST_CORE_RESET = 0,
    FW_BCAST_ENABLE,
    FW_BCAST_FW_EXECUTE,
    FW_BCAST_FW_VERIFY,
    FW_BCAST_END
} fw_broadcast_stage_e;

/* ==========================================================================
 * phy_sdk API (implemented in phy_sdk.c)
 * ========================================================================== */
void phy_sdk_set_bus_funcs(phy_sdk_bus_read_t read_fn, phy_sdk_bus_write_t write_fn);

/* Shared type conversion: phy_sdk_access_t -> plp_aperta_phymod_phy_access_t.
 *(the coreaccess struct was identical to the PHY access struct). */
void phy_sdk_acc_to_phy(const phy_sdk_access_t *src, plp_aperta_phymod_phy_access_t *dst);

int phy_sdk_core_firmware_info_get(phy_sdk_access_t *access, phy_sdk_fw_info_t *fw_info);
int phy_sdk_rxtx_laneswap_set(phy_sdk_access_t *access, phy_sdk_laneswap_map_t *lm);
int phy_sdk_rxtx_polarity_set(phy_sdk_access_t *access, uint32_t tx_pol, uint32_t rx_pol);
int phy_sdk_rxtx_polarity_get(phy_sdk_access_t *access, uint32_t *tx_pol, uint32_t *rx_pol);
int phy_sdk_core_init(phy_sdk_access_t *access, int init_mode, int fw_load_method, const phy_sdk_aperta_fw_init_t *fw_init);
int phy_sdk_cleanup(phy_sdk_access_t *access);
int phy_sdk_fw_load(phy_sdk_access_t *access, phy_sdk_aperta_fw_init_t *fw_init, fw_load_method_e load_method);

int plp_aperta_reg32_read(const plp_aperta_phymod_phy_access_t *phy, uint32_t reg_addr, uint32_t *data);
int plp_aperta_reg32_write(const plp_aperta_phymod_phy_access_t *phy, uint32_t reg_addr, uint32_t data);
int plp_aperta_direct_reg_read(const plp_aperta_phymod_phy_access_t *phy, uint32_t reg_addr, uint32_t *reg_data);
int plp_aperta_direct_reg_write(const plp_aperta_phymod_phy_access_t *phy, uint32_t reg_addr, uint32_t reg_data);

int plp_aperta_phymod_util_lane_config_get(const plp_aperta_phymod_access_t *phys, int *start_lane, int *num_of_lane);
int plp_aperta_tscbh_core_lane_map_set(const plp_aperta_phymod_phy_access_t *core, const plp_aperta_phymod_lane_map_t *lane_map);
int plp_aperta_tscbh_core_lane_map_get(const plp_aperta_phymod_phy_access_t *core, plp_aperta_phymod_lane_map_t *lane_map);
int plp_aperta_tscbh_phy_polarity_set(const plp_aperta_phymod_phy_access_t *phy, const plp_aperta_phymod_polarity_t *polarity);
int plp_aperta_tscbh_phy_polarity_get(const plp_aperta_phymod_phy_access_t *phy, plp_aperta_phymod_polarity_t *polarity);
int plp_aperta_tscbh_phy_cl72_set(const plp_aperta_phymod_phy_access_t *phy, uint32_t cl72_en);

int plp_aperta_sw_intf_set(const plp_aperta_phymod_phy_access_t *phy, int if_type);
int plp_aperta_sw_intf_get(const plp_aperta_phymod_phy_access_t *phy, uint8_t lane_index, int *if_type);
int plp_aperta_phy_interface_config_set(const plp_aperta_phymod_phy_access_t *phy, const plp_aperta_phymod_phy_inf_config_t *config);

int plp_aperta_pm_interface_config_set(const plp_aperta_phymod_phy_access_t *phy, const plp_aperta_phymod_phy_inf_config_t *config);
int plp_aperta_phy_macsec_init(const plp_aperta_phymod_phy_access_t *phy, int macsec_static_bypass);
int plp_aperta_dload_fw(const plp_aperta_phymod_phy_access_t *core, int fw_method);
int plp_aperta_tscbh_core_init_pass1_self(const plp_aperta_phymod_phy_access_t *core);
int plp_aperta_tscbh_core_init_pass2_self(const plp_aperta_phymod_phy_access_t *core, const plp_aperta_phymod_core_init_config_t *init_config);
int _pm_msg_send(const plp_aperta_phymod_phy_access_t *phy, uint8_t function, uint8_t operation, uint8_t *tx_msg, uint8_t *rx_msg, uint8_t *result);

/* soft */
void _pm_get_port_from_lm_sp(uint32_t speed, uint32_t lane_rate, uint32_t lm, uint32_t *prt, uint32_t *ln_sel);
/* ==========================================================================
 * SDK constants
 * ========================================================================== */
#define PHY_SDK_ALL_LANE_MAP    0xFF
/* Match the reference SDK (bcm_common_defines.h): BCM_LINE_SIDE=0,
 * BCM_SYSTEM_SIDE=1.  Access helpers map if_side==PHY_SDK_SYSTEM_SIDE ->
 * phymodPortLocSys, else -> phymodPortLocLine.  phy_fw_load leaves if_side==0
 * (memset), which MUST be the LINE side so PASS1 firmware download runs. */
#define PHY_SDK_SYSTEM_SIDE     1
#define PHY_SDK_LINE_SIDE       0
#define PHY_SDK_SUCCESS         STATUS_SUCCESS

#endif /* APERTA_PHY_SDK_H */