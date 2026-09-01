#pragma once

#include <string>

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
  virtual DbusResponse Reboot(const std::string& jsonRebootRequest) = 0;
  virtual DbusResponse RebootStatus(const std::string& jsonStatusRequest) = 0;
};
