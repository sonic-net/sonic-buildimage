/*
 *
 * $Copyright: (c) 2024 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
*/

#ifndef SMBUS_LOG_H_
#define SMBUS_LOG_H_

#if defined NO_CONSOLE_OUTPUT_SUPPORT

#define LOGX(...)

#else

#include <stdio.h>
#include <stdint.h>
#include "smbus_libtypes.h"

#if defined(_MSC_VER) && (_MSC_VER < 0x1700)
#define __func__ __FUNCTION__
#endif


#define EOL "\n"

typedef enum _LOG_LEVEL {
    LOG_ERROR = 0,
    LOG_MILESTONE,
    LOG_INFO,
    LOG_INFOX,
    LOG_TRACE,
    LOG_DBG1,
    LOG_DBG2,
    LOG_RAW,
} LOG_LEVEL;

enum _LOG_LEVEL getLogLevel(void);
void setLogLevel(enum _LOG_LEVEL level);
const char* getLogLevelStr(enum _LOG_LEVEL level);

void setLogXinfoOption(U8 opt);
void restoreDefLogXinfoOption(void);
int  logHeadInfo(enum _LOG_LEVEL L);
int  logTailInfo(enum _LOG_LEVEL L, int n, const char* fileName, const char* funcName, int lineNum, const char* EoL);

int  log_printf(const char *format, ...);
void setLogFileName(const char* log_fname, int argc, char* const argv[]);

#define LOGX(LVL, ...)  \
  do if (LVL == LOG_RAW) {      \
      log_printf(__VA_ARGS__);      \
    } else if ((LVL) <= getLogLevel()) { \
      int n = logHeadInfo(LVL); n = log_printf(__VA_ARGS__);      \
      logTailInfo(LVL, n, __FILE__, __func__, __LINE__, EOL); \
  }  while(0)

#endif

#endif /* SMBUS_LOG_H_ */

