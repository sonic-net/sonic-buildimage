// Host Account Management
#include "./usr/include/ham/ham.h"
#include "dbus-proxy.h"
#include "../shared/dbus-address.h" /* DBUS_BUS_NAME_BASE, DBUS_OBJ_PATH_BASE */
#include "../shared/utils.h"        /* split_any() */

#include <syslog.h>

bool ham_useradd(const char * login, const char * roles_p, const char * hashed_pw)
{
    bool success = false;
    try
    {
        accounts_proxy_c  acct(get_dbusconn(), DBUS_BUS_NAME_BASE, DBUS_OBJ_PATH_BASE);

        std::vector< std::string > roles = split_any(roles_p, ", \t");

        ::DBus::Struct<bool/*success*/, std::string/*errmsg*/> ret;

        ret = acct.useradd(login, roles, hashed_pw);
        success = ret._1;
        if (!success)
            syslog(LOG_ERR, "ham_useradd(login=\"%s\") - Error %s\n", login, ret._2.c_str());
    }
    catch (DBus::Error & ex)
    {
        syslog(LOG_CRIT, "ham_useradd(login=\"%s\", roles_p=\"%s\" - Exception %s\n", login, roles_p, ex.what());
    }

    return success;
}


