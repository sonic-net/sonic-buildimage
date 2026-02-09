#include "interfaces.h"
#include "reboot_interfaces.h"
#include "rebootbe.h"

int main(int argc, char** argv) {
  HostServiceDbus dbus_interface;
  ::rebootbackend::RebootBE rebootbe(dbus_interface);
  rebootbe.Start();
  return 0;
}
