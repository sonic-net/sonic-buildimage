// System-Assigned Credentials (sac)
#include "./usr/include/ham/ham.h"
#include "dbus-proxy.h"
#include "../shared/dbus-address.h" /* DBUS_BUS_NAME_BASE, DBUS_OBJ_PATH_BASE */
#include "../shared/utils.h"        /* split_any() */

#include <syslog.h>

bool sac_user_confirm(const char * login_p, const char * roles_p, const char * auth_method_p)
{
    bool success = false;
    try
    {
        sac_proxy_c       sac(get_dbusconn(), DBUS_BUS_NAME_BASE, DBUS_OBJ_PATH_BASE);

        std::string  errmsg = sac.user_confirm(login_p, split_any(roles_p, ", \t"), auth_method_p);
        success = errmsg.empty(); // empty error message means success
        if (!success)
            syslog(LOG_ERR, "SAC - user_confirm() - User \"%s\": Error! %s", login_p, errmsg.c_str());
    }
    catch (DBus::Error & ex)
    {
        syslog(LOG_ERR, "SAC - user_confirm() - User \"%s\": DBus Exception %s", login_p, ex.what());
    }

    return success;
}
