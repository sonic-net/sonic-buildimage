#!/usr/bin/env python3
import sys
import os
import subprocess
import signal
import syslog

syslog.openlog(ident="picocom", logoption=syslog.LOG_PID, facility=syslog.LOG_USER)

is_target = False
for arg in sys.argv:
    if "ttySwitchCpu0" in arg:
        is_target = True
        break

if is_target:
    try:
        res = subprocess.run(
            "dfd_debug sysfs_data_wr /dev/cpld1 0x59 0x01",
            shell=True,
            capture_output=True,
            text=True,
            close_fds=True,
        )
        if res.stdout:
            syslog.syslog(syslog.LOG_INFO, f"dfd_debug open stdout: {res.stdout.strip()}")
        if res.stderr:
            syslog.syslog(syslog.LOG_WARNING, f"dfd_debug open stderr: {res.stderr.strip()}")
        if res.returncode != 0:
            syslog.syslog(syslog.LOG_ERR, f"dfd_debug open exit {res.returncode}")
    except Exception as e:
        syslog.syslog(syslog.LOG_ERR, f"dfd_debug open failed: {e}")
    syslog.syslog(syslog.LOG_INFO, "Opening serial port /dev/ttySwitchCpu0")

cmd = ["/usr/bin/picocom.real"] + sys.argv[1:]
p = subprocess.Popen(cmd)

def signal_handler(signum, frame):
    try:
        syslog.syslog(syslog.LOG_DEBUG, f"Received signal {signum}, forwarding to picocom...")
        p.send_signal(signum)
    except Exception as e:
        syslog.syslog(syslog.LOG_ERR, f"Failed to forward signal: {e}")

signal.signal(signal.SIGTERM, signal_handler)
signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGHUP, signal_handler)

p.wait()

if is_target:
    try:
        res = subprocess.run(
            "dfd_debug sysfs_data_wr /dev/cpld1 0x59 0x00",
            shell=True,
            capture_output=True,
            text=True,
            close_fds=True,
        )
        if res.stdout:
            syslog.syslog(syslog.LOG_INFO, f"dfd_debug close stdout: {res.stdout.strip()}")
        if res.stderr:
            syslog.syslog(syslog.LOG_WARNING, f"dfd_debug close stderr: {res.stderr.strip()}")
        if res.returncode != 0:
            syslog.syslog(syslog.LOG_ERR, f"dfd_debug close exit {res.returncode}")
    except Exception as e:
        syslog.syslog(syslog.LOG_ERR, f"dfd_debug close failed: {e}")
    syslog.syslog(syslog.LOG_INFO, "Closing serial port /dev/ttySwitchCpu0")

syslog.closelog()

sys.exit(p.returncode)