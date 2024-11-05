#include "test_utils_common.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "dbconnector.h"
#include "notificationconsumer.h"
#include "redis_utils.h"
#include "select.h"
#include "selectableevent.h"
#include "table.h"
#include "timestamp.h"

namespace rebootbackend {

void TestUtils::wait_for_finish(swss::Select &s,
                                swss::SelectableEvent &finished,
                                long timeout_seconds) {
  swss::Selectable *sel;
  int ret;

  ret = s.select(&sel, timeout_seconds * 1000);
  EXPECT_EQ(ret, swss::Select::OBJECT);
  EXPECT_EQ(sel, &finished);
}

}  // namespace rebootbackend
