#pragma once

#include <string>

#include "warm_restart.h"

class DbusInterface {
 public:
  enum class DbusStatus {
    DBUS_SUCCESS,
    DBUS_FAIL,
  };

  struct DbusResponse {
    DbusStatus status;
    std::string json_string;
  };

  virtual ~DbusInterface() = default;
  virtual DbusResponse Reboot(const std::string& json_reboot_request) = 0;
  virtual DbusResponse RebootStatus(const std::string& json_status_request) = 0;
  virtual DbusResponse StopContainers(const std::string& json_stop_request) = 0;
  virtual DbusResponse StopContainerStatus(
      const std::string& json_status_request) = 0;
};

class CriticalStateInterface {
 public:
  virtual ~CriticalStateInterface() = default;
  virtual bool is_system_critical() = 0;
  virtual void report_minor_alarm(const std::string& reason) = 0;
  virtual void report_critical_state(const std::string& reason) = 0;
};

namespace rebootbackend {

// Class to help interfacing with the telemetry tables in Redis. Not thread
// safe.
class TelemetryInterface {
 public:
  virtual ~TelemetryInterface() = default;

  // Records the warmboot start time. Also increments the warmboot counter.
  // Writes entries:
  // WARM_RESTART_PERFORMANCE_TABLE|system, fields: status, start-timestamp
  // WARM_RESTART_PERFORMANCE_HISTORY|<warmboot_count>|system
  //   fields: status, start-timestamp
  // to the state DB.
  // Must be called before snapshot_stage_start or the warmboot counter may be
  // corrupted.
  virtual void record_overall_start() = 0;

  // Records the warmboot end time, when all operations in the NSF boot have
  // been completed. Writes to both the performance and history tables.
  // Writes entries:
  // WARM_RESTART_PERFORMANCE_TABLE|system, fields: status, finish-timestamp
  // WARM_RESTART_PERFORMANCE_HISTORY|<warmboot_count>|system
  // fields: status, finish-timestamp
  // to the state DB.
  virtual void record_overall_end(bool success) = 0;

  // Records the start time of a particular warmboot stage.
  // Writes entries:
  // WARM_RESTART_PERFORMANCE_TABLE|<step name>, fields: status, start-timestamp
  // WARM_RESTART_PERFORMANCE_TABLE|<step name>|<app>
  //   fields: status, start-timestamp
  // WARM_RESTART_PERFORMANCE_HISTORY|<warmboot_count>|<step name>,
  //   fields: status, start-timestamp
  // WARM_RESTART_PERFORMANCE_HISTORY|<warmboot_count>|<step name>|<app>
  //   fields: status, start-timestamp
  // to the state DB.
  virtual void record_stage_start(swss::WarmStart::WarmBootStage nsf_stage) = 0;
  // Records the end time of a particular warmboot stage.
  // Writes entries:
  // WARM_RESTART_PERFORMANCE_TABLE|<step name>
  //   fields: status, finish-timestamp
  // WARM_RESTART_PERFORMANCE_TABLE|<step name>|<app>
  //   fields: status, finish-timestamp
  // WARM_RESTART_PERFORMANCE_HISTORY|<warmboot_count>|<step name>,
  //   fields: status, finish-timestamp
  // WARM_RESTART_PERFORMANCE_HISTORY|<warmboot_count>|<step name>|<app>
  //   fields: status, finish-timestamp
  // to the state DB.
  virtual void record_stage_end(swss::WarmStart::WarmBootStage nsf_stage,
                                bool success) = 0;
};

}  // namespace rebootbackend