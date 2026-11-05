/*
 * Copyright 2021 Broadcom Inc.
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
#ifndef PACOPER_H
#define PACOPER_H

#include <vector>
#include <swss/dbconnector.h>
#include <swss/schema.h>
#include <swss/table.h>
#include <swss/macaddress.h>
#include <swss/producerstatetable.h>
#include <swss/table.h>
#include <swss/select.h>
#include <swss/timestamp.h>
#include <swss/redisapi.h>
#include <swss/tokenize.h>

using namespace swss;
using namespace std;

#define AUTHMGR_MAX_HISTENT_PER_INTERFACE   48

class FpDbAdapter {
public:
    FpDbAdapter(DBConnector *stateDb, DBConnector *configDb, DBConnector *appDb);
    Table m_PacGlobalOperTbl;
    Table m_PacPortOperTbl;
    Table m_PacAuthClientOperTbl;

private:
};

string fetch_interface_name(int);

#endif /* PACOPER_H */
