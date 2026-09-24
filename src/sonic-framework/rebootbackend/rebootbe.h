#pragma once
#include "dbconnector.h"
#include "notificationconsumer.h"
#include "notificationproducer.h"
#include "reboot_common.h"
#include "reboot_interfaces.h"
#include "reboot_thread.h"
#include "selectableevent.h"
#include "status_code_util.h"

namespace rebootbackend {

#define REBOOT_REQUEST_NOTIFICATION_CHANNEL "Reboot_Request_Channel"
#define REBOOT_RESPONSE_NOTIFICATION_CHANNEL "Reboot_Response_Channel"
#define REBOOT_KEY "Reboot"
#define REBOOT_STATUS_KEY "RebootStatus"
#define CANCEL_REBOOT_KEY "CancelReboot"
#define DATA_TUPLE_KEY "MESSAGE"

class RebootBE {
 public:
  struct NotificationRequest {
    std::string op;
    std::string ret_string;
  };

  enum class RebManagerStatus {
    WARM_INIT_WAIT,
    IDLE,
    COLD_REBOOT_IN_PROGRESS,
    WARM_REBOOT_IN_PROGRESS
  };

  RebootBE(DbusInterface &interface);

  RebManagerStatus GetCurrentStatus();

  void Start();
  void Stop();

 private:
  std::mutex m_status_mutex;
  RebManagerStatus m_current_status = RebManagerStatus::IDLE;
  swss::SelectableEvent m_done;

  swss::DBConnector m_db;
  swss::NotificationProducer m_rebootResponse;
  swss::NotificationConsumer m_notificationConsumer;

  DbusInterface &m_dbus;

  // Signalled by reboot thread when thread completes.
  swss::SelectableEvent m_reboot_thread_finished;
  RebootThread m_reboot_thread;

  void SetCurrentStatus(RebManagerStatus new_status);

  // Reboot_Request_Channel notifications should all contain {"MESSAGE" : Data}
  // in the notification Data field.
  // Return true if "MESSAGE" is found, false otherwise.
  // Set message_value to the Data string if found, "" otherwise.
  // consumer is input: this is the consumer from which we pop
  // reboot/cancel/status requests.
  // request is output: this the request recevied from consumer
  bool retrieve_notification_data(swss::NotificationConsumer &consumer,
                                  NotificationRequest &request);
  NotificationResponse handle_reboot_request(
      const std::string &json_reboot_request);
  NotificationResponse handle_status_request(
      const std::string &json_status_request);
  NotificationResponse handle_cancel_request(
      const std::string &json_cancel_request);
  void send_notification_response(const std::string key,
                                  const swss::StatusCode code,
                                  const std::string message);

  // Returns true if a reboot is allowed at this time given the current 
  // warm manager state and reboot type, and false otherwise.
  bool reboot_allowed(const gnoi::system::RebootMethod reboot_method);

  void do_task(swss::NotificationConsumer &consumer);

  void handle_init_finish();
  void handle_reboot_finish();
  void handle_done();

  friend class RebootBETestWithoutStop;
};

}  // namespace rebootbackend
