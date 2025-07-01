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

#ifndef AUTHMGR_AUTHMETHOD_H
#define AUTHMGR_AUTHMETHOD_H

#include <stddef.h>
#include "mab_socket.h"

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

#define ETHERNET_PREFIX "Ethernet"

typedef struct authmgrMethodEvent_s
{
  authmgrNotifyEvent_t event;
   char8 eventStr[16];
}authmgrMethodEvent_t;

RC_t authmgrDot1xEventSend (uint32 intIfNum, uint32 event,  enetMacAddr_t *macAddr);
RC_t authmgrDot1xIntfAdminModeGet (uint32 intIfNum,  BOOL *enabled);
RC_t authmgrDot1xIntfPortControlModeSet (uint32 intIfNum,  AUTHMGR_PORT_CONTROL_t portControl);
RC_t authmgrDot1xPortPaeCapabilitiesGet (uint32 intIfNum,  uchar8 * capabilities);
int wpa_sync_send(char * ctrl_ifname, char * cmd, char *buf, size_t *len);
int authmgrMabDataSend(mab_pac_cmd_t *req, char *resp, unsigned int *len);
RC_t authmgrMabEventSend (uint32 intIfNum, uint32 event,  enetMacAddr_t *macAddr);
RC_t authmgrMabIntfAdminModeGet (uint32 intIfNum,  BOOL *enabled);

int handle_async_resp_data(int *listen_sock);

/* End of function prototypes */

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* AUTHMGR_AUTHMETHOD_H */

