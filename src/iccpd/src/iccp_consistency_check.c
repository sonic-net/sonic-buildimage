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

#include "../include/iccp_consistency_check.h"
#include "../include/system.h"
#include "../include/port.h"
#include "../include/logger.h"

/* Return 0 if the checking procedure is failed; otherwise, 1 (non-zero) will be returned. */
typedef int (*ConsistencyCheckFunc)(char* ifname);

const char *reasons[] = {
    /* REASON_NONE */
    "Success",
    /* REASON_INTERRFACE_MODE_IS_ASYNC */
    "Port-channel interface is not in the same mode in local and peer device, please check whether the ip addr settings is correct or not.",
    /* REASON_PEER_IF_IP_IS_ASYNC */
    "IP address of peer interface is not synchronized, please check the IP address setting on the corresponding interface.",
    /* REASON_PEER_IF_VLAN_IS_ASYNC */
    "VLAN settings on this port-channel interface is not synchronized, please check your configuration.",
    /* REASON_MAX_ARRAY_SIZE */
    NULL
};

/* Consistency Checking functions */
static int iccp_check_interface_mode( char* ifname)
{
    struct CSM* csm = NULL;
    struct LocalInterface* local_if = NULL;
    struct PeerInterface* peer_if = NULL;

    local_if = local_if_find_by_name(ifname);
    if (local_if == NULL)
        return -2;

    csm = local_if->csm;
    if (csm == NULL)
        return -3;

    peer_if = peer_if_find_by_name(csm, ifname);
    if (peer_if == NULL)
        return -4;

    if (peer_if->l3_mode != local_if->l3_mode)
        return -5;

    return 1;
}

static int iccp_check_interface_layer3_addr(char* ifname)
{
    struct CSM* csm = NULL;
    struct LocalInterface* local_if = NULL;
    struct PeerInterface* peer_if = NULL;

    local_if = local_if_find_by_name(ifname);
    if (local_if == NULL)
        return -2;

    csm = local_if->csm;
    if (csm == NULL)
        return -3;

    peer_if = peer_if_find_by_name(csm, ifname);
    if (peer_if == NULL)
        return -4;

    if (peer_if->ipv4_addr != local_if->ipv4_addr)
        return -5;

    return 1;
}

static int iccp_check_interface_vlan(char* ifname)
{
    struct CSM* csm = NULL;
    struct PeerInterface* peer_if = NULL;
    struct VLAN_ID* local_vlan = NULL;
    struct VLAN_ID* peer_vlan = NULL;
    struct LocalInterface* local_if = NULL;

    local_if = local_if_find_by_name(ifname);
    if (local_if == NULL)
        return -2;

    csm = local_if->csm;
    if (csm == NULL)
        return -3;

    peer_if = peer_if_find_by_name(csm, ifname);
    if (peer_if == NULL)
        return -4;

    RB_FOREACH (local_vlan, vlan_rb_tree, &(local_if->vlan_tree))
    {
        peer_vlan = RB_FIND(vlan_rb_tree, &(peer_if->vlan_tree), local_vlan);

        if (peer_vlan == NULL)
        {
            return -5;
        }
    }

    RB_FOREACH (peer_vlan, vlan_rb_tree, &(peer_if->vlan_tree))
    {
        local_vlan = RB_FIND(vlan_rb_tree, &(local_if->vlan_tree), peer_vlan);

        if (local_vlan == NULL)
        {
            return -6;
        }
    }

    return 1;
}

static const ConsistencyCheckFunc check_func[] = {
    NULL,
    iccp_check_interface_mode,          /* REASON_INTERFACE_MODE_IS_ASYNC */
    iccp_check_interface_layer3_addr,   /* REASON_PEER_IF_IP_IS_ASYNC */
    iccp_check_interface_vlan,          /* REASON_PEER_IF_VLAN_IS_ASYNC */
    NULL                                /* REASON_MAX_ARRAY_SIZE */
};
#define ARRAY_SIZE(array_name) (sizeof(array_name) / sizeof(array_name[0]))

enum Reason_ID iccp_consistency_check(char* ifname)
{
    int i = 0;
    int ret = 0;

    for (i = REASON_INTERRFACE_MODE_IS_ASYNC; i < REASON_MAX_ARRAY_SIZE; ++i)
    {
        if (check_func[i] == NULL)
            continue;
        ret = check_func[i](ifname);
        if (ret != 1)
        {
            ICCPD_LOG_WARN(__FUNCTION__, "%s ret = %d", reasons[i], ret);
            fprintf(stdout, "%s \n", reasons[i]);
            return i;
        }
    }

    return REASON_NONE;
}
