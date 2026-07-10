/*
 *  mlacp_link_handler.c
 *  mLACP link handler
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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/queue.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <linux/un.h>
#include <linux/if_arp.h>
#include <sys/ioctl.h>
#include "../include/system.h"
#include "../include/logger.h"
#include "../include/mlacp_tlv.h"

#include "../include/iccp_csm.h"
#include "mclagdctl/mclagdctl.h"
#include "../include/iccp_cmd_show.h"
#include "../include/iccp_cli.h"
#include "../include/iccp_cmd.h"
#include "../include/mlacp_link_handler.h"
#include "../include/mlacp_sync_prepare.h"
#include "../include/iccp_netlink.h"
#include "../include/scheduler.h"
#include "../include/iccp_ifm.h"

/*****************************************
* Enum
*
* ***************************************/
typedef enum route_manipulate_type
{
    ROUTE_NONE,
    ROUTE_ADD,
    ROUTE_DEL
} ROUTE_MANIPULATE_TYPE_E;

/*****************************************
* Global
*
* ***************************************/
char g_ipv4_str[INET_ADDRSTRLEN];
char g_ipv6_str[INET6_ADDRSTRLEN];
char g_iccp_mlagsyncd_recv_buf[ICCP_MLAGSYNCD_RECV_MSG_BUFFER_SIZE] = { 0 };
char g_iccp_mlagsyncd_send_buf[ICCP_MLAGSYNCD_SEND_MSG_BUFFER_SIZE] = { 0 };


extern void mlacp_sync_mac(struct CSM* csm);

#define SYNCD_SEND_RETRY_INTERVAL_USEC    50000 //50 mseconds
#define SYNCD_SEND_RETRY_MAX              5

#define SYNCD_RECV_RETRY_INTERVAL_USEC    50000 //50 mseconds
#define SYNCD_RECV_RETRY_MAX              5

/*****************************************
* Tool : show ip string
*
* ***************************************/
char *show_ip_str(uint32_t ipv4_addr)
{
    struct in_addr in_addr;

    memset(g_ipv4_str, 0, sizeof(g_ipv4_str));
    in_addr.s_addr = ipv4_addr;
    inet_ntop(AF_INET, &in_addr, g_ipv4_str, INET_ADDRSTRLEN);

    return g_ipv4_str;
}

char *show_ipv6_str(char *ipv6_addr)
{
    memset(g_ipv6_str, 0, sizeof(g_ipv6_str));
    inet_ntop(AF_INET6, ipv6_addr, g_ipv6_str, INET6_ADDRSTRLEN);

    return g_ipv6_str;
}

static int getHwAddr(char *buff, char *mac)
{
    int i = 0;
    unsigned int p[6];

    if ( buff == NULL || mac == NULL )
    {
        return MCLAG_ERROR;
    }

    if (sscanf(mac, "%x:%x:%x:%x:%x:%x", &p[0], &p[1], &p[2], &p[3], &p[4], &p[5]) < 6)
    {
        return MCLAG_ERROR;
    }

    for (i = 0; i < 6; i++)
    {
        buff[i] = p[i];
    }

    return 0;
}

static int arp_set_handler(struct CSM* csm,
                           struct LocalInterface* lif,
                           int add)
{
    struct Msg* msg = NULL;
    struct ARPMsg* arp_msg = NULL;
    char mac_str[18] = "";
    int err = 0;

    if (!csm || !lif)
        return 0;

    if (add)
        goto add_arp;
    else
        goto del_arp;

    /* Process Add */
add_arp:
    if (MLACP(csm).current_state != MLACP_STATE_EXCHANGE)
        return 0;

    TAILQ_FOREACH(msg, &MLACP(csm).arp_list, tail)
    {
        mac_str[0] = '\0';
        arp_msg = (struct ARPMsg*)msg->buf;

        /* only process add*/
        if (arp_msg->op_type == NEIGH_SYNC_DEL)
            continue;

        /* find the ARP for lif_list*/
        if (strcmp(lif->name, arp_msg->ifname) != 0)
            continue;

        sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", arp_msg->mac_addr[0], arp_msg->mac_addr[1], arp_msg->mac_addr[2],
                arp_msg->mac_addr[3], arp_msg->mac_addr[4], arp_msg->mac_addr[5]);

        err = iccp_netlink_neighbor_request(AF_INET, (uint8_t *)&arp_msg->ipv4_addr, 1, arp_msg->mac_addr, arp_msg->ifname, 0, 4);
        ICCPD_LOG_NOTICE(__FUNCTION__, "Add dynamic ARP to kernel [%s], status %d", show_ip_str(arp_msg->ipv4_addr), err);
    }
    goto done;

del_arp:
    /* Process Del */
    TAILQ_FOREACH(msg, &MLACP(csm).arp_list, tail)
    {
        arp_msg = (struct ARPMsg*)msg->buf;

        /* find the ARP for lif_list*/
        if (strcmp(lif->name, arp_msg->ifname) != 0)
            continue;

        /* don't process del*/
        if (arp_msg->op_type == NEIGH_SYNC_DEL)
            continue;

        err = iccp_netlink_neighbor_request(AF_INET, (uint8_t *)&arp_msg->ipv4_addr, 0, arp_msg->mac_addr, arp_msg->ifname, 0, 5);
        /* link broken, del all dynamic arp on the lif */
        ICCPD_LOG_NOTICE(__FUNCTION__, "Del dynamic ARP [%s], status %d", show_ip_str(arp_msg->ipv4_addr), err);
    }

done:
    return 0;
}

static int ndisc_set_handler(struct CSM *csm, struct LocalInterface *lif, int add)
{
    struct Msg *msg = NULL;
    struct NDISCMsg *ndisc_msg = NULL;
    char mac_str[18] = "";
    int err = 0;

    if (!csm || !lif)
        return 0;

    if (add)
        goto add_ndisc;
    else
        goto del_ndisc;

    /* Process Add */
add_ndisc:
    if (MLACP(csm).current_state != MLACP_STATE_EXCHANGE)
        return 0;

    TAILQ_FOREACH(msg, &MLACP(csm).ndisc_list, tail)
    {
        mac_str[0] = '\0';
        ndisc_msg = (struct NDISCMsg *)msg->buf;

        /* only process add */
        if (ndisc_msg->op_type == NEIGH_SYNC_DEL)
            continue;

        /* find the ND for lif_list */
        if (strcmp(lif->name, ndisc_msg->ifname) != 0)
            continue;

        sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", ndisc_msg->mac_addr[0], ndisc_msg->mac_addr[1], ndisc_msg->mac_addr[2],
                ndisc_msg->mac_addr[3], ndisc_msg->mac_addr[4], ndisc_msg->mac_addr[5]);

        err = iccp_netlink_neighbor_request(AF_INET6, (uint8_t *)ndisc_msg->ipv6_addr, 1, ndisc_msg->mac_addr, ndisc_msg->ifname, 0, 6);
        ICCPD_LOG_NOTICE(__FUNCTION__, "Add dynamic ND to kernel [%s], status %d", show_ipv6_str((char *)ndisc_msg->ipv6_addr), err);
    }
    goto done;

del_ndisc:
    /* Process Del */
    TAILQ_FOREACH(msg, &MLACP(csm).ndisc_list, tail)
    {
        ndisc_msg = (struct NDISCMsg *)msg->buf;

        /* find the ND for lif_list */
        if (strcmp(lif->name, ndisc_msg->ifname) != 0)
            continue;

        /* don't process del */
        if (ndisc_msg->op_type == NEIGH_SYNC_DEL)
            continue;

        err = iccp_netlink_neighbor_request(AF_INET6, (uint8_t *)ndisc_msg->ipv6_addr, 1, ndisc_msg->mac_addr, ndisc_msg->ifname, 0, 7);

        /* link broken, del all dynamic ndisc on the lif */
        ICCPD_LOG_NOTICE(__FUNCTION__, "Del dynamic ND [%s], status %d", show_ipv6_str((char *)ndisc_msg->ipv6_addr), err);
    }

done:
    return 0;
}

/*****************************************
 * Port-Channel Status Handler
 *
 ****************************************/
static void update_vlan_if_info(struct CSM *csm,
                                struct LocalInterface *local_if,
                                struct LocalInterface *vlan_if,
                                int po_state)
{
    if (!csm || !local_if || !vlan_if)
        return;

    vlan_if->mlacp_state = MLACP(csm).current_state;

    return;
}

static void update_l3_if_info(struct CSM *csm,
                              struct LocalInterface *local_if,
                              struct LocalInterface *l3_if,
                              int po_state)
{
    if (!csm || !l3_if)
        return;

    l3_if->mlacp_state = MLACP(csm).current_state;

    return;
}

static void update_po_if_info(struct CSM *csm,
                              struct LocalInterface *local_if,
                              int po_state)
{
    if (!csm || !local_if)
        return;

    /* update local po info*/
    if (local_if->po_active != po_state)
    {
        local_if->changed = 1;
        local_if->po_active = (po_state != 0);

        /*printf("update po [%s=%d]\n",local_if->name, local_if->po_active);*/
    }
    local_if->mlacp_state = MLACP(csm).current_state;

    return;
}

static void set_l3_itf_state(struct CSM *csm,
                             struct LocalInterface *set_l3_local_if,
                             ROUTE_MANIPULATE_TYPE_E route_type)
{
    if (!csm || !set_l3_local_if)
        return;

    if (set_l3_local_if && (route_type != ROUTE_NONE))
    {
        /*set_default_route(csm);*/

        /*ICCPD_LOG_DEBUG(__FUNCTION__, "  route set Interface = %s   route type = %d   route = %s   nexthop via = %s ",
                        set_l3_local_if->name, route_type, show_ip_str(htonl(set_l3_local_if->ipv4_addr)), csm->peer_ip );*/

        /* set static route*/
        if (route_type == ROUTE_ADD)
        {
            /*If the L3 intf is not Vlan, del ARP; else wait ARP age*/
            if (strncmp(set_l3_local_if->name, VLAN_PREFIX, 4) != 0)
            {
                arp_set_handler(csm, set_l3_local_if, 0);     /* del arp*/
                ndisc_set_handler(csm, set_l3_local_if, 0);     /* del nd */
            }
        }
        else if (route_type == ROUTE_DEL)
        {
            arp_set_handler(csm, set_l3_local_if, 1);     /* add arp*/
            ndisc_set_handler(csm, set_l3_local_if, 1); /* add nd */
        }
    }

    return;
}

static int peer_po_is_alive(struct CSM *csm, int po_ifindex)
{
    struct PeerInterface *pif = NULL;
    int pif_active = 0;

    if (!csm)
        return 0;

    LIST_FOREACH(pif, &(MLACP(csm).pif_list), mlacp_next)
    {
        if (pif->type != IF_T_PORT_CHANNEL)
            continue;
        if (pif->po_id != po_ifindex)
            continue;

        if (pif->po_active)
            pif_active = 1; /*pif alive*/
        break;
    }

    return pif_active;
}

// return -1 if failed
ssize_t iccp_send_to_mclagsyncd(uint8_t msg_type, char *send_buff, uint16_t msg_len)
{
    struct System *sys;
    ssize_t write = 0;
    int num_retry = 0;
    size_t pos = 0;
    int send_len = 0;

    sys = system_get_instance();
    if (sys == NULL)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Invalid system instance");
        return MCLAG_ERROR;
    }

    if (sys->sync_fd)
    {
        while (msg_len > 0)
        {
            send_len = send(sys->sync_fd, &send_buff[pos], msg_len, MSG_DONTWAIT);

            if (send_len == -1)
            {
                if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                {
                    ++num_retry;
                    if (num_retry > SYNCD_SEND_RETRY_MAX)
                    {
                        ICCPD_LOG_ERR("ICCP_FSM", "Send to mclagsyncd Non-blocking send() retry failed,msg_type: %d msg_len/send_len %d/%d",
                                msg_type, msg_len, send_len);
                        SYSTEM_SET_SYNCD_TX_DBG_COUNTER(
                                            sys, msg_type, ICCP_DBG_CNTR_STS_ERR);
                        return MCLAG_ERROR;
                    }
                    else
                    {
                        usleep(SYNCD_SEND_RETRY_INTERVAL_USEC);
                        send_len = 0;
                    }
                }
                else
                {
                    ICCPD_LOG_ERR("ICCP_FSM", "Send to mclagsyncd Non-blocking send() failed, msg_type: %d errno %d",
                            msg_type, errno);
                    SYSTEM_SET_SYNCD_TX_DBG_COUNTER(sys, msg_type, ICCP_DBG_CNTR_STS_ERR);
                    return MCLAG_ERROR;
                }
            }
            else if (send_len == 0)
            {
                ICCPD_LOG_ERR("ICCP_FSM", "Send to mclagsyncd Non-blocking send() failed socket closed msg_type: %d errno %d",
                        msg_type, errno);
                SYSTEM_SET_SYNCD_TX_DBG_COUNTER(
                                    sys, msg_type, ICCP_DBG_CNTR_STS_ERR);
                return MCLAG_ERROR;
            }
            msg_len -= send_len;
            pos += send_len;
        }
        SYSTEM_SET_SYNCD_TX_DBG_COUNTER(sys, msg_type, ICCP_DBG_CNTR_STS_OK);
    }

    return pos;

}

#if 0
static void mlacp_clean_fdb(void)
{
    struct IccpSyncdHDr * msg_hdr;
    char *msg_buf = g_iccp_mlagsyncd_send_buf;
    ssize_t rc;
    struct System *sys;

    sys = system_get_instance();
    if (sys == NULL)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Invalid system instance");
        return;
    }
    memset(msg_buf, 0, ICCP_MLAGSYNCD_SEND_MSG_BUFFER_SIZE);
    msg_hdr = (struct IccpSyncdHDr *)msg_buf;
    msg_hdr->ver = ICCPD_TO_MCLAGSYNCD_HDR_VERSION;
    msg_hdr->type = MCLAG_MSG_TYPE_FLUSH_FDB;
    msg_hdr->len = sizeof(struct IccpSyncdHDr);

    if (sys->sync_fd)
    {
        rc = iccp_send_to_mclagsyncd(msg_hdr->type, msg_buf, msg_hdr->len);

        if (rc <= 0)
        {
            ICCPD_LOG_WARN(__FUNCTION__, "Send to Mclagsyncd failed rc: %d",rc);
        }

    }
    ICCPD_LOG_DEBUG(__FUNCTION__, "Notify mclagsyncd to clear FDB");
    return;
}
#endif

void set_peerlink_mlag_port_learn(struct LocalInterface *lif, int enable)
{
    struct IccpSyncdHDr * msg_hdr;
    mclag_sub_option_hdr_t * sub_msg;
    char *msg_buf = g_iccp_mlagsyncd_send_buf;
    int msg_len;
    struct System *sys;
    ssize_t rc;

    sys = system_get_instance();
    if (sys == NULL)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Invalid system instance");
        return;
    }

    if (!lif)
        return;
    memset(msg_buf, 0, ICCP_MLAGSYNCD_SEND_MSG_BUFFER_SIZE);
    msg_hdr = (struct IccpSyncdHDr *)msg_buf;
    msg_hdr->ver = ICCPD_TO_MCLAGSYNCD_HDR_VERSION;
    msg_hdr->type = MCLAG_MSG_TYPE_PORT_MAC_LEARN_MODE;

    msg_hdr->len = sizeof(struct IccpSyncdHDr);

    sub_msg = (mclag_sub_option_hdr_t*)&msg_buf[msg_hdr->len];
    sub_msg->op_type = MCLAG_SUB_OPTION_TYPE_MAC_LEARN_DISABLE;

    if (enable)
        sub_msg->op_type = MCLAG_SUB_OPTION_TYPE_MAC_LEARN_ENABLE;

    msg_len = strlen(lif->name);
    memcpy(sub_msg->data, lif->name, msg_len);

    sub_msg->op_len = msg_len;
    msg_hdr->len += sizeof(mclag_sub_option_hdr_t);
    msg_hdr->len += sub_msg->op_len;

    ICCPD_LOG_DEBUG(__FUNCTION__, "Send %s port MAC learn msg to mclagsyncd for %s",
                    sub_msg->op_type == MCLAG_SUB_OPTION_TYPE_MAC_LEARN_DISABLE ? "DISABLE":"ENABLE", lif->name);

    /*send msg*/
    if (sys->sync_fd)
    {
        rc = write(sys->sync_fd,msg_buf, msg_hdr->len);
        if ((rc <= 0) || (rc != msg_hdr->len))
        {
            SYSTEM_SET_SYNCD_TX_DBG_COUNTER(
                sys, msg_hdr->type, ICCP_DBG_CNTR_STS_ERR);
            ICCPD_LOG_ERR(__FUNCTION__, "Failed to write for %s, rc %d",
                lif->name, rc);
        }
        else
        {
            SYSTEM_SET_SYNCD_TX_DBG_COUNTER(
                sys, msg_hdr->type, ICCP_DBG_CNTR_STS_OK);
        }
    }
    return;
}

/* Send request to Mclagsyncd to enable or disable traffic on
 * MLAG interface
 */
static int mlacp_link_set_traffic_dist_mode(
    char                    *po_name,
    bool                    is_enable)
{
    struct IccpSyncdHDr     *msg_hdr;
    mclag_sub_option_hdr_t  *sub_msg;
    char                    *msg_buf = g_iccp_mlagsyncd_send_buf;
    int                     msg_len;
    struct System           *sys;
    ssize_t                 rc = 0;

    sys = system_get_instance();
    if (sys == NULL)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Invalid system instance");
        return MCLAG_ERROR;
    }

    memset(msg_buf, 0, ICCP_MLAGSYNCD_SEND_MSG_BUFFER_SIZE);
    msg_hdr = (struct IccpSyncdHDr *)msg_buf;
    msg_hdr->ver = ICCPD_TO_MCLAGSYNCD_HDR_VERSION;
    msg_hdr->type = is_enable ?
        MCLAG_MSG_TYPE_SET_TRAFFIC_DIST_ENABLE :
        MCLAG_MSG_TYPE_SET_TRAFFIC_DIST_DISABLE;
    msg_hdr->len = sizeof(struct IccpSyncdHDr);

    /* Sub-message: port-channel name */
    sub_msg =(mclag_sub_option_hdr_t*) &msg_buf[msg_hdr->len];
    sub_msg->op_type = MCLAG_SUB_OPTION_TYPE_MCLAG_INTF_NAME;
    sub_msg->op_len = strlen(po_name);
    memcpy(sub_msg->data, po_name, sub_msg->op_len);

    msg_hdr->len += (sizeof(mclag_sub_option_hdr_t) + sub_msg->op_len);

    if (sys->sync_fd)
        rc = iccp_send_to_mclagsyncd(msg_hdr->type, msg_buf, msg_hdr->len);

    if ((rc <= 0) || (rc != msg_hdr->len))
    {
        ICCPD_LOG_ERR(__FUNCTION__,
            "Failed to write traffic %s for %s, rc %d",
            is_enable ? "enable" : "disable", po_name, rc);
        return MCLAG_ERROR;
    }
    else
    {
        ICCPD_LOG_DEBUG(__FUNCTION__, "%s traffic dist for interface %s",
            is_enable ? "Enable" : "Disable", po_name);
        return 0;
    }
}

/* Send request to Mclagsyncd to update ICCP state
 * The message includes MLAG id and ICCP state
 */
int mlacp_link_set_iccp_state(
    int                     mlag_id,
    bool                    is_oper_up)
{
    struct IccpSyncdHDr     *msg_hdr;
    mclag_sub_option_hdr_t  *sub_msg;
    char                    *msg_buf = g_iccp_mlagsyncd_send_buf;
    struct System           *sys;
    ssize_t                 rc = 0;

    sys = system_get_instance();
    if (sys == NULL)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Invalid system instance");
        return MCLAG_ERROR;
    }
    /* On startup, session down processing is triggered as part of
     * peer link info setting before the socket to Mclagsyncd is setup.
     * Check for valid socket to log a notification instead of an error
     */
    if ((sys->sync_fd <= 0) && (!is_oper_up))
    {
        ICCPD_LOG_NOTICE(__FUNCTION__,
            "Unconnected socket to Mclagsyncd, skip mlag %d ICCP down update",
            mlag_id);
        return MCLAG_ERROR;
    }
    memset(msg_buf, 0, ICCP_MLAGSYNCD_SEND_MSG_BUFFER_SIZE);
    msg_hdr = (struct IccpSyncdHDr *)msg_buf;
    msg_hdr->ver = ICCPD_TO_MCLAGSYNCD_HDR_VERSION;
    msg_hdr->type = MCLAG_MSG_TYPE_SET_ICCP_STATE;
    msg_hdr->len = sizeof(struct IccpSyncdHDr);

    /* Sub-message: mlag ID */
    sub_msg = (mclag_sub_option_hdr_t *)&msg_buf[msg_hdr->len];
    sub_msg->op_type = MCLAG_SUB_OPTION_TYPE_MCLAG_ID;
    sub_msg->op_len = sizeof(mlag_id);
    memcpy(sub_msg->data, &mlag_id, sub_msg->op_len);
    msg_hdr->len += (sizeof(mclag_sub_option_hdr_t) + sub_msg->op_len);

    /* Sub-message: operational status */
    sub_msg = (mclag_sub_option_hdr_t *)&msg_buf[msg_hdr->len];
    sub_msg->op_type = MCLAG_SUB_OPTION_TYPE_OPER_STATUS;
    sub_msg->op_len = sizeof(is_oper_up);
    memcpy(sub_msg->data, &is_oper_up, sub_msg->op_len);
    msg_hdr->len += (sizeof(mclag_sub_option_hdr_t) + sub_msg->op_len);

    if (sys->sync_fd)
        rc = iccp_send_to_mclagsyncd(msg_hdr->type, msg_buf, msg_hdr->len);

    if ((rc <= 0) || (rc != msg_hdr->len))
    {
        ICCPD_LOG_ERR(__FUNCTION__,
            "Failed to write mlag %d, ICCP status %s, rc %d",
            mlag_id, is_oper_up ? "up" : "down", rc);
        return MCLAG_ERROR;
    }
    else
    {
        ICCPD_LOG_DEBUG(__FUNCTION__, "Set mlag %d, ICCP status %s",
            mlag_id, is_oper_up ? "up" : "down");
        return 0;
    }
}

/* Send request to Mclagsyncd to update ICCP role
 * The message includes MLAG id and ICCP active/standby role. System ID
 * is also included for active role
 */
int mlacp_link_set_iccp_role(
    int                     mlag_id,
    bool                    is_active_role,
    uint8_t                 *system_id)
{
    struct IccpSyncdHDr     *msg_hdr;
    mclag_sub_option_hdr_t  *sub_msg;
    char                    *msg_buf = g_iccp_mlagsyncd_send_buf;
    struct System           *sys;
    ssize_t                 rc = 0;

    sys = system_get_instance();
    if (sys == NULL)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Invalid system instance");
        return MCLAG_ERROR;
    }

    memset(msg_buf, 0, ICCP_MLAGSYNCD_SEND_MSG_BUFFER_SIZE);
    msg_hdr = (struct IccpSyncdHDr *)msg_buf;
    msg_hdr->ver = ICCPD_TO_MCLAGSYNCD_HDR_VERSION;
    msg_hdr->type = MCLAG_MSG_TYPE_SET_ICCP_ROLE;
    msg_hdr->len = sizeof(struct IccpSyncdHDr);

    /* Sub-message: mlag ID */
    sub_msg = (mclag_sub_option_hdr_t *)&msg_buf[msg_hdr->len];
    sub_msg->op_type = MCLAG_SUB_OPTION_TYPE_MCLAG_ID;
    sub_msg->op_len = sizeof(mlag_id);
    memcpy(sub_msg->data, &mlag_id, sub_msg->op_len);
    msg_hdr->len += (sizeof(mclag_sub_option_hdr_t) + sub_msg->op_len);

    /* Sub-message: active/standby role */
    sub_msg = (mclag_sub_option_hdr_t *)&msg_buf[msg_hdr->len];
    sub_msg->op_type = MCLAG_SUB_OPTION_TYPE_ICCP_ROLE;
    sub_msg->op_len = sizeof(is_active_role);
    memcpy(sub_msg->data, &is_active_role, sub_msg->op_len);
    msg_hdr->len += (sizeof(mclag_sub_option_hdr_t) + sub_msg->op_len);

    /* Sub-message: system ID if it is active role */
    if (is_active_role)
    {
        sub_msg = (mclag_sub_option_hdr_t *)&msg_buf[msg_hdr->len];
        sub_msg->op_type = MCLAG_SUB_OPTION_TYPE_SYSTEM_ID;
        sub_msg->op_len = ETHER_ADDR_LEN;
        memcpy(sub_msg->data, system_id, sub_msg->op_len);
        msg_hdr->len += (sizeof(mclag_sub_option_hdr_t) + sub_msg->op_len);
    }
    if (sys->sync_fd)
        rc = iccp_send_to_mclagsyncd(msg_hdr->type, msg_buf, msg_hdr->len);

    if ((rc <= 0) || (rc != msg_hdr->len))
    {
        ICCPD_LOG_ERR(__FUNCTION__,
            "Failed to write mlag %d, ICCP role %s, rc %d",
            mlag_id, is_active_role ? "active" : "standby", rc);
        return MCLAG_ERROR;
    }
    else
    {
        ICCPD_LOG_DEBUG(__FUNCTION__, "Set mlag %d, ICCP role to %s",
            mlag_id, is_active_role ? "active" : "standby");
        return 0;
    }
}

/* Send request to Mclagsyncd to update ICCP system ID
 * The message includes MLAG id and system id
 */
int mlacp_link_set_iccp_system_id(
    int                     mlag_id,
    uint8_t                 *system_id)
{
    struct IccpSyncdHDr     *msg_hdr;
    mclag_sub_option_hdr_t  *sub_msg;
    char                    *msg_buf = g_iccp_mlagsyncd_send_buf;
    struct System           *sys;
    ssize_t                 rc = 0;

    sys = system_get_instance();
    if (sys == NULL)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Invalid system instance");
        return MCLAG_ERROR;
    }

    memset(msg_buf, 0, ICCP_MLAGSYNCD_SEND_MSG_BUFFER_SIZE);
    msg_hdr = (struct IccpSyncdHDr *)msg_buf;
    msg_hdr->ver = ICCPD_TO_MCLAGSYNCD_HDR_VERSION;
    msg_hdr->type = MCLAG_MSG_TYPE_SET_ICCP_SYSTEM_ID;
    msg_hdr->len = sizeof(struct IccpSyncdHDr);

    /* Sub-message: mlag ID */
    sub_msg = (mclag_sub_option_hdr_t *)&msg_buf[msg_hdr->len];
    sub_msg->op_type = MCLAG_SUB_OPTION_TYPE_MCLAG_ID;
    sub_msg->op_len = sizeof(mlag_id);
    memcpy(sub_msg->data, &mlag_id, sub_msg->op_len);
    msg_hdr->len += (sizeof(mclag_sub_option_hdr_t) + sub_msg->op_len);

    /* Sub-message: system ID */
    sub_msg = (mclag_sub_option_hdr_t*)&msg_buf[msg_hdr->len];
    sub_msg->op_type = MCLAG_SUB_OPTION_TYPE_SYSTEM_ID;
    sub_msg->op_len = ETHER_ADDR_LEN;
    memcpy(sub_msg->data, system_id, sub_msg->op_len);
    msg_hdr->len += (sizeof(mclag_sub_option_hdr_t) + sub_msg->op_len);

    if (sys->sync_fd)
        rc = iccp_send_to_mclagsyncd(msg_hdr->type, msg_buf, msg_hdr->len);

    if ((rc <= 0) || (rc != msg_hdr->len))
    {
        ICCPD_LOG_ERR(__FUNCTION__,
            "Failed to write mlag %d, ICCP system ID %s, rc %d",
            mlag_id, mac_addr_to_str(system_id), rc);
        return MCLAG_ERROR;
    }
    else
    {
        ICCPD_LOG_DEBUG(__FUNCTION__,
            "Set mlag %d, ICCP system ID to %s",
            mlag_id, mac_addr_to_str(system_id));
        return 0;
    }
}

/* Send request to Mclagsyncd to remove MCLAG table entry.
 * The message includes MLAG id
 */
int mlacp_link_del_iccp_info(
    int                     mlag_id)
{
    struct IccpSyncdHDr     *msg_hdr;
    mclag_sub_option_hdr_t  *sub_msg;
    char                    *msg_buf = g_iccp_mlagsyncd_send_buf;
    struct System           *sys;
    ssize_t                 rc = 0;

    sys = system_get_instance();
    if (sys == NULL)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Invalid system instance");
        return MCLAG_ERROR;
    }

    memset(msg_buf, 0, ICCP_MLAGSYNCD_SEND_MSG_BUFFER_SIZE);
    msg_hdr = (struct IccpSyncdHDr *)msg_buf;
    msg_hdr->ver = ICCPD_TO_MCLAGSYNCD_HDR_VERSION;
    msg_hdr->type = MCLAG_MSG_TYPE_DEL_ICCP_INFO;
    msg_hdr->len = sizeof(struct IccpSyncdHDr);

    /* Sub-message: mlag ID */
    sub_msg = (mclag_sub_option_hdr_t *)&msg_buf[msg_hdr->len];
    sub_msg->op_type = MCLAG_SUB_OPTION_TYPE_MCLAG_ID;
    sub_msg->op_len = sizeof(mlag_id);
    memcpy(sub_msg->data, &mlag_id, sub_msg->op_len);
    msg_hdr->len += (sizeof(mclag_sub_option_hdr_t) + sub_msg->op_len);

    if (sys->sync_fd)
        rc = send(sys->sync_fd,msg_buf, msg_hdr->len, MSG_DONTWAIT);

    if ((rc <= 0) || (rc != msg_hdr->len))
    {
        ICCPD_LOG_ERR(__FUNCTION__,
            "Failed to write mlag %d delete request, rc %d", mlag_id, rc);
        return MCLAG_ERROR;
    }
    else
    {
        SYSTEM_SET_SYNCD_TX_DBG_COUNTER(sys, msg_hdr->type, ICCP_DBG_CNTR_STS_OK);
        ICCPD_LOG_DEBUG("ICCP_FSM", "Delete mlag %d", mlag_id);
        return 0;
    }
}


/* Send request to Mclagsyncd to update remote interface state
 * The message includes MLAG id, LAG interface name and operational status
 */
int mlacp_link_set_remote_if_state(
    int                     mlag_id,
    char                    *po_name,
    bool                    is_oper_up)
{
    struct IccpSyncdHDr     *msg_hdr;
    mclag_sub_option_hdr_t  *sub_msg;
    char                    *msg_buf = g_iccp_mlagsyncd_send_buf;
    struct System           *sys;
    ssize_t                 rc = 0;

    sys = system_get_instance();
    if (sys == NULL)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Invalid system instance");
        return MCLAG_ERROR;
    }

    memset(msg_buf, 0, ICCP_MLAGSYNCD_SEND_MSG_BUFFER_SIZE);
    msg_hdr = (struct IccpSyncdHDr *)msg_buf;
    msg_hdr->ver = ICCPD_TO_MCLAGSYNCD_HDR_VERSION;
    msg_hdr->type = MCLAG_MSG_TYPE_SET_REMOTE_IF_STATE;
    msg_hdr->len = sizeof(struct IccpSyncdHDr);

    /* Sub-message: mlag ID */
    sub_msg = (mclag_sub_option_hdr_t *)&msg_buf[msg_hdr->len];
    sub_msg->op_type = MCLAG_SUB_OPTION_TYPE_MCLAG_ID;
    sub_msg->op_len = sizeof(mlag_id);
    memcpy(sub_msg->data, &mlag_id, sub_msg->op_len);
    msg_hdr->len += (sizeof(mclag_sub_option_hdr_t) + sub_msg->op_len);

    /* Sub-message: MLAG interface name */
    sub_msg = (mclag_sub_option_hdr_t *)&msg_buf[msg_hdr->len];
    sub_msg->op_type = MCLAG_SUB_OPTION_TYPE_MCLAG_INTF_NAME;
    sub_msg->op_len = strlen(po_name);
    memcpy(sub_msg->data, po_name, sub_msg->op_len);
    msg_hdr->len += (sizeof(mclag_sub_option_hdr_t) + sub_msg->op_len);

    /* Sub-message: operational status */
    sub_msg = (mclag_sub_option_hdr_t *)&msg_buf[msg_hdr->len];
    sub_msg->op_type = MCLAG_SUB_OPTION_TYPE_OPER_STATUS;
    sub_msg->op_len = sizeof(is_oper_up);
    memcpy(sub_msg->data, &is_oper_up, sub_msg->op_len);
    msg_hdr->len += (sizeof(mclag_sub_option_hdr_t) + sub_msg->op_len);

    if (sys->sync_fd)
        rc = iccp_send_to_mclagsyncd(msg_hdr->type, msg_buf, msg_hdr->len);

    if ((rc <= 0) || (rc != msg_hdr->len))
    {
        ICCPD_LOG_ERR(__FUNCTION__,
            "Failed to write mlag %d, remote if %s status %s, rc %d",
            mlag_id, po_name, is_oper_up ? "up" : "down", rc);
        return MCLAG_ERROR;
    }
    else
    {
        SYSTEM_SET_SYNCD_TX_DBG_COUNTER(sys, msg_hdr->type, ICCP_DBG_CNTR_STS_OK);
        ICCPD_LOG_DEBUG("ICCP_FSM", "Set mlag %d, remote if %s status %s",
            mlag_id, po_name, is_oper_up ? "up" : "down");
        return 0;
    }
}


/* Send request to Mclagsyncd to remove remote interface table entry
 * The message includes MLAG id and remote interface name
 */
int mlacp_link_del_remote_if_info(
   int                  mlag_id,
   char                 *po_name)
{
    struct IccpSyncdHDr     *msg_hdr;
    mclag_sub_option_hdr_t  *sub_msg;
    char                    *msg_buf = g_iccp_mlagsyncd_send_buf;
    struct System           *sys;
    ssize_t                 rc = 0;

    sys = system_get_instance();
    if (sys == NULL)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Invalid system instance");
        return MCLAG_ERROR;
    }

    memset(msg_buf, 0, ICCP_MLAGSYNCD_SEND_MSG_BUFFER_SIZE);
    msg_hdr = (struct IccpSyncdHDr *)msg_buf;
    msg_hdr->ver = ICCPD_TO_MCLAGSYNCD_HDR_VERSION;
    msg_hdr->type = MCLAG_MSG_TYPE_DEL_REMOTE_IF_INFO;
    msg_hdr->len = sizeof(struct IccpSyncdHDr);

    /* Sub-message: mlag ID */
    sub_msg = (mclag_sub_option_hdr_t *)&msg_buf[msg_hdr->len];
    sub_msg->op_type = MCLAG_SUB_OPTION_TYPE_MCLAG_ID;
    sub_msg->op_len = sizeof(mlag_id);
    memcpy(sub_msg->data, &mlag_id, sub_msg->op_len);
    msg_hdr->len += (sizeof(mclag_sub_option_hdr_t) + sub_msg->op_len);

    /* Sub-message: MLAG interface name */
    sub_msg = (mclag_sub_option_hdr_t *)&msg_buf[msg_hdr->len];
    sub_msg->op_type = MCLAG_SUB_OPTION_TYPE_MCLAG_INTF_NAME;
    sub_msg->op_len = strlen(po_name);
    memcpy(sub_msg->data, po_name, sub_msg->op_len);
    msg_hdr->len += (sizeof(mclag_sub_option_hdr_t) + sub_msg->op_len);

    if (sys->sync_fd)
        rc = iccp_send_to_mclagsyncd(msg_hdr->type, msg_buf, msg_hdr->len);

    if ((rc <= 0) || (rc != msg_hdr->len))
    {
        ICCPD_LOG_ERR(__FUNCTION__,
            "Failed to write mlag %d, del remote if %s, rc %d",
            mlag_id, po_name, rc);
        return MCLAG_ERROR;
    }
    else
    {
        SYSTEM_SET_SYNCD_TX_DBG_COUNTER(sys, msg_hdr->type, ICCP_DBG_CNTR_STS_OK);
        ICCPD_LOG_DEBUG("ICCP_FSM", "Delete mlag %d, remote if %s",
            mlag_id, po_name);
        return 0;
    }
}

/* Send request to Mclagsyncd to update port isolation state
 * The message includes LAG interface name and enable/disable state
 */
int mlacp_link_set_peerlink_port_isolation(
    int                     mlag_id,
    char                    *po_name,
    bool                    is_isolation_enable)
{
    struct IccpSyncdHDr     *msg_hdr;
    mclag_sub_option_hdr_t  *sub_msg;
    char                    *msg_buf = g_iccp_mlagsyncd_send_buf;
    struct System           *sys;
    ssize_t                 rc = 0;

    sys = system_get_instance();
    if (sys == NULL)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Invalid system instance");
        return MCLAG_ERROR;
    }
    memset(msg_buf, 0, ICCP_MLAGSYNCD_SEND_MSG_BUFFER_SIZE);
    msg_hdr = (struct IccpSyncdHDr *)msg_buf;
    msg_hdr->ver = ICCPD_TO_MCLAGSYNCD_HDR_VERSION;
    msg_hdr->type = MCLAG_MSG_TYPE_SET_PEER_LINK_ISOLATION;
    msg_hdr->len = sizeof(struct IccpSyncdHDr);

    /* Sub-message: mlag ID */
    sub_msg = (mclag_sub_option_hdr_t *)&msg_buf[msg_hdr->len];
    sub_msg->op_type = MCLAG_SUB_OPTION_TYPE_MCLAG_ID;
    sub_msg->op_len = sizeof(mlag_id);
    memcpy(sub_msg->data, &mlag_id, sub_msg->op_len);
    msg_hdr->len += (sizeof(mclag_sub_option_hdr_t) + sub_msg->op_len);

    /* Sub-message: MLAG interface name */
    sub_msg = (mclag_sub_option_hdr_t *)&msg_buf[msg_hdr->len];
    sub_msg->op_type = MCLAG_SUB_OPTION_TYPE_MCLAG_INTF_NAME;
    sub_msg->op_len = strlen(po_name);
    memcpy(sub_msg->data, po_name, sub_msg->op_len);
    msg_hdr->len += (sizeof(mclag_sub_option_hdr_t) + sub_msg->op_len);

    /* Sub-message: isolation enable/disable */
    sub_msg = (mclag_sub_option_hdr_t *)&msg_buf[msg_hdr->len];
    sub_msg->op_type = MCLAG_SUB_OPTION_TYPE_ISOLATION_STATE;
    sub_msg->op_len = sizeof(is_isolation_enable);
    memcpy(sub_msg->data, &is_isolation_enable, sub_msg->op_len);
    msg_hdr->len += (sizeof(mclag_sub_option_hdr_t) + sub_msg->op_len);

    if (sys->sync_fd)
        rc = iccp_send_to_mclagsyncd(msg_hdr->type, msg_buf, msg_hdr->len);

    if ((rc <= 0) || (rc != msg_hdr->len))
    {
        ICCPD_LOG_ERR(__FUNCTION__,
            "Failed to write mlag %d, %s port isolation %s, rc %d",
            mlag_id, po_name, is_isolation_enable ? "enable" : "disable", rc);
        return MCLAG_ERROR;
    }
    else
    {
        SYSTEM_SET_SYNCD_TX_DBG_COUNTER(sys, msg_hdr->type, ICCP_DBG_CNTR_STS_OK);
        ICCPD_LOG_NOTICE("ICCP_FSM", "Set mlag %d, %s port isolation %s",
            mlag_id, po_name, is_isolation_enable ? "enable" : "disable");
        return 0;
    }
}

static void set_peerlink_mlag_port_kernel_forward(
    struct CSM *csm,
    struct LocalInterface *lif,
    int enable)
{
    if (!csm || !csm->peer_link_if || !lif)
        return;

    char cmd[256] = { 0 };

    sprintf(cmd, "ebtables %s FORWARD -i %s -o %s -j DROP",
            "-D", csm->peer_link_if->name, lif->name);
    ICCPD_LOG_DEBUG(__FUNCTION__, " ebtable cmd  %s", cmd );
    system(cmd);

    sprintf(cmd, "ebtables %s FORWARD -i %s -o %s -j DROP",
            (enable) ? "-I" : "-D", csm->peer_link_if->name, lif->name);
    ICCPD_LOG_DEBUG(__FUNCTION__, " ebtable cmd  %s", cmd );
    system(cmd);

    return;
}

void update_peerlink_isolate_from_all_csm_lif(
    struct CSM* csm)
{
    struct LocalInterface *lif = NULL;
    struct IccpSyncdHDr * msg_hdr;
    mclag_sub_option_hdr_t * sub_msg;
    char *msg_buf = g_iccp_mlagsyncd_send_buf;
    struct System *sys;

    char mlag_po_buf[MCLAG_MEMBER_NAME_STR_LEN];
    int src_len = 0, dst_len = 0;
    ssize_t rc;

    sys = system_get_instance();
    if (sys == NULL)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Invalid system instance");
        return;
    }

    if (!csm || !csm->peer_link_if)
        return;

    memset(msg_buf, 0, ICCP_MLAGSYNCD_SEND_MSG_BUFFER_SIZE);
    memset(mlag_po_buf, 0, MCLAG_MEMBER_NAME_STR_LEN);

    msg_hdr = (struct IccpSyncdHDr *)msg_buf;
    msg_hdr->ver = ICCPD_TO_MCLAGSYNCD_HDR_VERSION;
    msg_hdr->type = MCLAG_MSG_TYPE_PORT_ISOLATE;
    msg_hdr->len = sizeof(struct IccpSyncdHDr);

    /*sub msg src*/
    sub_msg = (mclag_sub_option_hdr_t *)&msg_buf[msg_hdr->len];
    sub_msg->op_type = MCLAG_SUB_OPTION_TYPE_ISOLATE_SRC;

    if (csm->peer_link_if->type == IF_T_VXLAN)
    {
        /*TBD: vxlan tunnel port isolation will be supportted later*/
        return;
#if 0
        int begin_eth_port = 0;

        /*VTTNL0001;Ethernet0001,Ethernet0002*/
        /*src_len= strlen(csm->peer_link_if->name); */
        src_len += snprintf(src_buf + src_len, sizeof(src_buf) - src_len, "%s", csm->peer_link_if->name);
        src_len += snprintf(src_buf + src_len, sizeof(src_buf) - src_len, "%s", ";");

        /*traverse all ethernet port  */
        LIST_FOREACH(lif, &(sys->lif_list), system_next)
        {
            if (lif->type != IF_T_PORT)
                continue;

            /* need to isolate port,  get it's name */
            if (begin_eth_port != 0)
            {
                src_len += snprintf(src_buf + src_len, sizeof(src_buf) - src_len, "%s", ",");
            }

            src_len += snprintf(src_buf + src_len, sizeof(src_buf) - src_len, "%s", lif->name);
            begin_eth_port = 1;
        }
        memcpy(sub_msg->data, src_buf, src_len);

        ICCPD_LOG_DEBUG(__FUNCTION__, "isolate src %s, data %s, len %d", src_buf, sub_msg->data, src_len);
#endif
    }
    else
    {
        src_len = strlen(csm->peer_link_if->name);
        memcpy(sub_msg->data, csm->peer_link_if->name, src_len);
    }
    sub_msg->op_len = src_len;

    /*sub msg dst */
    msg_hdr->len += sub_msg->op_len;
    msg_hdr->len += sizeof(mclag_sub_option_hdr_t);
    sub_msg = (mclag_sub_option_hdr_t  *)&msg_buf[msg_hdr->len];
    sub_msg->op_type = MCLAG_SUB_OPTION_TYPE_ISOLATE_DST;

    /*traverse all portchannel member port and send msg to syncd */
    LIST_FOREACH(lif, &(MLACP(csm).lif_list), mlacp_next)
    {
        if (lif->type != IF_T_PORT_CHANNEL)
            continue;

        /* check pif port state and lif pochannel state */
        if (lif->isolate_to_peer_link == 1)
        {
            //The return value of the snprintf function is the length of the source string.
            if ((sizeof(mlag_po_buf) - dst_len) > 
                (strlen(lif->name) + strlen(lif->portchannel_member_buf) + 2))
            {
                /* need to isolate port,  get it's member name */
                if (strlen(mlag_po_buf) != 0)
                {
                    dst_len += snprintf(mlag_po_buf + dst_len, sizeof(mlag_po_buf) - dst_len, "%s", ",");
                }

                dst_len += snprintf(mlag_po_buf + dst_len, sizeof(mlag_po_buf) - dst_len, "%s%s%s",
                                    lif->name, lif->portchannel_member_buf[0] == 0 ? "" : ",", lif->portchannel_member_buf);
            }
            else
            {
                ICCPD_LOG_WARN(__FUNCTION__, "the remaining length %d is not enough to store:%s%s%s", 
                    sizeof(mlag_po_buf) - dst_len, lif->name, lif->portchannel_member_buf[0] == 0 ? "" : ",", lif->portchannel_member_buf);
            }
        }
    }

    sub_msg->op_len = dst_len;
    msg_hdr->len += sizeof(mclag_sub_option_hdr_t);
    msg_hdr->len += sub_msg->op_len;

    if (dst_len)
    {
        memcpy(sub_msg->data, mlag_po_buf, dst_len);
        ICCPD_LOG_DEBUG(__FUNCTION__, "Send port isolate msg to mclagsyncd, src port %s, dst port %s", csm->peer_link_if->name, mlag_po_buf);
    }
    else
    {
        ICCPD_LOG_DEBUG(__FUNCTION__, "Send port isolate msg to mclagsyncd, src port %s, dst port is NULL", csm->peer_link_if->name);
    }

    /*send msg*/
    if (sys->sync_fd)
    {
        rc = write(sys->sync_fd,msg_buf, msg_hdr->len);
        if ((rc <= 0) || (rc != msg_hdr->len))
        {
            SYSTEM_SET_SYNCD_TX_DBG_COUNTER(
                sys, msg_hdr->type, ICCP_DBG_CNTR_STS_ERR);
            ICCPD_LOG_ERR(__FUNCTION__, "Failed to write, rc %d", rc);
        }
        else
        {
            SYSTEM_SET_SYNCD_TX_DBG_COUNTER(
                sys, msg_hdr->type, ICCP_DBG_CNTR_STS_OK);
        }
    }

    return;
}

static void set_peerlink_mlag_port_isolate(
    struct CSM *csm,
    struct LocalInterface *lif,
    int enable,
    bool is_unbind_pending)
{
    if (!lif)
        return;

    lif->isolate_to_peer_link = enable;

    if (!csm || !csm->peer_link_if )
        return;

    if (MLACP(csm).current_state != MLACP_STATE_EXCHANGE)
        return;

    ICCPD_LOG_DEBUG(__FUNCTION__, "%s  port-isolate from %s to %s",
                    enable ? "Enable" : "Disable", csm->peer_link_if->name, lif->name);
    ICCPD_LOG_DEBUG("ICCP_FSM", "Set port isolation %s: mlag_if %s, members %s",
        enable ? "enable" : "disable", lif->name, lif->portchannel_member_buf);

    update_peerlink_isolate_from_all_csm_lif(csm);

    /* Kernel also needs to block traffic from peerlink to mlag-port*/
    set_peerlink_mlag_port_kernel_forward(csm, lif, enable);

    /* Do not need to send update to Mclagsyncd to update the local
     * MLAG interface table in STATE_DB because Mclagsyncd will delete
     * the entry
     */
    if (!is_unbind_pending)
        mlacp_link_set_peerlink_port_isolation(csm->mlag_id, lif->name, enable);
}

void peerlink_port_isolate_cleanup(struct CSM* csm)
{
    struct LocalInterface *local_if = NULL;

    if (!csm)
        return;

    /* Clean all port block*/
    LIST_FOREACH(local_if, &(MLACP(csm).lif_list), mlacp_next)
    {
        if (local_if->type == IF_T_PORT_CHANNEL)
        {
            set_peerlink_mlag_port_isolate(csm, local_if, 0, false);
        }
    }

    return;
}

void update_peerlink_isolate_from_pif(
    struct CSM *csm,
    struct PeerInterface *pif,
    int pif_po_state,
    int new_create)
{
    struct LocalInterface *lif = NULL;
    int lif_po_state = 1;

    if (!csm || !csm->peer_link_if || !pif)
        return;
    if (new_create == 0 && pif_po_state == pif->po_active)
        return;
    if (MLACP(csm).current_state != MLACP_STATE_EXCHANGE)
        return;

    /* peer link changed*/
    LIST_FOREACH(lif, &(MLACP(csm).lif_list), mlacp_next)
    {
        if (strcmp(lif->name, pif->name) != 0)
            continue;

        lif_po_state = lif->po_active;
        break;
    }

    if (!lif)
    {
        ICCPD_LOG_WARN(__FUNCTION__, "Can't find local if for %s", pif->name);
        return;
    }

    ICCPD_LOG_DEBUG(__FUNCTION__, "From if %s local(%s) / peer(%s)",
                    lif->name,
                    (lif_po_state) ? "up" : "down",
                    (pif_po_state) ? "up" : "down");

    if (lif_po_state == 1)
    {
        if (pif_po_state == 1)
        {
            /* both peer-pair link up, enable port-isolate*/
            ICCPD_LOG_DEBUG("ICCP_FSM", "Enable port-isolate: from peer_link %s to mlag_if %s",
                csm->peer_link_if->name, lif->name);
            set_peerlink_mlag_port_isolate(csm, lif, 1, false);
        }
        else
        {
            /* local link up, and peer link changes to down, disable port-isolate*/
            ICCPD_LOG_DEBUG("ICCP_FSM", "Disable port-isolate: from peer_link %s to mlag_if %s",
                csm->peer_link_if->name, lif->name);
            set_peerlink_mlag_port_isolate(csm, lif, 0, false);
        }
    }
    else
    {
        ;/* local link down, do nothing*/
    }

    return;
}

static void update_peerlink_isolate_from_lif(
    struct CSM* csm,
    struct LocalInterface* lif,
    int lif_po_state)
{
    struct PeerInterface *pif = NULL;
    int pif_po_state = 1;

    if (!csm || !csm->peer_link_if || !lif)
        return;
    /*if (lif_po_state == lif->po_active) return;*/
    if (MLACP(csm).current_state != MLACP_STATE_EXCHANGE)
        return;

    /* local link changed*/
    LIST_FOREACH(pif, &(MLACP(csm).pif_list), mlacp_next)
    {
        if (strcmp(pif->name, lif->name) != 0)
            continue;

        pif_po_state = pif->po_active;
        break;
    }

    ICCPD_LOG_DEBUG(__FUNCTION__, " from local %s local(%s) / peer(%s)",
                    lif->name, (lif_po_state) ? "up" : "down", (pif_po_state) ? "up" : "down");

    if (lif_po_state == 1)
    {
        if (pif_po_state == 1)
        {
            /* both peer-pair link up, enable port-isolate*/
            ICCPD_LOG_DEBUG(__FUNCTION__, "Enable port-isolate from %s to %s",
                            csm->peer_link_if->name, lif->name);
            set_peerlink_mlag_port_isolate(csm, lif, 1, false);
        }
        else
        {
            /* peer link down, local link changes to up, disable port-isolate*/
            ICCPD_LOG_DEBUG(__FUNCTION__, " Disable port-isolate from %s to %s",
                            csm->peer_link_if->name, lif->name);
            set_peerlink_mlag_port_isolate(csm, lif, 0, false);
        }
    }
    else
    {
        ;/* local link down, do nothing*/
    }

    return;
}

static void update_l2_po_state(struct CSM *csm,
                               struct LocalInterface *lif,
                               int po_state)
{
    ROUTE_MANIPULATE_TYPE_E route_type = ROUTE_NONE;
    struct VLAN_ID *vlan = NULL;
    struct LocalInterface *set_l3_vlan_if = NULL;

    if (!csm || !lif)
        return;

    /*Is there any L3 vlan over L2 po?*/
    RB_FOREACH (vlan, vlan_rb_tree, &(lif->vlan_tree))
    {
        route_type = ROUTE_NONE;

        if (!vlan->vlan_itf)
            continue;

        /* If the po is under a vlan, update vlan state first*/
        update_vlan_if_info(csm, lif, vlan->vlan_itf, po_state);

        if (!local_if_is_l3_mode(vlan->vlan_itf))
            continue;

        /*NOTE
         * assume only one mlag per vlan
         * need to add rules for per mlag per vlan later (arp list?)
         */
        set_l3_vlan_if = vlan->vlan_itf;
        if (po_state != lif->po_active
            || MLACP(csm).current_state != set_l3_vlan_if->mlacp_state)
        {
            if (po_state == 1)
            {
                route_type = ROUTE_DEL;
            }
            else if (po_state == 0
                     && MLACP(csm).current_state == MLACP_STATE_EXCHANGE)
            {
                if (peer_po_is_alive(csm, lif->ifindex) == 1)
                    route_type = ROUTE_ADD;
            }
        }

        /*update_po_arp_list(csm, set_l3_vlan_if);*/
        set_l3_itf_state(csm, set_l3_vlan_if, route_type);
        update_l3_if_info(csm, lif, set_l3_vlan_if, po_state);
    }

    return;
}

static void update_l3_po_state(struct CSM *csm,
                               struct LocalInterface *lif,
                               int po_state)
{
    ROUTE_MANIPULATE_TYPE_E route_type = ROUTE_NONE;
    struct LocalInterface *set_l3_lif = NULL;

    /*L3 po*/
    set_l3_lif = lif;

    if (!csm || !lif)
        return;

    if (po_state != lif->po_active
        && po_state == 1)
    {
        /* po alive, clean static route & recover the ARP*/
        route_type = ROUTE_DEL;
    }
    else if (po_state != lif->po_active
             && po_state == 0
             && MLACP(csm).current_state == MLACP_STATE_EXCHANGE)
    {
        /* po is not alive & peer-link alive, set static route*/
        if (peer_po_is_alive(csm, lif->po_id) == 1)
            route_type = ROUTE_ADD;
    }
    else if (MLACP(csm).current_state != lif->mlacp_state
             && MLACP(csm).current_state == MLACP_STATE_EXCHANGE
             && po_state == 0)
    {
        /* when peer-pair link ready, set static route for broken po link*/
        if (peer_po_is_alive(csm, lif->po_id) == 1)
            route_type = ROUTE_ADD;
    }

    /*update_po_arp_list(csm, set_l3_lif);*/
    set_l3_itf_state(csm, set_l3_lif, route_type);
    update_l3_if_info(csm, lif, set_l3_lif, po_state);

    return;
}

int is_local_vlan_on(struct VLAN_ID* vlan_id_list)
{
    if (!vlan_id_list->vlan_itf)
        return 0;

    return 1;
}

void syn_arp_info_to_peer(struct CSM *csm, struct LocalInterface *local_if)
{
    struct Msg *msg = NULL;
    struct ARPMsg *arp_msg = NULL, *arp_info = NULL;
    struct Msg *msg_send = NULL;

    if (!csm || !local_if)
        return;

    if (!TAILQ_EMPTY(&(MLACP(csm).arp_list)))
    {
        TAILQ_FOREACH(msg, &MLACP(csm).arp_list, tail)
        {
            arp_info = (struct ARPMsg*)msg->buf;

            if (strcmp(arp_info->ifname, local_if->name) != 0)
                continue;

            arp_msg = (struct ARPMsg*)msg->buf;
            arp_msg->op_type = NEIGH_SYNC_ADD;
            arp_msg->flag = 0;

            if (iccp_csm_init_msg(&msg_send, (char*)arp_msg, sizeof(struct ARPMsg)) == 0)
            {
                TAILQ_INSERT_TAIL(&(MLACP(csm).arp_msg_list), msg_send, tail);
                /*ICCPD_LOG_DEBUG( __FUNCTION__, "Enqueue ARP[ADD] for %s",
                                 show_ip_str(htonl(arp_msg->ipv4_addr)));*/
            }
            else
                ICCPD_LOG_WARN(__FUNCTION__, "Failed to enqueue ARP[ADD] for %s",
                                show_ip_str(htonl(arp_msg->ipv4_addr)));
        }
    }

    return;
}

void syn_ndisc_info_to_peer(struct CSM *csm, struct LocalInterface *local_if)
{
    struct Msg *msg = NULL;
    struct NDISCMsg *ndisc_msg = NULL, *ndisc_info = NULL;
    struct Msg *msg_send = NULL;

    if (!csm || !local_if)
        return;

    if (!TAILQ_EMPTY(&(MLACP(csm).ndisc_list)))
    {
        TAILQ_FOREACH(msg, &MLACP(csm).ndisc_list, tail)
        {
            ndisc_info = (struct NDISCMsg *)msg->buf;

            if (strcmp(ndisc_info->ifname, local_if->name) != 0)
                continue;

            ndisc_msg = (struct NDISCMsg *)msg->buf;
            ndisc_msg->op_type = NEIGH_SYNC_ADD;
            ndisc_msg->flag = 0;

            if (iccp_csm_init_msg(&msg_send, (char *)ndisc_msg, sizeof(struct NDISCMsg)) == 0)
            {
                TAILQ_INSERT_TAIL(&(MLACP(csm).ndisc_msg_list), msg_send, tail);
                ICCPD_LOG_DEBUG(__FUNCTION__, "Enqueue ND[ADD] for %s", show_ipv6_str((char *)ndisc_msg->ipv6_addr));
            }
            else
                ICCPD_LOG_DEBUG(__FUNCTION__, "Failed to enqueue ND[ADD] for %s", show_ipv6_str((char *)ndisc_msg->ipv6_addr));
        }
    }

    return;
}

void update_stp_peer_link(struct CSM *csm,
                          struct PeerInterface *pif,
                          int po_state, int new_create)
{
    struct LocalInterface *lif = NULL;
    struct VLAN_ID *vlan = NULL;

    if (!csm || !pif)
        return;

    ICCPD_LOG_DEBUG("ICCP_FSM",
        "PEER_MLAG_IF %s %s: po_active %d, new_state %s, sync_state %s",
        pif->name, new_create ? "add" : "update",
        pif->po_active, po_state ? "up" : "down", mlacp_state(csm));

    if (new_create == 0 && po_state == pif->po_active)
        return;

    LIST_FOREACH(lif, &(MLACP(csm).lif_list), mlacp_next)
    {
        if (strcmp(lif->name, pif->name) != 0)
            continue;

        /* update lif route if pif link status changes */
        if (local_if_is_l3_mode(lif))
        {
            if (po_state == 1 && lif->po_active == 0)
                set_l3_itf_state(csm, lif, ROUTE_ADD);
            else if (po_state == 0 && lif->po_active == 0)
                set_l3_itf_state(csm, lif, ROUTE_DEL);

            /*If pif change to active, and local is also active, syn arp to peer*/
            if (po_state == 1 && lif->po_active == 1)
            {
                syn_arp_info_to_peer(csm, lif);
                syn_ndisc_info_to_peer(csm, lif);
            }
        }
        else
        {
            RB_FOREACH(vlan, vlan_rb_tree, &(lif->vlan_tree))
            {
                if (!is_local_vlan_on(vlan))
                    continue;
                if (!local_if_is_l3_mode(vlan->vlan_itf))
                    continue;

                /*NOTE
                 * assume only one mlag per bridge
                 * need to add rules for per mlag per bridge later (arp list?)
                 */
                if (po_state == 1 && lif->po_active == 0)
                    set_l3_itf_state(csm, vlan->vlan_itf, ROUTE_ADD);
                else if (po_state == 0 && lif->po_active == 0)
                    set_l3_itf_state(csm, vlan->vlan_itf, ROUTE_DEL);

                /*If pif change to active, and local is also active, syn arp to peer*/
                if (po_state == 1 && lif->po_active == 1)
                {
                    syn_arp_info_to_peer(csm, vlan->vlan_itf);
                    syn_ndisc_info_to_peer(csm, vlan->vlan_itf);
                }
            }
        }

        break;
    }
    return;
}

void iccp_send_fdb_entry_to_syncd( struct MACMsg* mac_msg, uint8_t mac_type, uint8_t oper)
{
    struct IccpSyncdHDr * msg_hdr;
    char *msg_buf = g_iccp_mlagsyncd_send_buf;
    struct System *sys;
    struct mclag_fdb_info * mac_info;
    ssize_t rc;
    uint8_t null_mac[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    sys = system_get_instance();
    if (sys == NULL)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Invalid system instance");
        return;
    }

    if (memcmp(mac_msg->mac_addr, null_mac, ETHER_ADDR_LEN) == 0)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Invalid MAC address do not send to Syncd.");
        return;
    }

    memset(msg_buf, 0, ICCP_MLAGSYNCD_SEND_MSG_BUFFER_SIZE);

    msg_hdr = (struct IccpSyncdHDr *)msg_buf;
    msg_hdr->ver = ICCPD_TO_MCLAGSYNCD_HDR_VERSION;
    msg_hdr->type = MCLAG_MSG_TYPE_SET_FDB;

    /*mac msg */
    mac_info = (struct mclag_fdb_info *)&msg_buf[sizeof(struct IccpSyncdHDr)];
    mac_info->vid = mac_msg->vid;
    memcpy(mac_info->port_name, mac_msg->ifname, MAX_L_PORT_NAME);
    memcpy(mac_info->mac, mac_msg->mac_addr, ETHER_ADDR_LEN);
    mac_info->type = mac_type;
    mac_info->op_type = oper;
    msg_hdr->len = sizeof(struct IccpSyncdHDr) + sizeof(struct mclag_fdb_info);

    ICCPD_LOG_DEBUG("ICCP_FDB", "Send fdb to syncd: write mac msg vid : %d ; ifname %s ; mac %s fdb type %d ; op type %s",
        mac_info->vid, mac_info->port_name, mac_addr_to_str(mac_info->mac), mac_info->type,
        oper == MAC_SYNC_ADD ? "add" : "del");

    /*send msg*/
    if (sys->sync_fd > 0 )
    {
        rc = iccp_send_to_mclagsyncd(msg_hdr->type, msg_buf, msg_hdr->len);
        if (rc <= 0)
        {
            ICCPD_LOG_WARN(__FUNCTION__, "Send to Mclagsyncd failed rc: %d",rc);
        }
    }
    else
    {
        SYSTEM_SET_SYNCD_TX_DBG_COUNTER(sys, msg_hdr->type, ICCP_DBG_CNTR_STS_ERR);
        ICCPD_LOG_ERR(__FUNCTION__, "Invalid sync_fd Failed to write, fd %d", sys->sync_fd);
    }

    if (oper == MAC_SYNC_DEL)
        mac_msg->add_to_syncd = 0;
    else
        mac_msg->add_to_syncd = 1;

    return;
}

void add_mac_to_chip(struct MACMsg* mac_msg, uint8_t mac_type)
{
    iccp_send_fdb_entry_to_syncd( mac_msg, mac_type, MAC_SYNC_ADD);

    return;
}

void del_mac_from_chip(struct MACMsg* mac_msg)
{
    iccp_send_fdb_entry_to_syncd(  mac_msg, mac_msg->fdb_type, MAC_SYNC_DEL);

    return;
}

uint8_t set_mac_local_age_flag(struct CSM *csm, struct MACMsg* mac_msg, uint8_t set, uint8_t update_peer )
{
    uint8_t new_age_flag = 0;

    new_age_flag = mac_msg->age_flag;

    if (set == 0)/*remove age flag*/
    {
        if (new_age_flag & MAC_AGE_LOCAL)
        {
            new_age_flag &= ~MAC_AGE_LOCAL;

            ICCPD_LOG_DEBUG("ICCP_FDB", "After Remove local age, flag: %d interface  %s, "
                "add %s vlan-id %d, old age_flag %d", new_age_flag, mac_msg->ifname,
                mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid, mac_msg->age_flag);

            /*send mac MAC_SYNC_ADD message to peer*/
            if ((MLACP(csm).current_state == MLACP_STATE_EXCHANGE) && update_peer)
            {
                mac_msg->op_type = MAC_SYNC_ADD;
                if (!MAC_IN_MSG_LIST(&(MLACP(csm).mac_msg_list), mac_msg, tail))
                {
                    TAILQ_INSERT_TAIL(&(MLACP(csm).mac_msg_list), mac_msg, tail);
                }
                /*ICCPD_LOG_DEBUG(__FUNCTION__, "MAC-msg-list enqueue: %s, add %s vlan-id %d, age_flag %d",
                               mac_msg->ifname, mac_msg->mac_str, mac_msg->vid, mac_msg->age_flag);*/
            }
        }
    }
    else/*set age flag*/
    {
        if (!(new_age_flag & MAC_AGE_LOCAL))
        {
            new_age_flag |= MAC_AGE_LOCAL;

            ICCPD_LOG_DEBUG("ICCP_FDB", "After local age set, flag: %d interface %s, "
                    "MAC %s vlan-id %d, old age_flag %d", new_age_flag, mac_msg->ifname,
                    mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid, mac_msg->age_flag);

            /*send mac MAC_SYNC_DEL message to peer*/
            if ((MLACP(csm).current_state == MLACP_STATE_EXCHANGE) && update_peer)
            {
                mac_msg->op_type = MAC_SYNC_DEL;
                if (!MAC_IN_MSG_LIST(&(MLACP(csm).mac_msg_list), mac_msg, tail))
                {
                    TAILQ_INSERT_TAIL(&(MLACP(csm).mac_msg_list), mac_msg, tail);
                }

                ICCPD_LOG_DEBUG("ICCP_FDB", "Set local age: MAC-msg-list enqueue interface: %s, oper: %s "
                        "MAC %s vlan-id %d, age_flag %d", mac_msg->ifname,
                        (mac_msg->op_type == MAC_SYNC_ADD) ? "add":"del",
                        mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid, mac_msg->age_flag);
            }
        }
    }
    return new_age_flag;
}

/*Deal with mac add,del,move when portchannel up or down*/
static void update_l2_mac_state(struct CSM *csm,
                                struct LocalInterface *lif,
                                int po_state)
{
    struct MACMsg* mac_msg = NULL,  *mac_temp = NULL;
    struct PeerInterface* pif = NULL;
    pif = peer_if_find_by_name(csm, lif->name);

    if (!csm || !lif)
        return;

    if (po_state == 0)
    {
        lif->po_down_time = time(NULL);
        ICCPD_LOG_DEBUG("ICCP_FDB", "Intf down,  ifname: %s, po_down_time: %u", lif->name, lif->po_down_time);
    }
    else
    {
        lif->po_down_time = 0;
        ICCPD_LOG_DEBUG("ICCP_FDB", "Intf up,  ifname: %s, clear po_down_time, time %u", lif->name, lif->po_down_time);
    }


    RB_FOREACH_SAFE (mac_msg, mac_rb_tree, &MLACP(csm).mac_rb, mac_temp)
    {
        /* find the MAC for this interface*/
        if (strcmp(lif->name, mac_msg->origin_ifname) != 0)
            continue;

        /*portchannel down*/
        if (po_state == 0)
        {
            // MAC is locally learned do not delete MAC, Move to peer_link
            if ((mac_msg->age_flag == MAC_AGE_PEER) && pif && (pif->state == PORT_STATE_UP))
            {
                if ((strlen(csm->peer_itf_name) != 0) && csm->peer_link_if && csm->peer_link_if->state == PORT_STATE_UP)
                {
                    memcpy(mac_msg->ifname, csm->peer_itf_name, MAX_L_PORT_NAME);

                    ICCPD_LOG_DEBUG("ICCP_FDB", "Intf down, MAC learn local only, age flag %d, "
                       "redirect MAC to peer-link: %s, MAC %s vlan-id %d",
                       mac_msg->age_flag, mac_msg->ifname,
                       mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid);

                    add_mac_to_chip(mac_msg, mac_msg->fdb_type);
                }
                else
                {
                    del_mac_from_chip(mac_msg);
                    memcpy(mac_msg->ifname, csm->peer_itf_name, MAX_L_PORT_NAME);
                    ICCPD_LOG_DEBUG("ICCP_FDB", "Intf down,  MAC learn local only, age flag %d, "
                       "can not redirect, del MAC as peer-link %s not available or down, "
                       "MAC %s vlan-id %d", mac_msg->age_flag, mac_msg->ifname,
                       mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid);
                }

                mac_msg->pending_local_del = 1;

                continue;
            }

            mac_msg->age_flag = set_mac_local_age_flag(csm, mac_msg, 1, 1);

            ICCPD_LOG_DEBUG("ICCP_FDB", "Intf down, age flag %d, MAC %s, "
                "vlan-id %d, Interface: %s", mac_msg->age_flag ,
                mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid, mac_msg->ifname);

            if (mac_msg->age_flag == (MAC_AGE_LOCAL | MAC_AGE_PEER))
            {
                /*send mac del message to mclagsyncd.*/
                if (mac_msg->fdb_type != MAC_TYPE_STATIC)
                    del_mac_from_chip(mac_msg);

                ICCPD_LOG_DEBUG("ICCP_FDB", "Intf down, del MAC %s, vlan-id %d,"
                        " Interface: %s,", mac_addr_to_str(mac_msg->mac_addr),
                       mac_msg->vid, mac_msg->ifname);

                MAC_RB_REMOVE(mac_rb_tree, &MLACP(csm).mac_rb, mac_msg);

                // free only if not in change list to be send to peer node,
                // else free is taken care after sending the update to peer
                if (!MAC_IN_MSG_LIST(&(MLACP(csm).mac_msg_list), mac_msg, tail))
                {
                    free(mac_msg);
                }
            }
            else
            {
                /*If local is aged but peer is not aged, redirect the mac to peer-link*/
                if (strlen(csm->peer_itf_name) != 0)
                {
                    /*Send mac add message to mclagsyncd. fdb_type is not changed*/
                    /*Is need to delete the old item before add?(Old item probably is static)*/
                    if (csm->peer_link_if && csm->peer_link_if->state == PORT_STATE_UP)
                    {
                        memcpy(mac_msg->ifname, csm->peer_itf_name, MAX_L_PORT_NAME);
                        add_mac_to_chip(mac_msg, mac_msg->fdb_type);
                        ICCPD_LOG_DEBUG("ICCP_FDB", "Intf down, age flag %d, "
                           "redirect MAC to peer-link: %s, MAC %s vlan-id %d",
                           mac_msg->age_flag, mac_msg->ifname,
                           mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid);
                    }
                    else
                    {
                        /*must redirect but peerlink is down, del mac from ASIC*/
                        /*if peerlink change to up, mac will add back to ASIC*/
                        del_mac_from_chip(mac_msg);
                        memcpy(mac_msg->ifname, csm->peer_itf_name, MAX_L_PORT_NAME);
                        ICCPD_LOG_DEBUG("ICCP_FDB", "Intf down, age flag %d, "
                           "can not redirect, del MAC as peer-link: %s down, "
                           "MAC %s vlan-id %d", mac_msg->age_flag, mac_msg->ifname,
                           mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid);
                    }
                }
                else
                {
                    /*peer-link is not configured, del mac from ASIC, mac still in mac_rb*/
                    del_mac_from_chip(mac_msg);

                    ICCPD_LOG_DEBUG("ICCP_FDB", "Intf down, flag %d, peer-link: %s not available, "
                    "MAC %s vlan-id %d", mac_msg->age_flag, mac_msg->ifname,
                    mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid);
                }
            }
        }
        else /*portchannel up*/
        {
            /*the old item is redirect to peerlink for portchannel down*/
            /*when this portchannel up, recover the mac back*/
            if (strcmp(mac_msg->ifname, csm->peer_itf_name) == 0)
            {
                ICCPD_LOG_DEBUG("ICCP_FDB", "Intf up, redirect MAC to Interface: %s,"
                " MAC %s vlan-id %d, age flag: %d ", mac_msg->ifname,
                mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid, mac_msg->age_flag);

                if (mac_msg->pending_local_del)
                    mac_msg->pending_local_del = 0;

                /*Remove MAC_AGE_LOCAL flag*/
                // commenting this code to fix an issue, when interface comes back up dont delete age flag
                // as the MAC is remote now, delete only if MAC learns again.
                //mac_msg->age_flag = set_mac_local_age_flag(csm, mac_msg, 0, 1);

                /*Reverse interface from peer-link to the original portchannel*/
                memcpy(mac_msg->ifname, mac_msg->origin_ifname, MAX_L_PORT_NAME);

                /*Send dynamic or static mac add message to mclagsyncd*/

                if (mac_msg->age_flag == MAC_AGE_LOCAL)
                    add_mac_to_chip(mac_msg, mac_msg->fdb_type);
                else
                    add_mac_to_chip(mac_msg, MAC_TYPE_DYNAMIC_LOCAL);
            }
            else
            {
                // Delete the MAC from HW so that it can be re-learned accordingly if traffic is still hitting local node.
                if (mac_msg->pending_local_del)
                {
                    ICCPD_LOG_DEBUG("ICCP_FDB", "Intf up, Clear pending MAC: interface: %s, mac %s vlan-id %d, age_flag %d",
                            mac_msg->ifname, mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid, mac_msg->age_flag);

                    del_mac_from_chip(mac_msg);

                    // if static dont delete mac
                    if (mac_msg->fdb_type != MAC_TYPE_STATIC)
                    {
                        //TBD do we need to send delete notification to peer .?
                        MAC_RB_REMOVE(mac_rb_tree, &MLACP(csm).mac_rb, mac_msg);

                        mac_msg->op_type = MAC_SYNC_DEL;
                        if (!MAC_IN_MSG_LIST(&(MLACP(csm).mac_msg_list), mac_msg, tail))
                        {
                            free(mac_msg);
                        }
                    }
                    else
                        mac_msg->pending_local_del = 0;
                }

// Dont set local learn unless learned from MCLAGSYNCD.
// When interface is UP MAC addresses gets re-learned
#if 0
                /*this may be peerlink is not configured and portchannel is down*/
                /*when this portchannel up, add the mac back to ASIC*/
                ICCPD_LOG_DEBUG("ICCP_FDB", "Intf up, add MAC %s to ASIC,"
                    " vlan-id %d Interface %s", mac_addr_to_str(mac_msg->mac_addr),
                    mac_msg->vid, mac_msg->ifname);

                /*Remove MAC_AGE_LOCAL flag*/
                mac_msg->age_flag = set_mac_local_age_flag(csm, mac_msg, 0, 1);


                memcpy(mac_msg->ifname, mac_msg->origin_ifname, MAX_L_PORT_NAME);

                /*Send dynamic or static mac add message to mclagsyncd*/
                add_mac_to_chip(mac_msg, mac_msg->fdb_type);
#endif
            }
        }
    }

    return;
}

void update_orphan_port_mac(struct CSM *csm,
                            struct LocalInterface *lif,
                            int state)
{
    struct MACMsg* mac_msg = NULL, *mac_temp = NULL;

    if (!csm || !lif)
        return;

    if (!state)
        return;

    RB_FOREACH_SAFE (mac_msg, mac_rb_tree, &MLACP(csm).mac_rb, mac_temp)
    {
        if (strcmp(mac_msg->origin_ifname, lif->name ) != 0)
            continue;

        ICCPD_LOG_DEBUG("ICCP_FDB", "Orphan port is UP sync MAC: interface %s, "
                "MAC %s vlan-id %d, age flag: %d, exchange state :%d", mac_msg->origin_ifname,
                mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid,
                mac_msg->age_flag, MLACP(csm).current_state);

        // sync local macs on orphan port , if any
        if ((mac_msg->age_flag ==  MAC_AGE_PEER) && (MLACP(csm).current_state == MLACP_STATE_EXCHANGE))
        {
            mac_msg->op_type = MAC_SYNC_ADD;

            if (!MAC_IN_MSG_LIST(&(MLACP(csm).mac_msg_list), mac_msg, tail))
            {
                TAILQ_INSERT_TAIL(&(MLACP(csm).mac_msg_list), mac_msg, tail);
            }
        }
    }
}

void mlacp_convert_remote_mac_to_local(struct CSM *csm, char *po_name)
{
    struct MACMsg* mac_msg = NULL, *mac_temp = NULL;
    struct LocalInterface* lif = NULL;
    lif = local_if_find_by_name(po_name);

    if (!csm || !lif)
        return;

    if (lif->state == PORT_STATE_DOWN)
    {
        ICCPD_LOG_DEBUG("ICCP_FDB", "Do not Convert remote mac as Local interface %s is down", po_name);
        return;
    }

    RB_FOREACH_SAFE (mac_msg, mac_rb_tree, &MLACP(csm).mac_rb, mac_temp)
    {
        if (strcmp(mac_msg->origin_ifname, po_name) != 0)
            continue;

        // convert only remote macs.
        if (mac_msg->age_flag == MAC_AGE_LOCAL)
        {
            mac_msg->age_flag = MAC_AGE_PEER;
            ICCPD_LOG_DEBUG("ICCP_FDB", "Convert remote mac on Origin Interface as local: interface %s, "
                    "interface %s, MAC %s vlan-id %d age flag:%d", mac_msg->origin_ifname,
                    mac_msg->ifname, mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid, mac_msg->age_flag);

            /*Send mac add message to mclagsyncd with aging enabled*/
            add_mac_to_chip(mac_msg, MAC_TYPE_DYNAMIC_LOCAL);

            mac_msg->op_type = MAC_SYNC_ADD;

            if (!MAC_IN_MSG_LIST(&(MLACP(csm).mac_msg_list), mac_msg, tail))
            {
                TAILQ_INSERT_TAIL(&(MLACP(csm).mac_msg_list), mac_msg, tail);
            }
        }
    }
}

//update remote macs to point to peerlink, if peer link is configured
static void update_remote_macs_to_peerlink(struct CSM *csm, struct LocalInterface *lif)
{
    struct MACMsg* mac_entry = NULL;

    if (!csm || !lif)
        return;

    RB_FOREACH (mac_entry, mac_rb_tree, &MLACP(csm).mac_rb)
    {
        /* find the MAC for this interface*/
        if (strcmp(lif->name, mac_entry->origin_ifname) != 0)
            continue;

        //consider only remote mac; rest of MACs no need to handle
        if(mac_entry->age_flag & MAC_AGE_PEER)
        {
            continue;
        }

        ICCPD_LOG_DEBUG("ICCP_FDB", "Update remote macs to peer: age flag %d, MAC %s, "
                "vlan-id %d, Interface: %s", mac_entry->age_flag ,
                mac_addr_to_str(mac_entry->mac_addr), mac_entry->vid, mac_entry->ifname);

        //If local interface unbinded, redirect the mac to peer-link if peer
        //link is configured
        if (strlen(csm->peer_itf_name) != 0)
        {
            /*Send mac add message to mclagsyncd. fdb_type is not changed*/
            if (csm->peer_link_if && csm->peer_link_if->state == PORT_STATE_UP)
            {
                //if the mac is already pointing to peer interface, no need to
                //change it
                if (strcmp(mac_entry->ifname, csm->peer_itf_name) != 0)
                {
                    memcpy(mac_entry->ifname, csm->peer_itf_name, MAX_L_PORT_NAME);
                    add_mac_to_chip(mac_entry, mac_entry->fdb_type);
                    ICCPD_LOG_DEBUG("ICCP_FDB", "Update remote macs to peer: age flag %d, "
                            "redirect MAC to peer-link: %s, MAC %s vlan-id %d",
                            mac_entry->age_flag, mac_entry->ifname,
                            mac_addr_to_str(mac_entry->mac_addr), mac_entry->vid);
                }
            }
        }
    }
    return;
}

void mlacp_portchannel_state_handler(struct CSM* csm,
                                     struct LocalInterface* local_if,
                                     int po_state)
{
    if (!csm || !local_if)
        return;
    ICCPD_LOG_DEBUG("ICCP_FSM",
            "MLAG_IF(%s) %s %s: state %s, po_active %d, traffic_dis %d, sync_state %s  cfg_sync/changed %d/%d",
            local_if_is_l3_mode(local_if) ? "L3" : "L2",
            local_if->name, po_state ? "up" : "down",
            (local_if->state == PORT_STATE_UP) ? "up" : "down",
            local_if->po_active, local_if->is_traffic_disable,
            mlacp_state(csm), local_if->port_config_sync, local_if->changed);

    update_peerlink_isolate_from_lif(csm, local_if, po_state);

    update_l2_mac_state(csm, local_if, po_state);

    if (!local_if_is_l3_mode(local_if))
        update_l2_po_state(csm, local_if, po_state);
    else
        update_l3_po_state(csm, local_if, po_state);

    update_po_if_info(csm, local_if, po_state);

    /* Disable packet tx/rx on MLAG interface when it is down
     * Traffic is re-enabled back after the interface is up and ack is
     * received from peer
     */
    if (po_state == 0)
        mlacp_link_disable_traffic_distribution(local_if);

    return;
}

void mlacp_mlag_intf_detach_handler(struct CSM* csm, struct LocalInterface* local_if)
{
    if (!csm || !local_if)
        return;

    ICCPD_LOG_DEBUG("ICCP_FSM",
        "MLAG_IF(%s) %s Detach: state %s, po_active %d, traffic_dis %d, sync_state %s",
        local_if_is_l3_mode(local_if) ? "L3" : "L2",
        local_if->name,
        (local_if->state == PORT_STATE_UP) ? "up" : "down",
        local_if->po_active, local_if->is_traffic_disable,
        mlacp_state(csm));

    //set port isolate for lifpo
    mlacp_mlag_link_del_handler(csm, local_if);

    //point remotely learnt macs to peer-link
    update_remote_macs_to_peerlink(csm, local_if);

    //Handle Route/ARP changes as if portchannel is down
    if (!local_if_is_l3_mode(local_if))
        update_l2_po_state(csm, local_if, 0);
    else
        update_l3_po_state(csm, local_if, 0);


    //If the traffic is disabled due to interface flap; while coming up, if
    //mclag interface is removed before receiving ack, it will be in
    //blocked state; to address timing scenario unblock Tx/Rx of
    //traffic on this portchannel if the traffic is blocked on this port
    if(local_if->is_traffic_disable)
    {
        if ( !csm->peer_link_if || !(strcmp(csm->peer_link_if->name, local_if->name)) )
        {
            mlacp_link_enable_traffic_distribution(local_if);
        }
    }

    return;
}

//Handler to handle when mclag interface is deleted on peer end
void mlacp_peer_mlag_intf_delete_handler(struct CSM* csm, char *mlag_if_name)
{
    struct LocalInterface *local_if = NULL;
    if (!csm)
        return;

    local_if = local_if_find_by_name(mlag_if_name);

    if (!local_if)
        return;

    ICCPD_LOG_DEBUG("ICCP_FSM",
        "MLAG_IF(%s) %s Peer IF Delete Event: state %s, po_active %d, traffic_dis %d, sync_state %s",
        local_if_is_l3_mode(local_if) ? "L3" : "L2",
        local_if->name,
        (local_if->state == PORT_STATE_UP) ? "up" : "down",
        local_if->po_active, local_if->is_traffic_disable,
        mlacp_state(csm));

    //if it is standby node change back the mac to its original system mac
    recover_if_ipmac_on_standby(local_if, 2);

    return;
}


static void mlacp_conn_handler_fdb(struct CSM* csm)
{
    if (!csm)
        return;
    ICCPD_LOG_DEBUG(__FUNCTION__, " Sync MAC addresses to peer ");
    mlacp_sync_mac(csm);
    return;
}

void mlacp_fix_bridge_mac(struct CSM* csm)
{
    char syscmd[128];
    int ret = 0;
    char macaddr[64];
    uint8_t null_mac[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    if (memcmp(MLACP(csm).system_id, null_mac, ETHER_ADDR_LEN) != 0)
    {
        memset(macaddr, 0, 64);
        snprintf(macaddr, 64, "%02x:%02x:%02x:%02x:%02x:%02x",
             MLACP(csm).system_id[0], MLACP(csm).system_id[1], MLACP(csm).system_id[2],
             MLACP(csm).system_id[3], MLACP(csm).system_id[4], MLACP(csm).system_id[5]);

        /*When changing the mac of a vlan member port, the mac of Bridge will be changed.*/
        /*The Bridge mac can not be the same as peer system id, so fix the Bridge MAC address here.*/
        sprintf(syscmd, "ip link set dev Bridge address %s > /dev/null 2>&1", macaddr);
        ret = system(syscmd);
        ICCPD_LOG_DEBUG(__FUNCTION__, "  %s  ret = %d", syscmd, ret);
    }

    return;
}

/*****************************************
* Peer connect/disconnect handler
*
* ***************************************/
void mlacp_peer_conn_handler(struct CSM* csm)
{
    struct LocalInterface *lif = NULL;
    struct PeerInterface* peer_if;
    static int once_connected = 0;
    struct System* sys = NULL;
    struct If_info * cif = NULL;

    if (!csm)
        return;

    if ((sys = system_get_instance()) == NULL)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Invalid system instance");
        return;
    }

    if (csm->warm_reboot_disconn_time != 0)
    {
        /*If peer reconnected, reset peer disconnect time*/
        csm->warm_reboot_disconn_time = 0;
        ICCPD_LOG_DEBUG(__FUNCTION__, "Peer warm reboot and reconnect, reset warm disconnect time!");
    }

    if (csm->peer_link_if)
    {
        set_peerlink_mlag_port_learn(csm->peer_link_if, 0);
        set_peerlink_learn_kernel(csm, 0, 7);
    }

    ICCPD_LOG_NOTICE("ICCP_FSM", "ICCP session up: warm reboot %s, role %s",
        (sys->warmboot_start == WARM_REBOOT) ? "yes" : "no",
        (csm->role_type == STP_ROLE_STANDBY) ? "standby" : "active");

    /*If peer connect again, don't flush FDB*/
    if (once_connected == 0)
    {
        once_connected = 1;
        mlacp_fix_bridge_mac(csm);
        // do not required to flush FDB
        //if (sys->warmboot_start != WARM_REBOOT)
          //  mlacp_clean_fdb();
    }

    sys->csm_trans_time = time(NULL);
    mlacp_conn_handler_fdb(csm);

    LIST_FOREACH(lif, &(MLACP(csm).lif_list), mlacp_next)
    {
        if (lif->type == IF_T_PORT_CHANNEL)
        {
            mlacp_portchannel_state_handler(csm, lif, (lif->state == PORT_STATE_UP) ? 1 : 0);
        }
    }

    /* Send ICCP up update to Mclagsyncd */
    mlacp_link_set_iccp_state(csm->mlag_id, true);

    /* Send remote interface status update to Mclagsyncd */
    LIST_FOREACH(peer_if, &(MLACP(csm).pif_list), mlacp_next)
    {
        mlacp_link_set_remote_if_state(
            csm->mlag_id, peer_if->name,
            (peer_if->state == PORT_STATE_UP)? true : false);
    }
    /* Port isolation is cleaned up when session goes down via
     * peerlink_port_isolate_cleanup(). MlagOrch blocks all traffic from
     * ISL to all MLAG interfaces to avoid packet duplicate and transient
     * loop to cover the case where peer link is still up when ICCP goes
     * down. On session up, update port isolation group based on the
     * latest remote interface state. This is needed to cover the case
     * where all remote MLAG interfaces are down after ICCP comes back up
     */
    update_peerlink_isolate_from_all_csm_lif(csm);

    if (csm->peer_link_if)
    {
        update_vlan_if_mac_on_iccp_up(csm->peer_link_if, 1, NULL);
    }

    sync_unique_ip();
    return;
}

void mlacp_peer_disconn_fdb_handler(struct CSM* csm)
{
    struct MACMsg* mac_msg = NULL, *mac_temp = NULL;

    RB_FOREACH_SAFE (mac_msg, mac_rb_tree, &MLACP(csm).mac_rb, mac_temp)
    {
        ICCPD_LOG_DEBUG("ICCP_FDB", "ICCP session down: existing flag %d interface %s, MAC %s vlan-id %d,"
                " pending_del %s", mac_msg->age_flag, mac_msg->ifname,
                mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid,
                (mac_msg->pending_local_del) ? "true":"false");

        if (strcmp(mac_msg->ifname, csm->peer_itf_name) == 0)
        {
            mac_msg->age_flag |= MAC_AGE_PEER;

            /* local and peer both aged, to be deleted*/
            // delete peer-link check, delete all remote macs which are aged local and remote.
            if ((mac_msg->age_flag == (MAC_AGE_LOCAL | MAC_AGE_PEER)) || mac_msg->pending_local_del)
            {
                ICCPD_LOG_DEBUG("ICCP_FDB", "ICCP session down: del MAC pointing to peer_link for %s, "
                    "MAC %s vlan-id %d", mac_msg->ifname, mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid);

                /*Send mac del message to mclagsyncd, may be already deleted*/
                del_mac_from_chip(mac_msg);

                MAC_RB_REMOVE(mac_rb_tree, &MLACP(csm).mac_rb, mac_msg);
                // free only if not in change list to be send to peer node,
                // else free is taken care after sending the update to peer
                if (!MAC_IN_MSG_LIST(&(MLACP(csm).mac_msg_list), mac_msg, tail))
                {
                    free(mac_msg);
                }
            }
        }
        else
        {
            if (!mac_msg->age_flag)
            {
                // MAC learned on both nodes convert to local not update to ASIC required.
                mac_msg->age_flag = MAC_AGE_PEER;
                ICCPD_LOG_DEBUG("ICCP_FDB", "ICCP session down: MAC learned on both nodes update to local only"
                    " flag %d interface %s, MAC %s vlan-id %d", mac_msg->age_flag, mac_msg->ifname,
                    mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid);
            }
            else if (mac_msg->age_flag == MAC_AGE_LOCAL)
            {
                // MAC is remote pointing to MCLAG PO convert to local
                add_mac_to_chip(mac_msg, MAC_TYPE_DYNAMIC_LOCAL);
                mac_msg->age_flag = MAC_AGE_PEER;
                ICCPD_LOG_DEBUG("ICCP_FDB", "ICCP session down: MAC is remote convert to local"
                    " flag %d interface %s, MAC %s vlan-id %d", mac_msg->age_flag, mac_msg->ifname,
                    mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid);
            }
            //else MAC is local (mac_msg->age_flag == MAC_AGE_PEER) no changes required
        }
    }
}

void mlacp_peer_disconn_handler(struct CSM* csm)
{
    uint8_t null_mac[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    struct LocalInterface* lif = NULL;
    struct LocalInterface* peer_link_if = NULL;
    struct PeerInterface* peer_if;
    struct Msg* msg = NULL;
    struct MACMsg* mac_msg = NULL, *mac_temp = NULL;
    struct System* sys = NULL;
    uint8_t remote_system_mac[ETHER_ADDR_LEN];

    if (!csm)
        return;

    if ((sys = system_get_instance()) == NULL)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Invalid system instance");
        return;
    }
    if (MLACP(csm).current_state != MLACP_STATE_INIT)
    {
        ICCPD_LOG_NOTICE("ICCP_FSM",
            "ICCP session down: warm reboot %s, role %s, sync_state %s",
            (sys->warmboot_exit == WARM_REBOOT) ? "yes" : "no",
            (csm->role_type == STP_ROLE_STANDBY) ? "standby" : "active",
            mlacp_state(csm));
    }
    /*If warm reboot, don't change FDB and MAC address*/
    if (sys->warmboot_exit == WARM_REBOOT)
        return;

    if (csm->peer_warm_reboot_time != 0)
    {
        /*If peer disconnected, recover peer to normal reboot for next time*/
        csm->peer_warm_reboot_time = 0;
        /*peer connection must be establised again within 90s
          from last disconnection for peer warm reboot*/
        time(&csm->warm_reboot_disconn_time);
        ICCPD_LOG_DEBUG(__FUNCTION__, "Peer warm reboot and disconnect, recover to normal reboot for next time!");
        return;
    }

    mlacp_peer_disconn_fdb_handler(csm);

    /* Send ICCP down update to Mclagsyncd before clearing all port isolation
     * so that mclagsync can differentiate between session down and all remote
     * MLAG interface down
     */
    mlacp_link_set_iccp_state(csm->mlag_id, false);

    /* Clean all port block*/
    peerlink_port_isolate_cleanup(csm);

    memset(remote_system_mac, 0, ETHER_ADDR_LEN);
    memcpy(remote_system_mac, MLACP(csm).remote_system.system_id, ETHER_ADDR_LEN);
    memcpy(MLACP(csm).remote_system.system_id, null_mac, ETHER_ADDR_LEN);

    /*If peer is disconnected, recover the MAC address.*/
    LIST_FOREACH(lif, &(MLACP(csm).lif_list), mlacp_next)
    {
        if (csm->role_type == STP_ROLE_STANDBY)
            recover_if_ipmac_on_standby(lif, 3);

        /* Re-enable traffic tx/rx for MLAG interface regardless of its state
         */
        if (lif->is_traffic_disable)
        {
            mlacp_link_enable_traffic_distribution(lif);
        }
    }

    if (csm->peer_link_if)
    {
        update_vlan_if_mac_on_iccp_up(csm->peer_link_if, 0, remote_system_mac);
    }

    ICCPD_LOG_DEBUG(__FUNCTION__, "Peer disconnect %u times",
        SYSTEM_GET_SESSION_DOWN_COUNTER(sys));
    SYSTEM_INCR_SESSION_DOWN_COUNTER(sys);

    /* On standby, system ID is reverted back to its local system ID.
     * Update Mclagsyncd
     * */
    if (csm->role_type == STP_ROLE_STANDBY)
        mlacp_link_set_iccp_system_id(csm->mlag_id, MLACP(csm).system_id);

    /* Delete remote interface info */
    LIST_FOREACH(peer_if, &(MLACP(csm).pif_list), mlacp_next)
    {
        mlacp_link_del_remote_if_info(csm->mlag_id, peer_if->name);
    }
    return;
}

void mlacp_peerlink_up_handler(struct CSM* csm)
{
    struct Msg* msg = NULL;
    struct MACMsg* mac_msg = NULL;

    if (!csm)
        return;

    ICCPD_LOG_DEBUG("ICCP_FSM", "PEER_LINK %s up: sync_state %s",
        csm->peer_itf_name, mlacp_state(csm));

    /*If peer link up, set all the mac that point to the peer-link in ASIC*/
    RB_FOREACH (mac_msg, mac_rb_tree, &MLACP(csm).mac_rb)
    {
        /* Find the MAC that the port is peer-link to be added*/
        if (strcmp(mac_msg->ifname, csm->peer_itf_name) != 0)
            continue;

        ICCPD_LOG_DEBUG("ICCP_FDB", "Peer link up, add MAC to ASIC for peer-link: %s, "
                "MAC %s vlan-id %d", mac_msg->ifname, mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid);

        /*Send mac add message to mclagsyncd, local age flag is already set*/
        add_mac_to_chip(mac_msg, mac_msg->fdb_type);
    }

    return;
}

void mlacp_peerlink_down_handler(struct CSM* csm)
{
    struct MACMsg* mac_temp = NULL;
    struct MACMsg* mac_msg = NULL;

    if (!csm)
        return;

    ICCPD_LOG_DEBUG("ICCP_FSM", "PEER_LINK %s down: sync_state %s",
        csm->peer_itf_name, mlacp_state(csm));

    /*If peer link down, remove all the mac that point to the peer-link*/
    RB_FOREACH_SAFE (mac_msg, mac_rb_tree, &MLACP(csm).mac_rb, mac_temp)
    {
        /* Find the MAC that the port is peer-link to be deleted*/
        if (strcmp(mac_msg->ifname, csm->peer_itf_name) != 0)
            continue;

        if (!mac_msg->pending_local_del)
            mac_msg->age_flag = set_mac_local_age_flag(csm, mac_msg, 1, 1);

        ICCPD_LOG_DEBUG("ICCP_FDB", "Peer link down, del MAC for peer-link: %s,"
            " MAC %s vlan-id %d", mac_msg->ifname, mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid);

        /*Send mac del message to mclagsyncd*/
        del_mac_from_chip(mac_msg);

        /*If peer is not age, keep the MAC in mac_rb, but ASIC is deleted*/
        if (mac_msg->age_flag == (MAC_AGE_LOCAL | MAC_AGE_PEER))
        {
            /*If local and peer both aged, del the mac*/
            MAC_RB_REMOVE(mac_rb_tree, &MLACP(csm).mac_rb, mac_msg);

            // free only if not in change list to be send to peer node,
            // else free is taken care after sending the update to peer
            if (!MAC_IN_MSG_LIST(&(MLACP(csm).mac_msg_list), mac_msg, tail))
            {
                free(mac_msg);
            }
        }
    }

    SYSTEM_INCR_PEER_LINK_DOWN_COUNTER(system_get_instance());
    return;
}


/*****************************************
* Po add/remove handler
*
*****************************************/
void mlacp_mlag_link_add_handler(struct CSM *csm, struct LocalInterface *lif)
{
    if (!csm || !lif)
        return;
    if (MLACP(csm).current_state != MLACP_STATE_EXCHANGE)
        return;

    //enable peerlink isolation only if the both mclag interfaces are up
    update_peerlink_isolate_from_lif(csm, lif, lif->po_active);

    //if it is standby node and peer interface is configured, update
    //standby node mac to active's mac for this lif
    if (csm->role_type == STP_ROLE_STANDBY)
    {
        struct PeerInterface* pif=NULL;
        pif = peer_if_find_by_name(csm, lif->name);

        if (pif)
        {
            update_if_ipmac_on_standby(lif, 4);
            mlacp_link_set_iccp_system_id(csm->mlag_id, lif->mac_addr);
        }
    }

    return;
}

void mlacp_mlag_link_del_handler(struct CSM *csm, struct LocalInterface *lif)
{
    if (!csm || !lif)
        return;

    if (MLACP(csm).current_state != MLACP_STATE_EXCHANGE)
        return;

    set_peerlink_mlag_port_isolate(csm, lif, 0, true);

    return;
}

int iccp_connect_syncd()
{
    struct System* sys = NULL;
    int ret = 0;
    int fd = 0;
    struct sockaddr_in serv;
    static int count = 0;
    struct epoll_event event;

    if ((sys = system_get_instance()) == NULL)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Invalid system instance");
        goto conn_fail;
    }
    if (sys->sync_fd >= 0)
        return 0;

    /*Print the fail log message every 60s*/
    if (count >= 600)
    {
        count = 0;
    }
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        if (count == 0)
            ICCPD_LOG_WARN(__FUNCTION__, "Failed to create unix socket: %s", strerror(errno));
        goto conn_fail;
    }

    /* Make server socket. */
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(2626);
#ifdef HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
    serv.sin_len = sizeof(struct sockaddr_in);
#endif /* HAVE_STRUCT_SOCKADDR_IN_SIN_LEN */
    serv.sin_addr.s_addr = htonl(0x7f000006);

    ret = connect(fd, (struct sockaddr *)&serv, sizeof(serv));
    if (ret < 0)
    {
        if (count == 0)
            ICCPD_LOG_WARN(__FUNCTION__, "Failed to connect to mclag syncd: errno str %s", strerror(errno));
        close(fd);
        goto conn_fail;
    }

    ICCPD_LOG_NOTICE(__FUNCTION__, "Success to link syncd");
    sys->sync_fd = fd;

    event.data.fd = fd;
    event.events = EPOLLIN;
    ret = epoll_ctl(sys->epoll_fd, EPOLL_CTL_ADD, fd, &event);

    count = 0;
    return 0;

conn_fail:
    if (count == 0)
        ICCPD_LOG_DEBUG(__FUNCTION__, "Mclag syncd socket connect fail");

    count++;

    return MCLAG_ERROR;
}

void syncd_info_close()
{
    struct System* sys = NULL;

    if ((sys = system_get_instance()) == NULL)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Invalid system instance");
        return;
    }

    if (sys->sync_fd > 0)
    {
        close(sys->sync_fd);
        sys->sync_fd = -1;
    }

    return;
}

int iccp_get_receive_fdb_sock_fd(struct System *sys)
{
    return sys->sync_fd;
}

/*When received MAC add and del packets from mclagsyncd, update mac information*/
void do_mac_update_from_syncd(uint8_t mac_addr[ETHER_ADDR_LEN], uint16_t vid, char *ifname, uint8_t fdb_type, uint8_t op_type)
{
    struct System *sys = NULL;
    struct CSM *csm = NULL;
    struct Msg *msg = NULL;
    struct MACMsg *mac_msg = NULL, *mac_info = NULL, *new_mac_msg = NULL;
    struct MACMsg mac_find;
    uint8_t mac_exist = 0;
    char buf[MAX_BUFSIZE];
    size_t msg_len = 0;
    uint8_t from_mclag_intf = 0;/*0: orphan port, 1: MCLAG port*/
    struct CSM *first_csm = NULL;
    uint8_t null_mac[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    struct LocalInterface *lif_po = NULL, *mac_lif = NULL;

    if (!(sys = system_get_instance()))
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Invalid system instance");
        return;
    }

    if (memcmp(mac_addr, null_mac, ETHER_ADDR_LEN) == 0)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Invalid MAC address from syncd do not add.");
        return;
    }

    /* create MAC msg*/
    memset(buf, 0, MAX_BUFSIZE);
    msg_len = sizeof(struct MACMsg);
    mac_msg = (struct MACMsg*)buf;
    mac_msg->op_type = op_type;
    mac_msg->fdb_type = fdb_type;
    memcpy(mac_msg->mac_addr, mac_addr, ETHER_ADDR_LEN);
    mac_msg->vid = vid;

    mac_msg->age_flag = 0;
    mac_msg->pending_local_del = 0;

    ICCPD_LOG_DEBUG("ICCP_FDB", "MAC update from mclagsyncd: vid %d mac %s port %s type: %d optype %s  ",
            vid, mac_addr_to_str(mac_addr), ifname, fdb_type, op_type == MAC_SYNC_ADD ? "add" : "del");
    /*Debug*/
    #if 0
    /* dump receive MAC info*/
    fprintf(stderr, "\n======== MAC Update==========\n");
    fprintf(stderr, "  MAC    =  %s \n", mac_addr_to_str(mac_addr));
    fprintf(stderr, "  vlan id = %d\n", vid);
    fprintf(stderr, "  fdb type = %s\n", fdb_type == MAC_TYPE_STATIC ? "static" : "dynamic");
    fprintf(stderr, "  op type = %s\n", op_type == MAC_SYNC_ADD ? "add" : "del");
    fprintf(stderr, "==============================\n");
    #endif

    /* Find MLACP itf, may be mclag enabled port-channel*/
    LIST_FOREACH(csm, &(sys->csm_list), next)
    {
        if (csm && !first_csm)
        {
            /*Record the first CSM, only one CSM in the system currently*/
            first_csm = csm;
        }

        /*If MAC is from peer-link, break; peer-link is not in MLACP(csm).lif_list*/
        if (strcmp(ifname, csm->peer_itf_name) == 0)
            break;

        LIST_FOREACH(lif_po, &(MLACP(csm).lif_list), mlacp_next)
        {
            if (lif_po->type != IF_T_PORT_CHANNEL)
                continue;

            if (strcmp(lif_po->name, ifname) == 0)
            {
                from_mclag_intf = 1;
                break;
            }
        }

        if (from_mclag_intf == 1)
            break;
    }

    if (!first_csm)
        return;

    /*If support multiple CSM, the MAC list of orphan port must be moved to sys->mac_rb*/
    csm = first_csm;

    struct PeerInterface* pif = NULL;
    pif = peer_if_find_by_name(csm, ifname);

    memset(&mac_find, 0, sizeof(struct MACMsg));
    mac_find.vid = vid;
    memcpy(mac_find.mac_addr,mac_addr, ETHER_ADDR_LEN);

    mac_info = RB_FIND(mac_rb_tree, &MLACP(csm).mac_rb ,&mac_find);
    if(mac_info)
    {
        mac_exist = 1;
        ICCPD_LOG_DEBUG("ICCP_FDB", "MAC update from mclagsyncd: RB_FIND success for the MAC entry : %s, "
            " vid: %d , ifname %s, type: %d, age flag: %d", mac_addr_to_str(mac_info->mac_addr),
            mac_info->vid, mac_info->ifname, mac_info->fdb_type, mac_info->age_flag );
    }

    /*handle mac add*/
    if (op_type == MAC_SYNC_ADD)
    {
        /* Find local itf*/
        if (!(mac_lif = local_if_find_by_name(ifname)))
        {
            ICCPD_LOG_ERR(__FUNCTION__, " interface %s not present failed "
                "to add MAC %s vlan %d", ifname, mac_addr_to_str(mac_addr), vid);
            return;
        }

        sprintf(mac_msg->ifname, "%s", ifname);
        sprintf(mac_msg->origin_ifname, "%s", ifname);

        /*If the recv mac port is peer-link, no need to handle*/
        if (strcmp(csm->peer_itf_name, mac_msg->ifname) == 0)
        {
            ICCPD_LOG_DEBUG("ICCP_FDB", "MAC learn received on peer_link %s ignore MAC %s vlan %d",
                mac_msg->ifname, mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid);
            return;
        }

        /*same MAC exist*/
        if (mac_exist)
        {
            /*If the current mac port is peer-link, it will handle by port up event*/
            /*if(strcmp(csm->peer_itf_name, mac_info->ifname) == 0)
               {
                return;
               }*/
            if (mac_lif->state == PORT_STATE_DOWN)
            {
                ICCPD_LOG_DEBUG("ICCP_FDB", "MAC update from mclagsyncd: MAC add received, "
                    "MAC exists interface %s down, MAC %s vlan %d, is mclag interface : %s ",
                    mac_msg->ifname, mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid,
                    from_mclag_intf ? "true":"false" );

                // if from mclag intf update mac to point to peer_link.
                //else ignore mac.
                if (from_mclag_intf  && pif && (pif->state == PORT_STATE_UP))
                {
                    ICCPD_LOG_DEBUG("ICCP_FDB", "MAC update from mclagsyncd: MAC add received, "
                        "MAC exists interface %s down, point to peer link MAC %s, vlan %d, is pending local del : %s ",
                        mac_msg->ifname, mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid,
                        mac_info->pending_local_del ? "true":"false" );

                    mac_info->pending_local_del = 1;
                    mac_info->fdb_type = mac_msg->fdb_type;
                    memcpy(&mac_info->origin_ifname, mac_msg->ifname, MAX_L_PORT_NAME);

                    //existing mac must be pointing to peer_link, else update if info and send to syncd
                    if (strcmp(mac_info->ifname, csm->peer_itf_name) == 0)
                    {
                        add_mac_to_chip(mac_info, mac_msg->fdb_type);
                    }
                    else
                    {
                        // this for the case of MAC move , existing mac may point to different interface.
                        // need to update the ifname and update to syncd.
                        memcpy(&mac_info->ifname, csm->peer_itf_name, MAX_L_PORT_NAME);
                        add_mac_to_chip(mac_info, mac_msg->fdb_type);
                    }

                    return;
                }

                //else
                //Update the MAC
            }

            /* update MAC*/
            if (mac_info->fdb_type != mac_msg->fdb_type
                || strcmp(mac_info->ifname, mac_msg->ifname) != 0
                || strcmp(mac_info->origin_ifname, mac_msg->ifname) != 0)
            {
                mac_info->fdb_type = mac_msg->fdb_type;
                sprintf(mac_info->ifname, "%s", mac_msg->ifname);
                sprintf(mac_info->origin_ifname, "%s", mac_msg->ifname);

                /*Remove MAC_AGE_LOCAL flag*/
                mac_info->age_flag = set_mac_local_age_flag(csm, mac_info, 0, 1);

                ICCPD_LOG_DEBUG("ICCP_FDB", "MAC update from mclagsyncd: Update MAC %s, vlan %d ifname %s",
                    mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid, mac_msg->ifname);
                // MAC is local now Del entry from MCLAG_FDB_TABLE if peer not aged.
                if (!(mac_msg->age_flag & MAC_AGE_PEER))
                {
                    ICCPD_LOG_DEBUG("ICCP_FDB", " MAC update from mclagsyncd: MAC move Update MAC remote to local %s, vlan %d"
                            " ifname %s, del entry from MCLAG_FDB_TABLE",
                            mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid, mac_msg->ifname);
		    mac_info->age_flag = MAC_AGE_PEER;
                    del_mac_from_chip(mac_msg);
                }
            }
            else
            {
                /*All info are the same, Remove MAC_AGE_LOCAL flag, then return*/
                /*In theory, this will be happened that mac age and then learn*/
                mac_info->age_flag = set_mac_local_age_flag(csm, mac_info, 0, 1);
                ICCPD_LOG_DEBUG("ICCP_FDB", "MAC update from mclagsyncd: Duplicate update MAC %s, vlan %d ifname %s",
                        mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid, mac_msg->ifname);
                // MAC is local now Del entry from MCLAG_FDB_TABLE if peer not aged.
                if (!(mac_msg->age_flag & MAC_AGE_PEER))
                {
                    ICCPD_LOG_DEBUG("ICCP_FDB", "MAC update from mclagsyncd: Update MAC remote to local %s, vlan %d"
                            " ifname %s, del entry from MCLAG_FDB_TABLE",
                            mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid, mac_msg->ifname);
                    del_mac_from_chip(mac_msg);
                }
                return;
            }
        }
        else/*same MAC not exist*/
        {
            /*If the port the mac learn is change to down before the mac
               sync to iccp, this mac must be deleted */
#if 0
            if ((mac_lif->state == PORT_STATE_DOWN))
            {
                if ((!from_mclag_intf) && (mac_msg->fdb_type != MAC_TYPE_STATIC))
                {
                    //ignore mac add
                    ICCPD_LOG_DEBUG("ICCP_FDB", "MAC update from mclagsyncd: mclag interface %s down, MAC %s,"
                       " vlan %d ignore mac add.", ifname, mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid);
                    return;
                }
            }
#endif

            // If both local and remote MCLAG interfaces are down, ignore MAC and send delete to HW.
            if (from_mclag_intf && (mac_lif->state == PORT_STATE_DOWN) && pif && (pif->state == PORT_STATE_DOWN))
            {
                ICCPD_LOG_DEBUG("ICCP_FDB", "MAC update from mclagsyncd: mclag interface %s down on local and remote ignore MAC %s,"
                    " vlan %d ", ifname, mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid);
                del_mac_from_chip(mac_msg);
                return;
            }
            /*set MAC_AGE_PEER flag before send this item to peer*/
            mac_msg->age_flag |= MAC_AGE_PEER;
            ICCPD_LOG_DEBUG("ICCP_FDB", "MAC update from mclagsyncd: Add peer age flag, age %d interface %s, "
                "MAC %s vlan-id %d ", mac_msg->age_flag, mac_msg->ifname,
                mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid);
            mac_msg->op_type = MAC_SYNC_ADD;

            /*enqueue mac to mac-list*/
            if (iccp_csm_init_mac_msg(&new_mac_msg, (char*)mac_msg, msg_len) == 0)
            {
                RB_INSERT(mac_rb_tree, &MLACP(csm).mac_rb, new_mac_msg);

                ICCPD_LOG_DEBUG("ICCP_FDB", "MAC update from mclagsyncd: MAC-list enqueue interface %s, "
                        "MAC %s vlan-id %d", mac_msg->ifname,
                        mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid);

                //if port is down do not sync the MAC.
                // For MAC learned on MCLAG interface point to peer_link.
                // MAC learned on orphan port save MAC, when Orphan port is UP sync MAC
                if (mac_lif->state == PORT_STATE_DOWN)
                {
                    if (from_mclag_intf && pif && (pif->state == PORT_STATE_UP))
                    {
                        mac_msg->pending_local_del = 1;
                        memcpy(&mac_msg->ifname, csm->peer_itf_name, MAX_L_PORT_NAME);
                        add_mac_to_chip(mac_msg, mac_msg->fdb_type);
                        ICCPD_LOG_DEBUG("ICCP_FDB", "MAC update from mclagsyncd: mclag interface %s down, MAC %s,"
                           " vlan %d point to peer link %s", ifname, mac_addr_to_str(mac_msg->mac_addr),
                           mac_msg->vid, mac_msg->ifname);
                    }
                    return;
                }

                if ((MLACP(csm).current_state == MLACP_STATE_EXCHANGE))
                {
                    TAILQ_INSERT_TAIL(&(MLACP(csm).mac_msg_list), new_mac_msg, tail);

                    ICCPD_LOG_DEBUG("ICCP_FDB", "MAC update from mclagsyncd: MAC-msg-list enqueue interface %s, "
                        "MAC %s vlan-id %d, age_flag %d", new_mac_msg->ifname,
                        mac_addr_to_str(new_mac_msg->mac_addr), new_mac_msg->vid, new_mac_msg->age_flag);
                }
            }
            else
                ICCPD_LOG_DEBUG("ICCP_FDB", "MAC update from mclagsyncd: Failed to enqueue interface %s, MAC %s vlan-id %d",
                    mac_msg->ifname, mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid);
        }
    }
    else/*handle mac del*/
    {
        /*same MAC exist*/
        if (mac_exist)
        {
            /*orphan port mac or origin from_mclag_intf but state is down*/
            if (strcmp(mac_info->ifname, csm->peer_itf_name) == 0)
            {
                if (mac_info->pending_local_del)
                {
                    //do not delete the MAC.
                    ICCPD_LOG_DEBUG("ICCP_FDB", "MAC update from mclagsyncd: do not del pending MAC on %s(peer-link), "
                        "MAC %s vlan-id %d", mac_info->ifname,
                        mac_addr_to_str(mac_info->mac_addr), mac_info->vid);
                    return;
                }
                /*Set MAC_AGE_LOCAL flag*/
                mac_info->age_flag = set_mac_local_age_flag(csm, mac_info, 1, 1);

                if (mac_info->age_flag == (MAC_AGE_LOCAL | MAC_AGE_PEER))
                {
                    ICCPD_LOG_DEBUG("ICCP_FDB", "MAC update from mclagsyncd: Recv MAC del interface %s(peer-link), "
                        "MAC %s vlan-id %d", mac_info->ifname,
                        mac_addr_to_str(mac_info->mac_addr), mac_info->vid);

                    if (mac_info->add_to_syncd)
                    {
                        del_mac_from_chip(mac_info);
                    }

                    /*If peer link is down, del the mac*/
                    MAC_RB_REMOVE(mac_rb_tree, &MLACP(csm).mac_rb, mac_info);

                    // free only if not in change list to be send to peer node,
                    // else free is taken care after sending the update to peer
                    if (!MAC_IN_MSG_LIST(&(MLACP(csm).mac_msg_list), mac_info, tail))
                    {
                        free(mac_info);
                    }
                }
                else if (csm->peer_link_if && csm->peer_link_if->state != PORT_STATE_DOWN)
                {
                    /*peer-link learn mac is control by iccpd, ignore the chip del info*/
                    add_mac_to_chip(mac_info, mac_info->fdb_type);

                    ICCPD_LOG_DEBUG("ICCP_FDB", "MAC update from mclagsyncd: Recv MAC del interface %s(peer-link is up), "
                        "add back MAC %s vlan-id %d", mac_info->ifname,
                        mac_addr_to_str(mac_info->mac_addr), mac_info->vid);
                }

                return;
            }

            /*Add MAC_AGE_LOCAL flag*/
            mac_info->age_flag = set_mac_local_age_flag(csm, mac_info, 1, 1);

            if (mac_info->age_flag == (MAC_AGE_LOCAL | MAC_AGE_PEER))
            {
                ICCPD_LOG_DEBUG("ICCP_FDB", "MAC update from mclagsyncd: Recv MAC del interface %s, "
                    "MAC %s vlan-id %d", mac_info->ifname,
                    mac_addr_to_str(mac_info->mac_addr), mac_info->vid);

                //before removing the MAC send del to syncd if added before.
                if (mac_info->add_to_syncd)
                {
                    del_mac_from_chip(mac_info);
                }
                /*If local and peer both aged, del the mac (local orphan mac is here)*/
                MAC_RB_REMOVE(mac_rb_tree, &MLACP(csm).mac_rb, mac_info);

                // free only if not in change list to be send to peer node,
                // else free is taken care after sending the update to peer
                if (!MAC_IN_MSG_LIST(&(MLACP(csm).mac_msg_list), mac_info, tail))
                {
                    free(mac_info);
                }
            }
            else
            {
                ICCPD_LOG_DEBUG("ICCP_FDB", "MAC update from mclagsyncd: Recv MAC del interface %s, "
                    "MAC %s vlan-id %d, peer is not age, add back to chip",
                    mac_info->ifname, mac_addr_to_str(mac_info->mac_addr), mac_info->vid);

                if (from_mclag_intf && lif_po && lif_po->state == PORT_STATE_DOWN)
                {
                    /*If local if is down, redirect the mac to peer-link*/
                    if (strlen(csm->peer_itf_name) != 0)
                    {
                        memcpy(&mac_info->ifname, csm->peer_itf_name, MAX_L_PORT_NAME);

                        if (csm->peer_link_if && csm->peer_link_if->state == PORT_STATE_UP)
                        {
                            add_mac_to_chip(mac_info, mac_info->fdb_type);
                            ICCPD_LOG_DEBUG("ICCP_FDB", "MAC update from mclagsyncd: Recv MAC del interface %s(down), "
                                "MAC %s vlan-id %d, redirect to peer-link",
                                mac_info->ifname, mac_addr_to_str(mac_info->mac_addr), mac_info->vid);
                        }
                    }

                    return;
                }

                /*If local is aged but peer is not aged, Send mac add message to mclagsyncd*/
                /*it is from_mclag_intf and port state is up, local orphan mac can not be here*/
                /* Find local itf*/
                if (!(mac_lif = local_if_find_by_name(mac_info->ifname)))
                    return;
                if (mac_lif->state == PORT_STATE_UP)
                    add_mac_to_chip(mac_info, mac_info->fdb_type);
            }
        }
    }

    return;
}

int iccp_mclagsyncd_mclag_domain_cfg_handler(struct System *sys, char *msg_buf)
{
    struct IccpSyncdHDr * msg_hdr;
    struct mclag_domain_cfg_info* cfg_info;
    int count, i = 0;
    char system_mac_str[ETHER_ADDR_STR_LEN];

    msg_hdr = (struct IccpSyncdHDr *)msg_buf;

    count = (msg_hdr->len- sizeof(struct IccpSyncdHDr ))/sizeof(struct mclag_domain_cfg_info);
    ICCPD_LOG_DEBUG(__FUNCTION__, "recv domain cfg msg ; count %d   ",count);

    for (i = 0; i < count; i++)
    {
        cfg_info = (struct mclag_domain_cfg_info *)((char *)(msg_buf) + sizeof(struct IccpSyncdHDr) + i * sizeof(struct mclag_domain_cfg_info));

        memcpy(system_mac_str, mac_addr_to_str(cfg_info->system_mac), sizeof(system_mac_str));

        ICCPD_LOG_NOTICE(__FUNCTION__, "recv cfg msg ; domain_id:%d op_type:%d attr_bmap:0x%x local_ip:%s peer_ip:%s peer_ifname:%s system_mac:%s session_timeout:%d keepalive_time:%d",cfg_info->domain_id, cfg_info->op_type, cfg_info->attr_bmap, cfg_info->local_ip, cfg_info->peer_ip, cfg_info->peer_ifname, system_mac_str, cfg_info->session_timeout, cfg_info->keepalive_time);

        if (cfg_info->op_type == MCLAG_CFG_OPER_ADD || cfg_info->op_type == MCLAG_CFG_OPER_UPDATE) //mclag domain create/update
        {
            if (cfg_info->op_type == MCLAG_CFG_OPER_ADD)
            {
                set_mc_lag_by_id(cfg_info->domain_id);
                set_local_system_id(system_mac_str);
            }

            if(cfg_info->attr_bmap & MCLAG_CFG_ATTR_SRC_ADDR)
            {
                set_local_address(cfg_info->domain_id, cfg_info->local_ip);
            }
            if(cfg_info->attr_bmap & MCLAG_CFG_ATTR_PEER_ADDR)
            {
                set_peer_address(cfg_info->domain_id, cfg_info->peer_ip);
            }

            if(cfg_info->attr_bmap & MCLAG_CFG_ATTR_PEER_LINK)
            {
                set_peer_link(cfg_info->domain_id, cfg_info->peer_ifname);
            }

            if(cfg_info->attr_bmap & MCLAG_CFG_ATTR_KEEPALIVE_INTERVAL)
            {
                if (cfg_info->keepalive_time != -1)
                {
                    set_keepalive_time(cfg_info->domain_id, cfg_info->keepalive_time);
                }
                else
                {
                    set_keepalive_time(cfg_info->domain_id, CONNECT_INTERVAL_SEC);
                }
            }

            if(cfg_info->attr_bmap & MCLAG_CFG_ATTR_SESSION_TIMEOUT)
            {
                if (cfg_info->session_timeout != -1)
                {
                    set_session_timeout(cfg_info->domain_id, cfg_info->session_timeout);
                }
                else
                {
                    set_session_timeout(cfg_info->domain_id, HEARTBEAT_TIMEOUT_SEC);
                }
            }
        } //MCLAG Domain create/update End
        else if (cfg_info->op_type == MCLAG_CFG_OPER_DEL) //mclag domain delete
        {
            unset_mc_lag_by_id(cfg_info->domain_id);
        } //MCLAG Domain delete End
        else if (cfg_info->op_type == MCLAG_CFG_OPER_ATTR_DEL) //mclag domain attribute delete
        {
            if(cfg_info->attr_bmap & MCLAG_CFG_ATTR_PEER_LINK)
            {
                unset_peer_link(cfg_info->domain_id);
            }
            else if(cfg_info->attr_bmap & MCLAG_CFG_ATTR_KEEPALIVE_INTERVAL)
            {
                //reset to default
                set_keepalive_time(cfg_info->domain_id, CONNECT_INTERVAL_SEC);
            }
            else if(cfg_info->attr_bmap & MCLAG_CFG_ATTR_SESSION_TIMEOUT)
            {
                //reset to default
                set_session_timeout(cfg_info->domain_id, HEARTBEAT_TIMEOUT_SEC);
            }
            else if(cfg_info->attr_bmap & MCLAG_CFG_ATTR_SRC_ADDR)
            {
                unset_local_address(cfg_info->domain_id);
            }
            else if(cfg_info->attr_bmap & MCLAG_CFG_ATTR_PEER_ADDR)
            {
                unset_peer_address(cfg_info->domain_id);
            }
        } //MCLAG Domain Attribute delete End
    }

    return 0;
}

int iccp_mclagsyncd_mclag_iface_cfg_handler(struct System *sys, char *msg_buf)
{
    struct IccpSyncdHDr * msg_hdr;
    struct mclag_iface_cfg_info* cfg_info;
    int count, i = 0;

    msg_hdr = (struct IccpSyncdHDr *)msg_buf;

    count = (msg_hdr->len- sizeof(struct IccpSyncdHDr))/sizeof(struct mclag_iface_cfg_info);
    ICCPD_LOG_DEBUG(__FUNCTION__, "recv domain cfg msg ; count %d   ",count);

    for (i =0; i<count; i++)
    {
        cfg_info = (struct mclag_iface_cfg_info*)((char *)(msg_buf) + sizeof(struct IccpSyncdHDr) + i * sizeof(struct mclag_iface_cfg_info));
        ICCPD_LOG_NOTICE(__FUNCTION__, "recv mclag iface cfg msg ; domain_id:%d op_type:%d mclag_iface:%s ",cfg_info->domain_id, cfg_info->op_type, cfg_info->mclag_iface);

        if (cfg_info->op_type == MCLAG_CFG_OPER_ADD)
        {
            iccp_cli_attach_mclag_domain_to_port_channel(cfg_info->domain_id, cfg_info->mclag_iface);
        }
        else if (cfg_info->op_type == MCLAG_CFG_OPER_DEL)
        {
            iccp_cli_detach_mclag_domain_to_port_channel(cfg_info->mclag_iface);
        }
    }
    return 0;
}

int iccp_mclagsyncd_mclag_unique_ip_cfg_handler(struct System *sys, char *msg_buf)
{
    struct IccpSyncdHDr *msg_hdr;
    struct mclag_unique_ip_cfg_info *cfg_info;
    struct LocalInterface *lif = NULL;
    int count = 0, i = 0;
    int sync_add = 0, is_v4 = 0, is_v6 = 0;
    struct Unq_ip_If_info* unq_ip_if = NULL;

    msg_hdr = (struct IccpSyncdHDr *)msg_buf;

    count = (msg_hdr->len- sizeof(struct IccpSyncdHDr))/sizeof(struct mclag_unique_ip_cfg_info);
    ICCPD_LOG_DEBUG(__FUNCTION__, "recv domain cfg msg, count %d ",count);

    for (i =0; i<count; i++)
    {
        cfg_info = (struct mclag_unique_ip_cfg_info*)((char *)(msg_buf) + sizeof(struct IccpSyncdHDr) + i * sizeof(struct mclag_unique_ip_cfg_info));
        ICCPD_LOG_NOTICE(__FUNCTION__, "recv mclag unique ip cfg msg, op_type:%d ifname:%s ",
                cfg_info->op_type, cfg_info->mclag_unique_ip_ifname);

        if (cfg_info->op_type == MCLAG_CFG_OPER_ADD)
        {
            LIST_FOREACH(unq_ip_if, &(sys->unq_ip_if_list), if_next)
            {
                if (strcmp(unq_ip_if->name, cfg_info->mclag_unique_ip_ifname) == 0)
                {
                    break;
                }
            }

            if (!unq_ip_if)
            {
                unq_ip_if = (struct Unq_ip_If_info *)malloc(sizeof(struct Unq_ip_If_info));
                if (!unq_ip_if)
                    return -1;

                snprintf(unq_ip_if->name, MAX_L_PORT_NAME, "%s", cfg_info->mclag_unique_ip_ifname);
                ICCPD_LOG_DEBUG(__FUNCTION__, "Add mclag_unique_ip_ifname %s", unq_ip_if->name);
                LIST_INSERT_HEAD(&(sys->unq_ip_if_list), unq_ip_if, if_next);
            }

        }
        else if (cfg_info->op_type == MCLAG_CFG_OPER_DEL)
        {
            LIST_FOREACH(unq_ip_if, &(sys->unq_ip_if_list), if_next)
            {
                if (strcmp(unq_ip_if->name, cfg_info->mclag_unique_ip_ifname) == 0)
                {
                    ICCPD_LOG_DEBUG(__FUNCTION__, "Del mclag_unique_ip_ifname %s", unq_ip_if->name);
                    LIST_REMOVE(unq_ip_if, if_next);
                    free(unq_ip_if);
                    break;
                }
            }
        }

        lif = local_if_find_by_name(cfg_info->mclag_unique_ip_ifname);
        if (lif)
        {
            if (cfg_info->op_type == MCLAG_CFG_OPER_ADD)
            {
                lif->is_l3_proto_enabled = true;
                sync_add = 1;
                if (lif->ipv4_addr)
                {
                    is_v4 = 1;
                }

                if (lif->prefixlen_v6)
                {
                    is_v6 = 1;
                }
            }
            else if (cfg_info->op_type == MCLAG_CFG_OPER_DEL)
            {
                lif->is_l3_proto_enabled = false;
                sync_add = 0;
                if (lif->ipv4_addr)
                {
                    is_v4 = 1;
                }

                if (lif->prefixlen_v6)
                {
                    is_v6 = 1;
                }
            }

            ICCPD_LOG_DEBUG(__FUNCTION__,"add %d, v4 %d, v6 %d, l3_mode %d, is_l3 %d",
                    sync_add, is_v4, is_v6, lif->l3_mode, local_if_is_l3_mode(lif));

            if (local_if_is_l3_mode(lif))
            {
                if (sync_add) {
                    update_vlan_if_mac_on_standby(lif, 6);
                }

                syn_local_neigh_mac_info_to_peer(lif, sync_add, is_v4, is_v6, 1, 1, 0, 1);
            } else {
                if (!sync_add) {
                    recover_vlan_if_mac_on_standby(lif, 6, NULL);
                    syn_local_neigh_mac_info_to_peer(lif, sync_add, is_v4, is_v6, 1, 1, 0, 1);
                }
            }
        }
    }
    return 0;
}

int iccp_mclagsyncd_vlan_mbr_update_handler(struct System *sys, char *msg_buf)
{
    struct IccpSyncdHDr * msg_hdr;
    struct mclag_vlan_mbr_info* vlan_mbr_info;
    int count, i = 0;
    int add_count = 0;
    int del_count = 0;

    msg_hdr = (struct IccpSyncdHDr *)msg_buf;

    count = (msg_hdr->len- sizeof(struct IccpSyncdHDr))/sizeof(struct mclag_vlan_mbr_info);

    for (i =0; i<count; i++)
    {
        vlan_mbr_info = (struct mclag_vlan_mbr_info*)((char *)(msg_buf) + sizeof(struct IccpSyncdHDr) + i * sizeof(struct mclag_vlan_mbr_info));
        ICCPD_LOG_DEBUG(__FUNCTION__, "recv mclag vlan member updates op_type:%d vlan_id:%d mbr_if:%s ",
                vlan_mbr_info->op_type, vlan_mbr_info->vid, vlan_mbr_info->mclag_iface);

        if (vlan_mbr_info->op_type == MCLAG_CFG_OPER_ADD)
        {
            vlan_mbrship_change_handler(vlan_mbr_info->vid, vlan_mbr_info->mclag_iface, 1 /*add */);
            add_count++;
        }
        else if (vlan_mbr_info->op_type == MCLAG_CFG_OPER_DEL)
        {
            vlan_mbrship_change_handler(vlan_mbr_info->vid, vlan_mbr_info->mclag_iface, 0 /* del */);
            del_count++;
        }
    }
    ICCPD_LOG_NOTICE(__FUNCTION__, "Rx vlan member update count %d add/delete count: %d/%d", count, add_count, del_count);

    return 0;
}

int iccp_receive_fdb_handler_from_syncd(struct System *sys, char *msg_buf)
{
    int count = 0;
    int i = 0;
    struct IccpSyncdHDr * msg_hdr;
    struct mclag_fdb_info * mac_info;

    msg_hdr = (struct IccpSyncdHDr *)msg_buf;

    count = (msg_hdr->len- sizeof(struct IccpSyncdHDr))/sizeof(struct mclag_fdb_info);
    ICCPD_LOG_DEBUG(__FUNCTION__, "recv msg fdb count %d   ",count );

    for (i =0; i<count;i++)
    {
        mac_info = (struct mclag_fdb_info *)&msg_buf[sizeof(struct IccpSyncdHDr )+ i * sizeof(struct mclag_fdb_info)];

        do_mac_update_from_syncd(mac_info->mac, mac_info->vid, mac_info->port_name, mac_info->type, mac_info->op_type);
    }
    return 0;
}

int iccp_mclagsyncd_msg_handler(struct System *sys)
{
    int num_bytes_rxed = 0;
    char *msg_buf = g_iccp_mlagsyncd_recv_buf;
    struct IccpSyncdHDr * msg_hdr;
    int pos = 0;
    int recv_len = 0;
    int num_retry = 0;
    errno = 0;

    if (sys == NULL)
        return MCLAG_ERROR;
    memset(msg_buf, 0, ICCP_MLAGSYNCD_RECV_MSG_BUFFER_SIZE);

    /* read (max_size - msg_size) so that we have space to
       accomodate anything remaining in the last message */
    num_bytes_rxed = recv(sys->sync_fd, msg_buf,
            ICCP_MLAGSYNCD_RECV_MSG_BUFFER_SIZE - MCLAG_MAX_MSG_LEN, MSG_DONTWAIT );

    if (num_bytes_rxed <= 0)
    {
        // if received count is 0 socket is closed.
        if (num_bytes_rxed == 0)
        {
            ICCPD_LOG_WARN("ICCP_FSM", "Recv fom Mclagsyncd read erro:%d ", num_bytes_rxed);
            SYSTEM_INCR_RX_READ_SOCK_ZERO_COUNTER(system_get_instance());
            return MCLAG_ERROR;
        }

        while( num_bytes_rxed < 0 )
        {
            recv_len = recv(sys->sync_fd, msg_buf,
                        ICCP_MLAGSYNCD_RECV_MSG_BUFFER_SIZE - MCLAG_MAX_MSG_LEN, MSG_DONTWAIT );

            if (recv_len == -1)
            {
                if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                {
                    ICCPD_LOG_NOTICE(
                        "ICCP_FSM", "Recv fom Mclagsyncd Non-blocking recv errno %d, num_retry %d",
                        errno, num_retry);
                    ++num_retry;
                    if (num_retry > SYNCD_RECV_RETRY_MAX)
                    {
                        ICCPD_LOG_NOTICE(
                            "ICCP_FSM", "Recv fom Mclagsyncd retry failed recv_len: %d", recv_len);
                        SYSTEM_INCR_RX_RETRY_FAIL_COUNTER(system_get_instance());
                        return MCLAG_ERROR;
                    }
                    else
                    {
                        usleep(SYNCD_RECV_RETRY_INTERVAL_USEC);
                        recv_len = 0;
                    }
                }
                else
                {
                    ICCPD_LOG_NOTICE("ICCP_FSM", "Recv fom Mclagsyncd retry failed recv_len: %d", recv_len);
                    SYSTEM_INCR_HDR_READ_SOCK_ERR_COUNTER(system_get_instance());
                    return MCLAG_ERROR;
                }
            }
            else if (recv_len == 0)
            {
                ICCPD_LOG_NOTICE("ICCP_FSM", "Recv fom Mclagsyncd error %d connection closed ", recv_len );
                SYSTEM_INCR_HDR_READ_SOCK_ZERO_LEN_COUNTER(system_get_instance());
                return MCLAG_ERROR;
            }

            num_bytes_rxed += recv_len;
        }
    }

    num_retry = 0;
    while (pos < num_bytes_rxed) //iterate through all msgs
    {
        if ((num_bytes_rxed - pos) < sizeof(struct IccpSyncdHDr))
        {
            int recv_len = 0, len = 0;
            int pending_len = sizeof(struct IccpSyncdHDr) - (num_bytes_rxed - pos);

            ICCPD_LOG_NOTICE(__FUNCTION__, "Recv fom Mclagsync header less than expected, trying to retrieve %d bytes more ", pending_len);

            while (recv_len < pending_len)
            {
                int remaining_len = pending_len-recv_len;
                len = recv(sys->sync_fd, msg_buf+num_bytes_rxed+recv_len, remaining_len, MSG_DONTWAIT);
                if (len <= 0)
                {
                    if (len == 0)
                    {
                        ICCPD_LOG_WARN("ICCP_FSM", "Recv fom Mclagsync header less than expected data read error; recv_len:%d pending_len:%d ", recv_len, pending_len);
                        SYSTEM_INCR_HDR_READ_SOCK_ZERO_LEN_COUNTER(system_get_instance());
                        return MCLAG_ERROR;
                    }

                    if (len == -1)
                    {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                        {
                            ICCPD_LOG_NOTICE(
                                "ICCP_FSM", "Recv fom Mclagsync header less than expected Non-blocking recv errno %d, num_retry %d",
                                errno, num_retry);
                            ++num_retry;
                            if (num_retry > SYNCD_RECV_RETRY_MAX)
                            {
                                ICCPD_LOG_ERR(
                                    "ICCP_FSM", "Recv fom Mclagsync header less than expected Non-blocking recv() retry failed, len: %d, errno: %d", len, errno);
                                SYSTEM_INCR_RX_RETRY_FAIL_COUNTER(system_get_instance());
                                return MCLAG_ERROR;
                            }
                            else
                            {
                                usleep(SYNCD_RECV_RETRY_INTERVAL_USEC);
                                len = 0;
                            }
                        }
                        else
                        {
                            ICCPD_LOG_WARN("ICCP_FSM", "Recv fom Mclagsyncd header less than expected error; recv_len:%d errno %d",
                                           recv_len, errno);
                            SYSTEM_INCR_HDR_READ_SOCK_ERR_COUNTER(system_get_instance());
                            return MCLAG_ERROR;
                        }
                    }
                }
                ICCPD_LOG_NOTICE("ICCP_FSM", "received %d pending bytes", len);
                recv_len += len;
            }
        }

        msg_hdr = (struct IccpSyncdHDr *)(&msg_buf[pos]);
        ICCPD_LOG_DEBUG(__FUNCTION__, "rcv msg version %d type %d len %d pos:%d num_bytes_rxed:%d ",
                msg_hdr->ver , msg_hdr->type, msg_hdr->len, pos, num_bytes_rxed);

        if (!msg_hdr->len)
        {
            ICCPD_LOG_ERR(__FUNCTION__, "msg length zero!!!!! ");
            return MCLAG_ERROR;
        }
        if (msg_hdr->ver != 1)
        {
            ICCPD_LOG_ERR(__FUNCTION__, "msg version %d wrong!!!!! ", msg_hdr->ver);
            pos += msg_hdr->len;
            continue;
        }
        if ((pos + msg_hdr->len) > num_bytes_rxed)
        {
            int recv_len = 0, len = 0;
            int pending_len = pos + msg_hdr->len - num_bytes_rxed;

            ICCPD_LOG_NOTICE(__FUNCTION__, "Recv fom Mclagsyncd msg less than expected, trying to retrieve %d bytes more ",
                pending_len);

            while (recv_len < pending_len)
            {
                int remaining_len = pending_len-recv_len;
                len = recv(sys->sync_fd, msg_buf+num_bytes_rxed+recv_len, remaining_len, MSG_DONTWAIT);
                if (len <= 0)
                {
                    if (len == 0)
                    {
                        ICCPD_LOG_WARN("ICCP_FSM", "Recv fom Mclagsyncd msg less than expected read error; len:%d ",len);
                        SYSTEM_INCR_TLV_READ_SOCK_ZERO_LEN_COUNTER(system_get_instance());
                        return MCLAG_ERROR;
                    }

                    if (len == -1)
                    {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                        {
                            ICCPD_LOG_NOTICE(
                                "ICCP_FSM", "Recv fom Mclagsyncd msg less than expected Non-blocking recv errno %d, num_retry %d",
                                errno, num_retry);
                            ++num_retry;
                            if (num_retry > SYNCD_RECV_RETRY_MAX)
                            {
                                ICCPD_LOG_ERR("ICCP_FSM", "Recv fom Mclagsyncd msg less than expected Non-blocking recv() retry failed len %d",len);
                                SYSTEM_INCR_RX_RETRY_FAIL_COUNTER(system_get_instance());
                                return MCLAG_ERROR;
                            }
                            else
                            {
                                usleep(SYNCD_RECV_RETRY_INTERVAL_USEC);
                                len = 0;
                            }
                        }
                        else
                        {
                            ICCPD_LOG_WARN("ICCP_FSM", "Recv fom Mclagsyncd msg less than expectedread retry error len:%d , errno %d",
                                           len, errno);
                            SYSTEM_INCR_TLV_READ_SOCK_ERR_COUNTER(system_get_instance());
                            return MCLAG_ERROR;
                        }
                    }

                }
                ICCPD_LOG_NOTICE(__FUNCTION__, "received %d pending bytes", len);
                recv_len += len;
            }
        }

        if (msg_hdr->type == MCLAG_SYNCD_MSG_TYPE_FDB_OPERATION)
        {
            iccp_receive_fdb_handler_from_syncd(sys, &msg_buf[pos]);
        }
        else if (msg_hdr->type == MCLAG_SYNCD_MSG_TYPE_CFG_MCLAG_DOMAIN)
        {
            iccp_mclagsyncd_mclag_domain_cfg_handler(sys, &msg_buf[pos]);
        }
        else if (msg_hdr->type == MCLAG_SYNCD_MSG_TYPE_CFG_MCLAG_IFACE)
        {
            iccp_mclagsyncd_mclag_iface_cfg_handler(sys, &msg_buf[pos]);
        }
        else if (msg_hdr->type == MCLAG_SYNCD_MSG_TYPE_CFG_MCLAG_UNIQUE_IP)
        {
            iccp_mclagsyncd_mclag_unique_ip_cfg_handler(sys, &msg_buf[pos]);
        }
        else if (msg_hdr->type == MCLAG_SYNCD_MSG_TYPE_VLAN_MBR_UPDATES)
        {
            iccp_mclagsyncd_vlan_mbr_update_handler(sys, &msg_buf[pos]);
        }
        else
        {
            ICCPD_LOG_ERR(__FUNCTION__, "recv unknown msg type %d ", msg_hdr->type);
            pos += msg_hdr->len;
            continue;
        }
        pos += msg_hdr->len;
        SYSTEM_SET_SYNCD_RX_DBG_COUNTER(sys, msg_hdr->type, ICCP_DBG_CNTR_STS_OK);
    }
    return 0;
}


 /*
  * Send request to Mclagsyncd to disable traffic for MLAG interface
  */
 void mlacp_link_disable_traffic_distribution(struct LocalInterface *lif)
 {
     int    rc;

     /* Update traffic distribution only if local interface is still bound to MLAG */
     if (!lif || !lif->csm)
         return;

     /* Expecting ACK from peer only after reaching EXCHANGE state */
     if (MLACP(lif->csm).current_state != MLACP_STATE_EXCHANGE)
         return;

     /* Disable traffic distribution for all LAG member ports when LAG goes down.
      * If MLAG interface goes down again while waiting for i/f up ack,
      * do not need to update hardware again
      */
     if ((lif->type == IF_T_PORT_CHANNEL) && (!lif->po_active) &&
              (!lif->is_traffic_disable))
     {
         rc = mlacp_link_set_traffic_dist_mode(lif->name, false);
         ICCPD_LOG_DEBUG("ICCP_FSM", "MLAG_IF %s: set traffic disable, rc %d",
             lif->name, rc);
         if (rc == 0)
             lif->is_traffic_disable = true;
     }
 }

 /*
  * Send request to Mclagsyncd to enable traffic for MLAG interface
  * Note:
  * 1. Caller should check for LAG up before calling this API in normal case.
  * 2. For the ICCP session down case or LAG interface is no longer MLAG
  *    interface, this API is called regardless of the LAG state
  */
 void mlacp_link_enable_traffic_distribution(struct LocalInterface *lif)
 {
    int     rc;

     /* Update traffic distribution only if local interface is still bound to MLAG */
     if (!lif || !lif->csm)
         return;

    if ((lif->type == IF_T_PORT_CHANNEL) && lif->is_traffic_disable)
    {
        rc = mlacp_link_set_traffic_dist_mode(lif->name, true);
        ICCPD_LOG_DEBUG("ICCP_FSM", "MLAG_IF %s: set traffic enable, rc %d",
            lif->name, rc);
        if (rc == 0)
            lif->is_traffic_disable = false;
    }
}

char * mclagd_ctl_cmd_str(int req_type)
{
    switch (req_type)
    {
        case INFO_TYPE_DUMP_STATE:
            return "dump config";

        case INFO_TYPE_DUMP_ARP:
            return "dump arp";

        case INFO_TYPE_DUMP_NDISC:
            return "dump nd";

        case INFO_TYPE_DUMP_MAC:
            return "dump mac";

        case INFO_TYPE_DUMP_LOCAL_PORTLIST:
            return "dump local portlist";

        case INFO_TYPE_DUMP_PEER_PORTLIST:
            return "dump peer portlist";

        case INFO_TYPE_DUMP_DBG_COUNTERS:
            return "dump debug counters";

        case INFO_TYPE_DUMP_UNIQUE_IP:
            return "dump unique_ip";

        case INFO_TYPE_CONFIG_LOGLEVEL:
            return "config loglevel";

        default:
            break;
    }

    return "error req type";
}

int mclagd_ctl_sock_create()
{
    struct sockaddr_un addr;
    struct System* sys = NULL;
    struct epoll_event event;
    int addr_len;
    int ret = 0;

    if ((sys = system_get_instance()) == NULL)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Invalid system instance");
        return MCLAG_ERROR;
    }

    if (sys->sync_ctrl_fd > 0)
        return sys->sync_ctrl_fd;

    sys->sync_ctrl_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sys->sync_ctrl_fd < 0)
    {
        ICCPD_LOG_WARN(__FUNCTION__, "Failed to create mclagd ctl sock");
        return sys->sync_ctrl_fd;
    }

    unlink(sys->mclagdctl_file_path);

    memset((void*)&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, 107, "%s", sys->mclagdctl_file_path);
    addr_len = sizeof(addr.sun_family) + strlen(sys->mclagdctl_file_path);

    if ((ret = bind(sys->sync_ctrl_fd, (struct sockaddr*)&addr, addr_len)) < 0)
    {
        ICCPD_LOG_WARN(__FUNCTION__, "Failed to bind mclagd ctl socket %s:%s", sys->mclagdctl_file_path, strerror(errno));
        close(sys->sync_ctrl_fd);
        return MCLAG_ERROR;
    }

    if (listen(sys->sync_ctrl_fd, 5) < 0)
    {
        ICCPD_LOG_WARN(__FUNCTION__, "Failed to listen unix mclagd ctl socket%s:%s", sys->mclagdctl_file_path, strerror(errno));
        close(sys->sync_ctrl_fd);
        return MCLAG_ERROR;
    }

    event.data.fd = sys->sync_ctrl_fd;
    event.events = EPOLLIN;
    epoll_ctl(sys->epoll_fd, EPOLL_CTL_ADD, sys->sync_ctrl_fd, &event);
    FD_SET(sys->sync_ctrl_fd, &(sys->readfd));
    sys->readfd_count++;

    return sys->sync_ctrl_fd;
}

int mclagd_ctl_sock_accept(int fd)
{
    struct sockaddr_in client_addr;
    int client_fd = 0;
    unsigned int addr_len = 0;

    client_fd = accept(fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0)
    {
        ICCPD_LOG_WARN(__FUNCTION__, "Failed to accept a client from mclagdctl");
        return MCLAG_ERROR;
    }

    return client_fd;
}

int mclagd_ctl_sock_read(int fd, char *r_buf, int total_len)
{
    int read_len = 0;
    int ret = 0;
    struct timeval tv = { 0 };
    fd_set read_fd;

    while (read_len < total_len)
    {
        FD_ZERO(&read_fd);
        FD_SET(fd, &read_fd);
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        switch ((ret = select(fd + 1, &read_fd, NULL, NULL, &tv)))
        {
            case -1:
            /* error*/
            case 0:
                /* timeout*/
                return MCLAG_ERROR;

            default:
                break;
        }

        if (FD_ISSET(fd, &read_fd))
            ret = read(fd, r_buf + read_len, total_len - read_len);
        if (ret <= 0)
        {
            return MCLAG_ERROR;
        }
        read_len += ret;
    }

    return read_len;
}

int mclagd_ctl_sock_write(int fd, char *w_buf, int total_len)
{
    int write_len = 0;
    int ret = 0;

    while (write_len < total_len)
    {
        ret = write(fd, w_buf + write_len, total_len - write_len);
        if (ret <= 0)
        {
            return 0;
        }
        write_len += ret;
    }

    return write_len;
}

void mclagd_ctl_handle_dump_state(int client_fd, int mclag_id)
{
    char * Pbuf = NULL;
    char buf[512] = { 0 };
    int state_num = 0;
    int ret = 0;
    struct mclagd_reply_hdr *hd = NULL;
    int len_tmp = 0;

    ret = iccp_mclag_config_dump(&Pbuf, &state_num, mclag_id);
    if (ret != EXEC_TYPE_SUCCESS)
    {
        len_tmp = sizeof(struct mclagd_reply_hdr);
        memcpy(buf, &len_tmp, sizeof(int));
        hd = (struct mclagd_reply_hdr *)(buf + sizeof(int));
        hd->exec_result = ret;
        hd->info_type = INFO_TYPE_DUMP_STATE;
        hd->data_len = 0;
        mclagd_ctl_sock_write(client_fd, buf, MCLAGD_REPLY_INFO_HDR);

        if (Pbuf)
            free(Pbuf);

        return;
    }
    hd = (struct mclagd_reply_hdr *)(Pbuf + sizeof(int));
    hd->exec_result = EXEC_TYPE_SUCCESS;
    hd->info_type = INFO_TYPE_DUMP_STATE;
    hd->data_len = state_num * sizeof(struct mclagd_state);

    len_tmp = (hd->data_len + sizeof(struct mclagd_reply_hdr));
    memcpy(Pbuf, &len_tmp, sizeof(int));
    mclagd_ctl_sock_write(client_fd, Pbuf, MCLAGD_REPLY_INFO_HDR + hd->data_len);

    if (Pbuf)
        free(Pbuf);

    return;
}

void mclagd_ctl_handle_dump_arp(int client_fd, int mclag_id)
{
    char * Pbuf = NULL;
    char buf[512] = { 0 };
    int arp_num = 0;
    int ret = 0;
    struct mclagd_reply_hdr *hd = NULL;
    int len_tmp = 0;

    ret = iccp_arp_dump(&Pbuf, &arp_num, mclag_id);
    if (ret != EXEC_TYPE_SUCCESS)
    {
        len_tmp = sizeof(struct mclagd_reply_hdr);
        memcpy(buf, &len_tmp, sizeof(int));
        hd = (struct mclagd_reply_hdr *)(buf + sizeof(int));
        hd->exec_result = ret;
        hd->info_type = INFO_TYPE_DUMP_ARP;
        hd->data_len = 0;
        mclagd_ctl_sock_write(client_fd, buf, MCLAGD_REPLY_INFO_HDR);

        if (Pbuf)
            free(Pbuf);

        return;
    }

    hd = (struct mclagd_reply_hdr *)(Pbuf + sizeof(int));
    hd->exec_result = EXEC_TYPE_SUCCESS;
    hd->info_type = INFO_TYPE_DUMP_ARP;
    hd->data_len = arp_num * sizeof(struct mclagd_arp_msg);
    len_tmp = (hd->data_len + sizeof(struct mclagd_reply_hdr));
    memcpy(Pbuf, &len_tmp, sizeof(int));
    mclagd_ctl_sock_write(client_fd, Pbuf, MCLAGD_REPLY_INFO_HDR + hd->data_len);

    if (Pbuf)
        free(Pbuf);

    return;
}

void mclagd_ctl_handle_dump_ndisc(int client_fd, int mclag_id)
{
    char *Pbuf = NULL;
    char buf[512] = { 0 };
    int ndisc_num = 0;
    int ret = 0;
    struct mclagd_reply_hdr *hd = NULL;
    int len_tmp = 0;

    ret = iccp_ndisc_dump(&Pbuf, &ndisc_num, mclag_id);
    if (ret != EXEC_TYPE_SUCCESS)
    {
        len_tmp = sizeof(struct mclagd_reply_hdr);
        memcpy(buf, &len_tmp, sizeof(int));
        hd = (struct mclagd_reply_hdr *)(buf + sizeof(int));
        hd->exec_result = ret;
        hd->info_type = INFO_TYPE_DUMP_NDISC;
        hd->data_len = 0;
        mclagd_ctl_sock_write(client_fd, buf, MCLAGD_REPLY_INFO_HDR);

        if (Pbuf)
            free(Pbuf);

        return;
    }

    hd = (struct mclagd_reply_hdr *)(Pbuf + sizeof(int));
    hd->exec_result = EXEC_TYPE_SUCCESS;
    hd->info_type = INFO_TYPE_DUMP_NDISC;
    hd->data_len = ndisc_num * sizeof(struct mclagd_ndisc_msg);
    len_tmp = (hd->data_len + sizeof(struct mclagd_reply_hdr));
    memcpy(Pbuf, &len_tmp, sizeof(int));
    mclagd_ctl_sock_write(client_fd, Pbuf, MCLAGD_REPLY_INFO_HDR + hd->data_len);

    if (Pbuf)
        free(Pbuf);

    return;
}

void mclagd_ctl_handle_dump_mac(int client_fd, int mclag_id)
{
    char * Pbuf = NULL;
    char buf[512] = { 0 };
    int mac_num = 0;
    int ret = 0;
    struct mclagd_reply_hdr *hd = NULL;
    int len_tmp = 0;

    ret = iccp_mac_dump(&Pbuf, &mac_num, mclag_id);
    if (ret != EXEC_TYPE_SUCCESS)
    {
        len_tmp = sizeof(struct mclagd_reply_hdr);
        memcpy(buf, &len_tmp, sizeof(int));
        hd = (struct mclagd_reply_hdr *)(buf + sizeof(int));
        hd->exec_result = ret;
        hd->info_type = INFO_TYPE_DUMP_MAC;
        hd->data_len = 0;
        mclagd_ctl_sock_write(client_fd, buf, MCLAGD_REPLY_INFO_HDR);

        if (Pbuf)
            free(Pbuf);

        return;
    }

    hd = (struct mclagd_reply_hdr *)(Pbuf + sizeof(int));
    hd->exec_result = EXEC_TYPE_SUCCESS;
    hd->info_type = INFO_TYPE_DUMP_MAC;
    hd->data_len = mac_num * sizeof(struct mclagd_mac_msg);

    len_tmp = (hd->data_len + sizeof(struct mclagd_reply_hdr));
    memcpy(Pbuf, &len_tmp, sizeof(int));

    mclagd_ctl_sock_write(client_fd, Pbuf, MCLAGD_REPLY_INFO_HDR + hd->data_len);

    if (Pbuf)
        free(Pbuf);

    return;
}

void mclagd_ctl_handle_dump_local_portlist(int client_fd, int mclag_id)
{
    char * Pbuf = NULL;
    char buf[512] = { 0 };
    int lif_num = 0;
    int ret = 0;
    struct mclagd_reply_hdr *hd = NULL;
    int len_tmp = 0;

    ret = iccp_local_if_dump(&Pbuf, &lif_num, mclag_id);
    if (ret != EXEC_TYPE_SUCCESS)
    {
        len_tmp = sizeof(struct mclagd_reply_hdr);
        memcpy(buf, &len_tmp, sizeof(int));
        hd = (struct mclagd_reply_hdr *)(buf + sizeof(int));
        hd->exec_result = ret;
        hd->info_type = INFO_TYPE_DUMP_LOCAL_PORTLIST;
        hd->data_len = 0;
        mclagd_ctl_sock_write(client_fd, buf, MCLAGD_REPLY_INFO_HDR);

        if (Pbuf)
            free(Pbuf);

        return;
    }

    hd = (struct mclagd_reply_hdr *)(Pbuf + sizeof(int));
    hd->exec_result = EXEC_TYPE_SUCCESS;
    hd->info_type = INFO_TYPE_DUMP_LOCAL_PORTLIST;
    hd->data_len = lif_num * sizeof(struct mclagd_local_if);
    len_tmp = (hd->data_len + sizeof(struct mclagd_reply_hdr));
    memcpy(Pbuf, &len_tmp, sizeof(int));
    mclagd_ctl_sock_write(client_fd, Pbuf, MCLAGD_REPLY_INFO_HDR + hd->data_len);

    if (Pbuf)
        free(Pbuf);

    return;
}

void mclagd_ctl_handle_dump_peer_portlist(int client_fd, int mclag_id)
{
    char * Pbuf = NULL;
    char buf[512] = { 0 };
    int pif_num = 0;
    int ret = 0;
    struct mclagd_reply_hdr *hd = NULL;
    int len_tmp = 0;

    ret = iccp_peer_if_dump(&Pbuf, &pif_num, mclag_id);
    if (ret != EXEC_TYPE_SUCCESS)
    {
        len_tmp = sizeof(struct mclagd_reply_hdr);
        memcpy(buf, &len_tmp, sizeof(int));
        hd = (struct mclagd_reply_hdr *)(buf + sizeof(int));
        hd->exec_result = ret;
        hd->info_type = INFO_TYPE_DUMP_PEER_PORTLIST;
        hd->data_len = 0;
        mclagd_ctl_sock_write(client_fd, buf, MCLAGD_REPLY_INFO_HDR);

        if (Pbuf)
            free(Pbuf);

        return;
    }

    hd = (struct mclagd_reply_hdr *)(Pbuf + sizeof(int));
    hd->exec_result = EXEC_TYPE_SUCCESS;
    hd->info_type = INFO_TYPE_DUMP_PEER_PORTLIST;
    hd->data_len = pif_num * sizeof(struct mclagd_peer_if);
    len_tmp = (hd->data_len + sizeof(struct mclagd_reply_hdr));
    memcpy(Pbuf, &len_tmp, sizeof(int));
    mclagd_ctl_sock_write(client_fd, Pbuf, MCLAGD_REPLY_INFO_HDR + hd->data_len);

    if (Pbuf)
        free(Pbuf);

    return;
}

void mclagd_ctl_handle_dump_dbg_counters(int client_fd, int mclag_id)
{
    char * Pbuf = NULL;
    char buf[512] = {0};
    int data_len = 0;
    int ret = 0;
    struct mclagd_reply_hdr *hd = NULL;
    int len_tmp = 0;

    ret = iccp_cmd_dbg_counter_dump(&Pbuf, &data_len, mclag_id);
    if (ret != EXEC_TYPE_SUCCESS)
    {
        len_tmp = sizeof(struct mclagd_reply_hdr);
        memcpy(buf, &len_tmp, sizeof(int));
        hd = (struct mclagd_reply_hdr *)(buf + sizeof(int));
        hd->exec_result = ret;
        hd->info_type = INFO_TYPE_DUMP_DBG_COUNTERS;
        hd->data_len = 0;
        mclagd_ctl_sock_write(client_fd, buf, MCLAGD_REPLY_INFO_HDR);

        if (Pbuf)
            free(Pbuf);
        return;
    }

    hd = (struct mclagd_reply_hdr *)(Pbuf + sizeof(int));
    hd->exec_result = EXEC_TYPE_SUCCESS;
    hd->info_type = INFO_TYPE_DUMP_DBG_COUNTERS;
    hd->data_len = data_len;
    len_tmp = (hd->data_len + sizeof(struct mclagd_reply_hdr));
    memcpy(Pbuf, &len_tmp, sizeof(int));
    mclagd_ctl_sock_write(client_fd, Pbuf, MCLAGD_REPLY_INFO_HDR + hd->data_len);

    if (Pbuf)
       free(Pbuf);
}

void mclagd_ctl_handle_dump_unique_ip(int client_fd, int mclag_id)
{
    char *Pbuf = NULL;
    char buf[512] = { 0 };
    int lif_num = 0;
    int ret = 0;
    struct mclagd_reply_hdr *hd = NULL;
    int len_tmp = 0;

    ret = iccp_unique_ip_if_dump(&Pbuf, &lif_num, mclag_id);
    if (ret != EXEC_TYPE_SUCCESS)
    {
        len_tmp = sizeof(struct mclagd_reply_hdr);
        memcpy(buf, &len_tmp, sizeof(int));
        hd = (struct mclagd_reply_hdr *)(buf + sizeof(int));
        hd->exec_result = ret;
        hd->info_type = INFO_TYPE_DUMP_LOCAL_PORTLIST;
        hd->data_len = 0;
        mclagd_ctl_sock_write(client_fd, buf, MCLAGD_REPLY_INFO_HDR);

        if (Pbuf)
            free(Pbuf);

        return;
    }

    hd = (struct mclagd_reply_hdr *)(Pbuf + sizeof(int));
    hd->exec_result = EXEC_TYPE_SUCCESS;
    hd->info_type = INFO_TYPE_DUMP_UNIQUE_IP;
    hd->data_len = lif_num * sizeof(struct mclagd_unique_ip_if);
    len_tmp = (hd->data_len + sizeof(struct mclagd_reply_hdr));
    memcpy(Pbuf, &len_tmp, sizeof(int));
    mclagd_ctl_sock_write(client_fd, Pbuf, MCLAGD_REPLY_INFO_HDR + hd->data_len);

    if (Pbuf)
        free(Pbuf);

    return;
}

void mclagd_ctl_handle_config_loglevel(int client_fd, int log_level)
{
    char buf[sizeof(struct mclagd_reply_hdr)+sizeof(int)];
    struct mclagd_reply_hdr *hd = NULL;
    int len_tmp = 0;

    logger_set_configuration(log_level);

    len_tmp = sizeof(struct mclagd_reply_hdr);
    memcpy(buf, &len_tmp, sizeof(int));
    hd = (struct mclagd_reply_hdr *)(buf + sizeof(int));
    hd->exec_result = EXEC_TYPE_SUCCESS;
    hd->info_type = INFO_TYPE_CONFIG_LOGLEVEL;
    hd->data_len = 0;
    mclagd_ctl_sock_write(client_fd, buf, MCLAGD_REPLY_INFO_HDR);

    return;
}

int mclagd_ctl_interactive_process(int client_fd)
{
    char buf[512] = { 0 };
    int ret = 0;

    struct mclagdctl_req_hdr* req = NULL;

    if (client_fd < 0)
        return MCLAG_ERROR;

    ret = mclagd_ctl_sock_read(client_fd, buf, sizeof(struct mclagdctl_req_hdr));

    if (ret < 0)
        return MCLAG_ERROR;

    req = (struct mclagdctl_req_hdr*)buf;

    ICCPD_LOG_DEBUG(__FUNCTION__, "Receive request %s from mclagdctl", mclagd_ctl_cmd_str(req->info_type));

    switch (req->info_type)
    {
        case INFO_TYPE_DUMP_STATE:
            mclagd_ctl_handle_dump_state(client_fd, req->mclag_id);
            break;

        case INFO_TYPE_DUMP_ARP:
            mclagd_ctl_handle_dump_arp(client_fd, req->mclag_id);
            break;

        case INFO_TYPE_DUMP_NDISC:
            mclagd_ctl_handle_dump_ndisc(client_fd, req->mclag_id);
            break;

        case INFO_TYPE_DUMP_MAC:
            mclagd_ctl_handle_dump_mac(client_fd, req->mclag_id);
            break;

        case INFO_TYPE_DUMP_LOCAL_PORTLIST:
            mclagd_ctl_handle_dump_local_portlist(client_fd, req->mclag_id);
            break;

        case INFO_TYPE_DUMP_PEER_PORTLIST:
            mclagd_ctl_handle_dump_peer_portlist(client_fd, req->mclag_id);
            break;

        case INFO_TYPE_DUMP_DBG_COUNTERS:
             mclagd_ctl_handle_dump_dbg_counters(client_fd, req->mclag_id);
            break;

        case INFO_TYPE_DUMP_UNIQUE_IP:
            mclagd_ctl_handle_dump_unique_ip(client_fd, req->mclag_id);
            break;

        case INFO_TYPE_CONFIG_LOGLEVEL:
            mclagd_ctl_handle_config_loglevel(client_fd, req->mclag_id);
            break;

        default:
            return MCLAG_ERROR;
    }

    return 0;
}

int syn_local_mac_info_to_peer(struct CSM* csm, struct LocalInterface *local_if, int sync_add, int is_sag)
{
    struct MACMsg mac_msg = {0};
    int msg_len = 0, rc = MCLAG_ERROR;
    int vid = 0;
    uint8_t null_mac[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    if (!csm || !local_if) {
        ICCPD_LOG_DEBUG(__FUNCTION__,"invalid parameters");
        return MCLAG_ERROR;
    }

    if (memcmp(MLACP(csm).system_id, null_mac, ETHER_ADDR_LEN) == 0) {
        ICCPD_LOG_DEBUG(__FUNCTION__,"invalid system id");
        return MCLAG_ERROR;
    }

    if (sync_add) {
        mac_msg.op_type = MAC_SYNC_ADD;
    } else {
        mac_msg.op_type = MAC_SYNC_DEL;
    }

    if (is_sag) {
        sscanf (local_if->name,"sag%d.256",&vid);
    } else {
        sscanf (local_if->name,"Vlan%d",&vid);
    }

    mac_msg.vid = vid;
    mac_msg.fdb_type = MAC_TYPE_STATIC;
    memcpy(mac_msg.origin_ifname, csm->peer_itf_name, MAX_L_PORT_NAME);
    memcpy(mac_msg.mac_addr, MLACP(csm).system_id, ETHER_ADDR_LEN);

    ICCPD_LOG_DEBUG(__FUNCTION__,"add %d, mac name %s, vid %d", sync_add, mac_msg.origin_ifname, mac_msg.vid);
    ICCPD_LOG_DEBUG(__FUNCTION__,"mac [%02X:%02X:%02X:%02X:%02X:%02X]",
        mac_msg.mac_addr[0], mac_msg.mac_addr[1], mac_msg.mac_addr[2], mac_msg.mac_addr[3], mac_msg.mac_addr[4], mac_msg.mac_addr[5]);

    memset(g_csm_buf, 0, CSM_BUFFER_SIZE);
    msg_len = mlacp_prepare_for_mac_info_to_peer(csm, g_csm_buf, CSM_BUFFER_SIZE, &mac_msg, 0);
    if (msg_len > 0)
        rc = iccp_csm_send(csm, g_csm_buf, msg_len);

    if (rc <= 0)
    {
        ICCPD_LOG_NOTICE(__FUNCTION__, "failed rc %d, msg_len %d", rc, msg_len);
    }
    else
    {
        ICCPD_LOG_DEBUG(__FUNCTION__,"success");
    }
    return rc;
}

int syn_local_arp_info_to_peer(struct CSM* csm, struct LocalInterface *local_if, int sync_add, int ack)
{
    struct ARPMsg arp_msg = {0};
    int msg_len = 0, rc = MCLAG_ERROR;

    if (!csm || !local_if) {
        ICCPD_LOG_DEBUG(__FUNCTION__,"invalid parameters");
        return MCLAG_ERROR;
    }

    if (sync_add) {
        arp_msg.op_type = NEIGH_SYNC_ADD;
        if (ack) {
            arp_msg.flag |= NEIGH_SYNC_FLAG_ACK;
        }
    } else {
        arp_msg.op_type = NEIGH_SYNC_DEL;
    }

    arp_msg.ipv4_addr = local_if->ipv4_addr;
    arp_msg.flag |= NEIGH_SYNC_FLAG_SELF_IP;
    memcpy(arp_msg.ifname, local_if->name, MAX_L_PORT_NAME);
    memcpy(arp_msg.mac_addr, local_if->mac_addr, ETHER_ADDR_LEN);

    ICCPD_LOG_DEBUG(__FUNCTION__," add %d ack %d ifname %s, ip %s", sync_add, ack, arp_msg.ifname, show_ip_str(arp_msg.ipv4_addr));
    ICCPD_LOG_DEBUG(__FUNCTION__," mac [%02X:%02X:%02X:%02X:%02X:%02X]",
        arp_msg.mac_addr[0], arp_msg.mac_addr[1], arp_msg.mac_addr[2], arp_msg.mac_addr[3], arp_msg.mac_addr[4], arp_msg.mac_addr[5]);

    memset(g_csm_buf, 0, CSM_BUFFER_SIZE);
    msg_len = mlacp_prepare_for_arp_info(csm, g_csm_buf, CSM_BUFFER_SIZE, &arp_msg, 0, NEIGH_SYNC_SELF_IP);
    if (msg_len > 0)
        rc = iccp_csm_send(csm, g_csm_buf, msg_len);

    if (rc <= 0)
    {
        ICCPD_LOG_NOTICE(__FUNCTION__, "failed rc %d, msg_len %d", rc, msg_len);
    }
    else
    {
        ICCPD_LOG_DEBUG(__FUNCTION__,"success");
    }

    return rc;
}

int syn_local_nd_info_to_peer(struct CSM* csm, struct LocalInterface *local_if, int sync_add, int ack, int is_ipv6_ll, int dir)
{
    struct NDISCMsg nd_msg = {0};
    int msg_len = 0, rc = MCLAG_ERROR;
    uint8_t null_mac[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    if (!csm || !local_if) {
        ICCPD_LOG_DEBUG(__FUNCTION__,"invalid parameters");
        return MCLAG_ERROR;
    }

    if (sync_add) {
        nd_msg.op_type = NEIGH_SYNC_ADD;
        if (ack) {
            nd_msg.flag |= NEIGH_SYNC_FLAG_ACK;
        }
    } else {
        nd_msg.op_type = NEIGH_SYNC_DEL;
    }

    if (is_ipv6_ll) {
        if (local_if->ll_prefixlen_v6 == 0) {
            ICCPD_LOG_NOTICE(__FUNCTION__, " Link Local Address not configured.");
            return MCLAG_ERROR;
        }
        nd_msg.flag |= NEIGH_SYNC_FLAG_SELF_LL;
        memcpy(nd_msg.ipv6_addr, local_if->ipv6_ll_addr, 16);
        if (memcmp(MLACP(csm).system_id, null_mac, ETHER_ADDR_LEN) == 0){
            ICCPD_LOG_NOTICE(__FUNCTION__, " system_id not initialised.");
            return MCLAG_ERROR;
        }
        memcpy(nd_msg.mac_addr, MLACP(csm).system_id, ETHER_ADDR_LEN);
    } else {
        nd_msg.flag |= NEIGH_SYNC_FLAG_SELF_IP;
        memcpy(nd_msg.ipv6_addr, local_if->ipv6_addr, 16);
        memcpy(nd_msg.mac_addr, local_if->mac_addr, ETHER_ADDR_LEN);
    }
    memcpy(nd_msg.ifname, local_if->name, MAX_L_PORT_NAME);

    ICCPD_LOG_DEBUG(__FUNCTION__,"add %d, ack %d, flag %x, is_ll %d, dir %d, ifname %s, ip %s", sync_add, ack,
            nd_msg.flag, is_ipv6_ll, dir, nd_msg.ifname, show_ipv6_str((char *)nd_msg.ipv6_addr));
    ICCPD_LOG_DEBUG(__FUNCTION__,"mac [%02X:%02X:%02X:%02X:%02X:%02X]",
        nd_msg.mac_addr[0], nd_msg.mac_addr[1], nd_msg.mac_addr[2], nd_msg.mac_addr[3], nd_msg.mac_addr[4], nd_msg.mac_addr[5]);

    memset(g_csm_buf, 0, CSM_BUFFER_SIZE);
    msg_len = mlacp_prepare_for_ndisc_info(csm, g_csm_buf, CSM_BUFFER_SIZE, &nd_msg, 0, NEIGH_SYNC_SELF_IP);
    if (msg_len > 0)
        rc = iccp_csm_send(csm, g_csm_buf, msg_len);

    if (rc <= 0)
    {
        ICCPD_LOG_NOTICE(__FUNCTION__, "failed rc %d, msg_len %d", rc, msg_len);
    }
    else
    {
        ICCPD_LOG_DEBUG(__FUNCTION__,"success");
    }

    return rc;
}

int syn_local_neigh_mac_info_to_peer(struct LocalInterface *local_if,
        int sync_add, int is_v4, int is_v6, int sync_mac, int ack, int is_ipv6_ll, int dir)
{
    struct System* sys = NULL;
    struct CSM* csm = NULL;

    if ((sys = system_get_instance()) == NULL) {
        ICCPD_LOG_DEBUG(__FUNCTION__,"system instance not present, skip sync");
        return MCLAG_ERROR;
    }

    while (!LIST_EMPTY(&(sys->csm_list)))
    {
        csm = LIST_FIRST(&(sys->csm_list));
        break;
    }

    if (!csm) {
        ICCPD_LOG_DEBUG(__FUNCTION__,"csm not present, skip sync");
        return MCLAG_ERROR;
    }

    if (MLACP(csm).current_state != MLACP_STATE_EXCHANGE) {
        ICCPD_LOG_DEBUG(__FUNCTION__,"Session not up, skip sync");
        return MCLAG_ERROR;
    }

    if (local_if->type != IF_T_VLAN) {
        ICCPD_LOG_DEBUG(__FUNCTION__,"invalid if type %d", local_if->type);
        return MCLAG_ERROR;
    }

    ICCPD_LOG_DEBUG(__FUNCTION__,"add %d, v4 %d, v6 %d, mac %d ack %d, is_ipv6_ll %d, dir %d",
            sync_add, is_v4, is_v6, sync_mac, ack, is_ipv6_ll, dir);

    if (csm->peer_link_if)
    {
        set_peerlink_learn_kernel(csm, 0, 9);
    }
    if (sync_mac) {
        syn_local_mac_info_to_peer(csm, local_if, sync_add, is_ipv6_ll);
    }

    if (is_v4) {
        syn_local_arp_info_to_peer(csm, local_if, sync_add, ack);
    }

    if (is_v6) {
        syn_local_nd_info_to_peer(csm, local_if, sync_add, ack, is_ipv6_ll, 1);
    }

    return 0;
}

int syn_ack_local_neigh_mac_info_to_peer(char *ifname, int is_ipv6_ll)
{
    struct LocalInterface *lif = NULL;
    int sync_add = 0, is_v4 = 0, is_v6 = 0;

    if (!ifname)
        return -1;

    lif = local_if_find_by_name(ifname);
    if (lif)
    {
        if (is_ipv6_ll) {
            syn_local_neigh_mac_info_to_peer(lif, 1, 0, 1, 1, 0, 1, 2);
            return 0;
        }

        if (lif->ipv4_addr)
        {
            is_v4 = 1;
        }

        if (lif->prefixlen_v6)
        {
            is_v6 = 1;
        }
        ICCPD_LOG_DEBUG(__FUNCTION__," v4 %d, v6 %d, l3_mode %d, proto %d", is_v4, is_v6, lif->l3_mode, lif->is_l3_proto_enabled);
        if (lif->l3_mode && lif->is_l3_proto_enabled) {
            syn_local_neigh_mac_info_to_peer(lif, 1, is_v4, is_v6, 1, 0, 0, 3);
        }
    }
    return 0;
}

int is_unique_ip_configured(char *ifname)
{
    struct System* sys = NULL;
    struct Unq_ip_If_info* unq_ip_if = NULL;

    if (!(sys = system_get_instance()))
        return 0;

    LIST_FOREACH(unq_ip_if, &(sys->unq_ip_if_list), if_next)
    {
        if (strcmp(unq_ip_if->name, ifname) == 0)
        {
            return 1;
        }
    }

    return 0;
}

int sync_unique_ip()
{
    struct System* sys = NULL;
    struct Unq_ip_If_info* unq_ip_if = NULL;
    struct LocalInterface* local_if = NULL;

    if (!(sys = system_get_instance()))
        return 0;

    LIST_FOREACH(unq_ip_if, &(sys->unq_ip_if_list), if_next)
    {
        ICCPD_LOG_DEBUG(__FUNCTION__, "unq_ip_if name %s", unq_ip_if->name);
        syn_ack_local_neigh_mac_info_to_peer(unq_ip_if->name, 0);
    }

    return 0;
}

void set_peer_mac_in_kernel(char *mac, int vlan, int add)
{
    char cmd[256] = { 0 };
    int ret = 0;

    ICCPD_LOG_DEBUG(__FUNCTION__,"mac %s, vlan %d, add %d", mac, vlan, add);

    if (add) {
        sprintf(cmd, "bridge fdb replace %s dev Bridge vlan %d local", mac, vlan);
    } else {
        sprintf(cmd, "bridge fdb del %s dev Bridge vlan %d local", mac, vlan);
    }

    ret = system(cmd);
    ICCPD_LOG_DEBUG(__FUNCTION__, " cmd  %s  ret = %d", cmd, ret);

    return;
}

void set_peerlink_learn_kernel(
    struct CSM* csm,
    int enable, int dir)
{
    struct LocalInterface *lif = NULL;
    if (!csm || !csm->peer_link_if)
        return;

    lif = csm->peer_link_if;

    ICCPD_LOG_DEBUG(__FUNCTION__,"ifname %s, enable %d, dir %d", lif->name, enable, dir);
    char cmd[256] = { 0 };
    int ret = 0;
    if (enable == 0) {
        sprintf(cmd, "bridge link set dev %s learning off", lif->name);
    } else {
        sprintf(cmd, "bridge link set dev %s learning on", lif->name);
    }

    ret = system(cmd);
    ICCPD_LOG_DEBUG(__FUNCTION__, " cmd  %s  ret = %d", cmd, ret);

    if (ret != 0)
    {
        ICCPD_LOG_DEBUG(__FUNCTION__, " cmd  %s  ret = %d", cmd, ret);
        csm->peer_link_learning_enable = enable;
        csm->peer_link_learning_retry_time = time(NULL);
    } else {
        csm->peer_link_learning_retry_time = 0;
    }

    return;
}
