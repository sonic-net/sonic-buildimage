#pragma once
#include <gmock/gmock.h>

#include "init_thread.h"
#include "reboot_interfaces.h"
#include "selectableevent.h"
#include "system/system.pb.h"

namespace rebootbackend {

class MockDbusInterface : public DbusInterface {
 public:
  MOCK_METHOD(DbusInterface::DbusResponse, Reboot, (const std::string &),
              (override));
  MOCK_METHOD(DbusInterface::DbusResponse, RebootStatus, (const std::string &),
              (override));
  MOCK_METHOD(DbusInterface::DbusResponse, StopContainers, (const std::string&),
              (override));
  MOCK_METHOD(DbusInterface::DbusResponse, StopContainerStatus,
              (const std::string&), (override));
};

class MockCriticalStateInterface : public CriticalStateInterface {
 public:
  MOCK_METHOD(bool, is_system_critical, (), (override));
  MOCK_METHOD(void, report_minor_alarm, (const std::string &), (override));
  MOCK_METHOD(void, report_critical_state, (const std::string &), (override));
};

class MockTelemetryInterface : public TelemetryInterface {
 public:
  ~MockTelemetryInterface() override = default;

  MOCK_METHOD(void, record_overall_start, (), (override));
  MOCK_METHOD(void, record_overall_end, (bool success), (override));
  MOCK_METHOD(void, record_stage_start,
              (swss::WarmStart::WarmBootStage nsf_stage), (override));
  MOCK_METHOD(void, record_stage_end,
              (swss::WarmStart::WarmBootStage nsf_stage, bool success),
              (override));
};

class MockInitThread : public InitThread {
 public:
  MockInitThread()
      : InitThread(m_unused_critical_state, m_unused_telemetry, m_unused_event,
                   m_unused_event) {}

  MOCK_METHOD(swss::StatusCode, Start, (), (override));
  MOCK_METHOD(void, Stop, (), (override));
  MOCK_METHOD(bool, Join, (), (override));
  MOCK_METHOD(gnoi::system::RebootStatusResponse, GetResponse, (), (override));
  MOCK_METHOD(InitThreadStatus::DetailedStatus, GetDetailedStatus, (),
              (override));

 private:
  MockCriticalStateInterface m_unused_critical_state;
  MockTelemetryInterface m_unused_telemetry;
  swss::SelectableEvent m_unused_event;
};

}  // namespace rebootbackend