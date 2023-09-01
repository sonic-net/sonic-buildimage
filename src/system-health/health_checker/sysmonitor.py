#!/usr/bin/python3

import os
import sys
import time
import glob
import multiprocessing
from datetime import datetime
from swsscommon import swsscommon
from sonic_py_common.logger import Logger
from . import utils
from sonic_py_common.task_base import ProcessTaskBase
from .config import Config
import signal

SYSLOG_IDENTIFIER = "system#monitor"
REDIS_TIMEOUT_MS = 0
system_allsrv_state = "DOWN"
spl_srv_list = ['database-chassis', 'gbsyncd']
SELECT_TIMEOUT_MSECS = 1000
QUEUE_TIMEOUT = 15
TASK_STOP_TIMEOUT = 10
logger = Logger(log_identifier=SYSLOG_IDENTIFIER)


#Subprocess which subscribes to STATE_DB FEATURE table for any update
#and push service events to main process via queue
class MonitorStateDbTask(ProcessTaskBase):

    def __init__(self,myQ):
        ProcessTaskBase.__init__(self)
        self.task_queue = myQ

    def subscribe_statedb(self):
        state_db = swsscommon.DBConnector("STATE_DB", REDIS_TIMEOUT_MS, False)
        sel = swsscommon.Select()
        cst = swsscommon.SubscriberStateTable(state_db, "FEATURE")
        sel.addSelectable(cst)

        while not self.task_stopping_event.is_set():
            (state, c) = sel.select(SELECT_TIMEOUT_MSECS)
            if state == swsscommon.Select.TIMEOUT:
                continue
            if state != swsscommon.Select.OBJECT:
                logger.log_warning("sel.select() did not return swsscommon.Select.OBJECT")
                continue
            (key, op, cfvs) = cst.pop()
            key_ext = key + ".service"
            timestamp = "{}".format(datetime.utcnow().strftime("%Y-%m-%d %H:%M:%S"))
            msg={"unit": key_ext, "evt_src":"feature", "time":timestamp}
            self.task_notify(msg)


    def task_worker(self):
        if self.task_stopping_event.is_set():
            return
        try:
            self.subscribe_statedb()
        except Exception as e:
            logger.log_error("subscribe_statedb exited- {}".format(str(e)))

    def task_notify(self, msg):
        if self.task_stopping_event.is_set():
            return
        self.task_queue.put(msg)


#Subprocess which subscribes to system dbus to listen for systemd events
#and push service events to main process via queue
class MonitorSystemBusTask(ProcessTaskBase):

    def __init__(self,myQ):
        ProcessTaskBase.__init__(self)
        self.task_queue = myQ

    def on_job_removed(self, id, job, unit, result):
        if result == "done" or result == "failed":
            timestamp = "{}".format(datetime.utcnow().strftime("%Y-%m-%d %H:%M:%S"))
            msg = {"unit": unit, "evt_src":"sysbus", "time":timestamp}
            self.task_notify(msg)
            return

    #Function for listening the systemd event on dbus
    def subscribe_sysbus(self):
        import dbus
        from gi.repository import GLib
        from dbus.mainloop.glib import DBusGMainLoop

        DBusGMainLoop(set_as_default=True)
        bus = dbus.SystemBus()
        systemd = bus.get_object('org.freedesktop.systemd1', '/org/freedesktop/systemd1')
        manager = dbus.Interface(systemd, 'org.freedesktop.systemd1.Manager')
        manager.Subscribe()
        manager.connect_to_signal('JobRemoved', self.on_job_removed)

        loop = GLib.MainLoop()
        loop.run()

    def task_worker(self):
        if self.task_stopping_event.is_set():
            return
        logger.log_info("Start Listening to systemd bus (pid {0})".format(os.getpid()))
        self.subscribe_sysbus()

    def task_notify(self, msg):
        if self.task_stopping_event.is_set():
            return
        self.task_queue.put(msg)

#Mainprocess which launches 2 subtasks - systembus task and statedb task
#and on receiving events, checks and updates the system ready status to state db
class Sysmonitor(ProcessTaskBase):

    def __init__(self):
        ProcessTaskBase.__init__(self)
        self._stop_timeout_secs = TASK_STOP_TIMEOUT
        self.dnsrvs_name = set()
        self.state_db = None
        self.config_db = None
        self.config = Config()
        self.mpmgr = multiprocessing.Manager()
        self.myQ = self.mpmgr.Queue()

    #Sets system ready status to state db
    def post_system_status(self, state):
        try:
            if not self.state_db:
                self.state_db = swsscommon.SonicV2Connector(use_unix_socket_path=True)
                self.state_db.connect(self.state_db.STATE_DB)

            self.state_db.set(self.state_db.STATE_DB, "SYSTEM_READY|SYSTEM_STATE", "Status", state)
            logger.log_info("Posting system ready status {} to statedb".format(state))

        except Exception as e:
            logger.log_error("Unable to post system ready status: {}".format(str(e)))

    #Forms the service list to be monitored
    def get_all_service_list(self):

        if not self.config_db:
            self.config_db = swsscommon.ConfigDBConnector(use_unix_socket_path=True)
            self.config_db.connect()

        dir_list = []
        #add the services from the below targets
        targets= ["/etc/systemd/system/multi-user.target.wants", "/etc/systemd/system/sonic.target.wants"]
        for path in targets:
            dir_list += [os.path.basename(i) for i in glob.glob('{}/*.service'.format(path))]

        #add the enabled docker services from config db feature table
        self.get_service_from_feature_table(dir_list)

        self.config.load_config()
        if self.config and self.config.ignore_services:
            for srv in self.config.ignore_services:
                if srv in dir_list:
                    dir_list.remove(srv)

        dir_list.sort()
        return dir_list

    def get_service_from_feature_table(self, dir_list):
        """Get service from CONFIG DB FEATURE table. During "config reload" command, filling FEATURE table
           is not an atomic operation, sonic-cfggen do it with two steps:
               1. Add an empty table entry to CONFIG DB
               2. Add all fields to the table

            So, if system health read db on middle of step 1 and step 2, it might read invalid data. A retry
            mechanism is here to avoid such issue.

        Args:
            dir_list (list): service list
        """
        max_retry = 3
        retry_delay = 1
        success = True

        while max_retry > 0:
            success = True
            feature_table = self.config_db.get_table("FEATURE")
            for srv, fields in feature_table.items():
                if 'state' not in fields:
                    success = False
                    logger.log_warning("FEATURE table is not fully ready: {}, retrying".format(feature_table))
                    break
                if fields["state"] not in ["disabled", "always_disabled"]:
                    srvext = srv + ".service"
                    if srvext not in dir_list:
                        dir_list.append(srvext)
            if not success:
                max_retry -= 1
                time.sleep(retry_delay)
            else:
                break
        if not success:
            logger.log_error("FEATURE table is not fully ready: {}, max retry reached".format(feature_table))

    #Checks FEATURE table from config db for the service' check_up_status flag
    #if marked to true, then read the service up_status from FEATURE table of state db.
    #else, just return Up
    def get_app_ready_status(self, service):
        if not self.state_db:
            self.state_db = swsscommon.SonicV2Connector(use_unix_socket_path=True)
            self.state_db.connect(self.state_db.STATE_DB)
        if not self.config_db:
            self.config_db = swsscommon.ConfigDBConnector(use_unix_socket_path=True)
            self.config_db.connect()

        fail_reason = ""
        check_app_up_status = ""
        up_status_flag = ""
        configdb_feature_table = self.config_db.get_table('FEATURE')
        update_time = "-"

        if service not in configdb_feature_table.keys():
            pstate = "Up"
        else:
            check_app_up_status = configdb_feature_table[service].get('check_up_status')
            if check_app_up_status is not None and (check_app_up_status.lower()) == "true":
                up_status_flag = self.state_db.get(self.state_db.STATE_DB, 'FEATURE|{}'.format(service), 'up_status')
                if up_status_flag is not None and (up_status_flag.lower()) == "true":
                    pstate = "Up"
                else:
                    fail_reason = self.state_db.get(self.state_db.STATE_DB, 'FEATURE|{}'.format(service), 'fail_reason')
                    if fail_reason is None:
                        fail_reason = "NA"
                    pstate = "Down"

                update_time = self.state_db.get(self.state_db.STATE_DB, 'FEATURE|{}'.format(service), 'update_time')
                if update_time is None:
                    update_time = "-"
            else:
                #Either check_up_status marked False or entry does not exist
                pstate = "Up"

        return pstate,fail_reason,update_time

    #Gets the service properties
    def run_systemctl_show(self, service):
        command = ('systemctl show {} --property=Id,LoadState,UnitFileState,Type,ActiveState,SubState,Result'.format(service))
        output = utils.run_command(command)
        srv_properties = output.split('\n')
        prop_dict = {}
        for prop in srv_properties:
            kv = prop.split("=", 1)
            if len(kv) == 2:
                prop_dict[kv[0]] = kv[1]

        return prop_dict

    #Sets the service status to state db
    def post_unit_status(self, srv_name, srv_status, app_status, fail_reason, update_time):
        if not self.state_db:
            self.state_db = swsscommon.SonicV2Connector(use_unix_socket_path=True)
            self.state_db.connect(self.state_db.STATE_DB)

        key = 'ALL_SERVICE_STATUS|{}'.format(srv_name)
        statusvalue = {}
        statusvalue['service_status'] = srv_status
        statusvalue['app_ready_status'] = app_status
        statusvalue['fail_reason'] = fail_reason
        statusvalue['update_time'] = update_time
        self.state_db.hmset(self.state_db.STATE_DB, key, statusvalue)

    #Reads the current status of the service and posts it to state db
    def get_unit_status(self, event):
        """ Get a unit status"""
        global spl_srv_list
        unit_status = "NOT OK"
        update_time = "-"

        try:
            service_status = "Down"
            service_up_status = "Down"
            service_name,last_name = event.split('.')

            sysctl_show = self.run_systemctl_show(event)

            load_state = sysctl_show.get('LoadState')
            if load_state == "loaded":
                status = sysctl_show['UnitFileState']
                fail_reason = sysctl_show['Result']
                active_state = sysctl_show['ActiveState']
                sub_state = sysctl_show['SubState']
                srv_type = sysctl_show['Type']

                #Raise syslog for service state change
                logger.log_info("{} service state changed to [{}/{}]".format(event, active_state, sub_state))

                if status == "enabled" or status == "enabled-runtime" or status == "static":
                    if fail_reason == "success":
                        fail_reason = "-"
                    if (active_state == "active" and sub_state == "exited"):
                        service_status = "OK"
                        service_up_status = "OK"
                        unit_status = "OK"
                    elif active_state == "active" and sub_state == "running":
                        service_status = "OK"
                        init_state,app_fail_reason,update_time = self.get_app_ready_status(service_name)
                        if init_state == "Up":
                            service_up_status = "OK"
                            unit_status = "OK"
                        else:
                            fail_reason = app_fail_reason
                            unit_status = "NOT OK"
                            if fail_reason == "docker start":
                                service_up_status = "Starting"
                                fail_reason = "-"
                    elif active_state == "activating":
                        service_status = "Starting"
                        service_up_status = "Starting"
                    elif active_state == "deactivating":
                        service_status = "Stopping"
                        service_up_status = "Stopping"
                    elif active_state == "inactive":
                        if srv_type == "oneshot" or service_name in spl_srv_list:
                            service_status = "OK"
                            service_up_status = "OK"
                            unit_status = "OK"
                        else:
                            unit_status = "NOT OK"
                            if fail_reason == "-":
                                fail_reason = "Inactive"
                    else:
                        unit_status = "NOT OK"

                    self.post_unit_status(service_name, service_status, service_up_status, fail_reason, update_time)

                    return unit_status

        except Exception as e:
            logger.log_error("Get unit status {}-{}".format(service_name, str(e)))


    #Gets status of all the services from service list
    def get_all_system_status(self):
        """ Shows the system ready status"""
        #global dnsrvs_name
        scan_srv_list = []

        scan_srv_list = self.get_all_service_list()
        for service in scan_srv_list:
            ustate = self.get_unit_status(service)
            if ustate == "NOT OK":
                if service not in self.dnsrvs_name:
                    self.dnsrvs_name.add(service)

        if len(self.dnsrvs_name) == 0:
            return "UP"
        else:
            return "DOWN"

    #Displays the system ready status message on console
    def print_console_message(self, message):
        with open('/dev/console', 'w') as console:
            console.write("\n{} {}\n".format(datetime.now().strftime("%b %d %H:%M:%S.%f"), message))

    #Publish the system ready status message on logger,console and state db
    def publish_system_status(self, astate):
        global system_allsrv_state
        if system_allsrv_state != astate:
            system_allsrv_state = astate
            if astate == "DOWN":
                msg = "System is not ready - one or more services are not up"
            elif astate == "UP":
                msg = "System is ready"
            logger.log_notice(msg)
            self.print_console_message(msg)
            self.post_system_status(astate)

    #Checks all the services and updates the current system status
    def update_system_status(self):
        try:
            astate = self.get_all_system_status()
            self.publish_system_status(astate)

        except Exception as e:
            logger.log_error("update system status exception:{}".format(str(e)))

    #Checks a service status and updates the system status
    def check_unit_status(self, event):
        #global dnsrvs_name
        if not self.state_db:
            self.state_db = swsscommon.SonicV2Connector(use_unix_socket_path=True)
            self.state_db.connect(self.state_db.STATE_DB)
        astate = "DOWN"

        full_srv_list = self.get_all_service_list()
        if event in full_srv_list:
            ustate = self.get_unit_status(event)
            if ustate == "OK" and system_allsrv_state == "UP":
                astate = "UP"
            elif ustate == "OK" and system_allsrv_state == "DOWN":
                if event in self.dnsrvs_name:
                    self.dnsrvs_name.remove(event)
                    if len(self.dnsrvs_name) == 0:
                        astate = "UP"
                    else:
                        astate = "DOWN"
            else:
                if event not in self.dnsrvs_name:
                    self.dnsrvs_name.add(event)
                astate = "DOWN"

            self.publish_system_status(astate)
        else:
            #if received event is not in current full service list but exists in STATE_DB & set,
            #then it should be removed from STATE_DB & set
            if event in self.dnsrvs_name:
                self.dnsrvs_name.remove(event)

            if len(self.dnsrvs_name) == 0:
                astate = "UP"
            else:
                astate = "DOWN"
            self.publish_system_status(astate)

            srv_name,last = event.split('.')
            # stop on service maybe propagated to timers and in that case,
            # the state_db entry for the service should not be deleted
            if last == "service":
                key = 'ALL_SERVICE_STATUS|{}'.format(srv_name)
                key_exists = self.state_db.exists(self.state_db.STATE_DB, key)
                if key_exists == 1:
                    self.state_db.delete(self.state_db.STATE_DB, key)

        return 0

    def system_service(self):
        if not self.state_db:
            self.state_db = swsscommon.SonicV2Connector(use_unix_socket_path=True)
            self.state_db.connect(self.state_db.STATE_DB)

        try:
            monitor_system_bus = MonitorSystemBusTask(self.myQ)
            monitor_system_bus.task_run()

            monitor_statedb_table = MonitorStateDbTask(self.myQ)
            monitor_statedb_table.task_run()

        except Exception as e:
            logger.log_error("SubProcess-{}".format(str(e)))
            sys.exit(1)


        self.update_system_status()

        from queue import Empty
        # Queue to receive the STATEDB and Systemd state change event
        while not self.task_stopping_event.is_set():
            try:
                msg = self.myQ.get(timeout=QUEUE_TIMEOUT)
                event = msg["unit"]
                event_src = msg["evt_src"]
                event_time = msg["time"]
                logger.log_debug("Main process- received event:{} from source:{} time:{}".format(event,event_src,event_time))
                logger.log_info("check_unit_status for [ "+event+" ] ")
                self.check_unit_status(event)
            except Empty:
                pass
            except Exception as e:
                logger.log_error("system_service"+str(e))

        #cleanup tables  "'ALL_SERVICE_STATUS*', 'SYSTEM_READY*'" from statedb
        self.state_db.delete_all_by_pattern(self.state_db.STATE_DB, "ALL_SERVICE_STATUS|*")
        self.state_db.delete_all_by_pattern(self.state_db.STATE_DB, "SYSTEM_READY|*")

        monitor_system_bus.task_stop()
        monitor_statedb_table.task_stop()

    def task_worker(self):
        if self.task_stopping_event.is_set():
            return
        self.system_service()

    def task_stop(self):
        # Signal the process to stop
        self.task_stopping_event.set()
        #Clear the resources of mpmgr- Queue
        self.mpmgr.shutdown()

        # Wait for the process to exit
        self._task_process.join(self._stop_timeout_secs)

        # If the process didn't exit, attempt to kill it
        if self._task_process.is_alive():
            logger.log_notice("Attempting to kill sysmon main process with pid {}".format(self._task_process.pid))
            os.kill(self._task_process.pid, signal.SIGKILL)

        if self._task_process.is_alive():
            logger.log_error("Sysmon main process with pid {} could not be killed".format(self._task_process.pid))
            return False

        return True


