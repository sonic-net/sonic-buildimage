/*
 * MLACP Sync Infomation Preparation
 * mlacp_sync_prepare.c

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
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include <sys/queue.h>

#include "../include/system.h"
#include "../include/logger.h"
#include "../include/mlacp_fsm.h"
#include "../include/mlacp_tlv.h"
#include "../include/mlacp_link_handler.h"
#include "../include/iccp_ifm.h"
#include "../include/iccp_csm.h"

/*****************************************
* Static Function
*
* ***************************************/
static int mlacp_fill_icc_header(struct CSM* csm, ICCHdr* icc_hdr, size_t msg_len);

/*****************************************
* Create Sync Request TLV
*
* ***************************************/
int mlacp_prepare_for_sync_request_tlv(struct CSM* csm, char* buf, size_t max_buf_size)
{
    struct System* sys = NULL;
    ICCHdr* icc_hdr = NULL;
    mLACPSyncReqTLV* tlv = NULL;
    size_t msg_len = sizeof(ICCHdr) + sizeof(mLACPSyncReqTLV);

    if (csm == NULL)
        return MCLAG_ERROR;

    if (buf == NULL)
        return MCLAG_ERROR;

    if (msg_len > max_buf_size)
        return MCLAG_ERROR;

    if ((sys = system_get_instance()) == NULL)
        return MCLAG_ERROR;

    /* Prepare for sync request */
    memset(buf, 0, max_buf_size);

    icc_hdr = (ICCHdr*)buf;
    tlv = (mLACPSyncReqTLV*)&buf[sizeof(ICCHdr)];

    /* ICC header */
    mlacp_fill_icc_header(csm, icc_hdr, msg_len);

    /* mLACP Synchronization Request TLV */
    tlv->icc_parameter.u_bit = 0;
    tlv->icc_parameter.f_bit = 0;
    tlv->icc_parameter.type = htons(TLV_T_MLACP_SYNC_REQUEST);
    tlv->icc_parameter.len = htons(sizeof(mLACPSyncReqTLV) - sizeof(ICCParameter));

    tlv->req_num = 0;
    MLACP(csm).sync_req_num = 0;

    tlv->c_bit = 1;
    tlv->s_bit = 1;
    tlv->req_type = 0x3FFF;
    *(uint16_t *)((uint8_t *)tlv + sizeof(ICCParameter) + sizeof(uint16_t)) = htons(*(uint16_t *)((uint8_t *)tlv + sizeof(ICCParameter) + sizeof(uint16_t)));

    tlv->port_num_agg_id = 0;
    tlv->actor_key = 0;

    ICCPD_LOG_DEBUG("ICCP_FSM", "TX sync_request: role %s, sync_state %s",
        (csm->role_type == STP_ROLE_STANDBY) ? "standby" : "active",
        mlacp_state(csm));

    return msg_len;
}

/*****************************************
* Prprare Sync Data TLV
*
* ***************************************/
int mlacp_prepare_for_sync_data_tlv(struct CSM* csm, char* buf, size_t max_buf_size, int end)
{
    struct System* sys = NULL;
    ICCHdr* icc_hdr = (ICCHdr*)buf;
    mLACPSyncDataTLV* tlv = (mLACPSyncDataTLV*)&buf[sizeof(ICCHdr)];
    size_t msg_len = sizeof(ICCHdr) + sizeof(mLACPSyncDataTLV);

    if (csm == NULL)
        return MCLAG_ERROR;

    if (buf == NULL)
        return MCLAG_ERROR;

    if (msg_len > max_buf_size)
        return MCLAG_ERROR;

    if ((sys = system_get_instance()) == NULL)
        return MCLAG_ERROR;

    /* Prepare for sync request */
    memset(buf, 0, max_buf_size);

    icc_hdr = (ICCHdr*)buf;
    tlv = (mLACPSyncDataTLV*)&buf[sizeof(ICCHdr)];

    /* ICC header */
    mlacp_fill_icc_header(csm, icc_hdr, msg_len);

    /* mLACP Synchronization Data TLV */
    tlv->icc_parameter.u_bit = 0;
    tlv->icc_parameter.f_bit = 0;
    tlv->icc_parameter.type = htons(TLV_T_MLACP_SYNC_DATA);
    tlv->icc_parameter.len = htons(sizeof(mLACPSyncDataTLV) - sizeof(ICCParameter));

    tlv->req_num = htons(MLACP(csm).sync_req_num);
    if (end == 0)
        tlv->flags = 0x00;
    else
        tlv->flags = htons(0x01);

    ICCPD_LOG_DEBUG("ICCP_CSM", "TX sync %s", end ? "end" : "start");
    return msg_len;
}

/*****************************************
* Prprare Sync System-Config TLV
*
* ***************************************/
int mlacp_prepare_for_sys_config(struct CSM* csm, char* buf, size_t max_buf_size)
{
    struct System* sys = NULL;
    ICCHdr* icc_hdr = (ICCHdr*)buf;
    mLACPSysConfigTLV* tlv = (mLACPSysConfigTLV*)&buf[sizeof(ICCHdr)];
    size_t msg_len = sizeof(ICCHdr) + sizeof(mLACPSysConfigTLV);

    if (csm == NULL)
        return MCLAG_ERROR;

    if (buf == NULL)
        return MCLAG_ERROR;

    if (msg_len > max_buf_size)
        return MCLAG_ERROR;

    if ((sys = system_get_instance()) == NULL)
        return MCLAG_ERROR;

    /* Prepare for sync request */
    memset(buf, 0, max_buf_size);

    icc_hdr = (ICCHdr*)buf;
    tlv = (mLACPSysConfigTLV*)&buf[sizeof(ICCHdr)];

    /* ICC header */
    mlacp_fill_icc_header(csm, icc_hdr, msg_len);

    /* System Config TLV */
    tlv->icc_parameter.u_bit = 0;
    tlv->icc_parameter.f_bit = 0;
    tlv->icc_parameter.type = htons(TLV_T_MLACP_SYSTEM_CONFIG);
    tlv->icc_parameter.len = htons(sizeof(mLACPSysConfigTLV) - sizeof(ICCParameter));

    memcpy(tlv->sys_id, MLACP(csm).system_id, ETHER_ADDR_LEN);
    tlv->sys_priority = htons(MLACP(csm).system_priority);
    tlv->node_id = MLACP(csm).node_id;

    ICCPD_LOG_DEBUG("ICCP_FSM", "TX sys_config: systemID %s, priority %u, nodeID %u",
        mac_addr_to_str(MLACP(csm).system_id), MLACP(csm).system_priority,
        MLACP(csm).node_id);
    return msg_len;
}

/*Prprare Sync AggPort-State TLV */
int mlacp_prepare_for_Aggport_state(struct CSM* csm, char* buf, size_t max_buf_size, struct LocalInterface* local_if)
{
    struct System* sys = NULL;
    ICCHdr* icc_hdr = (ICCHdr*)buf;
    mLACPAggPortStateTLV* tlv = (mLACPAggPortStateTLV*)&buf[sizeof(ICCHdr)];
    size_t msg_len = sizeof(ICCHdr) + sizeof(mLACPAggPortStateTLV);

    if (csm == NULL)
        return MCLAG_ERROR;

    if (buf == NULL)
        return MCLAG_ERROR;

    if (local_if == NULL)
        return MCLAG_ERROR;

    if (local_if->type != IF_T_PORT_CHANNEL)
        return MCLAG_ERROR;

    if (msg_len > max_buf_size)
        return MCLAG_ERROR;

    if ((sys = system_get_instance()) == NULL)
        return MCLAG_ERROR;

    /* Prepare for sync request */
    memset(buf, 0, max_buf_size);

    icc_hdr = (ICCHdr*)buf;
    tlv = (mLACPAggPortStateTLV*)&buf[sizeof(ICCHdr)];

    /* ICC header */
    mlacp_fill_icc_header(csm, icc_hdr, msg_len);

    /* Port State TLV */
    tlv->icc_parameter.u_bit = 0;
    tlv->icc_parameter.f_bit = 0;
    tlv->icc_parameter.type = htons(TLV_T_MLACP_AGGREGATOR_STATE);
    tlv->icc_parameter.len = htons(sizeof(mLACPAggPortStateTLV) - sizeof(ICCParameter));

    tlv->partner_sys_priority = 0;
    tlv->partner_key = 0;
    tlv->agg_id = htons(local_if->po_id);
    tlv->actor_key = 0;
    tlv->agg_state = local_if->state;

    ICCPD_LOG_DEBUG("ICCP_FSM", "TX aggrport_state: %s is %s",
        local_if->name, (local_if->state == PORT_STATE_UP) ? "up" : "down");
    return msg_len;
}

/*****************************************
* Prprare Sync Purge Port
*
* ***************************************/
int mlacp_prepare_for_Aggport_config(struct CSM* csm,
                                     char* buf, size_t max_buf_size,
                                     struct LocalInterface* lif, int purge_flag)
{
    ICCHdr* icc_hdr = (ICCHdr*)buf;
    mLACPAggConfigTLV* tlv = (mLACPAggConfigTLV*)&buf[sizeof(ICCHdr)];
    size_t msg_len = sizeof(ICCHdr) + sizeof(mLACPAggConfigTLV);

    if (csm == NULL)
        return MCLAG_ERROR;

    if (buf == NULL)
        return MCLAG_ERROR;

    if (msg_len > max_buf_size)
        return MCLAG_ERROR;

    /* Prepare for sync request */
    memset(buf, 0, max_buf_size);

    icc_hdr = (ICCHdr*)buf;
    tlv = (mLACPAggConfigTLV*)&buf[sizeof(ICCHdr)];

    /* ICC header */
    mlacp_fill_icc_header(csm, icc_hdr, msg_len);

    /* Port Config TLV */
    tlv->icc_parameter.u_bit = 0;
    tlv->icc_parameter.f_bit = 0;
    tlv->icc_parameter.type = htons(TLV_T_MLACP_AGGREGATOR_CONFIG);

    tlv->icc_parameter.len = htons(sizeof(mLACPAggConfigTLV) - sizeof(ICCParameter));
    tlv->agg_id = htons(lif->po_id);
    if (purge_flag == 1)
        tlv->flags = 0x02; /*purge*/
    else
        tlv->flags = 0x1;
    tlv->agg_name_len = strlen(lif->name);
    memcpy(tlv->agg_name, lif->name, MAX_L_PORT_NAME);
    memcpy(tlv->mac_addr, lif->mac_addr, ETHER_ADDR_LEN);

    ICCPD_LOG_DEBUG("ICCP_FSM", "TX aggrport_config: %s, purge_flag %d, mac %s",
        lif->name, purge_flag, mac_addr_to_str(lif->mac_addr));
    return msg_len;
}

/*****************************************
* Preprare Sync MAC-Info TLV
*
* ***************************************/
int mlacp_prepare_for_mac_info_to_peer(struct CSM* csm, char* buf, size_t max_buf_size, struct MACMsg* mac_msg, int count)
{
    struct mLACPMACInfoTLV* tlv = NULL;
    size_t msg_len = 0;
    size_t tlv_len = 0;
    ICCHdr* icc_hdr = NULL;
    struct mLACPMACData *MacData;

    if (!csm)
        return MCLAG_ERROR;
    if (!buf)
        return MCLAG_ERROR;

    tlv_len = sizeof(struct mLACPMACInfoTLV) + sizeof(struct mLACPMACData) * (count + 1);

    if ((msg_len = sizeof(ICCHdr) + tlv_len) > max_buf_size)
        return MCLAG_ERROR;

    /* ICC header */
    icc_hdr = (ICCHdr*)buf;
    mlacp_fill_icc_header(csm, icc_hdr, msg_len);
    /* Prepare for MAC information TLV */
    tlv = (struct mLACPMACInfoTLV*)&buf[sizeof(ICCHdr)];
    tlv->icc_parameter.len = htons(tlv_len - sizeof(ICCParameter));
    tlv->num_of_entry = htons(count + 1);

    if (count == 0)
    {
        tlv->icc_parameter.u_bit = 0;
        tlv->icc_parameter.f_bit = 0;
        tlv->icc_parameter.type = htons(TLV_T_MLACP_MAC_INFO);
    }

    MacData = (struct mLACPMACData *)&buf[sizeof(ICCHdr) + sizeof(struct mLACPMACInfoTLV) + sizeof(struct mLACPMACData) * count];
    MacData->type = mac_msg->op_type;
    MacData->mac_type = mac_msg->fdb_type;
    memcpy(MacData->mac_addr, mac_msg->mac_addr,ETHER_ADDR_LEN);
    sprintf(MacData->ifname, "%s", mac_msg->origin_ifname);
    MacData->vid = htons(mac_msg->vid);

    ICCPD_LOG_DEBUG("ICCP_FDB", "Send MAC messge to peer, port %s  mac = %s, vid = %d, type = %s count %d ", mac_msg->origin_ifname,
                                  mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid, mac_msg->op_type == MAC_SYNC_ADD ? "add" : "del", count);

    return msg_len;
}

/*****************************************
* Preprare Sync ARP-Info TLV
*
* ***************************************/
int mlacp_prepare_for_arp_info(struct CSM* csm, char* buf, size_t max_buf_size, struct ARPMsg* arp_msg, int count, int dir)
{
    struct mLACPARPInfoTLV* tlv = NULL;
    size_t msg_len = 0;
    size_t tlv_len = 0;
    ICCHdr* icc_hdr = NULL;
    struct ARPMsg* ArpData;

    if (!csm)
        return MCLAG_ERROR;
    if (!buf)
        return MCLAG_ERROR;

    tlv_len = sizeof(struct mLACPARPInfoTLV) + sizeof(struct ARPMsg) * (count + 1);

    if ((msg_len = sizeof(ICCHdr) + tlv_len) > max_buf_size)
        return MCLAG_ERROR;

    /* ICC header */
    icc_hdr = (ICCHdr*)buf;
    mlacp_fill_icc_header(csm, icc_hdr, msg_len);

    /* Prepare for ARP information TLV */
    tlv = (struct mLACPARPInfoTLV*)&buf[sizeof(ICCHdr)];
    tlv->icc_parameter.len = htons(tlv_len - sizeof(ICCParameter));
    tlv->num_of_entry = htons(count + 1);

    if (count == 0)
    {
        tlv->icc_parameter.u_bit = 0;
        tlv->icc_parameter.f_bit = 0;
        tlv->icc_parameter.type = htons(TLV_T_MLACP_ARP_INFO);
    }

    ArpData = (struct ARPMsg *)&buf[sizeof(ICCHdr) + sizeof(struct mLACPARPInfoTLV) + sizeof(struct ARPMsg) * count];

    ArpData->op_type = arp_msg->op_type;
    ArpData->flag = arp_msg->flag;
    sprintf(ArpData->ifname, "%s", arp_msg->ifname);
    ArpData->ipv4_addr = arp_msg->ipv4_addr;
    memcpy(ArpData->mac_addr, arp_msg->mac_addr, ETHER_ADDR_LEN);

    ICCPD_LOG_DEBUG(__FUNCTION__, "Send ARP messge to peer, dir %d, flag %d, if name %s mac %02x:%02x:%02x:%02x:%02x:%02x IP %s",
            dir, ArpData->flag, ArpData->ifname,
            ArpData->mac_addr[0], ArpData->mac_addr[1], ArpData->mac_addr[2], ArpData->mac_addr[3], ArpData->mac_addr[4],
            ArpData->mac_addr[5], show_ip_str(ArpData->ipv4_addr));
    return msg_len;
}

/*****************************************
* Preprare Sync NDISC-Info TLV
*
* ***************************************/
int mlacp_prepare_for_ndisc_info(struct CSM *csm, char *buf, size_t max_buf_size, struct NDISCMsg *ndisc_msg, int count, int dir)
{
    struct mLACPNDISCInfoTLV *tlv = NULL;
    size_t msg_len = 0;
    size_t tlv_len = 0;
    ICCHdr *icc_hdr = NULL;
    struct NDISCMsg *NdiscData = NULL;

    if (!csm)
        return -1;
    if (!buf)
        return -1;

    tlv_len = sizeof(struct mLACPNDISCInfoTLV) + sizeof(struct NDISCMsg) * (count + 1);

    if ((msg_len = sizeof(ICCHdr) + tlv_len) > max_buf_size)
        return -1;

    /* ICC header */
    icc_hdr = (ICCHdr *)buf;
    mlacp_fill_icc_header(csm, icc_hdr, msg_len);

    /* Prepare for ND information TLV */
    tlv = (struct mLACPNDISCInfoTLV *)&buf[sizeof(ICCHdr)];
    tlv->icc_parameter.len = htons(tlv_len - sizeof(ICCParameter));
    tlv->num_of_entry = htons(count + 1);

    if (count == 0)
    {
        tlv->icc_parameter.u_bit = 0;
        tlv->icc_parameter.f_bit = 0;
        tlv->icc_parameter.type = htons(TLV_T_MLACP_NDISC_INFO);
    }

    NdiscData = (struct NDISCMsg *)&buf[sizeof(ICCHdr) + sizeof(struct mLACPNDISCInfoTLV) + sizeof(struct NDISCMsg) * count];

    NdiscData->op_type = ndisc_msg->op_type;
    NdiscData->flag = ndisc_msg->flag;
    sprintf(NdiscData->ifname, "%s", ndisc_msg->ifname);
    memcpy(NdiscData->ipv6_addr, ndisc_msg->ipv6_addr, 16);
    memcpy(NdiscData->mac_addr, ndisc_msg->mac_addr, ETHER_ADDR_LEN);

    ICCPD_LOG_DEBUG(__FUNCTION__, "Send ND messge to peer, dir %d, flag %d, if name %s  mac  =%02x:%02x:%02x:%02x:%02x:%02x IPv6 %s",
                    dir, NdiscData->flag, NdiscData->ifname,
                    NdiscData->mac_addr[0], NdiscData->mac_addr[1], NdiscData->mac_addr[2], NdiscData->mac_addr[3], NdiscData->mac_addr[4],
                    NdiscData->mac_addr[5], show_ipv6_str((char *)NdiscData->ipv6_addr));

    return msg_len;
}

/*****************************************
* Prprare Send portchannel info
*
* ***************************************/
int mlacp_prepare_for_port_channel_info(struct CSM* csm, char* buf,
                                        size_t max_buf_size,
                                        struct LocalInterface* port_channel)
{
    struct System* sys = NULL;
    ICCHdr* icc_hdr = NULL;
    struct mLACPPortChannelInfoTLV* tlv = NULL;
    size_t msg_len;
    size_t tlv_len;
    size_t name_len = MAX_L_PORT_NAME;
    struct VLAN_ID* vlan_id = NULL;
    int num_of_vlan_id = 0;

    if (csm == NULL )
        return MCLAG_ERROR;
    if (buf == NULL )
        return MCLAG_ERROR;
    if (port_channel == NULL )
        return MCLAG_ERROR;
    if (port_channel->type == IF_T_PORT)
        return MCLAG_ERROR;
    if ((sys = system_get_instance()) == NULL )
        return MCLAG_ERROR;

    /* Calculate VLAN ID Length */
    RB_FOREACH(vlan_id, vlan_rb_tree, &(port_channel->vlan_tree))
        if (vlan_id != NULL)
            num_of_vlan_id++;

    tlv_len = sizeof(struct mLACPPortChannelInfoTLV) + sizeof(struct mLACPVLANData) * num_of_vlan_id;

    if ((msg_len = sizeof(ICCHdr) + tlv_len) > max_buf_size)
        return MCLAG_ERROR;

    /* Prepare for port channel info */
    memset(buf, 0, max_buf_size);

    icc_hdr = (ICCHdr*)buf;
    tlv = (struct mLACPPortChannelInfoTLV*)&buf[sizeof(ICCHdr)];

    /* ICC header */
    mlacp_fill_icc_header(csm, icc_hdr, msg_len);

    /* Port Channel Info TLV */
    tlv->icc_parameter.u_bit = 0;
    tlv->icc_parameter.f_bit = 0;
    tlv->icc_parameter.type = htons(TLV_T_MLACP_PORT_CHANNEL_INFO);
    tlv->icc_parameter.len = htons(sizeof(struct mLACPPortChannelInfoTLV) - sizeof(ICCParameter) + sizeof(struct mLACPVLANData) * num_of_vlan_id);
    tlv->agg_id = htons(port_channel->po_id);
    tlv->ipv4_addr = htonl(port_channel->ipv4_addr);
    tlv->l3_mode = port_channel->l3_mode;
    tlv->po_id = htons(port_channel->po_id);

    if (strlen(port_channel->name) < name_len)
        name_len = strlen(port_channel->name);
    memcpy(tlv->if_name, port_channel->name, name_len);
    tlv->if_name_len = name_len;
    tlv->num_of_vlan_id = htons(num_of_vlan_id);

    num_of_vlan_id = 0;
    RB_FOREACH(vlan_id, vlan_rb_tree, &(port_channel->vlan_tree))
    {
        if (vlan_id != NULL )
        {
            tlv->vlanData[num_of_vlan_id].vlan_id = htons(vlan_id->vid);

            num_of_vlan_id++;
            ICCPD_LOG_DEBUG(__FUNCTION__, "PortChannel%d: ipv4 addr = %s vlan id %d num %d ", port_channel->po_id, show_ip_str( tlv->ipv4_addr), vlan_id->vid, num_of_vlan_id );
        }
    }

    ICCPD_LOG_DEBUG(__FUNCTION__, "PortChannel%d: ipv4 addr = %s  l3 mode %d", port_channel->po_id, show_ip_str( tlv->ipv4_addr),  tlv->l3_mode);
    ICCPD_LOG_DEBUG("ICCP_FSM", "TX po_info: %s has %d vlans", port_channel->name, num_of_vlan_id);
    return msg_len;
}

/*****************************************
* Prprare Send port peerlink  info
*
* ***************************************/
int mlacp_prepare_for_port_peerlink_info(struct CSM* csm, char* buf,
                                         size_t max_buf_size,
                                         struct LocalInterface* peerlink_port)
{
    struct System* sys = NULL;
    ICCHdr* icc_hdr = NULL;
    struct mLACPPeerLinkInfoTLV* tlv = NULL;
    size_t msg_len;
    size_t tlv_len;

    if (csm == NULL )
        return MCLAG_ERROR;
    if (buf == NULL )
        return MCLAG_ERROR;
    if (peerlink_port == NULL )
        return MCLAG_ERROR;
    if ((sys = system_get_instance()) == NULL )
        return MCLAG_ERROR;

    /* Prepare for port channel info */
    memset(buf, 0, max_buf_size);

    tlv_len = sizeof(struct mLACPPeerLinkInfoTLV);

    if ((msg_len = sizeof(ICCHdr) + tlv_len) > max_buf_size)
        return MCLAG_ERROR;

    icc_hdr = (ICCHdr*)buf;
    tlv = (struct mLACPPeerLinkInfoTLV*)&buf[sizeof(ICCHdr)];

    /* ICC header */
    mlacp_fill_icc_header(csm, icc_hdr, msg_len);

    /* Port Channel Info TLV */
    tlv->icc_parameter.u_bit = 0;
    tlv->icc_parameter.f_bit = 0;
    tlv->icc_parameter.type = htons(TLV_T_MLACP_PEERLINK_INFO);

    tlv->icc_parameter.len = htons(tlv_len - sizeof(ICCParameter));
    memcpy(tlv->if_name, peerlink_port->name, MAX_L_PORT_NAME);
    tlv->port_type = peerlink_port->type;

    ICCPD_LOG_DEBUG("ICCP_FSM", "TX peerlink_info: name %s, type %d",
        peerlink_port->name, peerlink_port->type);
    return msg_len;
}


/*****************************************
* Prprare Send Heartbeat
*
* ***************************************/
int mlacp_prepare_for_heartbeat(struct CSM* csm, char* buf, size_t max_buf_size)
{
    struct System* sys = NULL;
    ICCHdr* icc_hdr = (ICCHdr*)buf;
    struct mLACPHeartbeatTLV* tlv = (struct mLACPHeartbeatTLV*)&buf[sizeof(ICCHdr)];
    size_t msg_len = sizeof(ICCHdr) + sizeof(struct mLACPHeartbeatTLV);

    if (csm == NULL)
        return MCLAG_ERROR;

    if (buf == NULL)
        return MCLAG_ERROR;

    if (msg_len > max_buf_size)
        return MCLAG_ERROR;

    if ((sys = system_get_instance()) == NULL)
        return MCLAG_ERROR;

    /* Prepare for sync request */
    memset(buf, 0, max_buf_size);

    icc_hdr = (ICCHdr*)buf;
    tlv = (struct mLACPHeartbeatTLV*)&buf[sizeof(ICCHdr)];

    /* ICC header */
    mlacp_fill_icc_header(csm, icc_hdr, msg_len);

    /* System Config TLV */
    tlv->icc_parameter.u_bit = 0;
    tlv->icc_parameter.f_bit = 0;
    tlv->icc_parameter.type = htons(TLV_T_MLACP_HEARTBEAT);

    tlv->icc_parameter.len = htons(sizeof(struct mLACPHeartbeatTLV) - sizeof(ICCParameter));
    tlv->heartbeat = 0xFF;
    return msg_len;
}

/*****************************************
* Prepare Send warm-reboot flag
*
* ***************************************/
int mlacp_prepare_for_warm_reboot(struct CSM* csm, char* buf, size_t max_buf_size)
{
    struct System* sys = NULL;
    ICCHdr* icc_hdr = (ICCHdr*)buf;
    struct mLACPWarmbootTLV* tlv = (struct mLACPWarmbootTLV*)&buf[sizeof(ICCHdr)];
    size_t msg_len = sizeof(ICCHdr) + sizeof(struct mLACPWarmbootTLV);

    if (csm == NULL)
        return MCLAG_ERROR;

    if (buf == NULL)
        return MCLAG_ERROR;

    if (msg_len > max_buf_size)
        return MCLAG_ERROR;

    if ((sys = system_get_instance()) == NULL)
        return MCLAG_ERROR;

    /* Prepare for sync request */
    memset(buf, 0, max_buf_size);

    icc_hdr = (ICCHdr*)buf;
    tlv = (struct mLACPWarmbootTLV*)&buf[sizeof(ICCHdr)];

    /* ICC header */
    mlacp_fill_icc_header(csm, icc_hdr, msg_len);

    /* System Config TLV */
    tlv->icc_parameter.u_bit = 0;
    tlv->icc_parameter.f_bit = 0;
    tlv->icc_parameter.type = htons(TLV_T_MLACP_WARMBOOT_FLAG);

    tlv->icc_parameter.len = htons(sizeof(struct mLACPWarmbootTLV) - sizeof(ICCParameter));
    tlv->warmboot = 0x1;
    ICCPD_LOG_DEBUG("ICCP_FSM", "TX start warm reboot");
    return msg_len;
}

/*****************************************
* Prepare interface up ACK message
*
* ***************************************/
int mlacp_prepare_for_if_up_ack(
    struct CSM        *csm,
    char              *buf,
    size_t            max_buf_size,
    uint8_t           if_type,
    uint16_t          if_id,
    uint8_t           port_isolation_state)
{
    struct System* sys = NULL;
    ICCHdr* icc_hdr = (ICCHdr*) buf;
    struct mLACPIfUpAckTLV* tlv = (struct mLACPIfUpAckTLV*) &buf[sizeof(ICCHdr)];
    size_t msg_len = sizeof(ICCHdr) + sizeof(struct mLACPIfUpAckTLV);

    if(csm == NULL)
        return -1;

    if(buf == NULL)
        return -1;

    if(msg_len > max_buf_size)
        return -1;

    if((sys = system_get_instance()) == NULL)
        return -1;

    /* Prepare for sync request */
    memset(buf, 0, max_buf_size);

    icc_hdr = (ICCHdr*) buf;
    tlv = (struct mLACPIfUpAckTLV*) &buf[sizeof(ICCHdr)];

    /* ICC header */
    mlacp_fill_icc_header(csm, icc_hdr, msg_len);

    /* If up ack TLV */
    tlv->icc_parameter.u_bit = 0;
    tlv->icc_parameter.f_bit = 0;
    tlv->icc_parameter.type = htons(TLV_T_MLACP_IF_UP_ACK);

    tlv->icc_parameter.len = htons(sizeof(struct mLACPIfUpAckTLV) - sizeof(ICCParameter));
    tlv->if_type = if_type;
    tlv->if_id = htons(if_id);
    tlv->port_isolation_state = port_isolation_state;

    ICCPD_LOG_DEBUG("ICCP_FSM", "TX if_up_ack: PortChannel%d, isolation_set %d",
        if_id, port_isolation_state);
    return msg_len;
}

/*****************************************
* Tool : Prepare ICC Header
*
* ***************************************/
static int mlacp_fill_icc_header(struct CSM* csm, ICCHdr* icc_hdr, size_t msg_len)
{
    if (csm == NULL || icc_hdr == NULL)
        return MCLAG_ERROR;

    /* ICC header */
    icc_hdr->ldp_hdr.u_bit = 0x0;
    icc_hdr->ldp_hdr.msg_type = htons(MSG_T_RG_APP_DATA);

    icc_hdr->ldp_hdr.msg_len = htons(msg_len - MSG_L_INCLUD_U_BIT_MSG_T_L_FIELDS);
    icc_hdr->ldp_hdr.msg_id = htonl(ICCP_MSG_ID);
    ICCP_MSG_ID++;
    iccp_csm_fill_icc_rg_id_tlv(csm, icc_hdr);

    return 0;
}

