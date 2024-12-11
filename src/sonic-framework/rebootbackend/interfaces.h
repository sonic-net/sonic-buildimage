#pragma once
#include <dbus-c++/dbus.h>

#include <string>

#include "gnoi_reboot_dbus.h"              // auto generated gnoi_reboot_proxy
#include "reboot_interfaces.h"

/* Reboot is a request to the reboot sonic host service to request a reboot 
from the platform. This takes as an argument a string based json formatted 
Reboot request from system.proto.’https://github.com/openconfig/gnoi/blob/73a1e7675c5f963e7810bd3828203f2758eb47e8/system/system.proto#L107 */

class GnoiDbusReboot : public org::SONiC::HostService::gnoi_reboot_proxy,
                       public DBus::IntrospectableProxy,
                       public DBus::ObjectProxy {
 public:
  GnoiDbusReboot(DBus::Connection& connection, const char* dbus_bus_name_p,
                 const char* dbus_obj_name_p)
      : DBus::ObjectProxy(connection, dbus_obj_name_p, dbus_bus_name_p) {}
};

/* DbusResponse consists of STATUS: success/fail: i.e. was the dbus request 
successful DbusResponse.json_string: string based json formatted  RebootResponse
defined here:
https://github.com/openconfig/gnoi/blob/73a1e7675c5f963e7810bd3828203f2758eb47e8/system/system.proto#L119 */


class HostServiceDbus : public DbusInterface {
 public:
  DbusInterface::DbusResponse Reboot(
      const std::string& json_reboot_request) override;
  DbusInterface::DbusResponse RebootStatus(
      const std::string& json_status_request) override;

 private:
  static DBus::Connection& getConnection(void);
};
