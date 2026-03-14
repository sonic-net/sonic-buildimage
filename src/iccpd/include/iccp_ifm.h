/*
 * iccp_ifm.h
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

#ifndef ICCP_IFM_H
#define ICCP_IFM_H

#include <netlink/netlink.h>

int iccp_sys_local_if_list_get_init();

int iccp_neigh_get_init();

void do_arp_update_from_reply_packet(unsigned int ifindex, unsigned int addr, uint8_t mac_addr[ETHER_ADDR_LEN]);
void do_ndisc_update_from_reply_packet(unsigned int ifindex, char *ipv6_addr, uint8_t mac_addr[ETHER_ADDR_LEN]);

int do_one_neigh_request(struct nlmsghdr *n);

void iccp_from_netlink_port_state_handler( char * ifname, int state);

void vlan_mbrship_change_handler(unsigned int vlan_id, char *mbr_if_name, int add_flag);
void del_all_pending_vlan_mbr_ifs(struct System *sys);
void move_pending_vlan_mbr_to_lif(struct System *sys, struct LocalInterface* lif);
#endif // LACP_IFM_H

