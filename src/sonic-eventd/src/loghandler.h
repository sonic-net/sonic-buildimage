#include <iostream>
extern "C" void openSyslog();
extern "C" void writeToSyslog(std::string ev_id, int ev_sev, std::string ev_type, std::string ev_act, std::string ev_msg, std::string ev_static_msg);
extern "C" void closeSyslog();

