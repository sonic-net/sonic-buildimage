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
#include "stateverification.h"
#include "table.h"
#include "timestamp.h"
#include "warm_restart.h"

namespace rebootbackend {

void TestUtils::wait_for_finish(swss::Select &s,
                                swss::SelectableEvent &finished,
                                uint32_t timeout_seconds) {
  swss::Selectable *sel;
  int ret;

  ret = s.select(&sel, timeout_seconds * 1000);
  EXPECT_EQ(ret, swss::Select::OBJECT);
  EXPECT_EQ(sel, &finished);
}

std::string TestUtils::wait_for_state_verification_trigger(
    swss::NotificationConsumer &nc, uint32_t timeout_seconds, bool freeze) {
  swss::Select s;
  s.addSelectable(&nc);

  swss::Selectable *sel;
  int ret;
  ret = s.select(&sel, timeout_seconds * 1000);
  EXPECT_EQ(ret, swss::Select::OBJECT);
  if (ret != swss::Select::OBJECT) {
    return "";
  }

  std::string op, timestamp_data;
  std::vector<swss::FieldValueTuple> values;
  nc.pop(op, timestamp_data, values);
  auto fv = values[0];
  EXPECT_EQ(op, ALL_COMPONENT);
  EXPECT_EQ(fvField(fv), FREEZE_FIELD);
  EXPECT_EQ(fvValue(fv), freeze ? "true" : "false");
  return timestamp_data;
}

void TestUtils::confirm_no_state_verification_trigger(
    swss::NotificationConsumer &nc, uint32_t timeout_seconds) {
  swss::Select s;
  s.addSelectable(&nc);

  swss::Selectable *sel;
  int ret;
  ret = s.select(&sel, timeout_seconds * 1000);
  EXPECT_NE(ret, swss::Select::OBJECT);
}

void TestUtils::populate_registration_table(
    swss::DBConnector &db, const std::string &key, const bool &stop_on_freeze,
    const bool &freeze, const bool &checkpoint, const bool &reconciliation) {
  swss::Table registrationTable(&db,
                                STATE_WARM_RESTART_REGISTRATION_TABLE_NAME);

  std::string tableName = key;
  std::vector<swss::FieldValueTuple> values;

  values.push_back(swss::FieldValueTuple("stop_on_freeze",
                                         stop_on_freeze ? "true" : "false"));
  values.push_back(swss::FieldValueTuple("freeze", freeze ? "true" : "false"));
  values.push_back(
      swss::FieldValueTuple("checkpoint", checkpoint ? "true" : "false"));
  values.push_back(swss::FieldValueTuple("reconciliation",
                                         reconciliation ? "true" : "false"));
  values.push_back(swss::FieldValueTuple("timestamp", swss::getTimestamp()));

  registrationTable.set(tableName, values);
}

void TestUtils::populate_restart_table_state(swss::DBConnector &db,
                                             const std::string &app_name,
                                             const std::string &state) {
  swss::Table warmRestartTable(&db, STATE_WARM_RESTART_TABLE_NAME);
  warmRestartTable.hset(app_name, "state", state);
}

void TestUtils::write_state_verification_result(swss::DBConnector &db,
                                                const std::string &key,
                                                const std::string &status,
                                                const std::string &timestamp) {
  swss::Table state_verification_table(&db, STATE_VERIFICATION_RESP_TABLE);
  std::vector<swss::FieldValueTuple> fvs;
  fvs.push_back(swss::FieldValueTuple(TIMESTAMP_FIELD, timestamp));
  fvs.push_back(swss::FieldValueTuple(STATUS_FIELD, status));
  state_verification_table.set(key, fvs);
}

void TestUtils::clear_tables(swss::DBConnector &db) {
  const std::vector<std::string> kTablesToClear = {
      "BOOT_INFO",
      STATE_WARM_RESTART_TABLE_NAME,
      STATE_WARM_RESTART_REGISTRATION_TABLE_NAME,
      STATE_WARM_RESTART_INIT_TABLE_NAME,
      STATE_VERIFICATION_RESP_TABLE,
      STATE_WARM_RESTART_ENABLE_TABLE_NAME,
      STATE_WARM_RESTART_PERF_TABLE_NAME,
      STATE_WARM_RESTART_PERF_HISTORY_TABLE_NAME};

  for (const auto &table_name : kTablesToClear) {
    swss::Table table(&db, table_name);
    std::vector<std::string> keys;
    table.getKeys(keys);
    for (const auto &key : keys) {
      table.del(key);
    }
  }
}

void TestUtils::check_warmboot_enabled(swss::DBConnector &db,
                                       bool expected_state) {
  swss::Table warmRestartTable(&db, STATE_WARM_RESTART_ENABLE_TABLE_NAME);
  std::string actual_state;
  warmRestartTable.hget("system", "enable", actual_state);
  EXPECT_EQ(actual_state, expected_state ? "true" : "false");
}

void TestUtils::set_state_verification_enable(swss::DBConnector &config_db,
                                              bool bootup, bool enabled) {
  swss::Table warmRestartTable(&config_db, CFG_WARM_RESTART_TABLE_NAME);
  warmRestartTable.hset(
      "system",
      bootup ? "state_verification_bootup" : "state_verification_shutdown",
      enabled ? "true" : "false");
}

}  // namespace rebootbackend
