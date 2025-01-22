#
# fanutil.py
# Platform-specific FAN status interface for SONiC
#

import sys
from sonic_py_common.general import getstatusoutput_noshell

SENSORS_CMD = ["docker", "exec", "-i", "pmon", "/usr/bin/sensors"]
DOCKER_SENSORS_CMD = "/usr/bin/sensors"


try:
    from sonic_fan.fan_base import FanBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class FanUtil(FanBase):
    """Platform-specific FanUtil class"""
    _fan_mapping = {
        1 : '0',
        2 : '1',
        3 : '2'
    }

    def __init__(self):
        FanBase.__init__(self)

    def isDockerEnv(self):
        num_docker = open('/proc/self/cgroup', 'r').read().count(":/docker")
        if num_docker > 0:
            return True

    def get_num_fans(self):
        E3224F_MAX_FANTRAYS = 3
        return E3224F_MAX_FANTRAYS

    def get_presence(self, idx):
        sysfs_path = "/sys/devices/platform/dell-e3224f-cpld.0/fan" + self._fan_mapping[idx] + "_prs"
        return int(open(sysfs_path).read(), 16)

    def get_direction(self, idx):
        sysfs_path = "/sys/devices/platform/dell-e3224f-cpld.0/fan" + self._fan_mapping[idx] + "_dir"
        return open(sysfs_path).read()

    def get_speed(self, idx):
        dockerenv = self.isDockerEnv()
        if not dockerenv:
            status, cmd_output = getstatusoutput_noshell(SENSORS_CMD)
        else:
            status, cmd_output = getstatusoutput_noshell(DOCKER_SENSORS_CMD)
        if status:
            print('Failed to execute sensors command')
            sys.exit(0)
        fan_id = 'Fan ' + str(idx)
        found = False
        for line in cmd_output.splitlines():
            if line.startswith('emc2305-i2c-7-2c'):
                found = True
            if found and line.startswith(fan_id):
                return line.split()[3]
        return 0.0

    def get_status(self, idx):
        sysfs_path = "/sys/devices/platform/dell-e3224f-cpld.0/fan" + self._fan_mapping[idx] + "_prs"
        return int(open(sysfs_path).read(), 16)


    def set_speed(self, idx):
        return False
