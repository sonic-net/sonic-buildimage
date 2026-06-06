#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
import os
import re
import subprocess
import sys
import time


def start():
    subnet_path = "/sys/class/net/eth0.4088"
    retry_count = 10
    subnet_cmds = []
    subnet_cmds.append("ip link add link eth0 name eth0.4088 type vlan id 4088")
    subnet_cmds.append("ip addr add 240.1.1.1/30 brd 240.1.1.3 dev eth0.4088")
    subnet_cmds.append("ip link set dev eth0.4088 up")

    retry = 0
    while not os.path.exists(subnet_path) and retry < retry_count:
        try:
            for cmd in subnet_cmds:
                subprocess.run(cmd.split(), check=True)
        except subprocess.CalledProcessError as e:
            print("Start subnetwork Failed, retrying: %d, cmd: %s, returncode: %d" % (retry, cmd, e.returncode))
        retry = retry + 1
        time.sleep(5)

    if os.path.exists(subnet_path):
        print("Start subnetwork Success.")


def stop():
    subnet_path = "/sys/class/net/eth0.4088"
    subnet_cmds = []
    subnet_cmds.append("ip link set dev eth0.4088 down")
    subnet_cmds.append("ip link del eth0.4088")

    try:
        for cmd in subnet_cmds:
            subprocess.run(cmd.split(), check=True)
    except subprocess.CalledProcessError as e:
        print("Stop subnetwork Failed, returncode: " + e.returncode)

    if not os.path.exists(subnet_path):
        print("Stop subnetwork Success.")


def main():
    print(sys.argv[1])
    if sys.argv[1] == 'start':
        start()
    elif sys.argv[1] == 'stop':
        stop()
    else:
        print("Error parameter!\nRequired parameters : start or stop.")


if __name__ == '__main__':
    main()
