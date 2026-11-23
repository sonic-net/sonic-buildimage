#!/usr/bin/env python3

import os
import signal
import sys
from http.client import HTTPException
from time import monotonic

from sonic_py_common.logger import Logger
from supervisor.childutils import getRPCInterface
from supervisor.compat import xmlrpclib
from swsscommon import swsscommon


WATCHED = {
    "GNMI": {"gnmi": None, "certs": None},
    "DEVICE_METADATA": {
        "x509": None,
        "localhost": ("type", "switch_type", "subtype"),
    },
    "MGMT_VRF_CONFIG": {"vrf_global": ("mgmtVrfEnabled",)},
}
COALESCE_SECONDS = 1
PROGRAM = "gnmi-native"
logger = Logger("gnmi-config-listener")
logger.set_min_log_priority_info()


class GnmiConfigListener:
    def __init__(self):
        self.db = swsscommon.DBConnector("CONFIG_DB", 0, False)
        self.tables = {
            name: swsscommon.Table(self.db, name) for name in WATCHED
        }
        self.subscribers = {
            name: swsscommon.SubscriberStateTable(self.db, name)
            for name in WATCHED
        }
        self.selector = swsscommon.Select()
        for subscriber in self.subscribers.values():
            self.selector.addSelectable(subscriber)
        # Subscribe before the baseline; initial table dumps are not changes.
        self.config = self.read_config()
        self.deadline = None
        self.force_restart = False
        self.drain_after_restart = False

    def read_config(self):
        config = {}
        for name, entries in WATCHED.items():
            for key, fields in entries.items():
                _, values = self.tables[name].get(key)
                values = dict(values)
                if fields is not None:
                    values = {
                        field: value for field, value in values.items()
                        if field in fields
                    }
                config[name, key] = values
        return config

    def restart_if_changed(self):
        self.deadline = None
        config = self.read_config()
        if config == self.config and not self.force_restart:
            return
        self.drain_after_restart = False
        try:
            with getRPCInterface(os.environ) as proxy:
                supervisor = proxy.supervisor
                state = supervisor.getProcessInfo(PROGRAM)["statename"]
                if state == "STARTING":
                    self.deadline = monotonic() + COALESCE_SECONDS
                    return
                # Queued mutations can revert to this snapshot during stop/start.
                self.config = config
                self.force_restart = False
                if state != "RUNNING":
                    logger.log_notice("Skipping gNMI config restart: " + state)
                    return
                logger.log_notice(
                    "gNMI config changed; requesting supervisor restart"
                )
                supervisor.stopProcess(PROGRAM, True)
                supervisor.startProcess(PROGRAM, True)
                self.drain_after_restart = True
        except (OSError, xmlrpclib.Error, HTTPException) as error:
            self.config = config
            self.force_restart = False
            logger.log_error("gNMI supervisor restart failed: " + str(error))

    def run(self):
        while True:
            state, _ = self.selector.select(
                0 if self.drain_after_restart else 1000
            )
            if state == swsscommon.Select.ERROR:
                raise RuntimeError("gNMI config subscription select failed")
            if state == swsscommon.Select.OBJECT:
                for name, subscriber in self.subscribers.items():
                    key, _, _ = subscriber.pop()
                    if key in WATCHED[name]:
                        if self.drain_after_restart:
                            self.force_restart = True
                        if self.deadline is None:
                            self.deadline = monotonic() + COALESCE_SECONDS
            elif state == swsscommon.Select.TIMEOUT:
                self.drain_after_restart = False
            if self.deadline is not None and monotonic() >= self.deadline:
                self.restart_if_changed()


def signal_handler(signum, frame):
    sys.exit(0)


if __name__ == "__main__":
    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGINT, signal_handler)
    try:
        GnmiConfigListener().run()
    except Exception as error:
        logger.log_error("gNMI config listener failed: " + str(error))
        sys.exit(1)
