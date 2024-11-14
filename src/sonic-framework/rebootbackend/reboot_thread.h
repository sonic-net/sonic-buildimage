#pragma once

#include <chrono>
#include <mutex>
#include <thread>

#include "dbconnector.h"
#include "notificationproducer.h"
#include "reboot_common.h"
#include "reboot_interfaces.h"
#include "redis_utils.h"
#include "select.h"
#include "selectableevent.h"
#include "selectabletimer.h"
#include "subscriberstatetable.h"
#include "system/system.pb.h"

namespace rebootbackend {

#define SELECT_TIMEOUT_250_MS (250)
#define SELECT_TIMEOUT_500_MS (500)

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

    // RebootMethod is type of of reboot: cold, nsf, warm, fast from a
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
               CriticalStateInterface &critical_interface,
               TelemetryInterface &telemetry_interface,
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
  Progress nsf_reboot_helper(swss::Select &s);
  void do_nsf_reboot(swss::Select &s);
  void do_cold_reboot(swss::Select &s);

  // Inner loop select handler to wait for platform reboot.
  //   wait for timeout
  //   wait for a stop request (sigterm)
  // Returns:
  //   EXIT_EARLY: an issue occurred that stops NSF
  //   PROCEED: if reboot timeout expired
  Progress platform_reboot_select(swss::Select &s,
                                  swss::SelectableTimer &l_timer);

  // Wait for platform to reboot while waiting for possible stop
  // Returns:
  //   EXIT_EARLY: an issue occurred that stops NSF
  //   PROCEED: if reboot timeout expired
  Progress wait_for_platform_reboot(swss::Select &s);

  // Check for critical state: log error and update status.
  // Returns:
  //   true: if system is in critical state
  //   false: all is well
  bool check_and_log_critical_state(const std::string error_string);

  // Log error string, set status to RebootStatus_Status_STATUS_FAILURE
  // Set status message to error_string.
  void log_error_and_set_non_retry_failure(const std::string error_string);

  // Log error string, set status to
  // RebootStatus_Status_STATUS_RETRIABLE_FAILURE Set status message to
  // error_string.
  void log_error_and_set_failure_as_retriable(const std::string error_string);

  // Handle a database subscription update to STATE_VERIFICATION_RESP_TABLE
  //   Confirm update is for the "all" component with correct timestamp.
  //   if update is not_run: then restart
  // Args:
  //   sub: [input] selectable subscription to STATE_VERIFICATION_RESP_TABLE
  //        data is pending
  //   timestamp: [input && output] the timestamp sent to state verification
  //              with the state verification request
  // Returns:
  //   KEEP_WAITING: keep waiting for success, fail or timeout
  //   SUCCESS: state verification passed, procced
  //   FAILURE: state verification failed
  Status handle_state_verification_event(swss::SubscriberStateTable &sub,
                                         std::string &timestamp);

  // Infinite lop select for state verification
  //   Listen for a stop, timer expiration or database update
  // Args:
  //   s: select to be monitored (stop, timer, subscription) events
  //   l_timer: timeout expiration selectable
  //   sub: subscription to STATE_VERIFICATION_RESP_TABLE
  //   timestamp: [input] the timestamp sent with the state verification
  //              request. Used to match response with request.
  // Returns:
  //   EXIT_EARLY: an issue occurred that stops NSF
  //   PROCEED: if reboot timeout expired
  Progress state_verification_select(swss::Select &s,
                                     swss::SelectableTimer &l_timer,
                                     swss::SubscriberStateTable &sub,
                                     std::string &timestamp);

  // If enabled: perform non-frozen state verification
  //   Check for critical state, listen for stop, handle a timeout
  // Returns:
  //   EXIT_EARLY: an issue occurred that stops NSF
  //   PROCEED: if reboot timeout expired
  Progress perform_state_verification(swss::Select &s);

  // Perform freeze/quiescence with container stop support.
  //   Request platform stop (stop on freeze) containers.
  //   Request applications freeze and wait for registered apps
  //   to quiesce or checkpoint.
  //   Poll platform till containers have stopped.
  //   Check for critical state, listen for stop, handle a timeout
  // Returns:
  //   EXIT_EARLY: an issue occurred that stops NSF
  //   PROCEED: if reboot timeout expired
  Progress perform_freeze_w_container_stop(swss::Select &s);

  // Wait for stop_on_freeze containers to exit, then
  //   wait for freeze containers to quiesce (or checkpoint).
  //   Check for critical state, listen for stop, handle a timeout
  // Args:
  //   s: select to be monitored (stop, timer, subscription) events
  //   l_timer: timeout expiration selectable, running at function
  //            start
  //   request_id: [input] request_id populated in StopContainersRequest.
  //               This is used/needed when sending StopContainersStatusRequest.
  // Returns:
  //   EXIT_EARLY: an issue occurred that stops NSF
  //   PROCEED: if reboot timeout expired
  Progress wait_for_container_stop_and_quiescence(
      swss::Select &s, swss::SelectableTimer &l_timer,
      const std::string &request_id);

  // Build a json formatted stop container request proto message.
  // Message populated with containers that registered stop on freeze.
  // Args:
  //   json_request: [output] json formatted StopContainersRequest
  //                 message
  //   request_id: [output] request_id populated in StopContainersRequest.
  //               This is used/needed when sending StopContainersStatusRequest.
  Progress build_stop_container_request(std::string &json_request,
                                        std::string &request_id);

  // Send a StopContainersRequest message to the gnoi_stop_container
  //   sonic host service requesting list of containres be stopped.
  // Args:
  //   request_id: [output] request_id populated in StopContainersRequest.
  //               This is used/needed when sending StopContainersStatusRequest.
  Progress request_stop_on_freeze(std::string &request_id);

  // Send a StopContainersStatusRequest message to the gnoi_stop_container
  //   sonic host service to check if all containers have stopped.
  // Args:
  //   request_id: [input] request_id populated in StopContainersRequest.
  //               This is used/needed when sending StopContainersStatusRequest.
  // Returns:
  //   SUCCESS: containers have stopped, we're done.
  //   KEEP_WAITING: containers haven't stopped
  //   FAILURE: dbus error or protobuf conversion error
  //            suggest retry till timeout
  Status check_container_stop(const std::string &request_id);

  // Check if containers have stopped, or there are no containers to stop
  // Args:
  //   request_id: [input] request_id populated in StopContainersRequest.
  //               This is used/needed when sending StopContainersStatusRequest.
  // Returns:
  //   SUCCESS: containers have stopped, we're done.
  //   KEEP_WAITING: containers haven't stopped
  //   FAILURE: dbus error or protobuf conversion error
  //            suggest retry till timeout
  Status precheck_wait_for_container_stop(const std::string &request_id);

  // Poll the gnoi_stop_container host service to determine if requested
  //   set of containers have exited.
  //   Check for critical state, listen for stop, handle a timeout
  // Args:
  //   s: [input] select statement that has m_stop as a selectable
  //   request_id: [input] request_id populated in StopContainersRequest.
  //               This is used/needed when sending StopContainersStatusRequest.
  //   l_timer: timeout expiration selectable, running at function
  //            start
  Progress wait_for_container_stop(swss::Select &s,
                                   const std::string &request_id,
                                   swss::SelectableTimer &l_timer);

  // Perform freeze/quiescence.
  //   Request applications freeze and wait for registered apps
  //   to quiesce or checkpoint.
  //   Check for critical state, listen for stop, handle a timeout
  // Args:
  //   s: [input] select statement that has m_stop as a selectable
  //   l_timer: [inpu] timeout expiration selectable, running at function start
  // Returns:
  //   EXIT_EARLY: an issue occurred that stops NSF
  //   PROCEED: if reboot timeout expired
  Progress perform_freeze_quiescence(swss::Select &s,
                                     swss::SelectableTimer &l_timer);

  // Helper function for freeze quiescence stage:
  //   Check current database status before waiting for subscriptions updates.
  //   infinite loop select for checkpoint
  //   Listen for a stop, timer expiration or database update
  //   We must be quiescent for 10 seconds before PROCEED.
  // Returns:
  //   EXIT_EARLY: an issue occurred that stops NSF
  //   PROCEED: if reboot timeout expired
  Progress freeze_quiescence_select(swss::Select &s,
                                    swss::SelectableTimer &l_timer,
                                    swss::SubscriberStateTable &sub);

  // Perform checkpointing
  //   Request applications checkpoint and wait for registered apps
  //   to checkpoint.
  //   Check for critical state, listen for stop, handle a timeout
  // Returns:
  //   EXIT_EARLY: an issue occurred that stops NSF
  //   PROCEED: if reboot timeout expired
  Progress perform_checkpoint(swss::Select &s);

  // Hepler function for checkpoint: Check initial checkpoint
  //   status before entering select loop.
  // Returns:
  //   EXIT_EARLY: an issue occurred that stops NSF
  //   PROCEED: if reboot timeout expired
  Progress checkpoint_select_stage_one(swss::Select &s,
                                       swss::SelectableTimer &l_timer,
                                       swss::SubscriberStateTable &sub);

  // Helper function for checkpoint stage:
  //   infinite loop select for checkpoint
  //   Listen for a stop, timer expiration or database update
  // Returns:
  //   EXIT_EARLY: an issue occurred that stops NSF
  //   PROCEED: if reboot timeout expired
  Progress checkpoint_stage_two(swss::Select &s, swss::SelectableTimer &l_timer,
                                swss::SubscriberStateTable &sub);

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
  CriticalStateInterface &m_critical_interface;
  TelemetryInterface &m_telemetry;
  swss::DBConnector m_db;
  ThreadStatus m_status;
  gnoi::system::RebootRequest m_request;
  Registration m_registration;

  // Wait for system to reboot: allow unit test to shorten.
  // TODO: there is a plan to make these timer values
  //       available in CONFIG_DB
  static constexpr uint32_t kRebootTime = 260;
  uint32_t m_reboot_timeout = kRebootTime;

  static constexpr uint32_t kStateVerificationTime = 180;
  uint32_t m_state_verification_timeout = kStateVerificationTime;

  static constexpr uint32_t kQuiescenceTimeMs = 60 * ONE_THOUSAND;
  uint32_t m_quiescence_timeout_ms = kQuiescenceTimeMs;

  // We must remain quiescent for 5 seconds.
  static constexpr uint32_t kQuiescenceHoldTimeMs = 5 * ONE_THOUSAND;
  uint32_t m_quiescence_hold_time_ms = kQuiescenceHoldTimeMs;

  static constexpr uint32_t kCheckpointTime = 30;
  uint32_t m_checkpoint_timeout = kCheckpointTime;

  friend class RebootBETestWithoutStop;
  friend class RebootThreadTest;
};

}  // namespace rebootbackend
