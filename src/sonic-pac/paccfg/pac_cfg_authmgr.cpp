/*
 * Copyright 2019 Broadcom. The term "Broadcom" refers to Broadcom Inc. and/or
 * its subsidiaries.
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

#include "pac_cfg_authmgr.h"
#include "pac_authmgrcfg.h"
#include "datatypes.h"
#include "auth_mgr_exports.h"
#include "auth_mgr_mac_db.h"

using namespace std;
using namespace swss;

DBConnector db("APPL_DB", 0);
DBConnector cfgDb("CONFIG_DB", 0);
DBConnector stateDb("STATE_DB", 0);

PacCfg cfg(&db, &cfgDb, &stateDb);

extern "C" {

    RC_t pacCfgIntfLearningModeSet(char *interface,  AUTHMGR_PORT_LEARNING_t learning)
    {
        string port(interface);
        string learning_mode;
        bool status = false;
        RC_t rc =  FAILURE;

        if ( AUTHMGR_PORT_LEARNING_DISABLE == learning)
        {
            /* Drop all unknown source MAC packets. */
            learning_mode = "drop";

        }
        else if ( AUTHMGR_PORT_LEARNING_CPU == learning)
        {
            /* Trap all unknown source MAC packets to CPU. */
            learning_mode = "cpu_trap";
        }
        else if ( AUTHMGR_PORT_LEARNING_ENABLE == learning)
        {
            /* Enable learning on port. */
            learning_mode = "hardware";
        }

        /* Configure port learning mode */
        status = cfg.intfLearningSet(port, learning_mode);
        if (status != false)
        {
            /* Flush FDB entries on port. */
            status = cfg.intfFdbFlush(port); 
        }

        if (status == true)
        {
            rc =  SUCCESS;
        }

        return rc;
    }

    RC_t pacCfgIntfLearningModeGet(char *interface,  AUTHMGR_PORT_LEARNING_t *learning)
    {
        string port(interface);
        string learning_mode;
        bool status = false;
        RC_t rc =  FAILURE;

        status = cfg.intfLearningGet(port, &learning_mode);

        if (status != false)
        {
            if (learning_mode == "enable")
            {
                *learning =  AUTHMGR_PORT_LEARNING_DISABLE;
            }
            else 
            {
                *learning =  AUTHMGR_PORT_LEARNING_ENABLE;
            }
        }
        return rc;
    }

    RC_t pacCfgIntfAcquireSet(char *interface, bool acquire)
    {
        string port(interface);
        RC_t rc =  SUCCESS;
        
        /* Configure port learning mode to CPU trap */
        if (cfg.intfAcquireSet(port, acquire) != true)
        {
            rc =  FAILURE; 
        }

        return rc;
    }

    bool pacCfgIntfViolationPolicySet(char *interface, bool enable)
    {
        string port(interface);
        string learning_mode("cpu_trap");

        /* Violation policy consists of :
         * 1) Policy to trap unknown source MAC packets to CPU.
         * 2) Policy to trap static MAC move packets to CPU.
         * SONiC implements this via:
         * 1) Set learning mode of port to CPU trap.
         * 2) Use a CoPP system trap for trapping static MAC move packets.
         */

        /* Configure port learning mode to CPU trap */
        return cfg.intfLearningSet(port, learning_mode);
    }

    bool pacCfgIntfClientAdd(char *interface, unsigned char *macaddr, int vlan)
    {
        string port(interface);
        bool status = false;
        MacAddress mac(macaddr);

        if (vlan != 0)
        {
            /* Client authorized on only 1 VLAN. 
             * Delete static MAC entries added on any other VLAN for this MAC. 
             */

            /* Add specified MAC-VLAN pair as static MAC entry. */
            status = cfg.intfStaticMacAdd(port, mac, vlan);

            /* Client timeout logic. */

            /* Configure MAC-VLAN translation for client on port. */
            status = cfg.intfMacVlanTranslationAdd(port, mac, vlan);
        }
        else
        {
            /* Client authorized on all vlans. */
           
            /* Indicate to FDB mgr that this MAC needs to be added 
             * on all VLANs that this port is a member of. 
             */
            
            /* Client timeout logic. */
        }
      
        return status;
    }

    bool pacCfgIntfClientRemove(char *interface, unsigned char *macaddr, int vlan)
    {
        bool status = false;
        string port(interface);
        MacAddress mac(macaddr);

        if (vlan != 0)
        {
            /* MAC authorized on 1 VLAN, remove static FDB entry. */
            status = cfg.intfStaticMacRemove(port, mac, vlan);

            /* Remove MAC-VLAN translation configuration for the client. */
            if (status != true)
            {
                status = cfg.intfMacVlanTranslationRemove(port, mac, vlan);
            }
        }
        else
        {
            /* MAC authorized on all VLANs. Remove all configured static MAC-VLAN entries. */
        }

        return status;
    }

    void pacCfgIntfClientCleanup(void)
    {
       return cfg.intfStaticMacCleanup();
    }

    bool pacCfgIntfClientBlock(char *interface, unsigned char *macaddr, int vlan)
    {
        string port(interface);
        MacAddress mac(macaddr);

        /* Add the static MAC-VLAN pair with source and destination discard bits set. */
        return cfg.intfClientBlock(port, mac, vlan);
    }

    bool pacCfgIntfClientUnblock(char *interface, unsigned char *macaddr, int vlan)
    {
        string port(interface);
        MacAddress mac(macaddr);

        /* Delete the added static MAC-VLAN pair. */
        return cfg.intfStaticMacRemove(port, mac, vlan);
    }

    RC_t pacCfgFdbSendCfgNotification(authMgrFdbCfgType_t type, char *interface)
    {
        RC_t status =  SUCCESS;
        string op = "SET";
        vector<string> keys;
        string port(interface);
        string key;
        
        if (type ==  AUTHMGR_FDB_CFG_REMOVE)
        {
            op = "DEL";
        }

        /* Send a notification to get all MAC entries from 
         * CONFIG_DB and configure on port (set/del).
         */
        if (cfg.sendFdbNotification(op, port) != true)
        {
           status =  FAILURE;         
        }

        return status;
    }
}

