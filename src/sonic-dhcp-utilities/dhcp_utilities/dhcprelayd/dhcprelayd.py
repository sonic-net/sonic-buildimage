# TODO Add support for running different dhcrelay processes for each dhcp interface
# Currently if we run multiple dhcrelay processes, except for the last running process,
# others will not relay dhcp_release packet.
import psutil
import re
import subprocess
import sys
import syslog
import time
from swsscommon import swsscommon
from dhcp_utilities.common.utils import DhcpDbConnector, terminate_proc, is_smart_switch
from dhcp_utilities.common.dhcp_db_monitor import DhcpRelaydDbMonitor, DhcpServerTableIntfEnablementEventChecker, \
     VlanTableEventChecker, VlanIntfTableEventChecker, DhcpServerFeatureStateChecker, MidPlaneTableEventChecker, \
     DhcpV4RelayTableEventChecker

REDIS_SOCK_PATH = "/var/run/redis/redis.sock"
SUPERVISORD_CONF_PATH = "/etc/supervisor/conf.d/docker-dhcp-relay.supervisord.conf"
DHCP_SERVER_IPV4_SERVER_IP = "DHCP_SERVER_IPV4_SERVER_IP"
DHCP_SERVER_IPV4 = "DHCP_SERVER_IPV4"
DHCPV4_RELAY = "DHCPV4_RELAY"
VLAN = "VLAN"
MID_PLANE_BRIDGE = "MID_PLANE_BRIDGE"
DEVICE_METADATA = "DEVICE_METADATA"
DEFAULT_SELECT_TIMEOUT = 5000  # millisecond
DHCP_SERVER_INTERFACE = "eth0"
FEATURE_CHECKER = "DhcpServerFeatureStateChecker"
DHCP_SERVER_CHECKER = "DhcpServerTableIntfEnablementEventChecker"
DHCPV4_RELAY_CHECKER = "DhcpV4RelayTableEventChecker"
VLAN_CHECKER = "VlanTableEventChecker"
VLAN_INTF_CHECKER = "VlanIntfTableEventChecker"
MID_PLANE_CHECKER = "MidPlaneTableEventChecker"
VLAN_CHECKERS = [VLAN_CHECKER, VLAN_INTF_CHECKER]
KILLED_OLD = 1
NOT_KILLED = 2
NOT_FOUND_PROC = 3


class DhcpRelayd(object):
    enabled_dhcp_interfaces = set()
    dhcp_server_feature_enabled = None
    dhcp_relay_supervisor_config = {}
    supervisord_conf_path = ""
    enabled_checkers = set()
    smart_switch = False
    has_sonic_dhcpv4_relay = False
    sonic_dhcp_server_relay_active = False

    def __init__(self, db_connector, db_monitor, supervisord_conf_path=SUPERVISORD_CONF_PATH,
                 enabled_checkers=[FEATURE_CHECKER]):
        """
        Args:
            db_connector: db connector obj
            select_timeout: timeout setting for subscribe db change
        """
        self.db_connector = db_connector
        self.last_refresh_time = None
        self.dhcp_relayd_monitor = db_monitor
        self.enabled_dhcp_interfaces = set()
        self.dhcp_server_feature_enabled = None
        self.supervisord_conf_path = supervisord_conf_path
        self.enabled_checkers = set(enabled_checkers)
        self.has_sonic_dhcpv4_relay = False
        self.sonic_dhcp_server_relay_active = False

    def start(self):
        """
        Start function
        """
        self.dhcp_relay_supervisor_config = self._get_dhcp_relay_config()
        self.dhcp_server_feature_enabled = self._is_dhcp_server_enabled()
        device_metadata = self.db_connector.get_config_db_table(DEVICE_METADATA)
        self.smart_switch = is_smart_switch(device_metadata)
        self.has_sonic_dhcpv4_relay = \
            device_metadata.get("localhost", {}).get("has_sonic_dhcpv4_relay", "False") == "True"
        if self.has_sonic_dhcpv4_relay:
            self.enabled_checkers.add(DHCPV4_RELAY_CHECKER)
        # Sleep to wait dhcrelay process start
        time.sleep(5)
        if self.dhcp_server_feature_enabled:
            # ISC local mode always replaces the supervisor-owned relay. SONiC takes
            # ownership only after an enabled local DHCP interface is configured.
            if not self.has_sonic_dhcpv4_relay:
                self._execute_supervisor_dhcp_relay_process("stop")
            self.enabled_checkers.add(DHCP_SERVER_CHECKER)
        self.dhcp_relayd_monitor.enable_checkers(self.enabled_checkers)
        if self.dhcp_server_feature_enabled:
            self.refresh_dhcrelay()

    def refresh_dhcrelay(self, force_kill=False):
        """
        To refresh dhcrelay/dhcpmon process (start or restart)
        Args:
            force_kill: if True, force kill old processes
        """
        syslog.syslog(syslog.LOG_INFO, "Start to refresh dhcrelay related processes")
        dhcp_server_ipv4_table = self.db_connector.get_config_db_table(DHCP_SERVER_IPV4)
        vlan_table = self.db_connector.get_config_db_table(VLAN)
        mid_plane_table = self.db_connector.get_config_db_table(MID_PLANE_BRIDGE)
        mid_plane_bridge_name = mid_plane_table.get("GLOBAL", {}).get("bridge", None)

        dhcp_interfaces = set()
        self.enabled_dhcp_interfaces = set()
        checkers_to_be_enabled = set()
        for dhcp_interface, config in dhcp_server_ipv4_table.items():
            # Reason for add to enabled_dhcp_interfaces firstly is for below scenario:
            # Firstly vlan 1000 is not in vlan table but enabled in dhcp_server table, then add vlan1000 to vlan table
            # we need to refresh
            if config["state"] == "enabled":
                dhcp_interfaces.add(dhcp_interface)
                self.enabled_dhcp_interfaces.add(dhcp_interface)
            if dhcp_interface not in vlan_table and dhcp_interface != mid_plane_bridge_name:
                dhcp_interfaces.discard(dhcp_interface)
                continue
            if dhcp_interface in vlan_table:
                checkers_to_be_enabled |= set(VLAN_CHECKERS)
            elif dhcp_interface == mid_plane_bridge_name and self.smart_switch:
                checkers_to_be_enabled |= set([MID_PLANE_CHECKER])
        self._enable_checkers(checkers_to_be_enabled - self.enabled_checkers)

        # Checkers for feature, DHCP server, and external SONiC relay state should not be disabled.
        checkers_to_be_disabled = self.enabled_checkers - checkers_to_be_enabled - \
            set([FEATURE_CHECKER, DHCP_SERVER_CHECKER, DHCPV4_RELAY_CHECKER])
        self._disable_checkers(checkers_to_be_disabled)

        device_metadata = self.db_connector.get_config_db_table(DEVICE_METADATA)
        self.has_sonic_dhcpv4_relay = \
            device_metadata.get("localhost", {}).get("has_sonic_dhcpv4_relay", "False") == "True"
        if self.has_sonic_dhcpv4_relay:
            local_relay_active = bool(dhcp_interfaces)
            if local_relay_active:
                if (not self.sonic_dhcp_server_relay_active and
                   "dhcp4relay" in self.dhcp_relay_supervisor_config):
                    self._execute_supervisor_dhcp_relay_process("stop", ["dhcp4relay"])
                cmds = ["/usr/sbin/dhcp4relay"]
                if device_metadata.get("localhost", {}).get("subtype") == "DualToR":
                    cmds += ["-u", "Loopback0"]
                self._start_sonic_dhcp_relay_process(dhcp_interfaces, cmds, force_kill)
            elif self.sonic_dhcp_server_relay_active:
                self._kill_exist_relay_releated_process([], "dhcp4relay", True)
                self._check_sonic_dhcpv4_relay_config_transition()
                if "dhcp4relay" in self.dhcp_relay_supervisor_config:
                    self._execute_supervisor_dhcp_relay_process("start", ["dhcp4relay"])
            self.sonic_dhcp_server_relay_active = local_relay_active
        else:
            dhcp_server_ip = self._get_dhcp_server_ip() if dhcp_interfaces else None
            self._start_dhcrelay_process(dhcp_interfaces, dhcp_server_ip, force_kill)

        # TODO dhcpmon is not ready for count packet for dhcp_server, hence comment invoke it for now
        # self._start_dhcpmon_process(dhcp_interfaces, force_kill)

    def wait(self):
        """
        Wait function, check db change here
        """
        while True:
            check_param = {
                "enabled_dhcp_interfaces": self.enabled_dhcp_interfaces,
                "dhcp_server_feature_enabled": self.dhcp_server_feature_enabled
            }
            res = (self.dhcp_relayd_monitor.check_db_update(check_param))
            self._proceed_with_check_res(res, self.dhcp_server_feature_enabled)

    def _proceed_with_check_res(self, check_res, previous_dhcp_server_status):
        """
        Proceed depends on check result
        Args:
            check_res: result of checker, sample: {
                "DhcpServerFeatureStateChecker": True,
                "VlanIntfTableEventChecker": False
            }
            previous_dhcp_server_status: previous enabled/disabled status of dhcp_server feature
        """
        dhcp_feature_status_changed = False
        if FEATURE_CHECKER in check_res:
            dhcp_feature_status_changed = check_res[FEATURE_CHECKER]
            self.dhcp_server_feature_enabled = not previous_dhcp_server_status if dhcp_feature_status_changed \
                else previous_dhcp_server_status
        # If dhcp_server feature is enabled, dhcprelayd will manage dhcpmon/dhcrelay process
        if self.dhcp_server_feature_enabled:
            # disabled -> enabled, we need to enable dhcp_server checker and do refresh processes
            if dhcp_feature_status_changed:
                self._enable_checkers([DHCP_SERVER_CHECKER])
                if not self.has_sonic_dhcpv4_relay:
                    self._execute_supervisor_dhcp_relay_process("stop")
                self.refresh_dhcrelay()
            # enabled -> enabled, just need to check dhcp_server related tables to see whether need to refresh
            else:
                # Check vlan_interface table change, if it changed, need to refresh with force kill
                if (check_res.get(VLAN_INTF_CHECKER, False) or check_res.get(MID_PLANE_CHECKER, False) or
                   check_res.get(MID_PLANE_CHECKER, False)):
                    self.refresh_dhcrelay(True)
                elif check_res.get(VLAN_CHECKER, False) or check_res.get(DHCP_SERVER_CHECKER, False):
                    self.refresh_dhcrelay(False)
                if self.has_sonic_dhcpv4_relay:
                    if check_res.get(DHCPV4_RELAY_CHECKER, False):
                        self._check_sonic_dhcpv4_relay_config_transition()
                    if not self.sonic_dhcp_server_relay_active:
                        self._check_dhcp_relay_processes()

        # If dhcp_server feature is disabled, dhcprelayd will checke whether dhcpmon/dhcrelay processes,
        # if they are not running as expected, dhcprelayd will kill itself to make dhcp_relay container restart.
        else:
            # enabled -> disabled, we need to disable dhcp_server related checkers and start dhcrelay/dhcpmon
            # processes follow supervisord configuration
            if dhcp_feature_status_changed:
                checkers_to_be_disabled = [DHCP_SERVER_CHECKER] + VLAN_CHECKERS
                if self.smart_switch:
                    checkers_to_be_disabled.append(MID_PLANE_CHECKER)
                self._disable_checkers(self.enabled_checkers & set(checkers_to_be_disabled))
                if self.has_sonic_dhcpv4_relay:
                    if self.sonic_dhcp_server_relay_active:
                        self._kill_exist_relay_releated_process([], "dhcp4relay", True)
                        self.sonic_dhcp_server_relay_active = False
                        self._check_sonic_dhcpv4_relay_config_transition()
                        if "dhcp4relay" in self.dhcp_relay_supervisor_config:
                            self._execute_supervisor_dhcp_relay_process("start", ["dhcp4relay"])
                    else:
                        self._check_sonic_dhcpv4_relay_config_transition()
                        self._check_dhcp_relay_processes()
                else:
                    self._kill_exist_relay_releated_process([], "dhcpmon", True)
                    self._kill_exist_relay_releated_process([], "dhcrelay", True)
                    self._execute_supervisor_dhcp_relay_process("start")
            # disabled -> disabled, to check whether dhcpmon/dhcrelay running status consistent with supervisord
            # configuration
            else:
                if check_res.get(DHCPV4_RELAY_CHECKER, False):
                    self._check_sonic_dhcpv4_relay_config_transition()
                self._check_dhcp_relay_processes()

    def _enable_checkers(self, checkers):
        """
        Update set of enabled_checkers and enable checkers
        Args:
            checkers: checkers need to be enabled
        """
        new_enabled_checkers = set(checkers) - self.enabled_checkers
        self.dhcp_relayd_monitor.enable_checkers(new_enabled_checkers)
        self.enabled_checkers = self.enabled_checkers | new_enabled_checkers

    def _disable_checkers(self, checkers):
        """
        Update set of enabled_checkers and disable checkers
        Args:
            checkers: checkers need to be disabled
        """
        new_disabled_checkers = set(checkers) & self.enabled_checkers
        self.dhcp_relayd_monitor.disable_checkers(new_disabled_checkers)
        self.enabled_checkers = self.enabled_checkers - new_disabled_checkers

    def _is_dhcp_server_enabled(self):
        """
        Check whether dhcp_server feature is enabled via running config_db
        Returns:
            If dhcp_server feature is enabled, return True. Else, return False
        """
        feature_table = self.db_connector.get_config_db_table("FEATURE")
        return feature_table.get("dhcp_server", {}).get("state", "disabled") == "enabled"

    def _is_sonic_dhcpv4_relay_configured(self):
        relay_table = self.db_connector.get_config_db_table(DHCPV4_RELAY)
        return any(config.get("dhcpv4_servers") for config in relay_table.values())

    def _check_sonic_dhcpv4_relay_config_transition(self):
        if not self.has_sonic_dhcpv4_relay:
            return

        relay_configured = self._is_sonic_dhcpv4_relay_configured()
        supervisor_relay_configured = "dhcp4relay" in self.dhcp_relay_supervisor_config
        if relay_configured != supervisor_relay_configured:
            syslog.syslog(
                syslog.LOG_INFO,
                "DHCPv4 relay configuration presence changed; restart dhcp_relay to regenerate supervisor config"
            )
            sys.exit(1)

    def _execute_supervisor_dhcp_relay_process(self, op, programs=None):
        """
        Start or stop relay releated processes managed by supervisord
        Args:
            op: string of operation, require to be "start" or "stop"
            programs: optional subset of parsed supervisor programs to operate on
        """
        if op not in ["stop", "start"]:
            syslog.syslog(syslog.LOG_ERR, "Error operation: {}".format(op))
            sys.exit(1)
        configured_programs = self.dhcp_relay_supervisor_config.keys()
        target_programs = configured_programs if programs is None else \
            [program for program in programs if program in configured_programs]
        for program in target_programs:
            supervisor_programs = ["dhcp-relay:dhcp4relay", "dhcp4relay"] \
                if program == "dhcp4relay" else [program]
            for index, supervisor_program in enumerate(supervisor_programs):
                cmds = ["supervisorctl", op, supervisor_program]
                syslog.syslog(syslog.LOG_INFO, "Execute {} for {} by: {}".format(op, supervisor_program, cmds))
                try:
                    res = subprocess.run(cmds, check=True)
                except subprocess.CalledProcessError as error:
                    if index + 1 < len(supervisor_programs):
                        continue
                    syslog.syslog(syslog.LOG_ERR, "Error in execute: {}".format(error))
                    sys.exit(1)
                if res.returncode != 0:
                    if index + 1 < len(supervisor_programs):
                        continue
                    syslog.syslog(syslog.LOG_ERR, "Error in execute: {}".format(res))
                    sys.exit(1)
                break
            else:
                sys.exit(1)
            syslog.syslog(syslog.LOG_INFO, "Supervisor {} for {} succeeded".format(op, program))

    def _check_dhcp_relay_processes(self):
        """
        Check whether IPv4 relay processes are running as expected.
        If not, dhcprelayd will exit with code 1.
        """
        procs = {}
        for proc in psutil.process_iter():
            err = None
            for i in range(5):
                err = None
                try:
                    if proc.name() in ["dhcrelay", "dhcp4relay"]:
                        procs[proc.pid] = [proc.ppid(), proc.cmdline()]
                except psutil.NoSuchProcess:
                    pass
                except Exception as e:
                    err = e
                if err is None:
                    break
            if err:
                raise err

        # When there is network io, dhcrelay would create child process to proceed them, psutil has chance to get
        # duplicated cmdline. Hence ignore chlid process in here
        running_cmds = []
        for _, (parent_pid, cmdline) in procs.items():
            if parent_pid in procs:
                continue
            running_cmds.append(cmdline)

        running_cmds.sort()
        expected_cmds = [
            value for key, value in self.dhcp_relay_supervisor_config.items()
            if "isc-dhcpv4-relay" in key or key == "dhcp4relay"
        ]
        expected_cmds.sort()
        if running_cmds != expected_cmds:
            syslog.syslog(syslog.LOG_ERR, "Running processes is not as expected! Runnning: {}. Expected: {}"
                          .format(running_cmds, expected_cmds))
            sys.exit(1)

    def _get_dhcp_relay_config(self):
        """
        Get supervisord configuration for dhcrelay/dhcpmon
        Returns:
            Dict of cmds, sample:{
                'isc-dhcpv4-relay-Vlan1000': [
                    '/usr/sbin/dhcrelay', '-d', '-m', 'discard', '-a', '%h:%p', '%P', '--name-alias-map-file',
                    '/tmp/port-name-alias-map.txt', '-id', 'Vlan1000', '-iu', 'PortChannel101', '-iu',
                    'PortChannel102', '-iu', 'PortChannel103', '-iu', 'PortChannel104', '192.0.0.1', '192.0.0.2',
                    '192.0.0.3', '192.0.0.4'
                ],
                'dhcpmon-Vlan1000': [
                    '/usr/sbin/dhcpmon', '-id', 'Vlan1000', '-iu', 'PortChannel101', '-iu', 'PortChannel102', '-iu',
                    'PortChannel103', '-iu', 'PortChannel104', '-im', 'eth0'
                ]
            }
        """
        res = {}
        with open(self.supervisord_conf_path, "r") as conf_file:
            content = conf_file.read()
            cmds = re.findall(
                r"\[program:((?:(?:isc-dhcpv4-relay|dhcpmon)-.+)|dhcp4relay)\]\ncommand=(.+)",
                content
            )
            for cmd in cmds:
                key = "dhcpmon:{}".format(cmd[0]) if "dhcpmon" in cmd[0] else cmd[0]
                res[key] = cmd[1].replace("%%", "%").split(" ")
        return res

    def _start_dhcrelay_process(self, new_dhcp_interfaces, dhcp_server_ip, force_kill):
        # To check whether need to kill dhcrelay process
        kill_res = self._kill_exist_relay_releated_process(new_dhcp_interfaces, "dhcrelay", force_kill)
        if kill_res == NOT_KILLED:
            # Means old running status consistent with the new situation, no need to run new
            return

        # No need to start new dhcrelay process
        if len(new_dhcp_interfaces) == 0:
            return

        cmds = ["/usr/sbin/dhcrelay", "-d", "-m", "discard", "-a", "%h:%p", "%P", "--name-alias-map-file",
                "/tmp/port-name-alias-map.txt"]
        for dhcp_interface in new_dhcp_interfaces:
            cmds += ["-id", dhcp_interface]
        cmds += ["-iu", "docker0", dhcp_server_ip]
        popen_res = subprocess.Popen(cmds)
        # To make sure process start successfully not in zombie status
        proc = psutil.Process(popen_res.pid)
        time.sleep(1)
        if proc.status() == psutil.STATUS_ZOMBIE:
            syslog.syslog(syslog.LOG_ERR, "Failed to start dhcrelay process with: {}".format(cmds))
            terminate_proc(proc)
            sys.exit(1)

        syslog.syslog(syslog.LOG_INFO, "dhcrelay process started successfully, cmds: {}".format(cmds))

    def _start_sonic_dhcp_relay_process(self, new_dhcp_interfaces, cmds, force_kill):
        kill_res = self._kill_exist_relay_releated_process(
            new_dhcp_interfaces, "dhcp4relay", force_kill
        )
        if kill_res == NOT_KILLED or len(new_dhcp_interfaces) == 0:
            return

        popen_res = subprocess.Popen(cmds)
        proc = psutil.Process(popen_res.pid)
        time.sleep(1)
        if proc.status() == psutil.STATUS_ZOMBIE:
            syslog.syslog(syslog.LOG_ERR, "Failed to start dhcp4relay process with: {}".format(cmds))
            terminate_proc(proc)
            sys.exit(1)

        syslog.syslog(syslog.LOG_INFO, "dhcp4relay process started successfully, cmds: {}".format(cmds))

    def _start_dhcpmon_process(self, new_dhcp_interfaces, force_kill):
        # To check whether need to kill dhcrelay process
        kill_res = self._kill_exist_relay_releated_process(new_dhcp_interfaces, "dhcpmon", force_kill)
        if kill_res == NOT_KILLED:
            # Means old running status consistent with the new situation, no need to run new
            return

        # No need to start new dhcrelay process
        if len(new_dhcp_interfaces) == 0:
            return

        pids_cmds = {}
        for dhcp_interface in new_dhcp_interfaces:
            cmds = ["/usr/sbin/dhcpmon", "-id", dhcp_interface, "-iu", "docker0", "-im", "eth0"]
            popen_res = subprocess.Popen(cmds)
            pids_cmds[popen_res.pid] = cmds
        time.sleep(1)
        # To make sure process start successfully not in zombie status
        for pid, cmds in pids_cmds.items():
            proc = psutil.Process(pid)
            if proc.status() == psutil.STATUS_ZOMBIE:
                syslog.syslog(syslog.LOG_ERR, "Failed to start dhcpmon process: {}".format(cmds))
                terminate_proc(proc)
            else:
                syslog.syslog(syslog.LOG_INFO, "dhcpmon process started successfully, cmds: {}".format(cmds))

    def _kill_exist_relay_releated_process(self, new_dhcp_interfaces, process_name, force_kill):
        old_dhcp_interfaces = set()
        # Because in system there maybe more than 1 dhcpmon processes are running, so we need list to store
        target_procs = []

        # Get old dhcrelay process and get old dhcp interfaces
        for proc in psutil.process_iter():
            try:
                if proc.name() == process_name:
                    cmds = proc.cmdline()
                    index = 0
                    target_procs.append(proc)
                    while index < len(cmds):
                        if cmds[index] == "-id":
                            old_dhcp_interfaces.add(cmds[index + 1])
                            index += 2
                        else:
                            index += 1
            except psutil.NoSuchProcess:
                continue
        if len(target_procs) == 0:
            return NOT_FOUND_PROC

        # No need to kill
        if not force_kill and (
            process_name == "dhcrelay" and old_dhcp_interfaces == new_dhcp_interfaces or
            process_name == "dhcpmon" and old_dhcp_interfaces == new_dhcp_interfaces or
            process_name == "dhcp4relay" and len(new_dhcp_interfaces) > 0
        ):
            return NOT_KILLED
        for proc in target_procs:
            terminate_proc(proc)
            syslog.syslog(syslog.LOG_INFO, "Kill process: {}".format(process_name))
        return KILLED_OLD

    def _get_dhcp_server_ip(self):
        dhcp_server_ip_table = swsscommon.Table(self.db_connector.state_db, DHCP_SERVER_IPV4_SERVER_IP)
        for _ in range(10):
            state, ip = dhcp_server_ip_table.hget(DHCP_SERVER_INTERFACE, "ip")
            if state:
                return ip
            else:
                syslog.syslog(syslog.LOG_INFO, "Cannot get dhcp server ip")
                time.sleep(10)
        syslog.syslog(syslog.LOG_ERR, "Cannot get dhcp_server ip from state_db")
        sys.exit(1)


def main():
    dhcp_db_connector = DhcpDbConnector(redis_sock=REDIS_SOCK_PATH)
    sel = swsscommon.Select()
    checkers = []
    checkers.append(DhcpServerTableIntfEnablementEventChecker(sel, dhcp_db_connector.config_db))
    checkers.append(VlanIntfTableEventChecker(sel, dhcp_db_connector.config_db))
    checkers.append(VlanTableEventChecker(sel, dhcp_db_connector.config_db))
    checkers.append(MidPlaneTableEventChecker(sel, dhcp_db_connector.config_db))
    checkers.append(DhcpServerFeatureStateChecker(sel, dhcp_db_connector.config_db))
    checkers.append(DhcpV4RelayTableEventChecker(sel, dhcp_db_connector.config_db))
    db_monitor = DhcpRelaydDbMonitor(dhcp_db_connector, sel, checkers, DEFAULT_SELECT_TIMEOUT)
    dhcprelayd = DhcpRelayd(dhcp_db_connector, db_monitor, enabled_checkers=[FEATURE_CHECKER])
    dhcprelayd.start()
    dhcprelayd.wait()


if __name__ == "__main__":
    main()
