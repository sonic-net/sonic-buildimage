#!/usr/bin/env python3
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

def doBash(cmd):
    try:
        status, output = subprocess.getstatusoutput(cmd)
        return status, output
    except Exception as e:
        return 1, ""

def pmon_critical_service_check():

    """
    Check pmon critical services are running

    Args:
        None

    Returns:
        None

    Stdout:
        When pmon is running, print the following:
            pmon:OK
            pmon_fancontrol:OK
            pmon_ledd:Not OK
            pmon_lm-sensors:OK
            pmon_psud:OK
            pmon_syseepromd:OK
            pmon_thermalctld:OK
            pmon_watchdogd:Not OK
            pmon_xcvrd:OK
        When pmon is not running, print the following:
            pmon:Not OK
    """

    pmon_critical_service_list = [
        "fancontrol",
        "ledd",
        "sensord",
        "pmon_daemon:psud",
        "pmon_daemon:syseepromd",
        "pmon_daemon:thermalctld",
        "pmon_daemon:watchdogd",
        "pmon_daemon:xcvrd"
    ]

    # check docker pmon is running
    status, output = doBash("systemctl is-active pmon")
    if status == 0 and output.strip() == "active":
        print("pmon:OK")
        for service in pmon_critical_service_list:
            if service == "sensord":
                status, output = doBash("docker exec pmon service sensord status")
                if status != 0 and "not running" in output:
                    print("pmon_sensord:Not OK")
                else:
                    print("pmon_sensord:OK")
                continue
            status, output = doBash("docker exec pmon supervisorctl status " + service)
            if service.startswith("pmon_daemon:"):
                service = service.replace("pmon_daemon:", "pmon_")
            else:
                service = "pmon_" + service
            if status == 0 and "RUNNING" in output.upper():
                print("{}:OK".format(service))
            else:
                print("{}:Not OK".format(service))
    else:
        print("pmon:Not OK")

def platform_critical_service_check():

    """
    Check platform critical services are running

    Args:
        None

    Returns:
        None

    Stdout:
        Print the following:
            clounix-common-platform-init:OK/Not OK
            pddf-platform-init:OK/Not OK
            pddf-s3ip-init:OK/Not OK
    """
    platform_critical_service_list = [
        "clounix-common-platform-init",
        "pddf-platform-init",
        "pddf-s3ip-init"
    ]

    # check platform critical services are running
    for service in platform_critical_service_list:
        status, output = doBash("systemctl is-active " + service)
        if status == 0 and output.strip() == "active":
            print("{}:OK".format(service))
        else:
            print("{}:Not OK".format(service))

def main():
    print("Services")
    pmon_critical_service_check()
    platform_critical_service_check()

if __name__ == "__main__":
    main()