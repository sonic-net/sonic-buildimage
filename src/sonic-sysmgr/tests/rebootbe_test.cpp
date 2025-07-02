#include "rebootbe.h"

#include <gmock/gmock.h>
#include <google/protobuf/util/json_util.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "mock_reboot_interfaces.h"
#include "reboot_common.h"
#include "select.h"
#include "status_code_util.h"
#include "system/system.pb.h"
#include "timestamp.h"

namespace rebootbackend {

#define ONE_SECOND (1)
#define TWO_SECONDS (2)
#define TENTH_SECOND_MS (100)
#define SELECT_TIMEOUT_250_MS (250)
#define ONE_SECOND_MS (1000)
#define TWO_SECONDS_MS (2000)

namespace gpu = ::google::protobuf::util;
using namespace gnoi::system;

using ::testing::_;
using ::testing::AllOf;
using ::testing::AtLeast;
using ::testing::ExplainMatchResult;
using ::testing::HasSubstr;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::StrictMock;

MATCHER_P2(IsStatus, status, message, "") {
  return (arg.status().status() == status &&
          ExplainMatchResult(message, arg.status().message(), result_listener));
}

MATCHER_P3(ActiveCountMethod, active, count, method, "") {
  return (arg.active() == active && arg.count() == (uint32_t)count &&
          arg.method() == method);
}

class RebootBETestWithoutStop : public ::testing::Test {
 protected:
  RebootBETestWithoutStop()
      : m_dbus_interface(),
        m_db("STATE_DB", 0),
        m_config_db("CONFIG_DB", 0),
        m_rebootbeRequestChannel(&m_db, REBOOT_REQUEST_NOTIFICATION_CHANNEL),
        m_rebootbeReponseChannel(&m_db, REBOOT_RESPONSE_NOTIFICATION_CHANNEL),
        m_rebootbe(m_dbus_interface) {
    sigterm_requested = false;

    m_s.addSelectable(&m_rebootbeReponseChannel);

    // Make the tests log to stdout, instead of syslog.
    swss::Table logging_table(&m_config_db, CFG_LOGGER_TABLE_NAME);
    logging_table.hset("rebootbackend", swss::DAEMON_LOGOUTPUT, "STDOUT");
    swss::Logger::restartLogger();
  }
  virtual ~RebootBETestWithoutStop() = default;

  void force_warm_start_state(bool enabled) {
    swss::Table enable_table(&m_db, STATE_WARM_RESTART_ENABLE_TABLE_NAME);
    enable_table.hset("system", "enable", enabled ? "true" : "false");
    enable_table.hset("sonic-sysmgr", "enable", enabled ? "true" : "false");

    swss::Table restart_table(&m_db, STATE_WARM_RESTART_TABLE_NAME);
    restart_table.hset("rebootbackend", "restore_count", enabled ? "0" : "");
  }

  void start_rebootbe() {
    m_rebootbe_thread =
        std::make_unique<std::thread>(&RebootBE::Start, &m_rebootbe);
  }

  void set_mock_defaults() {
    ON_CALL(m_dbus_interface, Reboot(_))
        .WillByDefault(Return(DbusInterface::DbusResponse{
            DbusInterface::DbusStatus::DBUS_SUCCESS, ""}));
  }

  void overwrite_reboot_timeout(uint32_t timeout_seconds) {
    m_rebootbe.m_RebootThread.m_reboot_timeout = timeout_seconds;
  }

  void send_stop_reboot_thread() { m_rebootbe.m_RebootThread.Stop(); }

  void SendRebootRequest(const std::string &op, const std::string &data,
                         const std::string &field, const std::string &value) {
    std::vector<swss::FieldValueTuple> values;
    values.push_back(swss::FieldValueTuple{field, value});

    m_rebootbeRequestChannel.send(op, data, values);
  }

  void SendRebootViaProto(RebootRequest &request) {
    std::string json_string;
    gpu::MessageToJsonString(request, &json_string);

    SendRebootRequest("Reboot", "StatusCode", DATA_TUPLE_KEY, json_string);
  }

  void SendRebootStatusRequest(void) {
    SendRebootRequest("RebootStatus", "StatusCode", DATA_TUPLE_KEY,
                      "json status request");
  }

  void start_reboot_via_rpc(
      RebootRequest &request,
      swss::StatusCode expected_result = swss::StatusCode::SWSS_RC_SUCCESS) {
    SendRebootViaProto(request);
    while (true) {
      int ret;
      swss::Selectable *sel;
      ret = m_s.select(&sel, SELECT_TIMEOUT_250_MS);
      if (ret != swss::Select::OBJECT) continue;
      if (sel != &m_rebootbeReponseChannel) continue;
      break;
    }
    std::string op, data;
    std::vector<swss::FieldValueTuple> ret_values;
    m_rebootbeReponseChannel.pop(op, data, ret_values);

    EXPECT_THAT(op, StrEq("Reboot"));
    EXPECT_THAT(data, StrEq(swss::statusCodeToStr(expected_result)));
  }

  gnoi::system::RebootStatusResponse do_reboot_status_rpc() {
    SendRebootStatusRequest();
    while (true) {
      int ret;
      swss::Selectable *sel;
      ret = m_s.select(&sel, SELECT_TIMEOUT_250_MS);
      if (ret != swss::Select::OBJECT) continue;
      if (sel != &m_rebootbeReponseChannel) continue;
      break;
    }
    std::string op, data;
    std::vector<swss::FieldValueTuple> ret_values;
    m_rebootbeReponseChannel.pop(op, data, ret_values);

    EXPECT_THAT(op, StrEq("RebootStatus"));
    EXPECT_EQ(data, swss::statusCodeToStr(swss::StatusCode::SWSS_RC_SUCCESS));

    std::string json_response;
    for (auto &fv : ret_values) {
      if (DATA_TUPLE_KEY == fvField(fv)) {
        json_response = fvValue(fv);
      }
    }
    gnoi::system::RebootStatusResponse response;
    gpu::JsonStringToMessage(json_response, &response);
    return response;
  }

  void GetNotificationResponse(swss::NotificationConsumer &consumer,
                               std::string &op, std::string &data,
                               std::vector<swss::FieldValueTuple> &values) {
    swss::Select s;
    s.addSelectable(&consumer);
    swss::Selectable *sel;
    s.select(&sel, SELECT_TIMEOUT_250_MS);

    consumer.pop(op, data, values);
  }

  NotificationResponse handle_reboot_request(std::string &json_request) {
    return m_rebootbe.HandleRebootRequest(json_request);
  }

  // Mock interfaces.
  NiceMock<MockDbusInterface> m_dbus_interface;

  // DB connectors
  swss::DBConnector m_db;
  swss::DBConnector m_config_db;

  // Reboot thread signaling.
  swss::NotificationProducer m_rebootbeRequestChannel;
  swss::Select m_s;
  swss::NotificationConsumer m_rebootbeReponseChannel;

  // Module under test.
  std::unique_ptr<std::thread> m_rebootbe_thread;
  RebootBE m_rebootbe;
};

class RebootBETest : public RebootBETestWithoutStop {
 protected:
  ~RebootBETest() {
    m_rebootbe.Stop();
    m_rebootbe_thread->join();
  }
};

// Test fixture to skip through the startup sequence into the main loop.
// Param indicates if RebootBE should be initialized into a state where the
// system came up in warmboot.
class RebootBEAutoStartTest : public RebootBETest,
                              public ::testing::WithParamInterface<bool> {
 protected:
  RebootBEAutoStartTest() {
    start_rebootbe();

    std::this_thread::sleep_for(std::chrono::milliseconds(ONE_SECOND_MS));
    EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::RebManagerStatus::IDLE);
  }
};

// Normal operation testing.
TEST_P(RebootBEAutoStartTest, NonExistentMessage) {
  swss::NotificationConsumer consumer(&m_db,
                                      REBOOT_RESPONSE_NOTIFICATION_CHANNEL);

  // No "MESSAGE" in field/values
  SendRebootRequest("Reboot", "StatusCode", "field1", "field1_value");
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::RebManagerStatus::IDLE);

  std::string op, data;
  std::vector<swss::FieldValueTuple> ret_values;
  GetNotificationResponse(consumer, op, data, ret_values);

  EXPECT_THAT(op, StrEq("Reboot"));
  EXPECT_THAT(
      data,
      StrEq(swss::statusCodeToStr(swss::StatusCode::SWSS_RC_INVALID_PARAM)));
}

TEST_P(RebootBEAutoStartTest, TestCancelReboot) {
  swss::NotificationConsumer consumer(&m_db,
                                      REBOOT_RESPONSE_NOTIFICATION_CHANNEL);

  SendRebootRequest("CancelReboot", "StatusCode", DATA_TUPLE_KEY,
                    "json cancelreboot request");
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::RebManagerStatus::IDLE);

  std::string op, data;
  std::vector<swss::FieldValueTuple> ret_values;
  GetNotificationResponse(consumer, op, data, ret_values);

  EXPECT_THAT(op, StrEq("CancelReboot"));
  EXPECT_THAT(
      data,
      StrEq(swss::statusCodeToStr(swss::StatusCode::SWSS_RC_UNIMPLEMENTED)));
}

TEST_P(RebootBEAutoStartTest, TestUnrecognizedOP) {
  swss::NotificationConsumer consumer(&m_db,
                                      REBOOT_RESPONSE_NOTIFICATION_CHANNEL);

  SendRebootRequest("NonOp", "StatusCode", DATA_TUPLE_KEY, "invalid op code");
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::RebManagerStatus::IDLE);

  std::string op, data;
  std::vector<swss::FieldValueTuple> ret_values;
  GetNotificationResponse(consumer, op, data, ret_values);

  EXPECT_THAT(op, StrEq("NonOp"));
  EXPECT_THAT(
      data,
      StrEq(swss::statusCodeToStr(swss::StatusCode::SWSS_RC_INVALID_PARAM)));
}

TEST_P(RebootBEAutoStartTest, TestColdRebootDbusToCompletion) {
  DbusInterface::DbusResponse dbus_response{
      DbusInterface::DbusStatus::DBUS_SUCCESS, ""};
  EXPECT_CALL(m_dbus_interface, Reboot(_))
      .Times(3)
      .WillRepeatedly(Return(dbus_response));

  overwrite_reboot_timeout(1);
  RebootRequest request;
  request.set_method(RebootMethod::COLD);
  start_reboot_via_rpc(request);

  std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::RebManagerStatus::COLD_REBOOT_IN_PROGRESS);
  sleep(TWO_SECONDS);

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::RebManagerStatus::IDLE);
  gnoi::system::RebootStatusResponse response = do_reboot_status_rpc();
  EXPECT_THAT(response, ActiveCountMethod(false, 1, RebootMethod::COLD));
  EXPECT_THAT(response,
              IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
                       "platform failed to reboot"));

  start_reboot_via_rpc(request);
  sleep(TWO_SECONDS);

  start_reboot_via_rpc(request);
  sleep(TWO_SECONDS);

  response = do_reboot_status_rpc();
  // Verifiy count is 3 after three reboot attempts.
  EXPECT_THAT(response, ActiveCountMethod(false, 3, RebootMethod::COLD));
  EXPECT_THAT(response,
              IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
                       "platform failed to reboot"));
}

TEST_P(RebootBEAutoStartTest, TestWarmRebootDbusToCompletion) {
  DbusInterface::DbusResponse dbus_response{
      DbusInterface::DbusStatus::DBUS_SUCCESS, ""};
  EXPECT_CALL(m_dbus_interface, Reboot(_))
      .Times(1)
      .WillRepeatedly(Return(dbus_response));

  overwrite_reboot_timeout(1);
  RebootRequest request;
  request.set_method(RebootMethod::WARM);
  start_reboot_via_rpc(request);
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::RebManagerStatus::WARM_REBOOT_IN_PROGRESS);

  sleep(TWO_SECONDS);

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::RebManagerStatus::IDLE);
  gnoi::system::RebootStatusResponse response = do_reboot_status_rpc();
  EXPECT_THAT(response, ActiveCountMethod(false, 1, RebootMethod::WARM));
  EXPECT_THAT(response,
              IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
                       "failed to warm reboot"));
}

TEST_P(RebootBEAutoStartTest, TestColdBootSigterm) {
  sigterm_requested = true;
  set_mock_defaults();
  overwrite_reboot_timeout(1);

  RebootRequest request;
  request.set_method(RebootMethod::COLD);
  start_reboot_via_rpc(request);

  sleep(ONE_SECOND);

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::RebManagerStatus::IDLE);
  gnoi::system::RebootStatusResponse second_resp = do_reboot_status_rpc();
  EXPECT_THAT(second_resp, ActiveCountMethod(false, 1, RebootMethod::COLD));
  EXPECT_THAT(
      second_resp,
      IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN, ""));
}

TEST_P(RebootBEAutoStartTest, TestWarmBootSigterm) {
  sigterm_requested = true;
  set_mock_defaults();
  overwrite_reboot_timeout(1);

  RebootRequest request;
  request.set_method(RebootMethod::WARM);
  start_reboot_via_rpc(request);

  sleep(ONE_SECOND);

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::RebManagerStatus::IDLE);
  gnoi::system::RebootStatusResponse second_resp = do_reboot_status_rpc();
  EXPECT_THAT(second_resp, ActiveCountMethod(false, 1, RebootMethod::WARM));
  EXPECT_THAT(
      second_resp,
      IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN, ""));
}

TEST_P(RebootBEAutoStartTest, TestColdBootDbusError) {
  // Return FAIL from dbus reboot call.
  DbusInterface::DbusResponse dbus_response{
      DbusInterface::DbusStatus::DBUS_FAIL, "dbus reboot failed"};
  EXPECT_CALL(m_dbus_interface, Reboot(_))
      .Times(1)
      .WillOnce(Return(dbus_response));

  RebootRequest request;
  request.set_method(RebootMethod::COLD);
  start_reboot_via_rpc(request);

  sleep(TWO_SECONDS);

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::RebManagerStatus::IDLE);
  gnoi::system::RebootStatusResponse second_resp = do_reboot_status_rpc();
  EXPECT_THAT(second_resp, ActiveCountMethod(false, 1, RebootMethod::COLD));
  EXPECT_THAT(second_resp,
              IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
                       "dbus reboot failed"));
}

TEST_P(RebootBEAutoStartTest, TestWarmBootDbusError) {
  // Return FAIL from dbus reboot call.
  DbusInterface::DbusResponse dbus_response{
      DbusInterface::DbusStatus::DBUS_FAIL, "dbus reboot failed"};
  EXPECT_CALL(m_dbus_interface, Reboot(_))
      .Times(1)
      .WillOnce(Return(dbus_response));

  RebootRequest request;
  request.set_method(RebootMethod::WARM);
  start_reboot_via_rpc(request);

  sleep(TWO_SECONDS);

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::RebManagerStatus::IDLE);
  gnoi::system::RebootStatusResponse second_resp = do_reboot_status_rpc();
  EXPECT_THAT(second_resp, ActiveCountMethod(false, 1, RebootMethod::WARM));
  EXPECT_THAT(second_resp,
              IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
                       "dbus reboot failed"));
}

TEST_P(RebootBEAutoStartTest, TestStopDuringColdBoot) {
  set_mock_defaults();

  RebootRequest request;
  request.set_method(RebootMethod::COLD);
  start_reboot_via_rpc(request);
  std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::RebManagerStatus::COLD_REBOOT_IN_PROGRESS);

  send_stop_reboot_thread();
  std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::RebManagerStatus::IDLE);

  gnoi::system::RebootStatusResponse response = do_reboot_status_rpc();
  EXPECT_THAT(response, ActiveCountMethod(false, 1, RebootMethod::COLD));
  EXPECT_THAT(
      response,
      IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN, ""));
}

TEST_P(RebootBEAutoStartTest, TestStopDuringWarmBoot) {
  set_mock_defaults();

  RebootRequest request;
  request.set_method(RebootMethod::WARM);
  start_reboot_via_rpc(request);
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::RebManagerStatus::WARM_REBOOT_IN_PROGRESS);

  send_stop_reboot_thread();
  std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::RebManagerStatus::IDLE);

  gnoi::system::RebootStatusResponse response = do_reboot_status_rpc();
  EXPECT_THAT(response, ActiveCountMethod(false, 1, RebootMethod::WARM));
  EXPECT_THAT(
      response,
      IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN, ""));
}

TEST_P(RebootBEAutoStartTest, TestInvalidJsonRebootRequest) {
  std::string json_request = "abcd";
  NotificationResponse response = handle_reboot_request(json_request);
  EXPECT_EQ(swss::StatusCode::SWSS_RC_INTERNAL, response.status);
}

TEST_P(RebootBEAutoStartTest, TestWarmFailureFollowedByColdBoot) {
  DbusInterface::DbusResponse dbus_response{
      DbusInterface::DbusStatus::DBUS_SUCCESS, ""};
  overwrite_reboot_timeout(1);

  RebootRequest request;
  request.set_method(RebootMethod::WARM);
  start_reboot_via_rpc(request);

  std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::RebManagerStatus::WARM_REBOOT_IN_PROGRESS);

  std::this_thread::sleep_for(std::chrono::milliseconds(TWO_SECONDS_MS));
  gnoi::system::RebootStatusResponse response = do_reboot_status_rpc();
  EXPECT_THAT(response, ActiveCountMethod(false, 1, RebootMethod::WARM));

  request.set_method(RebootMethod::COLD);
  start_reboot_via_rpc(request);

  // We have to wait for the 1 second reboot Timeout
  std::this_thread::sleep_for(std::chrono::milliseconds(TWO_SECONDS_MS));

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::RebManagerStatus::IDLE);
  response = do_reboot_status_rpc();
  EXPECT_THAT(response, ActiveCountMethod(false, 2, RebootMethod::COLD));
  EXPECT_THAT(response,
              IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
                       "platform failed to reboot"));
}

INSTANTIATE_TEST_SUITE_P(TestWithStartupWarmbootEnabledState,
                         RebootBEAutoStartTest, testing::Values(true, false));

}  // namespace rebootbackend
