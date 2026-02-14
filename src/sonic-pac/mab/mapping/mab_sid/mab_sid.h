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

#ifndef INCLUDE_MAB_SID_H
#define INCLUDE_MAB_SID_H

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

#include "commdefs.h"
#include "datatypes.h"

/*********************************************************************
* @purpose  Get thread default Stack Size
*
* @param    void    
*
* @returns  CNFGR_MSG_HANDLER_THREAD_STACK_SIZE 
*
* @comments none 
*       
* @end
*********************************************************************/
 int32 mabSidDefaultStackSize();

/*********************************************************************
* @purpose  Get thread default task slice
*
* @param    void    
*
* @returns  CNFGR_MSG_HANDLER_THREAD_STACK_SIZE 
*
* @comments none 
*       
* @end
*********************************************************************/
 int32 mabSidDefaultTaskSlice();

/*********************************************************************
* @purpose  Get thread default task priority
*
* @param    void    
*
* @returns  CNFGR_MSG_HANDLER_THREAD_STACK_SIZE 
*
* @comments none 
*       
* @end
*********************************************************************/
 int32 mabSidDefaultTaskPriority();

/* USE C Declarations */
#ifdef __cplusplus
}
#endif


#endif /* INCLUDE_MAB_SID_H */
