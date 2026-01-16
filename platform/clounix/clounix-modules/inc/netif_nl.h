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

/* FILE NAME:  netif_nl.h
 * PURPOSE:
 *      It provide xxx API.
 * NOTES:
 */

#ifndef NETIF_NL_H
#define NETIF_NL_H

#include <linux/skbuff.h>

#define NETIF_NL_NETLINK_MC_GROUP_NUM (32)
#define NETIF_NL_NETLINK_NAME_LEN     (16)

typedef enum {
    NETIF_NL_INTF_PROPERTY_IGR_SAMPLING_RATE,
    NETIF_NL_INTF_PROPERTY_EGR_SAMPLING_RATE,
    NETIF_NL_INTF_PROPERTY_LAST
} NETIF_NL_INTF_PROPERTY_T;

typedef enum {
    NETIF_NL_PKT_PSAMPLE_INVALID = 0,
    NETIF_NL_PKT_PSAMPLE_INGRESS,
    NETIF_NL_PKT_PSAMPLE_EGRESS
} NETIF_NL_PKT_PSAMPLE_DIR_T;

/* must be the same with CLX_NETIF_RX_DST_NETLINK_T */
typedef struct {
    C8_T name[NETIF_NL_NETLINK_NAME_LEN];
    C8_T mc_group_name[NETIF_NL_NETLINK_NAME_LEN];
} NETIF_NL_RX_DST_NETLINK_T;

typedef struct {
    UI16_T egr_intf_idx;
    UI16_T egr_intf_id;
    UI16_T psample_dir;
} NETIF_NL_RX_PKT_EXTRA_T;

typedef struct {
    NETIF_NL_RX_DST_NETLINK_T *nl;
    NETIF_NL_RX_PKT_EXTRA_T pkt;
} NETIF_NL_RX_COOKIES_T;

/* must be the same with CLX_NETIF_NETLINK_MC_GROUP_T */
typedef struct {
    C8_T name[NETIF_NL_NETLINK_NAME_LEN];

} NETIF_NL_NETLINK_MC_GROUP_T;

/* must be the same with CLX_NETIF_NETLINK_T */
typedef struct {
    UI32_T id;
    C8_T name[NETIF_NL_NETLINK_NAME_LEN];
    NETIF_NL_NETLINK_MC_GROUP_T mc_group[NETIF_NL_NETLINK_MC_GROUP_NUM];
    UI32_T mc_group_num;

} NETIF_NL_NETLINK_T;

CLX_ERROR_NO_T
netif_nl_rxSkb(const UI32_T unit, struct sk_buff *ptr_skb, void *ptr_cookie);

CLX_ERROR_NO_T
netif_nl_setIntfProperty(const UI32_T unit,
                         const UI32_T id,
                         const NETIF_NL_INTF_PROPERTY_T property,
                         const UI32_T param0,
                         const UI32_T param1);

CLX_ERROR_NO_T
netif_nl_getIntfProperty(const UI32_T unit,
                         const UI32_T port,
                         const NETIF_NL_INTF_PROPERTY_T property,
                         UI32_T *ptr_param0,
                         UI32_T *ptr_param1);

CLX_ERROR_NO_T
netif_nl_createNetlink(const UI32_T unit, NETIF_NL_NETLINK_T *ptr_netlink, UI32_T *ptr_netlink_id);

CLX_ERROR_NO_T
netif_nl_destroyNetlink(const UI32_T unit, const UI32_T group_id);

CLX_ERROR_NO_T
netif_nl_destroyAllNetlink(const UI32_T unit);

CLX_ERROR_NO_T
netif_nl_getNetlink(const UI32_T unit, const UI32_T netlink_id, NETIF_NL_NETLINK_T *ptr_netlink);

CLX_ERROR_NO_T
netif_nl_init(void);

#endif /* end of NETIF_NL_H */
