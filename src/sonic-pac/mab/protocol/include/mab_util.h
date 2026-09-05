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

#ifndef INCLUDE_MAB_UTIL_H
#define INCLUDE_MAB_UTIL_H

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

#include "comm_mask.h"
#include "apptimer_api.h"
#include "mab_radius.h"
#include "mab_exports.h"


#define MAB_LPORT_KEY_PACK(_x, _y, _z, _val) \
  do { \
    _val |= ((_x<<16) | (_y<<4) | (_z)); \
  } while (0);

#define MAB_LPORT_KEY_UNPACK(_x, _y, _z, _val) \
  do { \
    _x = (_val & 0XFFFF0000)>>16; \
    _y = (_val & 0X0000FFF0)>>4; \
    _z = (_val & 0X0000000F); \
  } while (0);


#define MAB_PORT_GET(_x, _val) \
  _x = (_val & 0XFFFF0000)>>16;


#define MAB_LPORT_GET(_y, _val) \
  _y = (_val & 0X0000FFF0)>>4;


#define MAB_TYPE_GET(_z, _val) \
  _z = (_val & 0X0000000F);


#define MAB_IF_NULLPTR_RETURN_LOG(_p) \
  if ( NULLPTR == _p) \
  { \
    MAB_EVENT_TRACE("%s is NULLPTR.", #_p); \
    return  FAILURE; \
  }

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
RC_t mabHostIsDynamicNodeAllocCheck( AUTHMGR_HOST_CONTROL_t hostMode,  BOOL *valid);

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_MAB_UTIL_H */
