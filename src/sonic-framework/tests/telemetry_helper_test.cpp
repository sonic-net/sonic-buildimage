#include "telemetry_helper.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <unordered_set>
#include <vector>

#include "init_thread.h"
#include "reboot_interfaces.h"
#include "redis_utils.h"
#include "table.h"
#include "test_utils_common.h"
#include "warm_restart.h"

namespace rebootbackend {

using swss::WarmStart;
using ::testing::_;
using ::testing::Contains;
using ::testing::ExplainMatchResult;
using ::testing::IsEmpty;
using ::testing::StrEq;

// A fake app class that has methods to trigger telemetry writes. Used to
// abstract some test logic out of the tests themselves.
class FakeApp {
 public:
  FakeApp(const std::string &app_name) : m_app_name(app_name) {}

  void record_start(WarmStart::WarmBootStage nsf_stage) const {
    swss::WarmStart::updateAppWarmBootStageStart(nsf_stage, m_app_name);
  }

  // There are two different API calls in warm_restart.cpp to report final
  // status.
  void record_end(WarmStart::WarmBootStage nsf_stage, bool success) const {
    if (success) {
      swss::WarmStart::updateAppWarmBootStageEnd(nsf_stage_to_state(nsf_stage),
                                                 m_app_name);
    } else {
      swss::WarmStart::updateAppWarmBootStageEndOnFailure(nsf_stage,
                                                          m_app_name);
    }
  }

  const std::string &get_name() const { return m_app_name; }

 private:
  static WarmStart::WarmStartState nsf_stage_to_state(
      WarmStart::WarmBootStage nsf_stage) {
    switch (nsf_stage) {
      case WarmStart::WarmBootStage::STAGE_FREEZE: {
        return WarmStart::WarmStartState::QUIESCENT;
      }
      case WarmStart::WarmBootStage::STAGE_CHECKPOINT: {
        return WarmStart::WarmStartState::CHECKPOINTED;
      }
      case WarmStart::WarmBootStage::STAGE_RECONCILIATION: {
        return WarmStart::WarmStartState::RECONCILED;
      }
      case WarmStart::WarmBootStage::STAGE_UNFREEZE: {
        return WarmStart::WarmStartState::COMPLETED;
      }
      default: {
        return WarmStart::WarmStartState::COMPLETED;
      }
    }
  }

  std::string m_app_name;
};

class TelemetryHelperTest : public ::testing::Test {
 public:
  TelemetryHelperTest()
      : m_db("STATE_DB", 0),
        m_separator(swss::TableBase::getTableSeparator(m_db.getDbId())),
        m_telemetry_helper() {
    swss::WarmStart::initialize("fake_app", "fake_docker");
    TestUtils::clear_tables(m_db);

    for (const auto &app : kFakeApps) {
      TestUtils::populate_registration_table(
          m_db, concat_key("docker1", app.get_name()), false, true, true, true);
    }
  }

  // Checks for DB entries in the form:
  // WARM_RESTART_PERFORMANCE_TABLE|system
  // WARM_RESTART_PERFORMANCE_HISTORY|<warmboot_count>|system
  void check_overall_entries(int count, const std::string &expected_status) {
    check_overall_entries(count, expected_status,
                          default_fields_for_status(expected_status));
  }

  void check_overall_entries(
      int count, const std::string &expected_status,
      const std::unordered_set<std::string> &expected_fields) {
    check_table_entries(STATE_WARM_RESTART_PERF_TABLE_NAME, "system",
                        expected_status, expected_fields);
    check_table_entries(STATE_WARM_RESTART_PERF_HISTORY_TABLE_NAME,
                        concat_key(std::to_string(count), "system"),
                        expected_status, expected_fields);
  }

  // Checks for DB entries in the form:
  // WARM_RESTART_PERFORMANCE_TABLE|<step name>
  // WARM_RESTART_PERFORMANCE_TABLE|<step name>|<app>
  // WARM_RESTART_PERFORMANCE_HISTORY|<warmboot_count>|<step name>
  // WARM_RESTART_PERFORMANCE_HISTORY|<warmboot_count>|<step name>|<app>
  void check_stage_start_entries(int count,
                                 WarmStart::WarmBootStage nsf_stage) {
    fully_parameterized_stage_entries(count, nsf_stage, {}, "in-progress", "",
                                      default_fields_for_status("in-progress"));
  }

  void check_stage_end_entries(int count, WarmStart::WarmBootStage nsf_stage,
                               bool success) {
    std::string expected_status = get_end_status_string(success);
    fully_parameterized_stage_entries(
        count, nsf_stage, kFakeApps, expected_status,
        get_app_end_status_string(nsf_stage, success),
        default_fields_for_status(expected_status));
  }

  void fully_parameterized_stage_entries(
      int count, WarmStart::WarmBootStage nsf_stage,
      const std::vector<FakeApp> apps, const std::string &expected_status,
      const std::string &expected_app_status,
      const std::unordered_set<std::string> &expected_fields) {
    std::string stage_name =
        swss::WarmStart::warmBootStageToNameMap()->at(nsf_stage);
    check_table_entries(STATE_WARM_RESTART_PERF_TABLE_NAME, stage_name,
                        expected_status, expected_fields);
    check_table_entries(STATE_WARM_RESTART_PERF_HISTORY_TABLE_NAME,
                        concat_key(std::to_string(count), stage_name),
                        expected_status, expected_fields);

    for (const auto &app : apps) {
      std::string key_to_check = concat_key(stage_name, app.get_name());
      check_table_entries(STATE_WARM_RESTART_PERF_TABLE_NAME, key_to_check,
                          expected_app_status, expected_fields);
      check_table_entries(STATE_WARM_RESTART_PERF_HISTORY_TABLE_NAME,
                          concat_key(std::to_string(count), key_to_check),
                          expected_app_status, expected_fields);
    }
  }

  // A filled out swss::FieldValueTuple with the specified values.
  std::vector<swss::FieldValueTuple> default_fvs(
      const std::string &status, const std::string &start_timestamp,
      const std::string &end_timestamp) {
    std::vector<swss::FieldValueTuple> field_values(
        {{swss::WarmStart::kPerfTableStatusAttr, status},
         {swss::WarmStart::kPerfTableStartTimeAttr, start_timestamp}});
    if (!end_timestamp.empty()) {
      field_values.push_back(
          {swss::WarmStart::kPerfTableFinishTimeAttr, end_timestamp});
    }
    return field_values;
  }

  std::string get_end_status_string(bool success) {
    return success ? "success" : "failure";
  }

  std::string get_app_end_status_string(WarmStart::WarmBootStage nsf_stage,
                                        bool success) {
    if (nsf_stage == WarmStart::WarmBootStage::STAGE_FREEZE) {
      return success ? "quiescent" : "failure";
    }
    return success ? "success" : "failure";
  }

 private:
  // Not to be used directly.
  void check_table_entries(
      const std::string &table_name, const std::string &key,
      const std::string &expected_status,
      const std::unordered_set<std::string> &expected_fields) {
    // Check that we have the key.
    swss::Table table(&m_db, table_name);
    std::vector<std::string> keys;
    table.getKeys(keys);
    EXPECT_THAT(keys, Contains(key))
        << "Table: " << table_name << " did not contain key: " << key;

    // Check that we have the right fields for those keys.
    std::vector<swss::FieldValueTuple> field_values;
    bool result = table.get(key, field_values);
    EXPECT_TRUE(result);

    std::unordered_set<std::string> remaining_expected_fields(expected_fields);
    for (const auto &field_value : field_values) {
      remaining_expected_fields.erase(fvField(field_value));
    }
    std::ostringstream stream;
    std::copy(expected_fields.begin(), expected_fields.end(),
              std::ostream_iterator<std::string>(stream, ", "));
    EXPECT_THAT(remaining_expected_fields, IsEmpty())
        << "Table: " << table_name << " had incorrect fields for key: " << key
        << ". Expected fields: " << stream.str();

    // Check that the value of "status" is correct.
    std::string actual_status;
    result =
        table.hget(key, swss::WarmStart::kPerfTableStatusAttr, actual_status);
    EXPECT_THAT(actual_status, StrEq(expected_status))
        << "Table: " << table_name << " had incorrect status for key: " << key;
  }

  std::string concat_key(const std::string &str1, const std::string &str2) {
    return str1 + m_separator + str2;
  }

  const std::unordered_set<std::string> &default_fields_for_status(
      std::string status) {
    return status == "in-progress" ? kStartFields : kEndFields;
  }

 protected:
  const std::unordered_set<std::string> kStartFields = {
      swss::WarmStart::kPerfTableStatusAttr,
      swss::WarmStart::kPerfTableStartTimeAttr};
  const std::unordered_set<std::string> kEndFields = {
      swss::WarmStart::kPerfTableStatusAttr,
      swss::WarmStart::kPerfTableStartTimeAttr,
      swss::WarmStart::kPerfTableFinishTimeAttr};
  const std::unordered_set<std::string> kEndWithoutStartFields = {
      swss::WarmStart::kPerfTableStatusAttr,
      swss::WarmStart::kPerfTableFinishTimeAttr};
  const std::vector<WarmStart::WarmBootStage> kStagesInOrder = {
      WarmStart::WarmBootStage::STAGE_FREEZE,
      WarmStart::WarmBootStage::STAGE_CHECKPOINT,
      WarmStart::WarmBootStage::STAGE_RECONCILIATION,
      WarmStart::WarmBootStage::STAGE_UNFREEZE};
  const std::vector<FakeApp> kFakeApps = {FakeApp("app1"), FakeApp("app2"),
                                          FakeApp("app3")};

  swss::DBConnector m_db;
  std::string m_separator;
  TelemetryHelper m_telemetry_helper;
};

class TelemetryHelperWithResultTest
    : public TelemetryHelperTest,
      public ::testing::WithParamInterface<bool> {};

TEST_F(TelemetryHelperTest, OverallStartInitializesCounter) {
  for (int i = 1; i < 6; i++) {
    m_telemetry_helper.record_overall_start();

    EXPECT_THAT(get_warm_restart_counter(m_db), StrEq(std::to_string(i)));
    check_overall_entries(i, "in-progress");
  }
}

TEST_F(TelemetryHelperTest, OverallStartIgnoresFakeCounter) {
  swss::Table table(&m_db, "BOOT_INFO");
  table.hset("system", "warmboot-count", "fake_counter");

  m_telemetry_helper.record_overall_start();

  EXPECT_THAT(get_warm_restart_counter(m_db), StrEq("1"));
  check_overall_entries(1, "in-progress");
}

TEST_F(TelemetryHelperTest, OverallStartClearsPerf) {
  swss::Table table(&m_db, STATE_WARM_RESTART_PERF_TABLE_NAME);
  table.set("system",
            default_fvs("fake_status", "fake_timestamp", "fake_timestamp2"));
  table.set("freeze|fake_app",
            default_fvs("fake_status", "fake_timestamp", "fake_timestamp2"));

  m_telemetry_helper.record_overall_start();

  EXPECT_THAT(get_warm_restart_counter(m_db), StrEq("1"));
  check_overall_entries(1, "in-progress");

  std::vector<swss::FieldValueTuple> unused;
  EXPECT_FALSE(table.get("freeze|fake_app", unused));
}

TEST_P(TelemetryHelperWithResultTest, OverallEndWithoutStart) {
  m_telemetry_helper.record_overall_end(GetParam());

  check_overall_entries(0, get_end_status_string(GetParam()),
                        kEndWithoutStartFields);
}

TEST_P(TelemetryHelperWithResultTest, OverallEndWorks) {
  m_telemetry_helper.record_overall_start();

  m_telemetry_helper.record_overall_end(GetParam());

  check_overall_entries(1, get_end_status_string(GetParam()));
}

TEST_F(TelemetryHelperTest, StageStartWithoutStart) {
  for (const auto &nsf_stage : kStagesInOrder) {
    m_telemetry_helper.record_stage_start(nsf_stage);
    for (const auto &app : kFakeApps) {
      app.record_start(nsf_stage);
    }
    check_stage_start_entries(0, nsf_stage);
  }
}

TEST_F(TelemetryHelperTest, StageStartWorks) {
  m_telemetry_helper.record_overall_start();

  for (const auto &nsf_stage : kStagesInOrder) {
    m_telemetry_helper.record_stage_start(nsf_stage);
    for (const auto &app : kFakeApps) {
      app.record_start(nsf_stage);
    }
    check_stage_start_entries(1, nsf_stage);
  }
}

TEST_P(TelemetryHelperWithResultTest, StageEndWithoutStart) {
  for (const auto &nsf_stage : kStagesInOrder) {
    m_telemetry_helper.record_stage_start(nsf_stage);
    for (const auto &app : kFakeApps) {
      app.record_start(nsf_stage);
      app.record_end(nsf_stage, GetParam());
    }
    m_telemetry_helper.record_stage_end(nsf_stage, GetParam());
    check_stage_end_entries(0, nsf_stage, GetParam());
  }
}

TEST_P(TelemetryHelperWithResultTest, StageEndWithoutStageStart) {
  m_telemetry_helper.record_overall_start();

  std::string expected_status = get_end_status_string(GetParam());

  for (const auto &nsf_stage : kStagesInOrder) {
    for (const auto &app : kFakeApps) {
      app.record_start(nsf_stage);
      app.record_end(nsf_stage, GetParam());
    }
    m_telemetry_helper.record_stage_end(nsf_stage, GetParam());

    std::string expected_app_status =
        get_app_end_status_string(nsf_stage, GetParam());
    fully_parameterized_stage_entries(1, nsf_stage, kFakeApps, expected_status,
                                      expected_app_status,
                                      kEndWithoutStartFields);
  }
}

TEST_P(TelemetryHelperWithResultTest, StageEndWorks) {
  m_telemetry_helper.record_overall_start();

  for (const auto &nsf_stage : kStagesInOrder) {
    m_telemetry_helper.record_stage_start(nsf_stage);
    for (const auto &app : kFakeApps) {
      app.record_start(nsf_stage);
      app.record_end(nsf_stage, GetParam());
    }
    m_telemetry_helper.record_stage_end(nsf_stage, GetParam());
    check_stage_end_entries(1, nsf_stage, GetParam());
  }
}

TEST_P(TelemetryHelperWithResultTest, EndToEndWorks) {
  m_telemetry_helper.record_overall_start();
  check_overall_entries(1, "in-progress");

  for (const auto &nsf_stage : kStagesInOrder) {
    m_telemetry_helper.record_stage_start(nsf_stage);
    for (const auto &app : kFakeApps) {
      app.record_start(nsf_stage);
    }
    check_stage_start_entries(1, nsf_stage);

    for (const auto &app : kFakeApps) {
      app.record_end(nsf_stage, GetParam());
    }
    m_telemetry_helper.record_stage_end(nsf_stage, GetParam());
    check_stage_end_entries(1, nsf_stage, GetParam());
  }

  m_telemetry_helper.record_overall_end(GetParam());
  check_overall_entries(1, get_end_status_string(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(TestWithResultSuite, TelemetryHelperWithResultTest,
                         testing::Values(true, false));

}  // namespace rebootbackend
