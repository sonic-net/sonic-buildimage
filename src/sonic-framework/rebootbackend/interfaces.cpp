#include "interfaces.h"

#include <dbus-c++/dbus.h>  // DBus

#include "reboot_interfaces.h"

constexpr char kRebootBusName[] = "org.SONiC.HostService.reboot";
constexpr char kRebootPath[] = "/org/SONiC/HostService/reboot";

constexpr char kContainerShutdownBusName[] =
    "org.SONiC.HostService.container_shutdown";
constexpr char kContainerShutdownPath[] =
    "/org/SONiC/HostService/container_shutdown";

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
    const std::string& jsonRebootRequest) {
  int32_t status;

  DbusReboot reboot_client(getConnection(), kRebootBusName, kRebootPath);
  std::string retString;
  std::vector<std::string> options;
  options.push_back(jsonRebootRequest);
  try {
    reboot_client.issue_reboot(options, status, retString);
  } catch (DBus::Error& ex) {
    return DbusResponse{
        DbusStatus::DBUS_FAIL,
        "HostServiceDbus::Reboot: failed to call reboot host service"};
  }

  // reboot.py returns 0 for success, 1 for failure
  if (status == 0) {
    // Successful reboot response is an empty string.
    return DbusResponse{DbusStatus::DBUS_SUCCESS, ""};
  }
  return DbusResponse{DbusStatus::DBUS_FAIL, retString};
}

DbusInterface::DbusResponse HostServiceDbus::RebootStatus(
    const std::string& jsonStatusRequest) {
  DbusReboot reboot_client(getConnection(), kRebootBusName, kRebootPath);
  int32_t status;
  std::string retString;

  try {
    reboot_client.get_reboot_status(status, retString);
  } catch (DBus::Error& ex) {
    return DbusResponse{
        DbusStatus::DBUS_FAIL,
        "HostServiceDbus::RebootStatus: failed to call reboot status "
        "host service"};
  }

  // reboot.py returns 0 for success, 1 for failure
  if (status == 0) {
    return DbusResponse{DbusStatus::DBUS_SUCCESS, retString};
  }
  return DbusResponse{DbusStatus::DBUS_FAIL, retString};
}
