/*
 * iccp_cmd_show.h
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

#ifndef _ICCP_CMD_SHOW_H
#define _ICCP_CMD_SHOW_H

#define ICCP_MAX_PORT_NAME 20
#define ICCP_MAX_IP_STR_LEN 16

extern int iccp_mclag_config_dump(char * *buf, int *num, int mclag_id);
extern int iccp_arp_dump(char * *buf, int *num, int mclag_id);
extern int iccp_ndisc_dump(char * *buf, int *num, int mclag_id);
extern int iccp_mac_dump(char * *buf, int *num, int mclag_id);
extern int iccp_local_if_dump(char * *buf, int *num, int mclag_id);
extern int iccp_peer_if_dump(char * *buf, int *num, int mclag_id);
extern int iccp_cmd_dbg_counter_dump(char * *buf, int *data_len, int mclag_id);
extern int iccp_unique_ip_if_dump(char * *buf, int *num, int mclag_id);
#endif
