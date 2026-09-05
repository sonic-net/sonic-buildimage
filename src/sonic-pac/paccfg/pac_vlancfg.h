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

#ifndef _PAC_VLAN_CFG_H
#define _PAC_VLAN_CFG_H

#include <string>
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

#define STATE_DB_SEPARATOR  "|"
#define CONFIG_DB_SEPARATOR "|"
#define PAC_INTERNAL_VLAN  4095

namespace swss {

    class PacCfgVlan {
        public:
            PacCfgVlan(DBConnector *appDb, DBConnector *cfgDb, DBConnector *stateDb,
                       DBConnector *asicDb, DBConnector *countersDb);
            ~PacCfgVlan();

            /* Add a port to a VLAN. */
            bool vlanMemberAdd(int vlan, std::string port, std::string tagging_mode);

            /* Remove port from VLAN. */
            bool vlanMemberRemove(std::string port, int vlan);

            bool vlanMemberClean(int vlan);

            /* Set port PVID. */
            bool portPVIDset(std::string port, int pvid);

            /* Get port PVID. */
            bool portPVIDGet(std::string port, int *pvid);

            /* Check if port exists */
            bool portCheckValid(std::string port);

            /* Send notification to VLAN mgr for VLAN config. */
            bool sendVlanNotification(std::string op, std::string port, vector<std::string> keys);

        private:
        /* Tables for writing config */
        Table m_appPortTable;
        Table m_stateOperPortTable;
        Table m_cfgVlanTable;
        Table m_cfgVlanMemberTable;
        Table m_stateOperFdbTable;
        Table m_stateOperVlanMemberTable;
        Table m_vlanStateTable;
        Table m_vlanMemberStateTable;
        Table m_countersPortNameMapTable;

        ProducerStateTable m_appPacTable;
        std::shared_ptr<swss::NotificationProducer> m_vlanCfgNotificationProducer;
        std::shared_ptr<swss::NotificationProducer> m_resvVlanNotificationProducer;

        DBConnector *m_cfgDb;
        DBConnector *m_asicDb;

    };
}

#endif /* _PAC_VLAN_CFG_ */
