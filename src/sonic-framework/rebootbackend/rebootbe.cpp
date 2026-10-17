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

namespace rebootbackend {

namespace gpu = ::google::protobuf::util;

bool sigterm_requested = false;

RebootBE::RebootBE(DbusInterface &dbus_interface)
    : m_db("STATE_DB", 0),
      m_rebootResponse(&m_db, REBOOT_RESPONSE_NOTIFICATION_CHANNEL),
      m_notificationConsumer(&m_db, REBOOT_REQUEST_NOTIFICATION_CHANNEL),
      m_dbus(dbus_interface),
      m_reboot_thread(dbus_interface, 
                      m_reboot_thread_finished) {
  swss::Logger::linkToDbNative("rebootbackend");
}

RebootBE::RebManagerStatus RebootBE::GetCurrentStatus() {
  const std::lock_guard<std::mutex> lock(m_status_mutex);
  return m_current_status;
}

void RebootBE::SetCurrentStatus(RebManagerStatus new_status) {
  const std::lock_guard<std::mutex> lock(m_status_mutex);
  m_current_status = new_status;
}

void RebootBE::Start() {
  SWSS_LOG_ENTER();
  SWSS_LOG_NOTICE("--- Starting rebootbackend ---");

  swss::WarmStart::initialize("rebootbackend", "sonic-framework");
  swss::WarmStart::checkWarmStart("rebootbackend", "sonic-framework",
                                  /*incr_restore_cnt=*/false);

  swss::Select s;
  s.addSelectable(&m_notificationConsumer);
  s.addSelectable(&m_done);
  s.addSelectable(&m_reboot_thread_finished);

  SWSS_LOG_NOTICE("RebootBE entering operational loop");
  while (true) {
    swss::Selectable *sel;
    int ret;

    ret = s.select(&sel);
    if (ret == swss::Select::ERROR) {
      SWSS_LOG_NOTICE("Error: %s!", strerror(errno));
    } else if (ret == swss::Select::OBJECT) {
      if (sel == &m_notificationConsumer) {
        do_task(m_notificationConsumer);
      } else if (sel == &m_reboot_thread_finished) {
        handle_reboot_finish();
      } else if (sel == &m_done) {
        handle_done();
        break;
      }
    }
  }
  return;
}

void RebootBE::Stop() {
  SWSS_LOG_ENTER();
  m_done.notify();
  return;
}

bool RebootBE::retrieve_notification_data(
    swss::NotificationConsumer &consumer,
    RebootBE::NotificationRequest &request) {
  SWSS_LOG_ENTER();

  request.op = "";
  request.ret_string = "";

  std::string data;
  std::vector<swss::FieldValueTuple> values;
  consumer.pop(request.op, data, values);

  for (auto &fv : values) {
    if (DATA_TUPLE_KEY == fvField(fv)) {
      request.ret_string = fvValue(fv);
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
void RebootBE::send_notification_response(const std::string key,
                                          const swss::StatusCode code,
                                          const std::string message) {
  SWSS_LOG_ENTER();

  std::vector<swss::FieldValueTuple> ret_values;
  ret_values.push_back(swss::FieldValueTuple(DATA_TUPLE_KEY, message));

  m_rebootResponse.send(key, swss::statusCodeToStr(code), ret_values);
}

NotificationResponse RebootBE::handle_reboot_request(
    const std::string &json_reboot_request) {
  using namespace google::protobuf::util;

  SWSS_LOG_ENTER();

  // On success an emtpy string is returned. RebootResponse in system.proto
  // is an empty proto.
  NotificationResponse response = {.status = swss::StatusCode::SWSS_RC_SUCCESS,
                                   .json_string = ""};

  gnoi::system::RebootRequest request;
  Status status = gpu::JsonStringToMessage(json_reboot_request, &request);

  if (!status.ok()) {
    std::string error_string =
        "unable to convert json to rebootRequest protobuf: " +
        status.message().as_string();
    SWSS_LOG_ERROR("%s", error_string.c_str());
    SWSS_LOG_ERROR("json = |%s|", json_reboot_request.c_str());
    response.status = swss::StatusCode::SWSS_RC_INTERNAL,
    response.json_string = error_string;
    return response;
  }

  if (!reboot_allowed(request.method())) {
    response.status = swss::StatusCode::SWSS_RC_IN_USE;
    response.json_string =
        "Reboot not allowed at this time. Reboot or "
        "post-warmboot in progress";
    SWSS_LOG_WARN("%s", response.json_string.c_str());
    return response;
  }

  SWSS_LOG_NOTICE("Forwarding request to RebootThread: %s",
                  request.DebugString().c_str());
  response = m_reboot_thread.Start(request);
  if (response.status == swss::StatusCode::SWSS_RC_SUCCESS) {
    if (request.method() == gnoi::system::RebootMethod::COLD) {
      SetCurrentStatus(RebManagerStatus::COLD_REBOOT_IN_PROGRESS);
    } else if (request.method() == gnoi::system::RebootMethod::WARM) {
      SetCurrentStatus(RebManagerStatus::WARM_REBOOT_IN_PROGRESS);
    }
  }
  return response;
}

bool RebootBE::reboot_allowed(const gnoi::system::RebootMethod reboot_method) {
  RebManagerStatus current_status = GetCurrentStatus();
  switch (current_status) {
    case RebManagerStatus::COLD_REBOOT_IN_PROGRESS:
    case RebManagerStatus::WARM_REBOOT_IN_PROGRESS: {
      return false;
    }
    case RebManagerStatus::WARM_INIT_WAIT: {
      return reboot_method == gnoi::system::RebootMethod::COLD;
    }
    case RebManagerStatus::IDLE: {
      return true;
    }
    default: {
      return true;
    }
  }
}

NotificationResponse RebootBE::handle_status_request(
    const std::string &json_status_request) {
  SWSS_LOG_ENTER();

  gnoi::system::RebootStatusResponse reboot_response =
       			m_reboot_thread.GetResponse();
                          
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

NotificationResponse RebootBE::handle_cancel_request(
    const std::string &json_cancel_request) {
  SWSS_LOG_ENTER();

  NotificationResponse response;

  // CancelReboot isn't supported: not needed until/unless delayed support
  // is added: return unimplemented.
  response.status = swss::StatusCode::SWSS_RC_UNIMPLEMENTED;
  response.json_string = "Cancel reboot isn't supported";
  SWSS_LOG_WARN("%s", response.json_string.c_str());
  return response;
}

void RebootBE::do_task(swss::NotificationConsumer &consumer) {
  SWSS_LOG_ENTER();

  NotificationResponse response;
  RebootBE::NotificationRequest request;

  if (!retrieve_notification_data(consumer, request)) {
    // Response is simple string (not json) on error.
    response.json_string =
        "MESSAGE not present in reboot notification request message, op = " +
        request.op;
    SWSS_LOG_ERROR("%s", response.json_string.c_str());
    response.status = swss::StatusCode::SWSS_RC_INVALID_PARAM;
  } else if (request.op == REBOOT_KEY) {
    response = handle_reboot_request(request.ret_string);
  } else if (request.op == REBOOT_STATUS_KEY) {
    response = handle_status_request(request.ret_string);
  } else if (request.op == CANCEL_REBOOT_KEY) {
    response = handle_cancel_request(request.ret_string);
  } else {
    // Response is simple string (not json) on error.
    response.json_string =
        "Unrecognized op in reboot request, op = " + request.op;
    SWSS_LOG_ERROR("%s", response.json_string.c_str());
    response.status = swss::StatusCode::SWSS_RC_INVALID_PARAM;
  }
  send_notification_response(request.op, response.status, response.json_string);
}

void RebootBE::handle_init_finish() {
  SWSS_LOG_ENTER();
  SWSS_LOG_NOTICE("Receieved notification that InitThread is done");
  RebManagerStatus current_status = GetCurrentStatus();
  if (current_status == RebManagerStatus::WARM_INIT_WAIT) {
    SetCurrentStatus(RebManagerStatus::IDLE);
  }
}

void RebootBE::handle_reboot_finish() {
  SWSS_LOG_ENTER();
  SWSS_LOG_WARN(
      "Receieved notification that reboot has finished. This probably means "
      "something is wrong");
  m_reboot_thread.Join();
  SetCurrentStatus(RebManagerStatus::IDLE);
}

void RebootBE::handle_done() {
  SWSS_LOG_INFO("RebootBE received signal to stop");

  if (m_reboot_thread.GetResponse().active()) {
    m_reboot_thread.Stop();
    m_reboot_thread.Join();
  }
}

}  // namespace rebootbackend
