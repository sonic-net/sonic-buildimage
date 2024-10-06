#pragma once
#include <gmock/gmock.h>

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
};

/*class MockInitThread : public InitThread {
 public:
  MockInitThread()
      : InitThread(m_unused_event,
                   m_unused_event) {}

  MOCK_METHOD(swss::StatusCode, Start, (), (override));
  MOCK_METHOD(void, Stop, (), (override));
  MOCK_METHOD(bool, Join, (), (override));
  MOCK_METHOD(gnoi::system::RebootStatusResponse, GetResponse, (), (override));
  MOCK_METHOD(InitThreadStatus::DetailedStatus, GetDetailedStatus, (),
              (override));

 private:
  swss::SelectableEvent m_unused_event;
};*/

}  // namespace rebootbackend
