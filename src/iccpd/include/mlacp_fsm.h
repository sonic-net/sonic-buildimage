/*
 *  mlacp_fsm.h
 *  mLACP finite state machine handler.
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

#ifndef _MLACP_FSM_H
#define _MLACP_FSM_H

#include "../include/port.h"
#include "../include/mlacp_tlv.h"

#define MLCAP_SYNC_PHY_DEV_SEC     1     /*every 1 sec*/

#define MLACP_LOCAL_IF_DOWN_TIMER 600  // 600 seconds.

#define MLACP(csm_ptr)  (csm_ptr->app_csm.mlacp)

struct CSM;

enum MLACP_APP_STATE
{
    MLACP_STATE_INIT,
    MLACP_STATE_STAGE1,
    MLACP_STATE_STAGE2,
    MLACP_STATE_EXCHANGE,
    MLACP_STATE_ERROR,
};

typedef enum MLACP_APP_STATE MLACP_APP_STATE_E;

/* for sender only*/
enum MLACP_SYNC_STATE
{
    MLACP_SYNC_SYSCONF = 0,
    MLACP_SYNC_AGGCONF,
    MLACP_SYNC_AGGSTATE,
    MLACP_SYNC_AGGINFO,
    MLACP_SYNC_PEERLINKINFO,
    MLACP_SYNC_ARP_INFO,
    MLACP_SYNC_NDISC_INFO,
    MLACP_SYNC_DONE,
};

typedef enum MLACP_SYNC_STATE MLACP_SYNC_STATE_E;

struct Remote_System
{
    uint8_t system_id[ETHER_ADDR_LEN];
    uint16_t system_priority;
    uint32_t node_id;
};

/****************************************************************
 * Debug counters to track message sent and received between
 * MC-LAG peers over ICCP
 ***************************************************************/
typedef uint8_t ICCP_DBG_CNTR_DIR_e;
enum ICCP_DBG_CNTR_DIR_e
{
    ICCP_DBG_CNTR_DIR_TX  = 0,
    ICCP_DBG_CNTR_DIR_RX  = 1,
    ICCP_DBG_CNTR_DIR_MAX
};

typedef uint8_t ICCP_DBG_CNTR_STS_e;
enum ICCP_DBG_CNTR_STS_e
{
    ICCP_DBG_CNTR_STS_OK  = 0,
    ICCP_DBG_CNTR_STS_ERR = 1,     /* Send error or receive processing error*/
    ICCP_DBG_CNTR_STS_MAX
};

/* Change MCLAGDCTL_MAX_DBG_COUNTERS if ICCP_DBG_CNTR_MSG_MAX is more than 32 */
enum ICCP_DBG_CNTR_MSG
{
    ICCP_DBG_CNTR_MSG_SYS_CONFIG       = 0,
    ICCP_DBG_CNTR_MSG_AGGR_CONFIG      = 1,
    ICCP_DBG_CNTR_MSG_AGGR_STATE       = 2,
    ICCP_DBG_CNTR_MSG_MAC_INFO         = 3,
    ICCP_DBG_CNTR_MSG_ARP_INFO         = 4,
    ICCP_DBG_CNTR_MSG_L2MC_INFO        = 5,
    ICCP_DBG_CNTR_MSG_PORTCHANNEL_INFO = 6,
    ICCP_DBG_CNTR_MSG_PEER_LINK_INFO   = 7,
    ICCP_DBG_CNTR_MSG_HEART_BEAT       = 8,
    ICCP_DBG_CNTR_MSG_NAK              = 9,
    ICCP_DBG_CNTR_MSG_SYNC_DATA        = 10,
    ICCP_DBG_CNTR_MSG_SYNC_REQ         = 11,
    ICCP_DBG_CNTR_MSG_WARM_BOOT        = 12,
    ICCP_DBG_CNTR_MSG_IF_UP_ACK        = 13,
    ICCP_DBG_CNTR_MSG_STP_CONNECT      = 14,
    ICCP_DBG_CNTR_MSG_STP_DISCONNECT   = 15,
    ICCP_DBG_CNTR_MSG_STP_SYSTEM_CONFIG = 16,
    ICCP_DBG_CNTR_MSG_STP_REGION_NAME  = 17,
    ICCP_DBG_CNTR_MSG_STP_REVISION_LEVEL = 18,
    ICCP_DBG_CNTR_MSG_STP_INSTANCE_PRIORITY = 19,
    ICCP_DBG_CNTR_MSG_STP_CONFIGURATION_DIGEST = 20,
    ICCP_DBG_CNTR_MSG_STP_TC_INSTANCES = 21,
    ICCP_DBG_CNTR_MSG_STP_ROOT_TIME_PARAM = 22,
    ICCP_DBG_CNTR_MSG_STP_MIST_ROOT_TIME_PARAM = 23,
    ICCP_DBG_CNTR_MSG_STP_SYNC_REQ     = 24,
    ICCP_DBG_CNTR_MSG_STP_SYNC_DATA    = 25,
    ICCP_DBG_CNTR_MSG_STP_PO_PORT_MAP  = 26,
    ICCP_DBG_CNTR_MSG_STP_AGE_OUT      = 27,
    ICCP_DBG_CNTR_MSG_STP_COMMON_MSG   = 28,
    ICCP_DBG_CNTR_MSG_MAX
};
typedef enum ICCP_DBG_CNTR_MSG ICCP_DBG_CNTR_MSG_e;

/* Count messages sent to MCLAG peer */
#define MLACP_SET_ICCP_TX_DBG_COUNTER(csm, tlv_type, status)\
do{\
    ICCP_DBG_CNTR_MSG_e dbg_type;\
    dbg_type = mlacp_fsm_iccp_to_dbg_msg_type(tlv_type);\
    if (csm && ((dbg_type) < ICCP_DBG_CNTR_MSG_MAX) && ((status) < ICCP_DBG_CNTR_STS_MAX))\
        ++MLACP(csm).dbg_counters.iccp_counters[dbg_type][ICCP_DBG_CNTR_DIR_TX][status];\
}while(0);

/* Count messages received from MCLAG peer */
#define MLACP_SET_ICCP_RX_DBG_COUNTER(csm, tlv_type, status)\
do{\
    ICCP_DBG_CNTR_MSG_e dbg_type;\
    dbg_type = mlacp_fsm_iccp_to_dbg_msg_type(tlv_type);\
    if (csm && ((dbg_type) < ICCP_DBG_CNTR_MSG_MAX) && ((status) < ICCP_DBG_CNTR_STS_MAX))\
        ++MLACP(csm).dbg_counters.iccp_counters[dbg_type][ICCP_DBG_CNTR_DIR_RX][status];\
}while(0);

typedef struct mlacp_dbg_counter_info
{
    uint64_t iccp_counters[ICCP_DBG_CNTR_MSG_MAX][ICCP_DBG_CNTR_DIR_MAX][ICCP_DBG_CNTR_STS_MAX];
}mlacp_dbg_counter_info_t;

struct mLACP
{
    int id;
    int sync_req_num;

    MLACP_APP_STATE_E current_state;
    MLACP_SYNC_STATE_E sync_state;

    uint8_t wait_for_sync_data;
    uint8_t need_to_sync;
    uint8_t node_id;
    uint8_t system_id[ETHER_ADDR_LEN];
    uint16_t system_priority;
    uint8_t system_config_changed;

    struct Remote_System remote_system;
    const char* error_msg;
    TAILQ_HEAD(mlacp_msg_list, Msg) mlacp_msg_list;
    TAILQ_HEAD(arp_msg_list, Msg) arp_msg_list;
    TAILQ_HEAD(arp_info_list, Msg) arp_list;
    TAILQ_HEAD(ndisc_msg_list, Msg) ndisc_msg_list;
    TAILQ_HEAD(ndisc_info_list, Msg) ndisc_list;
    TAILQ_HEAD(mac_msg_list, MACMsg) mac_msg_list;

    struct mac_rb_tree mac_rb;

    LIST_HEAD(lif_list, LocalInterface) lif_list;
    LIST_HEAD(lif_purge_list, LocalInterface) lif_purge_list;
    LIST_HEAD(pif_list, PeerInterface) pif_list;

    /* ICCP message tx/rx debug counters */
    mlacp_dbg_counter_info_t  dbg_counters;
};

void mlacp_init(struct CSM* csm, int all);
void mlacp_finalize(struct CSM* csm);
void mlacp_fsm_transit(struct CSM* csm);
void mlacp_enqueue_msg(struct CSM*, struct Msg*);
struct Msg* mlacp_dequeue_msg(struct CSM*);
char* mlacp_state(struct CSM* csm);

/* from app_csm*/
extern int mlacp_bind_local_if(struct CSM* csm, struct LocalInterface* local_if);
extern int mlacp_unbind_local_if(struct LocalInterface* local_if);

/* Debug counter API */
ICCP_DBG_CNTR_MSG_e mlacp_fsm_iccp_to_dbg_msg_type(uint32_t tlv_type);

#endif /* _MLACP_HANDLER_H */
