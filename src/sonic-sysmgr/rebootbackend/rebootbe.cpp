#include "rebootbe.h"

#include <google/protobuf/util/json_util.h>
#include <unistd.h>

#include <memory>
#include <mutex>
#include <string>

#include "logger.h"
#include "notificationconsumer.h"
#include "notificationproducer.h"
#include "reboot_common.h"
#include "reboot_interfaces.h"
#include "select.h"
#include "status_code_util.h"
#include "warm_restart.h"

namespace rebootbackend {

namespace gpu = ::google::protobuf::util;

RebootBE::RebootBE(DbusInterface &dbus_interface)
    : m_db("STATE_DB", 0),
      m_RebootResponse(&m_db, REBOOT_RESPONSE_NOTIFICATION_CHANNEL),
      m_NotificationConsumer(&m_db, REBOOT_REQUEST_NOTIFICATION_CHANNEL),
      m_dbus(dbus_interface),
      m_RebootThread(dbus_interface, m_RebootThreadFinished) {
  swss::Logger::linkToDbNative("rebootbackend");
}

RebootBE::RebManagerStatus RebootBE::GetCurrentStatus() {
  return m_CurrentStatus;
}

void RebootBE::SetCurrentStatus(RebManagerStatus newStatus) {
  const std::lock_guard<std::mutex> lock(m_StatusMutex);
  m_CurrentStatus = newStatus;
}

void RebootBE::Start() {
  SWSS_LOG_ENTER();
  SWSS_LOG_NOTICE("--- Starting rebootbackend ---");
  swss::WarmStart::initialize("rebootbackend", "sonic-sysmgr");
  swss::WarmStart::checkWarmStart("rebootbackend", "sonic-sysmgr",
                                  /*incr_restore_cnt=*/false);

  swss::Select s;
  s.addSelectable(&m_NotificationConsumer);
  s.addSelectable(&m_Done);
  s.addSelectable(&m_RebootThreadFinished);

  SWSS_LOG_NOTICE("RebootBE entering operational loop");
  while (true) {
    swss::Selectable *sel;
    int ret;

    ret = s.select(&sel);
    if (ret == swss::Select::ERROR) {
      SWSS_LOG_NOTICE("Error: %s!", strerror(errno));
    } else if (ret == swss::Select::OBJECT) {
      if (sel == &m_NotificationConsumer) {
        DoTask(m_NotificationConsumer);
      } else if (sel == &m_RebootThreadFinished) {
        HandleRebootFinish();
      } else if (sel == &m_Done) {
        HandleDone();
        break;
      }
    }
  }
  return;
}

void RebootBE::Stop() {
  SWSS_LOG_ENTER();
  m_Done.notify();
  return;
}

bool RebootBE::RetrieveNotificationData(
    swss::NotificationConsumer &consumer,
    RebootBE::NotificationRequest &request) {
  SWSS_LOG_ENTER();

  request.op = "";
  request.retString = "";

  std::string data;
  std::vector<swss::FieldValueTuple> values;
  consumer.pop(request.op, data, values);

  for (auto &fv : values) {
    if (DATA_TUPLE_KEY == fvField(fv)) {
      request.retString = fvValue(fv);
      return true;
    }
  }
  return false;
}

// Send a response on the Reboot_Response_Channel notification channel..
//   Key is one of: Reboot, RebootStatus, or CancelReboot
//   code is swss::StatusCode, hopefully SWSS_RC_SUCCESS.
//   message is json formatted RebootResponse, RebootStatusResponse
//     or CancelRebootResponse as defined in system.proto
void RebootBE::SendNotificationResponse(const std::string key,
                                        const swss::StatusCode code,
                                        const std::string message) {
  SWSS_LOG_ENTER();

  std::vector<swss::FieldValueTuple> ret_values;
  ret_values.push_back(swss::FieldValueTuple(DATA_TUPLE_KEY, message));

  m_RebootResponse.send(key, swss::statusCodeToStr(code), ret_values);
}

NotificationResponse RebootBE::RequestRebootStatus(
    const std::string &jsonStatusRequest) {
  SWSS_LOG_ENTER();
  SWSS_LOG_NOTICE("Sending reboot status request to platform");

  NotificationResponse response = {.status = swss::StatusCode::SWSS_RC_SUCCESS,
                   .json_string = "{}"};

  // Send a request to the reboot host service via dbus.
  DbusInterface::DbusResponse dbus_response =
      m_dbus.RebootStatus(jsonStatusRequest);

  if (dbus_response.status == DbusInterface::DbusStatus::DBUS_FAIL) {
    SWSS_LOG_ERROR("Failed to send reboot status request to platform: %s",
                    dbus_response.json_string.c_str());
    response.status = swss::StatusCode::SWSS_RC_INTERNAL;
    return response;
  }

  response.json_string = dbus_response.json_string;
  SWSS_LOG_NOTICE("Received reboot status response from platform: %s",
                  response.json_string.c_str());
  return response;
}

NotificationResponse RebootBE::HandleRebootRequest(
    const std::string &jsonRebootRequest) {
  using namespace gpu;

  SWSS_LOG_ENTER();

  // On success an emtpy string is returned. RebootResponse in system.proto
  // is an empty proto.
  NotificationResponse response = {.status = swss::StatusCode::SWSS_RC_SUCCESS,
                                   .json_string = ""};

  gnoi::system::RebootRequest request;
  Status status = gpu::JsonStringToMessage(jsonRebootRequest, &request);

  if (!status.ok()) {
    std::string error_string =
        "unable to convert json to rebootRequest protobuf: " +
        status.message().as_string();
    SWSS_LOG_ERROR("%s", error_string.c_str());
    SWSS_LOG_ERROR("json = |%s|", jsonRebootRequest.c_str());
    response.status = swss::StatusCode::SWSS_RC_INTERNAL,
    response.json_string = error_string;
    return response;
  }

  if (!RebootAllowed(request.method())) {
    response.status = swss::StatusCode::SWSS_RC_IN_USE;
    response.json_string =
        "Reboot not allowed at this time. Reboot, halt or "
        "post-warmboot in progress";
    SWSS_LOG_WARN("%s", response.json_string.c_str());
    return response;
  }

  SWSS_LOG_NOTICE("Forwarding request to RebootThread: %s",
                  request.DebugString().c_str());
  response = m_RebootThread.Start(request);
  if (response.status == swss::StatusCode::SWSS_RC_SUCCESS) {
    if (request.method() == gnoi::system::RebootMethod::COLD) {
      SetCurrentStatus(RebManagerStatus::COLD_REBOOT_IN_PROGRESS);
    } else if (request.method() == gnoi::system::RebootMethod::HALT) {
      SetCurrentStatus(RebManagerStatus::HALT_REBOOT_IN_PROGRESS);
    } else if (request.method() == gnoi::system::RebootMethod::WARM) {
      SetCurrentStatus(RebManagerStatus::WARM_REBOOT_IN_PROGRESS);
    }
  }
  return response;
}

bool RebootBE::RebootAllowed(const gnoi::system::RebootMethod rebMethod) {
  RebManagerStatus current_status = GetCurrentStatus();
  switch (current_status) {
    case RebManagerStatus::COLD_REBOOT_IN_PROGRESS:
    case RebManagerStatus::HALT_REBOOT_IN_PROGRESS:
    case RebManagerStatus::WARM_REBOOT_IN_PROGRESS: {
      return false;
    }
    case RebManagerStatus::IDLE: {
      return true;
    }
    default: {
      return true;
    }
  }
}

NotificationResponse RebootBE::HandleStatusRequest(
    const std::string &jsonStatusRequest) {
  SWSS_LOG_ENTER();

  //For Halt reboot, we need to send the status request to the platform
  if (m_CurrentStatus == RebManagerStatus::HALT_REBOOT_IN_PROGRESS) {
    return RequestRebootStatus(jsonStatusRequest);
  }

  gnoi::system::RebootStatusResponse reboot_response =
      m_RebootThread.GetResponse();

  std::string json_reboot_response_string;
  google::protobuf::util::Status status =
      gpu::MessageToJsonString(reboot_response, &json_reboot_response_string);

  NotificationResponse response;
  if (status.ok()) {
    response.status = swss::StatusCode::SWSS_RC_SUCCESS;
    response.json_string = json_reboot_response_string;
  } else {
    std::string error_string =
        "unable to convert reboot status response protobuf to json: " +
        status.message().as_string();
    SWSS_LOG_ERROR("%s", error_string.c_str());
    response.status = swss::StatusCode::SWSS_RC_INTERNAL;
    response.json_string = error_string;
  }

  return response;
}

NotificationResponse RebootBE::HandleCancelRequest(
    const std::string &jsonCancelRequest) {
  SWSS_LOG_ENTER();

  NotificationResponse response;

  // CancelReboot isn't supported: not needed until/unless delayed support
  // is added: return unimplemented.
  response.status = swss::StatusCode::SWSS_RC_UNIMPLEMENTED;
  response.json_string = "Cancel reboot isn't supported";
  SWSS_LOG_WARN("%s", response.json_string.c_str());
  return response;
}

void RebootBE::DoTask(swss::NotificationConsumer &consumer) {
  SWSS_LOG_ENTER();

  NotificationResponse response;
  RebootBE::NotificationRequest request;

  if (!RetrieveNotificationData(consumer, request)) {
    // Response is simple string (not json) on error.
    response.json_string =
        "MESSAGE not present in reboot notification request message, op = " +
        request.op;
    SWSS_LOG_ERROR("%s", response.json_string.c_str());
    response.status = swss::StatusCode::SWSS_RC_INVALID_PARAM;
  } else if (request.op == REBOOT_KEY) {
    response = HandleRebootRequest(request.retString);
  } else if (request.op == REBOOT_STATUS_KEY) {
    response = HandleStatusRequest(request.retString);
  } else if (request.op == CANCEL_REBOOT_KEY) {
    response = HandleCancelRequest(request.retString);
  } else {
    // Response is simple string (not json) on error.
    response.json_string =
        "Unrecognized op in reboot request, op = " + request.op;
    SWSS_LOG_ERROR("%s", response.json_string.c_str());
    response.status = swss::StatusCode::SWSS_RC_INVALID_PARAM;
  }
  SendNotificationResponse(request.op, response.status, response.json_string);
}

void RebootBE::HandleRebootFinish() {
  SWSS_LOG_ENTER();
  SWSS_LOG_WARN(
      "Receieved notification that reboot has finished. This probably means "
      "something is wrong");
  m_RebootThread.Join();
  SetCurrentStatus(RebManagerStatus::IDLE);
}

void RebootBE::HandleDone() {
  SWSS_LOG_INFO("RebootBE received signal to stop");

  if (m_RebootThread.GetResponse().active()) {
    m_RebootThread.Stop();
    m_RebootThread.Join();
  }
}

}  // namespace rebootbackend
