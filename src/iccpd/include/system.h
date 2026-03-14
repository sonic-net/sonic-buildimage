/*
 * system.h
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

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/socket.h>
#include <linux/netlink.h>

#include "../include/port.h"

#define FRONT_PANEL_PORT_PREFIX "Ethernet"
#define PORTCHANNEL_PREFIX      "PortChannel"
#define VLAN_PREFIX             "Vlan"
#define VXLAN_TUNNEL_PREFIX     "VTTNL"

#define WARM_REBOOT 1

#define MCLAG_ERROR -1

struct CSM;

#ifndef MAX_BUFSIZE
    #define MAX_BUFSIZE 4096
#endif

extern char mac_print_str[ETHER_ADDR_STR_LEN];

#define MAC_IN_MSG_LIST(head, elm, field)   \
    (((elm)->field.tqe_next != NULL) ||     \
    ((elm)->field.tqe_prev != NULL))

#define CLEAR_MAC_IN_MSG_LIST(head, elm, field)   \
    (elm)->field.tqe_next = NULL;    \
    (elm)->field.tqe_prev = NULL;

#define MAC_TAILQ_REMOVE(head, elm, field) do {  \
    TAILQ_REMOVE(head, elm, field);              \
    (elm)->field.tqe_next = NULL;                \
    (elm)->field.tqe_prev = NULL;                \
} while (/*CONSTCOND*/0)

#define MAC_RB_REMOVE(name, head, elm) do {  \
    RB_REMOVE(name, head, elm);              \
    (elm)->mac_entry_rb.rbt_parent = NULL;   \
    (elm)->mac_entry_rb.rbt_left = NULL;     \
    (elm)->mac_entry_rb.rbt_right = NULL;    \
} while (/*CONSTCOND*/0)

/* Debug counters */
/* Debug counters to track messages ICCPd sent to MclagSyncd */
typedef uint8_t SYNCD_DBG_CNTR_STS_e;
enum SYNCD_DBG_CNTR_STS_e
{
    SYNCD_DBG_CNTR_STS_OK  = 0,
    SYNCD_DBG_CNTR_STS_ERR = 1,    /* Send error or receive processing error*/
    SYNCD_DBG_CNTR_STS_MAX
};

typedef uint8_t SYNCD_TX_DBG_CNTR_MSG_e;
enum SYNCD_TX_DBG_CNTR_MSG_e
{
    SYNCD_TX_DBG_CNTR_MSG_PORT_ISOLATE               = 0,
    SYNCD_TX_DBG_CNTR_MSG_PORT_MAC_LEARN_MODE        = 1,
    SYNCD_TX_DBG_CNTR_MSG_FLUSH_FDB                  = 2,
    SYNCD_TX_DBG_CNTR_MSG_SET_IF_MAC                 = 3,
    SYNCD_TX_DBG_CNTR_MSG_SET_FDB                    = 4,
    SYNCD_TX_DBG_CNTR_MSG_SET_TRAFFIC_DIST_ENABLE    = 5,
    SYNCD_TX_DBG_CNTR_MSG_SET_TRAFFIC_DIST_DISABLE   = 6,
    SYNCD_TX_DBG_CNTR_MSG_SET_ICCP_STATE             = 7,
    SYNCD_TX_DBG_CNTR_MSG_SET_ICCP_ROLE              = 8,
    SYNCD_TX_DBG_CNTR_MSG_SET_ICCP_SYSTEM_ID         = 9,
    SYNCD_TX_DBG_CNTR_MSG_DEL_ICCP_INFO              = 10,
    SYNCD_TX_DBG_CNTR_MSG_SET_REMOTE_IF_STATE        = 11,
    SYNCD_TX_DBG_CNTR_MSG_DEL_REMOTE_IF_INFO         = 12,
    SYNCD_TX_DBG_CNTR_MSG_PEER_LINK_ISOLATION        = 13,
    SYNCD_TX_DBG_CNTR_MSG_SET_ICCP_PEER_SYSTEM_ID    = 14,
    SYNCD_TX_DBG_CNTR_MSG_MAX
};

typedef uint8_t SYNCD_RX_DBG_CNTR_MSG_e;
enum SYNCD_RX_DBG_CNTR_MSG_e
{
    SYNCD_RX_DBG_CNTR_MSG_MAC = 0,
    SYNCD_RX_DBG_CNTR_MSG_CFG_MCLAG_DOMAIN = 1,
    SYNCD_RX_DBG_CNTR_MSG_CFG_MCLAG_IFACE  = 2,
    SYNCD_RX_DBG_CNTR_MSG_CFG_MCLAG_UNIQUE_IP = 3,
    SYNCD_RX_DBG_CNTR_MSG_VLAN_MBR_UPDATES = 4,
    SYNCD_RX_DBG_CNTR_MSG_MAX
};

/* Count messages ICCP daemon sent to MclagSyncd */
#define SYSTEM_SET_SYNCD_TX_DBG_COUNTER(sys, syncd_msg_type, status)\
do{\
    SYNCD_TX_DBG_CNTR_MSG_e dbg_type;\
    dbg_type = system_syncdtx_to_dbg_msg_type(syncd_msg_type);\
    if (sys && ((dbg_type) < SYNCD_TX_DBG_CNTR_MSG_MAX) && ((status) < SYNCD_DBG_CNTR_STS_MAX))\
    {\
      ++sys->dbg_counters.syncd_tx_counters[dbg_type][status];\
    }\
}while(0);

/* Count messages ICCP daemon received from MclagSyncd */
#define SYSTEM_SET_SYNCD_RX_DBG_COUNTER(sys, syncd_msg_type, status)\
do{\
    SYNCD_RX_DBG_CNTR_MSG_e dbg_type;\
    dbg_type = system_syncdrx_to_dbg_msg_type(syncd_msg_type);\
    if (sys && ((dbg_type) < SYNCD_RX_DBG_CNTR_MSG_MAX) && ((status) < SYNCD_DBG_CNTR_STS_MAX))\
    {\
        ++sys->dbg_counters.syncd_rx_counters[dbg_type][status];\
    }\
}while(0);

#define SYSTEM_INCR_SESSION_DOWN_COUNTER(sys)\
    if (sys)\
        ++sys->dbg_counters.session_down_counter;\

#define SYSTEM_GET_SESSION_DOWN_COUNTER(sys)\
    ( (sys) ? (sys)->dbg_counters.session_down_counter: 0)

#define SYSTEM_INCR_PEER_LINK_DOWN_COUNTER(sys)\
    if (sys)\
        ++sys->dbg_counters.peer_link_down_counter;\

#define SYSTEM_INCR_WARMBOOT_COUNTER(sys)\
    if (sys)\
        ++sys->dbg_counters.warmboot_counter;

#define SYSTEM_INCR_INVALID_PEER_MSG_COUNTER(sys)\
    if (sys)\
        ++sys->dbg_counters.rx_peer_invalid_msg_counter;

#define SYSTEM_GET_INVALID_PEER_MSG_COUNTER(sys)\
    ((sys) ? ((sys)->dbg_counters.rx_peer_invalid_msg_counter) ? 0)

#define SYSTEM_INCR_RX_READ_SOCK_ZERO_COUNTER(sys)\
   if (sys)\
       ++sys->dbg_counters.rx_read_sock_zero_len_counter;

#define SYSTEM_INCR_HDR_READ_SOCK_ERR_COUNTER(sys)\
    if (sys)\
        ++sys->dbg_counters.rx_peer_hdr_read_sock_err_counter;

#define SYSTEM_INCR_HDR_READ_SOCK_ZERO_LEN_COUNTER(sys)\
    if (sys)\
        ++sys->dbg_counters.rx_peer_hdr_read_sock_zero_len_counter;

#define SYSTEM_INCR_TLV_READ_SOCK_ERR_COUNTER(sys)\
    if (sys)\
        ++sys->dbg_counters.rx_peer_tlv_read_sock_err_counter;

#define SYSTEM_INCR_TLV_READ_SOCK_ZERO_LEN_COUNTER(sys)\
    if (sys)\
        ++sys->dbg_counters.rx_peer_tlv_read_sock_zero_len_counter;

#define SYSTEM_INCR_SOCKET_CLOSE_ERR_COUNTER(sys)\
    if (sys)\
        ++sys->dbg_counters.socket_close_err_counter;

#define SYSTEM_INCR_SOCKET_CLEANUP_COUNTER(sys)\
    if (sys)\
        ++sys->dbg_counters.socket_cleanup_counter;

#define SYSTEM_INCR_RX_RETRY_FAIL_COUNTER(sys)\
    if (sys)\
        ++sys->dbg_counters.rx_retry_fail_counter;

#define SYSTEM_INCR_MAC_ENTRY_ALLOC_COUNTER(sys)\
    if (sys)\
        ++sys->dbg_counters.mac_entry_alloc_counter;

#define SYSTEM_INCR_MAC_ENTRY_FREE_COUNTER(sys)\
    if (sys)\
        ++sys->dbg_counters.mac_entry_free_counter;

#define SYSTEM_INCR_RX_READ_SOCK_ZERO_COUNTER(sys)\
    if (sys)\
        ++sys->dbg_counters.rx_read_sock_zero_len_counter;

#define SYSTEM_INCR_RX_READ_SOCK_ERR_COUNTER(sys)\
    if (sys)\
        ++sys->dbg_counters.rx_read_sock_err_counter;

#define SYSTEM_INCR_RX_READ_STP_SOCK_ZERO_COUNTER(sys)\
    if (sys)\
        ++sys->dbg_counters.rx_read_stp_sock_zero_len_counter;

#define SYSTEM_INCR_RX_READ_STP_SOCK_ERR_COUNTER(sys)\
    if (sys)\
        ++sys->dbg_counters.rx_read_stp_sock_err_counter;
#define SYSTEM_SET_RETRY_COUNTER(sys, num_retry)\
    if (sys)\
    {\
        sys->dbg_counters.rx_retry_total_counter += num_retry;\
        if (num_retry > sys->dbg_counters.rx_retry_max_counter)\
            sys->dbg_counters.rx_retry_max_counter = num_retry;\
    }

#define SYSTEM_INCR_NETLINK_UNKNOWN_IF_NAME(ifname)\
do {\
    struct System *sys;\
    sys = system_get_instance();\
    if (sys)\
    {\
        ++sys->dbg_counters.unknown_if_name_count;\
        if (sys->dbg_counters.unknown_if_name_count < 10)\
        {\
            ICCPD_LOG_NOTICE("ICCP_FSM","NETLINK_COUNTER: Unknown if_name %s", ifname);\
        }\
    }\
}while(0)

#define SYSTEM_INCR_NETLINK_RX_ERROR()\
do {\
    struct System *sys;\
    sys = system_get_instance();\
    if (sys)\
        ++sys->dbg_counters.rx_error_count;\
}while(0)

typedef struct system_dbg_counter_info
{
    /* Netlink message counters */
    uint32_t newlink_count;
    uint32_t dellink_count;
    uint32_t newnbr_count;
    uint32_t delnbr_count;
    uint32_t newaddr_count;
    uint32_t deladdr_count;
    uint32_t unknown_type_count;
    uint32_t rx_error_count;

    /* Netlink link sub-message count */
    uint32_t unknown_if_name_count;

    /* Netlink neighbor sub-message count */
    uint32_t newmac_count;
    uint32_t delmac_count;

    uint32_t session_down_counter;    //not counting down due to warmboot
    uint32_t peer_link_down_counter;
    uint32_t warmboot_counter;
    uint32_t rx_peer_invalid_msg_counter; //counts partial msgs received as sending end is not sending partial msgs
    uint32_t rx_peer_hdr_read_sock_err_counter; //counts socket header read errors
    uint32_t rx_peer_hdr_read_sock_zero_len_counter; //counts socket header read zero length
    uint32_t rx_peer_tlv_read_sock_err_counter; //counts socket data/tlv read errors
    uint32_t rx_peer_tlv_read_sock_zero_len_counter; //counts socket data/tlv read zero length
    uint32_t socket_close_err_counter; //socket close failure
    uint32_t socket_cleanup_counter; //socket cleanup outside of session down
    uint32_t rx_retry_max_counter; //max non-blocking RX retry for one message
    uint32_t rx_retry_total_counter; //total number of non-blocking RX retry
    uint32_t rx_retry_fail_counter; //total number of non-blocking RX retry failure

    uint32_t rx_read_sock_zero_len_counter; //counts socket header read zero length from sync_fd
    uint32_t rx_read_sock_err_counter; //counts socket header read zero length from sync_fd
    uint32_t rx_read_stp_sock_zero_len_counter; //counts socket header read zero length from syncd
    uint32_t rx_read_stp_sock_err_counter; //counts socket header read zero length from syncd

    uint32_t mac_entry_alloc_counter;
    uint32_t mac_entry_free_counter;

    uint64_t syncd_tx_counters[SYNCD_TX_DBG_CNTR_MSG_MAX][SYNCD_DBG_CNTR_STS_MAX];
    uint64_t syncd_rx_counters[SYNCD_RX_DBG_CNTR_MSG_MAX][SYNCD_DBG_CNTR_STS_MAX];
}system_dbg_counter_info_t;

struct System
{
    int server_fd;/* Peer-Link Socket*/
    int sync_fd;
    int sync_ctrl_fd;
    int arp_receive_fd;
    int ndisc_receive_fd;
    int epoll_fd;

    struct nl_sock * genric_sock;
    int genric_sock_seq;
    int family;
    struct nl_sock * route_sock;
    int route_sock_seq;
    struct nl_sock * genric_event_sock;
    struct nl_sock * route_event_sock;

    int sig_pipe_r;
    int sig_pipe_w;
    int warmboot_start;
    int warmboot_exit;

    /* Info List*/
    LIST_HEAD(csm_list, CSM) csm_list;
    LIST_HEAD(lif_all_list, LocalInterface) lif_list;
    LIST_HEAD(lif_purge_all_list, LocalInterface) lif_purge_list;
    LIST_HEAD(unq_ip_all_if_list, Unq_ip_If_info) unq_ip_if_list;
    LIST_HEAD(pending_vlan_mbr_if_list, PendingVlanMbrIf) pending_vlan_mbr_if_list;

    /* Settings */
    char* log_file_path;
    char* cmd_file_path;
    char* config_file_path;
    char* mclagdctl_file_path;
    int pid_file_fd;
    int telnet_port;
    fd_set readfd; /*record socket need to listen*/
    int readfd_count;
    time_t csm_trans_time;
    int need_sync_team_again;
    int need_sync_netlink_again;

    /* ICCDd/MclagSyncd debug counters */
    system_dbg_counter_info_t dbg_counters;
};

struct CSM* system_create_csm();
struct CSM* system_get_csm_by_peer_ip(const char*);
struct CSM* system_get_csm_by_peer_ifname(char *ifname);
struct CSM* system_get_csm_by_mlacp_id(int id);
struct CSM* system_get_first_csm();
struct System* system_get_instance();
void system_finalize();
void system_init(struct System*);
SYNCD_TX_DBG_CNTR_MSG_e system_syncdtx_to_dbg_msg_type(uint32_t msg_type);
SYNCD_RX_DBG_CNTR_MSG_e system_syncdrx_to_dbg_msg_type(uint32_t msg_type);

char *mac_addr_to_str(uint8_t mac_addr[ETHER_ADDR_LEN]);
void system_update_netlink_counters(uint16_t netlink_msg_type, struct nlmsghdr *nlh);

#endif /* SYSTEM_H_ */
