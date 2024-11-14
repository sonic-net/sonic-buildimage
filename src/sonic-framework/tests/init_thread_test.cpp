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
#include "warm_restart.h"

namespace rebootbackend {

using WarmBootStage = ::swss::WarmStart::WarmBootStage;

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
  if (status.thread_state.method() != gnoi::system::RebootMethod::NSF) {
    *result_listener << "Proto method was not NSF: "
                     << status.thread_state.status().status();
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
  if (status.thread_state.method() != gnoi::system::RebootMethod::NSF) {
    *result_listener << "Proto method was not NSF: "
                     << status.thread_state.status().status();
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
        m_nsf_channel(&m_db, swss::WarmStart::kNsfManagerNotificationChannel),
        m_init_thread(m_critical_interface, m_telemetry, m_finished,
                      m_stack_unfrozen) {
    swss::WarmStart::initialize("fake_app", "fake_docker");
    // sigterm_requested and the Redis tables have global state that is
    // maintained across tests.
    sigterm_requested = false;
    TestUtils::clear_tables(m_db);
    init_redis_defaults();
    overwrite_reconciliation_timeout(5);
  }

  void init_redis_defaults() {
    set_warm_restart_enable(m_db, true);
    TestUtils::set_state_verification_enable(m_config_db, /*bootup=*/true,
                                             /*enabled=*/true);
  }

  void set_default_success_expects() {
    EXPECT_CALL(m_critical_interface, is_system_critical())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(m_telemetry, record_overall_end(true)).Times(1);
  }

  void check_final_success_expects(bool state_verification_enabled) {
    TestUtils::check_warmboot_enabled(m_db, /*expected_state=*/false);
    if (state_verification_enabled) {
      check_nsf_manager_notification_sent(
          swss::WarmStart::WarmBootNotification::kUnfreeze);
    }
  }

  void check_final_failure_expects() {
    TestUtils::check_warmboot_enabled(m_db, /*expected_state=*/false);
  }

  void check_nsf_manager_notification_sent(
      swss::WarmStart::WarmBootNotification notification_type) {
    swss::Select s;
    s.addSelectable(&m_nsf_channel);

    swss::Selectable *sel;
    int select_result = s.select(&sel, 2000);
    EXPECT_EQ(select_result, swss::Select::OBJECT);

    if (sel == &m_nsf_channel) {
      std::string op, data;
      std::vector<swss::FieldValueTuple> values;
      m_nsf_channel.pop(op, data, values);
      EXPECT_EQ(op, swss::WarmStart::warmBootNotificationNameMap()->at(
                        notification_type));
    }
  }

  void set_telemetry_stage_expects(WarmBootStage nsf_stage, bool success) {
    EXPECT_CALL(m_telemetry, record_stage_start(nsf_stage)).Times(1);
    EXPECT_CALL(m_telemetry, record_stage_end(nsf_stage, success)).Times(1);
  }

  void overwrite_reconciliation_timeout(uint32_t timeout_seconds) {
    m_init_thread.m_reconciliation_timeout = timeout_seconds;
  }

  void overwrite_state_verification_timeout(uint32_t timeout_seconds) {
    m_init_thread.m_state_verification_timeout = timeout_seconds;
  }

  void overwrite_unfreeze_timeout(uint32_t timeout_seconds) {
    m_init_thread.m_unfreeze_timeout = timeout_seconds;
  }

  void populate_default_init_table() {
    swss::Table initTable(&m_db, STATE_WARM_RESTART_INIT_TABLE_NAME);
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

  void advance_through_reconciliation() {
    EXPECT_CALL(m_telemetry,
                record_stage_end(WarmBootStage::STAGE_RECONCILIATION,
                                 /*success=*/true))
        .Times(1);
    advance_through_registration();
    set_apps_to_state("reconciled");
  }

  // Must be run in a separate thread.
  void advance_through_state_verification() {
    swss::NotificationConsumer nc(&m_db, STATE_VERIFICATION_REQ_CHANNEL);

    advance_through_reconciliation();

    std::string timestamp = TestUtils::wait_for_state_verification_trigger(
        nc, kSelectTimeoutSeconds, /*freeze=*/true);
    EXPECT_THAT(timestamp, Not(IsEmpty()));

    if (timestamp.empty()) {
      return;
    }

    TestUtils::write_state_verification_result(m_db, ALL_COMPONENT, SV_PASS,
                                               timestamp);
  }

  // Must be run in a separate thread.
  void advance_through_unfreeze_with_state(std::string final_state) {
    set_telemetry_stage_expects(WarmBootStage::STAGE_UNFREEZE,
                                final_state == "completed");

    swss::NotificationConsumer nc(&m_db, STATE_VERIFICATION_REQ_CHANNEL);

    advance_through_reconciliation();

    std::string timestamp = TestUtils::wait_for_state_verification_trigger(
        nc, kSelectTimeoutSeconds, /*freeze=*/true);
    EXPECT_THAT(timestamp, Not(IsEmpty()));
    if (timestamp.empty()) {
      return;
    }

    // Set apps to their final state before unfreeze runs.
    set_apps_to_state(final_state);

    TestUtils::write_state_verification_result(m_db, ALL_COMPONENT, SV_PASS,
                                               timestamp);
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

TEST_F(InitThreadTest, NoNsfIfCritical) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(m_telemetry, record_overall_end(false)).Times(1);

  EXPECT_EQ(m_init_thread.Start(),
            swss::StatusCode::SWSS_RC_FAILED_PRECONDITION);

  EXPECT_THAT(
      m_init_thread.GetDetailedStatus(),
      IsErrorStatus(InitThreadStatus::ErrorCondition::DETECTED_CRITICAL_STATE));
  check_final_failure_expects();
}

TEST_F(InitThreadTest, RegistrationCancelsForCritical) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(AtLeast(2))
      .WillOnce(Return(false))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(m_telemetry, record_overall_end(false)).Times(1);

  populate_default_init_table();

  EXPECT_EQ(m_init_thread.Start(), swss::StatusCode::SWSS_RC_SUCCESS);
  m_init_thread.Join();

  EXPECT_THAT(
      m_init_thread.GetDetailedStatus(),
      IsErrorStatus(InitThreadStatus::ErrorCondition::REGISTRATION_FAILED));
  check_final_failure_expects();
}

TEST_F(InitThreadTest, SigtermRequestedBeforeRun) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(m_critical_interface, report_critical_state(_)).Times(1);
  EXPECT_CALL(m_telemetry, record_overall_end(false)).Times(1);

  populate_default_init_table();
  sigterm_requested = true;

  EXPECT_EQ(m_init_thread.Start(), swss::StatusCode::SWSS_RC_SUCCESS);
  m_init_thread.Join();

  EXPECT_THAT(m_init_thread.GetDetailedStatus(), IsErrorStatus(_));
  check_final_failure_expects();
}

TEST_F(InitThreadTest, RegistrationStopped) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(m_critical_interface, report_critical_state(_)).Times(1);
  EXPECT_CALL(m_telemetry, record_overall_end(false)).Times(1);

  populate_default_init_table();

  EXPECT_EQ(m_init_thread.Start(), swss::StatusCode::SWSS_RC_SUCCESS);
  // This will stop the thread in the registration loop.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  m_init_thread.Stop();
  m_init_thread.Join();

  EXPECT_THAT(
      m_init_thread.GetDetailedStatus(),
      IsErrorStatus(InitThreadStatus::ErrorCondition::REGISTRATION_FAILED));
  check_final_failure_expects();
}

TEST_F(InitThreadTest, RegistrationTimeout) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(m_critical_interface, report_critical_state(_)).Times(1);
  EXPECT_CALL(m_telemetry, record_overall_end(false)).Times(1);

  populate_default_init_table();
  overwrite_reconciliation_timeout(1);

  EXPECT_EQ(m_init_thread.Start(), swss::StatusCode::SWSS_RC_SUCCESS);
  m_init_thread.Join();

  EXPECT_THAT(
      m_init_thread.GetDetailedStatus(),
      IsErrorStatus(InitThreadStatus::ErrorCondition::REGISTRATION_FAILED));
  check_final_failure_expects();
}

TEST_F(InitThreadTest, ReconciliationStopped) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(m_critical_interface, report_critical_state(_)).Times(1);
  EXPECT_CALL(m_telemetry, record_overall_end(false)).Times(1);
  EXPECT_CALL(m_telemetry, record_stage_end(WarmBootStage::STAGE_RECONCILIATION,
                                            /*success=*/false))
      .Times(1);

  advance_through_registration();

  EXPECT_EQ(m_init_thread.Start(), swss::StatusCode::SWSS_RC_SUCCESS);
  // Registration is done, so this will stop the thread in the reconciliation
  // loop.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  m_init_thread.Stop();
  m_init_thread.Join();

  EXPECT_THAT(
      m_init_thread.GetDetailedStatus(),
      IsErrorStatus(InitThreadStatus::ErrorCondition::RECONCILIATION_FAILED));
  check_final_failure_expects();
}

TEST_F(InitThreadTest, ReconciliationCritical) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(AtLeast(3))
      .WillOnce(Return(false))
      .WillOnce(Return(false))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(m_telemetry, record_overall_end(false)).Times(1);
  EXPECT_CALL(m_telemetry, record_stage_end(WarmBootStage::STAGE_RECONCILIATION,
                                            /*success=*/false))
      .Times(1);

  advance_through_registration();

  EXPECT_EQ(m_init_thread.Start(), swss::StatusCode::SWSS_RC_SUCCESS);
  m_init_thread.Join();

  EXPECT_THAT(
      m_init_thread.GetDetailedStatus(),
      IsErrorStatus(InitThreadStatus::ErrorCondition::RECONCILIATION_FAILED));
  check_final_failure_expects();
}

TEST_F(InitThreadTest, ReconciliationTimeout) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(m_critical_interface, report_critical_state(_)).Times(1);
  EXPECT_CALL(m_telemetry, record_overall_end(false)).Times(1);
  EXPECT_CALL(m_telemetry, record_stage_end(WarmBootStage::STAGE_RECONCILIATION,
                                            /*success=*/false))
      .Times(1);

  advance_through_registration();

  overwrite_reconciliation_timeout(1);

  EXPECT_EQ(m_init_thread.Start(), swss::StatusCode::SWSS_RC_SUCCESS);
  m_init_thread.Join();

  EXPECT_THAT(
      m_init_thread.GetDetailedStatus(),
      IsErrorStatus(InitThreadStatus::ErrorCondition::RECONCILIATION_FAILED));
  check_final_failure_expects();
}

TEST_F(InitThreadTest, ReconciliationAlreadyFailed) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(m_critical_interface, report_critical_state(_)).Times(1);
  EXPECT_CALL(m_telemetry, record_overall_end(false)).Times(1);
  EXPECT_CALL(m_telemetry, record_stage_end(WarmBootStage::STAGE_RECONCILIATION,
                                            /*success=*/false))
      .Times(1);

  advance_through_registration();
  TestUtils::populate_restart_table_state(m_db, "app1", "failed");

  swss::Select s;
  s.addSelectable(&m_finished);

  EXPECT_EQ(m_init_thread.Start(), swss::StatusCode::SWSS_RC_SUCCESS);
  m_init_thread.Join();

  EXPECT_THAT(
      m_init_thread.GetDetailedStatus(),
      IsErrorStatus(InitThreadStatus::ErrorCondition::RECONCILIATION_FAILED));
  check_final_failure_expects();
}

TEST_F(InitThreadTest, ReconciliationAlreadyDone) {
  TestUtils::set_state_verification_enable(m_config_db, /*bootup=*/true,
                                           /*enabled=*/false);
  set_default_success_expects();
  EXPECT_CALL(m_telemetry, record_stage_end(WarmBootStage::STAGE_RECONCILIATION,
                                            /*success=*/true))
      .Times(1);

  advance_through_registration();
  TestUtils::populate_restart_table_state(m_db, "app1", "reconciled");
  TestUtils::populate_restart_table_state(m_db, "app2", "reconciled");
  TestUtils::populate_restart_table_state(m_db, "app3", "reconciled");

  swss::Select s;
  s.addSelectable(&m_finished);

  EXPECT_EQ(m_init_thread.Start(), swss::StatusCode::SWSS_RC_SUCCESS);
  TestUtils::wait_for_finish(s, m_finished, kSelectTimeoutSeconds);
  m_init_thread.Join();

  EXPECT_THAT(m_init_thread.GetDetailedStatus(), IsDoneStatus());
  check_final_success_expects(/*state_verification_enabled=*/false);
}

TEST_F(InitThreadTest, ReconciliationFails) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(m_critical_interface, report_critical_state(_)).Times(1);
  EXPECT_CALL(m_telemetry, record_overall_end(false)).Times(1);
  EXPECT_CALL(m_telemetry, record_stage_end(WarmBootStage::STAGE_RECONCILIATION,
                                            /*success=*/false))
      .Times(1);

  advance_through_registration();

  // Apps register one-by-one.
  auto test_sequence = [&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TestUtils::populate_restart_table_state(m_db, "app1", "failed");
  };
  std::thread test_thread = std::thread(test_sequence);

  EXPECT_EQ(m_init_thread.Start(), swss::StatusCode::SWSS_RC_SUCCESS);
  swss::Select s;
  s.addSelectable(&m_finished);
  TestUtils::wait_for_finish(s, m_finished, kSelectTimeoutSeconds);
  m_init_thread.Join();

  test_thread.join();

  EXPECT_THAT(
      m_init_thread.GetDetailedStatus(),
      IsErrorStatus(InitThreadStatus::ErrorCondition::RECONCILIATION_FAILED));
  check_final_failure_expects();
}

TEST_F(InitThreadTest, StateVerificationStopped) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(m_critical_interface, report_critical_state(_)).Times(1);
  EXPECT_CALL(m_telemetry, record_overall_end(false)).Times(1);

  advance_through_reconciliation();

  EXPECT_EQ(m_init_thread.Start(), swss::StatusCode::SWSS_RC_SUCCESS);
  // Registration and reconciliation are done, so this will stop the thread in
  // the state verification loop.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  m_init_thread.Stop();
  m_init_thread.Join();

  EXPECT_THAT(m_init_thread.GetDetailedStatus(),
              IsErrorStatus(
                  InitThreadStatus::ErrorCondition::STATE_VERIFICATION_FAILED));
  check_final_failure_expects();
}

TEST_F(InitThreadTest, StateVerificationCritical) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(AtLeast(4))
      .WillOnce(Return(false))
      .WillOnce(Return(false))
      .WillOnce(Return(false))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(m_telemetry, record_overall_end(false)).Times(1);

  advance_through_reconciliation();

  EXPECT_EQ(m_init_thread.Start(), swss::StatusCode::SWSS_RC_SUCCESS);
  m_init_thread.Join();

  EXPECT_THAT(m_init_thread.GetDetailedStatus(),
              IsErrorStatus(
                  InitThreadStatus::ErrorCondition::STATE_VERIFICATION_FAILED));
  check_final_failure_expects();
}

TEST_F(InitThreadTest, StateVerificationFails) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(m_critical_interface, report_critical_state(_)).Times(1);
  EXPECT_CALL(m_telemetry, record_overall_end(false)).Times(1);

  advance_through_reconciliation();

  overwrite_state_verification_timeout(1);

  std::string timestamp;
  auto test_sequence = [&] {
    swss::NotificationConsumer nc(&m_db, STATE_VERIFICATION_REQ_CHANNEL);
    timestamp = TestUtils::wait_for_state_verification_trigger(
        nc, kSelectTimeoutSeconds, /*freeze=*/true);

    TestUtils::write_state_verification_result(m_db, ALL_COMPONENT, SV_FAIL,
                                               timestamp);
  };
  std::thread test_thread = std::thread(test_sequence);

  EXPECT_EQ(m_init_thread.Start(), swss::StatusCode::SWSS_RC_SUCCESS);
  m_init_thread.Join();

  test_thread.join();

  EXPECT_THAT(timestamp, Not(IsEmpty()));
  EXPECT_THAT(m_init_thread.GetDetailedStatus(),
              IsErrorStatus(
                  InitThreadStatus::ErrorCondition::STATE_VERIFICATION_FAILED));
  check_final_failure_expects();
}

TEST_F(InitThreadTest, UnfreezeStopped) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(m_critical_interface, report_minor_alarm(_)).Times(1);
  EXPECT_CALL(m_telemetry, record_overall_end(false)).Times(1);
  set_telemetry_stage_expects(WarmBootStage::STAGE_UNFREEZE,
                              /*success=*/false);

  swss::Select s;
  s.addSelectable(&m_stack_unfrozen);
  std::thread test_thread =
      std::thread(&InitThreadTest::advance_through_state_verification, this);

  EXPECT_EQ(m_init_thread.Start(), swss::StatusCode::SWSS_RC_SUCCESS);
  TestUtils::wait_for_finish(s, m_stack_unfrozen, kSelectTimeoutSeconds);
  // Registration, reconciliation, and state verification are done, so this
  // will stop the thread in the unfreeze loop.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  m_init_thread.Stop();
  m_init_thread.Join();

  test_thread.join();

  EXPECT_THAT(m_init_thread.GetDetailedStatus(),
              IsErrorStatus(InitThreadStatus::ErrorCondition::UNFREEZE_FAILED));
  check_final_failure_expects();
}

TEST_F(InitThreadTest, UnfreezeCritical) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(AtLeast(5))
      .WillOnce(Return(false))
      .WillOnce(Return(false))
      .WillOnce(Return(false))
      .WillOnce(Return(false))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(m_critical_interface, report_minor_alarm(_)).Times(1);
  EXPECT_CALL(m_telemetry, record_overall_end(false)).Times(1);
  set_telemetry_stage_expects(WarmBootStage::STAGE_UNFREEZE,
                              /*success=*/false);

  swss::Select s;
  s.addSelectable(&m_stack_unfrozen);
  std::thread test_thread =
      std::thread(&InitThreadTest::advance_through_state_verification, this);

  EXPECT_EQ(m_init_thread.Start(), swss::StatusCode::SWSS_RC_SUCCESS);
  TestUtils::wait_for_finish(s, m_stack_unfrozen, kSelectTimeoutSeconds);
  m_init_thread.Join();

  test_thread.join();

  EXPECT_THAT(m_init_thread.GetDetailedStatus(),
              IsErrorStatus(InitThreadStatus::ErrorCondition::UNFREEZE_FAILED));
  check_final_failure_expects();
}

TEST_F(InitThreadTest, UnfreezeTimeout) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(m_critical_interface, report_minor_alarm(_)).Times(1);
  EXPECT_CALL(m_telemetry, record_overall_end(false)).Times(1);
  set_telemetry_stage_expects(WarmBootStage::STAGE_UNFREEZE,
                              /*success=*/false);

  swss::Select s;
  s.addSelectable(&m_stack_unfrozen);
  std::thread test_thread =
      std::thread(&InitThreadTest::advance_through_state_verification, this);

  overwrite_unfreeze_timeout(1);

  EXPECT_EQ(m_init_thread.Start(), swss::StatusCode::SWSS_RC_SUCCESS);
  TestUtils::wait_for_finish(s, m_stack_unfrozen, kSelectTimeoutSeconds);
  m_init_thread.Join();

  test_thread.join();

  EXPECT_THAT(m_init_thread.GetDetailedStatus(),
              IsErrorStatus(InitThreadStatus::ErrorCondition::UNFREEZE_FAILED));
  check_final_failure_expects();
}

TEST_F(InitThreadTest, UnfreezeAlreadyFailed) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(m_critical_interface, report_minor_alarm(_)).Times(1);
  EXPECT_CALL(m_telemetry, record_overall_end(false)).Times(1);

  swss::Select s;
  s.addSelectable(&m_stack_unfrozen);
  std::thread test_thread = std::thread(
      &InitThreadTest::advance_through_unfreeze_with_state, this, "failed");

  EXPECT_EQ(m_init_thread.Start(), swss::StatusCode::SWSS_RC_SUCCESS);
  TestUtils::wait_for_finish(s, m_stack_unfrozen, kSelectTimeoutSeconds);
  m_init_thread.Join();

  test_thread.join();

  EXPECT_THAT(m_init_thread.GetDetailedStatus(),
              IsErrorStatus(InitThreadStatus::ErrorCondition::UNFREEZE_FAILED));
  check_final_failure_expects();
}

TEST_F(InitThreadTest, UnfreezeAlreadyDone) {
  set_default_success_expects();

  swss::Select s;
  s.addSelectable(&m_stack_unfrozen);
  std::thread test_thread = std::thread(
      &InitThreadTest::advance_through_unfreeze_with_state, this, "completed");

  EXPECT_EQ(m_init_thread.Start(), swss::StatusCode::SWSS_RC_SUCCESS);
  TestUtils::wait_for_finish(s, m_stack_unfrozen, kSelectTimeoutSeconds);
  m_init_thread.Join();

  test_thread.join();

  EXPECT_THAT(m_init_thread.GetDetailedStatus(), IsDoneStatus());
  check_final_success_expects(/*state_verification_enabled=*/true);
}

class InitThreadTestWithSvResult
    : public InitThreadTest,
      public testing::WithParamInterface<std::string> {};

TEST_P(InitThreadTestWithSvResult, FullPassWorks) {
  set_default_success_expects();
  if (GetParam() == SV_NOT_RUN) {
    EXPECT_CALL(m_critical_interface, report_minor_alarm(_)).Times(1);
  }
  EXPECT_CALL(m_telemetry, record_stage_end(WarmBootStage::STAGE_RECONCILIATION,
                                            /*success=*/true))
      .Times(1);
  set_telemetry_stage_expects(WarmBootStage::STAGE_UNFREEZE,
                              /*success=*/true);

  populate_default_init_table();

  // Apps register one-by-one.
  auto test_sequence = [&] {
    // Registration step.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_THAT(m_init_thread.GetDetailedStatus(),
                IsActiveStatus(
                    InitThreadStatus::ThreadStatus::WAITING_FOR_REGISTRATION));
    TestUtils::populate_registration_table(m_db, "docker1|app1", false, true,
                                           true, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TestUtils::populate_registration_table(m_db, "docker2|app2", false, true,
                                           true, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TestUtils::populate_registration_table(m_db, "docker3|app3", false, true,
                                           true, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Reconciliation step.
    EXPECT_THAT(
        m_init_thread.GetDetailedStatus(),
        IsActiveStatus(
            InitThreadStatus::ThreadStatus::WAITING_FOR_RECONCILIATION));
    TestUtils::populate_restart_table_state(m_db, "app1", "reconciled");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TestUtils::populate_restart_table_state(m_db, "app2", "reconciled");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // Start listening for the verfication request just before completing
    // reconciliation.
    swss::NotificationConsumer nc(&m_db, STATE_VERIFICATION_REQ_CHANNEL);
    TestUtils::populate_restart_table_state(m_db, "app3", "reconciled");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // State verification step.
    EXPECT_THAT(
        m_init_thread.GetDetailedStatus(),
        IsActiveStatus(
            InitThreadStatus::ThreadStatus::WAITING_FOR_STATE_VERIFICATION));
    std::string timestamp = TestUtils::wait_for_state_verification_trigger(
        nc, kSelectTimeoutSeconds, /*freeze=*/true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TestUtils::write_state_verification_result(m_db, ALL_COMPONENT, GetParam(),
                                               "wrong_timestamp");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TestUtils::write_state_verification_result(m_db, "fake_component",
                                               GetParam(), "timestamp");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    swss::Select unfreeze_select;
    unfreeze_select.addSelectable(&m_stack_unfrozen);
    TestUtils::write_state_verification_result(m_db, ALL_COMPONENT, GetParam(),
                                               timestamp);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Unfreeze step.
    EXPECT_THAT(
        m_init_thread.GetDetailedStatus(),
        IsActiveStatus(InitThreadStatus::ThreadStatus::WAITING_FOR_UNFREEZE));
    TestUtils::wait_for_finish(unfreeze_select, m_stack_unfrozen,
                               kSelectTimeoutSeconds);
    TestUtils::populate_restart_table_state(m_db, "app1", "completed");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TestUtils::populate_restart_table_state(m_db, "app2", "completed");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TestUtils::populate_restart_table_state(m_db, "app3", "completed");
  };
  std::thread test_thread = std::thread(test_sequence);

  EXPECT_EQ(m_init_thread.Start(), swss::StatusCode::SWSS_RC_SUCCESS);

  swss::Select s;
  s.addSelectable(&m_finished);
  TestUtils::wait_for_finish(s, m_finished, kSelectTimeoutSeconds);

  test_thread.join();
  m_init_thread.Join();

  EXPECT_THAT(m_init_thread.GetDetailedStatus(), IsDoneStatus());
  check_final_success_expects(/*state_verification_enabled=*/true);
}

INSTANTIATE_TEST_SUITE_P(FullPassSuite, InitThreadTestWithSvResult,
                         testing::Values(SV_PASS, SV_NOT_RUN));

TEST_F(InitThreadTest, StateVerificationTimeoutIsSuccess) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(m_critical_interface, report_minor_alarm(_)).Times(1);
  EXPECT_CALL(m_telemetry, record_overall_end(true)).Times(1);

  advance_through_reconciliation();
  set_telemetry_stage_expects(WarmBootStage::STAGE_UNFREEZE,
                              /*success=*/true);

  overwrite_state_verification_timeout(1);

  std::string timestamp;
  auto test_sequence = [&] {
    swss::Select unfreeze_select;
    unfreeze_select.addSelectable(&m_stack_unfrozen);

    // State verification step. State Verification daemon reports no results
    // and times out.
    swss::NotificationConsumer nc(&m_db, STATE_VERIFICATION_REQ_CHANNEL);
    timestamp = TestUtils::wait_for_state_verification_trigger(
        nc, kSelectTimeoutSeconds, /*freeze=*/true);

    // Unfreeze step.
    TestUtils::wait_for_finish(unfreeze_select, m_stack_unfrozen,
                               kSelectTimeoutSeconds);
    EXPECT_THAT(
        m_init_thread.GetDetailedStatus(),
        IsActiveStatus(InitThreadStatus::ThreadStatus::WAITING_FOR_UNFREEZE));
    TestUtils::populate_restart_table_state(m_db, "app1", "completed");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TestUtils::populate_restart_table_state(m_db, "app2", "completed");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TestUtils::populate_restart_table_state(m_db, "app3", "completed");
  };
  std::thread test_thread = std::thread(test_sequence);

  EXPECT_EQ(m_init_thread.Start(), swss::StatusCode::SWSS_RC_SUCCESS);

  swss::Select s;
  s.addSelectable(&m_finished);
  TestUtils::wait_for_finish(s, m_finished, kSelectTimeoutSeconds);

  test_thread.join();
  m_init_thread.Join();

  EXPECT_THAT(m_init_thread.GetDetailedStatus(), IsDoneStatus());
  check_final_success_expects(/*state_verification_enabled=*/true);
}

TEST_F(InitThreadTest, FullPassNoStateVerification) {
  TestUtils::set_state_verification_enable(m_config_db, /*bootup=*/true,
                                           /*enabled=*/false);

  set_default_success_expects();
  EXPECT_CALL(m_telemetry, record_stage_end(WarmBootStage::STAGE_RECONCILIATION,
                                            /*success=*/true))
      .Times(1);

  populate_default_init_table();

  // Apps register one-by-one.
  auto test_sequence = [&] {
    // Registration step.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TestUtils::populate_registration_table(m_db, "docker1|app1", false, true,
                                           true, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TestUtils::populate_registration_table(m_db, "docker2|app2", false, true,
                                           true, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TestUtils::populate_registration_table(m_db, "docker3|app3", false, true,
                                           true, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Reconciliation step.
    TestUtils::populate_restart_table_state(m_db, "app1", "reconciled");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TestUtils::populate_restart_table_state(m_db, "app2", "reconciled");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // Start listening for the verfication request just before completing
    // reconciliation.
    swss::Select unfreeze_select;
    unfreeze_select.addSelectable(&m_stack_unfrozen);
    swss::NotificationConsumer nc(&m_db, STATE_VERIFICATION_REQ_CHANNEL);
    TestUtils::populate_restart_table_state(m_db, "app3", "reconciled");

    // No state verification signal.
    TestUtils::confirm_no_state_verification_trigger(
        nc, kShortSelectTimeoutSeconds);
  };
  std::thread test_thread = std::thread(test_sequence);

  EXPECT_EQ(m_init_thread.Start(), swss::StatusCode::SWSS_RC_SUCCESS);
  swss::Select s;
  s.addSelectable(&m_finished);
  TestUtils::wait_for_finish(s, m_finished, kSelectTimeoutSeconds);

  m_init_thread.Join();
  test_thread.join();

  EXPECT_THAT(m_init_thread.GetDetailedStatus(), IsDoneStatus());
  check_final_success_expects(/*state_verification_enabled=*/false);
}

}  // namespace rebootbackend
