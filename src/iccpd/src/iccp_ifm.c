/*
 * iccp_ifm.c
 *
 * Copyright(c) 2016-2019 Nephos/Estinet.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 *  Maintainer: jianjun, grace Li from nephos
 */

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_bridge.h>
#include <netlink/msg.h>

#include "../include/system.h"
#include "../include/iccp_cli.h"
#include "../include/logger.h"
#include "../include/mlacp_sync_update.h"
#include "../include/mlacp_link_handler.h"
#include "../include/port.h"
#include "../include/iccp_netlink.h"

#define fwd_neigh_state_valid(state) (state & (NUD_REACHABLE | NUD_STALE | NUD_DELAY | NUD_PROBE | NUD_PERMANENT))

#ifndef NDA_RTA
#define NDA_RTA(r) \
    ((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ndmsg))))
#endif

static int iccp_valid_handler(struct nl_msg *msg, void *arg)
{
    struct nlmsghdr *nlh = nlmsg_hdr(msg);
    unsigned int event = 0;

    if (nlh->nlmsg_type != RTM_NEWLINK)
        return 0;

    if (nl_msg_parse(msg, &iccp_event_handler_obj_input_newlink, &event) < 0)
        ICCPD_LOG_ERR(__FUNCTION__, "Unknown message type.");

    return 0;
}

/*Get kernel interfaces and ports during initialization*/
int iccp_sys_local_if_list_get_init()
{
    struct System *sys = NULL;
    struct nl_cb *cb;
    struct nl_cb *orig_cb;
    struct rtgenmsg rt_hdr = {
        .rtgen_family   = AF_UNSPEC,
    };
    int ret;
    int retry = 1;

    if (!(sys = system_get_instance()))
        return MCLAG_ERROR;

    while (retry)
    {
        retry = 0;
        ret = nl_send_simple(sys->route_sock, RTM_GETLINK, NLM_F_DUMP,
                             &rt_hdr, sizeof(rt_hdr));
        if (ret < 0)
        {
            ICCPD_LOG_ERR(__FUNCTION__, "send netlink msg error.");
            return ret;
        }

        orig_cb = nl_socket_get_cb(sys->route_sock);
        cb = nl_cb_clone(orig_cb);
        nl_cb_put(orig_cb);
        if (!cb)
        {
            ICCPD_LOG_ERR(__FUNCTION__, "nl cb clone error.");
            return -ENOMEM;
        }

        nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, iccp_valid_handler, sys);

        ret = nl_recvmsgs(sys->route_sock, cb);
        nl_cb_put(cb);
        if (ret < 0)
        {
            if (ret != -NLE_DUMP_INTR)
            {
                ICCPD_LOG_ERR(__FUNCTION__, "No retry, receive netlink msg error. ret = %d  errno = %d ", ret, errno);
                return ret;
            }
            ICCPD_LOG_NOTICE(__FUNCTION__, "Retry: receive netlink msg error. ret = %d  errno = %d ", ret, errno);
            retry = 1;
        }
    }

    return ret;
}

static void do_arp_learn_from_kernel(struct ndmsg *ndm, struct rtattr *tb[], int msgtype, int is_del)
{
    struct System *sys = NULL;
    struct CSM *csm = NULL;
    struct Msg *msg = NULL;
    struct ARPMsg *arp_msg = NULL, *arp_info = NULL;
    struct VLAN_ID *vlan_id_list = NULL;
    struct Msg *msg_send = NULL;
    uint16_t vid = 0;
    int entry_exists = 0;
    struct LocalInterface *peer_link_if = NULL;
    int ln = 0;
    uint16_t vlan_id = 0;
    struct VLAN_ID vlan_key = { 0 };

    char buf[MAX_BUFSIZE] = { 0 };
    size_t msg_len = 0;

    struct LocalInterface *lif_po = NULL, *arp_lif = NULL;

    int verify_arp = 0;
    int arp_update = 0;

    if (!(sys = system_get_instance()))
        return;

    /* Find local itf*/
    if (!(arp_lif = local_if_find_by_ifindex(ndm->ndm_ifindex)))
        return;

    /* create ARP msg*/
    msg_len = sizeof(struct ARPMsg);
    arp_msg = (struct ARPMsg *)&buf;
    arp_msg->op_type = NEIGH_SYNC_LIF;
    sprintf(arp_msg->ifname, "%s", arp_lif->name);
    if (tb[NDA_DST])
        memcpy(&arp_msg->ipv4_addr, RTA_DATA(tb[NDA_DST]), RTA_PAYLOAD(tb[NDA_DST]));
    if (!is_del && tb[NDA_LLADDR])
        memcpy(arp_msg->mac_addr, RTA_DATA(tb[NDA_LLADDR]), RTA_PAYLOAD(tb[NDA_LLADDR]));

    arp_msg->ipv4_addr = arp_msg->ipv4_addr;

    ICCPD_LOG_NOTICE(__FUNCTION__, "ARP type %s, state (%04X)(%d) ifindex [%d] (%s) ip %s, mac [%02X:%02X:%02X:%02X:%02X:%02X]",
                    msgtype == RTM_NEWNEIGH ? "New":"Del", ndm->ndm_state, fwd_neigh_state_valid(ndm->ndm_state),
                    ndm->ndm_ifindex, arp_lif->name,
                    show_ip_str(arp_msg->ipv4_addr),
                    arp_msg->mac_addr[0], arp_msg->mac_addr[1], arp_msg->mac_addr[2], arp_msg->mac_addr[3], arp_msg->mac_addr[4],
                    arp_msg->mac_addr[5]);

    if (msgtype != RTM_DELNEIGH) {
        if (arp_lif->ipv4_addr == 0) {
            ICCPD_LOG_DEBUG(__FUNCTION__, "IP not configured on %s, ignore ARP", arp_lif->name);
            return;
        }
    }

    uint8_t bcast_mac[ETHER_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    if (memcmp(arp_msg->mac_addr, bcast_mac, ETHER_ADDR_LEN) == 0)
    {
        ICCPD_LOG_DEBUG(__FUNCTION__, "Ignoring neighbor entry due to bcast lladdr");
        msgtype = RTM_DELNEIGH;
    }

    if ((strncmp(arp_msg->ifname, VLAN_PREFIX, strlen(VLAN_PREFIX)) == 0)) {
        sscanf (arp_msg->ifname, "Vlan%hu", &vlan_id);
    }

    if (vlan_id) {
        memset(&vlan_key, 0, sizeof(struct VLAN_ID));
        vlan_key.vid = vlan_id;
    }

    /* Find MLACP itf, member of port-channel*/
    LIST_FOREACH(csm, &(sys->csm_list), next)
    {
        vid = 0;
        ln = __LINE__;
        LIST_FOREACH(lif_po, &(MLACP(csm).lif_list), mlacp_next)
        {
            if (lif_po->type != IF_T_PORT_CHANNEL) {
                ln = __LINE__;
                continue;
            }

            if (!local_if_is_l3_mode(lif_po))
            {
                vid = 0;
                /* Is the L2 MLAG itf belong to a vlan?*/
                vlan_id_list = RB_FIND(vlan_rb_tree, &(lif_po->vlan_tree), &vlan_key);

                if (!vlan_id_list) {
                    ln = __LINE__;
                    continue;
                }

                if (!vlan_id_list->vlan_itf) {
                    ln = __LINE__;
                    continue;
                }

                if (vlan_id_list->vlan_itf->ifindex != ndm->ndm_ifindex) {
                    ln = __LINE__;
                    continue;
                }

                vid = vlan_id_list->vid;
                ICCPD_LOG_DEBUG(__FUNCTION__, "ARP is from mclag enabled member port of vlan %d", vid);
            }
            else
            {
                /* Is the ARP belong to a L3 mode MLAG itf?*/
                if (ndm->ndm_ifindex != lif_po->ifindex) {
                    ln = __LINE__;
                    continue;
                }

                ICCPD_LOG_DEBUG(__FUNCTION__, "ARP is from mclag enabled intf %s", lif_po->name);
            }

            verify_arp = 1;

            break;
        }

        if (lif_po) {
            break;
        } else if (csm->peer_link_if){
           peer_link_if = csm->peer_link_if;
           if (!local_if_is_l3_mode(peer_link_if)) {
               vid = 0;
               vlan_id_list = RB_FIND(vlan_rb_tree, &(peer_link_if->vlan_tree), &vlan_key);

               if (vlan_id_list && vlan_id_list->vlan_itf) {
                   if (vlan_id_list->vlan_itf->ifindex == ndm->ndm_ifindex) {
                       vid = vlan_id_list->vid;
                       ICCPD_LOG_DEBUG(__FUNCTION__, "ARP is from peer link vlan %d", vid);
                       verify_arp = 1;
                       lif_po = peer_link_if;
                       break;
                   }
               }
           }
        }
    }

    if (!(csm && lif_po)) {
        ICCPD_LOG_DEBUG(__FUNCTION__, "ARP received no PO ln %d", ln);
        return;
    }
    if (!verify_arp) {
        ICCPD_LOG_DEBUG(__FUNCTION__, "ARP received no verify_arp ln %d", ln);
        return;
    }

    if (vid != 0) {
        if (vid && vlan_id_list && vlan_id_list->vlan_itf) {
            if (arp_msg->ipv4_addr == vlan_id_list->vlan_itf->ipv4_addr) {
                ICCPD_LOG_DEBUG(__FUNCTION__, "Ignore My ip %s", show_ip_str(arp_msg->ipv4_addr));
                return;
            }
        }
    }

    /* update lif ARP*/
    TAILQ_FOREACH(msg, &MLACP(csm).arp_list, tail)
    {
        arp_info = (struct ARPMsg *)msg->buf;
        if (arp_info->ipv4_addr != arp_msg->ipv4_addr)
            continue;

        entry_exists = 1;
        if (msgtype == RTM_DELNEIGH)
        {
            /* delete ARP*/
            TAILQ_REMOVE(&MLACP(csm).arp_list, msg, tail);
            free(msg->buf);
            free(msg);
            msg = NULL;
            ICCPD_LOG_DEBUG(__FUNCTION__, "Delete ARP %s", show_ip_str(arp_msg->ipv4_addr));
        }
        else
        {
            /* update ARP*/
            if (arp_info->op_type != arp_msg->op_type
                || strcmp(arp_info->ifname, arp_msg->ifname) != 0
                || memcmp(arp_info->mac_addr, arp_msg->mac_addr, ETHER_ADDR_LEN) != 0)
            {
                arp_update = 1;
                arp_info->op_type = arp_msg->op_type;
                sprintf(arp_info->ifname, "%s", arp_msg->ifname);
                memcpy(arp_info->mac_addr, arp_msg->mac_addr, ETHER_ADDR_LEN);
                ICCPD_LOG_DEBUG(__FUNCTION__, "Update ARP for %s", show_ip_str(arp_msg->ipv4_addr));
            }
        }
        break;
    }

    if (msg && !arp_update)
        return;

    if (msgtype != RTM_DELNEIGH)
    {
        /* enquene lif_msg (add)*/
        if (!msg)
        {
            arp_msg->op_type = NEIGH_SYNC_LIF;
            arp_msg->learn_flag = NEIGH_LOCAL;
            if (iccp_csm_init_msg(&msg, (char *)arp_msg, msg_len) == 0)
            {
                mlacp_enqueue_arp(csm, msg);
                /*ICCPD_LOG_DEBUG(__FUNCTION__, "ARP-list enqueue: %s, add %s",
                                arp_msg->ifname, show_ip_str((arp_msg->ipv4_addr));*/
            }
            else
                ICCPD_LOG_WARN(__FUNCTION__, "Failed to enqueue ARP-list: %s, add %s",
                                arp_msg->ifname, show_ip_str(arp_msg->ipv4_addr));
        }

        /* enqueue iccp_msg (add)*/
        if (MLACP(csm).current_state == MLACP_STATE_EXCHANGE)
        {
            arp_msg->op_type = NEIGH_SYNC_ADD;
            arp_msg->flag = 0;
            if (iccp_csm_init_msg(&msg_send, (char *)arp_msg, msg_len) == 0)
            {
                TAILQ_INSERT_TAIL(&(MLACP(csm).arp_msg_list), msg_send, tail);
                /*ICCPD_LOG_DEBUG(__FUNCTION__, "Enqueue ARP[ADD] message for %s",
                                show_ip_str(arp_msg->ipv4_addr));*/
            }
            else
                ICCPD_LOG_WARN(__FUNCTION__, "Failed to enqueue ARP[ADD] message for %s",
                                show_ip_str(arp_msg->ipv4_addr));
        }
    }
    else
    {
        /* enqueue iccp_msg (delete)*/
        if ((MLACP(csm).current_state == MLACP_STATE_EXCHANGE) && entry_exists)
        {
            arp_msg->op_type = NEIGH_SYNC_DEL;
            arp_msg->flag = 0;
            if (iccp_csm_init_msg(&msg_send, (char *)arp_msg, msg_len) == 0)
            {
                TAILQ_INSERT_TAIL(&(MLACP(csm).arp_msg_list), msg_send, tail);
                /*ICCPD_LOG_DEBUG(__FUNCTION__, "Enqueue ARP[DEL] message for %s",
                                show_ip_str(arp_msg->ipv4_addr));*/
            }
            else
                ICCPD_LOG_WARN(__FUNCTION__, "Failed to enqueue ARP[DEL] message for %s",
                                show_ip_str(arp_msg->ipv4_addr));
        } else {
            ICCPD_LOG_DEBUG(__FUNCTION__, "ARP[DEL] message for %s skipped. entry_exists %d",
                                show_ip_str(arp_msg->ipv4_addr), entry_exists);
        }
    }

    return;
}

static void do_ndisc_learn_from_kernel(struct ndmsg *ndm, struct rtattr *tb[], int msgtype, int is_del)
{
    struct System *sys = NULL;
    struct CSM *csm = NULL;
    struct Msg *msg = NULL;
    struct NDISCMsg *ndisc_msg = NULL, *ndisc_info = NULL;
    struct VLAN_ID *vlan_id_list = NULL;
    struct Msg *msg_send = NULL;
    uint16_t vid = 0;
    int entry_exists = 0;
    int is_link_local = 0;
    int ln = 0;
    uint16_t vlan_id = 0;
    struct VLAN_ID vlan_key = { 0 };

    char buf[MAX_BUFSIZE] = { 0 };
    size_t msg_len = 0;
    char addr_null[16] = { 0 };

    struct LocalInterface *lif_po = NULL, *ndisc_lif = NULL;
    struct LocalInterface *peer_link_if = NULL;

    int verify_neigh = 0;
    int neigh_update = 0;

    if (!(sys = system_get_instance()))
        return;

    /* Find local itf */
    if (!(ndisc_lif = local_if_find_by_ifindex(ndm->ndm_ifindex)))
        return;

    /* create NDISC msg */
    msg_len = sizeof(struct NDISCMsg);
    ndisc_msg = (struct NDISCMsg *)&buf;
    ndisc_msg->op_type = NEIGH_SYNC_LIF;
    sprintf(ndisc_msg->ifname, "%s", ndisc_lif->name);
    if (tb[NDA_DST])
        memcpy(&ndisc_msg->ipv6_addr, RTA_DATA(tb[NDA_DST]), RTA_PAYLOAD(tb[NDA_DST]));
    if (!is_del && tb[NDA_LLADDR])
        memcpy(ndisc_msg->mac_addr, RTA_DATA(tb[NDA_LLADDR]), RTA_PAYLOAD(tb[NDA_LLADDR]));

    ICCPD_LOG_NOTICE(__FUNCTION__, "ndisc type %s, state (%04X)(%d), ifindex [%d] (%s), ip %s, mac [%02X:%02X:%02X:%02X:%02X:%02X]",
                    msgtype == RTM_NEWNEIGH ? "New" : "Del", ndm->ndm_state, fwd_neigh_state_valid(ndm->ndm_state),
                    ndm->ndm_ifindex, ndisc_lif->name,
                    show_ipv6_str((char *)ndisc_msg->ipv6_addr),
                    ndisc_msg->mac_addr[0], ndisc_msg->mac_addr[1], ndisc_msg->mac_addr[2], ndisc_msg->mac_addr[3], ndisc_msg->mac_addr[4],
                    ndisc_msg->mac_addr[5]);


    if (msgtype != RTM_DELNEIGH) {
        if (memcmp(ndisc_lif->ipv6_addr, addr_null, 16) == 0) {
            ICCPD_LOG_DEBUG(__FUNCTION__, "IPv6 address not configured on %s, ignore ND", ndisc_lif->name);
            return;
        }
    }

    uint8_t bcast_mac[ETHER_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    if (memcmp(ndisc_msg->mac_addr, bcast_mac, ETHER_ADDR_LEN) == 0)
    {
        ICCPD_LOG_DEBUG(__FUNCTION__, "Ignoring neighbor entry due to bcast lladdr");
        msgtype = RTM_DELNEIGH;
        return;
    }

    if ((strncmp(ndisc_msg->ifname, VLAN_PREFIX, strlen(VLAN_PREFIX)) == 0)) {
        sscanf (ndisc_msg->ifname, "Vlan%hu", &vlan_id);
    }

    if (vlan_id) {
        memset(&vlan_key, 0, sizeof(struct VLAN_ID));
        vlan_key.vid = vlan_id;
    }
    /* Find MLACP itf, member of port-channel */
    LIST_FOREACH(csm, &(sys->csm_list), next)
    {
        vid = 0;
        ln = __LINE__;
        LIST_FOREACH(lif_po, &(MLACP(csm).lif_list), mlacp_next)
        {
            if (lif_po->type != IF_T_PORT_CHANNEL) {
                ln = __LINE__;
                continue;
            }

            vid = 0;
            if (!local_if_is_l3_mode(lif_po))
            {
                /* Is the L2 MLAG itf belong to a vlan?*/
                vlan_id_list = RB_FIND(vlan_rb_tree, &(lif_po->vlan_tree), &vlan_key);

                if (!vlan_id_list) {
                    ln = __LINE__;
                    continue;
                }

                if (!vlan_id_list->vlan_itf) {
                    ln = __LINE__;
                    continue;
                }

                if (vlan_id_list->vlan_itf->ifindex != ndm->ndm_ifindex) {
                    ln = __LINE__;
                    continue;
                }

                vid = vlan_id_list->vid;
                ICCPD_LOG_DEBUG(__FUNCTION__, "neighor is from intf %s of vlan %d", lif_po->name, vid);
            }
            else
            {
                /* Is the ND belong to a L3 mode MLAG itf? */
                if (ndm->ndm_ifindex != lif_po->ifindex) {
                    ln = __LINE__;
                    continue;
                }

                ICCPD_LOG_DEBUG(__FUNCTION__, "neighbor is from intf %s", lif_po->name);
            }

            verify_neigh = 1;

            break;
        }

        if (lif_po) {
            break;
        } else if (csm->peer_link_if){
           peer_link_if = csm->peer_link_if;
           if (!local_if_is_l3_mode(peer_link_if)) {
               vid = 0;
               vlan_id_list = RB_FIND(vlan_rb_tree, &(peer_link_if->vlan_tree), &vlan_key);

               if (vlan_id_list && vlan_id_list->vlan_itf) {
                   if (vlan_id_list->vlan_itf->ifindex == ndm->ndm_ifindex) {
                       vid = vlan_id_list->vid;
                       ICCPD_LOG_DEBUG(__FUNCTION__, "ND is from peer link vlan %d", vid);
                       verify_neigh = 1;
                       lif_po = peer_link_if;
                       break;
                   }
               }
           }
        }
    }

    if (!(csm && lif_po)) {
        ICCPD_LOG_DEBUG(__FUNCTION__, "ND received no PO ln %d", ln);
        return;
    }
    if (!verify_neigh) {
        ICCPD_LOG_DEBUG(__FUNCTION__, "ND received no verify_ndisc ln %d", ln);
        return;
    }

    if ((memcmp(show_ipv6_str((char *)ndisc_msg->ipv6_addr), "FE80", 4) == 0)
            || (memcmp(show_ipv6_str((char *)ndisc_msg->ipv6_addr), "fe80", 4) == 0))
    {
        is_link_local = 1;
    }

    if (vid && vlan_id_list && vlan_id_list->vlan_itf) {
        if (memcmp((char *)ndisc_msg->ipv6_addr, (char *)vlan_id_list->vlan_itf->ipv6_addr, 16) == 0)
        {
            ICCPD_LOG_DEBUG(__FUNCTION__, "Ignoring neighbor entry for My Ip %s", show_ipv6_str((char *)ndisc_msg->ipv6_addr));
            return;
        }

        if (is_link_local)
        {
            if (memcmp((char *)ndisc_msg->ipv6_addr, (char *)vlan_id_list->vlan_itf->ipv6_ll_addr, 16) == 0)
            {
                ICCPD_LOG_DEBUG(__FUNCTION__, "Ignoring neighbor entry for My Ip %s", show_ipv6_str((char *)ndisc_msg->ipv6_addr));
                return;
            }
        }
    }

    /* update lif ND */
    TAILQ_FOREACH(msg, &MLACP(csm).ndisc_list, tail)
    {
        ndisc_info = (struct NDISCMsg *)msg->buf;

        if (memcmp(&ndisc_info->ipv6_addr, &ndisc_msg->ipv6_addr, 16) != 0)
            continue;

        entry_exists = 1;
        if (msgtype == RTM_DELNEIGH)
        {
            /* delete ND */
            TAILQ_REMOVE(&MLACP(csm).ndisc_list, msg, tail);
            free(msg->buf);
            free(msg);
            msg = NULL;
            ICCPD_LOG_DEBUG(__FUNCTION__, "Delete neighbor %s", show_ipv6_str((char *)ndisc_msg->ipv6_addr));
        }
        else
        {
            /* update ND */
            if (ndisc_info->op_type != ndisc_info->op_type
                || strcmp(ndisc_info->ifname, ndisc_info->ifname) != 0 || memcmp(ndisc_info->mac_addr, ndisc_info->mac_addr, ETHER_ADDR_LEN) != 0)
            {
                neigh_update = 1;
                ndisc_info->op_type = ndisc_msg->op_type;
                sprintf(ndisc_info->ifname, "%s", ndisc_msg->ifname);
                memcpy(ndisc_info->mac_addr, ndisc_msg->mac_addr, ETHER_ADDR_LEN);
                ICCPD_LOG_DEBUG(__FUNCTION__, "Update neighbor for %s", show_ipv6_str((char *)ndisc_msg->ipv6_addr));
            }
        }
        break;
    }

    if (msg && !neigh_update)
        return;

    if (msgtype != RTM_DELNEIGH)
    {
        /* enquene lif_msg (add) */
        if (!msg)
        {
            ndisc_msg->op_type = NEIGH_SYNC_LIF;
            ndisc_msg->learn_flag = NEIGH_LOCAL;
            if (iccp_csm_init_msg(&msg, (char *)ndisc_msg, msg_len) == 0)
            {
                mlacp_enqueue_ndisc(csm, msg);
                /* ICCPD_LOG_DEBUG(__FUNCTION__, "Ndisc-list enqueue: %s, add %s", ndisc_msg->ifname, show_ipv6_str((char *)ndisc_msg->ipv6_addr)); */
            }
            else
                ICCPD_LOG_DEBUG(__FUNCTION__, "Failed to enqueue Ndisc-list: %s, add %s",
                                ndisc_msg->ifname, show_ipv6_str((char *)ndisc_msg->ipv6_addr));
        }

        /* enqueue iccp_msg (add) */
        if (MLACP(csm).current_state == MLACP_STATE_EXCHANGE)
        {
            ndisc_msg->op_type = NEIGH_SYNC_ADD;
            ndisc_msg->flag = 0;
            if (iccp_csm_init_msg(&msg_send, (char *)ndisc_msg, msg_len) == 0)
            {
                TAILQ_INSERT_TAIL(&(MLACP(csm).ndisc_msg_list), msg_send, tail);
                /* ICCPD_LOG_DEBUG(__FUNCTION__, "Enqueue Ndisc[ADD] for %s", show_ipv6_str((char *)ndisc_msg->ipv6_addr)); */
            }
            else
                ICCPD_LOG_DEBUG(__FUNCTION__, "Failed to enqueue Ndisc[ADD] message for %s", show_ipv6_str((char *)ndisc_msg->ipv6_addr));
        }
    }
    else
    {
        /* enqueue iccp_msg (delete) */
        /* send delete notification, only if entry is present in DB*/
        if ((MLACP(csm).current_state == MLACP_STATE_EXCHANGE) && entry_exists)
        {
            ndisc_msg->op_type = NEIGH_SYNC_DEL;
            ndisc_msg->flag = 0;
            if (iccp_csm_init_msg(&msg_send, (char *)ndisc_msg, msg_len) == 0)
            {
                TAILQ_INSERT_TAIL(&(MLACP(csm).ndisc_msg_list), msg_send, tail);
                /* ICCPD_LOG_DEBUG(__FUNCTION__, "Enqueue Ndisc[DEL] for %s", show_ipv6_str((char *)ndisc_msg->ipv6_addr)); */
            }
            else
                ICCPD_LOG_DEBUG(__FUNCTION__, "Failed to enqueue Ndisc[DEL] message for %s", show_ipv6_str((char *)ndisc_msg->ipv6_addr));
        } else {
            ICCPD_LOG_DEBUG(__FUNCTION__, " Ndisc[DEL] message for %s skipped. entry_exists %d",
                    show_ipv6_str((char *)ndisc_msg->ipv6_addr), entry_exists);
        }
    }

    return;
}

int parse_rtattr_flags(struct rtattr *tb[], int max, struct rtattr *rta, int len, unsigned short flags)
{
    unsigned short type;

    memset(tb, 0, sizeof(struct rtattr *) * (max + 1));
    while (RTA_OK(rta, len))
    {
        type = rta->rta_type & ~flags;
        if ((type <= max) && (!tb[type]))
            tb[type] = rta;
        rta = RTA_NEXT(rta, len);
    }
    return 0;
}

int parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len)
{
    return parse_rtattr_flags(tb, max, rta, len, 0);
}

void ifm_parse_rtattr(struct rtattr **tb, int max, struct rtattr *rta, int len)
{
    while (RTA_OK(rta, len))
    {
        if (rta->rta_type <= max)
            tb[rta->rta_type] = rta;
        rta = RTA_NEXT(rta, len);
    }
}

int do_one_neigh_request(struct nlmsghdr *n)
{
    struct ndmsg *ndm = NLMSG_DATA(n);
    int len = n->nlmsg_len;
    struct rtattr *tb[NDA_MAX + 1] = {{0}};
    int is_del = 0;
    int msgtype = n->nlmsg_type;
    struct CSM* csm = NULL;

    if (n->nlmsg_type == NLMSG_DONE)
    {
        return 0;
    }

    /* process msg_type RTM_NEWNEIGH, RTM_GETNEIGH, RTM_DELNEIGH */
    if (n->nlmsg_type != RTM_NEWNEIGH && n->nlmsg_type  != RTM_DELNEIGH )
        return(0);

    /*Check if mclag configured*/
    csm = system_get_first_csm();
    if (!csm)
        return(0);

    len -= NLMSG_LENGTH(sizeof(*ndm));
    if (len < 0)
        return MCLAG_ERROR;

    ifm_parse_rtattr(tb, NDA_MAX, NDA_RTA(ndm), len);

    if (ndm->ndm_state == NUD_INCOMPLETE
        || ndm->ndm_state == NUD_FAILED
        || ndm->ndm_state == NUD_NOARP
        || ndm->ndm_state == NUD_PERMANENT
        || ndm->ndm_state == NUD_NONE)
    {
        if ((ndm->ndm_state == NUD_FAILED)
                || (ndm->ndm_state == NUD_INCOMPLETE))
        {
            is_del = 1;
            msgtype = RTM_DELNEIGH;
        }

        if (!is_del) {
            return(0);
        }
    }

    if (!tb[NDA_DST] || ndm->ndm_type != RTN_UNICAST)
    {
        return(0);
    }

    if (ndm->ndm_family == AF_INET)
    {
        do_arp_learn_from_kernel(ndm, tb, msgtype, is_del);
    }

    if (ndm->ndm_family == AF_INET6)
    {
        do_ndisc_learn_from_kernel(ndm, tb, msgtype, is_del);
    }

    return (0);
}

/*Handle arp received from kernel*/
static int iccp_neigh_valid_handler(struct nl_msg *msg, void *arg)
{
    struct nlmsghdr *nlh = nlmsg_hdr(msg);

    do_one_neigh_request(nlh);

    return 0;
}

int iccp_neigh_get_init()
{
    struct System *sys = NULL;
    struct nl_cb *cb;
    struct nl_cb *orig_cb;
    struct rtgenmsg rt_hdr = {
        .rtgen_family   = AF_UNSPEC,
    };
    int ret;
    int retry = 1;

    if (!(sys = system_get_instance()))
        return MCLAG_ERROR;

    while (retry)
    {
        retry = 0;
        ret = nl_send_simple(sys->route_sock, RTM_GETNEIGH, NLM_F_DUMP,
                             &rt_hdr, sizeof(rt_hdr));
        if (ret < 0)
        {
            ICCPD_LOG_ERR(__FUNCTION__, "Send netlink msg error.");
            return ret;
        }

        orig_cb = nl_socket_get_cb(sys->route_sock);
        cb = nl_cb_clone(orig_cb);
        nl_cb_put(orig_cb);
        if (!cb)
        {
            ICCPD_LOG_ERR(__FUNCTION__, "nl cb clone error.");
            return -ENOMEM;
        }

        nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, iccp_neigh_valid_handler, sys);

        ret = nl_recvmsgs(sys->route_sock, cb);
        nl_cb_put(cb);
        if (ret < 0)
        {
            ICCPD_LOG_ERR(__FUNCTION__, "Receive netlink msg error.");
            if (ret != -NLE_DUMP_INTR)
                return ret;

            retry = 1;
        }
    }

    return ret;
}

/*When received ARP packets from kernel, update arp information*/
void do_arp_update_from_reply_packet(unsigned int ifindex, unsigned int addr, uint8_t mac_addr[ETHER_ADDR_LEN])
{
    struct System *sys = NULL;
    struct CSM *csm = NULL;
    struct Msg *msg = NULL;
    struct ARPMsg *arp_msg = NULL, *arp_info = NULL;
    struct VLAN_ID *vlan_id_list = NULL;
    struct Msg *msg_send = NULL;
    uint16_t vid = 0;
    struct LocalInterface *peer_link_if = NULL;
    int ln = 0;
    uint16_t vlan_id = 0;
    struct VLAN_ID vlan_key = { 0 };

    char buf[MAX_BUFSIZE] = { 0 };
    size_t msg_len = 0;

    struct LocalInterface *lif_po = NULL, *arp_lif = NULL;

    int verify_arp = 0;

    if (!(sys = system_get_instance()))
        return;

    /* Find local itf*/
    if (!(arp_lif = local_if_find_by_ifindex(ifindex)))
        return;

    /* create ARP msg*/
    msg_len = sizeof(struct ARPMsg);
    arp_msg = (struct ARPMsg*)&buf;
    arp_msg->op_type = NEIGH_SYNC_LIF;
    sprintf(arp_msg->ifname, "%s", arp_lif->name);
    memcpy(&arp_msg->ipv4_addr, &addr, 4);
    memcpy(arp_msg->mac_addr, mac_addr, 6);

    ICCPD_LOG_DEBUG(__FUNCTION__, "ARP ifindex [%d] (%s) ip %s mac [%02X:%02X:%02X:%02X:%02X:%02X]",
                    ifindex, arp_lif->name,
                    show_ip_str(arp_msg->ipv4_addr),
                    arp_msg->mac_addr[0], arp_msg->mac_addr[1], arp_msg->mac_addr[2], arp_msg->mac_addr[3], arp_msg->mac_addr[4],
                    arp_msg->mac_addr[5]);

    if (arp_lif->ipv4_addr == 0) {
        ICCPD_LOG_DEBUG(__FUNCTION__, "IP not configured on %s, ignore ARP", arp_lif->name);
        return;
    }

    uint8_t bcast_mac[ETHER_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    if (memcmp(arp_msg->mac_addr, bcast_mac, ETHER_ADDR_LEN) == 0)
    {
        ICCPD_LOG_DEBUG(__FUNCTION__, "Ignoring neighbor entry due to bcast lladdr");
        return;
    }

    if ((strncmp(arp_lif->name, VLAN_PREFIX, strlen(VLAN_PREFIX)) == 0)) {
        sscanf (arp_lif->name, "Vlan%hu", &vlan_id);
    }

    if (vlan_id) {
        memset(&vlan_key, 0, sizeof(struct VLAN_ID));
        vlan_key.vid = vlan_id;
    }
    /* Find MLACP itf, member of port-channel*/
    LIST_FOREACH(csm, &(sys->csm_list), next)
    {
        vid = 0;
        ln = __LINE__;
        LIST_FOREACH(lif_po, &(MLACP(csm).lif_list), mlacp_next)
        {
            if (lif_po->type != IF_T_PORT_CHANNEL) {
                ln = __LINE__;
                continue;
            }

            vid = 0;
            if (!local_if_is_l3_mode(lif_po))
            {
                /* Is the L2 MLAG itf belong to a vlan?*/
                vlan_id_list = RB_FIND(vlan_rb_tree, &(lif_po->vlan_tree), &vlan_key);

                if (!vlan_id_list) {
                    ln = __LINE__;
                    continue;
                }

                if (!vlan_id_list->vlan_itf) {
                    ln = __LINE__;
                    continue;
                }

                if (vlan_id_list->vlan_itf->ifindex != ifindex) {
                    ln = __LINE__;
                    continue;
                }

                vid = vlan_id_list->vid;
                ICCPD_LOG_DEBUG(__FUNCTION__, "ARP is from mclag enabled port %s of vlan %d",
                                              lif_po->name, vid);
            }
            else
            {
                /* Is the ARP belong to a L3 mode MLAG itf?*/
                if (ifindex != lif_po->ifindex) {
                    ln = __LINE__;
                    continue;
                }
                ICCPD_LOG_DEBUG(__FUNCTION__, "ARP is from mclag enabled intf %s", lif_po->name);
            }

            verify_arp = 1;

            break;
        }

        if (lif_po) {
            break;
        } else if (csm->peer_link_if){
           peer_link_if = csm->peer_link_if;
           if (!local_if_is_l3_mode(peer_link_if)) {
               vid = 0;
               vlan_id_list = RB_FIND(vlan_rb_tree, &(peer_link_if->vlan_tree), &vlan_key);

               if (vlan_id_list && vlan_id_list->vlan_itf) {
                   if (vlan_id_list->vlan_itf->ifindex == ifindex) {
                       vid = vlan_id_list->vid;
                       ICCPD_LOG_DEBUG(__FUNCTION__, "ARP is from peer link vlan %d", vid);
                       verify_arp = 1;
                       lif_po = peer_link_if;
                       break;
                   }
               }
           }
        }
    }

    if (!(csm && lif_po)) {
        ICCPD_LOG_DEBUG(__FUNCTION__, "ARP received no PO ln %d", ln);
        return;
    }
    if (!verify_arp) {
        ICCPD_LOG_DEBUG(__FUNCTION__, "ARP received no verify_arp ln %d", ln);
        return;
    }

    if (vid == 0) {
        if (iccp_check_if_addr_from_netlink(AF_INET, (uint8_t *)&addr, arp_lif))
        {
            ICCPD_LOG_DEBUG(__FUNCTION__, "ARP %s is identical with the ip address of interface %s",
                                          show_ip_str(arp_msg->ipv4_addr), arp_lif->name);
            return;
        }
    } else {
        if (vid && vlan_id_list && vlan_id_list->vlan_itf) {
            if (arp_msg->ipv4_addr == vlan_id_list->vlan_itf->ipv4_addr) {
                ICCPD_LOG_DEBUG(__FUNCTION__, "Ignore My ip %s", show_ip_str(arp_msg->ipv4_addr));
                return;
            }
        }
    }

    /* update lif ARP*/
    TAILQ_FOREACH(msg, &MLACP(csm).arp_list, tail)
    {
        arp_info = (struct ARPMsg*)msg->buf;
        if (arp_info->ipv4_addr != arp_msg->ipv4_addr)
            continue;

        /* update ARP*/
        if (arp_info->op_type != arp_msg->op_type
            || strcmp(arp_info->ifname, arp_msg->ifname) != 0
            || memcmp(arp_info->mac_addr, arp_msg->mac_addr, ETHER_ADDR_LEN) != 0)
        {
            arp_info->op_type = arp_msg->op_type;
            sprintf(arp_info->ifname, "%s", arp_msg->ifname);
            memcpy(arp_info->mac_addr, arp_msg->mac_addr, ETHER_ADDR_LEN);
            ICCPD_LOG_DEBUG(__FUNCTION__, "Update ARP for %s",
                            show_ip_str(arp_msg->ipv4_addr));
        }
        break;
    }

    /* enquene lif_msg (add)*/
    if (!msg)
    {
        arp_msg->op_type = NEIGH_SYNC_LIF;
        arp_msg->learn_flag = NEIGH_LOCAL;
        if (iccp_csm_init_msg(&msg, (char*)arp_msg, msg_len) == 0)
        {
            mlacp_enqueue_arp(csm, msg);
            /*ICCPD_LOG_DEBUG(__FUNCTION__, "ARP-list enqueue: %s, add %s",
                            arp_msg->ifname, show_ip_str(arp_msg->ipv4_addr));*/
        }
        else
            ICCPD_LOG_WARN(__FUNCTION__, "Failed to enqueue ARP-list: %s, add %s",
                            arp_msg->ifname, show_ip_str(arp_msg->ipv4_addr));
    }

    /* enqueue iccp_msg (add)*/
    if (MLACP(csm).current_state == MLACP_STATE_EXCHANGE)
    {
        arp_msg->op_type = NEIGH_SYNC_ADD;
        arp_msg->flag = 0;
        if (iccp_csm_init_msg(&msg_send, (char*)arp_msg, msg_len) == 0)
        {
            TAILQ_INSERT_TAIL(&(MLACP(csm).arp_msg_list), msg_send, tail);
            /*ICCPD_LOG_DEBUG(__FUNCTION__, "Enqueue ARP[ADD] for %s",
                            show_ip_str(arp_msg->ipv4_addr));*/
        }
        else
            ICCPD_LOG_WARN(__FUNCTION__, "Failed to enqueue ARP[ADD] message for %s",
                            show_ip_str(arp_msg->ipv4_addr));
    }

    return;
}

void do_ndisc_update_from_reply_packet(unsigned int ifindex, char *ipv6_addr, uint8_t mac_addr[ETHER_ADDR_LEN])
{
    struct System *sys = NULL;
    struct CSM *csm = NULL;
    struct Msg *msg = NULL;
    struct NDISCMsg *ndisc_msg = NULL, *ndisc_info = NULL;
    struct VLAN_ID *vlan_id_list = NULL;
    struct Msg *msg_send = NULL;
    char mac_str[18] = "";
    uint8_t null_mac[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint16_t vid = 0;
    int err = 0, ln = 0;
    struct LocalInterface *peer_link_if = NULL;
    int is_link_local = 0;

    char buf[MAX_BUFSIZE] = { 0 };
    size_t msg_len = 0;
    char addr_null[16] = { 0 };
    uint16_t vlan_id = 0;
    struct VLAN_ID vlan_key = { 0 };

    struct LocalInterface *lif_po = NULL, *ndisc_lif = NULL;

    int verify_ndisc = 0;

    if (!(sys = system_get_instance()))
        return;

    /* Find local itf */
    if (!(ndisc_lif = local_if_find_by_ifindex(ifindex)))
        return;

    sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

    /* create Ndisc msg */
    msg_len = sizeof(struct NDISCMsg);
    ndisc_msg = (struct NDISCMsg *)&buf;
    ndisc_msg->op_type = NEIGH_SYNC_LIF;
    sprintf(ndisc_msg->ifname, "%s", ndisc_lif->name);
    memcpy((char *)ndisc_msg->ipv6_addr, ipv6_addr, 16);
    memcpy(ndisc_msg->mac_addr, mac_addr, ETHER_ADDR_LEN);

    ICCPD_LOG_DEBUG(__FUNCTION__, "nd ifindex [%d] (%s) ip %s mac %s", ifindex, ndisc_lif->name, show_ipv6_str(ipv6_addr), mac_str);

    if (memcmp(ndisc_lif->ipv6_addr, addr_null, 16) == 0) {
        ICCPD_LOG_DEBUG(__FUNCTION__, "IPv6 address not configured on %s, ignore ND", ndisc_lif->name);
        return;
    }

    uint8_t bcast_mac[ETHER_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    if (memcmp(ndisc_msg->mac_addr, bcast_mac, ETHER_ADDR_LEN) == 0)
    {
        ICCPD_LOG_DEBUG(__FUNCTION__, "Ignoring neighbor entry due to bcast lladdr");
        return;
    }

    if ((strncmp(ndisc_lif->name, VLAN_PREFIX, strlen(VLAN_PREFIX)) == 0)) {
        sscanf (ndisc_lif->name, "Vlan%hu", &vlan_id);
    }

    if (vlan_id) {
        memset(&vlan_key, 0, sizeof(struct VLAN_ID));
        vlan_key.vid = vlan_id;
    }
    /* Find MLACP itf, member of port-channel */
    LIST_FOREACH(csm, &(sys->csm_list), next)
    {
        vid = 0;
        LIST_FOREACH(lif_po, &(MLACP(csm).lif_list), mlacp_next)
        {
            if (lif_po->type != IF_T_PORT_CHANNEL) {
                ln = __LINE__;
                continue;
            }

            vid = 0;
            if (!local_if_is_l3_mode(lif_po))
            {
                /* Is the L2 MLAG itf belong to a vlan?*/
                vlan_id_list = RB_FIND(vlan_rb_tree, &(lif_po->vlan_tree), &vlan_key);

                if (!vlan_id_list) {
                    ln = __LINE__;
                    continue;
                }

                if (!vlan_id_list->vlan_itf) {
                    ln = __LINE__;
                    continue;
                }

                if (vlan_id_list->vlan_itf->ifindex != ifindex) {
                    ln = __LINE__;
                    continue;
                }

                vid = vlan_id_list->vid;
            }
            else
            {
                /* Is the ND belong to a L3 mode MLAG itf? */
                if (ifindex != lif_po->ifindex) {
                    ln = __LINE__;
                    continue;
                }
                ICCPD_LOG_DEBUG(__FUNCTION__, "ND is from intf %s", lif_po->name);
            }

            verify_ndisc = 1;

            break;
        }

        if (lif_po) {
            break;
        } else if (csm->peer_link_if) {
           peer_link_if = csm->peer_link_if;
           if (!local_if_is_l3_mode(peer_link_if)) {
               vid = 0;
               vlan_id_list = RB_FIND(vlan_rb_tree, &(peer_link_if->vlan_tree), &vlan_key);

               if (vlan_id_list && vlan_id_list->vlan_itf) {
                   if (vlan_id_list->vlan_itf->ifindex == ifindex) {
                       vid = vlan_id_list->vid;
                       ICCPD_LOG_DEBUG(__FUNCTION__, "ND is from peer link vlan %d", vid);
                       verify_ndisc = 1;
                       lif_po = peer_link_if;
                       break;
                   }
               }
           }
        }
    }

    if (!(csm && lif_po)) {
        ICCPD_LOG_DEBUG(__FUNCTION__, "ND received no PO ln %d", ln);
        return;
    }
    if (!verify_ndisc) {
        ICCPD_LOG_DEBUG(__FUNCTION__, "ND received no verify_ndisc ln %d", ln);
        return;
    }

    if ((memcmp(show_ipv6_str((char *)ndisc_msg->ipv6_addr), "FE80", 4) == 0)
            || (memcmp(show_ipv6_str((char *)ndisc_msg->ipv6_addr), "fe80", 4) == 0))
    {
        is_link_local = 1;
    }

    if (vid && vlan_id_list && vlan_id_list->vlan_itf) {
        if (memcmp((char *)ndisc_msg->ipv6_addr, (char *)vlan_id_list->vlan_itf->ipv6_addr, 16) == 0)
        {
            ICCPD_LOG_DEBUG(__FUNCTION__, "Ignoring neighbor entry for My Ip %s", show_ipv6_str((char *)ndisc_msg->ipv6_addr));
            return;
        }

        if (is_link_local)
        {
            if (memcmp((char *)ndisc_msg->ipv6_addr, (char *)vlan_id_list->vlan_itf->ipv6_ll_addr, 16) == 0)
            {
                ICCPD_LOG_DEBUG(__FUNCTION__, "Ignoring neighbor entry for My Ip %s", show_ipv6_str((char *)ndisc_msg->ipv6_addr));
                return;
            }
        }
    }

    /* update lif ND */
    TAILQ_FOREACH(msg, &MLACP(csm).ndisc_list, tail)
    {
        ndisc_info = (struct NDISCMsg *)msg->buf;

        if (memcmp((char *)ndisc_info->ipv6_addr, (char *)ndisc_msg->ipv6_addr, 16) != 0)
            continue;

        /* If MAC addr is NULL, use the old one */
        if (memcmp(mac_addr, null_mac, ETHER_ADDR_LEN) == 0)
        {
            memcpy(ndisc_msg->mac_addr, ndisc_info->mac_addr, ETHER_ADDR_LEN);
            sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", ndisc_info->mac_addr[0], ndisc_info->mac_addr[1],
                    ndisc_info->mac_addr[2], ndisc_info->mac_addr[3], ndisc_info->mac_addr[4], ndisc_info->mac_addr[5]);
        }

        /* update ND */
        if (ndisc_info->op_type != ndisc_msg->op_type
            || strcmp(ndisc_info->ifname, ndisc_msg->ifname) != 0 || memcmp(ndisc_info->mac_addr, ndisc_msg->mac_addr, ETHER_ADDR_LEN) != 0)
        {
            ndisc_info->op_type = ndisc_msg->op_type;
            sprintf(ndisc_info->ifname, "%s", ndisc_msg->ifname);
            memcpy(ndisc_info->mac_addr, ndisc_msg->mac_addr, ETHER_ADDR_LEN);
             ICCPD_LOG_DEBUG(__FUNCTION__, "Update ND for %s", show_ipv6_str((char *)ndisc_msg->ipv6_addr));
        }
        break;
    }

    /* enquene lif_msg (add) */
    if (!msg)
    {
        /* If MAC addr is NULL, and same ipv6 item is not exist in ndisc_list */
        if (memcmp(mac_addr, null_mac, ETHER_ADDR_LEN) == 0)
        {
            return;
        }

        ndisc_msg->op_type = NEIGH_SYNC_LIF;
        ndisc_msg->learn_flag = NEIGH_LOCAL;
        if (iccp_csm_init_msg(&msg, (char *)ndisc_msg, msg_len) == 0)
        {
            mlacp_enqueue_ndisc(csm, msg);
            /* ICCPD_LOG_DEBUG(__FUNCTION__, "NDISC-list enqueue: %s, add %s", ndisc_msg->ifname, show_ipv6_str((char *)ndisc_msg->ipv6_addr)); */
        }
        else
            ICCPD_LOG_WARN(__FUNCTION__, "Failed to enqueue NDISC-list: %s, add %s", ndisc_msg->ifname, show_ipv6_str((char *)ndisc_msg->ipv6_addr));
    }

    ICCPD_LOG_DEBUG(__FUNCTION__, "add nd entry(%s, %s, %s) to kernel",
            ndisc_msg->ifname, show_ipv6_str((char *)ndisc_msg->ipv6_addr), mac_str);
    if ((err = iccp_netlink_neighbor_request(AF_INET6, (uint8_t *)ndisc_msg->ipv6_addr, 1, ndisc_msg->mac_addr, ndisc_msg->ifname, 0, 3)) < 0)
    {
        if (err != ICCP_NLE_SEQ_MISMATCH) {
            ICCPD_LOG_NOTICE(__FUNCTION__, "Failed to add nd entry(%s, %s, %s) to kernel, status %d",
                        ndisc_msg->ifname, show_ipv6_str((char *)ndisc_msg->ipv6_addr), mac_str, err);
            return;
        }
    }

    /* enqueue iccp_msg (add) */
    if (MLACP(csm).current_state == MLACP_STATE_EXCHANGE)
    {
        ndisc_msg->op_type = NEIGH_SYNC_ADD;
        ndisc_msg->flag = 0;
        if (iccp_csm_init_msg(&msg_send, (char *)ndisc_msg, msg_len) == 0)
        {
            TAILQ_INSERT_TAIL(&(MLACP(csm).ndisc_msg_list), msg_send, tail);
            /* ICCPD_LOG_DEBUG(__FUNCTION__, "Enqueue ND[ADD] for %s", show_ipv6_str((char *)ndisc_msg->ipv6_addr)); */
        }
        else
            ICCPD_LOG_WARN(__FUNCTION__, "Failed to enqueue ND[ADD] message for %s", show_ipv6_str((char *)ndisc_msg->ipv6_addr));
    }

    return;
}

void iccp_from_netlink_port_state_handler( char * ifname, int state)
{
    struct CSM *csm = NULL;
    struct LocalInterface *lif_po = NULL;
    struct System *sys;
    int po_is_active = 0;
    int is_mclag_intf = 0;

    if ((sys = system_get_instance()) == NULL)
    {
        ICCPD_LOG_WARN(__FUNCTION__, "Failed to obtain System instance.");
        return;
    }

    po_is_active = (state == PORT_STATE_UP);

    /* traverse all CSM */
    LIST_FOREACH(csm, &(sys->csm_list), next)
    {
        /*If peer-link changes to down or up */
        if (strcmp(ifname, csm->peer_itf_name) == 0)
        {
            if (po_is_active == 0)
                mlacp_peerlink_down_handler(csm);
            else
                mlacp_peerlink_up_handler(csm);

            break;
        }

        LIST_FOREACH(lif_po, &(MLACP(csm).lif_list), mlacp_next)
        {
            if (lif_po->type == IF_T_PORT_CHANNEL && strncmp(lif_po->name, ifname, MAX_L_PORT_NAME) == 0)
            {
                mlacp_portchannel_state_handler(csm, lif_po, po_is_active);
                is_mclag_intf = 1;
            }
        }
        if (!is_mclag_intf)
       {
           lif_po = local_if_find_by_name(ifname);
           update_orphan_port_mac(csm, lif_po, po_is_active);
       }

    }

    return;
}


//find pending vlan member interface
struct PendingVlanMbrIf* find_pending_vlan_mbr_if(struct System *sys, const char* ifname)
{
    struct PendingVlanMbrIf* mbr_if = NULL;

    if (!ifname || !sys)
        return NULL;

    LIST_FOREACH(mbr_if, &(sys->pending_vlan_mbr_if_list), if_next)
    {
        if (strcmp(mbr_if->name, ifname) == 0)
            return mbr_if;
    }

    return NULL;
}

//add specific vlan id to pending vlan membership interface
int add_pending_vlan_mbr(struct PendingVlanMbrIf* mbr_if, uint16_t vid)
{
    struct VLAN_ID *vlan = NULL;
    struct VLAN_ID vlan_key = { 0 };
    char vlan_name[16] = "";
    if (!mbr_if)
    {
        return MCLAG_ERROR;
    }

    sprintf(vlan_name, "Vlan%d", vid);
    memset(&vlan_key, 0, sizeof(struct VLAN_ID));
    vlan_key.vid = vid;

    vlan = RB_FIND(vlan_rb_tree, &(mbr_if->vlan_tree), &vlan_key);

    if (!vlan)
    {
        vlan = (struct VLAN_ID*)malloc(sizeof(struct VLAN_ID));
        if (!vlan)
        {
            ICCPD_LOG_WARN(__FUNCTION__, "Add Pending Vlan Member for If:%s Vlan:%d creation, malloc failed", mbr_if->name, vid);
            return MCLAG_ERROR;
        }

        memset(vlan, 0, sizeof(struct VLAN_ID));
        vlan->vid = vid;

        ICCPD_LOG_DEBUG(__FUNCTION__, "Add VLAN %d to pending vlan member if:%s ", vid, mbr_if->name);
        RB_INSERT(vlan_rb_tree, &(mbr_if->vlan_tree), vlan);
    }
    return 0;
}

//delete specific vlan id from pending vlan membership interface
void del_pending_vlan_mbr(struct PendingVlanMbrIf* mbr_if, uint16_t vid)
{
    struct VLAN_ID *vlan = NULL;
    struct VLAN_ID vlan_key = { 0 };
    if (!mbr_if)
    {
        return;
    }
    memset(&vlan_key, 0, sizeof(struct VLAN_ID));
    vlan_key.vid = vid;

    vlan = RB_FIND(vlan_rb_tree, &(mbr_if->vlan_tree), &vlan_key);

    if (vlan != NULL)
    {
        VLAN_RB_REMOVE(vlan_rb_tree, &(mbr_if->vlan_tree), vlan);
        free(vlan);
        ICCPD_LOG_DEBUG(__FUNCTION__, "Remove VLAN %d from pending vlan mbr If:%s ",vid, mbr_if->name);
    }
    return;
}


//delete all pending vlan members for a given vlan member interface
void del_all_pending_vlan_mbrs(struct PendingVlanMbrIf* lif)
{
    struct VLAN_ID* vlan = NULL;
    struct VLAN_ID* vlan_temp = NULL;

    ICCPD_LOG_DEBUG(__FUNCTION__, "Remove all Pending VLANs from %s", lif->name);
    RB_FOREACH_SAFE(vlan, vlan_rb_tree, &(lif->vlan_tree), vlan_temp)
    {
        VLAN_RB_REMOVE(vlan_rb_tree, &(lif->vlan_tree), vlan);
        free(vlan);
    }
    return;
}

//update vlan membership for a given interface; this function can be called
//whenever vlan membership is received from mclagsyncd and local interface is not found
void update_pending_vlan_mbr(char *mbr_if_name, unsigned int vlan_id,  int add_flag)
{
    struct System *sys = NULL;
    struct PendingVlanMbrIf *mbr_if;
    struct VLAN_ID* vlan = NULL;
    struct VLAN_ID* vlan_temp = NULL;

    if (!(sys = system_get_instance()))
    {
        return;
    }

    ICCPD_LOG_DEBUG(__FUNCTION__, "update pending vlan:%d member for if:%s event:%s \n", vlan_id, mbr_if_name, add_flag ? "add":"delete");
    mbr_if = find_pending_vlan_mbr_if(sys, mbr_if_name);
    //if mbr_if not found create
    if (!mbr_if)
    {
        if (!add_flag)
        {
            return;
        }

        ICCPD_LOG_DEBUG(__FUNCTION__, "Create pending vlan member if %s \n", mbr_if_name);
        if (!(mbr_if = (struct PendingVlanMbrIf *)malloc(sizeof(struct PendingVlanMbrIf))))
        {
            ICCPD_LOG_WARN(__FUNCTION__, "Pending Vlan Member If:%s creation, malloc failed", mbr_if_name);
            return;
        }
        snprintf(mbr_if->name, MAX_L_PORT_NAME, "%s", mbr_if_name);
        RB_INIT(vlan_rb_tree, &mbr_if->vlan_tree);
        LIST_INSERT_HEAD(&(sys->pending_vlan_mbr_if_list), mbr_if, if_next);
    }
    if (add_flag)
    {
        //add to pending vlan member if
        add_pending_vlan_mbr(mbr_if, vlan_id);
    }
    else
    {
        //delete from pending vlan member if
        del_pending_vlan_mbr(mbr_if, vlan_id);
    }
}


//move pending vlan membership from pending member interface to system lif
void move_pending_vlan_mbr_to_lif(struct System *sys, struct LocalInterface* lif)
{
    struct PendingVlanMbrIf *mbr_if;
    struct VLAN_ID* vlan = NULL;
    struct VLAN_ID* vlan_temp = NULL;
    if (!sys || !lif)
    {
        return;
    }
    ICCPD_LOG_NOTICE(__FUNCTION__, "Move pending vlan members for %s, %d\n", lif->name, lif->ifindex);
    mbr_if = find_pending_vlan_mbr_if(sys, lif->name);
    if (!mbr_if)
    {
        ICCPD_LOG_INFO(__FUNCTION__, "No pending vlan members for %s, %d\n", lif->name, lif->ifindex);
        return;
    }

    RB_FOREACH_SAFE(vlan, vlan_rb_tree, &(mbr_if->vlan_tree), vlan_temp)
    {
        //add vlan to system lif
        local_if_add_vlan(lif, vlan->vid);

        //delete from pending vlan member if
        del_pending_vlan_mbr(mbr_if, vlan->vid);
    }

    ICCPD_LOG_DEBUG(__FUNCTION__, "Delete pending vlan member if %s \n", lif->name);
    LIST_REMOVE(mbr_if, if_next);
    free(mbr_if);

}

//remove all pending vlan member interface and vlan memberships
void del_all_pending_vlan_mbr_ifs(struct System *sys)
{
    struct PendingVlanMbrIf *pending_vlan_mbr_if = NULL;

    ICCPD_LOG_NOTICE(__FUNCTION__, "Delete all pending vlan members \n");

    while (!LIST_EMPTY(&(sys->pending_vlan_mbr_if_list)))
    {
        pending_vlan_mbr_if = LIST_FIRST(&(sys->pending_vlan_mbr_if_list));
        LIST_REMOVE(pending_vlan_mbr_if, if_next);
        del_all_pending_vlan_mbrs(pending_vlan_mbr_if);
        free(pending_vlan_mbr_if);
    }
}

void vlan_mbrship_change_handler(unsigned int vlan_id, char *mbr_if_name, int add_flag)
{
    struct LocalInterface *lif = NULL;
    struct LocalInterface *vlan_lif = NULL;
    char vlan_name[16] = "";

    lif = local_if_find_by_name(mbr_if_name);
    if (!lif)
    {
        ICCPD_LOG_NOTICE(__FUNCTION__, "Rx vlan:%d mbr if:%s event %s; No MCLAG If", vlan_id, mbr_if_name, add_flag ? "add":"delete");
        update_pending_vlan_mbr(mbr_if_name, vlan_id, add_flag);
        return;
    }
    ICCPD_LOG_DEBUG(__FUNCTION__, "Rx vlan:%d mbr if:%s event %s", vlan_id, mbr_if_name, add_flag ? "add":"delete");

    if (add_flag) //vlan membership got added
    {
        sprintf(vlan_name, "Vlan%d", vlan_id);
        vlan_lif = local_if_find_by_name(vlan_name);
        if (!vlan_lif) {
            ICCPD_LOG_NOTICE(__FUNCTION__, "%s LIF not present", vlan_name);
            vlan_lif = local_if_create( -1, vlan_name, IF_T_VLAN, IF_OPER_DOWN);
            if (vlan_lif) {
                ICCPD_LOG_NOTICE(__FUNCTION__, "%s LIF created with ifindex -1", vlan_name);
            }
        }

        local_if_add_vlan(lif, vlan_id);
    }
    else //vlan membership got deleted
    {
        local_if_del_vlan(lif, vlan_id);
    }
    return;
}
