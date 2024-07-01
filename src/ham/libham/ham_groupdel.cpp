// Host Account Management
#include "./usr/include/ham/ham.h"
#include "dbus-proxy.h"
#include "../shared/dbus-address.h" /* DBUS_BUS_NAME_BASE, DBUS_OBJ_PATH_BASE */


#include <syslog.h>

bool ham_groupdel(const char * group)
{
    bool success = false;
    try
    {
        accounts_proxy_c  acct(get_dbusconn(), DBUS_BUS_NAME_BASE, DBUS_OBJ_PATH_BASE);

        ::DBus::Struct<bool/*success*/, std::string/*errmsg*/> ret;
        ret = acct.groupdel(group);
        success = ret._1;
        if (!success)
            syslog(LOG_ERR, "ham_groupdel(group=\"%s\") - Error %s\n", group, ret._2.c_str());
    }
    catch (DBus::Error & ex)
    {
        syslog(LOG_CRIT, "ham_groupdel(group=\"%s\") - Exception %s\n", group, ex.what());
    }

    return success;
}

