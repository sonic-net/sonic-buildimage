#include <iostream>
extern "C" void openSyslog();
extern "C" void writeToSyslog(const char* ev_id, int ev_sev, const char* ev_type,
                             const char* ev_act, const char* ev_src, const char* ev_msg,
                             const char* ev_static_msg);
extern "C" void closeSyslog();

