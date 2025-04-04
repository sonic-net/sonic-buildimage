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
#include "netdispatcher.h"
#include "netlink.h"
#include "select.h"
#include <iostream>
#include "hostapdmgr.h"
#include <unistd.h>

int main(int argc, char *argv[])
{
  swss::DBConnector configDb("CONFIG_DB", 0);
  swss::DBConnector stateDb("STATE_DB", 0);
  swss::DBConnector appDb("APPL_DB", 0);
  swss::DBConnector log_db(LOGLEVEL_DB, DBConnector::DEFAULT_UNIXSOCKET, 0);
  HostapdMgr hostapd(&configDb, &appDb);

  try
  {
    SWSS_LOG_NOTICE("-----Starting HostapdMgr-----");

    //register for the table events
    swss::Select s;
    s.addSelectables(hostapd.getSelectables());

    //register for the table events
    NetLink netlink;
    netlink.registerGroup(RTNLGRP_LINK);
    netlink.dumpRequest(RTM_GETLINK);

    // kill any stale hostapd
    hostapd.killHostapd();

    // cleanup stale hostapd conf files
    string cmd = "rm -f /etc/hostapd/E*.conf";
    if (system(cmd.c_str()))
    {
      SWSS_LOG_WARN("Could not delete stale conf files.");
    }

    // remove stale hostapd_config.json
    cmd = "rm -f /etc/hostapd/hostapd_config.json";
    if (system(cmd.c_str()))
    {
      SWSS_LOG_WARN("Could not delete stale hostapd_config.json file.");
    }

    // cleanup stale hostapd socket files
    cmd = "rm -f /var/run/hostapd/E*";
    if (system(cmd.c_str()))
    {
      SWSS_LOG_WARN("Could not delete stale hostapd socket files.");
    }


    NetDispatcher::getInstance().registerMessageHandler(RTM_NEWLINK, (swss::NetMsg*)&hostapd);
    NetDispatcher::getInstance().registerMessageHandler(RTM_DELLINK, (swss::NetMsg*)&hostapd);

    s.addSelectable(&netlink);

    //wait for the events and process them
    while (1)
    {
      SWSS_LOG_NOTICE("Waiting for HOSTAPD Table Events");

      swss::Selectable *sel = NULL;
      int ret;

      ret = s.select(&sel);

      if (sel != &netlink)
      {
        //Pass on the processing to the Hostapd Manager
        hostapd.processDbEvent(sel);
      }
    }
  }
  catch (const exception &e)
  {
    SWSS_LOG_ERROR("Runtime error: %s", e.what());
  }

  return -1;
}

