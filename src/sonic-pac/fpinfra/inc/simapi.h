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



#ifndef _SIMAPI_H_
#define _SIMAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
* @purpose  Get the Unit's System Mac Address Type
*
* @param    none
*
* @returns  sysMacType   System Mac Address Type
*
* @comments
*
* @end
*********************************************************************/
extern uint32 simGetSystemIPMacType(void);

/*********************************************************************
* @purpose  Get the Unit's System Burned in Mac Address
*
* @param    *sysBIA   @b{(output)} pointer to system burned in mac address
*                              Length  MAC_ADDR_LEN
*
* @returns  none
*
* @comments
*
* @end
*********************************************************************/
extern void simGetSystemIPBurnedInMac( uchar8 *sysBIA);

/*********************************************************************
* @purpose  Get the Unit's System Locally Administered Mac Address
*
* @param    *sysLAA   @b{(output)} pointer to system local admin mac address
*
* @returns  none
*
* @comments
*
* @end
*********************************************************************/
extern void simGetSystemIPLocalAdminMac( uchar8 *sysLAA);

/*********************************************************************
*
* @purpose  Get the stack up time
*
* @param    none
*
* @returns  stack up time in seconds
*
* @comments  Stack up time is the time since the stack performed a cold
*            restart. Stack up time does not reset on a warm restart.
*
* @end
*
*********************************************************************/
extern uint32 simSystemUpTimeGet(void);

/**********************************************************************
* @purpose  know whether the summer-time is in effect or not.
*
* @returns   TRUE
*            FALSE
*
* @end
*********************************************************************/
uint32 simAdjustedTimeGet();

#ifdef __cplusplus
}
#endif

#endif  /* _SIMAPI_H_ */
