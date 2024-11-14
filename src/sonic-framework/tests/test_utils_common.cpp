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

void TestUtils::clear_tables(swss::DBConnector &db) {
  const std::vector<std::string> kTablesToClear = {
      "BOOT_INFO",
      STATE_WARM_RESTART_TABLE_NAME,
      STATE_WARM_RESTART_ENABLE_TABLE_NAME };

  for (const auto &table_name : kTablesToClear) {
    swss::Table table(&db, table_name);
    std::vector<std::string> keys;
    table.getKeys(keys);
    for (const auto &key : keys) {
      table.del(key);
    }
  }
}


}  // namespace rebootbackend
