#!/usr/bin/env python3
try:
    import subprocess
    import re
    import time
    from sonic_platform import platform
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

sonic_logger = logger.Logger('thermal_shutdown')

# Thermal Shutdown
# CPU Board
# Core>=80
# TMP75_2(0x4F)>=80
# Main board
# TMP75_0(0x48)>=80
# TMP75_1(0x49)>=80
# ASIC maximum peak temperature>=108

def power_off_device():
    command = "sync"
    subprocess.getstatusoutput(command)
    time.sleep(3)
    command = "ipmitool chassis power off"
    subprocess.getstatusoutput(command)

def get_asic_temperature():
    command = "bcmcmd \"show temp\" | grep \"maximum peak temperature\""
    status, output = subprocess.getstatusoutput(command)
    if status:
        sonic_logger.log_warning("Failed to get asic temperature.")
        return 0

    temperature = [float(s) for s in re.findall(r'-?\d+\.?\d*', output)]
    return temperature[0]


class thermal_shutdown_monitor(object):
    # Critical temperatures
    CRIT_CPU_TEMPERATURE = 80
    CRIT_ASIC_TEMPERATURE = 108
    CRIT_THERMAL_TEMPERATURE = 80
    CRIT_THERMAL_NAME = "TMP75"

    def monitor(self):
        global platform_chassis

        # Check asic temperature
        asic_temperature = get_asic_temperature()
        if asic_temperature >= self.CRIT_ASIC_TEMPERATURE:
            sonic_logger.log_warning("ASIC temperature {} is over critical ASIC temperature {}, shutdown device.".format(
                                        asic_temperature, self.CRIT_ASIC_TEMPERATURE))
            power_off_device()
            return True

        # Check cpu temperature
        cpu_temperature = platform_chassis.get_thermal(0).get_cpu_temperature()
        if cpu_temperature >= self.CRIT_CPU_TEMPERATURE:
            sonic_logger.log_warning("CPU temperature {} is over critial CPU temperature {}, shutdown device.".format(
                                        cpu_temperature, self.CRIT_CPU_TEMPERATURE))
            power_off_device()
            return True

        # Check thermal temperature
        for thermal in platform_chassis.get_all_thermals():
            if self.CRIT_THERMAL_NAME in thermal.get_name():
                thermal_temperature = thermal.get_temperature()
                if thermal_temperature >= self.CRIT_THERMAL_TEMPERATURE:
                    sonic_logger.log_warning("Thermal {} temperature {} is over critial thermal temperature {}, shutdown device.".format(
                                                thermal.get_name(), thermal_temperature, self.CRIT_THERMAL_TEMPERATURE))
                    power_off_device()
                    return True
        return True


def main():
    global platform_chassis
    platform_chassis = platform.Platform().get_chassis()

    # thermal shutdown monitor
    monitor = thermal_shutdown_monitor()
    POLL_INTERVAL=30

    # start monitor
    while True:
        monitor.monitor()
        time.sleep(POLL_INTERVAL)


if __name__ == '__main__':
    main()
