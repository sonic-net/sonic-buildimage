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
#include <swss/tokenize.h>
#include "pac_authmgrcfg.h"

using namespace std;
using namespace swss;

// PAC SONIC config engine 
PacCfg::PacCfg(DBConnector *db, DBConnector *cfgDb, DBConnector *stateDb) :
    m_cfgFdbTable(cfgDb, CFG_FDB_TABLE_NAME),
    m_stateOperFdbTable(stateDb, STATE_OPER_FDB_TABLE_NAME),
    m_stateOperPortTable(stateDb, STATE_OPER_PORT_TABLE_NAME)
{
    Logger::linkToDbNative("paccfg");
    SWSS_LOG_NOTICE("PAC: config object");

    /* FDB flush notification producer */
    m_flushFdb = std::make_shared<swss::NotificationProducer>(db, "FLUSHFDBREQUEST");
}

PacCfg::~PacCfg()
{

}

// Set learning mode for a port 
bool PacCfg::intfLearningSet(string port, string learning)
{
   string key(port);
   vector<FieldValueTuple> fvVector;
  
   // Configure port learning mode for FDB manager 
   fvVector.emplace_back("learn_mode", learning);
   m_stateOperPortTable.set(key, fvVector);

   return true;    
}

// Get learning mode of a port 
bool PacCfg::intfLearningGet(string port, string *learning)
{
   return true;
}

// Add a static MAC address to FDB 
bool PacCfg::intfStaticMacAdd(string port, MacAddress mac, int vlan)
{
   string key = VLAN_PREFIX + to_string(vlan);
   key += STATE_DB_SEPARATOR;
   key += mac.to_string();

   vector<FieldValueTuple> fvVector;

   fvVector.push_back(FieldValueTuple("port", port));
   fvVector.push_back(FieldValueTuple("type", "static"));
   m_stateOperFdbTable.set(key, fvVector);
   
   return true;    
}

// Remove an added static MAC address. 
bool PacCfg::intfStaticMacRemove(string port, MacAddress mac, int vlan)
{
   string key = VLAN_PREFIX + to_string(vlan);
   key += STATE_DB_SEPARATOR;
   key += mac.to_string();

   m_stateOperFdbTable.del(key);

   return true;    
}

// for now, blindly delete all entries.
// Ideally we need to delete only the entries owned by PAC.
void PacCfg::intfStaticMacCleanup(void)
{
   vector<string> keys;
   m_stateOperFdbTable.getKeys(keys);
   for (const auto key : keys)
   {
      m_stateOperFdbTable.del(key);
   }
}

// Acquire/Release port. 
bool PacCfg::intfAcquireSet(string port, bool acquire)
{
   vector<FieldValueTuple> fvVector;
  
   // Configure port acquire config for port.
   if (acquire == true)
   {
       fvVector.emplace_back("acquired", "true");
   }
   else
   {
       fvVector.emplace_back("acquired", "false");
   }
   m_stateOperPortTable.set(port, fvVector);
 
   return true;
}

// Block a client 
bool PacCfg::intfClientBlock(string port, MacAddress mac, int vlan)
{
   // Add a static MAC entry with discard bits set. 
   string key = VLAN_PREFIX + to_string(vlan);
   key += STATE_DB_SEPARATOR;
   key += mac.to_string();

   vector<FieldValueTuple> fvVector;
   fvVector.push_back(FieldValueTuple("discard", "true"));
   fvVector.push_back(FieldValueTuple("port", port));
   fvVector.push_back(FieldValueTuple("type", "static"));

   m_stateOperFdbTable.set(key, fvVector);
   
   return true;    
}

bool PacCfg::intfFdbFlush(string port)
{
   vector<FieldValueTuple> values;

   SWSS_LOG_DEBUG("send fdb flush by port %s notification ", port.c_str());

   // Send FDB flush notification. 
   m_flushFdb->send("PORT", port, values);

   return true;
}

bool PacCfg::intfMacVlanTranslationAdd(string port, MacAddress mac, int vlan)
{
   // Add MAC-VLAN translation for given MAC-VLAN pair. 
   return true;
}

bool PacCfg::intfMacVlanTranslationRemove(string port, MacAddress mac, int vlan)
{
   // Remove MAC-VLAN translation for given MAC-VLAN pair. 
   return true;
}

bool PacCfg::sendFdbNotification(string op, string port)
{
   vector<string> keys;
   vector<FieldValueTuple> entry;
   const char delimiter = '|';

   /* Retrieve static MAC entries configure on port and 
    * send a notification to add/remove those entries on port.
    */
   m_cfgFdbTable.getKeys(keys);

   for (auto id : keys)
   {
       m_cfgFdbTable.get(id, entry);
       for (auto i : entry)
       {
           if (fvField(i) == "port")
           {
               if (fvValue(i) == port)
               {
                   vector<FieldValueTuple> values;

                   /* Get MAC and VLAN from key */
                   vector <string> tokens = tokenize(id, delimiter, 1);

                   /* tokens[0] is VLAN (Vlan20) and tokens[1] is mac (00:01:02:03:04:05). */
                   values.push_back(FieldValueTuple("mac", tokens[1]));
                   values.push_back(FieldValueTuple("Vlan", tokens[0]));
                   m_fdbCfgNotificationProducer->send(op, port, values);
               } 
           }
       }
   }

   return true;
}
