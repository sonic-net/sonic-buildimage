#include "reboot_thread.h"

#include <gmock/gmock.h>
#include <google/protobuf/util/json_util.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <string>
#include <thread>
#include <vector>

#include "mock_reboot_interfaces.h"
#include "reboot_common.h"
#include "reboot_interfaces.h"
#include "select.h"
#include "selectableevent.h"
#include "status_code_util.h"
#include "system/system.pb.h"
#include "timestamp.h"

namespace rebootbackend {


// using namespace gnoi::system;
namespace gpu = ::google::protobuf::util;
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
  gnoi::system::RebootStatusResponse response = m_status.get_response();

  EXPECT_FALSE(response.active());
  EXPECT_THAT(response.reason(), StrEq(""));
  EXPECT_EQ(response.count(), 0);
  EXPECT_EQ(response.method(), gnoi::system::RebootMethod::UNKNOWN);
  EXPECT_EQ(
      response.status().status(),
      gnoi::system::RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN);
  EXPECT_THAT(response.status().message(), StrEq(""));

  EXPECT_FALSE(m_status.get_active());
}

TEST_F(RebootStatusTest, TestGetStatus) {
  std::chrono::nanoseconds curr_ns =
      std::chrono::high_resolution_clock::now().time_since_epoch();

  m_status.set_start_status(gnoi::system::RebootMethod::COLD, "reboot because");

  gnoi::system::RebootStatusResponse response = m_status.get_response();
  EXPECT_EQ(
      response.status().status(),
      gnoi::system::RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN);

  m_status.set_completed_status(
      gnoi::system::RebootStatus_Status::RebootStatus_Status_STATUS_SUCCESS,
      "anything");

  response = m_status.get_response();

  // message should be empty while reboot is active
  EXPECT_THAT(response.status().message(), StrEq(""));

  uint64_t reboot_ns = response.when();
  EXPECT_TRUE(reboot_ns > (uint64_t)curr_ns.count());

  m_status.set_inactive();
  response = m_status.get_response();
  EXPECT_THAT(response.status().message(), StrEq("anything"));
  EXPECT_EQ(
      response.status().status(),
      gnoi::system::RebootStatus_Status::RebootStatus_Status_STATUS_SUCCESS);
  EXPECT_EQ(0, response.when());
}

TEST_F(RebootStatusTest, TestHaltGetStatus) {
  std::chrono::nanoseconds curr_ns =
      std::chrono::high_resolution_clock::now().time_since_epoch();

  m_status.set_start_status(gnoi::system::RebootMethod::HALT, "reboot because");

  gnoi::system::RebootStatusResponse response = m_status.get_response();
  EXPECT_EQ(
      response.status().status(),
      gnoi::system::RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN);

  m_status.set_completed_status(
      gnoi::system::RebootStatus_Status::RebootStatus_Status_STATUS_SUCCESS,
      "anything");

  response = m_status.get_response();

  // message should be empty while reboot is active
  EXPECT_THAT(response.status().message(), StrEq(""));

  uint64_t reboot_ns = response.when();
  EXPECT_TRUE(reboot_ns > (uint64_t)curr_ns.count());

  m_status.set_inactive();
  response = m_status.get_response();
  EXPECT_THAT(response.status().message(), StrEq("anything"));
  EXPECT_EQ(
      response.status().status(),
      gnoi::system::RebootStatus_Status::RebootStatus_Status_STATUS_SUCCESS);
  EXPECT_EQ(0, response.when());
}

TEST_F(RebootStatusTest, TestGetWarmStatus) {
  std::chrono::nanoseconds curr_ns =
      std::chrono::high_resolution_clock::now().time_since_epoch();

  m_status.set_start_status(gnoi::system::RebootMethod::WARM, "reboot because");

  gnoi::system::RebootStatusResponse response = m_status.get_response();
  EXPECT_EQ(
      response.status().status(),
      gnoi::system::RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN);

  m_status.set_completed_status(
      gnoi::system::RebootStatus_Status::RebootStatus_Status_STATUS_SUCCESS,
      "anything");

  response = m_status.get_response();

  // message should be empty while reboot is active
  EXPECT_THAT(response.status().message(), StrEq(""));

  uint64_t reboot_ns = response.when();
  EXPECT_TRUE(reboot_ns > (uint64_t)curr_ns.count());

  m_status.set_inactive();
  response = m_status.get_response();
  EXPECT_THAT(response.status().message(), StrEq("anything"));
  EXPECT_EQ(
      response.status().status(),
      gnoi::system::RebootStatus_Status::RebootStatus_Status_STATUS_SUCCESS);
  EXPECT_EQ(0, response.when());
}

class RebootThreadTest : public ::testing::Test {
 protected:
  RebootThreadTest()
      : m_dbus_interface(),
        m_db("STATE_DB", 0),
        m_config_db("CONFIG_DB", 0),
        m_reboot_thread(m_dbus_interface, m_finished) {
    sigterm_requested = false;
  }

  void overwrite_reboot_timeout(uint32_t timeout_seconds) {
    m_reboot_thread.m_reboot_timeout = timeout_seconds;
  }

  gnoi::system::RebootStatusResponse get_response(void) {
    return m_reboot_thread.m_status.get_response();
  }

  void set_start_status(const gnoi::system::RebootMethod &method,
                        const std::string &reason) {
    return m_reboot_thread.m_status.set_start_status(method, reason);
  }

  void set_completed_status(const gnoi::system::RebootStatus_Status &status,
                            const std::string &message) {
    return m_reboot_thread.m_status.set_completed_status(status, message);
  }

  void force_inactive(void) { return m_reboot_thread.m_status.set_inactive(); }

  void force_active(void) { return m_reboot_thread.m_status.set_inactive(); }

  void do_reboot(void) { return m_reboot_thread.do_reboot(); }

  void wait_for_finish(swss::Select &s, swss::SelectableEvent &finished,
                       long timeout_seconds) {
    swss::Selectable *sel;

    int ret = s.select(&sel, timeout_seconds * 1000);
    EXPECT_EQ(ret, swss::Select::OBJECT);
    EXPECT_EQ(sel, &finished);
  }

  Progress wait_for_platform_reboot(swss::Select &s) {
    return m_reboot_thread.wait_for_platform_reboot(s);
  }

  swss::SelectableEvent &return_m_stop_reference() {
    return m_reboot_thread.m_stop;
  }

  swss::DBConnector m_db;
  swss::DBConnector m_config_db;
  NiceMock<MockDbusInterface> m_dbus_interface;
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
  gnoi::system::RebootRequest request;
  request.set_method(gnoi::system::RebootMethod::COLD);
  overwrite_reboot_timeout(2);
  m_reboot_thread.Start(request);
  m_reboot_thread.Stop();
  m_reboot_thread.Join();
  gnoi::system::RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(
      response,
      IsStatus(
          gnoi::system::RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN,
          ""));
}

TEST_F(RebootThreadTest, TestCleanExit) {
  EXPECT_CALL(m_dbus_interface, Reboot(_))
      .Times(1)
      .WillOnce(Return(DbusInterface::DbusResponse{
          DbusInterface::DbusStatus::DBUS_SUCCESS, ""}));

  overwrite_reboot_timeout(1);

  swss::Select s;
  s.addSelectable(&m_finished);

  gnoi::system::RebootRequest request;
  request.set_method(gnoi::system::RebootMethod::COLD);
  request.set_message("time to reboot");
  m_reboot_thread.Start(request);
  wait_for_finish(s, m_finished, 5);

  // Status should be active until we call join
  gnoi::system::RebootStatusResponse response = get_response();
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

  overwrite_reboot_timeout(2);

  gnoi::system::RebootRequest request;
  request.set_method(gnoi::system::RebootMethod::COLD);
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

  overwrite_reboot_timeout(1);

  swss::Select s;
  s.addSelectable(&m_finished);

  gnoi::system::RebootRequest request;
  request.set_method(gnoi::system::RebootMethod::COLD);
  request.set_message("time to reboot");
  m_reboot_thread.Start(request);
  wait_for_finish(s, m_finished, 3);

  // First thread has stopped: we need to join before
  // restart will succeed
  NotificationResponse response = m_reboot_thread.Start(request);
  EXPECT_EQ(response.status, swss::StatusCode::SWSS_RC_IN_USE);

  // This should join the first start.
  bool ret = m_reboot_thread.Join();
  EXPECT_TRUE(ret);
}

TEST_F(RebootThreadTest, TestUnsupportedRebootType) {
  gnoi::system::RebootRequest request;
  request.set_method(gnoi::system::RebootMethod::POWERDOWN);

  NotificationResponse response = m_reboot_thread.Start(request);
  EXPECT_EQ(response.status, swss::StatusCode::SWSS_RC_INVALID_PARAM);
  EXPECT_EQ(response.json_string,
            "RebootThread: Start rx'd unsupported method");
}

TEST_F(RebootThreadTest, TestInvalidMethodfDoReboot) {
  set_start_status(gnoi::system::RebootMethod::POWERUP, "time to reboot");
  do_reboot();
  force_inactive();
  gnoi::system::RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(
      response,
      IsStatus(
          gnoi::system::RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN,
          ""));
}

TEST_F(RebootThreadTest, TestNoWarmIfNonRetriableFailure) {
  set_start_status(gnoi::system::RebootMethod::WARM, "time to reboot");
  set_completed_status(
      gnoi::system::RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
      "failed to warm reboot");
  force_inactive();

  gnoi::system::RebootRequest request;
  request.set_method(gnoi::system::RebootMethod::WARM);

  NotificationResponse response = m_reboot_thread.Start(request);
  EXPECT_EQ(response.status, swss::StatusCode::SWSS_RC_FAILED_PRECONDITION);
  EXPECT_EQ(response.json_string,
            "RebootThread: last WARM reboot failed with non-retriable failure");
}

TEST_F(RebootThreadTest, TestSigTermStartofDoReboot) {
  sigterm_requested = true;
  set_start_status(gnoi::system::RebootMethod::WARM, "time to reboot");
  do_reboot();
  force_inactive();
  gnoi::system::RebootStatusResponse response = m_reboot_thread.GetResponse();
  EXPECT_THAT(
      response,
      IsStatus(
          gnoi::system::RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN,
          ""));
}

TEST_F(RebootThreadTest, TestWaitForRebootPositive) {
  overwrite_reboot_timeout(1);
  set_start_status(gnoi::system::RebootMethod::WARM, "time to reboot");
  swss::Select s;
  swss::SelectableEvent m_stop;
  s.addSelectable(&m_stop);
  RebootThread::Progress progress = wait_for_platform_reboot(s);
  EXPECT_EQ(progress, RebootThread::Progress::PROCEED);
}

}  // namespace rebootbackend
