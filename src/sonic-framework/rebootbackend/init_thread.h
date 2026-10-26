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


}  // namespace rebootbackend
