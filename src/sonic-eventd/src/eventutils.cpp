#include "eventutils.h"
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <nlohmann/json.hpp>
#include "logger.h"
#include "table.h"

using namespace swss;
using namespace std;

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
    ifstream ifs(filename);

    if (!ifs.is_open()) {
        SWSS_LOG_ERROR("Failed to open file: %s", filename);
        return false;
    }

    json j;
    try {
        j = json::parse(ifs);
    }
    catch (const json::parse_error &e) {
        SWSS_LOG_ERROR("Error parsing config file %s:%s ", filename, e.what());
        return false;
    }
    catch (const std::exception &e) {
        SWSS_LOG_ERROR("Unexpected error parsing config file %s: %s", filename, e.what());
        return false;
    }

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
    ifstream file(filename);
    if (!file.is_open()) {
        SWSS_LOG_ERROR("Failed to open file: %s", filename);
        return false;
    }

    json j;
    try {
         j = json::parse(file);
    }
    catch (const json::parse_error &e) {
        SWSS_LOG_ERROR("Error parsing profile file %s:%s ", filename, e.what());
        return false;
    }
    catch (const std::exception &e) {
        SWSS_LOG_ERROR("Unexpected error parsing config file %s: %s", filename, e.what());
        return false;
    }

    if (!j.contains("events") || j["events"].empty()) {
        SWSS_LOG_NOTICE("No entries in 'events' field in %s", filename);
        return false;
    }


    for (const auto& elem : j["events"]) {
        if (!elem.contains("name") || !elem.contains("severity") ||
            !elem.contains("enable")) {
            SWSS_LOG_ERROR("Missing required fields in event entry in %s", filename);
            continue;
        }
        struct EventInfo_t ev_info;
        string ev_name = elem["name"];
        ev_info.severity = elem["severity"];
        ev_info.enable = elem["enable"];
        if (elem.contains("message")) {
            ev_info.static_event_msg = elem["message"];
        }
        tmp_event_table.emplace(ev_name, ev_info);
    }

    return true;
}

