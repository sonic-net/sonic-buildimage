#include "init_thread.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "mock_reboot_interfaces.h"
#include "reboot_interfaces.h"
#include "redis_utils.h"
#include "select.h"
#include "selectableevent.h"
#include "stateverification.h"
#include "status_code_util.h"
#include "table.h"
#include "test_utils_common.h"
#include "timestamp.h"

namespace rebootbackend {

using ::testing::_;
using ::testing::AtLeast;
using ::testing::ExplainMatchResult;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::StrictMock;

constexpr int kSelectTimeoutSeconds = 5;
constexpr int kShortSelectTimeoutSeconds = 1;

MATCHER(IsDoneStatus, "") {
  const InitThreadStatus::DetailedStatus &status = arg;
  if (status.thread_state.active()) {
    *result_listener << "Status was active, expected inactive";
    return false;
  }
  if (status.detailed_thread_status != InitThreadStatus::ThreadStatus::DONE) {
    *result_listener << "Status was not DONE: "
                     << status.detailed_thread_status;
    return false;
  }
  if (status.thread_state.status().status() !=
      gnoi::system::RebootStatus_Status::RebootStatus_Status_STATUS_SUCCESS) {
    *result_listener << "Proto status was not SUCCESS: "
                     << status.thread_state.status().status();
    return false;
  }
  return true;
}

MATCHER_P(IsActiveStatus, state_matcher, "") {
  const InitThreadStatus::DetailedStatus &status = arg;
  if (!status.thread_state.active()) {
    *result_listener << "Status was inactive, expected active";
    return false;
  }
  if (status.thread_state.status().status() !=
      gnoi::system::RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN) {
    *result_listener << "Proto status was not UNKNOWN: "
                     << status.thread_state.status().status();
    return false;
  }
  return ExplainMatchResult(state_matcher, status.detailed_thread_status,
                            result_listener);
}

MATCHER_P(IsErrorStatus, error_condition_matcher, "") {
  const InitThreadStatus::DetailedStatus &status = arg;
  if (status.thread_state.active()) {
    *result_listener << "Status was active, expected inactive";
    return false;
  }
  if (status.detailed_thread_status != InitThreadStatus::ThreadStatus::ERROR) {
    *result_listener << "Status was not ERROR: "
                     << status.detailed_thread_status;
    return false;
  }
  if (status.thread_state.status().status() !=
      gnoi::system::RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE) {
    *result_listener << "Proto status was not FAILURE: "
                     << status.thread_state.status().status();
    return false;
  }
  return ExplainMatchResult(error_condition_matcher,
                            status.detailed_thread_error_condition,
                            result_listener);
}

class InitThreadTest : public ::testing::Test {
 public:
  InitThreadTest()
      : m_db("STATE_DB", 0),
        m_config_db("CONFIG_DB", 0),
        m_critical_interface(),
        m_init_thread(m_critical_interface, m_telemetry, m_finished,
                      m_stack_unfrozen) {
    // sigterm_requested and the Redis tables have global state that is
    // maintained across tests.
    sigterm_requested = false;
    TestUtils::clear_tables(m_db);
    init_redis_defaults();
  }

  void populate_default_init_table() {
    initTable.hset("docker1|app1", "timestamp", "");
    initTable.hset("docker2|app2", "timestamp", "");
    initTable.hset("docker3|app3", "timestamp", "");
    // The invalid entry should not end up in the list of apps.
    initTable.hset("invalid", "timestamp", "");
  }

  void advance_through_registration() {
    populate_default_init_table();
    TestUtils::populate_registration_table(m_db, "docker1|app1", false, false,
                                           false, true);
    TestUtils::populate_registration_table(m_db, "docker2|app2", true, true,
                                           true, false);
    TestUtils::populate_registration_table(m_db, "docker3|app3", false, false,
                                           true, false);
  }

  void set_apps_to_state(std::string state) {
    TestUtils::populate_restart_table_state(m_db, "app1", state);
    TestUtils::populate_restart_table_state(m_db, "app2", state);
    TestUtils::populate_restart_table_state(m_db, "app3", state);
  }

 protected:
  swss::DBConnector m_db;
  swss::DBConnector m_config_db;
  StrictMock<MockCriticalStateInterface> m_critical_interface;
  StrictMock<MockTelemetryInterface> m_telemetry;
  swss::NotificationConsumer m_nsf_channel;
  swss::SelectableEvent m_finished;
  swss::SelectableEvent m_stack_unfrozen;
  InitThread m_init_thread;
};

TEST_F(InitThreadTest, TestJoinWithoutStart) {
  EXPECT_FALSE(m_init_thread.Join());
}

class InitThreadTestWithSvResult
    : public InitThreadTest,
      public testing::WithParamInterface<std::string> {};


}  // namespace rebootbackend
