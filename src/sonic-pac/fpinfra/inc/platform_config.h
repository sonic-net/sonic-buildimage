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




#ifndef PLATFORM_CONFIG_H_INCLUDED
#define PLATFORM_CONFIG_H_INCLUDED

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
uint32 platSlotLagSlotNumGet (void);

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
uint32 platSlotVlanSlotNumGet (void);

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
uint32 platSlotCpuSlotNumGet (void);

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
uint32 platSlotLoopbackSlotNumGet (void);

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
uint32 platSlotTunnelSlotNumGet (void);

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
uint32 platSlotServicePortSlotNumGet (void);

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
uint32 platSlotMaxPhysicalPortsPerSlotGet (void);

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
uint32 platIntfMaxCountGet (void);

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
uint32 platIntfTotalMaxCountGet (void);

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
uint32 platIntfVlanIntfMaxCountGet (void);

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
uint32 platIntfLoopbackIntfMaxCountGet (void);

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
uint32 platIntfTunnelIntfMaxCountGet (void);

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
uint32 platIntfServicePortIntfMaxCountGet (void);

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
uint32 platIntfCpuIntfMaxCountGet (void);

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
uint32 platIntfPhysicalIntfMaxCountGet (void);

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
uint32 platIntfLagIntfMaxCountGet (void);

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
uint32 platIntfStackIntfMaxCountGet (void);


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
uint32 platUnitTotalMaxPerStackGet (void);

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
uint32 platUnitMaxPhysicalPortsGet (void);

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
uint32 platSlotMaxPhysicalSlotsPerUnitGet (void);

#endif // PLATFORM_CONFIG_H_INCLUDED
