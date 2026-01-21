#include "redis_utils.h"

#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "dbconnector.h"
#include "notificationproducer.h"
//#include "stateverification.h"
#include "table.h"
#include "timestamp.h"
#include "warm_restart.h"

namespace rebootbackend {

using WarmStartState = ::swss::WarmStart::WarmStartState;


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

}  // namespace rebootbackend
