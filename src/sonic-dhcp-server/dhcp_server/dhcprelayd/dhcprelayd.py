# TODO Add support for running different dhcrelay processes for each dhcp interface
# Currently if we run multiple dhcrelay processes, except for the last running process,
# others will not relay dhcp_release packet.
import psutil
import subprocess
import sys
import syslog
import time
from swsscommon import swsscommon
from dhcp_server.common.utils import DhcpDbConnector, terminate_proc
from dhcp_server.common.dhcp_db_monitor import DhcpRelaydDbMonitor, DhcpServerTableIntfEnablementEventChecker, \
     VlanTableEventChecker, VlanIntfTableEventChecker

REDIS_SOCK_PATH = "/var/run/redis/redis.sock"
DHCP_SERVER_IPV4_SERVER_IP = "DHCP_SERVER_IPV4_SERVER_IP"
DHCP_SERVER_IPV4 = "DHCP_SERVER_IPV4"
VLAN = "VLAN"
DEFAULT_SELECT_TIMEOUT = 5000  # millisecond
DHCP_SERVER_INTERFACE = "eth0"
DEFAULT_CHECKER = ["DhcpServerTableIntfEnablementEventChecker", "VlanTableEventChecker", "VlanIntfTableEventChecker"]
KILLED_OLD = 1
NOT_KILLED = 2
NOT_FOUND_PROC = 3


class DhcpRelayd(object):
    enabled_dhcp_interfaces = set()

    def __init__(self, db_connector, db_monitor):
        """
        Args:
            db_connector: db connector obj
            select_timeout: timeout setting for subscribe db change
        """
        self.db_connector = db_connector
        self.last_refresh_time = None
        self.dhcp_relayd_monitor = db_monitor
        self.enabled_dhcp_interfaces = set()

    def start(self):
        """
        Start function
        """
        self.refresh_dhcrelay()

    def refresh_dhcrelay(self, force_kill=False):
        """
        To refresh dhcrelay/dhcpmon process (start or restart)
        Args:
            force_kill: if True, force kill old processes
        """
        syslog.syslog(syslog.LOG_INFO, "Start to refresh dhcrelay related processes")
        dhcp_server_ip = self._get_dhcp_server_ip()
        dhcp_server_ipv4_table = self.db_connector.get_config_db_table(DHCP_SERVER_IPV4)
        vlan_table = self.db_connector.get_config_db_table(VLAN)

        dhcp_interfaces = set()
        self.enabled_dhcp_interfaces = set()
        for dhcp_interface, config in dhcp_server_ipv4_table.items():
            # Reason for add to enabled_dhcp_interfaces firstly is for below scenario:
            # Firstly vlan 1000 is not in vlan table but enabled in dhcp_server table, then add vlan1000 to vlan table
            # we need to refresh
            if config["state"] == "enabled":
                dhcp_interfaces.add(dhcp_interface)
                self.enabled_dhcp_interfaces.add(dhcp_interface)
            if dhcp_interface not in vlan_table:
                dhcp_interfaces.discard(dhcp_interface)
                continue
        self._start_dhcrelay_process(dhcp_interfaces, dhcp_server_ip, force_kill)
        self._start_dhcpmon_process(dhcp_interfaces, force_kill)

    def wait(self):
        """
        Wait function, check db change here
        """
        while True:
            res = (self.dhcp_relayd_monitor.check_db_update({"enabled_dhcp_interfaces": self.enabled_dhcp_interfaces}))
            (dhcp_server_res, vlan_res, vlan_intf_res) = res
            # vlan ip change require kill old dhcp_relay related processes
            if vlan_intf_res:
                self.refresh_dhcrelay(True)
            elif dhcp_server_res or vlan_res:
                self.refresh_dhcrelay(False)

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
                syslog.syslog(syslog.LOG_ERR, "Faild to start dhcpmon process: {}".format(cmds))
                terminate_proc(proc)
            else:
                syslog.syslog(syslog.LOG_INFO, "dhcpmon process started successfully, cmds: {}".format(cmds))

    def _kill_exist_relay_releated_process(self, new_dhcp_interfaces, process_name, force_kill):
        old_dhcp_interfaces = set()
        # Because in system there maybe more than 1 dhcpmon processes are running, so we need list to store
        target_procs = []

        # Get old dhcrelay process and get old dhcp interfaces
        for proc in psutil.process_iter():
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
        if len(target_procs) == 0:
            return NOT_FOUND_PROC

        # No need to kill
        if not force_kill and (process_name == "dhcrelay" and old_dhcp_interfaces == new_dhcp_interfaces or
           process_name == "dhcpmon" and old_dhcp_interfaces == (new_dhcp_interfaces)):
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
    db_monitor = DhcpRelaydDbMonitor(dhcp_db_connector, sel, checkers, DEFAULT_SELECT_TIMEOUT)
    db_monitor.enable_checker(DEFAULT_CHECKER)
    dhcprelayd = DhcpRelayd(dhcp_db_connector, db_monitor)
    dhcprelayd.start()
    dhcprelayd.wait()


if __name__ == "__main__":
    main()
