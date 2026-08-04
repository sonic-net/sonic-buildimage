/*
 * Copyright 2019 Broadcom Inc.
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

#include <cstring>
#include <string>
#include "mabmgr.h"
#include "mab_api.h"
#include "mab_exports.h"
#include "mab_include.h"
#include "datatypes.h"
#include "nimapi.h"
#include <arpa/inet.h>
#include <netdb.h>
#include "fpSonicUtils.h"

#define MABD_CMD_BUFFSZ  2048
MabMgr *mab;

const string INTFS_PREFIX = "E";

MabMgr::MabMgr(DBConnector *configDb, DBConnector *stateDb, DBConnector *appDb) :
                           m_confMabPortTbl(configDb, "MAB_PORT_CONFIG_TABLE"),
                           m_confRadiusServerTable(configDb, "RADIUS_SERVER"),
                           m_confRadiusGlobalTable(configDb, "RADIUS") {

    Logger::linkToDbNative("mabmgr");
    mab = this;
}

std::vector<Selectable*> MabMgr::getSelectables() {
    vector<Selectable *> selectables{ &m_confMabPortTbl, &m_confRadiusServerTable, &m_confRadiusGlobalTable }; 
    return selectables;
}


bool MabMgr::processDbEvent(Selectable *tbl) {

    SWSS_LOG_DEBUG("Received a MAB Database event");

    //check the source table and accordingly invoke the appropriate handlers

    if (tbl == ((Selectable *) & m_confMabPortTbl)) {
        return processMabConfigPortTblEvent(tbl);
    }

    if (tbl == ((Selectable *) & m_confRadiusServerTable)) {
        return processRadiusServerTblEvent(tbl);
    }

    if (tbl == ((Selectable *) & m_confRadiusGlobalTable)) {
        return processRadiusGlobalTblEvent(tbl);
    }

    SWSS_LOG_DEBUG("Received event UNKNOWN to MAB, ignoring ");
    return false;
}

//Process the config db table events

bool MabMgr::processMabConfigPortTblEvent(Selectable *tbl) 
{
  SWSS_LOG_ENTER();
  SWSS_LOG_DEBUG("Received a table config event on MAB_PORT_CONFIG_TABLE table");

  std::deque<KeyOpFieldsValuesTuple> entries;
  m_confMabPortTbl.pops(entries);

  SWSS_LOG_DEBUG("Received %d entries", (int) entries.size());

  /* Nothing popped */
  if (entries.empty())
  {
      return false;
  }

  // Check through all the data
  for (auto entry : entries) 
  {
    std::string key = kfvKey(entry);
    std::string  op = kfvOp(entry);
    bool task_result = false;
    uint32 intIfNum;

    SWSS_LOG_DEBUG("Received %s as key and %s as OP", key.c_str(), op.c_str());

    if(key.find(INTFS_PREFIX) == string::npos)
    {
        SWSS_LOG_NOTICE("Invalid key format. No 'E' prefix: %s", key.c_str());
        continue;
    }

    if(fpGetIntIfNumFromHostIfName(key.c_str(), &intIfNum) !=  SUCCESS)
    {
        SWSS_LOG_NOTICE("Unable to get the internal interface number for %s.", key.c_str());
        continue;
    }

    if (op == SET_COMMAND)
        {
            task_result = doMabPortTableSetTask(entry, intIfNum);
        }
        else if (op == DEL_COMMAND)
        {
            task_result = doMabPortTableDeleteTask(entry, intIfNum);
        }
        if (!task_result)
            return false;
     }
     return true;
}

bool MabMgr::doMabPortTableSetTask(const KeyOpFieldsValuesTuple & t, uint32 & intIfNum)
{
    SWSS_LOG_ENTER();
    const std::string & key = kfvKey(t);

    // Update mabPortConfigCache cache with incoming table data
    mabPortConfigCacheParams_t mabPortConfigCache;
    mabPortConfigCache.mab_enable = MABMGR_MAB_PORT_ENABLE_DEF;
    mabPortConfigCache.mab_auth_type = MABMGR_MAB_PORT_AUTH_TYPE_DEF;

    for (auto item = kfvFieldsValues(t).begin(); item != kfvFieldsValues(t).end(); item++)
    {
        const std::string & field = fvField(*item);
        const std::string & value = fvValue(*item);

        if (field == "mab_enable")
        {
            if (value == "true")
               mabPortConfigCache.mab_enable =  ENABLE;
            else if (value == "false")
               mabPortConfigCache.mab_enable =  DISABLE;
            else {
               SWSS_LOG_WARN("Invalid configuration option received for mab enable: %s", value.c_str());
               continue;
            }
        }
        if (field == "mab_auth_type")
        {
            if (value == "pap")
                mabPortConfigCache.mab_auth_type=  AUTHMGR_PORT_MAB_AUTH_TYPE_PAP;
            else if (value == "chap")
                mabPortConfigCache.mab_auth_type =  AUTHMGR_PORT_MAB_AUTH_TYPE_CHAP;
            else if (value == "eap-md5")
                mabPortConfigCache.mab_auth_type =  AUTHMGR_PORT_MAB_AUTH_TYPE_EAP_MD5;
            else {
               SWSS_LOG_WARN("Invalid configuration option received for mab auth type: %s", value.c_str());
               continue;
            }
        }
    }

    mabPortConfigTableMap::iterator iter = m_mabPortConfigMap.find(key);
    if(iter == m_mabPortConfigMap.end())
    {
        m_mabPortConfigMap.insert(pair<std::string, mabPortConfigCacheParams_t>(key, mabPortConfigCache));
        mabPortConfigTableMap::iterator iter = m_mabPortConfigMap.find(key);

        if(mabPortConfigCache.mab_enable != MABMGR_MAB_PORT_ENABLE_DEF)
        {
            if ( SUCCESS != mabPortMABEnableSet(intIfNum, mabPortConfigCache.mab_enable))
            {
              iter->second.mab_enable = MABMGR_MAB_PORT_ENABLE_DEF;
              SWSS_LOG_ERROR("Unable to enable MAB operationally.");
            }
        }
        if(mabPortConfigCache.mab_auth_type != MABMGR_MAB_PORT_AUTH_TYPE_DEF)
        {
            if ( SUCCESS != mabPortMABAuthTypeSet(intIfNum, mabPortConfigCache.mab_auth_type))
            {
              iter->second.mab_auth_type = MABMGR_MAB_PORT_AUTH_TYPE_DEF;
              SWSS_LOG_ERROR("Unable to set MAB authentication type operationally.");
            }
        }
     }
     else //Interface entry already exists in local cache, check for any parameter change for Add/Update/Delete
     {
        // mab_enable
        if (((iter->second.mab_enable == MABMGR_MAB_PORT_ENABLE_DEF) &&
            (mabPortConfigCache.mab_enable != MABMGR_MAB_PORT_ENABLE_DEF)) ||
            ((iter->second.mab_enable != MABMGR_MAB_PORT_ENABLE_DEF) &&
            (mabPortConfigCache.mab_enable != iter->second.mab_enable)))
        {
            if ( SUCCESS == mabPortMABEnableSet(intIfNum, mabPortConfigCache.mab_enable))
            {
              iter->second.mab_enable = mabPortConfigCache.mab_enable;
            }
            else
	    {
              SWSS_LOG_ERROR("Unable to enable/disable MAB operationally.");
              return false;
            }
        }
        // mab_auth_type
        if (((iter->second.mab_auth_type == MABMGR_MAB_PORT_AUTH_TYPE_DEF) &&
            (mabPortConfigCache.mab_auth_type != MABMGR_MAB_PORT_AUTH_TYPE_DEF)) ||
            ((iter->second.mab_auth_type != MABMGR_MAB_PORT_AUTH_TYPE_DEF) &&
            (mabPortConfigCache.mab_auth_type != iter->second.mab_auth_type)))
        {
            if ( SUCCESS == mabPortMABAuthTypeSet(intIfNum, mabPortConfigCache.mab_auth_type))
            {
              iter->second.mab_auth_type = mabPortConfigCache.mab_auth_type;
            }
            else
	    {
              SWSS_LOG_ERROR("Unable to set MAB authentication type operationally.");
              return false;
            }
        }
      }
      return true;
}

bool MabMgr::doMabPortTableDeleteTask(const KeyOpFieldsValuesTuple & t, uint32 & intIfNum)
{
    SWSS_LOG_ENTER();
    const std::string & key = kfvKey(t);
    mabPortConfigTableMap::iterator iter = m_mabPortConfigMap.find(key);
    if(iter != m_mabPortConfigMap.end())
    {
      if (iter->second.mab_enable != MABMGR_MAB_PORT_ENABLE_DEF)
      {
            if ( SUCCESS == mabPortMABEnableSet(intIfNum, MABMGR_MAB_PORT_ENABLE_DEF))
            {
                iter->second.mab_enable = MABMGR_MAB_PORT_ENABLE_DEF;
            }
            else
            {
              SWSS_LOG_ERROR("Unable to set MAB enable with default.");
              return false;
            }
      }
      if (iter->second.mab_auth_type != MABMGR_MAB_PORT_AUTH_TYPE_DEF)
      {
            if ( SUCCESS == mabPortMABAuthTypeSet(intIfNum, MABMGR_MAB_PORT_AUTH_TYPE_DEF))
            {
              iter->second.mab_auth_type = MABMGR_MAB_PORT_AUTH_TYPE_DEF;
            }
            else
            {
              SWSS_LOG_ERROR("Unable to set MAB authentication type with default.");
              return false;
            }
      }
    }
    return true;
}

void MabMgr::updateRadiusServerGlobalKey(string newKey, string oldKey) {

   SWSS_LOG_ENTER();
   bool update = false;
   RC_t rc =  FAILURE;

   if (0 == newKey.compare(oldKey))
   {
      return;
   }

   for (auto& item: m_radius_info.radius_auth_server_list)
   {
      /* server specific is configured */
      if (0 != item.second.server_key.size())
      {
         continue;
      }

      /* Check and update Radius server if using Global key */
      if (0 != newKey.size())
      {
         item.second.server_update = true;
         update = true;
      }
      else
      {
         rc = mabRadiusServerUpdate(RADIUS_MAB_SERVER_DELETE, "auth",
                                    item.second.server_ip.c_str(),
                                    item.second.server_priority.c_str(),
                                    oldKey.c_str(),
                                    item.second.server_port.c_str());
         if ( SUCCESS != rc)
         {
             SWSS_LOG_ERROR("Unable to update radius server details for MAB ip = %s,  port = %s, priority = %s",
                            item.second.server_ip.c_str(),
                            item.second.server_port.c_str(),
                            item.second.server_priority.c_str());
         }
      }
   }

   /* Due to global key change update server needed */
   if (true == update)
   {
      updateRadiusServer();
   }
}

void MabMgr::updateRadiusServer() {

   SWSS_LOG_ENTER();
   RC_t rc =  FAILURE;
   struct addrinfo* result;
   char ip[INET6_ADDRSTRLEN+1];
   void * src = NULL;

   SWSS_LOG_NOTICE("Deriving new RADIUS Servers for MAB");

   for (auto& item: m_radius_info.radius_auth_server_list)
   {
       if (false == item.second.server_update)
       {
           SWSS_LOG_INFO("skipped %s as update not needed.", item.first.c_str());
           continue;  
       }

       if (getaddrinfo(item.first.c_str(), NULL, NULL, &result) || result == NULL)
       {
           SWSS_LOG_WARN("skipped %s as it could not resolve.", item.first.c_str());
           item.second.dns_ok = false;
           continue;
       }

       if(result->ai_family == AF_INET)
           src = &((struct sockaddr_in*)result->ai_addr)->sin_addr;
       else
           src = &((struct sockaddr_in6*)result->ai_addr)->sin6_addr;

       inet_ntop(result->ai_family, src, ip, INET6_ADDRSTRLEN+1);
       freeaddrinfo(result);

       //Check if radius server has key configured. If not,
       // pick global key. If key does not exist, skip to next server. 
       if ((item.second.server_key  == "") && (m_radius_info.m_radiusGlobalKey == ""))
       {
           SWSS_LOG_WARN("skipped %s as no key is configured.", item.first.c_str());
           continue;  
       }

       string newKey = m_radius_info.m_radiusGlobalKey; 
       if (item.second.server_key != "")
       {
           newKey = item.second.server_key;
       }

       string radiusIp(ip);
       item.second.server_ip = radiusIp;
       
       rc = mabRadiusServerUpdate(RADIUS_MAB_SERVER_ADD, "auth", item.second.server_ip.c_str(),
                              item.second.server_priority.c_str(),
                              newKey.c_str(),
                              item.second.server_port.c_str());
       if ( SUCCESS != rc)
       {
           SWSS_LOG_ERROR("Radius server update - Unable to update radius server details for MAB.");
           return;
       }
       SWSS_LOG_NOTICE("Updating radius details for MAB  ip = %s,  port = %s, priority = %s", 
                       item.second.server_ip.c_str(),
                       item.second.server_port.c_str(),
                       item.second.server_priority.c_str());
       item.second.server_update = false;
       item.second.dns_ok = true;
   }
   return;
}

void MabMgr::reloadRadiusServers() 
{
   SWSS_LOG_ENTER();
   RC_t rc =  FAILURE;
   bool server_update = false;

   SWSS_LOG_NOTICE("Reloading RADIUS Servers for MAB");

   /*Check for servers that failed DNS resolution  */
   for (auto& item: m_radius_info.radius_auth_server_list)
   {
       if (false == item.second.dns_ok)
       {
           item.second.server_update = true;
           server_update = true;
       }
   }

   if (true == server_update)
   {
       SWSS_LOG_NOTICE("Reloading DNS failed RADIUS Servers for MAB");
       updateRadiusServer();
   }

   rc = mabRadiusServerUpdate(RADIUS_MAB_SERVERS_RELOAD, "auth", 
                              NULL, NULL, NULL, NULL);

   if ( SUCCESS != rc)
   {
       SWSS_LOG_ERROR("RADIUS Servers reload - Unable to reload.");
   }

   return;
}

bool MabMgr::processRadiusServerTblEvent(Selectable *tbl) 
{
  SWSS_LOG_ENTER();
  SWSS_LOG_NOTICE("Received a RADIUS_SERVER event");

  deque<KeyOpFieldsValuesTuple> entries;
  m_confRadiusServerTable.pops(entries);

  SWSS_LOG_NOTICE("Received %d entries", (int) entries.size());

  /* Nothing popped */
  if (entries.empty())
  {
    return false;
  }

  // Check through all the data
  for (auto entry : entries)
  {
    string key = kfvKey(entry);
    string val = kfvOp(entry);
    string cmd("");

    SWSS_LOG_NOTICE("Received %s as key and %s as OP", key.c_str(), val.c_str());

    if (val == SET_COMMAND)
    {
      SWSS_LOG_NOTICE("SET operation on RADIUS_SERVER table");

      m_radius_info.radius_auth_server_list[key].server_port = "";
      m_radius_info.radius_auth_server_list[key].server_key = "";
      m_radius_info.radius_auth_server_list[key].server_priority = "";
      m_radius_info.radius_auth_server_list[key].server_update = true;
      m_radius_info.radius_auth_server_list[key].dns_ok = true;

      // Look at the data that is sent for this key

      for (auto i : kfvFieldsValues(entry))
      {
        string a = fvField(i);
        string b = fvValue(i);

        SWSS_LOG_DEBUG("Received %s as field and %s as value", a.c_str(), b.c_str());

        if (a == "passkey")
        {
          m_radius_info.radius_auth_server_list[key].server_key = b;
        }
        else if (a == "auth_port")
        {
          m_radius_info.radius_auth_server_list[key].server_port = b; 
        }
        else if (a == "priority")
        {
          m_radius_info.radius_auth_server_list[key].server_priority = b; 
        }
      }
      updateRadiusServer();
    }
    else if (val == DEL_COMMAND)
    {
      RC_t rc =  FAILURE;
      SWSS_LOG_INFO("Delete Radius server for MAB %s ", 
                       m_radius_info.radius_auth_server_list[key].server_ip.c_str()); 
      // server deleted
      rc = mabRadiusServerUpdate(RADIUS_MAB_SERVER_DELETE, "auth",
                                 m_radius_info.radius_auth_server_list[key].server_ip.c_str(),
                                 m_radius_info.radius_auth_server_list[key].server_priority.c_str(),
                                 m_radius_info.radius_auth_server_list[key].server_key.c_str(),
                                 m_radius_info.radius_auth_server_list[key].server_port.c_str());
      if (rc !=  SUCCESS)
      {
         SWSS_LOG_ERROR("Radius server delete - Unable to delete radius server details for MAB.");
      }
      m_radius_info.radius_auth_server_list.erase(key);
    }
  }

  return true;
}

bool MabMgr::processRadiusGlobalTblEvent(Selectable *tbl) 
{
  SWSS_LOG_ENTER();
  SWSS_LOG_NOTICE("Received a RADIUS event");
  string tmp_radiusGlobalKey(m_radius_info.m_radiusGlobalKey);

  deque<KeyOpFieldsValuesTuple> entries;
  m_confRadiusGlobalTable.pops(entries);

  SWSS_LOG_NOTICE("Received %d entries", (int) entries.size());

  /* Nothing popped */
  if (entries.empty())
  {
    return false;
  }

  // Check through all the data
  for (auto entry : entries)
  {
    string key = kfvKey(entry);
    string val = kfvOp(entry);
    string cmd("");

    SWSS_LOG_NOTICE("Received %s as key and %s as OP", key.c_str(), val.c_str());

    if (val == SET_COMMAND)
    {
      SWSS_LOG_NOTICE("SET operation on RADIUS table");

      // Look at the data that is sent for this key
      for (auto i : kfvFieldsValues(entry))
      {
        string a = fvField(i);
        string b = fvValue(i);

        SWSS_LOG_DEBUG("Received %s as field and %s as value", a.c_str(), b.c_str());

        if (a == "passkey")
        {
          m_radius_info.m_radiusGlobalKey = b;
        }
      }
    }
    else if (val == DEL_COMMAND)
    {
      m_radius_info.m_radiusGlobalKey = ""; 
    }
  }

  updateRadiusServerGlobalKey(m_radius_info.m_radiusGlobalKey, tmp_radiusGlobalKey);

  return true;
}
