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

#ifndef _MABMGR_H_
#define _MABMGR_H_

#include <swss/dbconnector.h>
#include <swss/schema.h>
#include <swss/table.h>
#include <swss/macaddress.h>
#include <swss/notificationproducer.h>
#include <swss/subscriberstatetable.h>
#include <swss/producerstatetable.h>
#include <swss/table.h>
#include <swss/select.h>
#include <swss/timestamp.h>

#include "redisapi.h"
#include "auth_mgr_exports.h"
#include "mab_exports.h"

typedef struct radius_server_info_s {
  std::string server_port;
  std::string server_key;
  std::string server_ip;
  std::string server_priority;
  bool        server_update;
  bool        dns_ok;
}radius_server_info_t;

typedef std::map<std::string, radius_server_info_t> radius_server_info_map_t;

typedef struct radius_info_s {
  std::string m_radiusGlobalKey;
  radius_server_info_map_t radius_auth_server_list;
}radius_info_t;

#define MABMGR_MAB_PORT_ENABLE_DEF     DISABLE
#define MABMGR_MAB_PORT_AUTH_TYPE_DEF  AUTHMGR_PORT_MAB_AUTH_TYPE_EAP_MD5

/* MAB port config table param cache Info */
typedef struct mabPortConfigCacheParams_t {
    bool mab_enable;
    AUTHMGR_PORT_MAB_AUTH_TYPE_t  mab_auth_type;
} mabPortConfigCacheParams_t;

/* MAP to store MAB port config table params,
 * Key is "interface-id" (Eg. Ethernet0)
 * Value is "mabPortConfigCacheParams_t"
 */
typedef std::map<std::string, mabPortConfigCacheParams_t> mabPortConfigTableMap;

using namespace swss;
using namespace std;

class MabMgr
{ 
public:
    MabMgr(DBConnector *configDb, DBConnector *stateDb, DBConnector *appDb);
    std::vector<Selectable*> getSelectables();
    bool processDbEvent(Selectable *source);

private:
    //tables this component listens to
    SubscriberStateTable m_confMabPortTbl;
    SubscriberStateTable m_confRadiusServerTable;
    SubscriberStateTable m_confRadiusGlobalTable;

    radius_info_t m_radius_info;
    mabPortConfigTableMap     m_mabPortConfigMap;

    // DB Event handler functions
    bool processMabConfigPortTblEvent(Selectable *tbl);
    bool processRadiusServerTblEvent(Selectable *tbl);
    bool processRadiusGlobalTblEvent(Selectable *tbl);
    bool doMabPortTableSetTask(const KeyOpFieldsValuesTuple & t, uint32 & intIfNum);
    bool doMabPortTableDeleteTask(const KeyOpFieldsValuesTuple & t, uint32 & intIfNum);

    void updateRadiusServer();
    void updateRadiusServerGlobalKey(std::string newKey, std::string oldKey);
    void reloadRadiusServers() ;
};

#endif // _MABMGR_H_
