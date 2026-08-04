#include "reboot_thread.h"

#include <google/protobuf/util/json_util.h>

#include <chrono>

#include "dbconnector.h"
#include "logger.h"
#include "notificationproducer.h"
#include "reboot_common.h"
#include "reboot_interfaces.h"
#include "select.h"
#include "selectableevent.h"
#include "selectabletimer.h"
#include "subscriberstatetable.h"
#include "system/system.pb.h"
#include "timestamp.h"

namespace rebootbackend {

using namespace ::gnoi::system;
using steady_clock = std::chrono::steady_clock;
using Progress = ::rebootbackend::RebootThread::Progress;
namespace gpu = ::google::protobuf::util;

bool sigterm_requested = false;

RebootThread::RebootThread(DbusInterface &dbus_interface,
                           swss::SelectableEvent &m_finished)
    : m_db("STATE_DB", 0),
      m_finished(m_finished),
      m_dbus_interface(dbus_interface) {}

void RebootThread::Stop(void) {
  SWSS_LOG_ENTER();
  // Notify reboot thread that stop has been requested.
  m_stop.notify();
}

bool RebootThread::Join(void) {
  SWSS_LOG_ENTER();

  if (!m_thread.joinable()) {
    SWSS_LOG_ERROR("RebootThread::Join called, but not joinable");
    return false;
  }

  try {
    m_thread.join();
    m_status.set_inactive();
    return true;
  } catch (const std::system_error &e) {
    SWSS_LOG_ERROR("Exception calling join: %s", e.what());
    return false;
  }
}

RebootStatusResponse RebootThread::GetResponse(void) {
  return m_status.get_response();
}

bool RebootThread::HasRun() { return m_status.get_reboot_count() > 0; }

Progress RebootThread::platform_reboot_select(swss::Select &s,
                                              swss::SelectableTimer &l_timer) {
  SWSS_LOG_ENTER();

  while (true) {
    swss::Selectable *sel;
    int select_ret;
    select_ret = s.select(&sel);

    if (select_ret == swss::Select::ERROR) {
      SWSS_LOG_NOTICE("Error: %s!", strerror(errno));
    } else if (select_ret == swss::Select::OBJECT) {
      if (sel == &m_stop) {
        // SIGTERM expected after platform reboot request
        SWSS_LOG_NOTICE(
            "m_stop rx'd (SIGTERM) while waiting for platform reboot");
        return Progress::EXIT_EARLY;
      } else if (sel == &l_timer) {
        return Progress::PROCEED;
      }
    }
  }
}

Progress RebootThread::wait_for_platform_reboot(swss::Select &s) {
  SWSS_LOG_ENTER();

  // Sleep for a long time: 260 seconds.
  // During this time platform should kill us as part of reboot.
  swss::SelectableTimer l_timer(
      timespec{.tv_sec = m_reboot_timeout, .tv_nsec = 0});
  s.addSelectable(&l_timer);

  l_timer.start();

  Progress progress = platform_reboot_select(s, l_timer);

  l_timer.stop();
  s.removeSelectable(&l_timer);
  return progress;
}

void RebootThread::do_reboot(void) {
  SWSS_LOG_ENTER();

  swss::Select s;
  s.addSelectable(&m_stop);

  // Check if stop was requested before Selectable was setup
  if (sigterm_requested) {
    SWSS_LOG_ERROR("sigterm_requested was raised, exiting");
    return;
  }

  if (m_request.method() == RebootMethod::COLD) {
    do_cold_reboot(s);
  } else if (m_request.method() == RebootMethod::HALT) {
    do_halt_reboot(s);
  } else if (m_request.method() == RebootMethod::WARM) {
    do_warm_reboot(s);
  } else {
    // This shouldn't be possible. Reference check_start_preconditions()
    SWSS_LOG_ERROR("Received unrecognized method type = %s",
                   RebootMethod_Name(m_request.method()).c_str());
  }
}

RebootThread::Progress RebootThread::send_dbus_reboot_request() {
  SWSS_LOG_ENTER();
  SWSS_LOG_NOTICE("Sending reboot request to platform");

  std::string json_string;
  gpu::Status status = gpu::MessageToJsonString(m_request, &json_string);
  if (!status.ok()) {
    std::string error_string = "unable to convert reboot protobuf to json: " +
                               status.message().as_string();
    log_error_and_set_non_retry_failure(error_string);
    return Progress::EXIT_EARLY;
  }

  // Send the reboot request to the reboot host service via dbus.
  DbusInterface::DbusResponse dbus_response =
      m_dbus_interface.Reboot(json_string);

  if (dbus_response.status == DbusInterface::DbusStatus::DBUS_FAIL) {
    log_error_and_set_non_retry_failure(dbus_response.json_string);
    return Progress::EXIT_EARLY;
  }
  return Progress::PROCEED;
}

void RebootThread::do_cold_reboot(swss::Select &s) {
  SWSS_LOG_ENTER();
  SWSS_LOG_NOTICE("Sending cold reboot request to platform");
  if (send_dbus_reboot_request() == Progress::EXIT_EARLY) {
    return;
  }

  // Wait for platform to reboot. If we return, reboot failed.
  if (wait_for_platform_reboot(s) == Progress::EXIT_EARLY) {
    return;
  }

  // We shouldn't be here. Platform reboot should've killed us.
  log_error_and_set_non_retry_failure("platform failed to reboot");

  // Set critical state
  // m_critical_interface.report_critical_state("platform failed to reboot");
  return;
}

void RebootThread::do_halt_reboot(swss::Select &s) {
  SWSS_LOG_ENTER();
  SWSS_LOG_NOTICE("Sending halt reboot request to platform");
  if (send_dbus_reboot_request() == Progress::EXIT_EARLY) {
    return;
  }

  // Wait for platform to halt. If we return, reboot failed.
  // Logging, error status and monitoring for critical state are handled within.
  if (wait_for_platform_reboot(s) == Progress::EXIT_EARLY) {
    return;
  }

  // We shouldn't be here. Platform reboot halt should've killed us.
  log_error_and_set_non_retry_failure("platform failed to halt the system");

  return;
}

void RebootThread::do_warm_reboot(swss::Select &s) {
  SWSS_LOG_ENTER();
  SWSS_LOG_NOTICE("Sending warm reboot request to platform");
  if (send_dbus_reboot_request() == Progress::EXIT_EARLY) {
    return;
  }

  // Wait for warm reboot. If we return, reboot failed.
  if (wait_for_platform_reboot(s) == Progress::EXIT_EARLY) {
    return;
  }

  // We shouldn't be here. Platform reboot should've killed us.
  log_error_and_set_non_retry_failure("failed to warm reboot");

  return;
}

void RebootThread::reboot_thread(void) {
  SWSS_LOG_ENTER();

  do_reboot();

  // Notify calling thread that reboot thread has exited.
  // Calling thread will call Join(): join and set thread status to inactive.
  m_finished.notify();
}

bool RebootThread::check_start_preconditions(const RebootRequest &request,
                                             NotificationResponse &response) {
  // We have to join a previous executing thread before restarting.
  // Active is cleared in Join.
  if (m_status.get_active()) {
    response.json_string = "RebootThread: can't Start while active";
    response.status = swss::StatusCode::SWSS_RC_IN_USE;
  } else if (request.method() != RebootMethod::COLD &&
             request.method() != RebootMethod::HALT &&
             request.method() != RebootMethod::WARM) {
    response.json_string = "RebootThread: Start rx'd unsupported method";
    response.status = swss::StatusCode::SWSS_RC_INVALID_PARAM;
  } else if (request.method() == RebootMethod::WARM) {
    if (m_status.get_last_reboot_status() ==
        RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE) {
      // If the last reboot failed with a non-retriable failure, don't retry.
      // But, we will allow a cold boot to recover.
      response.json_string =
          "RebootThread: last WARM reboot failed with non-retriable failure";
      response.status = swss::StatusCode::SWSS_RC_FAILED_PRECONDITION;
    }
  } else if (request.delay() != 0) {
    response.json_string = "RebootThread: delayed start not supported";
    response.status = swss::StatusCode::SWSS_RC_INVALID_PARAM;
  }

  if (response.status == swss::StatusCode::SWSS_RC_SUCCESS) {
    return true;
  }

  SWSS_LOG_ERROR("%s", response.json_string.c_str());
  // Log the reboot request contents.
  gpu::Status status;
  std::string json_request;
  status = gpu::MessageToJsonString(request, &json_request);
  if (status.ok()) {
    SWSS_LOG_ERROR("check_start_preconditions: RebootRequest = %s",
                   json_request.c_str());
  } else {
    SWSS_LOG_ERROR(
        "check_start_preconditions: error calling MessageToJsonString");
  }
  return false;
}

NotificationResponse RebootThread::Start(const RebootRequest &request) {
  SWSS_LOG_ENTER();

  NotificationResponse response = {.status = swss::StatusCode::SWSS_RC_SUCCESS,
                                   .json_string = ""};

  // Confirm we're not running, method is supported and we're not delayed.
  if (!check_start_preconditions(request, response)) {
    // Errors logged in check_start_preconditions.
    return response;
  }

  m_request = request;

  // From this point errors will be reported via RebootStatusRequest.
  m_status.set_start_status(request.method(), request.message());

  try {
    m_thread = std::thread(&RebootThread::reboot_thread, this);
  } catch (const std::system_error &e) {
    std::string error_string = "Exception launching reboot thread: ";
    error_string += e.what();
    log_error_and_set_failure_as_retriable(error_string);

    // Notify calling thread that thread has finished.
    // Calling thread MUST call Join, which will join and clear active bit.
    m_finished.notify();
  }
  return response;
}

void RebootThread::log_error_and_set_non_retry_failure(
    const std::string error_string) {
  SWSS_LOG_ENTER();
  SWSS_LOG_ERROR("%s", error_string.c_str());
  m_status.set_completed_status(
      RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE, error_string);
}

void RebootThread::log_error_and_set_failure_as_retriable(
    const std::string error_string) {
  SWSS_LOG_ENTER();
  SWSS_LOG_ERROR("%s", error_string.c_str());
  m_status.set_completed_status(
      RebootStatus_Status::RebootStatus_Status_STATUS_RETRIABLE_FAILURE,
      error_string);
}

}  // namespace rebootbackend
