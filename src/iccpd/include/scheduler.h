/*
 * scheduler.h
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

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <errno.h>

#include <stdio.h>
#include <string.h>
#include <sys/queue.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

struct CSM;
struct System;

#define CONNECT_INTERVAL_SEC        1
#define CONNECT_TIMEOUT_MSEC        100
#define HEARTBEAT_TIMEOUT_SEC       15
#define TRANSIT_INTERVAL_SEC        1
#define EPOLL_TIMEOUT_MSEC          100

int scheduler_prepare_session(struct CSM*);
int scheduler_check_csm_config(struct CSM*);
int scheduler_unregister_sock_read_event_callback(struct CSM*);
void scheduler_session_disconnect_handler(struct CSM*);
void scheduler_init();
void scheduler_finalize();
void scheduler_loop();
void scheduler_start();
void scheduler_server_sock_init();
int scheduler_csm_read_callback(struct CSM* csm);
int iccp_get_server_sock_fd();
int scheduler_server_accept();
int iccp_receive_signal_handler(struct System* sys);
void scheduler_csm_socket_cleanup(struct CSM* csm, int location);

#endif /* SCHEDULER_H_ */
