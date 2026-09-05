/*
 * Copyright 2021 Broadcom Inc.
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

#include <netinet/in.h>
#include <arpa/inet.h>
#include "pacoper.h"
#include "pacoper_common.h"
#include "nimapi.h"
#include "resources.h"

std::vector<std::string> authMgrMethod {"none", "802.1x", "mab"};

std::vector<std::string> userMgrAuthMethod {"undefined", "local", 
                                            "none", "radius"};
                                            
std::vector<std::string> authMgrPortStatus {"na", "authorized", "unauthorized"};

std::vector<std::string> vlanType {"Unassigned", "RADIUS",
                                   "Default", "Blocked"};

FpDbAdapter::FpDbAdapter( DBConnector *stateDb, DBConnector *configDb, DBConnector *appDb) :
                                    m_PacGlobalOperTbl(stateDb, STATE_PAC_GLOBAL_OPER_TABLE),
                                    m_PacPortOperTbl(stateDb, STATE_PAC_PORT_OPER_TABLE),
                                    m_PacAuthClientOperTbl(stateDb, STATE_PAC_AUTHENTICATED_CLIENT_OPER_TABLE)
{
}

DBConnector *stateDb = new DBConnector("STATE_DB", 0);
DBConnector *configDb = new DBConnector("CONFIG_DB", 0);
DBConnector *appDb = new DBConnector("APPL_DB", 0);
FpDbAdapter * Fp = new FpDbAdapter(stateDb, configDb, appDb);


string fetch_interface_name(int intIfNum)
{
  string name("");
   char8 ifName[ NIM_IF_ALIAS_SIZE + 1];

  if (nimGetIntfName(intIfNum,  ALIASNAME, ( uchar8*)ifName) !=  SUCCESS)
  {
	   return "FAILURE";
  }
  
  name = ifName;
  return name;
}

void PacAuthClientOperTblSet(uint32 intIfNum,  enetMacAddr_t macAddr, 
                            pac_authenticated_clients_oper_table_t *client_info)
{
  vector<FieldValueTuple> fvs;
  char c[18];
  unsigned int i;
  string serverState("");
  string serverClass("");
   enetMacAddr_t zeroMac;

  SWSS_LOG_NOTICE("----- PacAuthClientOperTbl func called from AuthMgr -----");

  memset (&zeroMac, 0, sizeof ( enetMacAddr_t));
  if (0 == memcmp (zeroMac.addr, macAddr.addr,  ENET_MAC_ADDR_LEN))
  {
    return;
  }

  if ( AUTHMGR_PORT_STATUS_AUTHORIZED != client_info->auth_status)
  {
    return;
  }

  string userName(client_info->userName, 
                  client_info->userName + 
                  sizeof(client_info->userName)/sizeof(client_info->userName[0]));  

  memset(c, 0, sizeof(c));

  sprintf(c, "%02X:%02X:%02X:%02X:%02X:%02X",
          macAddr.addr[0], macAddr.addr[1], macAddr.addr[2],
          macAddr.addr[3], macAddr.addr[4], macAddr.addr[5]);

  string macAddress(c);

  string interfaceName = fetch_interface_name(intIfNum);

  string key = interfaceName + "|";

  key += macAddress;

  fvs.emplace_back("current_id", to_string(client_info->currentIdL));
  fvs.emplace_back("auth_status", authMgrPortStatus[client_info->auth_status]);
  fvs.emplace_back("authenticated_method", authMgrMethod[client_info->authenticatedMethod]);

  for (i = 0; i < client_info->serverStateLen; i++) 
  {
    memset(c, 0, sizeof(c));
    sprintf(c, "%02X", client_info->serverState[i]);
    serverState += c;
  }

  fvs.emplace_back("server_state", serverState);
  fvs.emplace_back("server_state_len", to_string(client_info->serverStateLen));

  for (i = 0; i < client_info->serverClassLen; i++) 
  {
    memset(c, 0, sizeof(c));
    sprintf(c, "%02X", client_info->serverClass[i]);
    serverState += c;
  }

  fvs.emplace_back("server_class", serverClass);
  fvs.emplace_back("server_class_len", to_string(client_info->serverClassLen));

  fvs.emplace_back("session_timeout_RADIUS", to_string(client_info->sessionTimeoutRcvdFromRadius));
  fvs.emplace_back("session_timeout_oper", to_string(client_info->sessionTimeoutOper));
  fvs.emplace_back("user_name", userName);
  fvs.emplace_back("user_name_len", to_string(client_info->userNameLen));
  fvs.emplace_back("termination_action", to_string(client_info->terminationAction));
  fvs.emplace_back("vlan_id", to_string(client_info->vlanId));
  fvs.emplace_back("vlan_type", vlanType[client_info->vlanType]);
  fvs.emplace_back("backend_auth_method", userMgrAuthMethod[client_info->backend_auth_method]);
  fvs.emplace_back("session_time", to_string(client_info->sessionTime));
  fvs.emplace_back("termination_action_time_left", to_string(client_info->lastAuthTime));

  Fp->m_PacAuthClientOperTbl.set(key, fvs);

 }

void PacAuthClientOperTblDel(uint32 intIfNum,  enetMacAddr_t macAddr)
{
  string interfaceName = fetch_interface_name(intIfNum);

  char c[18];

  sprintf(c, "%02X:%02X:%02X:%02X:%02X:%02X",
          macAddr.addr[0], macAddr.addr[1], macAddr.addr[2],
          macAddr.addr[3], macAddr.addr[4], macAddr.addr[5]);

  string macAddress(c);

  string key = interfaceName + "|";
  key += macAddress;

  Fp->m_PacAuthClientOperTbl.del(key);

}

void PacAuthClientOperTblCleanup(void)
{
   vector<string> keys;
   Fp->m_PacAuthClientOperTbl.getKeys(keys);
   for (const auto key : keys)
   {
      Fp->m_PacAuthClientOperTbl.del(key);
   }
}

void PacGlobalOperTblSet(pac_global_oper_table_t *info)
{
  vector<FieldValueTuple> fvs;
  
  SWSS_LOG_NOTICE("----- PacOperTbl API called from AuthMgr -----");

  fvs.emplace_back("num_clients_authenticated", to_string(info->authCount));
  fvs.emplace_back("num_clients_authenticated_monitor", to_string(info->authCountMonMode));

  Fp->m_PacGlobalOperTbl.set("GLOBAL", fvs);
}

void PacGlobalOperTblCleanup(void)
{
   vector<string> keys;
   Fp->m_PacGlobalOperTbl.getKeys(keys);
   for (const auto key : keys)
   {
      Fp->m_PacGlobalOperTbl.del(key);
   }
}

void PacPortOperTblSet(uint32 intIfNum,  AUTHMGR_METHOD_t *enabledMethods, 
                        AUTHMGR_METHOD_t *enabledPriority)
{
  vector<FieldValueTuple> fvs;
  uint32 idx;
  string methods("");
  string priorities("");
  
  SWSS_LOG_NOTICE("----- PacPortOperTbl API called from AuthMgr -----");

  string key = fetch_interface_name(intIfNum);

  for (idx = 0; idx < 2; idx++)
  {
    if (idx != 0)
    {
      methods += ",";
    }
     
    if (!enabledMethods[idx])
    {
      methods += "undefined"; 
    }
    else
    {
      if ( AUTHMGR_METHOD_8021X == enabledMethods[idx])
      {
        methods += "dot1x"; 
      }
      else if ( AUTHMGR_METHOD_MAB == enabledMethods[idx])
      {
        methods += "mab"; 
      }
    }
  }

  for (idx = 0; idx < 2; idx++)
  {
    if (idx != 0)
    {
      priorities += ",";
    }
    if (!enabledPriority[idx])
    {
      priorities += "undefined"; 
    }
    else
    {
      if ( AUTHMGR_METHOD_8021X == enabledPriority[idx])
      {
        priorities += "dot1x"; 
      }
      else if ( AUTHMGR_METHOD_MAB == enabledPriority[idx])
      {
        priorities += "mab"; 
      }
    }
  }

  fvs.emplace_back("enabled_method_list@", methods);
  fvs.emplace_back("enabled_priority_list@", priorities);
  
  Fp->m_PacPortOperTbl.set(key, fvs);
}

void PacPortOperTblCleanup(void)
{
   vector<string> keys;
   Fp->m_PacPortOperTbl.getKeys(keys);
   for (const auto key : keys)
   {
      Fp->m_PacPortOperTbl.del(key);
   }
}


void PacOperTblCleanup(void)
{
   PacAuthClientOperTblCleanup();
   PacGlobalOperTblCleanup();
}


