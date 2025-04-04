#ifndef __SAC_PROXY_H__
#define __SAC_PROXY_H__

#include <dbus-c++/dbus.h>           /* DBus */
#include "../shared/dbus-address.h"  /* DBUS_BUS_NAME_BASE, DBUS_OBJ_PATH_BASE */
#include "../shared/org.SONiC.HostAccountManagement.dbus-proxy.h"

class sac_proxy_c : public ham::sac_proxy,
    public DBus::IntrospectableProxy,
    public DBus::ObjectProxy
{
public:
    sac_proxy_c(DBus::Connection &connection, const char *dbus_bus_name_p, const char *dbus_obj_name_p) :
        DBus::ObjectProxy(connection, dbus_obj_name_p, dbus_bus_name_p)
    {
    }
};


#endif // __SAC_PROXY_H__
