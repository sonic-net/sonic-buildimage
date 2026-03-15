#!/usr/bin/env python3

import os
import subprocess
import time

class WatchdogDevLoader:
    def __init__(self):
        self.drivers = ["iTCO_wdt", "wdat_wdt"]

    def __run_command(self, command, timeout=None):
        """
        Run shell command with shell=False
        Args:
            command(list of strings): Shell command string in list format.
            timeout(float): The period in seconds after which to timeout.
        Returns:
            A tuple(status, output). Return(exitcode, output) of executing command.
        """
        try:
            output = subprocess.check_output(command, universal_newlines=True,
                    timeout=timeout, stderr=subprocess.STDOUT)
            status = 0
        except subprocess.CalledProcessError as ex:
            output = ex.output
            status = ex.returncode
        except Exception as ex:
            output = "{}".format(ex)
            status = -1

        if output[-1:] == '\n':
            output = output[:-1]

        return status, output

    def load_driver(self):
        # Return True directly if watchdog dev already exist
        if os.path.exists("/dev/watchdog0"):
            return True

        # Load drivers and check success
        for driver in self.drivers:
            # Attempt to load the driver
            self.__run_command(["modprobe", driver])
            time.sleep(10)

            # return True if /dev/watchdog0 exists
            if os.path.exists("/dev/watchdog0"):
                return True

            # If loading fails, remove the driver before trying the next one
            self.__run_command(["modprobe", "-r", driver])
            continue

        # No driver was successfully loaded
        return False

if __name__ == '__main__':
    loader = WatchdogDevLoader()
    if loader.load_driver():
        print("Watchdog driver loaded successfully.")
    else:
        print("Failed to load any watchdog driver.")
