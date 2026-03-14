/********************************************************************************
 * mlacp_sync_update.c
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
 *******************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include <sys/queue.h>
#include <netinet/ip.h>

#include "../include/system.h"
#include "../include/logger.h"
#include "../include/mlacp_tlv.h"
#include "../include/iccp_csm.h"
#include "../include/mlacp_link_handler.h"
#include "../include/iccp_netlink.h"
#include "../include/iccp_consistency_check.h"
#include "../include/port.h"
#include "../include/openbsd_tree.h"

/*****************************************
* Port-Conf Update
*
* ***************************************/

int mlacp_fsm_update_system_conf(struct CSM* csm, mLACPSysConfigTLV*sysconf)
{
    struct LocalInterface* lif = NULL;
    uint8_t old_remote_system_id[ETHER_ADDR_LEN];

    ICCPD_LOG_NOTICE("ICCP_FSM", "RX system_conf: systemID %s, priority %d, remote nodeID %d, nodeID %d",
        mac_addr_to_str(MLACP(csm).remote_system.system_id),
        MLACP(csm).remote_system.system_priority,
        MLACP(csm).remote_system.node_id,
        MLACP(csm).node_id);

    /*NOTE
       a little tricky, we change the NodeID local side if collision happened first time*/
    if (sysconf->node_id == MLACP(csm).node_id)
        MLACP(csm).node_id++;

    memcpy(old_remote_system_id, MLACP(csm).remote_system.system_id, ETHER_ADDR_LEN);

    memcpy(MLACP(csm).remote_system.system_id, sysconf->sys_id, ETHER_ADDR_LEN);
    MLACP(csm).remote_system.system_priority = ntohs(sysconf->sys_priority);
    MLACP(csm).remote_system.node_id = sysconf->node_id;

    LIST_FOREACH(lif, &(MLACP(csm).lif_list), mlacp_next)
    {
        update_if_ipmac_on_standby(lif, 2);
    }

    /* On standby, update system ID upon receiving change from active */
    if ((csm->role_type == STP_ROLE_STANDBY) &&
        (memcmp(old_remote_system_id, sysconf->sys_id, ETHER_ADDR_LEN) != 0))
    {
        mlacp_link_set_iccp_system_id(csm->mlag_id, sysconf->sys_id);
    }

    return 0;
}

/*****************************************
* Port-Conf Update
*
* ***************************************/
int mlacp_fsm_update_Agg_conf(struct CSM* csm, mLACPAggConfigTLV* portconf)
{
    struct PeerInterface* pif = NULL;
    uint8_t po_active;
    uint8_t new_create = 0;
    struct LocalInterface *lif;

    if (!csm)
        return MCLAG_ERROR;

    ICCPD_LOG_DEBUG("ICCP_FSM", "RX aggrport_config: name %s, po_id %d, flag 0x%x, MAC %s",
        portconf->agg_name, ntohs(portconf->agg_id), portconf->flags,
        mac_addr_to_str(portconf->mac_addr));

    /* Looking for the peer port instance, is any peer if exist?*/
    pif = peer_if_find_by_name(csm, portconf->agg_name);

    /* Process purge*/
    if (portconf->flags & 0x02)
    {
        /*Purge*/
        if (pif != NULL )
        {
            //This handler would take of handling mlacp changes based on peer
            //mclag interface delete; recover back mac of po to original
            //on standby etc
            mlacp_peer_mlag_intf_delete_handler(csm, pif->name);

            /* Delete remote interface info from STATE_DB */
            mlacp_link_del_remote_if_info(csm->mlag_id, pif->name);
            peer_if_destroy(pif);
        }
        else
            MLACP(csm).need_to_sync = 1;
        /*ICCPD_LOG_INFO("mlacp_fsm",
            "    Peer port %s is removed from port-channel member.",portconf->port_name);*/

        return 0;
    }

    if (pif == NULL && portconf->flags & 0x01)
    {
        pif = peer_if_create(csm, ntohs(portconf->agg_id), IF_T_PORT_CHANNEL);
        if (pif == NULL)
            return MCLAG_ERROR;

        new_create = 1;
    }

    pif->po_id = ntohs(portconf->agg_id);
    memcpy(pif->name, portconf->agg_name, portconf->agg_name_len);
    memcpy(pif->mac_addr, portconf->mac_addr, ETHER_ADDR_LEN);

    po_active = (pif->state == PORT_STATE_UP);
    update_stp_peer_link(csm, pif, po_active, new_create);
    update_peerlink_isolate_from_pif(csm, pif, po_active, new_create);
    pif->po_active = po_active;

    /* When peer MLAG interface does not exist on active, standby does not set
     * its MLAG interface MAC address to the active system MAC address.
     * Peer MLAG interface is added now, update the standby MLAG interface
     * MAC address
     */
    if (new_create)
    {
        mlacp_link_set_remote_if_state(csm->mlag_id, pif->name, (po_active)? true : false);
        if ((csm->role_type == STP_ROLE_STANDBY))
        {
            lif = local_if_find_by_name(portconf->agg_name);
            if (lif)
                update_if_ipmac_on_standby(lif, 3);
        }
    }
    return 0;
}

/*****************************************
* Agg Port-State Update
*
* ***************************************/
int mlacp_fsm_update_Aggport_state(struct CSM* csm, mLACPAggPortStateTLV* tlv)
{
    struct PeerInterface* peer_if = NULL;
    uint8_t po_active;

    if (csm == NULL || tlv == NULL)
        return MCLAG_ERROR;

    ICCPD_LOG_DEBUG("ICCP_FSM", "RX aggrport_state: po_id %d, state %s, sync_state %s",
        ntohs(tlv->agg_id), (tlv->agg_state == PORT_STATE_UP) ? "up" : "down",
        mlacp_state(csm));

    po_active = (tlv->agg_state == PORT_STATE_UP);

    LIST_FOREACH(peer_if, &(MLACP(csm).pif_list), mlacp_next)
    {
        if (peer_if->type != IF_T_PORT_CHANNEL)
            continue;

        if (peer_if->po_id != ntohs(tlv->agg_id))
            continue;

        peer_if->state = tlv->agg_state;

        update_stp_peer_link(csm, peer_if, po_active, 0);
        update_peerlink_isolate_from_pif(csm, peer_if, po_active, 0);

        peer_if->po_active = po_active;
        ICCPD_LOG_DEBUG(__FUNCTION__, "Update peer interface %s to state %s", peer_if->name, tlv->agg_state ? "down" : "up");

        /* Update remote interface state if ICCP reaches EXCHANGE state.
         * Otherwise, it is updated after the session comes up
         */
        if (MLACP(csm).current_state == MLACP_STATE_EXCHANGE)
        {
            if (tlv->agg_state == PORT_STATE_DOWN)
                mlacp_convert_remote_mac_to_local(csm, peer_if->name);

            mlacp_link_set_remote_if_state(
                csm->mlag_id, peer_if->name,
                (tlv->agg_state == PORT_STATE_UP)? true : false);
        }
        break;
    }

    return 0;
}

/*****************************************
* Recv from peer, MAC-Info Update
* ***************************************/
int mlacp_fsm_update_mac_entry_from_peer( struct CSM* csm, struct mLACPMACData *MacData)
{
    struct Msg* msg = NULL;
    struct MACMsg *mac_msg = NULL, *new_mac_msg = NULL;
    struct MACMsg mac_data, mac_find;
    struct LocalInterface* local_if = NULL;
    uint8_t from_mclag_intf = 0;/*0: orphan port, 1: MCLAG port*/
    memset(&mac_data, 0, sizeof(struct MACMsg));
    memset(&mac_find, 0, sizeof(struct MACMsg));
    uint8_t null_mac[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    ICCPD_LOG_INFO("ICCP_FDB",
        "Received MAC Info, interface=[%s] vid[%d] MAC[%s] OperType[%s] MacType[%d] ",
        MacData->ifname, ntohs(MacData->vid), mac_addr_to_str(MacData->mac_addr),
        MacData->type == MAC_SYNC_ADD ? "add" : "del", MacData->mac_type);

    if (memcmp(MacData->mac_addr, null_mac, ETHER_ADDR_LEN) == 0)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Invalid MAC address from peer do not add.");
        return 0;
    }


    /*Find the interface in MCLAG interface list*/
    LIST_FOREACH(local_if, &(MLACP(csm).lif_list), mlacp_next)
    {
        if (local_if->type == IF_T_PORT_CHANNEL && strcmp(local_if->name, MacData->ifname) == 0)
        {
            from_mclag_intf = 1;
            break;
        }
    }

    mac_find.vid = ntohs(MacData->vid);
    memcpy(&mac_find.mac_addr, MacData->mac_addr, ETHER_ADDR_LEN);
    mac_msg = RB_FIND(mac_rb_tree, &MLACP(csm).mac_rb ,&mac_find);

    /*Same MAC is exist in local switch, this may be mac move*/
    //if (strcmp(mac_msg->mac_str, MacData->mac_str) == 0 && mac_msg->vid == ntohs(MacData->vid))
    if (mac_msg)
    {
        ICCPD_LOG_DEBUG("ICCP_FDB", "Recv MAC update from peer RB_FIND success, existing MAC age flag:%d interface %s, "
            "MAC %s vlan-id %d, fdb_type: %d, op_type %s", mac_msg->age_flag, mac_msg->ifname,
            mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid, mac_msg->fdb_type,
            (mac_msg->op_type == MAC_SYNC_ADD) ? "add":"del");

        if (MacData->type == MAC_SYNC_ADD)
        {
            mac_msg->age_flag &= ~MAC_AGE_PEER;

            if (from_mclag_intf && mac_msg->pending_local_del)
            {
                mac_msg->pending_local_del = 0;

                mac_msg->age_flag = MAC_AGE_LOCAL;

                // AS MAC was learned locally and in pending state due to IF is down,
                // send del to peer when MAC add received from peer
                mac_msg->op_type = MAC_SYNC_DEL;
                if (!MAC_IN_MSG_LIST(&(MLACP(csm).mac_msg_list), mac_msg, tail))
                {
                    TAILQ_INSERT_TAIL(&(MLACP(csm).mac_msg_list), mac_msg, tail);
                }
            }

            ICCPD_LOG_DEBUG("ICCP_FDB", "Recv ADD, Remove peer age flag:%d interface %s, "
                "MAC %s vlan-id %d, op_type %s, from_mclag_intf: %d ", mac_msg->age_flag, mac_msg->ifname,
                mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid,
                (mac_msg->op_type == MAC_SYNC_ADD) ? "add":"del", from_mclag_intf);

            /*mac_msg->fdb_type = tlv->fdb_type;*/
            /*The port ifname is different to the local item*/
            if (strcmp(mac_msg->ifname, MacData->ifname) != 0 || strcmp(mac_msg->origin_ifname, MacData->ifname) != 0)
            {
                if (mac_msg->fdb_type != MAC_TYPE_STATIC)
                {
                    /*Update local item*/
                    memcpy(&mac_msg->origin_ifname, MacData->ifname, MAX_L_PORT_NAME);
                }
                else
                {
                    ICCPD_LOG_DEBUG("ICCP_FDB", "Ignore Recv MAC ADD, Local static present,"
                        " interface  %s, MAC %s vlan-id %d ", mac_msg->ifname,
                        mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid);
                    //set back the peer age flag
                    mac_msg->age_flag |= MAC_AGE_PEER;
                    return 0;
                }

                /*If the MAC is learned from orphan port, or from MCLAG port but the local port is down*/
                if (from_mclag_intf == 0 || local_if->state == PORT_STATE_DOWN )
                {
                    /*Set MAC_AGE_LOCAL flag*/
                    mac_msg->age_flag = set_mac_local_age_flag(csm, mac_msg, 1, 1);

                    if (strlen(csm->peer_itf_name) != 0)
                    {
                        if (strcmp(mac_msg->ifname, csm->peer_itf_name) == 0)
                        {
                            /*This MAC is already point to peer-link*/
                            ICCPD_LOG_NOTICE("ICCP_FDB", "Remote MAC ADD local IF down, MAC already points to Peer_link done processing "
                                " interface  %s, MAC %s vlan-id %d ", mac_msg->ifname,
                                mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid);
                            return 0;
                        }

                        if (csm->peer_link_if && (csm->peer_link_if->state == PORT_STATE_UP))
                        {
                            /*Redirect the mac to peer-link*/
                            memcpy(&mac_msg->ifname, csm->peer_itf_name, MAX_L_PORT_NAME);

                            /*Send mac add message to mclagsyncd*/
                            add_mac_to_chip(mac_msg, mac_msg->fdb_type);

                            ICCPD_LOG_DEBUG("ICCP_FDB", "Remote MAC ADD , local mac exist move to peer link up "
                                " interface  %s, MAC %s vlan-id %d ", mac_msg->ifname,
                                mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid);

                        }
                        else
                        {
                            /*Redirect the mac to peer-link, if peerlink is down FdbOrch deletes MAC*/
                            memcpy(&mac_msg->ifname, csm->peer_itf_name, MAX_L_PORT_NAME);

                            add_mac_to_chip(mac_msg, mac_msg->fdb_type);

                            ICCPD_LOG_DEBUG("ICCP_FDB", "Remote MAC ADD , local mac exist move to peer link down "
                                    " interface  %s, MAC %s vlan-id %d ", mac_msg->ifname,
                                    mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid);
                        }
                    }
                    else
                    {
                        /*must redirect but no peerlink, del mac from ASIC*/
                        del_mac_from_chip(mac_msg);

                        /*Update local item*/
                        memcpy(&mac_msg->ifname, MacData->ifname, MAX_L_PORT_NAME);

                        /*if orphan port mac but no peerlink, don't keep this mac*/
                        if (from_mclag_intf == 0)
                        {
                            MAC_RB_REMOVE(mac_rb_tree, &MLACP(csm).mac_rb, mac_msg);

                            // free only if not in change list to be send to peer node,
                            // else free is taken care after sending the update to peer
                            if (!MAC_IN_MSG_LIST(&(MLACP(csm).mac_msg_list), mac_msg, tail))
                            {
                                free(mac_msg);
                            }

                            ICCPD_LOG_ERR(__FUNCTION__, "Ignore Recv MAC ADD "
                                "MAC %s vlan %d interface %s peer link not available ",
                                mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid, mac_msg->ifname);
                            return 0;
                        }
                    }
                }
                else
                {
                    /*Update local item*/
                    memcpy(&mac_msg->ifname, MacData->ifname, MAX_L_PORT_NAME);

                    /*from MCLAG port and the local port is up, add mac to ASIC to update port*/
                    add_mac_to_chip(mac_msg, mac_msg->fdb_type);
                }
            }
	    else if(!from_mclag_intf && (strcmp(mac_msg->ifname, MacData->ifname) == 0))
            {
                // local to remote MAC move on Orphan port.
                if (strlen(csm->peer_itf_name) != 0)
                {
                    if (strcmp(mac_msg->ifname, csm->peer_itf_name) == 0)
                    {
                        /*This MAC is already point to peer-link*/
                        ICCPD_LOG_DEBUG("ICCP_FDB", "Remote MAC ADD learn on Orphan port ,MAC already points to Peer_link"
                            " interface  %s, MAC %s vlan-id %d ", mac_msg->ifname,
                            mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid);
                        return 0;
                    }

                    if (csm->peer_link_if && csm->peer_link_if->state == PORT_STATE_UP)
                    {
                        /*Redirect the mac to peer-link*/
                        memcpy(&mac_msg->ifname, csm->peer_itf_name, MAX_L_PORT_NAME);

                        ICCPD_LOG_DEBUG("ICCP_FDB", "Remote MAC ADD learn on Orphan port ,point MAC address to Peer_link"
                            "interface  %s, MAC %s vlan-id %d ", mac_msg->ifname,
                            mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid);
                        /*Send mac add message to mclagsyncd*/
                        add_mac_to_chip(mac_msg, mac_msg->fdb_type);
                    }
                    else
                    {
                        /*Redirect the mac to peer-link*/
                         /*must redirect but if peerlink is down FdbOrch will delete MAC */
                        memcpy(&mac_msg->ifname, csm->peer_itf_name, MAX_L_PORT_NAME);
                        add_mac_to_chip(mac_msg, mac_msg->fdb_type);

                        ICCPD_LOG_DEBUG("ICCP_FDB", "Remote MAC ADD learn on Orphan port ,point MAC address to Peer_link"
                            " peer link is down, delete the MAC, interface  %s, MAC %s vlan-id %d ", mac_msg->ifname,
                            mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid);
                    }
                }
            }

            // Code to exchange MAC_SYNC_ACK notifications can be enabled in future, if MAC SYNC issues observed.
            #if 0
            mac_msg->op_type = MAC_SYNC_ACK;
            if (iccp_csm_init_msg(&msg_send, (char*)mac_msg, sizeof(struct MACMsg)) == 0)
            {
                /*Reply mac ack message to peer, peer will clean MAC_AGE_PEER flag*/
                TAILQ_INSERT_TAIL(&(MLACP(csm).mac_msg_list), msg_send, tail);
                ICCPD_LOG_DEBUG(__FUNCTION__, "Recv ADD, MAC-msg-list enqueue: %s, "
                    "add %s vlan-id %d, op_type %d", mac_msg->ifname,
                    mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid, mac_msg->op_type);
            }
            #endif
        }
        // Code to exchange MAC_SYNC_ACK notifications can be enabled in future, if MAC SYNC issues observed.
        #if 0
        else if (tlv->type == MAC_SYNC_ACK)
        {
            /*Clean the MAC_AGE_PEER flag*/
            mac_msg->age_flag &= ~MAC_AGE_PEER;
            ICCPD_LOG_DEBUG(__FUNCTION__, "Recv ACK, Remove peer age flag:%d ifname  %s, "
                "add %s vlan-id %d, op_type %d", mac_msg->age_flag, mac_msg->ifname,
                mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid, mac_msg->op_type);
        }
        #endif

    }

    /* delete/add MAC list*/
    if (mac_msg && (MacData->type == MAC_SYNC_DEL))
    {
        mac_msg->age_flag |= MAC_AGE_PEER;
        ICCPD_LOG_DEBUG("ICCP_FDB", "Recv MAC DEL from peer: Add peer age flag: %d interface %s, "
            "MAC %s vlan %d, op_type %s", mac_msg->age_flag, mac_msg->ifname,
            mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid,
            (mac_msg->op_type == MAC_SYNC_ADD) ? "add":"del");

        if (mac_msg->age_flag == (MAC_AGE_LOCAL | MAC_AGE_PEER))
        {
            /*send mac del message to mclagsyncd.*/
            del_mac_from_chip(mac_msg);

            /*If local and peer both aged, del the mac*/
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
            return 0;
        }
    }
    else if (!mac_msg && (MacData->type == MAC_SYNC_ADD))
    {
        mac_msg = (struct MACMsg*)&mac_data;
        mac_msg->fdb_type = MacData->mac_type;
        mac_msg->vid = ntohs(MacData->vid);
        memcpy(mac_msg->mac_addr, MacData->mac_addr, ETHER_ADDR_LEN);
        sprintf(mac_msg->ifname, "%s", MacData->ifname);
        sprintf(mac_msg->origin_ifname, "%s", MacData->ifname);
        mac_msg->age_flag = 0;

        /*Set MAC_AGE_LOCAL flag*/
        mac_msg->age_flag = set_mac_local_age_flag(csm, mac_msg, 1, 0);

        /*If the MAC is learned from orphan port, or from MCLAG port but the local port is down*/
        if (from_mclag_intf == 0 || local_if->state == PORT_STATE_DOWN)
        {

            if (strlen(csm->peer_itf_name) == 0)
            {
                /*if orphan port mac but no peerlink, don't keep this mac*/
                //MAC to be saved and program when peer_link is configured..? TBD
                if (from_mclag_intf == 0)
                {
                    ICCPD_LOG_DEBUG("ICCP_FDB", "Recv MAC ADD from peer: Ignore MAC learn on orphan port "
                        "peer-link is not configured interface %s, MAC %s vlan-id %d, "
                        " op_type %d", from_mclag_intf, mac_msg->ifname,
                        mac_addr_to_str(mac_msg->mac_addr),
                        mac_msg->vid, mac_msg->op_type);
                    return 0;
                }
            }
            else
            {
                /*Redirect the mac to peer-link*/
                memcpy(&mac_msg->ifname, csm->peer_itf_name, MAX_L_PORT_NAME);

                ICCPD_LOG_DEBUG("ICCP_FDB", "Recv MAC ADD from peer: Redirect to peerlink for orphan port or portchannel is down,"
                    " age flag: %d interface %s, MAC %s vlan %d, op_type %d",
                    mac_msg->age_flag, mac_msg->ifname, mac_addr_to_str(mac_msg->mac_addr),
                    mac_msg->vid, mac_msg->op_type);
            }
        }

        if (iccp_csm_init_mac_msg(&new_mac_msg, (char*)mac_msg, sizeof(struct MACMsg)) == 0)
        {
            /*ICCPD_LOG_INFO(__FUNCTION__, "add mac queue successfully");*/
            RB_INSERT(mac_rb_tree, &MLACP(csm).mac_rb, new_mac_msg);

            /*If the mac is from orphan port, or from MCLAG port but the local port is down*/
            if (strcmp(mac_msg->ifname, csm->peer_itf_name) == 0)
            {
                /*Send mac add message to mclagsyncd*/
                if (csm->peer_link_if && csm->peer_link_if->state == PORT_STATE_UP)
                    add_mac_to_chip(mac_msg, mac_msg->fdb_type);
            }
            else if(local_if->state != PORT_STATE_DOWN)
            {
                /*from MCLAG port and the local port is up*/
                add_mac_to_chip(mac_msg, mac_msg->fdb_type);
            }
            // Code to exchange MAC_SYNC_ACK notifications can be enabled in future, if MAC SYNC issues observed.
            #if 0
            mac_msg->op_type = MAC_SYNC_ACK;
            if (iccp_csm_init_msg(&msg_send, (char*)mac_msg, sizeof(struct MACMsg)) == 0)
            {
                /*Reply mac ack message to peer, peer will clean MAC_AGE_PEER flag*/
                TAILQ_INSERT_TAIL(&(MLACP(csm).mac_msg_list), msg_send, tail);
                ICCPD_LOG_DEBUG(__FUNCTION__, "MAC-msg-list enqueue: %s, add %s vlan-id %d, op_type %d",
                    mac_msg->ifname, mac_addr_to_str(mac_msg->mac_addr), mac_msg->vid, mac_msg->op_type);
            }
            #endif
        }
    }

    return 0;
}

int mlacp_fsm_update_mac_info_from_peer(struct CSM* csm, struct mLACPMACInfoTLV* tlv)
{
    int count = 0;
    int i;

    if (!csm || !tlv)
        return MCLAG_ERROR;
    count = ntohs(tlv->num_of_entry);
    ICCPD_LOG_INFO(__FUNCTION__, "Received MAC Info count  %d ", count );

    for (i = 0; i < count; i++)
    {
        mlacp_fsm_update_mac_entry_from_peer(csm, &(tlv->MacEntry[i]));
    }
}

/*****************************************
 * Tool : Add ARP Info into ARP list
 *
 ****************************************/
void mlacp_enqueue_arp(struct CSM* csm, struct Msg* msg)
{
    struct ARPMsg *arp_msg = NULL;

    if (!csm)
    {
        if (msg)
            free(msg);
        return;
    }
    if (!msg)
        return;

    arp_msg = (struct ARPMsg*)msg->buf;
    if (arp_msg->op_type != NEIGH_SYNC_DEL)
    {
        TAILQ_INSERT_TAIL(&(MLACP(csm).arp_list), msg, tail);
    }

    return;
}

/*****************************************
 * Tool : Add Ndisc Info into ndisc list
 *
 ****************************************/
void mlacp_enqueue_ndisc(struct CSM *csm, struct Msg *msg)
{
    struct NDISCMsg *ndisc_msg = NULL;

    if (!csm)
    {
        if (msg)
            free(msg);
        return;
    }
    if (!msg)
        return;

    ndisc_msg = (struct NDISCMsg *)msg->buf;
    if (ndisc_msg->op_type != NEIGH_SYNC_DEL)
    {
        TAILQ_INSERT_TAIL(&(MLACP(csm).ndisc_list), msg, tail);
    }

    return;
}

/*****************************************
* ARP-Info Update
* ***************************************/
int mlacp_fsm_update_arp_entry(struct CSM* csm, struct ARPMsg *arp_entry)
{
    struct Msg* msg = NULL;
    struct ARPMsg *arp_msg = NULL, arp_data;
    struct LocalInterface *local_if = NULL;
    struct LocalInterface *vlan_if = NULL;
    struct LocalInterface *peer_link_if = NULL;
    struct LocalInterface *local_vlan_if = NULL;
    struct VLAN_ID *vlan_id_list = NULL;
    int set_arp_flag = 0;
    int my_ip_arp_flag = 0;
    int vlan_count = 0;
    char mac_str[18] = "";
    int err = 0, ln = 0;
    int permanent_neigh = 0;
    uint16_t vlan_id = 0;
    struct VLAN_ID vlan_key = { 0 };
    int vid_intf_present = 0;

    if (!csm || !arp_entry)
        return MCLAG_ERROR;

    #if 0
    ICCPD_LOG_INFO(__FUNCTION__,
                   "Received ARP Info, intf[%s] IP[%s], MAC[%02x:%02x:%02x:%02x:%02x:%02x]",
                   arp_entry->ifname, show_ip_str(arp_entry->ipv4_addr),
                   arp_entry->mac_addr[0], arp_entry->mac_addr[1], arp_entry->mac_addr[2],
                   arp_entry->mac_addr[3], arp_entry->mac_addr[4], arp_entry->mac_addr[5]);
    #endif

    sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", arp_entry->mac_addr[0], arp_entry->mac_addr[1], arp_entry->mac_addr[2],
            arp_entry->mac_addr[3], arp_entry->mac_addr[4], arp_entry->mac_addr[5]);

    ICCPD_LOG_DEBUG(__FUNCTION__, "Received ARP Info, Flag %x, intf[%s] IP[%s], MAC[%s]", arp_entry->flag, arp_entry->ifname,
            show_ip_str(arp_entry->ipv4_addr), mac_str);

    if (strncmp(arp_entry->ifname, VLAN_PREFIX, strlen(VLAN_PREFIX)) == 0) {
        sscanf (arp_entry->ifname, "Vlan%hu", &vlan_id);
    }

    if (vlan_id)
    {
        memset(&vlan_key, 0, sizeof(struct VLAN_ID));
        vlan_key.vid = vlan_id;

        peer_link_if = local_if_find_by_name(csm->peer_itf_name);

        if (peer_link_if && !local_if_is_l3_mode(peer_link_if))
        {
            ln = __LINE__;
            /* Is peer-linlk itf belong to a vlan the same as peer?*/
            vlan_id_list = RB_FIND(vlan_rb_tree, &(peer_link_if->vlan_tree), &vlan_key);

            if (vlan_id_list)
            {
                vlan_count++;
                if (vlan_id_list->vlan_itf) {
                    if (strcmp(vlan_id_list->vlan_itf->name, arp_entry->ifname) == 0) {
                        ln = __LINE__;
                        vid_intf_present = 1;
                    }

                    if (vid_intf_present && local_if_is_l3_mode(vlan_id_list->vlan_itf)) {
                        if (arp_entry->ipv4_addr == vlan_id_list->vlan_itf->ipv4_addr) {
                            my_ip_arp_flag = 1;
                        }
                    }

                    ICCPD_LOG_DEBUG(__FUNCTION__,
                            "ARP is learnt from intf %s, peer-link %s is the member of this vlan",
                            vlan_id_list->vlan_itf->name, peer_link_if->name);

                    /* Peer-link belong to L3 vlan is alive, set the ARP info*/
                    set_arp_flag = 1;
                }
            }

            ICCPD_LOG_DEBUG(__FUNCTION__, "ARP Received ln %d, vlan_count %d, set_arp_flag %d, my_ip %d",
                    ln, vlan_count, set_arp_flag, my_ip_arp_flag);

            if (vlan_count == 0)
            {
                vlan_if = local_if_find_by_name(arp_entry->ifname);
                if (vlan_if && vlan_if->is_l3_proto_enabled)
                {
                    if (arp_entry->ipv4_addr == vlan_if->ipv4_addr) {
                        my_ip_arp_flag = 1;
                    }
                    set_arp_flag = 1;
                }
            }
        }
    }

    if(my_ip_arp_flag)
    {
        ICCPD_LOG_DEBUG(__FUNCTION__," ignoring ARP sync for self ip %s ", show_ip_str(arp_entry->ipv4_addr));
        return 0;
    }

    if (set_arp_flag == 0)
    {
        LIST_FOREACH(local_if, &(MLACP(csm).lif_list), mlacp_next)
        {
            if (local_if->type == IF_T_PORT_CHANNEL)
            {
                if (!local_if_is_l3_mode(local_if))
                {
                    /* Is the L2 MLAG itf belong to a vlan the same as peer?*/
                    if (vlan_id) {
                        vlan_id_list = RB_FIND(vlan_rb_tree, &(local_if->vlan_tree), &vlan_key);
                        if (vlan_id_list && vlan_id_list->vlan_itf) {
                            if (arp_entry->ipv4_addr == vlan_id_list->vlan_itf->ipv4_addr) {
                                my_ip_arp_flag = 1;
                            }

                            ICCPD_LOG_DEBUG(__FUNCTION__,
                                "ARP is learnt from intf %s, mclag %s is the member of this vlan",
                                vlan_id_list->vlan_itf->name, local_if->name);
                        }
                    }

                    ICCPD_LOG_DEBUG(__FUNCTION__, "ARP received PO %s, active %d, my_ip %d, ln %d",
                            local_if->name, local_if->po_active, my_ip_arp_flag, ln);
                    if (vlan_id_list && local_if->po_active == 1)
                    {
                        /* Any po of L3 vlan is alive, set the ARP info*/
                        set_arp_flag = 1;
                        break;
                    }
                }
                else
                {
                    /* Is the ARP belong to a L3 mode MLAG itf?*/
                    if (strcmp(local_if->name, arp_entry->ifname) == 0)
                    {
                        ICCPD_LOG_DEBUG(__FUNCTION__,
                                        "ARP is learnt from mclag L3 intf %s, active %d",
                                        local_if->name, local_if->po_active);
                        if (local_if->po_active == 1)
                        {
                            /* po is alive, set the ARP info*/
                            set_arp_flag = 1;
                            break;
                        }
                    }
                    else
                    {
                        ln = __LINE__;
                        continue;
                    }
                }
            }
        }
    }

    if(my_ip_arp_flag)
    {
        ICCPD_LOG_DEBUG(__FUNCTION__," ignoring ARP sync for self ip %s ", show_ip_str(arp_entry->ipv4_addr));
        return 0;
    }

    /* set dynamic ARP*/
    if (set_arp_flag == 1)
    {
        if (arp_entry->flag & NEIGH_SYNC_FLAG_SELF_IP)
        {
            permanent_neigh = 1;
        }

        if (arp_entry->op_type == NEIGH_SYNC_ADD)
        {
            err = iccp_netlink_neighbor_request(AF_INET, (uint8_t *)&arp_entry->ipv4_addr, 1, arp_entry->mac_addr, arp_entry->ifname, permanent_neigh, 8);
            if (err < 0)
            {
                if (err != ICCP_NLE_SEQ_MISMATCH) {
                    ICCPD_LOG_ERR(__FUNCTION__, "ARP add failure for %s %s %s, status %d",
                                arp_entry->ifname, show_ip_str(arp_entry->ipv4_addr), mac_str, err);
                    return MCLAG_ERROR;
                }
            }

           if (arp_entry->flag & NEIGH_SYNC_FLAG_ACK)
           {
               ICCPD_LOG_DEBUG(__FUNCTION__,"Sync ARP on ACK ");
               syn_ack_local_neigh_mac_info_to_peer(arp_entry->ifname, 0);
           }
        }
        else
        {
            err = iccp_netlink_neighbor_request(AF_INET, (uint8_t *)&arp_entry->ipv4_addr, 0, arp_entry->mac_addr, arp_entry->ifname, permanent_neigh, 9);
            if (err < 0)
            {
                if (err != ICCP_NLE_SEQ_MISMATCH) {
                    ICCPD_LOG_ERR(__FUNCTION__, "ARP delete failure for %s %s %s, status %d",
                                arp_entry->ifname, show_ip_str(arp_entry->ipv4_addr), mac_str, err);
                    return MCLAG_ERROR;
                }
            }
        }

        ICCPD_LOG_DEBUG(__FUNCTION__, "ARP update for %s %s %s",
                        arp_entry->ifname, show_ip_str(arp_entry->ipv4_addr), mac_str);
    }
    else
    {
        ICCPD_LOG_NOTICE(__FUNCTION__, "Failure: port-channel is not alive, ln %d", ln);
        /*TODO Set static route through peer-link or just skip it?*/
        local_vlan_if = local_if_find_by_name(arp_entry->ifname);
        if (local_vlan_if) {
            if (arp_entry->ipv4_addr == local_vlan_if->ipv4_addr) {
                ICCPD_LOG_DEBUG(__FUNCTION__, "ignore my ip %s", show_ip_str(arp_entry->ipv4_addr));
                return 0;
            }
        }
    }

    /* update ARP list*/
    TAILQ_FOREACH(msg, &(MLACP(csm).arp_list), tail)
    {
        arp_msg = (struct ARPMsg*)msg->buf;
        if (arp_msg->ipv4_addr == arp_entry->ipv4_addr)
        {
            /*arp_msg->op_type = tlv->type;*/
            sprintf(arp_msg->ifname, "%s", arp_entry->ifname);
            memcpy(arp_msg->mac_addr, arp_entry->mac_addr, ETHER_ADDR_LEN);
            break;
        }
    }

    /* delete/add ARP list*/
    if (msg && arp_entry->op_type == NEIGH_SYNC_DEL)
    {
        TAILQ_REMOVE(&(MLACP(csm).arp_list), msg, tail);
        free(msg->buf);
        free(msg);
        /*ICCPD_LOG_INFO(__FUNCTION__, "Del arp queue successfully");*/
    }
    else if (!msg && arp_entry->op_type == NEIGH_SYNC_ADD)
    {
        arp_msg = (struct ARPMsg*)&arp_data;
        sprintf(arp_msg->ifname, "%s", arp_entry->ifname);
        arp_msg->ipv4_addr = arp_entry->ipv4_addr;
        arp_msg->op_type = arp_entry->op_type;
        arp_msg->flag = 0;
        arp_msg->learn_flag = NEIGH_REMOTE;
        memcpy(arp_msg->mac_addr, arp_entry->mac_addr, ETHER_ADDR_LEN);
        if (iccp_csm_init_msg(&msg, (char*)arp_msg, sizeof(struct ARPMsg)) == 0)
        {
            mlacp_enqueue_arp(csm, msg);
            /*ICCPD_LOG_INFO(__FUNCTION__, "Add arp queue successfully");*/
        }
    }

    /* remove all ARP msg queue, when receive peer's ARP list at the same time*/
    TAILQ_FOREACH(msg, &(MLACP(csm).arp_msg_list), tail)
    {
        arp_msg = (struct ARPMsg*)msg->buf;
        if (arp_msg->ipv4_addr == arp_entry->ipv4_addr)
            break;
    }

    while (msg)
    {
        arp_msg = (struct ARPMsg*)msg->buf;
        TAILQ_REMOVE(&(MLACP(csm).arp_msg_list), msg, tail);
        free(msg->buf);
        free(msg);
        TAILQ_FOREACH(msg, &(MLACP(csm).arp_msg_list), tail)
        {
            arp_msg = (struct ARPMsg*)msg->buf;
            if (arp_msg->ipv4_addr == arp_entry->ipv4_addr)
                break;
        }
    }

    return 0;
}

int mlacp_fsm_update_arp_info(struct CSM* csm, struct mLACPARPInfoTLV* tlv)
{
    int count = 0;
    int i;

    if (!csm || !tlv)
        return MCLAG_ERROR;
    count = ntohs(tlv->num_of_entry);
    ICCPD_LOG_DEBUG(__FUNCTION__, "Received ARP Info count  %d ", count );

    for (i = 0; i < count; i++)
    {
        mlacp_fsm_update_arp_entry(csm, &(tlv->ArpEntry[i]));
    }
}

/*****************************************
* NDISC-Info Update
* ***************************************/
int mlacp_fsm_update_ndisc_entry(struct CSM *csm, struct NDISCMsg *ndisc_entry)
{
    struct Msg *msg = NULL;
    struct NDISCMsg *ndisc_msg = NULL, ndisc_data;
    struct LocalInterface *local_if;
    struct LocalInterface *vlan_if = NULL;
    struct LocalInterface *peer_link_if = NULL;
    struct LocalInterface *local_vlan_if = NULL;
    struct VLAN_ID *vlan_id_list = NULL;
    int set_ndisc_flag = 0;
    char mac_str[18] = "";
    int my_ip_nd_flag = 0;
    int vlan_count = 0;
    int err = 0, ln = 0;
    int permanent_neigh = 0;
    int is_ack_ll = 0;
    int is_link_local = 0;
    uint16_t vlan_id = 0;
    struct VLAN_ID vlan_key = { 0 };
    int vid_intf_present = 0;

    if (!csm || !ndisc_entry)
        return MCLAG_ERROR;

    sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", ndisc_entry->mac_addr[0], ndisc_entry->mac_addr[1], ndisc_entry->mac_addr[2],
            ndisc_entry->mac_addr[3], ndisc_entry->mac_addr[4], ndisc_entry->mac_addr[5]);

    ICCPD_LOG_DEBUG(__FUNCTION__,
           "Received ND Info, intf[%s] Flag %x, IP[%s], MAC[%s]",
           ndisc_entry->ifname, ndisc_entry->flag, show_ipv6_str((char *)ndisc_entry->ipv6_addr), mac_str);

    if (strncmp(ndisc_entry->ifname, VLAN_PREFIX, strlen(VLAN_PREFIX)) == 0) {
        sscanf (ndisc_entry->ifname, "Vlan%hu", &vlan_id);
    }

    if ((memcmp(show_ipv6_str((char *)ndisc_entry->ipv6_addr), "FE80", 4) == 0)
            || (memcmp(show_ipv6_str((char *)ndisc_entry->ipv6_addr), "fe80", 4) == 0))
    {
        is_link_local = 1;
    }

    if (vlan_id)
    {
        memset(&vlan_key, 0, sizeof(struct VLAN_ID));
        vlan_key.vid = vlan_id;

        peer_link_if = local_if_find_by_name(csm->peer_itf_name);

        if (peer_link_if && !local_if_is_l3_mode(peer_link_if))
        {
            ln = __LINE__;
            /* Is peer-linlk itf belong to a vlan the same as peer? */
            vlan_id_list = RB_FIND(vlan_rb_tree, &(peer_link_if->vlan_tree), &vlan_key);

            if (vlan_id_list)
            {
                vlan_count++;
                if (vlan_id_list->vlan_itf) {
                    if (strcmp(vlan_id_list->vlan_itf->name, ndisc_entry->ifname) == 0) {
                        ln = __LINE__;
                        vid_intf_present = 1;
                    }

                    if (vid_intf_present && local_if_is_l3_mode(vlan_id_list->vlan_itf)) {
                        if (memcmp((char *)ndisc_entry->ipv6_addr, (char *)vlan_id_list->vlan_itf->ipv6_addr, 16) == 0)
                        {
                            my_ip_nd_flag = 1;
                        }

                        if ((my_ip_nd_flag == 0) && is_link_local)
                        {
                            if (memcmp((char *)ndisc_entry->ipv6_addr, (char *)vlan_id_list->vlan_itf->ipv6_ll_addr, 16) == 0)
                            {
                                my_ip_nd_flag = 1;
                            }
                        }
                    }

                    ICCPD_LOG_DEBUG(__FUNCTION__,
                            "ND is learnt from intf %s, peer-link %s is the member of this vlan",
                            vlan_id_list->vlan_itf->name, peer_link_if->name);

                    /* Peer-link belong to L3 vlan is alive, set the NDISC info */
                    set_ndisc_flag = 1;
                }
            }

            ICCPD_LOG_DEBUG(__FUNCTION__, "ND Received ln %d, vlan_count %d, set_ndisc_flag %d, my_ip %d",
                    ln, vlan_count, set_ndisc_flag, my_ip_nd_flag);

            if (vlan_count == 0)
            {
                vlan_if = local_if_find_by_name(ndisc_entry->ifname);
                if (vlan_if && vlan_if->is_l3_proto_enabled)
                {
                    if (memcmp((char *)ndisc_entry->ipv6_addr, (char *)vlan_if->ipv6_addr, 16) == 0)
                    {
                        my_ip_nd_flag = 1;
                    }

                    if ((my_ip_nd_flag == 0) && is_link_local)
                    {
                        if (memcmp((char *)ndisc_entry->ipv6_addr, (char *)vlan_if->ipv6_ll_addr, 16) == 0)
                        {
                            my_ip_nd_flag = 1;
                        }
                    }
                    set_ndisc_flag = 1;
                }
            }
        }
    }

    if(my_ip_nd_flag)
    {
        ICCPD_LOG_DEBUG(__FUNCTION__," ignoring ND sync for self ipv6 %s ", show_ipv6_str((char *)ndisc_entry->ipv6_addr));
        return 0;
    }

    if (set_ndisc_flag == 0)
    {
        LIST_FOREACH(local_if, &(MLACP(csm).lif_list), mlacp_next)
        {
            if (local_if->type == IF_T_PORT_CHANNEL)
            {
                if (!local_if_is_l3_mode(local_if))
                {
                    ln = __LINE__;
                    /* Is the L2 MLAG itf belong to a vlan the same as peer? */
                    if (vlan_id) {
                        vlan_id_list = RB_FIND(vlan_rb_tree, &(local_if->vlan_tree), &vlan_key);
                        if (vlan_id_list && vlan_id_list->vlan_itf) {

                            if (memcmp((char *)ndisc_entry->ipv6_addr, (char *)vlan_id_list->vlan_itf->ipv6_addr, 16) == 0)
                            {
                                my_ip_nd_flag = 1;
                            }

                            ICCPD_LOG_DEBUG(__FUNCTION__, "ND is learnt from intf %s, %s is the member of this vlan, my_ip %d",
                                    vlan_id_list->vlan_itf->name, local_if->name, my_ip_nd_flag);
                        }
                    }

                    ICCPD_LOG_DEBUG(__FUNCTION__, "ND received PO %s, active %d, ln %d",
                            local_if->name, local_if->po_active, ln);
                    if (vlan_id_list && local_if->po_active == 1)
                    {
                        /* Any po of L3 vlan is alive, set the NDISC info */
                        set_ndisc_flag = 1;
                        break;
                    }
                }
                else
                {
                    /* Is the ARP belong to a L3 mode MLAG itf? */
                    if (strcmp(local_if->name, ndisc_entry->ifname) == 0)
                    {
                        ICCPD_LOG_DEBUG(__FUNCTION__, "ND is learnt from mclag L3 intf %s, active %d",
                                local_if->name, local_if->po_active);
                        if (local_if->po_active == 1)
                        {
                            /* po is alive, set the NDISC info */
                            set_ndisc_flag = 1;
                            break;
                        }
                    }
                    else
                    {
                        ln = __LINE__;
                        continue;
                    }
                }
            }
        }
    }

    if(my_ip_nd_flag)
    {
        ICCPD_LOG_DEBUG(__FUNCTION__," ignoring ND sync for self ipv6 %s ", show_ipv6_str((char *)ndisc_entry->ipv6_addr));
        return 0;
    }
    /* set dynamic Ndisc */
    if (set_ndisc_flag == 1)
    {
        if (ndisc_entry->flag & NEIGH_SYNC_FLAG_SELF_LL)
        {
            permanent_neigh = 1;
            is_ack_ll = 1;
        }

        if (ndisc_entry->flag & NEIGH_SYNC_FLAG_SELF_IP)
        {
            permanent_neigh = 1;
        }

        if (ndisc_entry->op_type == NEIGH_SYNC_ADD)
        {
            err = iccp_netlink_neighbor_request(AF_INET6, (uint8_t *)ndisc_entry->ipv6_addr, 1, ndisc_entry->mac_addr, ndisc_entry->ifname, permanent_neigh, 10);
            if (err < 0)
            {
                if (err != ICCP_NLE_SEQ_MISMATCH) {
                    ICCPD_LOG_NOTICE(__FUNCTION__, "Failed to add nd entry(%s %s %s) to kernel, status %d",
                                ndisc_entry->ifname, show_ipv6_str((char *)ndisc_entry->ipv6_addr), mac_str, err);
                    return MCLAG_ERROR;
                }
            }

           if (ndisc_entry->flag & NEIGH_SYNC_FLAG_ACK)
           {
               ICCPD_LOG_DEBUG(__FUNCTION__,"Sync ND on ACK ");
               syn_ack_local_neigh_mac_info_to_peer(ndisc_entry->ifname, is_ack_ll);
           }
        }
        else
        {
            err = iccp_netlink_neighbor_request(AF_INET6, (uint8_t *)ndisc_entry->ipv6_addr, 0, ndisc_entry->mac_addr, ndisc_entry->ifname, permanent_neigh, 11);
            if (err < 0)
            {
                if (err != ICCP_NLE_SEQ_MISMATCH) {
                    ICCPD_LOG_NOTICE(__FUNCTION__, "Failed to delete nd entry(%s %s %s) from kernel status %d",
                                ndisc_entry->ifname, show_ipv6_str((char *)ndisc_entry->ipv6_addr), mac_str, err);
                    return MCLAG_ERROR;
                }
            }
        }

        ICCPD_LOG_DEBUG(__FUNCTION__, "NDISC update for %s %s %s", ndisc_entry->ifname, show_ipv6_str((char *)ndisc_entry->ipv6_addr), mac_str);
    }
    else
    {
        ICCPD_LOG_NOTICE(__FUNCTION__, "Failure: port-channel is not alive, ln %d", ln);
        /* TODO Set static route through peer-link or just skip it? */
        local_vlan_if = local_if_find_by_name(ndisc_entry->ifname);
        if (local_vlan_if) {
            if (memcmp((char *)ndisc_entry->ipv6_addr, (char *)local_vlan_if->ipv6_addr, 16) == 0) {
                ICCPD_LOG_DEBUG(__FUNCTION__, "ignore my ip %s", show_ipv6_str((char *)ndisc_entry->ipv6_addr));
                return 0;
            }
        }
    }

    /* update NDISC list */
    TAILQ_FOREACH(msg, &(MLACP(csm).ndisc_list), tail)
    {
        ndisc_msg = (struct NDISCMsg *)msg->buf;
        if (memcmp((char *)ndisc_msg->ipv6_addr, (char *)ndisc_entry->ipv6_addr, 16) == 0)
        {
            /* ndisc_msg->op_type = tlv->type; */
            sprintf(ndisc_msg->ifname, "%s", ndisc_entry->ifname);
            memcpy(ndisc_msg->mac_addr, ndisc_entry->mac_addr, ETHER_ADDR_LEN);
            break;
        }
    }

    /* delete/add NDISC list */
    if (msg && ndisc_entry->op_type == NEIGH_SYNC_DEL)
    {
        TAILQ_REMOVE(&(MLACP(csm).ndisc_list), msg, tail);
        free(msg->buf);
        free(msg);
        /* ICCPD_LOG_INFO(__FUNCTION__, "Del ndisc queue successfully"); */
    }
    else if (!msg && ndisc_entry->op_type == NEIGH_SYNC_ADD)
    {
        ndisc_msg = (struct NDISCMsg *)&ndisc_data;
        sprintf(ndisc_msg->ifname, "%s", ndisc_entry->ifname);
        memcpy((char *)ndisc_msg->ipv6_addr, (char *)ndisc_entry->ipv6_addr, 16);
        ndisc_msg->op_type = ndisc_entry->op_type;
        ndisc_msg->flag = 0;
        ndisc_msg->learn_flag = NEIGH_REMOTE;
        memcpy(ndisc_msg->mac_addr, ndisc_entry->mac_addr, ETHER_ADDR_LEN);
        if (iccp_csm_init_msg(&msg, (char *)ndisc_msg, sizeof(struct NDISCMsg)) == 0)
        {
            mlacp_enqueue_ndisc(csm, msg);
            /* ICCPD_LOG_INFO(__FUNCTION__, "Add ndisc queue successfully"); */
        }
    }

    /* remove all NDISC msg queue, when receive peer's NDISC list at the same time */
    TAILQ_FOREACH(msg, &(MLACP(csm).ndisc_msg_list), tail)
    {
        ndisc_msg = (struct NDISCMsg *)msg->buf;
        if (memcmp((char *)ndisc_msg->ipv6_addr, (char *)ndisc_entry->ipv6_addr, 16) == 0)
            break;
    }

    while (msg)
    {
        ndisc_msg = (struct NDISCMsg *)msg->buf;
        TAILQ_REMOVE(&(MLACP(csm).ndisc_msg_list), msg, tail);
        free(msg->buf);
        free(msg);
        TAILQ_FOREACH(msg, &(MLACP(csm).ndisc_msg_list), tail)
        {
            ndisc_msg = (struct NDISCMsg *)msg->buf;
            if (memcmp((char *)ndisc_msg->ipv6_addr, (char *)ndisc_entry->ipv6_addr, 16) == 0)
                break;
        }
    }

    return 0;
}

int mlacp_fsm_update_ndisc_info(struct CSM *csm, struct mLACPNDISCInfoTLV *tlv)
{
    int count = 0;
    int i;

    if (!csm || !tlv)
        return MCLAG_ERROR;
    count = ntohs(tlv->num_of_entry);
    ICCPD_LOG_INFO(__FUNCTION__, "Received NDISC Info count  %d ", count);

    for (i = 0; i < count; i++)
    {
        mlacp_fsm_update_ndisc_entry(csm, &(tlv->NdiscEntry[i]));
    }
}

/*****************************************
* Port-Channel-Info Update
* ***************************************/
int mlacp_fsm_update_port_channel_info(struct CSM* csm,
                                       struct mLACPPortChannelInfoTLV* tlv)
{
    struct PeerInterface* peer_if = NULL;
    struct VLAN_ID* peer_vlan_id = NULL;
    int i = 0;

    if (csm == NULL || tlv == NULL )
        return MCLAG_ERROR;

    LIST_FOREACH(peer_if, &(MLACP(csm).pif_list), mlacp_next)
    {
        if (peer_if->type != IF_T_PORT_CHANNEL)
            continue;

        if (peer_if->po_id != ntohs(tlv->agg_id))
            continue;

        RB_FOREACH(peer_vlan_id, vlan_rb_tree, &(peer_if->vlan_tree))
        {
            peer_vlan_id->vlan_removed = 1;
        }

        /* Record peer info*/
        peer_if->ipv4_addr = ntohl(tlv->ipv4_addr);
        peer_if->l3_mode = tlv->l3_mode;

        for (i = 0; i < ntohs(tlv->num_of_vlan_id); i++)
        {
            peer_if_add_vlan(peer_if, ntohs(tlv->vlanData[i].vlan_id));
        }

        peer_if_clean_unused_vlan(peer_if);

        iccp_consistency_check(peer_if->name);

        ICCPD_LOG_DEBUG("ICCP_FSM", "RX Peer po_info: %s ipv4 addr %s l3 mode  %d",
            peer_if->name, show_ip_str( tlv->ipv4_addr), peer_if->l3_mode);
        break;
    }

    return 0;
}

/*****************************************
* Peerlink port Update
* ***************************************/
int mlacp_fsm_update_peerlink_info(struct CSM* csm,
                                   struct mLACPPeerLinkInfoTLV* tlv)
{
    if (csm == NULL || tlv == NULL )
        return MCLAG_ERROR;

    if (!csm->peer_link_if)
    {
        ICCPD_LOG_WARN(__FUNCTION__, "Peerlink port info recv from peer, local peerlink is not exist!");
        return 0;
    }

    if (csm->peer_link_if->type != tlv->port_type)
        ICCPD_LOG_DEBUG(__FUNCTION__, "Peerlink port type of peer %d is not same with local %d !", tlv->port_type, csm->peer_link_if->type);

    if (tlv->port_type == IF_T_VXLAN && strncmp(csm->peer_itf_name, tlv->if_name, strlen(csm->peer_itf_name)))
        ICCPD_LOG_DEBUG(__FUNCTION__, "Peerlink port is vxlan port, but peerlink port of peer %s is not same with local %s !", tlv->if_name, csm->peer_itf_name);

    return 0;
}

/*****************************************
* Heartbeat Update
*****************************************/
int mlacp_fsm_update_heartbeat(struct CSM* csm, struct mLACPHeartbeatTLV* tlv)
{
    if (!csm || !tlv)
        return MCLAG_ERROR;

    time(&csm->heartbeat_update_time);

    return 0;
}

/*****************************************
* warm-reboot flag Update
*****************************************/
int mlacp_fsm_update_warmboot(struct CSM* csm, struct mLACPWarmbootTLV* tlv)
{
    if (!csm || !tlv)
        return MCLAG_ERROR;

    time(&csm->peer_warm_reboot_time);
    ICCPD_LOG_DEBUG("ICCP_FSM", "RX peer warm reboot: start, sync_state %s",
        mlacp_state(csm));
    return 0;
}
