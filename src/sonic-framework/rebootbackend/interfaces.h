#pragma once
#include <dbus-c++/dbus.h>

#include <string>

#include "gnoi_container_shutdown_dbus.h"  // auto generated
                                           // gnoi_container_shutdown_proxy
#include "gnoi_reboot_dbus.h"              // auto generated gnoi_reboot_proxy
#include "reboot_interfaces.h"

class GnoiDbusContainerShutdown
    : public org::SONiC::HostService::gnoi_container_shutdown_proxy,
      public DBus::IntrospectableProxy,
      public DBus::ObjectProxy {
 public:
  GnoiDbusContainerShutdown(DBus::Connection& connection,
                            const char* dbus_bus_name_p,
                            const char* dbus_obj_name_p)
      : DBus::ObjectProxy(connection, dbus_obj_name_p, dbus_bus_name_p) {}
};

class GnoiDbusReboot : public org::SONiC::HostService::gnoi_reboot_proxy,
                       public DBus::IntrospectableProxy,
                       public DBus::ObjectProxy {
 public:
  GnoiDbusReboot(DBus::Connection& connection, const char* dbus_bus_name_p,
                 const char* dbus_obj_name_p)
      : DBus::ObjectProxy(connection, dbus_obj_name_p, dbus_bus_name_p) {}
};

class HostServiceDbus : public DbusInterface {
 public:
  DbusInterface::DbusResponse Reboot(
      const std::string& json_reboot_request) override;
  DbusInterface::DbusResponse RebootStatus(
      const std::string& json_status_request) override;
  DbusInterface::DbusResponse StopContainers(
      const std::string& json_stop_request) override;
  DbusInterface::DbusResponse StopContainerStatus(
      const std::string& json_status_request) override;

 private:
  static DBus::Connection& getConnection(void);
};

class CriticalState : public CriticalStateInterface {
 public:
  bool is_system_critical() override;
  void report_minor_alarm(const std::string& reason) override;
  void report_critical_state(const std::string& reason) override;
};
