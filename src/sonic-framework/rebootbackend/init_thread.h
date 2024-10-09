#pragma once

#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

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
#include "warm_restart.h"

namespace rebootbackend {

// Holds a thread safe representation of the InitThread internal state.
// Thread-safe: the expectation is one thread will write and multiple threads
//   will read.
class InitThreadStatus {
 public:
  enum ThreadStatus {
    NOT_STARTED = 0,
    PENDING = 1,
    WAITING_FOR_REGISTRATION = 2,
    WAITING_FOR_RECONCILIATION = 3,
    WAITING_FOR_STATE_VERIFICATION = 4,
    WAITING_FOR_UNFREEZE = 5,
    FINALIZE = 6,
    DONE = 7,
    ERROR = 8,
  };

  enum ErrorCondition {
    NO_ERROR = 0,
    UNKNOWN = 1,
    INTERNAL_ERROR = 2,
    REGISTRATION_FAILED = 3,
    RECONCILIATION_FAILED = 4,
    STATE_VERIFICATION_FAILED = 5,
    UNFREEZE_FAILED = 6,
    DETECTED_CRITICAL_STATE = 7,
  };

  struct DetailedStatus {
    gnoi::system::RebootStatusResponse thread_state;
    InitThreadStatus::ThreadStatus detailed_thread_status =
        InitThreadStatus::ThreadStatus::NOT_STARTED;
    InitThreadStatus::ErrorCondition detailed_thread_error_condition =
        InitThreadStatus::ErrorCondition::NO_ERROR;
  };

  InitThreadStatus() {
    m_status.detailed_thread_status = ThreadStatus::NOT_STARTED;
    m_status.detailed_thread_error_condition = ErrorCondition::NO_ERROR;

    m_status.thread_state.set_active(false);
    m_status.thread_state.set_method(gnoi::system::RebootMethod::COLD);
    m_status.thread_state.mutable_status()->set_status(
        gnoi::system::RebootStatus_Status::RebootStatus_Status_STATUS_SUCCESS);
    m_status.thread_state.mutable_status()->set_message("");
  }

  void set_start_status() {
    const std::lock_guard<std::mutex> lock(m_mutex);
    m_status.detailed_thread_status = ThreadStatus::PENDING;
    m_status.detailed_thread_error_condition = ErrorCondition::NO_ERROR;

    m_status.thread_state.set_active(true);
    m_status.thread_state.set_method(gnoi::system::RebootMethod::NSF);
    m_status.thread_state.mutable_status()->set_status(
        gnoi::system::RebootStatus_Status::RebootStatus_Status_STATUS_UNKNOWN);
    m_status.thread_state.mutable_status()->set_message("");
  }

  bool get_active(void) {
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_status.thread_state.active();
  }

  void set_detailed_thread_status(ThreadStatus new_status) {
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (m_status.thread_state.active()) {
      m_status.detailed_thread_status = new_status;
    }
  }

  void set_success() {
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (m_status.thread_state.active()) {
      m_status.detailed_thread_status = ThreadStatus::DONE;
      m_status.thread_state.mutable_status()->set_status(
          gnoi::system::RebootStatus_Status::
              RebootStatus_Status_STATUS_SUCCESS);
    }
  }

  void set_error(ErrorCondition error_condition,
                 const std::string &error_message) {
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (m_status.thread_state.active()) {
      m_status.detailed_thread_status = ThreadStatus::ERROR;
      m_status.detailed_thread_error_condition = error_condition;
      m_status.thread_state.mutable_status()->set_status(
          gnoi::system::RebootStatus_Status::
              RebootStatus_Status_STATUS_FAILURE);
      m_status.thread_state.mutable_status()->set_message(error_message);
    }
  }

  void set_inactive() {
    const std::lock_guard<std::mutex> lock(m_mutex);
    m_status.thread_state.set_active(false);
  }

  DetailedStatus get_detailed_thread_status() {
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_status;
  }

  gnoi::system::RebootStatusResponse get_response() {
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_status.thread_state;
  }

 private:
  std::mutex m_mutex;
  DetailedStatus m_status;
};

class InitThread {
 public:
  InitThread(CriticalStateInterface &critical_interface,
             TelemetryInterface &telemetry_interface,
             swss::SelectableEvent &m_finished,
             swss::SelectableEvent &m_stack_unfrozen);
  virtual ~InitThread() = default;

  // Starts running the init thread tasks. Returns SWSS_RC_SUCCESS if the
  // internal thread was started successfully, and an error otherwise. If an
  // error is returned, this call may safely be retried, but will likely
  // continue to return errors.
  virtual swss::StatusCode Start();

  // Request InitThread stop/exit. Notifies the internal thread that it must
  // exit. Only used when platform is shutting down all containers/processes.
  virtual void Stop(void);

  // Must be called by launching task after notification is sent to m_finished.
  virtual bool Join(void);

  // Return Status of last reboot attempt.
  virtual gnoi::system::RebootStatusResponse GetResponse();

  // Returns a representation of the detailed thread status.
  virtual InitThreadStatus::DetailedStatus GetDetailedStatus();

 private:
  enum class SelectStatus { SUCCESS, FAILURE, KEEP_WAITING };

  static SelectStatus ToSelectStatus(Registration::Response result);
  static SelectStatus ToSelectStatus(InitRegistration::Status status);
  static swss::StatusCode ToStatusCode(SelectStatus select_status);

  // Internal implementation of Start(). Returns SWSS_RC_SUCCESS if the init
  // thread was started successfully, and an error otherwise. If an error is
  // returned, final cleanup actions must be taken.
  swss::StatusCode internal_start();

  // Function containing the core logic. Invoked as a separate thread. Runs
  // through the steps required for reconciliation monitoring.
  void init_thread(void);

  // Perform the final required actions before exiting:
  // 1. Clear the NSF flag.
  // 2. Record final stats (if able)
  void do_final_failed_actions();

  // Helper function for the registration step. Waits for all applications that
  // had registered warmboot intent before the warmboot to re-register warmboot
  // intent after the warmboot. The provided timer must already have been
  // started prior to this function call.
  // Returns SWSS_RC_SUCCESS if re-registration is successful, and an error
  // otherwise.
  swss::StatusCode handle_registration_step(
      swss::SelectableTimer &timer_select);

  // Helper function for the reconciliation step. Wait for all apps to reach
  // the reconcilied state. The provided timer must already have been started
  // prior to this function call.
  // Returns SWSS_RC_SUCCESS if waiting for reconciliation is successful, and an
  // error otherwise.
  swss::StatusCode handle_reconciliation_step(
      swss::SelectableTimer &timer_select);

  // Helper function for the unfreeze step. Wait for all apps to reach
  // the completed state.
  // Returns SWSS_RC_SUCCESS if waiting for unfreeze is successful, and an
  // error otherwise.
  swss::StatusCode handle_unfreeze_step();

  // Wait until all apps reach a target state, or until the provided timer
  // expires. The timer must already have been started prior to this function
  // call.
  swss::StatusCode wait_for_state(swss::WarmStart::WarmBootStage nsf_stage,
                                  swss::SelectableTimer &timer_select);

  // Helper function for the state verification step. Trigger state verification
  // then wait for all apps to report successful state verification.
  // Returns SWSS_RC_SUCCESS if waiting for state verification is successful,
  // and an error otherwise.
  swss::StatusCode handle_state_verification_step();

  // Helper function for select loops. Sets up events for m_done, timer_select,
  // and table_sub, checks if already done with initial_check, then enters
  // an event handling loop, forwarding events to table_event_callback until
  // the callback indicates operation is complete.
  swss::StatusCode select_loop(
      swss::Selectable &timer_select, swss::SubscriberStateTable &table_sub,
      const std::function<SelectStatus()> &initial_check,
      const std::function<SelectStatus(const swss::KeyOpFieldsValuesTuple &)>
          &table_event_callback);

  // Thread and internal status.
  std::thread m_thread;
  InitThreadStatus m_status;

  // Event handles used to notify the caller when InitThread is finished, when
  // the stack is unfrozen, and pass Stop events through to the dependent
  // thread to stop operation prematurely.
  swss::SelectableEvent &m_finished;
  swss::SelectableEvent &m_stack_unfrozen;
  swss::SelectableEvent m_stop;

  // Interfaces to external systems: the Redis database and critical state
  // system.
  swss::DBConnector m_db;
  CriticalStateInterface &m_critical_interface;
  TelemetryInterface &m_telemetry;

  // Various operation timeouts in seconds: allow unit test to shorten.
  static constexpr uint32_t kReconciliationTimeout = 300;
  uint32_t m_reconciliation_timeout = kReconciliationTimeout;

  // Various operation timeouts in seconds: allow unit test to shorten.
  static constexpr uint32_t kStateVerificationTimeout = 180;
  uint32_t m_state_verification_timeout = kStateVerificationTimeout;

  static constexpr uint32_t kUnfreezeTimeout = 60;
  uint32_t m_unfreeze_timeout = kUnfreezeTimeout;

  friend class InitThreadTest;
};

}  // namespace rebootbackend
