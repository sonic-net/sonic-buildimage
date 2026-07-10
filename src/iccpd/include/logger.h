/*
 * logger.h
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

#ifndef LOGGER_H_
#define LOGGER_H_

#include <stdint.h>
#include <syslog.h>

#include "../include/cmd_option.h"

typedef enum _iccpd_log_level_t
{
    CRITICAL_LOG_LEVEL       = 0,
    ERR_LOG_LEVEL            = 1,
    WARN_LOG_LEVEL           = 2,
    NOTICE_LOG_LEVEL         = 3,
    INFO_LOG_LEVEL           = 4,
    DEBUG_LOG_LEVEL          = 5
} _iccpd_log_level_t;


#define LOGBUF_SIZE 1024
#define ICCPD_UTILS_SYSLOG    (syslog)

#define ICCPD_LOG_CRITICAL(tag, format, args ...) write_log(CRITICAL_LOG_LEVEL, tag, format, ## args)
#define ICCPD_LOG_ERR(tag, format, args ...) write_log(ERR_LOG_LEVEL, tag, format, ## args)
#define ICCPD_LOG_WARN(tag, format, args ...) write_log(WARN_LOG_LEVEL, tag, format, ## args)
#define ICCPD_LOG_NOTICE(tag, format, args ...) write_log(NOTICE_LOG_LEVEL, tag, format, ## args)
#define ICCPD_LOG_INFO(tag, format, args ...) write_log(INFO_LOG_LEVEL, tag, format, ## args)
#define ICCPD_LOG_DEBUG(tag, format, args ...) write_log(DEBUG_LOG_LEVEL, tag, format, ## args)

struct LoggerConfig
{
    uint8_t console_log_enabled;
    uint8_t log_level;
    uint8_t init;
};

struct LoggerConfig* logger_get_configuration();
void logger_set_configuration(int log_level);
char* log_level_to_string(int level);
void log_setup(char* progname, char* path);
void log_finalize();
void log_init(struct CmdOptionParser* parser);
void write_log(const int level, const char* tag, const char *format, ...);

#endif /* LOGGER_H_ */


