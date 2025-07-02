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

#include <vector>
#include <string>
#include <netinet/in.h>
#include <linux/if.h>
#include "hostapdmgr.h"
#include <cstring>
#include <iostream>
#include <fstream>
#include <signal.h>
#include <netlink/route/link.h>
#include <swss/netmsg.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include "wpa_ctrl.h"
#include "tokenize.h"

#define TEAM_DRV_NAME   "team"

const string INTFS_PREFIX = "E";
const string HOSTAPD_PID_FILE = "/etc/hostapd/hostapdPid";

HostapdMgr *hostapd;

HostapdMgr::HostapdMgr(DBConnector *configDb, DBConnector *appDb) :
                           m_confHostapdPortTbl(configDb, CFG_PAC_PORT_CONFIG_TABLE),
                           m_confHostapdGlobalTbl(configDb, CFG_PAC_HOSTAPD_GLOBAL_CONFIG_TABLE),
                           m_confRadiusServerTable(configDb, "RADIUS_SERVER"),
                           m_confRadiusGlobalTable(configDb, "RADIUS")

{
  Logger::linkToDbNative("hostapdmgr");
  memset(&m_glbl_info, 0, sizeof(m_glbl_info));
  active_intf_cnt = 0;
  start_hostapd = false;
  stop_hostapd = false;

  hostapd = this;
}

string HostapdMgr::getStdIfFormat(string key)
{
  if((key.find("E") == string::npos) || (key.length() > 8))
  {
    return key;
  }
  string key1("");
  key1 = "Eth" + key.substr(1,1) + '/' + key.substr(3);
  return key1;
}

vector<Selectable*> HostapdMgr::getSelectables() {
    vector<Selectable *> selectables{ &m_confHostapdPortTbl, &m_confHostapdGlobalTbl, &m_confRadiusServerTable, &m_confRadiusGlobalTable};
    return selectables;
}

bool HostapdMgr::processDbEvent(Selectable *tbl) {

    SWSS_LOG_ENTER();
    SWSS_LOG_DEBUG("Received a HOSTAPD Database event");

    //check the source table and accordingly invoke the appropriate handlers

    if (tbl == ((Selectable *) & m_confHostapdPortTbl)) {
        return processHostapdConfigPortTblEvent(tbl);
    }

    if (tbl == ((Selectable *) & m_confHostapdGlobalTbl)) {
        return processHostapdConfigGlobalTblEvent(tbl);
    }

    if (tbl == ((Selectable *) & m_confRadiusServerTable)) {
        return processRadiusServerTblEvent(tbl);
    }

    if (tbl == ((Selectable *) & m_confRadiusGlobalTable)) {
        return processRadiusGlobalTblEvent(tbl);
    }

    SWSS_LOG_DEBUG("Received event UNKNOWN to HOSTAPD, ignoring ");
    return false;
}

//Process the config db table events

bool HostapdMgr::processHostapdConfigPortTblEvent(Selectable *tbl) 
{

  SWSS_LOG_ENTER();
  SWSS_LOG_DEBUG("Received an table config event on PAC_PORT_CONFIG_TABLE table");

  deque<KeyOpFieldsValuesTuple> entries;
  m_confHostapdPortTbl.pops(entries);

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

    SWSS_LOG_NOTICE("Received %s as key and %s as OP", key.c_str(), val.c_str());

    if (m_intf_info.find(key) == m_intf_info.end())
    {
      SWSS_LOG_NOTICE("Cannot find interface %s in local db. Adding it now", key.c_str());
      hostapd_intf_info_t intf;
      intf.control_mode = "force-authorized";
      intf.capabilities = "none";
      intf.admin_status = 0;
      intf.link_status = 0;
      intf.config_created = false;
      setPort(key, intf);
    }

    SWSS_LOG_NOTICE("intf-- %s capabilities %s ctrl_mode %s admin_status %d link_status %d, global_auth %d", 
                     key.c_str(), m_intf_info[key].capabilities.c_str(),
                     m_intf_info[key].control_mode.c_str(),m_intf_info[key].admin_status,
                     m_intf_info[key].link_status, m_glbl_info.enable_auth);

     if (m_radius_info.radius_auth_server_list.size())
     {
       SWSS_LOG_NOTICE("m_radius_info.radius_auth_server_list.size() is non-zero ");
     }
    
    if (val == SET_COMMAND) 
    {
      vector<string> new_interfaces;
      vector<string> del_interfaces;

      // Look at the data that is sent for this key
      for (auto i : kfvFieldsValues(entry)) 
      {
        string a = fvField(i);
        string b = fvValue(i);

        SWSS_LOG_NOTICE("Received %s as field and %s as value", a.c_str(), b.c_str());

        if ((a == "port_pae_role") && (m_intf_info[key].capabilities != b))
        {
          if (b == "authenticator") 
          {
            // pae role authenticator
            if ((m_glbl_info.enable_auth) && (m_intf_info[key].link_status) && (!m_intf_info[key].config_created) &&
                (m_intf_info[key].control_mode == "auto") && (m_radiusServerInUse != "")) 
            {
              /* create config file */
              createConfFile(key);
              
              /* update interfaces list */
              new_interfaces.push_back(key);
            }
          }
          else 
          {
            // pae role none
            if (m_intf_info[key].config_created)
            {
              /* delete config file */
              deleteConfFile(key);
              
              /* update interfaces list */
              del_interfaces.push_back(key);
            }
          }
          m_intf_info[key].capabilities = b;
        }
        else if ((a == "port_control_mode") && (m_intf_info[key].control_mode != b))
        {
          if (b == "auto") 
          {
            // port control mode auto

            if ((m_glbl_info.enable_auth) && (m_intf_info[key].link_status) && (!m_intf_info[key].config_created) &&
                (m_intf_info[key].capabilities == "authenticator") && (m_radiusServerInUse != "")) 
            {
              /* create config file */
              createConfFile(key);
              
              /* update interfaces list */
              new_interfaces.push_back(key);
            }
          }
          else 
          {
            // pae role none

            if (m_intf_info[key].config_created)
            {
              /* delete config file */
              deleteConfFile(key);
              
              /* update interfaces list */
              del_interfaces.push_back(key);
            }
          }
          m_intf_info[key].control_mode = b;
        }
      }

      /* update JSON for new_interfaces and del_interfaces */
      informHostapd("new", new_interfaces);
      informHostapd("deleted", del_interfaces);
    }
    else if (val == DEL_COMMAND) 
    {
      SWSS_LOG_WARN("Unexpected DEL operation on PAC_PORT_CONFIG_TABLE, ignoring");
      continue;
    }   
  }

  return true;
}

bool HostapdMgr::processHostapdConfigGlobalTblEvent(Selectable *tbl) 
{
  SWSS_LOG_ENTER();
  SWSS_LOG_DEBUG("Received an table config event on HOSTAPD_GLOBAL_CONFIG_TABLE table");

  deque<KeyOpFieldsValuesTuple> entries;
  m_confHostapdGlobalTbl.pops(entries);

  SWSS_LOG_NOTICE("Received %d entries", (int) entries.size());

  /* Nothing popped */
  if (entries.empty()) 
  {
    return false;
  }

  SWSS_LOG_NOTICE("enable_auth %d: ", m_glbl_info.enable_auth);

  // Check through all the data
  for (auto entry : entries) 
  {
    string key = kfvKey(entry);
    string val = kfvOp(entry);

    SWSS_LOG_NOTICE("Received %s as key and %s as OP", key.c_str(), val.c_str());

    if (val == SET_COMMAND) 
    {
      // Look at the data that is sent for this key
      for (auto i : kfvFieldsValues(entry)) 
      {
        string a = fvField(i);
        string b = fvValue(i);

        SWSS_LOG_DEBUG("Received %s as field and %s as value", a.c_str(), b.c_str());

        vector<string> interfaces;

        if (a == "dot1x_system_auth_control" )
        {
          if (b == "true")
          {
            // dot1x enabled
            if (!m_glbl_info.enable_auth)
            {
              SWSS_LOG_NOTICE("set m_glbl_info.enable_auth to 1");
              m_glbl_info.enable_auth = 1;

              for (auto const& entry: m_intf_info) 
              {
                SWSS_LOG_NOTICE("--intf-- %s capabilities %s ctrl_mode %s admin_status %d link_status %d, global_auth %d",
                                entry.first.c_str(), m_intf_info[key].capabilities.c_str(), m_intf_info[key].control_mode.c_str(),
                                m_intf_info[key].admin_status, m_intf_info[key].link_status, m_glbl_info.enable_auth);
                if ((entry.second.capabilities == "authenticator") && (entry.second.control_mode == "auto") && 
                    (entry.second.link_status) && (!entry.second.config_created) && (m_radiusServerInUse != ""))
                {

                  /* create config file */
                  createConfFile(entry.first);

                  /* update interfaces list */
                  interfaces.push_back(entry.first);
                }
              }

              /* Update JSON */

              informHostapd("new", interfaces);
            }
          } 
          else if (b == "false")
          {
            // dot1x disabled
            if (m_glbl_info.enable_auth) 
			{
			  m_glbl_info.enable_auth = 0;
			  SWSS_LOG_NOTICE("setting m_glbl_info.enable_auth to 0");

			  for (auto const& entry: m_intf_info) 
			  {
				SWSS_LOG_NOTICE("received false for intf %s capabilities %s ctrl_mode %s admin_status %d link_status %d, global_auth %d",
					entry.first.c_str(), m_intf_info[key].capabilities.c_str(), m_intf_info[key].control_mode.c_str(),m_intf_info[key].admin_status,
					m_intf_info[key].link_status, m_glbl_info.enable_auth);

				if (entry.second.config_created)
				{
				  /* delete config file */
				  deleteConfFile(entry.first);

				  /* update interfaces list */
				  interfaces.push_back(entry.first);
				}
			  }

			  /* Update JSON */
			  informHostapd("deleted", interfaces);
			}
          }
        }
      }
    }
    else if (val == DEL_COMMAND) 
    {
      SWSS_LOG_WARN("Unexpected DEL operation on HOSTAPD_GLOBAL_CONFIG_TABLE, ignoring");
      continue;
    }   
  }
  return true;
}

string execute(string command) {
   char buffer[128];
   string result = "";

   SWSS_LOG_DEBUG("command is %s", command);
   // Open pipe to file
   FILE* pipe = popen(command.c_str(), "r");
   if (!pipe) {
      return "popen failed!";
   }

   // read till end of process:
   while (!feof(pipe)) {

      // use buffer to read and add to result
      if (fgets(buffer, 128, pipe) != NULL)
         result += buffer;
   }

   pclose(pipe);
   return result;
}

static bool cmp(pair<string, radius_server_info_t>& a,
                pair<string, radius_server_info_t>& b)
{
  return (stoi(a.second.server_priority) > stoi(b.second.server_priority));
}

void HostapdMgr::updateRadiusServer() {

   SWSS_LOG_ENTER();

   SWSS_LOG_NOTICE("Update RADIUS Servers for HOSTAPD");

   // Run over all radius information 
   vector<pair<string, radius_server_info_t>> sortedMap;

   for (auto& item: m_radius_info.radius_auth_server_list)
   {
       item.second.server_priority = (item.second.server_priority == "") ?
                                        "0": item.second.server_priority;
       sortedMap.push_back(item);
   }

   /*  When no Radius servers configured reset m_radiusServerInUse field */
   if (0 == sortedMap.size())
   {
       m_radiusServerInUse = "";
   }

   for (auto & item: m_radius_info.radius_auth_server_list)
   {
       struct addrinfo* result;
       char ip[INET6_ADDRSTRLEN+1];
       void * src = NULL;

       item.second.config_ok = false;

       if (getaddrinfo(item.first.c_str(), NULL, NULL, &result) || result == NULL)
       {
           SWSS_LOG_WARN("skipped %s as it could not resolve.", item.first.c_str());
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

       string radiusIp(ip);
       item.second.config_ok = true;
       item.second.server_ip = radiusIp;
       // Check against in-use radius server and
       // update hostapd if necessary.
       if (item.second.server_priority > m_radiusServerInUseInfo.server_priority)
       {
           m_radiusServerInUse = ""; 
       }

       if (m_radiusServerInUse == "")
       {
           m_radiusServerInUse = radiusIp;
           m_radiusServerInUseInfo.server_port = item.second.server_port;
           m_radiusServerInUseInfo.server_key = m_radius_info.m_radiusGlobalKey;
           m_radiusServerInUseInfo.server_priority = item.second.server_priority;
           if (item.second.server_key != "")
           { 
               m_radiusServerInUseInfo.server_key = item.second.server_key;
           } 
       }
   }

   if (m_glbl_info.enable_auth && m_radiusServerInUse != "")
   {
       // Update in use radius server and update hostapd.
       vector<string> interfaces;
       for (auto const& entry: m_intf_info)
       {
          if ((m_glbl_info.enable_auth) && (entry.second.capabilities == "authenticator") && 
              (entry.second.control_mode == "auto") &&
              (entry.second.link_status))
          {
             /* create config file */
             createConfFile(entry.first);
 
             /* update interfaces list */
             interfaces.push_back(entry.first);
          }
       }
 
       /* update JSON file */
       informHostapd("modified", interfaces);
       return; 
   }

   // Check if global auth is enabled or not 
   else if (!m_glbl_info.enable_auth || m_radiusServerInUse == "")
   {
       // No valid radius server found. Delete conf files and kill hostapd. 
       vector<string> interfaces;
       for (auto const& entry: m_intf_info)
       {
           if (entry.second.config_created)
           {
               /* delete config file */
               deleteConfFile(entry.first);
              
               /* update interfaces list */
               interfaces.push_back(entry.first);
           }
       }
        
       /* Update JSON */
       informHostapd("deleted", interfaces);
   }
   return;
}

bool HostapdMgr::processRadiusServerTblEvent(Selectable *tbl) {

  SWSS_LOG_ENTER();
  SWSS_LOG_NOTICE("Received a RADIUS SERVER event");

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
      m_radius_info.radius_auth_server_list[key].server_port = "";
      m_radius_info.radius_auth_server_list[key].server_key = "";
      m_radius_info.radius_auth_server_list[key].server_priority = "";

      // Look at the data that is sent for this key
      for (auto i : kfvFieldsValues(entry)) 
      {
        string a = fvField(i);
        string b = fvValue(i);
 
        SWSS_LOG_NOTICE("Received %s as field and %s as value", a.c_str(), b.c_str());

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
    }
    else if (val == DEL_COMMAND) 
    {
      SWSS_LOG_WARN("DEL operation on RADIUS_SERVER table");

      SWSS_LOG_NOTICE("Erasing server key");
      // sever deleted
      m_radius_info.radius_auth_server_list.erase(key);
    }   
  }

  updateRadiusServer();

  return true;
}

bool HostapdMgr::processRadiusGlobalTblEvent(Selectable *tbl) {

  SWSS_LOG_ENTER();
  SWSS_LOG_NOTICE("Received a RADIUS table event");
  string key(m_radius_info.m_radiusGlobalKey);

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
  
    // Global radius table modification results in a SET OP itself.
    // Incoming field values will not have passkey or nas_ip. Hence, we intiialize to NULL
    // so that incoming data will decide the final value.
    m_radius_info.m_radiusGlobalKey = "";

    SWSS_LOG_NOTICE("Received %s as key and %s as OP", key.c_str(), val.c_str());

    if (val == SET_COMMAND) 
    {
      // Look at the data that is sent for this key
      for (auto i : kfvFieldsValues(entry)) 
      {
        string a = fvField(i);
        string b = fvValue(i);
 
        SWSS_LOG_NOTICE("Received %s as field and %s as value", a.c_str(), b.c_str());

        if (a == "passkey")
        {
          m_radius_info.m_radiusGlobalKey = b;
        }
      }
    }
    else if (val == DEL_COMMAND) 
    {
      SWSS_LOG_WARN("DEL operation on RADIUS table");

      m_radius_info.m_radiusGlobalKey = "";
    }
  }

  // Since RADIUS config has been modified, deduce the new 
  // RADIUS server to be used and inform hostapd if required.
  if (m_radius_info.m_radiusGlobalKey != key)
  {
    updateRadiusServer();
  }

  return true;
}

void HostapdMgr::onMsg(int nlmsg_type, struct nl_object *obj)
{

  SWSS_LOG_ENTER();

  SWSS_LOG_DEBUG("nlmsg_type %d", nlmsg_type);

  if ((nlmsg_type != RTM_NEWLINK) && (nlmsg_type != RTM_DELLINK))
  {
    return;
  }

  struct rtnl_link *link = (struct rtnl_link *)obj;
  string key = rtnl_link_get_name(link);
  SWSS_LOG_DEBUG("key %s", getStdIfFormat(key).c_str());

  if (key.compare(0, INTFS_PREFIX.length(), INTFS_PREFIX))
  {
    return;
  }

  unsigned int flags = rtnl_link_get_flags(link);
  bool admin = flags & IFF_UP;
  bool oper = flags & IFF_LOWER_UP;

  unsigned int ifindex = rtnl_link_get_ifindex(link);
  char *type = rtnl_link_get_type(link);

  if (type)
  {
    SWSS_LOG_NOTICE("nlmsg type:%d key:%s admin:%d oper:%d  ifindex:%d type:%s",
                   nlmsg_type, getStdIfFormat(key).c_str(), admin, oper, ifindex, type);
  }
  else
  {
    SWSS_LOG_NOTICE("nlmsg type:%d key:%s admin:%d oper:%d ifindex:%d ",
                   nlmsg_type, getStdIfFormat(key).c_str(), admin, oper, ifindex);
  }


  /* teamd instances are dealt in teamsyncd */
  if (type && !strcmp(type, TEAM_DRV_NAME))
  {
    return;
  }

  if(key.find("E") == string::npos)
  {
    SWSS_LOG_NOTICE("Skipping non Ethernet interface %s", key.c_str());
    return;
  }

  string key1("");

  if(key.length() > 8)
  {
    // Native format: Ethernetx
    key1 = key;
  }
  else
  {
    key1 = "Eth" + key.substr(1,1) + '/' + key.substr(3);
  }

  /* New interface handling */
  if (m_intf_info.find(key1) == m_intf_info.end())
  {
    hostapd_intf_info_t intf;

    intf.control_mode = "force-authorized";
    intf.capabilities = "none";
    intf.admin_status = 0;
    intf.link_status = 0;
    intf.config_created = false;

    SWSS_LOG_NOTICE("New interface %s", key1.c_str());
    setPort(key1, intf);
  }

  vector<string> interfaces;
  
  /* Interface delete handling */
  if (nlmsg_type == RTM_DELLINK)
  {
    if (m_intf_info.find(key1) == m_intf_info.end())
    {
        SWSS_LOG_NOTICE("Unknown interface %s for Delete event ", key1.c_str());
        return;
    }

    SWSS_LOG_NOTICE("Delete %s event", key1.c_str());

    if (m_intf_info[key1].config_created)
    {
      /* delete config file */
      deleteConfFile(key1);

      /* update interfaces list */
      interfaces.push_back(key1);

      /* update JSON file */
      informHostapd("deleted", interfaces);
    }
    
    delPort(key1);
    return;
  }

  SWSS_LOG_NOTICE(": intf %s capabilities %s ctrl_mode %s admin_status %d link_status %d, global_auth %d admin %d oper %d", key1.c_str(), m_intf_info[key1].capabilities.c_str(), m_intf_info[key1].control_mode.c_str(),m_intf_info[key1].admin_status, m_intf_info[key1].link_status, m_glbl_info.enable_auth, admin, oper);
  /* Set the admin state first*/
  if (admin != m_intf_info[key1].admin_status)
  {
    m_intf_info[key1].admin_status = admin;
  }

  /* followed by the oper state */
  if (oper != m_intf_info[key1].link_status)
  {

    m_intf_info[key1].link_status = oper;

    if ((m_glbl_info.enable_auth) && (m_intf_info[key1].capabilities == "authenticator") &&
        (m_intf_info[key1].control_mode == "auto"))
    { 

      if ((m_intf_info[key1].link_status) && (!m_intf_info[key1].config_created) &&
          (m_radiusServerInUse != ""))
      {
        /* create config file */
        createConfFile(key1);
        
        /* update interfaces list */
        interfaces.push_back(key1);
        
        /* update JSON file */
        informHostapd("new", interfaces);
      }
      /* down't bring down hostapd interface when admin state goes down.
       * it will get deleted with RTM_DELLINK.
       */
    }
  }
}

void HostapdMgr::killHostapd(void)
{
  pid_t pid = getHostapdPid();
  if (pid)
  {
    kill(pid, 9);
  }
}

void HostapdMgr::setPort(const string & alias, const hostapd_intf_info_t & port)
{
  SWSS_LOG_ENTER();
  m_intf_info[alias] = port; 
}

void HostapdMgr::delPort(const string & alias)
{
  SWSS_LOG_ENTER();
  m_intf_info.erase(alias);
}

static bool file_exists(const string& file_name) 
{
  ifstream ifile;

  ifile.open(file_name);

  if(ifile) 
  {
    return true;;
  } 
  else 
  {
    return false;
  }
}

static string getHostIntfName(string ifname) 
{
  size_t pos;
  const std::string find = "/";
  const std::string replace = "_";
  const std::string e = "E";
  const std::string ethernet = "Eth";

  if(ifname.length() > 8)
  {
    // Ethernet0 format
  }
  else
  {
    // look for Eth1/1 format
    pos = ifname.find(find);
    
    if (pos != string::npos)
    {
      while(pos != string::npos)
      {
          ifname.replace(pos, replace.size(), replace);
          pos = ifname.find(find, pos + replace.size());
      }
      ifname.replace(0, ethernet.size(), e);
    }
    else
    {
      return ifname;
    }
  }
  return ifname;
}

void HostapdMgr::informHostapd(const string& type, const vector<string> & interfaces)
{
  SWSS_LOG_ENTER();

  string content;
  pid_t pid = 0;
  string pid_file(HOSTAPD_PID_FILE);

  SWSS_LOG_NOTICE("informHostapd(): Interface size %d", (int) interfaces.size());

  if (!interfaces.size()) {
    return;
  }


  string file;
  string cmd;
  string cmd_pid;

  file = "/etc/hostapd/hostapd_config.json";

  cmd = "rm -f ";
  cmd += file;


  if (start_hostapd)
  {
    int rc = 0;
    start_hostapd = false;

  if (system(cmd.c_str()))
  {
     SWSS_LOG_WARN("command %s could not be executed.", cmd.c_str());
  }
  else
  {
      SWSS_LOG_NOTICE("hostapd_config json file is deleted successfully before starting hostapd");
  }

    cmd_pid = "rm -f ";
    cmd_pid += HOSTAPD_PID_FILE;

    SWSS_LOG_NOTICE("Executing %s ", cmd_pid.c_str());

    rc = system(cmd_pid.c_str());
    SWSS_LOG_NOTICE("rc = %d, errno %d(%s) ", rc, errno, strerror(errno));

    if (rc < 0)
    {
       SWSS_LOG_WARN("%s could not be deleted.", pid_file.c_str());
    }

    // start hostapd

    content = "hostapd -d -P ";
    content += HOSTAPD_PID_FILE;
    content += " ";

    for(auto item: interfaces)
    {
      SWSS_LOG_NOTICE("starting hostapd on %s ", item.c_str());
      content += "/etc/hostapd/";
      content += getHostIntfName(item);
      content += ".conf ";
    }

    content += " & " ;

    SWSS_LOG_NOTICE("Executing: %s ", content.c_str());

    rc = system(content.c_str());
    SWSS_LOG_NOTICE("rc = %d, errno %d(%s) ", rc, errno, strerror(errno));

    if (rc < 0)
    {
       SWSS_LOG_WARN("hostapd could not be started.");
    }

    pid = getHostapdPid();
    if (pid)
    {
      SWSS_LOG_NOTICE("hostapd started with PID %d ", pid);
    }
    else
    {
      SWSS_LOG_NOTICE("hostapd could not be started: PID %d ", pid);
    }

    if (0 == waitForHostapdInit(pid))
    {
      SWSS_LOG_NOTICE("hostapd initialized with PID %d ", pid);
    }
    else
    {
      SWSS_LOG_NOTICE("hostapd could not be initialized with PID %d ", pid);
    }
  }
  else if (stop_hostapd)
  {
    // kill  hostapd

    stop_hostapd = false;

  if (system(cmd.c_str()))
  {
     SWSS_LOG_WARN("command %s could not be executed.", cmd.c_str());
  }
  else
  {
      SWSS_LOG_NOTICE("hostapd_config json file is deleted successfully before stopping hostapd");
  }


    pid = getHostapdPid();

    if (pid)
    {
      SWSS_LOG_NOTICE("terminating hostapd PID %d ", pid);
      kill(pid, 9);
    }
    else
    {
      SWSS_LOG_NOTICE("hostapd PID could not be found: PID %d ", pid);
    }
  }
  else 
  {
    string file;
    unsigned int cnt = 10;

    file = "/etc/hostapd/hostapd_config.json";
  
    while (cnt)
	{  
	  if (file_exists(file))
	  {
		SWSS_LOG_NOTICE("JSON file still exists. wait till the old file is read (%d)", cnt);
        cnt--;
		sleep(1);
	  }
      else
      {
        break;
      }
	}

     if (0 == cnt)
     {
		SWSS_LOG_NOTICE("JSON file still exists. not sending signal 1 to hostapd (%d)", cnt);
        return;
     }

    if ((type == "new") || (type == "modified")){ 

      content = "{\n";
      content += "\"";
      content += (type + "_interfaces\": \n");
      content += "[\n";

      for(auto item: interfaces) {
        content += "{\n";

        content += "\"if_name\": ";

        content += "\"";
        content += (getHostIntfName(item) + "\",\n");

        content += "\"path\": ";
        content += "\"/etc/hostapd/";
        content += getHostIntfName(item); 
        content += ".conf\"";
        content += "\n";
        content += "}";

        if (item.compare(interfaces.back())) {
          content += ",";
        }
          
        content += "\n";
      }
      content += "]\n";
      content += "}\n";

    }
    else if (type == "deleted") {
      content = "{\n";
      content += "\"";
      content += (type + "_interfaces\": \n");
      content += "[\n";

      for(auto item: interfaces) {
        content += "{\n";
        content += "\"if_name\": ";
        content += "\"";
        content += (getHostIntfName(item) + "\"\n");

        content += "}";


        if (item.compare(interfaces.back())) {
          content += ",";
        }
          
        content += "\n";
      }
      content += "]\n";
      content += "}\n";
    }
    else {
      return;
    }

    // Write to the file
    writeToFile(file, content);

    SWSS_LOG_NOTICE("sending Signal 1 to hostapd");

    // signal
    sendSignal();
  }
}

void HostapdMgr::createConfFile(const string& intf)
{
  SWSS_LOG_ENTER();

  string file;
  string content;
  bool exists = false;

  file = "/etc/hostapd/";
  file += (getHostIntfName(intf) + ".conf");

  content = "interface="; 
  content += (getHostIntfName(intf) + "\n");

  content += "driver=wired\n";
  content += "logger_stdout=63\n"; // 0x3f: Turn on for all hostapd modules
  content += "logger_stdout_level=2\n";
  content += "logger_syslog=-1\n";
  content += "logger_syslog_level=2\n";
  content += "ieee8021x=1\n";
 
  content += "ctrl_interface=/var/run/hostapd\n";
  content += "use_pae_group_addr=0\n";

  vector<pair<string, radius_server_info_t>> auth_sortedMap;
  for (auto& item: m_radius_info.radius_auth_server_list) 
  {
    if (false == item.second.config_ok)
    { 
      continue;
    } 
    auth_sortedMap.push_back(item);
  }

  if (0 != auth_sortedMap.size())
  {
      sort(auth_sortedMap.begin(), auth_sortedMap.end(), cmp);
  }

  for (auto const& item: auth_sortedMap)
  {
    if ((item.second.server_key  == "") && (m_radius_info.m_radiusGlobalKey == ""))
    {
       SWSS_LOG_WARN("Update in config file skipped %s as no key is configured.", item.first.c_str());
       continue;
    }
    content += "auth_server_addr="; 
    content += (item.second.server_ip.c_str());
    content += "\n";

    content += "auth_server_port="; 
    content += (item.second.server_port + "\n");

    content += "auth_server_shared_secret="; 
    if (item.second.server_key  == "")
    {
       content += (m_radius_info.m_radiusGlobalKey + "\n");
    }
    else
    {
       content += (item.second.server_key + "\n");
    }

    /* Write only the highest priroty server */
    break;
  }

  SWSS_LOG_NOTICE("active intf count %d ", active_intf_cnt);
  if (file_exists(file))
  {
    exists = true;
  }

  // Write to the file
  writeToFile(file, content);

  if (!active_intf_cnt) 
  {
    SWSS_LOG_NOTICE("setting start hostapd flag to true");
    start_hostapd = true;
  }

  if (false == exists)
  {
    active_intf_cnt++;
    SWSS_LOG_NOTICE("incrementing intf count %d", active_intf_cnt);
  }

  m_intf_info[intf].config_created = true;
}

void HostapdMgr::deleteConfFile(const string& intf)
{
  SWSS_LOG_ENTER();

  string file;
  string cmd;

  file = "/etc/hostapd/"; 
  file += (getHostIntfName(intf) + ".conf");

  cmd = "rm -f ";
  cmd += file;

  if (system(cmd.c_str()))
  {
     SWSS_LOG_WARN("command %s could not be executed.", cmd.c_str());
  }

  if (active_intf_cnt) 
  {
    SWSS_LOG_NOTICE("decrementing intf count %d", active_intf_cnt);
    active_intf_cnt--;
  }

  if (!active_intf_cnt) 
  {
    stop_hostapd = true;
    SWSS_LOG_NOTICE("setting stop hostapd flag to true");
  }
  m_intf_info[intf].config_created = false;
}

void HostapdMgr::writeToFile(const string& filename, const string& value)
{
  SWSS_LOG_ENTER();
  ofstream file;

  file.open(filename, ofstream::out | ofstream::trunc);

  // Write to the file
  file << value;

  // Close the file
  file.close();
}

void HostapdMgr::sendSignal(void)
{
  SWSS_LOG_ENTER();

  pid_t pid = 0;

  if (pid = getHostapdPid())
  {
    kill(pid, 1);
  }
}

pid_t HostapdMgr::getHostapdPid(void)
{
  SWSS_LOG_ENTER();
  pid_t pid = 0;
  int count = 10;
  
  while (system("pidof hostapd > ./temp.out"))
  {
     SWSS_LOG_WARN("command could not be executed. Remaining retry(%d)..", count--);
     usleep(100*1000);

     if (count <=0)
     {
        return 0;
     }
  }

  ifstream infile("./temp.out");
  if (!infile.is_open())
  {
     SWSS_LOG_WARN("The PID file is not readable");
     return 0;
  }

  string line;
  getline(infile, line);
  if (line.empty())
  {
     SWSS_LOG_WARN("The PID file is empty");
  }
  else
  {
     /*Store the PID value */
     pid = stoi(line, nullptr, 10);
  }

  return pid;
}

int HostapdMgr::waitForHostapdInit(pid_t hostapd_pid)
{
  SWSS_LOG_ENTER();
  pid_t pid = 0;
  int count = 10;
  string pid_file(HOSTAPD_PID_FILE);
  
  while (!file_exists(pid_file))
  {
     SWSS_LOG_WARN("%s not found. Remaining retry(%d)..", pid_file.c_str(), count--);
     usleep(100*1000);

     if (count <=0)
     {
        SWSS_LOG_WARN("Max retries exceeded to read from %s.", pid_file.c_str());
        return -1;
     }
  }

  ifstream infile(HOSTAPD_PID_FILE);
  string line;
  getline(infile, line);
  if (line.empty())
  {
     SWSS_LOG_WARN("The PID file %s is empty", pid_file.c_str());
     return -1;
  }

   /*Store the PID value */
   pid = stoi(line, nullptr, 10);

   SWSS_LOG_NOTICE("%s has pid %d", pid_file.c_str(), pid);
   SWSS_LOG_NOTICE("hostapd_pid = %d", hostapd_pid);

   return (pid == hostapd_pid)? 0 : -1;
}

