#!/usr/bin/env python3

import datetime
import inspect
import json
import os
import sys
import syslog
import subprocess
from collections import defaultdict
from ctrmgr.ctrmgr_iptables import iptable_proxy_rule_upd

from swsscommon import swsscommon
from sonic_py_common import device_info

sys.path.append(os.path.dirname(os.path.realpath(__file__)))
import kube_commands

UNIT_TESTING = 0
UNIT_TESTING_ACTIVE = 0

# Kube config file
SONIC_CTR_CONFIG = "/etc/sonic/remote_ctr.config.json"

CONFIG_DB_NAME = "CONFIG_DB"
STATE_DB_NAME = "STATE_DB"

# DB SERVER
SERVER_TABLE = "KUBERNETES_MASTER"
SERVER_KEY = "SERVER"
CFG_SER_IP = "ip"
CFG_SER_PORT = "port"
CFG_SER_DISABLE = "disable"
CFG_SER_INSECURE = "insecure"

ST_SER_IP = "ip"
ST_SER_PORT = "port"
ST_SER_CONNECTED = "connected"
ST_SER_UPDATE_TS = "update_time"

# DB FEATURE
FEATURE_TABLE = "FEATURE"
CFG_FEAT_OWNER = "set_owner"
CFG_FEAT_NO_FALLBACK = "no_fallback_to_local"
CFG_FEAT_FAIL_DETECTION = "remote_fail_detection"

ST_FEAT_OWNER = "current_owner"
ST_FEAT_UPDATE_TS = "update_time"
ST_FEAT_CTR_ID = "container_id"
ST_FEAT_CTR_VER = "container_version"
ST_FEAT_CTR_STABLE_VER = "container_stable_version"
ST_FEAT_CTR_LAST_VER = "container_last_version"
ST_FEAT_REMOTE_STATE = "remote_state"
ST_FEAT_SYS_STATE = "system_state"

KUBE_LABEL_TABLE = "KUBE_LABELS"
KUBE_LABEL_SET_KEY = "SET"

MODE_KUBE = "kube"
MODE_LOCAL = "local"
OWNER_KUBE = "kube"
OWNER_LOCAL = "local"
OWNER_NONE = "none"
REMOTE_RUNNING = "running"
REMOTE_READY = "ready"
REMOTE_PENDING = "pending"
REMOTE_STOPPED = "stopped"
REMOTE_NONE = "none"

remote_connected = False

dflt_cfg_ser = {
        CFG_SER_IP: "",
        CFG_SER_PORT: "6443",
        CFG_SER_DISABLE: "false",
        CFG_SER_INSECURE: "true"
        }

dflt_st_ser = {
        ST_SER_IP: "",
        ST_SER_PORT: "",
        ST_SER_CONNECTED: "",
        ST_SER_UPDATE_TS: ""
        }

dflt_cfg_feat= {
        CFG_FEAT_OWNER: "local",
        CFG_FEAT_NO_FALLBACK: "false",
        CFG_FEAT_FAIL_DETECTION: "300"
        }

dflt_st_feat= {
        ST_FEAT_OWNER: "none",
        ST_FEAT_UPDATE_TS: "",
        ST_FEAT_CTR_ID: "",
        ST_FEAT_CTR_VER: "",
        ST_FEAT_CTR_STABLE_VER: "",
        ST_FEAT_CTR_LAST_VER: "",
        ST_FEAT_REMOTE_STATE: "none",
        ST_FEAT_SYS_STATE: ""
        }

JOIN_LATENCY = "join_latency_on_boot_seconds"
JOIN_RETRY = "retry_join_interval_seconds"
LABEL_RETRY = "retry_labels_update_seconds"
TAG_IMAGE_LATEST = "tag_latest_image_on_wait_seconds"
TAG_RETRY = "retry_tag_latest_seconds"
CLEAN_IMAGE_RETRY = "retry_clean_image_seconds"
USE_K8S_PROXY = "use_k8s_as_http_proxy"

remote_ctr_config = {
    JOIN_LATENCY: 10,
    JOIN_RETRY: 10,
    LABEL_RETRY: 2,
    TAG_IMAGE_LATEST: 5,
    TAG_RETRY: 5,
    CLEAN_IMAGE_RETRY: 5,
    USE_K8S_PROXY: ""
    }

DISABLED_FEATURE_SET = {"database"}

def log_debug(m):
    msg = "{}: {}".format(inspect.stack()[1][3], m)
    syslog.syslog(syslog.LOG_DEBUG, msg)


def log_error(m):
    msg = "{}: {}".format(inspect.stack()[1][3], m)
    syslog.syslog(syslog.LOG_ERR, msg)


def log_info(m):
    msg = "{}: {}".format(inspect.stack()[1][3], m)
    syslog.syslog(syslog.LOG_INFO, msg)


def ts_now():
    return str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))


def is_systemd_active(feat):
    if not UNIT_TESTING:
        status = subprocess.call(['systemctl', 'is-active', '--quiet', str(feat)])
    else:
        status = UNIT_TESTING_ACTIVE
    log_debug("system status for {}: {}".format(feat, str(status)))
    return status == 0


def restart_systemd_service(server, feat, owner):
    log_debug("Restart service {} to owner:{}".format(feat, owner))
    if not UNIT_TESTING:
        subprocess.call(["systemctl", "reset-failed", str(feat)])
        status = subprocess.call(["systemctl", "restart", str(feat)])
    else:
        server.mod_db_entry(STATE_DB_NAME,
                FEATURE_TABLE, feat, {"restart": "true"})
        status = 0
    if status != 0:
        syslog.syslog(syslog.LOG_ERR,
                "Failed to restart {} to switch to {}".format(feat, owner))
    else:
        syslog.syslog(syslog.LOG_INFO,
                "Restarted {} to switch to {}".format(feat, owner))
    return status


def init():
    if os.path.exists(SONIC_CTR_CONFIG):
        with open(SONIC_CTR_CONFIG, "r") as s:
            d = json.load(s)
            remote_ctr_config.update(d)

class MainServer:
    """ Implements main io-loop of the application
        Accept handler registration per db/table.
        Call handler on update
    """

    SELECT_TIMEOUT = 1000

    def __init__(self):
        """ Constructor """
        self.db_connectors = {}
        self.selector = swsscommon.Select()
        self.callbacks = defaultdict(lambda: defaultdict(list))  # db -> table -> handlers[]
        self.timer_handlers = defaultdict(list)
        self.subscribers = set()

    def register_db(self, db_name):
        """ Get DB connector, if not there """
        if db_name not in self.db_connectors:
            self.db_connectors[db_name] = swsscommon.DBConnector(db_name, 0)


    def register_timer(self, ts, handler, args=None):
        """ Register timer based handler.
            The handler will be called on/after give timestamp, ts
        """
        self.timer_handlers[ts].append((handler, args))


    def register_handler(self, db_name, table_name, handler):
        """
        Handler registration for any update in given table
        in given db. The handler will be called for any update
        to the table in this db.
        """
        self.register_db(db_name)

        if table_name not in self.callbacks[db_name]:
            conn = self.db_connectors[db_name]
            subscriber = swsscommon.SubscriberStateTable(conn, table_name)
            self.subscribers.add(subscriber)
            self.selector.addSelectable(subscriber)
        self.callbacks[db_name][table_name].append(handler)


    def get_db_entry(self, db_name, table_name, key):
        """ Return empty dict if key not present """
        conn = self.db_connectors[db_name]
        tbl = swsscommon.Table(conn, table_name)
        return dict(tbl.get(key)[1])


    def mod_db_entry(self, db_name, table_name, key, data):
        """ Modify entry for given table|key with given dict type data """
        conn = self.db_connectors[db_name]
        tbl = swsscommon.Table(conn, table_name)
        log_debug("mod_db_entry: db={} tbl={} key={} data={}".format(db_name, table_name, key, str(data)))
        tbl.set(key, list(data.items()))


    def set_db_entry(self, db_name, table_name, key, data):
        """ Set given data as complete data, which includes
            removing any fields that are in DB but not in data
        """
        conn = self.db_connectors[db_name]
        tbl = swsscommon.Table(conn, table_name)
        ct_data = dict(tbl.get(key)[1])
        for k in ct_data:
            if k not in data:
                # Drop fields that are not in data
                tbl.hdel(key, k)
        tbl.set(key, list(data.items()))


    def run(self):
        """ Main loop """
        while True:
            timeout = MainServer.SELECT_TIMEOUT
            ct_ts = datetime.datetime.now()
            while self.timer_handlers:
                k = sorted(self.timer_handlers.keys())[0]
                if k <= ct_ts:
                    lst = self.timer_handlers[k]
                    del self.timer_handlers[k]
                    for fn in lst:
                        if fn[1] is None:
                            fn[0]()
                        else:
                            fn[0](*fn[1])
                else:
                    timeout = (k - ct_ts).seconds
                    break

            state, _ = self.selector.select(timeout)
            if state == self.selector.TIMEOUT:
                continue
            elif state == self.selector.ERROR:
                if not UNIT_TESTING:
                    raise Exception("Received error from select")
                else:
                    log_debug("Skipped Exception; Received error from select")
                    return

            for subscriber in self.subscribers:
                key, op, fvs = subscriber.pop()
                if not key:
                    continue
                if subscriber.getTableName() == FEATURE_TABLE and key in DISABLED_FEATURE_SET:
                    continue
                log_debug("Received message : '%s'" % str((key, op, fvs)))
                for callback in (self.callbacks
                        [subscriber.getDbConnector().getDbName()]
                        [subscriber.getTableName()]):
                    callback(key, op, dict(fvs))



def set_node_labels(server):
    labels = {}

    version_info = (device_info.get_sonic_version_info() if not UNIT_TESTING
            else { "build_version": "20201230.111"})
    dev_data = server.get_db_entry(CONFIG_DB_NAME, 'DEVICE_METADATA',
            'localhost')
    dep_type = dev_data['type'] if 'type' in dev_data else "unknown"

    labels["sonic_version"] = version_info['build_version']
    labels["hwsku"] = device_info.get_hwsku() if not UNIT_TESTING else "mock"
    labels["deployment_type"] = dep_type
    platform = device_info.get_platform()
    labels["worker.sonic/platform"] = platform if platform is not None else ""
    server.mod_db_entry(STATE_DB_NAME,
            KUBE_LABEL_TABLE, KUBE_LABEL_SET_KEY, labels)


def _update_entry(ct, upd):
    # Helper function, to update with case lowered.
    ret = dict(ct)
    for (k, v) in upd.items():
        ret[k.lower()] = v.lower()

    return ret


#
# SERVER-config changes:
#   Act if IP or disable changes.
#   If disabled or IP removed:
#       reset connection
#   else:
#       join
#   Update state-DB appropriately
#
class RemoteServerHandler:
    def __init__(self, server):
        """ Register self for updates """
        self.server = server

        server.register_handler(
                CONFIG_DB_NAME, SERVER_TABLE, self.on_config_update)
        self.cfg_server = _update_entry(dflt_cfg_ser, server.get_db_entry(
            CONFIG_DB_NAME, SERVER_TABLE, SERVER_KEY))

        log_debug("startup config: {}".format(str(self.cfg_server)))

        server.register_db(STATE_DB_NAME)
        self.st_server = _update_entry(dflt_st_ser, server.get_db_entry(
            STATE_DB_NAME, SERVER_TABLE, SERVER_KEY))

        self.start_time = datetime.datetime.now()

        if remote_ctr_config[USE_K8S_PROXY] == "y":
            iptable_proxy_rule_upd(self.cfg_server[CFG_SER_IP])

        if not self.st_server[ST_FEAT_UPDATE_TS]:
            # This is upon system start. Sleep 10m before join
            self.start_time += datetime.timedelta(
                    seconds=remote_ctr_config[JOIN_LATENCY])
            server.register_timer(self.start_time, self.handle_update)
            self.pending = True
            log_debug("Pause to join {} seconds @ {}".format(
                remote_ctr_config[JOIN_LATENCY], self.start_time))
        else:
            self.pending = False
            self.handle_update()



    def on_config_update(self, key, op, data):
        """ On config update """
        if key != SERVER_KEY:
            return

        cfg_data = _update_entry(dflt_cfg_ser, data)
        if self.cfg_server == cfg_data:
            log_debug("No change in server config")
            return

        log_debug("Received config update: {}".format(str(data)))
        self.cfg_server = cfg_data

        if remote_ctr_config[USE_K8S_PROXY] == "y":
            iptable_proxy_rule_upd(self.cfg_server[CFG_SER_IP])

        if self.pending:
            tnow = datetime.datetime.now()
            if tnow < self.start_time:
                # Pausing for initial latency since reboot or last retry
                due_secs = (self.start_time - tnow).seconds
            else:
                due_secs = 0
            log_debug("Pending to start in {} seconds at {}".format(
                due_secs, self.start_time))
            return

        self.handle_update()


    def handle_update(self):
        # Called upon start and upon any config-update only.
        # Called by timer / main thread.
        # Main thread calls only if timer is not running.
        #
        self.pending = False

        ip = self.cfg_server[CFG_SER_IP]
        disable = self.cfg_server[CFG_SER_DISABLE] != "false"

        pre_state = dict(self.st_server)
        log_debug("server: handle_update: disable={} ip={}".format(disable, ip))
        if disable or not ip:
            self.do_reset()
        else:
            self.do_join(ip, self.cfg_server[ST_SER_PORT],
                    self.cfg_server[CFG_SER_INSECURE])

        if pre_state != self.st_server:
            self.st_server[ST_SER_UPDATE_TS] = ts_now()
            self.server.set_db_entry(STATE_DB_NAME,
                    SERVER_TABLE, SERVER_KEY, self.st_server)
            log_debug("Update server state={}".format(str(self.st_server)))


    def do_reset(self):
        global remote_connected

        kube_commands.kube_reset_master(True)

        self.st_server[ST_SER_CONNECTED] = "false"
        log_debug("kube_reset_master called")

        remote_connected = False


    def do_join(self, ip, port, insecure):
        global remote_connected
        (ret, out, err) = kube_commands.kube_join_master(
                ip, port, insecure)

        if ret == 0:
            self.st_server[ST_SER_CONNECTED] = "true"
            self.st_server[ST_SER_IP] = ip
            self.st_server[ST_SER_PORT] = port
            remote_connected = True

            set_node_labels(self.server)
            log_debug("kube_join_master succeeded")
        else:
            # Ensure state reflects the current status
            self.st_server[ST_SER_CONNECTED] = "false"
            remote_connected = False

            # Join failed. Retry after an interval
            self.start_time = datetime.datetime.now()
            self.start_time += datetime.timedelta(
                    seconds=remote_ctr_config[JOIN_RETRY])
            self.server.register_timer(self.start_time, self.handle_update)
            self.pending = True

            log_debug("kube_join_master failed retry after {} seconds @{}".
                    format(remote_ctr_config[JOIN_RETRY], self.start_time))

#
# Feature changes
#
#   Handle Set_owner change:
#       restart service and/or label add/drop
#
#   Handle remote_state change:
#       When pending, trigger restart
#
class FeatureTransitionHandler:
    def __init__(self, server):
        self.server = server
        self.cfg_data = defaultdict(lambda: defaultdict(str))
        self.st_data = defaultdict(lambda: defaultdict(str))
        server.register_handler(
                CONFIG_DB_NAME, FEATURE_TABLE, self.on_config_update)
        server.register_handler(
                STATE_DB_NAME, FEATURE_TABLE, self.on_state_update)
        return


    def handle_update(self, feat, set_owner, ct_owner, remote_state):
        # Called upon startup once for every feature in config & state DBs.
        # There after only called upon changes in either that requires action
        #
        if not is_systemd_active(feat):
            # Restart the service manually when kube upgrade happens to decrease the down time
            if set_owner == MODE_KUBE and ct_owner == OWNER_NONE and remote_state == REMOTE_STOPPED:
                restart_systemd_service(self.server, feat, OWNER_KUBE)
            return

        label_add = set_owner == "kube"
        service_restart = False

        if set_owner == "local":
            if ct_owner != "local":
                service_restart = True
        else:
            if (ct_owner != "none") and (remote_state == "pending"):
                service_restart = True

        log_debug(
                "feat={} set={} ct={} state={} restart={} label_add={}".format(
                    feat, set_owner, ct_owner, remote_state, service_restart,
                    label_add))
        # read labels and add/drop if different
        self.server.mod_db_entry(STATE_DB_NAME, KUBE_LABEL_TABLE, KUBE_LABEL_SET_KEY,
                { "{}_enabled".format(feat): ("true" if label_add else "false") })


        # service_restart
        if service_restart:
            restart_systemd_service(self.server, feat, set_owner)



    def on_config_update(self, key, op, data):
        # Hint/Note:
        # If the key don't pre-exist:
        #   This attempt to read will create the key for given
        #   field with empty string as value and return empty string

        init = key not in self.cfg_data
        old_set_owner = self.cfg_data[key][CFG_FEAT_OWNER]

        self.cfg_data[key] = _update_entry(dflt_cfg_feat, data)
        set_owner = self.cfg_data[key][CFG_FEAT_OWNER]

        if (not init) and (old_set_owner == set_owner):
            # No change, bail out
            log_debug("No change in feat={} set_owner={}. Bail out.".format(
                key, set_owner))
            return

        if key in self.st_data:
            log_debug("{} init={} old_set_owner={} owner={}".format(key, init, old_set_owner, set_owner))
            self.handle_update(key, set_owner,
                    self.st_data[key][ST_FEAT_OWNER],
                    self.st_data[key][ST_FEAT_REMOTE_STATE])

        else:
            # Handle it upon data from state-db, which must come
            # if systemd is up
            log_debug("Yet to get state info key={}. Bail out.".format(key))
            return


    def on_state_update(self, key, op, data):
        # Hint/Note:
        # If the key don't pre-exist:
        #   This attempt to read will create the key for given
        #   field with empty string as value and return empty string

        init = key not in self.st_data
        old_remote_state = self.st_data[key][ST_FEAT_REMOTE_STATE]

        self.st_data[key] = _update_entry(dflt_st_feat, data)
        remote_state = self.st_data[key][ST_FEAT_REMOTE_STATE]
        current_owner = self.st_data[key][ST_FEAT_OWNER]

        if (remote_state == REMOTE_RUNNING) and (old_remote_state != remote_state):
            # Tag latest
            start_time = datetime.datetime.now() + datetime.timedelta(
                    seconds=remote_ctr_config[TAG_IMAGE_LATEST])
            self.server.register_timer(start_time, self.do_tag_latest, (
                    key,
                    self.st_data[key][ST_FEAT_CTR_ID],
                    self.st_data[key][ST_FEAT_CTR_VER]))

            log_debug("try to tag latest label after {} seconds @{}".format(
                    remote_ctr_config[TAG_IMAGE_LATEST], start_time))
        
        # This is for going back to local without waiting the systemd restart time
        # when k8s is down, can't deploy containers to worker and need to go back to local
        # if current owner is already local, we don't do restart
        if (current_owner != OWNER_LOCAL) and (remote_state == REMOTE_NONE) and (old_remote_state == REMOTE_STOPPED):
            restart_systemd_service(self.server, key, OWNER_LOCAL)
            return

        if (not init):
            if (old_remote_state == remote_state):
            # if no remote state change, do nothing.
                return
            if (remote_state not in (REMOTE_PENDING, REMOTE_STOPPED)):
            # if remote state not in pending or stopped, do nothing.
                return

        if key in self.cfg_data:
            log_debug("{} init={} old_remote_state={} remote_state={}".format(key, init, old_remote_state, remote_state))
            self.handle_update(key, self.cfg_data[key][CFG_FEAT_OWNER],
                    self.st_data[key][ST_FEAT_OWNER],
                    remote_state)
        return

    def do_tag_latest(self, feat, docker_id, image_ver):
        ret = kube_commands.tag_latest(feat, docker_id, image_ver)
        if ret == 1:
            # Tag latest failed. Retry after an interval
            self.start_time = datetime.datetime.now()
            self.start_time += datetime.timedelta(
                    seconds=remote_ctr_config[TAG_RETRY])
            self.server.register_timer(self.start_time, self.do_tag_latest, (feat, docker_id, image_ver))

            log_debug("Tag latest as local failed retry after {} seconds @{}".
                    format(remote_ctr_config[TAG_RETRY], self.start_time))
        elif ret == 0:
            last_version = self.st_data[feat][ST_FEAT_CTR_STABLE_VER]
            if last_version == image_ver:
                last_version = self.st_data[feat][ST_FEAT_CTR_LAST_VER]
            self.server.mod_db_entry(STATE_DB_NAME, FEATURE_TABLE, feat,
                {ST_FEAT_CTR_STABLE_VER: image_ver,
                ST_FEAT_CTR_LAST_VER: last_version})
            self.st_data[ST_FEAT_CTR_LAST_VER] = last_version
            self.st_data[ST_FEAT_CTR_STABLE_VER] = image_ver
            self.do_clean_image(feat, image_ver, last_version)
        elif ret == -1:
            # This means the container we want to tag latest is not running
            # so we don't need to do clean up
            pass

    def do_clean_image(self, feat, current_version, last_version):
        ret = kube_commands.clean_image(feat, current_version, last_version)
        if ret != 0:
            # Clean up old version images failed. Retry after an interval
            self.start_time = datetime.datetime.now()
            self.start_time += datetime.timedelta(
                    seconds=remote_ctr_config[CLEAN_IMAGE_RETRY])
            self.server.register_timer(self.start_time, self.do_clean_image, (feat, current_version, last_version))

            log_debug("Clean up old version images failed retry after {} seconds @{}".
                    format(remote_ctr_config[CLEAN_IMAGE_RETRY], self.start_time))


#
# Label re-sync
#   When labels applied at remote are pending, start monitoring
#   API server reachability and trigger the label handler to
#   apply the labels
#
#   Non-empty KUBE_LABELS|PENDING, monitor API Server reachability
#
class LabelsPendingHandler:
    def __init__(self, server):
        server.register_handler(STATE_DB_NAME, KUBE_LABEL_TABLE, self.on_update)
        self.server = server
        self.pending = False
        self.set_labels = {}
        return


    def on_update(self, key, op, data):
        # For any update sync with kube API server.
        # Don't optimize, as API server could differ with DB's contents
        # in many ways.
        #
        if (key == KUBE_LABEL_SET_KEY):
            self.set_labels = dict(data)
        else:
            return

        if remote_connected and not self.pending:
            self.update_node_labels()
        else:
            log_debug("Skip label update: connected:{} pending:{}".
                    format(remote_connected, self.pending))



    def update_node_labels(self):
        # Called on config update by main thread upon init or config-change or
        # it was not connected during last config update
        # NOTE: remote-server-handler forces a config update notification upon
        # join.
        # Or it could be called by timer thread, if last upate to API server
        # failed.
        #
        self.pending = False
        ret = kube_commands.kube_write_labels(self.set_labels)
        if (ret != 0):
            self.pending = True
            pause = remote_ctr_config[LABEL_RETRY]
            ts = (datetime.datetime.now() + datetime.timedelta(seconds=pause))
            self.server.register_timer(ts, self.update_node_labels)

        log_debug("ret={} set={} pending={}".format(ret,
            str(self.set_labels), self.pending))
        return


def main():
    init()
    server = MainServer()
    RemoteServerHandler(server)
    FeatureTransitionHandler(server)
    LabelsPendingHandler(server)
    server.run()
    log_debug("ctrmgrd.py main called")
    return 0


if __name__ == '__main__':
    if os.geteuid() != 0:
        exit("Please run as root. Exiting ...")
    main()
