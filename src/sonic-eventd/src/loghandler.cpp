#include <iostream>
#include <syslog.h>

extern "C" void openSyslog() {
    openlog (NULL, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL4);
}

extern "C" void writeToSyslog(std::string ev_id, int ev_sev, std::string ev_type, std::string ev_act, std::string ev_msg, std::string ev_static_msg) {
    int SYSLOG_FACILITY = LOG_LOCAL4;
    if (ev_act.empty()) {
        const char LOG_FORMAT[] = "[%s], %%%s: %s %s"; 
                                                      // event Type
                                                      // Event Name
                                                      // Static Desc
                                                      // Dynamic Desc

        // raise a syslog message
        syslog(LOG_MAKEPRI(ev_sev, SYSLOG_FACILITY), LOG_FORMAT,
            ev_type.c_str(), 
            ev_id.c_str(), ev_static_msg.c_str(), ev_msg.c_str());
    } else {
        const char LOG_FORMAT[] = "[%s] (%s), %%%s: %s %s"; 
                                                      // event Type
                                                      // event action
                                                      // Event Name
                                                      // Static Desc
                                                      // Dynamic Desc
        // raise a syslog message
        syslog(LOG_MAKEPRI(ev_sev, SYSLOG_FACILITY), LOG_FORMAT,
            ev_type.c_str(), ev_act.c_str(), 
            ev_id.c_str(), ev_static_msg.c_str(), ev_msg.c_str());
    }
}

extern "C" void closeSyslog() {
    closelog ();
}
