#!/usr/bin/python3
#
# Nokia 7215 interface rename monitor.
#
# Assumption: this daemon starts before syncd renames the front-panel
# interfaces, so it observes them while they are still named eth1/eth2.
# That initial baseline is what lets the eth1->Ethernet0 and
# eth2->Ethernet1 rename events later be recognized as a trigger to
# restart neighsyncd. If this script ever starts after the rename has
# already happened, it will see only Ethernet0/Ethernet1 from the start
# and will not fire a restart on its own. This daemon does not depend on neighsyncd.

import os
import re
import select
import signal
import subprocess
import time

try:
    from sonic_py_common import logger
except ImportError:
    logger = None


WATCHED_RENAMES = {
    "eth1": "Ethernet0",
    "eth2": "Ethernet1",
}
EXPECTED_INTERFACES = set(WATCHED_RENAMES.values())
SYS_CLASS_NET = "/sys/class/net"
RESTART_LOG_FILE = "/var/log/intf_sync.log"
RESTART_LOG_MAX_LINES = 100
QUIET_PERIOD_SEC = 2
CONVERGENCE_TIMEOUT_SEC = 30
IP_MONITOR_RESTART_DELAY_SEC = 2
NEIGHSYNCD_RESTART_RETRIES = 10
NEIGHSYNCD_RESTART_RETRY_DELAY_SEC = 1


class ServiceLogger:
    def __init__(self):
        if logger:
            self.logger = logger.Logger("IntfSync")
            self.logger.set_min_log_priority_info()
        else:
            self.logger = None

    def notice(self, msg):
        if self.logger:
            self.logger.log_notice(msg)
        else:
            print(msg, flush=True)

    def info(self, msg):
        if self.logger:
            self.logger.log_info(msg)
        else:
            print(msg, flush=True)

    def warning(self, msg):
        if self.logger:
            self.logger.log_warning(msg)
        else:
            print("WARNING: %s" % msg, flush=True)

    def error(self, msg):
        if self.logger:
            self.logger.log_error(msg)
        else:
            print("ERROR: %s" % msg, flush=True)


log = ServiceLogger()
stop_requested = False


def handle_signal(signum, frame):
    global stop_requested
    stop_requested = True


def read_link_snapshot():
    links = {}

    try:
        names = os.listdir(SYS_CLASS_NET)
    except OSError as err:
        write_restart_log("failed to list %s: %s" % (SYS_CLASS_NET, err))
        return links

    for name in names:
        if name not in WATCHED_RENAMES and name not in EXPECTED_INTERFACES:
            continue

        ifindex_path = os.path.join(SYS_CLASS_NET, name, "ifindex")
        try:
            with open(ifindex_path) as fp:
                ifindex = int(fp.read().strip())
        except (OSError, ValueError) as err:
            write_restart_log("failed to read %s: %s" % (ifindex_path, err))
            continue

        links[ifindex] = name

    return links


def parse_ip_monitor_line(line):
    # Example: "3: Ethernet0: <BROADCAST,MULTICAST,UP> mtu 1500 ..."
    # Example: "Deleted 3: Ethernet0: <BROADCAST,MULTICAST> ..."
    match = re.match(r"^\s*(?:Deleted\s+)?(?P<ifindex>\d+):\s+(?P<name>[^:@\s]+)", line)
    if not match:
        return None, None

    return int(match.group("ifindex")), match.group("name")


def expected_interfaces_ready():
    names = set()

    try:
        names = set(os.listdir(SYS_CLASS_NET))
    except OSError as err:
        write_restart_log("failed to list %s: %s" % (SYS_CLASS_NET, err))
        return False

    return EXPECTED_INTERFACES.issubset(names)


def wait_for_expected_interfaces():
    deadline = time.time() + CONVERGENCE_TIMEOUT_SEC

    while time.time() < deadline and not stop_requested:
        if expected_interfaces_ready():
            return True
        time.sleep(1)

    return expected_interfaces_ready()


def docker_cmd():
    if os.path.exists("/usr/bin/docker"):
        return "/usr/bin/docker"
    return "docker"


def write_restart_log(msg):
    timestamp = time.strftime("%Y-%m-%d %H:%M:%S %z", time.localtime())
    try:
        with open(RESTART_LOG_FILE, "a") as fp:
            fp.write("%s %s\n" % (timestamp, msg))
    except OSError as err:
        log.warning("failed to write %s: %s" % (RESTART_LOG_FILE, err))
        return

    try:
        with open(RESTART_LOG_FILE, "r") as fp:
            lines = fp.readlines()
        if len(lines) > RESTART_LOG_MAX_LINES:
            lines = lines[-RESTART_LOG_MAX_LINES:]
            tmp_path = RESTART_LOG_FILE + ".tmp"
            with open(tmp_path, "w") as fp:
                fp.writelines(lines)
            os.replace(tmp_path, RESTART_LOG_FILE)
    except OSError as err:
        log.warning("failed to rotate %s: %s" % (RESTART_LOG_FILE, err))


def restart_neighsyncd():
    if not wait_for_expected_interfaces():
        write_restart_log("Ethernet0/Ethernet1 did not converge; skip neighsyncd restart")
        return False

    cmd = [docker_cmd(), "exec", "swss", "supervisorctl", "restart", "neighsyncd"]

    for attempt in range(1, NEIGHSYNCD_RESTART_RETRIES + 1):
        try:
            rc = subprocess.call(
                cmd,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
        except OSError as err:
            rc = None
            write_restart_log(
                "neighsyncd restart attempt %d/%d failed to execute: %s"
                % (attempt, NEIGHSYNCD_RESTART_RETRIES, err)
            )

        if rc == 0:
            write_restart_log(
                "neighsyncd restart succeeded after front-panel interface rename "
                "on attempt %d/%d" % (attempt, NEIGHSYNCD_RESTART_RETRIES)
            )
            return True

        if rc is not None:
            write_restart_log(
                "neighsyncd restart attempt %d/%d failed, rc=%d"
                % (attempt, NEIGHSYNCD_RESTART_RETRIES, rc)
            )

        if attempt < NEIGHSYNCD_RESTART_RETRIES and not stop_requested:
            time.sleep(NEIGHSYNCD_RESTART_RETRY_DELAY_SEC)

    write_restart_log("neighsyncd restart failed after %d attempts" % NEIGHSYNCD_RESTART_RETRIES)
    return False


def start_ip_monitor():
    return subprocess.Popen(
        ["ip", "-o", "monitor", "link"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
        bufsize=1,
    )


def monitor_loop():
    ifindex_to_name = read_link_snapshot()
    pending_restart_at = None

    while not stop_requested:
        proc = start_ip_monitor()
        write_restart_log("started ip monitor link")

        while not stop_requested:
            if proc.poll() is not None:
                write_restart_log("ip monitor exited, rc=%s" % proc.returncode)
                break

            # Dual-purpose: wake on ip monitor output, or tick once per second
            # so pending_restart_at, stop_requested, and proc.poll() are checked.
            ready, _, _ = select.select([proc.stdout], [], [], 1)
            if ready:
                line = proc.stdout.readline()
                if not line:
                    continue

                ifindex, new_name = parse_ip_monitor_line(line)
                if ifindex is None:
                    continue

                old_name = ifindex_to_name.get(ifindex)
                ifindex_to_name[ifindex] = new_name

                expected_new_name = WATCHED_RENAMES.get(old_name)
                if expected_new_name == new_name:
                    write_restart_log(
                        "detected interface rename ifindex=%d %s -> %s; "
                        "scheduling neighsyncd restart"
                        % (ifindex, old_name, new_name)
                    )
                    # Debounce so eth1->Ethernet0 and eth2->Ethernet1 arriving
                    # back-to-back collapse into a single neighsyncd restart.
                    pending_restart_at = time.time() + QUIET_PERIOD_SEC

            if pending_restart_at and time.time() >= pending_restart_at:
                pending_restart_at = None
                restart_neighsyncd()

        try:
            proc.terminate()
            proc.wait(timeout=IP_MONITOR_RESTART_DELAY_SEC)
        except Exception:
            try:
                proc.kill()
            except Exception:
                pass

        if not stop_requested:
            time.sleep(IP_MONITOR_RESTART_DELAY_SEC)


def main():
    signal.signal(signal.SIGHUP, signal.SIG_IGN)
    signal.signal(signal.SIGINT, handle_signal)
    signal.signal(signal.SIGTERM, handle_signal)

    write_restart_log("intf sync started")
    monitor_loop()
    write_restart_log("intf sync stopped")


if __name__ == "__main__":
    main()
