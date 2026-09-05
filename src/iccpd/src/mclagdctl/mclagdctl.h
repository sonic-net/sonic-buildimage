/* Copyright(c) 2016-2019 Nephos.
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
 *  Maintainer: Jim Jiang from nephos
 */
#include <stdint.h>
#include <stdbool.h>
#include "../../include/system.h"

#define ETHER_ADDR_LEN 6
#define MCLAGDCTL_PARA1_LEN 16
#define MCLAGDCTL_PARA2_LEN 32
#define MCLAGDCTL_PARA3_LEN 64
#define MCLAGDCTL_CMD_SIZE 4096

#define MCLAGDCTL_MAX_L_PORT_NANE 32
#define MCLAGDCTL_INET_ADDR_LEN 32
#define MCLAGDCTL_INET6_ADDR_LEN 64
#define MCLAGDCTL_ETHER_ADDR_LEN 6
#define MCLAGDCTL_PORT_MEMBER_BUF_LEN 512
#define ETHER_ADDR_STR_LEN 18

typedef int (*call_enca_msg_fun)(char *msg, int mclag_id,  int argc, char **argv);
typedef int (*call_parse_msg_fun)(char *msg, int data_len);

enum MAC_TYPE_CTL
{
    MAC_TYPE_STATIC_CTL     = 1,
    MAC_TYPE_DYNAMIC_CTL    = 2,
};

enum MAC_AGE_TYPE_CTL
{
    MAC_AGE_LOCAL_CTL   = 1,    /*MAC in local switch is ageout*/
    MAC_AGE_PEER_CTL    = 2     /*MAC in peer switch is ageout*/
};

enum id_command_type
{
    ID_CMDTYPE_NONE = 0,
    ID_CMDTYPE_D,
    ID_CMDTYPE_D_S,
    ID_CMDTYPE_D_A,
    ID_CMDTYPE_D_P,
    ID_CMDTYPE_D_P_L,
    ID_CMDTYPE_D_P_P,
    ID_CMDTYPE_D_D,
    ID_CMDTYPE_D_D_C,
    ID_CMDTYPE_C,
    ID_CMDTYPE_C_L,
    ID_CMDTYPE_C_D,
};

enum mclagdctl_notify_peer_type
{
    INFO_TYPE_NONE = 0,
    INFO_TYPE_DUMP_STATE,
    INFO_TYPE_DUMP_ARP,
    INFO_TYPE_DUMP_NDISC,
    INFO_TYPE_DUMP_MAC,
    INFO_TYPE_DUMP_UNIQUE_IP,
    INFO_TYPE_DUMP_LOCAL_PORTLIST,
    INFO_TYPE_DUMP_PEER_PORTLIST,
    INFO_TYPE_DUMP_DBG_COUNTERS,
    INFO_TYPE_CONFIG_LOGLEVEL,
    INFO_TYPE_CONFIG_DOWN,
    INFO_TYPE_FINISH,
};

enum log_level_type
{
    CRITICAL = 0,
    ERR= 1,
    WARN = 2,
    NOTICE= 3,
    INFO = 4,
    DEBUG = 5
};

struct mclagdctl_req_hdr
{
    int info_type;
    int mclag_id;
    char para1[MCLAGDCTL_PARA2_LEN];
    char para2[MCLAGDCTL_PARA2_LEN];
    char para3[MCLAGDCTL_PARA2_LEN];
};

struct mclagd_reply_hdr
{
    int info_type;
    int data_len;
    int exec_result;
};

#define EXEC_TYPE_SUCCESS  -1
#define EXEC_TYPE_NO_EXIST_SYS  -2
#define EXEC_TYPE_NO_EXIST_MCLAGID  -3
#define EXEC_TYPE_FAILED -4

#define MCLAG_ERROR -1

#define MCLAGD_REPLY_INFO_HDR (sizeof(struct mclagd_reply_hdr) + sizeof(int))

#define MCLAGDCTL_COMMAND_PARAM_MAX_CNT 8
struct command_type
{
    enum id_command_type id;
    enum id_command_type parent_id;
    enum mclagdctl_notify_peer_type info_type;
    char *name;
    char *params[MCLAGDCTL_COMMAND_PARAM_MAX_CNT];
    call_enca_msg_fun enca_msg;
    call_parse_msg_fun parse_msg;
};

struct mclagd_state
{
    int mclag_id;
    int keepalive;
    int info_sync_done;
    char local_ip[MCLAGDCTL_INET_ADDR_LEN];
    char peer_ip[MCLAGDCTL_INET_ADDR_LEN];
    char peer_link_if[MCLAGDCTL_MAX_L_PORT_NANE];
    unsigned char peer_link_mac[MCLAGDCTL_ETHER_ADDR_LEN];
    int role;
    char enabled_po[MCLAGDCTL_PORT_MEMBER_BUF_LEN];
    int session_timeout;
    int keepalive_time;
    char loglevel[MCLAGDCTL_PARA1_LEN];
};

#define NEIGH_LOCAL   1
#define NEIGH_REMOTE  2

struct mclagd_arp_msg
{
    char op_type;
    char ifname[MCLAGDCTL_MAX_L_PORT_NANE];
    char ipv4_addr[MCLAGDCTL_INET_ADDR_LEN];
    unsigned char mac_addr[MCLAGDCTL_ETHER_ADDR_LEN];
    uint8_t learn_flag;
};

struct mclagd_ndisc_msg
{
    char op_type;
    char ifname[MCLAGDCTL_MAX_L_PORT_NANE];
    char ipv6_addr[MCLAGDCTL_INET6_ADDR_LEN];
    unsigned char mac_addr[MCLAGDCTL_ETHER_ADDR_LEN];
    uint8_t learn_flag;
};

struct mclagd_mac_msg
{
    unsigned char     op_type;/*add or del*/
    unsigned char     fdb_type;/*static or dynamic*/
    uint8_t     mac_addr[ETHER_ADDR_LEN];
    unsigned short vid;
    /*Current if name that set in chip*/
    char     ifname[MCLAGDCTL_MAX_L_PORT_NANE];
    /*if we set the mac to peer-link, origin_ifname store the
       original if name that learned from chip*/
    char     origin_ifname[MCLAGDCTL_MAX_L_PORT_NANE];
    unsigned char age_flag;/*local or peer is age?*/
};

struct mclagd_local_if
{
    int ifindex;
    char type[MCLAGDCTL_PARA1_LEN];
    char name[MCLAGDCTL_MAX_L_PORT_NANE];
    unsigned char mac_addr[MCLAGDCTL_ETHER_ADDR_LEN];
    char  state[MCLAGDCTL_PARA1_LEN];
    char ipv4_addr[MCLAGDCTL_INET_ADDR_LEN];
    unsigned char prefixlen;

    unsigned char l3_mode;
    unsigned char is_peer_link;
    char portchannel_member_buf[MCLAGDCTL_PORT_MEMBER_BUF_LEN];
    int po_id; /* Port Channel ID */
    unsigned char po_active;
    char mlacp_state[MCLAGDCTL_PARA1_LEN];
    unsigned char isolate_to_peer_link;
    bool is_traffic_disable;
    char vlanlist[MCLAGDCTL_PARA3_LEN];
};

struct mclagd_peer_if
{
    int ifindex;
    unsigned char type[MCLAGDCTL_PARA1_LEN];
    char name[MCLAGDCTL_MAX_L_PORT_NANE];
    unsigned char mac_addr[MCLAGDCTL_ETHER_ADDR_LEN];
    unsigned char  state[MCLAGDCTL_PARA1_LEN];
    int po_id;
    unsigned char po_active;
};

typedef struct mclagd_dbg_counter_info
{
    system_dbg_counter_info_t system_dbg;
    uint8_t    num_iccp_counter_blocks;
    uint8_t    iccp_dbg_counters[0];
    /* Variable length counter buffers containing N mlacp_dbg_counter_info_t
     */
}mclagd_dbg_counter_info_t;

struct mclagd_unique_ip_if
{
    int active;
    char name[MCLAGDCTL_MAX_L_PORT_NANE];
};

extern int mclagdctl_enca_dump_state(char *msg, int mclag_id,  int argc, char **argv);
extern int mclagdctl_parse_dump_state(char *msg, int data_len);
extern int mclagdctl_enca_dump_arp(char *msg, int mclag_id, int argc, char **argv);
extern int mclagdctl_enca_dump_ndisc(char *msg, int mclag_id, int argc, char **argv);
extern int mclagdctl_parse_dump_arp(char *msg, int data_len);
extern int mclagdctl_parse_dump_ndisc(char *msg, int data_len);
extern int mclagdctl_enca_dump_mac(char *msg, int mclag_id, int argc, char **argv);
extern int mclagdctl_parse_dump_mac(char *msg, int data_len);
extern int mclagdctl_enca_dump_local_portlist(char *msg, int mclag_id,  int argc, char **argv);
extern int mclagdctl_parse_dump_local_portlist(char *msg, int data_len);
extern int mclagdctl_enca_dump_peer_portlist(char *msg, int mclag_id,  int argc, char **argv);
extern int mclagdctl_parse_dump_peer_portlist(char *msg, int data_len);
int mclagdctl_enca_config_loglevel(char *msg, int log_level,  int argc, char **argv);
int mclagdctl_parse_config_loglevel(char *msg, int data_len);

extern int mclagdctl_enca_dump_dbg_counters(char *msg, int mclag_id, int argc, char **argv);
extern int mclagdctl_parse_dump_dbg_counters(char *msg, int data_len);
extern int mclagdctl_enca_dump_unique_ip(char *msg, int mclag_id, int argc, char **argv);
extern int mclagdctl_parse_dump_unique_ip(char *msg, int data_len);
