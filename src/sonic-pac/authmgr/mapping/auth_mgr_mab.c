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
#include "osapi.h"
#include <unistd.h>

/*********************************************************************
 * * @purpose function to send auth mgr events to dot1x daemon
 * * 
 * * @param intIfNum   interface number
 * * @param event   authmgr event
 * * @param macAddr  client mac
 * * @return  SUCCESS or  FAILURE
 * * 
 * * @comments
 * * 
 * * @end      
 * *********************************************************************/
RC_t authmgrMabEventSend (uint32 intIfNum, uint32 event,  enetMacAddr_t *macAddr)
{ 
   uchar8 ctrl_ifname[ NIM_IF_ALIAS_SIZE + 1];
   char8 reply[1024] = {};
  mab_pac_cmd_t cmd_buf;
  unsigned int reply_len = sizeof(reply);
  unsigned int retryCnt = 3;

  nimGetIntfName (intIfNum,  ALIASNAME, ctrl_ifname);

  memset(&cmd_buf, 0, sizeof(cmd_buf));

  osapiStrncpySafe(cmd_buf.intf, ctrl_ifname, sizeof(cmd_buf.intf)-1);
  osapiStrncpySafe(cmd_buf.cmd, "event-notify", strlen("event-notify")+1);

  memcpy(cmd_buf.mac_addr, macAddr->addr, 6);

  cmd_buf.notif_event = event;

retry:

  if (retryCnt)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
        "authmgrMabEventSend sending %s for interface %s\n", cmd_buf.cmd, ctrl_ifname);

    memset(&reply, 0, sizeof(reply));

    if (0 == authmgrMabDataSend(&cmd_buf, reply, &reply_len))
    { 
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
          "%s:%d reply = %s\n", __func__, __LINE__, reply);

      if ((reply_len ) && (0 == strncmp("OK", reply, strlen("OK"))))
      {
        AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
            "%s:%d success\n", __func__, __LINE__);
        return  SUCCESS;
      }
      else
      {
        AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
            "%s:%d failure\n", __func__, __LINE__);
        usleep(10*1000);
      }
    }
    retryCnt--;
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
        "%s:%d retrying again (%d)", __func__, __LINE__, retryCnt);
    goto retry;
  }

  return  FAILURE;
}

/*********************************************************************
 * * @purpose function to get dot1x admin mode
 * *
 * * @param intIfNum   interface number
 * * @param event   authmgr event
 * * @param macAddr  client mac
 * * @return  SUCCESS or  FAILURE
 * *
 * * @comments
 * *
 * * @end
 * *********************************************************************/
RC_t authmgrMabIntfAdminModeGet (uint32 intIfNum,  BOOL *enabled)
{
  mab_pac_cmd_t cmd_buf;
   char8 ctrl_ifname[16] = {};
   char8 buf[128] = {};
  unsigned int len = sizeof(buf);

  *enabled =  FALSE;
  nimGetIntfName (intIfNum,  ALIASNAME, ctrl_ifname);

  memset(&cmd_buf, 0, sizeof(cmd_buf));

  osapiStrncpySafe(cmd_buf.intf, ctrl_ifname, sizeof(cmd_buf.intf)-1);
  osapiStrncpySafe(cmd_buf.cmd, "PING", strlen("PING")+1);

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
  "authmgrMabDataSend PING for %s start \n", ctrl_ifname);
  if (0 == authmgrMabDataSend(&cmd_buf, buf, &len))
  {
    if (0 == strncmp("PONG", buf, strlen("PONG")))
    {
      *enabled =  TRUE;
    }
  }
  else
  {
  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
    "authmgrMabDataSend not successful\n");
  }

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
  " Mab reply buf = %s\n", buf);
  return  SUCCESS;
}
