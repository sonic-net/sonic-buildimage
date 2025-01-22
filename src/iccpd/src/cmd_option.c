/*
 * cmd_option.c
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

#include "../include/cmd_option.h"

struct CmdOption* cmd_option_find(struct CmdOptionParser* parser, char* opt_name)
{
    struct CmdOption* opt = NULL;

    if (opt_name == NULL)
        return NULL;

    LIST_FOREACH(opt, &(parser->option_list), next)
    {
        if (strcmp(opt->option, opt_name) == 0)
            return opt;
    }

    return NULL;
}

void cmd_option_delete(struct CmdOption* opt)
{
    if (opt == NULL)
        return;

    LIST_REMOVE(opt, next);
    if (opt->option != NULL)
        free(opt->option);
    if (opt->parameter != NULL)
        free(opt->parameter);
    if (opt->desc != NULL)
        free(opt->desc);
    free(opt);
}

struct CmdOption* cmd_option_add(struct CmdOptionParser* parser, char* opt_name)
{
    struct CmdOption* opt = NULL;

    if (opt_name == NULL)
        return NULL;
    if ((opt = cmd_option_find(parser, opt_name)) != NULL)
        return opt;

    if ((opt = (struct CmdOption*)malloc(sizeof(struct CmdOption))) == NULL)
    {
        strerror(errno);
    }
    else
    {
        opt->option = opt_name;
        opt->parameter = NULL;
        opt->desc = NULL;
        LIST_INSERT_HEAD(&(parser->option_list), opt, next);
    }

    return opt;
}

static void cmd_option_register(struct CmdOptionParser* parser, char* syntax, char* desc)
{
    char buf[OPTION_MAX_LEN] = {0};
    struct CmdOption* opt = NULL;
    char* opt_name = NULL;
    char* param = NULL;
    char* desc_copy = NULL;
    char* token = NULL;
    char* saveptr;

    if (parser == NULL)
        return;
    if (syntax == NULL)
        return;

    snprintf(buf, OPTION_MAX_LEN - 1, "%s", syntax);

    if ((token = strtok_r(buf, " ", &saveptr)) == NULL)
        return;

    opt_name = strdup(token);
    if ((token = strtok_r(NULL, " ", &saveptr)) != NULL)
        param = strdup(token);
    desc_copy = strdup(desc);
    if ((opt = cmd_option_find(parser, opt_name)) != NULL)
        goto failed;
    if ((opt = cmd_option_add(parser, opt_name)) == NULL)
    {
        goto failed;
    }
    opt->parameter = param;
    opt->desc = desc_copy;

    return;

 failed:
    if (opt_name)
        free(opt_name);
    if (desc_copy != NULL)
        free(desc_copy);
    if (param != NULL)
        free(param);
    if (opt != NULL)
        free(opt);
}

void cmd_option_parser_init(struct CmdOptionParser* parser)
{
    if (parser == NULL)
        return;

    LIST_INIT(&parser->option_list);
    cmd_option_register(parser, "-l <LOG_FILE_PATH>", "Set log file path.\n(Default: /var/log/iccpd.log)");
    cmd_option_register(parser, "-p <TCP_PORT>", "Set the port used for telnet listening port.\n(Default: 2015)");
    cmd_option_register(parser, "-c", "Dump log message to console. (Default: No)");
    cmd_option_register(parser, "-h", "Show the usage.");
}

void cmd_option_parser_finalize(struct CmdOptionParser* parser)
{
    while (!LIST_EMPTY(&(parser->option_list)))
    {
        struct CmdOption* opt = NULL;
        opt = LIST_FIRST(&(parser->option_list));
        cmd_option_delete(opt);
    }
}

void cmd_option_parser_dump_usage(struct CmdOptionParser* parser, char* prog_name)
{
    char buf[MSG_LEN];
    struct CmdOption* opt = NULL;
    int index, begin, length;
    char first_line = 0;

    fprintf(stdout, "Usage: %s [Options]\n", prog_name);
    fprintf(stdout, "\n");
    fprintf(stdout, "Options:\n");
    LIST_FOREACH(opt, &(parser->option_list), next)
    {
        index = 0;
        begin = 0;
        length = 0;
        first_line = 1;
        memset(buf, 0, MSG_LEN);
        if (opt->parameter != NULL)
            snprintf(buf, MSG_LEN - 1, "%s %s", opt->option, opt->parameter);
        else
            snprintf(buf, MSG_LEN - 1, "%s", opt->option);
        fprintf(stdout, "%24s    ", buf);

        while (index < strlen(opt->desc))
        {
            while (index < strlen(opt->desc)
                   && opt->desc[index] != '\n' && length < 49)
            {
                ++index;
                ++length;
            }

            memset(buf, 0, MSG_LEN);
            strncpy(buf, &(opt->desc[begin]), length);
            if (length == 49 && index < strlen(opt->desc)
                && opt->desc[index] != '\n'
                && opt->desc[index - 1] != ' '
                && opt->desc[index] != ' ')
            {
                buf[length] = '-';
                buf[length + 1] = '\0';
            }
            if (length < 49)
                ++index;
            begin = index;
            length = 0;
            if (first_line != 0)
            {
                fprintf(stdout, "%-52s\n", buf);
                first_line = 0;
            }
            else
                fprintf(stdout, "%28c%-52s\n", ' ', buf);
        }

        fflush(stdout);
    }
}

int cmd_option_parser_parse(struct CmdOptionParser* parser, int argc, char* argv[])
{
    int index = 1;
    struct CmdOption* opt = NULL;
    char* opt_name = NULL;
    char* val = NULL;
    int num = 0;

    if (parser == NULL)
        return -255;

    while (index < argc)
    {
        opt_name = argv[index];
        opt = cmd_option_find(parser, opt_name);
        if (opt == NULL)
        {
            fprintf(stderr, "Unknown option %s, skip it.\n", opt_name);
            ++index;
            continue;
        }

        if (opt->parameter != NULL)
        {
            ++index;
            if (index >= argc)
            {
                fprintf(stderr, "Error: Insufficient parameter for option %s\n", opt_name);
                cmd_option_parser_dump_usage(parser, argv[0]);
                return -1;
            }
            val = argv[index];
        }

        if (strncmp(opt_name, "-h", 2) == 0)
        {
            cmd_option_parser_dump_usage(parser, argv[0]);
            return -1;
        }

        if (strncmp(opt_name, "-l", 2) == 0)
            parser->log_file_path = val;

        if (strncmp(opt_name, "-p", 2) == 0)
        {
            num = atoi(val);
            if (num > 0 && num < 65535)
                parser->telnet_port = num;
        }
        else if (strncmp(opt_name, "-c", 2) == 0)
            parser->console_log = 1;
        else
            fprintf(stderr, "Unknown option name %s, skip it.\n", opt_name);

        ++index;
    }

    return 0;
}
