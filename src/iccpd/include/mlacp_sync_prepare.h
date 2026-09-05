/*
 * mlacp_sync_prepare.h
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

#ifndef __MLACP_SYNC_PREPARE__
#define __MLACP_SYNC_PREPARE__

struct CSM;
/*****************************************
* Tool Function
*
* ***************************************/
void update_system_id(struct CSM* csm);

/*****************************************
* LACP Sync
*
* ***************************************/
int mlacp_sync_with_kernel_callback();

/*****************************************
* MLACP Sync
*
* ***************************************/
int mlacp_prepare_for_sync_request_tlv(struct CSM* csm, char* buf, size_t max_buf_size);
int mlacp_prepare_for_sync_data_tlv(struct CSM* csm, char* buf, size_t max_buf_size, int end);
int mlacp_prepare_for_sys_config(struct CSM* csm, char* buf, size_t max_buf_size);
int mlacp_prepare_for_mac_info_to_peer(struct CSM* csm, char* buf, size_t max_buf_size, struct MACMsg* mac_msg, int count);
int mlacp_prepare_for_arp_info(struct CSM* csm, char* buf, size_t max_buf_size, struct ARPMsg* arp_msg, int count, int dir);
int mlacp_prepare_for_ndisc_info(struct CSM *csm, char *buf, size_t max_buf_size, struct NDISCMsg *ndisc_msg, int count, int dir);
int mlacp_prepare_for_heartbeat(struct CSM* csm, char* buf, size_t max_buf_size);
int mlacp_prepare_for_Aggport_state(struct CSM* csm, char* buf, size_t max_buf_size, struct LocalInterface* local_if);
int mlacp_prepare_for_Aggport_config(struct CSM* csm, char* buf, size_t max_buf_size, struct LocalInterface* lif, int purge_flag);
int mlacp_prepare_for_port_channel_info(struct CSM* csm, char* buf, size_t max_buf_size, struct LocalInterface* port_channel);
int mlacp_prepare_for_port_peerlink_info(struct CSM* csm, char* buf, size_t max_buf_size, struct LocalInterface* peerlink_port);
int iccp_netlink_if_hwaddr_set(uint32_t ifindex, uint8_t *addr, unsigned int addr_len);
int mlacp_prepare_for_if_up_ack(
    struct CSM        *csm,
    char              *buf,
    size_t            max_buf_size,
    uint8_t           if_type,
    uint16_t          if_id,
    uint8_t           port_isolation_enable);
#endif
