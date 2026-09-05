/*
 * iccp_csm.c
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
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../include/logger.h"
#include "../include/system.h"
#include "../include/scheduler.h"
#include "../include/msg_format.h"
#include "../include/iccp_csm.h"
#include "../include/iccp_cli.h"
#include "../include/mlacp_link_handler.h"
/*****************************************
* Define
*
* ***************************************/
#define ICCP_CSM_QUEUE_REINIT(list) \
    { \
        struct Msg* msg = NULL; \
        while (!TAILQ_EMPTY(&(list))) { \
            msg = TAILQ_FIRST(&(list)); \
            TAILQ_REMOVE(&(list), msg, tail); \
            free(msg->buf); \
            free(msg); \
        } \
        TAILQ_INIT(&(list)); \
    }

/*****************************************
* Global
*
* ***************************************/
char g_csm_buf[CSM_BUFFER_SIZE] = { 0 };

uint32_t ICCP_MSG_ID = 0x1;

/* Enter Connection State Machine NONEXISTENT handle function */
static void iccp_csm_enter_state_nonexistent(struct CSM* csm)
{
    iccp_csm_finalize(csm);
}

/* Enter Connection State Machine INITIALIZED handle function */
static void iccp_csm_enter_state_initialized(struct CSM* csm)
{
    if (csm == NULL)
        return;

    csm->iccp_info.sender_capability_flag = 0x1;
}

/* Enter Connection State Machine CAPREC handle function */
static void iccp_csm_enter_state_caprec(struct CSM* csm)
{
    if (csm == NULL)
        return;

    csm->iccp_info.sender_capability_flag = 0x1;
    csm->iccp_info.peer_capability_flag = 0x1;
}

/* Enter Connection State Machine CONNECTING handle function */
static void iccp_csm_enter_state_connecting(struct CSM* csm)
{
    if (csm == NULL)
        return;

    csm->iccp_info.sender_rg_connect_flag = 0x1;
}

/* Enter Connection State Machine OPERATIONAL handle function */
static void iccp_csm_enter_state_operational(struct CSM* csm)
{
    if (csm == NULL)
        return;

    csm->iccp_info.sender_rg_connect_flag = 0x1;
    csm->iccp_info.peer_rg_connect_flag = 0x1;
}

void *iccp_get_csm()
{
    struct CSM* csm = NULL;
    struct System* sys = NULL;

    if ((sys = system_get_instance()) == NULL)
    {
        return NULL;
    }

    csm = system_create_csm();

    return csm;
}

/* Connection State Machine instance initialization */
void iccp_csm_init(struct CSM* csm)
{
    iccp_csm_status_reset(csm, 1);
    memset(csm->sender_ip, 0, INET_ADDRSTRLEN);
    memset(csm->peer_ip, 0, INET_ADDRSTRLEN);
    memset(csm->iccp_info.sender_name, 0, MAX_L_ICC_SENDER_NAME);
    csm->iccp_info.icc_rg_id = 0x0;
    csm->keepalive_time      = CONNECT_INTERVAL_SEC;
    csm->session_timeout     = HEARTBEAT_TIMEOUT_SEC;
}

/* Connection State Machine instance status reset */
void iccp_csm_status_reset(struct CSM* csm, int all)
{
    ICCP_CSM_QUEUE_REINIT(csm->msg_list);

    if (all)
    {
        bzero(csm, sizeof(struct CSM));
        ICCP_CSM_QUEUE_REINIT(csm->msg_list);
    }

    csm->sock_fd = -1;
    pthread_mutex_init(&csm->conn_mutex, NULL);
    csm->connTimePrev = 0;
    csm->heartbeat_send_time = 0;
    csm->heartbeat_update_time = 0;
    csm->peer_warm_reboot_time = 0;
    csm->warm_reboot_disconn_time = 0;
    csm->peer_link_learning_retry_time = 0;
    csm->peer_link_learning_enable = 0;
    csm->role_type = STP_ROLE_NONE;
    csm->sock_read_event_ptr = NULL;
    csm->peer_link_if = NULL;
    csm->u_msg_in_count = 0x0;
    csm->i_msg_in_count = 0x0;
    csm->icc_msg_in_count = 0x0;
    csm->icc_msg_out_count = 0x0;
    csm->iccp_info.status_code = 0x0;
    csm->iccp_info.rejected_msg_id = 0x0;
    csm->current_state = ICCP_NONEXISTENT;
    csm->iccp_info.peer_capability_flag = 0x0;
    csm->iccp_info.peer_rg_connect_flag = 0x0;
    csm->iccp_info.sender_capability_flag = 0x0;
    csm->iccp_info.sender_rg_connect_flag = 0x0;
    app_csm_init(csm, all);

    memset(&csm->msg_log, 0, sizeof(struct MsgLog));
}

/* Connection State Machine instance tear down */
void iccp_csm_finalize(struct CSM* csm)
{
    struct If_info * cif = NULL;
    struct System* sys = NULL;
    struct LocalInterface* lifp = NULL;
    struct LocalInterface* local_if = NULL;
    struct LocalInterface* local_if_next = NULL;

    if (csm == NULL)
        return;

    if ((sys = system_get_instance()) == NULL)
        return;

    /*If warm reboot, don't change port block and peer link MAC learning*/
    if (sys->warmboot_exit != WARM_REBOOT)
    {
        /*Enable peer link port MAC learning*/
        if (csm->peer_link_if) {
            set_peerlink_mlag_port_learn(csm->peer_link_if, 1);
            set_peerlink_learn_kernel(csm, 1, 1);
        }
    }

    /* Disconnect from peer */
    scheduler_session_disconnect_handler(csm);

    //Delete all MCLAG interfaces
    local_if = LIST_FIRST(&(MLACP(csm).lif_list));
    while (local_if != NULL)
    {
        local_if_next =  LIST_NEXT(local_if,mlacp_next);
        iccp_cli_detach_mclag_domain_to_port_channel(local_if->name);
        local_if = local_if_next;
    }

    /* Release all Connection State Machine instance */
    app_csm_finalize(csm);

    LIST_FOREACH(lifp, &(MLACP(csm).lif_list), mlacp_next)
    {
        ICCPD_LOG_NOTICE(__FUNCTION__, "Resetting CSM pointer for %s", lifp->name);
        lifp->csm = NULL;
    }

    while (!LIST_EMPTY(&(csm->if_bind_list)))
    {
        cif = LIST_FIRST(&(csm->if_bind_list));
        LIST_REMOVE(cif,csm_next);
        free(cif);
    }

    /* Release iccp_csm */
    pthread_mutex_destroy(&(csm->conn_mutex));
    iccp_csm_msg_list_finalize(csm);
    LIST_REMOVE(csm, next);
    free(csm);
}

/* Message list of Connection State Machine instance tear down */
void iccp_csm_msg_list_finalize(struct CSM* csm)
{
    struct Msg* msg = NULL;

    if (csm == NULL)
        return;

    while (!TAILQ_EMPTY(&(csm->msg_list)))
    {
        msg = TAILQ_FIRST(&(csm->msg_list));
        TAILQ_REMOVE(&(csm->msg_list), msg, tail);
        free(msg);
    }
}

/* Send message to peer */
int iccp_csm_send(struct CSM* csm, char* buf, int msg_len)
{
    LDPHdr* ldp_hdr = (LDPHdr*)buf;
    ICCParameter* param = NULL;
    ssize_t rc;
    uint16_t tlv_type;

    if (csm == NULL || buf == NULL || csm->sock_fd <= 0 || msg_len <= 0)
        return MCLAG_ERROR;

    if (ntohs(ldp_hdr->msg_type) == MSG_T_CAPABILITY)
        param = (struct ICCParameter*)&buf[sizeof(LDPHdr)];
    else
        param = (struct ICCParameter*)&buf[sizeof(ICCHdr)];

    ICCPD_LOG_DEBUG(__FUNCTION__, "Send(%d): len=[%d] msg_type=[%s (0x%X, 0x%X)]", csm->sock_fd, msg_len, get_tlv_type_string(ntohs(param->type)), ldp_hdr->msg_type, ntohs(param->type));
    csm->msg_log.msg[csm->msg_log.end_index].msg_id = ntohl(ldp_hdr->msg_id);
    csm->msg_log.msg[csm->msg_log.end_index].type = ntohs(ldp_hdr->msg_type);
    csm->msg_log.msg[csm->msg_log.end_index].tlv = ntohs(param->type);
    ++csm->msg_log.end_index;
    if (csm->msg_log.end_index >= 128)
        csm->msg_log.end_index = 0;

    tlv_type = ntohs(param->type);
    rc = write(csm->sock_fd, buf, msg_len);
    if ((rc <= 0) || (rc != msg_len))
    {
        MLACP_SET_ICCP_TX_DBG_COUNTER(
            csm, tlv_type, ICCP_DBG_CNTR_STS_ERR);
        ICCPD_LOG_ERR("ICCP_FSM", "Failed to write msg %s/0x%x, msg_len:%d, rc %d Error:%s ", get_tlv_type_string(tlv_type), tlv_type, msg_len, rc, strerror(errno));
    }
    else
    {
        MLACP_SET_ICCP_TX_DBG_COUNTER(
            csm, tlv_type, ICCP_DBG_CNTR_STS_OK);
    }
    return (rc);
}

/* Connection State Machine Transition */
void iccp_csm_transit(struct CSM* csm)
{
    int len = -1;
    struct Msg* msg = NULL;
    ICCP_CONNECTION_STATE_E prev_state;
    char *state_str[] = {"NONEXISTENT", "INITIALIZED", "CAPSENT", "CAPREC", "CONNECTING", "OPERATIONAL"};

    if (!csm)
        return;

    prev_state = csm->current_state;

    /* No connection, but have state change? reset it...*/
    if (csm->current_state != ICCP_NONEXISTENT && csm->sock_fd <= 0)
    {
        ICCPD_LOG_NOTICE(__FUNCTION__, "csm %d change state from %s to NONEXISTENT.", csm->mlag_id, state_str[csm->current_state]);
        csm->current_state = ICCP_NONEXISTENT;
        iccp_csm_enter_state_nonexistent(csm);
        return;
    }

    msg = iccp_csm_dequeue_msg(csm);

    switch (csm->current_state)
    {
        case ICCP_NONEXISTENT:
            scheduler_prepare_session(csm);
            if (csm->sock_fd > 0 && scheduler_check_csm_config(csm) > 0)
                csm->current_state = ICCP_INITIALIZED;
            break;

        case ICCP_INITIALIZED:
            if (msg)
                iccp_csm_correspond_from_msg(csm, msg);
            len = iccp_csm_prepare_iccp_msg(csm, g_csm_buf, CSM_BUFFER_SIZE);
            iccp_csm_send(csm, g_csm_buf, len);
            if (csm->iccp_info.sender_capability_flag == 0x1 && csm->iccp_info.peer_capability_flag == 0x0)
                csm->current_state = ICCP_CAPSENT;
            else if (csm->iccp_info.sender_capability_flag == 0x1 && csm->iccp_info.peer_capability_flag == 0x1)
                csm->current_state = ICCP_CAPREC;
            break;

        case ICCP_CAPSENT:
            if (msg)
                iccp_csm_correspond_from_msg(csm, msg);
            if (csm->iccp_info.sender_capability_flag == 0x1 && csm->iccp_info.peer_capability_flag == 0x1)
                csm->current_state = ICCP_CAPREC;
            break;

        case ICCP_CAPREC:
            if (msg)
                iccp_csm_correspond_from_msg(csm, msg);
            memset(g_csm_buf, 0, CSM_BUFFER_SIZE);
            len = iccp_csm_prepare_iccp_msg(csm, g_csm_buf, CSM_BUFFER_SIZE);
            iccp_csm_send(csm, g_csm_buf, len);
            if (csm->iccp_info.peer_rg_connect_flag == 0x0 && csm->iccp_info.status_code == 0x0)
                csm->current_state = ICCP_CONNECTING;
            else if (csm->iccp_info.peer_rg_connect_flag == 0x1 && csm->iccp_info.status_code == 0x0)
                csm->current_state = ICCP_OPERATIONAL;
            break;

        case ICCP_CONNECTING:
            if (msg)
                iccp_csm_correspond_from_msg(csm, msg);
            memset(g_csm_buf, 0, CSM_BUFFER_SIZE);
            len = iccp_csm_prepare_iccp_msg(csm, g_csm_buf, CSM_BUFFER_SIZE);
            iccp_csm_send(csm, g_csm_buf, len);
            if (csm->iccp_info.status_code > 0x0)
                csm->current_state = ICCP_CAPREC;
            else if (csm->iccp_info.peer_rg_connect_flag == 0x1 && csm->iccp_info.status_code == 0x0)
                csm->current_state = ICCP_OPERATIONAL;
            break;

        case ICCP_OPERATIONAL:
            if (msg)
                iccp_csm_correspond_from_msg(csm, msg);
            if (csm->iccp_info.sender_rg_connect_flag == 0x0 || csm->iccp_info.peer_rg_connect_flag == 0x0)
            {
                memset(g_csm_buf, 0, CSM_BUFFER_SIZE);
                len = iccp_csm_prepare_iccp_msg(csm, g_csm_buf, CSM_BUFFER_SIZE);
                iccp_csm_send(csm, g_csm_buf, len);
                csm->current_state = ICCP_CAPREC;
            }
            break;

        default:
            break;
    }

    if (prev_state != csm->current_state || (csm->current_state && msg != NULL))
    {
        if (prev_state != csm->current_state)
            ICCPD_LOG_NOTICE(__FUNCTION__, "csm %d change state from %s to %s.", csm->mlag_id, state_str[prev_state], state_str[csm->current_state]);

        switch (csm->current_state)
        {
            case ICCP_NONEXISTENT:
                iccp_csm_enter_state_nonexistent(csm);
                break;

            case ICCP_INITIALIZED:
                iccp_csm_enter_state_initialized(csm);
                break;

            case ICCP_CAPSENT:
                /* Do nothing on this state */
                break;

            case ICCP_CAPREC:
                iccp_csm_enter_state_caprec(csm);
                break;

            case ICCP_CONNECTING:
                iccp_csm_enter_state_connecting(csm);
                break;

            case ICCP_OPERATIONAL:
                iccp_csm_enter_state_operational(csm);
                break;

            default:
                break;
        }
    }
}

/* Set up ICCP message */
int iccp_csm_prepare_iccp_msg(struct CSM* csm, char* buf, size_t max_buf_size)
{
    size_t msg_len = -1;

    if (csm == NULL || buf == NULL)
        return MCLAG_ERROR;

    switch (csm->current_state)
    {
        case ICCP_NONEXISTENT:
            /* Do nothing on this state */
            break;

        case ICCP_INITIALIZED:
            msg_len = iccp_csm_prepare_capability_msg(csm, buf, max_buf_size);
            break;

        case ICCP_CAPSENT:
            /* Do nothing on this state */
            break;

        case ICCP_CAPREC:
            if (csm->iccp_info.status_code > 0x0)
            {
                msg_len = iccp_csm_prepare_nak_msg(csm, buf, max_buf_size);
                break;
            }
            msg_len = iccp_csm_prepare_rg_connect_msg(csm, buf, max_buf_size);
            break;

        case ICCP_CONNECTING:
            if (csm->iccp_info.status_code > 0x0)
            {
                msg_len = iccp_csm_prepare_nak_msg(csm, buf, max_buf_size);
                break;
            }
            break;

        case ICCP_OPERATIONAL:
            if (csm->iccp_info.peer_rg_connect_flag == 0x0)
            {
                msg_len = iccp_csm_prepare_rg_disconnect_msg(csm, buf, max_buf_size);
                break;
            }
            break;
    }

    return msg_len;
}

/* ICCP capability message handle function */
int iccp_csm_prepare_capability_msg(struct CSM* csm, char* buf, size_t max_buf_size)
{
    LDPHdr* ldp_hdr = (LDPHdr*)buf;
    LDPICCPCapabilityTLV* cap = (LDPICCPCapabilityTLV*)&buf[sizeof(LDPHdr)];
    size_t msg_len = sizeof(LDPHdr) + sizeof(LDPICCPCapabilityTLV);

    memset(buf, 0, max_buf_size);

    /* LDP header */
    ldp_hdr->u_bit = 0x0;
    ldp_hdr->msg_type = htons(MSG_T_CAPABILITY);
    ldp_hdr->msg_len = htons(msg_len - MSG_L_INCLUD_U_BIT_MSG_T_L_FIELDS);
    ldp_hdr->msg_id = htonl(ICCP_MSG_ID++);

    /* LDP ICCP capability TLV */
    cap->icc_parameter.u_bit = 0x1;
    cap->icc_parameter.f_bit = 0x0;
    cap->icc_parameter.type = TLV_T_ICCP_CAPABILITY;
    *(uint16_t *)cap = htons(*(uint16_t *)cap);

    cap->icc_parameter.len = htons(TLV_L_ICCP_CAPABILITY);

    cap->s_bit = csm->iccp_info.sender_capability_flag;
    *(uint16_t *)((uint8_t *)cap + sizeof(ICCParameter)) = htons(*(uint16_t *)((uint8_t *)cap + sizeof(ICCParameter)));

    cap->major_ver = 0x1;
    cap->minior_ver = 0x0;

    return msg_len;
}

void iccp_csm_fill_icc_rg_id_tlv(struct CSM* csm, ICCHdr* icc_hdr)
{
    if (!csm || !icc_hdr)
        return;

    icc_hdr->icc_rg_id_tlv.type = htons(TLV_T_ICC_RG_ID);
    icc_hdr->icc_rg_id_tlv.len = htons(TLV_L_ICC_RG_ID);
    icc_hdr->icc_rg_id_tlv.icc_rg_id = htonl(csm->iccp_info.icc_rg_id);
}

/* ICCP NAK message handle function */
int iccp_csm_prepare_nak_msg(struct CSM* csm, char* buf, size_t max_buf_size)
{
    ICCHdr* icc_hdr = (ICCHdr*)buf;
    NAKTLV* nak = (NAKTLV*)&buf[sizeof(ICCHdr)];
    size_t msg_len = sizeof(ICCHdr) + sizeof(NAKTLV);

    memset(buf, 0, max_buf_size);

    /* ICC header */
    icc_hdr->ldp_hdr.u_bit = 0x0;
    icc_hdr->ldp_hdr.msg_type = htons(MSG_T_NOTIFICATION);
    icc_hdr->ldp_hdr.msg_len = htons(msg_len - MSG_L_INCLUD_U_BIT_MSG_T_L_FIELDS);
    icc_hdr->ldp_hdr.msg_id = htonl(ICCP_MSG_ID++);
    iccp_csm_fill_icc_rg_id_tlv(csm, icc_hdr);

    /* NAL TLV */
    nak->icc_parameter.u_bit = 0x0;
    nak->icc_parameter.f_bit = 0x0;
    nak->icc_parameter.type = htons(TLV_T_NAK);
    nak->icc_parameter.len = htons(sizeof(((struct NAKTLV*)0)->iccp_status_code) + sizeof(((struct NAKTLV*)0)->rejected_msg_id));

    switch (csm->iccp_info.status_code)
    {
        case STATUS_CODE_U_ICCP_RG:
            nak->iccp_status_code = htonl(csm->iccp_info.status_code);
            nak->rejected_msg_id = htonl(csm->iccp_info.rejected_msg_id);
            break;

        /* Unsupported */
        case STATUS_CODE_ICCP_CONNECTION_COUNT_EXCEEDED:
        case STATUS_CODE_ICCP_APP_CONNECTION_COUNT_EXCEEDED:
        case STATUS_CODE_ICCP_APP_NOT_IN_RG:
        case STATUS_CODE_INCOMPATIBLE_ICCP_PROTOCOL_VER:
        case STATUS_CODE_ICCP_REJECTED_MSG:
        case STATUS_CODE_ICCP_ADMINISTRATIVELY_DISABLED:
        case STATUS_CODE_ICCP_RG_REMOVED:
        case STATUS_CODE_ICCP_APP_REMOVED_FROM_RG:
            break;
    }

    return msg_len;
}

/* ICCP RG connect handle function */
int iccp_csm_prepare_rg_connect_msg(struct CSM* csm, char* buf, size_t max_buf_size)
{
    ICCHdr* icc_hdr = (ICCHdr*)buf;
    ICCSenderNameTLV* sender = (ICCSenderNameTLV*)&buf[sizeof(ICCHdr)];
    size_t name_len = strlen(csm->iccp_info.sender_name);
    size_t msg_len = sizeof(ICCHdr) + sizeof(ICCParameter) + name_len;

    memset(buf, 0, max_buf_size);

    /* ICC header */
    icc_hdr->ldp_hdr.u_bit = 0x0;
    icc_hdr->ldp_hdr.msg_type = htons(MSG_T_RG_CONNECT);
    icc_hdr->ldp_hdr.msg_len = htons(msg_len - MSG_L_INCLUD_U_BIT_MSG_T_L_FIELDS);
    icc_hdr->ldp_hdr.msg_id = htonl(ICCP_MSG_ID++);
    iccp_csm_fill_icc_rg_id_tlv(csm, icc_hdr);

    /* ICC sender name TLV */
    sender->icc_parameter.u_bit = 0x0;
    sender->icc_parameter.f_bit = 0x0;
    sender->icc_parameter.type = htons(TLV_T_ICC_SENDER_NAME);
    sender->icc_parameter.len = htons(name_len);
    memcpy(sender->sender_name, csm->iccp_info.sender_name, name_len);

    return msg_len;
}

/* ICCP RG disconnect handle function */
int iccp_csm_prepare_rg_disconnect_msg(struct CSM* csm, char* buf, size_t max_buf_size)
{
    ICCHdr* icc_hdr = (ICCHdr*)buf;
    DisconnectCodeTLV* disconn_code = (DisconnectCodeTLV*)&buf[sizeof(ICCHdr)];
    size_t msg_len = sizeof(ICCHdr) + sizeof(DisconnectCodeTLV);

    memset(buf, 0, max_buf_size);

    /* ICC header */
    icc_hdr->ldp_hdr.u_bit = 0x0;
    icc_hdr->ldp_hdr.msg_type = htons(MSG_T_RG_DISCONNECT);
    icc_hdr->ldp_hdr.msg_len = htons(msg_len - MSG_L_INCLUD_U_BIT_MSG_T_L_FIELDS);
    icc_hdr->ldp_hdr.msg_id = htonl(ICCP_MSG_ID++);
    iccp_csm_fill_icc_rg_id_tlv(csm, icc_hdr);

    /* Disconnect code TLV */
    disconn_code->icc_parameter.u_bit = 0x0;
    disconn_code->icc_parameter.f_bit = 0x0;
    disconn_code->icc_parameter.type = htons(TLV_T_DISCONNECT_CODE);
    disconn_code->icc_parameter.len = htons(sizeof(((struct DisconnectCodeTLV*)0)->iccp_status_code));
    disconn_code->iccp_status_code = htonl(csm->iccp_info.status_code);

    return msg_len;
}

/* Check ID(MC-LAG ID, mLACP ID, RG ID) from received message */
static void iccp_csm_check_id_from_msg(struct CSM* csm, struct Msg* msg)
{
    ICCHdr* icc_hdr = NULL;

    if (!csm || !msg || !msg->buf)
        return;

    icc_hdr = (ICCHdr*)msg->buf;

    /* Capability Message doesn't have ICC RG ID TLV */
    if (icc_hdr->ldp_hdr.msg_type == MSG_T_CAPABILITY)
        return;

    /* Check if received message ID same as local configuration */
    if (ntohl(icc_hdr->icc_rg_id_tlv.icc_rg_id) == csm->iccp_info.icc_rg_id)
    {
        if (csm->iccp_info.status_code == STATUS_CODE_U_ICCP_RG)
        {
            csm->iccp_info.status_code = 0x0;
            csm->iccp_info.rejected_msg_id = 0x0;
        }
    }
    else if (ntohl(icc_hdr->icc_rg_id_tlv.icc_rg_id) != csm->iccp_info.icc_rg_id)
    {
        csm->iccp_info.status_code = STATUS_CODE_U_ICCP_RG;
        csm->iccp_info.rejected_msg_id = ntohl(icc_hdr->icc_rg_id_tlv.icc_rg_id);
    }
}

/* Receive message correspond function */
void iccp_csm_correspond_from_msg(struct CSM* csm, struct Msg* msg)
{
    ICCHdr* icc_hdr = NULL;

    if (csm == NULL || msg == NULL || msg->buf == NULL)
        return;

    icc_hdr = (ICCHdr*)msg->buf;
    NAKTLV* nak = (NAKTLV*)( icc_hdr + sizeof(ICCHdr));
    iccp_csm_check_id_from_msg(csm, msg);

    if (icc_hdr->ldp_hdr.msg_type == MSG_T_CAPABILITY)
        iccp_csm_correspond_from_capability_msg(csm, msg);
    else if (icc_hdr->ldp_hdr.msg_type == MSG_T_RG_CONNECT)
        iccp_csm_correspond_from_rg_connect_msg(csm, msg);
    else if (icc_hdr->ldp_hdr.msg_type == MSG_T_RG_DISCONNECT)
        iccp_csm_correspond_from_rg_disconnect_msg(csm, msg);
    else if (icc_hdr->ldp_hdr.msg_type == MSG_T_NOTIFICATION)
    {
        ICCPD_LOG_DEBUG(__FUNCTION__, "Received MSG_T_NOTIFICATION ,err status %s reason of %s",  get_status_string(ntohl(nak->iccp_status_code)), get_status_string(csm->iccp_info.status_code));
        sleep(1);
    }
    else if (icc_hdr->ldp_hdr.msg_type == MSG_T_RG_APP_DATA)
    {
        ;// do nothing
    }
    else
    {
        ++csm->u_msg_in_count;
    }

    free(msg->buf);
    free(msg);
}

/* Receive capability message correspond function */
void iccp_csm_correspond_from_capability_msg(struct CSM* csm, struct Msg* msg)
{
    LDPICCPCapabilityTLV* cap = (LDPICCPCapabilityTLV*)&(msg->buf)[sizeof(LDPHdr)];

    *(uint16_t *)cap = ntohs(*(uint16_t *)cap);
    *(uint16_t *)((uint8_t *)cap + sizeof(ICCParameter)) = ntohs(*(uint16_t *)((uint8_t *)cap + sizeof(ICCParameter)));

    if (cap->icc_parameter.u_bit == 0x1
        && cap->icc_parameter.f_bit == 0x0
        && cap->icc_parameter.type == TLV_T_ICCP_CAPABILITY
        && ntohs(cap->icc_parameter.len) == (TLV_L_ICCP_CAPABILITY)
        && cap->s_bit == 1
        && cap->major_ver == 0x1
        && cap->minior_ver == 0x0)
    {
        csm->iccp_info.peer_capability_flag = 0x1;
    }
}

/* Receive RG connect message correspond function */
void iccp_csm_correspond_from_rg_connect_msg(struct CSM* csm, struct Msg* msg)
{
    ICCSenderNameTLV* sender = (ICCSenderNameTLV*)&(msg->buf)[sizeof(ICCHdr)];

    *(uint16_t *)sender = ntohs(*(uint16_t *)sender);

    if (sender->icc_parameter.u_bit == 0x0 &&
        sender->icc_parameter.f_bit == 0x0 &&
        sender->icc_parameter.type == TLV_T_ICC_SENDER_NAME)
    {
        csm->iccp_info.peer_rg_connect_flag = 0x1;
    }
}

/* Receive RG disconnect message correspond function */
void iccp_csm_correspond_from_rg_disconnect_msg(struct CSM* csm, struct Msg* msg)
{
    DisconnectCodeTLV* diconn_code = (DisconnectCodeTLV*)&(msg->buf)[sizeof(ICCHdr)];

    *(uint16_t *)diconn_code = ntohs(*(uint16_t *)diconn_code);

    if (diconn_code->icc_parameter.u_bit == 0x0
        && diconn_code->icc_parameter.f_bit == 0x0
        && diconn_code->icc_parameter.type == TLV_T_DISCONNECT_CODE
        && ntohs(diconn_code->icc_parameter.len) == (TLV_L_DISCONNECT_CODE)
        && ntohl(diconn_code->iccp_status_code) == (STATUS_CODE_ICCP_RG_REMOVED))
    {
        csm->iccp_info.sender_rg_connect_flag = 0x0;
        csm->iccp_info.peer_rg_connect_flag = 0x0;
    }
}

/* Add received message into message list */
void iccp_csm_enqueue_msg(struct CSM* csm, struct Msg* msg)
{
    ICCHdr* icc_hdr = NULL;
    NAKTLV* naktlv = NULL;
    int type = -1;
    int i = 0;

    if (csm == NULL)
    {
        if (msg != NULL)
            free(msg);
        return;
    }

    if (msg == NULL)
        return;

    icc_hdr = (ICCHdr*)msg->buf;

    *(uint16_t *)icc_hdr = ntohs(*(uint16_t *)icc_hdr);

    if (icc_hdr->ldp_hdr.msg_type == MSG_T_RG_APP_DATA)
    {
        app_csm_enqueue_msg(csm, msg);
    }
    else if (icc_hdr->ldp_hdr.msg_type == MSG_T_NOTIFICATION)
    {
        naktlv = (NAKTLV*)&msg->buf[sizeof(ICCHdr)];

        for (i = 0; i < MAX_MSG_LOG_SIZE; ++i)
        {
            if (ntohl(naktlv->rejected_msg_id) == csm->msg_log.msg[i].msg_id)
            {
                type = csm->msg_log.msg[i].type;
                break;
            }
        }

        if (type == MSG_T_RG_APP_DATA)
            app_csm_enqueue_msg(csm, msg);
        else
            TAILQ_INSERT_TAIL(&(csm->msg_list), msg, tail);
    }
    else
    {
        TAILQ_INSERT_TAIL(&(csm->msg_list), msg, tail);
    }
}

/* Get received message from message list */
struct Msg* iccp_csm_dequeue_msg(struct CSM* csm)
{
    struct Msg* msg = NULL;

    if (!TAILQ_EMPTY(&(csm->msg_list)))
    {
        msg = TAILQ_FIRST(&(csm->msg_list));
        TAILQ_REMOVE(&(csm->msg_list), msg, tail);

    }

    return msg;
}

/* Message initialization */
int iccp_csm_init_msg(struct Msg** msg, char* data, int len)
{
    struct Msg* iccp_msg = NULL;

    if (msg == NULL)
        return -2;

    if (data == NULL || len <= 0)
        return MCLAG_ERROR;

    iccp_msg = (struct Msg*)malloc(sizeof(struct Msg));
    if (iccp_msg == NULL)
        goto err_ret;

    iccp_msg->buf = (char*)malloc(len);
    if (iccp_msg->buf == NULL)
        goto err_ret;

    memcpy(iccp_msg->buf, data, len);
    iccp_msg->len = len;
    *msg = iccp_msg;

    return 0;

 err_ret:
    if (iccp_msg)
    {
        if (iccp_msg->buf)
            free(iccp_msg->buf);
        free(iccp_msg);
    }

    return MCLAG_ERROR;
}

/* MAC Message initialization */
int iccp_csm_init_mac_msg(struct MACMsg **mac_msg, char* data, int len)
{
    struct MACMsg* iccp_mac_msg = NULL;

    if (mac_msg == NULL)
        return -2;

    if (data == NULL || len <= 0)
        return MCLAG_ERROR;

    iccp_mac_msg = (struct MACMsg*)malloc(sizeof(struct MACMsg));
    if (iccp_mac_msg == NULL)
       return -3;

    memset(iccp_mac_msg, 0, sizeof(struct MACMsg));
    memcpy(iccp_mac_msg, data, len);

    *mac_msg = iccp_mac_msg;

    return 0;
}


void iccp_csm_stp_role_count(struct CSM *csm)
{
    struct If_info * cif = NULL;

    /* decide the role, lower ip to be active & socket client*/
    if (csm->role_type == STP_ROLE_NONE)
    {
        if (inet_addr(csm->sender_ip) < inet_addr(csm->peer_ip))
        {
            /* Active*/
            ICCPD_LOG_INFO(__FUNCTION__, "Role: [Active]");
            csm->role_type = STP_ROLE_ACTIVE;
            /* Send ICCP role update and system ID */
            mlacp_link_set_iccp_role(csm->mlag_id, true, MLACP(csm).system_id);
        }
        else
        {
            /* Standby*/
            ICCPD_LOG_INFO(__FUNCTION__, "Role [Standby]");
            csm->role_type = STP_ROLE_STANDBY;
            /* Send ICCP role update */
            mlacp_link_set_iccp_role(csm->mlag_id, false, NULL);
            /*Set Bridge MAC to MY MAC*/
            mlacp_fix_bridge_mac(csm);
        }
    }
}
