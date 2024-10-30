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

#ifndef _PAC_AUTHMGR_CFG_H
#define _PAC_AUTHMGR_CFG_H

#include <string.h>
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

using namespace std;

#define STATE_DB_SEPARATOR "|"
#define CONFIG_DB_SEPARATOR "|"

namespace swss {

    class PacCfg {
        public:
            PacCfg(DBConnector *appDb, DBConnector *cfgDb, DBConnector *stateDb);
            ~PacCfg();

            /* Update learning mode of a port. */
            bool intfLearningSet(std::string port, std::string learning);

            /* Get learning mode of a port. */
            bool intfLearningGet(std::string port, std::string *learning);

            /* Acquire/Release port. */
            bool intfAcquireSet(std::string port, bool acquire);

            /* Block a client's traffic. */
            bool intfClientBlock(std::string port, MacAddress mac, int vlan);

            /* Add a static MAC entry. */
            bool intfStaticMacAdd(std::string port, MacAddress mac, int vlan);

            /* Add a static MAC entry. */
            bool intfStaticMacRemove(std::string port, MacAddress mac, int vlan);

            /* Clean up all static MAC entries. */
            void intfStaticMacCleanup(void);

            /* Flush FDB entries on a port. */
            bool intfFdbFlush(std::string port);

            /* Add MAC-VLAN translation config. */
            bool intfMacVlanTranslationAdd(std::string port, MacAddress mac, int vlan);

            /* Remove MAC-VLAN translation config. */
            bool intfMacVlanTranslationRemove(std::string port, MacAddress mac, int vlan);

            /* Send notification to FDB mgr. */
            bool sendFdbNotification(std::string op, std::string port);

        private:
        /* Tables for writing config */
        Table m_cfgFdbTable;
        Table m_stateOperFdbTable;
        Table m_stateOperPortTable;

        std::shared_ptr<swss::NotificationProducer> m_flushFdb;
        std::shared_ptr<swss::NotificationProducer> m_fdbCfgNotificationProducer;

    };
}

#endif /* _PAC_AUTHMGR_CFG_H */
