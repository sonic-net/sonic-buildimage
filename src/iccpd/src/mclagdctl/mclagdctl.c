/*
 * Copyright(c) 2016-2019 Nephos.
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
 *  Maintainer: Jim Jiang from nephos
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "mclagdctl.h"
#include "../../include/mlacp_fsm.h"
#include "../../include/system.h"

static int mclagdctl_sock_fd = -1;
char *mclagdctl_sock_path = "/var/run/iccpd/mclagdctl.sock";

/*
   Already implemented command:
   mclagdctl -i dump state
   mclagdctl -i dump arp
   mclagdctl -i dump nd
   mclagdctl -i dump mac
   mclagdctl -i dump unique_ip
   mclagdctl -i dump portlist local
   mclagdctl -i dump portlist peer
 */

#define ETHER_ADDR_LEN 6
char mac_print_str[18];

char *mac_addr_to_str(uint8_t* mac_addr)
{
    memset(mac_print_str, 0, sizeof(mac_print_str));
    snprintf(mac_print_str, sizeof(mac_print_str), "%02x:%02x:%02x:%02x:%02x:%02x",
        mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

    return mac_print_str;
}

static struct command_type command_types[] =
{
    {
        .id = ID_CMDTYPE_D,
        .name = "dump",
        .enca_msg = NULL,
        .parse_msg = NULL,
    },
    {
        .id = ID_CMDTYPE_D_S,
        .parent_id = ID_CMDTYPE_D,
        .info_type = INFO_TYPE_DUMP_STATE,
        .name = "state",
        .enca_msg = mclagdctl_enca_dump_state,
        .parse_msg = mclagdctl_parse_dump_state,
    },
    {
        .id = ID_CMDTYPE_D_A,
        .parent_id = ID_CMDTYPE_D,
        .info_type = INFO_TYPE_DUMP_ARP,
        .name = "arp",
        .enca_msg = mclagdctl_enca_dump_arp,
        .parse_msg = mclagdctl_parse_dump_arp,
    },
    {
         .id = ID_CMDTYPE_D_A,
         .parent_id = ID_CMDTYPE_D,
         .info_type = INFO_TYPE_DUMP_NDISC,
         .name = "nd",
         .enca_msg = mclagdctl_enca_dump_ndisc,
         .parse_msg = mclagdctl_parse_dump_ndisc,
     },
    {
        .id = ID_CMDTYPE_D_A,
        .parent_id = ID_CMDTYPE_D,
        .info_type = INFO_TYPE_DUMP_MAC,
        .name = "mac",
        .enca_msg = mclagdctl_enca_dump_mac,
        .parse_msg = mclagdctl_parse_dump_mac,
    },
    {
        .id = ID_CMDTYPE_D_A,
        .parent_id = ID_CMDTYPE_D,
        .info_type = INFO_TYPE_DUMP_UNIQUE_IP,
        .name = "unique_ip",
        .enca_msg = mclagdctl_enca_dump_unique_ip,
        .parse_msg = mclagdctl_parse_dump_unique_ip,
    },
    {
        .id = ID_CMDTYPE_D_P,
        .parent_id = ID_CMDTYPE_D,
        .name = "portlist",
    },
    {
        .id = ID_CMDTYPE_D_P_L,
        .parent_id = ID_CMDTYPE_D_P,
        .info_type = INFO_TYPE_DUMP_LOCAL_PORTLIST,
        .name = "local",
        .enca_msg = mclagdctl_enca_dump_local_portlist,
        .parse_msg = mclagdctl_parse_dump_local_portlist,
    },
    {
        .id = ID_CMDTYPE_D_P_P,
        .parent_id = ID_CMDTYPE_D_P,
        .info_type = INFO_TYPE_DUMP_PEER_PORTLIST,
        .name = "peer",
        .enca_msg = mclagdctl_enca_dump_peer_portlist,
        .parse_msg = mclagdctl_parse_dump_peer_portlist,
    },
    {
        .id = ID_CMDTYPE_D_D,
        .parent_id = ID_CMDTYPE_D,
        .name = "debug",
    },
    {
        .id = ID_CMDTYPE_D_D_C,
        .parent_id = ID_CMDTYPE_D_D,
        .info_type = INFO_TYPE_DUMP_DBG_COUNTERS,
        .name = "counters",
        .enca_msg = mclagdctl_enca_dump_dbg_counters,
        .parse_msg = mclagdctl_parse_dump_dbg_counters,
    },
    {
        .id = ID_CMDTYPE_C,
        .name = "config",
        .enca_msg = NULL,
        .parse_msg = NULL,
    },
    {
        .id = ID_CMDTYPE_C_L,
        .parent_id = ID_CMDTYPE_C,
        .info_type = INFO_TYPE_CONFIG_LOGLEVEL,
        .name = "loglevel",
        .enca_msg = mclagdctl_enca_config_loglevel,
        .parse_msg = mclagdctl_parse_config_loglevel,
    },
};

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define COMMAND_TYPE_COUNT ARRAY_SIZE(command_types)

int mclagdctl_sock_connect()
{
    struct sockaddr_un addr = { 0 };
    int addrlen = 0;
    int ret = 0;

    if (mclagdctl_sock_fd >= 0)
        return 0;

    if (strlen(mclagdctl_sock_path) <= 0)
        return MCLAG_ERROR;

    mclagdctl_sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (mclagdctl_sock_fd < 0)
    {
        return MCLAG_ERROR;
    }

    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path) - 1, "%s", mclagdctl_sock_path);
    addrlen = sizeof(addr.sun_family) + strlen(mclagdctl_sock_path);

    if ((ret = connect(mclagdctl_sock_fd, (struct sockaddr*)&addr, addrlen)) < 0)
    {
        close(mclagdctl_sock_fd);
        mclagdctl_sock_fd = -1;
        return MCLAG_ERROR;
    }

    return 0;
}

void mclagdctl_sock_close()
{
    if (mclagdctl_sock_fd > 0)
    {
        close(mclagdctl_sock_fd);
        mclagdctl_sock_fd = -1;
    }

    return;
}

int mclagdctl_sock_write(int fd, unsigned char *w_buf, int total_len)
{
    int write_len = 0;
    int ret = 0;

    while (write_len < total_len)
    {
        ret = write(fd, w_buf + write_len, total_len - write_len);
        if (ret <= 0)
        {
            return 0;
        }
        write_len += ret;
    }

    return write_len;
}

int mclagdctl_sock_read(int fd, unsigned char *r_buf, int total_len)
{
    int read_len = 0;
    int ret = 0;
    struct timeval tv = { 0 };
    fd_set read_fd;

    while (read_len < total_len)
    {
        FD_ZERO(&read_fd);
        FD_SET(fd, &read_fd);
        tv.tv_sec = 10;
        tv.tv_usec = 0;

        switch ((ret = select(fd + 1, &read_fd, NULL, NULL, &tv)))
        {
            case -1:    // error
                fprintf(stdout, "Mclagdctl:Select return error:%s\n", strerror(errno));
                return MCLAG_ERROR;

            case 0:    // timeout
                fprintf(stdout, "Mclagdctl:Select timeout:%s\n", strerror(errno));
                return MCLAG_ERROR;

            default:
                break;
        }

        ret = read(fd, r_buf + read_len, total_len - read_len);
        if (ret <= 0)
        {
            return MCLAG_ERROR;
        }
        read_len += ret;
    }

    return read_len;
}

int mclagdctl_enca_dump_state(char *msg, int mclag_id,  int argc, char **argv)
{
    struct mclagdctl_req_hdr req;

    memset(&req, 0, sizeof(struct mclagdctl_req_hdr));
    req.info_type = INFO_TYPE_DUMP_STATE;
    req.mclag_id = mclag_id;
    memcpy((struct mclagdctl_req_hdr *)msg, &req, sizeof(struct mclagdctl_req_hdr));

    return 1;
}

int mclagdctl_parse_dump_state(char *msg, int data_len)
{
    struct mclagd_state * state_info = NULL;
    int len = 0;
    int count = 0;
    int pos = 0;

    len = sizeof(struct mclagd_state);

    for (; data_len >= len; data_len -= len, count++)
    {
        state_info = (struct mclagd_state*)(msg + len * count);

        if (inet_addr(state_info->local_ip) < inet_addr(state_info->peer_ip))
        {
            state_info->role = 1;
        }
        else
        {
            state_info->role = 2;
        }

        fprintf(stdout, "%s: %s\n", "The MCLAG's keepalive is", state_info->keepalive ? "OK" : "ERROR");
        fprintf(stdout, "%s: %s\n", "MCLAG info sync is",
            state_info->info_sync_done ? "completed" : "incomplete");
        if (state_info->mclag_id <= 0)
            fprintf(stdout, "%s: %s\n", "Domain id", "Unknown");
        else
            fprintf(stdout, "%s: %d\n", "Domain id", state_info->mclag_id);

        fprintf(stdout, "%s: %s\n", "Local Ip", state_info->local_ip);
        fprintf(stdout, "%s: %s\n", "Peer Ip", state_info->peer_ip);
        fprintf(stdout, "%s: %s\n", "Peer Link Interface", state_info->peer_link_if);
        fprintf(stdout, "%s: %d\n", "Keepalive time",      state_info->keepalive_time);
        fprintf(stdout, "%s: %d\n", "sesssion Timeout ",   state_info->session_timeout);

        fprintf(stdout, "%s: %02x:%02x:%02x:%02x:%02x:%02x \n",
                "Peer Link Mac",
                state_info->peer_link_mac[0], state_info->peer_link_mac[1],
                state_info->peer_link_mac[2], state_info->peer_link_mac[3],
                state_info->peer_link_mac[4], state_info->peer_link_mac[5]);

        /*if (state_info->role == 0)
            fprintf(stdout, "%s: %s\n", "Role", "None");
        */
        if (state_info->role == 1)
            fprintf(stdout, "%s: %s\n", "Role", "Active");
        else if (state_info->role == 2)
            fprintf(stdout, "%s: %s\n", "Role", "Standby");

        fprintf(stdout, "%s: %s\n", "MCLAG Interface", state_info->enabled_po);

        fprintf(stdout, "%s: %s\n", "Loglevel", state_info->loglevel);
    }

    return 0;
}

int mclagdctl_enca_dump_arp(char *msg, int mclag_id, int argc, char **argv)
{
    struct mclagdctl_req_hdr req;

    if (mclag_id <= 0)
    {
        fprintf(stderr, "Need to specify mclag-id through the parameter i !\n");
        return MCLAG_ERROR;
    }

    memset(&req, 0, sizeof(struct mclagdctl_req_hdr));
    req.info_type = INFO_TYPE_DUMP_ARP;
    req.mclag_id = mclag_id;
    memcpy((struct mclagdctl_req_hdr *)msg, &req, sizeof(struct mclagdctl_req_hdr));

    return 1;
}

int mclagdctl_enca_dump_ndisc(char *msg, int mclag_id, int argc, char **argv)
{
    struct mclagdctl_req_hdr req;

    if (mclag_id <= 0)
    {
        fprintf(stderr, "Need to specify mclag-id through the parameter i !\n");
        return MCLAG_ERROR;
    }

    memset(&req, 0, sizeof(struct mclagdctl_req_hdr));
    req.info_type = INFO_TYPE_DUMP_NDISC;
    req.mclag_id = mclag_id;
    memcpy((struct mclagdctl_req_hdr *)msg, &req, sizeof(struct mclagdctl_req_hdr));

    return 1;
}

int mclagdctl_parse_dump_arp(char *msg, int data_len)
{
    struct mclagd_arp_msg * arp_info = NULL;
    int len = 0;
    int count = 0;

    fprintf(stdout, "%-6s", "No.");
    fprintf(stdout, "%-20s", "IP");
    fprintf(stdout, "%-20s", "MAC");
    fprintf(stdout, "%-20s", "DEV");
    fprintf(stdout, "%s", "Flag");
    fprintf(stdout, "\n");

    len = sizeof(struct mclagd_arp_msg);

    for (; data_len >= len; data_len -= len, count++)
    {
        arp_info = (struct mclagd_arp_msg*)(msg + len * count);

        fprintf(stdout, "%-6d", count + 1);
        fprintf(stdout, "%-20s", arp_info->ipv4_addr);
        fprintf(stdout, "%02x:%02x:%02x:%02x:%02x:%02x",
                arp_info->mac_addr[0], arp_info->mac_addr[1],
                arp_info->mac_addr[2], arp_info->mac_addr[3],
                arp_info->mac_addr[4], arp_info->mac_addr[5]);
        fprintf(stdout, "   ");
        fprintf(stdout, "%-20s", arp_info->ifname);
        if (arp_info->learn_flag == NEIGH_REMOTE) {
            fprintf(stdout, "%s", "R");
        } else if (arp_info->learn_flag == NEIGH_LOCAL) {
            fprintf(stdout, "%s", "L");
        } else {
            fprintf(stdout, "%s", "-");
        }
        fprintf(stdout, "\n");
    }

    return 0;
}

int mclagdctl_parse_dump_ndisc(char *msg, int data_len)
{
    struct mclagd_ndisc_msg *ndisc_info = NULL;
    int len = 0;
    int count = 0;

    fprintf(stdout, "%-6s", "No.");
    fprintf(stdout, "%-52s", "IPv6");
    fprintf(stdout, "%-20s", "MAC");
    fprintf(stdout, "%-20s", "DEV");
    fprintf(stdout, "%s", "Flag");
    fprintf(stdout, "\n");

    len = sizeof(struct mclagd_ndisc_msg);

    for (; data_len >= len; data_len -= len, count++)
    {
        ndisc_info = (struct mclagd_ndisc_msg *)(msg + len * count);

        fprintf(stdout, "%-6d", count + 1);
        fprintf(stdout, "%-52s", ndisc_info->ipv6_addr);
        fprintf(stdout, "%02x:%02x:%02x:%02x:%02x:%02x",
                ndisc_info->mac_addr[0], ndisc_info->mac_addr[1],
                ndisc_info->mac_addr[2], ndisc_info->mac_addr[3], ndisc_info->mac_addr[4], ndisc_info->mac_addr[5]);
        fprintf(stdout, "   ");
        fprintf(stdout, "%-20s", ndisc_info->ifname);
        if (ndisc_info->learn_flag == NEIGH_REMOTE) {
            fprintf(stdout, "%s", "R");
        } else if (ndisc_info->learn_flag == NEIGH_LOCAL) {
            fprintf(stdout, "%s", "L");
        } else {
            fprintf(stdout, "%s", "-");
        }
        fprintf(stdout, "\n");
    }

    return 0;
}

int mclagdctl_enca_dump_mac(char *msg, int mclag_id, int argc, char **argv)
{
    struct mclagdctl_req_hdr req;

    if (mclag_id <= 0)
    {
        fprintf(stderr, "Need to specify mclag-id through the parameter i !\n");
        return MCLAG_ERROR;
    }

    memset(&req, 0, sizeof(struct mclagdctl_req_hdr));
    req.info_type = INFO_TYPE_DUMP_MAC;
    req.mclag_id = mclag_id;
    memcpy((struct mclagdctl_req_hdr *)msg, &req, sizeof(struct mclagdctl_req_hdr));

    return 1;
}

int mclagdctl_parse_dump_mac(char *msg, int data_len)
{
    struct mclagd_mac_msg * mac_info = NULL;
    int len = 0;
    int count = 0;

    fprintf(stdout, "%-60s\n", "TYPE: S-STATIC, D-DYNAMIC; AGE: L-Local age, P-Peer age");

    fprintf(stdout, "%-6s", "No.");
    fprintf(stdout, "%-5s", "TYPE");
    fprintf(stdout, "%-20s", "MAC");
    fprintf(stdout, "%-5s", "VID");
    fprintf(stdout, "%-20s", "DEV");
    fprintf(stdout, "%-20s", "ORIGIN-DEV");
    fprintf(stdout, "%-5s", "AGE");
    fprintf(stdout, "\n");

    len = sizeof(struct mclagd_mac_msg);

    for (; data_len >= len; data_len -= len, count++)
    {
        mac_info = (struct mclagd_mac_msg*)(msg + len * count);

        fprintf(stdout, "%-6d", count + 1);

        if (mac_info->fdb_type == MAC_TYPE_STATIC_CTL)
            fprintf(stdout, "%-5s", "S");
        else
            fprintf(stdout, "%-5s", "D");

        fprintf(stdout, "%-20s", mac_addr_to_str(mac_info->mac_addr));

        fprintf(stdout, "%-5d", mac_info->vid);
        fprintf(stdout, "%-20s", mac_info->ifname);
        fprintf(stdout, "%-20s", mac_info->origin_ifname);

        if ((mac_info->age_flag & MAC_AGE_LOCAL_CTL) && (mac_info->age_flag & MAC_AGE_PEER_CTL))
            fprintf(stdout, "%-5s", "LP");
        else if (mac_info->age_flag & MAC_AGE_LOCAL_CTL)
            fprintf(stdout, "%-5s", "L");
        else if (mac_info->age_flag & MAC_AGE_PEER_CTL)
            fprintf(stdout, "%-5s", "P");
        else
            fprintf(stdout, "%-5s", " ");
        fprintf(stdout, "\n");
    }

    return 0;
}

int mclagdctl_enca_dump_local_portlist(char *msg, int mclag_id, int argc, char **argv)
{
    struct mclagdctl_req_hdr req;

    if (mclag_id <= 0)
    {
        fprintf(stderr, "Need to specify mclag-id through the parameter i !\n");
        return MCLAG_ERROR;
    }

    memset(&req, 0, sizeof(struct mclagdctl_req_hdr));
    req.info_type = INFO_TYPE_DUMP_LOCAL_PORTLIST;
    req.mclag_id = mclag_id;
    memcpy((struct mclagdctl_req_hdr *)msg, &req, sizeof(struct mclagdctl_req_hdr));

    return 1;
}

int mclagdctl_parse_dump_local_portlist(char *msg, int data_len)
{
    struct mclagd_local_if * lif_info = NULL;
    int len = 0;
    int count = 0;
    int pos = 0;

    len = sizeof(struct mclagd_local_if);

    for (; data_len >= len; data_len -= len, count++)
    {
        lif_info = (struct mclagd_local_if*)(msg + len * count);

        for (pos = 0; pos < 60; ++pos)
            fprintf(stdout, "-");

        fprintf(stdout, "\n");

        if (memcmp(lif_info->type, "PortChannel", 11) == 0)
        {
            fprintf(stdout, "%s: %d\n", "Ifindex", lif_info->ifindex);
            fprintf(stdout, "%s: %s\n", "Type", lif_info->type);
            fprintf(stdout, "%s: %s\n", "PortName", lif_info->name);
            fprintf(stdout, "%s: %02x:%02x:%02x:%02x:%02x:%02x \n",
                    "MAC",
                    lif_info->mac_addr[0], lif_info->mac_addr[1],
                    lif_info->mac_addr[2], lif_info->mac_addr[3],
                    lif_info->mac_addr[4], lif_info->mac_addr[5]);

            fprintf(stdout, "%s: %s\n", "IPv4Address", lif_info->ipv4_addr);
            fprintf(stdout, "%s: %d\n", "Prefixlen", lif_info->prefixlen);
            fprintf(stdout, "%s: %s\n", "State", lif_info->state);
            fprintf(stdout, "%s: %s\n", "IsL3Interface", lif_info->l3_mode ? "Yes" : "No");
            /*fprintf(stdout, "%s: %s\n", "IsPeerlink", lif_info->is_peer_link ? "Yes" : "No");*/
            fprintf(stdout, "%s: %s\n", "MemberPorts", lif_info->portchannel_member_buf);
            /*fprintf(stdout,"%s: %d\n" ,"PortchannelId", lif_info->po_id);*/
            fprintf(stdout,"%s: %d\n" ,"PortchannelIsUp", lif_info->po_active);
            /*fprintf(stdout,"%s: %s\n", "MlacpState", lif_info->mlacp_state);*/
            fprintf(stdout, "%s: %s\n", "IsIsolateWithPeerlink", lif_info->isolate_to_peer_link ? "Yes" : "No");
            fprintf(stdout,"%s: %s\n" ,"IsTrafficDisable", lif_info->is_traffic_disable ? "Yes":"No");
            fprintf(stdout, "%s: %s\n", "VlanList", lif_info->vlanlist);
        }
        else
        {
            fprintf(stdout, "%s: %d\n", "Ifindex", lif_info->ifindex);
            fprintf(stdout, "%s: %s\n", "Type", lif_info->type);
            fprintf(stdout, "%s: %s\n", "PortName", lif_info->name);
            fprintf(stdout, "%s: %s\n", "State", lif_info->state);
            fprintf(stdout, "%s: %s\n", "VlanList", lif_info->vlanlist);
            /*fprintf(stdout,"%s: %d\n" ,"PortchannelId", lif_info->po_id);*/
        }

        for (pos = 0; pos < 60; ++pos)
            fprintf(stdout, "-");

        fprintf(stdout, "\n\n");
    }

    return 0;
}

int mclagdctl_enca_dump_peer_portlist(char *msg, int mclag_id,  int argc, char **argv)
{
    struct mclagdctl_req_hdr req;

    if (mclag_id <= 0)
    {
        fprintf(stderr, "Need to specify mclag-id through the parameter i !\n");
        return MCLAG_ERROR;
    }

    memset(&req, 0, sizeof(struct mclagdctl_req_hdr));
    req.info_type = INFO_TYPE_DUMP_PEER_PORTLIST;
    req.mclag_id = mclag_id;
    memcpy((struct mclagdctl_req_hdr *)msg, &req, sizeof(struct mclagdctl_req_hdr));

    return 1;
}

int mclagdctl_parse_dump_peer_portlist(char *msg, int data_len)
{
    struct mclagd_peer_if * pif_info = NULL;
    int len = 0;
    int count = 0;
    int pos = 0;

    len = sizeof(struct mclagd_peer_if);

    for (; data_len >= len; data_len -= len, count++)
    {
        pif_info = (struct mclagd_peer_if*)(msg + len * count);

        for (pos = 0; pos < 60; ++pos)
            fprintf(stdout, "-");

        fprintf(stdout, "\n");

        fprintf(stdout, "%s: %d\n", "Ifindex", pif_info->ifindex);
        fprintf(stdout, "%s: %s\n", "Type", pif_info->type);
        fprintf(stdout, "%s: %s\n", "PortName", pif_info->name);
        fprintf(stdout, "%s: %02x:%02x:%02x:%02x:%02x:%02x \n",
                "MAC",
                pif_info->mac_addr[0], pif_info->mac_addr[1],
                pif_info->mac_addr[2], pif_info->mac_addr[3],
                pif_info->mac_addr[4], pif_info->mac_addr[5]);

        fprintf(stdout, "%s: %s\n", "State", pif_info->state);
        /*fprintf(stdout,"%s: %d\n" ,"PortchannelId", pif_info->po_id);
           fprintf(stdout,"%s: %d\n" ,"PortchannelIsActive", pif_info->po_active);*/

        for (pos = 0; pos < 60; ++pos)
            fprintf(stdout, "-");

        fprintf(stdout, "\n\n");
    }

    return 0;
}

int mclagdctl_enca_dump_unique_ip(char *msg, int mclag_id, int argc, char **argv)
{
    struct mclagdctl_req_hdr req;

    if (mclag_id <= 0)
    {
        fprintf(stderr, "Need to specify mclag-id through the parameter i !\n");
        return MCLAG_ERROR;
    }

    memset(&req, 0, sizeof(struct mclagdctl_req_hdr));
    req.info_type = INFO_TYPE_DUMP_UNIQUE_IP;
    req.mclag_id = mclag_id;
    memcpy((struct mclagdctl_req_hdr *)msg, &req, sizeof(struct mclagdctl_req_hdr));

    return 1;
}

int mclagdctl_parse_dump_unique_ip(char *msg, int data_len)
{
    struct mclagd_unique_ip_if *ip_if_info = NULL;
    int len = 0;
    int count = 0;
    int pos = 0;

    for (pos = 0; pos < 60; ++pos)
        fprintf(stdout, "-");
    fprintf(stdout, "\n");
    fprintf(stdout, "%-20s", "Ifname");
    fprintf(stdout, "%-5s", "Active");
    fprintf(stdout, "\n");

    for (pos = 0; pos < 60; ++pos)
        fprintf(stdout, "-");
    fprintf(stdout, "\n");

    len = sizeof(struct mclagd_unique_ip_if);

    for (; data_len >= len; data_len -= len, count++)
    {
        ip_if_info = (struct mclagd_unique_ip_if*)(msg + len * count);

        fprintf(stdout, "%-20s  %-5s\n", ip_if_info->name, ip_if_info->active?"Yes":"No");
    }

    if (count == 0)
    {
        fprintf(stdout, "%s\n", "Unique IP configuration not enabled on any interface");
    }

    for (pos = 0; pos < 60; ++pos)
        fprintf(stdout, "-");

    fprintf(stdout, "\n\n");
    return 0;
}

/* mclag_id parameter is optional */
int mclagdctl_enca_dump_dbg_counters(char *msg, int mclag_id, int argc, char **argv)
{
    struct mclagdctl_req_hdr req;

    memset(&req, 0, sizeof(struct mclagdctl_req_hdr));
    req.info_type = INFO_TYPE_DUMP_DBG_COUNTERS;
    req.mclag_id = mclag_id;
    memcpy((struct mclagdctl_req_hdr *)msg, &req, sizeof(struct mclagdctl_req_hdr));

    return 1;
}

static char *mclagdctl_dbg_counter_iccpid2str(ICCP_DBG_CNTR_MSG_e iccp_cntr_id)
{
    /* Keep the string to 15 characters.
     * Update mclagdctl_parse_dump_dbg_counters if increase
     */
    switch(iccp_cntr_id)
    {
        case ICCP_DBG_CNTR_MSG_SYS_CONFIG:
            return "SysConfig";
        case ICCP_DBG_CNTR_MSG_AGGR_CONFIG:
            return "AggrConfig";
        case ICCP_DBG_CNTR_MSG_AGGR_STATE:
            return "AggrState";
        case ICCP_DBG_CNTR_MSG_MAC_INFO:
            return "MacInfo";
        case ICCP_DBG_CNTR_MSG_ARP_INFO:
            return "ArpInfo";
        case ICCP_DBG_CNTR_MSG_PORTCHANNEL_INFO:
            return "PoInfo";
        case ICCP_DBG_CNTR_MSG_PEER_LINK_INFO:
            return "PeerLinkInfo";
        case ICCP_DBG_CNTR_MSG_HEART_BEAT:
            return "Heartbeat";
        case ICCP_DBG_CNTR_MSG_NAK:
            return "Nak";
        case ICCP_DBG_CNTR_MSG_SYNC_DATA:
            return "SyncData";
        case ICCP_DBG_CNTR_MSG_SYNC_REQ:
            return "SyncReq";
        case ICCP_DBG_CNTR_MSG_WARM_BOOT:
            return "Warmboot";
        case ICCP_DBG_CNTR_MSG_IF_UP_ACK:
            return "IfUpAck";
        default:
            return "Unknown";
    }
}

static char *mclagdctl_dbg_counter_syncdtx2str(SYNCD_TX_DBG_CNTR_MSG_e syncdtx_id)
{
    /* Keep the string to 20 characters.
     * Update mclagdctl_parse_dump_dbg_counters if increase
     */
    switch(syncdtx_id)
    {
        case SYNCD_TX_DBG_CNTR_MSG_PORT_ISOLATE:
            return "PortIsolation";
        case SYNCD_TX_DBG_CNTR_MSG_PORT_MAC_LEARN_MODE:
            return "MacLearnMode";
        case SYNCD_TX_DBG_CNTR_MSG_FLUSH_FDB:
            return "FlushFdb";
        case SYNCD_TX_DBG_CNTR_MSG_SET_IF_MAC:
            return "SetIfMac";
        case SYNCD_TX_DBG_CNTR_MSG_SET_FDB:
            return "SetFdb";
        case SYNCD_TX_DBG_CNTR_MSG_SET_TRAFFIC_DIST_ENABLE:
            return "TrafficDistEnable";
        case SYNCD_TX_DBG_CNTR_MSG_SET_TRAFFIC_DIST_DISABLE:
            return "TrafficDistDisable";
        case SYNCD_TX_DBG_CNTR_MSG_SET_ICCP_STATE:
            return "SetIccpState";
        case SYNCD_TX_DBG_CNTR_MSG_SET_ICCP_ROLE:
            return "SetIccpRole";
        case SYNCD_TX_DBG_CNTR_MSG_SET_ICCP_SYSTEM_ID:
            return "SetSystemId";
        case SYNCD_TX_DBG_CNTR_MSG_DEL_ICCP_INFO:
            return "DelIccpInfo";
        case SYNCD_TX_DBG_CNTR_MSG_SET_REMOTE_IF_STATE:
            return "SetRemoteIntfSts";
        case SYNCD_TX_DBG_CNTR_MSG_DEL_REMOTE_IF_INFO:
            return "DelRemoteIntf";
        case SYNCD_TX_DBG_CNTR_MSG_PEER_LINK_ISOLATION:
            return "PeerLinkIsolation";
        case SYNCD_TX_DBG_CNTR_MSG_SET_ICCP_PEER_SYSTEM_ID:
            return "SetPeerSystemId";
        default:
            return "Unknown";
    }
}

static char *mclagdctl_dbg_counter_syncdrx2str(SYNCD_RX_DBG_CNTR_MSG_e syncdrx_id)
{
    /* Keep the string to 20 characters.
     * Update mclagdctl_parse_dump_dbg_counters if increase
     */
    switch(syncdrx_id)
    {
        case SYNCD_RX_DBG_CNTR_MSG_MAC:
            return "FdbChange";
        case SYNCD_RX_DBG_CNTR_MSG_CFG_MCLAG_DOMAIN:
            return "CfgMclag";
        case SYNCD_RX_DBG_CNTR_MSG_CFG_MCLAG_IFACE:
            return "CfgMclagIface";
        case SYNCD_RX_DBG_CNTR_MSG_CFG_MCLAG_UNIQUE_IP:
            return "CfgMclagUniqueIp";
        case SYNCD_RX_DBG_CNTR_MSG_VLAN_MBR_UPDATES:
            return "vlanMbrshipChange";
        default:
            return "Unknown";
    }
}

int mclagdctl_parse_dump_dbg_counters(char *msg, int data_len)
{
    mclagd_dbg_counter_info_t *dbg_counter_p;
    system_dbg_counter_info_t *sys_counter_p;
    mlacp_dbg_counter_info_t  *iccp_counter_p;
    int                       i, j;

    dbg_counter_p = (mclagd_dbg_counter_info_t *)msg;
    sys_counter_p = (system_dbg_counter_info_t *)&dbg_counter_p->system_dbg;

    /* Global counters */
    fprintf(stdout, "%-20s%u\n", "ICCP session down:",
        sys_counter_p->session_down_counter);
    fprintf(stdout, "%-20s%u\n", "Peer link down:",
        sys_counter_p->peer_link_down_counter);
    fprintf(stdout, "%-20s%u\n", "Rx invalid msg:",
        sys_counter_p->rx_peer_invalid_msg_counter);
    fprintf(stdout, "%-20s%u\n", "Rx sock error(hdr):",
        sys_counter_p->rx_peer_hdr_read_sock_err_counter);
    fprintf(stdout, "%-20s%u\n", "Rx zero len(hdr):",
        sys_counter_p->rx_peer_hdr_read_sock_zero_len_counter);
    fprintf(stdout, "%-20s%u\n", "Rx sock error(tlv):",
        sys_counter_p->rx_peer_tlv_read_sock_err_counter);
    fprintf(stdout, "%-20s%u\n", "Rx zero len(tlv):",
        sys_counter_p->rx_peer_tlv_read_sock_zero_len_counter);
    fprintf(stdout, "%-20s%u\n", "Rx retry max:",
        sys_counter_p->rx_retry_max_counter);
    fprintf(stdout, "%-20s%u\n", "Rx retry total:",
        sys_counter_p->rx_retry_total_counter);
    fprintf(stdout, "%-20s%u\n", "Rx retry fail:",
        sys_counter_p->rx_retry_fail_counter);
    fprintf(stdout, "%-20s%u\n", "Socket close err:",
        sys_counter_p->socket_close_err_counter);
    fprintf(stdout, "%-20s%u\n", "Socket cleanup:",
        sys_counter_p->socket_cleanup_counter);

    fprintf(stdout, "\n");
    fprintf(stdout, "%-20s%u\n\n", "Warmboot:", sys_counter_p->warmboot_counter);

    /* ICCP daemon to Mclagsyncd messages */
    fprintf(stdout, "%-20s%-20s%-20s\n", "ICCP to MclagSyncd", "TX_OK", "TX_ERROR");
    fprintf(stdout, "%-20s%-20s%-20s\n", "------------------", "-----", "--------");
    for (i = 0; i < SYNCD_TX_DBG_CNTR_MSG_MAX; ++i)
    {
        fprintf(stdout, "%-20s%-20lu%-20lu\n",
            mclagdctl_dbg_counter_syncdtx2str(i),
            sys_counter_p->syncd_tx_counters[i][0],
            sys_counter_p->syncd_tx_counters[i][1]);
    }

    fprintf(stdout, "\n%-20s%-20s%-20s\n", "MclagSyncd to ICCP", "RX_OK", "RX_ERROR");
    fprintf(stdout, "%-20s%-20s%-20s\n", "------------------", "-----", "--------");
    for (i = 0; i < SYNCD_RX_DBG_CNTR_MSG_MAX; ++i)
    {
        fprintf(stdout, "%-20s%-20lu%-20lu\n",
            mclagdctl_dbg_counter_syncdrx2str(i),
            sys_counter_p->syncd_rx_counters[i][0],
            sys_counter_p->syncd_rx_counters[i][1]);
    }
    /* Print ICCP messages exchanged between MLAG peers */
    fprintf(stdout, "\n%-20s%-20s%-20s%-20s%-20s\n",
        "ICCP to Peer", "TX_OK", "RX_OK", "TX_ERROR", "RX_ERROR");
    fprintf(stdout, "%-20s%-20s%-20s%-20s%-20s\n",
        "------------", "-----", "-----", "--------", "--------");

    iccp_counter_p = (mlacp_dbg_counter_info_t *)dbg_counter_p->iccp_dbg_counters;
    for (i = 0; i < dbg_counter_p->num_iccp_counter_blocks; ++i)
    {
        if (i > 0)
            ++iccp_counter_p;

        for (j = 0; j < ICCP_DBG_CNTR_MSG_MAX; ++j)
        {
            fprintf(stdout, "%-20s%-20lu%-20lu%-20lu%-20lu\n",
                mclagdctl_dbg_counter_iccpid2str(j),
                iccp_counter_p->iccp_counters[j][0][0],
                iccp_counter_p->iccp_counters[j][1][0],
                iccp_counter_p->iccp_counters[j][0][1],
                iccp_counter_p->iccp_counters[j][1][1]);
        }
        fprintf(stdout, "\n");
    }
    /* Netlink counters */
    fprintf(stdout, "\nNetlink Counters\n");
    fprintf(stdout, "-----------------\n");
    fprintf(stdout, "Link add/del: %u/%u\n",
        sys_counter_p->newlink_count, sys_counter_p->dellink_count);
    fprintf(stdout, "  Unknown if_name: %u\n", sys_counter_p->unknown_if_name_count);
    fprintf(stdout, "Neighbor(ARP) add/del: %u/%u\n",
        sys_counter_p->newnbr_count, sys_counter_p->delnbr_count);
    fprintf(stdout, "  MAC entry add/del: %u/%u\n",
        sys_counter_p->newmac_count, sys_counter_p->delmac_count);
    fprintf(stdout, "Address add/del: %u/%u\n",
        sys_counter_p->newaddr_count, sys_counter_p->deladdr_count);
    fprintf(stdout, "Unexpected message type: %u\n", sys_counter_p->unknown_type_count);
    fprintf(stdout, "Receive error: %u\n\n", sys_counter_p->rx_error_count);
    return 0;
}

int mclagdctl_enca_config_loglevel(char *msg, int log_level,  int argc, char **argv)
{
    struct mclagdctl_req_hdr req;

    memset(&req, 0, sizeof(struct mclagdctl_req_hdr));
    req.info_type = INFO_TYPE_CONFIG_LOGLEVEL;
    req.mclag_id = log_level;
    memcpy((struct mclagdctl_req_hdr *)msg, &req, sizeof(struct mclagdctl_req_hdr));

    return 1;
}

int mclagdctl_parse_config_loglevel(char *msg, int data_len)
{

    int ret = *(int*)msg;

    if (ret == 0)
        fprintf(stdout, "%s\n", "Config loglevel success!");
    else
        fprintf(stdout, "%s\n", "Config loglevel failed!");

    return 0;
}

static bool __mclagdctl_cmd_executable(struct command_type *cmd_type)
{
    if (!cmd_type->enca_msg || !cmd_type->parse_msg)
        return 0;

    return 1;
}

static int __mclagdctl_cmd_param_cnt(struct command_type *cmd_type)
{
    int i = 0;

    while (cmd_type->params[i])
        i++;

    return i;
}

static struct command_type *__mclagdctl_get_cmd_by_parent(char *cmd_name,
                                                          enum id_command_type parent_id)
{
    int i;

    for (i = 0; i < COMMAND_TYPE_COUNT; i++)
    {
        if (!strncmp(command_types[i].name, cmd_name, strlen(cmd_name))
            && command_types[i].parent_id == parent_id)
            return &command_types[i];
    }

    return NULL;
}

static struct command_type *__mclagdctl_get_cmd_by_id(enum id_command_type id)
{
    int i;

    for (i = 0; i < COMMAND_TYPE_COUNT; i++)
    {
        if (command_types[i].id == id)
            return &command_types[i];
    }

    return NULL;
}

static int mclagdctl_find_cmd(struct command_type **pcmd_type, int *argc, char ***argv)
{
    char *cmd_name;
    enum id_command_type parent_id = ID_CMDTYPE_NONE;
    struct command_type *cmd_type;

    while (1)
    {
        if (!*argc)
        {
            fprintf(stderr, "None or incomplete command\n");
            return -EINVAL;
        }

        cmd_name = *argv[0];
        (*argc)--;
        (*argv)++;
        cmd_type = __mclagdctl_get_cmd_by_parent(cmd_name, parent_id);

        if (!cmd_type)
        {
            fprintf(stderr, "Unknown command \"%s\".\n", cmd_name);
            return -EINVAL;
        }

        if (__mclagdctl_cmd_executable(cmd_type) && __mclagdctl_cmd_param_cnt(cmd_type) >= *argc)
        {
            *pcmd_type = cmd_type;
            return 0;
        }

        parent_id = cmd_type->id;
    }
}

static int mclagdctl_check_cmd_params(struct command_type *cmd_type,
                                      int argc, char **argv)
{
    int i = 0;

    while (cmd_type->params[i])
    {
        if (i == argc)
        {
            fprintf(stderr, "Command line parameter \"%s\" expected.\n", cmd_type->params[i]);
            return -EINVAL;
        }
        i++;
    }

    return 0;
}

static void mclagdctl_print_cmd(struct command_type *cmd_type)
{
    if (cmd_type->parent_id != ID_CMDTYPE_NONE)
    {
        mclagdctl_print_cmd(__mclagdctl_get_cmd_by_id(cmd_type->parent_id));
        fprintf(stdout, " ");
    }
    fprintf(stdout, "%s", cmd_type->name);
}

static void mclagdctl_print_help(const char *argv0)
{
    int i, j;
    struct command_type *cmd_type;

    fprintf(stdout, "%s [options] command [command args]\n"
            "    -h --help                Show this help\n"
            "    -i --mclag-id            Specify one mclag id\n"
            "    -l --level               Specify log level     critical,err,warn,notice,info,debug\n",
            argv0);
    fprintf(stdout, "Commands:\n");

    for (i = 0; i < COMMAND_TYPE_COUNT; i++)
    {
        cmd_type = &command_types[i];
        if (!__mclagdctl_cmd_executable(cmd_type))
            continue;
        fprintf(stdout, "    ");
        mclagdctl_print_cmd(cmd_type);

        for (j = 0; cmd_type->params[j]; j++)
            fprintf(stdout, " %s", cmd_type->params[j]);

        fprintf(stdout, "\n");
    }
}

int main(int argc, char **argv)
{
    char buf[MCLAGDCTL_CMD_SIZE] = { 0 };
    char *argv0 = argv[0];
    char *rcv_buf = NULL;
    static const struct option long_options[] =
    {
        { "help",      no_argument,             NULL,        'h' },
        { "mclag id",  required_argument,       NULL,        'i' },
        { "log level", required_argument,       NULL,        'l' },
        { NULL,        0,                       NULL,        0   }
    };
    int opt;
    int err;
    struct command_type *cmd_type;
    int ret;
    unsigned para_int = 0;

    int len = 0;
    char *data;
    struct mclagd_reply_hdr *reply;

    while ((opt = getopt_long(argc, argv, "hi:l:", long_options, NULL)) >= 0)
    {
        switch (opt)
        {
            case 'h':
                mclagdctl_print_help(argv0);
                return EXIT_SUCCESS;

        case 'i':
            para_int = atoi(optarg);
            break;

        case 'l':
            switch (tolower(optarg[0]))
            {
                case 'c':
                    para_int = CRITICAL;
                    break;

                case 'e':
                    para_int = ERR;
                    break;

                case 'w':
                    para_int = WARN;
                    break;

                case 'n':
                    para_int = NOTICE;
                    break;

                case 'i':
                    para_int = INFO;
                    break;

                case 'd':
                    para_int = DEBUG;
                    break;

                default:
                    fprintf(stderr, "unknown option \"%c\".\n", opt);
                    mclagdctl_print_help(argv0);
                    return EXIT_FAILURE;
            }
            break;

            case '?':
                fprintf(stderr, "unknown option.\n");
                mclagdctl_print_help(argv0);
                return EXIT_FAILURE;

            default:
                fprintf(stderr, "unknown option \"%c\".\n", opt);
                mclagdctl_print_help(argv0);
                return EXIT_FAILURE;
        }
    }

    argv += optind;
    argc -= optind;

    err = mclagdctl_find_cmd(&cmd_type, &argc, &argv);
    if (err)
    {
        mclagdctl_print_help(argv0);
        return EXIT_FAILURE;
    }

    err = mclagdctl_check_cmd_params(cmd_type, argc, argv);
    if (err)
    {
        mclagdctl_print_help(argv0);
        return EXIT_FAILURE;
    }

    if (mclagdctl_sock_fd <= 0)
    {
        ret = mclagdctl_sock_connect();
        if (ret < 0)
            return EXIT_FAILURE;
    }

    if (cmd_type->enca_msg(buf, para_int, argc, argv) < 0)
    {
        ret = EXIT_FAILURE;
        goto mclagdctl_disconnect;
    }

    ret = mclagdctl_sock_write(mclagdctl_sock_fd, buf, sizeof(struct mclagdctl_req_hdr));

    if (ret <= 0)
    {
        fprintf(stderr, "Failed to send command to mclagd\n");
        ret = EXIT_FAILURE;
        goto mclagdctl_disconnect;
    }

    /*read data length*/
    memset(buf, 0, MCLAGDCTL_CMD_SIZE);
    ret = mclagdctl_sock_read(mclagdctl_sock_fd, buf, sizeof(int));
    if (ret <= 0)
    {
        fprintf(stderr, "Failed to read data length from mclagd\n");
        ret = EXIT_FAILURE;
        goto mclagdctl_disconnect;
    }

    /*cont length*/
    len = *((int*)buf);
    if (len <= 0)
    {
        ret = EXIT_FAILURE;
        fprintf(stderr, "pkt len = %d, error\n", len);
        goto mclagdctl_disconnect;
    }

    rcv_buf = (char *)malloc(len);
    if (!rcv_buf)
    {
        fprintf(stderr, "Failed to malloc rcv_buf for mclagdctl\n");
        goto mclagdctl_disconnect;
    }

    /*read data*/
    ret = mclagdctl_sock_read(mclagdctl_sock_fd, rcv_buf, len);
    if (ret <= 0)
    {
        fprintf(stderr, "Failed to read data from mclagd\n");
        ret = EXIT_FAILURE;
        goto mclagdctl_disconnect;
    }

    reply = (struct mclagd_reply_hdr *)rcv_buf;
    if (reply->info_type != cmd_type->info_type)
    {
        fprintf(stderr, "Reply info type from mclagd error\n");
        ret = EXIT_FAILURE;
        goto mclagdctl_disconnect;
    }

    if (reply->exec_result == EXEC_TYPE_NO_EXIST_SYS)
    {
        fprintf(stderr, "No exist sys in iccpd!\n");
        ret = EXIT_FAILURE;
        goto mclagdctl_disconnect;
    }

    if (reply->exec_result == EXEC_TYPE_NO_EXIST_MCLAGID)
    {
        fprintf(stderr, "Mclag-id %d hasn't been configured in iccpd!\n", para_int);
        ret = EXIT_FAILURE;
        goto mclagdctl_disconnect;
    }

    if (reply->exec_result == EXEC_TYPE_FAILED)
    {
        fprintf(stderr, "exec error in iccpd!\n");
        ret = EXIT_FAILURE;
        goto mclagdctl_disconnect;
    }

    cmd_type->parse_msg((char *)(rcv_buf + sizeof(struct mclagd_reply_hdr)), len - sizeof(struct mclagd_reply_hdr));

    ret = EXIT_SUCCESS;

 mclagdctl_disconnect:
    mclagdctl_sock_close();

    if (rcv_buf)
        free(rcv_buf);

    return ret;
}
