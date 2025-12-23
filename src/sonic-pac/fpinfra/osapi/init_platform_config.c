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


#include "datatypes.h"
#include "resources.h"
#include "product.h"

/*********************************************************************
* @purpose  Get the LAG slot number
*
* @param    void
*
* @returns
*
* @notes    none
*
* @end
*********************************************************************/
uint32 platSlotLagSlotNumGet (void)
{
  return  LAG_SLOT_NUM;
}

/*********************************************************************
* @purpose  Get the VLAN slot number
*
* @param    void
*
* @returns
*
* @notes    none
*
* @end
*********************************************************************/
uint32 platSlotVlanSlotNumGet (void)
{
  return  VLAN_SLOT_NUM;
}

/*********************************************************************
* @purpose  Get the CPU slot number
*
* @param    void
*
* @returns
*
* @notes    none
*
* @end
*********************************************************************/
uint32 platSlotCpuSlotNumGet (void)
{
  return  CPU_SLOT_NUM;
}

/*********************************************************************
* @purpose  Get the loopback slot number
*
* @param    void
*
* @returns
*
* @notes    none
*
* @end
*********************************************************************/
uint32 platSlotLoopbackSlotNumGet (void)
{
  return  LOOPBACK_SLOT_NUM;
}

/*********************************************************************
* @purpose  Get the tunnel slot number
*
* @param    void
*
* @returns
*
* @notes    none
*
* @end
*********************************************************************/
uint32 platSlotTunnelSlotNumGet (void)
{
  return  TUNNEL_SLOT_NUM;
}

/*********************************************************************
* @purpose  Get the service port slot number
*
* @param    void
*
* @returns
*
* @notes    none
*
* @end
*********************************************************************/
uint32 platSlotServicePortSlotNumGet (void)
{
  return  SERVICE_PORT_SLOT_NUM;
}

/*********************************************************************
* @purpose  Get the maximum number of interfaces for the device.
*
* @param    void
*
* @returns
*
* @notes    none
*
* @end
*********************************************************************/
uint32 platIntfTotalMaxCountGet (void)
{
  return  MAX_INTERFACE_COUNT;
}

/*********************************************************************
* @purpose  Maximum number of Vlan Interfaces
*
* @param    void
*
* @returns
*
* @notes    none
*
* @end
*********************************************************************/
uint32 platIntfVlanIntfMaxCountGet (void)
{
  return  MAX_NUM_VLAN_INTF;
}

/*********************************************************************
* @purpose  Maximum number of Loopback Interfaces
*
* @param    void
*
* @returns
*
* @notes    none
*
* @end
*********************************************************************/
uint32 platIntfLoopbackIntfMaxCountGet (void)
{
  return  MAX_NUM_LOOPBACK_INTF;
}

/*********************************************************************
* @purpose  Maximum number of Tunnel Interfaces
*
* @param    void
*
* @returns
*
* @notes    none
*
* @end
*********************************************************************/
uint32 platIntfTunnelIntfMaxCountGet (void)
{
  return  MAX_NUM_TUNNEL_INTF;
}

/*********************************************************************
* @purpose  Maximum number of Service Port Interfaces
*
* @param    void
*
* @returns
*
* @notes    none
*
* @end
*********************************************************************/
uint32 platIntfServicePortIntfMaxCountGet (void)
{
  return  MAX_NUM_SERVICE_PORT_INTF;
}

/*********************************************************************
* @purpose  Maximum number of CPU Interfaces
*
* @param    void
*
* @returns
*
* @notes    none
*
* @end
*********************************************************************/
uint32 platIntfCpuIntfMaxCountGet (void)
{
  return  MAX_CPU_SLOTS_PER_UNIT;
}

/*********************************************************************
* @purpose  Maximum number of Physical Interfaces
*
* @param    void
*
* @returns
*
* @notes    none
*
* @end
*********************************************************************/
uint32 platIntfPhysicalIntfMaxCountGet (void)
{
  return  MAX_PORT_COUNT;
}

/*********************************************************************
* @purpose  Maximum number of Stack Interfaces
*
* @param    void
*
* @returns
*
* @notes    none
*
* @end
*********************************************************************/
uint32 platIntfStackIntfMaxCountGet (void)
{
  return  MAX_NUM_STACK_INTF;
}

/*********************************************************************
* @purpose  Maximum number of Lag Interfaces
*
* @param    void
*
* @returns
*
* @notes    none
*
* @end
*********************************************************************/
uint32 platIntfLagIntfMaxCountGet (void)
{
   return  MAX_NUM_LAG_INTF;
}

/*********************************************************************
* @purpose  Maximum number of Units in a stack
*
* @param    void
*
* @returns
*
* @notes    none
*
* @end
*********************************************************************/
uint32 platUnitTotalMaxPerStackGet (void)
{
  return  MAX_UNITS_PER_STACK;
}

/*********************************************************************
* @purpose  Get maximum number of Physical slots per unit.
*
* @param    void
*
* @returns
*
* @notes    none
*
* @end
*********************************************************************/
uint32 platSlotMaxPhysicalSlotsPerUnitGet (void)
{
  return  MAX_PHYSICAL_SLOTS_PER_UNIT;
}

/*********************************************************************
* @purpose  Get maximum number of Physical ports per slot.
*
* @param    void
*
* @returns
*
* @notes    none
*
* @end
*********************************************************************/
uint32 platSlotMaxPhysicalPortsPerSlotGet (void)
{
  return  MAX_PHYSICAL_PORTS_PER_SLOT;
}

/*********************************************************************
* @purpose  Maximum number of ports per unit
*
* @param    void
*
* @returns
*
* @notes    none
*
* @end
*********************************************************************/
uint32 platUnitMaxPhysicalPortsGet (void)
{
  return  MAX_PHYSICAL_PORTS_PER_UNIT;
}

/*********************************************************************
* @purpose  Get maximum number of interfaces supported by the switch.
*
* @param    void
*
* @returns
*
* @notes    none
*
* @end
*********************************************************************/
uint32 platIntfMaxCountGet (void)
{
  return  MAX_INTERFACE_COUNT;
}

