#pragma once

#include <chrono>
#include <mutex>
#include <thread>

#include "dbconnector.h"
#include "notificationproducer.h"
#include "reboot_common.h"
#include "reboot_interfaces.h"
#include "select.h"
#include "selectableevent.h"
#include "selectabletimer.h"
#include "subscriberstatetable.h"
#include "system/system.pb.h"

namespace rebootbackend {

// Hold/manage the contents of a RebootStatusResponse as defined
// in system.proto
// Thread-safe: expectation is one thread will write and multiple
//   threads can read.
class ThreadStatus {
 public:
  ThreadStatus() {
    m_proto_status.set_active(false);

    // Reason for reboot as specified in message from a RebootRequest.
    // This is "message" in RebootRequest.
    m_proto_status.set_reason("");

    // Number of reboots since active.
    m_proto_status.set_count(0);

    // RebootMethod is type of of reboot: cold, halt, warm, fast from a
    // RebootRequest
    m_proto_status.set_method(gnoi::system::RebootMethod::UNKNOWN);

    // Status can be UNKNOWN, SUCCESS, RETRIABLE_FAILURE or FAILURE.
    m_proto_status.mutable_status()->set_status(
        gnoi::system::RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN);

    // In the event of error: message is human readable error explanation.
    m_proto_status.mutable_status()->set_message("");
  }

  void set_start_status(const gnoi::system::RebootMethod &method,
                        const std::string &reason) {
    m_mutex.lock();

    m_proto_status.set_active(true);
    m_proto_status.set_reason(reason);
    m_proto_status.set_count(m_proto_status.count() + 1);
    m_proto_status.set_method(method);
    m_proto_status.mutable_status()->set_status(
        gnoi::system::RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN);
    m_proto_status.mutable_status()->set_message("");

    // set when to time reboot starts
    std::chrono::nanoseconds ns =
        std::chrono::system_clock::now().time_since_epoch();
    m_proto_status.set_when(ns.count());

    m_mutex.unlock();
  }

  bool get_active(void) {
    m_mutex.lock();
    bool ret = m_proto_status.active();
    m_mutex.unlock();
    return ret;
  }

  void set_completed_status(const gnoi::system::RebootStatus_Status &status,
                            const std::string &message) {
    m_mutex.lock();

    // Status should only be updated while reboot is active
    if (m_proto_status.active()) {
      m_proto_status.mutable_status()->set_status(status);
      m_proto_status.mutable_status()->set_message(message);
    }

    m_mutex.unlock();
  }

  void set_inactive(void) {
    m_mutex.lock();
    m_proto_status.set_active(false);
    m_mutex.unlock();
  }

  int get_reboot_count() {
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_proto_status.count();
  }

  gnoi::system::RebootStatus_Status get_last_reboot_status(void) {
    gnoi::system::RebootStatusResponse response = get_response();
    return response.status().status();
  }

  gnoi::system::RebootStatusResponse get_response(void) {
    m_mutex.lock();
    // make a copy
    gnoi::system::RebootStatusResponse lstatus = m_proto_status;
    m_mutex.unlock();

    if (lstatus.active()) {
      // RebootStatus isn't applicable if we're active
      lstatus.mutable_status()->set_status(
          gnoi::system::RebootStatus_Status::
              RebootStatus_Status_STATUS_UNKNOWN);
      lstatus.mutable_status()->set_message("");
    } else {
      // When is only valid while we're active (since delayed
      // start isn't supported). Value is set when reboot begins.
      lstatus.set_when(0);
    }

    return lstatus;
  }

 private:
  std::mutex m_mutex;
  gnoi::system::RebootStatusResponse m_proto_status;
};

// RebootThread performs reboot actions leading up to a platform
// request to reboot.
// thread-compatible: expectation is Stop, Start and Join will be
//   called from the same thread.
class RebootThread {
 public:
  enum class Status { SUCCESS, FAILURE, KEEP_WAITING };
  enum class Progress { PROCEED, EXIT_EARLY };

  // interface: dbus reboot host service access
  // m_finished: let launching task know thread has finished
  RebootThread(DbusInterface &dbus_interface,
               swss::SelectableEvent &m_finished);

  NotificationResponse Start(const gnoi::system::RebootRequest &request);

  // Request thread stop/exit. Only used when platform is shutting down
  // all containers/processes.
  void Stop(void);

  // Called by launching task after notification sent to m_finished.
  bool Join(void);

  // Return Status of last reboot attempt
  gnoi::system::RebootStatusResponse GetResponse();

  // Returns true if the RebootThread has been started since the last reboot,
  // and false otherwise.
  bool HasRun();

 private:
  void reboot_thread(void);
  void do_reboot(void);
  Progress send_dbus_reboot_request();
  void do_cold_reboot(swss::Select &s);
  void do_halt_reboot(swss::Select &s);
  void do_warm_reboot(swss::Select &s);

  // Inner loop select handler to wait for platform reboot.
  //   wait for timeout
  //   wait for a stop request (sigterm)
  // Returns:
  //   EXIT_EARLY: an issue occurred that stops WARM
  //   PROCEED: if reboot timeout expired
  Progress platform_reboot_select(swss::Select &s,
                                  swss::SelectableTimer &l_timer);

  // Wait for platform to reboot while waiting for possible stop
  // Returns:
  //   EXIT_EARLY: an issue occurred that stops WARM
  //   PROCEED: if reboot timeout expired
  Progress wait_for_platform_reboot(swss::Select &s);

  // Log error string, set status to RebootStatus_Status_STATUS_FAILURE
  // Set status message to error_string.
  void log_error_and_set_non_retry_failure(const std::string error_string);

  // Log error string, set status to
  // RebootStatus_Status_STATUS_RETRIABLE_FAILURE Set status message to
  // error_string.
  void log_error_and_set_failure_as_retriable(const std::string error_string);

  // Request is input only.
  // Response is ouput only.
  // Return true if preconditions met, false otherwise.
  bool check_start_preconditions(const gnoi::system::RebootRequest &request,
                                 NotificationResponse &response);
  std::thread m_thread;

  // Signal m_finished to let main thread know weve completed.
  // Main thread should call Join.
  swss::SelectableEvent &m_finished;

  // m_stop signalled by main thread on sigterm: cleanup and exit.
  swss::SelectableEvent m_stop;
  DbusInterface &m_dbus_interface;
  swss::DBConnector m_db;
  ThreadStatus m_status;
  gnoi::system::RebootRequest m_request;

  // Wait for system to reboot: allow unit test to shorten.
  // TODO: there is a plan to make these timer values
  //       available in CONFIG_DB
  static constexpr uint32_t kRebootTime = 260;
  long m_reboot_timeout = kRebootTime;

  friend class RebootBETestWithoutStop;
  friend class RebootThreadTest;
};

}  // namespace rebootbackend
