#ifndef RSYSLOG_PLUGIN_H
#define RSYSLOG_PLUGIN_H

extern "C"
{
    #include <lua5.1/lua.h>
    #include <lua5.1/lualib.h>
    #include <lua5.1/lauxlib.h>
}
#include <string>
#include <memory>
#include <csignal>
#include <atomic>
#include <swss/logger.h>
#include <swss/events.h>
#include "syslog_parser.h"

using namespace std;
using namespace swss;

/**
 * Rsyslog Plugin will utilize an instance of a syslog parser to read syslog messages from rsyslog.d and will continuously read from stdin
 * A plugin instance is created for each container/host.
 *
 */

class RsyslogPlugin {
public:
    static atomic<bool> g_running;
    int onInit();
    bool onMessage(string msg, lua_State* luaState);
    void run();
    RsyslogPlugin(string moduleName, string regexPath);
    static void signalHandler(int signum) {
        if (signum == SIGTERM) {
            SWSS_LOG_INFO("Rsyslog plugin received SIGTERM, shutting down");
	    RsyslogPlugin::g_running = false;
        }
    }
private:
    unique_ptr<SyslogParser> m_parser;
    event_handle_t m_eventHandle;
    string m_regexPath;
    string m_moduleName;
    bool createRegexList();
};

#endif

