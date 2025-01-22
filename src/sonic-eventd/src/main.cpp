#include "logger.h"
#include "eventd.h"

void run_eventd_service();

int main()
{
    swss::Logger::setMinPrio(swss::Logger::SWSS_DEBUG);
    SWSS_LOG_INFO("The eventd service started"); 

    run_eventd_service();

    SWSS_LOG_INFO("The eventd service exited");

    return 0;
}

