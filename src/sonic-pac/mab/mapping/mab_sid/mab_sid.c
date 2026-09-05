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

#include "pacinfra_common.h"
#include "mab_common.h"
#include "mab_sid.h"
#include "mab_sid_const.h"


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
 int32 mabSidDefaultStackSize()
{
  return ( FD_CNFGR_MAB_DEFAULT_STACK_SIZE );
}

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
 int32 mabSidDefaultTaskSlice()
{
  return ( FD_CNFGR_MAB_DEFAULT_TASK_SLICE );
}

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
 int32 mabSidDefaultTaskPriority()
{
  return ( FD_CNFGR_MAB_DEFAULT_TASK_PRI );
}

