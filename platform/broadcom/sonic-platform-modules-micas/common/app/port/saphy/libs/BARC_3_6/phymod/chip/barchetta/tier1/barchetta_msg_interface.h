/*
 *
  * $Copyright: (c) 2020 Broadcom.
  * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 */
#ifndef __BARCHETTA_MSG_INTERFACE_H__
#define __BARCHETTA_MSG_INTERFACE_H__

/*
 * Includes
 */
#include <phymod/phymod.h>
#include <phymod/phymod_acc.h>
#include "bcmi_barchetta_defs.h"

#define BARCHETTA_MAX_NUM_DUPLEX_PORTS       (8)   /* Maximum number of duplex ports in Barchetta    */
#define BARCHETTA_MAX_NUM_SIMPLEX_PORTS      (16)  /* Maximum number of simplex ports in Barchetta   */
#define BARCHETTA_MAX_NUM_DUPLEX_LANES       (8)   /* Maximum number of duplex lanes in Barchetta    */
#define BARCHETTA_MAX_NUM_SIMPLEX_LANES      (16)  /* Maximum number of simplex lanes in Barchetta   */
#define BARCHETTA_NUM_OF_SIDES               (2)   /* Number of sides in Barchetta                   */
#define BARCHETTA_MSG_IF_BUF_LEN             (256) /* MSGIN or MSGOUT buffer length                  */
#define BARCHETTA_MSG_IF_RETRY_COUNT         (1000)/* Message interface retry count                  */

/* The following user readable macros are created for message interface hardware abstraction purpose */
#define m_BARCHETTA_GET_MSG_IN_BUF_BASE_ADDR()        BCMI_BARCHETTA_FW_FWREG_000r
#define m_BARCHETTA_GET_MSG_OUT_BUF_BASE_ADDR()       BCMI_BARCHETTA_FW_FWREG_080r
#define m_BARCHETTA_GET_MSG_IN_REG_ADDR()             BCMI_BARCHETTA_GEN_CNTRLS_MST_MSGINr
#define m_BARCHETTA_GET_MSG_OUT_REG_ADDR()            BCMI_BARCHETTA_GEN_CNTRLS_MST_MSGOUTr

/*!
 * @enum barchetta_side_select_e
 * Constants to select Side (SYS/LINE)
 */
typedef enum barchetta_side_select_e {
    BARCHETTA_SIDE_SYS   = (0x00), /* Select system side                 */
    BARCHETTA_SIDE_LINE  = (0x01), /* Select line side                   */
    BARCHETTA_BOTH_SIDES = (0x02)  /* Select Both sides (System and Line)*/

} barchetta_side_select_t;

/*!
 * @enum barchetta_msg_function_e
 * Constants to select Message function in the Message Interface module
 */
typedef enum barchetta_msg_function_e {
    /* Functions that respond with completion code (OP_SUCCESS/OP_ERROR) */
    BARCHETTA_FUNC_CONFIG_LANES    = (0x10), /* CONFIG_LANES (Configure Rx-Tx Lnae Pairs)          */
    BARCHETTA_FUNC_CONFIG_PORT     = (0x11), /* CONFIG_PORT(Configure a Port)                      */
    BARCHETTA_FUNC_CONFIG_PMD      = (0x12), /* Read PMD Configuration of a Port                   */
    BARCHETTA_FUNC_PAUSE_PORT      = (0x13), /* PAUSE_PORT(Firmware relinquishes the
                                                control of the port and gives control to software) */
    BARCHETTA_FUNC_RESUME_PORT     = (0x14), /* RESUME_PORT(Firmware take back control of the port)*/
    BARCHETTA_FUNC_CONFIG_POL_SWAP = (0x15), /* CONFIG_POLSWAPS (Configure Polarity Swaps)         */

    /* Functions that respond with OP_PROCESSING code immediately,
     actual execution is done in the background loop */
    BARCHETTA_FUNC_ENABLE_PORT     = (0x20), /* ENABLE_PORT(Enable a Port)                         */
    BARCHETTA_FUNC_DISABLE_PORT    = (0x21), /* DISABLE_PORT (Disable a Port)                      */
    BARCHETTA_FUNC_MODIFY_PORT     = (0x22), /* MODIFY_PORT (Modify a Port)                        */
    BARCHETTA_FUNC_SWITCH_MUX_PORT = (0x23), /* SWITCH_MUX_PORT (Switch Failover Mux of a Port)    */

    BARCHETTA_FUNC_RESERVED        = (0xFF) /* Reserved */

} barchetta_msg_function_t;

/*!
 * @enum barchetta_msg_operation_e
 * Constants to select Message operation in the Message Interface module
 */
typedef enum barchetta_msg_operation_e {
    /* Command operation */
    BARCHETTA_OP_WRITE        = (0x0), /* write operation (one/more consecutive entries)     */
    BARCHETTA_OP_READ         = (0x1), /* read  operation (one/more consecutive entries)     */
    BARCHETTA_OP_WRITE_EXT    = (0x2), /* write extended  (one/more non-consecutive entries) */
    BARCHETTA_OP_READ_EXT     = (0x3), /* read  extended  (one/more non-consecutive entries) */

    BARCHETTA_OP_START        = (0x0), /* start a firmware process (state machine)           */
    BARCHETTA_OP_START_RESULT = (0x1), /* get status of the started firmware process         */

    /* Response operation */
    BARCHETTA_OP_PROCESSING   = (0xD), /* operation in progress (processing)                 */
    BARCHETTA_OP_SUCCESS      = (0xE), /* operation result: success                          */
    BARCHETTA_OP_ERROR        = (0xF)  /* operation result: error                            */

} barchetta_msg_operation_t;

/*!
 * @enum barchetta_msg_status_e
 * Constants to select Message status in the Message Interface module
 */
typedef enum barchetta_msg_status_e {
    BARCHETTA_MSG_STATUS_IDLE  = (0x0), /* Message status idle               */
    BARCHETTA_MSG_STATUS_SENT  = (0x1), /* Message status sent               */
    BARCHETTA_MSG_STATUS_RECD  = (0x2), /* Message status received           */
    BARCHETTA_MSG_STATUS_PROCD = (0x3), /* Message status processed          */
    BARCHETTA_MSG_STATUS_RESET = (0xF)  /* Message status reset (chip reset) */

} barchetta_msg_status_t;

/*!
 * @enum barchetta_msg_if_ret_status_e
 * Constants to specify barchetta message interface API's return status
 */
typedef enum barchetta_msg_if_ret_status_e {
    BARCHETTA_MSG_IF_RET_SUCCESS = (0x0), /* Message interface API return with success */
    BARCHETTA_MSG_IF_RET_ERROR   = (0x1) /* Message interface API return with error   */

} barchetta_msg_if_ret_status_t;

/*!
 * @enum barchetta_msg_if_err_code_e
 * Constants for error codes returned in the Message Interface module
 */
typedef enum barchetta_msg_if_err_code_e {
    BARCHETTA_MSG_IF_ERR_FUNC_NOT_AVAILABLE = (0x10), /* Function not available  */
    BARCHETTA_MSG_IF_ERR_OP_NOT_AVAILABLE   = (0x11), /* Operation not available */
    BARCHETTA_MSG_IF_ERR_INCORRECT_LENGTH   = (0x12), /* Incorrect length        */
    BARCHETTA_MSG_IF_ERR_INCORRECT_PARAM    = (0x13), /* Incorrect parameters    */
    BARCHETTA_MSG_IF_ERR_FUNC_SPECIFIC      = (0x14), /* Function specific error */
    BARCHETTA_MSG_IF_ERR_HW_FW_BUSY         = (0x20), /* HW/FW busy error        */
    BARCHETTA_MSG_IF_ERR_UNKNOWN            = (0xFF)  /* Unknown error           */

} barchetta_msg_if_err_code_t;

/*!
 * @enum barchetta_port_direction_e
 * Constants to specify port type (Duplex/Simplex)
 */
typedef enum barchetta_port_direction_e {
    BARCHETTA_PORT_TYPE_DUPLEX = (0), /* Duplex port */
    BARCHETTA_PORT_TYPE_SIMPLEX_SR2LT = (1), /* Simplex system side Rx to line side Tx */
    BARCHETTA_PORT_TYPE_SIMPLEX_LR2ST = (2)  /* Simplex line side Rx to system side Tx */

} barchetta_port_type_t;

/*---------------------------------------------------------------------------*/
/*               Lane config data structures are defined below               */
/*---------------------------------------------------------------------------*/

/*!
 * Structure to hold duplex lanes data
 */
typedef struct barchetta_duplex_lanes_s {
    uint8_t num_sys_lanes;  /* Number of system side lanes */
    uint8_t num_line_lanes; /* Number of line side lanes   */
    /* System side RX package lane numbers that corresponds to logical RX lanes 0 to 7 in order */
    uint8_t sr_lane_list[BARCHETTA_MAX_NUM_DUPLEX_LANES];
    /* System side TX package lane numbers that corresponds to logical TX lanes 0 to 7 in order */
    uint8_t st_lane_list[BARCHETTA_MAX_NUM_DUPLEX_LANES];
    /* Line side RX package lane numbers that corresponds to logical RX lanes 0 to 7 in order   */
    uint8_t lr_lane_list[BARCHETTA_MAX_NUM_DUPLEX_LANES];
    /* Line side TX package lane numbers that corresponds to logical TX lanes 0 to 7 in order   */
    uint8_t lt_lane_list[BARCHETTA_MAX_NUM_DUPLEX_LANES];

} barchetta_duplex_lanes_t;

/*!
 * Structure to hold SR2LT lanes data
 *
 * NOTE: Subject to Change as Simplex is not defined completely yet
 *
 */
typedef struct barchetta_sr2lt_lanes_s {
    uint8_t num_sys_lanes;
    uint8_t num_line_lanes;
    uint8_t sr_lane_list[BARCHETTA_MAX_NUM_SIMPLEX_LANES];
    uint8_t lt_lane_list[BARCHETTA_MAX_NUM_SIMPLEX_LANES];

} barchetta_sr2lt_lanes_t;

/*!
 * Structure to hold LR2ST lanes data
 *
 * NOTE: Subject to Change as Simplex is not defined completely yet
 *
 */
typedef struct barchetta_lr2st_lanes_s {
    uint8_t num_sys_lanes;
    uint8_t num_line_lanes;
    uint8_t st_lane_list[BARCHETTA_MAX_NUM_SIMPLEX_LANES];
    uint8_t lr_lane_list[BARCHETTA_MAX_NUM_SIMPLEX_LANES];

} barchetta_lr2st_lanes_t;

/*!
 *  Structure that holds CONFIG_LANES message parameters
 */
typedef struct barchetta_config_lanes_s {
    uint8_t config_type; /* config_type (Duplex/SR2LT/LR2ST)        */
    union {
        barchetta_duplex_lanes_t duplex; /* Duplex lane config parameters structure */
        barchetta_sr2lt_lanes_t sr2lt;   /* SR2LT lane config parameters structure  */
        barchetta_lr2st_lanes_t lr2st;   /* LR2ST lane config parameter structure   */

    } lanes;

} barchetta_config_lanes_t;

/*---------------------------------------------------------------------------*/
/*               Polarity Config data structures are defined below           */
/*---------------------------------------------------------------------------*/

/*!
 * Structure to hold duplex polarity swaps
 */
typedef struct barchetta_duplex_polarity_swap_s {
    uint8_t num_sys_lanes;  /* Number of System side lanes */
    uint8_t num_line_lanes; /* Number of line side lanes   */

    /* System-side Rx lane polarity swap bits (each bit corresponds to a logical lane) */
    uint8_t sr_polarity_swap;
    /* System-side Tx lane polarity swap bits (each bit corresponds to a logical lane) */
    uint8_t st_polarity_swap;

    /* Line-side Rx lane polarity swap bits (each bit corresponds to a logical lane)   */
    uint8_t lr_polarity_swap;
    /* Line-side Tx lane polarity swap bits (each bit corresponds to a logical lane)   */
    uint8_t lt_polarity_swap;

} barchetta_duplex_polarity_swap_t;

/*!
 * Structure to hold SR2LT polarity swaps
 *
 * NOTE: Subject to Change as Simplex is not defined completely yet
 *
 */
typedef struct barchetta_sr2lt_polarity_swap_s {
    /* Number of lanes */
    uint8_t num_sys_lanes;
    uint8_t num_line_lanes;

    /* System-side Rx lane polarity swap bits (each bit corresponds to a logical lane) */
    uint8_t sr_polarity_swap;

    /* Line-side Tx lane polarity swap bits (each bit corresponds to a logical lane)   */
    uint8_t lt_polarity_swap;

} barchetta_sr2lt_polarity_swap_t;

/*!
 * Structure to hold LR2ST polarity swaps
 *
 * NOTE: Subject to Change as Simplex is not defined completely yet
 *
 */
typedef struct barchetta_lr2st_polarity_swap_s {
    /* Number of lanes */
    uint8_t num_sys_lanes;
    uint8_t num_line_lanes;

    /* System-side Tx lane polarity swap bits (each bit corresponds to a logical lane) */
    uint8_t st_polarity_swap;

    /* Line-side Rx lane polarity swap bits (each bit corresponds to a logical lane) */
    uint8_t lr_polarity_swap;

} barchetta_lr2st_polarity_swap_t;

/*!
 *  Structure that holds CONFIG_POLSWAPS message parmeters
 */
typedef struct barchetta_config_polarity_swap_s {
    uint8_t config_type;   /* Config Type (Duplex/SR2LT/LR2ST) */

    union {
        barchetta_duplex_polarity_swap_t duplex;
        barchetta_sr2lt_polarity_swap_t sr2lt;
        barchetta_lr2st_polarity_swap_t lr2st;
    } polarity_swap;

} barchetta_config_polarity_swap_t;

/*---------------------------------------------------------------------------*/
/*               Port config data structures are defined below               */
/*---------------------------------------------------------------------------*/

/*!
 * Structure to hold duplex port data (num of lanes, list of lanes in order)
 */
typedef struct barchetta_duplex_port_lanes_s {
    uint8_t num_sys_lanes; /* Number of system side lanes         */
    uint8_t num_line_lanes; /* Number of line side lanes          */
    /* System side Rx lane numbers (virtual/logical lane numbers) */
    uint8_t sys_lane_list[BARCHETTA_MAX_NUM_DUPLEX_LANES];
    /* Line side Rx lane numbers (virtual/logical lane numbers)   */
    uint8_t line_lane_list[BARCHETTA_MAX_NUM_DUPLEX_LANES];

} barchetta_duplex_port_lanes_t;

/*!
 * Structure to hold simplex System Side Rx to Line Side Tx port data
 *
 * NOTE: Subject to Change as Simplex is not defined completely yet
 *
 */
typedef struct barchetta_sr2lt_port_lanes_s {
    uint8_t num_sys_lanes;
    uint8_t num_line_lanes;
    uint8_t sys_lane_list[BARCHETTA_MAX_NUM_SIMPLEX_LANES];
    uint8_t line_lane_list[BARCHETTA_MAX_NUM_SIMPLEX_LANES];

} barchetta_sr2lt_port_lanes_t;

/*!
 * Structure to hold simplex Line Side Rx to System Side Tx port data
 *
 * NOTE: Subject to Change as Simplex is not defined completely yet
 *
 */
typedef struct barchetta_lr2st_port_lanes_s {
    uint8_t num_sys_lanes;
    uint8_t num_line_lanes;
    uint8_t sys_lane_list[BARCHETTA_MAX_NUM_SIMPLEX_LANES];
    uint8_t line_lane_list[BARCHETTA_MAX_NUM_SIMPLEX_LANES];

} barchetta_lr2st_port_lanes_t;

/*!
 *  Structure that holds CONFIG_PORT message parameters
 */
typedef struct barchetta_port_config_s {
    uint8_t port_num;  /* Hardware port number              */
    uint8_t port_type; /* Port Type (Duplex/SR2LT/LR2ST)    */
    uint8_t port_mode; /* Port Mode (Regular / FailoverMux) */
    union {
        /* Duplex port lane config parameters structure */
        barchetta_duplex_port_lanes_t duplex;
        /* SR2LT port lane config parameters structure  */
        barchetta_sr2lt_port_lanes_t sr2lt;
        /* LR2ST port lane config parameter structure   */
        barchetta_lr2st_port_lanes_t lr2st;

    } port_lanes;

} barchetta_port_config_t;

/*---------------------------------------------------------------------------*/
/*               Port Enable data structures are defined below               */
/*---------------------------------------------------------------------------*/

/*!
 * Structure to hold the PMD configuration of a port per side
 */
typedef struct barchetta_pmd_config_s {
    uint8_t clock_mode;        /* Re-timer clock (Recovered/Reference)       */
    uint8_t pll_sel;           /* PLL selection (0/1)                        */
    uint8_t osr;               /* Over sampling rate                         */

    uint8_t an_opt;            /* AN option (1: Enable, 0: Disable)          */
    uint8_t an_use_pcs_mon;    /* Use PCS Monitor for linkup                 */
    uint8_t an_master_lane;    /* AN Master lane                             */

    uint8_t tx_training_opt;   /* Tx Training Option (1: Enable; 0: Disable) */
    uint8_t fec_opt;           /* FEC Option (1: Enable, 0: Disable)         */
    uint16_t lane_config_word; /* PMD Lane Configuration Word (16-bits)      */

} barchetta_pmd_config_t;

/*!
 *  Structure that holds ENABLE_PORT message parameters
 */
typedef struct barchetta_enable_port_s {
    uint8_t port_num;     /* Hardware Port Number                            */
    uint8_t traffic_type; /* Ethernet/Fiber Channel (0: Ethernet, 1: FC)     */
    uint8_t ll_mode; /* Low Latency Mode                                     */
    uint8_t div2en;  /* DP Clk Div2En: For 40G_20x2 (Sys) -> 40G_10x4 (Line) */
    /* Port PMD Configuration Per Side */
    barchetta_pmd_config_t pmd_cfg[BARCHETTA_NUM_OF_SIDES];

} barchetta_enable_port_t;

/*---------------------------------------------------------------------------*/
/*               Port modify data structures are defined below               */
/*---------------------------------------------------------------------------*/
typedef struct barchetta_modify_port_s {
    uint8_t port_num;          /* Hardware Port Number                       */
    uint8_t pmd_side;          /* PMD Side                                   */

    uint8_t an_opt;            /* AN option (1: Enable, 0: Disable)          */
    uint8_t an_use_pcs_mon;    /* Use PCS Monitor for linkup                 */
    uint8_t an_master_lane;    /* AN Master lane                             */

    uint8_t tx_training_opt;   /* Tx Training Option (1: Enable; 0: Disable) */
    uint8_t fec_opt;           /* FEC Option (1: Enable, 0: Disable)         */
    uint16_t lane_config_word; /* PMD Lane Configuration Word (16-bits)      */

} barchetta_modify_port_t;

/*---------------------------------------------------------------------------*/
/*               config PMD data structures are defined below                */
/*---------------------------------------------------------------------------*/
typedef struct barchetta_config_pmd_s {
    uint8_t port_num;          /* Hardware Port Number                       */
    uint8_t pmd_side;          /* PMD Side                                   */

    uint8_t an_opt;            /* AN option (1: Enable, 0: Disable)          */
    uint8_t an_use_pcs_mon;    /* Use PCS Monitor for linkup                 */
    uint8_t an_master_lane;    /* AN Master lane                             */

    uint8_t tx_training_opt;   /* Tx Training Option (1: Enable; 0: Disable) */
    uint8_t fec_opt;           /* FEC Option (1: Enable, 0: Disable)         */
    uint16_t lane_config_word; /* PMD Lane Configuration Word (16-bits)      */

} barchetta_config_pmd_t;

/*---------------------------------------------------------------------------*/
/*                   Message Interface API declaration                       */
/*---------------------------------------------------------------------------*/

/***************************************************************************//**
 \brief    Function to send a message to chip-level firmware

 \details  This functions sends a message, waits for the firmware to process the message
 and finally reads bakc the response from firmware.

 \param    pa         [In]Pointer to phy access structure
 \param    function   [In] Message function to be executed (For Examle, CONFIG_PORT, ENABLE_PORT etc)
 \param    operation  [In] Message operation to be performed (For example, Write / Read)
 \param    tx_msg     [In] Pointer to Message In buffer content
 \param    rx_msg     [out]Pointer to Message Out buffer content
 \param    rx_msg_size[out]Size of rx_msg in Bytes

 \return   Return the firmware response operation

 \retval   #OP_PROCESSING  Firmware is processing the message
 \retval   #OP_SUCCESS     Firmware successfully processed the message
 \retval   #OP_ERROR       Firmware encountered an error while processing the message
 *******************************************************************************/
uint8_t _plp_barchetta_msg_send(
    const plp_barchetta_phymod_access_t *pa,
    uint8_t function,
    uint8_t operation,
    uint8_t *tx_msg,
    uint8_t *rx_msg,
    uint32_t rx_msg_size
);

/***************************************************************************//**
 \brief    Function to assign Virtual/Logical lane numbers to the Rx-Tx pairs
 selected by the user during startup (CONFIG_LANES.WRITE)

 \param    pa         [In]Pointer to phy access structure
 \param    lanes_cfg  [In]Pointer to lane configuration structure

 \return   Status information

 \retval   BARCHETTA_MSG_IF_RET_SUCCESS
 \retval   BARCHETTA_MSG_IF_RET_ERROR
 *******************************************************************************/
uint8_t _plp_barchetta_msg_config_lanes_wr(
    const plp_barchetta_phymod_access_t *pa,
    barchetta_config_lanes_t *lanes_cfg
);

/***************************************************************************//**
 \brief    Function to read the lane configuration (CONFIG_LANES.READ)

 \param    pa         [In]Pointer to phy access structure
 \param    lanes_cfg  [Out]Pointer to lane configuration structure

 \return   Status information

 \retval   BARCHETTA_MSG_IF_RET_SUCCESS
 \retval   BARCHETTA_MSG_IF_RET_ERROR
 *******************************************************************************/
uint8_t _plp_barchetta_msg_config_lanes_rd(
    const plp_barchetta_phymod_access_t *pa,
    barchetta_config_lanes_t *lanes_cfg
);

/***************************************************************************//**
 \brief    Function to configure polarity swaps based on user input

 \param    pa            [In]Pointer to phy access structure
 \param    polarity_cfg  [In]Pointer to polarity swap configuration structure

 \return   Status information

 \retval   BARCHETTA_MSG_IF_RET_SUCCESS
 \retval   BARCHETTA_MSG_IF_RET_ERROR
 *******************************************************************************/
uint8_t _plp_barchetta_msg_config_polarity_swap_wr(
    const plp_barchetta_phymod_access_t *pa,
    barchetta_config_polarity_swap_t *polarity_cfg
);

/***************************************************************************//**
 \brief    Function to read a configuration of polarity swaps (CONFIG_POLSWAPS.READ)

 \param    pa            [In]Pointer to phy access structure
 \param    polarity_cfg  [In]Pointer to polarity swap configuration structure

 \return   Status information

 \retval   BARCHETTA_MSG_IF_RET_SUCCESS
 \retval   BARCHETTA_MSG_IF_RET_ERROR
 *******************************************************************************/
uint8_t _plp_barchetta_msg_config_polarity_swap_rd(
    const plp_barchetta_phymod_access_t *pa,
    barchetta_config_polarity_swap_t *polarity_cfg
);

/***************************************************************************//**
 \brief    Function to configure a Port (CONFIG_PORT.WRITE)

 \param    pa         [In]Pointer to phy access structure
 \param    port_cfg   [In]Pointer to port configuration structure

 \return   Status information

 \retval   BARCHETTA_MSG_IF_RET_SUCCESS
 \retval   BARCHETTA_MSG_IF_RET_ERROR
 *******************************************************************************/
uint8_t _plp_barchetta_msg_config_port_wr(
    const plp_barchetta_phymod_access_t *pa,
    barchetta_port_config_t *port_cfg
);

/***************************************************************************//**
 \brief    Function to read a configuration of port (CONFIG_PORT.READ)

 \param    pa         [In] Pointer to phy access structure
 \param    port_num   [In] Port number for which the configuration to be read
 \param    port_cfg   [Out]Pointer to port configuration read out structure

 \return   Status information

 \retval   BARCHETTA_MSG_IF_RET_SUCCESS
 \retval   BARCHETTA_MSG_IF_RET_ERROR
 *******************************************************************************/
uint8_t _plp_barchetta_msg_config_port_rd(
    const plp_barchetta_phymod_access_t *pa,
    uint8_t port_num,
    barchetta_port_config_t *port_cfg
);

/***************************************************************************//**
 \brief    Function to Start the process of enabling a Port

 \param    pa         [In] Pointer to phy access structure
 \param    port_en    [In] Pointer to port enable structure

 \return   Status information

 \retval   BARCHETTA_MSG_IF_RET_SUCCESS
 \retval   BARCHETTA_MSG_IF_RET_ERROR
 *******************************************************************************/
uint8_t _plp_barchetta_msg_enable_port_start(
    const plp_barchetta_phymod_access_t *pa,
    barchetta_enable_port_t *port_en
);

/***************************************************************************//**
 \brief    Function to wait until the process of enabling a Port is complete

 \param    pa         [In] Pointer to phy access structure
 \param    port_num   [In] Port number

 \return   Status information

 \retval   BARCHETTA_MSG_IF_RET_SUCCESS
 \retval   BARCHETTA_MSG_IF_RET_ERROR
 *******************************************************************************/
uint8_t _plp_barchetta_msg_enable_port_start_result(
    const plp_barchetta_phymod_access_t *pa,
    uint8_t port_num
);

/***************************************************************************//**
 \brief    Function to Start enabling a Port and wait until it is complete

 \param    pa         [In] Pointer to phy access structure
 \param    port_en    [In] Pointer to port enable structure

 \return   Status information

 \retval   BARCHETTA_MSG_IF_RET_SUCCESS
 \retval   BARCHETTA_MSG_IF_RET_ERROR
 *******************************************************************************/
uint8_t _plp_barchetta_msg_enable_port(
    const plp_barchetta_phymod_access_t *pa,
    barchetta_enable_port_t *port_en
);

/***************************************************************************//**
 \brief    Function to read PMD configuration of a port (CONFIG_PMD.READ)

 \param    pa         [In] Pointer to phy access structure
 \param    port_num   [In] Port number for which the pmd configuration to be read
 \param    pmd_side   [In] PMD side information (SYSTEM/LINE)
 \param    port_cfg   [Out]Pointer to PMD configuration read out structure

 \return   Status information

 \retval   BARCHETTA_MSG_IF_RET_SUCCESS
 \retval   BARCHETTA_MSG_IF_RET_ERROR
 *******************************************************************************/
uint8_t _plp_barchetta_msg_config_pmd_rd(
    const plp_barchetta_phymod_access_t *pa,
    uint8_t port_num,
    uint8_t pmd_side,
    barchetta_config_pmd_t *pmd_cfg
);

/***************************************************************************//**
 \brief Function to Start the process of modifying a Port

 \param    pa         [In] Pointer to phy access structure
 \param    port_mod   [In] Pointer to port modify structure

 \return   Status information

 \retval   BARCHETTA_MSG_IF_RET_SUCCESS
 \retval   BARCHETTA_MSG_IF_RET_ERROR
 *******************************************************************************/
uint8_t _plp_barchetta_msg_modify_port_start(
    const plp_barchetta_phymod_access_t *pa,
    barchetta_modify_port_t *port_mod
);

/***************************************************************************//**
 \brief Function to wait until the process of modifying a Port is complete

 \param    pa         [In] Pointer to phy access structure
 \param    port_num   [In] Port number

 \return   Status information

 \retval   BARCHETTA_MSG_IF_RET_SUCCESS
 \retval   BARCHETTA_MSG_IF_RET_ERROR
 *******************************************************************************/
uint8_t _plp_barchetta_msg_modify_port_start_result(
    const plp_barchetta_phymod_access_t *pa,
    uint8_t port_num
);

/***************************************************************************//**
 \brief Function to Start modifying a Port and wait until the process is complete

 \param    pa         [In] Pointer to phy access structure
 \param    port_mod   [In] Pointer to port modify structure

 \return   Status information

 \retval   BARCHETTA_MSG_IF_RET_SUCCESS
 \retval   BARCHETTA_MSG_IF_RET_ERROR
 *******************************************************************************/
uint8_t _plp_barchetta_msg_modify_port(
    const plp_barchetta_phymod_access_t *pa,
    barchetta_modify_port_t *port_mod
);

/***************************************************************************//**
 \brief    Function to Start the process of disabling a Port

 \param    pa         [In] Pointer to phy access structure
 \param    port_num   [In] Port number

 \return   Status information

 \retval   BARCHETTA_MSG_IF_RET_SUCCESS
 \retval   BARCHETTA_MSG_IF_RET_ERROR
 *******************************************************************************/
uint8_t _plp_barchetta_msg_disable_port_start(
    const plp_barchetta_phymod_access_t *pa,
    uint8_t port_num
);

/***************************************************************************//**
 \brief Function to wait until the process of disabling a Port is complete

 \param    pa         [In] Pointer to phy access structure
 \param    port_num   [In] Port number

 \return   Status information

 \retval   BARCHETTA_MSG_IF_RET_SUCCESS
 \retval   BARCHETTA_MSG_IF_RET_ERROR
 *******************************************************************************/
uint8_t _plp_barchetta_msg_disable_port_start_result(
    const plp_barchetta_phymod_access_t *pa,
    uint8_t port_num
);

/***************************************************************************//**
 \brief    Function to Start disabling a Port and wait until it is complete

 \param    pa         [In] Pointer to phy access structure
 \param    port_num   [In] Port number

 \return   Status information

 \retval   BARCHETTA_MSG_IF_RET_SUCCESS
 \retval   BARCHETTA_MSG_IF_RET_ERROR
 *******************************************************************************/
uint8_t _plp_barchetta_msg_disable_port(
    const plp_barchetta_phymod_access_t *pa,
    uint8_t port_num
);

/***************************************************************************//**
 \brief    Function to pause a Port (PAUSE_PORT.WRITE)

 \param    pa         [In] Pointer to phy access structure
 \param    port_num   [In] Port number

 \return   Status information

 \retval   BARCHETTA_MSG_IF_RET_SUCCESS
 \retval   BARCHETTA_MSG_IF_RET_ERROR
 *******************************************************************************/
uint8_t _plp_barchetta_msg_pause_port_wr(
    const plp_barchetta_phymod_access_t *pa,
    uint8_t port_num
);

/***************************************************************************//**
 \brief    Function to resume a Port (RESUME_PORT.WRITE)

 \param    pa         [In] Pointer to phy access structure
 \param    port_num   [In] Port number

 \return   Status information

 \retval   BARCHETTA_MSG_IF_RET_SUCCESS
 \retval   BARCHETTA_MSG_IF_RET_ERROR
 *******************************************************************************/
uint8_t _plp_barchetta_msg_resume_port_wr(
    const plp_barchetta_phymod_access_t *pa,
    uint8_t port_num
);

/***************************************************************************//**
 \brief    Function to start switching over failover port on a mux port

 \param    pa         [In] Pointer to phy access structure
 \param    port_num   [In] Port number

 \return   Status information

 \retval   BARCHETTA_MSG_IF_RET_SUCCESS
 \retval   BARCHETTA_MSG_IF_RET_ERROR
 *******************************************************************************/
uint8_t _plp_barchetta_msg_switch_mux_port_start(
    const plp_barchetta_phymod_access_t *pa,
    uint8_t port_num
);

/***************************************************************************//**
 \brief    Function to wait until switching to failover port on a MUX

 \param    pa         [In] Pointer to phy access structure
 \param    port_num   [In] Port number

 \return   Status information

 \retval   BARCHETTA_MSG_IF_RET_SUCCESS
 \retval   BARCHETTA_MSG_IF_RET_ERROR
 *******************************************************************************/
uint8_t _plp_barchetta_msg_switch_mux_port_start_result(
    const plp_barchetta_phymod_access_t *pa,
    uint8_t port_num
);

#endif /* __BARCHETTA_MSG_INTERFACE_H__ */
