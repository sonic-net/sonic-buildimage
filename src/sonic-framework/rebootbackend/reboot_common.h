#pragma once

#include <time.h>

#include "status_code_util.h"

namespace rebootbackend {

#define ONE_THOUSAND (1000)

extern bool sigterm_requested;

extern timespec milliseconds_to_timespec(uint64_t time_ms);

struct NotificationResponse {
  swss::StatusCode status;
  std::string json_string;
}; 

}  // namespace rebootbackend
