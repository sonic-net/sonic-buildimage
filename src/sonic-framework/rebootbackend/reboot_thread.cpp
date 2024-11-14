#include "reboot_thread.h"

#include <google/protobuf/util/json_util.h>

#include <chrono>

#include "component_state_helper.h"
#include "container_stop.pb.h"
#include "dbconnector.h"
#include "logger.h"
#include "notificationproducer.h"
#include "reboot_common.h"
#include "reboot_interfaces.h"
#include "redis_utils.h"
#include "select.h"
#include "selectableevent.h"
#include "selectabletimer.h"
#include "stateverification.h"
#include "subscriberstatetable.h"
#include "system/system.pb.h"
#include "timestamp.h"
#include "warm_restart.h"

namespace rebootbackend {

using namespace ::gnoi::system;
using steady_clock = std::chrono::steady_clock;
using Progress = ::rebootbackend::RebootThread::Progress;
using WarmBootStage = ::swss::WarmStart::WarmBootStage;
using WarmStartState = ::swss::WarmStart::WarmStartState;
namespace gpu = ::google::protobuf::util;

RebootThread::RebootThread(DbusInterface &dbus_interface,
                           CriticalStateInterface &critical_interface,
                           TelemetryInterface &telemetry_interface,
                           swss::SelectableEvent &m_finished)
    : m_db("STATE_DB", 0),
      m_finished(m_finished),
      m_dbus_interface(dbus_interface),
      m_critical_interface(critical_interface),
      m_telemetry(telemetry_interface),
      m_registration() {}

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

  if (check_and_log_critical_state(
          "system entered critical state after platfrom reboot request")) {
    return Progress::EXIT_EARLY;
  }

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
  } else if (m_request.method() == RebootMethod::NSF) {
    do_nsf_reboot(s);
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

RebootThread::Progress RebootThread::nsf_reboot_helper(swss::Select &s) {
  SWSS_LOG_ENTER();

  SWSS_LOG_NOTICE("starting state verification: if enabled");
  if (Progress::EXIT_EARLY == perform_state_verification(s)) {
    SWSS_LOG_ERROR("state verification returned EXIT_EARLY");
    return Progress::EXIT_EARLY;
  }

  SWSS_LOG_NOTICE("starting freeze and container stop");
  m_telemetry.record_stage_start(WarmBootStage::STAGE_FREEZE);
  if (Progress::EXIT_EARLY == perform_freeze_w_container_stop(s)) {
    SWSS_LOG_ERROR("perform_freeze_w_container_stop: returned EXIT_EARLY");
    m_telemetry.record_stage_end(WarmBootStage::STAGE_FREEZE,
                                 /*success=*/false);
    return Progress::EXIT_EARLY;
  }
  m_telemetry.record_stage_end(WarmBootStage::STAGE_FREEZE, /*success=*/true);

  SWSS_LOG_NOTICE("starting checkpoint");
  m_telemetry.record_stage_start(WarmBootStage::STAGE_CHECKPOINT);
  if (Progress::EXIT_EARLY == perform_checkpoint(s)) {
    SWSS_LOG_ERROR("perform_checkpoint: returned EXIT_EARLY");
    m_telemetry.record_stage_end(WarmBootStage::STAGE_CHECKPOINT,
                                 /*success=*/false);
    return Progress::EXIT_EARLY;
  }
  m_telemetry.record_stage_end(WarmBootStage::STAGE_CHECKPOINT,
                               /*success=*/true);

  SWSS_LOG_NOTICE(
      "done all pre-reboot steps, sending reboot request to platform");
  if (send_dbus_reboot_request() == Progress::EXIT_EARLY) {
    return Progress::EXIT_EARLY;
  }

  // Wait for platform to reboot. If we return, reboot failed.
  // Logging, error status and monitoring for critical state are handled within.
  return wait_for_platform_reboot(s);
}

void RebootThread::do_nsf_reboot(swss::Select &s) {
  SWSS_LOG_ENTER();
  SWSS_LOG_NOTICE("Starting NSF reboot");

  // Delete the warm restart state and timestamp for all application
  init_warm_reboot_states(m_db);

  m_registration.fetch_registration_info();

  // Save the list of registered apps.
  m_registration.clear_all_init_apps();
  m_registration.save_all_init_apps();

  m_telemetry.record_overall_start();

  // Enable system warm restart: WARM_RESTART_ENABLE_TABLE|system
  set_warm_restart_enable(m_db, true);

  RebootThread::Progress progress = nsf_reboot_helper(s);
  if (progress == Progress::PROCEED) {
    // We shouldn't be here. No errors (EXIT_EARLY) occurred during
    // reboot process under our control. Platform reboot should've killed us.
    log_error_and_set_non_retry_failure("platform failed to reboot");

    // Set critical state
    m_critical_interface.report_critical_state("platform failed to reboot");
  }

  // NSF has failed. Either an error (EXIT_EARLY from nsf_reboot_helper)
  // or platform failed to kill us after waiting m_reboot_timeout.
  // Clear warm restart flag, close out telemetry.
  m_telemetry.record_overall_end(/*success=*/false);
  set_warm_restart_enable(m_db, false);
}

void RebootThread::do_cold_reboot(swss::Select &s) {
  SWSS_LOG_ENTER();
  SWSS_LOG_NOTICE("Sending cold reboot request to platform");
  if (send_dbus_reboot_request() == Progress::EXIT_EARLY) {
    return;
  }

  // Wait for platform to reboot. If we return, reboot failed.
  // Logging, error status and monitoring for critical state are handled within.
  if (wait_for_platform_reboot(s) == Progress::EXIT_EARLY) {
    return;
  }

  // We shouldn't be here. Platform reboot should've killed us.
  log_error_and_set_non_retry_failure("platform failed to reboot");

  // Set critical state
  m_critical_interface.report_critical_state("platform failed to reboot");
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
             request.method() != RebootMethod::NSF) {
    response.json_string = "RebootThread: Start rx'd unsupported method";
    response.status = swss::StatusCode::SWSS_RC_INVALID_PARAM;
  } else if (request.delay() != 0) {
    response.json_string = "RebootThread: delayed start not supported";
    response.status = swss::StatusCode::SWSS_RC_INVALID_PARAM;
  } else if (request.method() == RebootMethod::NSF) {
    if (m_status.get_last_reboot_status() ==
        RebootStatus_Status::RebootStatus_Status_STATUS_FAILURE) {
      // If the last reboot failed with a non-retriable failure, don't retry.
      // But, we will allow a cold boot to recover.
      response.json_string =
          "RebootThread: last NSF failed with non-retriable failure";
      response.status = swss::StatusCode::SWSS_RC_FAILED_PRECONDITION;
    } else if (m_critical_interface.is_system_critical()) {
      response.json_string = "RebootThread: in critical state, NSF not allowed";
      response.status = swss::StatusCode::SWSS_RC_FAILED_PRECONDITION;
    }
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

bool RebootThread::check_and_log_critical_state(
    const std::string error_string) {
  SWSS_LOG_ENTER();
  if (m_critical_interface.is_system_critical()) {
    // Critical state isn't retriable.
    log_error_and_set_non_retry_failure(error_string);
    return true;
  }
  return false;
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

RebootThread::Status RebootThread::handle_state_verification_event(
    swss::SubscriberStateTable &sub, std::string &timestamp) {
  swss::KeyOpFieldsValuesTuple kco;
  sub.pop(kco);

  std::string key = kfvKey(kco);

  if (key != ALL_COMPONENT) {
    // we only care about updates to the "all" key
    return Status::KEEP_WAITING;
  }

  std::vector<swss::FieldValueTuple> fvs = kfvFieldsValues(kco);
  std::string status;
  std::string ts;

  for (const auto &fv : fvs) {
    if (fvField(fv) == TIMESTAMP_FIELD) {
      ts = fvValue(fv);
    } else if (fvField(fv) == STATUS_FIELD) {
      status = fvValue(fv);
    }
  }

  if (ts != timestamp) {
    // if this wasn't our state verification request
    return Status::KEEP_WAITING;
  }

  // We've received a valid state verification update
  // key was ALL_COMPONENT and timestamp matched our
  // last request.

  if (status == SV_NOT_RUN) {
    // restart state verification
    timestamp = send_state_verification_notification(m_db, false);
    return Status::KEEP_WAITING;
  }

  if (status == SV_PASS) {
    return Status::SUCCESS;
  } else if (status == SV_FAIL) {
    // Hard failure is not retriable: not_run as final status
    // is retriable.
    log_error_and_set_non_retry_failure(
        "state verification failed during reboot");
    return Status::FAILURE;
  }

  return Status::KEEP_WAITING;
}

Progress RebootThread::state_verification_select(
    swss::Select &s, swss::SelectableTimer &l_timer,
    swss::SubscriberStateTable &sub, std::string &timestamp) {
  SWSS_LOG_ENTER();

  while (true) {
    swss::Selectable *sel;
    int select_ret;
    select_ret = s.select(&sel);

    if (select_ret == swss::Select::ERROR) {
      SWSS_LOG_NOTICE("Error: %s!", strerror(errno));
      continue;
    }
    if (select_ret != swss::Select::OBJECT) {
      SWSS_LOG_NOTICE("select returned unexpedted non-OBJECT");
      continue;
    }

    if (sel == &m_stop) {
      SWSS_LOG_ERROR("m_stop (sigterm) rx'd during reboot state verification");
      return Progress::EXIT_EARLY;
    } else if (sel == &l_timer) {
      // Timeout during state verification is a retriable error.
      log_error_and_set_failure_as_retriable(
          "timeout occurred during reboot state verification: retriable error");
      return Progress::EXIT_EARLY;
    } else if (sel == &sub) {
      Status status = handle_state_verification_event(sub, timestamp);

      if (status == Status::SUCCESS) {
        SWSS_LOG_NOTICE("state verification reported success");
        return Progress::PROCEED;
      } else if (status == Status::FAILURE) {
        // error is logged and error string set in
        // handle_state_verification_event.
        return Progress::EXIT_EARLY;
      } else {
        continue;
      }
    }
  }
}

Progress RebootThread::perform_state_verification(swss::Select &s) {
  if (check_and_log_critical_state(
          "system entered critical state before reboot state verification")) {
    return Progress::EXIT_EARLY;
  }

  if (!swss::WarmStart::isStateVerificationShutdownEnabled()) {
    // if state verification isn't enabled in CONFIG_DB: skip state verification
    SWSS_LOG_NOTICE("State verification not enabled");
    return Progress::PROCEED;
  }

  swss::SelectableTimer l_timer(
      timespec{.tv_sec = m_state_verification_timeout, .tv_nsec = 0});
  s.addSelectable(&l_timer);

  swss::SubscriberStateTable sub(&m_db, STATE_VERIFICATION_RESP_TABLE);
  s.addSelectable(&sub);

  l_timer.start();

  // Send a non-frozen state verifiation request.
  std::string timestamp = send_state_verification_notification(m_db, false);
  SWSS_LOG_NOTICE("State verification triggered, waiting for result");

  Progress progress = state_verification_select(s, l_timer, sub, timestamp);

  l_timer.stop();
  s.removeSelectable(&l_timer);
  s.removeSelectable(&sub);
  return progress;
}

//
// Stop On Freeze Support
//

// Perform quiescence and container stop in parallel.
// First we request container stop.
// Freeze is sent to all containers.
// We wait for containers to quiesce (or checkpoint).
// We wait for containers to stop.
Progress RebootThread::perform_freeze_w_container_stop(swss::Select &s) {
  SWSS_LOG_ENTER();
  std::string request_id;

  SWSS_LOG_NOTICE("Requesting container stop on freeze");
  if (Progress::EXIT_EARLY == request_stop_on_freeze(request_id)) {
    SWSS_LOG_ERROR("request_stop_on_freeze: returned EXIT_EARLY");
    return Progress::EXIT_EARLY;
  }
  SWSS_LOG_NOTICE("Stop on freeze request sent.");

  swss::SelectableTimer l_timer(
      milliseconds_to_timespec(m_quiescence_timeout_ms));
  s.addSelectable(&l_timer);
  l_timer.start();

  Progress progress =
      wait_for_container_stop_and_quiescence(s, l_timer, request_id);

  s.removeSelectable(&l_timer);
  return progress;
}

Progress RebootThread::wait_for_container_stop_and_quiescence(
    swss::Select &s, swss::SelectableTimer &l_timer,
    const std::string &request_id) {
  SWSS_LOG_NOTICE("waiting for containers to stop");
  if (Progress::EXIT_EARLY == wait_for_container_stop(s, request_id, l_timer)) {
    SWSS_LOG_ERROR("wait_for_container_stop: returned EXIT_EARLY");
    return Progress::EXIT_EARLY;
  }

  SWSS_LOG_NOTICE("starting freeze quiescence");
  if (Progress::EXIT_EARLY == perform_freeze_quiescence(s, l_timer)) {
    SWSS_LOG_ERROR(
        "perform_freeze_quiescence: returned EXIT_EARLY. Outstanding apps: %s",
        m_registration
            .join_pending_apps(swss::WarmStart::WarmBootStage::STAGE_FREEZE)
            .c_str());
    return Progress::EXIT_EARLY;
  }

  return Progress::PROCEED;
}

Progress RebootThread::build_stop_container_request(std::string &json_request,
                                                    std::string &request_id) {
  SWSS_LOG_ENTER();

  request_id = swss::getTimestamp();
  StopContainersRequest request;
  request.set_request_id(request_id);

  // Get the list of apps that need to be stopped
  Registration::RegistrationSet stop_set =
      m_registration.get_stop_on_freeze_set();

  for (const std::string &app : stop_set) {
    request.add_container_names(app);
  }

  gpu::Status status = gpu::MessageToJsonString(request, &json_request);

  if (!status.ok()) {
    std::string error_string =
        "unable to convert StopContainersRequest protobuf to json: " +
        status.message().as_string();
    log_error_and_set_non_retry_failure(error_string);
    return Progress::EXIT_EARLY;
  }
  return Progress::PROCEED;
}

Progress RebootThread::request_stop_on_freeze(std::string &request_id) {
  SWSS_LOG_ENTER();

  // Get the list of apps that need to be stopped
  Registration::RegistrationSet stop_set =
      m_registration.get_stop_on_freeze_set();

  if (stop_set.empty()) {
    return Progress::PROCEED;
  }

  std::string json_request;
  if (build_stop_container_request(json_request, request_id) ==
      Progress::EXIT_EARLY) {
    return Progress::EXIT_EARLY;
  }

  // Send the stop containers request to the stop container host service via
  // dbus.
  DbusInterface::DbusResponse dbus_response =
      m_dbus_interface.StopContainers(json_request);

  if (dbus_response.status == DbusInterface::DbusStatus::DBUS_FAIL) {
    log_error_and_set_non_retry_failure(dbus_response.json_string);
    return Progress::EXIT_EARLY;
  }
  return Progress::PROCEED;
}

RebootThread::Status RebootThread::check_container_stop(
    const std::string &request_id) {
  SWSS_LOG_ENTER();
  StopContainersStatusRequest request;
  request.set_request_id(request_id);

  std::string json_request;
  gpu::Status status = gpu::MessageToJsonString(request, &json_request);

  if (!status.ok()) {
    SWSS_LOG_ERROR(
        "unable to convert StopContainersStatusRequest protobuf to json: %s",
        status.message().as_string().c_str());
    return Status::FAILURE;
  }

  // Send the stop containers request to the stop container host service via
  // dbus.
  DbusInterface::DbusResponse dbus_response =
      m_dbus_interface.StopContainerStatus(json_request);

  if (dbus_response.status == DbusInterface::DbusStatus::DBUS_FAIL) {
    SWSS_LOG_ERROR("StopContainersStatus returned ERROR: %s",
                   dbus_response.json_string.c_str());
    return Status::FAILURE;
  }

  StopContainersResponse response;
  status = gpu::JsonStringToMessage(dbus_response.json_string, &response);
  if (!status.ok()) {
    SWSS_LOG_ERROR(
        "unable to convert StopContainersStatus json |%s| to prototobuf: |%s|",
        dbus_response.json_string.c_str(),
        status.message().as_string().c_str());
    return Status::FAILURE;
  }

  if (response.status() == ShutdownStatus::DONE) {
    return Status::SUCCESS;
  } else if (response.status() == ShutdownStatus::ERROR) {
    SWSS_LOG_ERROR(
        "Container stop service reported error shutting down containers: %s",
        response.DebugString().c_str());
  }
  return Status::KEEP_WAITING;
}

RebootThread::Status RebootThread::precheck_wait_for_container_stop(
    const std::string &request_id) {
  // Get the list of apps that need to be stopped
  Registration::RegistrationSet stop_set =
      m_registration.get_stop_on_freeze_set();

  if (stop_set.empty()) {
    return Status::SUCCESS;
  }
  if (check_container_stop(request_id) == Status::SUCCESS) {
    return Status::SUCCESS;
  }
  return Status::KEEP_WAITING;
}

Progress RebootThread::wait_for_container_stop(swss::Select &s,
                                               const std::string &request_id,
                                               swss::SelectableTimer &l_timer) {
  SWSS_LOG_ENTER();

  // Have containers stopped? Are there no containers to stop?
  if (Status::SUCCESS == precheck_wait_for_container_stop(request_id)) {
    return Progress::PROCEED;
  }

  if (check_and_log_critical_state("system entered critical state while "
                                   "waiting for containers to stop")) {
    return Progress::EXIT_EARLY;
  }

  while (true) {
    swss::Selectable *sel;
    int select_ret;
    select_ret = s.select(&sel, SELECT_TIMEOUT_500_MS);

    if (Status::SUCCESS == check_container_stop(request_id)) {
      return Progress::PROCEED;
    }

    if (select_ret == swss::Select::ERROR) {
      SWSS_LOG_NOTICE("Error: %s!", strerror(errno));
      continue;
    }

    if (select_ret == swss::Select::TIMEOUT) {
      // Don't flood logs on timeout.
      continue;
    }

    if (select_ret != swss::Select::OBJECT) {
      SWSS_LOG_NOTICE("select returned unexpected non-OBJECT");
      continue;
    }

    if (sel == &m_stop) {
      SWSS_LOG_NOTICE(
          "m_stop (sigterm) rx'd while waiting for containers to stop");
      return Progress::EXIT_EARLY;
    } else if (sel == &l_timer) {
      log_error_and_set_non_retry_failure(
          "timeout occurred waiting for containers to stop");
      return Progress::EXIT_EARLY;
    }
  }
}

//
// Freeze Quiescence Support
//
Progress RebootThread::perform_freeze_quiescence(
    swss::Select &s, swss::SelectableTimer &l_timer) {
  SWSS_LOG_ENTER();
  if (check_and_log_critical_state(
          "system entered critical state before freezing")) {
    return Progress::EXIT_EARLY;
  }

  swss::SubscriberStateTable sub(&m_db, STATE_WARM_RESTART_TABLE_NAME);
  s.addSelectable(&sub);

  send_nsf_manager_notification(m_db,
                                swss::WarmStart::WarmBootNotification::kFreeze);
  SWSS_LOG_NOTICE(
      "freeze signal sent, waiting for apps to reach frozen state: %s",
      m_registration
          .join_pending_apps(swss::WarmStart::WarmBootStage::STAGE_FREEZE)
          .c_str());

  Progress progress = freeze_quiescence_select(s, l_timer, sub);

  s.removeSelectable(&sub);
  return progress;
}

Progress RebootThread::freeze_quiescence_select(
    swss::Select &s, swss::SelectableTimer &l_timer,
    swss::SubscriberStateTable &sub) {
  SWSS_LOG_ENTER();

  steady_clock::time_point start_time;
  bool quiesced = false;

  // Check the current status of all registered apps.
  Registration::Response response = m_registration.check_quiesced();

  if (response.status == Registration::Status::FAILURE) {
    log_error_and_set_non_retry_failure(response.error_string);
    return Progress::EXIT_EARLY;
  }

  if (response.status == Registration::Status::COMPLETED) {
    // We're quiesced: set start time for 10 second quiescence hold timer
    quiesced = true;
    start_time = steady_clock::now();
  }

  while (true) {
    swss::Selectable *sel;
    int select_ret;

    // Set timeout to 250 milli-seconds. We'll wake up at least every
    // quarter second and can check if quiescence hold time has passed.
    select_ret = s.select(&sel, SELECT_TIMEOUT_250_MS);

    if (quiesced) {
      if (steady_clock::now() - start_time >
          std::chrono::milliseconds(m_quiescence_hold_time_ms)) {
        // We've been quiesced for 10 seconds: we're ready to PROCEED.
        return Progress::PROCEED;
      }
    }

    if (select_ret == swss::Select::TIMEOUT) {
      // Don't flood logs on timeout.
      continue;
    }

    if (select_ret == swss::Select::ERROR) {
      SWSS_LOG_NOTICE("Error: %s!", strerror(errno));
      continue;
    }
    if (select_ret != swss::Select::OBJECT) {
      SWSS_LOG_NOTICE("select returned unexpected non-OBJECT");
      continue;
    }

    if (sel == &m_stop) {
      std::string error_string =
          "m_stop (sigterm) rx'd during reboot " +
          Registration::get_warm_boot_stage_name(WarmBootStage::STAGE_FREEZE);
      SWSS_LOG_ERROR("%s\n", error_string.c_str());
      return Progress::EXIT_EARLY;
    } else if (sel == &l_timer) {
      // TODO: use getWarmBootStageFromState() or warmBootStateToStageMap()
      //       to get warm restart stage rather state
      std::string error_string =
          "timeout occurred during reboot stage " +
          Registration::get_warm_boot_stage_name(WarmBootStage::STAGE_FREEZE);
      log_error_and_set_non_retry_failure(error_string);
      return Progress::EXIT_EARLY;
    } else if (sel == &sub) {
      swss::KeyOpFieldsValuesTuple kco;
      sub.pop(kco);

      Registration::Response response =
          m_registration.handle_state_event(WarmBootStage::STAGE_FREEZE, kco);

      if (response.status == Registration::Status::FAILURE) {
        log_error_and_set_non_retry_failure(response.error_string);
        return Progress::EXIT_EARLY;
      } else if (response.status == Registration::Status::COMPLETED) {
        // We're quiesced: set start time for 10 second quiescence hold timer
        quiesced = true;
        start_time = steady_clock::now();
      } else {
        // Registration::Status::IN_PROCESS
        quiesced = false;
      }
    }
  }
}

//
// Checkpoint support.
//
Progress RebootThread::perform_checkpoint(swss::Select &s) {
  SWSS_LOG_ENTER();
  if (check_and_log_critical_state(
          "system entered critical state before checkpointing")) {
    return Progress::EXIT_EARLY;
  }

  swss::SelectableTimer l_timer(
      timespec{.tv_sec = m_checkpoint_timeout, .tv_nsec = 0});
  s.addSelectable(&l_timer);

  swss::SubscriberStateTable sub(&m_db, STATE_WARM_RESTART_TABLE_NAME);
  s.addSelectable(&sub);

  l_timer.start();

  send_nsf_manager_notification(
      m_db, swss::WarmStart::WarmBootNotification::kCheckpoint);
  SWSS_LOG_NOTICE(
      "checkpoint signal sent, waiting for apps to reach checkpointed state: "
      "%s",
      m_registration
          .join_pending_apps(swss::WarmStart::WarmBootStage::STAGE_CHECKPOINT)
          .c_str());

  Progress progress = checkpoint_select_stage_one(s, l_timer, sub);

  l_timer.stop();
  s.removeSelectable(&l_timer);
  s.removeSelectable(&sub);
  return progress;
}

Progress RebootThread::checkpoint_select_stage_one(
    swss::Select &s, swss::SelectableTimer &l_timer,
    swss::SubscriberStateTable &sub) {
  SWSS_LOG_ENTER();

  // We're subscribed: so we wont miss any events.
  // Check the current status of all registered apps.
  Registration::Response response = m_registration.check_checkpointed();
  if (response.status == Registration::Status::COMPLETED) {
    return Progress::PROCEED;
  }
  if (response.status == Registration::Status::FAILURE) {
    log_error_and_set_non_retry_failure(
        "check_checkpointed returned error: " + response.error_string +
        ". Outstanding apps: " +
        m_registration.join_pending_apps(
            swss::WarmStart::WarmBootStage::STAGE_CHECKPOINT));
    return Progress::EXIT_EARLY;
  }
  return checkpoint_stage_two(s, l_timer, sub);
}

Progress RebootThread::checkpoint_stage_two(swss::Select &s,
                                            swss::SelectableTimer &l_timer,
                                            swss::SubscriberStateTable &sub) {
  while (true) {
    swss::Selectable *sel;
    int select_ret;
    select_ret = s.select(&sel);

    if (select_ret == swss::Select::ERROR) {
      SWSS_LOG_NOTICE("Error: %s!", strerror(errno));
      continue;
    }
    if (select_ret != swss::Select::OBJECT) {
      SWSS_LOG_NOTICE("select returned unexpected non-OBJECT");
      continue;
    }

    if (sel == &m_stop) {
      std::string error_string = "m_stop (sigterm) rx'd during reboot " +
                                 Registration::get_warm_boot_stage_name(
                                     WarmBootStage::STAGE_CHECKPOINT);
      SWSS_LOG_ERROR("%s\n", error_string.c_str());
      return Progress::EXIT_EARLY;
    } else if (sel == &l_timer) {
      // TODO: use getWarmBootStageFromState() or warmBootStateToStageMap()
      //       to get warm restart stage rather state
      std::string error_string =
          "timeout occurred during reboot stage " +
          Registration::get_warm_boot_stage_name(
              WarmBootStage::STAGE_CHECKPOINT) +
          ". Outstanding apps: " +
          m_registration.join_pending_apps(
              swss::WarmStart::WarmBootStage::STAGE_CHECKPOINT);
      log_error_and_set_non_retry_failure(error_string);
      return Progress::EXIT_EARLY;
    } else if (sel == &sub) {
      swss::KeyOpFieldsValuesTuple kco;
      sub.pop(kco);
      Registration::Response response = m_registration.handle_state_event(
          WarmBootStage::STAGE_CHECKPOINT, kco);
      if (response.status == Registration::Status::COMPLETED) {
        return Progress::PROCEED;
      } else if (response.status == Registration::Status::FAILURE) {
        log_error_and_set_non_retry_failure(response.error_string);
        return Progress::EXIT_EARLY;
      } else {
        continue;
      }
    }
  }
}

}  // namespace rebootbackend
