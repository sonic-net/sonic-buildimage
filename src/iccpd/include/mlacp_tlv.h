/*
 *  mlacp_tlv.h
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

#ifndef MLACP_TLV_H_
#define MLACP_TLV_H_

#include <sys/queue.h>

#include "../include/msg_format.h"
#include "../include/port.h"
#include "../include/openbsd_tree.h"
#include "../include/mlacp_tlv.h"


#define MLACP_SYSCONF_NODEID_MSB_MASK       0x80
#define MLACP_SYSCONF_NODEID_NODEID_MASK    0x70
#define MLACP_SYSCONF_NODEID_FREE_MASK      0x0F

/*
 * RFC 7275
 * 7.2.3.  mLACP System Config TLV
 * [Page 51]
 */
struct mLACPSysConfigTLV
{
    ICCParameter icc_parameter;
    /* [IEEE-802.1AX], Section 5.3.2. */
    uint8_t sys_id[ETHER_ADDR_LEN];
    /* [IEEE-802.1AX], Section 5.3.2. */
    uint16_t sys_priority;
    /*
     * RFC 7275
     * 7.2.3.  mLACP System Config TLV
     * [Page 51]
     */
    uint8_t node_id;
} __attribute__ ((packed));

typedef struct mLACPSysConfigTLV mLACPSysConfigTLV;

/*
 * RFC 7275
 * 7.2.4.  mLACP Aggregator Config TLV
 * [Page 52]
 * NOTE: In this project, Aggregator configuration and state TLV is not support.
 */
struct mLACPAggConfigTLV
{
    ICCParameter icc_parameter;
    /*
     * RFC 7275
     * 7.2.4.  mLACP Aggregator Config TLV
     * [Page 53]
     */
    uint8_t ro_id[8];
    /* [IEEE-802.1AX], Section 5.4.6. */
    uint16_t agg_id;
    /*
     * RFC 7275
     * 7.2.4.  mLACP Aggregator Config TLV
     * [Page 53]
     */
    uint8_t mac_addr[ETHER_ADDR_LEN];
    /* [IEEE-802.1AX], Section 5.3.5. */
    uint16_t actor_key;
    /*
     * RFC 7275
     * 7.2.4.  mLACP Aggregator Config TLV
     * [Page 53]
     */
    uint16_t member_ports_priority;
    uint8_t flags;
    /*
     * RFC 7275
     * 7.2.4.  mLACP Aggregator Config TLV
     * [Page 54]
     */
    uint8_t agg_name_len;
    char agg_name[MAX_L_PORT_NAME];
} __attribute__ ((packed));

typedef struct mLACPAggConfigTLV mLACPAggConfigTLV;

/*
 * RFC 7275
 * 7.2.4.  mLACP Port Config TLV
 * [Page 54]
 */
struct mLACPPortConfigTLV
{
    ICCParameter icc_parameter;
    /* [IEEE-802.1AX], Section 5.3.4. */
    uint16_t port_num;
    /*
     * RFC 7275
     * 7.2.5.  mLACP Port Config TLV
     * [Page 55]
     */
    uint8_t mac_addr[ETHER_ADDR_LEN];
    /* [IEEE-802.1AX], Section 5.3.5. */
    uint16_t actor_key;
    /* [IEEE-802.1AX], Section 5.3.4. */
    uint16_t port_priority;
    /* IF-MIB [RFC2863] */
    uint32_t port_speed;
    /*
     * RFC 7275
     * 7.2.4.  mLACP Port Config TLV
     * [Page 55]
     */
    uint8_t flags;
    /*
     * RFC 7275
     * 7.2.4.  mLACP Port Config TLV
     * [Page 56]
     */
    uint8_t port_name_len;
    /* IF-MIB [RFC2863] */
    char port_name[MAX_L_PORT_NAME];

    /* NOS */
    uint8_t l3_mode;
} __attribute__ ((packed));

typedef struct mLACPPortConfigTLV mLACPPortConfigTLV;

/*
 * RFC 7275
 * 7.2.6.  mLACP Port Priority TLV
 * [Page 56]
 */
struct mLACPPortPriorityTLV
{
    ICCParameter icc_parameter;
    /*
     * RFC 7275
     * 7.2.6.  mLACP Port Priority TLV
     * [Page 57]
     */
    uint16_t op_code;
    /* [IEEE-802.1AX], Section 5.3.4. */
    uint16_t port_num;
    /* [IEEE-802.1AX], Section 5.4.6. */
    uint16_t agg_id;
    /* [IEEE-802.1AX], Section 5.3.4. */
    uint16_t last_port_priority;
    uint16_t current_port_priority;
} __attribute__ ((packed));

typedef struct mLACPPortPriorityTLV mLACPPortPriorityTLV;

/*
 * RFC 7275
 * 7.2.7.  mLACP Port State TLV
 * [Page 58]
 */
struct mLACPPortStateTLV
{
    ICCParameter icc_parameter;
    /* [IEEE-802.1AX], Section 5.4.2.2, item r. */
    uint8_t partner_sys_id[ETHER_ADDR_LEN];
    /* [IEEE-802.1AX], Section 5.4.2.2, item q. */
    uint16_t partner_sys_priority;
    /* [IEEE-802.1AX], Section 5.4.2.2, item u. */
    uint16_t partner_port_num;
    /* [IEEE-802.1AX], Section 5.4.2.2, item t. */
    uint16_t partner_port_priority;
    /* [IEEE-802.1AX], Section 5.4.2.2, item s. */
    uint16_t partner_key;
    /* [IEEE-802.1AX], Section 5.4.2.2, item v. */
    uint8_t partner_state;
    /* [IEEE-802.1AX], Section 5.4.2.2, item m. */
    uint8_t actor_state;
    /* [IEEE-802.1AX], Section 5.3.4. */
    uint16_t actor_port_num;
    /* [IEEE-802.1AX], Section 5.3.5. */
    uint16_t actor_key;
    /* [IEEE-802.1AX], Section 5.4.8 */
    uint8_t selected;
    /*
     * RFC 7275
     * 7.2.7.  mLACP Port State TLV
     * [Page 60]
     */
    uint8_t port_state;
    /* [IEEE-802.1AX], Section 5.4.6. */
    uint16_t agg_id;

    /* NOS */
    uint16_t port_id;
    uint8_t l3_mode;
    uint8_t is_peer_link;
} __attribute__ ((packed));

typedef struct mLACPPortStateTLV mLACPPortStateTLV;

/*
 * RFC 7275
 * 7.2.8.  mLACP Aggregator State TLV
 * [Page 60]
 * NOTE: In this project, Aggregator configuration and state TLV is not support.
 */
struct mLACPAggPortStateTLV
{
    ICCParameter icc_parameter;
    /* [IEEE-802.1AX], Section 5.4.2.2, item r. */
    uint8_t partner_sys_id[ETHER_ADDR_LEN];
    /* [IEEE-802.1AX], Section 5.4.2.2, item q. */
    uint16_t partner_sys_priority;
    /* [IEEE-802.1AX], Section 5.4.2.2, item s. */
    uint16_t partner_key;
    /* [IEEE-802.1AX], Section 5.4.6. */
    uint16_t agg_id;
    /* [IEEE-802.1AX], Section 5.3.5. */
    uint16_t actor_key;
    /*
     * RFC 7275
     * 7.2.8.  mLACP Aggregator State TLV
     * [Page 61]
     */
    uint8_t agg_state;
} __attribute__ ((packed));

typedef struct mLACPAggPortStateTLV mLACPAggPortStateTLV;

/*
 * RFC 7275
 * 7.2.9.  mLACP Synchronization Request TLV
 * [Page 61]
 */
struct mLACPSyncReqTLV
{
    ICCParameter icc_parameter;
    /*
     * RFC 7275
     * 7.2.9.  mLACP Synchronization Request TLV
     * [Page 62]
     */
    uint16_t req_num;

#if __BYTE_ORDER == __BIG_ENDIAN
    uint16_t c_bit : 1;
    /*
     * RFC 7275
     * 7.2.9.  mLACP Synchronization Request TLV
     * [Page 63]
     */
    uint16_t s_bit : 1;
    uint16_t req_type : 14;
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    uint16_t req_type : 14;
    /*
     * RFC 7275
     * 7.2.9.  mLACP Synchronization Request TLV
     * [Page 63]
     */
    uint16_t s_bit : 1;
    uint16_t c_bit : 1;
#endif
    /* [IEEE-802.1AX], Section 5.3.4. */
    /* [IEEE-802.1AX], Section 5.4.6. */
    uint16_t port_num_agg_id;
    /* [IEEE-802.1AX], Section 5.3.5. */
    uint16_t actor_key;
} __attribute__ ((packed));

typedef struct mLACPSyncReqTLV mLACPSyncReqTLV;

/*
 * RFC 7275
 * 7.2.10.  mLACP Synchronization Data TLV
 * [Page 63]
 */
struct mLACPSyncDataTLV
{
    ICCParameter icc_parameter;
    /*
     * RFC 7275
     * 7.2.10.  mLACP Synchronization Data TLV
     * [Page 64]
     */
    uint16_t req_num;
    uint16_t flags;
} __attribute__ ((packed));

typedef struct mLACPSyncDataTLV mLACPSyncDataTLV;

/* VLAN Information TLV*/
struct mLACPVLANData
{
    uint16_t vlan_id;
} __attribute__ ((packed));

/*
 * Port Channel Information TLV
 */
struct mLACPPortChannelInfoTLV
{
    ICCParameter icc_parameter;
    uint16_t agg_id;
    char if_name[MAX_L_PORT_NAME];
    uint8_t if_name_len;
    uint8_t l3_mode;
    uint32_t ipv4_addr;
    uint16_t po_id;
    uint16_t num_of_vlan_id;
    struct mLACPVLANData vlanData[0];
} __attribute__ ((packed));

typedef struct mLACPPortChannelInfoTLV mLACPPortChannelInfoTLV;

/*
 * Port PeerLink Information TLV
 */
struct mLACPPeerLinkInfoTLV
{
    ICCParameter icc_parameter;
    char if_name[MAX_L_PORT_NAME];
    uint8_t port_type;
} __attribute__ ((packed));

typedef struct mLACPPeerLinkInfoTLV mLACPPeerLinkInfoTLV;

struct mLACPVLANInfoTLV
{
    ICCParameter icc_parameter;
    uint16_t id; /* Local Interface ID, not VLAN ID */
    uint16_t num_of_vlan_id;
    struct mLACPVLANData vlanData[0];
} __attribute__ ((packed));

/* Mac entry Information TLV*/
struct mLACPMACData
{
    uint8_t     type;/*add or del*/
    uint8_t     mac_type;
    uint8_t     mac_addr[ETHER_ADDR_LEN];
    uint16_t vid;
    /*Current if name that set in chip*/
    char     ifname[MAX_L_PORT_NAME];
} __attribute__ ((packed));

/*
 * MAC Information TLV
 */
struct mLACPMACInfoTLV
{
    ICCParameter    icc_parameter;
    uint16_t num_of_entry;
    struct mLACPMACData MacEntry[0];
} __attribute__ ((packed));

#define NEIGH_LOCAL   1
#define NEIGH_REMOTE  2

struct ARPMsg
{
    uint8_t op_type;
    uint8_t flag;
    uint8_t learn_flag;       /*Loacl or Remote*/
    char ifname[MAX_L_PORT_NAME];
    uint32_t ipv4_addr;         // net order
    uint8_t mac_addr[ETHER_ADDR_LEN];
};

struct NDISCMsg
{
    uint8_t op_type;
    uint8_t flag;
    uint8_t learn_flag;       /*Loacl or Remote*/
    char ifname[MAX_L_PORT_NAME];
    uint32_t ipv6_addr[4];
    uint8_t mac_addr[ETHER_ADDR_LEN];
};

/*
 * ARP Information TLV
 */
struct mLACPARPInfoTLV
{
    ICCParameter icc_parameter;
    /* Local Interface ID */
    uint16_t num_of_entry;
    struct ARPMsg ArpEntry[0];
} __attribute__ ((packed));

/*
 * NDISC Information TLV
 */
struct mLACPNDISCInfoTLV
{
    ICCParameter icc_parameter;
    /* Local Interface ID */
    uint16_t num_of_entry;
    struct NDISCMsg NdiscEntry[0];
} __attribute__ ((packed));

/*
 * NOS: STP Information TLV
 */
struct stp_msg_s;
struct mLACPSTPInfoTLV
{
    ICCParameter    icc_parameter;
    uint8_t         stp_msg[0];
} __attribute__ ((packed));

/*
 * NOS: Heartbeat
 */
struct mLACPHeartbeatTLV
{
    ICCParameter    icc_parameter;
    uint8_t         heartbeat;
} __attribute__ ((packed));

/*
 * NOS: Warm_reboot
 */
struct mLACPWarmbootTLV
{
    ICCParameter    icc_parameter;
    uint8_t         warmboot;
} __attribute__ ((packed));

/*
 * NOS: interface up ack message
 * ACK is sent by MLAG peer after processing MLAG interface up notification.
 * The ack indicates that port-isolation is applied on the MLAG peer node
 */
typedef uint8_t PORT_ISOLATION_STATE_e;
enum PORT_ISOLATION_STATE_e
{
    PORT_ISOLATION_STATE_DISABLE = 0,
    PORT_ISOLATION_STATE_ENABLE = 1
};

typedef uint8_t IF_UP_ACK_TYPE_e;
enum IF_UP_ACK_TYPE_e
{
    IF_UP_ACK_TYPE_PHY_PORT = 0,
    IF_UP_ACK_TYPE_PORT_CHANNEL = 1
};

struct mLACPIfUpAckTLV {
    ICCParameter    icc_parameter;
    uint8_t         if_type;
    uint8_t         port_isolation_state;
    uint16_t        if_id;                   /* LAG: agg_id */
}__attribute__ ((packed));

enum NEIGH_OP_TYPE
{
    NEIGH_SYNC_LIF = 0,
    NEIGH_SYNC_ADD = 1,
    NEIGH_SYNC_DEL = 2,
};

enum NEIGH_FLAG
{
    NEIGH_SYNC_FLAG_ACK = 1,
    NEIGH_SYNC_FLAG_SELF_LL = 2,
    NEIGH_SYNC_FLAG_SELF_IP = 4,
};

enum NEIGH_SYNC_DIR
{
    NEIGH_SYNC_CLIENT_IP = 1,
    NEIGH_SYNC_SELF_IP = 2,
};

enum MAC_AGE_TYPE
{
    MAC_AGE_LOCAL   = 1,    /*MAC in local switch is ageout*/
    MAC_AGE_PEER    = 2,    /*MAC in peer switch is ageout*/
};

enum MAC_OP_TYPE
{
    MAC_SYNC_ADD    = 1,
    MAC_SYNC_DEL    = 2,
    MAC_SYNC_ACK    = 4,
};

enum MAC_TYPE
{
    MAC_TYPE_STATIC     = 1,
    MAC_TYPE_DYNAMIC    = 2,
    MAC_TYPE_DYNAMIC_LOCAL = 3,  /* Used while sending MAC to Syncd to program with aging enabled. */
};

struct MACMsg
{
    RB_ENTRY(MACMsg) mac_entry_rb;
    uint16_t    vid;
    uint8_t     mac_addr[ETHER_ADDR_LEN];
    uint8_t     op_type;    /*add or del*/
    uint8_t     fdb_type;   /*static or dynamic*/

    /*Current if name that set in chip*/
    char     ifname[MAX_L_PORT_NAME];
    /*if we set the mac to peer-link, origin_ifname store the
       original if name that learned from chip*/
    char     origin_ifname[MAX_L_PORT_NAME];
    uint8_t age_flag;/*local or peer is age?*/
    uint8_t pending_local_del;
    uint8_t add_to_syncd;

    TAILQ_ENTRY(MACMsg) tail;     // entry into mac_msg_list
};

RB_HEAD(mac_rb_tree, MACMsg);
RB_PROTOTYPE(mac_rb_tree, MACMsg, mac_entry_rb, MACMsg_compare);

#endif /* MLACP_TLV_H_ */
