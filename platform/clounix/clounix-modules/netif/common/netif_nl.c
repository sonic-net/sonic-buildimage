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

/* FILE NAME:  netif_xxx.c
 * PURPOSE:
 *      It provide xxx API.
 * NOTES:
 */
#include <clx_error.h>
#include <clx_types.h>

#include <netif_osal.h>
#include <netif_perf.h>
#include <netif_nl.h>
#include <osal/osal_mdc.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <net/genetlink.h>

#define NETIF_NL_FAMILY_NUM_MAX (256)
#define NETIF_NL_INTF_NUM_MAX   (256)

#define NETIF_NL_GET_FAMILY_META(__idx__) &(_netif_nl_cb.fam_entry[__idx__].meta)
#define NETIF_NL_GET_INTF_IGR_SAMPLE_RATE(__inft_id__) \
    (_netif_nl_cb.intf_entry[__inft_id__].igr_sample_rate)
#define NETIF_NL_GET_INTF_EGR_SAMPLE_RATE(__inft_id__) \
    (_netif_nl_cb.intf_entry[__inft_id__].egr_sample_rate)

#define NETIF_NL_FAMILY_IS_PSAMPLE(__ptr_family__)                                             \
    (0 ==                                                                                      \
     strncmp(__ptr_family__->name, NETIF_NL_PSAMPLE_FAMILY_NAME, NETIF_NL_NETLINK_NAME_LEN)) ? \
        1 :                                                                                    \
        0

#define NETIF_NL_FAMILY_IS_MOD(__ptr_family__)                                                  \
    (0 == strncmp(__ptr_family__->name, NETIF_NL_MOD_FAMILY_NAME, NETIF_NL_NETLINK_NAME_LEN)) ? \
        1 :                                                                                     \
        0

/* porting part */
#define NETIF_NL_VER_NUM                     (1)
#define NETIF_NL_PSAMPLE_MAX_ATTR_NUM        (NETIF_NL_PSAMPLE_ATTR_LAST)
#define NETIF_NL_REGISTER_FAMILY(__family__) genl_register_family(__family__)

#define NETIF_NL_UNREGISTER_FAMILY(__family__) genl_unregister_family(__family__)
#define NETIF_NL_ALLOC_SKB(__len__)            genlmsg_new(__len__, GFP_ATOMIC)
#define NETIF_NL_FREE_SKB(__ptr_skb__)         nlmsg_free(__ptr_skb__)

#define NETIF_NL_SEND_PKT(__ptr_family__, __mcgrp_id__, __ptr_skb__)                         \
    genlmsg_multicast_netns(__ptr_family__, &init_net, __ptr_skb__, 0, /* pid, avoid loop */ \
                            __mcgrp_id__, GFP_ATOMIC)
#define NETIF_NL_SET_SKB_ATTR_HDR(__skb__, __family__, __hdr_len__, __cmd__) \
    genlmsg_put(__skb__, 0, 0, __family__, __hdr_len__, __cmd__)
#define NETIF_NL_END_SKB_ATTR_HDR(__skb__, __hdr__) genlmsg_end(__skb__, __hdr__)

#define NETIF_NL_SET_16_BIT_ATTR(__skb__, __attr__, __data__) \
    nla_put_u16(__skb__, __attr__, __data__)
#define NETIF_NL_SET_32_BIT_ATTR(__skb__, __attr__, __data__) \
    nla_put_u32(__skb__, __attr__, __data__)

/*
 * <----------- nla_total_size(payload) ------------->
 * +------------------+- - -+- - - - - - - - - +- - -+
 * | Attribute Header | Pad |     Payload      | Pad |
 * +------------------+- - -+- - - - - - - - - +- - -+
 *
 *
 * <-------- nla_attr_size(payload) ---------->
 * +------------------+- - -+- - - - - - - - - +- - -+
 * | Attribute Header | Pad |     Payload      | Pad |
 * +------------------+- - -+- - - - - - - - - +- - -+
 *
 */
/* total size = attr data size + attr header size */
#define NETIF_NL_GET_ATTR_TOTAL_SIZE(__data_size__) nla_total_size(__data_size__)
#define NETIF_NL_GET_ATTR_SIZE(__data_size__)       nla_attr_size(__data_size__) /* without padding */

/* psample's family and group parameter */
#define NETIF_NL_PSAMPLE_FAMILY_NAME        "psample"
#define NETIF_NL_PSAMPLE_MC_GROUP_NAME_DATA "packets"
#define NETIF_NL_PSAMPLE_MC_GROUP_NAME_CFG  "config"
#define NETIF_NL_PSAMPLE_MC_GROUP_NUM       (NETIF_NL_PSAMPLE_MC_GROUP_ID_LAST)
#define NETIF_NL_DEFAULT_MC_GROUP_NUM       (1)

#define NETIF_NL_MOD_FAMILY_NAME "mod"

#define NETIF_NL_PKT_LEN_MAX               (9216)
#define NETIF_NL_PSAMPLE_DFLT_USR_GROUP_ID (1)

typedef enum {
    NETIF_NL_PSAMPLE_MC_GROUP_ID_CONFIG = 0,
    NETIF_NL_PSAMPLE_MC_GROUP_ID_SAMPLE,
    NETIF_NL_PSAMPLE_MC_GROUP_ID_LAST,
} NETIF_NL_PSAMPLE_MC_GROUP_ID_T;

typedef enum {
    NETIF_NL_PSAMPLE_ATTR_IIFINDEX = 0,
    NETIF_NL_PSAMPLE_ATTR_OIFINDEX,
    NETIF_NL_PSAMPLE_ATTR_ORIGSIZE,
    NETIF_NL_PSAMPLE_ATTR_SAMPLE_GROUP,
    NETIF_NL_PSAMPLE_ATTR_GROUP_SEQ,
    NETIF_NL_PSAMPLE_ATTR_SAMPLE_RATE,
    NETIF_NL_PSAMPLE_ATTR_DATA,
    NETIF_NL_PSAMPLE_ATTR_LAST
} NETIF_NL_PSAMPLE_ATTR_ID_T;

typedef struct genl_multicast_group NETIF_NL_MC_GROUP_T;
typedef struct genl_family NETIF_NL_FAMILY_T;

static NETIF_NL_MC_GROUP_T _netif_nl_psample_mc_group[NETIF_NL_PSAMPLE_MC_GROUP_ID_LAST];
static C8_T *_ptr_netif_nl_psample_mc_group_name[NETIF_NL_PSAMPLE_MC_GROUP_ID_LAST] = {
    NETIF_NL_PSAMPLE_MC_GROUP_NAME_CFG, NETIF_NL_PSAMPLE_MC_GROUP_NAME_DATA};

static NETIF_NL_MC_GROUP_T _netif_nl_default_mc_group[NETIF_NL_DEFAULT_MC_GROUP_NUM];
static C8_T *_ptr_netif_nl_default_mc_group_name[NETIF_NL_DEFAULT_MC_GROUP_NUM] = {
    "default",
};

typedef struct {
    NETIF_NL_FAMILY_T meta;
    BOOL_T valid;

} NETIF_NL_FAMILY_ENTRY_T;

typedef struct {
    UI32_T igr_sample_rate;
    UI32_T egr_sample_rate;
    UI32_T trunc_size;
} NETIF_NL_INTF_ENTRY_T;

typedef struct {
    NETIF_NL_FAMILY_ENTRY_T fam_entry[NETIF_NL_FAMILY_NUM_MAX];
    NETIF_NL_INTF_ENTRY_T intf_entry[NETIF_NL_INTF_NUM_MAX]; /* sorted in intf_id */
    UI32_T seq_num;
} NETIF_NL_CB_T;

static NETIF_NL_CB_T _netif_nl_cb;

/* should extract to common */
struct net_device_priv {
    struct net_device *ptr_net_dev;
    struct net_device_stats stats;
    UI32_T unit;
    UI32_T id;
    UI32_T port;
    UI16_T vlan;
    UI32_T speed;
};

static CLX_ERROR_NO_T
_netif_nl_setIntfIgrSampleRate(const UI32_T unit, const UI32_T id, const UI32_T rate)
{
    NETIF_NL_CB_T *ptr_cb = &_netif_nl_cb;

    ptr_cb->intf_entry[id].igr_sample_rate = rate;

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_netif_nl_setIntfEgrSampleRate(const UI32_T unit, const UI32_T id, const UI32_T rate)
{
    NETIF_NL_CB_T *ptr_cb = &_netif_nl_cb;

    ptr_cb->intf_entry[id].egr_sample_rate = rate;

    return (CLX_E_OK);
}

CLX_ERROR_NO_T
netif_nl_setIntfProperty(const UI32_T unit,
                         const UI32_T id,
                         const NETIF_NL_INTF_PROPERTY_T property,
                         const UI32_T param0,
                         const UI32_T param1)
{
    CLX_ERROR_NO_T rc = CLX_E_BAD_PARAMETER;

    if (NETIF_NL_INTF_PROPERTY_IGR_SAMPLING_RATE == property) {
        OSAL_PRINT(OSAL_DBG_NETLINK,
                   "receive set igr sample rate req, id=%d, property=%d, param0=%d, param=%d\n", id,
                   property, param0, param1);
        rc = _netif_nl_setIntfIgrSampleRate(unit, id, param0);
    } else if (NETIF_NL_INTF_PROPERTY_EGR_SAMPLING_RATE == property) {
        OSAL_PRINT(OSAL_DBG_NETLINK,
                   "receive set egr sample rate req, id=%d, property=%d, param0=%d, param=%d\n", id,
                   property, param0, param1);
        rc = _netif_nl_setIntfEgrSampleRate(unit, id, param0);
    } else {
        OSAL_PRINT(OSAL_DBG_NETLINK, "[error] unknown property, property=%d\n", property);
    }

    return (rc);
}

static CLX_ERROR_NO_T
_netif_nl_getIntfIgrSampleRate(const UI32_T unit, const UI32_T id, UI32_T *ptr_rate)
{
    NETIF_NL_CB_T *ptr_cb = &_netif_nl_cb;

    *ptr_rate = ptr_cb->intf_entry[id].igr_sample_rate;

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_netif_nl_getIntfEgrSampleRate(const UI32_T unit, const UI32_T id, UI32_T *ptr_rate)
{
    NETIF_NL_CB_T *ptr_cb = &_netif_nl_cb;

    *ptr_rate = ptr_cb->intf_entry[id].egr_sample_rate;

    return (CLX_E_OK);
}

CLX_ERROR_NO_T
netif_nl_getIntfProperty(const UI32_T unit,
                         const UI32_T id,
                         const NETIF_NL_INTF_PROPERTY_T property,
                         UI32_T *ptr_param0,
                         UI32_T *ptr_param1)
{
    CLX_ERROR_NO_T rc = CLX_E_BAD_PARAMETER;

    if (NETIF_NL_INTF_PROPERTY_IGR_SAMPLING_RATE == property) {
        rc = _netif_nl_getIntfIgrSampleRate(unit, id, ptr_param0);
    } else if (NETIF_NL_INTF_PROPERTY_EGR_SAMPLING_RATE == property) {
        rc = _netif_nl_getIntfEgrSampleRate(unit, id, ptr_param0);
    } else {
        OSAL_PRINT(OSAL_DBG_NETLINK, "[error] unknown property, property=%d\n", property);
    }

    return (rc);
}

CLX_ERROR_NO_T
_netif_nl_allocNlFamilyEntry(NETIF_NL_CB_T *ptr_cb, UI32_T *ptr_index)
{
    UI32_T idx;
    CLX_ERROR_NO_T rc = CLX_E_TABLE_FULL;

    for (idx = 0; idx < NETIF_NL_FAMILY_NUM_MAX; idx++) {
        if (FALSE == ptr_cb->fam_entry[idx].valid) {
            *ptr_index = idx;
            ptr_cb->fam_entry[idx].valid = TRUE;
            rc = CLX_E_OK;
            break;
        }
    }

    return (rc);
}

void
_netif_nl_freeNlFamilyEntry(NETIF_NL_CB_T *ptr_cb, const UI32_T index)
{
    OSAL_PRINT(OSAL_DBG_NETLINK, "[DBG] free netlink family entry, idx=%d\n", index);
    ptr_cb->fam_entry[index].valid = FALSE;
}

CLX_ERROR_NO_T
_netif_nl_setNlMcgroupPsample(NETIF_NL_FAMILY_T *ptr_nl_family)
{
    NETIF_NL_MC_GROUP_T *ptr_nl_mc_group = _netif_nl_psample_mc_group;
    UI32_T idx;

    /* init the mc group and hook the group to family */
    osal_memset(ptr_nl_mc_group, 0x0,
                (NETIF_NL_PSAMPLE_MC_GROUP_NUM * sizeof(NETIF_NL_MC_GROUP_T)));

    for (idx = 0; idx < NETIF_NL_PSAMPLE_MC_GROUP_ID_LAST; idx++) {
        osal_memcpy(ptr_nl_mc_group[idx].name, _ptr_netif_nl_psample_mc_group_name[idx],
                    osal_strlen(_ptr_netif_nl_psample_mc_group_name[idx]));
    }
    ptr_nl_family->n_mcgrps = NETIF_NL_PSAMPLE_MC_GROUP_NUM;
    ptr_nl_family->mcgrps = ptr_nl_mc_group;

    return (CLX_E_OK);
}

CLX_ERROR_NO_T
_netif_nl_setNlMcgroupDefault(NETIF_NL_FAMILY_T *ptr_nl_family)
{
    NETIF_NL_MC_GROUP_T *ptr_nl_mc_group = _netif_nl_default_mc_group;
    UI32_T idx;

    /* init the mc group and hook the group to family */
    osal_memset(ptr_nl_mc_group, 0x0,
                (NETIF_NL_DEFAULT_MC_GROUP_NUM * sizeof(NETIF_NL_MC_GROUP_T)));

    for (idx = 0; idx < NETIF_NL_DEFAULT_MC_GROUP_NUM; idx++) {
        osal_memcpy(ptr_nl_mc_group[idx].name, _ptr_netif_nl_default_mc_group_name[idx],
                    osal_strlen(_ptr_netif_nl_default_mc_group_name[idx]));
    }
    ptr_nl_family->n_mcgrps = NETIF_NL_DEFAULT_MC_GROUP_NUM;
    ptr_nl_family->mcgrps = ptr_nl_mc_group;

    return (CLX_E_OK);
}

#define NETIF_NL_IS_FAMILY_ENTRY_VALID(__idx__) \
    (TRUE == _netif_nl_cb.fam_entry[__idx__].valid) ? (TRUE) : (FALSE)
CLX_ERROR_NO_T
netif_nl_createNetlink(const UI32_T unit, NETIF_NL_NETLINK_T *ptr_netlink, UI32_T *ptr_netlink_id)
{
    NETIF_NL_CB_T *ptr_cb = &_netif_nl_cb;
    UI32_T entry_id;
    NETIF_NL_FAMILY_T *ptr_nl_family;
    NETIF_NL_MC_GROUP_T *ptr_nl_mcgrp;
    UI32_T idx;
    int ret;
    CLX_ERROR_NO_T rc;

    rc = _netif_nl_allocNlFamilyEntry(ptr_cb, &entry_id);
    if (CLX_E_OK == rc) {
        ptr_nl_family = NETIF_NL_GET_FAMILY_META(entry_id);

        /* fill in the meta data for that netlink family */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0)
        ptr_nl_family->id = GENL_ID_GENERATE; /* family id can be ignored since linux 4.10 */
#endif
        ptr_nl_family->version = NETIF_NL_VER_NUM;
        ptr_nl_family->maxattr = NETIF_NL_PSAMPLE_MAX_ATTR_NUM;
        ptr_nl_family->netnsok = true;
        osal_memcpy(ptr_nl_family->name, ptr_netlink->name, NETIF_NL_NETLINK_NAME_LEN);

        /* fill in the mc group info */
        ptr_nl_mcgrp = osal_alloc(sizeof(NETIF_NL_MC_GROUP_T) * ptr_netlink->mc_group_num);
        if (NULL != ptr_nl_mcgrp) {
            OSAL_PRINT(OSAL_DBG_NETLINK, "[DBG] create mc group:\n");
            for (idx = 0; idx < ptr_netlink->mc_group_num; idx++) {
                OSAL_PRINT(OSAL_DBG_NETLINK, "[DBG] - mcgrp%d: %s\n", idx,
                           ptr_netlink->mc_group[idx].name);
                osal_memcpy(ptr_nl_mcgrp[idx].name, ptr_netlink->mc_group[idx].name,
                            NETIF_NL_NETLINK_NAME_LEN);
            }
            ptr_nl_family->n_mcgrps = ptr_netlink->mc_group_num;
            ptr_nl_family->mcgrps = ptr_nl_mcgrp;

            /* register the family to kernel */
            ret = NETIF_NL_REGISTER_FAMILY(ptr_nl_family);
            if (0 == ret) {
                *ptr_netlink_id = entry_id;
                OSAL_PRINT(OSAL_DBG_NETLINK,
                           "[DBG] create netlink family, name=%s, entry_idx=%d, mcgrp_num=%d\n",
                           ptr_netlink->name, entry_id, ptr_nl_family->n_mcgrps);
                rc = CLX_E_OK;
            } else {
                OSAL_PRINT(OSAL_DBG_NETLINK,
                           "[DBG] register netlink family failed, name=%s, ret=%d\n",
                           ptr_netlink->name, ret);
                osal_free(ptr_nl_mcgrp);
                _netif_nl_freeNlFamilyEntry(ptr_cb, entry_id);
                rc = CLX_E_OTHERS;
            }
        } else {
            OSAL_PRINT(OSAL_DBG_NETLINK, "[DBG] alloc mcgrp failed\n");
            rc = CLX_E_NO_MEMORY;
        }
    }

    return (rc);
}

CLX_ERROR_NO_T
netif_nl_destroyNetlink(const UI32_T unit, const UI32_T netlink_id)
{
    NETIF_NL_CB_T *ptr_cb = &_netif_nl_cb;
    UI32_T entry_idx = netlink_id;
    NETIF_NL_FAMILY_T *ptr_nl_family;
    int ret;
    CLX_ERROR_NO_T rc;

    if (TRUE == NETIF_NL_IS_FAMILY_ENTRY_VALID(entry_idx)) {
        ptr_nl_family = NETIF_NL_GET_FAMILY_META(entry_idx);
        ret = NETIF_NL_UNREGISTER_FAMILY(ptr_nl_family);
        if (0 == ret) {
            osal_free(ptr_nl_family->mcgrps);
            _netif_nl_freeNlFamilyEntry(ptr_cb, entry_idx);
            rc = CLX_E_OK;
        } else {
            OSAL_PRINT(OSAL_DBG_NETLINK,
                       "[DBG] unregister netlink family failed, name=%s, ret=%d\n",
                       ptr_nl_family->name, ret);
            rc = CLX_E_OTHERS;
        }
    } else {
        OSAL_PRINT(OSAL_DBG_NETLINK, "[DBG] destroy netlink failed, invalid netlink_id %d\n",
                   netlink_id);
        rc = CLX_E_ENTRY_NOT_FOUND;
    }

    return (rc);
}

CLX_ERROR_NO_T
netif_nl_destroyAllNetlink(const UI32_T unit)
{
    NETIF_NL_CB_T *ptr_cb = &_netif_nl_cb;
    NETIF_NL_FAMILY_T *ptr_nl_family;
    int ret;
    CLX_ERROR_NO_T rc = CLX_E_OK;
    UI32_T idx;

    for (idx = 0; idx < NETIF_NL_FAMILY_NUM_MAX; idx++) {
        if (TRUE == NETIF_NL_IS_FAMILY_ENTRY_VALID(idx)) {
            ptr_nl_family = NETIF_NL_GET_FAMILY_META(idx);
            OSAL_PRINT(OSAL_DBG_NETLINK, "[DBG] unregister netlink family, name=%s\n",
                       ptr_nl_family->name);
            ret = NETIF_NL_UNREGISTER_FAMILY(ptr_nl_family);
            if (0 == ret) {
                osal_free(ptr_nl_family->mcgrps);
                _netif_nl_freeNlFamilyEntry(ptr_cb, idx);
                rc = CLX_E_OK;
            } else {
                OSAL_PRINT(OSAL_DBG_NETLINK,
                           "[DBG] unregister netlink family failed, name=%s, ret=%d\n",
                           ptr_nl_family->name, ret);
                rc = CLX_E_OTHERS;
            }
        }
    }
    OSAL_PRINT(OSAL_DBG_NETLINK | OSAL_DBG_DEBUG, "[DBG] destroy all netlink\n");

    return (rc);
}

CLX_ERROR_NO_T
netif_nl_getNetlink(const UI32_T unit, const UI32_T netlink_id, NETIF_NL_NETLINK_T *ptr_netlink)
{
    UI32_T entry_idx = netlink_id;
    NETIF_NL_FAMILY_T *ptr_meta;
    UI32_T grp_idx;
    CLX_ERROR_NO_T rc = CLX_E_OK;

    if (TRUE == NETIF_NL_IS_FAMILY_ENTRY_VALID(entry_idx)) {
        OSAL_PRINT(OSAL_DBG_NETLINK, "[DBG] get valid netlink, id=%d\n", netlink_id);

        ptr_netlink->id = netlink_id;
        ptr_meta = NETIF_NL_GET_FAMILY_META(entry_idx);

        ptr_netlink->mc_group_num = ptr_meta->n_mcgrps;
        osal_memcpy(ptr_netlink->name, ptr_meta->name, NETIF_NL_NETLINK_NAME_LEN);

        for (grp_idx = 0; grp_idx < ptr_meta->n_mcgrps; grp_idx++) {
            osal_memcpy(ptr_netlink->mc_group[grp_idx].name, ptr_meta->mcgrps[grp_idx].name,
                        NETIF_NL_NETLINK_NAME_LEN);
        }
    } else {
        OSAL_PRINT(OSAL_DBG_NETLINK, "[DBG] get netlink failed, invalid netlink_id %d\n",
                   netlink_id);
        rc = CLX_E_ENTRY_NOT_FOUND;
    }

    return (rc);
}

CLX_ERROR_NO_T
_netif_nl_getFamilyByName(NETIF_NL_CB_T *ptr_cb,
                          const C8_T *ptr_name,
                          NETIF_NL_FAMILY_T **pptr_nl_family)
{
    UI32_T idx;
    CLX_ERROR_NO_T rc = CLX_E_ENTRY_NOT_FOUND;

    for (idx = 0; idx < NETIF_NL_FAMILY_NUM_MAX; idx++) {
        if ((TRUE == ptr_cb->fam_entry[idx].valid) &&
            (0 == strncmp(ptr_cb->fam_entry[idx].meta.name, ptr_name, NETIF_NL_NETLINK_NAME_LEN))) {
            *pptr_nl_family = &(ptr_cb->fam_entry[idx].meta);
            rc = CLX_E_OK;
            break;
        }
    }

    if (CLX_E_ENTRY_NOT_FOUND == rc) {
        OSAL_PRINT(OSAL_DBG_NETLINK, "[DBG] find family failed, name=%s\n", ptr_name);
    }

    return (rc);
}

CLX_ERROR_NO_T
_netif_nl_getMcgrpIdByName(NETIF_NL_FAMILY_T *ptr_nl_family,
                           const C8_T *ptr_mcgrp_name,
                           UI32_T *ptr_mcgrp_id)
{
    UI32_T idx;
    CLX_ERROR_NO_T rc = CLX_E_ENTRY_NOT_FOUND;

    for (idx = 0; idx < ptr_nl_family->n_mcgrps; idx++) {
        if ((0 ==
             strncmp(ptr_nl_family->mcgrps[idx].name, ptr_mcgrp_name, NETIF_NL_NETLINK_NAME_LEN))) {
            *ptr_mcgrp_id = idx;
            rc = CLX_E_OK;
            break;
        }
    }

    if (CLX_E_OK != rc) {
        OSAL_PRINT(OSAL_DBG_NETLINK, "[DBG] find mcgrp %s failed in family %s\n", ptr_mcgrp_name,
                   ptr_nl_family->name);
    }

    return (rc);
}

UI32_T
_netif_nl_getDataLen(UI32_T msg_hdr_len, struct sk_buff *ptr_ori_skb)
{
    UI32_T data_len;
    data_len = NETIF_NL_GET_ATTR_TOTAL_SIZE(ptr_ori_skb->len);

    if ((msg_hdr_len + NETIF_NL_GET_ATTR_TOTAL_SIZE(ptr_ori_skb->len)) > NETIF_NL_PKT_LEN_MAX) {
        data_len = NETIF_NL_PKT_LEN_MAX - msg_hdr_len - NLA_HDRLEN - NLA_ALIGNTO;
    } else {
        data_len = ptr_ori_skb->len;
    }

    return data_len;
}

UI32_T
_netif_nl_getMsgHdrLen(NETIF_NL_FAMILY_T *ptr_nl_family)
{
    UI32_T msg_hdr_len = 0;

    if (NETIF_NL_FAMILY_IS_PSAMPLE(ptr_nl_family)) {
        msg_hdr_len = NETIF_NL_GET_ATTR_TOTAL_SIZE(sizeof(UI16_T)) + /* PSAMPLE_ATTR_IIFINDEX */
            NETIF_NL_GET_ATTR_TOTAL_SIZE(sizeof(UI16_T)) +           /* PSAMPLE_ATTR_OIFINDEX  */
            NETIF_NL_GET_ATTR_TOTAL_SIZE(sizeof(UI32_T)) +           /* PSAMPLE_ATTR_SAMPLE_RATE */
            NETIF_NL_GET_ATTR_TOTAL_SIZE(sizeof(UI32_T)) +           /* PSAMPLE_ATTR_ORIGSIZE */
            NETIF_NL_GET_ATTR_TOTAL_SIZE(sizeof(UI32_T)) +           /* PSAMPLE_ATTR_SAMPLE_GROUP */
            NETIF_NL_GET_ATTR_TOTAL_SIZE(sizeof(UI32_T));            /* PSAMPLE_ATTR_GROUP_SEQ */
    } else if (NETIF_NL_FAMILY_IS_MOD(ptr_nl_family)) {
        msg_hdr_len = 0;
    } else {
        OSAL_PRINT(OSAL_DBG_NETLINK, "[DBG] unknown netlink family\n");
        return -1;
    }

    return msg_hdr_len;
}

CLX_ERROR_NO_T
_netif_nl_allocNetlinkSkb(NETIF_NL_CB_T *ptr_cb,
                          NETIF_NL_FAMILY_T *ptr_nl_family,
                          struct sk_buff *ptr_ori_skb,
                          struct sk_buff **pptr_nl_skb)
{
    CLX_ERROR_NO_T rc = CLX_E_OK;
    UI32_T msg_hdr_len;
    UI32_T data_len;
    struct sk_buff *ptr_nl_skb;

    /* need to fill specific skb header format */
    msg_hdr_len = _netif_nl_getMsgHdrLen(ptr_nl_family);
    if (-1 == msg_hdr_len) {
        return CLX_E_OTHERS;
    }

    data_len = _netif_nl_getDataLen(msg_hdr_len, ptr_ori_skb);
    if (-1 == data_len) {
        return CLX_E_OTHERS;
    }

    ptr_nl_skb = NETIF_NL_ALLOC_SKB(NETIF_NL_GET_ATTR_TOTAL_SIZE(data_len) + msg_hdr_len);
    if (NULL == ptr_nl_skb) {
        OSAL_PRINT(OSAL_DBG_NETLINK, "[DBG] alloc netlink skb failed\n");
    }

    *pptr_nl_skb = ptr_nl_skb;
    return (rc);
}

CLX_ERROR_NO_T
_netif_nl_setPsampleNetlinkSkb(NETIF_NL_CB_T *ptr_cb,
                               NETIF_NL_FAMILY_T *ptr_nl_family,
                               struct sk_buff *ptr_ori_skb,
                               NETIF_NL_RX_COOKIES_T *ptr_cookies,
                               struct sk_buff *ptr_nl_skb)
{
    UI16_T igr_intf_idx;
    struct net_device_priv *ptr_priv;
    UI32_T rate;
    UI32_T intf_id;
    void *ptr_nl_hdr = NULL;
    struct nlattr *ptr_nl_attr;
    UI32_T msg_hdr_len;
    UI32_T data_len;
    CLX_ERROR_NO_T rc = CLX_E_OK;

    msg_hdr_len = _netif_nl_getMsgHdrLen(ptr_nl_family);
    if (-1 == msg_hdr_len) {
        return CLX_E_OTHERS;
    }

    data_len = _netif_nl_getDataLen(msg_hdr_len, ptr_ori_skb);
    if (-1 == data_len) {
        return CLX_E_OTHERS;
    }

    /* to create a netlink msg header (cmd=0) */
    ptr_nl_hdr = NETIF_NL_SET_SKB_ATTR_HDR(ptr_nl_skb, ptr_nl_family, 0, 0);
    if (NULL != ptr_nl_hdr) {
        /* obtain the intf index for the igr_port */
        igr_intf_idx = ptr_ori_skb->dev->ifindex;
        OSAL_PRINT(
            OSAL_DBG_NETLINK,
            "update ingress port %u and egress port %u for netlink egr intfid:%d psample_dir:%d",
            igr_intf_idx, ptr_cookies->pkt.egr_intf_idx, ptr_cookies->pkt.egr_intf_id,
            ptr_cookies->pkt.psample_dir);
        NETIF_NL_SET_16_BIT_ATTR(ptr_nl_skb, NETIF_NL_PSAMPLE_ATTR_IIFINDEX, (UI16_T)igr_intf_idx);
        NETIF_NL_SET_16_BIT_ATTR(ptr_nl_skb, NETIF_NL_PSAMPLE_ATTR_OIFINDEX,
                                 (UI16_T)ptr_cookies->pkt.egr_intf_idx);

        /* meta header */
        /* use the igr port id as the index for the database to get sample rate */
        ptr_priv = netdev_priv(ptr_ori_skb->dev);
        intf_id = ptr_priv->port;
        if (ptr_cookies->pkt.psample_dir == NETIF_NL_PKT_PSAMPLE_INGRESS) {
            rate = NETIF_NL_GET_INTF_IGR_SAMPLE_RATE(intf_id);
        } else {
            /* sample rate is anyone of port when egr_port is not valid */
            rate = NETIF_NL_GET_INTF_EGR_SAMPLE_RATE(ptr_cookies->pkt.egr_intf_id %
                                                     NETIF_NL_INTF_NUM_MAX);
        }
        NETIF_NL_SET_32_BIT_ATTR(ptr_nl_skb, NETIF_NL_PSAMPLE_ATTR_SAMPLE_RATE, rate);
        NETIF_NL_SET_32_BIT_ATTR(ptr_nl_skb, NETIF_NL_PSAMPLE_ATTR_ORIGSIZE, data_len);
        NETIF_NL_SET_32_BIT_ATTR(ptr_nl_skb, NETIF_NL_PSAMPLE_ATTR_SAMPLE_GROUP,
                                 ptr_cookies->pkt.psample_dir);
        NETIF_NL_SET_32_BIT_ATTR(ptr_nl_skb, NETIF_NL_PSAMPLE_ATTR_GROUP_SEQ, ptr_cb->seq_num);
        ptr_cb->seq_num++;

        /* data */
        ptr_nl_attr = (struct nlattr *)skb_put(ptr_nl_skb, NETIF_NL_GET_ATTR_TOTAL_SIZE(data_len));
        ptr_nl_attr->nla_type = NETIF_NL_PSAMPLE_ATTR_DATA;
        /* get the attr size without padding, since it's the last one */
        ptr_nl_attr->nla_len = NETIF_NL_GET_ATTR_SIZE(data_len);
        skb_copy_bits(ptr_ori_skb, 0, nla_data(ptr_nl_attr), data_len);

        NETIF_NL_END_SKB_ATTR_HDR(ptr_nl_skb, ptr_nl_hdr);
    } else {
        rc = CLX_E_OTHERS;
    }

    return (rc);
}

CLX_ERROR_NO_T
_netif_nl_setGenericNetlinkSkb(NETIF_NL_CB_T *ptr_cb,
                               NETIF_NL_FAMILY_T *ptr_nl_family,
                               struct sk_buff *ptr_ori_skb,
                               NETIF_NL_RX_COOKIES_T *ptr_cookies,
                               struct sk_buff *ptr_nl_skb)
{
    void *ptr_nl_hdr = NULL;
    struct nlattr *ptr_nl_attr;
    UI32_T msg_hdr_len;
    UI32_T data_len;
    CLX_ERROR_NO_T rc = CLX_E_OK;

    msg_hdr_len = _netif_nl_getMsgHdrLen(ptr_nl_family);
    if (-1 == msg_hdr_len) {
        return CLX_E_OTHERS;
    }

    data_len = _netif_nl_getDataLen(msg_hdr_len, ptr_ori_skb);
    if (-1 == data_len) {
        return CLX_E_OTHERS;
    }

    /* to create a netlink msg header (cmd=0) */
    ptr_nl_hdr = NETIF_NL_SET_SKB_ATTR_HDR(ptr_nl_skb, ptr_nl_family, 0, 0);
    if (NULL != ptr_nl_hdr) {
        /* data */
        ptr_nl_attr = (struct nlattr *)skb_put(ptr_nl_skb, NETIF_NL_GET_ATTR_TOTAL_SIZE(data_len));
        ptr_nl_attr->nla_type = NETIF_NL_PSAMPLE_ATTR_DATA;
        /* get the attr size without padding, since it's the last one */
        ptr_nl_attr->nla_len = NETIF_NL_GET_ATTR_SIZE(data_len);
        skb_copy_bits(ptr_ori_skb, 0, nla_data(ptr_nl_attr), data_len);

        NETIF_NL_END_SKB_ATTR_HDR(ptr_nl_skb, ptr_nl_hdr);
    } else {
        rc = CLX_E_OTHERS;
    }

    return (rc);
}

CLX_ERROR_NO_T
_netif_nl_setNetlinkSkb(NETIF_NL_CB_T *ptr_cb,
                        NETIF_NL_FAMILY_T *ptr_nl_family,
                        struct sk_buff *ptr_ori_skb,
                        NETIF_NL_RX_COOKIES_T *ptr_cookies,
                        struct sk_buff *ptr_nl_skb)
{
    CLX_ERROR_NO_T rc = CLX_E_OK;

    /* need to fill specific skb header format */
    if (NETIF_NL_FAMILY_IS_PSAMPLE(ptr_nl_family)) {
        rc = _netif_nl_setPsampleNetlinkSkb(ptr_cb, ptr_nl_family, ptr_ori_skb, ptr_cookies,
                                            ptr_nl_skb);
        if (CLX_E_OK != rc) {
            OSAL_PRINT(OSAL_DBG_NETLINK, "[DBG] set psample netlink skb failed\n");
        }
    } else if (NETIF_NL_FAMILY_IS_MOD(ptr_nl_family)) {
        rc = _netif_nl_setGenericNetlinkSkb(ptr_cb, ptr_nl_family, ptr_ori_skb, ptr_cookies,
                                            ptr_nl_skb);
        if (CLX_E_OK != rc) {
            OSAL_PRINT(OSAL_DBG_NETLINK, "[DBG] set generic netlink skb failed\n");
        }
    } else {
        OSAL_PRINT(OSAL_DBG_NETLINK, "[DBG] unknown netlink family\n");
        rc = CLX_E_OTHERS;
    }

    return (rc);
}

CLX_ERROR_NO_T
_netif_nl_sendNetlinkSkb(NETIF_NL_FAMILY_T *ptr_nl_family,
                         UI32_T nl_mcgrp_id,
                         struct sk_buff *ptr_nl_skb)
{
    int ret;
    CLX_ERROR_NO_T rc;

    ret = NETIF_NL_SEND_PKT(ptr_nl_family, nl_mcgrp_id, ptr_nl_skb);
    if (0 == ret) {
        rc = CLX_E_OK;
    } else {
        /* in errno_base.h, #define  ESRCH        3  : No such process */
        OSAL_PRINT(OSAL_DBG_NETLINK, "send skb to mc group failed, ret=%d\n", ret);
        rc = CLX_E_OTHERS;
    }

    return (rc);
}

void
_netif_nl_freeNetlinkSkb(struct sk_buff *ptr_nl_skb)
{
    OSAL_PRINT(OSAL_DBG_NETLINK, "[DBG] free nl skb\n");
    NETIF_NL_FREE_SKB(ptr_nl_skb);
}

CLX_ERROR_NO_T
_netif_nl_forwardPkt(NETIF_NL_CB_T *ptr_cb,
                     NETIF_NL_RX_COOKIES_T *ptr_cookies,
                     struct sk_buff *ptr_ori_skb)
{
    struct sk_buff *ptr_nl_skb = NULL;
    NETIF_NL_FAMILY_T *ptr_nl_family;
    UI32_T nl_mcgrp_id;
    CLX_ERROR_NO_T rc;

    rc = _netif_nl_getFamilyByName(ptr_cb, ptr_cookies->nl->name, &ptr_nl_family);
    if (CLX_E_OK != rc) {
        return rc;
    }

    rc = _netif_nl_getMcgrpIdByName(ptr_nl_family, ptr_cookies->nl->mc_group_name, &nl_mcgrp_id);
    if (CLX_E_OK != rc) {
        return rc;
    }

    rc = _netif_nl_allocNetlinkSkb(ptr_cb, ptr_nl_family, ptr_ori_skb, &ptr_nl_skb);
    if (CLX_E_OK != rc) {
        return rc;
    }

    rc = _netif_nl_setNetlinkSkb(ptr_cb, ptr_nl_family, ptr_ori_skb, ptr_cookies, ptr_nl_skb);
    if (CLX_E_OK != rc) {
        return rc;
    }

    rc = _netif_nl_sendNetlinkSkb(ptr_nl_family, nl_mcgrp_id, ptr_nl_skb);
    if (CLX_E_OK != rc) {
        return rc;
    }

    return (rc);
}

CLX_ERROR_NO_T
netif_nl_rxSkb(const UI32_T unit, struct sk_buff *ptr_skb, void *ptr_cookie)
{
    NETIF_NL_CB_T *ptr_cb = &_netif_nl_cb;
    CLX_ERROR_NO_T rc;
    NETIF_NL_RX_COOKIES_T *ptr_data = NULL;

    if (NULL == ptr_cookie) {
        return CLX_E_OTHERS;
    }

    ptr_data = (NETIF_NL_RX_COOKIES_T *)ptr_cookie;

    /* send the packet to netlink mcgroup */
    rc = _netif_nl_forwardPkt(ptr_cb, ptr_data, ptr_skb);

    /* need to free the original skb anyway */
    osal_skb_free(ptr_skb);

    return (rc);
}

CLX_ERROR_NO_T
netif_nl_init(void)
{
    osal_memset(&_netif_nl_cb, 0x0, sizeof(NETIF_NL_CB_T));

    return (CLX_E_OK);
}

CLX_ERROR_NO_T
netif_nl_deinit(void)
{
    return (CLX_E_OK);
}
