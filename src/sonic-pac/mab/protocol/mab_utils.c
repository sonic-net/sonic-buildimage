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

#include <ctype.h>
#include "mab_include.h"
#include "mab_db.h"
#include "mab_client.h"
#include "mab_timer.h"
#include "mab_control.h"
#include "mab_struct.h"

extern mabBlock_t *mabBlock;
/*********************************************************************
 * @purpose function to check policy validation based on host mode 
 *
 * @param    hostMode   @b{(input))  hostmode 
 * @param    *appyPolicy  @b{(input)) bool value 
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabHostIsDynamicNodeAllocCheck( AUTHMGR_HOST_CONTROL_t hostMode,  BOOL *valid)
{
	*valid =  TRUE;
	return  SUCCESS;
}


RC_t mabNimIntfStateCheck(uint32 intIfNum)
{
   INTF_STATES_t  state;
  mabPortCfg_t   *pCfg;

  if(mabIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
  {
    return  FAILURE;
  }

  state = nimGetIntfState(intIfNum);
  if ((state !=  INTF_ATTACHED)
      && (state !=  INTF_ATTACHING)
      && (state !=  INTF_DETACHING))
  {
    return  FAILURE;
  }

  return  SUCCESS;
}
