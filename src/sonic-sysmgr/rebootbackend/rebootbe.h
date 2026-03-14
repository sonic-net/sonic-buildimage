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

constexpr char REBOOT_REQUEST_NOTIFICATION_CHANNEL[] = "Reboot_Request_Channel";
constexpr char REBOOT_RESPONSE_NOTIFICATION_CHANNEL[] =
    "Reboot_Response_Channel";
constexpr char REBOOT_KEY[] = "Reboot";
constexpr char REBOOT_STATUS_KEY[] = "RebootStatus";
constexpr char CANCEL_REBOOT_KEY[] = "CancelReboot";
constexpr char DATA_TUPLE_KEY[] = "MESSAGE";

class RebootBE {
 public:
  enum class RebManagerStatus {
    IDLE,
    COLD_REBOOT_IN_PROGRESS,
    HALT_REBOOT_IN_PROGRESS,
    WARM_REBOOT_IN_PROGRESS
  };

  struct NotificationRequest {
    std::string op;
    std::string retString;
  };

  RebootBE(DbusInterface &interface);

  // To get the current reboot status.
  RebManagerStatus GetCurrentStatus();

  // Checks for the notification and takes appropriate action.
  void Start();

  // Notifies completion status of reboot.
  void Stop();

 private:
  std::mutex m_StatusMutex;
  RebManagerStatus m_CurrentStatus = RebManagerStatus::IDLE;
  swss::SelectableEvent m_Done;

  swss::DBConnector m_db;
  swss::NotificationProducer m_RebootResponse;
  swss::NotificationConsumer m_NotificationConsumer;

  DbusInterface &m_dbus;

  // Signalled by reboot thread when thread completes.
  swss::SelectableEvent m_RebootThreadFinished;
  RebootThread m_RebootThread;

  void SetCurrentStatus(RebManagerStatus newStatus);

  // Reboot_Request_Channel notifications should all contain {"MESSAGE" : Data}
  // in the notification Data field.
  // Return true if "MESSAGE" is found, false otherwise.
  // Set message_value to the Data string if found, "" otherwise.
  // consumer is input: this is the consumer from which we pop
  // reboot/cancel/status requests.
  // request is output: this the request recevied from consumer
  bool RetrieveNotificationData(swss::NotificationConsumer &consumer,
                                NotificationRequest &request);
  NotificationResponse RequestRebootStatus(
      const std::string &jsonStatusRequest);
  NotificationResponse HandleRebootRequest(
      const std::string &jsonRebootRequest);
  NotificationResponse HandleStatusRequest(
      const std::string &jsonStatusRequest);
  NotificationResponse HandleCancelRequest(
      const std::string &jsonCancelRequest);
  void SendNotificationResponse(const std::string key,
                                const swss::StatusCode code,
                                const std::string message);

  // Returns true if a reboot is allowed at this time given the current
  // warm manager state and reboot type, and false otherwise.
  bool RebootAllowed(const gnoi::system::RebootMethod rebMethod);

  void DoTask(swss::NotificationConsumer &consumer);

  void HandleRebootFinish();
  void HandleDone();

  friend class RebootBETestWithoutStop;
};

}  // namespace rebootbackend
