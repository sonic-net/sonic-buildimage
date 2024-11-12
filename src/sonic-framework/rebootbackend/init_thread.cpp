#include "init_thread.h"

#include <google/protobuf/util/json_util.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <functional>
#include <mutex>
#include <unordered_set>
#include <vector>

//#include "component_state_helper.h"
#include "dbconnector.h"
#include "logger.h"
#include "notificationproducer.h"
#include "reboot_interfaces.h"
#include "rebootbe.h"
#include "redis_utils.h"
#include "redisselect.h"
#include "select.h"
#include "selectableevent.h"
#include "selectabletimer.h"
//#include "stateverification.h"
#include "status_code_util.h"
#include "subscriberstatetable.h"
#include "warm_restart.h"

namespace rebootbackend {

using WarmStartState = ::swss::WarmStart::WarmStartState;
//using WarmBootStage = ::swss::WarmStart::WarmBootStage;

InitThread::InitThread(
//		       CriticalStateInterface &critical_interface,
                       TelemetryInterface &telemetry_interface,
                       swss::SelectableEvent &m_finished,
                       swss::SelectableEvent &m_stack_unfrozen)
    : m_db("STATE_DB", 0),
      m_finished(m_finished),
      m_stack_unfrozen(m_stack_unfrozen),
//    m_critical_interface(critical_interface),
      m_telemetry(telemetry_interface) {}

swss::StatusCode InitThread::Start() {
  swss::StatusCode result = internal_start();
  if (result != swss::StatusCode::SWSS_RC_SUCCESS) {
    do_final_failed_actions();
    m_status.set_inactive();
  }
  return result;
}

swss::StatusCode InitThread::internal_start() {
  SWSS_LOG_ENTER();

  /* if (m_critical_interface.is_system_critical()) {
    const std::string error_string =
        "InitThread: in critical state, not starting stack.";
    SWSS_LOG_ERROR("%s", error_string.c_str());
    m_status.set_start_status();
    m_status.set_error(
        InitThreadStatus::ErrorCondition::DETECTED_CRITICAL_STATE,
        error_string);
    return swss::StatusCode::SWSS_RC_FAILED_PRECONDITION;
  } */

  m_status.set_start_status();

  try {
    m_thread = std::thread(&InitThread::init_thread, this);
  } catch (const std::system_error &e) {
    std::string error_string = "Exception launching init thread: ";
    error_string += e.what();
    SWSS_LOG_ERROR("%s", error_string.c_str());

    m_status.set_error(InitThreadStatus::ErrorCondition::INTERNAL_ERROR,
                       error_string);

    return swss::StatusCode::SWSS_RC_INTERNAL;
  }
  return swss::StatusCode::SWSS_RC_SUCCESS;
}

void InitThread::init_thread(void) {
  SWSS_LOG_ENTER();

  // Check if stop was requested before m_stop was constructed. If m_stop has
  // been signaled already, this will be caught in later Select's.
  if (sigterm_requested) {
    const std::string error_string = "sigterm_requested was raised, exiting";
    SWSS_LOG_ERROR("%s", error_string.c_str());
    m_status.set_error(InitThreadStatus::ErrorCondition::INTERNAL_ERROR,
                       error_string);
    do_final_failed_actions();
    m_finished.notify();
    return;
  }

  swss::SelectableTimer registration_reconciliation_timer(
      timespec{.tv_sec = m_reconciliation_timeout, .tv_nsec = 0});
  registration_reconciliation_timer.start();

  m_status.set_detailed_thread_status(
      InitThreadStatus::ThreadStatus::WAITING_FOR_REGISTRATION);
  swss::StatusCode result =
      handle_registration_step(registration_reconciliation_timer);
  if (result != swss::StatusCode::SWSS_RC_SUCCESS) {
    m_status.set_error(InitThreadStatus::ErrorCondition::REGISTRATION_FAILED,
                       "Registration failed with error");
    do_final_failed_actions();
    m_finished.notify();
    return;
  }

  m_status.set_detailed_thread_status(
      InitThreadStatus::ThreadStatus::WAITING_FOR_RECONCILIATION);
  // Reconciliation start time is recorded by the platform layer when the
  // database is started.

  /* result = handle_reconciliation_step(registration_reconciliation_timer);
  if (result == swss::StatusCode::SWSS_RC_SUCCESS) {
    m_telemetry.record_stage_end(WarmBootStage::STAGE_RECONCILIATION,
                                 true);
  } else {
    m_status.set_error(InitThreadStatus::ErrorCondition::RECONCILIATION_FAILED,
                       "Reconciliation failed with error");
     m_telemetry.record_stage_end(WarmBootStage::STAGE_RECONCILIATION,
                                 false);
    do_final_failed_actions();
    m_finished.notify();
    return;
  } */

  registration_reconciliation_timer.stop();

/*  bool state_verification_enabled =
      swss::WarmStart::isStateVerificationBootupEnabled();
  if (state_verification_enabled) {
    m_status.set_detailed_thread_status(
        InitThreadStatus::ThreadStatus::WAITING_FOR_STATE_VERIFICATION);
    result = handle_state_verification_step();
    if (result != swss::StatusCode::SWSS_RC_SUCCESS) {
      m_status.set_error(
          InitThreadStatus::ErrorCondition::STATE_VERIFICATION_FAILED,
          "State verification failed with error");
      do_final_failed_actions();
      m_finished.notify();
      return;
    }

    m_telemetry.record_stage_start(WarmBootStage::STAGE_UNFREEZE);
    send_nsf_manager_notification(
        m_db, swss::WarmStart::WarmBootNotification::kUnfreeze);
    m_stack_unfrozen.notify();

    m_status.set_detailed_thread_status(
        InitThreadStatus::ThreadStatus::WAITING_FOR_UNFREEZE);

    result = handle_unfreeze_step();
    if (result == swss::StatusCode::SWSS_RC_SUCCESS) {
      m_telemetry.record_stage_end(WarmBootStage::STAGE_UNFREEZE,
                                   true);
    } else {
      m_status.set_error(InitThreadStatus::ErrorCondition::UNFREEZE_FAILED,
                         "Unfreeze failed with error");
      m_telemetry.record_stage_end(WarmBootStage::STAGE_UNFREEZE,
                                   false);
      do_final_failed_actions();
      m_finished.notify();
      return;
    }
  } else {
    SWSS_LOG_NOTICE("Skipping state verification and unfreeze polling");
  } */

  m_telemetry.record_overall_end(/*success=*/true);
  // We've completed warm restart: clear the flag
  set_warm_restart_enable(m_db, false);
  m_status.set_success();
  // Notify calling thread that init thread has exited.
  // Calling thread MUST call Join() to join and set thread status to inactive.
  m_finished.notify();
  SWSS_LOG_NOTICE(
      "InitThread done post-boot steps. System unblocked for future warmboots");
}

void InitThread::Stop(void) {
  SWSS_LOG_ENTER();
  m_stop.notify();
}

bool InitThread::Join(void) {
  SWSS_LOG_ENTER();

  if (!m_thread.joinable()) {
    SWSS_LOG_ERROR("InitThread::Join called, but not joinable");
    return false;
  }

  bool ret = true;
  try {
    m_thread.join();
    m_status.set_inactive();
  } catch (const std::system_error &e) {
    SWSS_LOG_ERROR("Exception calling join: %s", e.what());
    ret = false;
  }
  return ret;
}

InitThreadStatus::DetailedStatus InitThread::GetDetailedStatus() {
  return m_status.get_detailed_thread_status();
}

gnoi::system::RebootStatusResponse InitThread::GetResponse() {
  return m_status.get_response();
}

InitThread::SelectStatus InitThread::ToSelectStatus(
    Registration::Response result) {
  switch (result.status) {
    case Registration::Status::COMPLETED: {
      return SelectStatus::SUCCESS;
    }
    case Registration::Status::FAILURE: {
      return SelectStatus::FAILURE;
    }
    case Registration::Status::IN_PROCESS: {
      return SelectStatus::KEEP_WAITING;
    }
  }
  return SelectStatus::FAILURE;
}

InitThread::SelectStatus InitThread::ToSelectStatus(
    InitRegistration::Status status) {
  switch (status) {
    case InitRegistration::Status::COMPLETED: {
      return SelectStatus::SUCCESS;
    }
    case InitRegistration::Status::IN_PROGRESS: {
      return SelectStatus::KEEP_WAITING;
    }
  }
  return SelectStatus::FAILURE;
}

swss::StatusCode InitThread::ToStatusCode(SelectStatus select_status) {
  switch (select_status) {
    case SelectStatus::SUCCESS: {
      return swss::StatusCode::SWSS_RC_SUCCESS;
    }
    case SelectStatus::FAILURE: {
      return swss::StatusCode::SWSS_RC_INTERNAL;
    }
    case SelectStatus::KEEP_WAITING: {
      return swss::StatusCode::SWSS_RC_INTERNAL;
    }
  }
  return swss::StatusCode::SWSS_RC_INTERNAL;
}

void InitThread::do_final_failed_actions() {
  SWSS_LOG_ENTER();
  InitThreadStatus::DetailedStatus detailed_status =
      m_status.get_detailed_thread_status();
  if (detailed_status.detailed_thread_status ==
          InitThreadStatus::ThreadStatus::ERROR &&
      detailed_status.detailed_thread_error_condition ==
          InitThreadStatus::ErrorCondition::UNFREEZE_FAILED) {
    SWSS_LOG_NOTICE(
        "Error occurred after sending unfreeze, raising minor alarm");
   }
/*    m_critical_interface.report_minor_alarm(
        "Encountered error during unfreeze");
  } else if (!m_critical_interface.is_system_critical()) {
    SWSS_LOG_NOTICE(
        "Error occured and system is not already critical, raising critical "
        "state");
    m_critical_interface.report_critical_state(
        "Encountered error with InitThread in state: " +
        std::to_string(detailed_status.detailed_thread_error_condition));
  } */
  set_warm_restart_enable(m_db, false);
  m_telemetry.record_overall_end(/*success=*/false);
}

swss::StatusCode InitThread::handle_registration_step(
    swss::SelectableTimer &timer_select) {
  SWSS_LOG_ENTER();
  SWSS_LOG_NOTICE("Starting InitThread Registration step");

  // TODO(b/322034421): Improve critical state detection.
/*  if (m_critical_interface.is_system_critical()) {
    SWSS_LOG_ERROR("InitThread: in critical state, not unfreezing stack.");
    return swss::StatusCode::SWSS_RC_FAILED_PRECONDITION;
  } */

/*  swss::SubscriberStateTable table_sub(
      &m_db, STATE_WARM_RESTART_REGISTRATION_TABLE_NAME); */

  InitRegistration init_registration;
  init_registration.fetch_init_app_info();
  SWSS_LOG_NOTICE("Waiting for apps to reregister: %s",
                  init_registration.join_pending_apps().c_str());
  /* auto initial_check_lambda = [&]() {
    return InitThread::ToSelectStatus(
        init_registration.check_reregistration_status());
  };
  auto handle_table_event_lambda =
      [&](const swss::KeyOpFieldsValuesTuple &kco) {
        return InitThread::ToSelectStatus(
            init_registration.handle_registration_event(kco));
      }; */

  /* swss::StatusCode result = select_loop(
     // timer_select, table_sub, initial_check_lambda, handle_table_event_lambda);
      timer_select, initial_check_lambda, handle_table_event_lambda);
  if (result == swss::StatusCode::SWSS_RC_SUCCESS) {
    SWSS_LOG_NOTICE("InitThread Registration step reported success");
  } else {
    SWSS_LOG_ERROR(
        "Error while waiting for re-registration: missing apps: %s Error "
        "text: %s",
        init_registration.join_pending_apps().c_str(),
        swss::statusCodeToStr(result).c_str());
  }
  return result; */
  return swss::StatusCode::SWSS_RC_SUCCESS;
}

/* swss::StatusCode InitThread::handle_reconciliation_step(
    swss::SelectableTimer &timer_select) {
  SWSS_LOG_ENTER();
  SWSS_LOG_NOTICE("Starting InitThread Reconciliation step");

  // TODO(b/322034421): Improve critical state detection.
  if (m_critical_interface.is_system_critical()) {
    SWSS_LOG_ERROR("InitThread: in critical state, not unfreezing stack.");
    return swss::StatusCode::SWSS_RC_FAILED_PRECONDITION;
  }

  // Precise error logged within.
  swss::StatusCode result =
      wait_for_state(WarmBootStage::STAGE_RECONCILIATION, timer_select); 
  SWSS_LOG_NOTICE("InitThread Reconciliation step finished with status: %s",
                  swss::statusCodeToStr(result).c_str()); 
  return result; 
} */

/* swss::StatusCode InitThread::handle_unfreeze_step() {
  SWSS_LOG_ENTER();
  SWSS_LOG_NOTICE("Starting InitThread Unfreeze step");

  // TODO(b/322034421): Improve critical state detection.
  if (m_critical_interface.is_system_critical()) {
    SWSS_LOG_ERROR(
        "InitThread: in critical state, not monitoring for stack unfreeze");
    return swss::StatusCode::SWSS_RC_FAILED_PRECONDITION;
  }

  swss::SelectableTimer timer_select(
      timespec{.tv_sec = m_unfreeze_timeout, .tv_nsec = 0});
  timer_select.start();

  // Precise error logged within.
  swss::StatusCode result =
      wait_for_state(WarmBootStage::STAGE_UNFREEZE, timer_select);
  SWSS_LOG_NOTICE("InitThread Unfreeze step finished with status: %s",
                  swss::statusCodeToStr(result).c_str());
  return result; 
} */

/* swss::StatusCode InitThread::wait_for_state( 
    WarmBootStage nsf_stage, 
      swss::SelectableTimer &timer_select) {
  swss::SubscriberStateTable table_sub(&m_db, STATE_WARM_RESTART_TABLE_NAME);

  const std::string stage_name =
      Registration::get_warm_boot_stage_name(nsf_stage);
  Registration registration;
  registration.fetch_registration_info();
  SWSS_LOG_NOTICE("Waiting for apps: %s to reach state: %s",
                  registration.join_pending_apps(nsf_stage).c_str(),
                  stage_name.c_str());
  auto initial_check_lambda = [&]() {
    return InitThread::ToSelectStatus(registration.check_stage(nsf_stage));
  };
  auto handle_table_event_lambda =
      [&](const swss::KeyOpFieldsValuesTuple &kco) {
        return InitThread::ToSelectStatus(
            registration.handle_state_event(nsf_stage, kco));
      };

  swss::StatusCode result = select_loop(
      timer_select, table_sub, initial_check_lambda, handle_table_event_lambda);
  if (result == swss::StatusCode::SWSS_RC_SUCCESS) {
    SWSS_LOG_NOTICE("All apps reached state: %s", stage_name.c_str());
  } else {
    SWSS_LOG_ERROR(
        "Error while waiting for state: %s missing apps: %s Error "
        "text: %s",
        stage_name.c_str(), registration.join_pending_apps(nsf_stage).c_str(),
        swss::statusCodeToStr(result).c_str());
  }
  return result;
} */

/* swss::StatusCode InitThread::handle_state_verification_step() {
  SWSS_LOG_ENTER();
  SWSS_LOG_NOTICE("Starting InitThread State Verfication step");

  // TODO(b/322034421): Improve critical state detection.
  if (m_critical_interface.is_system_critical()) {
    SWSS_LOG_ERROR("InitThread: in critical state, not unfreezing stack.");
    return swss::StatusCode::SWSS_RC_FAILED_PRECONDITION;
  }

  swss::SubscriberStateTable table_sub(&m_db, STATE_VERIFICATION_RESP_TABLE);
  swss::SelectableTimer timer_select(
      timespec{.tv_sec = m_state_verification_timeout, .tv_nsec = 0});
  timer_select.start();

  std::string timestamp =
      send_state_verification_notification(m_db, true);
  SWSS_LOG_NOTICE("State verification triggered, waiting for result");

  auto initial_check_lambda = [&]() -> SelectStatus {
    return SelectStatus::KEEP_WAITING;
  };
  auto handle_verification_event_lambda =
      [&](const swss::KeyOpFieldsValuesTuple &kco) -> SelectStatus {
     if (kfvKey(kco) != ALL_COMPONENT) {
      return SelectStatus::KEEP_WAITING;
    } 

    std::string status;
    std::string ts;
    for (const auto &fv : kfvFieldsValues(kco)) {
      if (fvField(fv) == TIMESTAMP_FIELD) {
        ts = fvValue(fv);
      } else if (fvField(fv) == STATUS_FIELD) {
        status = fvValue(fv);
      }
    }

    if (ts != timestamp) {
      return SelectStatus::KEEP_WAITING;
    }

    if (status == SV_PASS) {
      SWSS_LOG_NOTICE("State verification reported success");
      return SelectStatus::SUCCESS;
    } else if (status == SV_NOT_RUN) {
      const std::string message =
          "State verification did not run. Treating as success for NSF";
      SWSS_LOG_WARN("%s", message.c_str());
      m_critical_interface.report_minor_alarm(message);
      return SelectStatus::SUCCESS;
    } else if (status == SV_FAIL) {
      SWSS_LOG_ERROR("State verification reported failure");
      return SelectStatus::FAILURE;
    }
    return SelectStatus::KEEP_WAITING;
  };

  swss::StatusCode result =
      select_loop(timer_select, table_sub, initial_check_lambda,
                  handle_verification_event_lambda);
  if (result == swss::StatusCode::SWSS_RC_DEADLINE_EXCEEDED) {
    SWSS_LOG_WARN("State verification timed out, raising minor alarm: %s",
                  swss::statusCodeToStr(result).c_str());
    m_critical_interface.report_minor_alarm(
        "State verification timed out. Treating as success for NSF");
    return swss::StatusCode::SWSS_RC_SUCCESS;
  } else if (result != swss::StatusCode::SWSS_RC_SUCCESS) {
    SWSS_LOG_ERROR("Error while waiting for state verification: %s",
                   swss::statusCodeToStr(result).c_str());
  }
  return result;
} */

/* swss::StatusCode InitThread::select_loop(
    swss::Selectable &timer_select, swss::SubscriberStateTable &table_sub,
    const std::function<InitThread::SelectStatus()> &initial_check,
    const std::function<InitThread::SelectStatus(
        const swss::KeyOpFieldsValuesTuple &)> &table_event_callback) {
  SWSS_LOG_ENTER();

  swss::Select s;
  s.addSelectable(&m_stop);
  s.addSelectable(&table_sub);
  s.addSelectable(&timer_select);

  SelectStatus select_status = initial_check();
  if (select_status != SelectStatus::KEEP_WAITING) {
    return ToStatusCode(select_status);
  }

  while (true) {
    swss::Selectable *sel;
    int select_result;
    select_result = s.select(&sel);

    if (select_result == swss::Select::ERROR) {
      SWSS_LOG_ERROR("Error in select loop: %s", strerror(errno));
      continue;
    } else if (select_result != swss::Select::OBJECT) {
      SWSS_LOG_NOTICE("Got unexpected non-object from select: %d",
                      select_result);
      continue;
    }

    if (sel == &m_stop) {
      SWSS_LOG_ERROR("m_stop rx'd (SIGTERM) in select loop");
      return swss::StatusCode::SWSS_RC_INTERNAL;
    } else if (sel == &timer_select) {
      SWSS_LOG_ERROR("Timed out in select loop");
      return swss::StatusCode::SWSS_RC_DEADLINE_EXCEEDED;
    } else if (sel == &table_sub) {
      swss::KeyOpFieldsValuesTuple kco;
      table_sub.pop(kco);
      select_status = table_event_callback(kco);
      if (select_status != SelectStatus::KEEP_WAITING) {
        return ToStatusCode(select_status);
      }
    } else {
      SWSS_LOG_ERROR("Got unexpected object event in select loop");
    }
  }
} */

}  // namespace rebootbackend
