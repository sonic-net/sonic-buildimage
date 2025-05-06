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

#ifdef _PAC_CFG_AUTHMGR_H
#define _PAC_CFG_AUTHMGR_H

#include "pac_cfg.h"
#include "pacinfra_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Set port learning config */
RC_t pacCfgIntfLearningModeSet(char *interface,  AUTHMGR_PORT_LEARNING_t learning);

/* Get port learning config */
RC_t pacCfgIntfLearningModeGet(char *interface,  AUTHMGR_PORT_LEARNING_t *learning);

/* Setup port learning config */
bool pacCfgIntfViolationPolicySet(char *interface, bool enable);

/* Add dot1x client */
bool pacCfgIntfClientAdd(char *interface, unisgned char *m_mac, int vlan);

/* Delete dot1x client */
bool pacCfgIntfClientRemove(char *interface, unsigned char *m_mac, int vlan);

/* Block a client's traffic while client is getting authorized */
bool pacCfgIntfClientBlock(char *interface, unsigned char *m_mac, int vlan);

/* Unblock a client's traffic */
bool pacCfgIntfClientUnBlock(char *interface, unsigned char *m_mac, int vlan);

/* Send a notification to remove/add static MAC entries on a port. */
RC_t pacCfgFdbSendCfgNotification(authMgrFdbCfgType_t type, char *interface);

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* _PAC_CFG_AUTHMGR_H */
