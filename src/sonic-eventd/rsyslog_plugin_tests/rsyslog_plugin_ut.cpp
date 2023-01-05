extern "C"
{
    #include <lua5.1/lua.h>
    #include <lua5.1/lualib.h>
    #include <lua5.1/lauxlib.h>
}
#include <iostream>
#include <fstream>
#include <memory>
#include <regex>
#include "gtest/gtest.h"
#include "json.hpp"
#include "events.h"
#include "../rsyslog_plugin/rsyslog_plugin.h"
#include "../rsyslog_plugin/syslog_parser.h"
#include "../rsyslog_plugin/timestamp_formatter.h"

using namespace std;
using namespace swss;
using json = nlohmann::json;

const string g_stored_year = "2024";

vector<EventParam> createEventParams(vector<string> params, vector<string> luaCodes) {
    vector<EventParam> eventParams;
    for(long unsigned int i = 0; i < params.size(); i++) {
        EventParam ep = EventParam();
        ep.paramName = params[i];
        ep.luaCode = luaCodes[i];
        eventParams.push_back(ep);
    }
    return eventParams;
}

TEST(syslog_parser, matching_regex) {    
    json jList = json::array();
    vector<RegexStruct> regexList;
    string regexString = "^([a-zA-Z]{3})?\\s*([0-9]{1,2})?\\s*([0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{0,6})?\\s*message (.*) other_data (.*) even_more_data (.*)";
    vector<string> params = { "month", "day", "time", "message", "other_data", "even_more_data" };
    vector<string> luaCodes = { "", "", "", "", "", "" };
    regex expression(regexString);
    
    RegexStruct rs = RegexStruct();
    rs.tag = "test_tag";
    rs.regexExpression = expression;
    rs.params = createEventParams(params, luaCodes);
    regexList.push_back(rs);

    string tag;
    event_params_t paramDict;

    event_params_t expectedDict;
    expectedDict["message"] = "test_message";
    expectedDict["other_data"] = "test_data";
    expectedDict["even_more_data"] = "test_data";

    unique_ptr<SyslogParser> parser(new SyslogParser());
    parser->m_regexList = regexList;
    lua_State* luaState = luaL_newstate();
    luaL_openlibs(luaState);

    bool success = parser->parseMessage("message test_message other_data test_data even_more_data test_data", tag, paramDict, luaState);
    EXPECT_EQ(true, success);
    EXPECT_EQ("test_tag", tag);
    EXPECT_EQ(expectedDict, paramDict);
    
    lua_close(luaState);
}

TEST(syslog_parser, matching_regex_timestamp) {    
    json jList = json::array();
    vector<RegexStruct> regexList;
    string regexString = "^([a-zA-Z]{3})?\\s*([0-9]{1,2})?\\s*([0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{0,6})?\\s*message (.*) other_data (.*)";
    vector<string> params = { "month", "day", "time", "message", "other_data" };
    vector<string> luaCodes = { "", "", "", "", "" };
    regex expression(regexString);

    RegexStruct rs = RegexStruct();
    rs.tag = "test_tag";
    rs.regexExpression = expression;
    rs.params = createEventParams(params, luaCodes);
    regexList.push_back(rs);

    string tag;
    event_params_t paramDict;

    event_params_t expectedDict;
    expectedDict["message"] = "test_message";
    expectedDict["other_data"] = "test_data";
    // Adding stored year to messages as syslog don't contain year
    expectedDict["timestamp"] = g_stored_year + "-07-21T02:10:00.000000Z";

    unique_ptr<SyslogParser> parser(new SyslogParser());
    parser->m_regexList = regexList;
    lua_State* luaState = luaL_newstate();
    luaL_openlibs(luaState);

    parser->m_timestampFormatter->m_storedTimestamp = "010100:00:00.000000";
    parser->m_timestampFormatter->m_storedYear = g_stored_year;
    bool success = parser->parseMessage("Jul 21 02:10:00.000000 message test_message other_data test_data", tag, paramDict, luaState);
    EXPECT_EQ(true, success);
    EXPECT_EQ("test_tag", tag);
    EXPECT_EQ(expectedDict, paramDict);
    
    lua_close(luaState);
}

TEST(syslog_parser, no_matching_regex) {
    json jList = json::array();
    vector<RegexStruct> regexList;
    string regexString = "^([a-zA-Z]{3})?\\s*([0-9]{1,2})?\\s*([0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{0,6})?\\s*no match";
    vector<string> params = { "month", "day", "time" };
    vector<string> luaCodes = { "", "", "" };
    regex expression(regexString);

    RegexStruct rs = RegexStruct();
    rs.tag = "test_tag";
    rs.regexExpression = expression;
    rs.params = createEventParams(params, luaCodes);
    regexList.push_back(rs);

    string tag;
    event_params_t paramDict;

    unique_ptr<SyslogParser> parser(new SyslogParser());
    parser->m_regexList = regexList;
    lua_State* luaState = luaL_newstate();
    luaL_openlibs(luaState);

    bool success = parser->parseMessage("Test Message", tag, paramDict, luaState);
    EXPECT_EQ(false, success);

    lua_close(luaState);
}

TEST(syslog_parser, lua_code_valid_1) {
    json jList = json::array();
    vector<RegexStruct> regexList;
    string regexString = "^([a-zA-Z]{3})?\\s*([0-9]{1,2})?\\s*([0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{0,6})?\\s*.* (sent|received) (?:to|from) .* ([0-9]{2,3}.[0-9]{2,3}.[0-9]{2,3}.[0-9]{2,3}) active ([1-9]{1,3})/([1-9]{1,3}) .*";
    vector<string> params = { "month", "day", "time", "is-sent", "ip", "major-code", "minor-code" };
    vector<string> luaCodes = { "", "", "", "ret=tostring(arg==\"sent\")", "", "", "" };
    regex expression(regexString);

    RegexStruct rs = RegexStruct();
    rs.tag = "test_tag";
    rs.regexExpression = expression;
    rs.params = createEventParams(params, luaCodes);
    regexList.push_back(rs);

    string tag;
    event_params_t paramDict;

    event_params_t expectedDict;
    expectedDict["is-sent"] = "true";
    expectedDict["ip"] = "100.95.147.229";
    expectedDict["major-code"] = "2";
    expectedDict["minor-code"] = "2";

    unique_ptr<SyslogParser> parser(new SyslogParser());
    parser->m_regexList = regexList;
    lua_State* luaState = luaL_newstate();
    luaL_openlibs(luaState);

    bool success = parser->parseMessage("NOTIFICATION: sent to neighbor 100.95.147.229 active 2/2 (peer in wrong AS) 2 bytes", tag, paramDict, luaState);
    EXPECT_EQ(true, success);
    EXPECT_EQ("test_tag", tag);
    EXPECT_EQ(expectedDict, paramDict);
    
    lua_close(luaState);
}

TEST(syslog_parser, lua_code_valid_2) {
    json jList = json::array();
    vector<RegexStruct> regexList;
    string regexString = "([a-zA-Z]{3})?\\s*([0-9]{1,2})?\\s*([0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{0,6})?\\s*.* (sent|received) (?:to|from) .* ([0-9]{2,3}.[0-9]{2,3}.[0-9]{2,3}.[0-9]{2,3}) active ([1-9]{1,3})/([1-9]{1,3}) .*";
    vector<string> params = { "month", "day", "time", "is-sent", "ip", "major-code", "minor-code" };
    vector<string> luaCodes = { "", "", "", "ret=tostring(arg==\"sent\")", "", "", "" };
    regex expression(regexString);

    RegexStruct rs = RegexStruct();
    rs.tag = "test_tag";
    rs.regexExpression = expression;
    rs.params = createEventParams(params, luaCodes);
    regexList.push_back(rs);

    string tag;
    event_params_t paramDict;

    event_params_t expectedDict;
    expectedDict["is-sent"] = "false";
    expectedDict["ip"] = "10.10.24.216";
    expectedDict["major-code"] = "6";
    expectedDict["minor-code"] = "2";
    expectedDict["timestamp"] = g_stored_year + "-12-03T12:36:24.503424Z";

    unique_ptr<SyslogParser> parser(new SyslogParser());
    parser->m_regexList = regexList;
    lua_State* luaState = luaL_newstate();
    luaL_openlibs(luaState);

    parser->m_timestampFormatter->m_storedTimestamp = "010100:00:00.000000";
    parser->m_timestampFormatter->m_storedYear = g_stored_year;
    bool success = parser->parseMessage("Dec  3 12:36:24.503424 NOTIFICATION: received from neighbor 10.10.24.216 active 6/2 (Administrative Shutdown) 0 bytes", tag, paramDict, luaState);
    EXPECT_EQ(true, success);
    EXPECT_EQ("test_tag", tag);
    EXPECT_EQ(expectedDict, paramDict);
    
    lua_close(luaState);
}

TEST(rsyslog_plugin, onInit_emptyJSON) {
    unique_ptr<RsyslogPlugin> plugin(new RsyslogPlugin("test_mod_name", "./rsyslog_plugin_tests/test_regex_1.rc.json"));
    EXPECT_NE(0, plugin->onInit());
}

TEST(rsyslog_plugin, onInit_missingRegex) {
    unique_ptr<RsyslogPlugin> plugin(new RsyslogPlugin("test_mod_name", "./rsyslog_plugin_tests/test_regex_3.rc.json"));
    EXPECT_NE(0, plugin->onInit());
}

TEST(rsyslog_plugin, onInit_invalidRegex) {
    unique_ptr<RsyslogPlugin> plugin(new RsyslogPlugin("test_mod_name", "./rsyslog_plugin_tests/test_regex_4.rc.json"));
    EXPECT_NE(0, plugin->onInit());
}

TEST(rsyslog_plugin, onMessage) {
    unique_ptr<RsyslogPlugin> plugin(new RsyslogPlugin("test_mod_name", "./rsyslog_plugin_tests/test_regex_2.rc.json"));
    EXPECT_EQ(0, plugin->onInit());
    ifstream infile("./rsyslog_plugin_tests/test_syslogs.txt");
    string logMessage;
    bool parseResult;
    lua_State* luaState = luaL_newstate();
    luaL_openlibs(luaState);
    while(infile >> logMessage >> parseResult) {
        EXPECT_EQ(parseResult, plugin->onMessage(logMessage, luaState));
    }
    lua_close(luaState);
    infile.close();
}

TEST(rsyslog_plugin, onMessage_noParams) {
    unique_ptr<RsyslogPlugin> plugin(new RsyslogPlugin("test_mod_name", "./rsyslog_plugin_tests/test_regex_5.rc.json"));
    EXPECT_EQ(0, plugin->onInit());
    ifstream infile("./rsyslog_plugin_tests/test_syslogs_2.txt");
    string logMessage;
    bool parseResult;
    lua_State* luaState = luaL_newstate();
    luaL_openlibs(luaState);
    while(infile >> logMessage >> parseResult) {
        EXPECT_EQ(parseResult, plugin->onMessage(logMessage, luaState));
    }
    lua_close(luaState);
    infile.close();
}

TEST(timestampFormatter, changeTimestampFormat) {
    unique_ptr<TimestampFormatter> formatter(new TimestampFormatter());
    
    vector<string> timestampOne = { "Jul", "20", "10:09:40.230874" };
    vector<string> timestampTwo = { "Jan", "1", "00:00:00.000000" };
    vector<string> timestampThree = { "Dec", "31", "23:59:59.000000" }; 

    formatter->m_storedTimestamp = "010100:00:00.000000";
    formatter->m_storedYear = g_stored_year;

    string formattedTimestampOne = formatter->changeTimestampFormat(timestampOne);
    string expectedTimestampOne = g_stored_year + "-07-20T10:09:40.230874Z";

    EXPECT_EQ(expectedTimestampOne, formattedTimestampOne);

    EXPECT_EQ("072010:09:40.230874", formatter->m_storedTimestamp);

    formatter->m_storedTimestamp = "010100:00:00.000000";
    formatter->m_storedYear = g_stored_year;

    string formattedTimestampTwo = formatter->changeTimestampFormat(timestampTwo);
    string expectedTimestampTwo = g_stored_year + "-01-01T00:00:00.000000Z";
    EXPECT_EQ(expectedTimestampTwo, formattedTimestampTwo);

    formatter->m_storedTimestamp = "010100:00:00.000000";
    formatter->m_storedYear = "2025";

    string formattedTimestampThree = formatter->changeTimestampFormat(timestampThree);
    EXPECT_EQ("2025-12-31T23:59:59.000000Z", formattedTimestampThree);
}

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
