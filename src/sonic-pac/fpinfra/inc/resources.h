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

#ifndef  __INC_RESOURCES_H__
#define  __INC_RESOURCES_H__



#define  MAX_BUFFER_POOLS            128

#define  MAX_NUM_VLAN_INTF  128
#define  MAX_NUM_LOOPBACK_INTF 64
#define  MAX_NUM_TUNNEL_INTF 8
#define  MAX_NUM_SERVICE_PORT_INTF 1
#define  MAX_CPU_SLOTS_PER_UNIT    1
#define  MAX_NUM_LAG_INTF 64
#define  MAX_NUM_STACK_INTF 1

#define  MAX_PHYSICAL_SLOTS_PER_UNIT 1

/* The driver indexes the slot by ZERO base */
/* and the application code is ONE base     */
#define  LAG_SLOT_NUM                         ( MAX_PHYSICAL_SLOTS_PER_UNIT + 0) /* slot is ZERO base */
#define  VLAN_SLOT_NUM                        ( LAG_SLOT_NUM                + 1) /* slot is ZERO base */
#define  CPU_SLOT_NUM                         ( VLAN_SLOT_NUM               + 1)
#define  LOOPBACK_SLOT_NUM                    ( CPU_SLOT_NUM                + 1)
#define  TUNNEL_SLOT_NUM                      ( LOOPBACK_SLOT_NUM           + 1)
#define  SERVICE_PORT_SLOT_NUM                ( TUNNEL_SLOT_NUM             + 1)


#define  SYSNAME         1
#define  ALIASNAME       2
#define  SYSNAME_LONG    3
#define  HOSTNAME        4

#define FD_NIM_DEFAULT_NETWORK_INTERFACE_TYPE                          NW_INTF_TYPE_SWITCHPORT
#define  PLATFORM_MAX_VLAN_ID                            (4094)


/* OS task dispatching parameters.
*/
#define  DEFAULT_STACK_SIZE                           (1024 * 32)
#define  DEFAULT_TASK_SLICE                                1
#define  DEFAULT_TASK_PRIORITY                           255
#define  MEDIUM_TASK_PRIORITY                            100
#define  TASK_PRIORITY_LEVEL(x)                           (x)

#define OSAPI_MAX_TIMERS                                  4000


/* Network Buffers */
/* The maximum network buffers depend upon reserved buffers
   If you are changing one check if you need to change the other
*/
#define  MAX_NETWORK_BUFF_PER_BOX                        246


#endif /* __INC_RESOURCES_H__*/
