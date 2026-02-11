#ifndef SYSLOG_PARSER_H
#define SYSLOG_PARSER_H

extern "C"
{
    #include <lua5.1/lua.h>
    #include <lua5.1/lualib.h>
    #include <lua5.1/lauxlib.h>
}

#include <vector>
#include <string>
#include <regex>
#include <nlohmann/json.hpp>
#include <swss/events.h>
#include "timestamp_formatter.h"

using namespace std;
using json = nlohmann::json;

struct EventParam {
    string paramName;
    string luaCode;
};

struct RegexStruct {
    regex regexExpression;
    vector<EventParam> params;
    string tag;
};

/**
 * Syslog Parser is responsible for parsing log messages fed by rsyslog.d and returns
 * matched result to rsyslog_plugin to use with events publish API
 *
 */

class SyslogParser {
public:
    unique_ptr<TimestampFormatter> m_timestampFormatter;
    vector<RegexStruct> m_regexList;
    bool parseMessage(string message, string& tag, event_params_t& paramDict, lua_State* luaState);
    SyslogParser();
};

#endif
