/*
 * system.c
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
#include <netlink/msg.h>

#include "../include/iccp_csm.h"
#include "../include/logger.h"
#include "../include/iccp_netlink.h"
#include "../include/scheduler.h"
#include "../include/mlacp_link_handler.h"
#include "../include/iccp_ifm.h"

#define ETHER_ADDR_LEN 6
char mac_print_str[ETHER_ADDR_STR_LEN];

/* Singleton */
struct System* system_get_instance()
{
    static struct System* sys = NULL;

    if (sys == NULL )
    {
        sys = (struct System*)malloc(sizeof(struct System));
        if (sys == NULL )
        {
            ICCPD_LOG_WARN(__FUNCTION__, "Failed to obtain system instance.");
            return NULL;
        }
        system_init(sys);
    }

    return sys;
}

/* System instance initialization */
void system_init(struct System* sys)
{
    if (sys == NULL )
        return;

    memset(sys, 0, sizeof(struct System));
    sys->server_fd = -1;
    sys->sync_fd = -1;
    sys->sync_ctrl_fd = -1;
    sys->arp_receive_fd = -1;
    sys->ndisc_receive_fd = -1;
    sys->epoll_fd = -1;
    sys->family = -1;
    sys->warmboot_start = 0;
    sys->warmboot_exit = 0;
    LIST_INIT(&(sys->csm_list));
    LIST_INIT(&(sys->lif_list));
    LIST_INIT(&(sys->lif_purge_list));
    LIST_INIT(&(sys->unq_ip_if_list));
    LIST_INIT(&(sys->pending_vlan_mbr_if_list));

    sys->log_file_path = strdup("/var/log/iccpd.log");
    sys->cmd_file_path = strdup("/var/run/iccpd/iccpd.vty");
    sys->config_file_path = strdup("/etc/iccpd/iccpd.conf");
    sys->mclagdctl_file_path = strdup("/var/run/iccpd/mclagdctl.sock");
    sys->pid_file_fd = 0;
    sys->telnet_port = 2015;
    FD_ZERO(&(sys->readfd));
    sys->readfd_count = 0;
    sys->csm_trans_time = 0;
    sys->need_sync_team_again = 0;
    sys->need_sync_netlink_again = 0;
    scheduler_server_sock_init();
    iccp_system_init_netlink_socket();
    iccp_init_netlink_event_fd(sys);
}

/* System instance tear down */
void system_finalize()
{
    struct System* sys = NULL;
    struct CSM* csm = NULL;
    struct LocalInterface* local_if = NULL;
    struct Unq_ip_If_info* unq_ip_if = NULL;

    if ((sys = system_get_instance()) == NULL )
        return;

    ICCPD_LOG_NOTICE(__FUNCTION__,
        "System resource pool is destructing. Warmboot exit (%d)",
        sys->warmboot_exit);

    while (!LIST_EMPTY(&(sys->csm_list)))
    {
        csm = LIST_FIRST(&(sys->csm_list));
        /* Remove ICCP info from STATE_DB if it is not warm reboot */
        if (sys->warmboot_exit != WARM_REBOOT)
            mlacp_link_del_iccp_info(csm->mlag_id);
        iccp_csm_finalize(csm);
    }

    /* Release all port objects */
    while (!LIST_EMPTY(&(sys->lif_list)))
    {
        local_if = LIST_FIRST(&(sys->lif_list));
        LIST_REMOVE(local_if, system_next);
        local_if_finalize(local_if);
    }

    while (!LIST_EMPTY(&(sys->lif_purge_list)))
    {
        local_if = LIST_FIRST(&(sys->lif_purge_list));
        LIST_REMOVE(local_if, system_purge_next);
        local_if_finalize(local_if);
    }

    //remove all pending vlan membership entries
    del_all_pending_vlan_mbr_ifs(sys);

    while (!LIST_EMPTY(&(sys->unq_ip_if_list)))
    {
        unq_ip_if = LIST_FIRST(&(sys->unq_ip_if_list));
        LIST_REMOVE(unq_ip_if, if_next);
        free(unq_ip_if);
    }

    iccp_system_dinit_netlink_socket();

    if (sys->log_file_path != NULL )
        free(sys->log_file_path);
    if (sys->cmd_file_path != NULL )
        free(sys->cmd_file_path);
    if (sys->config_file_path != NULL )
        free(sys->config_file_path);
    if (sys->pid_file_fd > 0)
        close(sys->pid_file_fd);
    if (sys->server_fd > 0)
        close(sys->server_fd);
    if (sys->sync_fd > 0)
        close(sys->sync_fd);
    if (sys->sync_ctrl_fd > 0)
        close(sys->sync_ctrl_fd);
    if (sys->arp_receive_fd > 0)
        close(sys->arp_receive_fd);
    if (sys->ndisc_receive_fd > 0)
        close(sys->ndisc_receive_fd);
    if (sys->sig_pipe_r > 0)
        close(sys->sig_pipe_r);
    if (sys->sig_pipe_w > 0)
        close(sys->sig_pipe_w);

    if (sys->epoll_fd)
        close(sys->epoll_fd);

    free(sys);
    ICCPD_LOG_INFO(__FUNCTION__, "System resource pool destructed successfully...");
}

struct CSM* system_create_csm()
{
    struct System* sys = NULL;
    struct CSM* csm = NULL;

    if ((sys = system_get_instance()) == NULL )
        return NULL;

    /* Create a new csm */
    csm = (struct CSM*)malloc(sizeof(struct CSM));
    if (csm == NULL )
        return NULL;
    else
        memset(csm, 0, sizeof(struct CSM));
    iccp_csm_init(csm);
    LIST_INSERT_HEAD(&(sys->csm_list), csm, next);

    return csm;
}

/* Get connect state machine instance by peer ip */
struct CSM* system_get_csm_by_peer_ip(const char* peer_ip)
{
    struct System* sys = NULL;
    struct CSM* csm = NULL;

    if ((sys = system_get_instance()) == NULL )
        return NULL;

    LIST_FOREACH(csm, &(sys->csm_list), next)
    {
        if (strcmp(csm->peer_ip, peer_ip) == 0)
            return csm;
    }

    return NULL;
}

//function to get CSM by peer interface name
struct CSM* system_get_csm_by_peer_ifname(char *ifname)
{
    struct CSM *csm = NULL;
    struct System* sys = NULL;

    if (!ifname)
    {
        return NULL;
    }

    if ((sys = system_get_instance()) == NULL)
    {
        return NULL;
    }

    /* traverse all CSM and find matching csm with peer ifname */
    LIST_FOREACH(csm, &(sys->csm_list), next)
    {
        //return  matching csm
        if (strcmp(ifname, csm->peer_itf_name) == 0)
        {
            return csm;
        }
    }
    return NULL;
}

struct CSM* system_get_csm_by_mlacp_id(int id)
{
    struct System* sys = NULL;
    struct CSM* csm = NULL;

    if ((sys = system_get_instance()) == NULL )
        return NULL;

    LIST_FOREACH(csm, &(sys->csm_list), next)
    {
        if (csm->app_csm.mlacp.id == id)
            return csm;
    }

    return NULL;
}

struct CSM* system_get_first_csm()
{
    struct System* sys = NULL;
    struct CSM* csm = NULL;

    if ((sys = system_get_instance()) == NULL )
        return NULL;

    LIST_FOREACH(csm, &(sys->csm_list), next)
    {
        return csm;
    }

    return NULL;
}

SYNCD_TX_DBG_CNTR_MSG_e system_syncdtx_to_dbg_msg_type(uint32_t msg_type)
{
    switch(msg_type)
    {
        case MCLAG_MSG_TYPE_PORT_ISOLATE:
            return SYNCD_TX_DBG_CNTR_MSG_PORT_ISOLATE;

        case MCLAG_MSG_TYPE_PORT_MAC_LEARN_MODE:
            return SYNCD_TX_DBG_CNTR_MSG_PORT_MAC_LEARN_MODE;

        case MCLAG_MSG_TYPE_FLUSH_FDB:
            return SYNCD_TX_DBG_CNTR_MSG_FLUSH_FDB;

        case MCLAG_MSG_TYPE_SET_MAC:
            return SYNCD_TX_DBG_CNTR_MSG_SET_IF_MAC;

        case MCLAG_MSG_TYPE_SET_FDB:
            return SYNCD_TX_DBG_CNTR_MSG_SET_FDB;

        case MCLAG_MSG_TYPE_SET_TRAFFIC_DIST_ENABLE:
            return SYNCD_TX_DBG_CNTR_MSG_SET_TRAFFIC_DIST_ENABLE;

        case MCLAG_MSG_TYPE_SET_TRAFFIC_DIST_DISABLE:
            return SYNCD_TX_DBG_CNTR_MSG_SET_TRAFFIC_DIST_DISABLE;

        case MCLAG_MSG_TYPE_SET_ICCP_STATE:
            return SYNCD_TX_DBG_CNTR_MSG_SET_ICCP_STATE;

        case MCLAG_MSG_TYPE_SET_ICCP_ROLE:
            return SYNCD_TX_DBG_CNTR_MSG_SET_ICCP_ROLE;

        case MCLAG_MSG_TYPE_SET_ICCP_SYSTEM_ID:
            return SYNCD_TX_DBG_CNTR_MSG_SET_ICCP_SYSTEM_ID;

        case MCLAG_MSG_TYPE_SET_REMOTE_IF_STATE:
            return SYNCD_TX_DBG_CNTR_MSG_SET_REMOTE_IF_STATE;

        case MCLAG_MSG_TYPE_DEL_ICCP_INFO:
            return SYNCD_TX_DBG_CNTR_MSG_DEL_ICCP_INFO;

        case MCLAG_MSG_TYPE_DEL_REMOTE_IF_INFO:
            return SYNCD_TX_DBG_CNTR_MSG_DEL_REMOTE_IF_INFO;

        case MCLAG_MSG_TYPE_SET_PEER_LINK_ISOLATION:
            return SYNCD_TX_DBG_CNTR_MSG_PEER_LINK_ISOLATION;

        case MCLAG_MSG_TYPE_SET_ICCP_PEER_SYSTEM_ID:
            return SYNCD_TX_DBG_CNTR_MSG_SET_ICCP_PEER_SYSTEM_ID;

        default:
            return SYNCD_TX_DBG_CNTR_MSG_MAX;
    }
}

SYNCD_RX_DBG_CNTR_MSG_e system_syncdrx_to_dbg_msg_type(uint32_t msg_type)
{
    switch(msg_type)
    {
        case MCLAG_SYNCD_MSG_TYPE_FDB_OPERATION:
            return SYNCD_RX_DBG_CNTR_MSG_MAC;
        case MCLAG_SYNCD_MSG_TYPE_CFG_MCLAG_DOMAIN:
            return SYNCD_RX_DBG_CNTR_MSG_CFG_MCLAG_DOMAIN;
        case MCLAG_SYNCD_MSG_TYPE_CFG_MCLAG_IFACE:
            return SYNCD_RX_DBG_CNTR_MSG_CFG_MCLAG_IFACE;
        case MCLAG_SYNCD_MSG_TYPE_CFG_MCLAG_UNIQUE_IP:
            return SYNCD_RX_DBG_CNTR_MSG_CFG_MCLAG_UNIQUE_IP;
        case MCLAG_SYNCD_MSG_TYPE_VLAN_MBR_UPDATES:
            return SYNCD_RX_DBG_CNTR_MSG_VLAN_MBR_UPDATES;
        default:
            return SYNCD_RX_DBG_CNTR_MSG_MAX;
    }
}

char *mac_addr_to_str(uint8_t mac_addr[ETHER_ADDR_LEN])
{
    memset(mac_print_str, 0, sizeof(mac_print_str));
    snprintf(mac_print_str, sizeof(mac_print_str), "%02x:%02x:%02x:%02x:%02x:%02x",
        mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

    return mac_print_str;
}

void system_update_netlink_counters(
    uint16_t netlink_msg_type,
    struct nlmsghdr *nlh)
{
    struct System *sys;
    struct ndmsg *ndm = NLMSG_DATA(nlh);

    sys = system_get_instance();
    if (!sys)
        return;

    switch (netlink_msg_type)
    {
        case RTM_NEWLINK:
            ++sys->dbg_counters.newlink_count;
            break;
        case RTM_DELLINK:
            ++sys->dbg_counters.dellink_count;
            break;
        case RTM_NEWNEIGH:
            ++sys->dbg_counters.newnbr_count;
            if (ndm->ndm_family == AF_BRIDGE)
                ++sys->dbg_counters.newmac_count;
            break;
        case RTM_DELNEIGH:
            ++sys->dbg_counters.delnbr_count;
            if (ndm->ndm_family == AF_BRIDGE)
                ++sys->dbg_counters.delmac_count;
            break;
        case RTM_NEWADDR:
            ++sys->dbg_counters.newaddr_count;
            break;
        case RTM_DELADDR:
            ++sys->dbg_counters.deladdr_count;
            break;
        default:
            ++sys->dbg_counters.unknown_type_count;
            if (sys->dbg_counters.unknown_type_count < 5)
            {
                ICCPD_LOG_NOTICE(__FUNCTION__, "NETLINK_COUNTER: Unknown type %d", netlink_msg_type);
            }
            break;
    }
}
