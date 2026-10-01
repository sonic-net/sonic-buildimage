/*
 * Copyright 2024 Broadcom Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <syslog.h>
#include <stdio.h>
#include "datatypes.h"

/* Maximum message log size */
#define LOG_MSG_MAX_MSG_SIZE                   512
#define  LOG_FORMAT_BUF_SIZE                 LOG_MSG_MAX_MSG_SIZE

/* LOG SEVERITY LEVELS */

typedef enum
{
  /* Emergency (0): system is unusable. System failure has occurred. */
   LOG_SEVERITY_EMERGENCY = 0,
  /* Alert (1): action must be taken immediately.
  * Unrecoverable failure of a component. System failure likely.
  */
   LOG_SEVERITY_ALERT = 1,
  /* Critical (2): critical conditions.
  * Recoverable failure of a component that may lead to system failure.
  */
   LOG_SEVERITY_CRITICAL = 2,
  /*  Error (3): error conditions. Recoverable failure of a component. */
   LOG_SEVERITY_ERROR = 3,
  /* Warning (4): warning conditions. Minor failure,
  * e.g. misconfiguration of a component.
  */
   LOG_SEVERITY_WARNING = 4,
  /*  Notice(5): normal but significant conditions. */
   LOG_SEVERITY_NOTICE = 5,
  /* Informational(6): informational messages. */
   LOG_SEVERITY_INFO = 6,
  /* Debug(7): debug-level messages. */
   LOG_SEVERITY_DEBUG = 7,
  /* None should be last value*/
   LOG_SEVERITY_NONE = 0xff
}  LOG_SEVERITY_t;



/* Default Msg Log Macro for all logs  */
/* Log with a format string and variable arguments */
/* Extra casts are for cpp support */
#ifndef LOGF
#define LOGF syslog
#endif
#if 0
#define  LOGF(__sev__, __comp__, __fmt__,__args__...)                         \
       {   \
          syslog(__sev__,  __fmt__, ## __args__); \
       }
#endif
#define LOG_ERROR(error_code)                                                   \
       {   \
          syslog(LOG_ERR, "ERROR(%d) at %s:%d", (uint32) (error_code), __FILE__, __LINE__); \
       }

#define log_error_code(error_code, file, line) LOG_ERROR(error_code)

#define LOG_ERROR_OPT_RESET(__sev__,                          \
                              error_code, __logmsg__ )                  \
       {   \
           char8 l7_logf_buf[ LOG_FORMAT_BUF_SIZE];    \
          snprintf (l7_logf_buf,  LOG_FORMAT_BUF_SIZE, "ERROR(%d:%s) at %s:%d", (uint32) (error_code), __logmsg__, __FILE__, __LINE__);   \
          l7_logf_buf[ LOG_FORMAT_BUF_SIZE - 1] = 0;    \
          syslog(LOG_ERR, "%s", l7_logf_buf); \
       }


#ifdef __cplusplus
}
#endif

#endif

