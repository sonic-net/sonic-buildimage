#!/usr/bin/env python3
# {C} Copyright 2023 AMD Systems Inc. All rights reserved
#############################################################################
# Pensando
#############################################################################

import sys
import signal
import subprocess
import threading
import time
from datetime import datetime
import json
import docker
import redis
import syslog
from sonic_py_common import daemon_base, logger, syslogger
from swsscommon.swsscommon import SonicV2Connector
from sonic_py_common.task_base import ProcessTaskBase
import multiprocessing
import grpc
from concurrent import futures
from google.protobuf.empty_pb2 import Empty

SYSLOG_IDENTIFIER = 'dpu-db-utild'
logger_instance = syslogger.SysLogger(SYSLOG_IDENTIFIER)

def log_info(msg, also_print_to_console=False):
    logger_instance.log_info(msg, also_print_to_console)

def log_err(msg, also_print_to_console=False):
    logger_instance.log_error(msg, also_print_to_console)

try:
    from health_checker.manager import HealthCheckerManager
    from sonic_py_common import daemon_base
    import sonic_platform
    from sonic_platform.chassis import Chassis
    from sonic_platform.helper import APIHelper
    from pathlib import Path
    grpc_files_path = (str(Path(sonic_platform.__file__).parent.absolute()))
    if grpc_files_path not in sys.path:
        sys.path.append(grpc_files_path)
    from sonic_platform import oper_pb2, oper_pb2_grpc, system_pb2, system_pb2_grpc, dpu_util_pb2, dpu_util_pb2_grpc
except Exception as e:
    log_err(f'failed to load modules due to {e}')

DPU_HEALTH_INFO_TABLE_NAME = 'DPU_STATE'
REBOOT_CAUSE_INFO_TABLE_NAME = 'REBOOT_CAUSE'
VERSION_UPDATE_TABLE_NAME = 'PLATFORM|VERSION|CURRENT'
REDIS_CHASSIS_SERVER_PORT = 6380
REDIS_CHASSIS_SERVER_IP = '169.254.200.254'
CHASSIS_STATE_DB = 13
LOCALHOST = "127.0.0.1"
EVENT_PORT = 11360
PDS_PORT = 11357
NOT_AVAILABLE = 'N/A'
ERR_UNKNOWN = 1
IP = "0.0.0.0"
PORT = 12000
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
        self.events = []
        self.events.extend(CRITICAL_EVENTS)
        self.events.extend(NETWORK_EVENTS)
        self.event_thread = None
        self.event_stop = False

        # dpu state db update related attributes
        try:
            self.apiHelper = APIHelper()
            self.dpu_docker_name = self.apiHelper.get_dpu_docker_container_name()
        except:
            log_err('Failed to get dpu docker name')
        self.db = db
        self.chassis = chassis
        slot = get_slot_id(self.chassis)
        self.slot_id = str(slot) if slot != -1 else 'UNDEFINED'
        self.table = f'{DPU_HEALTH_INFO_TABLE_NAME}|DPU{self.slot_id}'
        self._update_dpu_date_plane_db()

    def _log_emerg(self, msg):
        # syslogger.SysLogger doesn't have LOG_EMERG, hence creating one
        syslog.openlog(SYSLOG_IDENTIFIER)
        syslog.syslog(syslog.LOG_EMERG,msg)
        syslog.closelog()

    def _bool_to_healthd_status(self, status):
        if status:
            return "OK"
        else:
            return "Not OK"

    def bool_to_link_status(self, status):
        if status:
            return "UP"
        else:
            return "DOWN"

    def _is_dpu_container_running(self):
        try:
            client = docker.from_env()
            containers = client.containers.list(all=True)
            dpu_docker_status = False
            for container in containers:
                container_name = container.name
                container_status = container.status
                if container_name == self.dpu_docker_name:
                    dpu_docker_status = True if container_status == 'running' else False
            return dpu_docker_status
        except Exception as e:
            log_err(f"failed to fetch dpu docker running status due to {e}")
            return False

    def _fetch_pdsagent_status(self):
        try:
            cmd = "ps -ef | grep 'pdsagent\|pds_dp_app'"
            output = self.apiHelper.run_docker_cmd(cmd)
            name = 'pdsagent'
            pdsagent_status = False
            if "pdsagent" in output:
                pdsagent_status = True
            elif "pds_dp_app" in output:
                pdsagent_status = True
                name = 'pds_dp_app'
            else:
                pdsagent_status = False
            return name, pdsagent_status
        except Exception as e:
            log_err(f'failed to fetch pdsagent status due to {e}')
            return 'pdsagent', False

    def _fetch_pciemgrd_status(self):
        try:
            cmd = "ps -ef | grep 'pciemgrd'"
            output = self.apiHelper.run_docker_cmd(cmd)
            pciemgrd_status = False
            if "pciemgrd" in output:
                pciemgrd_status = True
            else:
                pciemgrd_status = False
            return pciemgrd_status
        except Exception as e:
            log_err(f'failed to fetch pciemgrd status due to {e}')
            return False

    def _update_dpu_date_plane_db(self):
        try:
            dpu_status = 'DOWN'
            pdsagent_name, pdsagent_status = self._fetch_pdsagent_status()
            pciemgrd_status = self._fetch_pciemgrd_status()
            if pdsagent_status and pciemgrd_status:
                dpu_status = 'UP'
            dpu_docker_status = self._is_dpu_container_running()
            reason = f"DPU container named {self.dpu_docker_name} is{'' if dpu_docker_status else ' not'} running,"
            reason += f" {pdsagent_name} running : {self._bool_to_healthd_status(pdsagent_status)},"
            reason += f" pciemgrd running : {self._bool_to_healthd_status(pciemgrd_status)}"
            fvs_data_plane_data = [
                ('dpu_data_plane_state', dpu_status),
                ('dpu_data_plane_time', datetime.now().strftime('%Y%m%d %H:%M:%S')),
                ('dpu_data_plane_reason', reason)
            ]
            for name, value in fvs_data_plane_data:
                self.db.hset(self.table, name, value)
        except Exception as e:
            log_err(f'Failed to init dpu date plane entries due to {e}')

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
                eth_link_reason.append(f"{name} is {self.bool_to_link_status(eth_state)}")
            return state, "host-ethlink-status: " + ', '.join(eth_link_reason)
        except Exception as e:
            log_err(f'failed to fetch eth uplink status due to {e}')
            return False, "Failed to fetch host eth link status"

    def _update_dpu_control_plane_db(self):
        try:
            control_plane_status = self.db.hget(self.table, 'dpu_control_plane_state')
            control_plane_reason = self.db.hget(self.table, 'dpu_control_plane_reason')
            if (control_plane_status == None) or (control_plane_status == 'UP'):
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
                ('dpu_control_plane_state', self.bool_to_link_status(control_plane_status)),
                ('dpu_control_plane_time', datetime.now().strftime('%Y%m%d %H:%M:%S')),
                ('dpu_control_plane_reason', ', '.join(control_plane_reason))
            ]
            for name, value in fvs_control_plane_data:
                self.db.hset(self.table, name, value)
        except Exception as e:
            log_err(f'Failed to populate dpu control plane entries due to {e}')

    def _getGrpcEventMessage(self):
        while True:
            if self.event_stop:
                return
            grpcmsg = oper_pb2.OperInfoRequest()
            spec = grpcmsg.Request.add()
            spec.InfoType = oper_pb2.OPER_INFO_TYPE_EVENT
            spec.Action = oper_pb2.OPER_INFO_OP_SUBSCRIBE
            for event in self.events:
                spec.EventFilter.Types.append(event)
            yield grpcmsg

    def _process_event(self, event):
        global g_count

        try:
            event_type = event.EventInfo.Type
            event_description = event.EventInfo.Description
            event_message = event.EventInfo.Message
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
                dpu_data_plane_status = 'DOWN'
                dpu_data_plane_reason = event_description
                fvs_data_plane_data = [
                    ('dpu_data_plane_state', dpu_data_plane_status),
                    ('dpu_data_plane_time', datetime.now().strftime('%Y%m%d %H:%M:%S')),
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

    def _event_listener(self):
        channel_addr = "{}:{}".format(LOCALHOST,str(EVENT_PORT))
        channel = grpc.insecure_channel(channel_addr)
        stub = oper_pb2_grpc.OperSvcStub(channel)
        resp = stub.OperInfoSubscribe(self._getGrpcEventMessage())
        time.sleep(1)
        for event in resp:
            self._process_event(event)

    def start(self):
        # spawn operd listener thread
        self.event_stop = False
        if (self.event_thread == None) or (not self.event_thread.is_alive()):
            self.event_thread = threading.Thread(target=self._event_listener)
            self.event_thread.daemon = True
            self.event_thread.start()

    def stop(self):
        self.log_warning("Stopping event listener thread")
        self.event_stop = True
        if self.event_thread is not None:
            self.event_thread.join()
            self.event_thread = None

class DpuUtilSvcServicer(dpu_util_pb2_grpc.DpuUtilSvcServicer):

    def GetRebootCause(self, request, context):
        try:
            slot_id = get_slot_id(None)
            file_name = "/host/reboot-cause/previous-reboot-cause.json"
            file = open(file_name, "r")
            data = file.read()
            if data != '':
                json_data = json.loads(data)
                name = json_data['gen_time']
                cause = json_data['cause']
                time = json_data['time']
                user = json_data['user']
                comment = json_data['comment']
                reboot_cause = dpu_util_pb2.RebootCause(
                    name=name, cause=cause, time=time, user=user, comment=comment
                    )
                return dpu_util_pb2.RebootCauseResponse(slot_id=slot_id, reboot_cause=reboot_cause, success=True)
            else:
                log_err("Reboot-cause history is not available in reboot-causes")
                return dpu_util_pb2.RebootCauseResponse(slot_id=slot_id, reboot_cause=Empty, success=False)
        except Exception as e:
            log_err(f"Failed to sent reboot cause response due to {e}")
            return dpu_util_pb2.RebootCauseResponse(slot_id=slot_id, reboot_cause=Empty, success=False)

class RebootCauseGRPCUpdater():

    def __init__(self):
        self.process = multiprocessing.Process(target=self.serve)

    def serve(self):
        channel_addr = "{}:{}".format(IP, str(PORT))
        server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
        dpu_util_pb2_grpc.add_DpuUtilSvcServicer_to_server(DpuUtilSvcServicer(), server)
        server.add_insecure_port(channel_addr)
        server.start()
        log_info(f"Server started. Listening on port {PORT}")
        server.wait_for_termination()

    def start(self):
        self.process.start()

    def stop(self):
        if self.process.is_alive():
            self.process.terminate()
            self.process.join()
            log_info("gRPC server process terminated.")

class VersionUpdater():
    def __init__(self, chassis, db):
        self.db = db
        self.chassis = chassis
        self.slot_id = get_slot_id(self.chassis)
        self.table = f'{VERSION_UPDATE_TABLE_NAME}|DPU{self.slot_id}|1'
        try:
            self.apiHelper = APIHelper()
            self.dpu_docker_name = self.apiHelper.get_dpu_docker_container_name()
        except:
            log_err('Failed to get dpu docker name')

    def delete_table_version_entries(self):
        try:
            if self.db:
                sonic_version_info_table = self.db.keys(self.table)
                if len(sonic_version_info_table) == 1:
                    table = sonic_version_info_table[0]
                    self.db.delete(table)
        except Exception as e:
            log_err(f'failed to delete fetch sonic version entries due to {e}')

    def _fetch_version_info(self):
        try:
            from sonic_installer.bootloader import get_bootloader
            bootloader = get_bootloader()
            curimage = bootloader.get_current_image()
            curimage = curimage.split('-')[-1]
            fwupdate_version_list = []
            try:
                cmd = "/nic/tools/fwupdate -l | grep -v 'FATAL' "
                cmd += "| grep -v 'sh: /dev/mapper: unknown operand'| "
                cmd += "grep -v 'standard metadata magic not found'"
                output = self.apiHelper.run_docker_cmd(cmd)
                fw_version_data = json.loads(output)
                try:
                    fwupdate_version_list.append(fw_version_data['mainfwa']['system_image']['software_version'])
                except:
                    fwupdate_version_list.append(NOT_AVAILABLE)
                try:
                    fwupdate_version_list.append(fw_version_data['mainfwa']['uboot']['software_version'])
                except:
                    fwupdate_version_list.append(NOT_AVAILABLE)
                try:
                    fwupdate_version_list.append(fw_version_data['goldfw']['kernel_fit']['software_version'])
                except:
                    fwupdate_version_list.append(NOT_AVAILABLE)
                try:
                    fwupdate_version_list.append(fw_version_data['goldfw']['uboot']['software_version'])
                except:
                    fwupdate_version_list.append(NOT_AVAILABLE)
            except Exception as e:
                log_err(f"Failed to fetch mainfwa/goldfw version info due to {e}")
                fwupdate_version_list = [NOT_AVAILABLE]*4
            version_info_list = [curimage]
            version_info_list.extend(fwupdate_version_list)
            return version_info_list
        except Exception as e:
            log_err(f"Failed to fetch sonic version info due to {e}")
            return [NOT_AVAILABLE]*6

    def update(self):
        log_info("Start sonic version info updating")
        try:
            stat = self._fetch_version_info()
            if stat == None:
                raise Exception("Failed to fetch sonic version info")
            self._refresh_sonic_version_db(stat)
        except Exception as e:
            log_err(f'failed to fetch sonic version data due to {e}')
        log_info("End sonic version info updating")

    def _refresh_sonic_version_db(self, stat):
        try:
            if self.db == None:
                return
            self.delete_table_version_entries()
            [current, mainfwa_sw, mainfwa_uboot, goldfw_sw, goldfw_uboot] = stat
            inventory_value = ""
            inventory_value += f"dpu_sonic_version_current: {current}"
            inventory_value += f"\nmainfwa_sw_version: {mainfwa_sw}"
            inventory_value += f"\ngoldfw_sw_version: {goldfw_sw}"
            inventory_value += f"\nmainfwa_uboot_version: {mainfwa_uboot}"
            inventory_value += f"\ngoldfw_uboot_version: {goldfw_uboot}"
            self.db.set(self.table, inventory_value)
        except Exception as e:
            log_err('Failed to update sonic version values - {}'.format(repr(e)))
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
        self.reboot_cause_updater = RebootCauseGRPCUpdater()
        self.reboot_cause_updater.start()
        self.version_updater = VersionUpdater(self.chassis, self.db)
        self.version_updater.update()
        self.event_handler = EventHandler(self.chassis, self.db)
        self.event_handler.start()

    def deinit(self):
        """
        Deinitializer of DpuDBUtilDaemon
        """
        if self.db == None:
            return
        self.event_handler.stop()
        self.reboot_cause_updater.stop()

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
                self.reboot_cause_updater.stop()
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
