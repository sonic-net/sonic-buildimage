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
#include <unistd.h>
#include <iostream>
#include <swss/subscriberstatetable.h>
#include <swss/select.h>
#include "datatypes.h"
#include "osapi.h"
#include "pacmgr.h"
#include "auth_mgr_include.h"
#include "fpinfra.h"
#include "pacoper_common.h"

extern "C" {
extern void pacCfgIntfClientCleanup(void);
extern void pacCfgVlanCleanup(void);
}

#define SELECT_TIMEOUT 10000
swss::DBConnector stateDb("STATE_DB", 0);
swss::DBConnector configDb("CONFIG_DB", 0);
swss::DBConnector appDb("APPL_DB", 0);

PacMgr pacmgr(&configDb, &stateDb, &appDb);
swss::Select s;
pacSocket_map_t g_pacSocketMap;


int main(int argc, char *argv[])
{
    fpinfraInit();

    if (authmgrInit () !=  SUCCESS)
    {
      SWSS_LOG_ERROR("authmgr initialization failed");
      return -1;
    }

    if (osapiWaitForTaskInit ( AUTHMGR_DB_TASK_SYNC,  WAIT_FOREVER) !=
                               SUCCESS)
    {
      return -1;
    }

    try
    {
        SWSS_LOG_NOTICE("-----Starting PacMgr-----");

        pacCfgIntfClientCleanup();
        PacOperTblCleanup();
        //register for the table events
        s.addSelectables(pacmgr.getSelectables());

        //wait for the events and process them
        while (true)
        {
            swss::Selectable *sel = NULL;
            s.select(&sel);

            if (g_pacSocketMap.find(sel) != g_pacSocketMap.end())
            {
                continue;
            }

            try
            {
              //Pass on the processing to the Pac Manager
              pacmgr.processDbEvent(sel);
            }
            catch (const exception &e)
            {
                SWSS_LOG_ERROR("Got exception from processDbEvent: %s", e.what());
            }
        }
    }
    catch (const exception &e)
    {
        SWSS_LOG_ERROR("Runtime error: %s", e.what());
    }
    return -1;
}
