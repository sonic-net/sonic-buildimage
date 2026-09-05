/* Copyright(c) 2016-2019 Nephos/Estinet.
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

#ifndef ICCP_NETLINK_H
#define ICCP_NETLINK_H
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <netlink/netlink.h>
#include <linux/types.h>
#include "../include/system.h"
#include "../include/port.h"
#include <netinet/icmp6.h>
#include <linux/ipv6.h>

#define NDISC_NEIGHBOUR_ADVERTISEMENT	136
#define ND_OPT_TARGET_LL_ADDR 2
#define NEXTHDR_ICMP 58
#define ICCP_NLE_SEQ_MISMATCH -16

struct nd_msg
{
    struct icmp6_hdr icmph;
    struct in6_addr target;
    __u8 opt[0];
};

struct in6_pktinfo
{
    struct in6_addr ipi6_addr;  /* src/dst IPv6 address */
    unsigned int ipi6_ifindex;  /* send/recv interface index */
};

int iccp_get_port_member_list(struct LocalInterface *lif);
void iccp_event_handler_obj_input_newlink(struct nl_object *obj, void *arg);
void iccp_event_handler_obj_input_dellink(struct nl_object *obj, void *arg);
int iccp_system_init_netlink_socket();
void iccp_system_dinit_netlink_socket();
int iccp_init_netlink_event_fd(struct System *sys);
int iccp_handle_events(struct System *sys);
void update_if_ipmac_on_standby(struct LocalInterface *lif_po, int dir);
int iccp_sys_local_if_list_get_addr();
int iccp_netlink_neighbor_request(int family, uint8_t *addr, int add, uint8_t *mac, char *portname, int permanent, int dir);
int iccp_check_if_addr_from_netlink(int family, uint8_t *addr, struct LocalInterface *lif);

void recover_if_ipmac_on_standby(struct LocalInterface* lif_po, int dir);
void update_vlan_if_mac_on_standby(struct LocalInterface* lif_vlan, int dir);
void recover_vlan_if_mac_on_standby(struct LocalInterface* lif_vlan, int dir, uint8_t *remote_system_mac);
void update_vlan_if_mac_on_iccp_up(struct LocalInterface* lif_peer, int is_up, uint8_t *remote_system_mac);
#endif
