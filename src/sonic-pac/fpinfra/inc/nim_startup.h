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


#ifndef NIM_STARTUP_H
#define NIM_STARTUP_H

/* Startup Tree Data structure */
typedef struct
{

  uint32           priority;
   COMPONENT_IDS_t  componentId;
  StartupNotifyFcn    startupFunction;
  void *avlData;

} nimStartUpTreeData_t;

/*********************************************************************
* @purpose  Allocate the memory for the startup AVL tree
*
* @param    void
*
* @returns  void
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimStartUpPhaseOneInit();

/*********************************************************************
* @purpose  Create an StartUp
*
* @param    componentId   @b{(input)}   Component ID of startup function
* @param    priority      @b{(input)}   priority to execute startup function
* @param    startupFcn    @b{(input)}   Function pointer to startup routine
*
* @notes
*
* @end
*
*********************************************************************/
void nimStartUpCreate( COMPONENT_IDS_t componentId,
                      uint32 priority,
                      StartupNotifyFcn startupFcn);

/*********************************************************************
* @purpose  Find the first startup function
*
* @param    pRetData     @b{(input)}  pointer to return data
*
* @returns   SUCCESS if an entry exists
* @returns   FAILURE if no entry exists
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimStartUpFirstGet(nimStartUpTreeData_t *pRetData);

/*********************************************************************
* @purpose  Find the next startup function
*
* @param    pRetData     @b{(input)}  pointer to search/return data
*
* @returns   SUCCESS if a next entry exists
* @returns   FAILURE if no next entry exists
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimStartUpNextGet(nimStartUpTreeData_t *pRetData);

/*********************************************************************
*
* @purpose  Waits for the component to complete its STARTUP processing,
*           then gives the nimStartUpEvSema signaling cardmgr to proceed
*           to the next component.
*
* @param    none
*
* @returns  void
*
* @notes    none
*
* @end
*********************************************************************/
extern void nimStartupEventWait(void);
#endif /* NIM_STARTUP_H */
