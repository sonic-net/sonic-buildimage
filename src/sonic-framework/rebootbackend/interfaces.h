#pragma once
#include <dbus-c++/dbus.h>

#include <string>

#include "reboot_dbus.h"  // auto generated reboot_proxy
#include "reboot_interfaces.h"

class DbusReboot : public org::SONiC::HostService::reboot_proxy,
                       public DBus::IntrospectableProxy,
                       public DBus::ObjectProxy {
 public:
  DbusReboot(DBus::Connection& connection, const char* dbus_bus_name_p,
                 const char* dbus_obj_name_p)
      : DBus::ObjectProxy(connection, dbus_obj_name_p, dbus_bus_name_p) {}
};

class HostServiceDbus : public DbusInterface {
 public:
  DbusInterface::DbusResponse Reboot(
      const std::string& json_reboot_request) override;
  DbusInterface::DbusResponse RebootStatus(
      const std::string& json_status_request) override;

 private:
  static DBus::Connection& getConnection(void);
};
