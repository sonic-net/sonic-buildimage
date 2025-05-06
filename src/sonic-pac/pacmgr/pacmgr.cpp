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

#include <vector>
#include <string>
#include <sstream>
#include <netinet/in.h>

#include "select.h"
#include "pacmgr.h"
#include "auth_mgr_exports.h"
#include "datatypes.h"
#include "packet.h"
#include "auth_mgr_api.h"
#include "auth_mgr_common.h"
#include "comm_mask.h"
#include "nimapi.h"
#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>
#include "select.h"
#include <errno.h>
#include <unistd.h>
#include <fstream>
#include <sys/socket.h>
#include <algorithm>
#include <netlink/route/link.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/if_ether.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "fpSonicUtils.h"

extern PacMgr pacmgr;
extern swss::Select s;
extern pacSocket_map_t g_pacSocketMap;

const string INTFS_PREFIX = "E";

PacMgr *pac;

PacMgr::PacMgr(DBConnector *configDb, DBConnector *stateDb,  DBConnector *appDb) :
               m_confPacTbl(configDb, CFG_PAC_PORT_CONFIG_TABLE),
               m_confPacGblTbl(configDb, CFG_PAC_GLOBAL_CONFIG_TABLE),
               m_confPacHostapdGblTbl(configDb, CFG_PAC_HOSTAPD_GLOBAL_CONFIG_TABLE),
               m_confVlanTbl(configDb, CFG_VLAN_TABLE_NAME),
               m_confVlanMemTbl(configDb, CFG_VLAN_MEMBER_TABLE_NAME),
               m_vlanTbl(stateDb, STATE_VLAN_TABLE_NAME),
               m_vlanMemTbl(stateDb, STATE_VLAN_MEMBER_TABLE_NAME),
               m_clearNotificationConsumer(configDb, "clearAuthSessions")
{
  Logger::linkToDbNative("pacmgr");
  memset(&m_glbl_info, 0, sizeof(m_glbl_info));

  SWSS_LOG_DEBUG("Installing PacMgr commands");
  pac = this;
}

std::vector<Selectable*> PacMgr::getSelectables() 
{
    vector<Selectable *> selectables{ &m_confPacTbl, &m_confPacGblTbl, &m_confPacHostapdGblTbl, 
                                      &m_vlanTbl, &m_vlanMemTbl, &m_clearNotificationConsumer};

    selectables.push_back(&m_confVlanTbl);
    selectables.push_back(&m_confVlanMemTbl);
    selectables.push_back(&pacqueue);
    return selectables;
}

bool PacMgr::processDbEvent(Selectable *tbl) {

    //check the source table and accordingly invoke the appropriate handlers

    if (tbl == ((Selectable *) & m_confPacTbl)) {
        return processPacPortConfTblEvent(tbl);
    }

    if (tbl == ((Selectable *) & m_confPacGblTbl)) {
        return processPacGlobalCfgTblEvent(tbl);
    }

    if (tbl == ((Selectable *) & m_vlanTbl)) {
        return processVlanTblEvent(tbl);  
    }

    if (tbl == ((Selectable *) & m_vlanMemTbl)) {
        return processVlanMemTblEvent(tbl);  
    }

    if (tbl == ((Selectable *) & m_confVlanTbl)) {
        return processConfVlanTblEvent(tbl);  
    }

    if (tbl == ((Selectable *) & m_confVlanMemTbl)) {
        return processConfVlanMemTblEvent(tbl);  
    }

    if (tbl == ((NotificationConsumer *) & m_clearNotificationConsumer)) {
        return processPacAuthSessionsClearNotifyEvent((NotificationConsumer *)tbl);
    }

    if (tbl == ((Selectable *) & m_confPacHostapdGblTbl)) {
        return processPacHostapdConfGlobalTblEvent(tbl);
    }

    if (tbl == ((Selectable *) & pacqueue)) {
         return processPacMsgQueue(tbl);
    }

    return false;
}

bool PacMgr::processPacMsgQueue(Selectable *tbl)
{
  pacqueue.readQueue();
  return true;
}

//Process the config db table events

bool PacMgr::processPacPortConfTblEvent(Selectable *tbl) {

    SWSS_LOG_ENTER();

    std::deque<KeyOpFieldsValuesTuple> entries;
    m_confPacTbl.pops(entries);

    SWSS_LOG_DEBUG("Received %d entries on config event on PAC_PORT_CONFIG_TABLE table", (int) entries.size());

    /* Nothing popped */
    if (entries.empty())
    {
        return false;
    }

    // Check through all the data
    for (auto entry : entries) {

        std::string key = kfvKey(entry);
        std::string op = kfvOp(entry);
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
            task_result = doPacPortTableSetTask(entry, intIfNum);
        }
        else if (op == DEL_COMMAND)
        {
            task_result = doPacPortTableDeleteTask(entry, intIfNum);
        }
        if (!task_result)
            return false;
     }
     return true;
}

bool PacMgr::doPacPortTableSetTask(const KeyOpFieldsValuesTuple & t, uint32 & intIfNum)
{
    SWSS_LOG_ENTER();
    const std::string & key = kfvKey(t);

    // Update pacPortConfigCache cache with incoming table data
    pacPortConfigCacheParams_t pacPortConfigCache;

    pacPortConfigCache.port_control_mode = AUTHMGR_PORT_CONTROL_MODE_DEF;
    pacPortConfigCache.host_control_mode = AUTHMGR_HOST_CONTROL_MODE_DEF;
    pacPortConfigCache.reauth_enable = AUTHMGR_REAUTH_ENABLE_DEF;
    pacPortConfigCache.reauth_period = AUTHMGR_REAUTH_PERIOD_DEF;
    pacPortConfigCache.reauth_period_from_server = AUTHMGR_REAUTH_PERIOD_FROM_SERVER_DEF;
    pacPortConfigCache.max_users_per_port = AUTHMGR_MAX_USERS_PER_PORT_DEF;
    pacPortConfigCache.max_reauth_attempts = AUTHMGR_MAX_REAUTH_ATTEMPTS_DEF;
    pacPortConfigCache.port_pae_role = AUTHMGR_PORT_PAE_ROLE_DEF;
    pacPortConfigCache.priority_list[INDEX_0] =  AUTHMGR_METHOD_8021X;
    pacPortConfigCache.priority_list[INDEX_1] =  AUTHMGR_METHOD_MAB;
    pacPortConfigCache.method_list[INDEX_0] =  AUTHMGR_METHOD_8021X;
    pacPortConfigCache.method_list[INDEX_1] =  AUTHMGR_METHOD_MAB;

    for (auto item = kfvFieldsValues(t).begin(); item != kfvFieldsValues(t).end(); item++)
    {
        const std::string & field = fvField(*item);
        const std::string & value = fvValue(*item);

        if (field == "port_control_mode")
        {
            if (value == "auto")
               pacPortConfigCache.port_control_mode =  AUTHMGR_PORT_AUTO;
            else if (value == "force-authorized")
               pacPortConfigCache.port_control_mode =  AUTHMGR_PORT_FORCE_AUTHORIZED;
            else if (value == "force-unauthorized")
               pacPortConfigCache.port_control_mode =  AUTHMGR_PORT_FORCE_UNAUTHORIZED;
            else {
               SWSS_LOG_WARN("Invalid port control mode received: %s", value.c_str());
               continue;
            }
        }
        if (field == "host_control_mode")
        {
            if (value == "single-host")
                pacPortConfigCache.host_control_mode=  AUTHMGR_SINGLE_AUTH_MODE;
            else if (value == "multi-host")
                pacPortConfigCache.host_control_mode =  AUTHMGR_MULTI_HOST_MODE;
            else if (value == "multi-auth")
                pacPortConfigCache.host_control_mode =  AUTHMGR_MULTI_AUTH_MODE;
            else {
               SWSS_LOG_WARN("Invalid host control mode received: %s", value.c_str());
               continue;
            }
        }
        if (field == "reauth_enable")
        {
            if(value == "true")
               pacPortConfigCache.reauth_enable =  TRUE;
            else if(value == "false")
               pacPortConfigCache.reauth_enable =  FALSE;
            else {
               SWSS_LOG_WARN("Invalid value received for reauth enable: %s", value.c_str());
               continue;
            }
        }

        try
        {
            if (field == "reauth_period")
            {
               pacPortConfigCache.reauth_period = stoi(value);
            }
            if (field == "reauth_period_from_server")
            {
                if(value == "true")
                   pacPortConfigCache.reauth_period_from_server =  TRUE;
                else if(value == "false")
                   pacPortConfigCache.reauth_period_from_server =  FALSE;
                else {
                   SWSS_LOG_WARN("Invalid option received for reauth period from server: %s", value.c_str());
                   continue;
                }
            }

            if (field == "max_users_per_port")
            {
                pacPortConfigCache.max_users_per_port = stoi(value);
            }
        }
        catch (...)
        {
            SWSS_LOG_WARN("Invalid value:%s received for %s from server", value.c_str(), field.c_str());
            continue;
        }
        if (field == "port_pae_role")
        {
            if(value == "authenticator")
               pacPortConfigCache.port_pae_role =  DOT1X_PAE_PORT_AUTH_CAPABLE;
            else if(value == "none")
               pacPortConfigCache.port_pae_role =  DOT1X_PAE_PORT_NONE_CAPABLE;
            else {
               SWSS_LOG_WARN("Invalid option received for port pae role: %s", value.c_str());
               continue;
            }
        }
        if (field == "priority_list")
        {
            string priority_str;
            uint32 k = 0;
            std::istringstream iss(value);
            while (getline(iss, priority_str, ','))
            {
               if (priority_str == "dot1x")
               {
                  pacPortConfigCache.priority_list[k] =  AUTHMGR_METHOD_8021X;
                  k++;
               }
               else if (priority_str == "mab")
               {
                  pacPortConfigCache.priority_list[k] =  AUTHMGR_METHOD_MAB;
                  k++;
               }
               else
               {
                  SWSS_LOG_WARN("Invalid option received for priority list: %s", value.c_str());
                  continue;
               }
            }
            if (k == 1)
               pacPortConfigCache.priority_list[k] =  AUTHMGR_METHOD_NONE; 
        }
        if (field == "method_list")
        {
            string method_str;
            uint32 k = 0;
            std::istringstream iss(value);
            while (getline(iss, method_str, ','))
            {
               if (method_str == "dot1x")
               {
                  pacPortConfigCache.method_list[k] =  AUTHMGR_METHOD_8021X;
                  k++;
               }
               else if (method_str == "mab")
               {
                  pacPortConfigCache.method_list[k] =  AUTHMGR_METHOD_MAB;
                  k++;
               }
               else
               {
                  SWSS_LOG_WARN("Invalid option received for method list: %s", value.c_str());
                  continue;
               }
            }
            if (k == 1)
               pacPortConfigCache.method_list[k] =  AUTHMGR_METHOD_NONE; 
        }
    }

    pacPortConfigTableMap::iterator iter = m_pacPortConfigMap.find(key);

    if(iter == m_pacPortConfigMap.end())
    {
        m_pacPortConfigMap.insert(pair<std::string, pacPortConfigCacheParams_t>(key, pacPortConfigCache));
        pacPortConfigTableMap::iterator iter = m_pacPortConfigMap.find(key);

        if(pacPortConfigCache.port_control_mode != AUTHMGR_PORT_CONTROL_MODE_DEF)
        {
            if ( SUCCESS != authmgrPortControlModeSet(intIfNum, pacPortConfigCache.port_control_mode))
            {
              iter->second.port_control_mode = AUTHMGR_PORT_CONTROL_MODE_DEF;
            }
        }
        if(pacPortConfigCache.host_control_mode != AUTHMGR_HOST_CONTROL_MODE_DEF)
        {
            if ( SUCCESS != authmgrHostControlModeSet(intIfNum, pacPortConfigCache.host_control_mode))
            {
              iter->second.host_control_mode = AUTHMGR_HOST_CONTROL_MODE_DEF;
            }
        }
        if(pacPortConfigCache.reauth_enable != AUTHMGR_REAUTH_ENABLE_DEF)
        {
            if ( SUCCESS != authmgrPortReAuthEnabledSet(intIfNum, ( BOOL)pacPortConfigCache.reauth_enable))
            {
              iter->second.reauth_enable = AUTHMGR_REAUTH_ENABLE_DEF;
            }
        }

        if(pacPortConfigCache.reauth_period_from_server != AUTHMGR_REAUTH_PERIOD_FROM_SERVER_DEF)
        {
            if ( SUCCESS != authmgrPortReAuthPeriodSet(intIfNum, pacPortConfigCache.reauth_period, ( BOOL)pacPortConfigCache.reauth_period_from_server))
            {
              iter->second.reauth_period_from_server = AUTHMGR_REAUTH_PERIOD_FROM_SERVER_DEF;
              iter->second.reauth_period = AUTHMGR_PORT_REAUTH_PERIOD_DEF;
            }
        }

        if(pacPortConfigCache.reauth_period_from_server == AUTHMGR_REAUTH_PERIOD_FROM_SERVER_DEF)
        {
            if ( SUCCESS != authmgrPortReAuthPeriodSet(intIfNum, AUTHMGR_PORT_REAUTH_PERIOD_DEF, ( BOOL)pacPortConfigCache.reauth_period_from_server))
            {
              iter->second.reauth_period_from_server = AUTHMGR_REAUTH_PERIOD_FROM_SERVER_DEF;
            }
        }
        if(pacPortConfigCache.max_users_per_port != AUTHMGR_MAX_USERS_PER_PORT_DEF)
        {
            if ( SUCCESS != authmgrPortMaxUsersSet(intIfNum, pacPortConfigCache.max_users_per_port))
            {
              iter->second.max_users_per_port = AUTHMGR_MAX_USERS_PER_PORT_DEF;
            }
        }
        if(pacPortConfigCache.max_reauth_attempts != AUTHMGR_MAX_REAUTH_ATTEMPTS_DEF)
        {
            if ( SUCCESS != authmgrPortMaxAuthAttemptsSet(intIfNum, pacPortConfigCache.max_reauth_attempts))
            {
              iter->second.max_reauth_attempts = AUTHMGR_MAX_REAUTH_ATTEMPTS_DEF;
            }
        }
        if(pacPortConfigCache.port_pae_role != AUTHMGR_PORT_PAE_ROLE_DEF)
        {
            if ( SUCCESS != authmgrDot1xCapabilitiesUpdate(intIfNum, pacPortConfigCache.port_pae_role))
            {
              iter->second.port_pae_role = AUTHMGR_PORT_PAE_ROLE_DEF;
            }
        }
        if(pacPortConfigCache.priority_list[INDEX_0] != AUTHMGR_PRIORITY_LIST_0_DEF)
        {
               if ( SUCCESS != authmgrPortAuthMethodSet( AUTHMGR_TYPE_PRIORITY, intIfNum,  AUTHMGR_METHOD_START, pacPortConfigCache.priority_list[INDEX_0]))
               {
                  iter->second.priority_list[INDEX_0] = AUTHMGR_PRIORITY_LIST_0_DEF;
               }
        }
        if(pacPortConfigCache.priority_list[INDEX_1] != AUTHMGR_PRIORITY_LIST_1_DEF)
        {
               if ( SUCCESS != authmgrPortAuthMethodSet( AUTHMGR_TYPE_PRIORITY, intIfNum,  AUTHMGR_METHOD_START+1, pacPortConfigCache.priority_list[INDEX_1]))
               {
                  iter->second.priority_list[INDEX_1] = AUTHMGR_PRIORITY_LIST_1_DEF;
               }
        }
        if(pacPortConfigCache.method_list[INDEX_0] != AUTHMGR_METHOD_LIST_0_DEF)
        {
               if ( SUCCESS != authmgrPortAuthMethodSet( AUTHMGR_TYPE_ORDER, intIfNum,  AUTHMGR_METHOD_START, pacPortConfigCache.method_list[INDEX_0]))
               {
                  iter->second.method_list[INDEX_0] = AUTHMGR_METHOD_LIST_0_DEF;
               }
        }
        if(pacPortConfigCache.method_list[INDEX_1] != AUTHMGR_METHOD_LIST_1_DEF)
        {
               if ( SUCCESS != authmgrPortAuthMethodSet( AUTHMGR_TYPE_ORDER, intIfNum,  AUTHMGR_METHOD_START+1, pacPortConfigCache.method_list[INDEX_1]))
               {
                  iter->second.method_list[INDEX_1] = AUTHMGR_METHOD_LIST_1_DEF;
               }
        }
    }
    else //Interface entry already exists in local cache, check for any parameter change for Add/Update/Delete
    {
        // port_control_mode
        if (((iter->second.port_control_mode == AUTHMGR_PORT_CONTROL_MODE_DEF) &&
            (pacPortConfigCache.port_control_mode != AUTHMGR_PORT_CONTROL_MODE_DEF)) ||
            ((iter->second.port_control_mode != AUTHMGR_PORT_CONTROL_MODE_DEF) &&
            (pacPortConfigCache.port_control_mode != iter->second.port_control_mode)))
        {
            if ( SUCCESS == authmgrPortControlModeSet(intIfNum, pacPortConfigCache.port_control_mode))
            {
              iter->second.port_control_mode = pacPortConfigCache.port_control_mode;
            }
            else
	    {
              SWSS_LOG_ERROR("Unable to set the authentication port control mode.\n");
              return false;
            }
        }
        // host control mode
        if (((iter->second.host_control_mode == AUTHMGR_HOST_CONTROL_MODE_DEF) &&
            (pacPortConfigCache.host_control_mode != AUTHMGR_HOST_CONTROL_MODE_DEF)) ||
            ((iter->second.host_control_mode != AUTHMGR_HOST_CONTROL_MODE_DEF) &&
            (pacPortConfigCache.host_control_mode != iter->second.host_control_mode)))
        {
            if ( SUCCESS == authmgrHostControlModeSet(intIfNum, pacPortConfigCache.host_control_mode))
            {
              iter->second.host_control_mode = pacPortConfigCache.host_control_mode;
            }
            else
	    {
              SWSS_LOG_ERROR("Unable to set the authentication host control mode.\n");
              return false;
            }
        }
        // reauth_enable
        if (((iter->second.reauth_enable == AUTHMGR_REAUTH_ENABLE_DEF) &&
            (pacPortConfigCache.reauth_enable != AUTHMGR_REAUTH_ENABLE_DEF)) ||
            ((iter->second.reauth_enable != AUTHMGR_REAUTH_ENABLE_DEF) &&
            (pacPortConfigCache.reauth_enable != iter->second.reauth_enable)))
        {
            if ( SUCCESS == authmgrPortReAuthEnabledSet(intIfNum, ( BOOL)pacPortConfigCache.reauth_enable))
            {
              iter->second.reauth_enable = pacPortConfigCache.reauth_enable;
            }
            else
	    {
              SWSS_LOG_ERROR("Unable to set the authentication reauth enable.\n");
              return false;
            }
        }
        
        // reauth_period_from_server
        if (((iter->second.reauth_period_from_server == AUTHMGR_REAUTH_PERIOD_FROM_SERVER_DEF) &&
            (pacPortConfigCache.reauth_period_from_server != AUTHMGR_REAUTH_PERIOD_FROM_SERVER_DEF)) ||
            ((iter->second.reauth_period_from_server != AUTHMGR_REAUTH_PERIOD_FROM_SERVER_DEF) &&
            ((pacPortConfigCache.reauth_period_from_server != iter->second.reauth_period_from_server) ||
             (pacPortConfigCache.reauth_period != iter->second.reauth_period))))
        {
            uint32 val = AUTHMGR_REAUTH_PERIOD_DEF;
            if (AUTHMGR_REAUTH_PERIOD_DEF != pacPortConfigCache.reauth_period)
            {
               val = pacPortConfigCache.reauth_period;
            }
            if ( SUCCESS == authmgrPortReAuthPeriodSet(intIfNum, val, ( BOOL)pacPortConfigCache.reauth_period_from_server))
            {
              iter->second.reauth_period_from_server = pacPortConfigCache.reauth_period_from_server;
              iter->second.reauth_period = val;
            }
            else
	    {
              SWSS_LOG_ERROR("Unable to set the reauth period from server.\n");
              return false;
            }
        }
        // max_users_per_port
        if (((iter->second.max_users_per_port == AUTHMGR_MAX_USERS_PER_PORT_DEF) &&
            (pacPortConfigCache.max_users_per_port != AUTHMGR_MAX_USERS_PER_PORT_DEF)) ||
            ((iter->second.max_users_per_port != AUTHMGR_MAX_USERS_PER_PORT_DEF) &&
            (pacPortConfigCache.max_users_per_port != iter->second.max_users_per_port)))
        {
            if ( SUCCESS == authmgrPortMaxUsersSet(intIfNum, pacPortConfigCache.max_users_per_port))
            {
              iter->second.max_users_per_port = pacPortConfigCache.max_users_per_port;
            }
            else
	    {
              SWSS_LOG_ERROR("Unable to set max users per port.\n");
              return false;
            }
        }
        // max_reauth_attempts
        if (((iter->second.max_reauth_attempts == AUTHMGR_MAX_REAUTH_ATTEMPTS_DEF) &&
            (pacPortConfigCache.max_reauth_attempts != AUTHMGR_MAX_REAUTH_ATTEMPTS_DEF)) ||
            ((iter->second.max_reauth_attempts != AUTHMGR_MAX_REAUTH_ATTEMPTS_DEF) &&
            (pacPortConfigCache.max_reauth_attempts != iter->second.max_reauth_attempts)))
        {
            if ( SUCCESS == authmgrPortMaxAuthAttemptsSet(intIfNum, pacPortConfigCache.max_reauth_attempts))
            {
              iter->second.max_reauth_attempts = pacPortConfigCache.max_reauth_attempts;
            }
            else
	    {
              SWSS_LOG_ERROR("Unable to set authentication retry max attempts..");
              return false;
            }
        }
        // port_pae_role
        if (((iter->second.port_pae_role == AUTHMGR_PORT_PAE_ROLE_DEF) &&
            (pacPortConfigCache.port_pae_role != AUTHMGR_PORT_PAE_ROLE_DEF)) ||
            ((iter->second.port_pae_role != AUTHMGR_PORT_PAE_ROLE_DEF) &&
            (pacPortConfigCache.port_pae_role != iter->second.port_pae_role)))
        {
            if ( SUCCESS == authmgrDot1xCapabilitiesUpdate(intIfNum, pacPortConfigCache.port_pae_role))
            {
              iter->second.port_pae_role = pacPortConfigCache.port_pae_role;
            }
            else
	    {
              SWSS_LOG_ERROR("Unable to set the PAE mode on the specified port.");
              return false;
            }
        }
 
        // priority_list
        if (((iter->second.priority_list[INDEX_0] == AUTHMGR_PRIORITY_LIST_0_DEF ) &&
            (pacPortConfigCache.priority_list[INDEX_0] != AUTHMGR_PRIORITY_LIST_0_DEF)) ||
            ((iter->second.priority_list[INDEX_0] != AUTHMGR_PRIORITY_LIST_0_DEF) &&
            (pacPortConfigCache.priority_list[INDEX_0] != iter->second.priority_list[INDEX_0])))
        {
            if ( SUCCESS == authmgrPortAuthMethodSet( AUTHMGR_TYPE_PRIORITY, intIfNum,  AUTHMGR_METHOD_START, pacPortConfigCache.priority_list[INDEX_0]))
            { 
               iter->second.priority_list[INDEX_0] = pacPortConfigCache.priority_list[INDEX_0];
            }
        }
        if (((iter->second.priority_list[INDEX_1] == AUTHMGR_PRIORITY_LIST_1_DEF ) &&
            (pacPortConfigCache.priority_list[INDEX_1] != AUTHMGR_PRIORITY_LIST_1_DEF)) ||
            ((iter->second.priority_list[INDEX_1] != AUTHMGR_PRIORITY_LIST_1_DEF) &&
            (pacPortConfigCache.priority_list[INDEX_1] != iter->second.priority_list[INDEX_1])))
        {
            if ( SUCCESS == authmgrPortAuthMethodSet( AUTHMGR_TYPE_PRIORITY, intIfNum,  AUTHMGR_METHOD_START+1, pacPortConfigCache.priority_list[INDEX_1]))
            {
               iter->second.priority_list[INDEX_1] = pacPortConfigCache.priority_list[INDEX_1];
            }
        }

        // method_list
        if (((iter->second.method_list[INDEX_0] == AUTHMGR_METHOD_LIST_0_DEF ) &&
            (pacPortConfigCache.method_list[INDEX_0] != AUTHMGR_METHOD_LIST_0_DEF)) ||
            ((iter->second.method_list[INDEX_0] != AUTHMGR_METHOD_LIST_0_DEF) &&
            (pacPortConfigCache.method_list[INDEX_0] != iter->second.method_list[INDEX_0])))
        {
            if ( SUCCESS == authmgrPortAuthMethodSet( AUTHMGR_TYPE_ORDER, intIfNum,  AUTHMGR_METHOD_START, pacPortConfigCache.method_list[INDEX_0]))
            { 
               iter->second.method_list[INDEX_0] = pacPortConfigCache.method_list[INDEX_0];
            }
        }
        if (((iter->second.method_list[INDEX_1] == AUTHMGR_METHOD_LIST_1_DEF ) &&
            (pacPortConfigCache.method_list[INDEX_1] != AUTHMGR_METHOD_LIST_1_DEF)) ||
            ((iter->second.method_list[INDEX_1] != AUTHMGR_METHOD_LIST_1_DEF) &&
            (pacPortConfigCache.method_list[INDEX_1] != iter->second.method_list[INDEX_1])))
        {
            if ( SUCCESS == authmgrPortAuthMethodSet( AUTHMGR_TYPE_ORDER, intIfNum,  AUTHMGR_METHOD_START+1, pacPortConfigCache.method_list[INDEX_1]))
            { 
               iter->second.method_list[INDEX_1] = pacPortConfigCache.method_list[INDEX_1];
            }
        }
     }
     return true;
}

bool PacMgr::doPacPortTableDeleteTask(const KeyOpFieldsValuesTuple & t, uint32 & intIfNum)
{
    SWSS_LOG_ENTER();
    const std::string & key = kfvKey(t);
    pacPortConfigTableMap::iterator iter = m_pacPortConfigMap.find(key);
    if(iter != m_pacPortConfigMap.end())
    {
      if ( SUCCESS == authmgrPortInfoReset(intIfNum,  TRUE))
      {
        iter->second.port_control_mode = AUTHMGR_PORT_CONTROL_MODE_DEF;
        iter->second.host_control_mode = AUTHMGR_HOST_CONTROL_MODE_DEF;
        iter->second.reauth_enable = AUTHMGR_REAUTH_ENABLE_DEF;
        iter->second.reauth_period = AUTHMGR_REAUTH_PERIOD_DEF;
        iter->second.reauth_period_from_server = AUTHMGR_REAUTH_PERIOD_FROM_SERVER_DEF;
        iter->second.max_users_per_port = AUTHMGR_MAX_USERS_PER_PORT_DEF;
        iter->second.max_reauth_attempts = AUTHMGR_MAX_REAUTH_ATTEMPTS_DEF;
        iter->second.port_pae_role = AUTHMGR_PORT_PAE_ROLE_DEF;
        iter->second.priority_list[INDEX_0] =  AUTHMGR_METHOD_8021X;
        iter->second.priority_list[INDEX_1] =  AUTHMGR_METHOD_MAB;
        iter->second.method_list[INDEX_0] =  AUTHMGR_METHOD_8021X;
        iter->second.method_list[INDEX_1] =  AUTHMGR_METHOD_MAB;
      }
    }
    return true;
}

bool PacMgr::processPacGlobalCfgTblEvent(Selectable *tbl) {

    std::deque<KeyOpFieldsValuesTuple> entries;
    m_confPacGblTbl.pops(entries);
    bool task_result = false;

    SWSS_LOG_DEBUG("Received %d entries on config event on PAC_GLOBAL_CONFIG_TABLE table.", (int) entries.size());

    // Check through all the data
    for (auto entry : entries) {

        std::string key = kfvKey(entry);
        std::string op = kfvOp(entry);

        SWSS_LOG_DEBUG("Received %s as key and %s as OP", key.c_str(), op.c_str());

        if (op == SET_COMMAND)
        {
            task_result = doPacGlobalTableSetTask(entry);
        }
        else if (op == DEL_COMMAND)
        {
            task_result = doPacGlobalTableDeleteTask();
        }
        if (!task_result)
            return false;
    }
    return true;
}

bool PacMgr::doPacGlobalTableSetTask(const KeyOpFieldsValuesTuple & t)
{
    SWSS_LOG_ENTER();

    return true;
}

bool PacMgr::doPacGlobalTableDeleteTask()
{
    SWSS_LOG_ENTER();

    return true;
}

bool PacMgr::processVlanTblEvent(Selectable *tbl) 
{
    std::deque<KeyOpFieldsValuesTuple> entries;
    m_vlanTbl.pops(entries);

    SWSS_LOG_DEBUG("Received %d entries on event on STATE_VLAN_TABLE.", (int) entries.size());

    /* Nothing popped */
    if (entries.empty())
    {
        return false;
    }

    // Check through all the data
    for (auto entry : entries) {

        std::string key = kfvKey(entry);
        std::string op = kfvOp(entry);

        SWSS_LOG_DEBUG("Received %s as key and %s as OP", key.c_str(), op.c_str());

        if (strncmp(key.c_str(), VLAN_PREFIX, 4))
        {
            SWSS_LOG_DEBUG("Invalid key format. No 'Vlan' prefix: %s", key.c_str());
            continue;
        }
      
        // Remove VLAN prefix
        int vlan; 
        try
        {
            vlan = stoi(key.substr(4));
        }
        catch (...)
        {
            SWSS_LOG_WARN("Invalid key format. Not a number after 'Vlan' prefix: %s", key.c_str());
            continue;
        }
 
        dot1qNotifyData_t vlanData;
        memset(&vlanData, 0, sizeof(vlanData));
        vlanData.data.vlanId = vlan;

        if (op == SET_COMMAND)
        {
            // Inform authmgr about creation of VLAN.  
            if ( SUCCESS != authmgrVlanChangeCallback(&vlanData, 0, VLAN_ADD_NOTIFY))
            {
                SWSS_LOG_WARN("Unable to notify authmgr of event VLAN_ADD_NOTIFY."); 
                continue;
            }
        }
        else if (op == DEL_COMMAND)
        {
            // Inform authmgr about deletion of VLAN.  
            if ( SUCCESS != authmgrVlanChangeCallback(&vlanData, 0, VLAN_DELETE_PENDING_NOTIFY))
            {
                SWSS_LOG_WARN("Unable to notify authmgr of event VLAN_DELETE_PENDING_NOTIFY."); 
                continue;
            }
        }
        else
        {
            SWSS_LOG_DEBUG("Invalid operation received on STATE_VLAN_TABLE."); 
        }
    }
 
    return true;
}

bool PacMgr::processVlanMemTblEvent(Selectable *tbl)
{
    std::deque<KeyOpFieldsValuesTuple> entries;
    m_vlanMemTbl.pops(entries);

    SWSS_LOG_DEBUG("Received %d entries on event on STATE_VLAN_MEMBER_TABLE.", (int) entries.size());

    /* Nothing popped */
    if (entries.empty())
    {
        return false;
    }

    // Check through all the data
    for (auto entry : entries) {

        std::string key = kfvKey(entry);
        std::string op = kfvOp(entry);

        SWSS_LOG_DEBUG("Received %s as key and %s as OP", key.c_str(), op.c_str());

        if (strncmp(key.c_str(), VLAN_PREFIX, 4))
        {
            SWSS_LOG_DEBUG("Invalid key format. No 'Vlan' prefix: %s", key.c_str());
            continue;
        }

        int vlan; 
        string port;
        dot1qNotifyData_t vlanData;
        uint32 intIfNum;

        try
        {
            key = key.substr(4);
            size_t found = key.find(STATEDB_KEY_SEPARATOR);
            // Remove VLAN prefix
    
            if (found != string::npos)
            {
                // Parse VLAN and port
                vlan = stoi(key.substr(0, found));
                port = key.substr(found+1);
            }
            else
            {
                SWSS_LOG_WARN("Invalid key format %s for STATE_VLAN_MEMBER_TABLE.", kfvKey(entry).c_str());
                continue;
            }
            if(port.find(INTFS_PREFIX) == string::npos)
            {
                continue;
            }

            memset(&vlanData, 0, sizeof(vlanData));
            vlanData.data.vlanId = vlan;
        }
        catch (...)
        {
            SWSS_LOG_WARN("Invalid key format %s for STATE_VLAN_MEMBER_TABLE.", kfvKey(entry).c_str());
            continue;
        }

        if(fpGetIntIfNumFromHostIfName(port.c_str(), &intIfNum) !=  SUCCESS)
        {
            SWSS_LOG_NOTICE("Unable to get the internal interface number for %s.", port.c_str());
            continue;
        }
        for (auto i : kfvFieldsValues(entry)) 
        {
            string field = fvField(i);
            string value = fvValue(i);
  
            if (field == "tagging_mode")
            {
                if (value == "tagged")
                {
                    vlanData.tagged =  TRUE;
                } 
            }
        }

        if (op == SET_COMMAND)
        {
            // Inform authmgr about addition of port to VLAN.  
            if ( SUCCESS != authmgrVlanChangeCallback(&vlanData, intIfNum, VLAN_ADD_PORT_NOTIFY))
            {
                SWSS_LOG_WARN("Unable to notify authmgr of event VLAN_ADD_NOTIFY."); 
                continue;
            }
        }
        else if (op == DEL_COMMAND)
        {
            // Inform authmgr about deletion of port from VLAN.  
            if ( SUCCESS != authmgrVlanChangeCallback(&vlanData, intIfNum, VLAN_DELETE_PORT_NOTIFY))
            {
                SWSS_LOG_WARN("Unable to notify authmgr of event VLAN_ADD_NOTIFY."); 
                continue;
            }
        }
        else
        {
            SWSS_LOG_WARN("Invalid operation received on STATE_VLAN_TABLE."); 
        }
    }
 
    return true;
}

bool PacMgr::processConfVlanTblEvent(Selectable *tbl) 
{
    std::deque<KeyOpFieldsValuesTuple> entries;
    m_confVlanTbl.pops(entries);

    SWSS_LOG_DEBUG("Received %d entries on event on CONF_VLAN_TABLE.", (int) entries.size());

    /* Nothing popped */
    if (entries.empty())
    {
        return false;
    }

    // Check through all the data
    for (auto entry : entries) {

        std::string key = kfvKey(entry);
        std::string op = kfvOp(entry);

        SWSS_LOG_DEBUG("Received %s as key and %s as OP", key.c_str(), op.c_str());

        if (strncmp(key.c_str(), VLAN_PREFIX, 4))
        {
            SWSS_LOG_WARN("Invalid key format. No 'Vlan' prefix: %s", key.c_str());
            continue;
        }
      
        // Remove VLAN prefix
        int vlan; 
        try
        {
            vlan = stoi(key.substr(4));
        }
        catch (...)
        {
            SWSS_LOG_WARN("Invalid key format. Not a number after 'Vlan' prefix: %s", key.c_str());
            continue;
        }
 
        dot1qNotifyData_t vlanData;
        memset(&vlanData, 0, sizeof(vlanData));
        vlanData.data.vlanId = vlan;


        if (op == SET_COMMAND)
        {
            // Inform authmgr about creation of VLAN.  
            if ( SUCCESS != authmgrVlanConfChangeCallback(&vlanData, 0, VLAN_ADD_NOTIFY))
            {
                SWSS_LOG_WARN("Unable to notify authmgr of event VLAN_ADD_NOTIFY."); 
                continue;
            }
        }
        else if (op == DEL_COMMAND)
        {
            // Inform authmgr about deletion of VLAN.  
            if ( SUCCESS != authmgrVlanConfChangeCallback(&vlanData, 0, VLAN_DELETE_PENDING_NOTIFY))
            {
                SWSS_LOG_WARN("Unable to notify authmgr of event VLAN_DELETE_PENDING_NOTIFY."); 
                continue;
            }
        }
        else
        {
            SWSS_LOG_WARN("Invalid operation received on STATE_VLAN_TABLE."); 
        }
    }
 
    return true;
}

bool PacMgr::processConfVlanMemTblEvent(Selectable *tbl)
{
    std::deque<KeyOpFieldsValuesTuple> entries;
    m_confVlanMemTbl.pops(entries);

    SWSS_LOG_DEBUG("Received %d entries on event on CONF_VLAN_MEMBER_TABLE.", (int) entries.size());

    /* Nothing popped */
    if (entries.empty())
    {
        return false;
    }

    // Check through all the data
    for (auto entry : entries) {

        std::string key = kfvKey(entry);
        std::string op = kfvOp(entry);

        SWSS_LOG_DEBUG("Received %s as key and %s as OP", key.c_str(), op.c_str());

        if (strncmp(key.c_str(), VLAN_PREFIX, 4))
        {
            SWSS_LOG_WARN("Invalid key format. No 'Vlan' prefix: %s", key.c_str());
            continue;
        }
      
        dot1qNotifyData_t vlanData;
        uint32 intIfNum;
        int vlan; 
        string port;

        try
        {
            key = key.substr(4);
            size_t found = key.find(STATEDB_KEY_SEPARATOR);
            // Remove VLAN prefix
    
            if (found != string::npos)
            {
                // Parse VLAN and port
                vlan = stoi(key.substr(0, found));
                port = key.substr(found+1);
            }
            else
            {
                SWSS_LOG_WARN("Invalid key format %s for STATE_VLAN_MEMBER_TABLE.", kfvKey(entry).c_str());
                continue;
            }
   
            if(port.find(INTFS_PREFIX) == string::npos)
            {
                continue;
            }
    
            memset(&vlanData, 0, sizeof(vlanData));
            vlanData.data.vlanId = vlan;
        }
        catch (...)
        {
            SWSS_LOG_WARN("Invalid key format %s for STATE_VLAN_MEMBER_TABLE.", kfvKey(entry).c_str());
            continue;
        }

        if(fpGetIntIfNumFromHostIfName(port.c_str(), &intIfNum) !=  SUCCESS)
        {
            SWSS_LOG_NOTICE("Unable to get the internal interface number for %s.", port.c_str());
            continue;
        }

        for (auto i : kfvFieldsValues(entry)) 
        {
            string field = fvField(i);
            string value = fvValue(i);
  
            if (field == "tagging_mode")
            {
                if (value == "tagged")
                {
                    vlanData.tagged =  TRUE;
                } 
            }
        }

        if (op == SET_COMMAND)
        {
            // Inform authmgr about addition of port to VLAN.  
            if ( SUCCESS != authmgrVlanConfChangeCallback(&vlanData, intIfNum, VLAN_ADD_PORT_NOTIFY))
            {
                SWSS_LOG_WARN("Unable to notify authmgr of event VLAN_ADD_NOTIFY."); 
                continue;
            }
        }
        else if (op == DEL_COMMAND)
        {
            // Inform authmgr about deletion of port from VLAN.  
            if ( SUCCESS != authmgrVlanConfChangeCallback(&vlanData, intIfNum, VLAN_DELETE_PORT_NOTIFY))
            {
                SWSS_LOG_WARN("Unable to notify authmgr of event VLAN_ADD_NOTIFY."); 
                continue;
            }
        }
        else
        {
            SWSS_LOG_WARN("Invalid operation received on STATE_VLAN_TABLE."); 
        }
    }
 
    return true;
}

bool PacMgr::processPacAuthSessionsClearNotifyEvent(NotificationConsumer *tbl) {

  SWSS_LOG_DEBUG("Received PAC authentication sessions clear notification");
  std::string op, data;
  std::vector<swss::FieldValueTuple> values;
  uint32 iface =  ALL_INTERFACES, nextIface =  ALL_INTERFACES;

  SWSS_LOG_ENTER();

  m_clearNotificationConsumer.pop(op, data, values);
  SWSS_LOG_DEBUG("Clear Auth session for %s: data: %s", op.c_str(), data.c_str());

  if (0 == strcmp(data.c_str(), "all"))  /* clear all authentication sessions */
  {
        iface =  ALL_INTERFACES;
        if ( SUCCESS != authmgrFirstValidIntfNumber(&iface))
        {
	  SWSS_LOG_ERROR("Switch doesn't contain valid interfaces.");
          return false;
        }

        do
        {
          if ( SUCCESS != authmgrPortInitializeSet(iface,  TRUE))
          {
            SWSS_LOG_ERROR("Unable to clear/initialize authentication sessions for Interface : %d.", iface);
            return false;
          }
          if( SUCCESS != authmgrNextValidIntf(iface, &nextIface))
          {
            break;
          }
          iface = nextIface;
        }while(1);
  }
  else if(data.find(INTFS_PREFIX) != string::npos)
  {
        uint32 intIfNum;

        if(fpGetIntIfNumFromHostIfName(data.c_str(), &intIfNum) !=  SUCCESS)
        {
            SWSS_LOG_NOTICE("Unable to get the internal interface number for %s.", data.c_str());
            return false;
        }

        if ( SUCCESS != authmgrPortInitializeSet(intIfNum,  TRUE))
        {
	  SWSS_LOG_ERROR("Unable to clear authentication session of Interface : %s", data.c_str());
	  return false;
        }
  }
  else if ( NULLPTR != data.c_str())
  {
         enetMacAddr_t clientMacAddr;
        uint32 mac_addr[6];
        memset(&clientMacAddr.addr,0, ENET_MAC_ADDR_LEN);
        sscanf(data.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",
               &mac_addr[0], &mac_addr[1],
               &mac_addr[2], &mac_addr[3],
               &mac_addr[4], &mac_addr[5]);

        for (int i = 0; i < 6; i++)
        {
          clientMacAddr.addr[i] = ( uchar8)mac_addr[i];
        }
        if(authmgrClientDelete(clientMacAddr) !=  SUCCESS)
        {
            SWSS_LOG_WARN("Failed to Clear authentication session for %s", data.c_str());
            return false;
        }
  }

  return true;
}

bool PacMgr::processPacHostapdConfGlobalTblEvent(Selectable *tbl) 
{
  authmgrAuthRespParams_t callbackParams;

  deque<KeyOpFieldsValuesTuple> entries;
  m_confPacHostapdGblTbl.pops(entries);

  SWSS_LOG_DEBUG("Received %d entries on config event on HOSTAPD_GLOBAL_CONFIG_TABLE table", (int) entries.size());

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

    SWSS_LOG_DEBUG("Received %s as key and %s as OP", key.c_str(), val.c_str());

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
              m_glbl_info.enable_auth = 1;
            }
          } 
          else if (b == "false")
          {
            // dot1x disabled
            if (m_glbl_info.enable_auth) 
            {
              m_glbl_info.enable_auth = 0;

              memset(&callbackParams, 0, sizeof(callbackParams));
              callbackParams.clientParams.info.enableStatus =  FALSE;

              authmgrPortClientAuthStatusUpdate( ALL_INTERFACES,  AUTHMGR_METHOD_8021X, 
                                                 AUTHMGR_METHOD_CHANGE, &callbackParams);
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

void PacMgr::processPacket(int m_pac_socket)
{
    struct sockaddr_ll from;
    struct sockaddr_ll to;
    struct iovec iov;
    struct msghdr msg;
    uint16_t vlan_id = 0;
    ssize_t  packetLen = 0;
    static char pkt[MAX_PACKET_SIZE];
    struct cmsghdr *cmsg;
    struct tpacket_auxdata *aux;
    union {
      char buf[CMSG_SPACE(MAX_PACKET_SIZE)];
      struct cmsghdr align;
    } cmsg_buf;
    char ifname[IF_NAMESIZE] = {0};

    uint32 intIfNum;
     enetMacAddr_t macAddr;
     uchar8           eap_ethtype[] = {0x88, 0x8e};
     uchar8 intfMac[ETHER_ADDR_LEN];

    memset(pkt, 0, sizeof(pkt));
    memset(&from, 0, sizeof(struct sockaddr_ll));
    memset(&to, 0, sizeof(struct sockaddr_ll));
    memset(&iov, 0, sizeof(struct iovec));
    memset(&msg, 0, sizeof(struct msghdr));

    msg.msg_name        = &from;
    msg.msg_namelen     = sizeof(from);
    msg.msg_iov         = &iov;
    msg.msg_iovlen      = 1;
    msg.msg_controllen   = sizeof(cmsg_buf);
    msg.msg_control      = &cmsg_buf;
    msg.msg_flags       = 0;

    iov.iov_len         = MAX_PACKET_SIZE;
    iov.iov_base        = pkt;

    packetLen = recvmsg(m_pac_socket, &msg, MSG_TRUNC);

    if (0 > packetLen)
    {
        if (errno == ENETDOWN)
            SWSS_LOG_NOTICE("%s : errno : Network is down %d \n", __FUNCTION__, from.sll_ifindex);
        else
            SWSS_LOG_NOTICE("%d : errno : %s \n", from.sll_ifindex, strerror(errno));

        return;
    }
    


    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg))
    {
        if (cmsg->cmsg_level == SOL_PACKET || cmsg->cmsg_type == PACKET_AUXDATA)
        {
            aux = (struct tpacket_auxdata *)CMSG_DATA(cmsg);
            if (aux->tp_status & TP_STATUS_VLAN_VALID)
            {
                vlan_id = (aux->tp_vlan_tci & 0x0fff);
                break;
            }
        }
    }

    if_indextoname(from.sll_ifindex, ifname);

    string name(ifname);
    if(name.find(INTFS_PREFIX) == string::npos)
    {
        // SWSS_LOG_NOTICE("Unsupported interface format. No 'E' prefix: %s", ifname);
        return;
    }

    memcpy (macAddr.addr, (unsigned char*)&pkt[ETHER_ADDR_LEN], sizeof(macAddr));

    if (memcmp(&pkt[12], eap_ethtype, sizeof(eap_ethtype)) == 0)
    {
        SWSS_LOG_NOTICE("Received packet is EAPOL. Ignoring unlearnt packet trigger due to EAPOL pkt type %02X from %s", pkt[15], ifname);
        SWSS_LOG_NOTICE("Src MAC %02X:%02X:%02X:%02X:%02X:%02X ", 
                       (unsigned char)macAddr.addr[0], (unsigned char)macAddr.addr[1],
                       (unsigned char)macAddr.addr[2], (unsigned char)macAddr.addr[3],
                       (unsigned char)macAddr.addr[4], (unsigned char)macAddr.addr[5]);

        return;
    }

    if(fpGetIntIfNumFromHostIfName(ifname, &intIfNum) !=  SUCCESS)
    {
        SWSS_LOG_NOTICE("Unable to get the internal interface number for %s.", ifname);
        return;
    }

    if (nimGetIntfAddress(intIfNum,  0, intfMac) !=  SUCCESS)
    {
        SWSS_LOG_NOTICE("Unable to fetch interface MAC for %s", ifname);
        return;
    }

    if (0 == memcmp(macAddr.addr, intfMac, ETHER_ADDR_LEN))
    {
//        SWSS_LOG_NOTICE("own mac.. returning");
        return;
    }

    authmgrUnauthAddrCallBack(intIfNum, macAddr, ( ushort16)vlan_id);
    return;
}

void PacMgr::createPacSocket(char *if_name, bool isCreate)
{
    std::string ifname(if_name);

    if (isCreate == true)
    {
        auto it = g_pacSocketMap.begin();
        while(it != g_pacSocketMap.end())
        {
            auto interface_name = it->second;

            if (ifname == interface_name)
            {
                SWSS_LOG_DEBUG("Already exists. Found the entry in socket map for interface %s", PAC_GET_STD_IF_FORMAT(if_name));
                return;
            }
            it++;
        }

        pacSocket *p = NULL;
        p = new pacSocket(ifname);
        if (p == NULL)
        {
            SWSS_LOG_ERROR("createPacSocket failed for %s", PAC_GET_STD_IF_FORMAT(if_name));
            return;
        }
        g_pacSocketMap[p] = ifname;
        s.addSelectable(p);
    }
    else
    {
        pacSocket *p = NULL;
        auto it = g_pacSocketMap.begin();
        while(it != g_pacSocketMap.end())
        {
            auto interface_name = it->second;

            if (ifname == interface_name)
            {
                SWSS_LOG_NOTICE("Found the entry in socket map for interface %s", PAC_GET_STD_IF_FORMAT(if_name));
                p = (pacSocket *)it->first;
                break;
            }
            it++;
        }

        if(p)
        {
            s.removeSelectable(p);
            g_pacSocketMap.erase(p);
            delete (p);
        }
    }
    SWSS_LOG_NOTICE("Create/Delete (%d) pacSocket for ifname %s", isCreate, PAC_GET_STD_IF_FORMAT(if_name));
    return;
}

int PacMgr::pacQueuePost(char *if_name, bool isCreate)
{
    return pacqueue.post(if_name, isCreate);
}

extern "C" {
    void pacCreateDeleteSocket(char *if_name, bool isCreate)
    {
        try
        {
            pacmgr.pacQueuePost(if_name, isCreate);
        }
        catch (const exception &e)
        {
            SWSS_LOG_NOTICE("Create/Delete (%d) pacSocket for ifname %s caught exception %s", 
                             isCreate, PAC_GET_STD_IF_FORMAT(if_name), e.what());
        }
    }
}

pacSocket::pacSocket(string ifname, int priority) :
    Selectable(priority), m_pac_socket(0)
{
    int val = 0;
    int ret = 0;
    struct sockaddr_ll ll_my;

    // open a raw socket
    m_pac_socket = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (m_pac_socket	 < 0)
    {
        SWSS_LOG_ERROR("socket() API returned error %d", m_pac_socket);
        throw system_error(errno, system_category());
    }
    else
    {
        SWSS_LOG_DEBUG("Created socket %d", m_pac_socket);
    }

    //get vlan info from socket
    val = 1;
    if (-1 == setsockopt (m_pac_socket, SOL_PACKET, PACKET_AUXDATA, &val,
                sizeof (val)))
    {
        SWSS_LOG_NOTICE("error socket %d", m_pac_socket);

    }

    memset(&ll_my, 0, sizeof(ll_my));
    ll_my.sll_family = PF_PACKET;
    ll_my.sll_ifindex = if_nametoindex(ifname.c_str());
    ret = bind(m_pac_socket, (struct sockaddr *) &ll_my, sizeof(ll_my));
    if (ret < 0)
    {
        SWSS_LOG_NOTICE("Binding the socket to the interface %s failed", PAC_GET_STD_IF_FORMAT(ifname));
    }

    SWSS_LOG_NOTICE("Created a socket for the interface %s(%d)",
                    PAC_GET_STD_IF_FORMAT(ifname), ll_my.sll_ifindex);
}


pacSocket::~pacSocket()
{
    SWSS_LOG_DEBUG("Delete socket %d", m_pac_socket);


    if (m_pac_socket)
    {
        SWSS_LOG_NOTICE("Closing socket %d", m_pac_socket);
        close(m_pac_socket);
    }
}

int pacSocket::getFd()
{
    return m_pac_socket;
}

uint64_t pacSocket::readData()
{
    SWSS_LOG_DEBUG("%s %d: Read data for the PAC packet", __FUNCTION__, __LINE__);
    pacmgr.processPacket(m_pac_socket);
    return 0;
}

pacQueue::pacQueue(int priority) :
    Selectable(priority)
{
    int status;
    status = pipe(m_pipefd);
    if (status < 0)
    {
        SWSS_LOG_ERROR("pipe() API returned error %d", status);
        throw system_error(errno, system_category());
    }
    else
    {
        // set read fd as non-blocking
        int flags = fcntl(m_pipefd[0], F_GETFL, 0);
        fcntl(m_pipefd[0], F_SETFL, flags | O_NONBLOCK);
        SWSS_LOG_NOTICE("Created pacmgr msg queue with fd [%d, %d] ", m_pipefd[0], m_pipefd[1]);
    }
}


pacQueue::~pacQueue()
{
    for (int i = 0; i < 2; i++)
    {
        if (m_pipefd[i])
        {
            close(m_pipefd[i]);
            SWSS_LOG_NOTICE("Closed pacmgr pipe[%d] fd %d", i, m_pipefd[i]);
        }
    }
}

int pacQueue::getFd()
{
    return m_pipefd[0];
}

uint64_t pacQueue::readData()
{
    return 0;
}

int pacQueue::post(char *if_name, bool isCreate)
{
    struct pacQueueMsg msg;
    int status;

    strncpy(msg.ifname, if_name, sizeof(msg.ifname)-1);
    msg.oper = isCreate;
    status = write(m_pipefd[1], &msg, sizeof(msg));
    if (status < 0)
    {
        SWSS_LOG_ERROR("write() API returned error %d", status);
        return status;
    }

    SWSS_LOG_NOTICE("posted to pacmgr msg queue with interface(%s) and oper(%d) ", PAC_GET_STD_IF_FORMAT(if_name), isCreate);
    return 0;
}

int pacQueue::readQueue(void)
{
    struct pacQueueMsg msg;
    int len;
    
    memset(&msg, 0, sizeof(msg));

    while ((len = read(m_pipefd[0], (void*) &msg, (size_t) sizeof(msg))) > 0)
    {
        if (len == sizeof(msg))
        {
            SWSS_LOG_NOTICE("Read pacmgr msg queue and got interface(%s) and oper(%d) ", PAC_GET_STD_IF_FORMAT(msg.ifname), (int) msg.oper);
            pacmgr.createPacSocket(msg.ifname, msg.oper);
        }
        else
        {
            SWSS_LOG_ERROR("readQueue: read() API read only %d instead of %d", len, (int) sizeof(msg));
            return -1;
        }
        memset(&msg, 0, sizeof(msg));
    }
    return 0;
}
