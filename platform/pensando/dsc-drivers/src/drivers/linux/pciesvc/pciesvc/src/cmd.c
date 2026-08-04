// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022, Advanced Micro Devices Inc.
 */

#include "pciesvc_impl.h"

static pciesvc_cmdres_t resbuf;

static int
cmd_nop(const pciesvc_cmd_nop_t *cmd,
        pciesvc_cmdres_nop_t *res)
{
    res->status = 0;
    return 0;
}

static int
cmd_set_log_level(const pciesvc_cmd_set_log_level_t *cmd,
                  pciesvc_cmdres_set_log_level_t *res)
{
    res->old_level = pciesvc_log_level;
    pciesvc_log_level = cmd->log_level;
    res->status = 0;
    return 0;
}

int
pciesvc_cmd_read(char *buf, const long int off, const size_t count)
{
    int n;

    if (off < 0 || off > sizeof(resbuf)) {
        return -1;
    }
    if (off + count > sizeof(resbuf)) {
        /* clamp read size to remainder of resbuf */
        n = sizeof(resbuf) - off;
    } else {
        n = count;
    }
    pciesvc_memcpy(buf, ((char *)&resbuf) + off, n);
    return n;
}

int
pciesvc_cmd_write(const char *buf, const long int off, const size_t count)
{
    pciesvc_cmd_t *cmd;
    pciesvc_cmdres_t *res = &resbuf;
    int r;

    if (off != 0 || count < sizeof(pciesvc_cmd_t)) {
        return -1;
    }

    cmd = (pciesvc_cmd_t *)buf;
    pciesvc_memset(res, 0, sizeof(*res));

    switch (cmd->cmd) {
    case PCIESVC_CMD_NOP:
        r = cmd_nop(&cmd->nop, &res->nop);
        break;
    case PCIESVC_CMD_SET_LOG_LEVEL:
        r = cmd_set_log_level(&cmd->set_log_level, &res->set_log_level);
        break;
    default:
        res->status = PCIESVC_CMDSTATUS_UNKNOWN_CMD;
        r = 0;  /* cmd_write "succeeded" */
        break;
    }

    return r < 0 ? r : count;
}
