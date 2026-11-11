#!/usr/bin/env python3

#
# Copyright (C) 2024 Micas Networks Inc.
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

import os
import subprocess
import sys
import time


def start():
    subnet_path = "/sys/class/net/eth0.4088"
    subnet_addr = "fe80::2/64"
    retry_count = 10
    subnet_cmds = []
    subnet_cmds.append("ip link add link eth0 name eth0.4088 type vlan id 4088")
    subnet_cmds.append("ip -6 addr replace fe80::2/64 dev eth0.4088")
    subnet_cmds.append("ip link set dev eth0.4088 up")

    retry = 0
    while retry < retry_count:
        try:
            if not os.path.exists(subnet_path):
                cmd = subnet_cmds[0]
                subprocess.run(cmd.split(), check=True)

            for cmd in subnet_cmds[1:]:
                subprocess.run(cmd.split(), check=True)

            cmd = "ip -6 addr show dev eth0.4088"
            result = subprocess.run(
                cmd.split(),
                capture_output=True,
                text=True,
                check=True
            )

            if subnet_addr in result.stdout:
                print("Start subnetwork Success.")
                return

            print("Start subnetwork Failed, address verification failed, retrying: %d" %
                  (retry + 1))

        except subprocess.CalledProcessError as e:
            print("Start subnetwork Failed, retrying: %d, cmd: %s, returncode: %d" %
                  (retry + 1, cmd, e.returncode))

        retry = retry + 1
        time.sleep(5)

    print("Start subnetwork Failed: %s not configured after %d retries." %
          (subnet_addr, retry_count))
    sys.exit(1)


def stop():
    subnet_path = "/sys/class/net/eth0.4088"

    if not os.path.exists(subnet_path):
        print("Stop subnetwork Success.")
        return

    subnet_cmds = []
    subnet_cmds.append("ip link set dev eth0.4088 down")
    subnet_cmds.append("ip link del eth0.4088")

    try:
        for cmd in subnet_cmds:
            subprocess.run(cmd.split(), check=True)
    except subprocess.CalledProcessError as e:
        print("Stop subnetwork Failed, cmd: %s, returncode: %d" %
              (cmd, e.returncode))
        sys.exit(1)

    if not os.path.exists(subnet_path):
        print("Stop subnetwork Success.")
    else:
        print("Stop subnetwork Failed: interface still exists.")
        sys.exit(1)


def main():
    if len(sys.argv) != 2:
        print("Error parameter!\nRequired parameters : start or stop.")
        sys.exit(1)

    if sys.argv[1] == 'start':
        start()
    elif sys.argv[1] == 'stop':
        stop()
    else:
        print("Error parameter!\nRequired parameters : start or stop.")
        sys.exit(1)


if __name__ == '__main__':
    main()