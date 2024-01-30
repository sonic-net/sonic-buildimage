#include "eventutils.h"
#include <swss/logger.h>
#include <swss/table.h>
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <nlohmann/json.hpp>


using namespace swss;
using json = nlohmann::json;

bool isValidSeverity(string severityStr) {
    transform(severityStr.begin(), severityStr.end(), severityStr.begin(), ::toupper);
    if (severityStr == EVENT_SEVERITY_MAJOR_STR) return true;
    if (severityStr == EVENT_SEVERITY_CRITICAL_STR) return true;
    if (severityStr == EVENT_SEVERITY_MINOR_STR) return true;
    if (severityStr == EVENT_SEVERITY_WARNING_STR) return true;
    if (severityStr == EVENT_SEVERITY_INFORMATIONAL_STR) return true;
    return false;
}

bool isValidEnable(string enableStr) {
    if (enableStr == EVENT_ENABLE_TRUE_STR) return true;
    if (enableStr == EVENT_ENABLE_FALSE_STR) return true;
    return false;
}

bool parse_config(const char *filename, unsigned int& days, unsigned int& count) {
    days = EHT_MAX_DAYS;
    count = EHT_MAX_ELEMS;
    std::ifstream ifs(filename);
    json j = json::parse(ifs);
    for (json::iterator it = j.begin(); it != j.end(); ++it) {
        if(it.key() == "max-days") {
            days = it.value();
        }
        if(it.key() == "max-records") {
            count = it.value();
        }
    }
    return true;
}

bool parse(const char *filename, EventMap& tmp_event_table) {
    ifstream fs(filename);
    if (!fs.is_open()) {
        return false;
    }

    fstream file(filename, fstream::in);
    json j;
    file >> j;

    if (j["events"].size() == 0) {
        SWSS_LOG_ERROR("No entries in 'events' field in %s", filename);
        return false;
    }

    for (size_t i = 0; i < j["events"].size(); i++) {
        auto elem = j["events"][i];
        struct EventInfo_t ev_info;
        string ev_name = elem["name"];
        ev_info.severity = elem["severity"];
        ev_info.enable = elem["enable"];
        ev_info.static_event_msg = elem["message"];
        tmp_event_table.emplace(ev_name, ev_info);
    }

    return true;
}

