/*
 * Copyright 2022 Clounix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 */

/* FILE NAME:  hal_dflt.h
 * PURPOSE:
 *  1. Provide definition for sdk default
 *  2. Provide registration/deregistration
 *  3. Provide data structure declaration
 *  4. Provide
 *
 * NOTES:
 */

#ifndef HAL_DFLT_H
#define HAL_DFLT_H


/* INCLUDE FILE DECLARATIONS
 */
#include <clx_error.h>
#include <clx_types.h>


/* NAMING CONSTANT DECLARATIONS
 */

/* NAMING CONSTANT DECLARATIONS
   ----USER CONFIG (follow CLX_CFG_TYPE_T enum sequence)----
 */
/* SWC module */
#if defined(CLX_EN_SDN)
#define HAL_DFLT_CFG_CHIP_MODE                                      (1)
    /* chip operating mode. 0: legacy mode; 1:hybrid mode */
#else
#define HAL_DFLT_CFG_CHIP_MODE                                      (0)
    /* chip operating mode. 0: legacy mode; 1:hybrid mode */
#endif

/* PORT module related configuration */
#define HAL_DFLT_CFG_PORT_MAX_SPEED                                 (0)
    /* port max speed is not applied */
#define HAL_DFLT_CFG_PORT_LANE_NUM                                  (0)
    /* port lane num is not applied */
#define HAL_DFLT_CFG_PORT_TX_LANE                                   (0)
    /* port tx lane is not applied */
#define HAL_DFLT_CFG_PORT_RX_LANE                                   (0)
    /* port rx lane is not appied */
#define HAL_DFLT_CFG_PORT_TX_POLARITY_REV                           (0)
    /* port tx polarity rev is not applied */
#define HAL_DFLT_CFG_PORT_RX_POLARITY_REV                           (0)
    /* port rx polarity rev is not applied */
#define HAL_DFLT_CFG_PORT_EXT_LANE                                  (0)
    /* port ext lane is not applied */
#define HAL_DFLT_CFG_PORT_VALID                                     (0)
    /* port valid is not applied */

/* l2 module related configuration */
#define HAL_DFLT_CFG_L2_THREAD_PRI                                  (50)
#define HAL_DFLT_CFG_L2_THREAD_STACK                                (64 * 1024)
    /* customize L2 thread stack size in bytes */
#define HAL_DFLT_CFG_L2_ADDR_MODE                                   (0)
    /* L2 address operation mode. 0: Polling mode, 1: FIFO mode */
#define HAL_DFLT_CFG_L2_POLLING_INTERVAL                            (4000)
    /* Times (msec) to poll all L2 FDB entries */

/* PKT module related configuration */
#define HAL_DFLT_CFG_PKT_TX_GPD_NUM                                 (1024)
#define HAL_DFLT_CFG_PKT_RX_GPD_NUM                                 (1024)
#define HAL_DFLT_CFG_PKT_RX_SCHED_MODE                              (0)
    /* 0: RR mode, 1: WRR mode                      */
#define HAL_DFLT_CFG_PKT_TX_QUEUE_LEN                               (HAL_DFLT_CFG_PKT_TX_GPD_NUM * 10)
#define HAL_DFLT_CFG_PKT_RX_QUEUE_LEN                               (HAL_DFLT_CFG_PKT_RX_GPD_NUM * 10)
#define HAL_DFLT_CFG_PKT_RX_QUEUE_WEIGHT                            (0)
    /* valid while CLX_CFG_TYPE_PKT_RX_SCHED_MODE is 1
     * param0: queue
     * param1: NA
     * value : weight
     */
#define HAL_DFLT_CFG_PKT_RX_ISR_THREAD_PRI                          (80)
#define HAL_DFLT_CFG_PKT_RX_ISR_THREAD_STACK                        (64 * 1024)
    /* customize PKT RX ISR thread stack size in bytes */
#define HAL_DFLT_CFG_PKT_RX_FREE_THREAD_PRI                         (80)
#define HAL_DFLT_CFG_PKT_RX_FREE_THREAD_STACK                       (64 * 1024)
    /* customize PKT RX free thread stack size in bytes */
#define HAL_DFLT_CFG_PKT_TX_ISR_THREAD_PRI                          (80)
#define HAL_DFLT_CFG_PKT_TX_ISR_THREAD_STACK                        (64 * 1024)
    /* customize PKT TX ISR thread stack size in bytes */
#define HAL_DFLT_CFG_PKT_TX_FREE_THREAD_PRI                         (80)
#define HAL_DFLT_CFG_PKT_TX_FREE_THREAD_STACK                       (64 * 1024)
    /* customize PKT TX free thread stack size in bytes */
#define HAL_DFLT_CFG_PKT_ERROR_ISR_THREAD_PRI                       (80)
#define HAL_DFLT_CFG_PKT_ERROR_ISR_THREAD_STACK                     (64 * 1024)
    /* customize PKT ERROR ISR thread stack size in bytes */
#define HAL_DFLT_CFG_PKT_DMA_ENHANCE_ENABLE                         (0)


/* STAT module related configuration */
#define HAL_DFLT_CFG_CNT_THREAD_PRI                                 (0)
#define HAL_DFLT_CFG_CNT_THREAD_STACK                               (0)
    /* customize CNT thread stack size in bytes */

/* IFMON module related configuration */
#define HAL_DFLT_CFG_IFMON_THREAD_PRI                               (0)
#define HAL_DFLT_CFG_IFMON_THREAD_STACK                             (0)
    /* customize IFMON thread stack size in bytes */

/* share memory related configuration */
#define HAL_DFLT_CFG_SHARE_MEM_SDN_ENTRY_NUM                        (0)
    /* SDN flow table entry number from share memory */
#define HAL_DFLT_CFG_SHARE_MEM_L3_ENTRY_NUM                         (0)
    /* L3 entry number from share memory */
#define HAL_DFLT_CFG_SHARE_MEM_L2_ENTRY_NUM                         (0)
    /* L2 entry number from share memory */

/* DLB related configuration */
#define HAL_DFLT_CFG_DLB_MONITOR_MODE                               (0)
    /* DLB monitor mode. 1: async, 0: sync */
#define HAL_DFLT_CFG_DLB_LAG_MONITOR_THREAD_PRI                     (0)
#define HAL_DFLT_CFG_DLB_LAG_MONITOR_THREAD_SLEEP_TIME              (0)
#define HAL_DFLT_CFG_DLB_L3_MONITOR_THREAD_PRI                      (0)
#define HAL_DFLT_CFG_DLB_L3_MONITOR_THREAD_SLEEP_TIME               (0)
#define HAL_DFLT_CFG_DLB_L3_INTR_THREAD_PRI                         (0)
#define HAL_DFLT_CFG_DLB_NVO3_MONITOR_THREAD_PRI                    (0)
#define HAL_DFLT_CFG_DLB_NVO3_MONITOR_THREAD_SLEEP_TIME             (0)
#define HAL_DFLT_CFG_DLB_NVO3_INTR_THREAD_PRI                       (0)

/* l3 related configuration */
#define HAL_DFLT_CFG_L3_ECMP_MIN_BLOCK_SIZE                         (0)
#define HAL_DFLT_CFG_L3_ECMP_BLOCK_SIZE                             (0)
#define HAL_DFLT_CFG_TCAM_L3_WITH_IPV6_PREFIX_128_REGION_ENTRY_NUM  (0)
#define HAL_DFLT_CFG_TCAM_L3_WITH_IPV6_PREFIX_64_REGION_ENTRY_NUM   (0)

/* share memory related configuration */
#define HAL_DFLT_CFG_HASH_L2_FDB_REGION_ENTRY_NUM                   (0)
#define HAL_DFLT_CFG_HASH_L2_GROUP_REGION_ENTRY_NUM                 (0)
#define HAL_DFLT_CFG_HASH_SECURITY_REGION_ENTRY_NUM                 (0)
#define HAL_DFLT_CFG_HASH_L3_WITH_IPV6_PREFIX_128_REGION_ENTRY_NUM  (0)
#define HAL_DFLT_CFG_HASH_L3_WITH_IPV6_PREFIX_64_REGION_ENTRY_NUM   (0)
#define HAL_DFLT_CFG_HASH_L3_WITHOUT_PREFIX_REGION_ENTRY_NUM        (0)
#define HAL_DFLT_CFG_HASH_L3_RPF_REGION_ENTRY_NUM                   (0)
#define HAL_DFLT_CFG_HASH_FLOW_REGION_ENTRY_NUM                     (0)

#define HAL_DFLT_CFG_PORT_FC_MODE                                   (0)
    /* only use to init port TM buffer
     * configuration for specific FC mode,
     * which not enable/disable FC/PFC
     * for the port/pcp.
     * param0: port.
     * param1: Invalid.
     * value : 0, FC disable;
     *         1, 802.3x FC;
     *         2, PFC.
     */
#define HAL_DFLT_CFG_PORT_PFC_STATE                                 (0)
    /* valid while CLX_CFG_TYPE_PORT_TYPE_FC_MODE
     * of the port is PFC.
     * param0: port.
     * param1: pcp.
     * value : 0, PFC disable;
     *         1, PFC enable.
     */
#define HAL_DFLT_CFG_PORT_PFC_QUEUE_STATE                           (0)
    /* valid while CLX_CFG_TYPE_PORT_TYPE_FC_MODE
     * of the port is PFC.
     * param0: port.
     * param1: queue.
     * value : 0, PFC disable;
     *         1, PFC enable;
     */
#define HAL_DFLT_CFG_PORT_PFC_MAPPING                               (0)
    /* valid while CLX_CFG_TYPE_PORT_FC_MODE
     * of the port is PFC.
     * param0: port.
     * param1: queue.
     * value : PCP bitmap;
     *
     */
#define HAL_DFLT_CFG_TRILL_ENABLE                                   (0)
    /* TRILL module related configuration */
#define HAL_DFLT_CFG_USE_UNIT_PORT                                  (0)
    /* use UNIT_PORT or native port of CLX_PORT_T
     * 1 : UNIT_PORT, 0 : native port
     */
#define HAL_DFLT_CFG_MAC_VLAN_ENABLE                                (0)
    /* use dadicate mac vlan table */
#define HAL_DFLT_CFG_CPI_PORT_MODE                                  (0)
    /* use to init CPI port working mode.
     * param0: CPI port number.
     * param1: NA.
     * value : 0, CPI mode.
     *         1, Ether mode.
     */
#define HAL_DFLT_CFG_PHY_ADDR                                       (0)
#define HAL_DFLT_CFG_LED_CFG                                        (0)
#define HAL_DFLT_CFG_USER_BUF_CTRL                                  (0)
#define HAL_DFLT_CFG_FAIR_BUF_CTRL                                  (0)
    /* to enable the fairness in flow-control traffic.
     * value : 0, disable fairness.
     *         1, enable fairness.
     */
#define HAL_DFLT_CFG_HRM_BUF_SIZE                                   (0)
    /* to assign the head room size of port speed.
     * param0: Port speed.
     *         0, 1G (default)
     *         1, 10G
     *         2, 25G
     *         3, 40G
     *         4, 50G
     *         5, 100G
     * value : cell number.
     */
#define HAL_DFLT_CFG_STEERING_TRUNCATE_ENABLE                       (0)
    /* set value 0: Do not truncate steering packets.
     * set value 1: steering packets will be trucated to 1 cell and
     * the cell size is based on chip.
     */
#define HAL_DFLT_CFG_FABRIC_MODE_ENABLE                             (0)
    /* set value 0: Non-farbic chip mode. (default)
     * set value 1: Fabric chip mode.
     */
#define HAL_DFLT_CFG_ACL_TCP_FLAGS_ENCODE_ENABLE                    (1)
    /* set value 0: Do not encode tcp flags at acl entry.
     *              (Can only match bit 0-6 of tcp flags.)
     * set value 1: Encode tcp flags at acl entry. (default)
     */
#define HAL_DFLT_CFG_TCAM_ECC_SCAN_ENABLE                           (0)
    /* set value 0: Disable ECC TCAM scanning. (default)
     * set value 1: Enable  ECC TCAM scanning.
     */
#define HAL_DFLT_CFG_PORT_BUF_MAX                                   (0)
    /*
     * Port max buffer threshold and unit is cell count.
     * param0: port.
     * param1: 0, ingress;
     *         1, egress.
     * value : 0, disable;
     *         others, enable max threshold.
     */
#define HAL_DFLT_CFG_INGRESS_DYNAMIC_BUF                            (0)
    /*
     * Queue dynamic alpha setting and value will be
     * enlarge to multiple of 256. For example, set value
     * as 16 to indicate alpha as 1/16. Set value
     * as 256 to indicate alpha as 1.
     * param0: port.
     * param1: queue (0~7: sc).
     * value : alpha * 256.
     */
#define HAL_DFLT_CFG_EGRESS_DYNAMIC_BUF                             (0)
    /*
     * Queue dynamic alpha setting and value will be
     * enlarge to multiple of 256. For example, set value
     * as 16 to indicate alpha as 1/16. Set value
     * as 256 to indicate alpha as 1.
     * param0: port.
     * param1: queue (0~7: uc, 8~15: mc).
     * value : alpha * 256.
     */

#define HAL_DFLT_CFG_DCQCN_ENABLE                                   (0)
    /* set value 0: Disable DCQCN. (default)
     * set value 1: Enable  DCQCN.
     */

#define HAL_DFLT_CFG_QUEUE_GROUP_MAP                                (0)
    /* To map MC egress accounting from qeueu to group.
     * param0: queue id. (0~7).
     * value : group id.
     */

#define HAL_DFLT_CFG_PRI_GROUP_MAP                                  (0)
    /* To account egress group from egress queue.
     * PG setting is availabled when CLX_CFG_TYPE_USER_BUF_CTRL is enabled.
     * param0: queue id. (0~47 for PCIE, 0-7 for others).
     * param1: 0, Front port UC type.
               1, Front port MC type.
               2, Local CPU type.
               3, Remote CPU type.
               4, PCIE type.
               5, MIRROR type.
     * value : group id.
     */


#define HAL_DFLT_CFG_MPLS_SR_NUM                                    (0)
    /* MPLS Segment Routing
     * value: encapsulation number
    */
#define HAL_DFLT_CFG_L2_BYPASS_LAG_PRUNE_GROUP_NUM                  (0)
    /* default value: 0 */
#define HAL_DFLT_CFG_L3_BYPASS_LAG_PRUNE_GROUP_NUM                  (0)
    /* default value: 0 */

/* DTEL related configuration */
#define HAL_DFLT_CFG_DTEL_PROFILE_NUM                               (0)
    /* DTEL profile number, it share with sFlow */

/* l3 related configuration */
#define HAL_DFLT_CFG_TCAM_L3_SIP_ENABLE                             (0)
    /* default value: 0, to expand TCAM capacity */
#define HAL_DFLT_CFG_L3_ECMP_FDL_ENABLE                             (0)
    /* default value: 0, to reserve adj for FDL */

#define HAL_DFLT_CFG_HASH_L3_IPV4_PREFIX_LENGTH_ENTRY_NUM           (0)
    /* param0: prefix-length (1~32) */
    /* param1: 1: vrf, 0: global */
    /* value: entry number */

#define HAL_DFLT_CFG_HASH_L3_IPV6_PREFIX_LENGTH_ENTRY_NUM           (0)
    /* param0: prefix-length (1~128) */
    /* param1: 1: vrf, 0: global */
    /* value: entry number */

#define HAL_DFLT_CFG_ACL_DROP_REDIRECT_CPU_PKT                      (0)
    /* set value 0: acl drop_cpu action would not drop redirect to cpu pkt.
     *              (default)
     * set value 1: acl drop_cpu action would drop redirect to cpu pkt.
     */
#define HAL_DFLT_CFG_STACKING_CHIP_PORT_NUM                         (0)
    /* In stacking/chassis system, max used port num per device.
     * default value: hw default max port
     */
#define HAL_DFLT_CFG_BUF_SNAPSHOT_INTERVAL                          (0)

#define HAL_DFLT_CFG_LAG_MC_RESILIENT_ENABLE                        (0)
    /* set value 0: disable (default).
     * set value 1: enable.
     */


/* NAMING CONSTANT DECLARATIONS
   ----DEFAULT PROPERTY ----
 */
/* ---- VLAN ---- */
#define HAL_DFLT_VLAN           (1)
#define HAL_DFLT_BDID           (1)

/* ---- PORT ---- */
#define HAL_DFLT_L2_MTU         (1536)
#define HAL_DFLT_TPID_1ST       (0x8100)
#define HAL_DFLT_TPID_2ND       (0x0)




/* ---- IFMON ---- */

/* ---- L2 ---- */

/* ---- STP ---- */
#define HAL_DFLT_STP_ID         (0)

/* ---- LAG ---- */

/* ---- MIR ---- */

/* ---- L3 ---- */

/* ---- L3T ---- */

/* ---- QOS ---- */

/* ---- METER ---- */

/* ---- PKT ---- */

/* ---- ACL ---- */

/* ---- STAT ---- */

/* ---- SEC ---- */

/* ---- SFLOW ---- */

/* ---- TM ---- */

/* ---- VM ---- */

/* ---- FCOE ---- */

/* ---- NV ---- */

/* ---- SWC ---- */

/* ---- SDN ---- */

/* ---- MPLS ---- */

/* ---- TRILL ---- */

/* ---- SFC ---- */

/* ---- STK ---- */


/* MACRO FUNCTION DECLARATIONS
 */

#endif
