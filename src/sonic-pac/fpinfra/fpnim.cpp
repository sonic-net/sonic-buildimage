/*
 * Copyright 2024 Broadcom Inc.
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
#include <vector>
#include <iostream>
#include <string>
#include <swss/subscriberstatetable.h>
#include "fpnim.h"
#include "netdispatcher.h"
#include "netlink.h"
#include "nimsync.h"
#include "fpinfra.h"

extern "C" {
#include "datatypes.h"
#include "commdefs.h"
#include "pacinfra_common.h"
#include "log.h"
#include "osapi.h"
#include "resources.h"
#include "nim_cnfgr.h"
#include "sysapi.h"
#include "sysapi_hpc.h"
#include "nim_events.h"
#include "nimapi.h"
#include "osapi_priv.h"
#include "nim_startup.h"
}

using namespace swss;
using namespace std;

extern pthread_key_t osapi_task_key;
extern int osapi_task_key_created;

/* Initialize static member*/
FpNim *FpNim::instance = 0;

FpNim::FpNim(DBConnector *applDb, DBConnector *cfgDb) :
m_portTable(applDb, APP_PORT_TABLE_NAME),
m_devMetaTbl(cfgDb, CFG_DEVICE_METADATA_TABLE_NAME){ }

FpNim* FpNim::getInstance() {
   return instance;
}

FpNim* FpNim::getInstance(DBConnector *applDb, DBConnector * cfgDb) {
   if (!instance) {
   instance = new FpNim(applDb, cfgDb);
   instance->applDb = applDb;
   instance->cfgDb = cfgDb;
   }
   return instance;
}

void FpNim::init(void) {
  if(nimPhaseOneInit() !=  SUCCESS)
    return;

  if(nimPhaseTwoInit() !=  SUCCESS)
    return;

  if(nimPhaseThreeInit() !=  SUCCESS)
    return;

  if(nimPhaseExecInit() !=  SUCCESS)
    return;
}

void FpNim::nimStartupInvoke(void) {

  /* Wait till the component registers with NIM */
  nimStartUpTreeData_t startupData;
  while(nimStartUpFirstGet(&startupData) !=  SUCCESS)
  {
    sleep(1);
  }

  /* Now make startup callback */
  nimStartupCallbackInvoke(NIM_INTERFACE_CREATE_STARTUP);
  nimStartupCallbackInvoke(NIM_INTERFACE_ACTIVATE_STARTUP);
}

void FpNim::createAllPorts(NimSync & sync) {
    int port = 0;
    int unit = 1;
    int slot = 0;
    vector<string> keys;
    m_portTable.getKeys(keys);
    SWSS_LOG_NOTICE("m_portTable->getKeys %zd", keys.size());

    SYSAPI_HPC_PORT_DESCRIPTOR_t portData =
    {
       IANA_GIGABIT_ETHERNET,
       PORTCTRL_PORTSPEED_FULL_10GSX,
       PHY_CAP_PORTSPEED_ALL,
       /* MTRJ,*/
       PORT_FEC_DISABLE,
       CAP_FEC_NONE
    };

     enetMacAddr_t macAddr;
    if (getSystemMac(macAddr.addr) != 0)
    {
        SWSS_LOG_ERROR("Failed to read system Mac");
    }

    for (const auto& alias: keys)
    {
        NimPort p(0, 0);
        
        if(alias.find(INTFS_PREFIX) == string::npos)
          continue;

        SWSS_LOG_NOTICE("Keys %s", alias.c_str());
        sync.setPort(alias, p);

        try
        {
           if(alias.length() > 8)
           {
               port = std::stoi(alias.substr(8)) + 1; //FP ports starts from 1 whereas SONiC has Ethernet0
           }
           else
           {
               port = std::stoi(alias.substr(5)); //FP ports starts from 1 whereas SONiC has Eth1/1
           }
        }
        catch (...)
        {
          SWSS_LOG_NOTICE("Invalid interface %s", alias.c_str());
          continue;
        }

        if(nimCmgrNewIntfChangeCallback(unit, slot, port, 0,  CREATE, &portData, &macAddr) !=  SUCCESS)
        {
          SWSS_LOG_NOTICE("Failed to add interface %s", alias.c_str());
          continue;
        }

        uint32 intIfNum;
        nimUSP_t  usp;
        usp.unit = unit;
        usp.slot = slot;
        usp.port = port;
        /* Set Alias in native (Ethernet0) format for applications to make use of it */
        if(nimGetIntIfNumFromUSP(&usp, &intIfNum) !=  SUCCESS)
        {
          SWSS_LOG_NOTICE("Failed to get IntIfNum for interface %s", alias.c_str());
          continue;
        }
        nimSetIntfifAlias(intIfNum, ( uchar8 *) alias.c_str());

        NIM_EVENT_NOTIFY_INFO_t eventInfo;
        eventInfo.component =  CARDMGR_COMPONENT_ID;
        eventInfo.pCbFunc = NULL;
        NIM_HANDLE_t handle;

        /* Generate Attach event*/
        eventInfo.event =  ATTACH;
        eventInfo.intIfNum = intIfNum;
        if (nimEventIntfNotify(eventInfo,&handle) !=  SUCCESS)
        {
            SWSS_LOG_NOTICE("Failed to generate Attach %s event ", alias.c_str());
        }
    }
}

/* To check the port init is done or not */
bool FpNim::isPortInitDone() {
    bool portInit = 0;
    long cnt = 0;

    while(!portInit) {
        std::vector<FieldValueTuple> tuples;
        portInit = m_portTable.get("PortInitDone", tuples);

        if(portInit)
            break;
        sleep(1);
        cnt++;
    }
    SWSS_LOG_NOTICE("PORT_INIT_DONE : %d %ld", portInit, cnt);
    return portInit;
}

std::string FpNim::getSystemMac() {
    std::string macStr;
    m_devMetaTbl.hget("localhost", "mac", macStr);
    SWSS_LOG_NOTICE("getSystemMac(): %s", macStr.c_str());
    return macStr;
}

int FpNim::getSystemMac(unsigned char *addr) {
    return macstr_to_mac(getSystemMac().c_str(), addr);
}

void pacHandleDumpError(void *cbData)
{
    NetLink *netlink = (NetLink *)cbData;
    SWSS_LOG_NOTICE("Netlink dump failed with NLE_DUMP_INTR, resending dump request");
    netlink->dumpRequest(RTM_GETLINK);
}

void* fpinfraTask(void * nimPtr) {

    FpNim * nim = (FpNim *) nimPtr;
    try {
        nim->isPortInitDone();

        NimSync sync;
        nim->createAllPorts(sync);
        nim->nimStartupInvoke();

        //register for the table events
        NetLink netlink;
        netlink.registerGroup(RTNLGRP_LINK);
        netlink.dumpRequest(RTM_GETLINK);
        cout << "Listen to Netlink messages..." << endl;
        NetDispatcher::getInstance().registerMessageHandler(RTM_NEWLINK, (swss::NetMsg*)&sync);
        NetDispatcher::getInstance().registerMessageHandler(RTM_DELLINK, (swss::NetMsg*)&sync);

        swss::Select s;
        s.addSelectable(&netlink);

        //wait for the events and process them
        while (true)
        {
            SWSS_LOG_NOTICE("Waiting for Netlink Events");
            swss::Selectable *sel = NULL;
            s.select(&sel);
        }
    } catch (const exception &e) {
        SWSS_LOG_ERROR("Run-Time error: %s", e.what());
    }
}

// Glabal variable used to protect against multiple invocation of fpinfraInit()
int fpinfraInitialized;

int fpinfraInit(void) {

    // Only the first call is serviced
    if(fpinfraInitialized)
        return 0;

    fpinfraInitialized = 1;
    //swss::Logger::linkToDbNative("fpInfra");
    SWSS_LOG_NOTICE("-----Initializing fpInfra -----");

    /* Initialize sysapi */
    (void)sysapiSystemInit();

    // connect to the databases
    swss::DBConnector *applDb = new swss::DBConnector("APPL_DB", 0);
    swss::DBConnector *cfgDb = new swss::DBConnector("CONFIG_DB", 0);

    FpNim * nim = FpNim::getInstance(applDb, cfgDb);
    nim->init();

    if (osapiTaskCreate("nimDbThread", (void*) fpinfraTask, -1, nim,
                                      DEFAULT_STACK_SIZE,
                                      DEFAULT_TASK_PRIORITY,
                                      DEFAULT_TASK_SLICE ) == NULL)
    {
        return -1;
    }

    return 0;
}
