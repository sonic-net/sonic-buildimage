#!/usr/bin/env python3

import fcntl
import json
import os
import signal
import subprocess
import sys
from contextlib import contextmanager, suppress
from http.client import HTTPException
from time import monotonic, sleep

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
LAUNCHER = "/usr/bin/gnmi-native.sh"
PREPARED_CONFIG_FILE = "/run/gnmi-config.json"
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
        self.rejected_config = None

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

    @contextmanager
    def prepared_config(self, config):
        snapshot = {name: {} for name in WATCHED}
        for (name, key), values in config.items():
            if values:
                snapshot[name][key] = values
        flags = os.O_WRONLY | os.O_CREAT | os.O_TRUNC | os.O_NOFOLLOW
        fd = os.open(PREPARED_CONFIG_FILE, flags, 0o600)
        try:
            with os.fdopen(fd, "w") as candidate:
                os.fchmod(candidate.fileno(), 0o600)
                json.dump(snapshot, candidate)
                candidate.flush()
                subprocess.run(
                    [LAUNCHER, "--check-config", PREPARED_CONFIG_FILE],
                    check=True, text=True, stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                )
                os.close(os.open(PREPARED_CONFIG_FILE + ".read", flags, 0o600))
                # Only a live lock publishes the validated input to the launcher.
                fcntl.flock(candidate, fcntl.LOCK_EX)
                yield
        finally:
            for path in (PREPARED_CONFIG_FILE, PREPARED_CONFIG_FILE + ".read"):
                with suppress(FileNotFoundError):
                    os.unlink(path)

    def wait_for_config_read(self, supervisor):
        process = supervisor.getProcessInfo(PROGRAM)
        pid = process["pid"]
        # Supervisor RUNNING can precede the launcher's first input read.
        while process["statename"] == "RUNNING" and process["pid"] == pid:
            with open(PREPARED_CONFIG_FILE + ".read") as received:
                if received.read().strip() == str(pid):
                    return
            sleep(0.1)
            process = supervisor.getProcessInfo(PROGRAM)
        raise OSError("gNMI process changed before reading prepared config")

    def restart_if_changed(self):
        self.deadline = None
        config = self.read_config()
        if config == self.config and not self.force_restart:
            if (self.rejected_config is not None
                    and config != self.rejected_config):
                logger.log_notice("gNMI rejected config withdrawn")
                self.rejected_config = None
            return
        if config == self.rejected_config and not self.force_restart:
            return
        self.drain_after_restart = False
        try:
            with getRPCInterface(os.environ) as proxy:
                supervisor = proxy.supervisor
                process = supervisor.getProcessInfo(PROGRAM)
                state = process["statename"]
                if state == "STARTING":
                    self.deadline = monotonic() + COALESCE_SECONDS
                    return
                self.force_restart = False
                if state != "RUNNING":
                    self.config = config
                    logger.log_notice("Skipping gNMI config restart: " + state)
                    return
                with self.prepared_config(config):
                    current = supervisor.getProcessInfo(PROGRAM)
                    if (current["statename"] != "RUNNING"
                            or current.get("pid") != process.get("pid")):
                        self.config = config
                        logger.log_notice(
                            "Skipping gNMI config restart: process changed"
                        )
                        return
                    # Preserve queued reverts while starting this exact snapshot.
                    self.config = config
                    self.rejected_config = None
                    logger.log_notice(
                        "gNMI config validated; requesting supervisor restart"
                    )
                    supervisor.stopProcess(PROGRAM, True)
                    supervisor.startProcess(PROGRAM, True)
                    self.wait_for_config_read(supervisor)
                    self.drain_after_restart = True
        except subprocess.CalledProcessError as error:
            self.rejected_config = config
            self.force_restart = False
            logger.log_warning(
                "gNMI config unapplied; keeping current runtime: "
                + (error.stdout or str(error)).strip()
            )
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
