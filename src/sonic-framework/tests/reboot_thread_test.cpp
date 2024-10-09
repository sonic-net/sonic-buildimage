#include "reboot_thread.h"

#include <gmock/gmock.h>
#include <google/protobuf/util/json_util.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <string>
#include <thread>
#include <vector>

#include "container_stop.pb.h"
#include "mock_reboot_interfaces.h"
#include "reboot_common.h"
#include "reboot_interfaces.h"
#include "redis_utils.h"
#include "select.h"
#include "selectableevent.h"
#include "stateverification.h"
#include "status_code_util.h"
#include "system/system.pb.h"
#include "test_utils_common.h"
#include "timestamp.h"
#include "warm_restart.h"

namespace rebootbackend {

#define TENTH_SECOND_MS (100)

using namespace gnoi::system;
namespace gpu = ::google::protobuf::util;
using WarmStartState = ::swss::WarmStart::WarmStartState;
using Progress = ::rebootbackend::RebootThread::Progress;
using RebootThread = ::rebootbackend::RebootThread;
using ::testing::_;
using ::testing::ExplainMatchResult;
using ::testing::HasSubstr;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::StrictMock;

MATCHER_P2(IsStatus, status, message, "") {
  return (arg.status().status() == status &&
          ExplainMatchResult(message, arg.status().message(), result_listener));
}

class RebootStatusTest : public ::testing::Test {
 protected:
  RebootStatusTest() : m_status() {}
  ThreadStatus m_status;
};

TEST_F(RebootStatusTest, TestInit) {
  RebootStatusResponse response = m_status.get_response();

  EXPECT_FALSE(response.active());
  EXPECT_THAT(response.reason(), StrEq(""));
  EXPECT_EQ(response.count(), 0);
  EXPECT_EQ(response.method(), RebootMethod::UNKNOWN);
  EXPECT_EQ(response.status().status(),
            RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN);
  EXPECT_THAT(response.status().message(), StrEq(""));

  EXPECT_FALSE(m_status.get_active());
}
TEST_F(RebootStatusTest, TestStartStatus) {
  m_status.set_start_status(RebootMethod::NSF, "reboot because");

  RebootStatusResponse response = m_status.get_response();

  EXPECT_TRUE(response.active());
  EXPECT_THAT(response.reason(), StrEq("reboot because"));
  EXPECT_EQ(response.count(), 1);
  EXPECT_EQ(response.method(), RebootMethod::NSF);
  EXPECT_THAT(response.status().message(), StrEq(""));

  EXPECT_TRUE(m_status.get_active());
}

TEST_F(RebootStatusTest, TestSets) {
  m_status.set_start_status(RebootMethod::NSF, "reboot because");

  RebootStatus_Status reboot_status = m_status.get_last_reboot_status();
  EXPECT_EQ(reboot_status,
            RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN);

  m_status.set_completed_status(
      RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE, "timeout");

  // Have to be inactive to read the status
  m_status.set_inactive();
  EXPECT_FALSE(m_status.get_active());

  RebootStatusResponse response = m_status.get_response();
  EXPECT_THAT(response.status().message(), StrEq("timeout"));
  EXPECT_EQ(response.status().status(),
            RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE);

  // Can't set message status while inactive
  m_status.set_completed_status(
      RebootStatus_Status::RebootStatus_Status_STATUS_RETRIABLE_FAILURE,
      "anything");
  response = m_status.get_response();
  EXPECT_THAT(response.status().message(), StrEq("timeout"));
  EXPECT_EQ(response.status().status(),
            RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE);
}

TEST_F(RebootStatusTest, TestGetStatus) {
  std::chrono::nanoseconds curr_ns = std::chrono::high_resolution_clock::now().time_since_epoch();

  m_status.set_start_status(RebootMethod::COLD, "reboot because");

  RebootStatusResponse response = m_status.get_response();
  EXPECT_EQ(response.status().status(),
            RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN);

  m_status.set_completed_status(
      RebootStatus_Status::RebootStatus_Status_STATUS_SUCCESS, "anything");

  response = m_status.get_response();

  // message should be empty while reboot is active
  EXPECT_THAT(response.status().message(), StrEq(""));

  uint64_t reboot_ns = response.when();
  EXPECT_TRUE(reboot_ns > (uint64_t)curr_ns.count());

  m_status.set_inactive();
  response = m_status.get_response();
  EXPECT_THAT(response.status().message(), StrEq("anything"));
  EXPECT_EQ(response.status().status(),
            RebootStatus_Status::RebootStatus_Status_STATUS_SUCCESS);
  EXPECT_EQ(0, response.when());
}

class RebootThreadTest : public ::testing::Test {
 protected:
  RebootThreadTest()
      : m_dbus_interface(),
        m_critical_interface(),
        m_db("STATE_DB", 0),
        m_config_db("CONFIG_DB", 0),
        m_reboot_thread(m_dbus_interface, m_critical_interface, m_telemetry,
                        m_finished) {
    swss::WarmStart::initialize("app1", "docker1");
    TestUtils::clear_tables(m_db);
    sigterm_requested = false;
  }

  void overwrite_reboot_timeout(uint32_t timeout_seconds) {
    m_reboot_thread.m_reboot_timeout = timeout_seconds;
  }

  void overwrite_state_verification_timeout(uint32_t timeout_seconds) {
    m_reboot_thread.m_state_verification_timeout = timeout_seconds;
  }

  void overwrite_quiescence_timeout_ms(uint32_t timeout_ms) {
    m_reboot_thread.m_quiescence_timeout_ms = timeout_ms;
  }

  void overwrite_quiescence_hold_time_ms(uint32_t timeout_ms) {
    m_reboot_thread.m_quiescence_hold_time_ms = timeout_ms;
  }

  void overwrite_checkpoint_timeout(uint32_t timeout_seconds) {
    m_reboot_thread.m_checkpoint_timeout = timeout_seconds;
  }

  RebootStatusResponse get_response(void) {
    return m_reboot_thread.m_status.get_response();
  }

  void set_start_status(const RebootMethod &method, const std::string &reason) {
    return m_reboot_thread.m_status.set_start_status(method, reason);
  }

  void set_completed_status(const RebootStatus_Status &status,
                            const std::string &message) {
    return m_reboot_thread.m_status.set_completed_status(status, message);
  }

  void force_inactive(void) { return m_reboot_thread.m_status.set_inactive(); }

  void force_active(void) { return m_reboot_thread.m_status.set_inactive(); }

  void do_reboot(void) { return m_reboot_thread.do_reboot(); }

  Progress wait_for_platform_reboot(swss::Select &s) {
    return m_reboot_thread.wait_for_platform_reboot(s);
  }

  Progress perform_state_verification(swss::Select &s) {
    return m_reboot_thread.perform_state_verification(s);
  }

  Progress perform_state_verification_select(swss::Select &s,
                                             swss::SelectableTimer &l_timer,
                                             swss::SubscriberStateTable &sub,
                                             std::string &timestamp) {
    return m_reboot_thread.state_verification_select(s, l_timer, sub,
                                                     timestamp);
  }

  RebootThread::Status handle_state_verification_event(
      swss::SubscriberStateTable &sub, std::string &timestamp) {
    return m_reboot_thread.handle_state_verification_event(sub, timestamp);
  }

  // stop: set to true if calling m_reboot_thread.Stop() is desired.
  // timeout_ms: replaces m_quiescence_timout_ms if > 0
  Progress perform_freeze_quiescence_w_stop(bool stop, int timeout_ms = -1) {
    timespec l_timespec;
    if (timeout_ms >= 0) {
      l_timespec = milliseconds_to_timespec(timeout_ms);
    } else {
      l_timespec =
          milliseconds_to_timespec(m_reboot_thread.m_quiescence_timeout_ms);
    }

    swss::SelectableTimer l_timer(l_timespec);

    swss::Select s;
    s.addSelectable(&l_timer);
    l_timer.start();
    s.addSelectable(&(return_m_stop_reference()));

    if (stop) {
      m_reboot_thread.Stop();
    }
    return m_reboot_thread.perform_freeze_quiescence(s, l_timer);
  }

  Progress checkpoint_stage_two(swss::Select &s, swss::SelectableTimer &l_timer,
                                swss::SubscriberStateTable &sub) {
    return m_reboot_thread.checkpoint_stage_two(s, l_timer, sub);
  }

  Progress perform_checkpoint(swss::Select &s) {
    return m_reboot_thread.perform_checkpoint(s);
  }

  RebootThread::Status check_container_stop(const std::string &request_id) {
    return m_reboot_thread.check_container_stop(request_id);
  }

  Progress wait_for_container_stop(int timeout_ms = -1) {
    timespec l_timespec;
    if (timeout_ms >= 0) {
      l_timespec = milliseconds_to_timespec(timeout_ms);
    } else {
      l_timespec =
          milliseconds_to_timespec(m_reboot_thread.m_quiescence_timeout_ms);
    }

    swss::SelectableTimer l_timer(l_timespec);

    swss::Select s;
    s.addSelectable(&l_timer);
    l_timer.start();
    s.addSelectable(&(return_m_stop_reference()));

    m_reboot_thread.Stop();

    return m_reboot_thread.wait_for_container_stop(s, "reqA", l_timer);
  }

  void fetch_registration_info() {
    m_reboot_thread.m_registration.fetch_registration_info();
  }

  swss::SelectableEvent &return_m_stop_reference() {
    return m_reboot_thread.m_stop;
  }

  swss::DBConnector m_db;
  swss::DBConnector m_config_db;
  NiceMock<MockDbusInterface> m_dbus_interface;
  NiceMock<MockCriticalStateInterface> m_critical_interface;
  StrictMock<MockTelemetryInterface> m_telemetry;
  swss::SelectableEvent m_finished;
  RebootThread m_reboot_thread;
};

MATCHER_P2(Status, status, message, "") {
  return (arg.status().status() == status && arg.status().message() == message);
}

TEST_F(RebootThreadTest, TestStop) {
  EXPECT_CALL(m_dbus_interface, Reboot(_))
      .Times(1)
      .WillOnce(Return(DbusInterface::DbusResponse{
          DbusInterface::DbusStatus::DBUS_SUCCESS, ""}));
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(1)
      .WillOnce(Return(false));
  EXPECT_CALL(m_critical_interface,
              report_critical_state("platform failed to reboot"))
      .Times(0);
  RebootRequest request;
  request.set_method(RebootMethod::COLD);
  overwrite_reboot_timeout(2);
  m_reboot_thread.Start(request);
  m_reboot_thread.Stop();
  m_reboot_thread.Join();
  gnoi::system::RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(
      response,
      IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN, ""));
}

TEST_F(RebootThreadTest, TestCleanExit) {
  EXPECT_CALL(m_dbus_interface, Reboot(_))
      .Times(1)
      .WillOnce(Return(DbusInterface::DbusResponse{
          DbusInterface::DbusStatus::DBUS_SUCCESS, ""}));
  EXPECT_CALL(m_critical_interface,
              report_critical_state(("platform failed to reboot")))
      .Times(1);
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(1)
      .WillOnce(Return(false));
  overwrite_reboot_timeout(1);

  swss::Select s;
  s.addSelectable(&m_finished);

  RebootRequest request;
  request.set_method(RebootMethod::COLD);
  request.set_message("time to reboot");
  m_reboot_thread.Start(request);
  TestUtils::wait_for_finish(s, m_finished, 5);

  // Status should be active until we call join
  RebootStatusResponse response = get_response();
  EXPECT_TRUE(response.active());
  EXPECT_THAT(response.reason(), StrEq("time to reboot"));
  EXPECT_EQ(response.count(), 1);

  EXPECT_THAT(response.status().message(), StrEq(""));

  m_reboot_thread.Join();

  response = get_response();
  EXPECT_FALSE(response.active());
  EXPECT_THAT(response.status().message(), StrEq("platform failed to reboot"));
}

TEST_F(RebootThreadTest, TestJoinWithoutStart) {
  bool ret = m_reboot_thread.Join();
  EXPECT_FALSE(ret);
}

// Call Start a second time while first thread is still executing.
TEST_F(RebootThreadTest, TestStartWhileRunning) {
  EXPECT_CALL(m_dbus_interface, Reboot(_))
      .Times(1)
      .WillOnce(Return(DbusInterface::DbusResponse{
          DbusInterface::DbusStatus::DBUS_SUCCESS, ""}));
  EXPECT_CALL(m_critical_interface,
              report_critical_state("platform failed to reboot"))
      .Times(1);
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(1)
      .WillOnce(Return(false));
  overwrite_reboot_timeout(2);

  RebootRequest request;
  request.set_method(RebootMethod::COLD);
  request.set_message("time to reboot");
  m_reboot_thread.Start(request);

  // First thread is still running ...
  NotificationResponse response = m_reboot_thread.Start(request);
  EXPECT_EQ(response.status, swss::StatusCode::SWSS_RC_IN_USE);
  EXPECT_THAT(response.json_string,
              StrEq("RebootThread: can't Start while active"));

  bool ret = m_reboot_thread.Join();
  EXPECT_TRUE(ret);
}

// Call Start a second time after first thread completed
// but before first thread was joined.
// Second start should fail.
TEST_F(RebootThreadTest, TestStartWithoutJoin) {
  EXPECT_CALL(m_dbus_interface, Reboot(_))
      .Times(1)
      .WillOnce(Return(DbusInterface::DbusResponse{
          DbusInterface::DbusStatus::DBUS_SUCCESS, ""}));
  EXPECT_CALL(m_critical_interface,
              report_critical_state("platform failed to reboot"))
      .Times(1);
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(1)
      .WillOnce(Return(false));
  overwrite_reboot_timeout(1);

  swss::Select s;
  s.addSelectable(&m_finished);

  RebootRequest request;
  request.set_method(RebootMethod::COLD);
  request.set_message("time to reboot");
  m_reboot_thread.Start(request);
  TestUtils::wait_for_finish(s, m_finished, 3);

  // First thread has stopped: we need to join before
  // restart will succeed
  NotificationResponse response = m_reboot_thread.Start(request);
  EXPECT_EQ(response.status, swss::StatusCode::SWSS_RC_IN_USE);

  // This should join the first start.
  bool ret = m_reboot_thread.Join();
  EXPECT_TRUE(ret);
}

TEST_F(RebootThreadTest, TestUnsupportedRebootType) {
  RebootRequest request;
  request.set_method(RebootMethod::POWERDOWN);

  NotificationResponse response = m_reboot_thread.Start(request);
  EXPECT_EQ(response.status, swss::StatusCode::SWSS_RC_INVALID_PARAM);
  EXPECT_EQ(response.json_string,
            "RebootThread: Start rx'd unsupported method");
}

TEST_F(RebootThreadTest, TestDelayedStartUnsupported) {
  RebootRequest request;
  request.set_method(RebootMethod::NSF);
  request.set_delay(1);

  NotificationResponse response = m_reboot_thread.Start(request);
  EXPECT_EQ(response.status, swss::StatusCode::SWSS_RC_INVALID_PARAM);
  EXPECT_THAT(response.json_string,
              StrEq("RebootThread: delayed start not supported"));
}

TEST_F(RebootThreadTest, TestNoNSFIfNonRetriableFailure) {
  set_start_status(RebootMethod::NSF, "time to reboot");
  set_completed_status(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
                       "platform failed to reboot");
  force_inactive();

  RebootRequest request;
  request.set_method(RebootMethod::NSF);

  NotificationResponse response = m_reboot_thread.Start(request);
  EXPECT_EQ(response.status, swss::StatusCode::SWSS_RC_FAILED_PRECONDITION);
  EXPECT_EQ(response.json_string,
            "RebootThread: last NSF failed with non-retriable failure");
}

TEST_F(RebootThreadTest, TestNoNSFIfSystemCritical) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(1)
      .WillOnce(Return(true));
  set_start_status(RebootMethod::NSF, "time to reboot");

  force_inactive();

  RebootRequest request;
  request.set_method(RebootMethod::NSF);

  NotificationResponse response = m_reboot_thread.Start(request);
  EXPECT_EQ(response.status, swss::StatusCode::SWSS_RC_FAILED_PRECONDITION);
  EXPECT_EQ(response.json_string,
            "RebootThread: in critical state, NSF not allowed");
}

TEST_F(RebootThreadTest, TestSigTermStartofDoReboot) {
  sigterm_requested = true;
  set_start_status(RebootMethod::NSF, "time to reboot");
  do_reboot();
  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(
      response,
      IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN, ""));
}

TEST_F(RebootThreadTest, TestInvalidMethodfDoReboot) {
  set_start_status(RebootMethod::POWERUP, "time to reboot");
  do_reboot();
  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(
      response,
      IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN, ""));
}

TEST_F(RebootThreadTest, TestWaitForRebootPositive) {
  overwrite_reboot_timeout(1);
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(1)
      .WillOnce(Return(false));
  set_start_status(RebootMethod::NSF, "time to reboot");
  swss::Select s;
  swss::SelectableEvent m_stop;
  s.addSelectable(&m_stop);
  RebootThread::Progress progress = wait_for_platform_reboot(s);
  EXPECT_EQ(progress, RebootThread::Progress::PROCEED);
  // EXPECT_EQ(progress, RebootThread::Progress::EXIT_EARLY);
}

TEST_F(RebootThreadTest, TestWaitForRebootCriticalState) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(1)
      .WillOnce(Return(true));
  set_start_status(RebootMethod::NSF, "time to reboot");
  swss::Select s;
  swss::SelectableEvent m_stop;
  s.addSelectable(&m_stop);
  RebootThread::Progress progress = wait_for_platform_reboot(s);
  EXPECT_EQ(progress, RebootThread::Progress::EXIT_EARLY);
  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(
      response,
      IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
               "system entered critical state after platfrom reboot request"));
}

TEST_F(RebootThreadTest, TestWaitForRebootRxStop) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(1)
      .WillOnce(Return(false));
  set_start_status(RebootMethod::NSF, "time to reboot");

  swss::Select s;
  s.addSelectable(&(return_m_stop_reference()));
  m_reboot_thread.Stop();
  RebootThread::Progress progress = wait_for_platform_reboot(s);
  EXPECT_EQ(progress, RebootThread::Progress::EXIT_EARLY);
  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(
      response,
      IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN, ""));
}

//
// State Verification Tests
//

TEST_F(RebootThreadTest, TestStateVerificationCriticalState) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(1)
      .WillOnce(Return(true));
  set_start_status(RebootMethod::NSF, "time to reboot");

  swss::Select s;
  s.addSelectable(&(return_m_stop_reference()));

  RebootThread::Progress progress = perform_state_verification(s);
  EXPECT_EQ(progress, RebootThread::Progress::EXIT_EARLY);

  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(
      response,
      IsStatus(
          RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
          "system entered critical state before reboot state verification"));
}

TEST_F(RebootThreadTest, TestStateVerificationDisabled) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(1)
      .WillOnce(Return(false));

  set_start_status(RebootMethod::NSF, "time to reboot");

  swss::Table warmRestartTable(&m_config_db, CFG_WARM_RESTART_TABLE_NAME);
  warmRestartTable.hset("system", "state_verification_shutdown", "false");

  swss::Select s;
  s.addSelectable(&(return_m_stop_reference()));

  RebootThread::Progress progress = perform_state_verification(s);
  EXPECT_EQ(progress, RebootThread::Progress::PROCEED);

  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(
      response,
      IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN, ""));
}

TEST_F(RebootThreadTest, TestStateVerificationTimeout) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(1)
      .WillOnce(Return(false));

  overwrite_state_verification_timeout(1);

  set_start_status(RebootMethod::NSF, "time to reboot");

  swss::Table warmRestartTable(&m_config_db, CFG_WARM_RESTART_TABLE_NAME);
  warmRestartTable.hset("system", "state_verification_shutdown", "true");

  swss::Select s;
  s.addSelectable(&(return_m_stop_reference()));

  RebootThread::Progress progress = perform_state_verification(s);
  EXPECT_EQ(progress, RebootThread::Progress::EXIT_EARLY);

  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(
      response,
      IsStatus(
          RebootStatus_Status::RebootStatus_Status_STATUS_RETRIABLE_FAILURE,
          "timeout occurred during reboot state verification: retriable "
          "error"));
}

TEST_F(RebootThreadTest, TestStateVerificationStop) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(1)
      .WillOnce(Return(false));

  overwrite_state_verification_timeout(1);

  set_start_status(RebootMethod::NSF, "time to reboot");

  swss::Table warmRestartTable(&m_config_db, CFG_WARM_RESTART_TABLE_NAME);
  warmRestartTable.hset("system", "state_verification_shutdown", "true");

  swss::Select s;
  s.addSelectable(&(return_m_stop_reference()));
  m_reboot_thread.Stop();

  RebootThread::Progress progress = perform_state_verification(s);
  EXPECT_EQ(progress, RebootThread::Progress::EXIT_EARLY);

  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(
      response,
      IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN, ""));
}

TEST_F(RebootThreadTest, TestStateVerificationSelectTimeout) {
  set_start_status(RebootMethod::NSF, "time to reboot");

  swss::Select s;
  s.addSelectable(&(return_m_stop_reference()));

  swss::SelectableTimer l_timer(timespec{.tv_sec = 1, .tv_nsec = 0});
  s.addSelectable(&l_timer);

  swss::SubscriberStateTable sub(&m_db, STATE_VERIFICATION_RESP_TABLE);
  s.addSelectable(&sub);

  l_timer.start();

  std::string timestamp = "timestamp-b";
  TestUtils::write_state_verification_result(m_db, ALL_COMPONENT, SV_PASS,
                                             "wrong-timestamp");

  RebootThread::Progress progress =
      perform_state_verification_select(s, l_timer, sub, timestamp);

  EXPECT_EQ(progress, RebootThread::Progress::EXIT_EARLY);

  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(
      response,
      IsStatus(
          RebootStatus_Status::RebootStatus_Status_STATUS_RETRIABLE_FAILURE,
          "timeout occurred during reboot state verification: retriable "
          "error"));
}

TEST_F(RebootThreadTest, TestStateVerificationSelectTimeoutNotRun) {
  set_start_status(RebootMethod::NSF, "time to reboot");

  swss::Select s;
  s.addSelectable(&(return_m_stop_reference()));

  swss::SelectableTimer l_timer(timespec{.tv_sec = 1, .tv_nsec = 0});
  s.addSelectable(&l_timer);

  swss::SubscriberStateTable sub(&m_db, STATE_VERIFICATION_RESP_TABLE);
  s.addSelectable(&sub);

  l_timer.start();

  std::string timestamp = "timestamp-b";
  TestUtils::write_state_verification_result(m_db, ALL_COMPONENT, SV_NOT_RUN,
                                             timestamp);

  RebootThread::Progress progress =
      perform_state_verification_select(s, l_timer, sub, timestamp);

  EXPECT_EQ(progress, RebootThread::Progress::EXIT_EARLY);
  EXPECT_NE("timestamp-b", timestamp);

  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(
      response,
      IsStatus(
          RebootStatus_Status::RebootStatus_Status_STATUS_RETRIABLE_FAILURE,
          "timeout occurred during reboot state verification: retriable "
          "error"));
}

TEST_F(RebootThreadTest, TestHandleStateVerificationKeepWaiting) {
  set_start_status(RebootMethod::NSF, "time to reboot");

  swss::Select s;
  swss::SubscriberStateTable sub(&m_db, STATE_VERIFICATION_RESP_TABLE);
  s.addSelectable(&sub);
  swss::Selectable *sel;

  // Unrecognized component returns KEEP_WAITING
  std::string timestamp = "timestamp-a";
  TestUtils::write_state_verification_result(m_db, "not-all-component",
                                             "dont care", timestamp);
  s.select(&sel);
  RebootThread::Status status = handle_state_verification_event(sub, timestamp);
  EXPECT_EQ(status, RebootThread::Status::KEEP_WAITING);

  // Wrong timestamp returns KEEP_WAITING
  TestUtils::write_state_verification_result(m_db, ALL_COMPONENT, SV_PASS,
                                             "wrong-timestamp");
  s.select(&sel);
  status = handle_state_verification_event(sub, timestamp);
  EXPECT_EQ(status, RebootThread::Status::KEEP_WAITING);

  // Unrecognized status + correct timestamp = KEEP_WAITING
  TestUtils::write_state_verification_result(m_db, ALL_COMPONENT,
                                             "undefined-status", timestamp);
  s.select(&sel);
  status = handle_state_verification_event(sub, timestamp);
  EXPECT_EQ(status, RebootThread::Status::KEEP_WAITING);

  // If we receive NOT_RUN as a status: we KEEP_WAITING
  // timestamp should be updated with new value after re-request of state
  // verification
  TestUtils::write_state_verification_result(m_db, ALL_COMPONENT, SV_NOT_RUN,
                                             timestamp);
  s.select(&sel);
  status = handle_state_verification_event(sub, timestamp);
  EXPECT_EQ(status, RebootThread::Status::KEEP_WAITING);
  EXPECT_NE(timestamp, "timestamp-a");
}

TEST_F(RebootThreadTest, TestHandleStateVerificationSuccess) {
  set_start_status(RebootMethod::NSF, "time to reboot");

  swss::Select s;
  swss::SubscriberStateTable sub(&m_db, STATE_VERIFICATION_RESP_TABLE);
  s.addSelectable(&sub);
  swss::Selectable *sel;

  // Pass + Correct timestamp == SUCCESS
  std::string timestamp = "timestamp-b";
  TestUtils::write_state_verification_result(m_db, ALL_COMPONENT, SV_PASS,
                                             timestamp);
  int select_ret = s.select(&sel);
  EXPECT_EQ(select_ret, swss::Select::OBJECT);
  EXPECT_EQ(sel, &sub);
  RebootThread::Status status = handle_state_verification_event(sub, timestamp);
  EXPECT_EQ(status, RebootThread::Status::SUCCESS);
}

TEST_F(RebootThreadTest, TestHandleStateVerificationFail) {
  set_start_status(RebootMethod::NSF, "time to reboot");

  swss::Select s;
  swss::SubscriberStateTable sub(&m_db, STATE_VERIFICATION_RESP_TABLE);
  s.addSelectable(&sub);
  swss::Selectable *sel;

  // Fail with correct timestampe = FAILURE
  // status and message are updated
  std::string timestamp = "timestamp-b";
  TestUtils::write_state_verification_result(m_db, ALL_COMPONENT, SV_FAIL,
                                             timestamp);
  s.select(&sel);
  RebootThread::Status status = handle_state_verification_event(sub, timestamp);
  EXPECT_EQ(status, RebootThread::Status::FAILURE);

  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(response,
              IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
                       "state verification failed during reboot"));
}

//
// Quiescence Tests
//

TEST_F(RebootThreadTest, TestPerformFreezeQuiescenceCriticalState) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(1)
      .WillOnce(Return(true));
  set_start_status(RebootMethod::NSF, "time to reboot");

  Progress progress = perform_freeze_quiescence_w_stop(false);
  EXPECT_EQ(progress, RebootThread::Progress::EXIT_EARLY);
  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(response,
              IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
                       "system entered critical state before freezing"));
}

TEST_F(RebootThreadTest, TestPerformFreezeQuiescenceTimeout) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(1)
      .WillOnce(Return(false));

  TestUtils::populate_registration_table(m_db, "docker1|app1", false, true,
                                         false, false);
  TestUtils::populate_registration_table(m_db, "docker2|app2", false, true,
                                         false, false);
  fetch_registration_info();

  overwrite_quiescence_timeout_ms(300);

  set_start_status(RebootMethod::NSF, "time to reboot");

  Progress progress = perform_freeze_quiescence_w_stop(false);
  EXPECT_EQ(progress, RebootThread::Progress::EXIT_EARLY);

  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(response,
              IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
                       "timeout occurred during reboot stage freeze"));
}

TEST_F(RebootThreadTest, TestPerformFreezeQuiescenceStop) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(1)
      .WillOnce(Return(false));

  TestUtils::populate_registration_table(m_db, "docker1|app1", false, true,
                                         false, false);
  TestUtils::populate_registration_table(m_db, "docker2|app2", false, true,
                                         false, false);
  fetch_registration_info();
  overwrite_quiescence_timeout_ms(1000);

  set_start_status(RebootMethod::NSF, "time to reboot");

  Progress progress = perform_freeze_quiescence_w_stop(true);
  EXPECT_EQ(progress, RebootThread::Progress::EXIT_EARLY);

  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();

  // No error on request to stop, just log.
  EXPECT_THAT(
      response,
      IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN, ""));
}

TEST_F(RebootThreadTest, TestPerformFreezeQuiescenceStartCompleted) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(1)
      .WillOnce(Return(false));

  TestUtils::populate_registration_table(m_db, "docker1|app1", false, true,
                                         false, false);
  TestUtils::populate_registration_table(m_db, "docker2|app2", false, true,
                                         false, false);
  TestUtils::populate_restart_table_state(
      m_db, "app1", get_warm_start_state_name(WarmStartState::CHECKPOINTED));
  TestUtils::populate_restart_table_state(
      m_db, "app2", get_warm_start_state_name(WarmStartState::QUIESCENT));
  fetch_registration_info();
  overwrite_quiescence_hold_time_ms(100);

  set_start_status(RebootMethod::NSF, "time to reboot");

  Progress progress = perform_freeze_quiescence_w_stop(false, 1000);
  EXPECT_EQ(progress, RebootThread::Progress::PROCEED);
}

TEST_F(RebootThreadTest,
       TestPerformFreezeQuiescenceUninterestingStatesAtStart) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(1)
      .WillOnce(Return(false));

  TestUtils::populate_registration_table(m_db, "docker2|app2", false, true,
                                         false, false);
  TestUtils::populate_restart_table_state(
      m_db, "app2", get_warm_start_state_name(WarmStartState::RECONCILED));
  TestUtils::populate_restart_table_state(
      m_db, "app2", get_warm_start_state_name(WarmStartState::INITIALIZED));
  TestUtils::populate_restart_table_state(
      m_db, "app2", get_warm_start_state_name(WarmStartState::FROZEN));
  fetch_registration_info();
  overwrite_quiescence_timeout_ms(1000);

  set_start_status(RebootMethod::NSF, "time to reboot");

  Progress progress = perform_freeze_quiescence_w_stop(false);
  EXPECT_EQ(progress, RebootThread::Progress::EXIT_EARLY);

  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(response,
              IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
                       "timeout occurred during reboot stage freeze"));
}

TEST_F(RebootThreadTest, TestPerformFreezeQuiescenceStartFailed) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(1)
      .WillOnce(Return(false));

  TestUtils::populate_registration_table(m_db, "docker1|app1", false, true,
                                         false, false);
  TestUtils::populate_registration_table(m_db, "docker2|app2", false, true,
                                         false, false);
  TestUtils::populate_restart_table_state(
      m_db, "app1", get_warm_start_state_name(WarmStartState::CHECKPOINTED));
  TestUtils::populate_restart_table_state(
      m_db, "app2", get_warm_start_state_name(WarmStartState::FAILED));
  fetch_registration_info();
  overwrite_quiescence_timeout_ms(500);

  set_start_status(RebootMethod::NSF, "time to reboot");

  Progress progress = perform_freeze_quiescence_w_stop(true);
  EXPECT_EQ(progress, RebootThread::Progress::EXIT_EARLY);

  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(
      response,
      IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
               "check_stage: app: app2 reported FAILED during stage: freeze"));
}

TEST_F(RebootThreadTest, TestPerformFreezeQuiescenceCompleted) {
  overwrite_quiescence_hold_time_ms(100);
  TestUtils::populate_registration_table(m_db, "docker1|app1", false, true,
                                         false, false);
  TestUtils::populate_registration_table(m_db, "docker2|app2", false, true,
                                         false, false);
  fetch_registration_info();

  set_start_status(RebootMethod::NSF, "time to reboot");
  auto test_sequence = [&] {
    // We want to skip past the initial completed check at start of
    // freeze_quiescence_select and process below as subscriptions updates
    std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));
    TestUtils::populate_restart_table_state(
        m_db, "app1", get_warm_start_state_name(WarmStartState::CHECKPOINTED));
    TestUtils::populate_restart_table_state(
        m_db, "app2", get_warm_start_state_name(WarmStartState::CHECKPOINTED));
  };

  std::thread test_thread = std::thread(test_sequence);

  Progress progress = perform_freeze_quiescence_w_stop(false);
  EXPECT_EQ(progress, Progress::PROCEED);
  test_thread.join();
}

TEST_F(RebootThreadTest,
       TestPerformFreezeQuiescenceUninterestingStatesViaSubscription) {
  TestUtils::populate_registration_table(m_db, "docker1|app1", false, true,
                                         false, false);
  fetch_registration_info();

  set_start_status(RebootMethod::NSF, "time to reboot");

  auto test_sequence = [&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));
    TestUtils::populate_restart_table_state(
        m_db, "app1", get_warm_start_state_name(WarmStartState::RECONCILED));
    TestUtils::populate_restart_table_state(
        m_db, "app1", get_warm_start_state_name(WarmStartState::WSDISABLED));
    TestUtils::populate_restart_table_state(
        m_db, "app1", get_warm_start_state_name(WarmStartState::WSUNKNOWN));
  };

  std::thread test_thread = std::thread(test_sequence);

  Progress progress = perform_freeze_quiescence_w_stop(false, 300);
  EXPECT_EQ(progress, Progress::EXIT_EARLY);

  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(response,
              IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
                       "timeout occurred during reboot stage freeze"));
  test_thread.join();
}

TEST_F(RebootThreadTest, TestPerformFreezeQuiescenceFailed) {
  TestUtils::populate_registration_table(m_db, "docker1|app1", false, true,
                                         false, false);
  TestUtils::populate_registration_table(m_db, "docker2|app2", false, true,
                                         false, false);
  fetch_registration_info();

  set_start_status(RebootMethod::NSF, "time to reboot");

  auto test_sequence = [&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));
    TestUtils::populate_restart_table_state(
        m_db, "app1", get_warm_start_state_name(WarmStartState::CHECKPOINTED));
    TestUtils::populate_restart_table_state(
        m_db, "app2", get_warm_start_state_name(WarmStartState::FAILED));
  };

  std::thread test_thread = std::thread(test_sequence);

  Progress progress = perform_freeze_quiescence_w_stop(false, 500);
  EXPECT_EQ(progress, Progress::EXIT_EARLY);

  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(response,
              IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
                       "handle_state_event: app: app2 reported FAILED when "
                       "looking for state: freeze"));
  test_thread.join();
}

TEST_F(RebootThreadTest, TestQuiescenceTimeoutDuringHoldTime) {
  overwrite_quiescence_hold_time_ms(2000);

  TestUtils::populate_registration_table(m_db, "docker2|app2", false, true,
                                         false, false);
  fetch_registration_info();

  set_start_status(RebootMethod::NSF, "time to reboot");

  auto test_sequence = [&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));
    TestUtils::populate_restart_table_state(
        m_db, "app2", get_warm_start_state_name(WarmStartState::QUIESCENT));
    TestUtils::populate_restart_table_state(
        m_db, "app2", get_warm_start_state_name(WarmStartState::INITIALIZED));
    TestUtils::populate_restart_table_state(
        m_db, "app2", get_warm_start_state_name(WarmStartState::CHECKPOINTED));
  };

  std::thread test_thread = std::thread(test_sequence);

  Progress progress = perform_freeze_quiescence_w_stop(false, 500);
  EXPECT_EQ(progress, Progress::EXIT_EARLY);

  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(response,
              IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
                       "timeout occurred during reboot stage freeze"));
  test_thread.join();
}

// Same as previous test with shorter hold_time.
TEST_F(RebootThreadTest, TestQuiescenceSuccessAfterHoldTime) {
  overwrite_quiescence_hold_time_ms(100);

  TestUtils::populate_registration_table(m_db, "docker2|app2", false, true,
                                         false, false);
  fetch_registration_info();

  set_start_status(RebootMethod::NSF, "time to reboot");

  auto test_sequence = [&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));
    TestUtils::populate_restart_table_state(
        m_db, "app2", get_warm_start_state_name(WarmStartState::QUIESCENT));
    TestUtils::populate_restart_table_state(
        m_db, "app2", get_warm_start_state_name(WarmStartState::INITIALIZED));
    TestUtils::populate_restart_table_state(
        m_db, "app2", get_warm_start_state_name(WarmStartState::CHECKPOINTED));
  };

  std::thread test_thread = std::thread(test_sequence);

  Progress progress = perform_freeze_quiescence_w_stop(false, 500);
  EXPECT_EQ(progress, Progress::PROCEED);
  test_thread.join();
}

TEST_F(RebootThreadTest, TestFailWhenExitQuiescence) {
  overwrite_quiescence_hold_time_ms(200);

  TestUtils::populate_registration_table(m_db, "docker2|app2", false, true,
                                         false, false);
  fetch_registration_info();

  set_start_status(RebootMethod::NSF, "time to reboot");

  auto test_sequence = [&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));
    // Enter quiescent state
    TestUtils::populate_restart_table_state(
        m_db, "app2", get_warm_start_state_name(WarmStartState::QUIESCENT));
    // Exit quiescent state during hold time.
    TestUtils::populate_restart_table_state(
        m_db, "app2", get_warm_start_state_name(WarmStartState::INITIALIZED));
  };

  std::thread test_thread = std::thread(test_sequence);

  Progress progress = perform_freeze_quiescence_w_stop(false, 500);
  EXPECT_EQ(progress, Progress::EXIT_EARLY);

  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(response,
              IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
                       "timeout occurred during reboot stage freeze"));
  test_thread.join();
}

//
// Checkpoint
//

TEST_F(RebootThreadTest, TestPerformCheckpointCriticalState) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(1)
      .WillOnce(Return(true));
  set_start_status(RebootMethod::NSF, "time to reboot");

  swss::Select s;
  s.addSelectable(&(return_m_stop_reference()));
  Progress progress = perform_checkpoint(s);
  EXPECT_EQ(progress, Progress::EXIT_EARLY);

  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(response,
              IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
                       "system entered critical state before checkpointing"));
}

TEST_F(RebootThreadTest, TestPerformCheckpointTimeout) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(1)
      .WillOnce(Return(false));

  TestUtils::populate_registration_table(m_db, "docker1|app1", false, false,
                                         true, false);
  TestUtils::populate_registration_table(m_db, "docker2|app2", false, false,
                                         true, false);
  fetch_registration_info();

  overwrite_checkpoint_timeout(1);

  set_start_status(RebootMethod::NSF, "time to reboot");

  swss::Select s;
  s.addSelectable(&(return_m_stop_reference()));

  Progress progress = perform_checkpoint(s);
  EXPECT_EQ(progress, Progress::EXIT_EARLY);

  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(
      response,
      IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
               HasSubstr("timeout occurred during reboot stage checkpoint")));
}

TEST_F(RebootThreadTest, TestPerformCheckpointStop) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(1)
      .WillOnce(Return(false));

  TestUtils::populate_registration_table(m_db, "docker1|app1", false, false,
                                         true, false);
  TestUtils::populate_registration_table(m_db, "docker2|app2", false, false,
                                         true, false);
  fetch_registration_info();
  overwrite_quiescence_timeout_ms(1000);

  set_start_status(RebootMethod::NSF, "time to reboot");

  swss::Select s;
  s.addSelectable(&(return_m_stop_reference()));
  m_reboot_thread.Stop();

  Progress progress = perform_checkpoint(s);
  EXPECT_EQ(progress, Progress::EXIT_EARLY);

  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();

  // No error on request to stop, just log.
  EXPECT_THAT(
      response,
      IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN, ""));
}

TEST_F(RebootThreadTest, TestPerformCheckpointStartCompleted) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(1)
      .WillOnce(Return(false));

  TestUtils::populate_registration_table(m_db, "docker1|app1", false, false,
                                         true, false);
  TestUtils::populate_registration_table(m_db, "docker2|app2", false, false,
                                         true, false);
  TestUtils::populate_restart_table_state(
      m_db, "app1", get_warm_start_state_name(WarmStartState::CHECKPOINTED));
  TestUtils::populate_restart_table_state(
      m_db, "app2", get_warm_start_state_name(WarmStartState::CHECKPOINTED));
  fetch_registration_info();
  overwrite_quiescence_timeout_ms(1000);

  set_start_status(RebootMethod::NSF, "time to reboot");

  swss::Select s;
  s.addSelectable(&(return_m_stop_reference()));
  m_reboot_thread.Stop();

  Progress progress = perform_checkpoint(s);
  EXPECT_EQ(progress, Progress::PROCEED);
}

TEST_F(RebootThreadTest, TestPerformCheckpointStartFailed) {
  EXPECT_CALL(m_critical_interface, is_system_critical())
      .Times(1)
      .WillOnce(Return(false));

  TestUtils::populate_registration_table(m_db, "docker1|app1", false, false,
                                         true, false);
  TestUtils::populate_registration_table(m_db, "docker2|app2", false, false,
                                         true, false);
  TestUtils::populate_restart_table_state(
      m_db, "app1", get_warm_start_state_name(WarmStartState::CHECKPOINTED));
  TestUtils::populate_restart_table_state(
      m_db, "app2", get_warm_start_state_name(WarmStartState::FAILED));
  fetch_registration_info();
  overwrite_quiescence_timeout_ms(1000);

  set_start_status(RebootMethod::NSF, "time to reboot");

  swss::Select s;
  s.addSelectable(&(return_m_stop_reference()));
  m_reboot_thread.Stop();

  Progress progress = perform_checkpoint(s);
  EXPECT_EQ(progress, Progress::EXIT_EARLY);

  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(response,
              IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
                       HasSubstr("check_stage: app: app2 reported FAILED "
                                 "during stage: checkpoint")));
}

TEST_F(RebootThreadTest, TestPerformCheckpointCompleted) {
  TestUtils::populate_registration_table(m_db, "docker1|app1", false, false,
                                         true, false);
  TestUtils::populate_registration_table(m_db, "docker2|app2", false, false,
                                         true, false);
  fetch_registration_info();

  swss::Select s;
  s.addSelectable(&(return_m_stop_reference()));

  swss::SelectableTimer l_timer(timespec{.tv_sec = 1, .tv_nsec = 0});
  s.addSelectable(&l_timer);

  swss::SubscriberStateTable sub(&m_db, STATE_WARM_RESTART_TABLE_NAME);
  s.addSelectable(&sub);

  set_start_status(RebootMethod::NSF, "time to reboot");

  TestUtils::populate_restart_table_state(
      m_db, "app1", get_warm_start_state_name(WarmStartState::CHECKPOINTED));
  TestUtils::populate_restart_table_state(
      m_db, "app2", get_warm_start_state_name(WarmStartState::CHECKPOINTED));

  l_timer.start();

  Progress progress = checkpoint_stage_two(s, l_timer, sub);
  EXPECT_EQ(progress, Progress::PROCEED);
}

TEST_F(RebootThreadTest, TestPerformCheckpointUninterestingStatesIgnored) {
  TestUtils::populate_registration_table(m_db, "docker1|app1", false, false,
                                         true, false);
  fetch_registration_info();

  swss::Select s;
  s.addSelectable(&(return_m_stop_reference()));

  swss::SelectableTimer l_timer(timespec{.tv_sec = 1, .tv_nsec = 0});
  s.addSelectable(&l_timer);

  swss::SubscriberStateTable sub(&m_db, STATE_WARM_RESTART_TABLE_NAME);
  s.addSelectable(&sub);

  set_start_status(RebootMethod::NSF, "time to reboot");

  // Confirm a non checkpoint state isn't treated as CHECKPOINTED
  TestUtils::populate_restart_table_state(
      m_db, "app1", get_warm_start_state_name(WarmStartState::RECONCILED));
  TestUtils::populate_restart_table_state(
      m_db, "app2", get_warm_start_state_name(WarmStartState::INITIALIZED));
  TestUtils::populate_restart_table_state(
      m_db, "app2", get_warm_start_state_name(WarmStartState::FROZEN));

  l_timer.start();

  Progress progress = checkpoint_stage_two(s, l_timer, sub);
  EXPECT_EQ(progress, Progress::EXIT_EARLY);

  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(
      response,
      IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
               HasSubstr("timeout occurred during reboot stage checkpoint")));
}

TEST_F(RebootThreadTest, TestPerformCheckpointFailed) {
  TestUtils::populate_registration_table(m_db, "docker1|app1", false, false,
                                         true, false);
  TestUtils::populate_registration_table(m_db, "docker2|app2", false, false,
                                         true, false);
  fetch_registration_info();

  swss::Select s;
  s.addSelectable(&(return_m_stop_reference()));

  swss::SelectableTimer l_timer(timespec{.tv_sec = 1, .tv_nsec = 0});
  s.addSelectable(&l_timer);

  swss::SubscriberStateTable sub(&m_db, STATE_WARM_RESTART_TABLE_NAME);
  s.addSelectable(&sub);

  set_start_status(RebootMethod::NSF, "time to reboot");

  TestUtils::populate_restart_table_state(
      m_db, "app1", get_warm_start_state_name(WarmStartState::CHECKPOINTED));
  TestUtils::populate_restart_table_state(
      m_db, "app2", get_warm_start_state_name(WarmStartState::FAILED));

  l_timer.start();

  Progress progress = checkpoint_stage_two(s, l_timer, sub);

  EXPECT_EQ(progress, Progress::EXIT_EARLY);
  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(
      response,
      IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
               "handle_state_event: app: app2 reported FAILED when looking for "
               "state: checkpoint"));
}

TEST_F(RebootThreadTest, TestPerformCheckpointUnexpectedStatesViaSubscription) {
  TestUtils::populate_registration_table(m_db, "docker1|app1", false, false,
                                         true, false);
  fetch_registration_info();

  swss::Select s;
  s.addSelectable(&(return_m_stop_reference()));

  swss::SelectableTimer l_timer(timespec{.tv_sec = 1, .tv_nsec = 0});
  s.addSelectable(&l_timer);

  swss::SubscriberStateTable sub(&m_db, STATE_WARM_RESTART_TABLE_NAME);
  s.addSelectable(&sub);

  set_start_status(RebootMethod::NSF, "time to reboot");

  TestUtils::populate_restart_table_state(
      m_db, "app1", get_warm_start_state_name(WarmStartState::INITIALIZED));
  TestUtils::populate_restart_table_state(
      m_db, "app1", get_warm_start_state_name(WarmStartState::RESTORED));
  TestUtils::populate_restart_table_state(
      m_db, "app1", get_warm_start_state_name(WarmStartState::REPLAYED));

  l_timer.start();

  Progress progress = checkpoint_stage_two(s, l_timer, sub);

  EXPECT_EQ(progress, Progress::EXIT_EARLY);
  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(
      response,
      IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
               HasSubstr("timeout occurred during reboot stage checkpoint")));
}

//
// Stop On Freeze Tests
//

TEST_F(RebootThreadTest, TestCheckContainerStopDbusFail) {
  DbusInterface::DbusResponse dbus_response{
      DbusInterface::DbusStatus::DBUS_FAIL, "dbus reboot failed"};
  EXPECT_CALL(m_dbus_interface, StopContainerStatus(_))
      .Times(1)
      .WillOnce(Return(dbus_response));

  RebootThread::Status status = check_container_stop("requestA");
  EXPECT_EQ(status, RebootThread::Status::FAILURE);
}

TEST_F(RebootThreadTest, TestCheckContainerStopJsonParseFailure) {
  DbusInterface::DbusResponse dbus_response{
      DbusInterface::DbusStatus::DBUS_SUCCESS, "dbus reboot failed"};
  EXPECT_CALL(m_dbus_interface, StopContainerStatus(_))
      .Times(1)
      .WillOnce(Return(dbus_response));

  RebootThread::Status status = check_container_stop("requestA");
  EXPECT_EQ(status, RebootThread::Status::FAILURE);
}

TEST_F(RebootThreadTest, TestCheckContainerStopSuccess) {
  StopContainersResponse response;
  response.set_status(ShutdownStatus::DONE);

  std::string json_response;
  gpu::MessageToJsonString(response, &json_response);

  DbusInterface::DbusResponse dbus_response{
      DbusInterface::DbusStatus::DBUS_SUCCESS, json_response.c_str()};
  EXPECT_CALL(m_dbus_interface, StopContainerStatus(_))
      .Times(1)
      .WillOnce(Return(dbus_response));

  RebootThread::Status status = check_container_stop("requestA");
  EXPECT_EQ(status, RebootThread::Status::SUCCESS);
}

TEST_F(RebootThreadTest, TestWaitForContainerStopMStopSignal) {
  TestUtils::populate_registration_table(m_db, "docker1|app1", true, false,
                                         false, false);
  fetch_registration_info();
  set_start_status(RebootMethod::NSF, "time to reboot");

  Progress progress = wait_for_container_stop(300);

  EXPECT_EQ(progress, Progress::EXIT_EARLY);
  force_inactive();
  RebootStatusResponse response = m_reboot_thread.GetResponse();

  // No error on request to stop, just log.
  EXPECT_THAT(
      response,
      IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN, ""));
}

TEST_F(RebootThreadTest, TestWaitForContainerStopDbusReturnsStopped) {
  StopContainersResponse response;
  response.set_status(ShutdownStatus::DONE);

  std::string json_response;
  gpu::MessageToJsonString(response, &json_response);

  DbusInterface::DbusResponse dbus_response{
      DbusInterface::DbusStatus::DBUS_SUCCESS, json_response.c_str()};
  EXPECT_CALL(m_dbus_interface, StopContainerStatus(_))
      .Times(1)
      .WillOnce(Return(dbus_response));

  TestUtils::populate_registration_table(m_db, "docker1|app1", true, false,
                                         false, false);
  fetch_registration_info();
  set_start_status(RebootMethod::NSF, "time to reboot");

  Progress progress = wait_for_container_stop(300);

  EXPECT_EQ(progress, Progress::PROCEED);
}

}  // namespace rebootbackend
