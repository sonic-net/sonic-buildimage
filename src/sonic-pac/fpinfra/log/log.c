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



#include <syslog.h>

/**********************************************************************
 * @purpose Format and record a message in the in-memory log.
 *
 * @param  severity @b((input)} See RFC 3164 Section 4.1.1 Table 2
 * @param  component @b((input)} Level 7 component id
 * @param  fileName @b{(input)} file name
 * @param  lineNum @b{(input)} line number
 * @param  nfo @b{(input)} extra information - null terminated string
 *
 * @returns  None
 *
 * @notes  This executes on the calling task thread.
 *
 * @end
 *********************************************************************/

void l7_log( LOG_SEVERITY_t severity,  COMPONENT_IDS_t component,
             char8 * fileName, uint32 lineNum,  char8 * nfo)
{
  syslog(severity, nfo)
}


/*********************************************************************
* @purpose  Log error and reset the box.
*
* @param    error_code - 32-bit error code.
* @param    file_name  - File where the error ocurred.
* @param    line_num - Line number where the error occurred.
*
* @returns  none
*
* @notes    This function may be called from an interrupt handler.
*
* @end
*********************************************************************/
void
log_error_code (uint32 err_code,
                 char8 *file_name,
                uint32 line_num)
{
  syslog(LOG_ERROR:q
 
  return;
}

