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
#include <swss/logger.h>
#include <swss/dbconnector.h>
#include <swss/producerstatetable.h>
#include <swss/macaddress.h>
#include "pac_vlancfg.h"

using namespace std;
using namespace swss;

// PAC SONIC config engine 
PacCfgVlan::PacCfgVlan(DBConnector *db, DBConnector *cfgDb, DBConnector *stateDb,
                       DBConnector *asicDb, DBConnector *countersDb) :
    m_appPortTable(db, APP_PORT_TABLE_NAME),
    m_stateOperPortTable(stateDb, STATE_OPER_PORT_TABLE_NAME),
    m_cfgVlanTable(cfgDb, CFG_VLAN_TABLE_NAME),
    m_cfgVlanMemberTable(cfgDb, CFG_VLAN_MEMBER_TABLE_NAME),
    m_stateOperFdbTable(stateDb, STATE_OPER_FDB_TABLE_NAME),
    m_stateOperVlanMemberTable(stateDb, STATE_OPER_VLAN_MEMBER_TABLE_NAME),
    m_vlanStateTable(stateDb, STATE_VLAN_TABLE_NAME),
    m_vlanMemberStateTable(stateDb, STATE_VLAN_MEMBER_TABLE_NAME),
    m_countersPortNameMapTable(countersDb, COUNTERS_PORT_NAME_MAP),
    m_appPacTable(db, APP_PAC_PORT_TABLE_NAME)
{
//    Logger::linkToDbNative("paccfgvlan");
    SWSS_LOG_NOTICE("PAC: VLAN config object");

    m_cfgDb  = cfgDb;
    m_asicDb = asicDb;

    /* Setup notification producer for VLAN notifications. */
    m_vlanCfgNotificationProducer = std::make_shared<swss::NotificationProducer>(stateDb, "VLANCFG");
}

PacCfgVlan::~PacCfgVlan()
{

}

bool PacCfgVlan::vlanMemberAdd(int vlan, string port, string tagging_mode)
{
   string key = VLAN_PREFIX + to_string(vlan) + STATE_DB_SEPARATOR + port;
   vector<FieldValueTuple> fvs;

   fvs.emplace_back("tagging_mode", tagging_mode);
   m_stateOperVlanMemberTable.set(key, fvs);

   return true;
}

bool PacCfgVlan::vlanMemberRemove(string port, int vlan)
{
   string key = VLAN_PREFIX + to_string(vlan) + STATE_DB_SEPARATOR + port;

   // Remove port from VLAN. 
   m_stateOperVlanMemberTable.del(key);

   return true;
}

bool PacCfgVlan::vlanMemberClean(int vlan)
{
   vector<string> keys;
   vector<FieldValueTuple> fvVector;
   fvVector.emplace_back("learn_mode", "drop");
   m_stateOperVlanMemberTable.getKeys(keys);
   for (const auto key : keys)
   {
      unsigned pos = key.find(STATE_DB_SEPARATOR);
      string  vlanStr = key.substr (0, pos);
      string intfStr  = key.substr (pos+1);
      if((VLAN_PREFIX + to_string(vlan)) == vlanStr)
      {
         // Remove port from VLAN after setting PVID back to zero.
         m_stateOperVlanMemberTable.del(key);
         m_stateOperPortTable.hdel(intfStr ,"pvid");
         m_stateOperPortTable.hdel(intfStr ,"acquired");
         m_stateOperPortTable.set(intfStr, fvVector);
      }
   }

   return true;
}

bool PacCfgVlan::portPVIDset(string port, int pvid)
{
   string key(port);
   vector<FieldValueTuple> fvVector;

   fvVector.emplace_back("pvid", to_string(pvid));
  
   m_stateOperPortTable.set(key, fvVector);

   return true;
}

bool PacCfgVlan::portPVIDGet(string port, int *pvid)
{
    string portOid;
    std::unordered_map<string, string>::iterator it;

    if (pvid == NULL)
    {
        return false;
    }

    *pvid = 0;
  
    /* Get the port OID for COUNTERS_DB. */
    if (m_countersPortNameMapTable.hget("", port, portOid) != true)
    {
        return false;
    }

    /* Port PVID from ASIC_DB */
    auto fieldValues = m_asicDb->hgetall("ASIC_STATE:SAI_OBJECT_TYPE_PORT:"+portOid);
    for (it = fieldValues.begin(); it != fieldValues.end(); it++)
    {
        if ((it->first) == "SAI_PORT_ATTR_PORT_VLAN_ID")
        {
            try
            {
                *pvid = stoi(it->second);
            }
            catch (...)
            {
                SWSS_LOG_WARN("Invalid value:%s for SAI_PORT_ATTR_PORT_VLAN_ID", (it->second).c_str());
            }
        }
    }

    return true;
}

bool PacCfgVlan::sendVlanNotification(string op, string port, vector<string> keys)
{
   vector<FieldValueTuple> values;

   for (vector<string>::iterator it = keys.begin();
                                 it != keys.end(); ++it)
   {
       FieldValueTuple tuple("Vlan|tagging_mode", *it);
       values.push_back(tuple);
   }

   m_vlanCfgNotificationProducer->send(op, port, values);
   return true;
}

