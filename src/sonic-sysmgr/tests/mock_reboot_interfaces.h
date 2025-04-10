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

}  // namespace rebootbackend
