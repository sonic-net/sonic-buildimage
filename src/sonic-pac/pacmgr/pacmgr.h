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

#ifndef _PACMGR_H_
#define _PACMGR_H_

#include <swss/dbconnector.h>
#include <swss/schema.h>
#include <swss/table.h>
#include <swss/macaddress.h>
#include <swss/notificationproducer.h>
#include <swss/notificationconsumer.h>
#include <swss/subscriberstatetable.h>
#include <swss/producerstatetable.h>
#include <swss/table.h>
#include <swss/select.h>
#include <swss/timestamp.h>

#include "redisapi.h"
#include "auth_mgr_exports.h"

#define STATEDB_KEY_SEPARATOR "|"
#define MAX_PACKET_SIZE       8192

#define INDEX_0 0
#define INDEX_1 1
#define PRIORITY_METHOD_MAX 2
#define AUTHMGR_PORT_CONTROL_MODE_DEF  AUTHMGR_PORT_FORCE_AUTHORIZED
#define AUTHMGR_HOST_CONTROL_MODE_DEF  AUTHMGR_MULTI_HOST_MODE
#define AUTHMGR_REAUTH_ENABLE_DEF FD_AUTHMGR_PORT_REAUTH_ENABLED
#define AUTHMGR_REAUTH_PERIOD_DEF FD_AUTHMGR_PORT_REAUTH_PERIOD
#define AUTHMGR_REAUTH_PERIOD_FROM_SERVER_DEF FD_AUTHMGR_PORT_REAUTH_PERIOD_FROM_SERVER
#define AUTHMGR_MAX_USERS_PER_PORT_DEF FD_AUTHMGR_PORT_MAX_USERS
#define AUTHMGR_MAX_REAUTH_ATTEMPTS_DEF 3 
#define AUTHMGR_PORT_REAUTH_PERIOD_DEF FD_AUTHMGR_PORT_REAUTH_PERIOD
#define AUTHMGR_PORT_PAE_ROLE_DEF  DOT1X_PAE_PORT_NONE_CAPABLE
#define AUTHMGR_PRIORITY_LIST_0_DEF  AUTHMGR_METHOD_8021X 
#define AUTHMGR_PRIORITY_LIST_1_DEF  AUTHMGR_METHOD_MAB
#define AUTHMGR_METHOD_LIST_0_DEF  AUTHMGR_METHOD_8021X 
#define AUTHMGR_METHOD_LIST_1_DEF  AUTHMGR_METHOD_MAB

#define PACMGR_IFNAME_SIZE 60  // NIM_IFNAME_SIZE

using namespace swss;
using namespace std;

typedef struct pac_hostapd_glbl_info_s {
  unsigned int enable_auth;
}pac_hostapd_glbl_info_t;

typedef std::map<Selectable *, std::string> pacSocket_map_t;

/* PAC GLOBAL config table Info */
typedef struct pacGlobalConfigCacheParams_t {
    uint8_t monitor_mode_enable;
    uint8_t dynamic_vlan_creation_enable;
} pacGlobalConfigCacheParams_t;

/* PAC port config table Info */
typedef struct pacPortConfigCacheParams_t {
     AUTHMGR_PORT_CONTROL_t port_control_mode;
     AUTHMGR_HOST_CONTROL_t host_control_mode;
    bool reauth_enable;
    uint32_t reauth_period;
    bool reauth_period_from_server;
    uint8_t max_users_per_port;
    uint8_t max_reauth_attempts;
    uint8_t port_pae_role;
     AUTHMGR_METHOD_t priority_list[PRIORITY_METHOD_MAX];
     AUTHMGR_METHOD_t method_list[PRIORITY_METHOD_MAX];
} pacPortConfigCacheParams_t;

/* To store PAC port configuration,
 * Key is "interface-id" (Eg. Ethernet0)
 * Value is "pacPortConfigCacheParams_t"
 */
typedef std::map<std::string, pacPortConfigCacheParams_t> pacPortConfigTableMap;

/* Pac Queue class to receive notification regarding socket
 * createtion/deletion for unauth client packets
 */
class pacQueue : public Selectable {
public:

    pacQueue(int priority = 0);
    virtual ~pacQueue();

    int getFd() override;
    uint64_t readData() override;
    int post(char *if_name, bool isCreate);
    int readQueue(void);

private:

    int m_pipefd[2];
};

struct pacQueueMsg {
    char ifname[PACMGR_IFNAME_SIZE];
    bool   oper;
};


class PacMgr
{ 
public:
    PacMgr(DBConnector *configDb, DBConnector *stateDb,  DBConnector *appDb);

    std::vector<Selectable *> getSelectables();
    bool processDbEvent(Selectable *source);
    void createPacSocket(char *if_name, bool isCreate);
    void processPacket(int m_pac_socket);
    int  pacQueuePost(char *if_name, bool isCreate);

    /* Placeholder for PAC Global table config params */
    static pacGlobalConfigCacheParams_t pacGlobalConfigTable;

private:

    pac_hostapd_glbl_info_t m_glbl_info;
    pacPortConfigTableMap     m_pacPortConfigMap;

    //tables this component listens to
    SubscriberStateTable m_confPacTbl;
    SubscriberStateTable m_confPacGblTbl;
    SubscriberStateTable m_confPacHostapdGblTbl;
   
    // VLAN tables that we listen to
    SubscriberStateTable m_confVlanTbl;
    SubscriberStateTable m_confVlanMemTbl;

    // VLAN state tables
    SubscriberStateTable m_vlanTbl;
    SubscriberStateTable m_vlanMemTbl;
    NotificationConsumer m_clearNotificationConsumer;
   // NotificationConsumer m_clearHistoryNotificationConsumer;

    // DB Event handler functions
    bool processPacPortConfTblEvent(Selectable *tbl);
    bool processPacGlobalCfgTblEvent(Selectable *tbl);
    bool processVlanTblEvent(Selectable *tbl);
    bool processVlanMemTblEvent(Selectable *tbl);
    bool processConfVlanTblEvent(Selectable *tbl);
    bool processConfVlanMemTblEvent(Selectable *tbl);
    bool processPacAuthSessionsClearNotifyEvent(NotificationConsumer *tbl);
    bool processPacHostapdConfGlobalTblEvent(Selectable *tbl);
    bool doPacGlobalTableSetTask(const KeyOpFieldsValuesTuple & t);
    bool doPacGlobalTableDeleteTask();
    bool doPacPortTableSetTask(const KeyOpFieldsValuesTuple & t, uint32 & intIfNum);
    bool doPacPortTableDeleteTask(const KeyOpFieldsValuesTuple & t, uint32 & intIfNum);

   // pacmgr queue to receive message about unauth address socket create/delete
   pacQueue pacqueue;
   bool processPacMsgQueue(Selectable *tbl);
};



namespace swss {

class pacSocket : public Selectable {
public:

    pacSocket(string ifname, int priority = 0);
    virtual ~pacSocket ();

    int getFd() override;
    uint64_t readData() override;

private:

    int m_pac_socket;
};

}

#endif // _PACMGR_H_
