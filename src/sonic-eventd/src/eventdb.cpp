#include "eventconsume.h"
#include <csignal>

void signalHandler(const int signal) {

    if (signal == SIGINT) {
        reload_config_flag.store(true);
    }
}

int main()
{
    swss::Logger::getInstance().setMinPrio(swss::Logger::SWSS_NOTICE);

    swss::DBConnector eventDb("EVENT_DB", 0);

    // register signal handlers
    signal(SIGINT, signalHandler);

    EventConsume evtd(&eventDb);

    evtd.run();

    return 0;
}

