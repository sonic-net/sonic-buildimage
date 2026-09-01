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
  virtual DbusResponse Reboot(const std::string& json_reboot_request) = 0;
  virtual DbusResponse RebootStatus(const std::string& json_status_request) = 0;
};

