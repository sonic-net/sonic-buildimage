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

#ifndef INCLUDE_MAB_SID_CONST_H
#define INCLUDE_MAB_SID_CONST_H

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif


/*
***********************************************************************
*                          CONSTANTS
***********************************************************************
*/

enum 
{
  FD_CNFGR_MAB_DEFAULT_STACK_SIZE     =  DEFAULT_STACK_SIZE,
  FD_CNFGR_MAB_DEFAULT_TASK_SLICE     =  DEFAULT_TASK_SLICE,
  FD_CNFGR_MAB_DEFAULT_TASK_PRI       =  TASK_PRIORITY_LEVEL( DEFAULT_TASK_PRIORITY)
};

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_MAB_SID_CONST_H */

