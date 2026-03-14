/*
 * port.h
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

#ifndef PORT_H_
#define PORT_H_

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <sys/queue.h>

#include "../include/openbsd_tree.h"

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif /* INET_ADDRSTRLEN */


#define ETHER_ADDR_LEN 6
#define ETHER_ADDR_STR_LEN 18
/*
 * RFC 7275
 * 7.2.4.  mLACP Port Config TLV
 * [Page 56]
 */
#define MAX_L_PORT_NAME 20

/* defined in RFC 7275 - 7.2.7 (p.59) */
#define PORT_STATE_UP               0x00
#define PORT_STATE_DOWN             0x01
#define PORT_STATE_ADMIN_DOWN       0x02
#define PORT_STATE_TEST             0x03

/* Interface Type */
#define IF_T_UNKNOW        -1
#define IF_T_PORT           0
#define IF_T_PORT_CHANNEL   1
#define IF_T_VLAN           2
#define IF_T_VXLAN          3
#define IF_T_BRIDGE         4

typedef struct
{
    char *ifname;
    int type;
} itf_type_t;

struct If_info
{
    char name[MAX_L_PORT_NAME];
    LIST_ENTRY(If_info) csm_next;
};

struct Unq_ip_If_info
{
    char name[MAX_L_PORT_NAME];
    LIST_ENTRY(Unq_ip_If_info) if_next;
};

struct VLAN_ID
{
    uint16_t vid;
    uint16_t vlan_removed;
    struct LocalInterface* vlan_itf; /* loacl vlan interface */
    RB_ENTRY(VLAN_ID) vlan_entry;
};

RB_HEAD(vlan_rb_tree, VLAN_ID);
RB_PROTOTYPE(vlan_rb_tree, VLAN_ID, vlan_rb_tree, vlan_node_compare);

struct PendingVlanMbrIf
{
    char name[MAX_L_PORT_NAME];
    struct vlan_rb_tree vlan_tree;
    LIST_ENTRY(PendingVlanMbrIf) if_next;
};

#define VLAN_RB_REMOVE(name, head, elm) do {  \
    RB_REMOVE(name, head, elm);              \
    (elm)->vlan_entry.rbt_parent = NULL;   \
    (elm)->vlan_entry.rbt_left = NULL;     \
    (elm)->vlan_entry.rbt_right = NULL;    \
} while (0)


struct PeerInterface
{
    int ifindex;
    int type;
    char name[MAX_L_PORT_NAME];

    uint8_t mac_addr[ETHER_ADDR_LEN];
    uint8_t state;
    uint32_t ipv4_addr;

    uint8_t l3_mode;
    uint8_t is_peer_link;
    int po_id;
    uint8_t po_active;

    struct CSM* csm;

    LIST_ENTRY(PeerInterface) mlacp_next;
    struct vlan_rb_tree vlan_tree;
};

struct LocalInterface
{
    int ifindex;
    int type;
    char name[MAX_L_PORT_NAME];

    uint8_t mac_addr[ETHER_ADDR_LEN];
    uint8_t mac_addr_ori[ETHER_ADDR_LEN];
    uint8_t state;
    uint32_t ipv4_addr;
    uint8_t prefixlen;
    uint32_t ipv6_addr[4];
    uint8_t prefixlen_v6;
    uint32_t ipv6_ll_addr[4];
    uint8_t ll_prefixlen_v6;

    uint8_t l3_mode;
    uint8_t l3_mac_addr[ETHER_ADDR_LEN];
    uint8_t is_peer_link;
    char portchannel_member_buf[512];
    uint8_t is_arp_accept;
    int po_id;          /* Port Channel ID */
    uint8_t po_active;  /* Port Channel is in active status? */
    int mlacp_state;    /* Record mlacp state */
    uint8_t isolate_to_peer_link;

    time_t po_down_time;

    struct CSM* csm;

    uint8_t changed;
    uint8_t port_config_sync;
    bool is_traffic_disable;   /* Disable traffic tx/rx  */
    bool is_l3_proto_enabled;  /* Enable L3 Protocol support */
    uint32_t vlan_count;
    uint32_t master_ifindex;   /* VRF ifindex*/

    struct vlan_rb_tree vlan_tree;

    LIST_ENTRY(LocalInterface) system_next;
    LIST_ENTRY(LocalInterface) system_purge_next;
    LIST_ENTRY(LocalInterface) mlacp_next;
    LIST_ENTRY(LocalInterface) mlacp_purge_next;
};

struct LocalInterface* local_if_create(int ifindex, char* ifname, int type, uint8_t state);
struct LocalInterface* local_if_find_by_name(const char* ifname);
struct LocalInterface* local_if_find_by_ifindex(int ifindex);
struct LocalInterface* local_if_find_by_po_id(int po_id);

void local_if_destroy(char *ifname);
void local_if_change_flag_clear(void);
void local_if_purge_clear(void);
int local_if_is_l3_mode(struct LocalInterface* local_if);

void local_if_init(struct LocalInterface*);
void local_if_finalize(struct LocalInterface*);

struct PeerInterface* peer_if_create(struct CSM* csm, int peer_if_number, int type);
struct PeerInterface* peer_if_find_by_name(struct CSM* csm, char* name);

void peer_if_destroy(struct PeerInterface* pif);
int peer_if_add_vlan(struct PeerInterface* peer_if, uint16_t vlan_id);
int peer_if_clean_unused_vlan(struct PeerInterface* peer_if);
/* VLAN manipulation */
int local_if_add_vlan(struct LocalInterface* local_if, uint16_t vid);
void local_if_del_vlan(struct LocalInterface* local_if, uint16_t vid);
void local_if_del_all_vlan(struct LocalInterface* lif);

/* ARP manipulation */
int set_sys_arp_accept_flag(char* ifname, int flag);

#endif /* PORT_H_ */
