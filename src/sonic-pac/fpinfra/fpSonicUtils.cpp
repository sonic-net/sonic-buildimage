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
#include <cstring>
#include <string>

using namespace std;

extern "C" {

#include "pacinfra_common.h"
#include "fpSonicUtils.h"

}

const string INTFS_PREFIX = "E";

using namespace std;

int fpGetIntIfNumFromHostIfName(const char *ifName, uint32 *outIntfNum)

{
    size_t pos;
    std::string::size_type sz;
    std::string name = (char*)ifName;

    if(name.find(INTFS_PREFIX) == string::npos)
    {
      return -1;
    }

    if((pos = name.find("/")) == string::npos)
    {
        pos = name.find("_");
    }

    if(pos == string::npos)
    {
      pos = 7; // assume Ethernetx format
      *outIntfNum = (std::stoi(name.substr(pos+1), &sz)) + 1;
    }
    else
    {
      *outIntfNum = std::stoi(name.substr(pos+1), &sz);
    }

    return 0;
}


int fpGetHostIntfName(uint32 physPort,  uchar8 *ifName)
{
        string tmp = "Ethernet" + to_string(physPort-1);
        strcpy((char*)ifName, tmp.c_str());
    return 0;
}




