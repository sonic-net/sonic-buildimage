#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "dbconnector.h"
#include "notificationconsumer.h"
#include "notificationproducer.h"
#include "selectableevent.h"
#include "status_code_util.h"
#include "warm_restart.h"

namespace rebootbackend {

// Return string corresponding to state
std::string get_warm_start_state_name(
    const swss::WarmStart::WarmStartState state);

// Send Freeze, Checkpoint or Unfreeze notifications to all subscribed
// apps on NSF_MANAGER_COMMON_NOTIFICATION_CHANNEL
void send_nsf_manager_notification(
    swss::DBConnector &db, swss::WarmStart::WarmBootNotification notification);

// For all keys in STATE_WARM_RESTART_TABLE_NAME: delete state field
// This is executed at the beginning of nsf/warm restart to clear out
// existing states.
// From:
//   https://github.com/sonic-net/sonic-utilities/blob/20d1495b6f7e82c4d9aa377c3c281d8d0d9d8594/scripts/fast-reboot#L167
void init_warm_reboot_states(const swss::DBConnector &db);

// Set the system warm start state to a new enabled/disabled state.
// STATE_WARM_RESTART_TABLE_NAME
//   key = system, field = enable, value = "true"/"false"
void set_warm_restart_enable(const swss::DBConnector &db, bool enabled);

// Send a request to state verifiation daemon to perform state
// verification.
// Set freeze == true if system is frozen (i.e. after reboot
//   before reconciliation and unfreeze have occurred.
// timestamp is used to verify that a state update is a response
// to our request, and not someone else's.
std::string send_state_verification_notification(swss::DBConnector &db,
                                                 const bool freeze);

// Returns true if key is in the formm "text<separator>text", and false
// otherwise.
bool is_valid_key(const std::string &key, const std::string &separator);

// Helper function: given key of form "docker|app"
// extract docker and app. (separator = | in this case)
// return false if docker or app are empty or separator
//   isn't present, else true.
// key and separator are inputs
// docker and app are outputs
bool get_docker_app_from_key(const std::string &key,
                             const std::string &separator, std::string &docker,
                             std::string &app);

std::string get_warm_start_state_name(swss::WarmStart::WarmStartState state);

// Sets the warm restart count in the database.
void set_warm_restart_counter(swss::DBConnector &db, int count);

// Returns the current warm restart count from the database. Returns an empty
// string if the warm restart count is not set, and a string representation
// of an integer otherwise.
std::string get_warm_restart_counter(swss::DBConnector &db);

// This class is meant to handle registration information in the
// STATE_WARM_RESTART_REGISTRATION_TABLE_NAME.
// - we maintain the list of registered applications
//   used after to reboot to wait for all to register
// - the list of apps we will wait to quiesce
// - the list of apps we will wait to checkpoint
// - the list of apps we will wait reconcile
// - the list of containers that have requested stop on freeze.
// Not thread safe.
class Registration {
 public:
  enum class Status { COMPLETED, FAILURE, IN_PROCESS };

  struct Response {
    Status status = Status::IN_PROCESS;
    std::string error_string = "";
  };

  typedef std::unordered_set<std::string> RegistrationSet;

  Registration();

  static std::string get_warm_boot_stage_name(
      swss::WarmStart::WarmBootStage stage);

  // Populate this class with contents of
  // STATE_WARM_RESTART_REGISTRATION_TABLE_NAME.
  void fetch_registration_info();

  // Return the set of containers that have requested stop on freeze.
  RegistrationSet get_stop_on_freeze_set();

  // Return the set of applications that have registered.
  RegistrationSet get_registered_app_set();

  // Check application states in warm restart table.
  // Pop applications that are quiesced or checkpointed.
  // Returns:
  //   IN_PROCESS: not all apps have quiesced.
  //   COMPLETED: all registered apps have quiesced.
  //   FAILURE: an application set its state to failed.
  //            error_string in response is populated
  Response check_quiesced();

  // Check application states in warm restart table.
  // Pop applications that are checkpointed.
  // Returns:
  //   IN_PROCESS: not all apps have checkpointed.
  //   COMPLETED: all registered apps have checkpointed.
  //   FAILURE: an application set its state to failed.
  //            error_string in response is populated
  Response check_checkpointed();

  // Check application states in warm restart table.
  // Pop applications that are checkpointed.
  // Returns:
  //   IN_PROCESS: not all apps have checkpointed.
  //   COMPLETED: all registered apps have checkpointed.
  //   FAILURE: an application set its state to failed.
  //            error_string in response is populated
  Response check_reconciled();

  // Check application states in warm restart table.
  // Pop applications that are unfrozen.
  // Returns:
  //   IN_PROCESS: not all apps have unfrozen.
  //   COMPLETED: all registered apps have unfrozen.
  //   FAILURE: an application set its state to failed.
  //            error_string in response is populated
  Response check_unfrozen();

  // Check application states in warm restart table.
  // Pop applications that have reached the target state.
  // Returns:
  //   IN_PROCESS: not all apps have reached the target state.
  //   COMPLETED: all registered apps have reached the target state.
  //   FAILURE: an application set its state to failed.
  //            error_string in response is populated
  Response check_stage(swss::WarmStart::WarmBootStage nsf_stage);

  // Handle an application state change, and update the tracked progress
  // towards that state.
  // Pop applications that have reached the monitored state.
  // FAILURE statuses are sticky, and subsequent calls to handle_state_event
  // will return the FAILURE status.
  // Returns:
  //   IN_PROCESS: not all apps have reached monitored_stage.
  //   COMPLETED: all registered apps have monitored_stage.
  //   FAILURE: an application set its state to failed.
  //            error_string in response is populated
  Response handle_state_event(swss::WarmStart::WarmBootStage monitored_stage,
                              const swss::KeyOpFieldsValuesTuple &kco);

  // Clear all registered app names in the WARM_RESTART_INIT_TABLE.
  void clear_all_init_apps();

  // Saves all registered app names to the WARM_RESTART_INIT_TABLE.
  void save_all_init_apps();

  // Returns a string representation of the current apps that have not
  // reached the target state For debug and logging only.
  std::string join_pending_apps(swss::WarmStart::WarmBootStage target_stage);

 private:
  // Helper function for check_<state> functions.
  // Pop applications that have reached one of the target state names.
  // Returns:
  //   IN_PROCESS: not all apps have reached the correct state.
  //   COMPLETED: all registered apps have reached the correct state.
  //   FAILURE: an application set its state to failed.
  //            error_string in response is populated
  Response check_states_are(RegistrationSet &set_to_check,
                            const std::unordered_set<std::string> &state_names);

  // Helper for handle_state_event: specific handling for FREZE stage.
  //   Pre-condition: caller verifies that operation is a set.
  //                  caller handles apps entering FAILED state.
  // Push applications that report a state other than quiescence/checkpointed.
  //   -- applications may exit/enter quiescence state.
  // Returns:
  //   IN_PROCESS: not all apps have reached monitored_stage.
  //   COMPLETED: all registered apps have monitored_stage.
  Response handle_quiescence_event(const swss::KeyOpFieldsValuesTuple &kco);

  // Filter a specified app list based on an application state change event.
  // Pop applications that have reached one of the target states.
  //   IN_PROCESS: not all apps have reached the correct state.
  //   COMPLETED: all registered apps have reached the correct state.
  //   FAILURE: an application set its state to failed.
  //            error_string in response is populated
  Response filter_app_list(RegistrationSet &set_to_filter,
                           const std::string app_name,
                           const swss::WarmStart::WarmBootStage monitored_state,
                           const swss::KeyOpFieldsValuesTuple &kco,
                           const std::unordered_set<std::string> &state_names);

  // Extracts a "status" field from the event, and returns the value of it.
  // Returns an empty string if the "status" field is not present.
  std::string extract_event_state(const swss::KeyOpFieldsValuesTuple &kco);

  // Clear contents of all sets.
  void clear_contents();

  const static std::unordered_map<swss::WarmStart::WarmBootStage,
                                  const std::unordered_set<std::string>>
      kStageToTargetStates;

  RegistrationSet m_registered;
  RegistrationSet m_stop;
  RegistrationSet m_ro_quiescent_list;
  std::unordered_map<swss::WarmStart::WarmBootStage, RegistrationSet>
      m_remaining_apps;

  swss::DBConnector m_db;

  std::string m_separator;

  friend class RebootThreadTest;
  friend class RedisTest;
};

// This class handles the monitoring of applications re-registered warmboot
// requirements after a warmboot. In general, data from
// WARM_RESTART_INIT_TABLE is used to generate a list of apps that must
// re-register, and the provided API functions monitor
// WARM_RESTART_REGISTRATION_TABLE to determine if all of these apps
// have registered again.
class InitRegistration {
 public:
  enum class Status { COMPLETED, IN_PROGRESS };

  typedef std::unordered_set<std::string> RegistrationSet;

  InitRegistration();

  // Reads the list of apps that must reregister from the
  // WARM_RESTART_INIT_TABLE.
  void fetch_init_app_info();

  // Returns the current state of re-registration.
  // Returns:
  //   COMPLETED: all apps that were registered before the warmboot have
  //     re-registered. This is a requirement for warmboot.
  //   IN_PROGRESS: not all apps have re-registered.
  Status get_reregistration_status();

  // Polls the WARM_RESTART_REGISTRATION_TABLE to retermine if re-registration
  // is complete. Pops elements from the internal set.
  // Returns:
  //   COMPLETED: all apps that were registered before the warmboot have
  //     re-registered. This is a requirement for warmboot.
  //   IN_PROGRESS: not all apps have re-registered.
  Status check_reregistration_status();

  // Handles an registration event in the WARM_RESTART_REGISTRATION_TABLE. Pops
  // elements from the internal set, and returns the current re-registration
  // status.
  // Returns:
  //   COMPLETED: all apps that were registered before the warmboot have
  //     re-registered. This is a requirement for warmboot.
  //   IN_PROGRESS: not all apps have re-registered.
  Status handle_registration_event(const swss::KeyOpFieldsValuesTuple &kco);

  const RegistrationSet &get_pending_apps() const;

  // Returns a string representation of the current apps that have not
  // re-registered. For debug and logging only.
  std::string join_pending_apps();

 private:
  // Pops the app name in key from the internal set, if it exists.
  void remove_pending_app(const std::string &key);

  RegistrationSet m_missing_registrations;
  swss::DBConnector m_db;
  std::string m_separator;

  friend class RedisTest;
};

}  // namespace rebootbackend
