/*
 * iccp_cmd_show.c
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
#include <stdbool.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <net/if.h>
#include <sys/queue.h>

#include "../include/iccp_csm.h"
#include "../include/mlacp_tlv.h"
#include "../include/system.h"
#include "../include/logger.h"
#include "mclagdctl/mclagdctl.h"
#include "../include/iccp_cmd_show.h"
#include "../include/mlacp_link_handler.h"

extern int local_if_l3_proto_enabled(const char* ifname);

int iccp_mclag_config_dump(char * *buf,  int *num, int mclag_id)
{
    struct mclagd_state state_info;
    struct System *sys = NULL;
    struct CSM *csm = NULL;
    struct LocalInterface *peer_link_if = NULL;
    struct LocalInterface *lif_po = NULL;
    struct LoggerConfig* logconfig;
    char unknown[] = { "Unknown" };
    int mclag_num = 0;
    int id_exist = 0;
    int str_size = 0;
    int len = 0;
    char *state_buf = NULL;
    int state_buf_size = MCLAGDCTL_CMD_SIZE;

    if (!(sys = system_get_instance()))
    {
        return EXEC_TYPE_NO_EXIST_SYS;
    }

    state_buf = (char*)malloc(state_buf_size);
    if (!state_buf)
        return EXEC_TYPE_FAILED;

    LIST_FOREACH(csm, &(sys->csm_list), next)
    {
        memset(&state_info, 0, sizeof(struct mclagd_state));

        if (csm->current_state == ICCP_OPERATIONAL)
            state_info.keepalive = 1;
        else
            state_info.keepalive = 0;

        if (MLACP(csm).current_state == MLACP_STATE_EXCHANGE)
            state_info.info_sync_done = 1;
        else
            state_info.info_sync_done = 0;

        if (mclag_id > 0)
        {
            if (csm->mlag_id == mclag_id)
                id_exist = 1;
            else
                continue;
        }

        peer_link_if = local_if_find_by_name(csm->peer_itf_name);

        if (csm->mlag_id <= 0)
            state_info.mclag_id = -1;
        else
            state_info.mclag_id = csm->mlag_id;

        memcpy(state_info.local_ip, csm->sender_ip, ICCP_MAX_IP_STR_LEN);
        memcpy(state_info.peer_ip, csm->peer_ip, ICCP_MAX_IP_STR_LEN);

        if (peer_link_if)
            memcpy(state_info.peer_link_if, peer_link_if->name, ICCP_MAX_PORT_NAME);
        else
            memcpy(state_info.peer_link_if, unknown, strlen(unknown));

        if (peer_link_if)
            memcpy(state_info.peer_link_mac, peer_link_if->mac_addr, 6);

        state_info.keepalive_time  = csm->keepalive_time;
        state_info.session_timeout = csm->session_timeout;

        logconfig = logger_get_configuration();
        memcpy(state_info.loglevel, log_level_to_string(logconfig->log_level), strlen( log_level_to_string(logconfig->log_level)));

        state_info.role = csm->role_type;

        str_size = MCLAGDCTL_PORT_MEMBER_BUF_LEN;

        LIST_FOREACH(lif_po, &(MLACP(csm).lif_list), mlacp_next)
        {

            if (str_size - len < ICCP_MAX_PORT_NAME)
                break;

            if (lif_po->type == IF_T_PORT_CHANNEL)
                len += snprintf(state_info.enabled_po + len, str_size - len, "%s,", lif_po->name);
        }

        /*Skip the last ','*/
        len = strlen(state_info.enabled_po);
        if (len > 0)
        {
            state_info.enabled_po[len - 1] = '\0';
        }

        memcpy(state_buf + MCLAGD_REPLY_INFO_HDR + mclag_num * sizeof(struct mclagd_state),
               &state_info, sizeof(struct mclagd_state));
        mclag_num++;

        if ((mclag_num + 1) * sizeof(struct mclagd_state) > (state_buf_size - MCLAGD_REPLY_INFO_HDR))
        {
            state_buf_size += MCLAGDCTL_CMD_SIZE;
            state_buf = (char*)realloc(state_buf, state_buf_size);
            if (!state_buf)
                return EXEC_TYPE_FAILED;
        }
    }

    *buf = state_buf;
    *num = mclag_num;

    if (mclag_id > 0 && !id_exist)
        return EXEC_TYPE_NO_EXIST_MCLAGID;

    return EXEC_TYPE_SUCCESS;
}

int iccp_arp_dump(char * *buf, int *num, int mclag_id)
{
    struct System *sys = NULL;
    struct CSM *csm = NULL;
    struct Msg *msg = NULL;
    struct ARPMsg *iccpd_arp = NULL;
    struct mclagd_arp_msg mclagd_arp;
    int arp_num = 0;
    int id_exist = 0;
    char * arp_buf = NULL;
    int arp_buf_size = MCLAGDCTL_CMD_SIZE;

    if (!(sys = system_get_instance()))
    {
        return EXEC_TYPE_NO_EXIST_SYS;
    }

    arp_buf = (char*)malloc(arp_buf_size);
    if (!arp_buf)
        return EXEC_TYPE_FAILED;

    LIST_FOREACH(csm, &(sys->csm_list), next)
    {
        if (mclag_id > 0)
        {
            if (csm->mlag_id == mclag_id)
                id_exist = 1;
            else
                continue;
        }

        TAILQ_FOREACH(msg, &MLACP(csm).arp_list, tail)
        {
            memset(&mclagd_arp, 0, sizeof(struct mclagd_arp_msg));
            iccpd_arp = (struct ARPMsg*)msg->buf;

            mclagd_arp.op_type = iccpd_arp->op_type;
            mclagd_arp.learn_flag = iccpd_arp->learn_flag;
            memcpy(mclagd_arp.ifname, iccpd_arp->ifname, strlen(iccpd_arp->ifname));
            memcpy(mclagd_arp.ipv4_addr, show_ip_str(iccpd_arp->ipv4_addr), 16);
            memcpy(mclagd_arp.mac_addr, iccpd_arp->mac_addr, 6);

            memcpy(arp_buf + MCLAGD_REPLY_INFO_HDR + arp_num * sizeof(struct mclagd_arp_msg),
                   &mclagd_arp, sizeof(struct mclagd_arp_msg));

            arp_num++;

            if ((arp_num + 1) * sizeof(struct mclagd_arp_msg) > (arp_buf_size - MCLAGD_REPLY_INFO_HDR))
            {
                arp_buf_size += MCLAGDCTL_CMD_SIZE;
                arp_buf = (char*)realloc(arp_buf, arp_buf_size);
                if (!arp_buf)
                    return EXEC_TYPE_FAILED;
            }
        }
    }

    *buf = arp_buf;
    *num = arp_num;

    if (mclag_id > 0 && !id_exist)
        return EXEC_TYPE_NO_EXIST_MCLAGID;

    return EXEC_TYPE_SUCCESS;
}

int iccp_ndisc_dump(char * *buf, int *num, int mclag_id)
{
    struct System *sys = NULL;
    struct CSM *csm = NULL;
    struct Msg *msg = NULL;
    struct NDISCMsg *iccpd_ndisc = NULL;
    struct mclagd_ndisc_msg mclagd_ndisc;
    int ndisc_num = 0;
    int id_exist = 0;
    char *ndisc_buf = NULL;
    int ndisc_buf_size = MCLAGDCTL_CMD_SIZE;

    if (!(sys = system_get_instance()))
    {
        return EXEC_TYPE_NO_EXIST_SYS;
    }

    ndisc_buf = (char *)malloc(ndisc_buf_size);
    if (!ndisc_buf)
        return EXEC_TYPE_FAILED;

    LIST_FOREACH(csm, &(sys->csm_list), next)
    {
        if (mclag_id > 0)
        {
            if (csm->mlag_id == mclag_id)
                id_exist = 1;
            else
                continue;
        }

        TAILQ_FOREACH(msg, &MLACP(csm).ndisc_list, tail)
        {
            memset(&mclagd_ndisc, 0, sizeof(struct mclagd_ndisc_msg));
            iccpd_ndisc = (struct NDISCMsg *)msg->buf;

            mclagd_ndisc.op_type = iccpd_ndisc->op_type;
            mclagd_ndisc.learn_flag = iccpd_ndisc->learn_flag;
            memcpy(mclagd_ndisc.ifname, iccpd_ndisc->ifname, strlen(iccpd_ndisc->ifname));
            memcpy(mclagd_ndisc.ipv6_addr, show_ipv6_str((char *)iccpd_ndisc->ipv6_addr), 46);
            memcpy(mclagd_ndisc.mac_addr, iccpd_ndisc->mac_addr, 6);

            memcpy(ndisc_buf + MCLAGD_REPLY_INFO_HDR + ndisc_num * sizeof(struct mclagd_ndisc_msg), &mclagd_ndisc, sizeof(struct mclagd_ndisc_msg));

            ndisc_num++;

            if ((ndisc_num + 1) * sizeof(struct mclagd_ndisc_msg) > (ndisc_buf_size - MCLAGD_REPLY_INFO_HDR))
            {
                ndisc_buf_size += MCLAGDCTL_CMD_SIZE;
                ndisc_buf = (char *)realloc(ndisc_buf, ndisc_buf_size);
                if (!ndisc_buf)
                    return EXEC_TYPE_FAILED;
            }
        }
    }

    *buf = ndisc_buf;
    *num = ndisc_num;

    if (mclag_id > 0 && !id_exist)
        return EXEC_TYPE_NO_EXIST_MCLAGID;

    return EXEC_TYPE_SUCCESS;
}

int iccp_mac_dump(char * *buf, int *num, int mclag_id)
{
    struct System *sys = NULL;
    struct CSM *csm = NULL;
    struct Msg *msg = NULL;
    struct MACMsg *iccpd_mac = NULL;
    struct mclagd_mac_msg mclagd_mac;
    int mac_num = 0;
    int id_exist = 0;
    char * mac_buf = NULL;
    int mac_buf_size = MCLAGDCTL_CMD_SIZE;

    if (!(sys = system_get_instance()))
    {
        return EXEC_TYPE_NO_EXIST_SYS;
    }

    mac_buf = (char*)malloc(mac_buf_size);
    if (!mac_buf)
        return EXEC_TYPE_FAILED;

    LIST_FOREACH(csm, &(sys->csm_list), next)
    {
        if (mclag_id > 0)
        {
            if (csm->mlag_id == mclag_id)
                id_exist = 1;
            else
                continue;
        }

        RB_FOREACH (iccpd_mac, mac_rb_tree, &MLACP(csm).mac_rb)
        {
            memset(&mclagd_mac, 0, sizeof(struct mclagd_mac_msg));

            mclagd_mac.op_type = iccpd_mac->op_type;
            mclagd_mac.fdb_type = iccpd_mac->fdb_type;
            memcpy(mclagd_mac.mac_addr, iccpd_mac->mac_addr, ETHER_ADDR_LEN);
            mclagd_mac.vid = iccpd_mac->vid;
            memcpy(mclagd_mac.ifname, iccpd_mac->ifname, strlen(iccpd_mac->ifname));
            memcpy(mclagd_mac.origin_ifname, iccpd_mac->origin_ifname, strlen(iccpd_mac->origin_ifname));
            mclagd_mac.age_flag = iccpd_mac->age_flag;

            memcpy(mac_buf + MCLAGD_REPLY_INFO_HDR + mac_num * sizeof(struct mclagd_mac_msg),
                   &mclagd_mac, sizeof(struct mclagd_mac_msg));

            mac_num++;

            if ((mac_num + 1) * sizeof(struct mclagd_mac_msg) > (mac_buf_size - MCLAGD_REPLY_INFO_HDR))
            {
                mac_buf_size += MCLAGDCTL_CMD_SIZE;
                mac_buf = (char*)realloc(mac_buf, mac_buf_size);
                if (!mac_buf)
                    return EXEC_TYPE_FAILED;
            }
        }
        }

    *buf = mac_buf;
    *num = mac_num;

    if (mclag_id > 0 && !id_exist)
        return EXEC_TYPE_NO_EXIST_MCLAGID;

    return EXEC_TYPE_SUCCESS;
}

int iccp_local_if_dump(char * *buf,  int *num, int mclag_id)
{
    struct System *sys = NULL;
    struct CSM *csm = NULL;
    struct LocalInterface *lif_po = NULL;
    struct LocalInterface *lif_peer = NULL;
    struct mclagd_local_if mclagd_lif;
    struct VLAN_ID* vlan_id = NULL;
    char * str_buf = NULL;
    int str_size = MCLAGDCTL_PARA3_LEN - 1;
    int len = 0;
    int lif_num = 0;
    int id_exist = 0;
    int lif_buf_size = MCLAGDCTL_CMD_SIZE;
    char * lif_buf = NULL;

    if (!(sys = system_get_instance()))
    {
        return EXEC_TYPE_NO_EXIST_SYS;
    }

    lif_buf = (char*)malloc(lif_buf_size);
    if (!lif_buf)
        return EXEC_TYPE_FAILED;

    LIST_FOREACH(csm, &(sys->csm_list), next)
    {
        if (mclag_id > 0)
        {
            if (csm->mlag_id == mclag_id)
                id_exist = 1;
            else
                continue;
        }

        LIST_FOREACH(lif_po, &(MLACP(csm).lif_list), mlacp_next)
        {
            memset(&mclagd_lif, 0, sizeof(struct mclagd_local_if));

            mclagd_lif.ifindex = lif_po->ifindex;

            if (lif_po->type == IF_T_UNKNOW)
                memcpy(mclagd_lif.type, "Unknown", 6);
            else if (lif_po->type == IF_T_PORT)
                memcpy(mclagd_lif.type, "Ethernet", 8);
            else if (lif_po->type == IF_T_PORT_CHANNEL)
                memcpy(mclagd_lif.type, "PortChannel", 11);

            memcpy(mclagd_lif.name, lif_po->name, MAX_L_PORT_NAME);
            memcpy(mclagd_lif.mac_addr, lif_po->mac_addr, ETHER_ADDR_LEN);

            if (lif_po->state == PORT_STATE_UP)
                memcpy(mclagd_lif.state, "Up", 2);
            else if (lif_po->state == PORT_STATE_DOWN)
                memcpy(mclagd_lif.state, "Down", 4);
            else if (lif_po->state == PORT_STATE_ADMIN_DOWN)
                memcpy(mclagd_lif.state, "Admin-down", 10);
            else if (lif_po->state == PORT_STATE_TEST)
                memcpy(mclagd_lif.state, "Test", 4);

            memcpy(mclagd_lif.ipv4_addr, show_ip_str(lif_po->ipv4_addr), 16);
            mclagd_lif.prefixlen = lif_po->prefixlen;

            mclagd_lif.l3_mode = local_if_is_l3_mode(lif_po);

            mclagd_lif.is_peer_link = lif_po->is_peer_link;

            memcpy(mclagd_lif.portchannel_member_buf, lif_po->portchannel_member_buf, 512);

            mclagd_lif.po_id = lif_po->po_id;
            mclagd_lif.po_active = lif_po->po_active;
            /*mlacp_state*/
            if (lif_po->mlacp_state == MLACP_STATE_INIT)
                memcpy(mclagd_lif.mlacp_state, "INIT", 4);
            else if (lif_po->mlacp_state == MLACP_STATE_STAGE1)
                memcpy(mclagd_lif.mlacp_state, "STAGE1", 6);
            else if (lif_po->mlacp_state == MLACP_STATE_STAGE2)
                memcpy(mclagd_lif.mlacp_state, "STAGE2", 6);
            else if (lif_po->mlacp_state == MLACP_STATE_EXCHANGE)
                memcpy(mclagd_lif.mlacp_state, "EXCHANGE", 8);
            else if (lif_po->mlacp_state == MLACP_STATE_ERROR)
                memcpy(mclagd_lif.mlacp_state, "ERROR", 5);

            mclagd_lif.isolate_to_peer_link = lif_po->isolate_to_peer_link;
            mclagd_lif.is_traffic_disable = lif_po->is_traffic_disable;

            str_buf = mclagd_lif.vlanlist;

            len = 0;
            int prev_vlan_id = 0;
            int range = 0;
            int to_be_printed = 0;

            RB_FOREACH (vlan_id, vlan_rb_tree, &(lif_po->vlan_tree))
            {
                if (str_size - len < 4)
                    break;
                if (!prev_vlan_id || vlan_id->vid != prev_vlan_id + 1)
                {
                    if (range)
                    {
                        if (str_size - len < (4 + ((range)?8:0)))
                        {
                            break;
                        }
                        len += snprintf(str_buf + len, str_size - len, "- %d ", prev_vlan_id);
                    }
                    len += snprintf(str_buf + len, str_size - len, "%d ", vlan_id->vid);
                    range = 0;
                    to_be_printed = 0;
                }
                else
                {
                    range = 1;
                    to_be_printed = 1;
                }
                prev_vlan_id = vlan_id->vid;
            }

            if (to_be_printed && (str_size - len > (4 + ((range)?8:0))))
            {
                if (prev_vlan_id)
                {
                    len += snprintf(str_buf + len, str_size - len, "%s%d ", range?"- ":"", prev_vlan_id);
                    range = 0;
                }
            }

            memcpy(lif_buf + MCLAGD_REPLY_INFO_HDR + lif_num * sizeof(struct mclagd_local_if),
                   &mclagd_lif, sizeof(struct mclagd_local_if));

            lif_num++;

            if ((lif_num + 1) * sizeof(struct mclagd_local_if) > (lif_buf_size - MCLAGD_REPLY_INFO_HDR))
            {
                lif_buf_size += MCLAGDCTL_CMD_SIZE;
                lif_buf = (char*)realloc(lif_buf, lif_buf_size);
                if (!lif_buf)
                    return EXEC_TYPE_FAILED;
            }
        }

        if (csm->peer_link_if) {

            lif_peer = csm->peer_link_if;

            memset(&mclagd_lif, 0, sizeof(struct mclagd_local_if));

            mclagd_lif.ifindex = lif_peer->ifindex;

            if (lif_peer->type == IF_T_UNKNOW)
                memcpy(mclagd_lif.type, "Unknown", strlen("unknown"));
            else if (lif_peer->type == IF_T_PORT)
                memcpy(mclagd_lif.type, "Ethernet", strlen("Ethernet"));
            else if (lif_peer->type == IF_T_PORT_CHANNEL)
                memcpy(mclagd_lif.type, "PortChannel", 11);

            memcpy(mclagd_lif.name, lif_peer->name, MAX_L_PORT_NAME);
            memcpy(mclagd_lif.mac_addr, lif_peer->mac_addr, ETHER_ADDR_LEN);

            if (lif_peer->state == PORT_STATE_UP)
                memcpy(mclagd_lif.state, "Up", strlen("Up"));
            else if (lif_peer->state == PORT_STATE_DOWN)
                memcpy(mclagd_lif.state, "Down", strlen("Down"));
            else if (lif_peer->state == PORT_STATE_ADMIN_DOWN)
                memcpy(mclagd_lif.state, "Admin-down", strlen("Admin-down"));
            else if (lif_peer->state == PORT_STATE_TEST)
                memcpy(mclagd_lif.state, "Test", strlen("Test"));

            memcpy(mclagd_lif.ipv4_addr, show_ip_str(lif_peer->ipv4_addr), 16);
            mclagd_lif.prefixlen = lif_peer->prefixlen;

            mclagd_lif.l3_mode = local_if_is_l3_mode(lif_peer);

            mclagd_lif.is_peer_link = lif_peer->is_peer_link;

            memcpy(mclagd_lif.portchannel_member_buf, lif_peer->portchannel_member_buf, 512);

            mclagd_lif.po_id = lif_peer->po_id;
            mclagd_lif.po_active = lif_peer->po_active;
            /*mlacp_state*/
            if (lif_peer->mlacp_state == MLACP_STATE_INIT)
                memcpy(mclagd_lif.mlacp_state, "INIT", strlen("INIT"));
            else if (lif_peer->mlacp_state == MLACP_STATE_STAGE1)
                memcpy(mclagd_lif.mlacp_state, "STAGE1", strlen("STAGE1"));
            else if (lif_peer->mlacp_state == MLACP_STATE_STAGE2)
                memcpy(mclagd_lif.mlacp_state, "STAGE2", strlen("STAGE2"));
            else if (lif_peer->mlacp_state == MLACP_STATE_EXCHANGE)
                memcpy(mclagd_lif.mlacp_state, "EXCHANGE", strlen("EXCHANGE"));
            else if (lif_peer->mlacp_state == MLACP_STATE_ERROR)
                memcpy(mclagd_lif.mlacp_state, "ERROR", strlen("ERROR"));

            mclagd_lif.isolate_to_peer_link = lif_peer->isolate_to_peer_link;
            mclagd_lif.is_traffic_disable = lif_peer->is_traffic_disable;

            str_buf = mclagd_lif.vlanlist;

            len = 0;
            int prev_vlan_id = 0;
            int range = 0;
            int to_be_printed = 0;

            RB_FOREACH (vlan_id, vlan_rb_tree, &(lif_peer->vlan_tree))
            {
                if (str_size - len < 4)
                    break;
                if (!prev_vlan_id || vlan_id->vid != prev_vlan_id + 1)
                {
                    if (range)
                    {
                        if (str_size - len < (4 + ((range)?8:0)))
                        {
                            break;
                        }
                        len += snprintf(str_buf + len, str_size - len, "- %d ", prev_vlan_id);
                    }
                    len += snprintf(str_buf + len, str_size - len, "%d ", vlan_id->vid);
                    range = 0;
                    to_be_printed = 0;
                }
                else
                {
                    range = 1;
                    to_be_printed = 1;
                }
                prev_vlan_id = vlan_id->vid;
            }

            if (to_be_printed && (str_size - len > (4 + ((range)?8:0))))
            {
                if (prev_vlan_id)
                {
                    len += snprintf(str_buf + len, str_size - len, "%s%d ", range?"- ":"", prev_vlan_id);
                    range = 0;
                }
            }

            memcpy(lif_buf + MCLAGD_REPLY_INFO_HDR + lif_num * sizeof(struct mclagd_local_if),
                   &mclagd_lif, sizeof(struct mclagd_local_if));

            lif_num++;

            if ((lif_num + 1) * sizeof(struct mclagd_local_if) > (lif_buf_size - MCLAGD_REPLY_INFO_HDR))
            {
                lif_buf_size += MCLAGDCTL_CMD_SIZE;
                lif_buf = (char*)realloc(lif_buf, lif_buf_size);
                if (!lif_buf)
                    return EXEC_TYPE_FAILED;
            }

        }
    }

    *buf = lif_buf;
    *num = lif_num;

    if (mclag_id > 0 && !id_exist)
        return EXEC_TYPE_NO_EXIST_MCLAGID;

    return EXEC_TYPE_SUCCESS;
}

int iccp_peer_if_dump(char * *buf, int *num, int mclag_id)
{
    struct System *sys = NULL;
    struct CSM *csm = NULL;
    struct PeerInterface *pif_po = NULL;
    struct mclagd_peer_if mclagd_pif;
    int pif_num = 0;
    int id_exist = 0;
    int pif_buf_size = MCLAGDCTL_CMD_SIZE;
    char *pif_buf = NULL;

    if (!(sys = system_get_instance()))
    {
        ICCPD_LOG_INFO(__FUNCTION__, "cannot find sys!\n");
        return EXEC_TYPE_NO_EXIST_SYS;
    }

    pif_buf = (char*)malloc(pif_buf_size);
    if (!pif_buf)
        return EXEC_TYPE_FAILED;

    LIST_FOREACH(csm, &(sys->csm_list), next)
    {
        if (mclag_id > 0)
        {
            if (csm->mlag_id == mclag_id)
                id_exist = 1;
            else
                continue;
        }

        LIST_FOREACH(pif_po, &(MLACP(csm).pif_list), mlacp_next)
        {
            memset(&mclagd_pif, 0, sizeof(struct mclagd_peer_if));

            mclagd_pif.ifindex = pif_po->ifindex;

            if (pif_po->type == IF_T_UNKNOW)
                memcpy(mclagd_pif.type, "Unknown", 6);
            else if (pif_po->type == IF_T_PORT)
                memcpy(mclagd_pif.type, "Ethernet", 8);
            else if (pif_po->type == IF_T_PORT_CHANNEL)
                memcpy(mclagd_pif.type, "PortChannel", 11);

            memcpy(mclagd_pif.name, pif_po->name, MAX_L_PORT_NAME);
            memcpy(mclagd_pif.mac_addr, pif_po->mac_addr, ETHER_ADDR_LEN);

            if (pif_po->state == PORT_STATE_UP)
                memcpy(mclagd_pif.state, "Up", 2);
            else if (pif_po->state == PORT_STATE_DOWN)
                memcpy(mclagd_pif.state, "Down", 4);
            else if (pif_po->state == PORT_STATE_ADMIN_DOWN)
                memcpy(mclagd_pif.state, "Admin-down", 10);
            else if (pif_po->state == PORT_STATE_TEST)
                memcpy(mclagd_pif.state, "Test", 4);

            mclagd_pif.po_id = pif_po->po_id;
            mclagd_pif.po_active = pif_po->po_active;

            memcpy(pif_buf + MCLAGD_REPLY_INFO_HDR + pif_num * sizeof(struct mclagd_peer_if),
                   &mclagd_pif, sizeof(struct mclagd_peer_if));

            pif_num++;

            if ((pif_num + 1) * sizeof(struct mclagd_peer_if) > (pif_buf_size - MCLAGD_REPLY_INFO_HDR))
            {
                pif_buf_size += MCLAGDCTL_CMD_SIZE;
                pif_buf = (char*)realloc(pif_buf, pif_buf_size);
                if (!pif_buf)
                    return EXEC_TYPE_FAILED;
            }
        }
    }

    *buf = pif_buf;
    *num = pif_num;

    if (mclag_id > 0 && !id_exist)
        return EXEC_TYPE_NO_EXIST_MCLAGID;

    return EXEC_TYPE_SUCCESS;
}

/* Allocate a buffer to return the internal debug counters
 * The allocated buffer should include MCLAGD_REPLY_INFO_HDR byte header
 * No buffer is allocated if error is returned
 */
int iccp_cmd_dbg_counter_dump(char **buf, int *data_len, int mclag_id)
{
    struct System *sys = NULL;
    struct CSM *csm = NULL;
    char *temp_ptr, *counter_buf = NULL;
    mclagd_dbg_counter_info_t *counter_ptr;
    int buf_size = 0;
    int id_exist = 0;
    int num_csm = 0;
    bool is_first_csm;

    if (!(sys = system_get_instance()))
    {
        ICCPD_LOG_INFO(__FUNCTION__, "cannot find sys!\n");
        return EXEC_TYPE_NO_EXIST_SYS;
    }
    if (mclag_id >0)
        num_csm = 1;
    else
    {
        LIST_FOREACH(csm, &(sys->csm_list), next)
        {
            ++num_csm;
        }
    }
    buf_size = MCLAGD_REPLY_INFO_HDR + sizeof(mclagd_dbg_counter_info_t) +
            (sizeof(mlacp_dbg_counter_info_t) * num_csm);
    counter_buf = (char*)malloc(buf_size);
    if (!counter_buf)
        return EXEC_TYPE_FAILED;

    memset(counter_buf, 0, buf_size);
    counter_ptr =
        (mclagd_dbg_counter_info_t *)(counter_buf + MCLAGD_REPLY_INFO_HDR);
    memcpy(&counter_ptr->system_dbg, &sys->dbg_counters, sizeof(sys->dbg_counters));
    counter_ptr->num_iccp_counter_blocks = num_csm;
    temp_ptr = counter_ptr->iccp_dbg_counters;
    is_first_csm = true;

    LIST_FOREACH(csm, &(sys->csm_list), next)
    {
        if (mclag_id >0)
        {
            if (csm->mlag_id == mclag_id)
                id_exist = 1;
            else
                continue;
        }
        if (is_first_csm)
            is_first_csm = false;
        else
            temp_ptr += sizeof(MLACP(csm).dbg_counters);

        memcpy(temp_ptr, &MLACP(csm).dbg_counters,
            sizeof(MLACP(csm).dbg_counters));
    }

    if (mclag_id >0 && !id_exist)
    {
        if (counter_buf)
            free(counter_buf);
        return EXEC_TYPE_NO_EXIST_MCLAGID;
    }
    *buf = counter_buf;
    *data_len = buf_size - MCLAGD_REPLY_INFO_HDR;
    return EXEC_TYPE_SUCCESS;
}

int iccp_unique_ip_if_dump(char **buf, int *num, int mclag_id)
{
    struct System *sys = NULL;
    struct mclagd_unique_ip_if mclagd_lif;
    char *str_buf = NULL;
    int str_size = MCLAGDCTL_PARA3_LEN - 1;
    int len = 0;
    int lif_num = 0;
    int lif_buf_size = MCLAGDCTL_CMD_SIZE;
    char *lif_buf = NULL;
    struct Unq_ip_If_info* unq_ip_if = NULL;

    if (!(sys = system_get_instance()))
    {
        ICCPD_LOG_INFO(__FUNCTION__, "cannot find sys!\n");
        return EXEC_TYPE_NO_EXIST_SYS;
    }

    lif_buf = (char*)malloc(lif_buf_size);
    if (!lif_buf)
        return EXEC_TYPE_FAILED;

    LIST_FOREACH(unq_ip_if, &(sys->unq_ip_if_list), if_next)
    {
        memset(&mclagd_lif, 0, sizeof(struct mclagd_unique_ip_if));
        memcpy(mclagd_lif.name, unq_ip_if->name, MAX_L_PORT_NAME);
        mclagd_lif.active = local_if_l3_proto_enabled(unq_ip_if->name);

        memcpy(lif_buf + MCLAGD_REPLY_INFO_HDR + lif_num * sizeof(struct mclagd_unique_ip_if),
                &mclagd_lif, sizeof(struct mclagd_unique_ip_if));

        lif_num++;

        if ((lif_num + 1) * sizeof(struct mclagd_unique_ip_if) > (lif_buf_size - MCLAGD_REPLY_INFO_HDR))
        {
            lif_buf_size += MCLAGDCTL_CMD_SIZE;
            lif_buf = (char*)realloc(lif_buf, lif_buf_size);
            if (!lif_buf)
                return EXEC_TYPE_FAILED;
        }
    }

    *buf = lif_buf;
    *num = lif_num;

    return EXEC_TYPE_SUCCESS;
}
