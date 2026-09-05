/*
 * cmd_option.h
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

#ifndef CMD_OPTION_H_
#define CMD_OPTION_H_

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#define OPTION_MAX_LEN 256
#define MSG_LEN  81

#define CMD_OPTION_PARSER_INIT_VALUE \
    { \
        .log_file_path = "/var/log/iccpd.log", \
        .pid_file_path = "/var/run/iccpd/iccpd.pid", \
        .cmd_file_path = "/var/run/iccpd/iccpd.vty", \
        .config_file_path = "/etc/iccpd/iccpd.conf", \
        .mclagdctl_file_path = "/var/run/iccpd/mclagdctl.sock", \
        .console_log = 0, \
        .telnet_port = 2015, \
        .init = cmd_option_parser_init, \
        .finalize = cmd_option_parser_finalize, \
        .dump_usage = cmd_option_parser_dump_usage, \
        .parse = cmd_option_parser_parse, \
    }

struct CmdOption
{
    char* desc;
    char* option;
    char* parameter;
    LIST_ENTRY(CmdOption) next;
};

struct CmdOptionParser
{
    char* log_file_path;
    char* pid_file_path;
    char* cmd_file_path;
    char* config_file_path;
    char *mclagdctl_file_path;
    uint8_t console_log;
    uint16_t telnet_port;
    LIST_HEAD(option_list, CmdOption) option_list;
    int (*parse)(struct CmdOptionParser*, int, char*[]);
    void (*init)(struct CmdOptionParser*);
    void (*finalize)(struct CmdOptionParser*);
    void (*dump_usage)(struct CmdOptionParser*, char*);
};

int cmd_option_parser_parse(struct CmdOptionParser*, int, char*[]);
struct CmdOption* cmd_option_add(struct CmdOptionParser*, char*);
struct CmdOption* cmd_option_find(struct CmdOptionParser*, char*);
void cmd_option_delete(struct CmdOption*);
void cmd_option_parser_init(struct CmdOptionParser*);
void cmd_option_parser_finalize(struct CmdOptionParser*);
void cmd_option_parser_dump_usage(struct CmdOptionParser*, char*);

#endif /* CMD_OPTION_H_ */
