/*
 * iccp_cli.h
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

#ifndef _ICCP_CLI_H
#define _ICCP_CLI_H

#include <stdint.h>

struct CSM;

typedef enum
{
    QU_TYPE_NONE,
    QU_TYPE_MLAG_ADD_PO
} cli_queue_type_et;

typedef struct cli_param_queue
{
    char ifname[16];
    cli_queue_type_et type;
    int param;
    int itf_add;
    LIST_ENTRY(cli_param_queue) cli_queue_next;
} cli_param_queue_st;

#define MCLAG_ID_STR    "mclag_id"
#define LOCAL_IP_STR    "local_ip"
#define PEER_IP_STR     "peer_ip"
#define PEER_LINK_STR   "peer_link"
#define MCLAG_INTF_STR  "mclag_interface"
#define SYSTEM_MAC_STR  "system_mac"

int set_mc_lag_id(struct CSM *csm, uint16_t domain);
int set_peer_link(int mid, const char *ifname);
int set_local_address(int mid, const char *addr);
int set_peer_address(int mid, const char *addr);
int unset_mc_lag_id(struct CSM *csm, uint16_t domain);
int unset_peer_link(int mid);
int unset_local_address(int mid);
int unset_peer_address(int mid);

int iccp_cli_attach_mclag_domain_to_port_channel(int domain, const char *ifname);
int iccp_cli_detach_mclag_domain_to_port_channel(const char *ifname);
int set_local_system_id(const char *mac);
int unset_local_system_id();

int set_keepalive_time(int mid, int keepalive_time);
int set_session_timeout(int mid, int session_timeout_val);

#endif
