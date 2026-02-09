#include "eventconsume.h"
#include <csignal>

static EventConsume *evtd_instance = NULL;

void signalHandler(const int signal) { 

    if (signal == SIGINT) {
        reload_config_flag.store(true);
    }
}

int main()
{
    swss::Logger::getInstance().setMinPrio(swss::Logger::SWSS_DEBUG);

    swss::DBConnector eventDb("EVENT_DB", 0);
    
    // register signal SIGINT and signal handler  
    signal(SIGINT, signalHandler);

    EventConsume evtd(&eventDb);
    evtd_instance = &evtd;

    evtd.run();

    return 0;
}

