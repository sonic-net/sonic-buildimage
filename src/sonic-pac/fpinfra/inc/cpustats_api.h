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

#ifndef INCLUDE_CPUSTATS_API_H
#define INCLUDE_CPUSTATS_API_H

#include "l3_addrdefs.h"

/* Amount of flows in the Custom Flow filter */
#define CPU_TRAFFIC_CUSTOM_FLOW_LENGTH  3

typedef enum
{
   SIM_CPU_TRAFFIC_FIRST = 1, /* range checking */

   SIM_CPU_TRAFFIC_STP =  SIM_CPU_TRAFFIC_FIRST,
   SIM_CPU_TRAFFIC_LACPDU,
   SIM_CPU_TRAFFIC_ARP,
   SIM_CPU_TRAFFIC_UDLD,
   SIM_CPU_TRAFFIC_LLDP,
   SIM_CPU_TRAFFIC_IP,
   SIM_CPU_TRAFFIC_OSPF,
   SIM_CPU_TRAFFIC_BGP,
   SIM_CPU_TRAFFIC_DHCP,
   SIM_CPU_TRAFFIC_BCAST,
   SIM_CPU_TRAFFIC_MCAST,
   SIM_CPU_TRAFFIC_UCAST,
   SIM_CPU_TRAFFIC_SRCIP,
   SIM_CPU_TRAFFIC_DSTIP,
   SIM_CPU_TRAFFIC_SRCMAC,
   SIM_CPU_TRAFFIC_DSTMAC,
   SIM_CPU_TRAFFIC_CUSTOM,
   SIM_CPU_TRAFFIC_SRCTCP,
   SIM_CPU_TRAFFIC_DSTTCP,
   SIM_CPU_TRAFFIC_SRCUDP,
   SIM_CPU_TRAFFIC_DSTUDP,

   SIM_CPU_TRAFFIC_LAST       /* range checking */
} simCpuTraffic_protocol_t;

/* Indices values for cpu traffic filter masks */
/* Number of bytes in mask */
#define  CPU_TRAFFIC_INDICES   (( SIM_CPU_TRAFFIC_LAST - 1) / (sizeof( uchar8) * 8) + 1)

/* structure definition for the cpu traffic filter Mask from cpustats_api.h*/
typedef struct
{
   uchar8 value[ CPU_TRAFFIC_INDICES];
}  CPU_FILTER_MASK_t;

typedef struct
{
   ushort16 offset[CPU_TRAFFIC_CUSTOM_FLOW_LENGTH];
   ushort16 val[CPU_TRAFFIC_CUSTOM_FLOW_LENGTH];
   ushort16 mask[CPU_TRAFFIC_CUSTOM_FLOW_LENGTH];
} cpuTrafficCustomFlow_t;


typedef struct cpuTrafficMatch_s
{
   CPU_FILTER_MASK_t   filterMask;
   enetMacAddr_t       srcMac;
   enetMacAddr_t       dstMac;
   l3_inet_addr_t         srcIp;
   l3_inet_addr_t         dstIp;
  cpuTrafficCustomFlow_t custFlow;
   ushort16            srcTcp;
   ushort16            dstTcp;
   ushort16            srcUdp;
   ushort16            dstUdp;
} cpuTrafficMatch_t;



#endif /* INCLUDE_CPUSTATS_API_H */
