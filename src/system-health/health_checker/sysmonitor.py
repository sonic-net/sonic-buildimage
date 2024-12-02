#!/usr/bin/python3

import glob
import multiprocessing
import os
import sys
import time
from copy import copy
from datetime import datetime

from sonic_py_common.logger import Logger
from sonic_py_common.task_base import ProcessTaskBase
from sonic_py_common import device_info
from swsscommon import swsscommon

from . import utils
from .config import Config
import signal
import jinja2

SYSLOG_IDENTIFIER = "system#monitor"
REDIS_TIMEOUT_MS = 0
spl_srv_list = ['database-chassis', 'gbsyncd', 'e2scrub_reap']
SELECT_TIMEOUT_MSECS = 1000
QUEUE_TIMEOUT = 15
TASK_STOP_TIMEOUT = 10
logger = Logger(log_identifier=SYSLOG_IDENTIFIER)
exclude_srv_list = ['ztp.service']

def get_timestamp():
    return '{}'.format(datetime.utcnow().strftime("%Y-%m-%d %H:%M:%S"))


#Subprocess which subscribes to STATE_DB FEATURE table for any update
#and push service events to main process via queue
class MonitorStateDbTask(ProcessTaskBase):

    def __init__(self,myQ):
        ProcessTaskBase.__init__(self)
        self.task_queue = myQ

    def subscribe_statedb(self):
        state_db = swsscommon.DBConnector("STATE_DB", REDIS_TIMEOUT_MS, False)
        sel = swsscommon.Select()
        feature_sub = swsscommon.SubscriberStateTable(state_db, "FEATURE")
        service_sub = swsscommon.SubscriberStateTable(state_db, "SERVICE_APP")
        sel.addSelectable(feature_sub)
        sel.addSelectable(service_sub)

        while not self.task_stopping_event.is_set():
            (state, c) = sel.select(SELECT_TIMEOUT_MSECS)
            if state == swsscommon.Select.TIMEOUT:
                continue
            if state != swsscommon.Select.OBJECT:
                logger.log_warning("sel.select() did not return swsscommon.Select.OBJECT")
                continue

            fd = c.getFd()
            if fd == feature_sub.getFd():
                (key, op, cfvs) = feature_sub.pop()
                source = 'feature'
            elif fd == service_sub.getFd():
                (key, op, cfvs) = service_sub.pop()
                source = 'service_app'
            else:
                logger.log_warning("sel.select() returned unexpected selectable OBJECT")
                continue

            key_ext = key + ".service"
            self.task_notify({"unit": key_ext,
                              "evt_src": source,
                              "time": get_timestamp()})


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
        from dbus.mainloop.glib import DBusGMainLoop
        from gi.repository import GLib

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

    def task_stop(self):
        # FIXME: Gracefully stop `loop.run()`.
        self._task_process.kill()
        return True

    def task_notify(self, msg):
        if self.task_stopping_event.is_set():
            return
        self.task_queue.put(msg)


# Subprocess to monitor system ready timeout. If timeout will be exceeded,
# send a message to queue and exit
class MonitorTimeout(ProcessTaskBase):
    def __init__(self, myQ, config):
        ProcessTaskBase.__init__(self)
        self._stop_timeout_secs = TASK_STOP_TIMEOUT
        self.task_queue = myQ

        if config and config.timeout:
            self.timeout = config.timeout
        else:
            self.timeout = 10
            logger.log_warning("Can't get timeout configuration, assuming 10 "
                               "minutes")

    def task_worker(self):
        if self.task_stopping_event.is_set():
            return

        logger.log_info(f'Set system ready timeout task for {self.timeout} '
                         'minutes')

        # Repeat until timeout or canceling
        while not self.task_stopping_event.is_set():
            if utils.get_uptime() > self.timeout * 60:
                logger.log_warning('Got system ready timeout. Notifying')
                self.task_notify({
                    'unit': 'timeout',
                    'evt_src': 'timeout',
                    'time': get_timestamp()
                })
                break

            time.sleep(15)

        logger.log_info('System ready timeout task finished')

    def task_notify(self, msg):
        if self.task_stopping_event.is_set():
            return
        self.task_queue.put(msg)


#Mainprocess which launches 2 subtasks - systembus task and statedb task
#and on receiving events, checks and updates the system ready status to state db
class Sysmonitor(ProcessTaskBase):

    def __init__(self):
        ProcessTaskBase.__init__(self)
        # Initialize the timeout to stop this task once "task_stop" event is
        # received
        self._stop_timeout_secs = TASK_STOP_TIMEOUT

        self.state_db = None
        self.config_db = None
        self.monitor_timeout = None

        # List of services in "DOWN" state
        self.dnsrvs_name = set()
        # Last reported system state
        self.last_reported_state = 'INIT'

        # Load system health monitoring configuration
        self.config = Config()
        self.config.load_config()
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

        dir_set = set()
        srvs_files = set()

        #add the services from the below targets
        targets= [
            "/etc/systemd/system/multi-user.target.wants",
            "/etc/systemd/system/sonic.target.wants",
        ]
        for path in targets:
            srvs_files_list = glob.glob('{}/*.service'.format(path))
            srvs_files |= {os.path.basename(i) for i in srvs_files_list}

        # By default, wait for all
        dir_set = copy(srvs_files)

        # Want only services: means all multi-user and sonic targets, except
        # services defined in FEATURE table. Feature table is expandable, so
        # it may and should control services which will have effect on
        # system-ready state by itself.
        # Leave only services we want to wait for
        if self.config and self.config.wait_services:
            # srv_file example      - swss@1.service, syncd@0.service
            # full_srv_name example - swss@1, syncd@0
            # srv_name example      - swss, syncd
            dir_set = set()
            for srv_name in self.config.wait_services:
                for srv_file in srvs_files:
                    full_srv_name = srv_file.removesuffix('.service')
                    if (full_srv_name == srv_name or
                        full_srv_name.startswith(srv_name + '@')):
                        dir_set.add(srv_file)

        # add the enabled docker services from config db feature table.
        # It will respect irrel_for_sysready flag.
        feature_table_srvs = self.get_service_from_feature_table()
        # srv_file example      - swss@1.service, syncd@0.service
        # full_srv_name example - swss@1, syncd@0
        # srv_name example      - swss, syncd
        for srv_name in feature_table_srvs:
            for srv_file in srvs_files:
                full_srv_name = srv_file.removesuffix('.service')
                if (full_srv_name == srv_name or
                    full_srv_name.startswith(srv_name + '@')):
                    dir_set.add(srv_file)

        # Ignore what we don't need (we are using here the list of services to
        # ignore for system health), of course if some service doesn't affect
        # system health, so why it should affect on system ready? Well, it
        # shouldn't.
        # So that last filter filters again all services we don't want.
        if self.config and self.config.ignore_services:
            # srv_file example      - swss@1.service, syncd@0.service
            # srv_name example      - swss, syncd
            for srv_file in copy(dir_set):
                srv_name = srv_file.removesuffix('.service').split('@')[0]
                if srv_name in self.config.ignore_services:
                    dir_set.remove(srv_file)

        # Remove services from exclude list Eg.ztp.service
        for srv in exclude_srv_list:
            if srv in dir_set:
                dir_set.remove(srv)

        # Feature is controllable by sysready_state parameter of
        # DEVICE_METADATA table. Setting it to "disable" will just simulate
        # "PortInitDone" behavior.
        device_metadata = self.config_db.get_table(
            swsscommon.CFG_DEVICE_METADATA_TABLE_NAME).get('localhost', {})
        feature_state = device_metadata.get('sysready_state', 'enabled')

        # Handle feature state.
        # Use only swss.service to rely on ready state
        if feature_state == 'disabled':
            # Order matters
            desired_srvs = ['swss.service']
            for srv in desired_srvs:
                if srv in dir_set:
                    dir_set = {srv}
                    break

        return sorted(dir_set)

    def get_render_value_for_field(self, configuration, device_config, expected_values):
        """ Returns the target value by rendering the configuration as J2 template.

        Args:
            configuration (str): Table value from CONFIG_DB for given field
            device_config (dict): DEVICE_METADATA section of CONFIG_DB and populated Device Running Metadata which is needed for rendering
            expected_values (list): Expected set of values for given field
        Returns:
            (str): Target value for given key
        """

        if configuration is None:
            return None

        template = jinja2.Template(configuration)
        target_value = template.render(device_config) # nosemgrep: python.flask.security.xss.audit.direct-use-of-jinja2.direct-use-of-jinja2
        if target_value not in expected_values:
            raise ValueError('Invalid value rendered for configuration {}: {}'.format(configuration, target_value))
        return target_value

    def get_service_from_feature_table(self):
        """Get service from CONFIG DB FEATURE table. During "config reload" command, filling FEATURE table
           is not an atomic operation, sonic-cfggen do it with two steps:
               1. Add an empty table entry to CONFIG DB
               2. Add all fields to the table

            So, if system health read db on middle of step 1 and step 2, it might read invalid data. A retry
            mechanism is here to avoid such issue.

        Return:
            srvs_list:  list:   tracked services list
        """
        max_retry = 3
        retry_delay = 1
        success = True

        srvs_list = []

        while max_retry > 0:
            success = True
            try:
                feature_table = self.config_db.get_table("FEATURE")
                device_config = {}
                device_config['DEVICE_METADATA'] = self.config_db.get_table('DEVICE_METADATA')
                device_config.update(device_info.get_device_runtime_metadata())
                for srv, fields in feature_table.items():
                    if 'state' not in fields:
                        success = False
                        logger.log_warning("FEATURE table is not fully ready: {}, retrying".format(feature_table))
                        break
                    state = self.get_render_value_for_field(fields["state"], device_config, ['enabled', 'disabled', 'always_enabled', 'always_disabled'])
                    if state not in ["disabled", "always_disabled"]:
                        if fields.get('irrel_for_sysready', '').lower() != 'true':
                            if srv not in srvs_list:
                                srvs_list.append(srv)
            except:
                success = False
            if not success:
                max_retry -= 1
                time.sleep(retry_delay)
            else:
                break
        if not success:
            logger.log_error("FEATURE table is not fully ready: {}, max retry reached".format(feature_table))

        return srvs_list

    # Checks FEATURE table from config db or data from json file for the
    # service' check_up_status flag if marked to true, then read the service
    # up_status from FEATURE table of state db. Else, just return Up
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

        app_up_status_cfg = set()
        if self.config and self.config.services_to_report_app_status:
            app_up_status_cfg = self.config.services_to_report_app_status

        # Config DB will have a configuration for non parametrized service
        # But if there is a parameter, the service should handle the right
        # reporting itself.
        service_noparam = service.split('@')[0]
        if service_noparam not in (app_up_status_cfg |
                                   configdb_feature_table.keys()):
            pstate = "Up"
            return pstate,fail_reason,update_time

        if service_noparam in configdb_feature_table.keys():
            table = 'FEATURE'
            check_app_up_status = \
                configdb_feature_table[service_noparam].get('check_up_status')
        else:
            table = 'SERVICE_APP'

        srvc_table = '{}|{}'.format(table, service)

        # Handle feature's application up status
        if table == 'FEATURE' and (not check_app_up_status or
                                   check_app_up_status.lower() == 'false'):
            # Don't check application up status if check_app_up_status flag is
            # not present or set to false
            pstate = "Up"
        else:
            up_status_flag = self.state_db.get(self.state_db.STATE_DB,
                                               srvc_table, 'up_status')
            if up_status_flag is not None and up_status_flag.lower() == "true":
                pstate = "Up"
            else:
                if (up_status_flag is not None and
                    up_status_flag.lower() == "false"):
                    # The application is in failed state if status flag set to
                    # false
                    pstate = "Failed"
                else:
                    # In that case it is just in init state
                    pstate = "Down"

                # Try to get fail reason anyway
                fail_reason = self.state_db.get(self.state_db.STATE_DB,
                                                srvc_table, 'fail_reason')
                if fail_reason is None:
                    fail_reason = "NA"

            update_time = self.state_db.get(self.state_db.STATE_DB, srvc_table,
                                            'update_time')
            if update_time is None:
                update_time = "-"

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

        # Log message with proper level
        logf = logger.log_notice
        msg = f'{srv_name} service status: {srv_status}, '
        msg += f'app status: {app_status}'

        if srv_status != 'OK':
            msg += f', fail reason: {fail_reason}'

            # So far good down reasons for each service status
            good_dr = {
                'Down': ['Inactive'],
                'Starting': ['-'],
                'Stopping': ['-']
            }

            # Only allowed combinations
            if fail_reason not in good_dr.get(srv_status, []):
                logf = logger.log_error

        logf(msg)

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
                        elif init_state == "Failed":
                            fail_reason = app_fail_reason
                            unit_status = "FAILED"
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
                        if service_name in spl_srv_list:
                            service_status = "OK"
                            service_up_status = "OK"
                            unit_status = "OK"
                        elif srv_type == "oneshot":
                            # If oneshot service is 
                            if sub_state == "exited":
                                service_status = "OK"
                                service_up_status = "OK"
                                unit_status = "OK"
                            else:
                                unit_status = "NOT OK"
                                if fail_reason == "-":
                                    fail_reason = "Inactive"
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
            elif ustate == "FAILED":
                return "FAILED"

        if len(self.dnsrvs_name) == 0:
            return "UP"
        else:
            return "INIT"

    #Displays the system ready status message on console
    def print_console_message(self, message):
        with open('/dev/console', 'w') as console:
            console.write("\n{} {}\n".format(datetime.now().strftime("%b %d %H:%M:%S.%f"), message))

    #Publish the system ready status message on logger,console and state db
    def publish_system_status(self, new_state):
        # Allowed transitions
        # 'from': ['to']
        transitions = {
            'INIT': ['UP', 'DOWN', 'FAILED'],
            'DOWN': ['FAILED']
        }

        # Check if state transition is allowed. If status update not allowed,
        # don't perform it
        if new_state not in transitions.get(self.last_reported_state, []):
            return

        if new_state == "DOWN":
            msg = "System is not ready - one or more services are not up"
        elif new_state == "FAILED":
            msg = "System is not ready - one or more services failed"
        else:
            msg = "System is ready"

        # Broadcast new system status
        logger.log_notice(msg)
        self.print_console_message(msg)
        self.post_system_status(new_state)
        self.last_reported_state = new_state

        # We don't need timeout anymore
        self.monitor_timeout.task_stop()

    #Checks a service status and updates the system status
    def check_unit_status(self, event):
        if not self.state_db:
            self.state_db = swsscommon.SonicV2Connector(use_unix_socket_path=True)
            self.state_db.connect(self.state_db.STATE_DB)
        astate = 'INIT'

        full_srv_list = self.get_all_service_list()
        if event in full_srv_list:
            ustate = self.get_unit_status(event)
            if ustate == 'OK':
                # If that unit was down/inactive, and now it is up, remove it
                # from the set of down units
                if event in self.dnsrvs_name:
                    self.dnsrvs_name.remove(event)
                # If now the set of down units is empty - we are good to report
                # "OK" system status
                if len(self.dnsrvs_name) == 0:
                    astate = 'UP'
            elif ustate == 'FAILED':
                # If unit reported failed state, the system should report
                # "NOT OK" status
                astate = 'FAILED'
            elif ustate == 'NOT OK':
                if event not in self.dnsrvs_name:
                    self.dnsrvs_name.add(event)
            else:
                logger.log_warning(f'Got wrong {event} unit state: {ustate}. '
                                   'Nothing to do')
                return

            self.publish_system_status(astate)
        else:
            #if received event is not in current full service list but exists in STATE_DB & set,
            #then it should be removed from STATE_DB & set
            if event in self.dnsrvs_name:
                self.dnsrvs_name.remove(event)

            if len(self.dnsrvs_name) == 0:
                astate = "UP"
                self.publish_system_status(astate)

            srv_name,last = event.split('.')
            # stop on service maybe propagated to timers and in that case,
            # the state_db entry for the service should not be deleted
            if last == "service":
                key = 'ALL_SERVICE_STATUS|{}'.format(srv_name)
                key_exists = self.state_db.exists(self.state_db.STATE_DB, key)
                if key_exists == 1:
                    self.state_db.delete(self.state_db.STATE_DB, key)

        return

    def system_service(self):
        if not self.state_db:
            self.state_db = swsscommon.SonicV2Connector(use_unix_socket_path=True)
            self.state_db.connect(self.state_db.STATE_DB)

        try:
            monitor_system_bus = MonitorSystemBusTask(self.myQ)
            monitor_system_bus.task_run()

            monitor_statedb_table = MonitorStateDbTask(self.myQ)
            monitor_statedb_table.task_run()

            self.monitor_timeout = MonitorTimeout(self.myQ, self.config)
            self.monitor_timeout.task_run()
        except Exception as e:
            logger.log_error("SubProcess-{}".format(str(e)))
            sys.exit(1)

        try:
            # Maybe everything is ready now, so let's try
            astate = self.get_all_system_status()
            self.publish_system_status(astate)
        except Exception as e:
            logger.log_error(f"Failed to update initial system status: {e}")

        from queue import Empty
        # Queue to receive the STATEDB and Systemd state change event
        while True:
            try:
                msg = self.myQ.get(timeout=QUEUE_TIMEOUT)
                if msg == "stop":
                    break
                event = msg["unit"]
                event_src = msg["evt_src"]
                event_time = msg["time"]
                logger.log_debug("Main process- received event:{} from source:{} time:{}".format(event,event_src,event_time))
                logger.log_info("check_unit_status for [ "+event+" ] ")
                if event_src == 'timeout':
                    # If got timeout, so not all daemons/apps reported their
                    # ready status. It mean the system is in "NOT OK" state.
                    # But we can't assume it is failed, so it is just down so
                    # far.
                    self.publish_system_status('DOWN')
                else:
                    self.check_unit_status(event)

                    # Recheck the down services, cause they may not update dbus
                    for service in self.dnsrvs_name.copy():
                        self.check_unit_status(service)
            except (Empty, EOFError):
                pass
            except Exception as e:
                logger.log_error("system_service"+str(e))

        #cleanup tables  "'ALL_SERVICE_STATUS*', 'SYSTEM_READY*'" from statedb
        self.state_db.delete_all_by_pattern(self.state_db.STATE_DB,
                                            "ALL_SERVICE_STATUS|*")
        self.state_db.delete_all_by_pattern(self.state_db.STATE_DB,
                                            "SYSTEM_READY|*")
        self.state_db.delete_all_by_pattern(self.state_db.STATE_DB,
                                            "SERVICE_APP|*")

        monitor_system_bus.task_stop()
        monitor_statedb_table.task_stop()
        self.monitor_timeout.task_stop()

    def task_worker(self):
        self.system_service()

    def task_stop(self):
        self.myQ.put("stop")

        # Wait for the process to exit
        self._task_process.join(self._stop_timeout_secs)

        # If the process didn't exit, attempt to kill it
        if self._task_process.is_alive():
            logger.log_notice("Attempting to kill sysmon main process with pid {}".format(self._task_process.pid))
            self._task_process.kill()
            self._task_process.join()
            return False

        return True
