#ifndef __EVENTUTILS_H__
#define __EVENTUTILS_H__

#include <string>
#include <chrono>
#include <atomic>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <functional>
#include <unordered_map>
#include <chrono>
#include <sstream>

using namespace std;

const string EVENT_SEVERITY_CRITICAL_STR = "CRITICAL";
const string EVENT_SEVERITY_MAJOR_STR    = "MAJOR";
const string EVENT_SEVERITY_MINOR_STR    = "MINOR";
const string EVENT_SEVERITY_WARNING_STR  = "WARNING";
const string EVENT_SEVERITY_INFORMATIONAL_STR = "INFORMATIONAL";

const string EVENT_ENABLE_TRUE_STR  = "true";
const string EVENT_ENABLE_FALSE_STR = "false";

const string EVENT_ACTION_RAISE_STR  = "RAISE";
const string EVENT_ACTION_CLEAR_STR  = "CLEAR";
const string EVENT_ACTION_ACK_STR    = "ACKNOWLEDGE";
const string EVENT_ACTION_UNACK_STR  = "UNACKNOWLEDGE";

constexpr char EVENTD_DEFAULT_MAP_FILE[] = "/etc/evprofile/default.json";

constexpr size_t EHT_MAX_ELEMS    = 40000;
constexpr size_t EHT_MAX_DAYS     = 30;
constexpr char EVENTD_CONF_FILE[] = "/etc/eventd.json";

typedef struct EventInfo_t {
    string  severity;
    string  enable;
    string  static_event_msg;
} EventInfo;

//unordered_map<string, EventInfo> static_event_table;
typedef unordered_map<string, EventInfo> EventMap; 

bool isValidSeverity(string severityStr);
bool isValidEnable(string enableStr);
bool parse_config(const char *filename, unsigned int& days, unsigned int& count);
bool parse(const char *filename, EventMap& tmp_event_table);

#endif
