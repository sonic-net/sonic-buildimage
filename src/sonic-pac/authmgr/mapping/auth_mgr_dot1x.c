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


#include "auth_mgr_include.h"
#include "auth_mgr_auth_method.h"
#include "auth_mgr_struct.h"
#include "fpSonicUtils.h"

extern authmgrCB_t *authmgrCB;

static authmgrMethodEvent_t authmgrMethodEventTbl[] = {
  {authmgrClientReAuthenticate, "EAPOL_REAUTH"},
  {authmgrClientAuthStart, "NEW_STA"},
  {authmgrClientDisconnect, "DEAUTHENTICATE"}
};

/*********************************************************************
* @purpose function to send auth mgr events to dot1x daemon
*
* @param intIfNum   interface number
* @param event   authmgr event
* @param macAddr  client mac
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrDot1xEventSend (uint32 intIfNum, uint32 event,  enetMacAddr_t *macAddr)
{
  size_t len = 0;
   char8 ctrl_ifname[16] = {};
   char8 buf[1024] = {};
   char8 reply[1024] = {};

  fpGetHostIntfName(intIfNum, ctrl_ifname);

  snprintf(buf, sizeof(buf), "%s %02X:%02X:%02X:%02X:%02X:%02X", authmgrMethodEventTbl[event-1].eventStr,
           macAddr->addr[0], macAddr->addr[1], macAddr->addr[2], macAddr->addr[3], 
           macAddr->addr[4], macAddr->addr[5]); 

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
                "authmgrDot1xEventSend sending %s on %s\n", buf, ctrl_ifname);
  
	memset(&reply, 0, sizeof(reply));

  if (0 == wpa_sync_send(ctrl_ifname, buf, reply, &len))
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
        "%s:%d reply = %s on %s\n", __func__, __LINE__, reply, ctrl_ifname);

    if (0 == strncmp("OK", reply, strlen("OK")))
    {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
          "%s:%d success iface %s\n", __func__, __LINE__, ctrl_ifname);
      return  SUCCESS;
    }
    else
    {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
          "%s:%d failure iface %s\n", __func__, __LINE__, ctrl_ifname);
    }
  }

  return  FAILURE;
}

/*********************************************************************
* @purpose function to get dot1x admin mode
*
* @param intIfNum   interface number
* @param event   authmgr event
* @param macAddr  client mac
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrDot1xIntfAdminModeGet (uint32 intIfNum,  BOOL *enabled)
{
  size_t len = 0;
   char8 ctrl_ifname[16] = {};
   char8 buf[128] = {};

  *enabled =  FALSE;

  fpGetHostIntfName(intIfNum, ctrl_ifname);

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
      "sending PING on %s\n", ctrl_ifname);

  if (0 == wpa_sync_send(ctrl_ifname, "PING", buf, &len))
  {
    if (0 == strncmp("PONG", buf, strlen("PONG")))
    {
      *enabled =  TRUE;
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
          "Received PONG on %s\n", ctrl_ifname);
    }
  }
  else
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
        "wpa_sync_send not successful on %s\n", ctrl_ifname);
  }

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
      "received buf = %s on %s \n", buf, ctrl_ifname);
  return  SUCCESS;
}

/*********************************************************************
* @purpose function to set port control mode of the dot1x daemon
*
* @param intIfNum   interface number
* @param event   authmgr event
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrDot1xIntfPortControlModeSet (uint32 intIfNum,  AUTHMGR_PORT_CONTROL_t portControl)
{
#define PORT_CNTRL_MODE_SET_STR "EAPOL_SET AdminControlledPortControl"
  size_t len = 0;
   char8 ctrl_ifname[16] = {};
   char8 buf[128] = {};
   char8 cmd[128] = {};

  switch (portControl)
  {
  case  AUTHMGR_PORT_AUTO:
    snprintf(cmd, sizeof(cmd), "%s %s", PORT_CNTRL_MODE_SET_STR, "Auto");
    break;
  case  AUTHMGR_PORT_FORCE_AUTHORIZED:
    snprintf(cmd, sizeof(cmd), "%s %s", PORT_CNTRL_MODE_SET_STR, "ForceAuthorized");
    break;
  case  AUTHMGR_PORT_FORCE_UNAUTHORIZED:
    snprintf(cmd, sizeof(cmd), "%s %s", PORT_CNTRL_MODE_SET_STR, "ForceUnAuthorized");
    break;
  default:
    printf("invalid mode..\n");
    return  FAILURE;

  }

  fpGetHostIntfName(intIfNum, ctrl_ifname);

  if (0 == wpa_sync_send(ctrl_ifname, cmd, buf, &len))
  {
    if (0 == strncmp("OK", buf, strlen("OK")))
    {
      printf("successfull\n");
    }
  }
  else
  {
    printf("wpa_sync_send not successful\n");
  }

  printf("buf = %s\n", buf);
  return  SUCCESS;
}

/*********************************************************************
* @purpose  Get the port's dot1x capabilities (Supplicant and/or Authenticator)
*
* @param    intIfNum      @b{(input)} internal interface number
* @param    *capabilities @b{(output)} dot1x capabilities bitmask
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrDot1xPortPaeCapabilitiesGet (uint32 intIfNum,
                                             uchar8 * capabilities)
{
  *capabilities = authmgrCB->globalInfo->authmgrPortInfo[intIfNum].paeCapabilities;
  return  SUCCESS;
}

