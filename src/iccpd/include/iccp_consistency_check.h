/*
 * iccp_consistency_check.c
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
 *
 */

#ifndef _ICCP_CONSISTENCY_CHECK_H
#define _ICCP_CONSISTENCY_CHECK_H


enum Reason_ID
{
    REASON_NONE = 0,
    REASON_INTERRFACE_MODE_IS_ASYNC,
    REASON_PEER_IF_IP_IS_ASYNC,
    REASON_PEER_IF_VLAN_IS_ASYNC,
    REASON_MAX_ARRAY_SIZE
};

extern const char *reasons[];

enum Reason_ID iccp_consistency_check(char* ifname);


#endif
