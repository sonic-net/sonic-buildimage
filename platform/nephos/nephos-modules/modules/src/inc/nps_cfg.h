/* Copyright (C) 2020  MediaTek, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program.
 */

/* FILE NAME:   nps_cfg.h
 * PURPOSE:
 *      Customer configuration on NPS SDK.
 * NOTES:
 */

#ifndef NPS_CFG_H
#define NPS_CFG_H

/* INCLUDE FILE DECLARATIONS
 */

#include <nps_types.h>
#include <nps_error.h>


/* NAMING CONSTANT DECLARATIONS
 */

/* MACRO FUNCTION DECLARATIONS
 */
#define NPS_CFG_MAXIMUM_CHIPS_PER_SYSTEM    (16)
#define NPS_CFG_USER_PORT_NUM               (96)

/* DATA TYPE DECLARATIONS
 */
typedef enum
{
    NPS_CFG_TYPE_CHIP_MODE,                 /* chip operating mode. 0: legacy mode, 1:hybrid mode */

    NPS_CFG_TYPE_PORT_MAX_SPEED,            /* Reference to NPS_PORT_SPEED_XXX */
    NPS_CFG_TYPE_PORT_LANE_NUM,
    NPS_CFG_TYPE_PORT_TX_LANE,
    NPS_CFG_TYPE_PORT_RX_LANE,
    NPS_CFG_TYPE_PORT_TX_POLARITY_REV,
    NPS_CFG_TYPE_PORT_RX_POLARITY_REV,
    NPS_CFG_TYPE_PORT_EXT_LANE,
    NPS_CFG_TYPE_PORT_VALID,

    /* l2 module related configuration */
    NPS_CFG_TYPE_L2_THREAD_PRI,
    NPS_CFG_TYPE_L2_THREAD_STACK,           /* customize L2 thread stack size in bytes */
    NPS_CFG_TYPE_L2_ADDR_MODE,              /* L2 address operation mode. 0: Polling mode, 1: FIFO mode */

    /* PKT module related configuration */
    NPS_CFG_TYPE_PKT_TX_GPD_NUM,
    NPS_CFG_TYPE_PKT_RX_GPD_NUM,
    NPS_CFG_TYPE_PKT_RX_SCHED_MODE,         /* 0: RR mode, 1: WRR mode                      */
    NPS_CFG_TYPE_PKT_TX_QUEUE_LEN,
    NPS_CFG_TYPE_PKT_RX_QUEUE_LEN,
    NPS_CFG_TYPE_PKT_RX_QUEUE_WEIGHT,       /* valid while NPS_CFG_TYPE_PKT_RX_SCHED_MODE is 1
                                             * param0: queue
                                             * param1: NA
                                             * value : weight
                                             */
    NPS_CFG_TYPE_PKT_RX_ISR_THREAD_PRI,
    NPS_CFG_TYPE_PKT_RX_ISR_THREAD_STACK,     /* customize PKT RX ISR thread stack size in bytes */
    NPS_CFG_TYPE_PKT_RX_FREE_THREAD_PRI,
    NPS_CFG_TYPE_PKT_RX_FREE_THREAD_STACK,    /* customize PKT RX free thread stack size in bytes */
    NPS_CFG_TYPE_PKT_TX_ISR_THREAD_PRI,
    NPS_CFG_TYPE_PKT_TX_ISR_THREAD_STACK,     /* customize PKT TX ISR thread stack size in bytes */
    NPS_CFG_TYPE_PKT_TX_FREE_THREAD_PRI,
    NPS_CFG_TYPE_PKT_TX_FREE_THREAD_STACK,    /* customize PKT TX free thread stack size in bytes */
    NPS_CFG_TYPE_PKT_ERROR_ISR_THREAD_PRI,
    NPS_CFG_TYPE_PKT_ERROR_ISR_THREAD_STACK,  /* customize PKT ERROR ISR thread stack size in bytes */

    /* STAT module related configuration */
    NPS_CFG_TYPE_CNT_THREAD_PRI,
    NPS_CFG_TYPE_CNT_THREAD_STACK,          /* customize CNT thread stack size in bytes */

    /* IFMON module related configuration */
    NPS_CFG_TYPE_IFMON_THREAD_PRI,
    NPS_CFG_TYPE_IFMON_THREAD_STACK,        /* customize IFMON thread stack size in bytes */

    /* share memory related configuration */
    NPS_CFG_TYPE_SHARE_MEM_SDN_ENTRY_NUM,   /* SDN flow table entry number from share memory */
    NPS_CFG_TYPE_SHARE_MEM_L3_ENTRY_NUM,    /* L3 entry number from share memory */
    NPS_CFG_TYPE_SHARE_MEM_L2_ENTRY_NUM,    /* L2 entry number from share memory */

    /* DLB related configuration */
    NPS_CFG_TYPE_DLB_MONITOR_MODE,          /* DLB monitor mode. 1: async, 0: sync */
    NPS_CFG_TYPE_DLB_LAG_MONITOR_THREAD_PRI,
    NPS_CFG_TYPE_DLB_LAG_MONITOR_THREAD_SLEEP_TIME,
    NPS_CFG_TYPE_DLB_L3_MONITOR_THREAD_PRI,
    NPS_CFG_TYPE_DLB_L3_MONITOR_THREAD_SLEEP_TIME,
    NPS_CFG_TYPE_DLB_L3_INTR_THREAD_PRI,
    NPS_CFG_TYPE_DLB_NVO3_MONITOR_THREAD_PRI,
    NPS_CFG_TYPE_DLB_NVO3_MONITOR_THREAD_SLEEP_TIME,
    NPS_CFG_TYPE_DLB_NVO3_INTR_THREAD_PRI,

    /* l3 related configuration */
    NPS_CFG_TYPE_L3_ECMP_MIN_BLOCK_SIZE,
    NPS_CFG_TYPE_L3_ECMP_BLOCK_SIZE,
    NPS_CFG_TYPE_TCAM_L3_WITH_IPV6_PREFIX_128_REGION_ENTRY_NUM,
    NPS_CFG_TYPE_TCAM_L3_WITH_IPV6_PREFIX_64_REGION_ENTRY_NUM,

    /* share memory related configuration */
    NPS_CFG_TYPE_HASH_L2_FDB_REGION_ENTRY_NUM,
    NPS_CFG_TYPE_HASH_L2_GROUP_REGION_ENTRY_NUM,
    NPS_CFG_TYPE_HASH_SECURITY_REGION_ENTRY_NUM,
    NPS_CFG_TYPE_HASH_L3_WITH_IPV6_PREFIX_128_REGION_ENTRY_NUM,
    NPS_CFG_TYPE_HASH_L3_WITH_IPV6_PREFIX_64_REGION_ENTRY_NUM,
    NPS_CFG_TYPE_HASH_L3_WITHOUT_PREFIX_REGION_ENTRY_NUM,
    NPS_CFG_TYPE_HASH_L3_RPF_REGION_ENTRY_NUM,
    NPS_CFG_TYPE_HASH_FLOW_REGION_ENTRY_NUM,

    NPS_CFG_TYPE_PORT_FC_MODE,                 /* only use to init port TM buffer
                                                * configuration for specific FC mode,
                                                * which not enable/disable FC/PFC
                                                * for the port/pcp.
                                                * param0: port.
                                                * param1: Invalid.
                                                * value : 0, FC disable;
                                                *         1, 802.3x FC;
                                                *         2, PFC.
                                                */
    NPS_CFG_TYPE_PORT_PFC_STATE,               /* valid while NPS_CFG_TYPE_PORT_TYPE_FC_MODE
                                                * of the port is PFC.
                                                * param0: port.
                                                * param1: pcp.
                                                * value : 0, PFC disable;
                                                *         1, PFC enable.
                                                */
    NPS_CFG_TYPE_PORT_PFC_QUEUE_STATE,         /* valid while NPS_CFG_TYPE_PORT_TYPE_FC_MODE
                                                * of the port is PFC.
                                                * param0: port.
                                                * param1: queue.
                                                * value : 0, PFC disable;
                                                *         1, PFC enable;
                                                */
    NPS_CFG_TYPE_PORT_PFC_MAPPING,             /* valid while NPS_CFG_TYPE_PORT_FC_MODE
                                                * of the port is PFC.
                                                * param0: port.
                                                * param1: queue.
                                                * value : PCP bitmap;
                                                *
                                                */
    NPS_CFG_TYPE_TRILL_ENABLE,                 /* TRILL module related configuration */
    NPS_CFG_TYPE_USE_UNIT_PORT,                /* use UNIT_PORT or native port of NPS_PORT_T
                                                * 1 : UNIT_PORT, 0 : native port
                                                */
    NPS_CFG_TYPE_MAC_VLAN_ENABLE,              /* use dadicate mac vlan table */
    NPS_CFG_TYPE_CPI_PORT_MODE,                /* use to init CPI port working mode.
                                                * param0: CPI port number.
                                                * param1: NA.
                                                * value : 0, CPI mode.
                                                *         1, Ether mode.
                                                */
    NPS_CFG_TYPE_PHY_ADDR,
    NPS_CFG_TYPE_LED_CFG,
    NPS_CFG_TYPE_USER_BUF_CTRL,
    NPS_CFG_TYPE_ARIES_SDP_MODE,               /* Select which Aries parser to use
                                                * value:  0, GTP (default)
                                                *         1, PPPOE
                                                *         2, TCP_SPLICING
                                                */
    NPS_CFG_TYPE_FAIR_BUF_CTRL,                /* to enable the fairness in flow-control traffic.
                                                * value : 0, disable fairness.
                                                *         1, enable fairness.
                                                */
    NPS_CFG_TYPE_HRM_BUF_SIZE,                 /* to assign the head room size of port speed.
                                                * param0: Port speed.
                                                *         0, 1G (default)
                                                *         1, 10G
                                                *         2, 25G
                                                *         3, 40G
                                                *         4, 50G
                                                *         5, 100G
                                                * value : cell number.
                                                */
    NPS_CFG_TYPE_STEERING_TRUNCATE_ENABLE,     /* set value 0: Do not truncate steering packets.
                                                * set value 1: steering packets will be trucated to 1 cell and
                                                * the cell size is based on chip.
                                                */
    NPS_CFG_TYPE_FABRIC_MODE_ENABLE,           /* set value 0: Non-farbic chip mode. (default)
                                                * set value 1: Fabric chip mode.
                                                */
    NPS_CFG_TYPE_ACL_TCP_FLAGS_ENCODE_ENABLE,  /* set value 0: Do not encode tcp flags at acl entry.
                                                *              (Can only match bit 0-6 of tcp flags.)
                                                * set value 1: Encode tcp flags at acl entry. (default)
                                                */
    NPS_CFG_TYPE_TCAM_ECC_SCAN_ENABLE,         /* set value 0: Disable ECC TCAM scanning. (default)
                                                * set value 1: Enable  ECC TCAM scanning.
                                                */
    NPS_CFG_TYPE_PORT_BUF_MAX,                 /*
                                                * Port max buffer threshold and unit is cell count.
                                                * param0: port.
                                                * param1: 0, ingress;
                                                *         1, egress.
                                                * value : 0, disable;
                                                *         others, enable max threshold.
                                                */
    NPS_CFG_TYPE_INGRESS_DYNAMIC_BUF,          /*
                                                * Queue dynamic alpha setting and value will be
                                                * enlarge to multiple of 256. For example, set value
                                                * as 16 to indicate alpha as 1/16. Set value
                                                * as 256 to indicate alpha as 1.
                                                * param0: port.
                                                * param1: queue (0~7: sc).
                                                * value : alpha * 256.
                                                */
    NPS_CFG_TYPE_EGRESS_DYNAMIC_BUF,           /*
                                                * Queue dynamic alpha setting and value will be
                                                * enlarge to multiple of 256. For example, set value
                                                * as 16 to indicate alpha as 1/16. Set value
                                                * as 256 to indicate alpha as 1.
                                                * param0: port.
                                                * param1: queue (0~7: uc, 8~15: mc).
                                                * value : alpha * 256.
                                                */


    NPS_CFG_TYPE_DCQCN_ENABLE,                 /* set value 0: Disable DCQCN. (default)
                                                * set value 1: Enable  DCQCN.
                                                */


    NPS_CFG_TYPE_LAST

}NPS_CFG_TYPE_T;

typedef struct NPS_CFG_VALUE_S
{
    UI32_T  param0;             /*(Optional) The optional parameter which is available
                                 * when the NPS_CFG_TYPE_T needs the first arguments*/
    UI32_T  param1;             /*(Optional) The optional parameter which is available
                                 * when the NPS_CFG_TYPE_T needs the second arguments*/
    I32_T   value;

}NPS_CFG_VALUE_T;

typedef NPS_ERROR_NO_T
    (*NPS_CFG_GET_FUNC_T)(
    const UI32_T            unit,
    const NPS_CFG_TYPE_T    cfg_type,
    NPS_CFG_VALUE_T         *ptr_cfg_value);

typedef NPS_ERROR_NO_T
    (*NPS_CFG_GET_LED_FUNC_T)
(
    const UI32_T            unit,
    UI32_T                  **pptr_led_cfg,
    UI32_T                  *ptr_cfg_size);

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */

/* FUNCTION NAME:   nps_cfg_register
 * PURPOSE:
 *      The function is to register NPS_CFG_GET_FUNC to SDK.
 *
 * INPUT:
 *      unit -- Device unit number.
 *      ptr_cfg_callback -- function to get the configuration value.
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *      NPS_E_OK -- Operate success.
 *      NPS_E_BAD_PARAMETER -- Bad parameter.
 *
 * NOTES:
 *      1.  During SDK initializtion, it will call registered NPS_CFG_GET_FUNC to get configuration
 *          and apply them.
 *          If No registered NPS_CFG_GET_FUNC or can not get specified NPS_CFG_TYPE_T
 *          configuration, SDK will apply default setting.
 *      2.  This function should be called before calling nps_init
 */
NPS_ERROR_NO_T
nps_cfg_register(
    const UI32_T            unit,
    NPS_CFG_GET_FUNC_T      ptr_cfg_callback);

/* FUNCTION NAME:   nps_cfg_led_register
 * PURPOSE:
 *      The function is to register NPS_CFG_GET_FUNC to SDK.
 *
 * INPUT:
 *      unit -- Device unit number.
 *      ptr_led_cfg_callback -- function to get LED configuration array.
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *      NPS_E_OK -- Operate success.
 *      NPS_E_BAD_PARAMETER -- Bad parameter.
 *
 * NOTES:
 *      1.  During SDK initializtion, it will call registered NPS_CFG_GET_FUNC to get configuration
 *          and apply them.
 *          If No registered NPS_CFG_GET_LED_FUNC or can not get specified external LED cfg
 *          configuration, SDK will apply default setting.
 *      2.  This function should be called before calling nps_init
 */
NPS_ERROR_NO_T
nps_cfg_led_register(
    const UI32_T            unit,
    NPS_CFG_GET_LED_FUNC_T  ptr_led_cfg_callback);

#endif  /* NPS_CFG_H */
