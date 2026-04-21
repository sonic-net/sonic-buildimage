#include <dbus-c++/dbus.h>      // DBus
#include "dbus-proxy.h" // get_dbusconn() prototype

DBus::Connection  & get_dbusconn()
{
    static DBus::Connection  * conn_p = nullptr;
    if (conn_p == nullptr)
    {
        // DBus::BusDispatcher is a "main loop" construct that
        // handles (i.e. dispatched) DBus messages. This should
        // be defined as a singleton to avoid memory leaks.
        static DBus::BusDispatcher dispatcher;

        // DBus::default_dispatcher must be initialized before DBus::Connection.
        DBus::default_dispatcher = &dispatcher;

        static DBus::Connection conn = DBus::Connection::SystemBus();

        conn_p = &conn;
    }

    return *conn_p;
}
