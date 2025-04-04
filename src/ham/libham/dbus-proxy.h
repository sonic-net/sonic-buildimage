// Host Account Management
#ifndef DBUS_PROXY_H
#define DBUS_PROXY_H

#include <dbus-c++/dbus.h>  // DBus

#include "../shared/org.SONiC.HostAccountManagement.dbus-proxy.h"

class accounts_proxy_c : public ham::accounts_proxy,
                         public DBus::IntrospectableProxy,
                         public DBus::ObjectProxy
{
public:
    accounts_proxy_c(DBus::Connection &connection, const char * dbus_bus_name_p, const char * dbus_obj_name_p) :
    DBus::ObjectProxy(connection, dbus_obj_name_p, dbus_bus_name_p)
    {
    }
};


class sac_proxy_c : public ham::sac_proxy,
                    public DBus::IntrospectableProxy,
                    public DBus::ObjectProxy
{
public:
    sac_proxy_c(DBus::Connection &conn, const char * dbus_bus_name_p, const char * dbus_obj_name_p) :
    DBus::ObjectProxy(conn, dbus_obj_name_p, dbus_bus_name_p)
    {
    }
};


extern DBus::Connection  & get_dbusconn();

#endif // DBUS_PROXY_H
