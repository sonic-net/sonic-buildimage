#pragma once

#include "dbconnector.h"
#include "reboot_interfaces.h"
#include "warm_restart.h"

namespace rebootbackend {

class TelemetryHelper : public TelemetryInterface {
 public:
  TelemetryHelper();
  ~TelemetryHelper() override = default;

  void record_overall_start() override;
  void record_overall_end(bool success) override;

  void record_stage_start(swss::WarmStart::WarmBootStage nsf_stage) override;
  void record_stage_end(swss::WarmStart::WarmBootStage nsf_stage,
                        bool success) override;

 private:
  // Initialize the warmboot count in Redis for a new reboot. Sets the count to
  // 0 if it is not populated, and increment the value otherwise.
  void initialize_warmboot_count();

  // Backup the performance entries for the overall NSF reboot from
  // WARM_RESTART_PERFORMANCE_TABLE to WARM_RESTART_PERFORMANCE_HISTORY_TABLE.
  void backup_overall_values(int count);

  // Backup the performance entries for a particular stage of the NSF reboot
  // from WARM_RESTART_PERFORMANCE_TABLE to
  // WARM_RESTART_PERFORMANCE_HISTORY_TABLE.
  void backup_stage_values(swss::WarmStart::WarmBootStage nsf_stage);

  // Fetch the reboot warmboot count value, or return the cached
  // value for the appropriate value if it has already been fetched.
  int get_reboot_count();

  // Clears all keys from the performance table in preparation for a new reboot.
  void clear_performance_table();

  swss::DBConnector m_db;
  std::string m_separator;

  bool m_fetched_count = false;
  int m_reboot_count = 0;
};

}  // namespace rebootbackend