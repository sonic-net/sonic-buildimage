/*******************************************************************************
 *  Copyright Statement:
 *  --------------------
 *  This software and the information contained therein are protected by
 *  copyright and other intellectual property laws and terms herein is
 *  confidential. The software may not be copied and the information
 *  contained herein may not be used or disclosed except with the written
 *  permission of Clounix (Shanghai) Technology Limited. (C) 2020-2023
 *
 *  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("CLOUNIX SOFTWARE")
 *  RECEIVED FROM CLOUNIX AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
 *  AN "AS-IS" BASIS ONLY. CLOUNIX EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 *  NEITHER DOES CLOUNIX PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 *  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 *  SUPPLIED WITH THE CLOUNIX SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
 *  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. CLOUNIX SHALL ALSO
 *  NOT BE RESPONSIBLE FOR ANY CLOUNIX SOFTWARE RELEASES MADE TO BUYER'S
 *  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *  BUYER'S SOLE AND EXCLUSIVE REMEDY AND CLOUNIX'S ENTIRE AND CUMULATIVE
 *  LIABILITY WITH RESPECT TO THE CLOUNIX SOFTWARE RELEASED HEREUNDER WILL BE,
 *  AT CLOUNIX'S OPTION, TO REVISE OR REPLACE THE CLOUNIX SOFTWARE AT ISSUE,
 *  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
 *  CLOUNIX FOR SUCH CLOUNIX SOFTWARE AT ISSUE.
 *
 *  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
 *  WITH THE LAWS OF THE PEOPLE'S REPUBLIC OF CHINA, EXCLUDING ITS CONFLICT OF
 *  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
 *  RELATED THERETO SHALL BE SETTLED BY LAWSUIT IN SHANGHAI,CHINA UNDER.
 *
 *******************************************************************************/

/* FILE NAME:   clx_netif.h
 * PURPOSE:
 *      Define the NET interface APIs in CLX SDK.
 * NOTES:
 */
#ifndef CLX_NETIF_H
#define CLX_NETIF_H

#include <clx_error.h>
#include <clx_types.h>
#include <clx_pkt.h>

#define CLX_NETIF_NAME_LEN            (32)
#define CLX_NETLINK_NAME_LEN          (16)
#define CLX_NETIF_PROFILE_NUM_MAX     (256)
#define CLX_NETIF_PROFILE_PATTERN_NUM (4)
#define CLX_NETIF_PROFILE_PATTERN_LEN (8)

/* ----------------------------------------------------------------------------------- struct */
typedef struct {
    UI32_T tx_pkt;
    UI32_T tx_queue_full;
    UI32_T tx_error;
    UI32_T rx_pkt;

} CLX_NETIF_INTF_CNT_T;

typedef struct {
    /* unique key */
    UI32_T id;
    C8_T name[CLX_NETIF_NAME_LEN];
    CLX_PORT_T port; /* only support unit port and local port */

    /* metadata */
    CLX_MAC_T mac;

#define CLX_NETIF_INTF_FLAGS_MAC           (1UL << 0)
#define CLX_NETIF_INTF_FLAGS_VLAN_TAG_TYPE (1UL << 1)
#define CLX_NETIF_INTF_FLAGS_VLAN_TAG      (1UL << 2)
    UI32_T flags;

#define CLX_NETIF_INTF_FLAGS_VLAN_TAG_STRIP    (0)
#define CLX_NETIF_INTF_FLAGS_VLAN_TAG_KEEP     (1)
#define CLX_NETIF_INTF_FLAGS_VLAN_TAG_ORIGINAL (2)
    UI8_T vlan_tag_type; /* 0:VLAN_TAG_STRIP 1:VLAN_TAG_KEEP 2:VLAN_TAG_ORIGINAL*/
    UI32_T vlan_tag;     /* lightning support */

} CLX_NETIF_INTF_T;

typedef struct {
    C8_T name[CLX_NETLINK_NAME_LEN];
    C8_T mc_group_name[CLX_NETLINK_NAME_LEN];
} CLX_NETIF_RX_DST_NETLINK_T;

typedef enum {
    CLX_NETIF_RX_DST_SDK = 0,
    CLX_NETIF_RX_DST_NETLINK,
    CLX_NETIF_RX_DST_LAST
} CLX_NETIF_RX_DST_TYPE_T;

typedef struct {
    /* unique key */
    UI32_T id;
    C8_T name[CLX_NETIF_NAME_LEN];
    UI32_T priority;

    /* match fields */
    CLX_PORT_T port; /* only support unit port and local port */
    CLX_PKT_RX_REASON_BITMAP_T reason_bitmap;
    UI8_T pattern[CLX_NETIF_PROFILE_PATTERN_NUM][CLX_NETIF_PROFILE_PATTERN_LEN];
    UI8_T mask[CLX_NETIF_PROFILE_PATTERN_NUM][CLX_NETIF_PROFILE_PATTERN_LEN];
    UI32_T offset[CLX_NETIF_PROFILE_PATTERN_NUM];

    /* for each flag 1:must hit, 0:don't care */
#define CLX_NETIF_PROFILE_FLAGS_PORT      (1UL << 0)
#define CLX_NETIF_PROFILE_FLAGS_REASON    (1UL << 1)
#define CLX_NETIF_PROFILE_FLAGS_PATTERN_0 (1UL << 2)
#define CLX_NETIF_PROFILE_FLAGS_PATTERN_1 (1UL << 3)
#define CLX_NETIF_PROFILE_FLAGS_PATTERN_2 (1UL << 4)
#define CLX_NETIF_PROFILE_FLAGS_PATTERN_3 (1UL << 5)
    UI32_T flags;

    CLX_NETIF_RX_DST_TYPE_T dst_type;
    CLX_NETIF_RX_DST_NETLINK_T netlink;

} CLX_NETIF_PROFILE_T;

/* ----------------------------------------------------------------------------------- APIs */
/**
 * @brief To create the Network Interface for Linux TCP/IP stack.
 *
 * support_chip all
 *
 * @param [in]     unit            - Device unit number
 * @param [in]     ptr_net_intf    - Pointer of the Network Interface
 * @param [out]    ptr_intf_id     - Pointer of the Network Interface ID
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_netif_createIntf(const UI32_T unit, CLX_NETIF_INTF_T *ptr_net_intf, UI32_T *ptr_intf_id);

/**
 * @brief To destroy the Network Interface for Linux TCP/IP stack.
 *
 * support_chip all
 *
 * @param [in]     unit       - Device unit number
 * @param [in]     intf_id    - The Network Interface ID
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_netif_destroyIntf(const UI32_T unit, const UI32_T intf_id);

/**
 * @brief To get the Network Interface for Linux TCP/IP stack.
 *
 * support_chip all
 *
 * @param [in]     unit            - Device unit number
 * @param [in]     intf_id         - The Network Interface ID
 * @param [out]    ptr_net_intf    - Pointer of the Network Interface
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_netif_getIntf(const UI32_T unit, const UI32_T intf_id, CLX_NETIF_INTF_T *ptr_net_intf);

/**
 * @brief To get the Network Profile counter for Linux TCP/IP stack.
 *
 * support_chip all
 *
 * @param [in]     unit             - Device unit number
 * @param [in]     intf_id          - The Network Interface ID
 * @param [out]    ptr_netif_cnt    - Pointer of the Network Interface counter
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_netif_getIntfCnt(const UI32_T unit, const UI32_T intf_id, CLX_NETIF_INTF_CNT_T *ptr_netif_cnt);

/**
 * @brief To clear the Network Profile counter for Linux TCP/IP stack.
 *
 * support_chip all
 *
 * @param [in]     unit       - Device unit number
 * @param [in]     intf_id    - The Network Interface ID
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_netif_clearIntfCnt(const UI32_T unit, const UI32_T intf_id);

/**
 * @brief To create the Network Profile for Rx packets to User Process.
 *
 * support_chip all
 *
 * @param [in]     unit               - Device unit number
 * @param [in]     ptr_net_profile    - Pointer of the Network Profile
 * @param [out]    ptr_profile_id     - Pointer of the Network Profile ID
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_netif_createProfile(const UI32_T unit,
                        CLX_NETIF_PROFILE_T *ptr_net_profile,
                        UI32_T *ptr_profile_id);

/**
 * @brief To destroy the Network Profile for Rx packets to User Process.
 *
 * support_chip all
 *
 * @param [in]     unit          - Device unit number
 * @param [in]     profile_id    - The Network Profile ID
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_netif_destroyProfile(const UI32_T unit, const UI32_T profile_id);

typedef enum {
    CLX_NETIF_INTF_PROPERTY_IGR_SAMPLING_RATE,
    CLX_NETIF_INTF_PROPERTY_EGR_SAMPLING_RATE,
    CLX_NETIF_INTF_PROPERTY_SKIP_PORT_STATE_EVENT,
    CLX_NETIF_INTF_PROPERTY_ADMIN_STATE,
    CLX_NETIF_INTF_PROPERTY_PDMA_RX_CNT,
    CLX_NETIF_INTF_PROPERTY_RX_RCH_STATUS,
    CLX_NETIF_INTF_PROPERTY_SET_SWITCH_ID,
    CLX_NETIF_INTF_PROPERTY_VLAN_TAG_TYPE,
    CLX_NETIF_INTF_PROPERTY_VLAN_TAG,
    CLX_NETIF_INTF_PROPERTY_LAST
} CLX_NETIF_INTF_PROPERTY_T;

/**
 * @brief To set Netif property.
 *
 * support_chip all
 *
 * @param [in]     unit        - Device unit number
 * @param [in]     intf_id     - Network Interface ID
 * @param [in]     property    - Property type
 * @param [in]     param0      - First parameter
 * @param [in]     param1      - Second parameter
 * @return         CLX_E_OK               - Operation success
 * @return         CLX_E_BAD_PARAMETER    - Bad parameter
 */
CLX_ERROR_NO_T
clx_netif_setIntfProperty(const UI32_T unit,
                          const UI32_T intf_id,
                          const CLX_NETIF_INTF_PROPERTY_T property,
                          const UI32_T param0,
                          const UI32_T param1);

/**
 * @brief To get Netif property.
 *
 * support_chip all
 *
 * @param [in]     unit        - Device unit number
 * @param [in]     intf_id     - Network Interface ID
 * @param [in]     property    - Property type
 * @param [in]     ptr_param0  - First parameter
 * @param [in]     ptr_param1  - Second parameter
 * @return         CLX_E_OK               - Operation success
 * @return         CLX_E_BAD_PARAMETER    - Bad parameter
 */
CLX_ERROR_NO_T
clx_netif_getIntfProperty(const UI32_T unit,
                          const UI32_T intf_id,
                          const CLX_NETIF_INTF_PROPERTY_T property,
                          UI32_T *ptr_param0,
                          UI32_T *ptr_param1);

#define CLX_NETIF_NETLINK_NUM_MAX          (256)
#define CLX_NETIF_NETLINK_MC_GROUP_NUM_MAX (32)

typedef struct {
    C8_T name[CLX_NETLINK_NAME_LEN];

} CLX_NETIF_NETLINK_MC_GROUP_T;

typedef struct {
    UI32_T id;
    C8_T name[CLX_NETLINK_NAME_LEN];
    CLX_NETIF_NETLINK_MC_GROUP_T mc_group[CLX_NETIF_NETLINK_MC_GROUP_NUM_MAX];
    UI32_T mc_group_num;

} CLX_NETIF_NETLINK_T;

/**
 * @brief To create Netlink.
 *
 * support_chip all
 *
 * @param [in]     unit              - Device unit number
 * @param [in]     ptr_netlink       - Pointer of the Netlink
 * @param [out]    ptr_netlink_id    - Pointer of the Netlink ID
 * @return         CLX_E_OK               - Operation success
 * @return         CLX_E_BAD_PARAMETER    - Bad parameter
 */
CLX_ERROR_NO_T
clx_netif_createNetlink(const UI32_T unit,
                        CLX_NETIF_NETLINK_T *ptr_netlink,
                        UI32_T *ptr_netlink_id);

/**
 * @brief To destroy Netlink.
 *
 * support_chip all
 *
 * @param [in]     unit          - Device unit number
 * @param [in]     netlink_id    - The Netlink ID
 * @return         CLX_E_OK               - Operation success
 * @return         CLX_E_BAD_PARAMETER    - Bad parameter
 */
CLX_ERROR_NO_T
clx_netif_destroyNetlink(const UI32_T unit, const UI32_T netlink_id);

/**
 * @brief To get the Netlink.
 *
 * support_chip all
 *
 * @param [in]     unit           - Device unit number
 * @param [in]     netlink_id     - The Netlink ID
 * @param [out]    ptr_netlink    - Pointer of the Netlink
 * @return         CLX_E_OK               - Operation success
 * @return         CLX_E_BAD_PARAMETER    - Bad parameter
 */
CLX_ERROR_NO_T
clx_netif_getNetlink(const UI32_T unit, const UI32_T netlink_id, CLX_NETIF_NETLINK_T *ptr_netlink);

#endif /* end of CLX_NETIF_H */
