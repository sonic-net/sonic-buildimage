// Test-only service: record requests and return configured results. Never reboot.
#include <dbus-c++/dbus.h>

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "reboot_dbus_mock.h"  // generated from the owner's reboot.xml --adaptor

namespace {
constexpr char kName[] = "org.SONiC.HostService.reboot";
constexpr char kPath[] = "/org/SONiC/HostService/reboot";

struct Controls {
  int32_t issue_status;
  int32_t query_status;
  std::string issue_message;
  std::string query_message;
};

class RebootMock : public org::SONiC::HostService::reboot_adaptor,
                   public DBus::IntrospectableAdaptor,
                   public DBus::ObjectAdaptor {
 public:
  RebootMock(DBus::Connection& bus, std::string control, std::string events)
      : DBus::ObjectAdaptor(bus, kPath),
        control_(std::move(control)), events_(std::move(events)) {}

  void issue_reboot(const std::vector<std::string>& options,
                    int32_t& status, std::string& message) override {
    if (options.size() != 1) {
      throw DBus::Error("org.SONiC.Test.InvalidRequest", "expected one JSON option");
    }
    record("issue_reboot", options.front());
    const auto c = controls();
    status = c.issue_status;
    message = c.issue_message;
  }

  void get_reboot_status(int32_t& status, std::string& message) override {
    record("get_reboot_status", "");
    const auto c = controls();
    status = c.query_status;
    message = c.query_message;
  }

 private:
  Controls controls() const {
    std::ifstream input(control_);
    std::string issue, query;
    Controls c;
    if (!std::getline(input, issue) || !std::getline(input, query) ||
        !std::getline(input, c.issue_message) ||
        !std::getline(input, c.query_message)) {
      throw DBus::Error("org.SONiC.Test.BadControls", "cannot read fixture controls");
    }
    c.issue_status = std::stoi(issue);
    c.query_status = std::stoi(query);
    return c;
  }

  void record(const std::string& method, const std::string& payload) const {
    // Length framing preserves arbitrary JSON bytes without another JSON library.
    std::ofstream output(events_, std::ios::app | std::ios::binary);
    output << method << ' ' << payload.size() << '\n';
    output.write(payload.data(), payload.size());
    output << '\n';
    output.flush();
    if (!output) throw DBus::Error("org.SONiC.Test.WriteFailed", "event write failed");
  }

  std::string control_;
  std::string events_;
};
}  // namespace

int main(int argc, char** argv) {
  if (argc != 4) return 2;  // control file, event file, readiness file
  try {
    DBus::BusDispatcher dispatcher;
    DBus::default_dispatcher = &dispatcher;
    DBus::Connection bus = DBus::Connection::SystemBus();
    bus.request_name(kName);
    RebootMock service(bus, argv[1], argv[2]);
    std::ofstream ready(argv[3]);
    ready << "private-reboot-service-ready\n";
    ready.close();
    if (!ready) return 3;
    dispatcher.enter();
  } catch (const DBus::Error&) {
    return 4;
  }
  return 0;
}
