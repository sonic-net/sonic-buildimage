#!/usr/bin/python3

'''
This script is to monitor CPLD1/factbtn_event
'''
import time
import syslog
import subprocess
import os

class FactbtnMonitor():
    def __init__(self):
        self.file_factbtn = "/sys/devices/platform/switchboard/CPLD1/factbtn_event"
        self.file_xcvrled = "/sys/devices/platform/switchboard/CPLD2/all_xcvr_led_ctrl"

    def clear(self):
        try:
            with open(self.file_factbtn, "r+") as f:
                raw = f.readline().strip()
                value = int(raw, 16)
                f.seek(0)
                f.write(hex(value & 0b1000))
                f.seek(0)
                f.write(hex(value & 0b1010))
        except EnvironmentError as error:        # parent of IOError, OSError
            syslog.syslog(f"write {self.file_factbtn} error: {error}")
            return False
        return True

    def run(self):
        syslog.openlog(logoption=syslog.LOG_PID)
        # clear before polling
        self.clear()
        syslog.syslog("Start polling")

        while True:
            time.sleep(3)
            try:
                with open(self.file_factbtn, "r") as f:
                    raw = f.readline().strip()
            except EnvironmentError as error:        # parent of IOError, OSError
                syslog.syslog(f"read {self.file_factbtn} error: {error}")
                continue
            value = int(raw, 16)

            if value & 0x01:
                syslog.syslog("Long press detected")

                # all port led on
                try:
                    with open(self.file_xcvrled, "w") as f:
                        f.seek(0)
                        f.write("0x19")
                except EnvironmentError as error:        # parent of IOError, OSError
                    syslog.syslog(f"write {self.file_xcvrled} error: {error}")
                    continue

                # reset file system
                syslog.syslog("call reset-factory with no reboot")
                p = subprocess.run(["/usr/local/bin/no_reboot.sh", "/usr/bin/reset-factory"])
                syslog.syslog(f"{p}")

                # reset BMC
                syslog.syslog("call BMC reset")
                p = subprocess.run(["/usr/local/bin/reset-to-factory-default", "only-bmc"])
                syslog.syslog(f"{p}")

                # all port led normal
                try:
                    with open(self.file_xcvrled, "w") as f:
                        f.seek(0)
                        f.write("0x0")
                except EnvironmentError as error:        # parent of IOError, OSError
                    syslog.syslog(f"write {self.file_xcvrled} error: {error}")
                    #continue

                # clear before reboot
                self.clear()
                syslog.syslog("reboot")
                os.system('shutdown -r now')
                break

if __name__ == "__main__":
    m = FactbtnMonitor()
    m.run()
