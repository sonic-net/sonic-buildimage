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

typedef struct
{
   IANA_INTF_TYPE_t   type;
   PORT_SPEEDS_t      defaultSpeed;
   uint64             phyCapabilities; /* combination of all applicable  PHY_CAPABILITIES_t */
  /* CONNECTOR_TYPES_t  connectorType;*/
   fec_mode_t         defaultFEC;
  uint32             fecCapabilities; /* combination of all applicable  FEC_CAPABILITY_t */
} SYSAPI_HPC_PORT_DESCRIPTOR_t;

/**************************************************************************
*
* @purpose  Return the number of physical ports, given a slot number.
*
* @param    slotNum      slot number for requested card ID.
*
* @returns  portCount    Number of physical ports in slot number
*
* @notes
*
* @end
*
*************************************************************************/
uint32 sysapiHpcPhysPortsInSlotGet(int slotNum)
{
  return 255; //needs to work on it
}
