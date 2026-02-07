#ifndef __EVENTUTILS_H__
#define __EVENTUTILS_H__

#include <string>
#include <unordered_map>

const std::string EVENT_SEVERITY_CRITICAL_STR = "CRITICAL";
const std::string EVENT_SEVERITY_MAJOR_STR    = "MAJOR";
const std::string EVENT_SEVERITY_MINOR_STR    = "MINOR";
const std::string EVENT_SEVERITY_WARNING_STR  = "WARNING";
const std::string EVENT_SEVERITY_INFORMATIONAL_STR = "INFORMATIONAL";

const std::string EVENT_ENABLE_TRUE_STR  = "true";
const std::string EVENT_ENABLE_FALSE_STR = "false";

const std::string EVENT_ACTION_RAISE_STR  = "RAISE";
const std::string EVENT_ACTION_CLEAR_STR  = "CLEAR";
const std::string EVENT_ACTION_ACK_STR    = "ACKNOWLEDGE";
const std::string EVENT_ACTION_UNACK_STR  = "UNACKNOWLEDGE";

constexpr char EVENTD_DEFAULT_MAP_FILE[] = "/etc/evprofile/default.json";

constexpr size_t EHT_MAX_ELEMS    = 40000;
constexpr size_t EHT_MAX_DAYS     = 30;
constexpr char EVENTD_CONF_FILE[] = "/etc/eventd.json";

typedef struct EventInfo_t {
    std::string  severity;
    std::string  enable;
    std::string  static_event_msg;
} EventInfo;

//unordered_map<string, EventInfo> static_event_table;
typedef std::unordered_map<std::string, EventInfo> EventMap; 

bool isValidSeverity(std::string severityStr);
bool isValidEnable(std::string enableStr);
bool parse_config(const char *filename, unsigned int& days, unsigned int& count);
bool parse(const char *filename, EventMap& tmp_event_table);

#endif
