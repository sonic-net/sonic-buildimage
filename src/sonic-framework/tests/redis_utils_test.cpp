#include "redis_utils.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <string>
#include <thread>
#include <vector>

#include "select.h"
#include "table.h"
#include "test_utils_common.h"
#include "timestamp.h"

namespace rebootbackend {

using WarmStartState = ::swss::WarmStart::WarmStartState;

using ::testing::AllOf;
using ::testing::HasSubstr;
using ::testing::StrEq;

class RedisTest : public ::testing::Test {
 protected:
  RedisTest() : m_db("STATE_DB", 0) {
    TestUtils::clear_tables(m_db);
  }

  swss::DBConnector m_db;
};

TEST_F(RedisTest, testSetWarmRestartEnable) {
  swss::Table warmRestartTable(&m_db, STATE_WARM_RESTART_ENABLE_TABLE_NAME);

  for (const auto &enabled : {true, false}) {
    warmRestartTable.del("system");

    set_warm_restart_enable(m_db, enabled);

    std::string value;
    bool ret = warmRestartTable.hget("system", "enable", value);
    EXPECT_TRUE(ret);
    EXPECT_EQ(value, enabled ? "true" : "false");
  }
}

TEST_F(RedisTest, GetWarmRestartCounter) {
  EXPECT_THAT(get_warm_restart_counter(m_db), StrEq(""));
  for (int i = 0; i < 5; i++) {
    set_warm_restart_counter(m_db, i);
    EXPECT_THAT(get_warm_restart_counter(m_db), StrEq(std::to_string(i)));
  }
}

class RedisTestWithWarmStartState
    : public RedisTest,
      public ::testing::WithParamInterface<WarmStartState> {};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(RedisTestWithWarmStartState);

INSTANTIATE_TEST_SUITE_P(TestOverWarmStartStates,
                         RedisTestWithWarmStartState, testing::Values(true, false));


}  // namespace rebootbackend
