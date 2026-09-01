#include "reboot_common.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <unistd.h>

namespace rebootbackend {

using ::testing::_;
using ::testing::AtLeast;
using ::testing::ExplainMatchResult;
using ::testing::StrEq;

MATCHER_P2(CheckTimespec, secs, nsecs, "") {
  return (arg.tv_sec == secs && arg.tv_nsec == nsecs);
}

TEST(RebootCommon, MillisecToTimespec) {
  timespec l_timespec = milliseconds_to_timespec(0);
  EXPECT_THAT(l_timespec, CheckTimespec(0, 0));
  l_timespec = milliseconds_to_timespec(200);
  EXPECT_THAT(l_timespec, CheckTimespec(0, 200 * 1000 * 1000));
  l_timespec = milliseconds_to_timespec(1800);
  EXPECT_THAT(l_timespec, CheckTimespec(1, 800 * 1000 * 1000));
}

}  // namespace rebootbackend
