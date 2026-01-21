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

void init_warm_reboot_states(const swss::DBConnector &db);

// Set the system warm start state to a new enabled/disabled state.
// STATE_WARM_RESTART_TABLE_NAME
//   key = system, field = enable, value = "true"/"false"
void set_warm_restart_enable(const swss::DBConnector &db, bool enabled);

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

}  // namespace rebootbackend
