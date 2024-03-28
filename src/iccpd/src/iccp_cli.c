/*
 * iccp_cli.c
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

#include <stdint.h>

#include "../include/system.h"
#include "../include/scheduler.h"
#include "../include/logger.h"
#include "../include/iccp_csm.h"
#include "../include/mlacp_link_handler.h"
#include "../include/iccp_netlink.h"
/*
 * 'id <1-65535>' command
 */
int set_mc_lag_id( struct CSM *csm, uint16_t id)
{
    if (!csm)
        return MCLAG_ERROR;

    ICCPD_LOG_INFO(__FUNCTION__, "Set mlag-id : %d", id);

    /* Mlag-ID, RG-ID, MLACP-ID
       Temporary let the three id be the same*/
    csm->mlag_id = id;
    csm->iccp_info.icc_rg_id = id;
    csm->app_csm.mlacp.id = id;
    return 0;
}

int unset_mc_lag_id( struct CSM *csm, uint16_t id)
{
    if (!csm)
        return MCLAG_ERROR;

    /* Remove ICCP info from STATE_DB */
    mlacp_link_del_iccp_info(csm->mlag_id);

    iccp_csm_finalize(csm);

    return 0;
}

/*
 * 'peer-link WORD' command
 */
int set_peer_link(int mid, const char* ifname)
{
    struct CSM* csm = NULL;
    struct LocalInterface *lif = NULL;
    size_t len = 0;

    len = strlen(ifname);

    if (strncmp(ifname, FRONT_PANEL_PORT_PREFIX, strlen(FRONT_PANEL_PORT_PREFIX)) != 0
        && strncmp(ifname, PORTCHANNEL_PREFIX, strlen(PORTCHANNEL_PREFIX)) != 0
        && strncmp(ifname, VXLAN_TUNNEL_PREFIX, strlen(VXLAN_TUNNEL_PREFIX)) != 0)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Peer-link is %s, must be Ethernet or PortChannel or VTTNL(Vxlan tunnel)", ifname);
        return MCLAG_ERROR;
    }

    csm = system_get_csm_by_mlacp_id(mid);
    if (csm == NULL)
        return MCLAG_ERROR;

    if (len > MAX_L_PORT_NAME)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Peer-link %s, Strlen %d greater than MAX:%d ", ifname, len, MAX_L_PORT_NAME);
        return MCLAG_ERROR;
    }

    if (strlen(csm->peer_itf_name) > 0)
    {
        if (strcmp(csm->peer_itf_name, ifname) == 0)
        {
            ICCPD_LOG_INFO(__FUNCTION__, "Peer-link not be changed");
            return 0;
        }
        else
        {
            ICCPD_LOG_INFO(__FUNCTION__, "Change peer-link : %s -> %s",
                           csm->peer_itf_name, ifname);

            /*disconnect the link for mac and arp sync up before change peer_itf_name*/
            scheduler_session_disconnect_handler(csm);

            if (csm->peer_link_if)
            {
                csm->peer_link_if->is_peer_link = 0;
                csm->peer_link_if = NULL;
            }
        }
    }
    else
    {
        ICCPD_LOG_INFO(__FUNCTION__, "Set mlag %d peer-link : %s",
                       csm->mlag_id, ifname);
    }

    memset(csm->peer_itf_name, 0, IFNAMSIZ);
    if (len > IFNAMSIZ)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "len=%d greater than IFNAMESIZ=%d", len, IFNAMSIZ);
        return MCLAG_ERROR;
    }
    memcpy(csm->peer_itf_name, ifname, len);

    /* update peer-link link handler*/
    lif = local_if_find_by_name(csm->peer_itf_name);
    if (lif)
    {
        /*When set peer-link, the local-if is already created*/
        csm->peer_link_if = lif;
        lif->is_peer_link = 1;
        MLACP(csm).system_config_changed = 1;

        if (lif->type == IF_T_PORT_CHANNEL)
            iccp_get_port_member_list(lif);

        set_peerlink_learn_kernel(csm, 0, 4);
    }

    /*disconnect the link for mac and arp sync up*/
    scheduler_session_disconnect_handler(csm);

    return 0;
}

int unset_peer_link(int mid)
{
    struct CSM* csm = NULL;

    csm = system_get_csm_by_mlacp_id(mid);
    if (csm == NULL)
        return MCLAG_ERROR;

    if (MLACP(csm).current_state == MLACP_STATE_EXCHANGE)
    {
        /*must be enabled mac learn*/
        if (csm->peer_link_if) {
            set_peerlink_mlag_port_learn(csm->peer_link_if, 1);
            set_peerlink_learn_kernel(csm, 1, 5);
        }
    }

    /* update peer-link link handler*/
    scheduler_session_disconnect_handler(csm);

    /* clean peer-link*/
    memset(csm->peer_itf_name, 0, MAX_L_PORT_NAME);
    if (csm->peer_link_if)
    {
        csm->peer_link_if->is_peer_link = 0;
        csm->peer_link_if = NULL;
        MLACP(csm).system_config_changed = 1;
    }

    return 0;
}

/*
 * 'local ip address A.B.C.D' command
 */
int set_local_address(int mid, const char* addr)
{
    struct CSM* csm = NULL;
    size_t len = 0;

    csm = system_get_csm_by_mlacp_id(mid);
    if (csm == NULL)
        return MCLAG_ERROR;
    if (addr == NULL)
        return MCLAG_ERROR;

    if (strlen(csm->sender_ip) > 0)
    {
        if (strcmp(csm->sender_ip, addr) == 0)
        {
            ICCPD_LOG_INFO(__FUNCTION__, "Local-address not be changed");
            return 0;
        }
        else
        {
            ICCPD_LOG_INFO(__FUNCTION__, "Change local-address : %s -> %s",
                           csm->sender_ip, addr);
            scheduler_session_disconnect_handler(csm);
        }
    }
    else
    {
        ICCPD_LOG_INFO(__FUNCTION__, "Set local-address : %s", addr);
    }

    len = strlen(addr);
    memset(csm->sender_ip, 0, INET_ADDRSTRLEN);
    if (len > INET_ADDRSTRLEN)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "len=%d greater than INET_ADDRSTRLEN=%d ", len, INET_ADDRSTRLEN);
        return MCLAG_ERROR;
    }
    memcpy(csm->sender_ip, addr, len);
    memset(csm->iccp_info.sender_name, 0, INET_ADDRSTRLEN);
    if (len > INET_ADDRSTRLEN)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "len=%d greater than INET_ADDRSTRLEN=%d ", len, INET_ADDRSTRLEN);
        return MCLAG_ERROR;
    }
    memcpy(csm->iccp_info.sender_name, addr, len);

    return 0;
}

int unset_local_address(int mid)
{
    struct CSM* csm = NULL;

    csm = system_get_csm_by_mlacp_id(mid);
    if (csm == NULL)
        return MCLAG_ERROR;

    memset(csm->sender_ip, 0, INET_ADDRSTRLEN);
    memset(csm->iccp_info.sender_name, 0, INET_ADDRSTRLEN);

    /* reset link*/
    scheduler_session_disconnect_handler(csm);

    return 0;
}

/*
 * 'peer-address A.B.C.D' command
 */
int set_peer_address(int mid, const char* addr)
{
    struct CSM* csm = NULL;
    size_t len = 0;

    csm = system_get_csm_by_mlacp_id(mid);
    if (csm == NULL)
        return MCLAG_ERROR;
    if (addr == NULL)
        return MCLAG_ERROR;

    len = strlen(addr);

    if (strlen(csm->peer_ip) > 0)
    {
        if (strcmp(csm->peer_ip, addr) == 0)
        {
            ICCPD_LOG_INFO(__FUNCTION__, "Peer-address not be changed");
            return 0;
        }
        else
        {
            ICCPD_LOG_INFO(__FUNCTION__, "Change peer-address : %s -> %s",
                           csm->peer_ip, addr);
            scheduler_session_disconnect_handler(csm);
        }
    }
    else
    {
        ICCPD_LOG_INFO(__FUNCTION__, "Set peer-address : %s", addr);
    }

    memset(csm->peer_ip, 0, INET_ADDRSTRLEN);
    if (len > INET_ADDRSTRLEN)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "len=%d greater than INET_ADDRSTRLEN=%d ", len, INET_ADDRSTRLEN);
        return MCLAG_ERROR;
    }
    memcpy(csm->peer_ip, addr, len);

    return 0;
}

int unset_peer_address(int mid)
{
    struct CSM* csm = NULL;

    csm = system_get_csm_by_mlacp_id(mid);
    if (csm == NULL)
        return MCLAG_ERROR;

    memset(csm->peer_ip, 0, INET_ADDRSTRLEN);

    /* reset link*/
    scheduler_session_disconnect_handler(csm);

    return 0;
}

int set_keepalive_time(int mid, int keepalive_time)
{
    struct CSM* csm = NULL;
    size_t len = 0;

    csm = system_get_csm_by_mlacp_id(mid);
    if (csm == NULL)
        return MCLAG_ERROR;

    ICCPD_LOG_DEBUG(__FUNCTION__, "Set keepalive_time : %d", keepalive_time);

    if (csm->keepalive_time != keepalive_time)
    {
        csm->keepalive_time = keepalive_time;
        //reset heartbeat send time to send keepalive immediately
        csm->heartbeat_send_time = 0;
    }
    return 0;
}

int set_session_timeout(int mid, int session_timeout_val)
{
    struct CSM* csm = NULL;
    size_t len = 0;

    csm = system_get_csm_by_mlacp_id(mid);
    if (csm == NULL)
        return MCLAG_ERROR;

    ICCPD_LOG_DEBUG(__FUNCTION__, "Set session timeout : %d", session_timeout_val);

    csm->session_timeout = session_timeout_val;
    return 0;
}


int iccp_cli_attach_mclag_domain_to_port_channel( int domain, const char* ifname)
{
    struct CSM* csm = NULL;
    struct LocalInterface *lif = NULL;
    struct If_info * cif = NULL;

    if (!ifname)
        return MCLAG_ERROR;

    if (strncmp(ifname, PORTCHANNEL_PREFIX, strlen(PORTCHANNEL_PREFIX)) != 0)
    {
        ICCPD_LOG_WARN(__FUNCTION__, "Attach interface(%s) is not a port-channel", ifname);
        return MCLAG_ERROR;
    }

    csm = system_get_csm_by_mlacp_id(domain);
    if (csm == NULL)
    {
        ICCPD_LOG_WARN(__FUNCTION__, "MC-LAG ID %d doesn't exist", domain);
        return MCLAG_ERROR;
    }

    lif = local_if_find_by_name(ifname);
    if (lif)
    {
        mlacp_bind_port_channel_to_csm(csm, ifname);
    }

    LIST_FOREACH(cif, &(csm->if_bind_list), csm_next)
    {
        if (strcmp(cif->name, ifname) == 0)
            break;
    }

    if (cif == NULL)
    {
        cif = (struct If_info *)malloc(sizeof(struct If_info));
        if (!cif)
            return MCLAG_ERROR;

        snprintf(cif->name, MAX_L_PORT_NAME, "%s", ifname);
        LIST_INSERT_HEAD(&(csm->if_bind_list), cif, csm_next);
    }

    return 0;
}

int iccp_cli_detach_mclag_domain_to_port_channel( const char* ifname)
{
    int unbind_poid = -1;
    struct CSM *csm = NULL;
    struct LocalInterface *lif_po = NULL;
    struct LocalInterface *lif = NULL;
    struct If_info * cif = NULL;

    if (!ifname)
        return MCLAG_ERROR;

    if (strncmp(ifname, PORTCHANNEL_PREFIX, strlen(PORTCHANNEL_PREFIX)) != 0)
    {
        ICCPD_LOG_WARN(__FUNCTION__, "Detach interface(%s) is not a port-channel", ifname);
        return MCLAG_ERROR;
    }

    /* find po*/
    if (!(lif_po = local_if_find_by_name(ifname))
        || lif_po->type != IF_T_PORT_CHANNEL
        || lif_po->po_id <= 0
        || lif_po->csm == NULL)
    {
        if (lif_po)
        {
            ICCPD_LOG_DEBUG(__FUNCTION__, "CSM already detached for ifname = %s", lif_po->name);
        }
        return MCLAG_ERROR;
    }

    /* find csm*/
    csm = lif_po->csm;

    if(!csm)
    {
        ICCPD_LOG_WARN(__FUNCTION__, "unexpected condition!!!; lif->csm not found!; Detach mclag from ifname = %s", lif_po->name);
        return 0;
    }

    ICCPD_LOG_DEBUG(__FUNCTION__, "Detach mclag id = %d from ifname = %s",
                    csm->mlag_id, lif_po->name);

    //if it is standby node change back the mac to its original system mac
    recover_if_ipmac_on_standby(lif_po, 1);

    /* process link state handler before detaching it.*/
    mlacp_mlag_intf_detach_handler(csm, lif_po);

    unbind_poid = lif_po->po_id;
    mlacp_unbind_local_if(lif_po);
    LIST_FOREACH(lif, &(csm->app_csm.mlacp.lif_list), mlacp_next)
    {
        if (lif->type == IF_T_PORT && lif->po_id == unbind_poid)
            mlacp_unbind_local_if(lif);
    }

    LIST_FOREACH(cif, &(csm->if_bind_list), csm_next)
    {
        if (strcmp(ifname, cif->name) == 0)
            LIST_REMOVE(cif, csm_next);
    }

    return 0;
}

/* This function parses a string to a binary mac address (uint8_t[6])
    The string should contain mac address only. No spaces are allowed.
    The mac address separators could be either ':' or '-'*/
int parseMacString(const char * str_mac, uint8_t* bin_mac)
{
    int i;

    if (bin_mac == NULL)
    {
        return MCLAG_ERROR;
    }

    /* 6 hexadecimal numbers (two digits each) + 5 delimiters*/
    if (strlen(str_mac) != ETHER_ADDR_LEN * 2 + 5)
    {
        return MCLAG_ERROR;
    }

    /* first check that all mac address separators are equal to each other
        2, 5, 8, 11, and 14 are MAC address separator positions*/
    if (!(str_mac[2]  == str_mac[5]
          && str_mac[5]  == str_mac[8]
          && str_mac[8]  == str_mac[11]
          && str_mac[11] == str_mac[14]))
    {
        return MCLAG_ERROR;
    }

    /* then check that the first separator is equal to ':' or '-'*/
    if (str_mac[2] != ':' && str_mac[2] != '-')
    {
        return MCLAG_ERROR;
    }

    for (i = 0; i < ETHER_ADDR_LEN; ++i)
    {
        int left = i * 3;       /* left  digit position of hexadecimal number*/
        int right = left + 1;   /* right digit position of hexadecimal number*/

        if (str_mac[left] >= '0' && str_mac[left] <= '9')
        {
            bin_mac[i] = (uint8_t)(str_mac[left] - '0');
        }
        else if (str_mac[left] >= 'A' && str_mac[left] <= 'F')
        {
            bin_mac[i] = (uint8_t)(str_mac[left] - 'A' + 0x0a);
        }
        else if (str_mac[left] >= 'a' && str_mac[left] <= 'f')
        {
            bin_mac[i] = (uint8_t)(str_mac[left] - 'a' + 0x0a);
        }
        else
        {
            return MCLAG_ERROR;
        }

        bin_mac[i] = (uint8_t)(bin_mac[i] << 4);

        if (str_mac[right] >= '0' && str_mac[right] <= '9')
        {
            bin_mac[i] |= (uint8_t)(str_mac[right] - '0');
        }
        else if (str_mac[right] >= 'A' && str_mac[right] <= 'F')
        {
            bin_mac[i] |= (uint8_t)(str_mac[right] - 'A' + 0x0a);
        }
        else if (str_mac[right] >= 'a' && str_mac[right] <= 'f')
        {
            bin_mac[i] |= (uint8_t)(str_mac[right] - 'a' + 0x0a);
        }
        else
        {
            return MCLAG_ERROR;
        }
    }

    return 0;
}

int set_local_system_id(const char* mac)
{
    struct System* sys = NULL;
    struct CSM* csm = NULL;

    if ((sys = system_get_instance()) == NULL )
        return 0;

    LIST_FOREACH(csm, &(sys->csm_list), next)
    {
        parseMacString(mac, MLACP(csm).system_id);

        ICCPD_LOG_DEBUG(__FUNCTION__, "   Set local systemID [%02X:%02X:%02X:%02X:%02X:%02X].",
                        MLACP(csm).system_id[0], MLACP(csm).system_id[1], MLACP(csm).system_id[2],
                        MLACP(csm).system_id[3], MLACP(csm).system_id[4], MLACP(csm).system_id[5]);
    }

    return 0;
}

int unset_local_system_id( )
{
    uint8_t null_mac[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    struct System* sys = NULL;
    struct CSM* csm = NULL;

    if ((sys = system_get_instance()) == NULL )
        return 0;

    LIST_FOREACH(csm, &(sys->csm_list), next)
    {
        memcpy(MLACP(csm).system_id, null_mac, ETHER_ADDR_LEN);
    }

    return 0;
}

