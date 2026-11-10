#include <iostream>
#include <ctime>
#include <swss/logger.h>
#include "syslog_parser.h"

/**
 * Parses syslog message and returns structured event
 *
 * @param nessage us syslog message being fed in by rsyslog.d
 * @return return structured event json for publishing
 *
*/

bool SyslogParser::parseMessage(string message, string& eventTag, event_params_t& paramMap, lua_State* luaState) {
    for(long unsigned int i = 0; i < m_regexList.size(); i++) {
        smatch matchResults;
        if(!regex_search(message, matchResults, m_regexList[i].regexExpression) || m_regexList[i].params.size() != matchResults.size() - 1 || matchResults.size() < 4) {
            continue;
        }
        string formattedTimestamp;
        if(!matchResults[1].str().empty() && !matchResults[2].str().empty() && !matchResults[3].str().empty()) { // found timestamp components
            formattedTimestamp = m_timestampFormatter->changeTimestampFormat({ matchResults[1].str(), matchResults[2].str(), matchResults[3].str() });
	}
        if(!formattedTimestamp.empty()) {
            paramMap["timestamp"] = formattedTimestamp;
	} else {
            SWSS_LOG_INFO("Timestamp is invalid and is not able to be formatted");
	}

        // found matching regex
        eventTag = m_regexList[i].tag;
	// check params for lua code
        for(long unsigned int j = 3; j < m_regexList[i].params.size(); j++) {
	    string resultValue = matchResults[j + 1].str();
	    string paramName = m_regexList[i].params[j].paramName;
	    const char* luaCode = m_regexList[i].params[j].luaCode.c_str();

            if(luaCode == NULL || *luaCode == 0) {
                SWSS_LOG_INFO("Invalid lua code, empty or missing");
                paramMap[paramName] = resultValue;
		continue;
	    }

	    // execute lua code
            lua_pushstring(luaState, resultValue.c_str());
            lua_setglobal(luaState, "arg");
            if(luaL_dostring(luaState, luaCode) == 0) {
                lua_pop(luaState, lua_gettop(luaState));
	    } else { // error in lua code
		SWSS_LOG_ERROR("Invalid lua code, unable to do operation.\n");
		paramMap[paramName] = resultValue;
		continue;
            }
            lua_getglobal(luaState, "ret");
            paramMap[paramName] = lua_tostring(luaState, -1);
            lua_pop(luaState, 1);
	}
        return true;
    }
    return false;
}

SyslogParser::SyslogParser() {
    m_timestampFormatter = unique_ptr<TimestampFormatter>(new TimestampFormatter());
}
