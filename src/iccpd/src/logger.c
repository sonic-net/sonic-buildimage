/*
 * logger.c
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
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#include "../include/cmd_option.h"
#include "../include/logger.h"

static uint32_t _iccpd_log_level_map[] =
{
    LOG_CRIT,
    LOG_ERR,
    LOG_WARNING,
    LOG_NOTICE,
    LOG_INFO,
    LOG_DEBUG
};

char* log_level_to_string(int level)
{
    switch (level)
    {
        case CRITICAL_LOG_LEVEL:
            return "CRITICAL";

        case ERR_LOG_LEVEL:
            return "ERROR";

        case WARN_LOG_LEVEL:
            return "WARN";

        case NOTICE_LOG_LEVEL:
            return "NOTICE";

        case INFO_LOG_LEVEL:
            return "INFO";

        case DEBUG_LOG_LEVEL:
            return "DEBUG";
    }

    return "INFO";
}

struct LoggerConfig* logger_get_configuration()
{
    static struct LoggerConfig config;

    if (config.init == 0)
    {
        config.console_log_enabled = 0;
        config.log_level = NOTICE_LOG_LEVEL;
        config.init = 1;
    }

    return &config;
}

void logger_set_configuration(int log_level)
{
    struct LoggerConfig* config = logger_get_configuration();

    config->log_level = log_level;
    config->init = 1;

    return;
}

void log_init(struct CmdOptionParser* parser)
{
    struct LoggerConfig* config = logger_get_configuration();

    config->console_log_enabled = parser->console_log;
}

void log_finalize()
{
    /*do nothing*/
}

void write_log(int level, const char* tag, const char* format, ...)
{
    struct LoggerConfig* config = logger_get_configuration();
    char buf[LOGBUF_SIZE];
    va_list args;
    unsigned int   prefix_len;
    unsigned int   avbl_buf_len;
    unsigned int   print_len;

#if 0
    if (!config->console_log_enabled)
        return;
#endif

    if (level > config->log_level)
        return;

    prefix_len = snprintf(buf, LOGBUF_SIZE, "[%s.%s] ", tag, log_level_to_string(level));
    avbl_buf_len = LOGBUF_SIZE - prefix_len;

    va_start(args, format);
    print_len = vsnprintf(buf + prefix_len, avbl_buf_len, format, args);
    va_end(args);

    /* Since osal_vsnprintf doesn't always return the exact size written to the buffer,
     * we must check if the user string length exceeds the remaing buffer size.
     */
    if (print_len > avbl_buf_len)
    {
        print_len = avbl_buf_len;
    }

    buf[prefix_len + print_len] = '\0';
    ICCPD_UTILS_SYSLOG(_iccpd_log_level_map[level], "%s", buf);

    return;
}

