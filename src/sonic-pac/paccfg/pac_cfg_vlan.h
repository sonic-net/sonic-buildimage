/*
 * Copyright 2019 Broadcom. The term "Broadcom" refers to Broadcom Inc. and/or
 * its subsidiaries.
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

#ifdef _PAC_CFG_VLAN_H
#define _PAC_CFG_VLAN_H

#include "pac_vlancfg.h"
#include "pacinfra_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Set port PVID */
RC_t pacCfgPortPVIDSet(char *interface, int pvid);

/* Set port PVID */
RC_t pacCfgPortPVIDGet(char *interface, int *pvid);

/* Get port PVID */
RC_t pacCfgPortPVIDGet(char *interface, int *pvid);

/* Set port VLAN membership */
RC_t pacCfgVlanMemberAdd(int vlan, char *interface, dot1qTaggingMode_t mode);

/* Remove port VLAN membership */
RC_t pacCfgVlanMemberRemove(int vlan, char *interface);

/* Remove all port VLAN membership */
RC_t pacCfgVlanMemberClean(int vlan);

/* Add a dynamic VLAN */
RC_t pacCfgVlanAdd(int vlan);

/* Remove a dynamic VLAN */
RC_t pacCfgVlanRemove(int vlan);

/* Remove all dynamic VLAN */
void pacCfgVlanCleanup(void);

/* Add a reserved dynamic VLAN */
RC_t pacCfgReservedVlanAdd(int vlan);

/* Remove a reserved dynamic VLAN */
RC_t pacCfgReservedVlanRemove(int vlan);

/* Send a VLAN config notification to VLAN mgr */
RC_t pacCfgVlanSendCfgNotification(authMgrVlanPortCfgType_t type,
                                      char *interface, authMgrVlanPortData_t *cfg);

/* Request for a Reserved VLAN */
void pacCfgResvVlanAllocate(void);

/* Release a Reserved VLAN */
void pacCfgResvVlanRelease(int resvVlan);

/* Check if VLAN is in reserved VLAN range. */
bool pacCfgIsReserveVlan(int resvVlan);

/* Check if VLAN is an L3 interface. */
bool pacCfgIsL3VlanInterface(int vlan);

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* _PAC_CFG_VLAN_H */
