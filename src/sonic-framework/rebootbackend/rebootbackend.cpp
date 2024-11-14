#include "interfaces.h"
#include "reboot_interfaces.h"
#include "rebootbe.h"
#include "telemetry_helper.h"

using namespace ::rebootbackend;

int main(int argc, char** argv) {
  HostServiceDbus dbus_interface;
  CriticalState critical_interface;
  TelemetryHelper telemetry_helper;
  RebootBE rebootbe(dbus_interface, critical_interface, telemetry_helper);
  rebootbe.Start();
  return 0;
}
