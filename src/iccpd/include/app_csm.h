/*
 * app_csm.h
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

#ifndef APP_CSM_H_
#define APP_CSM_H_

#include <sys/queue.h>

#include "../include/mlacp_fsm.h"

struct CSM;

enum APP_CONNECTION_STATE
{
    APP_NONEXISTENT,
    APP_RESET,
    APP_CONNSENT,
    APP_CONNREC,
    APP_CONNECTING,
    APP_OPERATIONAL
};

typedef enum APP_CONNECTION_STATE APP_CONNECTION_STATE_E;

struct AppCSM
{
    struct mLACP mlacp;
    APP_CONNECTION_STATE_E current_state;

    uint32_t rx_connect_msg_id;
    uint32_t tx_connect_msg_id;
    uint32_t invalid_msg_id;

    TAILQ_HEAD(app_msg_list, Msg) app_msg_list;

    uint8_t invalid_msg : 1;
    uint8_t nak_msg : 1;
};

void app_csm_init(struct CSM*, int all);
void app_csm_finalize(struct CSM*);
void app_csm_transit(struct CSM*);
int app_csm_prepare_iccp_msg(struct CSM*, char*, size_t);
void app_csm_enqueue_msg(struct CSM*, struct Msg*);
struct Msg* app_csm_dequeue_msg(struct CSM*);
void app_csm_correspond_from_msg(struct CSM*, struct Msg*);
void app_csm_correspond_from_connect_msg(struct CSM*, struct Msg*);
void app_csm_correspond_from_connect_ack_msg(struct CSM*, struct Msg*);
int app_csm_prepare_nak_msg(struct CSM*, char*, size_t);
int app_csm_prepare_connect_msg(struct CSM*, char*, size_t);
int app_csm_prepare_connect_ack_msg(struct CSM*, char*, size_t);

#endif /* APP_CSM_H_ */
