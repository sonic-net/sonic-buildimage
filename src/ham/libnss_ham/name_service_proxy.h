#ifndef __NAME_SERVICE_PROXY_H__
#define __NAME_SERVICE_PROXY_H__

#include <dbus-c++/dbus.h>          /* DBus */
#include "../shared/org.SONiC.HostAccountManagement.dbus-proxy.h"
#include "../shared/dbus-address.h" /* DBUS_BUS_NAME_BASE, DBUS_OBJ_PATH_BASE */

class name_service_proxy_c : public ham::name_service_proxy,
                             public DBus::IntrospectableProxy,
                             public DBus::ObjectProxy
{
public:
    name_service_proxy_c(DBus::Connection &conn, const char * dbus_bus_name_p, const char * dbus_obj_name_p) :
    DBus::ObjectProxy(conn, dbus_obj_name_p, dbus_bus_name_p)
    {
    }
};


#endif // __NAME_SERVICE_PROXY_H__
