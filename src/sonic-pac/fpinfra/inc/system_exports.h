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


#ifndef __SYSTEM_EXPORTS_H_
#define __SYSTEM_EXPORTS_H_

/* STACKING Component Feature List */
typedef enum
{
  /* STACKING supported */
   STACKING_FEATURE_SUPPORTED = 0,            /* general support statement */
   FPS_FEATURE_ID,                            /* Front-panel stacking support */
   FPS_QOS_REPLACEMENT_FEATURE_ID,            /* Special QOS support for front panel stacking ports */
   FPS_PORT_MODE_CFG_FEATURE_ID,              /* Ports are configurable from Ethernet to Stack mode */
   STACKING_NSF_FEATURE_ID,                   /* nonstop forwarding */
   STACKING_SFS_FEATURE_ID,                   /* Stack Firmware Sync Feature */
   STACK_PORT_DIAGNOSTICS_FEATURE_ID,         /* Stack Port Diagnostics info Feature */
   STACKING_LINUX_SYSTEM_HOST_FEATURE_ID,     /* Linux system host (same as SIM, but checked for Stacking component) */
   FPS_PORT_SPEED_CFG_FEATURE_ID,             /* Stack port speed is configurable */
   STACKING_FEATURE_ID_TOTAL                  /* total number of enum values */
}  STACKING_FEATURE_IDS_t;


#define MBUF_CLASS_NAME_LEN  32

/* Receive mbuf priorities. Used when requesting a receive mbuf.
 * Note that the value 0 is reserved, since it maps to the tx
 * priority in e_MbufClassIndex. */
typedef enum
{
   MBUF_RX_PRIORITY_NULL = 0,
   MBUF_RX_PRIORITY_HIGH = 1,
   MBUF_RX_PRIORITY_MID0 = 2,
   MBUF_RX_PRIORITY_MID1 = 3,
   MBUF_RX_PRIORITY_MID2 = 4,
   MBUF_RX_PRIORITY_NORMAL = 5
}  MBUF_RX_PRIORITY;

/* Mbuf class indices. Used for reporting mbuf statistics to UI. */
typedef enum
{
  MBUF_CLASS_TX = 0,
  MBUF_CLASS_RX_HIGH =  MBUF_RX_PRIORITY_HIGH,
  MBUF_CLASS_RX_MID0 =  MBUF_RX_PRIORITY_MID0,
  MBUF_CLASS_RX_MID1 =  MBUF_RX_PRIORITY_MID1,
  MBUF_CLASS_RX_MID2 =  MBUF_RX_PRIORITY_MID2,
  MBUF_CLASS_RX_NORMAL =  MBUF_RX_PRIORITY_NORMAL,
  MBUF_CLASS_LAST
} e_MbufClassIndex;

/* structure to communicate mbuf statistics to UI. */
typedef struct mbufStats_s
{
  /* mbuf priority. Corresponds to priorities defined in  MBUF_RX_PRIORITY
   * with tx priority considered to be 0. The priority is used as a table
   * index for SNMP. */
  e_MbufClassIndex mbufClass;

  /* Descriptive name of this class of mbufs. */
   uchar8 mbufClassName[MBUF_CLASS_NAME_LEN];

  /* Number of allocation attempts for this class of mbufs. */
  uint32 mbufAllocAttempts;

  /* Number of allocation failures for this class of mbufs. */
  uint32 mbufAllocFails;

} mbufStats_t;

typedef enum
{
   SYSMAC_NONE = 0,
   SYSMAC_BIA,            /* Burned-In Address */
   SYSMAC_LAA             /* Locally-Administered Address */
}  MACADDR_TYPE_t;


#endif /* __SYSTEM_EXPORTS_H_*/
