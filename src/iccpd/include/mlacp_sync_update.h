/*
 *
 * mlacp_sync_update.h
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

#ifndef __MLACP_SYNC_UPDATE__
#define __MLACP_SYNC_UPDATE__

#include "iccp_csm.h"
#include "mlacp_tlv.h"

/*****************************************
* FSM Sync Update API
*
* ***************************************/
int mlacp_fsm_update_system_conf(struct CSM* csm, mLACPSysConfigTLV* tlv);

int mlacp_fsm_update_Aggport_state(struct CSM* csm, mLACPAggPortStateTLV* tlv);

int mlacp_fsm_update_arp_info(struct CSM* csm, struct mLACPARPInfoTLV* tlv);
int mlacp_fsm_update_ndisc_info(struct CSM *csm, struct mLACPNDISCInfoTLV *tlv);

int mlacp_fsm_update_heartbeat(struct CSM* csm, struct mLACPHeartbeatTLV* tlv);

int mlacp_fsm_update_warmboot(struct CSM* csm, struct mLACPWarmbootTLV* tlv);

void mlacp_enqueue_arp(struct CSM* csm, struct Msg* msg);
void mlacp_enqueue_ndisc(struct CSM *csm, struct Msg *msg);
int mlacp_fsm_update_Agg_conf(struct CSM* csm, mLACPAggConfigTLV* portconf);
int mlacp_fsm_update_port_channel_info(struct CSM* csm, struct mLACPPortChannelInfoTLV* tlv);
int mlacp_fsm_update_peerlink_info(struct CSM* csm, struct mLACPPeerLinkInfoTLV* tlv);
int mlacp_fsm_update_mac_info_from_peer(struct CSM* csm, struct mLACPMACInfoTLV* tlv);
#endif
