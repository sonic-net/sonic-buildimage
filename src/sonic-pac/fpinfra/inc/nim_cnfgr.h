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

#ifndef NIM_CNFGR_H
#define NIM_CNFGR_H


/*********************************************************************
* @purpose  phase 1 to Initialize Network Interface Manager component
*
* @param    void
*
* @returns  void
*
* @notes    none
*
* @end
*********************************************************************/
EXT_API RC_t   nimPhaseOneInit();

/*********************************************************************
* @purpose  phase 2 to Initialize Network Interface Manager component
*
* @param    void
*
* @returns  void
*
* @notes    none
*
* @end
*********************************************************************/
EXT_API RC_t   nimPhaseTwoInit();

/*********************************************************************
* @purpose  phase 3 to Initialize Network Interface Manager component
*
* @param    void
*
* @returns  void
*
* @notes    none
*
* @end
*********************************************************************/
EXT_API RC_t   nimPhaseThreeInit();

/*********************************************************************
* @purpose  phase 5(exec) to Initialize Network Interface Manager component
*
* @param    void
*
* @returns  void
*
* @notes    none
*
* @end
*********************************************************************/
EXT_API RC_t   nimPhaseExecInit();

/*********************************************************************
* @purpose  Initialize and start Card Manager Task function.
*
* @param    none
*
* @returns  RC_t  Returns  SUCCESS or  ERROR
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimStartTask();

/*********************************************************************
* @purpose  Allocate all of the interface related data
*
* @param    none
*
* @returns  RC_t  Returns  SUCCESS or  ERROR
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimIntfDataAlloc();

#endif /* NIM_CNFGR_H */
