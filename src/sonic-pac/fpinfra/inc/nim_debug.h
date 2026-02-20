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

#ifndef NIM_DEBUG_H
#define NIM_DEBUG_H

#include "pacinfra_common.h"

/* Flags used for enabling NIM tracing */
#define  NIM_FLAG_LOG_MSG      ((uint32)(1 << 0)) /* Print NIM LOG Messages on the console */
#define  NIM_FLAG_STACK_TRACE  ((uint32)(1 << 1)) /* Print stack trace of the current task in case of failures */

void nimDebugDataDumpShow();
void nimDebugDataDumpShowTerse();
void nimDebugMsgQueueShow();
/*********************************************************************
* print content of nimMsgQueue
*
* @param    none
*
* @end
  *********************************************************************/
void nimDebugMsgQueueDump(void);


void nimLoggingSet(uint32 value);


void nim_log_msg_fmt( char8 * file, uint32 line,  char8 * msg);

typedef struct
{
   int32   failedComponent;  /* input */
}  NIM_CRASH_DATA_t;

extern  NIM_CRASH_DATA_t nimCrashInfo;
extern FILE *globalCrashLogFp;


/*********************************************************************
* @purpose  Print devshell help
*
* @param    None.
*
* @returns  None.
*
* @notes
*
* @end
*
*********************************************************************/
void nimDevshellHelpPrint();

#endif
 
