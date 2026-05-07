/*******************************************************************************
 *  Copyright Statement:
 *  --------------------
 *  This software and the information contained therein are protected by
 *  copyright and other intellectual property laws and terms herein is
 *  confidential. The software may not be copied and the information
 *  contained herein may not be used or disclosed except with the written
 *  permission of Clounix (Shanghai) Technology Limited. (C) 2020-2025
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

#include <net/genetlink.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include "knet_dev.h"
#include "knet_netif.h"

/* Generic netlink
 * +-----------------------------------------+
 * |         Netlink Header (nlmsghdr)       |  // 16B
 * |   (struct nlmsghdr)                     |
 * |   - nlmsg_len: msg total len            |
 * |   - nlmsg_type: (e.g., GENL_ID)         |
 * |   - nlmsg_flags: (e.g., NLM_F_ACK)      |
 * |   - nlmsg_seq: sequence                 |
 * |   - nlmsg_pid: PID                      |
 * +-----------------------------------------+
 * |   Generic Netlink Header (genlmsghdr)   |  // 4B
 * |   (struct genlmsghdr)                   |
 * |   - cmd: (e.g., CMD_READ)               |
 * |   - version: (e.g., 1)                  |
 * |   - reserved: padding                   |
 * +-----------------------------------------+
 * |   Netlink Attribute 1 (nlattr)          |  // flexible size
 * |   +----------------+----------------+   |
 * |   |  nla_type      |  nla_len       |   |
 * |   +----------------+----------------+   |
 * |   |  Attribute Data (N bytes)       |   |
 * |   +---------------------------------+   |
 * |   Size: nla_total_size(data_len)        |
 * +-----------------------------------------+
 * |   Netlink Attribute 2 (nlattr)          |
 * |   +----------------+----------------+   |
 * |   |  nla_type      |  nla_len       |   |
 * |   +----------------+----------------+   |
 * |   |  Attribute Data (N bytes)       |   |
 * |   +---------------------------------+   |
 * +-----------------------------------------+
 * |                   ...                   |
 * +-----------------------------------------+
 */

/* MOD netlink
 * +-----------------------------------------+
 * |         Netlink Header (nlmsghdr)       |  // 16B
 * |   (struct nlmsghdr)                     |
 * |   - nlmsg_len: msg total len            |
 * |   - nlmsg_type: (e.g., GENL_ID)         |
 * |   - nlmsg_flags: (e.g., NLM_F_ACK)      |
 * |   - nlmsg_seq: sequence                 |
 * |   - nlmsg_pid: PID                      |
 * +-----------------------------------------+
 * |   Generic Netlink Header (genlmsghdr)   |  // 4B
 * |   (struct genlmsghdr)                   |
 * |   - cmd: (e.g., CMD_READ)               |
 * |   - version: (e.g., 1)                  |
 * |   - reserved: padding                   |
 * +-----------------------------------------+
 *+-----------------------------------------+
 *|   Netlink Attribute 1 (IIFINDEX)        |  // NETIF_NL_ATTR_MOD_IGR_PORT
 *|   +----------------+----------------+   |
 *|   |  nlattr (4B)   |  uint32_t (4B) |   |
 *|   +----------------+----------------+   |
 *|   Size: nla_total_size(sizeof(uint32_t)) = 8B
 * +-----------------------------------------+
 *|   Netlink Attribute 2 (DATA)            |  //NETIF_NL_ATTR_MOD_DATA
 *|   +----------------+----------------+   |
 *|   |  nlattr (4B)   |  raw data (NB) |   |
 *|   +----------------+----------------+   |
 * +-----------------------------------------+
 * |                   ...                   |
 * +-----------------------------------------+
 */

/* Sflow netlink
 *+-----------------------------------------+
 *|        Generic Netlink Header           |  // genlmsghdr + nlmsghd
 *|   (genlmsg_total_size(0))               |  // type/version/cmd
 *|   Size: NLA_ALIGN(sizeof(struct genlmsghdr)) + NLMSG_HDRLEN
 *+-----------------------------------------+
 *|   Netlink Attribute 1 (IIFINDEX)        |  // NETIF_NL_ATTR_PSAMPLE_IIFINDEX
 *|   +----------------+----------------+   |
 *|   |  nlattr (4B)   |  uint16_t (2B) |   |
 *|   +----------------+----------------+   |
 *|   Size: nla_total_size(sizeof(uint16_t)) = 8B
 *+-----------------------------------------+
 *|   Netlink Attribute 2 (OIFINDEX)        |  // NETIF_NL_ATTR_PSAMPLE_OIFINDEX
 *|   +----------------+----------------+   |
 *|   |  nlattr (4B)   |  uint16_t (2B) |   |
 *|   +----------------+----------------+   |
 *|   Size: 8B
 *+-----------------------------------------+
 *|   Netlink Attribute 3 (ORIGSIZE)        |  // NETIF_NL_ATTR_PSAMPLE_ORIGSIZE
 *|   +----------------+----------------+   |
 *|   |  nlattr (4B)   |  uint32_t (4B) |   |
 *|   +----------------+----------------+   |
 *|   Size: nla_total_size(sizeof(uint32_t)) = 8B
 *+-----------------------------------------+
 *|   Netlink Attribute 4 (SAMPLE_GROUP)    |  // NETIF_NL_ATTR_PSAMPLE_SAMPLE_GROUP
 *|   +----------------+----------------+   |
 *|   |  nlattr (4B)   |  uint32_t (4B) |   |
 *|   +----------------+----------------+   |
 *|   Size: 8B
 *+-----------------------------------------+
 *|   Netlink Attribute 5 (GROUP_SEQ)       |  //NETIF_NL_ATTR_PSAMPLE_GROUP_SEQ
 *|   +----------------+----------------+   |
 *|   |  nlattr (4B)   |  uint32_t (4B) |   |
 *|   +----------------+----------------+   |
 *|   Size: 8B
 *+-----------------------------------------+
 *|   Netlink Attribute 6 (SAMPLE_RATE)     |  //NETIF_NL_ATTR_PSAMPLE_SAMPLE_RATE
 *|   +----------------+----------------+   |
 *|   |  nlattr (4B)   |  uint32_t (4B) |   |
 *|   +----------------+----------------+   |
 *|   Size: 8B
 *+-----------------------------------------+
 *|   Netlink Attribute 7 (DATA)            |  //NETIF_NL_ATTR_DATA
 *|   +----------------+----------------+   |
 *|   |  nlattr (4B)   |  raw data (NB) |   |
 *|   +----------------+----------------+   |
 *|   Size: nla_total_size(data_len) = 4 + data_len + padding
 *+-----------------------------------------+
 */

#define NETIF_NL_PKT_LEN_MAX (10200)

enum {
    /* for psample */
    NETIF_NL_ATTR_PSAMPLE_IIFINDEX = 0,
    NETIF_NL_ATTR_PSAMPLE_OIFINDEX,
    NETIF_NL_ATTR_PSAMPLE_ORIGSIZE,
    NETIF_NL_ATTR_PSAMPLE_SAMPLE_GROUP,
    NETIF_NL_ATTR_PSAMPLE_GROUP_SEQ,
    NETIF_NL_ATTR_PSAMPLE_SAMPLE_RATE,
    /* original data */
    NETIF_NL_ATTR_DATA,
    NETIF_NL_ATTR_LAST
};

/* for MOD */
enum {
    NETIF_NL_ATTR_MOD_IGR_PORT= 0,

    /* original data */
    NETIF_NL_ATTR_MOD_DATA,
    NETIF_NL_ATTR_MOD_LAST
};

struct netif_netlink {
    struct list_head list;
    struct genl_family family_entry;
    uint32_t id;
    uint32_t seq_num[NETIF_NL_PKT_PSAMPLE_MAX];
};

static unsigned char g_mod_default_mac[ETH_ALEN] = {0x70, 0x06, 0x92, 0x6D, 0x00, 0x01};

static int
netlink_get_header_len(uint32_t *msg_hdr_len, struct netlink_rx_cookie *ptr_cookies)
{
    if (ptr_cookies->netlink_type == NETLINK_RX_TYPE_SFLOW) {
        *msg_hdr_len = nla_total_size(sizeof(uint16_t)) + /* PSAMPLE_ATTR_IIFINDEX */
            nla_total_size(sizeof(uint16_t)) +            /* PSAMPLE_ATTR_OIFINDEX  */
            nla_total_size(sizeof(uint32_t)) +            /* PSAMPLE_ATTR_SAMPLE_RATE */
            nla_total_size(sizeof(uint32_t)) +            /* PSAMPLE_ATTR_ORIGSIZE */
            nla_total_size(sizeof(uint32_t)) +            /* PSAMPLE_ATTR_SAMPLE_GROUP */
            nla_total_size(sizeof(uint32_t));             /* PSAMPLE_ATTR_GROUP_SEQ */
    } else if (ptr_cookies->netlink_type == NETLINK_RX_TYPE_MOD) {
        *msg_hdr_len = nla_total_size(sizeof(uint32_t)); /* MOD_ATTR_IGR_PORT*/
    } else {
        *msg_hdr_len = 0;
    }
    return 0;
}

static int
netlink_get_data_len(struct sk_buff *ptr_ori_skb,
                     uint32_t *data_len,
                     struct netlink_rx_cookie *ptr_cookies)
{
    uint32_t msg_hdr_len;
    netlink_get_header_len(&msg_hdr_len, ptr_cookies);

    if ((msg_hdr_len + nla_total_size(ptr_ori_skb->len)) > NETIF_NL_PKT_LEN_MAX) {
        *data_len = NETIF_NL_PKT_LEN_MAX - msg_hdr_len - NLA_HDRLEN - NLA_ALIGNTO;
    } else {
        *data_len = ptr_ori_skb->len;
    }

    return 0;
}

static int
netlink_alloc_family_entry(uint32_t unit, uint32_t *ptr_index)
{
    *ptr_index =
        find_first_zero_bit(clx_netif_drv(unit)->netlink.netlink_id_bitmap, CLX_NETLINK_MAX_NUM);
    if (*ptr_index >= CLX_NETLINK_MAX_NUM) {
        return -ENOSPC;
    }
    set_bit(*ptr_index, clx_netif_drv(unit)->netlink.netlink_id_bitmap);
    return 0;
}

static void
netlink_free_family_entry(uint32_t unit, uint32_t index)
{
    clear_bit(index, clx_netif_drv(unit)->netlink.netlink_id_bitmap);
}

static int
netlink_set_mod_skb(struct netif_netlink *ptr_netlink,
                        struct sk_buff *ptr_ori_skb,
                        struct netlink_rx_cookie *ptr_cookies,
                        struct sk_buff *ptr_nl_skb)
{
    void *ptr_nl_hdr = NULL;
    uint32_t data_len;
    uint32_t igr_port_si = 0;
    int rc = 0;

    rc = netlink_get_data_len(ptr_ori_skb, &data_len, ptr_cookies);
    if (0 != rc) {
        return rc;
    }

    /* to create a netlink msg header (cmd=0) */
    ptr_nl_hdr = genlmsg_put(ptr_nl_skb, 0, 0, &ptr_netlink->family_entry, 0, 0);
    if (!ptr_nl_hdr) {
        return -EFAULT;
    }

    /* obtain the intf index for the igr_port */
    igr_port_si = ptr_cookies->pkt.igr_port_si;
    nla_put_u32(ptr_nl_skb, NETIF_NL_ATTR_MOD_IGR_PORT, (uint32_t)igr_port_si);

    /* data */
    rc = nla_put(ptr_nl_skb, NETIF_NL_ATTR_MOD_DATA, data_len, ptr_ori_skb->data);
    if (rc != 0) {
        dbg_print(DBG_ERR, "Failed to add DATA attribute\n");
        return rc;
    }

    genlmsg_end(ptr_nl_skb, ptr_nl_hdr);

    return (rc);
}

static int
netlink_set_sflow_skb(struct netif_netlink *ptr_netlink,
                      struct sk_buff *ptr_ori_skb,
                      struct netlink_rx_cookie *ptr_cookies,
                      struct sk_buff *ptr_nl_skb)
{
    uint16_t iifindex;
    uint16_t eifindex;
    struct net_device_priv *ptr_priv;
    uint32_t rate;
    void *ptr_nl_hdr = NULL;
    uint32_t data_len;
    int rc = 0;

    rc = netlink_get_data_len(ptr_ori_skb, &data_len, ptr_cookies);
    if (0 != rc) {
        return rc;
    }

    /* to create a netlink msg header (cmd=0) */
    ptr_nl_hdr = genlmsg_put(ptr_nl_skb, 0, 0, &ptr_netlink->family_entry, 0, 0);
    if (!ptr_nl_hdr) {
        return -EFAULT;
    }

    iifindex = ptr_cookies->pkt.iifindex;
    eifindex = ptr_cookies->pkt.eifindex;

    nla_put_u16(ptr_nl_skb, NETIF_NL_ATTR_PSAMPLE_IIFINDEX, (uint16_t)iifindex);
    nla_put_u16(ptr_nl_skb, NETIF_NL_ATTR_PSAMPLE_OIFINDEX, (uint16_t)eifindex);

    /* meta header */
    /* use the igr port id as the index for the database to get sample rate */
    ptr_priv = netdev_priv(ptr_ori_skb->dev);
    if (ptr_cookies->pkt.psample_dir == NETIF_NL_PKT_PSAMPLE_INGRESS) {
        rate = ptr_priv->igr_sample_rate;
    } else {
        /* sample rate is anyone of port when egr_port is not valid */
        rate = ptr_priv->egr_sample_rate;
    }
    nla_put_u32(ptr_nl_skb, NETIF_NL_ATTR_PSAMPLE_SAMPLE_RATE, rate);
    nla_put_u32(ptr_nl_skb, NETIF_NL_ATTR_PSAMPLE_ORIGSIZE, data_len);
    nla_put_u32(ptr_nl_skb, NETIF_NL_ATTR_PSAMPLE_SAMPLE_GROUP, ptr_cookies->pkt.psample_dir);
    nla_put_u32(ptr_nl_skb, NETIF_NL_ATTR_PSAMPLE_GROUP_SEQ,
                ptr_netlink->seq_num[ptr_cookies->pkt.psample_dir]);
    ptr_netlink->seq_num[ptr_cookies->pkt.psample_dir]++;

    /* data */
    rc = nla_put(ptr_nl_skb, NETIF_NL_ATTR_DATA, data_len, ptr_ori_skb->data);
    if (rc != 0) {
        dbg_print(DBG_ERR, "Failed to add DATA attribute\n");
        return rc;
    }

    genlmsg_end(ptr_nl_skb, ptr_nl_hdr);

    return (rc);
}

static int
netlink_set_generic_skb(struct netif_netlink *ptr_netlink,
                        struct sk_buff *ptr_ori_skb,
                        struct netlink_rx_cookie *ptr_cookies,
                        struct sk_buff *ptr_nl_skb)
{
    void *ptr_nl_hdr = NULL;
    uint32_t data_len;
    int rc = 0;

    rc = netlink_get_data_len(ptr_ori_skb, &data_len, ptr_cookies);
    if (0 != rc) {
        return rc;
    }

    /* to create a netlink msg header (cmd=0) */
    ptr_nl_hdr = genlmsg_put(ptr_nl_skb, 0, 0, &ptr_netlink->family_entry, 0, 0);
    if (!ptr_nl_hdr) {
        return -EFAULT;
    }
    /* data */
    rc = nla_put(ptr_nl_skb, NETIF_NL_ATTR_DATA, data_len, ptr_ori_skb->data);
    if (rc != 0) {
        dbg_print(DBG_ERR, "Failed to add DATA attribute\n");
        return rc;
    }

    genlmsg_end(ptr_nl_skb, ptr_nl_hdr);

    return (rc);
}

static struct netif_netlink *
netlink_get_netlink_by_name(uint32_t unit, char *ptr_name)
{
    struct netif_netlink *netlink, *tmp;
    struct netif_netlink *ret_netlink = NULL;

    list_for_each_entry_safe(netlink, tmp, &clx_netif_drv(unit)->netlink.list, list)
    {
        if (0 == strncmp(netlink->family_entry.name, ptr_name, CLX_NETLINK_NAME_LEN)) {
            ret_netlink = netlink;
            break;
        }
    }

    return ret_netlink;
}

static int
netlink_get_mcgroup_id_by_name(uint32_t unit,
                               struct netif_netlink *netlink,
                               char *ptr_mcgrp_name,
                               uint32_t *ptr_mcgrp_id)
{
    uint32_t idx;
    int rc = -EFAULT;

    for (idx = 0; idx < netlink->family_entry.n_mcgrps; idx++) {
        if ((0 ==
             strncmp(netlink->family_entry.mcgrps[idx].name, ptr_mcgrp_name,
                     CLX_NETLINK_NAME_LEN))) {
            *ptr_mcgrp_id = idx;
            rc = 0;
            break;
        }
    }

    if (0 != rc) {
        dbg_print(DBG_NETLINK, "[DBG] find mcgrp %s failed in family %s\n", ptr_mcgrp_name,
                  netlink->family_entry.name);
    }

    return (rc);
}

static struct sk_buff *
netlink_alloc_new_skb(struct netif_netlink *ptr_netlink,
                      struct sk_buff *ptr_ori_skb,
                      struct netlink_rx_cookie *ptr_cookies)
{
    int rc = 0;
    uint32_t msg_hdr_len;
    uint32_t data_len;
    struct sk_buff *ptr_nl_skb = NULL;

    rc = netlink_get_header_len(&msg_hdr_len, ptr_cookies);
    if (0 != rc) {
        return NULL;
    }

    rc = netlink_get_data_len(ptr_ori_skb, &data_len, ptr_cookies);
    if (0 != rc) {
        return NULL;
    }

    dbg_print(DBG_NETLINK, "msg_hdr_len:%u data_len:%u.\n", msg_hdr_len, data_len);

    ptr_nl_skb = genlmsg_new(nla_total_size(data_len) + msg_hdr_len, GFP_ATOMIC);
    if (!ptr_nl_skb) {
        dbg_print(DBG_NETLINK, "Failed to alloc netlink skb.\n");
    }

    return ptr_nl_skb;
}

static int
netlink_set_netlink_skb(struct netif_netlink *ptr_netlink,
                        struct sk_buff *ptr_ori_skb,
                        struct netlink_rx_cookie *ptr_cookies,
                        struct sk_buff *ptr_nl_skb)
{
    int rc = 0;

    if (ptr_cookies->netlink_type == NETLINK_RX_TYPE_SFLOW) {
        rc = netlink_set_sflow_skb(ptr_netlink, ptr_ori_skb, ptr_cookies, ptr_nl_skb);
        if (0 != rc) {
            dbg_print(DBG_NETLINK, "Failed to set sflow netlink skb.\n");
        }
    } else if (ptr_cookies->netlink_type == NETLINK_RX_TYPE_MOD) {
        rc = netlink_set_mod_skb(ptr_netlink, ptr_ori_skb, ptr_cookies, ptr_nl_skb);
        if (0 != rc) {
            dbg_print(DBG_NETLINK, "Failed to set mod netlink skb.\n");
        }
    } else {
        rc = netlink_set_generic_skb(ptr_netlink, ptr_ori_skb, ptr_cookies, ptr_nl_skb);
        if (0 != rc) {
            dbg_print(DBG_NETLINK, "Failed to set mod netlink skb.\n");
        }
    }

    return (rc);
}

static int
netlink_send_skb(struct genl_family *ptr_nl_family,
                 uint32_t nl_mcgrp_id,
                 struct sk_buff *ptr_nl_skb)
{
    int rc = 0;
    rc = genlmsg_multicast_netns(ptr_nl_family, &init_net, ptr_nl_skb, 0, nl_mcgrp_id, GFP_ATOMIC);
    if (0 != rc) {
        /* in errno_base.h, #define  ESRCH        3  : No such process */
        dbg_print(DBG_NETLINK, "send skb to mc group failed, rc=%d\n", rc);
        return rc;
    }

    return rc;
}

static int
netlink_forward_rx_packet(uint32_t unit,
                          struct sk_buff *ptr_ori_skb,
                          struct netlink_rx_cookie *ptr_cookies)
{
    struct sk_buff *ptr_nl_skb = NULL;
    struct netif_netlink *netlink;
    uint32_t mcgrp_id;
    int rc;

    netlink = netlink_get_netlink_by_name(unit, ptr_cookies->nl->family_name);
    if (!netlink) {
        dbg_print(DBG_NETLINK, "Netlink with family name %s not found.\n",
                  ptr_cookies->nl->family_name);
        return -ENODATA;
    }

    rc = netlink_get_mcgroup_id_by_name(unit, netlink, ptr_cookies->nl->mc_grp_name, &mcgrp_id);
    if (0 != rc) {
        dbg_print(DBG_NETLINK, "Find mcgrp %s failed in family %s\n", ptr_cookies->nl->mc_grp_name,
                  ptr_cookies->nl->family_name);
        return rc;
    }

    ptr_nl_skb = netlink_alloc_new_skb(netlink, ptr_ori_skb, ptr_cookies);
    if (!ptr_nl_skb) {
        dbg_print(DBG_NETLINK, "Failed to alloc netlink skb.\n");
        return -ENOMEM;
    }

    dbg_print(DBG_NETLINK, "origin skb len:%u nl skb len:%u.\n", ptr_ori_skb->len, ptr_nl_skb->len);

    rc = netlink_set_netlink_skb(netlink, ptr_ori_skb, ptr_cookies, ptr_nl_skb);
    if (0 != rc) {
        dbg_print(DBG_NETLINK, "Failed to allocate netlink skb.rc = %d.\n", rc);
        nlmsg_free(ptr_nl_skb);
        return rc;
    }

    print_packet(DBG_RX_PAYLOAD, ptr_nl_skb->data, ptr_nl_skb->len);

    rc = netlink_send_skb(&netlink->family_entry, mcgrp_id, ptr_nl_skb);
    if (0 != rc) {
        dbg_print(DBG_NETLINK, "Failed to send netlink skb.rc = %d.\n", rc);
        return rc;
    }

    return (rc);
}

int
clx_netif_create_netlink(uint32_t unit, unsigned long arg)
{
    struct clx_netif_ioctl_netlink knetlink;
    struct clx_netif_ioctl_netlink __user *user_netlink = (void __user *)arg;
    struct netif_netlink *new_netlink;
    struct genl_multicast_group *ptr_nl_mcgrp;
    unsigned long flags = 0;
    uint32_t idx;
    int rc;

    if (copy_from_user(&knetlink, user_netlink, sizeof(struct clx_netif_ioctl_netlink))) {
        return -EFAULT;
    }

    new_netlink = kmalloc(sizeof(struct netif_netlink), GFP_ATOMIC);
    if (!new_netlink)
        return -ENOMEM;
    memset(new_netlink, 0x0, sizeof(struct netif_netlink));

    /* fill in the meta data for that netlink family */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0)
    new_netlink->family_entry.id = GENL_ID_GENERATE; /* family id can be ignored since linux 4.10 */
#endif
    new_netlink->family_entry.version = 1;
    new_netlink->family_entry.netnsok = true;
    memcpy(new_netlink->family_entry.name, knetlink.family_name, CLX_NETLINK_NAME_LEN);
    new_netlink->family_entry.maxattr = NETIF_NL_ATTR_LAST;

    /* fill in the mc group info */
    ptr_nl_mcgrp = kmalloc(sizeof(struct genl_multicast_group) * knetlink.mc_grp_num, GFP_ATOMIC);
    if (!ptr_nl_mcgrp) {
        kfree(new_netlink);
        return -ENOMEM;
    }

    dbg_print(DBG_NETLINK, "Create netlink family name:%s.\n", new_netlink->family_entry.name);
    for (idx = 0; idx < knetlink.mc_grp_num; idx++) {
        memcpy(ptr_nl_mcgrp[idx].name, knetlink.mc_grp_name[idx], CLX_NETLINK_NAME_LEN);
        dbg_print(DBG_NETLINK, "     - mcgrp%d: %s\n", idx, ptr_nl_mcgrp[idx].name);
    }
    new_netlink->family_entry.n_mcgrps = knetlink.mc_grp_num;
    new_netlink->family_entry.mcgrps = ptr_nl_mcgrp;

    /* register the family to kernel */
    rc = genl_register_family(&new_netlink->family_entry);
    if (0 != rc) {
        dbg_print(DBG_NETLINK, "[DBG] register netlink family failed, name=%s, rc=%d\n",
                  knetlink.family_name, rc);
        kfree(ptr_nl_mcgrp);
        kfree(new_netlink);
        return -EFAULT;
    }

    /* Allocate a new family entry and append to the list. */
    spin_lock_irqsave(&clx_netif_drv(unit)->netlink.lock, flags);
    rc = netlink_alloc_family_entry(unit, &new_netlink->id);
    if (rc != 0) {
        dbg_print(DBG_NETLINK, "No valid netlink entry_id.\n");
        kfree(ptr_nl_mcgrp);
        kfree(new_netlink);
        return rc;
    }
    list_add_tail(&new_netlink->list, &clx_netif_drv(unit)->netlink.list);
    spin_unlock_irqrestore(&clx_netif_drv(unit)->netlink.lock, flags);

    if (copy_to_user(&user_netlink->id, &new_netlink->id, sizeof(new_netlink->id))) {
        dbg_print(DBG_ERR, "Failed to copy netlink id to user space\n");
        genl_unregister_family(&new_netlink->family_entry);
        kfree(ptr_nl_mcgrp);
        kfree(new_netlink);
        netlink_free_family_entry(unit, new_netlink->id);
        return -EFAULT;
    }

    dbg_print(DBG_NETLINK, "New netlink added, print more message here.....\n");
    return (rc);
}

int
clx_netif_destroy_netlink(uint32_t unit, unsigned long arg)
{
    struct netif_netlink *netlink, *tmp;
    bool found = false;
    uint32_t entry_idx;
    unsigned long flags = 0;

    if (copy_from_user(&entry_idx, (uint32_t __user *)arg, sizeof(uint32_t))) {
        return -EFAULT;
    }

    spin_lock_irqsave(&clx_netif_drv(unit)->netlink.lock, flags);
    list_for_each_entry_safe(netlink, tmp, &clx_netif_drv(unit)->netlink.list, list)
    {
        if (netlink->id == entry_idx) {
            /* unregister Netlink family */
            genl_unregister_family(&netlink->family_entry);
            /* free mcgrps */
            if (netlink->family_entry.mcgrps) {
                kfree(netlink->family_entry.mcgrps);
                netlink->family_entry.mcgrps = NULL;
                netlink->family_entry.n_mcgrps = 0;
            }

            // free the other resource.
            netlink_free_family_entry(unit, entry_idx);
            list_del(&netlink->list);
            kfree(netlink);
            found = true;
            dbg_print(DBG_PROFILE, "Netlink with ID %u destroyed successfully\n", entry_idx);
            break;
        }
    }
    spin_unlock_irqrestore(&clx_netif_drv(unit)->netlink.lock, flags);

    if (!found) {
        dbg_print(DBG_PROFILE, "Profile netlink with ID %u not found\n", entry_idx);
        return -ENOENT;
    }

    return (0);
}

static void
clx_netif_ioctl_netlink_destroy_all(uint32_t unit)
{
    struct netif_netlink *netlink, *tmp;
    unsigned long flags = 0;

    spin_lock_irqsave(&clx_netif_drv(unit)->netlink.lock, flags);
    list_for_each_entry_safe(netlink, tmp, &clx_netif_drv(unit)->netlink.list, list)
    {
        dbg_print(DBG_NETLINK, "Destroy netlink with ID %u.\n", netlink->id);
        /* unregister Netlink family */
        genl_unregister_family(&netlink->family_entry);
        /* free mcgrps */
        if (netlink->family_entry.mcgrps) {
            kfree(netlink->family_entry.mcgrps);
            netlink->family_entry.mcgrps = NULL;
            netlink->family_entry.n_mcgrps = 0;
        }

        // free the other resource.
        netlink_free_family_entry(unit, netlink->id);
        list_del(&netlink->list);
        kfree(netlink);
    }
    spin_unlock_irqrestore(&clx_netif_drv(unit)->netlink.lock, flags);
}

int
clx_netif_get_netlink(uint32_t unit, unsigned long arg)
{
    struct netif_netlink *netlink, *tmp;
    struct clx_netif_ioctl_netlink knetlink = {0};
    struct clx_netif_ioctl_netlink __user *user_netlink = (void __user *)arg;
    unsigned long flags = 0;
    bool found = false;
    uint32_t netlink_id, idx;

    if (copy_from_user(&netlink_id, &user_netlink->id, sizeof(uint32_t))) {
        dbg_print(DBG_ERR, "Failed to copy netlink from user space\n");
        return -EFAULT;
    }

    spin_lock_irqsave(&clx_netif_drv(unit)->netlink.lock, flags);
    list_for_each_entry_safe(netlink, tmp, &clx_netif_drv(unit)->netlink.list, list)
    {
        if (netlink->id == netlink_id) {
            found = true;
            break;
        }
    }
    spin_unlock_irqrestore(&clx_netif_drv(unit)->netlink.lock, flags);

    if (found) {
        dbg_print(DBG_NETLINK, "Found netlink with ID %u successfully\n", netlink_id);
        knetlink.id = netlink->id;
        knetlink.mc_grp_num = netlink->family_entry.n_mcgrps;
        memcpy(knetlink.family_name, netlink->family_entry.name, CLX_NETLINK_NAME_LEN);
        for (idx = 0; idx < netlink->family_entry.n_mcgrps; idx++) {
            memcpy(knetlink.mc_grp_name[idx], netlink->family_entry.mcgrps[idx].name,
                   CLX_NETLINK_NAME_LEN);
        }

        if (copy_to_user(user_netlink, &knetlink, sizeof(struct clx_netif_ioctl_netlink))) {
            dbg_print(DBG_ERR, "Failed to copy netlink to user space\n");
            return -EFAULT;
        }
    } else {
        dbg_print(DBG_NETLINK, "Netlink with ID %u not found\n", netlink_id);
        knetlink.rc = CLX_IOCTL_E_ENTRY_NOT_FOUND;
        if (copy_to_user(user_netlink, &knetlink, sizeof(struct clx_netif_ioctl_netlink))) {
            dbg_print(DBG_ERR, "Failed to copy netlink to user space\n");
            return -EFAULT;
        }
        return 0;
    }

    return 0;
}

int
clx_netif_set_pkt_mod(uint32_t unit, unsigned long arg)
{
    unsigned char __user *ptr_user_mac = (void __user *)arg;
    clx_mac_t mac;

    if (copy_from_user(mac, ptr_user_mac, sizeof(clx_mac_t))) {
        dbg_print(DBG_ERR, "Failed to copy mod mac from user space\n");
        return -EFAULT;
    }

    if (0 == memcmp(g_mod_default_mac, mac, sizeof(clx_mac_t))) {
        clx_netif_drv(unit)->enable_mod_dmac = false;
        memset(clx_netif_drv(unit)->mod_dmac, 0, sizeof(clx_mac_t));
    } else {
        clx_netif_drv(unit)->enable_mod_dmac = true;
        memcpy(clx_netif_drv(unit)->mod_dmac, mac, sizeof(clx_mac_t));
    }

    return 0;
}

int
clx_netif_get_pkt_mod(uint32_t unit, unsigned long arg)
{
    unsigned char __user *ptr_user_mac = (void __user *)arg;

    if (copy_to_user(ptr_user_mac, clx_netif_drv(unit)->mod_dmac, sizeof(clx_mac_t))) {
        dbg_print(DBG_ERR, "Failed to copy mod mac to user space\n");
        return -EFAULT;
    }

    return 0;
}

int
netif_netlink_reveive_skb(uint32_t unit,
                          struct dma_rx_packet *rx_packet,
                          uint32_t port_di,
                          void *ptr_cookie)
{
    int rc;
    struct netlink_rx_cookie *ptr_data = NULL;
    struct sk_buff *ptr_skb = NULL;
    unsigned long flags = 0;

    spin_lock_irqsave(&clx_netif_drv(unit)->netlink.lock, flags);
    if (NULL == ptr_cookie) {
        return -EFAULT;
    }

    ptr_data = (struct netlink_rx_cookie *)ptr_cookie;
    ptr_skb = netif_construct_skb_from_rx_packet(unit, port_di, rx_packet);

    /* send the packet to netlink mcgroup */
    rc = netlink_forward_rx_packet(unit, ptr_skb, ptr_data);
    if (rc != 0) {
        dbg_print(DBG_NETLINK, "Failed to forward rx packet. rc:%d\n", rc);
    }
    spin_unlock_irqrestore(&clx_netif_drv(unit)->netlink.lock, flags);

    if (rx_packet->list_count > 1) {
        dev_kfree_skb_any(ptr_skb);
    }

    rc = dma_free_rx_packet(unit, rx_packet, true);
    return (rc);
}

int
clx_netlink_init(void)
{
    uint32_t unit = 0;

    for (unit = 0; unit < clx_misc_dev->pci_dev_num; unit++) {
        INIT_LIST_HEAD(&clx_netif_drv(unit)->netlink.list);
        spin_lock_init(&clx_netif_drv(unit)->netlink.lock);
        // To allocate netlink id.
        bitmap_zero(clx_netif_drv(unit)->netlink.netlink_id_bitmap, CLX_NETLINK_MAX_NUM);
    }

    return 0;
}

void
clx_netlink_deinit(void)
{
    uint32_t unit = 0;

    for (unit = 0; unit < clx_misc_dev->pci_dev_num; unit++) {
        clx_netif_ioctl_netlink_destroy_all(unit);
    }
}