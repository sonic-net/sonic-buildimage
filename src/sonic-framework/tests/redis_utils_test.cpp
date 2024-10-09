#include "redis_utils.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <string>
#include <thread>
#include <vector>

#include "select.h"
#include "stateverification.h"
#include "table.h"
#include "test_utils_common.h"
#include "timestamp.h"
#include "warm_restart.h"

namespace rebootbackend {

using WarmStartState = ::swss::WarmStart::WarmStartState;
using WarmBootStage = ::swss::WarmStart::WarmBootStage;
using WarmBootNotification = ::swss::WarmStart::WarmBootNotification;

using ::testing::AllOf;
using ::testing::HasSubstr;
using ::testing::StrEq;

class RedisTest : public ::testing::Test {
 protected:
  RedisTest() : m_db("STATE_DB", 0), m_reg(), m_init_reg() {
    TestUtils::clear_tables(m_db);
  }

  swss::DBConnector m_db;
  Registration m_reg;
  InitRegistration m_init_reg;

  void clear_contents() { return m_reg.clear_contents(); }

  size_t get_state_set_size(WarmBootStage nsf_stage) {
    return m_reg.m_remaining_apps.at(nsf_stage).size();
  }

  size_t get_reregistration_set_size() {
    return m_init_reg.m_missing_registrations.size();
  }

  // Special version of name mapping that is compatible with the DB values.
  std::string warm_boot_stage_name(WarmBootStage stage) {
    switch (stage) {
      case (WarmBootStage::STAGE_FREEZE): {
        return "quiescent";
      }
      case (WarmBootStage::STAGE_CHECKPOINT): {
        return "checkpointed";
      }
      case (WarmBootStage::STAGE_RECONCILIATION): {
        return "reconciled";
      }
      case (WarmBootStage::STAGE_UNFREEZE): {
        return "completed";
      }
      default: {
        return "";
      }
    }
  }

  void populate_default_init_table() {
    swss::Table initTable(&m_db, STATE_WARM_RESTART_INIT_TABLE_NAME);
    initTable.hset("docker1|app1", "timestamp", "");
    initTable.hset("docker2|app2", "timestamp", "");
    initTable.hset("docker3|app3", "timestamp", "");
    initTable.hset("docker4|app1", "timestamp", "");
    // The invalid entry should not end up in the list of apps.
    initTable.hset("invalid", "timestamp", "");
  }

  friend class Registration;
};

TEST_F(RedisTest, testSendNsfManagerNotification) {
  swss::NotificationConsumer nc(
      &m_db, swss::WarmStart::kNsfManagerNotificationChannel);
  swss::Select s;
  s.addSelectable(&nc);

  send_nsf_manager_notification(m_db, WarmBootNotification::kFreeze);

  swss::Selectable *sel;
  bool ret = s.select(&sel, 1);
  EXPECT_EQ(ret, swss::Select::OBJECT);

  std::string op, data;
  std::vector<swss::FieldValueTuple> values;
  nc.pop(op, data, values);
  auto fv = values[0];

  EXPECT_EQ(op, swss::WarmStart::warmBootNotificationNameMap()->at(
                    WarmBootNotification::kFreeze));

  send_nsf_manager_notification(m_db, WarmBootNotification::kUnfreeze);

  ret = s.select(&sel, 1);
  EXPECT_EQ(ret, swss::Select::OBJECT);

  nc.pop(op, data, values);
  fv = values[0];

  EXPECT_EQ(op, swss::WarmStart::warmBootNotificationNameMap()->at(
                    WarmBootNotification::kUnfreeze));

  send_nsf_manager_notification(m_db, WarmBootNotification::kCheckpoint);

  ret = s.select(&sel, 1);
  EXPECT_EQ(ret, swss::Select::OBJECT);

  nc.pop(op, data, values);
  fv = values[0];

  EXPECT_EQ(op, swss::WarmStart::warmBootNotificationNameMap()->at(
                    WarmBootNotification::kCheckpoint));
}

TEST_F(RedisTest, testSendStateVerification) {
  swss::NotificationConsumer nc(&m_db, STATE_VERIFICATION_REQ_CHANNEL);
  swss::Select s;
  s.addSelectable(&nc);

  std::string timestamp = send_state_verification_notification(m_db, true);

  swss::Selectable *sel;
  bool ret = s.select(&sel, 1);
  EXPECT_EQ(ret, swss::Select::OBJECT);

  std::string op, data;
  std::vector<swss::FieldValueTuple> values;
  nc.pop(op, data, values);
  auto fv = values[0];

  EXPECT_EQ(op, ALL_COMPONENT);
  EXPECT_EQ(data, timestamp);
  EXPECT_EQ(FREEZE_FIELD, fvField(fv));
  EXPECT_EQ("true", fvValue(fv));

  std::string second_timestamp =
      send_state_verification_notification(m_db, false);

  ret = s.select(&sel, 1);
  EXPECT_EQ(ret, swss::Select::OBJECT);

  nc.pop(op, data, values);
  fv = values[0];

  EXPECT_EQ(op, ALL_COMPONENT);
  EXPECT_EQ(data, second_timestamp);
  EXPECT_EQ(FREEZE_FIELD, fvField(fv));
  EXPECT_EQ("false", fvValue(fv));
  EXPECT_NE(timestamp, second_timestamp);
}

TEST_F(RedisTest, testInitWarmRebootStates) {
  swss::Table warmRestartTable(&m_db, STATE_WARM_RESTART_TABLE_NAME);

  warmRestartTable.hset("app1", "state", "disabled");
  warmRestartTable.hset("app1", "timestamp", "abcdefg");
  warmRestartTable.hset("app2", "state", "reconciled");
  warmRestartTable.hset("app2", "timestamp", "zyxwvu");

  std::string value;
  bool ret = warmRestartTable.hget("app1", "state", value);
  EXPECT_TRUE(ret);

  ret = warmRestartTable.hget("app1", "timestamp", value);
  EXPECT_TRUE(ret);

  init_warm_reboot_states(m_db);

  ret = warmRestartTable.hget("app1", "state", value);
  EXPECT_FALSE(ret);

  ret = warmRestartTable.hget("app1", "timestamp", value);
  EXPECT_FALSE(ret);

  ret = warmRestartTable.hget("app2", "state", value);
  EXPECT_FALSE(ret);

  ret = warmRestartTable.hget("app2", "timestamp", value);
  EXPECT_FALSE(ret);
}

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

TEST_F(RedisTest, TestIsValidKeyAndGetDockerAppFromKey) {
  std::string key = "abc|def";
  std::string separator = "|";
  std::string docker, app;

  EXPECT_TRUE(is_valid_key(key, separator));
  EXPECT_TRUE(get_docker_app_from_key(key, separator, docker, app));
  EXPECT_EQ(docker, "abc");
  EXPECT_EQ(app, "def");

  key = "abcd|";
  EXPECT_FALSE(is_valid_key(key, separator));
  EXPECT_FALSE(get_docker_app_from_key(key, separator, docker, app));

  key = "|abcd";
  EXPECT_FALSE(is_valid_key(key, separator));
  EXPECT_FALSE(get_docker_app_from_key(key, separator, docker, app));

  key = "abcd";
  EXPECT_FALSE(is_valid_key(key, separator));
  EXPECT_FALSE(get_docker_app_from_key(key, separator, docker, app));

  separator = "";
  key = "abc|def";
  EXPECT_FALSE(is_valid_key(key, separator));
  EXPECT_FALSE(get_docker_app_from_key(key, separator, docker, app));
}

TEST_F(RedisTest, GetWarmRestartCounter) {
  EXPECT_THAT(get_warm_restart_counter(m_db), StrEq(""));
  for (int i = 0; i < 5; i++) {
    set_warm_restart_counter(m_db, i);
    EXPECT_THAT(get_warm_restart_counter(m_db), StrEq(std::to_string(i)));
  }
}

TEST_F(RedisTest, TestFetchRegistrationInfo) {
  TestUtils::populate_registration_table(m_db, "invalid", false, false, false,
                                         true);
  TestUtils::populate_registration_table(m_db, "docker1|app1", false, false,
                                         false, true);
  TestUtils::populate_registration_table(m_db, "docker2|app2", true, true, true,
                                         false);
  TestUtils::populate_registration_table(m_db, "docker3|app3", false, false,
                                         true, false);

  m_reg.fetch_registration_info();

  Registration::RegistrationSet set = m_reg.get_registered_app_set();

  EXPECT_TRUE(set.count("docker1|app1"));
  EXPECT_TRUE(set.count("docker2|app2"));
  EXPECT_TRUE(set.count("docker2|app2"));
  EXPECT_EQ(set.size(), 3);
}

TEST_F(RedisTest, TestStopOnFreezeList) {
  TestUtils::populate_registration_table(m_db, "docker1|app1", true, false,
                                         false, true);
  TestUtils::populate_registration_table(m_db, "docker2|app2", true, false,
                                         false, false);
  TestUtils::populate_registration_table(m_db, "docker3|app3", false, false,
                                         true, false);

  m_reg.fetch_registration_info();
  Registration::RegistrationSet set = m_reg.get_stop_on_freeze_set();
  EXPECT_EQ(2, set.size());
  EXPECT_EQ(1, set.count("docker1"));
  EXPECT_EQ(1, set.count("docker2"));
}

TEST_F(RedisTest, TestCheckQuiesced) {
  // No apps registered.
  m_reg.fetch_registration_info();
  Registration::Response response = m_reg.check_quiesced();
  EXPECT_EQ(response.status, Registration::Status::COMPLETED);

  // Apps registered, but have not reached the correct state.
  TestUtils::populate_registration_table(m_db, "docker1|app1", false, true,
                                         false, false);
  TestUtils::populate_registration_table(m_db, "docker2|app2", false, true,
                                         false, false);
  TestUtils::populate_registration_table(m_db, "docker3|app3", false, true,
                                         false, false);
  TestUtils::populate_registration_table(m_db, "docker4|app4", true, false,
                                         false, false);

  m_reg.fetch_registration_info();
  EXPECT_EQ(3, get_state_set_size(WarmBootStage::STAGE_FREEZE));

  // app1 and app2 reach the correct state.
  TestUtils::populate_restart_table_state(
      m_db, "app1", get_warm_start_state_name(WarmStartState::QUIESCENT));
  TestUtils::populate_restart_table_state(
      m_db, "app2", get_warm_start_state_name(WarmStartState::CHECKPOINTED));

  response = m_reg.check_quiesced();
  EXPECT_EQ(response.status, Registration::Status::IN_PROCESS);
  EXPECT_EQ(1, get_state_set_size(WarmBootStage::STAGE_FREEZE));

  // app3 reaches the correct state.
  TestUtils::populate_restart_table_state(
      m_db, "app3", get_warm_start_state_name(WarmStartState::CHECKPOINTED));

  response = m_reg.check_quiesced();
  EXPECT_EQ(response.status, Registration::Status::COMPLETED);
  EXPECT_EQ(0, get_state_set_size(WarmBootStage::STAGE_FREEZE));

  // app3 reports an error.
  m_reg.fetch_registration_info();
  TestUtils::populate_restart_table_state(
      m_db, "app3", get_warm_start_state_name(WarmStartState::FAILED));

  response = m_reg.check_quiesced();
  EXPECT_EQ(response.status, Registration::Status::FAILURE);
  EXPECT_FALSE(response.error_string.empty());
}

TEST_F(RedisTest, TestCheckCheckpointed) {
  // No apps registered.
  m_reg.fetch_registration_info();
  Registration::Response response = m_reg.check_checkpointed();
  EXPECT_EQ(response.status, Registration::Status::COMPLETED);

  // Apps registered, but have not reached the correct state.
  TestUtils::populate_registration_table(m_db, "docker1|app1", false, false,
                                         true, false);
  TestUtils::populate_registration_table(m_db, "docker2|app2", false, false,
                                         true, false);
  TestUtils::populate_registration_table(m_db, "docker3|app3", false, false,
                                         true, false);
  TestUtils::populate_registration_table(m_db, "docker4|app4", true, false,
                                         false, false);

  m_reg.fetch_registration_info();
  EXPECT_EQ(3, get_state_set_size(WarmBootStage::STAGE_CHECKPOINT));

  // app2 reaches the correct state. app1 has changed state, but is not yet
  // checkpointed.
  TestUtils::populate_restart_table_state(
      m_db, "app1", get_warm_start_state_name(WarmStartState::QUIESCENT));
  TestUtils::populate_restart_table_state(
      m_db, "app2", get_warm_start_state_name(WarmStartState::CHECKPOINTED));

  response = m_reg.check_checkpointed();
  EXPECT_EQ(response.status, Registration::Status::IN_PROCESS);
  EXPECT_EQ(2, get_state_set_size(WarmBootStage::STAGE_CHECKPOINT));

  // app1 and app3 reach the correct state.
  TestUtils::populate_restart_table_state(
      m_db, "app1", get_warm_start_state_name(WarmStartState::CHECKPOINTED));
  TestUtils::populate_restart_table_state(
      m_db, "app3", get_warm_start_state_name(WarmStartState::CHECKPOINTED));

  response = m_reg.check_checkpointed();
  EXPECT_EQ(response.status, Registration::Status::COMPLETED);
  EXPECT_EQ(0, get_state_set_size(WarmBootStage::STAGE_CHECKPOINT));

  // app3 reports an error.
  m_reg.fetch_registration_info();
  TestUtils::populate_restart_table_state(
      m_db, "app3", get_warm_start_state_name(WarmStartState::FAILED));

  response = m_reg.check_checkpointed();
  EXPECT_EQ(response.status, Registration::Status::FAILURE);
  EXPECT_FALSE(response.error_string.empty());
}

TEST_F(RedisTest, TestCheckReconciled) {
  // No apps registered.
  m_reg.fetch_registration_info();
  Registration::Response response = m_reg.check_reconciled();
  EXPECT_EQ(response.status, Registration::Status::COMPLETED);

  // Apps registered, but have not reached the correct state.
  TestUtils::populate_registration_table(m_db, "docker1|app1", false, false,
                                         false, true);
  TestUtils::populate_registration_table(m_db, "docker2|app2", false, false,
                                         false, true);
  TestUtils::populate_registration_table(m_db, "docker3|app3", false, false,
                                         false, true);
  TestUtils::populate_registration_table(m_db, "docker4|app4", true, false,
                                         false, false);

  m_reg.fetch_registration_info();
  EXPECT_EQ(3, get_state_set_size(WarmBootStage::STAGE_RECONCILIATION));
  EXPECT_THAT(m_reg.join_pending_apps(WarmBootStage::STAGE_RECONCILIATION),
              AllOf(HasSubstr("app1"), HasSubstr("app2"), HasSubstr("app3")));

  // app1 and app2 reach the correct state.
  TestUtils::populate_restart_table_state(
      m_db, "app1", get_warm_start_state_name(WarmStartState::RECONCILED));
  TestUtils::populate_restart_table_state(
      m_db, "app2", get_warm_start_state_name(WarmStartState::RECONCILED));

  response = m_reg.check_reconciled();
  EXPECT_EQ(response.status, Registration::Status::IN_PROCESS);
  EXPECT_EQ(1, get_state_set_size(WarmBootStage::STAGE_RECONCILIATION));
  EXPECT_THAT(m_reg.join_pending_apps(WarmBootStage::STAGE_RECONCILIATION),
              HasSubstr("app3"));

  // app3 reaches the correct state.
  TestUtils::populate_restart_table_state(
      m_db, "app3", get_warm_start_state_name(WarmStartState::RECONCILED));

  response = m_reg.check_reconciled();
  EXPECT_EQ(response.status, Registration::Status::COMPLETED);
  EXPECT_EQ(0, get_state_set_size(WarmBootStage::STAGE_RECONCILIATION));
  EXPECT_THAT(m_init_reg.join_pending_apps(), StrEq(""));

  // app3 reports an error.
  m_reg.fetch_registration_info();
  TestUtils::populate_restart_table_state(
      m_db, "app3", get_warm_start_state_name(WarmStartState::FAILED));

  response = m_reg.check_reconciled();
  EXPECT_EQ(response.status, Registration::Status::FAILURE);
  EXPECT_FALSE(response.error_string.empty());
  EXPECT_THAT(m_reg.join_pending_apps(WarmBootStage::STAGE_RECONCILIATION),
              HasSubstr("app3"));
}

TEST_F(RedisTest, TestCheckUnfrozen) {
  // No apps registered.
  m_reg.fetch_registration_info();
  Registration::Response response = m_reg.check_unfrozen();
  EXPECT_EQ(response.status, Registration::Status::COMPLETED);

  // Apps registered. app4 reaches the correct state, but the others have not.
  TestUtils::populate_registration_table(m_db, "docker1|app1", false, true,
                                         false, false);
  TestUtils::populate_registration_table(m_db, "docker2|app2", false, true,
                                         false, false);
  TestUtils::populate_registration_table(m_db, "docker3|app3", false, false,
                                         false, true);
  TestUtils::populate_registration_table(m_db, "docker4|app4", true, false,
                                         false, false);
  TestUtils::populate_restart_table_state(
      m_db, "app4", get_warm_start_state_name(WarmStartState::COMPLETED));

  m_reg.fetch_registration_info();
  response = m_reg.check_unfrozen();
  EXPECT_EQ(response.status, Registration::Status::IN_PROCESS);
  EXPECT_EQ(2, get_state_set_size(WarmBootStage::STAGE_UNFREEZE));
  EXPECT_THAT(m_reg.join_pending_apps(WarmBootStage::STAGE_UNFREEZE),
              AllOf(HasSubstr("app1"), HasSubstr("app2")));

  // app1 reaches the correct state.
  TestUtils::populate_restart_table_state(
      m_db, "app1", get_warm_start_state_name(WarmStartState::COMPLETED));

  response = m_reg.check_unfrozen();
  EXPECT_EQ(response.status, Registration::Status::IN_PROCESS);
  EXPECT_EQ(1, get_state_set_size(WarmBootStage::STAGE_UNFREEZE));
  EXPECT_THAT(m_reg.join_pending_apps(WarmBootStage::STAGE_UNFREEZE),
              HasSubstr("app2"));

  // app 2 reaches the correct state. We do not monitor app 3.
  TestUtils::populate_restart_table_state(
      m_db, "app2", get_warm_start_state_name(WarmStartState::COMPLETED));

  response = m_reg.check_unfrozen();
  EXPECT_EQ(response.status, Registration::Status::COMPLETED);
  EXPECT_EQ(0, get_state_set_size(WarmBootStage::STAGE_UNFREEZE));
  EXPECT_THAT(m_reg.join_pending_apps(WarmBootStage::STAGE_UNFREEZE),
              StrEq(""));

  // app1 reports an error.
  m_reg.fetch_registration_info();
  TestUtils::populate_restart_table_state(
      m_db, "app1", get_warm_start_state_name(WarmStartState::FAILED));

  response = m_reg.check_unfrozen();
  EXPECT_EQ(response.status, Registration::Status::FAILURE);
  EXPECT_FALSE(response.error_string.empty());
  EXPECT_THAT(m_reg.join_pending_apps(WarmBootStage::STAGE_UNFREEZE),
              AllOf(HasSubstr("app1")));
}

class RedisTestWithWarmStartState
    : public RedisTest,
      public ::testing::WithParamInterface<WarmBootStage> {};

TEST_P(RedisTestWithWarmStartState, TestEventHandling) {
  // Apps registered. No app has reported state.
  const std::vector<std::string> test_keys(
      {"docker1|app1", "docker2|app2", "docker3|app3"});
  for (const auto &key : test_keys) {
    TestUtils::populate_registration_table(
        m_db, key, false,
        GetParam() == WarmBootStage::STAGE_FREEZE ||
            GetParam() == WarmBootStage::STAGE_UNFREEZE,
        GetParam() == WarmBootStage::STAGE_CHECKPOINT,
        GetParam() == WarmBootStage::STAGE_RECONCILIATION);
  }
  TestUtils::populate_registration_table(m_db, "docker4|app4", true, false,
                                         false, false);
  m_reg.fetch_registration_info();

  // Ignore invalid operation
  swss::KeyOpFieldsValuesTuple state_event = {"app1", "DEL", {{"state", ""}}};
  Registration::Response response =
      m_reg.handle_state_event(GetParam(), state_event);
  EXPECT_EQ(response.status, Registration::Status::IN_PROCESS);
  EXPECT_EQ(3, get_state_set_size(GetParam()));

  // app1 reaches its final state, but the others have not reported state.
  state_event = {"app1", "SET", {{"state", warm_boot_stage_name(GetParam())}}};
  response = m_reg.handle_state_event(GetParam(), state_event);
  EXPECT_EQ(response.status, Registration::Status::IN_PROCESS);
  EXPECT_EQ(2, get_state_set_size(GetParam()));

  // All apps report final state one-by-one.
  for (size_t i = 1; i < test_keys.size(); i++) {
    std::string docker, app;
    bool ret = get_docker_app_from_key(test_keys[i], "|", docker, app);
    EXPECT_TRUE(ret);

    state_event = {app, "SET", {{"state", warm_boot_stage_name(GetParam())}}};
    response = m_reg.handle_state_event(GetParam(), state_event);

    if (i < test_keys.size() - 1) {
      EXPECT_EQ(response.status, Registration::Status::IN_PROCESS);
    } else {
      EXPECT_EQ(response.status, Registration::Status::COMPLETED);
    }
    EXPECT_EQ(get_state_set_size(GetParam()), test_keys.size() - (i + 1));
  }

  // app3 reports an error.
  m_reg.fetch_registration_info();
  state_event = {"app3", "SET", {{"state", "failed"}}};

  response = m_reg.handle_state_event(GetParam(), state_event);
  EXPECT_EQ(response.status, Registration::Status::FAILURE);
  EXPECT_FALSE(response.error_string.empty());
}

TEST_P(RedisTestWithWarmStartState, HandleEventSkipInvalidKey) {
  TestUtils::populate_registration_table(
      m_db, "docker1|app1", false,
      GetParam() == WarmBootStage::STAGE_FREEZE ||
          GetParam() == WarmBootStage::STAGE_UNFREEZE,
      GetParam() == WarmBootStage::STAGE_CHECKPOINT,
      GetParam() == WarmBootStage::STAGE_RECONCILIATION);
  m_reg.fetch_registration_info();

  swss::KeyOpFieldsValuesTuple state_event = {
      "invalid", "SET", {{"state", "completed"}}};

  Registration::Response response =
      m_reg.handle_state_event(GetParam(), state_event);
  EXPECT_EQ(response.status, Registration::Status::IN_PROCESS);
  EXPECT_EQ(get_state_set_size(GetParam()), 1);
}

INSTANTIATE_TEST_SUITE_P(TestOverWarmStateStates, RedisTestWithWarmStartState,
                         testing::Values(WarmBootStage::STAGE_FREEZE,
                                         WarmBootStage::STAGE_CHECKPOINT,
                                         WarmBootStage::STAGE_RECONCILIATION,
                                         WarmBootStage::STAGE_UNFREEZE));

TEST_F(RedisTest, TestHandleQuiescenceEvent) {
  TestUtils::populate_registration_table(m_db, "docker1|app1", false, true,
                                         false, false);
  m_reg.fetch_registration_info();

  swss::KeyOpFieldsValuesTuple state_event = {
      "app1", "DEL", {{"state", "checkpointed"}}};
  Registration::Response response =
      m_reg.handle_state_event(WarmBootStage::STAGE_FREEZE, state_event);
  EXPECT_EQ(response.status, Registration::Status::IN_PROCESS);

  state_event = {"app1", "SET", {{"state", "checkpointed"}}};
  response = m_reg.handle_state_event(WarmBootStage::STAGE_FREEZE, state_event);
  EXPECT_EQ(response.status, Registration::Status::COMPLETED);

  state_event = {"app1", "DEL", {{"state", "completed"}}};
  response = m_reg.handle_state_event(WarmBootStage::STAGE_FREEZE, state_event);
  EXPECT_EQ(response.status, Registration::Status::COMPLETED);

  state_event = {"app1", "SET", {{"state", "failed"}}};
  response = m_reg.handle_state_event(WarmBootStage::STAGE_FREEZE, state_event);
  EXPECT_EQ(response.status, Registration::Status::FAILURE);
}

TEST_F(RedisTest, TestHandleQuiescenceEnterExitCompleteState) {
  TestUtils::populate_registration_table(m_db, "docker1|app1", false, true,
                                         false, false);
  TestUtils::populate_registration_table(m_db, "docker2|app2", false, true,
                                         false, false);
  m_reg.fetch_registration_info();

  swss::KeyOpFieldsValuesTuple state_event = {
      "app1", "SET", {{"state", "quiescent"}}};
  Registration::Response response =
      m_reg.handle_state_event(WarmBootStage::STAGE_FREEZE, state_event);
  EXPECT_EQ(response.status, Registration::Status::IN_PROCESS);

  state_event = {"app2", "SET", {{"state", "checkpointed"}}};
  response = m_reg.handle_state_event(WarmBootStage::STAGE_FREEZE, state_event);
  EXPECT_EQ(response.status, Registration::Status::COMPLETED);

  state_event = {"app1", "SET", {{"state", "replayed"}}};
  response = m_reg.handle_state_event(WarmBootStage::STAGE_FREEZE, state_event);
  EXPECT_EQ(response.status, Registration::Status::IN_PROCESS);

  state_event = {"app1", "SET", {{"state", "quiescent"}}};
  response = m_reg.handle_state_event(WarmBootStage::STAGE_FREEZE, state_event);
  EXPECT_EQ(response.status, Registration::Status::COMPLETED);

  state_event = {"app2", "SET", {{"state", "restored"}}};
  response = m_reg.handle_state_event(WarmBootStage::STAGE_FREEZE, state_event);
  EXPECT_EQ(response.status, Registration::Status::IN_PROCESS);

  state_event = {"app2", "SET", {{"state", "checkpointed"}}};
  response = m_reg.handle_state_event(WarmBootStage::STAGE_FREEZE, state_event);
  EXPECT_EQ(response.status, Registration::Status::COMPLETED);
}

TEST_F(RedisTest, TestClearContents) {
  TestUtils::populate_registration_table(m_db, "docker1|app1", true, false,
                                         false, true);
  TestUtils::populate_registration_table(m_db, "docker2|app2", true, false,
                                         false, false);
  TestUtils::populate_registration_table(m_db, "docker3|app3", false, true,
                                         true, false);
  m_reg.fetch_registration_info();
  Registration::RegistrationSet set = m_reg.get_stop_on_freeze_set();
  EXPECT_EQ(2, set.size());
  EXPECT_EQ(1, get_state_set_size(WarmBootStage::STAGE_FREEZE));
  EXPECT_EQ(1, get_state_set_size(WarmBootStage::STAGE_CHECKPOINT));
  EXPECT_EQ(1, get_state_set_size(WarmBootStage::STAGE_RECONCILIATION));
  EXPECT_EQ(1, get_state_set_size(WarmBootStage::STAGE_UNFREEZE));

  clear_contents();

  set = m_reg.get_stop_on_freeze_set();
  EXPECT_TRUE(set.empty());

  set = m_reg.get_registered_app_set();
  EXPECT_TRUE(set.empty());

  EXPECT_EQ(0, get_state_set_size(WarmBootStage::STAGE_FREEZE));
  EXPECT_EQ(0, get_state_set_size(WarmBootStage::STAGE_CHECKPOINT));
  EXPECT_EQ(0, get_state_set_size(WarmBootStage::STAGE_RECONCILIATION));
  EXPECT_EQ(0, get_state_set_size(WarmBootStage::STAGE_UNFREEZE));
}

TEST_F(RedisTest, TestClearAllInitApps) {
  const std::vector<std::string> kTestKeys(
      {"docker1|app1", "docker2|app2", "docker3|app3", "docker4|app1"});
  for (const auto &key : kTestKeys) {
    TestUtils::populate_registration_table(m_db, key, false, false, false,
                                           true);
  }

  m_reg.fetch_registration_info();
  m_reg.save_all_init_apps();

  swss::Table initTable(&m_db, STATE_WARM_RESTART_INIT_TABLE_NAME);
  std::string value;
  for (const auto &key : kTestKeys) {
    EXPECT_TRUE(initTable.hget(key, "timestamp", value));
  }

  m_reg.clear_all_init_apps();

  for (const auto &key : kTestKeys) {
    EXPECT_FALSE(initTable.hget(key, "timestamp", value));
  }
}

TEST_F(RedisTest, TestSaveInitApps) {
  const std::vector<std::string> kTestKeys(
      {"docker1|app1", "docker2|app2", "docker3|app3", "docker4|app1"});
  for (const auto &key : kTestKeys) {
    TestUtils::populate_registration_table(m_db, key, false, false, false,
                                           true);
  }
  // The invalid entry should not end up in the table.
  TestUtils::populate_registration_table(m_db, "invalid", false, false, false,
                                         true);

  m_reg.fetch_registration_info();
  m_reg.save_all_init_apps();

  swss::Table initTable(&m_db, STATE_WARM_RESTART_INIT_TABLE_NAME);
  std::string value;

  for (const auto &key : kTestKeys) {
    EXPECT_TRUE(initTable.hget(key, "timestamp", value));
  }
}

TEST_F(RedisTest, TestInitTargetApps) {
  // Contains 4 valid apps and 1 invalid app.
  populate_default_init_table();

  m_init_reg.fetch_init_app_info();

  EXPECT_EQ(get_reregistration_set_size(), 4);
}

TEST_F(RedisTest, TestCheckReregistration) {
  populate_default_init_table();

  // Before reading the init table, we do not know apps need to re-register.
  EXPECT_EQ(m_init_reg.check_reregistration_status(),
            InitRegistration::Status::COMPLETED);
  EXPECT_EQ(get_reregistration_set_size(), 0);
  EXPECT_THAT(m_init_reg.join_pending_apps(), StrEq(""));

  // No apps have re-registered. All valid apps are still pending.
  m_init_reg.fetch_init_app_info();

  EXPECT_EQ(m_init_reg.check_reregistration_status(),
            InitRegistration::Status::IN_PROGRESS);
  EXPECT_EQ(get_reregistration_set_size(), 4);
  EXPECT_THAT(m_init_reg.join_pending_apps(),
              AllOf(HasSubstr("docker1|app1"), HasSubstr("docker2|app2"),
                    HasSubstr("docker3|app3"), HasSubstr("docker4|app1")));

  // app1 re-registers. Other apps remain outstanding.
  TestUtils::populate_registration_table(m_db, "docker1|app1", true, false,
                                         false, true);

  EXPECT_EQ(m_init_reg.check_reregistration_status(),
            InitRegistration::Status::IN_PROGRESS);
  EXPECT_EQ(get_reregistration_set_size(), 3);
  EXPECT_THAT(m_init_reg.join_pending_apps(),
              AllOf(HasSubstr("docker2|app2"), HasSubstr("docker3|app3"),
                    HasSubstr("docker4|app1")));

  // Other outstanding apps re-register
  TestUtils::populate_registration_table(m_db, "docker2|app2", true, false,
                                         false, true);
  TestUtils::populate_registration_table(m_db, "docker3|app3", true, false,
                                         false, true);
  TestUtils::populate_registration_table(m_db, "docker4|app1", true, false,
                                         false, true);

  EXPECT_EQ(m_init_reg.check_reregistration_status(),
            InitRegistration::Status::COMPLETED);
  EXPECT_EQ(get_reregistration_set_size(), 0);
  EXPECT_THAT(m_init_reg.join_pending_apps(), StrEq(""));
}

TEST_F(RedisTest, TestHandleRegistrationEvent) {
  populate_default_init_table();

  // No apps have re-registered. All valid apps are still pending.
  m_init_reg.fetch_init_app_info();

  EXPECT_EQ(m_init_reg.check_reregistration_status(),
            InitRegistration::Status::IN_PROGRESS);
  EXPECT_EQ(get_reregistration_set_size(), 4);

  // Trigger re-registration events for apps one-by-one.
  const std::vector<std::string> event_keys(
      {"docker1|app1", "docker2|app2", "docker3|app3", "docker4|app1"});
  for (size_t i = 0; i < event_keys.size(); i++) {
    const swss::KeyOpFieldsValuesTuple event = {
        event_keys[i], "HSET", {{"timestamp", ""}}};

    m_init_reg.handle_registration_event(event);

    if (i < event_keys.size() - 1) {
      EXPECT_EQ(m_init_reg.check_reregistration_status(),
                InitRegistration::Status::IN_PROGRESS);
    } else {
      EXPECT_EQ(m_init_reg.check_reregistration_status(),
                InitRegistration::Status::COMPLETED);
    }
    EXPECT_EQ(get_reregistration_set_size(), event_keys.size() - (i + 1));
  }
}

}  // namespace rebootbackend
