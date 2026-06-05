/*
 *
 * $Id:$
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
 */

#ifndef _PORTMOD_INTERNAL_H_
#define _PORTMOD_INTERNAL_H_

#include "../include/portmod.h"
#include "../include/pm8x50_shared.h"

#define MAX_PMS_PER_PHY (3)
#define MAX_VARS_PER_BUFFER (16)
#define VERSION(_ver) (_ver)
#define PORTMOD_IS_LEGACY_PHY_GET(a) \
    ((portmod_default_user_access_t *)((a)->user_acc))->is_legacy_phy_present
#define PORTMOD_IS_LEGACY_PHY_SET(a) \
    ((portmod_default_user_access_t *)((a)->user_acc))->is_legacy_phy_present = 1
#define PORTMOD_USER_ACC_LPORT_GET(a) \
    ((portmod_default_user_access_t *)((a)->user_acc))->port
#define PORTMOD_USER_ACC_LPORT_SET(a, p) \
    ((portmod_default_user_access_t *)((a)->user_acc))->port = p
#define PORTMOD_USER_ACC_UNIT_GET(a) \
    ((portmod_default_user_access_t *)((a)->user_acc))->unit
#define PORTMOD_USER_ACC_CMD_FOR_PHY_GET(a, idx) \
    ((portmod_default_user_access_t *)((a[idx].access).user_acc))->cmd_for_phy
#define PORTMOD_USER_ACC_CMD_FOR_PHY_SET(a, idx) \
    ((portmod_default_user_access_t *)((a[idx].access).user_acc))->cmd_for_phy = 1
#define PORTMOD_USER_ACC_CMD_FOR_PHY_CLR(a, idx) \
    ((portmod_default_user_access_t *)((a[idx].access).user_acc))->cmd_for_phy = 0

#define PORTMOD_PORT_IS_DEFAULT_TX_PARAMS_UPDATED(port_dynamic_state)     (port_dynamic_state & 0x1)
#define PORTMOD_PORT_DEFAULT_TX_PARAMS_UPDATED_SET(port_dynamic_state)    (port_dynamic_state |= 0x1)
#define PORTMOD_PORT_DEFAULT_TX_PARAMS_UPDATED_CLR(port_dynamic_state)    (port_dynamic_state &= 0xfffe)

#define PORTMOD_PORT_IS_AUTONEG_MODE_UPDATED(port_dynamic_state)          (port_dynamic_state & 0x2)
#define PORTMOD_PORT_AUTONEG_MODE_UPDATED_SET(port_dynamic_state)         (port_dynamic_state |= 0x2)
#define PORTMOD_PORT_AUTONEG_MODE_UPDATED_CLR(port_dynamic_state)         (port_dynamic_state &= 0xfffd)

typedef struct pm4x10_s *pm4x10_t;
typedef struct pm4x10q_s *pm4x10q_t;
typedef struct pm4x25_s *pm4x25_t;
typedef struct pm12x10_s *pm12x10_t;
typedef struct dnx_fabric_s *dnx_fabric_t;
typedef struct pmOsIlkn_s *pmOsIlkn_t;
typedef struct pm_qtc_s *pm_qtc_t;
typedef struct pm8x50_s *pm8x50_t;

typedef union pm_db_u{
    pm4x10_t      pm4x10_db;
    pm4x10q_t      pm4x10q_db;
    dnx_fabric_t  dnx_fabric;
    pm4x25_t      pm4x25_db;
    pm12x10_t     pm12x10_db;
    pmOsIlkn_t    pmOsIlkn_db;
    pm_qtc_t      pm_qtc_db;
    pm8x50_t        pm8x50_db;
}pm_db_t;

/* This structure cntain specific PM state.
    it's pointed from pms array in portmod.c */
struct pm_info_s{
    portmod_dispatch_type_t type; /* PM type (used manly for dispatching)*/
    int unit; /* PM unit ID */
    int wb_buffer_id; /* Buffer id is given for each PM (see warmboot description in portmod.c) */
    int wb_vars_ids[MAX_VARS_PER_BUFFER]; /* Allotcaed WB variables for this PM */
    pm_db_t pm_data; /* PM internal state. This information is internal, and mainted in the speicifc PM c code*/
};

typedef struct pm_info_s *pm_info_t;

/* When port macro is added to the PMM the user passing in specific PM required information.
   In some cases the PMM rebuild another structure before sending the information to the PM
   the *_internal_t strucutres represents the internal information as the PMM send to the PM
   (translation code can be found in portmod.c */

/* PM4x25: no translation is required */
typedef portmod_pm4x25_create_info_t portmod_pm4x25_create_info_internal_t;

/* PM4x10: no translation is required */
typedef portmod_pm4x10_create_info_t portmod_pm4x10_create_info_internal_t;

/* QTC: no translation is required */
typedef portmod_pm_qtc_create_info_t portmod_pm_qtc_create_info_internal_t;

/* DNX fabric internal create structure*/
typedef struct portmod_dnx_fabric_create_info_internal_s{
    plp_aperta_phymod_ref_clk_t ref_clk; /**< SerDes quad ref clock */
    plp_aperta_phymod_access_t access; /**< phymod access structure; defines the register access for the SerDes Core */
    plp_aperta_phymod_lane_map_t lane_map;
    plp_aperta_phymod_firmware_load_method_t fw_load_method;
    phymod_firmware_loader_f external_fw_loader; /**< firmware loader that will be used in case that fw_load_method=phymodFirmwareLoadMethodExternal */
    plp_aperta_phymod_polarity_t polarity; /**< Lanes Polarity */
    int fmac_schan_id; /**< FMAC schan id */
    int fsrd_schan_id; /**< FSRD schan id */
    int fsrd_internal_quad; /**< Core instance in FSRD */
    int first_phy_offset;
    int core_index;
    int is_over_nif;
    pm_info_t *pms; /**< PM used for fabric over nif */
}portmod_dnx_fabric_create_info_internal_t;

/* PM4x10Q internal create structure*/
typedef struct portmod_pm4x10q_create_info_internal_s{
    pm_info_t pm4x10;
    uint32_t blk_id;
    void* qsgmii_user_acc;
    void* pm4x10_user_acc;
}portmod_pm4x10q_create_info_internal_t;

/* PM12x10 internal create structure*/
typedef struct portmod_pm12x10_create_info_internal_s{
    pm_info_t pm4x25;
    pm_info_t pm4x10[3];
    uint32_t flags;
    int blk_id;
    int refclk_source;
}portmod_pm12x10_create_info_internal_t;

/* OS ILKN internal create structure*/
typedef struct portmod_os_ilkn_create_info_internal_s{
    int nof_aggregated_pms;
    pm_info_t *pms;
    int wm_high; /**< watermark high value */
    int wm_low; /**< watermark low value */
    uint8_t is_over_fabric;
}portmod_os_ilkn_create_info_internal_t;

/* PM8x50: no translation is required??? */
typedef portmod_pm8x50_create_info_t portmod_pm8x50_create_info_internal_t;

/* Union for internal create structures*/
typedef union portmod_pm_specific_create_info_internal_u{
    portmod_dnx_fabric_create_info_internal_t dnx_fabric;
    portmod_pm4x25_create_info_internal_t pm4x25;
    portmod_pm4x10_create_info_internal_t pm4x10;
    portmod_pm4x10q_create_info_internal_t pm4x10q;
    portmod_pm_qtc_create_info_internal_t pm_qtc;
    portmod_pm12x10_create_info_internal_t pm12x10;
    portmod_os_ilkn_create_info_internal_t os_ilkn;
    portmod_pm8x50_create_info_internal_t pm8x50;
}portmod_pm_specific_create_info_internal_t;

/* the structure which is used as input to portmod_port_macro_add function*/
typedef struct portmod_pm_create_info_internal_s {
    portmod_dispatch_type_t type; /**< PM type */
    int phys; /**< which PHYs belongs to the PM */
    portmod_pm_specific_create_info_internal_t pm_specific_info;
} portmod_pm_create_info_internal_t;

/* PMs can implement xxx_default_bus_update() internal API to update phymod bus
   When this API is implemented the below struct is used as input*/
typedef struct portmod_bus_update_s {
    phymod_firmware_loader_f external_fw_loader;
    plp_aperta_phymod_bus_t* default_bus;
    void* user_acc;
    int blk_id;
} portmod_bus_update_t;

/* External Phy information */
typedef struct portmod_ext_phy_core_info_s
{
    plp_aperta_phymod_core_access_t core_access; /**< core access */
    int is_initialized; /**< Phy is Initialized - need to convert to WB */
} portmod_ext_phy_core_info_t;

typedef struct portmod_xphy_core_info_s {
    plp_aperta_phymod_core_access_t core_access; /**< core access */
    int core_initialized; /**< external phy init status */
    plp_aperta_phymod_ref_clk_t ref_clk; /**< Phy ref clock */
    plp_aperta_phymod_firmware_load_method_t fw_load_method;
    int  force_fw_load; /* flag to specify to force load the FW */
    plp_aperta_phymod_polarity_t polarity; /**< Lanes Polarity */
    plp_aperta_phymod_lane_map_t lane_map; /* lane map information for the xphy */
    int is_initialized; /**< Phy is Initialized - need to convert to WB */
    int wb_var_ids[3];
    uint8_t gearbox_enable;
    uint8_t pin_compatibility_enable;
    uint8_t phy_mode_reverse;
    plp_aperta_phymod_core_init_config_t core_config;
    uint16_t primary_core_num;
} portmod_xphy_core_info_t;

#define PM_DRIVER(pm_info) (__plp_aperta_portmod__dispatch__[(pm_info)->type])

/* Initialize portmod_pm_create_info_internal_t structure */
int portmod_pm_create_info_internal_t_init(int unit, portmod_pm_create_info_internal_t *create_info_internal);

/* Translate PM id to PM type */
int portmod_pm_id_pm_type_get(int unit, int pm_id, portmod_dispatch_type_t *type);

/* Get  internal PM info from a port */
int portmod_pm_info_get(int unit, int port, pm_info_t *pm_info);
/* Get list of PMs attached to a phy */
int portmod_phy_pms_info_get(int unit, int phy, int max_pms, pm_info_t *pms_info, int *nof_pms);

int portmod_pm_info_type_get(int unit, int port, portmod_dispatch_type_t type, pm_info_t* pm_info);
int portmod_pm_info_from_pm_id_get(int unit, int pm_id, pm_info_t* pm_info);
int portmod_port_pm_id_get(int unit, int port, int *pm_id);
int portmod_port_interface_type_get(int unit, int port, soc_port_if_t *interface_t);
int portmod_port_main_core_access_get(int unit, int port, int phyn,
                                      plp_aperta_phymod_core_access_t *core_access,
                                      int *nof_cores);

/*is interface_t type supported by PM should be supported by all PMs types*/
int plp_aperta_portmod_pm_interface_type_is_supported(int unit, pm_info_t pm_info, soc_port_if_t interface_t, int* is_supported);

int plp_aperta_portmod_pm_init(int unit, const portmod_pm_create_info_internal_t* pm_add_info, int wb_buffer_index, pm_info_t pm_info);
int plp_aperta_portmod_pm_destroy(int unit, pm_info_t pm_info);
int plp_aperta_portmod_pm_core_info_get(int unit, pm_info_t pm_info, int phyn, portmod_pm_core_info_t* core_info);
int plp_aperta_portmod_port_attach(int unit, int port, const portmod_port_add_info_t* add_info);
int plp_aperta_portmod_port_detach(int unit, int port);
int plp_aperta_portmod_port_replace(int unit, int port, int new_port);

int portmod_next_wb_var_id_get(int unit, int *var_id);
int plp_aperta_portmod_ext_phy_lane_attach_to_pm(int unit, pm_info_t pm_info, int iphy, int phyn, const portmod_lane_connection_t* lane_connection);
int plp_aperta_portmod_ext_phy_lane_detach_from_pm(int unit, pm_info_t pm_info, int iphy, int phyn, portmod_lane_connection_t* lane_connection);

/*!
 * plp_aperta_portmod_port_mac_rsv_mask_set
 *
 * @brief  portmod port rsv mask set
 *
 * @param [in]  unit            - unit id
 * @param [in]  port            - logical port
 * @param [in]  rsv_mask        - rsv mask
 */
int plp_aperta_portmod_port_mac_rsv_mask_set(int unit, int port, uint32_t rsv_mask);

/*!
 * plp_aperta_portmod_port_mib_reset_toggle
 *
 * @brief  portmod port mib reset toggle
 *
 * @param [in]  unit            - unit id
 * @param [in]  port            - logical port
 * @param [in]  port_index      - internal port
 */
int plp_aperta_portmod_port_mib_reset_toggle(int unit, int port, int port_index);

int plp_aperta_portmod_xphy_lane_attach_to_pm(int unit, pm_info_t pm_info, int iphy, int phyn, const portmod_xphy_lane_connection_t* lane_connection);
int plp_aperta_portmod_xphy_lane_detach_from_pm(int unit, pm_info_t pm_info, int iphy, int phyn, portmod_xphy_lane_connection_t* lane_connection);
int portmod_max_pms_get(int unit, int* max_pms);
int portmod_eyescan_diag_dispatch(int unit, soc_port_t port, args_t *a);
int portmod_phy_pm_type_get(int unit, int phy, portmod_dispatch_type_t *type);
int portmod_xphy_addr_set(int unit, int idx, int xphy_addr);
int portmod_xphy_db_addr_set(int unit, int idx, int xphy_addr);
int portmod_xphy_all_valid_phys_get(int unit, int *active_phys);
int portmod_xphy_valid_phy_set (int unit, int xphy_idx, int valid);
int portmod_xphy_valid_phy_get (int unit, int xphy_idx, int *valid);
int portmod_xphy_db_addr_get(int unit, int xphy_idx, int* xphy_addr);


/*!
 * portmod_pm_is_in_pm12x10
 *
 * @brief Get whether the Port Macro is part of PM12x10
 *
 * @param [in]  unit            - unit id
 * @param [in]  pm_info         -
 * @param [out]  in_pm12x10     -
 */
int portmod_pm_is_in_pm12x10(int unit, pm_info_t pm_info, int* in_pm12x10);

/*!
 * portmod_port_lane_count_get
 *
 * @brief Get the number of lanes belong to  a logical port.
 *
 * @param [in]  unit            - unit id
 * @param [in]  port            - logical port
 * @param [in]  line_side       - line side 1, sys_side 0
 * @param [out]  num_lanes       - num of line side lanes
 */
int portmod_port_lane_count_get(int unit, int port, int line_side, int* num_lanes);


#ifdef FW_BCAST_DOWNLOAD
int portmod_fw_bcast(int unit, pm_info_t pm_info, const portmod_sbus_bcast_config_t* sbus_bcast_cfg);
#endif

/*!
 * portmod_pm_capability_get
 *
 * @brief Get PortMacro specific capabilites
 *
 * @param [in]  unit            - unit id
 * @param [out]  pm_cap          - PM specific capabilities
 */
/*int portmod_pm_specific_capability_get(int unit,
                                       portmod_pm_capability_t* pm_cap);*/

#endif /*_PORTMOD_INTERNAL_H_*/
