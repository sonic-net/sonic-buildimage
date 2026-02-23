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

#include <iostream>
#include <set>
#include <sstream>
#include <iomanip>
#include "nimsync.h"

#include <string.h>
#include <linux/if.h>
#include <swss/logger.h>
#include <swss/dbconnector.h>
#include <swss/producerstatetable.h>
#include <netlink/route/link.h>
#include <swss/netmsg.h>

extern "C" {
#include "pacinfra_common.h"
#include "resources.h"
#include "nim_events.h"
#include "nimapi.h"
}

using namespace std;
using namespace swss;

#define TEAM_DRV_NAME   "team"

static int hex2num(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

/*
 * Converts mac address in string format to array of 6 bytes
 * macstr  - mac address in string format (example: "11:22:33:44:55:66")
 * macarry - mac address in an char array
*/
int macstr_to_mac(const char *macstr, unsigned char *addr)
{
    for (int i = 0; i < 6; i++)
    {
        int a, b;

        a = hex2num(*macstr++);
        if (a < 0)
            return -1;
        b = hex2num(*macstr++);
        if (b < 0)
            return -1;
        *addr++ = (a << 4) | b;
        if (i < 5 && *macstr++ != ':')
            return -1;
    }
    return 0;
}

NimPort::NimPort(){
    m_adminState = 0;
    m_operState = 0;
}

NimPort::NimPort(const int &admin, const int &oper) :
m_adminState(admin),
m_operState(oper){ }




NimSync::NimSync() {}

NimPort& NimSync::getPort(const string & alias)
{
    return m_portList[alias]; 
}

void NimSync::setPort(const string & alias, const NimPort& port)
{
    m_portList[alias] = port; 
}

void NimSync::delPort(const string & alias)
{
    m_portList.erase(alias);
}

void NimSync::onMsg(int nlmsg_type, struct nl_object *obj)
{
    SWSS_LOG_ENTER();

    if ((nlmsg_type != RTM_NEWLINK) && (nlmsg_type != RTM_DELLINK))
    {
        return;
    }

    struct rtnl_link *link = (struct rtnl_link *)obj;
    string key = rtnl_link_get_name(link);

    if (key.compare(0, INTFS_PREFIX.length(), INTFS_PREFIX) &&
        key.compare(0, LAG_PREFIX.length(), LAG_PREFIX) &&
        key.compare(0, MGMT_PREFIX.length(), MGMT_PREFIX))
    {
        return;
    }

    unsigned int flags = rtnl_link_get_flags(link);
    bool admin = flags & IFF_UP;
    bool oper = flags & IFF_LOWER_UP;

    char addrStr[MAX_ADDR_SIZE+1] = {0};
    nl_addr2str(rtnl_link_get_addr(link), addrStr, MAX_ADDR_SIZE);

    unsigned int ifindex = rtnl_link_get_ifindex(link);
    int master = rtnl_link_get_master(link);
    char *type = rtnl_link_get_type(link);

    if (type)
    {
        SWSS_LOG_NOTICE("nlmsg type:%d key:%s admin:%d oper:%d addr:%s ifindex:%d master:%d type:%s",
                       nlmsg_type, getStdIfFormat(key).c_str(), admin, oper, addrStr, ifindex, master, type);
    }
    else
    {
        SWSS_LOG_NOTICE("nlmsg type:%d key:%s admin:%d oper:%d addr:%s ifindex:%d master:%d",
                       nlmsg_type, getStdIfFormat(key).c_str(), admin, oper, addrStr, ifindex, master);
    }

    if (!key.compare(0, MGMT_PREFIX.length(), MGMT_PREFIX))
    {
        return;
    }

    /* teamd instances are dealt in teamsyncd */
    if (type && !strcmp(type, TEAM_DRV_NAME))
    {
        return;
    }

    if(key.find(INTFS_PREFIX) == string::npos)
    {
        SWSS_LOG_NOTICE("Skipping non Ethernet interface %s", key.c_str());
        return;
    }

    uint32 intIfNum;
    NIM_HANDLE_t handle;

    nimUSP_t usp;
    usp.unit = 1;
    usp.slot = 0;

    try
    {
        if(key.length() > 8)
        {
            usp.port = std::stoi(key.substr(8)) + 1; // FP ports start from 1 corresponds to Ethernet0 on SONiC
        }
        else
        {
            usp.port = std::stoi(key.substr(3)); // FP ports start from 1 corresponds to E1_1 on SONiC
            // SONiC DB and netlink has different formats for the interface. Convert it into Eth1/1 format
            string tmp = "Eth" + key.substr(1,1) + '/' + key.substr(3);
            key = tmp;
        }
    }
    catch (...)
    {
        SWSS_LOG_NOTICE("Skipping invalid interface %s", key.c_str());
        return;
    }

    NIM_EVENT_NOTIFY_INFO_t eventInfo;
    eventInfo.component =  CARDMGR_COMPONENT_ID;
    eventInfo.pCbFunc = NULL;
     enetMacAddr_t macAddr;
    if (macstr_to_mac(addrStr, macAddr.addr) != 0)
    {
        SWSS_LOG_NOTICE("Invalid MAC address format %s", addrStr);
    }

    /* New interface handling */
    if (m_portList.find(key) == m_portList.end())
    {
        int port = 0;
        SYSAPI_HPC_PORT_DESCRIPTOR_t portData =
        {
           IANA_GIGABIT_ETHERNET,
           PORTCTRL_PORTSPEED_FULL_10GSX,
           PHY_CAP_PORTSPEED_ALL,
           /* MTRJ,*/
           PORT_FEC_DISABLE,
           CAP_FEC_NONE
        };

        NimPort p(0, 0);

        SWSS_LOG_NOTICE("New interface %s", key.c_str());
        setPort(key, p);

        if(key.length() > 8)
        {
            port = std::stoi(key.substr(8)) + 1; //FP ports starts from 1 whereas SONiC has Ethernet0
        }
        else
        {
            port = std::stoi(key.substr(5)); // Eth1/1 format after conversion
        }

        /* Generate Create followed by Attach event*/
        if(nimCmgrNewIntfChangeCallback(1, 0, port, 0,  CREATE, &portData, &macAddr) !=  SUCCESS)
        {
            SWSS_LOG_NOTICE("Failed to add interface %s", key.c_str());
            return;
        }

        /* Get internal interface number from Nim */
        if(nimGetIntIfNumFromUSP(&usp, &intIfNum) !=  SUCCESS)
        {
            SWSS_LOG_NOTICE("Failed to get intIfNum for %s", key.c_str());
        }

        if(nimSetIntfifAlias(intIfNum, ( uchar8 *) key.c_str()) !=  SUCCESS)
        {
            SWSS_LOG_NOTICE("Failed to set alias %s for intIfNum(%d)", key.c_str(), intIfNum);
        }

        /* Generate Attach event*/
        eventInfo.event =  ATTACH;
        eventInfo.intIfNum = intIfNum;
        if (nimEventIntfNotify(eventInfo,&handle) !=  SUCCESS)
        {
            SWSS_LOG_NOTICE("Failed to generate Attach %s event ", key.c_str());
        }
    }

    if(nimGetIntIfNumFromUSP(&usp, &intIfNum) !=  SUCCESS)
    {
        SWSS_LOG_NOTICE("Failed to get intIfNum for %s", key.c_str());
        return;
    }

    /* Interface delete handling */
    if (nlmsg_type == RTM_DELLINK)
    {
        if (m_portList.find(key) == m_portList.end())
        {
            SWSS_LOG_NOTICE("Unknown interface %s for Delete event ", key.c_str());
            return;
        }

        /* Generate Detach followed by Delete */
        eventInfo.event =  DETACH;
        eventInfo.intIfNum = intIfNum;
        if (nimEventIntfNotify(eventInfo,&handle) !=  SUCCESS)
        {
            SWSS_LOG_NOTICE("Failed to generate Detach %s event ", key.c_str());
        }
        else if (eventInfo.event =  DELETE, nimEventIntfNotify(eventInfo,&handle) !=  SUCCESS)
        {
            SWSS_LOG_NOTICE("Failed to generate Delete %s event ", key.c_str());
        }
        else
        {
            SWSS_LOG_NOTICE("Delete %s event", key.c_str());
            delPort(key);
        }
        return;
    }

    /* Set the admin state first*/
    if (admin != m_portList[key].m_adminState)
    {
        m_portList[key].m_adminState = admin;
        nimSetIntfAdminState(intIfNum, admin? ENABLE: DISABLE);
    }

   /* followed by the oper state */
   if ( oper != m_portList[key].m_operState)
    {
        m_portList[key].m_operState = oper;
        nimDtlIntfChangeCallback(&usp, oper? UP: DOWN, NULL);
    }
}

string NimSync::getStdIfFormat(string key)
{
  if((key.find("E") == string::npos) || (key.length() > 8))
  {
    return key;
  }
  string key1("");
  key1 = "Eth" + key.substr(1,1) + '/' + key.substr(3);
  return key1;
}

