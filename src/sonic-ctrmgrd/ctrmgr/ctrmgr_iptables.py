#!/usr/bin/env python3

import ipaddress
import os
import re
import socket
import subprocess
import syslog

UNIT_TESTING = 0

# NOTE:
# Unable to use python-iptables as that does not create rules per ip-tables default
# which is nf_tables. So rules added via iptc package will not be listed under
# "sudo iptables -t nat -L -n". But available in kernel. To list, we need to
# use legacy mode as "sudo iptables-legacy -t nat -L -n".
# As we can't use two modes and using non-default could make any debugging effort
# very tough.


from urllib.parse import urlparse

DST_FILE = "/etc/systemd/system/docker.service.d/http_proxy.conf"
DST_IP = None
DST_PORT = None
SQUID_PORT = "3128"

def _get_ip(ip_str):
    ret = ""
    if ip_str:
        try:
            ipaddress.ip_address(ip_str)
            ret = ip_str
        except ValueError:
            pass

        if not ret:
            try:
                ret = socket.gethostbyname(ip_str)
            except (OSError, socket.error):
                pass
        if not ret:
            syslog.syslog(syslog.LOG_ERR, "{} is neither IP nor resolves to IP".
                    format(ip_str))
    return ret


def _get_dst_info():
    global DST_IP, DST_PORT
    DST_IP = None
    DST_PORT = None
    print("DST_FILE={}".format(DST_FILE))
    if os.path.exists(DST_FILE):
        with open(DST_FILE, "r") as s:
            for line in s.readlines():
                url_match = re.search('^Environment=.HTTP_PROXY=(.+?)"', line)
                if url_match:
                    url = urlparse(url_match.group(1))
                    DST_IP = _get_ip(url.hostname)
                    DST_PORT = url.port
                    break
    else:
        print("{} not available".format(DST_FILE))
    print("DST_IP={}".format(DST_IP))


def _is_rule_match(rule):
    expect = "DNAT tcp -- 0.0.0.0/0 {} tcp dpt:{} to:".format(
            DST_IP, DST_PORT)

    # Remove duplicate spaces
    rule = " ".join(rule.split()).strip()

    if rule.startswith(expect):
        return rule[len(expect):]
    else:
        return ""


def check_proc(proc):
    if proc.returncode:
        syslog.syslog(syslog.LOG_ERR, "Failed to run: cmd: {}".format(proc.args))
        syslog.syslog(syslog.LOG_ERR, "Failed to run: stdout: {}".format(proc.stdout))
        syslog.syslog(syslog.LOG_ERR, "Failed to run: stderr: {}".format(proc.stderr))
        if not UNIT_TESTING:
            assert False


def iptable_proxy_rule_upd(ip_str, port = SQUID_PORT):
    _get_dst_info()
    if not DST_IP:
        # There is no proxy in use. Bail out.
        return ""

    destination = ""
    if ip_str:
        upd_ip = _get_ip(ip_str)
        if not upd_ip:
            return ""
        destination = "{}:{}".format(upd_ip, port)

    found = False
    num = 0

    while True:
        num += 1

        cmd = ["sudo", "iptables", "-t", "nat", "-n", "-L", "OUTPUT", str(num)]
        proc = subprocess.run(cmd, shell=False, capture_output=True)
        check_proc(proc)

        if not proc.stdout:
            # No more rule
            break

        rule_dest = _is_rule_match(proc.stdout.decode("utf-8").strip())
        if rule_dest:
            if not found and destination and (rule_dest == destination):
                found = True
            else:
                # Duplicate or different IP - delete it
                cmd = ["sudo", "iptables", "-t", "nat", "-D", "OUTPUT", str(num)]
                proc = subprocess.run(cmd, shell=False, capture_output=True)
                check_proc(proc)
                # Decrement number to accommodate deleted rule
                num -= 1

    if destination and not found:
        cmd = ["sudo", "iptables", "-t", "nat", "-A", "OUTPUT", "-p", "tcp", "-d", DST_IP, "--dport", DST_PORT, "-j", "DNAT", "--to-destination", destination]
        proc = subprocess.run(cmd, shell=False, capture_output=True)

        check_proc(proc)

    return destination
