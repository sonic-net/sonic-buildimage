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
#define BCM_PM_IF_MAJOR_VER_NO           3
#define BCM_PM_IF_MINOR_VER_NO           6
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
        if ((plp_barchetta_phy_ctrl.phy[_PHY_ID] != NULL) && (plp_barchetta_phy_ctrl.phy[_PHY_ID]->mutex_info.mutex_take != NULL)) {\
           PHYMOD_IF_ERR_RETURN(plp_barchetta_phy_ctrl.phy[_PHY_ID]->mutex_info.mutex_take(_PHY_ID, _P_CTXT));\
        }\
    } while (0)

#define BCM_PM_MUTEX_UNLOCK(_PHY_ID, _P_CTXT) \
    do {\
        BCM_PM_ERR_CHECK_PHYID_RET_INVALID(_PHY_ID);    \
        if ((plp_barchetta_phy_ctrl.phy[_PHY_ID] != NULL) && (plp_barchetta_phy_ctrl.phy[_PHY_ID]->mutex_info.mutex_give != NULL)) {\
           PHYMOD_IF_ERR_RETURN(plp_barchetta_phy_ctrl.phy[_PHY_ID]->mutex_info.mutex_give(_PHY_ID, _P_CTXT));\
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
        if ((_plp_barchetta_phyid_list[_PHY_ID].valid) && (_plp_barchetta_phyid_list[_PHY_ID].phy_id == _PHY_ID)) {\
            PHYMOD_DEBUG_ERROR(("This API needs to be called before bcm_pm_if_init\n"));\
            return BCM_PM_IF_PHY_EXISTING;\
        } \
    } while (0)


typedef struct _bcm_if_phymod_core_s {
    plp_barchetta_phymod_core_access_t pm_core;
    plp_barchetta_phymod_bus_t pm_bus;
    int ref_cnt;
    int init;
    int unit;
    int port;
    int (*read)(void *, uint32_t, uint32_t, uint32_t*);
    int (*write)(void *, uint32_t, uint32_t, uint32_t);
    int (*wrmask)(void *, uint32_t, uint32_t, uint32_t, uint32_t);
    plp_barchetta_phymod_core_init_config_t init_config;
} _bcm_if_phymod_core_t;

typedef struct bcm_if_phymod_phy_s {
    plp_barchetta_phymod_phy_access_t pm_phy;
    _bcm_if_phymod_core_t *core;
    plp_barchetta_phymod_phy_init_config_t  init_config;
    void* static_config;
    bcm_plp_mutex_info_t mutex_info;
    uint32_t valid;
} _bcm_if_phymod_phy_t;

typedef struct bcm_if_phymod_ctrl_s {
    int unit;
    int num_phys;
    _bcm_if_phymod_phy_t *phy[PHYMOD_IF_CONFIG_MAX_PHYS];
    plp_barchetta_phymod_symbols_t *symbols;
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

extern bcm_if_phymod_ctrl_t plp_barchetta_phy_ctrl;
extern bcm_if_phymod_phy_id_t _plp_barchetta_phyid_list[BCM_PM_IF_MAX_PHY];


#if defined(PHYMOD_XGBASET_SUPPORT)
void _bcm_pm_if_init_phy_id_idx(uint32_t phy_id);
void _bcm_plp_barchetta_pm_if_get_phy_id_idx(uint32_t phy_id_combo, uint32_t *idx, uint32_t *exist_phy);
int  _bcm_pm_if_phy_id_is_master(uint32_t phy_id);
#else  /* ! XGBASET_SUPPORT  */
void _bcm_plp_barchetta_pm_if_get_phy_id_idx(uint32_t phy_id, uint32_t *idx, uint32_t *exist_phy);
#endif  /*  XGBASET_SUPPORT  */

void _bcm_plp_barchetta_pm_if_core_init(_bcm_if_phymod_core_t *core, plp_barchetta_phymod_bus_t *core_bus, uint32_t phy_addr, void *user_acc);
int _bcm_plp_barchetta_if_phymod_phy_create(_bcm_if_phymod_phy_t **phy);
int _bcm_plp_barchetta_if_phymod_core_create(_bcm_if_phymod_core_t **core);
int _bcm_plp_barchetta_pm_phy_init(_bcm_if_phymod_phy_t *phy, bcm_pm_firmware_load_method_t firmware_load_method, void *init_param);
int _bcm_plp_barchetta_phy_user_phymod_ability(unsigned int tech_ability, unsigned int fec_ability, unsigned short pause_ability, bcm_plp_an_config_t an_config, plp_barchetta_phymod_autoneg_ability_t* ability);
int _bcm_plp_barchetta_phy_phymod_user_ability(plp_barchetta_phymod_autoneg_ability_t ability, unsigned short *fec_ability, unsigned short *pause_ability, bcm_plp_an_config_t* an_config);

int bcm_plp_barchetta_phy_get_phy_type (bcm_plp_access_t phy_info, plp_barchetta_phymod_dispatch_type_t *phy_type,
                         int (*read)(void* user_acc, unsigned int core_addr, unsigned int reg_addr, unsigned int* val));
#ifdef PHYMOD_GALLARDO28_SUPPORT
int _gallardo28_get_single_pmd_mode(bcm_plp_access_t phy_info,
                                            int (*read)(void* user_acc, unsigned int core_addr, unsigned int reg_addr, unsigned int* val),  unsigned int *pmd_mode);
#endif

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
        _bcm_plp_barchetta_pm_if_get_phy_id_idx(_PI.phy_addr, &IDX, &E_P);               \
        if (E_P != 1) {                                                    \
            _plp_barchetta_phyid_list[laddr].valid = 0;                                  \
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
            _PI.platform_ctxt = plp_barchetta_phy_ctrl.phy[IDX]->pm_phy.access.user_acc; \
        }                                                                  \
    } while (0)

#define BCM_PLP_FILL_PHY_ACCESS(_PI, IDX)                                               \
    do {                                                                                \
        int is_line_side = (_PI.if_side == 0 || _PI.if_side == 0xFF) ? TRUE : FALSE;    \
        IDX &= BCM_PHY_ADDR_LOGICAL_MASK;                                               \
        if ( _PI.platform_ctxt != NULL ) {                                              \
            plp_barchetta_phy_ctrl.phy[IDX]->pm_phy.access.user_acc        = _PI.platform_ctxt;       \
            plp_barchetta_phy_ctrl.phy[IDX]->core->pm_core.access.user_acc = _PI.platform_ctxt;       \
        }                                                                               \
        plp_barchetta_phy_ctrl.phy[IDX]->pm_phy.access.lane_mask = _PI.lane_map;                      \
        plp_barchetta_phy_ctrl.phy[IDX]->pm_phy.access.flags &= ~(1U << BCM_PM_INTERFACE_SIDE_SHIFT); \
        plp_barchetta_phy_ctrl.phy[IDX]->pm_phy.access.flags |=                                       \
                                ( (is_line_side ? BCM_LINE_SIDE : BCM_SYSTEM_SIDE)      \
                                  << BCM_PM_INTERFACE_SIDE_SHIFT );                     \
        plp_barchetta_phy_ctrl.phy[IDX]->pm_phy.port_loc = (is_line_side) ? phymodPortLocLine         \
                                                            : phymodPortLocSys;         \
        plp_barchetta_phy_ctrl.phy[IDX]->pm_phy.access.misc = &(_plp_barchetta_phyid_list[IDX]);                    \
    } while (0)

#define BCM_PLP_CORE_GET_P_CTXT(_PI, IDX, E_P)                                    \
    do {                                                                          \
        _bcm_plp_barchetta_pm_if_get_phy_id_idx(_PI.phy_addr, &IDX, &E_P);                      \
        if (E_P != 1) {                                                           \
            _plp_barchetta_phyid_list[BCM_PHY_ADDR_LOGICAL(_PI.phy_addr)].valid = 0;            \
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
            _PI.platform_ctxt = plp_barchetta_phy_ctrl.phy[IDX]->core->pm_core.access.user_acc; \
        }                                                                         \
    } while (0)

#define BCM_PLP_FILL_CORE_ACCESS(_PI, IDX)                                        \
    do {                                                                          \
        IDX &= BCM_PHY_ADDR_LOGICAL_MASK;                                         \
        if (_PI.platform_ctxt != NULL) {                                          \
            plp_barchetta_phy_ctrl.phy[IDX]->pm_phy.access.user_acc = _PI.platform_ctxt;        \
            plp_barchetta_phy_ctrl.phy[IDX]->core->pm_core.access.user_acc = _PI.platform_ctxt; \
        }                                                                         \
        plp_barchetta_phy_ctrl.phy[IDX]->core->pm_core.access.lane_mask = _PI.lane_map;         \
        BCM_SET_CORE_INTF_SIDE(plp_barchetta_phy_ctrl,_PI.if_side);                             \
        plp_barchetta_phy_ctrl.phy[IDX]->core->pm_core.port_loc =                               \
                     (_PI.if_side == 0) ? phymodPortLocLine : phymodPortLocSys;   \
        plp_barchetta_phy_ctrl.phy[IDX]->core->pm_core.access.misc = &(_plp_barchetta_phyid_list[IDX]);       \
    } while (0)

#else  /* ! XGBASET_SUPPORT  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */

#define BCM_PLP_PHY_GET_P_CTXT(_PI, IDX, E_P)                              \
    do{                                                                    \
        if (_PI.phy_addr >= BCM_PM_IF_MAX_PHY) {                           \
            PHYMOD_DEBUG_ERROR(("Invalid PHY . . .\n"));                   \
            rv = BCM_PM_IF_INVALID_PHY;                                    \
            goto ERR;                                                      \
        }                                                                  \
        _bcm_plp_barchetta_pm_if_get_phy_id_idx(_PI.phy_addr, &IDX, &E_P);               \
        if (E_P != 1) {                                                    \
            _plp_barchetta_phyid_list[_PI.phy_addr].valid = 0;                           \
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
            _PI.platform_ctxt = plp_barchetta_phy_ctrl.phy[IDX]->pm_phy.access.user_acc; \
        }                                                                  \
    }while(0)

#define BCM_PLP_FILL_PHY_ACCESS(_PI, IDX)                             \
    do{                                                               \
        if (_PI.platform_ctxt != NULL) {                              \
            plp_barchetta_phy_ctrl.phy[IDX]->pm_phy.access.user_acc = _PI.platform_ctxt;\
            plp_barchetta_phy_ctrl.phy[IDX]->core->pm_core.access.user_acc = _PI.platform_ctxt;\
        }                                                              \
        plp_barchetta_phy_ctrl.phy[IDX]->pm_phy.access.lane_mask = _PI.lane_map;     \
        plp_barchetta_phy_ctrl.phy[IDX]->pm_phy.access.flags &= ~(1 << BCM_PM_INTERFACE_SIDE_SHIFT);\
        plp_barchetta_phy_ctrl.phy[IDX]->pm_phy.access.flags |= (((_PI.if_side == 0 || _PI.if_side == 0xFF ) ? BCM_LINE_SIDE : BCM_SYSTEM_SIDE) << BCM_PM_INTERFACE_SIDE_SHIFT); \
        (_PI.if_side == 0 ||_PI.if_side == 0xFF ) ? (plp_barchetta_phy_ctrl.phy[IDX]->pm_phy.port_loc = phymodPortLocLine) : (plp_barchetta_phy_ctrl.phy[IDX]->pm_phy.port_loc = phymodPortLocSys);\
    }while(0)

#define BCM_PLP_CORE_GET_P_CTXT(_PI, IDX, E_P)                                    \
    do{                                                                           \
        _bcm_plp_barchetta_pm_if_get_phy_id_idx(_PI.phy_addr, &IDX, &E_P);                      \
        if (E_P != 1) {                                                           \
            _plp_barchetta_phyid_list[_PI.phy_addr].valid = 0;                                  \
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
            _PI.platform_ctxt = plp_barchetta_phy_ctrl.phy[IDX]->core->pm_core.access.user_acc; \
        }                                                                         \
    }while(0)

#define BCM_PLP_FILL_CORE_ACCESS(_PI, IDX)                            \
    do{                                                               \
        if (_PI.platform_ctxt != NULL) {                              \
            plp_barchetta_phy_ctrl.phy[IDX]->pm_phy.access.user_acc = _PI.platform_ctxt;\
            plp_barchetta_phy_ctrl.phy[IDX]->core->pm_core.access.user_acc = _PI.platform_ctxt;\
        }                                                              \
        plp_barchetta_phy_ctrl.phy[IDX]->core->pm_core.access.lane_mask = _PI.lane_map;     \
        BCM_SET_CORE_INTF_SIDE(plp_barchetta_phy_ctrl,_PI.if_side);                            \
        (_PI.if_side == 0) ? (plp_barchetta_phy_ctrl.phy[IDX]->core->pm_core.port_loc = phymodPortLocLine) : (plp_barchetta_phy_ctrl.phy[IDX]->core->pm_core.port_loc = phymodPortLocSys);\
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

int _bcm_plp_barchetta_phy_init_bcast(_bcm_if_phymod_phy_t *phy, bcm_plp_firmware_load_type_t *firmware_load_type);

#define BCM_PLP_DATA_PATH_DIRECTION(phy_info, pm_phy)                            \
    BCM_PLP_DATA_PATH_DIRECTION_CLR(pm_phy)                                      \
    if (phy_info.data_path_dir == bcmplpDataPathDirectionEgress) {               \
        pm_phy.access.flags |= 0x2;                                              \
    } else if(phy_info.data_path_dir == bcmplpDataPathDirectionIngress) {        \
        pm_phy.access.flags |= 0x4;                                              \
    } else if(phy_info.data_path_dir == bcmplpDataPathDirectionEgressIngress) {  \
        pm_phy.access.flags |= 0x6;                                              \
    }                                                                            \

#define BCM_PLP_DATA_PATH_DIRECTION_CLR(pm_phy)     pm_phy.access.flags &= 0xFFFFFFF9;

#endif
