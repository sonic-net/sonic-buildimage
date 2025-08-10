#include "interfaces.h"
#include "reboot_interfaces.h"
#include "rebootbe.h"

using namespace ::rebootbackend;

int main(int argc, char** argv) {
  HostServiceDbus dbus_interface;
  RebootBE rebootbe(dbus_interface);
  rebootbe.Start();
  return 0;
}
