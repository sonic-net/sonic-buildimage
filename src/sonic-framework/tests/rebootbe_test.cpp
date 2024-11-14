#include "rebootbe.h"

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
#include "select.h"
#include "stateverification.h"
#include "status_code_util.h"
#include "system/system.pb.h"
#include "test_utils_common.h"
#include "timestamp.h"

namespace rebootbackend {

#define ONE_SECOND (1)
#define TWO_SECONDS (2)
#define TENTH_SECOND_MS (100)
#define HALF_SECOND_MS (500)
#define ONE_SECOND_MS (1000)
#define FIFTEEN_HUNDRED_MS (1500)
#define TWO_SECONDS_MS (2000)

namespace gpu = ::google::protobuf::util;
using namespace gnoi::system;
using WarmStartState = ::swss::WarmStart::WarmStartState;
using WarmBootStage = ::swss::WarmStart::WarmBootStage;

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
        m_critical_interface(),
        m_db("STATE_DB", 0),
        m_config_db("CONFIG_DB", 0),
        m_rebootbeRequestChannel(&m_db, REBOOT_REQUEST_NOTIFICATION_CHANNEL),
        m_rebootbeReponseChannel(&m_db, REBOOT_RESPONSE_NOTIFICATION_CHANNEL),
        m_rebootbe(m_dbus_interface, m_critical_interface, m_telemetry) {
    sigterm_requested = false;
    TestUtils::clear_tables(m_db);

    auto mock_init_thread = std::make_unique<StrictMock<MockInitThread>>();
    m_init_thread = mock_init_thread.get();
    m_rebootbe.m_init_thread = std::move(mock_init_thread);

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
    enable_table.hset("sonic-framework", "enable", enabled ? "true" : "false");

    swss::Table restart_table(&m_db, STATE_WARM_RESTART_TABLE_NAME);
    restart_table.hset("rebootbackend", "restore_count", enabled ? "0" : "");
  }

  gnoi::system::RebootStatusResponse default_not_started_status() {
    InitThreadStatus status;
    return status.get_response();
  }

  gnoi::system::RebootStatusResponse default_done_status() {
    InitThreadStatus status;
    // We can't edit the status without it being active.
    status.set_start_status();
    status.set_success();
    status.set_inactive();
    return status.get_response();
  }

  gnoi::system::RebootStatusResponse default_running_status() {
    InitThreadStatus status;
    status.set_start_status();
    status.set_detailed_thread_status(
        InitThreadStatus::ThreadStatus::WAITING_FOR_RECONCILIATION);
    return status.get_response();
  }

  gnoi::system::RebootStatusResponse default_error_status() {
    InitThreadStatus status;
    status.set_start_status();
    status.set_error(InitThreadStatus::ErrorCondition::RECONCILIATION_FAILED,
                     "Fake reconciliation failed");
    return status.get_response();
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
    m_rebootbe.m_reboot_thread.m_reboot_timeout = timeout_seconds;
  }

  void overwrite_state_verification_timeout(uint32_t timeout_seconds) {
    m_rebootbe.m_reboot_thread.m_state_verification_timeout = timeout_seconds;
  }

  void overwrite_quiescent_timeout_ms(uint64_t timeout_ms) {
    m_rebootbe.m_reboot_thread.m_quiescence_timeout_ms = timeout_ms;
  }

  void overwrite_quiescent_hold_time_ms(uint64_t timeout_ms) {
    m_rebootbe.m_reboot_thread.m_quiescence_hold_time_ms = timeout_ms;
  }

  void overwrite_checkpoint_timeout(uint32_t timeout_seconds) {
    m_rebootbe.m_reboot_thread.m_checkpoint_timeout = timeout_seconds;
  }

  void send_stop_reboot_thread() { m_rebootbe.m_reboot_thread.Stop(); }

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
      ret = m_s.select(&sel, SELECT_TIMEOUT_MS);
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
      ret = m_s.select(&sel, SELECT_TIMEOUT_MS);
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
    s.select(&sel, SELECT_TIMEOUT_MS);

    consumer.pop(op, data, values);
  }

  NotificationResponse handle_reboot_request(std::string &json_request) {
    return m_rebootbe.handle_reboot_request(json_request);
  }

  void set_all_telemetry_expects(bool freeze_status = true,
                                 bool checkpoint_status = true) {
    set_telemetry_overall_expects(freeze_status && checkpoint_status);
    set_telemetry_stage_expects(WarmBootStage::STAGE_FREEZE, freeze_status);
    if (freeze_status) {
      set_telemetry_stage_expects(WarmBootStage::STAGE_CHECKPOINT,
                                  checkpoint_status);
    }
  }

  void set_telemetry_overall_expects(bool success) {
    EXPECT_CALL(m_telemetry, record_overall_start()).Times(1);
    if (!success) {
      EXPECT_CALL(m_telemetry, record_overall_end(success)).Times(1);
    }
  }

  void set_telemetry_stage_expects(WarmBootStage nsf_stage, bool success) {
    EXPECT_CALL(m_telemetry, record_stage_start(nsf_stage)).Times(1);
    EXPECT_CALL(m_telemetry, record_stage_end(nsf_stage, success)).Times(1);
  }

  swss::SelectableEvent &get_stack_unfrozen_select() {
    return m_rebootbe.m_stack_unfrozen;
  }

  swss::SelectableEvent &get_init_done_select() {
    return m_rebootbe.m_init_thread_done;
  }

  // Mock interfaces.
  NiceMock<MockDbusInterface> m_dbus_interface;
  NiceMock<MockCriticalStateInterface> m_critical_interface;
  StrictMock<MockTelemetryInterface> m_telemetry;

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

  // Not owned by test.
  StrictMock<MockInitThread> *m_init_thread;
};

class RebootBETest : public RebootBETestWithoutStop {
 protected:
  ~RebootBETest() {
    m_rebootbe.Stop();
    m_rebootbe_thread->join();
  }
};

// Init sequence testing.
TEST_F(RebootBETest, ColdbootInitWorks) {
  force_warm_start_state(false);

  EXPECT_CALL(*m_init_thread, GetResponse())
      .Times(2)
      .WillRepeatedly(Return(default_not_started_status()));

  start_rebootbe();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);

  gnoi::system::RebootStatusResponse response = do_reboot_status_rpc();
  EXPECT_THAT(response, ActiveCountMethod(false, 0, RebootMethod::COLD));
  EXPECT_THAT(
      response,
      IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_SUCCESS, ""));
}

TEST_F(RebootBETest, WarmbootInitWorks) {
  force_warm_start_state(true);

  {
    InSequence seq;
    EXPECT_CALL(*m_init_thread, Start())
        .WillOnce(Return(swss::StatusCode::SWSS_RC_SUCCESS));

    // Status request during warmboot init, then during Join sequence.
    EXPECT_CALL(*m_init_thread, GetResponse())
        .Times(2)
        .WillRepeatedly(Return(default_running_status()))
        .RetiresOnSaturation();

    // Normal Join sequence when reaching COMPLETED.
    EXPECT_CALL(*m_init_thread, Join()).WillOnce(Return(true));

    // Status request after warmboot init, then cleanup sequence.
    EXPECT_CALL(*m_init_thread, GetResponse())
        .Times(2)
        .WillRepeatedly(Return(default_done_status()));
  }

  start_rebootbe();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::NsfManagerStatus::NSF_INIT_WAIT);

  // Check status during init.
  EXPECT_THAT(
      do_reboot_status_rpc(),
      AllOf(ActiveCountMethod(true, 0, RebootMethod::NSF),
            IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN,
                     "")));

  get_stack_unfrozen_select().notify();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::NsfManagerStatus::NSF_INIT_WAIT);

  get_init_done_select().notify();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);

  // Check that NSF status is sticky after init, before a new coldboot starts.
  EXPECT_THAT(
      do_reboot_status_rpc(),
      AllOf(ActiveCountMethod(false, 0, RebootMethod::NSF),
            IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_SUCCESS,
                     "")));
}

TEST_F(RebootBETest, InitThreadFailsToStart) {
  force_warm_start_state(true);

  {
    InSequence seq;
    EXPECT_CALL(*m_init_thread, Start())
        .WillOnce(Return(swss::StatusCode::SWSS_RC_INTERNAL));

    // Cleanup sequence.
    EXPECT_CALL(*m_init_thread, GetResponse())
        .WillOnce(Return(default_not_started_status()));
  }

  start_rebootbe();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);
}

TEST_F(RebootBETest, WarmbootInProgressBlocksNewWarmboot) {
  force_warm_start_state(true);

  // Start InitThread, but do not run to completion.
  {
    InSequence seq;
    EXPECT_CALL(*m_init_thread, Start())
        .WillOnce(Return(swss::StatusCode::SWSS_RC_SUCCESS));

    // Cleanup sequence.
    EXPECT_CALL(*m_init_thread, GetResponse())
        .WillOnce(Return(default_done_status()));
  }

  start_rebootbe();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::NsfManagerStatus::NSF_INIT_WAIT);

  // Send a warmboot request, confirm it fails.
  RebootRequest request;
  request.set_method(RebootMethod::NSF);
  start_reboot_via_rpc(request, swss::StatusCode::SWSS_RC_IN_USE);

  std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::NsfManagerStatus::NSF_INIT_WAIT);
}

TEST_F(RebootBETest, ColdbootWhileWarmbootInProgress) {
  force_warm_start_state(true);
  set_mock_defaults();

  // Start InitThread, but do not run to completion.
  {
    InSequence seq;
    EXPECT_CALL(*m_init_thread, Start())
        .WillOnce(Return(swss::StatusCode::SWSS_RC_SUCCESS));

    // Cleanup sequence.
    EXPECT_CALL(*m_init_thread, GetResponse())
        .WillOnce(Return(default_done_status()));
  }

  start_rebootbe();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::NsfManagerStatus::NSF_INIT_WAIT);

  // Send a coldboot request, confirm it starts.
  RebootRequest request;
  request.set_method(RebootMethod::COLD);
  start_reboot_via_rpc(request);

  std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::NsfManagerStatus::COLD_REBOOT_IN_PROGRESS);

  // Cleanup without going through the whole reboot.
  send_stop_reboot_thread();
}

TEST_F(RebootBETestWithoutStop, WarmbootStopDuringInit) {
  force_warm_start_state(true);

  {
    InSequence seq;
    EXPECT_CALL(*m_init_thread, Start())
        .WillOnce(Return(swss::StatusCode::SWSS_RC_SUCCESS));

    // Stop triggers the cleanup sequnce without either of the SelectableEvent's
    // being triggered.
    EXPECT_CALL(*m_init_thread, GetResponse())
        .WillOnce(Return(default_running_status()));
    EXPECT_CALL(*m_init_thread, Stop()).Times(1);
    EXPECT_CALL(*m_init_thread, Join()).WillOnce(Return(true));
  }

  start_rebootbe();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::NsfManagerStatus::NSF_INIT_WAIT);

  // Manually join and verify state.
  m_rebootbe.Stop();
  m_rebootbe_thread->join();
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::NsfManagerStatus::NSF_INIT_WAIT);
}

TEST_F(RebootBETestWithoutStop, WarmbootErrorBeforeUnfreeze) {
  force_warm_start_state(true);

  {
    InSequence seq;
    // Immediately report an error from the InitThread after starting.
    auto done_lambda = [&] {
      get_init_done_select().notify();
      return swss::StatusCode::SWSS_RC_SUCCESS;
    };
    EXPECT_CALL(*m_init_thread, Start()).WillOnce(Invoke(done_lambda));

    // Normal Join sequence when reaching COMPLETED.
    EXPECT_CALL(*m_init_thread, GetResponse())
        .WillOnce(Return(default_error_status()))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_init_thread, Join()).WillOnce(Return(false));

    // Cleanup sequence.
    EXPECT_CALL(*m_init_thread, GetResponse())
        .WillOnce(Return(default_done_status()));
  }

  start_rebootbe();

  // Immediately handle InitThread error and become IDLE.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);

  // Manually join and verify state.
  m_rebootbe.Stop();
  m_rebootbe_thread->join();
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);
}

TEST_F(RebootBETestWithoutStop, WarmbootErrorBeforeComplete) {
  force_warm_start_state(true);

  {
    InSequence seq;
    EXPECT_CALL(*m_init_thread, Start())
        .WillOnce(Return(swss::StatusCode::SWSS_RC_SUCCESS));

    // Normal Join sequence when reaching COMPLETED.
    EXPECT_CALL(*m_init_thread, GetResponse())
        .WillOnce(Return(default_error_status()))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_init_thread, Join()).WillOnce(Return(false));

    // Cleanup sequnce.
    EXPECT_CALL(*m_init_thread, GetResponse())
        .WillOnce(Return(default_error_status()));
    EXPECT_CALL(*m_init_thread, Stop()).Times(1);
    EXPECT_CALL(*m_init_thread, Join()).WillOnce(Return(false));
  }

  start_rebootbe();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::NsfManagerStatus::NSF_INIT_WAIT);

  // Advance to waiting for unfreeze.
  get_stack_unfrozen_select().notify();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::NsfManagerStatus::NSF_INIT_WAIT);

  // Triggered as part of InitThread error reporting.
  get_init_done_select().notify();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);

  // Manually join and verify state.
  m_rebootbe.Stop();
  m_rebootbe_thread->join();
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);
}

// Test fixture to skip through the startup sequence into the main loop.
// Param indicates if RebootBE should be initialized into a state where the
// system came up in warmboot.
class RebootBEAutoStartTest : public RebootBETest,
                              public ::testing::WithParamInterface<bool> {
 protected:
  RebootBEAutoStartTest() {
    force_warm_start_state(GetParam());

    if (GetParam()) {
      EXPECT_CALL(*m_init_thread, Start())
          .WillOnce(Return(swss::StatusCode::SWSS_RC_SUCCESS));
      EXPECT_CALL(*m_init_thread, Join()).WillOnce(Return(true));
      EXPECT_CALL(*m_init_thread, GetResponse())
          .WillOnce(Return(default_running_status()))
          .WillRepeatedly(Return(default_done_status()));
    } else {
      EXPECT_CALL(*m_init_thread, GetResponse())
          .WillRepeatedly(Return(default_not_started_status()));
    }

    start_rebootbe();

    if (GetParam()) {
      get_stack_unfrozen_select().notify();
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      get_init_done_select().notify();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);
  }
};

// Normal operation testing.
TEST_P(RebootBEAutoStartTest, NonExistentMessage) {
  swss::NotificationConsumer consumer(&m_db,
                                      REBOOT_RESPONSE_NOTIFICATION_CHANNEL);

  // No "MESSAGE" in field/values
  SendRebootRequest("Reboot", "StatusCode", "field1", "field1_value");
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);

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
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);

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
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);

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

  EXPECT_CALL(m_critical_interface,
              report_critical_state("platform failed to reboot"))
      .Times(3);
  overwrite_reboot_timeout(1);

  RebootRequest request;
  request.set_method(RebootMethod::COLD);
  start_reboot_via_rpc(request);

  std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::NsfManagerStatus::COLD_REBOOT_IN_PROGRESS);
  sleep(TWO_SECONDS);

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);
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

TEST_P(RebootBEAutoStartTest, TestColdBootSigterm) {
  sigterm_requested = true;
  set_mock_defaults();
  overwrite_reboot_timeout(1);

  RebootRequest request;
  request.set_method(RebootMethod::COLD);
  start_reboot_via_rpc(request);

  sleep(ONE_SECOND);

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);
  gnoi::system::RebootStatusResponse second_resp = do_reboot_status_rpc();
  EXPECT_THAT(second_resp, ActiveCountMethod(false, 1, RebootMethod::COLD));
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

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);
  gnoi::system::RebootStatusResponse second_resp = do_reboot_status_rpc();
  EXPECT_THAT(second_resp, ActiveCountMethod(false, 1, RebootMethod::COLD));
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
            RebootBE::NsfManagerStatus::COLD_REBOOT_IN_PROGRESS);

  send_stop_reboot_thread();
  std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);

  gnoi::system::RebootStatusResponse response = do_reboot_status_rpc();
  EXPECT_THAT(response, ActiveCountMethod(false, 1, RebootMethod::COLD));
  EXPECT_THAT(
      response,
      IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN, ""));
}

TEST_P(RebootBEAutoStartTest, TestNSFToCompletion) {
  set_mock_defaults();

  EXPECT_CALL(m_critical_interface,
              report_critical_state("platform failed to reboot"))
      .Times(1);

  set_telemetry_overall_expects(false);
  set_telemetry_stage_expects(WarmBootStage::STAGE_FREEZE, true);
  set_telemetry_stage_expects(WarmBootStage::STAGE_CHECKPOINT, true);

  overwrite_reboot_timeout(1);
  overwrite_quiescent_hold_time_ms(100);

  // skip state verification
  TestUtils::set_state_verification_enable(m_config_db, false, false);

  TestUtils::populate_registration_table(m_db, "docker1|app1", false, true,
                                         true, false);

  auto test_sequence = [&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));
    EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
              RebootBE::NsfManagerStatus::NSF_REBOOT_IN_PROGRESS);
    TestUtils::populate_restart_table_state(
        m_db, "app1", get_warm_start_state_name(WarmStartState::CHECKPOINTED));
  };

  RebootRequest request;
  request.set_method(RebootMethod::NSF);
  start_reboot_via_rpc(request);

  // Warm start states are cleared at beginning of NSF boot.
  std::thread test_thread = std::thread(test_sequence);

  // 1 second reboot timeout
  // 1/10 second delay before warm state is written in test
  // 1/10 second delay for quiescent hold time
  std::this_thread::sleep_for(std::chrono::milliseconds(TWO_SECONDS_MS));

  gnoi::system::RebootStatusResponse response = do_reboot_status_rpc();
  test_thread.join();

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);
  EXPECT_THAT(response, ActiveCountMethod(false, 1, RebootMethod::NSF));
  EXPECT_THAT(response,
              IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
                       "platform failed to reboot"));
  TestUtils::check_warmboot_enabled(m_db, /*expected_state=*/false);
}

TEST_P(RebootBEAutoStartTest, TestStateVerificationFailedTimeout) {
  set_mock_defaults();

  overwrite_state_verification_timeout(1);
  set_telemetry_overall_expects(/*success=*/false);

  TestUtils::set_state_verification_enable(m_config_db, false, true);

  // Empty registration: if we fail, it should be because of
  // state verification
  TestUtils::populate_registration_table(m_db, "docker1|app1", false, false,
                                         false, false);

  RebootRequest request;
  request.set_method(RebootMethod::NSF);
  start_reboot_via_rpc(request);

  std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::NsfManagerStatus::NSF_REBOOT_IN_PROGRESS);

  // We have to wait for the 1 second state verification timeout
  std::this_thread::sleep_for(std::chrono::milliseconds(TWO_SECONDS_MS));

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);
  gnoi::system::RebootStatusResponse response = do_reboot_status_rpc();
  EXPECT_THAT(response, ActiveCountMethod(false, 1, RebootMethod::NSF));
  EXPECT_THAT(
      response,
      IsStatus(
          RebootStatus_Status::RebootStatus_Status_STATUS_RETRIABLE_FAILURE,
          "timeout occurred during reboot state verification: retriable "
          "error"));
  TestUtils::check_warmboot_enabled(m_db, /*expected_state=*/false);
}

TEST_P(RebootBEAutoStartTest, TestQuiescenceFailedTimeout) {
  set_mock_defaults();

  overwrite_quiescent_timeout_ms(400);
  set_all_telemetry_expects(/*freeze_status=*/false,
                            /*checkpoint_status=*/false);

  TestUtils::set_state_verification_enable(m_config_db, false, false);

  TestUtils::populate_registration_table(m_db, "docker1|app1", false, true,
                                         false, false);

  RebootRequest request;
  request.set_method(RebootMethod::NSF);
  start_reboot_via_rpc(request);

  std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::NsfManagerStatus::NSF_REBOOT_IN_PROGRESS);

  // We have to wait for the 1 second quiescence
  std::this_thread::sleep_for(std::chrono::milliseconds(ONE_SECOND_MS));

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);
  gnoi::system::RebootStatusResponse response = do_reboot_status_rpc();
  EXPECT_THAT(response, ActiveCountMethod(false, 1, RebootMethod::NSF));
  EXPECT_THAT(response,
              IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
                       "timeout occurred during reboot stage freeze"));
  TestUtils::check_warmboot_enabled(m_db, /*expected_state=*/false);
}

TEST_P(RebootBEAutoStartTest, TestCheckpointFailedTimeout) {
  set_mock_defaults();

  overwrite_checkpoint_timeout(1);
  overwrite_quiescent_hold_time_ms(100);
  set_all_telemetry_expects(/*freeze_status=*/true,
                            /*checkpoint_status=*/false);

  TestUtils::set_state_verification_enable(m_config_db, false, false);

  TestUtils::populate_registration_table(m_db, "docker1|app1", false, false,
                                         true, false);

  RebootRequest request;
  request.set_method(RebootMethod::NSF);
  start_reboot_via_rpc(request);

  std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::NsfManagerStatus::NSF_REBOOT_IN_PROGRESS);

  // 1/10 second for quiescence hold time
  // 1 second for checkpoint timeout
  std::this_thread::sleep_for(std::chrono::milliseconds(TWO_SECONDS_MS));

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);
  gnoi::system::RebootStatusResponse response = do_reboot_status_rpc();
  EXPECT_THAT(response, ActiveCountMethod(false, 1, RebootMethod::NSF));
  EXPECT_THAT(
      response,
      IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
               HasSubstr("timeout occurred during reboot stage checkpoint")));
  TestUtils::check_warmboot_enabled(m_db, /*expected_state=*/false);
}

TEST_P(RebootBEAutoStartTest, TestNSFDbusRebootError) {
  // Return FAIL from dbus reboot call.
  DbusInterface::DbusResponse dbus_response{
      DbusInterface::DbusStatus::DBUS_FAIL, "dbus reboot failed"};
  EXPECT_CALL(m_dbus_interface, Reboot(_))
      .Times(1)
      .WillOnce(Return(dbus_response));
  set_telemetry_overall_expects(/*status=*/false);
  set_telemetry_stage_expects(WarmBootStage::STAGE_FREEZE, /*status=*/true);
  set_telemetry_stage_expects(WarmBootStage::STAGE_CHECKPOINT,
                              /*status=*/true);

  overwrite_quiescent_hold_time_ms(100);

  TestUtils::set_state_verification_enable(m_config_db, false, false);

  // Empty registration.
  TestUtils::populate_registration_table(m_db, "docker1|app1", false, false,
                                         false, false);

  RebootRequest request;
  request.set_method(RebootMethod::NSF);
  start_reboot_via_rpc(request);

  // Short wait: there should be no state verification, checkpoint or
  //             or platform reboot delays
  // 1/10 second for quiescent hold time
  //     the quiescent select timeout is 250ms
  std::this_thread::sleep_for(std::chrono::milliseconds(HALF_SECOND_MS));

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);
  gnoi::system::RebootStatusResponse response = do_reboot_status_rpc();
  EXPECT_THAT(response, ActiveCountMethod(false, 1, RebootMethod::NSF));
  EXPECT_THAT(response,
              IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
                       "dbus reboot failed"));
  TestUtils::check_warmboot_enabled(m_db, /*expected_state=*/false);
}

// Test redis tables are cleared.
// - warm boot states should be cleared
// - existing apps in init table should be cleared
TEST_P(RebootBEAutoStartTest, TestRedisNSFSetup) {
  set_mock_defaults();
  set_telemetry_overall_expects(/*success=*/false);

  TestUtils::populate_restart_table_state(
      m_db, "app1", get_warm_start_state_name(WarmStartState::CHECKPOINTED));

  swss::Table warmRestartTable(&m_db, STATE_WARM_RESTART_TABLE_NAME);
  std::string state = "";
  warmRestartTable.hget("app1", "state", state);
  EXPECT_EQ(state, get_warm_start_state_name(WarmStartState::CHECKPOINTED));

  swss::Table initTable(&m_db, STATE_WARM_RESTART_INIT_TABLE_NAME);
  initTable.hset("docker2|app2", "timestamp", "fake-timestamp");
  std::string timestamp = "";
  initTable.hget("docker2|app2", "timestamp", timestamp);
  EXPECT_THAT(timestamp, StrEq("fake-timestamp"));

  overwrite_state_verification_timeout(1);

  TestUtils::set_state_verification_enable(m_config_db, false, true);

  // Empty registration: if we fail, it should be because of
  // state verification
  TestUtils::populate_registration_table(m_db, "docker1|app1", false, false,
                                         false, false);

  RebootRequest request;
  request.set_method(RebootMethod::NSF);
  start_reboot_via_rpc(request);

  std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::NsfManagerStatus::NSF_REBOOT_IN_PROGRESS);

  // We have to wait for the 1 second state verification timeout
  std::this_thread::sleep_for(std::chrono::milliseconds(TWO_SECONDS_MS));

  state = "";
  warmRestartTable.hget("app1", "state", state);
  EXPECT_NE(state, get_warm_start_state_name(WarmStartState::CHECKPOINTED));

  timestamp = "";
  initTable.hget("docker2|app2", "timestamp", timestamp);
  EXPECT_NE(timestamp, "fake-timestamp");

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);
  gnoi::system::RebootStatusResponse response = do_reboot_status_rpc();
  EXPECT_THAT(response, ActiveCountMethod(false, 1, RebootMethod::NSF));
  EXPECT_THAT(
      response,
      IsStatus(
          RebootStatus_Status::RebootStatus_Status_STATUS_RETRIABLE_FAILURE,
          HasSubstr("timeout occurred during reboot state verification: "
                    "retriable error")));
  TestUtils::check_warmboot_enabled(m_db, /*expected_state=*/false);
}

TEST_P(RebootBEAutoStartTest, TestNSFFailureFollowedByColdBoot) {
  DbusInterface::DbusResponse dbus_response{
      DbusInterface::DbusStatus::DBUS_SUCCESS, ""};
  EXPECT_CALL(m_dbus_interface, Reboot(_))
      .Times(1)
      .WillRepeatedly(Return(dbus_response));

  EXPECT_CALL(m_critical_interface,
              report_critical_state("platform failed to reboot"))
      .Times(1);
  overwrite_reboot_timeout(1);
  overwrite_checkpoint_timeout(1);
  overwrite_quiescent_hold_time_ms(100);

  set_all_telemetry_expects(/*freeze_status=*/true,
                            /*checkpoint_status=*/false);

  TestUtils::set_state_verification_enable(m_config_db, false, false);

  TestUtils::populate_registration_table(m_db, "docker1|app1", false, false,
                                         true, false);

  RebootRequest request;
  request.set_method(RebootMethod::NSF);
  start_reboot_via_rpc(request);

  std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));
  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::NsfManagerStatus::NSF_REBOOT_IN_PROGRESS);

  // 1/10 second quiescence hold time
  // 1 second checkpoint timeout
  std::this_thread::sleep_for(std::chrono::milliseconds(TWO_SECONDS_MS));

  gnoi::system::RebootStatusResponse response = do_reboot_status_rpc();

  EXPECT_THAT(response, ActiveCountMethod(false, 1, RebootMethod::NSF));
  EXPECT_THAT(
      response,
      IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
               HasSubstr("timeout occurred during reboot stage checkpoint")));

  request.set_method(RebootMethod::COLD);
  start_reboot_via_rpc(request);

  // We have to wait for the 1 second reboot Timeout
  std::this_thread::sleep_for(std::chrono::milliseconds(TWO_SECONDS_MS));

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);
  response = do_reboot_status_rpc();
  EXPECT_THAT(response, ActiveCountMethod(false, 2, RebootMethod::COLD));
  EXPECT_THAT(response,
              IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
                       "platform failed to reboot"));
  TestUtils::check_warmboot_enabled(m_db, /*expected_state=*/false);
}

TEST_P(RebootBEAutoStartTest, TestInvalidJsonRebootRequest) {
  std::string json_request = "abcd";
  NotificationResponse response = handle_reboot_request(json_request);
  EXPECT_EQ(swss::StatusCode::SWSS_RC_INTERNAL, response.status);
}

TEST_P(RebootBEAutoStartTest, TestStopDuringRebootStateVerification) {
  set_telemetry_overall_expects(/*success=*/false);

  // Enable state verification with default 260 sec timeout)
  TestUtils::set_state_verification_enable(m_config_db, false, true);

  // Empty registration.
  TestUtils::populate_registration_table(m_db, "docker1|app1", false, false,
                                         false, false);

  RebootRequest request;
  request.set_method(RebootMethod::NSF);
  start_reboot_via_rpc(request);

  std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::NsfManagerStatus::NSF_REBOOT_IN_PROGRESS);
  gnoi::system::RebootStatusResponse response = do_reboot_status_rpc();
  EXPECT_THAT(response, ActiveCountMethod(true, 1, RebootMethod::NSF));
  TestUtils::check_warmboot_enabled(m_db, /*expected_state=*/true);

  // Reboot thread is active: its waiting for state verification to complete
  // TearDown will call rebootbe.Stop() which will Stop and Join the
  // reboot thread
}

TEST_P(RebootBEAutoStartTest, TestStopDuringRebootFreezeStage) {
  set_all_telemetry_expects(/*freeze_status=*/false,
                            /*checkpoint_status=*/false);

  // Disable state verification
  TestUtils::set_state_verification_enable(m_config_db, false, false);

  // Register for checkpoint monitoring
  TestUtils::populate_registration_table(m_db, "docker1|app1", false, true,
                                         false, false);

  RebootRequest request;
  request.set_method(RebootMethod::NSF);
  start_reboot_via_rpc(request);

  std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::NsfManagerStatus::NSF_REBOOT_IN_PROGRESS);
  gnoi::system::RebootStatusResponse response = do_reboot_status_rpc();
  EXPECT_THAT(response, ActiveCountMethod(true, 1, RebootMethod::NSF));
  TestUtils::check_warmboot_enabled(m_db, /*expected_state=*/true);

  // Reboot thread is active: its waiting for app1 to quiesce.
  // TearDown will call rebootbe.Stop() which will Stop and Join the
  // reboot thread
}

TEST_P(RebootBEAutoStartTest, TestStopDuringRebootCheckpointStage) {
  set_all_telemetry_expects(/*freeze_status=*/true,
                            /*checkpoint_status=*/false);

  // Disable state verification.
  overwrite_quiescent_hold_time_ms(10);
  TestUtils::set_state_verification_enable(m_config_db, false, false);

  // Register app1 for checkpointing.
  TestUtils::populate_registration_table(m_db, "docker1|app1", false, false,
                                         true, false);

  RebootRequest request;
  request.set_method(RebootMethod::NSF);
  start_reboot_via_rpc(request);

  // With the short hold time we need 250+ms to allow the quiescence hold
  // time select timeout to fire.
  std::this_thread::sleep_for(std::chrono::milliseconds(HALF_SECOND_MS));

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::NsfManagerStatus::NSF_REBOOT_IN_PROGRESS);
  gnoi::system::RebootStatusResponse response = do_reboot_status_rpc();
  EXPECT_THAT(response, ActiveCountMethod(true, 1, RebootMethod::NSF));
  TestUtils::check_warmboot_enabled(m_db, /*expected_state=*/true);

  // Reboot thread is active: its waiting for app1 to checkpoint.
  // TearDown will call rebootbe.Stop() which will Stop and Join the
  // reboot thread
}

TEST_P(RebootBEAutoStartTest, TestStopDuringWaitPlatformReboot) {
  set_telemetry_overall_expects(/*status=*/false);
  set_telemetry_stage_expects(WarmBootStage::STAGE_FREEZE, /*status=*/true);
  set_telemetry_stage_expects(WarmBootStage::STAGE_CHECKPOINT,
                              /*status=*/true);

  // Disable state verification.
  overwrite_quiescent_hold_time_ms(10);
  TestUtils::set_state_verification_enable(m_config_db, false, false);

  // Empty registration.
  TestUtils::populate_registration_table(m_db, "docker1|app1", false, false,
                                         false, false);

  RebootRequest request;
  request.set_method(RebootMethod::NSF);
  start_reboot_via_rpc(request);

  // With the short hold time we need 250+ms to allow the quiescence hold
  // time select timeout to fire.
  std::this_thread::sleep_for(std::chrono::milliseconds(HALF_SECOND_MS));

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::NsfManagerStatus::NSF_REBOOT_IN_PROGRESS);
  gnoi::system::RebootStatusResponse response = do_reboot_status_rpc();
  EXPECT_THAT(response, ActiveCountMethod(true, 1, RebootMethod::NSF));
  TestUtils::check_warmboot_enabled(m_db, /*expected_state=*/true);

  // Reboot thread is active: its waiting for the platform to reboot.
  // TearDown will call rebootbe.Stop() which will Stop and Join the
  // reboot thread
}

//
// Stop On Freeze Tests
//
TEST_P(RebootBEAutoStartTest, TestStopDuringWaitForStopOnFreeze) {
  set_all_telemetry_expects(false);

  overwrite_quiescent_hold_time_ms(50);
  overwrite_quiescent_timeout_ms(1000);

  DbusInterface::DbusResponse dbus_response{
      DbusInterface::DbusStatus::DBUS_SUCCESS, ""};
  EXPECT_CALL(m_dbus_interface, StopContainerStatus(_))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(dbus_response));

  // Disable state verification.
  TestUtils::set_state_verification_enable(m_config_db, false, false);

  // Register app1 for checkpointing.
  TestUtils::populate_registration_table(m_db, "docker1|app1", true, false,
                                         false, false);

  RebootRequest request;
  request.set_method(RebootMethod::NSF);
  start_reboot_via_rpc(request);

  std::this_thread::sleep_for(std::chrono::milliseconds(TENTH_SECOND_MS));

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(),
            RebootBE::NsfManagerStatus::NSF_REBOOT_IN_PROGRESS);
  gnoi::system::RebootStatusResponse response = do_reboot_status_rpc();
  EXPECT_THAT(response, ActiveCountMethod(true, 1, RebootMethod::NSF));
  TestUtils::check_warmboot_enabled(m_db, /*expected_state=*/true);

  // Reboot thread is active: its waiting for docker1 to exit/stop
  // TearDown will call rebootbe.Stop() which will Stop and Join the
  // reboot thread
}

TEST_P(RebootBEAutoStartTest, TestStopOnFreezeTimeout) {
  set_all_telemetry_expects(false);

  overwrite_quiescent_hold_time_ms(50);
  overwrite_quiescent_timeout_ms(1000);

  // An empty string is not a valid json stop container status response
  DbusInterface::DbusResponse dbus_response{
      DbusInterface::DbusStatus::DBUS_SUCCESS, ""};
  EXPECT_CALL(m_dbus_interface, StopContainerStatus(_))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(dbus_response));

  TestUtils::set_state_verification_enable(m_config_db, false, false);

  TestUtils::populate_registration_table(m_db, "docker1|app1", true, false,
                                         false, false);

  RebootRequest request;
  request.set_method(RebootMethod::NSF);
  start_reboot_via_rpc(request);

  // Container stop status is checked every half second.
  std::this_thread::sleep_for(std::chrono::milliseconds(FIFTEEN_HUNDRED_MS));

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);
  gnoi::system::RebootStatusResponse response = do_reboot_status_rpc();
  EXPECT_THAT(response, ActiveCountMethod(false, 1, RebootMethod::NSF));
  EXPECT_THAT(response,
              IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
                       "timeout occurred waiting for containers to stop"));
  TestUtils::check_warmboot_enabled(m_db, /*expected_state=*/false);
}

TEST_P(RebootBEAutoStartTest, TestDbusErrorRequestingContainerStop) {
  set_all_telemetry_expects(false);

  // Return FAIL from dbus reboot call.
  DbusInterface::DbusResponse dbus_response{
      DbusInterface::DbusStatus::DBUS_FAIL,
      "dbus error calling StopContainers"};
  EXPECT_CALL(m_dbus_interface, StopContainers(_))
      .Times(1)
      .WillOnce(Return(dbus_response));

  TestUtils::set_state_verification_enable(m_config_db, false, false);

  TestUtils::populate_registration_table(m_db, "docker1|app1", true, false,
                                         false, false);

  RebootRequest request;
  request.set_method(RebootMethod::NSF);
  start_reboot_via_rpc(request);

  // Container stop status is checked every half second.
  std::this_thread::sleep_for(std::chrono::milliseconds(ONE_SECOND_MS));

  EXPECT_EQ(m_rebootbe.GetCurrentStatus(), RebootBE::NsfManagerStatus::IDLE);

  gnoi::system::RebootStatusResponse response = do_reboot_status_rpc();
  EXPECT_THAT(response, ActiveCountMethod(false, 1, RebootMethod::NSF));
  EXPECT_THAT(response,
              IsStatus(RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE,
                       "dbus error calling StopContainers"));
  TestUtils::check_warmboot_enabled(m_db, /*expected_state=*/false);
}

INSTANTIATE_TEST_SUITE_P(TestWithStartupWarmbootEnabledState,
                         RebootBEAutoStartTest, testing::Values(true, false));

}  // namespace rebootbackend
