#!/usr/bin/env python3
# {C} Copyright 2023 AMD Systems Inc. All rights reserved
#############################################################################
# Pensando
#############################################################################

import sys
import signal
import threading
import time
from datetime import datetime
import redis
import syslog
from sonic_py_common import daemon_base, logger, syslogger
import grpc
import queue

SYSLOG_IDENTIFIER = 'dpu-db-utild'
logger_instance = syslogger.SysLogger(SYSLOG_IDENTIFIER)

def log_info(msg, also_print_to_console=False):
    logger_instance.log_info(msg, also_print_to_console)

def log_err(msg, also_print_to_console=False):
    logger_instance.log_error(msg, also_print_to_console)

try:
    from sonic_py_common import daemon_base
    import sonic_platform
    from sonic_platform.chassis import Chassis
    from sonic_platform.helper import APIHelper
    from pathlib import Path
    grpc_files_path = (str(Path(sonic_platform.__file__).parent.absolute()))
    if grpc_files_path not in sys.path:
        sys.path.append(grpc_files_path)
    from sonic_platform import oper_pb2, oper_pb2_grpc
except Exception as e:
    log_err(f'failed to load modules due to {e}')

DPU_HEALTH_INFO_TABLE_NAME = 'DPU_STATE'
REDIS_CHASSIS_SERVER_PORT = 6380
REDIS_CHASSIS_SERVER_IP = '169.254.200.254'
CHASSIS_STATE_DB = 13
LOCALHOST = "127.0.0.1"
EVENT_PORT = 11360
PDS_PORT = 11357
NOT_AVAILABLE = 'N/A'
ERR_UNKNOWN = 1
SIGNALS_TO_NAMES_DICT = dict((getattr(signal, n), n) for n in dir(signal) if n.startswith('SIG') and '_' not in n)

# operd event functions
g_count = 0
CRITICAL_EVENTS = [
    2, 4, 6, 7, 11, 12, 15, 16, 17, 18, 19, 20, 4001, 4002
]
NETWORK_EVENTS = [
    1001,   # "Port link is up"
    1002    # "Port link is down"
]

# process handler

def get_slot_id(chassis):
        try:
            if chassis == None:
                chassis = Chassis()
            slot = chassis.get_my_slot()
        except Exception as e:
            log_err("failed to fetch chassis slot id due to {e}")
            slot = -1
        slot_id = str(slot) if slot != -1 else 'UNDEFINED'
        return slot_id

exit_code = ERR_UNKNOWN

class EventHandler(logger.Logger):

    def __init__(self, chassis, db):
        super(EventHandler, self).__init__(SYSLOG_IDENTIFIER)

        # operd attributes
        self.event_types = []
        self.event_types.extend(CRITICAL_EVENTS)
        self.event_types.extend(NETWORK_EVENTS)
        self.request_queue = queue.Queue()
        self.thread = None
        self.stop_event = threading.Event()

        # dpu state db update related attributes
        self.apiHelper = APIHelper()
        self.db = db
        self.chassis = chassis
        slot = get_slot_id(self.chassis)
        self.slot_id = str(slot) if slot != -1 else 'UNDEFINED'
        self.table = f'{DPU_HEALTH_INFO_TABLE_NAME}|DPU{self.slot_id}'

    def _log_emerg(self, msg):
        # syslogger.SysLogger doesn't have LOG_EMERG, hence creating one
        syslog.openlog(SYSLOG_IDENTIFIER)
        syslog.syslog(syslog.LOG_EMERG,msg)
        syslog.closelog()

    def _bool_to_link_status(self, status):
        if status:
            return "up"
        else:
            return "down"

    def _fetch_eth_link_status(self):
        try:
            cmd = "/nic/bin/pdsctl show interface --type uplink | grep -i uplink"
            output = self.apiHelper.run_docker_cmd(cmd)
            uplinks_info = output.split('\n')
            num_uplink = int(uplinks_info[-1].split(':')[-1].replace(' ',''))
            eth_link_reason = []
            state = True
            for i in range(num_uplink):
                uplink_info = uplinks_info[i].split()
                name = uplink_info[2]
                admin_status = uplink_info[3]
                oper_status = uplink_info[4]
                eth_state = False
                if admin_status == 'UP' and oper_status == 'UP':
                    eth_state = True
                state &= eth_state
                eth_link_reason.append(f"{name} is {self._bool_to_link_status(eth_state)}")
            return state, "host-ethlink-status: " + ', '.join(eth_link_reason)
        except Exception as e:
            log_err(f'failed to fetch eth uplink status due to {e}')
            return False, "Failed to fetch host eth link status"

    def _update_dpu_control_plane_db(self):
        try:
            control_plane_status = self.db.hget(self.table, 'dpu_control_plane_state')
            control_plane_reason = self.db.hget(self.table, 'dpu_control_plane_reason')
            if (control_plane_status == None) or (control_plane_status == 'up'):
                control_plane_status = True
            else:
                control_plane_status = False
            all_container_reason = control_plane_reason.replace(', host-ethlink-status:', 'host-ethlink-status:').split('host-ethlink-status:')[0]
            eth_link_status, eth_link_reason = self._fetch_eth_link_status()
            if control_plane_status:
                control_plane_status &= eth_link_status
            else:
                if ('All containers are up and running' in control_plane_reason):
                    control_plane_status = eth_link_status

            control_plane_reason = []
            if all_container_reason != '':
                control_plane_reason.append(all_container_reason)
            control_plane_reason.append(eth_link_reason)
            fvs_control_plane_data = [
                ('dpu_control_plane_state', self._bool_to_link_status(control_plane_status)),
                ('dpu_control_plane_time', datetime.now().strftime("%a %b %d %I:%M:%S %p UTC %Y")),
                ('dpu_control_plane_reason', ', '.join(control_plane_reason))
            ]
            for name, value in fvs_control_plane_data:
                self.db.hset(self.table, name, value)
        except Exception as e:
            log_err(f'Failed to populate dpu control plane entries due to {e}')

    def _process_event(self, event):
        global g_count

        try:
            event_type = event.Type
            event_description = event.Description
            event_message = event.Message
        except Exception as e:
            log_err(f"Failed to process event due to {e}")
            return

        try:
            if event_type in CRITICAL_EVENTS:
                # log event
                g_count += 1
                self._log_emerg(f"Event # {g_count} Time -> {time.asctime()}")
                self._log_emerg(f"Received event {event}")

                # update dpu state db for data plane
                if self.db == None:
                    return
                dpu_data_plane_status = 'down'
                dpu_data_plane_reason = event_description
                fvs_data_plane_data = [
                    ('dpu_data_plane_state', dpu_data_plane_status),
                    ('dpu_data_plane_time', datetime.now().strftime("%a %b %d %I:%M:%S %p UTC %Y")),
                    ('dpu_data_plane_reason', dpu_data_plane_reason)
                ]
                for name, value in fvs_data_plane_data:
                    self.db.hset(self.table, name, value)
        except Exception as e:
            log_err(f"Failed to update dpu state db data plane fields due to {e}")

        try:
            if event_type in NETWORK_EVENTS:
                # log event
                g_count += 1
                log_info(f"Event # {g_count} Time -> {time.asctime()}")
                log_info(f"Received event {event}")

                # update dpu state db for control plane
                if self.db == None:
                    return
                self._update_dpu_control_plane_db()
        except Exception as e:
            log_err(f"Failed to update dpu state db control plane fields due to {e}")

    def _build_event_request(self):
        """Builds a single gRPC subscription request."""
        request = oper_pb2.OperInfoRequest()
        spec = request.Request.add()
        spec.InfoType = oper_pb2.OPER_INFO_TYPE_EVENT
        spec.Action = oper_pb2.OPER_INFO_OP_SUBSCRIBE
        for t in self.event_types:
            spec.EventFilter.Types.append(t)
        return request

    def _request_generator(self):
        """Generator that yields the gRPC request once."""
        yield self._build_event_request()
        while not self.stop_event.is_set():
            time.sleep(1)

    def _event_listener(self):
        channel_addr = f"{LOCALHOST}:{EVENT_PORT}"
        log_info(f"Connecting to gRPC server at {channel_addr}")
        channel = grpc.insecure_channel(channel_addr)
        stub = oper_pb2_grpc.OperSvcStub(channel)

        try:
            response_stream = stub.OperInfoSubscribe(self._request_generator())
            for response in response_stream:
                if self.stop_event.is_set():
                    break
                if response.Status != 0:
                    log_info(f"Received non-OK status: {response.Status}")
                    continue
                event = response.EventInfo
                try:
                    self._process_event(event)
                except Exception as e:
                    log_err(f"_process_event() raised exception: {e}")
        except grpc.RpcError as e:
            log_err(f"gRPC error: {e}")
        finally:
            log_info("Event listener thread exiting")

    def start(self):
        if self.thread is None or not self.thread.is_alive():
            self.stop_event.clear()
            self.thread = threading.Thread(target=self._event_listener, daemon=True)
            self.thread.start()
            log_info("Event listener thread started")

    def stop(self):
        log_info("Stopping event listener thread")
        self.stop_event.set()
        if self.thread is not None:
            self.thread.join(timeout=2)
            log_info("Event listener thread stopped")
            self.thread = None

#
# Daemon =======================================================================
#
class DpuDBUtilDaemon(daemon_base.DaemonBase):
    # Interval to update CHASSIS_STATE_DB
    INTERVAL = 60

    def __init__(self):
        """
        Initializer of DpuDBUtilDaemon
        """
        super(DpuDBUtilDaemon, self).__init__(SYSLOG_IDENTIFIER)

        # Set minimum logging level to INFO
        self.set_min_log_priority_info()

        self.stop_event = threading.Event()

        self.wait_time = self.INTERVAL

        self.chassis = Chassis()
        self.db = None
        try:
            self.db = redis.Redis(
                host=REDIS_CHASSIS_SERVER_IP,
                port=REDIS_CHASSIS_SERVER_PORT,
                decode_responses=True,
                db=CHASSIS_STATE_DB)
            # checking if server is accessible
            self.db.ping()
        except Exception as e:
            self.db = None
            log_err(f'Failed to connect to db due to {e}')
            pass
        if self.db == None:
            return
        self.event_handler = EventHandler(self.chassis, self.db)
        self.event_handler.start()

    def deinit(self):
        """
        Deinitializer of DpuDBUtilDaemon
        """
        if self.db == None:
            return
        self.event_handler.stop()

    # Override signal handler from DaemonBase
    def signal_handler(self, sig, frame):
        """
        Signal handler
        :param sig: Signal number
        :param frame: not used
        :return:
        """
        FATAL_SIGNALS = [signal.SIGINT, signal.SIGTERM]
        NONFATAL_SIGNALS = [signal.SIGHUP]

        global exit_code

        if sig in FATAL_SIGNALS:
            log_info("Caught signal '{}' - exiting...".format(SIGNALS_TO_NAMES_DICT[sig]))
            exit_code = 128 + sig  # Make sure we exit with a non-zero code so that supervisor will try to restart us
            if self.db != None:
                self.event_handler.stop()
            self.stop_event.set()
        elif sig in NONFATAL_SIGNALS:
            log_info("Caught signal '{}' - ignoring...".format(SIGNALS_TO_NAMES_DICT[sig]))
        else:
            log_err("Caught unhandled signal '{}' - ignoring...".format(SIGNALS_TO_NAMES_DICT[sig]))

    # Main daemon logic
    def run(self):
        """
        Run main logical of this daemon
        :return:
        """
        if self.db == None:
            return False
        if self.stop_event.wait(self.wait_time):
            # We received a fatal signal
            return False
        return True


#
# Main =========================================================================
#
def main():
    dpudbd = DpuDBUtilDaemon()

    dpudbd.log_info("Starting up...")

    while dpudbd.run():
        pass

    dpudbd.log_info("Shutting down DpuDBUtilDaemon with exit code {}...".format(exit_code))

    dpudbd.deinit()
    return exit_code


if __name__ == '__main__':
    sys.exit(main())
