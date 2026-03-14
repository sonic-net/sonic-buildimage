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
#ifndef FPNIM_H
#define FPNIM_H
#include <vector>
#include <swss/dbconnector.h>
#include <swss/schema.h>
#include <swss/table.h>
#include <swss/macaddress.h>
#include <swss/notificationproducer.h>
#include <swss/producerstatetable.h>
#include <swss/subscriberstatetable.h>
#include <swss/table.h>
#include <swss/select.h>
#include <swss/timestamp.h>
#include <swss/redisapi.h>
#include "nimsync.h"

using namespace swss;
using namespace std;

/* FpNim is singleton class and only one instance of it runs on a process*/
class FpNim {
public:
    /* Delete default constructor. */
    FpNim() = delete;

    /* Delete copy constructor. */
    FpNim(FpNim &other) = delete;

    /* delete assingment operator.  */
    void operator=(const FpNim &) = delete;

    /**
     * Static method controls the access to the singleton instance. 
     * On the first run, it creates a singleton object and places it
     * into the static field. On subsequent runs, it returns the client existing
     * object stored in the static field.
     */
    static FpNim *getInstance(DBConnector *applDb, DBConnector *cfgDb);

    /* This overloaded getInstance returns the current instance pointer.
     * It can return NULL when called before fpinfraInt() call
    */
    static FpNim *getInstance();

    bool processDbEvent(Selectable *source);
    void init();
    bool isPortInitDone();
    void createAllPorts(NimSync & sync);
    void nimStartupInvoke(void);
    std::string getSystemMac();
    int getSystemMac(unsigned char *addr);
private:
    static FpNim *instance;
    DBConnector *applDb;
    DBConnector *cfgDb;

    /* tables this component listens to */
    Table m_portTable;
    Table m_devMetaTbl;

    /* Private constructor */
    FpNim(DBConnector *applDb, DBConnector *cfgDb);

    /* DB Event handler functions */
    bool processPortTblEvent(Selectable *tbl);
};

#endif /* FPNIM_H */
