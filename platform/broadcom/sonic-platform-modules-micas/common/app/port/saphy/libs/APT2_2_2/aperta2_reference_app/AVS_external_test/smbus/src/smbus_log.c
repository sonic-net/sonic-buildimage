/*
 *
 * $Copyright: (c) 2024 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
*/

#include "smbus_log.h"
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

static enum _LOG_LEVEL logLevel = LOG_INFO;
static U8 xinfo[] = {0x17, 0x11, 0x11, 0x13, 0x13, 0x17, 0x0};
static U8 currentXinfoOpt = 0x11, tailMargin = 120;
static FILE* fpLog = NULL;

const char* getTimeStamp(void)
{
    struct tm* local;
    time_t t = time(NULL);
    local = localtime(&t);
    return(asctime(local));
}

enum _LOG_LEVEL getLogLevel(void)
{
    return logLevel;
}

void setLogLevel(enum _LOG_LEVEL level)
{
    logLevel = level;
    currentXinfoOpt = 0xFF;
}

void setLogXinfoOption(U8 opt)
{
    currentXinfoOpt = opt;
}

static void logTimestamp(FILE *fp, const char* info_str)
{
    char time_str[64] = {'\0'};
    time_t the_time;
    struct tm *ptm;

    time(&the_time);
    ptm = localtime(&the_time);

    strftime(time_str, sizeof(time_str), "%a %b %d %H:%M:%S %Y", ptm);
    fprintf(fp, "\n======== LOG %s on %s ========\n", info_str, time_str);
}

void closeLogFile(void)
{
    if (fpLog) {
        logTimestamp(fpLog, " Ends ");
        fclose(fpLog);
    }
}

void setLogFileName(const char* log_fname, int argc, char* const argv[])
{
    if (fpLog) fclose(fpLog);

    fpLog = fopen(log_fname, "wt");

    if (fpLog != NULL) {
        int k;
        logTimestamp(fpLog, "Starts");
        fprintf(fpLog, "User %s @  computer %s ran \n", getenv("UserName"), getenv("ComputerName"));
        for (k=0; k<argc; ++k) {
            fprintf(fpLog, "%s ", argv[k]);
        }
        fprintf(fpLog, "\n");
        atexit(closeLogFile);
    }
}

int  logHeadInfo(enum _LOG_LEVEL L)
{
    int n = 0;
    char buf[64] = {'\0'}, *p = buf;
    U8 xinfo_opt = (currentXinfoOpt != 0xFF) ? currentXinfoOpt : xinfo[L];

    if (xinfo_opt & 0x20) {
        n += sprintf(p, "%s ", getTimeStamp());
        p += n;
    }

    if (xinfo_opt & 0x10) {
        n += sprintf(p, "{%-5s} ", getLogLevelStr(L));
    }

    buf[n] = '\0';
    printf("%s", buf);
    if (fpLog) {
        fprintf(fpLog, "%s", buf);
    }
    return n;
}

int  logTailInfo(enum _LOG_LEVEL L, int n, const char* fileName, const char* funcName, int lineNum, const char* EoL)
{
    U8 xinfo_opt = (currentXinfoOpt != 0xFF) ? currentXinfoOpt : xinfo[L];
    char buf[512] = {'\0'}, *p = buf;
    int cp;

    if ((xinfo_opt & 0xF)) {
        cp = sprintf(p, "%*s", (n<tailMargin) ? (tailMargin-n) : 0, "");
        n += cp; p += cp;
    }

    if (xinfo_opt & 0x1) {
        cp = sprintf(p, "[%s()]", funcName);
        n += cp; p += cp;
    }

    if (xinfo_opt & 0x4) {
        cp = sprintf(p, "@%s", fileName);
        n += cp; p += cp;
    }

    if (xinfo_opt & 0x2) {
        cp = sprintf(p, ":%d", lineNum);
        n += cp; p += cp;
    }

    *p = '\0';
    n += printf("%s%s", buf, EoL);
    if (fpLog) {
        fprintf(fpLog, "%s%s", buf, EoL);
        fflush(fpLog);
    }
    return n;
}

const char* getLogLevelStr(enum _LOG_LEVEL level)
{
    const char* LOG_LEVEL_STR[] = {"ERROR", " M/S ", "INFO ", "INFOX", "TRACE", "DBG_1", "DBG_2"};
    return LOG_LEVEL_STR[level];
}

int  log_printf(const char *format, ...)
{
    static char buf[4096] = {0};
    int n = 0;
    va_list args;
    va_start(args, format);

    if (fpLog) {
        n = vsprintf(buf, format, args);
        fprintf(fpLog, "%s", buf);
        printf("%s", buf);
    } else {
        n = vprintf(format, args);
    }
    va_end (args);

    return n;
}

