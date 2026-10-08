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

#ifndef NIM_IFINDEX_H
#define NIM_IFINDEX_H


#include "datatypes.h"
#include "commdefs.h"
#include "cnfgr_api.h"
#include "nim_data.h"

/* Shifts for creating unique IF_INDICES */
#define NIM_UNIT_MASK 0xff000000
#define NIM_SLOT_MASK 0x00ff0000
#define NIM_PORT_MASK 0x0000ffff

#define NIM_UNIT_SHIFT 24
#define NIM_SLOT_SHIFT 16
#define NIM_PORT_SHIFT  0


typedef struct {
  uint32 ifIndex;
  uint32 intIfNum;
  void *avlData;
} nimIfIndexTreeData_t;

/*********************************************************************
* @purpose  Get the unit id from ifIndex  
*
*
* @param    ifIndex
*
* @returns  unit id
*
* @notes   
*
*       
* @end
*
*********************************************************************/
uint32 nimIfIndexGetUnit(uint32 ifIndex);


/*********************************************************************
* @purpose  Get the slot id from ifIndex  
*
* @param    ifIndex
*
* @returns  slot id
*
* @notes   
*
*       
* @end
*
*********************************************************************/
uint32 nimIfIndexGetSlot(uint32 ifIndex);


/*********************************************************************
* @purpose  Get the port id from ifIndex  
*
* @param    ifIndex
*
* @returns  port id
*
* @notes   
*
*       
* @end
*
*********************************************************************/
uint32 nimIfIndexGetPort(uint32 ifIndex);

/*********************************************************************
* @purpose  Create an IfIndex
*
* @param    usp         @b{(input)}   The usp to encode in the ifIndex
* @param    type        @b{(input)}   The type of interface 
* @param    intIfNum    @b{(input)}   The intIfNum to marry the extIfNum to
* @param    ifIndex     @b{(output)}  The ifIndex created
*
* @returns   SUCCESS or  FAILURE
*
* @notes   
*
*       
* @end
*
*********************************************************************/
void nimIfIndexCreate(nimUSP_t usp, INTF_TYPES_t type,uint32 *ifIndex, uint32 intIfNum);

/*********************************************************************
* @purpose  Delete an IfIndex
*
* @param    ifIndex     @b{(output)}  The ifIndex to delete
*
* @returns   SUCCESS or  FAILURE
*
* @notes   
*
*       
* @end
*
*********************************************************************/
void nimIfIndexDelete(uint32 ifIndex);

/*********************************************************************
* @purpose  Find the first external inferface number
*
* @param    pExtIfNum      @b{(input)}  The present extIfNum
* @param    pIntIfNum  @b{(output)} The internal interface number
*
* @returns   ERROR if the extIfNum doesn't exist
*
* @notes    none 
*
* @end
*********************************************************************/
RC_t nimIfIndexFirstGet(uint32 *pExtIfNum, uint32 *pIntIfNum);

/*********************************************************************
* @purpose  Find the next external inferface number
*
* @param    extIfNum      @b{(input)}  The present extIfNum
* @param    pNextExtIfNum  @b{(output)} The next extIfNum
* @param    pIntIfNum  @b{(output)} The internal interface number
*
* @returns   ERROR if the extIfNum doesn't exist
*
* @notes    none 
*
* @end
*********************************************************************/
RC_t nimIfIndexNextGet(uint32 extIfNum, uint32 *pNextExtIfNum, uint32 *pIntIfNum);

/*********************************************************************
* @purpose  Find the intIfNum associated with the extIfNum 
*
* @param    extIfNum   @b{(input)}  The present extIfNum
* @param    pIntIfNum  @b{(output)} The internal interface number
*
* @returns   ERROR if the extIfNum doesn't exist
*
* @notes    none 
*
* @end
*********************************************************************/
RC_t nimIfIndexIntIfNumGet(uint32 extIfNum, uint32 *pIntIfNum);

/*********************************************************************
* @purpose  Allocate the memory for the ifIndex AVL tree
*
* @param    void
*
* @returns  void
*
* @notes    none 
*
* @end
*********************************************************************/
RC_t nimIfIndexPhaseOneInit();

/*********************************************************************
* @purpose  Find the intIfNum associated with the extIfNum 
*
* @param    extIfNum      @b{(input)}  The present extIfNum
* @param    pNextExtIfNum  @b{(output)} The next extIfNum
*
* @returns   ERROR if the extIfNum doesn't exist
*
* @notes    none 
*
* @end
*********************************************************************/
void nimIfIndexDataClear(void);
#endif /* NIM_IFINDEX_H */
