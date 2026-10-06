#!/usr/bin/env python

import os
import time

import sonic_platform
from swsscommon.swsscommon import ConfigDBConnector
from sonic_py_common import logger


DEBUG = False


PORT_TABLE_NAME = "PORT"
PORT_XCVR_LASER_FREQ_FIELD_NAME = "laser_freq"

SYSLOG_IDENTIFIER = "sfpshow_daemon"
log = logger.Logger(SYSLOG_IDENTIFIER)

freq_status = {}
pre_presence = {}

platform_chassis = None


def main():
    global platform_chassis
    try:
        platform_chassis = sonic_platform.platform.Platform().get_chassis()
    except Exception as e:
        log.log_error("Failed to instantiate Chassis due to {}".format(repr(e)))
        return

    config_db = ConfigDBConnector()
    config_db.connect()
    while True:
        port_tables = config_db.get_table(PORT_TABLE_NAME)

        for port in port_tables:
            lport = int(port_tables[port]["index"])
            sfp_obj = platform_chassis.get_sfp(lport)
            presence = platform_chassis.get_sfp(lport).get_presence()
            if not presence:
                if port in freq_status:
                    del freq_status[port]
                if DEBUG:
                    print("{} is not presence".format(lport))
                continue

            if PORT_XCVR_LASER_FREQ_FIELD_NAME not in port_tables[port]:
                if DEBUG:
                    print("{} is not configured the frequency.".format(lport))
                continue
            freq = float(port_tables[port][PORT_XCVR_LASER_FREQ_FIELD_NAME])

            if port in freq_status and freq_status[port] == freq:
                if DEBUG:
                    print("{} Config is not changed.".format(lport))
                continue

            _, _, _, lowf, highf = sfp_obj.get_supported_freq_config()
            if freq < lowf:
                print("{} configured freq:{} GHz is lower than the supported freq:{} GHz".format(
                    port, freq, lowf))
            if freq > highf:
                print("{} configured freq:{} GHz is higher than the supported freq:{} GHz".format(
                    port, freq, highf))
            if sfp_obj.get_tuning_in_progress():
                print("{} Tuning in progress, subport selection may fail!".format(port))
            if not sfp_obj.get_support_fine_tuning():
                print("{} does not support fine tune!!!".format(port))

            rc = sfp_obj.set_laser_freq(freq)
            if not rc:
                if DEBUG:
                    print("{} Can not apply the frequency due to module type is not ZR.".format(lport))

            try:
                file_path = "/sys/bus/i2c/devices/i2c-0/0-00{:02x}".format(0x10 + lport - 1) + "/freq"
                with open(file_path, "r") as f:
                    channel = int(f.read())
                file_path = "/sys/bus/i2c/devices/i2c-0/0-00{:02x}".format(0x10 + lport - 1) + "/fine_tune_freq"
                with open(file_path, "r") as f:
                    fine_tune_freq = int(f.read())
            except IOError:
                print("Can not open file {}.".format(file_path))
                freq_status[port] = freq

            new_freq = 193100 + \
                   (channel if channel < 2**(16 - 1) else channel - 2**16) * 25 + \
                   (fine_tune_freq if fine_tune_freq < 2**(16 - 1) else fine_tune_freq - 2**16) * 0.001
            if new_freq == freq:
                freq_status[port] = freq
            else:
                if DEBUG:
                    print("New frequency is {} {} {}".format(new_freq,
                                                             (channel if channel < 2**(16 - 1) else channel - 2**16) * 25,
                                                             (fine_tune_freq if fine_tune_freq < 2**(16 - 1) else fine_tune_freq - 2**16) * 0x001))
                if sfp_obj.get_support_fine_tuning():
                    lo_fine_tune_freq, hi_fine_tune_freq = sfp_obj.get_supported_fine_tune_freq_config()
                    fine_tune_freq = (fine_tune_freq if fine_tune_freq < 2**(16 - 1) else fine_tune_freq - 2**16) * 0.001
                    if DEBUG:
                        print("{} {} {}".format(lo_fine_tune_freq, hi_fine_tune_freq, fine_tune_freq))
                    if fine_tune_freq < lo_fine_tune_freq:
                        print("{} configured freq:{} GHz is lower than the supported fine tune freq:{} GHz".format(
                            port, fine_tune_freq, lo_fine_tune_freq))
                    if hi_fine_tune_freq < fine_tune_freq:
                        print("{} configured freq:{} GHz is higher than the supported fine tune freq:{} GHz".format(
                            port, fine_tune_freq, hi_fine_tune_freq))
        time.sleep(5)


if __name__ == '__main__':
    main()
