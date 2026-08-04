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

#include "mab_include.h"
#include "mab_struct.h"
#include "auth_mgr_exports.h"

extern mabBlock_t *mabBlock;

char *mabHostModeStringGet( AUTHMGR_HOST_CONTROL_t hostMode)
{
  switch (hostMode)
  {
    case  AUTHMGR_SINGLE_AUTH_MODE:return "MAB_SINGLE_AUTH_MODE";
    case  AUTHMGR_MULTI_HOST_MODE:return "MAB_MULTI_HOST_MODE";
    case  AUTHMGR_MULTI_AUTH_MODE:return "MAB_MULTI_AUTH_MODE";
    default: return "Unknown host mode";
  }
}

char *mabNodeTypeStringGet(authmgrNodeType_t type)
{
  switch (type)
  {
    case AUTHMGR_PHYSICAL:return "AUTHMGR_PHYSICAL";
    case AUTHMGR_LOGICAL:return "AUTHMGR_LOGICAL";
    default: return "Undefined";
  }
}

char *mabTimerTypeStringGet(mabTimerType_t type)
{
  switch (type)
  {
    case MAB_SERVER_AWHILE:return "MAB_SERVER_AWHILE";
    default: return "Undefined";
  }
}

char *mabVlanTypeStringGet(authmgrVlanType_t type)
{
  switch (type)
  {
    case AUTHMGR_VLAN_RADIUS:return "AUTHMGR_VLAN_RADIUS";
    case AUTHMGR_VLAN_GUEST:return "AUTHMGR_VLAN_GUEST";
    case AUTHMGR_VLAN_UNAUTH:return "AUTHMGR_VLAN_UNAUTH";
    case AUTHMGR_VLAN_DEFAULT:return "AUTHMGR_VLAN_DEFAULT";
    case AUTHMGR_VLAN_BLOCKED:return "AUTHMGR_VLAN_BLOCKED";
    default: return "Undefined";
  }
}
