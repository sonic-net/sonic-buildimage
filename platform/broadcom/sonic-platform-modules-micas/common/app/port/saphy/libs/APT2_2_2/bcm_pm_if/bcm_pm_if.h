/*
 *         
 * $Id: bcm_pm_if.h $
 * 
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$ 
 *         
 *     
 */

#ifndef BCM_PM_IF_H
#define BCM_PM_IF_H

#include <phymod/phymod_diagnostics.h>
#include <phymod/phymod.h>
#include <phymod/phymod_symbols.h>
#include <phymod/phymod_diag.h>
#include <phymod/phymod_util.h>
#include "bcm_pm_if_api.h"

#define BCM_PM_IF_CHIP_NAME              "NA"
#define BCM_PM_IF_CHIP_VER_NO            0
#define BCM_PM_IF_API_VER_NO             0
#define BCM_PM_IF_ENAHAN_VER_NO          0
#define BCM_PM_IF_MAJOR_VER_NO           2
#define BCM_PM_IF_MINOR_VER_NO           2
#define PHYMOD_IF_CONFIG_MAX_PHYS        BCM_PM_IF_MAX_PHY
#define BCM_PM_INTERFACE_SIDE_SHIFT      31
#define BCM_PM_IF_MAX_PHY                1024

#define BCM_PHY_ADDR_MASTER_SHIFT        16
#define BCM_PHY_ADDR_MASTER_MASK         0xFFFF0000

#if defined(PHYMOD_XGBASET_SUPPORT)

/* logical / physical PHY address */
#define BCM_PHY_ADDR_LOGICAL_MAX         BCM_PM_IF_MAX_PHY   /* 0x0400 */
#define BCM_PHY_ADDR_LOGICAL_MASK        (BCM_PHY_ADDR_LOGICAL_MAX - 1)
#define BCM_PHY_ADDR_LOGICAL(_a)         ((_a) & BCM_PHY_ADDR_LOGICAL_MASK)
#define BCM_PHY_ADDR_PHYSICAL_SHIFT      16
#define BCM_PHY_ADDR_PHYSICAL_MAX        0x00000020
#define BCM_PHY_ADDR_PHYSICAL_MASK       (BCM_PHY_ADDR_PHYSICAL_MAX - 1)
#define BCM_PHY_ADDR_PHYSICAL(_a)        ((_a) & BCM_PHY_ADDR_PHYSICAL_MASK)
#define BCM_PHY_ADDR_MASTER_SHIFT        16
#define BCM_PHY_ADDR_MASTER(_a)          (((_a) >> BCM_PHY_ADDR_MASTER_SHIFT) \
                                                 & BCM_PHY_ADDR_LOGICAL_MASK )
#define BCM_PHY_IS_MASTER_PORT(_a)       (BCM_PHY_ADDR_LOGICAL(_a) == BCM_PHY_ADDR_MASTER(_a))

#define BCM_PHY_ID_VALID                            (0x1U << 0)
#define BCM_PHY_MASTER_PORT                         (0x1U << 30)
#define BCM_PHY_PHYSICAL_ADDR_GIVEN_BY_APP          (0x1U << 31)
#define BCM_PHY_IS_PHYSICAL_ADDR_GIVEN_BY_APP(_f)   (BCM_PHY_PHYSICAL_ADDR_GIVEN_BY_APP & (_f))


#endif  /*  XGBASET_SUPPORT  */

#define BCM_PM_ERR_CHECK_PHYID_RET_INVALID(_PHY_ID_IDX) \
    do { \
        if ((_PHY_ID_IDX) >= BCM_PM_IF_MAX_PHY) {\
            PHYMOD_DEBUG_ERROR(("Invalid PHY_ID: 0x%X\n", _PHY_ID_IDX));\
            return BCM_PM_IF_INVALID_PHY;         \
        }\
    } while(0)

#define BCM_PM_MUTEX_LOCK(_PHY_ID, _P_CTXT) \
    do {\
        BCM_PM_ERR_CHECK_PHYID_RET_INVALID(_PHY_ID);    \
        if ((plp_aperta2_phy_ctrl.phy[_PHY_ID] != NULL) && (plp_aperta2_phy_ctrl.phy[_PHY_ID]->mutex_info.mutex_take != NULL)) {\
           PHYMOD_IF_ERR_RETURN(plp_aperta2_phy_ctrl.phy[_PHY_ID]->mutex_info.mutex_take(_PHY_ID, _P_CTXT));\
        }\
    } while (0)

#define BCM_PM_MUTEX_UNLOCK(_PHY_ID, _P_CTXT) \
    do {\
        BCM_PM_ERR_CHECK_PHYID_RET_INVALID(_PHY_ID);    \
        if ((plp_aperta2_phy_ctrl.phy[_PHY_ID] != NULL) && (plp_aperta2_phy_ctrl.phy[_PHY_ID]->mutex_info.mutex_give != NULL)) {\
           PHYMOD_IF_ERR_RETURN(plp_aperta2_phy_ctrl.phy[_PHY_ID]->mutex_info.mutex_give(_PHY_ID, _P_CTXT));\
        }\
    } while (0)

#define BCM_PM_RETURN_WITH_ERR(A, B) \
    do {   \
        PHYMOD_DEBUG_ERROR(B);     \
        if (((rv) = (A)) != BCM_PM_IF_SUCCESS) {\
            goto ERR;\
        }\
    } while (0)

#define BCM_PM_IF_ERR_RETURN(A) \
    do {   \
        if (((rv) = (A)) != BCM_PM_IF_SUCCESS) {\
            goto ERR; \
        } \
    } while (0)

#define BCM_PM_ERR_RETURN_EXIST_PHY(_EXIST_PHY, _PHY_ID_IDX) \
    do { \
        if ((_PHY_ID_IDX) >= BCM_PM_IF_MAX_PHY) {\
            PHYMOD_DEBUG_ERROR(("Max PHY Reached\n"));\
            (rv) = BCM_PM_IF_INTERNAL;\
            goto ERR;\
        }\
        if ((_EXIST_PHY) != 1) {\
            PHYMOD_DEBUG_ERROR(("PHY does not exist\n"));\
            (rv) = BCM_PM_IF_PHY_NA;\
            goto ERR;\
        }\
    } while(0)

#define BCM_PM_CHK_INIT_DONE(_PHY_ID)\
    do {\
        if (_PHY_ID >= BCM_PM_IF_MAX_PHY) {\
            PHYMOD_DEBUG_ERROR(("MAX PHY reached\n"));\
            return BCM_PM_IF_INTERNAL;\
        }\
        if ((_plp_aperta2_phyid_list[_PHY_ID].valid) && (_plp_aperta2_phyid_list[_PHY_ID].phy_id == _PHY_ID)) {\
            PHYMOD_DEBUG_ERROR(("This API needs to be called before bcm_pm_if_init\n"));\
            return BCM_PM_IF_PHY_EXISTING;\
        } \
    } while (0)


typedef struct _bcm_if_phymod_core_s {
    plp_aperta2_phymod_core_access_t pm_core;
    plp_aperta2_phymod_bus_t pm_bus;
    int ref_cnt;
    int init;
    int unit;
    int port;
    int (*read)(void *, uint32_t, uint32_t, uint32_t*);
    int (*write)(void *, uint32_t, uint32_t, uint32_t);
    int (*wrmask)(void *, uint32_t, uint32_t, uint32_t, uint32_t);
    plp_aperta2_phymod_core_init_config_t init_config;
} _bcm_if_phymod_core_t;

typedef struct bcm_if_phymod_phy_s {
    plp_aperta2_phymod_phy_access_t pm_phy;
    _bcm_if_phymod_core_t *core;
    plp_aperta2_phymod_phy_init_config_t  init_config;
    void* static_config;
    bcm_plp_mutex_info_t mutex_info;
    uint32_t valid;
} _bcm_if_phymod_phy_t;

typedef struct bcm_if_phymod_ctrl_s {
    int unit;
    int num_phys;
    _bcm_if_phymod_phy_t *phy[PHYMOD_IF_CONFIG_MAX_PHYS];
    plp_aperta2_phymod_symbols_t *symbols;
} bcm_if_phymod_ctrl_t;

#if defined(PHYMOD_XGBASET_SUPPORT)

typedef struct bcm_if_phymod_phy_id_s {
    uint32_t        phy_id;         /*  logical PHY address */
    uint32_t        physical_addr;  /* physical PHY address */
    uint32_t        valid;
} bcm_if_phymod_phy_id_t;

#else  /* ! XGBASET_SUPPORT  */

typedef struct bcm_if_phymod_phy_id_s {
    uint32_t phy_id;
    unsigned char valid;
} bcm_if_phymod_phy_id_t;

#endif  /*  XGBASET_SUPPORT  */

typedef enum {
    bcm_if_load_type_unicast,
    bcm_if_load_type_broadcast
}bcm_if_firmare_load_type_t;

typedef struct bcm_if_port_information_s {
    int line_id; /**< line side identifier of a port */
    int sys_id; /**< System side identifier of a port */
    int logical_port; /**< logical port associated to line_id and sys_id */
    bcm_plp_octal_crossing_t crossing; /**< octal crossing status of the port specified */
} bcm_if_port_information_t;

extern bcm_if_phymod_ctrl_t plp_aperta2_phy_ctrl;
extern bcm_if_phymod_phy_id_t _plp_aperta2_phyid_list[BCM_PM_IF_MAX_PHY];


#if defined(PHYMOD_XGBASET_SUPPORT)
void _bcm_pm_if_init_phy_id_idx(uint32_t phy_id);
void _bcm_plp_aperta2_pm_if_get_phy_id_idx(uint32_t phy_id_combo, uint32_t *idx, uint32_t *exist_phy);
int  _bcm_pm_if_phy_id_is_master(uint32_t phy_id);
#else  /* ! XGBASET_SUPPORT  */
void _bcm_plp_aperta2_pm_if_get_phy_id_idx(uint32_t phy_id, uint32_t *idx, uint32_t *exist_phy);
#endif  /*  XGBASET_SUPPORT  */

void _bcm_plp_aperta2_pm_if_core_init(_bcm_if_phymod_core_t *core, plp_aperta2_phymod_bus_t *core_bus, uint32_t phy_addr, void *user_acc);
int _bcm_plp_aperta2_if_phymod_phy_create(_bcm_if_phymod_phy_t **phy);
int _bcm_plp_aperta2_if_phymod_core_create(_bcm_if_phymod_core_t **core);
int _bcm_plp_aperta2_pm_phy_init(_bcm_if_phymod_phy_t *phy, bcm_pm_firmware_load_method_t firmware_load_method, void *init_param);
int _bcm_plp_aperta2_phy_user_phymod_ability(unsigned int tech_ability, unsigned int fec_ability, unsigned short pause_ability, bcm_plp_an_config_t an_config, plp_aperta2_phymod_autoneg_ability_t* ability);
int _bcm_plp_aperta2_phy_phymod_user_ability(plp_aperta2_phymod_autoneg_ability_t ability, unsigned short *fec_ability, unsigned short *pause_ability, bcm_plp_an_config_t* an_config);

int bcm_plp_aperta2_phy_get_phy_type (bcm_plp_access_t phy_info, plp_aperta2_phymod_dispatch_type_t *phy_type,
                         int (*read)(void* user_acc, unsigned int core_addr, unsigned int reg_addr, unsigned int* val),int (*write)(void* user_acc, unsigned int core_addr, unsigned int reg_addr, unsigned int val));
uint8_t _bcm_plp_aperta2_pm_if_get_right_most_set_bit_pos(uint32_t n);
#ifdef PHYMOD_GALLARDO28_SUPPORT
int _gallardo28_get_single_pmd_mode(bcm_plp_access_t phy_info,
                                            int (*read)(void* user_acc, unsigned int core_addr, unsigned int reg_addr, unsigned int* val),  unsigned int *pmd_mode);
#endif

int _bcm_plp_aperta2_pm_if_get_port_information(bcm_plp_access_t phy_info, bcm_if_port_information_t* port_information);

#define BCM_SET_INTF_SIDE(_phy_ctrl,_if_side) \
    do{ \
        _phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags &= ~(1 << BCM_PM_INTERFACE_SIDE_SHIFT); \
        _phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags |= (_if_side << BCM_PM_INTERFACE_SIDE_SHIFT); \
    }while(0)

#define BCM_SET_CORE_INTF_SIDE(_phy_ctrl,_if_side) \
    do{ \
        _phy_ctrl.phy[phy_id_idx]->core->pm_core.access.flags &= ~(1 << BCM_PM_INTERFACE_SIDE_SHIFT); \
        _phy_ctrl.phy[phy_id_idx]->core->pm_core.access.flags |= (_if_side << BCM_PM_INTERFACE_SIDE_SHIFT); \
    }while(0)

#if defined(PHYMOD_XGBASET_SUPPORT)  /* * * * * * * * * * * * * * * * * * * * * */

#define BCM_PLP_PHY_GET_P_CTXT(_PI, IDX, E_P)                              \
    do {                                                                   \
        uint32_t  laddr = BCM_PHY_ADDR_LOGICAL(_PI.phy_addr);              \
        if ( laddr >= BCM_PM_IF_MAX_PHY ) {                                \
            PHYMOD_DEBUG_ERROR(("Invalid PHY. .\n"));                      \
            rv = BCM_PM_IF_INVALID_PHY;                                    \
            goto ERR;                                                      \
        }                                                                  \
        _bcm_plp_aperta2_pm_if_get_phy_id_idx(_PI.phy_addr, &IDX, &E_P);               \
        if (E_P != 1) {                                                    \
            _plp_aperta2_phyid_list[laddr].valid = 0;                                  \
            PHYMOD_DEBUG_ERROR(("PHY does not exist\n"));                  \
            rv = BCM_PM_IF_PHY_NA;                                         \
            goto ERR;                                                      \
        }                                                                  \
        if (IDX == BCM_PM_IF_MAX_PHY) {                                    \
            PHYMOD_DEBUG_ERROR(("MAX PHY reached\n"));                     \
            rv = BCM_PM_IF_INTERNAL;                                       \
            goto ERR;                                                      \
        }                                                                  \
        if (_PI.platform_ctxt == NULL) {                                   \
            _PI.platform_ctxt = plp_aperta2_phy_ctrl.phy[IDX]->pm_phy.access.user_acc; \
        }                                                                  \
    } while (0)

#define BCM_PLP_FILL_PHY_ACCESS(_PI, IDX)                                               \
    do {                                                                                \
        int is_line_side = (_PI.if_side == 0 || _PI.if_side == 0xFF) ? TRUE : FALSE;    \
        IDX &= BCM_PHY_ADDR_LOGICAL_MASK;                                               \
        if ( _PI.platform_ctxt != NULL ) {                                              \
            plp_aperta2_phy_ctrl.phy[IDX]->pm_phy.access.user_acc        = _PI.platform_ctxt;       \
            plp_aperta2_phy_ctrl.phy[IDX]->core->pm_core.access.user_acc = _PI.platform_ctxt;       \
        }                                                                               \
        plp_aperta2_phy_ctrl.phy[IDX]->pm_phy.access.lane_mask = _PI.lane_map;                      \
        plp_aperta2_phy_ctrl.phy[IDX]->pm_phy.access.flags &= ~(1U << BCM_PM_INTERFACE_SIDE_SHIFT); \
        plp_aperta2_phy_ctrl.phy[IDX]->pm_phy.access.flags |=                                       \
                                ( (is_line_side ? BCM_LINE_SIDE : BCM_SYSTEM_SIDE)      \
                                  << BCM_PM_INTERFACE_SIDE_SHIFT );                     \
        plp_aperta2_phy_ctrl.phy[IDX]->pm_phy.port_loc = (is_line_side) ? phymodPortLocLine         \
                                                            : phymodPortLocSys;         \
        plp_aperta2_phy_ctrl.phy[IDX]->pm_phy.access.misc = &(_plp_aperta2_phyid_list[IDX]);                    \
    } while (0)

#define BCM_PLP_CORE_GET_P_CTXT(_PI, IDX, E_P)                                    \
    do {                                                                          \
        _bcm_plp_aperta2_pm_if_get_phy_id_idx(_PI.phy_addr, &IDX, &E_P);                      \
        if (E_P != 1) {                                                           \
            _plp_aperta2_phyid_list[BCM_PHY_ADDR_LOGICAL(_PI.phy_addr)].valid = 0;            \
            PHYMOD_DEBUG_ERROR(("PHY does not exist\n"));                         \
            rv = BCM_PM_IF_PHY_NA;                                                \
            goto ERR;                                                             \
        }                                                                         \
        if (IDX == BCM_PM_IF_MAX_PHY) {                                           \
            PHYMOD_DEBUG_ERROR(("MAX PHY reached\n"));                            \
            rv = BCM_PM_IF_INTERNAL;                                              \
            goto ERR;                                                             \
        }                                                                         \
        if (_PI.platform_ctxt == NULL) {                                          \
            _PI.platform_ctxt = plp_aperta2_phy_ctrl.phy[IDX]->core->pm_core.access.user_acc; \
        }                                                                         \
    } while (0)

#define BCM_PLP_FILL_CORE_ACCESS(_PI, IDX)                                        \
    do {                                                                          \
        IDX &= BCM_PHY_ADDR_LOGICAL_MASK;                                         \
        if (_PI.platform_ctxt != NULL) {                                          \
            plp_aperta2_phy_ctrl.phy[IDX]->pm_phy.access.user_acc = _PI.platform_ctxt;        \
            plp_aperta2_phy_ctrl.phy[IDX]->core->pm_core.access.user_acc = _PI.platform_ctxt; \
        }                                                                         \
        plp_aperta2_phy_ctrl.phy[IDX]->core->pm_core.access.lane_mask = _PI.lane_map;         \
        BCM_SET_CORE_INTF_SIDE(plp_aperta2_phy_ctrl,_PI.if_side);                             \
        plp_aperta2_phy_ctrl.phy[IDX]->core->pm_core.port_loc =                               \
                     (_PI.if_side == 0) ? phymodPortLocLine : phymodPortLocSys;   \
        plp_aperta2_phy_ctrl.phy[IDX]->core->pm_core.access.misc = &(_plp_aperta2_phyid_list[IDX]);       \
    } while (0)

#else  /* ! XGBASET_SUPPORT  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */

#define BCM_PLP_PHY_GET_P_CTXT(_PI, IDX, E_P)                              \
    do{                                                                    \
        if (_PI.phy_addr >= BCM_PM_IF_MAX_PHY) {                           \
            PHYMOD_DEBUG_ERROR(("Invalid PHY . . .\n"));                   \
            rv = BCM_PM_IF_INVALID_PHY;                                    \
            goto ERR;                                                      \
        }                                                                  \
        _bcm_plp_aperta2_pm_if_get_phy_id_idx(_PI.phy_addr, &IDX, &E_P);               \
        if (E_P != 1) {                                                    \
            _plp_aperta2_phyid_list[_PI.phy_addr].valid = 0;                           \
            PHYMOD_DEBUG_ERROR(("PHY does not exist\n"));                  \
            rv = BCM_PM_IF_PHY_NA;                                         \
            goto ERR;                                                      \
        }                                                                  \
        if (IDX == BCM_PM_IF_MAX_PHY) {                                    \
            PHYMOD_DEBUG_ERROR(("MAX PHY reached\n"));                     \
            rv = BCM_PM_IF_INTERNAL;                                       \
            goto ERR;                                                      \
        }                                                                  \
        if (_PI.platform_ctxt == NULL) {                                   \
            _PI.platform_ctxt = plp_aperta2_phy_ctrl.phy[IDX]->pm_phy.access.user_acc; \
        }                                                                  \
    }while(0)

#define BCM_PLP_FILL_PHY_ACCESS(_PI, IDX)                             \
    do{                                                               \
        if (_PI.platform_ctxt != NULL) {                              \
            plp_aperta2_phy_ctrl.phy[IDX]->pm_phy.access.user_acc = _PI.platform_ctxt;\
            plp_aperta2_phy_ctrl.phy[IDX]->core->pm_core.access.user_acc = _PI.platform_ctxt;\
        }                                                              \
        plp_aperta2_phy_ctrl.phy[IDX]->pm_phy.access.lane_mask = _PI.lane_map;     \
        plp_aperta2_phy_ctrl.phy[IDX]->pm_phy.access.flags &= ~(1 << BCM_PM_INTERFACE_SIDE_SHIFT);\
        plp_aperta2_phy_ctrl.phy[IDX]->pm_phy.access.flags |= (((_PI.if_side == 0 || _PI.if_side == 0xFF ) ? BCM_LINE_SIDE : BCM_SYSTEM_SIDE) << BCM_PM_INTERFACE_SIDE_SHIFT); \
        (_PI.if_side == 0 ||_PI.if_side == 0xFF ) ? (plp_aperta2_phy_ctrl.phy[IDX]->pm_phy.port_loc = phymodPortLocLine) : (plp_aperta2_phy_ctrl.phy[IDX]->pm_phy.port_loc = phymodPortLocSys);\
    }while(0)

#define BCM_PLP_CORE_GET_P_CTXT(_PI, IDX, E_P)                                    \
    do{                                                                           \
        _bcm_plp_aperta2_pm_if_get_phy_id_idx(_PI.phy_addr, &IDX, &E_P);                      \
        if (E_P != 1) {                                                           \
            _plp_aperta2_phyid_list[_PI.phy_addr].valid = 0;                                  \
            PHYMOD_DEBUG_ERROR(("PHY does not exist\n"));                         \
            rv = BCM_PM_IF_PHY_NA;                                                \
            goto ERR;                                                             \
        }                                                                         \
        if (IDX == BCM_PM_IF_MAX_PHY) {                                           \
            PHYMOD_DEBUG_ERROR(("MAX PHY reached\n"));                            \
            rv = BCM_PM_IF_INTERNAL;                                              \
            goto ERR;                                                             \
        }                                                                         \
        if (_PI.platform_ctxt == NULL) {                                          \
            _PI.platform_ctxt = plp_aperta2_phy_ctrl.phy[IDX]->core->pm_core.access.user_acc; \
        }                                                                         \
    }while(0)

#define BCM_PLP_FILL_CORE_ACCESS(_PI, IDX)                            \
    do{                                                               \
        if (_PI.platform_ctxt != NULL) {                              \
            plp_aperta2_phy_ctrl.phy[IDX]->pm_phy.access.user_acc = _PI.platform_ctxt;\
            plp_aperta2_phy_ctrl.phy[IDX]->core->pm_core.access.user_acc = _PI.platform_ctxt;\
        }                                                              \
        plp_aperta2_phy_ctrl.phy[IDX]->core->pm_core.access.lane_mask = _PI.lane_map;     \
        BCM_SET_CORE_INTF_SIDE(plp_aperta2_phy_ctrl,_PI.if_side);                            \
        (_PI.if_side == 0) ? (plp_aperta2_phy_ctrl.phy[IDX]->core->pm_core.port_loc = phymodPortLocLine) : (plp_aperta2_phy_ctrl.phy[IDX]->core->pm_core.port_loc = phymodPortLocSys);\
    }while(0)

#endif  /*  XGBASET_SUPPORT  * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define BCM_PLP_PHY_GET_NUM_OF_LANES(lane_map, num_of_lanes)                     \
    do {                                                                         \
        num_of_lanes = 0;                                                        \
        while (lane_map) {                                                       \
            lane_map &= (lane_map - 1);                                          \
            num_of_lanes++;                                                      \
        }                                                                        \
    } while(0)

#define BCM_PLP_PHY_GET_PER_LANE_DATARATE(speed, num_of_lanes, lane_data_rate)   \
    do {                                                                         \
        lane_data_rate = 0;                                                      \
        if (num_of_lanes != 0) {                                                 \
            lane_data_rate = (speed / num_of_lanes);                             \
        }                                                                        \
    } while(0)

#define BCM_PLP_PHY_IS_FC_LANE_DATARATE(x)                                       \
    (x == bcmpLplaneDataRate_1P0625G) || (x == bcmpLplaneDataRate_2P125G)    ||  \
    (x == bcmpLplaneDataRate_4P25G)   || (x == bcmpLplaneDataRate_8P5G)      ||  \
    (x == bcmpLplaneDataRate_9P95328G)|| (x == bcmpLplaneDataRate_10P51875G) ||  \
    (x == bcmpLplaneDataRate_14P025G) || (x == bcmpLplaneDataRate_28P05G)    ||  \
    (x == bcmpLplaneDataRate_56P1G)

int _bcm_plp_aperta2_phy_init_bcast(_bcm_if_phymod_phy_t *phy, bcm_plp_firmware_load_type_t *firmware_load_type);

#define BCM_PLP_DATA_PATH_DIRECTION(phy_info, pm_phy)                            \
    BCM_PLP_DATA_PATH_DIRECTION_CLR(pm_phy)                                      \
    if (phy_info.data_path_dir == bcmplpDataPathDirectionEgress) {               \
        pm_phy.access.flags |= 0x2;                                              \
    } else if(phy_info.data_path_dir == bcmplpDataPathDirectionIngress) {        \
        pm_phy.access.flags |= 0x4;                                              \
    } else if(phy_info.data_path_dir == bcmplpDataPathDirectionEgressIngress) {  \
        pm_phy.access.flags |= 0x6;                                              \
    }                                                                            \

#define BCM_PLP_CONVERT_SYNCE_CONFIG(dst_p, src_p)                \
    dst_p->clkGenSquelchCfg      = src_p->clkGenSquelchCfg      ; \
    dst_p->recoveredClkLane      = src_p->recoveredClkLane      ; \
    dst_p->squelchMonitorLanemap = src_p->squelchMonitorLanemap ; \
    dst_p->divider               = src_p->divider               ; \
    dst_p->rclk_out_pin_sel      = src_p->rclk_out_pin_sel      ; \
    dst_p->rclk_if_side          = src_p->rclk_if_side          ; \

#define BCM_PLP_CONVERT_TS_UNIV(dst_p, src_p)                     \
    dst_p->op            = src_p->op            ;                 \
    dst_p->nanosecond    = src_p->nanosecond    ;                 \
    dst_p->subnanosecond = src_p->subnanosecond ;                 \
    dst_p->hi_nanosecond = src_p->hi_nanosecond ;                 \

#define BCM_PLP_CONVERT_TS_PCH(dst_p, src_p)                      \
    dst_p->en        = src_p->en        ;                         \
    dst_p->mode      = src_p->mode      ;                         \
    dst_p->pkt_type  = src_p->pkt_type  ;                         \
    dst_p->ext_field = src_p->ext_field ;                         \
    dst_p->subport   = src_p->subport   ;                         \

#define BCM_PLP_MPLS_LABELS_COUNT    10

#define BCM_PLP_CONVERT_TS_MPLS_CTRL(dst_p, src_p)                    \
    do {                                                              \
        uint8_t index = 0;                                            \
        dst_p->flags         = src_p->flags ;                         \
        dst_p->special_label = src_p->special_label ;                 \
        for(index=0; index<BCM_PLP_MPLS_LABELS_COUNT; index++) {      \
            dst_p->labels[index].value = src_p->labels[index].value ; \
            dst_p->labels[index].mask  = src_p->labels[index].mask  ; \
            dst_p->labels[index].flags = src_p->labels[index].flags ; \
        }                                                             \
        dst_p->size = src_p->size ;                                   \
    } while(0)

#define BCM_PLP_IPV4_ADDR_SIZE  4
#define BCM_PLP_IPV6_ADDR_SIZE  16
#define BCM_PLP_MAC_ADDR_SIZE   6

#define BCM_PLP_CONVERT_TS_INBAND_FILTER(dst_p, src_p)                        \
    do {                                                                      \
       uint8_t idx = 0;                                                       \
       for(idx = 0; idx < BCM_PLP_IPV4_ADDR_SIZE; idx++) {                    \
           dst_p->match_addr.ip_addr[idx]  = src_p->match_addr.ip_addr[idx];  \
       }                                                                      \
       for(idx = 0; idx < BCM_PLP_MAC_ADDR_SIZE; idx++) {                     \
           dst_p->match_addr.mac_addr[idx] = src_p->match_addr.mac_addr[idx]; \
       }                                                                      \
       for(idx = 0; idx < BCM_PLP_IPV6_ADDR_SIZE; idx++) {                    \
           dst_p->match_addr.ip6_addr[idx] = src_p->match_addr.ip6_addr[idx]; \
       }                                                                      \
       dst_p->msg_type = src_p->msg_type ;                                    \
       dst_p->flags    = src_p->flags    ;                                    \
       dst_p->reserved = src_p->reserved ;                                    \
       dst_p->action   = src_p->action   ;                                    \
       dst_p->valid    = src_p->valid    ;                                    \
    } while(0)

#define BCM_PLP_CONVERT_TS_FSYNC(dst_p, src_p)          \
    dst_p->mode             = src_p->mode             ; \
    dst_p->length_threshold = src_p->length_threshold ; \
    dst_p->event_offset     = src_p->event_offset     ; \
    dst_p->flags            = src_p->flags            ; \
    dst_p->gmode            = src_p->gmode            ; \
    dst_p->syncout_mode     = src_p->syncout_mode     ; \

#define BCM_PLP_CONVERT_TS_CONFIG(dst_p, src_p)                                                         \
    do {                                                                                                \
        uint8_t index = 0;                                                                              \
        dst_p->flags                                  = src_p->flags                                  ; \
        dst_p->itpid                                  = src_p->itpid                                  ; \
        dst_p->otpid                                  = src_p->otpid                                  ; \
        dst_p->otpid2                                 = src_p->otpid2                                 ; \
        dst_p->timer_adjust.mode                      = src_p->timer_adjust.mode                      ; \
        dst_p->timer_adjust.delta                     = src_p->timer_adjust.delta                     ; \
        dst_p->inband_ctrl.flags                      = src_p->inband_ctrl.flags                      ; \
        dst_p->inband_ctrl.resv0_id                   = src_p->inband_ctrl.resv0_id                   ; \
        dst_p->inband_ctrl.timer_mode                 = src_p->inband_ctrl.timer_mode                 ; \
        dst_p->gmode                                  = src_p->gmode                                  ; \
        dst_p->syncout.mode                           = src_p->syncout.mode                           ; \
        dst_p->syncout.pulse_1_length                 = src_p->syncout.pulse_1_length                 ; \
        dst_p->syncout.pulse_2_length                 = src_p->syncout.pulse_2_length                 ; \
        dst_p->syncout.interval                       = src_p->syncout.interval                       ; \
        dst_p->syncout.timestamp                      = src_p->syncout.timestamp                      ; \
        dst_p->ts_divider                             = src_p->ts_divider                             ; \
        dst_p->original_timecode.isnegative           = src_p->original_timecode.isnegative           ; \
        dst_p->original_timecode.seconds              = src_p->original_timecode.seconds              ; \
        dst_p->original_timecode.nanoseconds          = src_p->original_timecode.nanoseconds          ; \
        dst_p->original_timecode.nanoseconds64        = src_p->original_timecode.nanoseconds64        ; \
        dst_p->original_timecode.syncref_delay.enable = src_p->original_timecode.syncref_delay.enable ; \
        dst_p->original_timecode.syncref_delay.value  = src_p->original_timecode.syncref_delay.value  ; \
        dst_p->rx_link_delay                          = src_p->rx_link_delay                          ; \
        dst_p->tx_sync_mode                           = src_p->tx_sync_mode                           ; \
        dst_p->tx_delay_req_mode                      = src_p->tx_delay_req_mode                      ; \
        dst_p->tx_pdelay_req_mode                     = src_p->tx_pdelay_req_mode                     ; \
        dst_p->tx_pdelay_resp_mode                    = src_p->tx_pdelay_resp_mode                    ; \
        dst_p->rx_sync_mode                           = src_p->rx_sync_mode                           ; \
        dst_p->rx_delay_req_mode                      = src_p->rx_delay_req_mode                      ; \
        dst_p->rx_pdelay_req_mode                     = src_p->rx_pdelay_req_mode                     ; \
        dst_p->rx_pdelay_resp_mode                    = src_p->rx_pdelay_resp_mode                    ; \
        dst_p->mpls_ctrl.flags                        = src_p->mpls_ctrl.flags                        ; \
        dst_p->mpls_ctrl.special_label                = src_p->mpls_ctrl.special_label                ; \
        dst_p->mpls_ctrl.size                         = src_p->mpls_ctrl.size                         ; \
        for(index=0; index<BCM_PLP_MPLS_LABELS_COUNT; index++) {                                        \
            dst_p->mpls_ctrl.labels[index].value      = src_p->mpls_ctrl.labels[index].value          ; \
            dst_p->mpls_ctrl.labels[index].mask       = src_p->mpls_ctrl.labels[index].mask           ; \
            dst_p->mpls_ctrl.labels[index].flags      = src_p->mpls_ctrl.labels[index].flags          ; \
        }                                                                                               \
        dst_p->sync_freq                              = src_p->sync_freq                              ; \
        dst_p->phy_1588_dpll_k1                       = src_p->phy_1588_dpll_k1                       ; \
        dst_p->phy_1588_dpll_k2                       = src_p->phy_1588_dpll_k2                       ; \
        dst_p->phy_1588_dpll_k3                       = src_p->phy_1588_dpll_k3                       ; \
        dst_p->phy_1588_dpll_loop_filter              = src_p->phy_1588_dpll_loop_filter              ; \
        dst_p->phy_1588_dpll_ref_phase                = src_p->phy_1588_dpll_ref_phase                ; \
        dst_p->phy_1588_dpll_ref_phase_delta          = src_p->phy_1588_dpll_ref_phase_delta          ; \
        dst_p->tx_inband_spare                        = src_p->tx_inband_spare                        ; \
        dst_p->tx_inband_clear                        = src_p->tx_inband_clear                        ; \
        dst_p->tx_inband32_format                     = src_p->tx_inband32_format                     ; \
        dst_p->tx_inband_strict                       = src_p->tx_inband_strict                       ; \
        dst_p->tx_mdio_sop_mem                        = src_p->tx_mdio_sop_mem                        ; \
        dst_p->tx_partial_tc                          = src_p->tx_partial_tc                          ; \
        dst_p->tx_mode_tc                             = src_p->tx_mode_tc                             ; \
        dst_p->rx_inband_spare                        = src_p->rx_inband_spare                        ; \
        dst_p->rx_inband_insert_4byt                  = src_p->rx_inband_insert_4byt                  ; \
        dst_p->rx_inband32_format                     = src_p->rx_inband32_format                     ; \
        dst_p->rx_inband_strict                       = src_p->rx_inband_strict                       ; \
        dst_p->rx_mdio_sop_mem                        = src_p->rx_mdio_sop_mem                        ; \
        dst_p->rx_partial_tc                          = src_p->rx_partial_tc                          ; \
        dst_p->rx_mode_tc                             = src_p->rx_mode_tc                             ; \
        dst_p->rx_mpls_ctrl.flags                     = src_p->rx_mpls_ctrl.flags                     ; \
        dst_p->rx_mpls_ctrl.special_label             = src_p->rx_mpls_ctrl.special_label             ; \
        dst_p->rx_mpls_ctrl.size                      = src_p->rx_mpls_ctrl.size                      ; \
        for(index=0; index<BCM_PLP_MPLS_LABELS_COUNT; index++) {                                        \
            dst_p->rx_mpls_ctrl.labels[index].value   = src_p->rx_mpls_ctrl.labels[index].value       ; \
            dst_p->rx_mpls_ctrl.labels[index].mask    = src_p->rx_mpls_ctrl.labels[index].mask        ; \
            dst_p->rx_mpls_ctrl.labels[index].flags   = src_p->rx_mpls_ctrl.labels[index].flags       ; \
        }                                                                                               \
        dst_p->tx_option.force_ts_to_sopmem           = src_p->tx_option.force_ts_to_sopmem           ; \
        dst_p->tx_option.ptpv2_chk_dis                = src_p->tx_option.ptpv2_chk_dis                ; \
        dst_p->tx_option.timecode_to_insertion        = src_p->tx_option.timecode_to_insertion        ; \
        dst_p->tx_option.keep_ori_crc                 = src_p->tx_option.keep_ori_crc                 ; \
        dst_p->rx_option.force_ts_to_sopmem           = src_p->rx_option.force_ts_to_sopmem           ; \
        dst_p->rx_option.ptpv2_chk_dis                = src_p->rx_option.ptpv2_chk_dis                ; \
        dst_p->rx_option.timecode_to_insertion        = src_p->rx_option.timecode_to_insertion        ; \
        dst_p->rx_option.keep_ori_crc                 = src_p->rx_option.keep_ori_crc                 ; \
        dst_p->tx_inband_prop.spare                   = src_p->tx_inband_prop.spare                   ; \
        dst_p->tx_inband_prop.clear_rsv012            = src_p->tx_inband_prop.clear_rsv012            ; \
        dst_p->tx_inband_prop.ins_4bytes              = src_p->tx_inband_prop.ins_4bytes              ; \
        dst_p->tx_inband_prop.ts32_format             = src_p->tx_inband_prop.ts32_format             ; \
        dst_p->tx_inband_prop.strict                  = src_p->tx_inband_prop.strict                  ; \
        dst_p->tx_inband_prop.mdio_sopmem             = src_p->tx_inband_prop.mdio_sopmem             ; \
        dst_p->tx_inband_prop.ts_80bits               = src_p->tx_inband_prop.ts_80bits               ; \
        dst_p->tx_inband_prop.partial_tc_mode         = src_p->tx_inband_prop.partial_tc_mode         ; \
        dst_p->tx_inband_prop.tc_mode                 = src_p->tx_inband_prop.tc_mode                 ; \
        dst_p->tx_inband_prop.update_resv0            = src_p->tx_inband_prop.update_resv0            ; \
        dst_p->tx_inband_prop.check_resv0             = src_p->tx_inband_prop.check_resv0             ; \
        dst_p->tx_inband_prop.inband_on               = src_p->tx_inband_prop.inband_on               ; \
        dst_p->tx_inband_prop.resv0_id                = src_p->tx_inband_prop.resv0_id                ; \
        dst_p->tx_inband_prop.write_sopmem            = src_p->tx_inband_prop.write_sopmem            ; \
        dst_p->tx_inband_prop.compare_sopmem          = src_p->tx_inband_prop.compare_sopmem          ; \
        dst_p->tx_inband_prop.cmp_domain_num          = src_p->tx_inband_prop.cmp_domain_num          ; \
        dst_p->tx_inband_prop.cmp_seq_num             = src_p->tx_inband_prop.cmp_seq_num             ; \
        dst_p->tx_inband_prop.cmp_src_port            = src_p->tx_inband_prop.cmp_src_port            ; \
        dst_p->tx_inband_prop.cmp_field_sel           = src_p->tx_inband_prop.cmp_field_sel           ; \
        dst_p->tx_inband_prop.cmp_vlan_id             = src_p->tx_inband_prop.cmp_vlan_id             ; \
        dst_p->rx_inband_prop.spare                   = src_p->rx_inband_prop.spare                   ; \
        dst_p->rx_inband_prop.clear_rsv012            = src_p->rx_inband_prop.clear_rsv012            ; \
        dst_p->rx_inband_prop.ins_4bytes              = src_p->rx_inband_prop.ins_4bytes              ; \
        dst_p->rx_inband_prop.ts32_format             = src_p->rx_inband_prop.ts32_format             ; \
        dst_p->rx_inband_prop.strict                  = src_p->rx_inband_prop.strict                  ; \
        dst_p->rx_inband_prop.mdio_sopmem             = src_p->rx_inband_prop.mdio_sopmem             ; \
        dst_p->rx_inband_prop.ts_80bits               = src_p->rx_inband_prop.ts_80bits               ; \
        dst_p->rx_inband_prop.partial_tc_mode         = src_p->rx_inband_prop.partial_tc_mode         ; \
        dst_p->rx_inband_prop.tc_mode                 = src_p->rx_inband_prop.tc_mode                 ; \
        dst_p->rx_inband_prop.update_resv0            = src_p->rx_inband_prop.update_resv0            ; \
        dst_p->rx_inband_prop.check_resv0             = src_p->rx_inband_prop.check_resv0             ; \
        dst_p->rx_inband_prop.inband_on               = src_p->rx_inband_prop.inband_on               ; \
        dst_p->rx_inband_prop.resv0_id                = src_p->rx_inband_prop.resv0_id                ; \
        dst_p->rx_inband_prop.write_sopmem            = src_p->rx_inband_prop.write_sopmem            ; \
        dst_p->rx_inband_prop.compare_sopmem          = src_p->rx_inband_prop.compare_sopmem          ; \
        dst_p->rx_inband_prop.cmp_domain_num          = src_p->rx_inband_prop.cmp_domain_num          ; \
        dst_p->rx_inband_prop.cmp_seq_num             = src_p->rx_inband_prop.cmp_seq_num             ; \
        dst_p->rx_inband_prop.cmp_src_port            = src_p->rx_inband_prop.cmp_src_port            ; \
        dst_p->rx_inband_prop.cmp_field_sel           = src_p->rx_inband_prop.cmp_field_sel           ; \
        dst_p->rx_inband_prop.cmp_vlan_id             = src_p->rx_inband_prop.cmp_vlan_id             ; \
        dst_p->tx_pkt_count_sel                       = src_p->tx_pkt_count_sel                       ; \
        dst_p->rx_pkt_count_sel                       = src_p->rx_pkt_count_sel                       ; \
        dst_p->fifo_level_intr_thold                  = src_p->fifo_level_intr_thold                  ; \
        dst_p->hb_capture_mode                        = src_p->hb_capture_mode                        ; \
        dst_p->nco_sync0_pulse                        = src_p->nco_sync0_pulse                        ; \
        dst_p->sop_ts_cap.dp_ts_wclk                  = src_p->sop_ts_cap.dp_ts_wclk                  ; \
        dst_p->sop_ts_cap.err_ecc2pkt                 = src_p->sop_ts_cap.err_ecc2pkt                 ; \
        dst_p->sop_ts_cap.stamping                    = src_p->sop_ts_cap.stamping                    ; \
        dst_p->sop_ts_cap.lov                         = src_p->sop_ts_cap.lov                         ; \
        dst_p->sop_ts_cap.ts_cap                      = src_p->sop_ts_cap.ts_cap                      ; \
        dst_p->otpid1                                 = src_p->otpid1                                 ; \
        dst_p->itpid_igr                              = src_p->itpid_igr                              ; \
        dst_p->otpid_igr                              = src_p->otpid_igr                              ; \
        dst_p->otpid1_igr                             = src_p->otpid1_igr                             ; \
        dst_p->otpid2_igr                             = src_p->otpid2_igr                             ; \
        dst_p->ip4udp_chksum_disable                  = src_p->ip4udp_chksum_disable                  ; \
        dst_p->vxlan_outer_chksum_disable             = src_p->vxlan_outer_chksum_disable             ; \
        dst_p->sopmem_intr_enable                     = src_p->sopmem_intr_enable                     ; \
        dst_p->sopmem_wr_on_err                       = src_p->sopmem_wr_on_err                       ; \
    } while(0)

#define BCM_PLP_DATA_PATH_DIRECTION_CLR(pm_phy)     pm_phy.access.flags &= 0xFFFFFFF9;

#endif
