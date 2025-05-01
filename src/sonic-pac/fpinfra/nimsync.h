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
#ifndef __NIMSYNC__
#define __NIMSYNC__

#include <string>
#include <map>
#include "dbconnector.h"
#include "netmsg.h"
#include "table.h"

using namespace swss;

const std::string MGMT_PREFIX = "eth";
const std::string INTFS_PREFIX = "E";
const std::string LAG_PREFIX = "PortChannel";


int macstr_to_mac(const char *macstr, unsigned char *addr);

class NimPort {
public:
    NimPort();
    NimPort(const int &admin, const int &oper);
    int m_adminState;
    int m_operState;
};


class NimSync : public NetMsg {
public:
    enum { MAX_ADDR_SIZE = 64 };
    NimSync();
    virtual void onMsg(int nlmsg_type, struct nl_object *obj);
    NimPort & getPort(const std::string & alias);
    void setPort(const std::string & alias, const NimPort & port);
    void delPort(const std::string & alias);
    std::string getStdIfFormat(std::string key);
private:
    std::map<std::string, NimPort> m_portList;
};

#endif
