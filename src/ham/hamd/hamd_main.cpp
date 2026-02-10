// Host Account Management
#include <glib.h>                       // g_main_loop_new(), g_main_context_default(), g_main_loop_run(), g_main_loop_unref(), g_main_loop_quit(), gboolean, etc...
#include <glib-unix.h>                  // g_unix_signal_add()
#include <dbus-c++/glib-integration.h>  // DBus::Glib::BusDispatcher
#include <stdlib.h>                     // EXIT_SUCCESS

#include "hamd.h"                       // hamd_c
#include "hamd_config.h"                // hamd_config_c
#include "hamd_redis.h"                 // redis_c
#include "../shared/utils.h"            // LOG_CONDITIONAL()

/**
 * @brief This callback will be invoked when this process receives SIGINT
 *        or SIGTERM.
 *
 * @param data Pointer to the GMainLoop object
 *
 * @return G_SOURCE_REMOVE to indicate that this event is no longer needed
 *         and should be removed from the main loop.
 */
static gboolean terminationSignalCallback(gpointer data)
{
    GMainLoop * loop_p = static_cast<GMainLoop *>(data);
    g_main_loop_quit(loop_p);
    return G_SOURCE_REMOVE; // No need to keep this event source since we're about to exit
}

/**
 * @brief This callback will be invoked when this process receives SIGHUP,
 *        which is usually triggered by "systemctl reload". This signal is
 *        used to tell the process to reload its configuration file and
 *        reconfigure itself without doing a complete shutdown-restart
 *        cycle.
 *
 * @param data Pointer to the hamd_c object
 *
 * @return G_SOURCE_CONTINUE to indicate that we want the main loop to keep
 *         processing this event.
 */
static gboolean reloadConfigCallback(gpointer data)
{
    hamd_c * hamd_p = static_cast<hamd_c *>(data);
    hamd_p->reload();
    return G_SOURCE_CONTINUE; // Keep this event source.
}

/**
 * @brief Program entry point
 *
 * @param argc Argument count (i.e. argv array size)
 * @param argv Array of argument strings passed to the program.
 *
 * @return int
 */
int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, 0); // Set stdout buffering to unbuffered

    //putenv("DBUSXX_VERBOSE=1");

    hamd_config_c  config(argc, argv);

    LOG_CONDITIONAL(config.tron_m, LOG_DEBUG, "Creating a GMainLoop");
    GMainContext * main_ctx_p = g_main_context_default();
    GMainLoop    * loop_p     = g_main_loop_new(main_ctx_p, FALSE);

    // Set up a signal handler for handling SIGINT and SIGTERM.
    g_unix_signal_add(SIGINT,  terminationSignalCallback, loop_p); // CTRL-C
    g_unix_signal_add(SIGTERM, terminationSignalCallback, loop_p); // systemctl stop

    // REDIS setup
    LOG_CONDITIONAL(config.tron_m, LOG_DEBUG, "Creating REDIS infra");
    redis_c  redis(config, main_ctx_p);

    // DBus setup
    LOG_CONDITIONAL(config.tron_m, LOG_DEBUG, "Initializing the loop's dispatcher");
    DBus::Glib::BusDispatcher   dispatcher;
    DBus::default_dispatcher = &dispatcher; // DBus::default_dispatcher must be initialized before DBus::Connection.
    dispatcher.attach(main_ctx_p);

    LOG_CONDITIONAL(config.tron_m, LOG_DEBUG, "Requesting System DBus connection \"" DBUS_BUS_NAME_BASE "\"");
    DBus::Connection  dbus_conn(DBus::Connection::SystemBus());
    dbus_conn.request_name(DBUS_BUS_NAME_BASE);

    // HAMD setup
    hamd_c  hamd(config, dbus_conn, redis);
    g_unix_signal_add(SIGHUP, reloadConfigCallback, &hamd); // callback for "systemctl reload hamd"

    // Main loop actvation
    LOG_CONDITIONAL(config.tron_m, LOG_DEBUG, "Entering main loop");
    g_main_loop_run(loop_p);

    LOG_CONDITIONAL(config.tron_m, LOG_DEBUG, "Cleaning up and exiting");

    hamd.cleanup();
    redis.cleanup();

    g_main_loop_unref(loop_p);

    LOG_CONDITIONAL(config.tron_m, LOG_DEBUG, "Exiting daemon.");

    fflush(stdout);

    exit(EXIT_SUCCESS);
}

