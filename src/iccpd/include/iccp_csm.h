/*
 * iccp_csm.h
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

#ifndef ICCP_CSM_H_
#define ICCP_CSM_H_

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include "../include/app_csm.h"
#include "../include/msg_format.h"
#include "../include/port.h"

#define CSM_BUFFER_SIZE 65536

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif /*IFNAMSIZ*/

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif                          /* INET_ADDRSTRLEN */
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif                          /* INET6_ADDRSTRLEN */
/* For socket binding */
#define ICCP_TCP_PORT 8888
#define MAX_ACCEPT_CONNETIONS 20

/* LDP message ID */
extern uint32_t ICCP_MSG_ID;

/* Global Buffer */
extern char g_csm_buf[CSM_BUFFER_SIZE];

struct IccpInfo
{
    uint32_t icc_rg_id;
    char sender_name[MAX_L_ICC_SENDER_NAME];
    uint32_t status_code;
    uint8_t peer_capability_flag : 1;
    uint8_t peer_rg_connect_flag : 1;
    uint8_t sender_capability_flag : 1;
    uint8_t sender_rg_connect_flag : 1;
    uint32_t rejected_msg_id;
};

/* Receive message node */
struct Msg
{
    char* buf;
    size_t len;
    TAILQ_ENTRY(Msg) tail;
};

/* Connection state */
enum ICCP_CONNECTION_STATE
{
    ICCP_NONEXISTENT,
    ICCP_INITIALIZED,
    ICCP_CAPSENT,
    ICCP_CAPREC,
    ICCP_CONNECTING,
    ICCP_OPERATIONAL
};

typedef enum ICCP_CONNECTION_STATE ICCP_CONNECTION_STATE_E;

typedef enum stp_role_type_e
{
    STP_ROLE_NONE,      /* mstp do nothing*/
    STP_ROLE_ACTIVE,    /* mstp report port state*/
    STP_ROLE_STANDBY    /* mstp fwd bpdu & set port state*/
} stp_role_type_et;

/* Connection state machine instance */
struct CSM
{
    int mlag_id;

    /* Socket info */
    int sock_fd;
    pthread_mutex_t conn_mutex;
    time_t connTimePrev;
    time_t heartbeat_send_time;
    time_t heartbeat_update_time;
    time_t peer_warm_reboot_time;
    time_t warm_reboot_disconn_time;
    char peer_itf_name[IFNAMSIZ];
    time_t peer_link_learning_retry_time;
    char peer_ip[INET_ADDRSTRLEN];
    char sender_ip[INET_ADDRSTRLEN];
    void* sock_read_event_ptr;

    int keepalive_time;
    int session_timeout;
    int peer_link_learning_enable;

    /* Msg queue */
    TAILQ_HEAD(msg_list, Msg) msg_list;

    /* STP role */
    stp_role_type_et role_type;

    /* Peers msg */
    struct LocalInterface* peer_link_if;
    struct IccpInfo iccp_info;
    struct AppCSM app_csm;
    ICCP_CONNECTION_STATE_E current_state;

    /* Statistic info */
    uint64_t icc_msg_in_count;  /* ICC message input count */
    uint64_t icc_msg_out_count; /* ICC message Output count */
    uint64_t u_msg_in_count;    /* Unknown message Input count */
    uint64_t i_msg_in_count;    /* Illegal message Input count */

    /* Log */
    struct MsgLog msg_log;

    LIST_ENTRY(CSM) next;
    LIST_HEAD(csm_if_list, If_info) if_bind_list;
};
int iccp_csm_send(struct CSM*, char*, int);
int iccp_csm_init_msg(struct Msg**, char*, int);
int iccp_csm_prepare_nak_msg(struct CSM*, char*, size_t);
int iccp_csm_prepare_iccp_msg(struct CSM*, char*, size_t);
int iccp_csm_prepare_capability_msg(struct CSM*, char*, size_t);
int iccp_csm_prepare_rg_connect_msg(struct CSM*, char*, size_t);
int iccp_csm_prepare_rg_disconnect_msg(struct CSM*, char*, size_t);
struct Msg* iccp_csm_dequeue_msg(struct CSM*);
void *iccp_get_csm();
void iccp_csm_init(struct CSM*);
void iccp_csm_transit(struct CSM*);
void iccp_csm_finalize(struct CSM*);
void iccp_csm_status_reset(struct CSM*, int);
void iccp_csm_stp_role_count(struct CSM *csm);
void iccp_csm_msg_list_finalize(struct CSM*);
void iccp_csm_enqueue_msg(struct CSM*, struct Msg*);
void iccp_csm_fill_icc_rg_id_tlv(struct CSM*, ICCHdr*);
void iccp_csm_correspond_from_msg(struct CSM*, struct Msg*);
void iccp_csm_correspond_from_capability_msg(struct CSM*, struct Msg*);
void iccp_csm_correspond_from_rg_connect_msg(struct CSM*, struct Msg*);
void iccp_csm_correspond_from_rg_disconnect_msg(struct CSM*, struct Msg*);

int mlacp_bind_port_channel_to_csm(struct CSM* csm, const char *ifname);
int iccp_csm_init_mac_msg(struct MACMsg **mac_msg, char* data, int len);
#endif /* ICCP_CSM_H_ */
