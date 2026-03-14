/*
 * app_csm.c
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

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/iccp_csm.h"
#include "../include/logger.h"
#include "../include/scheduler.h"
#include "../include/system.h"
#include "../include/iccp_netlink.h"
#include "../include/mlacp_link_handler.h"
/*****************************************
* Define
*
* ***************************************/
#define APP_CSM_QUEUE_REINIT(list) \
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

/* Application State Machine instance initialization */
void app_csm_init(struct CSM* csm, int all)
{
    if (csm == NULL )
        return;

    APP_CSM_QUEUE_REINIT(csm->app_csm.app_msg_list);

    if (all)
    {
        bzero(&(csm->app_csm), sizeof(struct AppCSM));
        APP_CSM_QUEUE_REINIT(csm->app_csm.app_msg_list);
    }

    csm->app_csm.current_state = APP_NONEXISTENT;
    csm->app_csm.rx_connect_msg_id = 0;
    csm->app_csm.tx_connect_msg_id = 0;
    csm->app_csm.invalid_msg_id = 0;
    csm->app_csm.invalid_msg = 0;
    csm->app_csm.nak_msg = 0;

    mlacp_init(csm, all);
}

/* Application State Machine instance tear down */
void app_csm_finalize(struct CSM* csm)
{
    mlacp_finalize(csm);
}

/* Application State Machine Transition */
void app_csm_transit(struct CSM* csm)
{
    if (csm == NULL )
        return;

    /* torn down event */
    if (csm->app_csm.current_state != APP_NONEXISTENT && csm->sock_fd <= 0)
    {
        csm->app_csm.current_state = APP_NONEXISTENT;
        return;
    }

    if (csm->app_csm.current_state != APP_OPERATIONAL && csm->current_state == ICCP_OPERATIONAL)
    {
        csm->app_csm.current_state = APP_OPERATIONAL;
    }

    return;
}

/* Add received message into application message list */
void app_csm_enqueue_msg(struct CSM* csm, struct Msg* msg)
{
    ICCHdr* icc_hdr = NULL;
    ICCParameter* param = NULL;
    NAKTLV* naktlv = NULL;
    int tlv = -1;
    int i = 0;

    if (csm == NULL )
    {
        if (msg != NULL )
            free(msg);
        return;
    }
    if (msg == NULL )
        return;

    icc_hdr = (ICCHdr*)msg->buf;
    param = (ICCParameter*)&msg->buf[sizeof(struct ICCHdr)];
    *(uint16_t *)param = ntohs(*(uint16_t *)param);

    if ( icc_hdr->ldp_hdr.msg_type == MSG_T_RG_APP_DATA)
    {
        if (param->type > TLV_T_MLACP_CONNECT && param->type < TLV_T_MLACP_LIST_END)
            mlacp_enqueue_msg(csm, msg);
        else
            TAILQ_INSERT_TAIL(&(csm->app_csm.app_msg_list), msg, tail);
    }
    else if (icc_hdr->ldp_hdr.msg_type == MSG_T_NOTIFICATION)
    {
        naktlv = (NAKTLV*)&msg->buf[sizeof(ICCHdr)];

        for (i = 0; i < MAX_MSG_LOG_SIZE; ++i)
        {
            if (ntohl(naktlv->rejected_msg_id) == csm->msg_log.msg[i].msg_id)
            {
                tlv = csm->msg_log.msg[i].tlv;
                break;
            }
        }

        if (tlv > TLV_T_MLACP_CONNECT && tlv <= TLV_T_MLACP_MAC_INFO)
            mlacp_enqueue_msg(csm, msg);
        else
            TAILQ_INSERT_TAIL(&(csm->app_csm.app_msg_list), msg, tail);
    }
    else
    {
        /* This packet is not for me, ignore it. */
        ICCPD_LOG_DEBUG(__FUNCTION__, "Ignore the packet with msg_type = %d", icc_hdr->ldp_hdr.msg_type);
    }
}

/* Get received message from message list */
struct Msg* app_csm_dequeue_msg(struct CSM* csm)
{
    struct Msg* msg = NULL;

    if (!TAILQ_EMPTY(&(csm->app_csm.app_msg_list)))
    {
        msg = TAILQ_FIRST(&(csm->app_csm.app_msg_list));
        TAILQ_REMOVE(&(csm->app_csm.app_msg_list), msg, tail);
    }

    return msg;
}

/* APP NAK message handle function */
int app_csm_prepare_nak_msg(struct CSM* csm, char* buf, size_t max_buf_size)
{
    ICCHdr* icc_hdr = (ICCHdr*)buf;
    NAKTLV* naktlv = (NAKTLV*)&buf[sizeof(ICCHdr)];
    size_t msg_len = sizeof(ICCHdr) + sizeof(NAKTLV);

    ICCPD_LOG_DEBUG(__FUNCTION__, " Response NAK");
    memset(buf, 0, max_buf_size);
    icc_hdr->ldp_hdr.u_bit = 0x0;
    icc_hdr->ldp_hdr.msg_type = htons(MSG_T_NOTIFICATION);
    icc_hdr->ldp_hdr.msg_len = htons(msg_len - MSG_L_INCLUD_U_BIT_MSG_T_L_FIELDS);
    icc_hdr->ldp_hdr.msg_id = htonl(ICCP_MSG_ID++);
    iccp_csm_fill_icc_rg_id_tlv(csm, icc_hdr);
    naktlv->icc_parameter.u_bit = 0;
    naktlv->icc_parameter.f_bit = 0;
    naktlv->icc_parameter.type = htons(TLV_T_NAK);
    naktlv->icc_parameter.len = htons(sizeof(NAKTLV) - 4);

    naktlv->iccp_status_code = htonl(STATUS_CODE_ICCP_REJECTED_MSG);
    naktlv->rejected_msg_id = htonl(csm->app_csm.invalid_msg_id);

    return msg_len;
}

int mlacp_bind_local_if(struct CSM* csm, struct LocalInterface* lif)
{
    struct LocalInterface* lifp = NULL;
    struct LocalInterface* lif_po = NULL;

    if (csm == NULL || lif == NULL)
        return MCLAG_ERROR;

    if (lif->csm == csm)
        return 0;

    /* remove purge from the csm*/
    do {
        LIST_FOREACH(lifp, &(MLACP(csm).lif_purge_list), mlacp_purge_next)
        {
            if (lifp == lif)
                break;
        }
        if (lifp)
            LIST_REMOVE(lifp, mlacp_purge_next);
    } while (lifp);

    /* already join csm?*/
    LIST_FOREACH(lifp, &(MLACP(csm).lif_list), mlacp_next)
    {
        if (lifp == lif)
            return 0;
    }

    /* join another csm beofre? remove from csm*/
    if (lif->csm != NULL)
        mlacp_unbind_local_if(lif);

    /* join new csm*/
    LIST_INSERT_HEAD(&(MLACP(csm).lif_list), lif, mlacp_next);
    lif->csm = csm;
    if (lif->type == IF_T_PORT_CHANNEL)
        lif->port_config_sync = 1;

    ICCPD_LOG_INFO(__FUNCTION__, "%s: MLACP bind on csm %p", lif->name, csm);
    if (lif->type == IF_T_PORT_CHANNEL)
        return 0;

    /* if join a po member, needs to check po joined also*/
    LIST_FOREACH(lif_po, &(MLACP(csm).lif_list), mlacp_next)
    {
        if (lif_po->type == IF_T_PORT_CHANNEL && lif_po->po_id == lif->po_id)
        {
            /*if join a po member, may swss restart, reset portchannel ip mac  to mclagsyncd*/
            update_if_ipmac_on_standby(lif_po, 1);
            return 0;
        }
    }

    if (lif_po == NULL)
    {
        lif_po = local_if_find_by_po_id(lif->po_id);
        if (lif_po == NULL)
        {
            ICCPD_LOG_WARN(__FUNCTION__, "Failed to find port_channel instance for %d.", lif->po_id);
            return MCLAG_ERROR;
        }

        lif_po->csm = csm;
        LIST_INSERT_HEAD(&(MLACP(csm).lif_list), lif_po, mlacp_next);
        lif_po->port_config_sync = 1;
        ICCPD_LOG_INFO(__FUNCTION__, "Add port_channel %d into local_if_list in CSM %p.", lif->po_id, csm);
    }

    return 0;
}

int mlacp_unbind_local_if(struct LocalInterface* lif)
{
    if (lif == NULL )
        return MCLAG_ERROR;

    if (lif->csm == NULL )
        return 0;

    ICCPD_LOG_INFO(__FUNCTION__, "%s: MLACP un-bind from csm %p", lif->name, lif->csm);
    LIST_REMOVE(lif, mlacp_next);

    if (MLACP(lif->csm).current_state  == MLACP_STATE_EXCHANGE && lif->type == IF_T_PORT_CHANNEL)
        LIST_INSERT_HEAD(&(MLACP(lif->csm).lif_purge_list), lif, mlacp_purge_next);
    if (lif->type == IF_T_PORT)
        lif->po_id = -1;
    lif->csm = NULL;

    return 0;
}

int mlacp_bind_port_channel_to_csm(struct CSM* csm, const char *ifname)
{
    struct System* sys = NULL;
    struct LocalInterface *lif_po = NULL;

    sys = system_get_instance();
    if (sys == NULL)
        return 0;

    if (csm == NULL)
        return 0;

    /* bind po first*/
    lif_po = local_if_find_by_name(ifname);
    if (lif_po)
    {
        mlacp_bind_local_if(csm, lif_po);
        iccp_get_port_member_list(lif_po);
    }
    else
    {
        ICCPD_LOG_WARN(__FUNCTION__, "%s: Failed to find a port instance .", ifname);
        return 0;
    }
    /* process link state handler after attaching it.*/

    mlacp_mlag_link_add_handler(csm, lif_po);

    /*ICCPD_LOG_WARN(tag, "po%d active =  %d\n", po_id, po_is_active);*/
    return 0;
}

