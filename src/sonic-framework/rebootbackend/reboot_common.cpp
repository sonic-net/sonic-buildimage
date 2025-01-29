#include "reboot_common.h"

#include <time.h>

namespace rebootbackend {

timespec milliseconds_to_timespec(uint64_t time_ms) {
  timespec l_timespec;
  l_timespec.tv_sec = time_ms / ONE_THOUSAND;
  l_timespec.tv_nsec = (time_ms % ONE_THOUSAND) * ONE_THOUSAND * ONE_THOUSAND;
  return l_timespec; 
}

}  // namespace rebootbackend
