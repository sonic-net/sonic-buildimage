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
#include <swss/subscriberstatetable.h>
#include <swss/select.h>
#include <iostream>
#include "datatypes.h"
#include "osapi.h"
#include "mabmgr.h"
#include "mab_include.h"
#include <unistd.h>
#include "fpinfra.h"

int main(int argc, char *argv[])
{
    cout<<"Invoking fpinfraInit" << endl;
    fpinfraInit();

    if (mabInit () !=  SUCCESS)
      cout<<"Fail" << endl;
    else
      cout<<"Success linked" << endl;

    if (osapiWaitForTaskInit ( MAB_DB_TASK_SYNC,  WAIT_FOREVER) !=
                               SUCCESS)
    {
      return -1;
    }

    cout<<"DB_TASK_SYNC Success" << endl;

    /* Set log level MSG_DEBUG to get hostapd logs for debugging purposes
     * Use the below values from wpa_debug.h
     * enum { MSG_EXCESSIVE, MSG_MSGDUMP, MSG_DEBUG, MSG_INFO, MSG_WARNING, MSG_ERROR };
     */
   // mab_radius_server_debug_level_set(2 /*MSG_DEBUG*/);

    try
    {
        SWSS_LOG_NOTICE("-----Starting MabMgr-----");
        sleep(20);
        swss::DBConnector stateDb("STATE_DB", 0);
        swss::DBConnector configDb("CONFIG_DB", 0);
        swss::DBConnector appDb("APPL_DB", 0);

        MabMgr mab(&configDb, &stateDb, &appDb);

#if 0
        // App Marking closest UP status
        Table feat_tbl(&stateDb, STATE_FEATURE_TABLE_NAME);
        std::vector<FieldValueTuple> attrs;
        FieldValueTuple up_ready_status("UP_STATUS", "True");
        FieldValueTuple fail_reason("FAIL_REASON", "");
        char buffer[100];
        std::time_t rawtime;
        struct tm *timeinfo;
        time(&rawtime);
        timeinfo = gmtime(&rawtime);
        strftime(buffer, 100, "%Y-%m-%d %H:%M:%S", timeinfo);
        FieldValueTuple time("TIME", string(buffer));
        attrs.push_back(up_ready_status);
        attrs.push_back(fail_reason);
        attrs.push_back(time);
        feat_tbl.set("mabd", attrs);
        SWSS_LOG_NOTICE("mabd marked its UP Status to True");
#endif
	//register for the table events
        swss::Select s;
        s.addSelectables(mab.getSelectables());

        //wait for the events and process them
        while (true)
        {
            SWSS_LOG_NOTICE("Waiting for MAB Table Events");

            swss::Selectable *sel = NULL;
            s.select(&sel);

            //Pass on the processing to the Mab Manager
            mab.processDbEvent(sel);
        }

    }
    catch (const exception &e)
    {
        SWSS_LOG_ERROR("Runtime error: %s", e.what());
    }
    return -1;
}
