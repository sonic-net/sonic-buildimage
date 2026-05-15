#include <iostream>
#include <syslog.h>
#include <cstring>



// Safe fallback for null or empty C-style strings
const char* safe(const char* s) {
    return (s && std::strlen(s) > 0) ? s : "";
}


extern "C" void openSyslog() {
    openlog (NULL, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL4);
}

extern "C" void writeToSyslog(const char* ev_id, int ev_sev, const char* ev_type, const char* ev_act,
                              const char* ev_src, const char* ev_msg, const char* ev_static_msg) {
    int SYSLOG_FACILITY = LOG_LOCAL4;

    if (!ev_act || std::strlen(ev_act) == 0) {
        const char LOG_FORMAT[] = "[%s], %%%s: %%%%%s: %s %s"; 
                                                      // event Type
                                                      // Event Name
                                                      // Event Source
                                                      // Static Desc
                                                      // Dynamic Desc

        // raise a syslog message
        syslog(LOG_MAKEPRI(ev_sev, SYSLOG_FACILITY), LOG_FORMAT,
            safe(ev_type), safe(ev_id), safe(ev_src), safe(ev_static_msg), safe(ev_msg));
    } else {
        const char LOG_FORMAT[] = "[%s] (%s), %%%s: %%%%%s: %s %s"; 
                                                      // event Type
                                                      // event action
                                                      // Event Name
                                                      // Event Source
                                                      // Static Desc
                                                      // Dynamic Desc
        // raise a syslog message
        syslog(LOG_MAKEPRI(ev_sev, SYSLOG_FACILITY), LOG_FORMAT,
            safe(ev_type), safe(ev_act), safe(ev_id), safe(ev_src), safe(ev_static_msg), safe(ev_msg));
    }
}

extern "C" void closeSyslog() {
    closelog ();
}
