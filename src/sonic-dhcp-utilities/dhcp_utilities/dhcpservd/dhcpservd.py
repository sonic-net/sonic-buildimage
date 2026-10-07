#!/usr/bin/env python
import psutil
import signal
import time
import subprocess
import sys
import syslog
import os
import tempfile
from .dhcp_cfggen import DhcpServCfgGenerator
from .dhcp_lease import LeaseManager
from dhcp_utilities.common.utils import DhcpDbConnector
from dhcp_utilities.common.dhcp_db_monitor import DhcpServdDbMonitor, DhcpServerTableCfgChangeEventChecker, \
    DhcpOptionTableEventChecker, DhcpRangeTableEventChecker, DhcpPortTableEventChecker, VlanIntfTableEventChecker, \
    VlanMemberTableEventChecker, VlanTableEventChecker, MidPlaneTableEventChecker, DpusTableEventChecker, \
    DhcpMatchTableEventChecker, DhcpBindingTableEventChecker
from swsscommon import swsscommon

KEA_DHCP4_CONFIG = "/etc/kea/kea-dhcp4.conf"
KEA_DHCP4_PROC_NAME = "kea-dhcp4"
KEA_VALIDATION_TIMEOUT = 30
KEA_LEASE_FILE_PATH = "/var/lib/kea/kea-lease.csv"
DHCPSERVD_READY_FLAG = "/tmp/dhcpservd_ready"
REDIS_SOCK_PATH = "/var/run/redis/redis.sock"
DHCP_SERVER_IPV4_SERVER_IP = "DHCP_SERVER_IPV4_SERVER_IP"
DHCP_SERVER_INTERFACE = "eth0"
AF_INET = 2
DEFAULT_SELECT_TIMEOUT = 5000  # millisecond
RECOVERY_CHECKERS = {
    "DhcpServerTableCfgChangeEventChecker",
    "DhcpPortTableEventChecker",
    "DhcpMatchTableEventChecker",
    "DhcpBindingTableEventChecker",
    "DhcpOptionTableEventChecker",
    "DhcpRangeTableEventChecker",
    "VlanTableEventChecker",
    "VlanIntfTableEventChecker",
    "VlanMemberTableEventChecker",
    "MidPlaneTableEventChecker",
    "DpusTableEventChecker"
}


class DhcpServd(object):
    enabled_checker = None
    dhcp_servd_monitor = None

    def __init__(self, dhcp_cfg_generator, db_connector, monitor, kea_dhcp4_config_path=KEA_DHCP4_CONFIG,
                 kea_dhcp4_binary=KEA_DHCP4_PROC_NAME):
        self.dhcp_cfg_generator = dhcp_cfg_generator
        self.db_connector = db_connector
        self.kea_dhcp4_config_path = kea_dhcp4_config_path
        self.kea_dhcp4_binary = kea_dhcp4_binary
        self.dhcp_servd_monitor = monitor
        self.enabled_checker = None
        self.used_range = set()
        self.enabled_dhcp_interfaces = set()
        self.enabled_port_interfaces = set()
        self.enabled_match_interfaces = set()
        self.used_options = set()
        self.used_matches = set()
        self.recovery_checkers = set()
        self.reload_pending = False

    def _enable_recovery_checkers(self):
        if self.enabled_checker is None or self.dhcp_servd_monitor is None:
            return
        currently_enabled = self.enabled_checker | self.recovery_checkers
        missing_checkers = RECOVERY_CHECKERS - currently_enabled
        if missing_checkers:
            self.dhcp_servd_monitor.enable_checkers(missing_checkers)
            self.recovery_checkers |= missing_checkers

    def _notify_kea_dhcp4_proc(self):
        """
        Send SIGHUP signal to kea-dhcp4 process
        """
        for proc in psutil.process_iter():
            try:
                if KEA_DHCP4_PROC_NAME in proc.name():
                    proc.send_signal(signal.SIGHUP)
                    break
            except psutil.NoSuchProcess:
                continue

    def dump_dhcp4_config(self):
        """
        Generate and validate a candidate config, then atomically activate it.
        """
        try:
            generation_result = self.dhcp_cfg_generator.generate()
        except ValueError as error:
            syslog.syslog(syslog.LOG_ERR, "Cannot generate Kea candidate config: {}".format(error))
            self.reload_pending = True
            self._enable_recovery_checkers()
            return False
        kea_dhcp4_config, used_ranges, enabled_dhcp_interfaces, used_options, enable_checker, \
            enabled_port_interfaces, enabled_match_interfaces, used_matches = generation_result

        config_dir = os.path.dirname(self.kea_dhcp4_config_path) or "."
        candidate_path = None
        try:
            with tempfile.NamedTemporaryFile(
                mode="w",
                dir=config_dir,
                prefix=".{}.".format(os.path.basename(self.kea_dhcp4_config_path)),
                delete=False
            ) as candidate:
                candidate.write(kea_dhcp4_config)
                candidate.flush()
                os.fsync(candidate.fileno())
                candidate_path = candidate.name

            validation = subprocess.run(
                [self.kea_dhcp4_binary, "-t", candidate_path],
                capture_output=True,
                text=True,
                timeout=KEA_VALIDATION_TIMEOUT
            )
            if validation.returncode != 0:
                validation_output = validation.stderr.strip() or validation.stdout.strip()
                syslog.syslog(
                    syslog.LOG_ERR,
                    "Kea rejected candidate config {}: {}".format(candidate_path, validation_output)
                )
                self.reload_pending = True
                self._enable_recovery_checkers()
                return False

            os.replace(candidate_path, self.kea_dhcp4_config_path)
            candidate_path = None
        except (OSError, subprocess.SubprocessError) as error:
            syslog.syslog(
                syslog.LOG_ERR,
                "Cannot validate or activate Kea candidate config: {}".format(error)
            )
            self.reload_pending = True
            self._enable_recovery_checkers()
            return False
        finally:
            if candidate_path is not None:
                try:
                    os.unlink(candidate_path)
                except FileNotFoundError:
                    pass

        currently_enabled = (self.enabled_checker or set()) | self.recovery_checkers
        if self.enabled_checker is not None and currently_enabled != enable_checker:
            # Has subcribe table and no equal, need to resubscribe
            disabled_checkers = currently_enabled - enable_checker
            enabled_checkers = enable_checker - currently_enabled
            if disabled_checkers:
                self.dhcp_servd_monitor.disable_checkers(disabled_checkers)
            if enabled_checkers:
                self.dhcp_servd_monitor.enable_checkers(enabled_checkers)
        self.enabled_checker = enable_checker
        self.recovery_checkers = set()
        self.reload_pending = False
        self.used_range = used_ranges
        self.enabled_dhcp_interfaces = enabled_dhcp_interfaces
        self.enabled_port_interfaces = enabled_port_interfaces
        self.enabled_match_interfaces = enabled_match_interfaces
        self.used_options = used_options
        self.used_matches = used_matches
        # After refresh kea-config, we need to SIGHUP kea-dhcp4 process to read new config
        self._notify_kea_dhcp4_proc()
        return True

    def _update_dhcp_server_ip(self):
        """
        Add ip address of "eth0" inside dhcp_server container as dhcp_server_ip into state_db
        """
        dhcp_server_ip = None
        for _ in range(10):
            dhcp_interface = psutil.net_if_addrs().get(DHCP_SERVER_INTERFACE, [])
            for address in dhcp_interface:
                if address.family == AF_INET:
                    dhcp_server_ip = address.address
                    self.db_connector.state_db.hset("{}|{}".format(DHCP_SERVER_IPV4_SERVER_IP, DHCP_SERVER_INTERFACE),
                                                    "ip", dhcp_server_ip)
                    return
            else:
                syslog.syslog(syslog.LOG_WARNING, "Cannot get ip address of {}, retry in 5s".format(DHCP_SERVER_INTERFACE))
                time.sleep(5)
        syslog.syslog(syslog.LOG_ERR, "Failed to get ip address of {} after 10 retries, exiting".format(DHCP_SERVER_INTERFACE))
        sys.exit(1)

    def start(self):
        start_time = time.time()
        syslog.syslog(syslog.LOG_INFO, "dhcpservd starting")
        if not self.dump_dhcp4_config():
            syslog.syslog(syslog.LOG_ERR, "Initial Kea configuration validation failed, exiting")
            sys.exit(1)
        syslog.syslog(syslog.LOG_INFO, "dump_dhcp4_config done, elapsed=%.3fs" % (time.time() - start_time))
        self._update_dhcp_server_ip()
        syslog.syslog(syslog.LOG_INFO, "update_dhcp_server_ip done, elapsed=%.3fs" % (time.time() - start_time))
        self.dhcp_servd_monitor.enable_checkers(self.enabled_checker)
        lease_manager = LeaseManager(self.db_connector, KEA_LEASE_FILE_PATH)
        lease_manager.start()
        self._signal_readiness()
        syslog.syslog(syslog.LOG_INFO, "SIGUSR1 handler registered, ready flag written, total startup=%.3fs" % (time.time() - start_time))

    def _signal_readiness(self):
        """Write readiness flag so wait_for_dhcpservd.sh can gate kea-dhcp4 startup."""
        try:
            with open(DHCPSERVD_READY_FLAG, "w") as f:
                f.write(str(os.getpid()))
        except OSError as err:
            syslog.syslog(
                syslog.LOG_ERR,
                "Failed to write readiness flag {}: {}, exiting".format(DHCPSERVD_READY_FLAG, err)
            )
            sys.exit(1)

    def wait(self):
        while True:
            db_snapshot = {} if self.reload_pending else {
                "enabled_dhcp_interfaces": self.enabled_dhcp_interfaces,
                "enabled_port_interfaces": self.enabled_port_interfaces,
                "enabled_match_interfaces": self.enabled_match_interfaces,
                "used_range": self.used_range,
                "used_options": self.used_options,
                "used_matches": self.used_matches
            }
            res = self.dhcp_servd_monitor.check_db_update(db_snapshot)
            if res:
                self.dump_dhcp4_config()


def main():
    dhcp_db_connector = DhcpDbConnector(redis_sock=REDIS_SOCK_PATH)
    hook_lib_path_res = subprocess.run(["find", "/", "-name", "libdhcp_run_script.so"],
                                       capture_output=True).stdout.decode().strip()
    if len(hook_lib_path_res) == 0:
        syslog.syslog(syslog.LOG_ERR, "Cannot find hook lib for kea-dhcp-server")
        sys.exit(1)
    dhcp_cfg_generator = DhcpServCfgGenerator(dhcp_db_connector, hook_lib_path_res.split("\n")[0])
    sel = swsscommon.Select()
    checkers = []
    checkers.append(DhcpServerTableCfgChangeEventChecker(sel, dhcp_db_connector.config_db))
    checkers.append(DhcpPortTableEventChecker(sel, dhcp_db_connector.config_db))
    checkers.append(DhcpMatchTableEventChecker(sel, dhcp_db_connector.config_db))
    checkers.append(DhcpBindingTableEventChecker(sel, dhcp_db_connector.config_db))
    checkers.append(DhcpOptionTableEventChecker(sel, dhcp_db_connector.config_db))
    checkers.append(DhcpRangeTableEventChecker(sel, dhcp_db_connector.config_db))
    checkers.append(VlanTableEventChecker(sel, dhcp_db_connector.config_db))
    checkers.append(VlanIntfTableEventChecker(sel, dhcp_db_connector.config_db))
    checkers.append(VlanMemberTableEventChecker(sel, dhcp_db_connector.config_db))
    checkers.append(DpusTableEventChecker(sel, dhcp_db_connector.config_db))
    checkers.append(MidPlaneTableEventChecker(sel, dhcp_db_connector.config_db))
    dhcp_servd_monitor = DhcpServdDbMonitor(dhcp_db_connector, sel, checkers, DEFAULT_SELECT_TIMEOUT)
    dhcpservd = DhcpServd(dhcp_cfg_generator, dhcp_db_connector, dhcp_servd_monitor)
    dhcpservd.start()
    dhcpservd.wait()


if __name__ == "__main__":
    main()
