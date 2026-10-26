#include "interfaces.h"

#include <dbus-c++/dbus.h>  // DBus

//#include "component_state_helper.h"
#include "reboot_interfaces.h"

constexpr char kRebootBusName[] = "org.SONiC.HostService.gnoi_reboot";
constexpr char kRebootPath[] = "/org/SONiC/HostService/gnoi_reboot";

constexpr char kContainerShutdownBusName[] = "org.SONiC.HostService.gnoi_container_shutdown";
constexpr char kContainerShutdownPath[] = "/org/SONiC/HostService/gnoi_container_shutdown";

// DBus::BusDispatcher dispatcher;
DBus::Connection& HostServiceDbus::getConnection(void) {
  static DBus::Connection* connPtr = nullptr;
  if (connPtr == nullptr) {
    static DBus::BusDispatcher dispatcher;
    DBus::default_dispatcher = &dispatcher;

    static DBus::Connection conn = DBus::Connection::SystemBus();
    connPtr = &conn;
  }
  return *connPtr;
}

DbusInterface::DbusResponse HostServiceDbus::Reboot(
    const std::string& json_reboot_request) {
  int32_t status;
  std::string ret_string;
  std::vector<std::string> options;
  options.push_back(json_reboot_request);

  GnoiDbusReboot reboot_client(getConnection(), kRebootBusName, kRebootPath);
  try {
    reboot_client.issue_reboot(options, status, ret_string);
  } catch (DBus::Error& ex) {
    return DbusResponse{
        DbusStatus::DBUS_FAIL,
        "HostServiceDbus::Reboot: failed to call reboot host service"};
  }

  // gnoi_reboot.py returns 0 for success, 1 for failure
  if (status == 0) {
    // Successful reboot response is an empty string.
    return DbusResponse{DbusStatus::DBUS_SUCCESS, ""};
  }
  return DbusResponse{DbusStatus::DBUS_FAIL, ret_string};
}

DbusInterface::DbusResponse HostServiceDbus::RebootStatus(
    const std::string& json_status_request) {
  int32_t status;
  std::string ret_string;

  GnoiDbusReboot reboot_client(getConnection(), kRebootBusName, kRebootPath);
  try {
    reboot_client.get_reboot_status(status, ret_string);
  } catch (DBus::Error& ex) {
    return DbusResponse{
        DbusStatus::DBUS_FAIL,
        "HostServiceDbus::RebootStatus: failed to call reboot status "
        "host service"};
  }

  // gnoi_reboot.py returns 0 for success, 1 for failure
  if (status == 0) {
    return DbusResponse{DbusStatus::DBUS_SUCCESS, ret_string};
  }
  return DbusResponse{DbusStatus::DBUS_FAIL, ret_string};
}

