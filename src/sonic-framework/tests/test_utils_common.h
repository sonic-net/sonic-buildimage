#pragma once
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "dbconnector.h"
#include "notificationconsumer.h"
#include "select.h"
#include "selectableevent.h"

namespace rebootbackend {

class TestUtils {
 public:
  static void wait_for_finish(swss::Select &s, swss::SelectableEvent &finished,
                              long timeout_seconds);

  static std::string wait_for_state_verification_trigger(
      swss::NotificationConsumer &nc, long timeout_seconds, bool freeze);

  static void confirm_no_state_verification_trigger(
      swss::NotificationConsumer &nc, long timeout_seconds);

  static void populate_registration_table(
      swss::DBConnector &db, const std::string &key, const bool &stop_on_freeze,
      const bool &freeze, const bool &checkpoint, const bool &reconciliation);

  static void populate_restart_table_state(swss::DBConnector &db,
                                           const std::string &app_name,
                                           const std::string &state);

  static void write_state_verification_result(swss::DBConnector &db,
                                              const std::string &key,
                                              const std::string &status,
                                              const std::string &timestamp);

  static void clear_tables(swss::DBConnector &db);


};

}  // namespace rebootbackend
