/*
 *         
 * $Id:$
 * 
 * $Copyright: (c) 2013 Broadcom Corp. All Rights Reserved.$
 *         
 *
 */

#ifndef _PORTMOD_INTERNAL_H_
#define _PORTMOD_INTERNAL_H_

#include <include/portmod.h>
#include <include/pm8x100_gen2_shared.h>

#define MAX_PMS_PER_PHY (3)
#define MAX_VARS_PER_BUFFER (64)
#define VERSION(_ver) (_ver)

#define PORTMOD_PORT_IS_DEFAULT_TX_PARAMS_UPDATED(port_dynamic_state)     (port_dynamic_state & 0x1)
#define PORTMOD_PORT_DEFAULT_TX_PARAMS_UPDATED_SET(port_dynamic_state)    (port_dynamic_state |= 0x1)
#define PORTMOD_PORT_DEFAULT_TX_PARAMS_UPDATED_CLR(port_dynamic_state)    (port_dynamic_state &= 0xfffe)

#define PORTMOD_PORT_IS_AUTONEG_MODE_UPDATED(port_dynamic_state)          (port_dynamic_state & 0x2)
#define PORTMOD_PORT_AUTONEG_MODE_UPDATED_SET(port_dynamic_state)         (port_dynamic_state |= 0x2)
#define PORTMOD_PORT_AUTONEG_MODE_UPDATED_CLR(port_dynamic_state)         (port_dynamic_state &= 0xfffd) 

#define PORTMOD_PORT_IS_DEFAULT_EXT_PHY_TX_PARAMS_UPDATED(port_dynamic_state)     (port_dynamic_state & 0x4)
#define PORTMOD_PORT_DEFAULT_EXT_PHY_TX_PARAMS_UPDATED_SET(port_dynamic_state)    (port_dynamic_state |= 0x4)
#define PORTMOD_PORT_DEFAULT_EXT_PHY_TX_PARAMS_UPDATED_CLR(port_dynamic_state)    (port_dynamic_state &= 0xfffb)

#define PORTMOD_PORT_IS_DEFAULT_TIMESTAMP_ADJUSTMENT_UPDATED(port_dynamic_state)     (port_dynamic_state & 0x8)
#define PORTMOD_PORT_DEFAULT_TIMESTAMP_ADJUSTMENT_UPDATED_SET(port_dynamic_state)    (port_dynamic_state |= 0x8)
#define PORTMOD_PORT_DEFAULT_TIMESTAMP_ADJUSTMENT_UPDATED_CLR(port_dynamic_state)    (port_dynamic_state &= 0xfff7)

#define PORTMOD_PORT_IS_AN_LINK_FAIL_INHIBIT_TIMER_UPDATED(port_dynamic_state)     (port_dynamic_state & 0x10)
#define PORTMOD_PORT_AN_LINK_FAIL_INHIBIT_TIMER_UPDATED_SET(port_dynamic_state)    (port_dynamic_state |= 0x10)
#define PORTMOD_PORT_AN_LINK_FAIL_INHIBIT_TIMER_UPDATED_CLR(port_dynamic_state)    (port_dynamic_state &= 0xffef)

/* following defines is for FEC init value */
#define PORTMOD_PORT_FEC_OFF         0x0
#define PORTMOD_PORT_FEC_CL74        0x1
#define PORTMOD_PORT_FEC_CL91        0x2
#define PORTMOD_PORT_FEC_CL108        0x3

/* RS FEC frame size in bits */
#define PORTMOD_PORT_FEC_FRAME_SIZE_INVALID 0
#define PORTMOD_PORT_FEC_FRAME_SIZE_RS_544 5440
#define PORTMOD_PORT_FEC_FRAME_SIZE_RS_528 10560

/*
 * Macros is to indicate RX Fragment types
 * used by plp_aperta2_portmod_preemption_control_set/get() Api's
 * to configure the RX fragment size
 */
#define PORTMOD_PREEMPTION_NON_FINAL_FRAG  0x0
#define PORTMOD_PREEMPTION_FINAL_FRAG      0x1

/* 
 * Macros for IEEE, HIGIG, HIGIG2 mode, matching BCM marcos 
 *  such as BCM_PORT_ENCAP_IEEE
 */
#define PORTMOD_ENCAP_IEEE_MODE      0
#define PORTMOD_ENCAP_HIGIG_MODE     1
#define PORTMOD_ENCAP_HIGIG2_MODE    3

/*
 * Define Portmacro Version
 */
typedef enum pm_type_e {
    PM_TYPE_INVALID,
    PM_TYPE_PM4X10,
    PM_TYPE_PM4X25,
    PM_TYPE_PM12X10,
    PM_TYPE_PM4X10Q,
    PM_TYPE_FCPM,
    PM_TYPE_CPM4X25,
    PM_TYPE_PM8X50,
    PM_TYPE_PM8X50_FLEXE,
    PM_TYPE_COUNT
}pm_type_t;

typedef enum pm_gen_e {
    PM_GEN_INVALID,
    PM_GEN1,
    PM_GEN2,
    PM_GEN3,
    PM_GEN4,
    PM_GEN_COUNT
}pm_gen_t;

typedef enum pm_tech_process_e {
    PM_TECH_INVALID,
    PM_TECH_28NM,    /* Technology process.  28nm */
    PM_TECH_16NM,    /* Technology process.  16nm */
    PM_TECH_7NM,     /* Technology process.   7nm */
    PM_TECH_5NM,     /* Technology process.   5nm */
    PM_TECH_COUNT
}pm_tech_process_t;

typedef enum pm_rev_num_e {
    PM_REVISION_NUM_INVALID,
    PM_REVISION_0,
    PM_REVISION_1,
    PM_REVISION_2,
    PM_REVISION_3,
    PM_REVISION_4,
    PM_REVISION_5,
    PM_REVISION_6,
    PM_REVISION_7,
    PM_REVISION_NUM_COUNT
}pm_rev_num_t;

typedef enum pm_rev_letter_e {
    PM_REVISION_LETTER_INVALID,
    PM_REVISION_A,
    PM_REVISION_B,
    PM_REVISION_C,
    PM_REVISION_D,
    PM_REVISION_LETTER_COUNT
}pm_rev_letter_t;

typedef struct pm_version_s{
    pm_type_t          type;
    pm_gen_t           gen;
    pm_tech_process_t  tech_process;
    pm_rev_num_t       revision_number;
    pm_rev_letter_t    revision_letter;
}pm_version_t;

typedef struct pm4x10_s *pm4x10_t;
typedef struct pm4x10q_s *pm4x10q_t;
typedef struct pm4x25_s *pm4x25_t;
typedef struct pm12x10_s *pm12x10_t;
typedef struct dnx_fabric_s *dnx_fabric_t;
typedef struct pm8x50_fabric_s *pm8x50_fabric_t;
typedef struct pm8x100_fabric_s *pm8x100_fabric_t;
typedef struct pmOsIlkn_s *pmOsIlkn_t;
typedef struct pm_qtc_s *pm_qtc_t;
typedef struct pm4x2p5_s *pm4x2p5_t;
typedef struct pm8x50_s *pm8x50_t;
typedef struct pm8x50_flexe_gen2_s *pm8x50_flexe_gen2_t;
typedef struct pm4x10_qtc_s *pm4x10_qtc_t;
typedef struct aperta2_pm8x100_gen2_s *pm8x100_gen2_t;

typedef union pm_db_u{
    pm4x10_t      pm4x10_db;
    pm4x10q_t      pm4x10q_db;
    dnx_fabric_t  dnx_fabric;
    pm8x50_fabric_t pm8x50_fabric;
    pm8x100_fabric_t pm8x100_fabric;
    pm4x25_t      pm4x25_db;
    pm12x10_t     pm12x10_db;
    pmOsIlkn_t    pmOsIlkn_db;
    pm_qtc_t      pm_qtc_db;
    pm4x2p5_t     pm4x2p5_db;
    pm8x50_t        pm8x50_db;
    pm8x50_flexe_gen2_t pm8x50_flexe_gen2_db;
    pm4x10_qtc_t    pm4x10_qtc_db;
    pm8x100_gen2_t aperta2_pm8x100_gen2_db;
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
    plp_aperta2_phymod_ref_clk_t ref_clk; /**< SerDes quad ref clock */
    plp_aperta2_phymod_ref_clk_t com_clk; /**< SerDes commmon ref clock */
    plp_aperta2_phymod_access_t access; /**< phymod access structure; defines the register access for the SerDes Core */
    plp_aperta2_phymod_lane_map_t lane_map;
    plp_aperta2_phymod_firmware_load_method_t fw_load_method;
    phymod_firmware_loader_f external_fw_loader; /**< firmware loader that will be used in case that fw_load_method=phymodFirmwareLoadMethodExternal */
    plp_aperta2_phymod_polarity_t polarity; /**< Lanes Polarity */
    int fmac_schan_id; /**< FMAC schan id */
    int fsrd_schan_id; /**< FSRD schan id */
    int fsrd_internal_quad; /**< Core instance in FSRD */
    int first_phy_offset;
    int core_index;
    int is_over_nif;
    pm_info_t *pms; /**< PM used for fabric over nif */
}portmod_dnx_fabric_create_info_internal_t;

/* PM8x50_fabric internal create structure - identical to create info */
typedef portmod_pm8x50_fabric_create_info_t portmod_pm8x50_fabric_create_info_internal_t;

/* PM8x100_fabric internal create structure - identical to create info */
typedef portmod_pm8x100_fabric_create_info_t portmod_pm8x100_fabric_create_info_internal_t;

/* PM4x10Q internal create structure*/
typedef struct portmod_pm4x10q_create_info_internal_s{
    pm_info_t pm4x10;
    uint32 blk_id;
    void* qsgmii_user_acc;
    void* pm4x10_user_acc;
    uint32 core_clock_khz;
}portmod_pm4x10q_create_info_internal_t;

/* PM12x10 internal create structure*/
typedef struct portmod_pm12x10_create_info_internal_s{
    pm_info_t pm4x25;
    pm_info_t pm4x10[3];
    uint32 flags;
    int blk_id;
    int refclk_source;
}portmod_pm12x10_create_info_internal_t;

/* PM4x2p5: no translation is required */
typedef portmod_pm4x2p5_create_info_t portmod_pm4x2p5_create_info_internal_t;

/* OS ILKN internal create structure*/
typedef struct portmod_os_ilkn_create_info_internal_s{
    int nof_aggregated_pms;
    pm_info_t *pms;
    int *pm_ids;
    int wm_high[PORTMOD_MAX_ILKN_PORTS_PER_ILKN_PM]; /**< watermark high value */
    int wm_low[PORTMOD_MAX_ILKN_PORTS_PER_ILKN_PM]; /**< watermark low value */
    uint8 is_over_fabric;
    uint32 core_clock_khz;
    portmod_ilkn_port_lane_map_callback_get_f ilkn_port_lane_map_get;
    portmod_ilkn_pm_lane_map_callback_get_f ilkn_pm_lane_map_get;
    portmod_ilkn_port_fec_units_set_callback_f ilkn_port_fec_units_set;
    uint32 ilkn_block_index;
    uint32 fec_disable_by_bypass;
    uint32 fec_dependency;
}portmod_os_ilkn_create_info_internal_t;

/* PM8x50: no translation is required??? */
typedef portmod_pm8x50_create_info_t portmod_pm8x50_create_info_internal_t;

/* PM8x50: no translation is required */
typedef portmod_pm8x50_flexe_gen2_create_info_t portmod_pm8x50_flexe_gen2_create_info_internal_t;

/* PM4x10_qtc: no translation is required */
typedef portmod_pm4x10_qtc_create_info_t portmod_pm4x10_qtc_create_info_internal_t;

/* PM8x100: no translation is required */
typedef portmod_pm8x100_gen2_create_info_t portmod_pm8x100_gen2_create_info_internal_t;

/* Union for internal create structures*/
typedef union portmod_pm_specific_create_info_internal_u{
    portmod_dnx_fabric_create_info_internal_t dnx_fabric;
    portmod_pm8x50_fabric_create_info_internal_t pm8x50_fabric;
    portmod_pm8x100_fabric_create_info_internal_t pm8x100_fabric;
    portmod_pm4x25_create_info_internal_t pm4x25;
    portmod_pm4x10_create_info_internal_t pm4x10;
    portmod_pm4x10q_create_info_internal_t pm4x10q;
    portmod_pm_qtc_create_info_internal_t pm_qtc;
    portmod_pm12x10_create_info_internal_t pm12x10;
    portmod_os_ilkn_create_info_internal_t os_ilkn;
    portmod_pm4x2p5_create_info_internal_t pm4x2p5;
    portmod_pm8x50_create_info_internal_t pm8x50;
    portmod_pm8x50_flexe_gen2_create_info_internal_t pm8x50_flexe_gen2;
    portmod_pm4x10_qtc_create_info_internal_t pm4x10_qtc;
    portmod_pm8x100_gen2_create_info_internal_t pm8x100_gen2;
}portmod_pm_specific_create_info_internal_t;

/* the structure which is used as input to portmod_port_macro_add function*/
typedef struct portmod_pm_create_info_internal_s {
    portmod_dispatch_type_t type; /**< PM type */
    portmod_pbmp_t phys; /**< which PHYs belongs to the PM */
    portmod_pm_specific_create_info_internal_t pm_specific_info;
} portmod_pm_create_info_internal_t;

/* PMs can implement xxx_default_bus_update() internal API to update phymod bus
   When this API is implemented the below struct is used as input*/
typedef struct portmod_bus_update_s {
    phymod_firmware_loader_f external_fw_loader;
    plp_aperta2_phymod_bus_t* default_bus;
    void* user_acc;
    int blk_id;
} portmod_bus_update_t;

/* External Phy information */
typedef struct portmod_ext_phy_core_info_s
{
    plp_aperta2_phymod_core_access_t core_access; /**< core access */
    int is_initialized; /**< Phy is Initialized - need to convert to WB */
} portmod_ext_phy_core_info_t;

#define PM_DRIVER(pm_info) (__plp_aperta2_portmod__dispatch__[(pm_info)->type])

#define XPHY_PBMP_IDX_MAX                       PORTMOD_MAX_NUM_XPHY_SUPPORTED
#define XPHY_PBMP_WORD_WIDTH                    32

#define XPHY_PBMP_WORD_MAX                      \
            ((XPHY_PBMP_IDX_MAX+XPHY_PBMP_WORD_WIDTH-1)/XPHY_PBMP_WORD_WIDTH)

#define XPHY_PBMP_WORD_GET(bm,word)             ((bm).pbits[(word)])
#define XPHY_PBMP_WENT(idx)                     ((idx)/XPHY_PBMP_WORD_WIDTH)

#define XPHY_PBMP_WBIT(idx)                     \
                                        (1U << ((idx) % XPHY_PBMP_WORD_WIDTH))

#define XPHY_PBMP_ENTRY(bm, idx)                \
                                    (XPHY_PBMP_WORD_GET(bm,XPHY_PBMP_WENT(idx)))

#define XPHY_PBMP_MEMBER(bm, idx)               \
                        ((XPHY_PBMP_ENTRY(bm, idx) & XPHY_PBMP_WBIT(idx)) != 0)

#define XPHY_PBMP_CLEAR(pbm)                    do { \
        int _w; \
        for (_w = 0; _w < XPHY_PBMP_WORD_MAX; _w++) { \
            XPHY_PBMP_WORD_GET(bm, _w) = 0; \
        } \
    } while (0)

#define XPHY_PBMP_IDX_ADD(bm, idx)             \
                            ( XPHY_PBMP_ENTRY(bm, idx) |=  XPHY_PBMP_WBIT(idx))

#define XPHY_PBMP_IDX_REMOVE(bm, idx)          \
                            ( XPHY_PBMP_ENTRY(bm, idx) &=  ~XPHY_PBMP_WBIT(idx))

#define XPHY_PBMP_ITER(bmp, idx)                \
    for ((idx) = 0; (idx) < XPHY_PBMP_IDX_MAX; (idx)++) \
        if (XPHY_PBMP_MEMBER((bmp), (idx)))

typedef struct xphy_pbmp {
    uint32  pbits[XPHY_PBMP_WORD_MAX];
} xphy_pbmp_t;

#define PORTMOD_DIV_ROUND_UP(x,y) ( ((x)+((y)-1)) / (y) )

#define PORTMOD_BIT_COUNT(pbm, count)  (count = _shr_popcount(pbm))

typedef enum portmod_txpi_sdm_type_e {
    FRIST_ORDER_WITH_FLOOR,     /* 1st order scheme with floor follows the traditional TXPI SDM implementation. */
    FRIST_ORDER_WITH_ROUNDING,  /* 1st order scheme with rounding is a new implementation that supposed to solve frequent updates introduced by the 1st order floor implementation. */
    SECOND_ORDER_WITH_ROUNDING, /* 2nd order scheme with rounding is a new implementation that supposed to solve the low frequency phase nose introduced by the 1st order implementation. */
    TXPI_SDM_SCHEME_COUNT
} portmod_txpi_sdm_type_t;

/* Flags to indicate the interrupt status and clear mechanism. */
/*******************************************************
 *************** Start of Flags ************************
 ******************************************************/

/* NO ECC error */
#define     PORTMOD_PM_ECC_ERR_NONE         PORTMOD_PM_ECC_NO_ERR

/* 1-bit ECC error */
#define     PORTMOD_INTR_ECC_1B_ERR         PORTMOD_PM_ECC_1B_ERR

/* 2-bit ECC error */
#define     PORTMOD_INTR_ECC_2B_ERR         PORTMOD_PM_ECC_2B_ERR

/* Multi-bit ECC error */
#define     PORTMOD_INTR_ECC_MULTIB_ERR     PORTMOD_PM_ECC_MULTIB_ERR

/* Write 1 to hardware to clear the interrupt status */
#define     PORTMOD_INTR_STATUS_W1TC        0x8

/* Interrupt status is clear-on-read */
#define     PORTMOD_INTR_STATUS_COR         0x10

/* Indicates that each bit indicates an interrupt status */
#define     PORTMOD_INTR_STATUS_BIT_FLDS    0x20

/*
 * indicates an ECC error interrupt along with
 * memory address information.
 */
#define     PORTMOD_INTR_READ_ECC_ADDR      0x40

/*
 * Indicates an ECC error interrupt with no
 * memory address information.
 * 1-bit ECC error.
 */
#define     PORTMOD_INTR_NO_READ_ECC_ADDR   0x80

/*
 * Indicates that the clear mechanism is per block
 * and not per port.
 */
#define     PORTMOD_INTR_BLOCK_CLEAR_REG    0x100

/*******************************************************
 *************** End of Flags **************************
 ******************************************************/

typedef struct portmod_intr_reg_info_s {
    uint32                  flags;
    portmod_intr_type_t     intr_type;
    soc_reg_t               en_reg;
    int             en_fld;
    soc_reg_t               st_reg;
    int             st_fld;
    soc_reg_t               clr_reg;
    int             clr_fld;
    char                    *str;
    soc_reg_t               addr_reg;
    int             addr_fld;
}portmod_intr_reg_info_t;


typedef struct portmod_pm_intr_info_s {
    /*
     * Interrupt type here means the highest level of
     * interrupt exposed an the PortMacro port block
     * level. Typically it could be one of below types
     * MAC error, TSC error, PMD error, Link intr,
     * MIB intr, Flow control intr.
     * Each port Macro could expose different interrupts
     */
    portmod_intr_type_t     intr_type;
    soc_reg_t               en_reg;
    int             en_fld;
    int                     is_intr_enable_group;
    portmod_intr_reg_info_t *intr_list_enable_reg;
    soc_reg_t               st_reg;
    int             st_fld;
    int                     is_intr_status_group;
    portmod_intr_reg_info_t *intr_list_status_reg;
    soc_reg_t               clr_reg;
    int             clr_fld;
    int                     is_intr_clear_group;
    portmod_intr_reg_info_t *intr_list_clear_reg;
    soc_mem_t               mem;
    char                    *mem_str;
}portmod_pm_intr_info_t;

/* Initialize portmod_pm_create_info_internal_t structure */
int portmod_pm_create_info_internal_t_init(int unit, portmod_pm_create_info_internal_t *create_info_internal);


/* Get  internal PM info from a port */
int portmod_pm_info_get(int unit, int port, pm_info_t *pm_info);
/* Get list of PMs attached to a phy */
int portmod_phy_pms_info_get(int unit, int phy, int max_pms, pm_info_t *pms_info, int *nof_pms, int *pms_ids);

int portmod_pm_info_type_get(int unit, int port, portmod_dispatch_type_t type, pm_info_t* pm_info);

int portmod_port_interface_type_get(int unit, int port, soc_port_if_t *interface);
int portmod_port_main_core_access_get(int unit, int port, int phyn,
                                      plp_aperta2_phymod_core_access_t *core_access,
                                      int *nof_cores);

/*is interface type supported by PM should be supported by all PMs types*/
int plp_aperta2_portmod_pm_interface_type_is_supported(int unit, pm_info_t pm_info, soc_port_if_t interface, int* is_supported);

int plp_aperta2_portmod_pm_init(int unit, const portmod_pm_create_info_internal_t* pm_add_info, int wb_buffer_index, pm_info_t pm_info);
int plp_aperta2_portmod_pm_destroy(int unit, pm_info_t pm_info);
int plp_aperta2_portmod_pm_core_info_get(int unit, pm_info_t pm_info, int phyn, portmod_pm_core_info_t* core_info);
int plp_aperta2_portmod_port_attach(int unit, int port, const portmod_port_add_info_t* add_info);
int plp_aperta2_portmod_port_detach(int unit, int port);
int plp_aperta2_portmod_port_replace(int unit, int port, int new_port);
int plp_aperta2_portmod_pm_aggregated_update(int unit, pm_info_t pm_info, int core_id, int nof_aggregated_pms, const pm_info_t* pms, const int* pm_ids);
int portmod_next_wb_var_id_get(int unit, int *var_id);
int plp_aperta2_portmod_ext_phy_lane_attach_to_pm(int unit, pm_info_t pm_info, int iphy, int phyn, const portmod_lane_connection_t* lane_connection);
int plp_aperta2_portmod_ext_phy_lane_detach_from_pm(int unit, pm_info_t pm_info, int iphy, int phyn, portmod_lane_connection_t* lane_connection);
int plp_aperta2_portmod_pm_wb_debug_log(int unit, pm_info_t pm_info);

/*!
 * plp_aperta2_portmod_port_mib_reset_toggle
 *
 * @brief  portmod port mib reset toggle
 *
 * @param [in]  unit            - unit id
 * @param [in]  port            - logical port
 * @param [in]  port_index      - internal port
 */
int plp_aperta2_portmod_port_mib_reset_toggle(int unit, int port, int port_index);

int plp_aperta2_portmod_xphy_lane_attach_to_pm(int unit, pm_info_t pm_info, int iphy, int phyn, const portmod_xphy_lane_connection_t* lane_connection);
int plp_aperta2_portmod_xphy_lane_detach_from_pm(int unit, pm_info_t pm_info, int iphy, int phyn, portmod_xphy_lane_connection_t* lane_connection);
int portmod_max_pms_get(int unit, int* max_pms);
int portmod_eyescan_diag_dispatch(int unit, soc_port_t port, args_t *a);
int portmod_phy_pm_type_get(int unit, int phy, portmod_dispatch_type_t *type);
int portmod_xphy_addr_set(int unit, int idx, int xphy_addr);
int portmod_xphy_db_addr_set(int unit, int idx, int xphy_addr);
int portmod_xphy_all_valid_phys_get(int unit, xphy_pbmp_t *active_phys);
int portmod_xphy_valid_phy_set (int unit, int xphy_idx, int valid);
int portmod_xphy_valid_phy_get (int unit, int xphy_idx, int *valid);
int portmod_xphy_db_addr_get(int unit, int xphy_idx, int* xphy_addr);


/*!
 * plp_aperta2_portmod_pm_is_in_pm12x10
 *
 * @brief Get whether the Port Macro is part of PM12x10
 *
 * @param [in]  unit            - unit id
 * @param [in]  pm_info         -
 * @param [out]  in_pm12x10     -
 */
int plp_aperta2_portmod_pm_is_in_pm12x10(int unit, pm_info_t pm_info, int* in_pm12x10);

/*!
 * plp_aperta2_portmod_port_lane_count_get
 *
 * @brief Get the number of lanes belong to  a logical port.
 *
 * @param [in]  unit            - unit id
 * @param [in]  port            - logical port
 * @param [in]  line_side       - line side 1, sys_side 0
 * @param [out]  num_lanes       - num of line side lanes
 */
int plp_aperta2_portmod_port_lane_count_get(int unit, int port, int line_side, int* num_lanes);


#ifdef FW_BCAST_DOWNLOAD
int plp_aperta2_portmod_fw_bcast(int unit, pm_info_t pm_info, const portmod_sbus_bcast_config_t* sbus_bcast_cfg);
#endif

/*!
 * portmod_pm_capability_get
 *
 * @brief Get PortMacro specific capabilites
 *
 * @param [in]  unit            - unit id
 * @param [out]  pm_cap          - PM specific capabilities
 */
int portmod_pm_specific_capability_get(int unit,
                                       portmod_pm_capability_t* pm_cap);


/*******************************************************************/
/**                  PMs Periodic events                          **/
/*******************************************************************/
/*
The below functions enable the PMs to regitser per-port periodic events. 
 
When PM decided periodic operation is required it can register it using 
portmod_port_periodic_callback_register API. 
 
When the operation is no longer required call 
portmod_port_periodic_callback_unregister API 
 
Note that callback should be re-registered upon WB. 
 
*/

/*!
 * Callback for per port periodic event
 */
typedef int (*portmod_shr_thread_manager_f)(int unit, int port);

/*!
 * 
 * portmod_port_periodic_callback_unregister 
 *  
 * @brief - unregister port periodic event
 * 
 * @param unit - unit #
 * @param port - port to unregister callback 
 *  
 * @note This API has protection between callback context and main thread.
 *       Except of callback context usage - it doesn't allow other multithreading. 
 */
int portmod_port_periodic_callback_unregister(int unit, int port);


/*!
 * 
 * portmod_port_periodic_callback_unregister 
 *  
 * @brief - unregister all port periodic events
 * 
 * @param unit - unit #
 *  
 * @note This API has protection between callback context and main thread.
 *       Except of callback context usage - it doesn't allow other multithreading. 
 */
int portmod_port_periodic_callback_unregister_all(int unit);

/*!
 * portmod_port_periodic_callback_register
 * 
 * @brief - register port periodic event
 * 
 * @param unit - unit #
 * @param port - port to register callback
 * @param callback - callback to be called periodically 
 * @param interval - expected period (actual period will be 
 *                 equal or smaller from given value).
 *  
 * @note 
 *       1. Callback must be re-registers after WB
 *       2. This API has protection between callback context and main thread.
 *          Except of callback context usage - it doesn't allow other multithreading. 
 */
int portmod_port_periodic_callback_register(int unit, int port, portmod_shr_thread_manager_f callback, int interval);


/*!
 * portmod_port_periodic_handler_get
 * 
 * @brief - get handler to periodic event object
 *          the purpose of it is unit testing only
 * 
 * @param unit - unit #
 * @param pe_handler - periodic event current interval
 *  
 */
int portmod_port_periodic_interval_get(int unit, int* interval);

/*!
 * portmod_portmon_disable_get
 *
 * @brief - get the port mintor disable configuration
 *
 * @param unit - unit #
 * @param disable - 1 or 0
 *
 */
int portmod_portmon_disable_get(int unit, int *disable);


/*******************************************************************/

int portmod_core_pm_id_get(int unit, int core_num, int *pm_id);
int plp_aperta2_portmod_pm_core_num_get(int unit, pm_info_t pm_info, int* core_num);



/*!
 * portmod_ilkn_block_phys_set
 *
 * @brief - set the phys used by a specific ilkn block id in SW-state
 *
 * @param unit - unit #
 * @param ilkn_block_id - ilkn block id
 * @param phys - bitmap of phys usded for the specific ilkn block
 * @param enable  - 0 | 1 set ore remove the phys to the ilkn block
 *
 */
int portmod_ilkn_block_phys_set(int unit, int ilkn_block_id, portmod_pbmp_t phys, int enable);
int plp_aperta2_portmod_pm_phys_get(int unit, pm_info_t pm_info, portmod_pbmp_t* phys);
int portmod_pm_phys_from_pm_id_get(int unit, int pm_id, portmod_pbmp_t *phys);

#endif /*_PORTMOD_INTERNAL_H_*/


