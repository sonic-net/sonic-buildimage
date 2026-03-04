#!/usr/bin/python3
'''
This script is to delay start services, to resolve the issu fisrt boot install hang
'''
import time
import syslog
import subprocess
import os

class DelayStart():
    def run(self):
        time.sleep(180)

        syslog.openlog(logoption=syslog.LOG_PID)
        syslog.syslog("starting sfp-max-temp-updater.service")
        p = subprocess.run(["/usr/bin/systemctl", "start", "sfp-max-temp-updater.service"],
                           capture_output=True, text=True)
        syslog.syslog(f"{p}")

        syslog.syslog("Finished delay start")


if __name__ == "__main__":
    m = DelayStart()
    m.run()

