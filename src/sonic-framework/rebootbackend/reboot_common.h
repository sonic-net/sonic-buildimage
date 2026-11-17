#pragma once

#include <time.h>

#include "status_code_util.h"

namespace rebootbackend {

extern bool sigterm_requested;
struct NotificationResponse {
  swss::StatusCode status;
  std::string json_string;
};

}  // namespace rebootbackend
