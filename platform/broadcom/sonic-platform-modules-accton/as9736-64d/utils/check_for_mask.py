#!/usr/bin/env python3
#
# Copyright (C) 2016 Accton Networks, Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import subprocess
import getopt
import sys
import logging
import time
import os
import re
import sonic_platform.platform

HW_REV_FILE="/host/hw_rev"

def main():

    if not os.path.exists(HW_REV_FILE):
        platform = sonic_platform.platform.Platform()
        chassis = platform.get_chassis()
        for i in range(1,200):
            hw_rev=chassis.get_revision()
            print("h/w revision is {}".format(hw_rev))
            if hw_rev != "N/A":
                break
            else:
                time.sleep(1)
        save_hw_rev(hw_rev)
        print("i = {}".format(i))
    else:
        hw_rev=load_hw_rev()
        print("Loading h/w revision {} from HW_REV_FILE".format(hw_rev))

    enabled=False
    # if h/w revision is R01 above then enabled is True
    enabled = bool(hw_rev and re.match(r'^R0[1-9]', hw_rev))
    print("Enabled is {}".format(enabled))
    if enabled is False:
        mask_service("watchdog-controller.service")
        sys.exit(1)
    sys.exit(0)

def save_hw_rev(info_str):
    with open(HW_REV_FILE, 'w') as f:
        f.write(info_str)

def load_hw_rev():
    with open(HW_REV_FILE, 'r') as f:
        return f.read().strip()




def stop_service(service_name):
    cmd = ['sudo', 'systemctl', 'stop', service_name]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode == 0:
        print(f"Service {service_name} stopped successfully.")
    else:
        print(f"Failed to stop {service_name}.")
        print(result.stderr)


def mask_service(service_name):
    cmd = ['sudo', 'systemctl', 'mask', service_name]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode == 0:
        print(f"Service {service_name} masked successfully.")
    else:
        print(f"Failed to mask {service_name}.")
        print(result.stderr)


if __name__ == "__main__":
    main()
