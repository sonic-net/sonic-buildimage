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

#include "pac_cfg_vlan.h"
#include "pac_vlancfg.h"
#include "datatypes.h"
#include "auth_mgr_exports.h"
#include "auth_mgr_vlan_db.h"
#include "exec.h"

using namespace std;
using namespace swss;

extern DBConnector db;
extern DBConnector cfgDb;
extern DBConnector stateDb;
DBConnector asicDb("ASIC_DB", 0);
DBConnector countersDb("COUNTERS_DB", 0);

PacCfgVlan vcfg(&db, &cfgDb, &stateDb, &asicDb, &countersDb);

extern "C" {

    RC_t pacCfgPortPVIDSet(char *interface, int pvid)
    {
        string port(interface);
        RC_t rc =  SUCCESS;

        /* Set port PVID */
        if (vcfg.portPVIDset(port, pvid) != true)
        {
          rc =  FAILURE; 
        }

        return rc;
    }

    RC_t pacCfgPortPVIDGet(char *interface, int *pvid)
    {
        string port(interface);
        RC_t rc =  FAILURE;
        
        if (pvid ==  NULL)
        {
            return  FAILURE;
        }

        if (vcfg.portPVIDGet(port, pvid) != true)
        {
            rc =  FAILURE;
        }

        return rc;
    }

    RC_t pacCfgVlanMemberAdd(int vlan, char *interface, dot1qTaggingMode_t mode)
    {
        string port(interface);
        RC_t rc =  SUCCESS;
        string tagging_mode = "untagged";

        if (mode ==  DOT1Q_MEMBER_TAGGED)
        {
          tagging_mode = "tagged";
        }

        /* Set VLAN membership */
        if (vcfg.vlanMemberAdd(vlan, port, tagging_mode) != true)
        {
          rc =  FAILURE; 
        }

        return rc;
    }

    RC_t pacCfgVlanMemberRemove(int vlan, char *interface)
    {
        string port(interface);
        RC_t rc =  SUCCESS;

        /* Set VLAN membership */
        if (vcfg.vlanMemberRemove(port, vlan) != true)
        {
          rc =  FAILURE; 
        }

        return rc;
    }

    RC_t pacCfgVlanMemberClean(int vlan)
    {
        RC_t rc =  SUCCESS;

        if (vcfg.vlanMemberClean(vlan) != true)
        {
          rc =  FAILURE;
        }

        return rc;
    }

    RC_t pacCfgVlanSendCfgNotification(authMgrVlanPortCfgType_t type,
                                          char *interface, authMgrVlanPortData_t *cfg)
    {
        string op = "SET";
        vector<string> keys;
        string key, mode;
        int i = 0;
        string port(interface);

        if (cfg ==  NULLPTR)
        {
            return  FAILURE;
        }

        if (type ==  AUTHMGR_INTF_CFG_REMOVE)
        {
            op = "DEL";
        }

        /* Iterate over config and create keys to be handled */
        for (i = 1; i <  DOT1Q_MAX_VLAN_ID; i++)
        {
            if ( VLAN_ISMASKBITSET(cfg->vlanMask, i))
            {
                mode = "untagged";
                if ( VLAN_ISMASKBITSET(cfg->tagging, i))
                {
                  mode = "tagged";  
                }
                key = "Vlan" + to_string(i) + STATE_DB_SEPARATOR + mode;
                keys.push_back(key);
            }
        }  
  
        if (vcfg.sendVlanNotification(op, port, keys) != true) 
        {
            return  FAILURE;
        }

        return  SUCCESS;
    }
}

