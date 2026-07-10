#include <iostream>
#include <vector>
#include <fstream>
#include <regex>
#include <ctime>
#include <unordered_map>
#include <atomic>
#include "rsyslog_plugin.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::atomic<bool> RsyslogPlugin::g_running;

bool RsyslogPlugin::onMessage(string msg, lua_State* luaState) {
    string tag;
    event_params_t paramDict;
    if(!m_parser->parseMessage(msg, tag, paramDict, luaState)) {
        SWSS_LOG_DEBUG("%s was not able to be parsed into a structured event\n", msg.c_str());
        return false;
    } else {
        int returnCode = event_publish(m_eventHandle, tag, &paramDict);
        if(returnCode != 0) {
            SWSS_LOG_ERROR("rsyslog_plugin was not able to publish event for %s.\n", tag.c_str());
            return false;
        }
        return true;
    }
}

void parseParams(vector<string> params, vector<EventParam>& eventParams) {
    for(long unsigned int i = 0; i < params.size(); i++) {
        if(params[i].empty()) {
            SWSS_LOG_ERROR("Empty param provided in regex file\n");
            continue;
        }
        EventParam ep = EventParam();
        auto delimPos = params[i].find(':');
        if(delimPos == string::npos) { // no lua code
            ep.paramName = params[i];
        } else {
            ep.paramName = params[i].substr(0, delimPos);
            ep.luaCode = params[i].substr(delimPos + 1);
            if(ep.luaCode.empty()) {
                SWSS_LOG_ERROR("Lua code missing after :\n");
            }
        }
        eventParams.push_back(ep);
    }
}

bool RsyslogPlugin::createRegexList() {
    fstream regexFile;
    json jsonList = json::array();
    regexFile.open(m_regexPath, ios::in);
    if (!regexFile) {
        SWSS_LOG_ERROR("No such path exists: %s for source %s\n", m_regexPath.c_str(), m_moduleName.c_str());
        return false;
    }
    try {
        regexFile >> jsonList;
    } catch (nlohmann::detail::parse_error& iaException) {
        SWSS_LOG_ERROR("Invalid JSON file: %s, throws exception: %s\n", m_regexPath.c_str(), iaException.what());
        return false;
    }

    string regexString;
    string timestampRegex = "^([a-zA-Z]{3})?\\s*([0-9]{1,2})?\\s*([0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{0,6})?\\s*";
    regex expression;
    vector<RegexStruct> regexList;

    for(long unsigned int i = 0; i < jsonList.size(); i++) {
        RegexStruct rs = RegexStruct();
        vector<EventParam> eventParams;
        try {
            string eventRegex = jsonList[i]["regex"];
            regexString = timestampRegex + eventRegex;
            string tag = jsonList[i]["tag"];
            vector<string> params = jsonList[i]["params"];
            vector<string> timestampParams = { "month", "day", "time" };
            params.insert(params.begin(), timestampParams.begin(), timestampParams.end());
            regex expr(regexString);
            expression = expr;
            parseParams(params, eventParams);
            rs.params = eventParams;
            rs.tag = tag;
            rs.regexExpression = expression;
            regexList.push_back(rs);
        } catch (nlohmann::detail::type_error& deException) {
            SWSS_LOG_ERROR("Missing required key, throws exception: %s\n", deException.what());
            return false;
        } catch (regex_error& reException) {
            SWSS_LOG_ERROR("Invalid regex, throws exception: %s\n", reException.what());
            return false;
        }
    }

    if(regexList.empty()) {
        SWSS_LOG_ERROR("Empty list of regex expressions.\n");
        return false;
    }

    m_parser->m_regexList = regexList;

    regexFile.close();
    return true;
}

void RsyslogPlugin::run() {
    signal(SIGTERM, RsyslogPlugin::signalHandler);
    lua_State* luaState = luaL_newstate();
    luaL_openlibs(luaState);
    string line;
    while(RsyslogPlugin::g_running && getline(cin, line)) {
        if(line.empty()) {
            continue;
        }
        onMessage(line, luaState);
    }
    lua_close(luaState);
}

int RsyslogPlugin::onInit() {
    m_eventHandle = events_init_publisher(m_moduleName);
    bool success = createRegexList();
    if(!success) {
        return 1; // invalid regex error code
    } else if(m_eventHandle == NULL) {
        return 2; // event init publish error code
    }
    return 0;
}

RsyslogPlugin::RsyslogPlugin(string moduleName, string regexPath) {
    m_parser = unique_ptr<SyslogParser>(new SyslogParser());
    m_moduleName = moduleName;
    m_regexPath = regexPath;
    RsyslogPlugin::g_running = true;
}
