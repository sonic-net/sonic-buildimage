#include "telemetry_helper.h"

#include <stdexcept>
#include <string>
#include <vector>

#include "dbconnector.h"
#include "reboot_interfaces.h"
#include "redis_utils.h"
#include "table.h"
#include "warm_restart.h"

namespace rebootbackend {

TelemetryHelper::TelemetryHelper()
    : m_db("STATE_DB", 0),
      m_separator(swss::TableBase::getTableSeparator(m_db.getDbId())) {}

void TelemetryHelper::record_overall_start() {
  clear_performance_table();

  swss::WarmStart::updateSystemWarmBootStart();

  initialize_warmboot_count();
  backup_overall_values(m_reboot_count);
}

void TelemetryHelper::record_overall_end(bool success) {
  swss::WarmStart::updateSystemWarmBootEnd(success ? "success" : "failure");

  backup_overall_values(get_reboot_count());
}

void TelemetryHelper::record_stage_start(
    swss::WarmStart::WarmBootStage nsf_stage) {
  swss::WarmStart::updateWarmBootStageStart(nsf_stage);

  backup_stage_values(nsf_stage);
}

void TelemetryHelper::record_stage_end(swss::WarmStart::WarmBootStage nsf_stage,
                                       bool success) {
  swss::WarmStart::updateWarmBootStageEnd(nsf_stage,
                                          success ? "success" : "failure");

  backup_stage_values(nsf_stage);
}

// Called at start of an nsf reboot: at least one nsf reboot (the one in
// progress) has occurred
void TelemetryHelper::initialize_warmboot_count() {
  std::string warmboot_counter_str = get_warm_restart_counter(m_db);
  if (warmboot_counter_str.empty()) {
    m_reboot_count = 1;
  }
  try {
    m_reboot_count = std::stoi(warmboot_counter_str) + 1;
  } catch (const std::logic_error &e) {
    m_reboot_count = 1;
  }
  set_warm_restart_counter(m_db, m_reboot_count);
  m_fetched_count = true;
}

void TelemetryHelper::backup_overall_values(int count) {
  swss::Table perf_table(&m_db, STATE_WARM_RESTART_PERF_TABLE_NAME);
  std::vector<swss::FieldValueTuple> values;
  if (!perf_table.get("system", values)) {
    return;
  }

  swss::Table hist_table(&m_db, STATE_WARM_RESTART_PERF_HISTORY_TABLE_NAME);
  hist_table.set(std::to_string(count) + m_separator + "system", values);
}

void TelemetryHelper::backup_stage_values(
    swss::WarmStart::WarmBootStage nsf_stage) {
  std::string stage_name =
      swss::WarmStart::warmBootStageToNameMap()->at(nsf_stage);
  int count = get_reboot_count();

  swss::Table perf_table(&m_db, STATE_WARM_RESTART_PERF_TABLE_NAME);
  swss::Table hist_table(&m_db, STATE_WARM_RESTART_PERF_HISTORY_TABLE_NAME);

  std::vector<std::string> perf_keys;
  perf_table.getKeys(perf_keys);
  for (const auto &key : perf_keys) {
    if (key.rfind(stage_name, 0) == 0) {
      std::vector<swss::FieldValueTuple> values;
      if (!perf_table.get(key, values)) {
        continue;
      }

      hist_table.set(std::to_string(count) + m_separator + key, values);
    }
  }
}

int TelemetryHelper::get_reboot_count() {
  if (!m_fetched_count) {
    try {
      m_reboot_count = std::stoi(get_warm_restart_counter(m_db));
    } catch (const std::logic_error &e) {
      m_reboot_count = 0;
    }
    m_fetched_count = true;
  }
  return m_reboot_count;
}

void TelemetryHelper::clear_performance_table() {
  swss::Table perf_table(&m_db, STATE_WARM_RESTART_PERF_TABLE_NAME);
  std::vector<std::string> perf_keys;
  perf_table.getKeys(perf_keys);
  for (const auto &key : perf_keys) {
    perf_table.del(key);
  }
}

}  // namespace rebootbackend
