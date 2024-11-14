#include "redis_utils.h"

#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "dbconnector.h"
#include "notificationproducer.h"
#include "stateverification.h"
#include "table.h"
#include "timestamp.h"
#include "warm_restart.h"

namespace rebootbackend {

using WarmStartState = ::swss::WarmStart::WarmStartState;

const std::unordered_map<swss::WarmStart::WarmBootStage,
                         const std::unordered_set<std::string>>
    Registration::kStageToTargetStates = {
        {swss::WarmStart::WarmBootStage::STAGE_FREEZE,
         {get_warm_start_state_name(WarmStartState::QUIESCENT),
          get_warm_start_state_name(WarmStartState::CHECKPOINTED)}},
        {swss::WarmStart::WarmBootStage::STAGE_CHECKPOINT,
         {get_warm_start_state_name(WarmStartState::CHECKPOINTED)}},
        {swss::WarmStart::WarmBootStage::STAGE_RECONCILIATION,
         {get_warm_start_state_name(WarmStartState::RECONCILED)}},
        {swss::WarmStart::WarmBootStage::STAGE_UNFREEZE,
         {get_warm_start_state_name(WarmStartState::COMPLETED)}},
};

void send_nsf_manager_notification(
    swss::DBConnector &db, swss::WarmStart::WarmBootNotification notification) {
  swss::NotificationProducer producer(
      &db, swss::WarmStart::kNsfManagerNotificationChannel);

  std::vector<swss::FieldValueTuple> values;
  std::string notification_string =
      swss::WarmStart::warmBootNotificationNameMap()->at(notification);

  producer.send(notification_string, "", values);
}

std::string send_state_verification_notification(swss::DBConnector &ldb,
                                                 const bool freeze) {
  swss::NotificationProducer producer(&ldb, STATE_VERIFICATION_REQ_CHANNEL);

  std::vector<swss::FieldValueTuple> values;
  values.push_back(
      swss::FieldValueTuple(FREEZE_FIELD, freeze ? "true" : "false"));

  std::string timestamp = swss::getTimestamp();
  producer.send(ALL_COMPONENT, timestamp, values);
  return timestamp;
}

void init_warm_reboot_states(const swss::DBConnector &db) {
  swss::Table table(&db, STATE_WARM_RESTART_TABLE_NAME);
  std::vector<std::string> keys;

  table.getKeys(keys);
  for (auto &key : keys) {
    table.hdel(key, "state");
    table.hdel(key, "timestamp");
  }
}

void set_warm_restart_enable(const swss::DBConnector &db, bool enabled) {
  swss::Table table(&db, STATE_WARM_RESTART_ENABLE_TABLE_NAME);
  table.hset("system", "enable", enabled ? "true" : "false");
}

bool is_valid_key(const std::string &key, const std::string &separator) {
  if (separator.empty()) {
    return false;
  }

  size_t pos = key.find(separator);
  // The separator must exist in the string, and cannot be the first or last
  // character.
  return !(pos == std::string::npos || pos == 0 || pos == key.size() - 1);
}

bool get_docker_app_from_key(const std::string &key,
                             const std::string &separator, std::string &docker,
                             std::string &app) {
  SWSS_LOG_ENTER();

  size_t pos = key.find(separator);

  if (separator.empty()) {
    SWSS_LOG_ERROR("separator [%s] shouldn't be empty", separator.c_str());
    return false;
  }

  if (pos == std::string::npos) {
    SWSS_LOG_ERROR("key [%s] should contain separator [%s]", key.c_str(),
                   separator.c_str());
    return false;
  }

  docker = key.substr(0, pos);
  app = key.substr(pos + separator.length(), std::string::npos);

  if (docker.empty()) {
    SWSS_LOG_ERROR("docker name shouldn't be empty, key = %s", key.c_str());
    return false;
  }

  if (app.empty()) {
    SWSS_LOG_ERROR("app name shouldn't be empty, key = %s", key.c_str());
    return false;
  }
  return true;
}

std::string get_warm_start_state_name(swss::WarmStart::WarmStartState state) {
  return swss::WarmStart::warmStartStateNameMap()->at(state).c_str();
}

void set_warm_restart_counter(swss::DBConnector &db, int count) {
  swss::Table table(&db, "BOOT_INFO");
  table.hset("system", "warmboot-count", std::to_string(count));
}

std::string get_warm_restart_counter(swss::DBConnector &db) {
  swss::Table warmRestartTable(&db, "BOOT_INFO");
  std::string counter;
  warmRestartTable.hget("system", "warmboot-count", counter);
  return counter;
}

Registration::Registration()
    : m_db("STATE_DB", 0),
      m_separator(swss::TableBase::getTableSeparator(m_db.getDbId())),
      m_remaining_apps(
          {{swss::WarmStart::WarmBootStage::STAGE_FREEZE, {}},
           {swss::WarmStart::WarmBootStage::STAGE_CHECKPOINT, {}},
           {swss::WarmStart::WarmBootStage::STAGE_RECONCILIATION, {}},
           {swss::WarmStart::WarmBootStage::STAGE_UNFREEZE, {}}}) {}

std::string Registration::get_warm_boot_stage_name(
    swss::WarmStart::WarmBootStage stage) {
  return swss::WarmStart::warmBootStageToNameMap()->at(stage);
}

void Registration::clear_contents() {
  m_registered.clear();
  m_stop.clear();
  for (auto &stage_set_pair : m_remaining_apps) {
    stage_set_pair.second.clear();
  }
}

Registration::RegistrationSet Registration::get_stop_on_freeze_set() {
  return m_stop;
}

Registration::RegistrationSet Registration::get_registered_app_set() {
  return m_registered;
}

void Registration::fetch_registration_info() {
  SWSS_LOG_ENTER();

  clear_contents();

  swss::Table table(&m_db, STATE_WARM_RESTART_REGISTRATION_TABLE_NAME);
  std::vector<std::string> keys;

  table.getKeys(keys);
  for (auto &key : keys) {
    std::string docker, app;
    if (!get_docker_app_from_key(key, m_separator, docker, app)) {
      SWSS_LOG_ERROR("skipping registration for key = %s", key.c_str());
      continue;
    }

    m_registered.insert(key);

    std::vector<swss::FieldValueTuple> values;
    table.get(key, values);
    for (auto &v : values) {
      // We only care about this field if value is "true".
      // Skip this key if value is false.
      if ("false" == fvValue(v)) continue;

      if (swss::WarmStart::kRegistrationStopOnFreezeKey == fvField(v)) {
        m_stop.insert(docker);
      }
      if (swss::WarmStart::kRegistrationFreezeKey == fvField(v)) {
        m_ro_quiescent_list.insert(app);
        m_remaining_apps.at(swss::WarmStart::WarmBootStage::STAGE_FREEZE)
            .insert(app);
        m_remaining_apps.at(swss::WarmStart::WarmBootStage::STAGE_UNFREEZE)
            .insert(app);
      }
      if (swss::WarmStart::kRegistrationCheckpointKey == fvField(v)) {
        m_remaining_apps.at(swss::WarmStart::WarmBootStage::STAGE_CHECKPOINT)
            .insert(app);
      }
      if (swss::WarmStart::kRegistrationReconciliationKey == fvField(v)) {
        m_remaining_apps
            .at(swss::WarmStart::WarmBootStage::STAGE_RECONCILIATION)
            .insert(app);
      }
    }
  }
}

Registration::Response Registration::check_quiesced() {
  return check_stage(swss::WarmStart::WarmBootStage::STAGE_FREEZE);
}

Registration::Response Registration::check_checkpointed() {
  return check_stage(swss::WarmStart::WarmBootStage::STAGE_CHECKPOINT);
}

Registration::Response Registration::check_reconciled() {
  return check_stage(swss::WarmStart::WarmBootStage::STAGE_RECONCILIATION);
}

Registration::Response Registration::check_unfrozen() {
  return check_stage(swss::WarmStart::WarmBootStage::STAGE_UNFREEZE);
}

Registration::Response Registration::check_stage(
    swss::WarmStart::WarmBootStage nsf_stage) {
  Registration::Response response = check_states_are(
      m_remaining_apps.at(nsf_stage), kStageToTargetStates.at(nsf_stage));
  if (response.status == Registration::Status::FAILURE) {
    response.error_string =
        "check_stage: app: " + response.error_string +
        " reported FAILED during stage: " + get_warm_boot_stage_name(nsf_stage);
  }
  return response;
}

Registration::Response Registration::check_states_are(
    RegistrationSet &set_to_check,
    const std::unordered_set<std::string> &state_names) {
  Registration::Response response;

  swss::Table warmRestartTable(&m_db, STATE_WARM_RESTART_TABLE_NAME);
  for (auto key = set_to_check.begin(); key != set_to_check.end();) {
    std::string state;

    warmRestartTable.hget(*key, "state", state);

    if (state == get_warm_start_state_name(WarmStartState::FAILED)) {
      response.status = Registration::Status::FAILURE;
      response.error_string = *key;
      return response;
    }

    if (state_names.find(state) != std::end(state_names)) {
      key = set_to_check.erase(key);
    } else {
      ++key;
    }
  }

  if (set_to_check.empty()) {
    response.status = Registration::Status::COMPLETED;
    return response;
  }

  response.status = Registration::Status::IN_PROCESS;
  return response;
}

Registration::Response Registration::handle_state_event(
    swss::WarmStart::WarmBootStage monitored_stage,
    const swss::KeyOpFieldsValuesTuple &kco) {
  SWSS_LOG_ENTER();

  RegistrationSet &set_to_check = m_remaining_apps.at(monitored_stage);

  std::string op = kfvOp(kco);
  if (op != "SET") {
    SWSS_LOG_ERROR("ignoring non-SET event: %s", op.c_str());
    return {set_to_check.empty() ? Registration::Status::COMPLETED
                                 : Registration::Status::IN_PROCESS,
            ""};
  }

  Registration::Response response =
      filter_app_list(set_to_check, kfvKey(kco), monitored_stage, kco,
                      kStageToTargetStates.at(monitored_stage));

  if (response.status == Registration::Status::FAILURE) {
    return response;
  }

  if (monitored_stage == swss::WarmStart::WarmBootStage::STAGE_FREEZE) {
    response = handle_quiescence_event(kco);
  }
  return response;
}

// Helper for handle_state_event.
//   Pre-condition: caller verifies that operation is a set.
//                  Caller handles apps entering FAILED state.
Registration::Response Registration::handle_quiescence_event(
    const swss::KeyOpFieldsValuesTuple &kco) {
  SWSS_LOG_ENTER();

  RegistrationSet &set_to_check =
      m_remaining_apps.at(swss::WarmStart::WarmBootStage::STAGE_FREEZE);

  std::string app_name = kfvKey(kco);
  std::string new_state = extract_event_state(kco);

  std::unordered_set<std::string> state_names =
      kStageToTargetStates.at(swss::WarmStart::WarmBootStage::STAGE_FREEZE);

  if (state_names.find(new_state) == std::end(state_names)) {
    // the new_state is not QUIESCENT or CHECKPOINTED, the app isn't quiescent
    if (m_ro_quiescent_list.find(app_name) != std::end(m_ro_quiescent_list)) {
      // We are monitoring quiescence for this app: app isn't quiescent
      // readd app to monitoring list (it might already be there ...)
      set_to_check.insert(app_name);
    }
  }
  return {set_to_check.empty() ? Registration::Status::COMPLETED
                               : Registration::Status::IN_PROCESS,
          ""};
}

Registration::Response Registration::filter_app_list(
    RegistrationSet &set_to_filter, const std::string app_name,
    const swss::WarmStart::WarmBootStage monitored_stage,
    const swss::KeyOpFieldsValuesTuple &kco,
    const std::unordered_set<std::string> &state_names) {
  std::string new_state = extract_event_state(kco);

  if (new_state == get_warm_start_state_name(WarmStartState::FAILED)) {
    return {Registration::Status::FAILURE,
            "handle_state_event: app: " + app_name +
                " reported FAILED when looking for state: " +
                get_warm_boot_stage_name(monitored_stage)};
  }

  if (state_names.find(new_state) != std::end(state_names)) {
    set_to_filter.erase(app_name);
  }

  Registration::Response response;
  response.status = set_to_filter.empty() ? Registration::Status::COMPLETED
                                          : Registration::Status::IN_PROCESS;

  return response;
}

std::string Registration::extract_event_state(
    const swss::KeyOpFieldsValuesTuple &kco) {
  for (const auto &field_value : kfvFieldsValues(kco)) {
    if (fvField(field_value) == "state") {
      return fvValue(field_value);
    }
  }
  return "";
}

void Registration::clear_all_init_apps() {
  swss::Table table(&m_db, STATE_WARM_RESTART_INIT_TABLE_NAME);
  std::vector<std::string> keys;
  table.getKeys(keys);
  for (auto &key : keys) {
    table.del(key);
  }
}

void Registration::save_all_init_apps() {
  SWSS_LOG_ENTER();
  std::ostringstream stream;
  std::copy(m_registered.begin(), m_registered.end(),
            std::ostream_iterator<std::string>(stream, ","));
  SWSS_LOG_NOTICE("Saving registered apps to init table: %s",
                  stream.str().c_str());

  swss::Table table(&m_db, STATE_WARM_RESTART_INIT_TABLE_NAME);
  std::string separator = swss::TableBase::getTableSeparator(m_db.getDbId());
  for (const auto &key : m_registered) {
    if (is_valid_key(key, separator)) {
      table.hset(key, "timestamp", swss::getTimestamp());
    } else {
      SWSS_LOG_ERROR("skipping saving key = %s", key.c_str());
    }
  }
}

std::string Registration::join_pending_apps(
    swss::WarmStart::WarmBootStage target_stage) {
  std::ostringstream stream;
  std::copy(m_remaining_apps.at(target_stage).begin(),
            m_remaining_apps.at(target_stage).end(),
            std::ostream_iterator<std::string>(stream, ","));
  return stream.str();
}

InitRegistration::InitRegistration()
    : m_db("STATE_DB", 0),
      m_separator(swss::TableBase::getTableSeparator(m_db.getDbId())) {}

void InitRegistration::fetch_init_app_info() {
  m_missing_registrations.clear();

  swss::Table table(&m_db, STATE_WARM_RESTART_INIT_TABLE_NAME);
  std::vector<std::string> keys;

  table.getKeys(keys);
  for (auto &key : keys) {
    if (!is_valid_key(key, m_separator)) {
      SWSS_LOG_ERROR("Could not parse init app name from key = %s",
                     key.c_str());
      continue;
    }

    m_missing_registrations.insert(key);
  }
}

InitRegistration::Status InitRegistration::get_reregistration_status() {
  return m_missing_registrations.empty()
             ? InitRegistration::Status::COMPLETED
             : InitRegistration::Status::IN_PROGRESS;
}

InitRegistration::Status InitRegistration::check_reregistration_status() {
  SWSS_LOG_ENTER();

  swss::Table table(&m_db, STATE_WARM_RESTART_REGISTRATION_TABLE_NAME);
  std::vector<std::string> keys;

  table.getKeys(keys);
  for (auto &key : keys) {
    remove_pending_app(key);
  }

  return get_reregistration_status();
}

InitRegistration::Status InitRegistration::handle_registration_event(
    const swss::KeyOpFieldsValuesTuple &kco) {
  remove_pending_app(kfvKey(kco));
  return get_reregistration_status();
}

const InitRegistration::RegistrationSet &InitRegistration::get_pending_apps()
    const {
  return m_missing_registrations;
}

void InitRegistration::remove_pending_app(const std::string &key) {
  SWSS_LOG_ENTER();
  if (!is_valid_key(key, m_separator)) {
    SWSS_LOG_ERROR("ignoring invalid key for reregistration = %s", key.c_str());
    return;
  }
  if (m_missing_registrations.find(key) != std::end(m_missing_registrations)) {
    m_missing_registrations.erase(key);
  }
}

std::string InitRegistration::join_pending_apps() {
  std::ostringstream stream;
  std::copy(m_missing_registrations.begin(), m_missing_registrations.end(),
            std::ostream_iterator<std::string>(stream, ","));
  return stream.str();
}

}  // namespace rebootbackend
