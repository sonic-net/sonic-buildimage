#!/usr/bin/env python
# -*- coding: utf-8 -*
# Copyright (c) 2019 Edgecore Networks Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
# LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
# FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
#
# See the Apache Version 2.0 License for specific language governing
# permissions and limitations under the License.
#
# HISTORY:
#    mm/dd/yyyy (A.D.)#
#    04/23/2021: Michael_Shih create for as9726_32d thermal plan
#    11/23/2023: Roger
#                1. Sync the log buffer to the disk before 
#                   powering off the DUT.
#                2. Change the decision of FAN direction
#                3. Enhance test data
# ------------------------------------------------------------------

try:
    import subprocess
    import getopt
    import sys
    import logging
    import logging.config
    import logging.handlers
    import signal
    import time  # this is only being used as part of the example
    from as9726_32d.fanutil import FanUtil
    from as9726_32d.thermalutil import ThermalUtil
    from swsscommon import swsscommon
    from sonic_py_common.general import getstatusoutput_noshell
    from sonic_platform import platform
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Deafults
VERSION = '1.0'
FUNCTION_NAME = '/usr/local/bin/accton_as9726_32d_monitor'

STATE_DB = 'STATE_DB'
TRANSCEIVER_DOM_SENSOR_TABLE = 'TRANSCEIVER_DOM_SENSOR'
TEMPERATURE_FIELD_NAME = 'temperature'

class switch(object):
    def __init__(self, value):
        self.value = value
        self.fall = False

    def __iter__(self):
        """Return the match method once, then stop"""
        yield self.match
        raise StopIteration

    def match(self, *args):
        """Indicate whether or not to enter a case suite"""
        if self.fall or not args:
            return True
        elif self.value in args: # changed for v1.5, see below
            self.fall = True
            return True
        else:
            return False



# Read fanN_direction=1: The air flow of Fan6 is “AFI-Back to Front”
#                     0: The air flow of Fan6 is “AFO-Front to back”
#
# Thermal policy:
# a.Defaut fan duty_cycle=100%
# b.One fan fail, set to fan duty_cycle=100%
# 1.For AFI:
#   Default fan duty_cycle will be 100%(fan_policy_state=LEVEL_FAN_MAX).
#   If all below case meet with, set to 75%(LEVEL_FAN_MID).
#   MB board
#   (MB board)
#   LM75-1(0X48)>=49.5
#   LM75-2(0X49)>=42.9
#   LM75-3(0X4A)>=46.3
#   LM75-4(0X4C)>=40.1
#   LM75-6(0X4F)>=39.4
#   (CPU board)
#   Core>=46
#   LM75-1(0X4B)>=34.8

#   When fan_policy_state=LEVEL_FAN_MID, meet with below case,  Fan duty_cycle will be 100%(LEVEL_FAN_DAX)
#   (MB board)
#   LM75-1(0X48)>=55.9
#   LM75-2(0X49)>=48.8
#   LM75-3(0X4A)>=51.5
#   LM75-4(0X4C)>=45.3
#   LM75-6(0X4F)>=43.4
#   (CPU board)
#   Core>=50
#   LM75-1(0X4B)>=43.4
#   Transceiver >=65

#Yellow Alarm
#MB board
#LM75-1(0X48)>=57.9
#LM75-2(0X49)>=51.9
#LM75-3(0X4A)>=48.9
#LM75-4(0X4C)>=55.9
#LM75-6(0X4F)>=48.5
#CPU Board
#Core>=52
#LM75-1(0X4B)>=41.8
#Transceiver >=73

#Red Alarm
#MB board
#LM75-1(0X48)>=62.9
#LM75-2(0X49)>=56.9
#LM75-3(0X4A)>=53.9
#LM75-4(0X4C)>=58.9
#LM75-6(0X4F)>=53.5
#CPU Board
#Core>=57
#LM75-1(0X4B)>=46.8
#Transceiver >=75

#Shut down
#MB board
#LM75-1(0X48)>=67.9
#LM75-2(0X49)>=61.9
#LM75-3(0X4A)>=58.9
#LM75-4(0X4C)>=63.9
#LM75-6(0X4F)>=58.5
#CPU Board
#Core>=62
#LM75-1(0X4B)>=51.8
#Transceiver >=77

# 2. For AFO:
#  At default, FAN duty_cycle was 100%(LEVEL_FAN_MAX). If all below case meet with, set to 75%(LEVEL_FAN_MID).
# (MB board)
# LM75-1(0X48)<=56
# LM75-2(0X49)<=53.5
# LM75-3(0X4A)<=52.5
# LM75-4(0X4C)<=52
# LM75-6(0X4F)<=52.8
# (CPU board)
# Core<=62
# LM75-1(0X4B)<=45.8

# When FAN duty_cycle was 75%(LEVEL_FAN_MID). If all below case meet with, set to 50%(LEVEL_FAN_DEF).
# (MB board)
# LM75-1(0X48)<=50
# LM75-2(0X49)<=47.3
# LM75-3(0X4A)<=46.4
# LM75-4(0X4C)<=44.6
# LM75-6(0X4F)<=47
# (CPU board)
# Core<=56
# LM75-1(0X4B)<=38.8

# When fan_speed 50%(LEVEL_FAN_DEF).
# Meet with below case, Fan duty_cycle will be 75%(LEVEL_FAN_MID)
# (MB board)
# LM75-1(0X48)>=63
# LM75-2(0X49)>=60.5
# LM75-3(0X4A)>=60
# LM75-4(0X4C)>=60
# LM75-6(0X4F)>=61
# (CPU board)
# Core>=72
# LM75-1(0X4B)>=50
# Transceiver >=55

# When FAN duty_cycle was 75%(LEVEL_FAN_MID). If all below case meet with, set to 100%(LEVEL_FAN_MAX).
# (MB board)
# LM75-1(0X48)>=63
# LM75-2(0X49)>=60
# LM75-3(0X4A)>=60
# LM75-4(0X4C)>=59
# LM75-6(0X4F)>=60

# (CPU board)
# Core >=69
# LM75-1(0X4B)>=51.5
# Transceiver >=65


#Yellow Alarm
#MB board
#LM75-1(0X48)>=67
#LM75-2(0X49)>=65
#LM75-3(0X4A)>=64
#LM75-4(0X4C)>=62
#LM75-6(0X4F)>=64
#CPU Board
#Core>=73
#LM75-1(0X4B)>=67
#Transceiver >=73

#Red Alarm
#MB board
#LM75-1(0X48)>=72
#LM75-2(0X49)>=70
#LM75-3(0X4A)>=69
#LM75-4(0X4C)>=67
#LM75-6(0X4F)>=69
#CPU Board
#Core>=78
#LM75-1(0X4B)>=72
#Transceiver >=75

#Shut down
#MB board
#LM75-1(0X48)>=77
#LM75-2(0X49)>=75
#LM75-3(0X4A)>=74
#LM75-4(0X4C)>=72
#LM75-6(0X4F)>=74
#CPU Board
#Core>=83
#LM75-1(0X4B)>=77
#Transceiver >=77

def power_off_dut():
    # Sync log buffer to disk
    cmd_str = ["sync"]
    status, output = getstatusoutput_noshell(cmd_str)
    cmd_str = ["/sbin/fstrim", "-av"]
    status, output = getstatusoutput_noshell(cmd_str)
    time.sleep(3)

    # Power off dut
    cmd_str = ["i2cset", "-y", "-f", "19", "0x60", "0x60", "0x10"]
    (status, output) = getstatusoutput_noshell(cmd_str)
    return (status == 0)

def shutdown_transceiver(iface_name):
    cmd_str = ["config", "interface", "shutdown", str(iface_name)]
    (status, output) = getstatusoutput_noshell(cmd_str)
    return (status == 0)

#If only one PSU insert(or one of PSU pwoer fail), and watt >800w. Must let DUT fan pwm >= 75% in AFO.
# Because the psu temp is high.
# Return 1: full load
# Return 0: Not full load
def check_psu_loading():
    psu_power_status=[1, 1]

    psu_power_good = {
        2: "/sys/bus/i2c/devices/9-0051/psu_power_good",
        1: "/sys/bus/i2c/devices/9-0050/psu_power_good",
    }
    psu_power_out = {
        2: "/sys/bus/i2c/devices/9-0059/psu_p_out",
        1: "/sys/bus/i2c/devices/9-0058/psu_p_out",
    }

    check_psu_watt=0
    for i in range(1,3):
        node = psu_power_good[i]
        try:
            with open(node, 'r') as power_status:
                status = int(power_status.read())
        except IOError:
            return None

        psu_power_status[i-1]=int(status)
        if status==0:
            check_psu_watt=1

    if check_psu_watt:
        for i in range(1,3):
            if psu_power_status[i-1]==1:
              #check watt
                node = psu_power_out[i]
                try:
                    with open(node, 'r') as power_status:
                        status = int(power_status.read())
                except IOError:
                    return None
                psu_p_out= int(status)
                if psu_p_out/1000 > 800:
                    return True
    else:
        return False

    return False

fan_policy_state=0
fan_policy_alarm=0
send_yellow_alarm=0
send_red_alarm=0
fan_fail=0
count_check=0

test_temp = 0
test_temp_list = [0] * (7 + 16) # 7 Thermal, 16 ZR/ZR+ Thermal
temp_test_data=0
test_temp_revert=0

exit_by_sigterm=0

platform_chassis= None

# Make a class we can use to capture stdout and sterr in the log
class device_monitor(object):
    # static temp var
    temp = 0
    new_duty_cycle = 0
    duty_cycle=0
    ori_duty_cycle = 0


    def __init__(self, log_file, log_level):
        """Needs a logger and a logger level."""

        self.thermal = ThermalUtil()
        self.fan = FanUtil()
        # set up logging to file
        logging.basicConfig(
            filename=log_file,
            filemode='w',
            level=log_level,
            format= '[%(asctime)s] {%(pathname)s:%(lineno)d} %(levelname)s - %(message)s',
            datefmt='%H:%M:%S'
        )
        # set up logging to console
        if log_level == logging.DEBUG:
            console = logging.StreamHandler()
            console.setLevel(log_level)
            formatter = logging.Formatter('%(asctime)s %(name)-12s: %(levelname)-8s %(message)s', datefmt='%H:%M:%S')
            console.setFormatter(formatter)
            logging.getLogger('').addHandler(console)

        sys_handler = logging.handlers.SysLogHandler(address = '/dev/log')
        sys_handler.setLevel(logging.WARNING)
        logging.getLogger('').addHandler(sys_handler)
        #logging.debug('SET. logfile:%s / loglevel:%d', log_file, log_level)

        self.transceiver_dom_sensor_table = None

    def get_transceiver_temperature(self, iface_name):
        if self.transceiver_dom_sensor_table is None:
            return 0.0

        (status, ret) = self.transceiver_dom_sensor_table.hget(iface_name, TEMPERATURE_FIELD_NAME)
        if status:
            try:
                return float(ret)
            except (TypeError, ValueError):
                pass

        return 0.0

    def manage_fans(self):

        global platform_chassis
        global fan_policy_state
        global fan_policy_alarm
        global send_yellow_alarm
        global send_red_alarm
        global fan_fail
        global count_check
        global test_temp
        global test_temp_list
        global temp_test_data
        global test_temp_revert

        CHECK_TIMES=3

        LEVEL_FAN_INIT=0
        LEVEL_FAN_MIN=1
        LEVEL_FAN_MID=2
        LEVEL_FAN_MAX=3
        LEVEL_FAN_YELLOW_ALARM=4
        LEVEL_FAN_RED_ALARM=5
        LEVEL_FAN_SHUTDOWN=6

        fan_policy_f2b = {  #AFO
            LEVEL_FAN_MIN: [50,  0x7],
            LEVEL_FAN_MID: [75,  0xb],
            LEVEL_FAN_MAX: [100, 0xf]
        }
        fan_policy_b2f = { #AFI
            LEVEL_FAN_MID: [75,  0xb],
            LEVEL_FAN_MAX: [100, 0xf]
        }

        TYPE_SENSOR = 1
        TYPE_TRANSCEIVER = 2
        # Support ZR/ZR+ Allocation Port
        monitor_port = [1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31]
        TRANSCEIVER_NUM_MAX = len(monitor_port)

        afi_thermal_spec = {
            "mid_to_max_temp"      : [(TYPE_SENSOR, 55900), (TYPE_SENSOR, 48800), (TYPE_SENSOR, 51500), (TYPE_SENSOR, 45300),
                                      (TYPE_SENSOR, 43400), (TYPE_SENSOR, 50000), (TYPE_SENSOR, 43400)],
            "max_to_mid_temp"      : [(TYPE_SENSOR, 49500), (TYPE_SENSOR, 42900), (TYPE_SENSOR, 46300), (TYPE_SENSOR, 40100),
                                      (TYPE_SENSOR, 39400), (TYPE_SENSOR, 46000), (TYPE_SENSOR, 34800)],
            "max_to_yellow_alarm"  : [(TYPE_SENSOR, 57900), (TYPE_SENSOR, 51900), (TYPE_SENSOR, 48900), (TYPE_SENSOR, 55900),
                                      (TYPE_SENSOR, 48500), (TYPE_SENSOR, 52000), (TYPE_SENSOR, 41800)],
            "yellow_to_red_alarm"  : [(TYPE_SENSOR, 62900), (TYPE_SENSOR, 56900), (TYPE_SENSOR, 53900), (TYPE_SENSOR, 58900),
                                      (TYPE_SENSOR, 53500), (TYPE_SENSOR, 57000), (TYPE_SENSOR, 46800)],
            "red_alarm_to_shutdown": [(TYPE_SENSOR, 67900), (TYPE_SENSOR, 61900), (TYPE_SENSOR, 58900), (TYPE_SENSOR, 63900),
                                      (TYPE_SENSOR, 58500), (TYPE_SENSOR, 62000), (TYPE_SENSOR, 51800)]
        }
        afi_thermal_spec["mid_to_max_temp"]       += [(TYPE_TRANSCEIVER, 65000)]*TRANSCEIVER_NUM_MAX
        afi_thermal_spec["max_to_mid_temp"]       += [(TYPE_TRANSCEIVER, 64000)]*TRANSCEIVER_NUM_MAX
        afi_thermal_spec["max_to_yellow_alarm"]   += [(TYPE_TRANSCEIVER, 73000)]*TRANSCEIVER_NUM_MAX
        afi_thermal_spec["yellow_to_red_alarm"]   += [(TYPE_TRANSCEIVER, 75000)]*TRANSCEIVER_NUM_MAX
        afi_thermal_spec["red_alarm_to_shutdown"] += [(TYPE_TRANSCEIVER, 77000)]*TRANSCEIVER_NUM_MAX

        afo_thermal_spec = {
            "min_to_mid_temp"      : [(TYPE_SENSOR, 63000), (TYPE_SENSOR, 60500), (TYPE_SENSOR, 60000), (TYPE_SENSOR, 60000),
                                      (TYPE_SENSOR, 61000), (TYPE_SENSOR, 72000), (TYPE_SENSOR, 50000)],
            "mid_to_max_temp"      : [(TYPE_SENSOR, 63000), (TYPE_SENSOR, 60000), (TYPE_SENSOR, 60000), (TYPE_SENSOR, 59000),
                                      (TYPE_SENSOR, 60000), (TYPE_SENSOR, 69000), (TYPE_SENSOR, 51500)],
            "max_to_mid_temp"      : [(TYPE_SENSOR, 56000), (TYPE_SENSOR, 53500), (TYPE_SENSOR, 52500), (TYPE_SENSOR, 52000),
                                      (TYPE_SENSOR, 52800), (TYPE_SENSOR, 62000), (TYPE_SENSOR, 45800)],
            "mid_to_min_temp"      : [(TYPE_SENSOR, 50000), (TYPE_SENSOR, 47300), (TYPE_SENSOR, 46400), (TYPE_SENSOR, 44600),
                                      (TYPE_SENSOR, 47000), (TYPE_SENSOR, 56000), (TYPE_SENSOR, 38800)],
            "max_to_yellow_alarm"  : [(TYPE_SENSOR, 67000), (TYPE_SENSOR, 65000), (TYPE_SENSOR, 64000), (TYPE_SENSOR, 62000),
                                      (TYPE_SENSOR, 64000), (TYPE_SENSOR, 73000), (TYPE_SENSOR, 67000)],
            "yellow_to_red_alarm"  : [(TYPE_SENSOR, 72000), (TYPE_SENSOR, 70000), (TYPE_SENSOR, 69000), (TYPE_SENSOR, 67000),
                                      (TYPE_SENSOR, 69000), (TYPE_SENSOR, 78000), (TYPE_SENSOR, 72000)],
            "red_alarm_to_shutdown": [(TYPE_SENSOR, 77000), (TYPE_SENSOR, 75000), (TYPE_SENSOR, 74000), (TYPE_SENSOR, 72000),
                                      (TYPE_SENSOR, 74000), (TYPE_SENSOR, 83000), (TYPE_SENSOR, 77000)]
        }
        afo_thermal_spec["min_to_mid_temp"]       += [(TYPE_TRANSCEIVER, 55000)]*TRANSCEIVER_NUM_MAX
        afo_thermal_spec["mid_to_max_temp"]       += [(TYPE_TRANSCEIVER, 65000)]*TRANSCEIVER_NUM_MAX
        afo_thermal_spec["max_to_mid_temp"]       += [(TYPE_TRANSCEIVER, 64000)]*TRANSCEIVER_NUM_MAX
        afo_thermal_spec["mid_to_min_temp"]       += [(TYPE_TRANSCEIVER, 54000)]*TRANSCEIVER_NUM_MAX
        afo_thermal_spec["max_to_yellow_alarm"]   += [(TYPE_TRANSCEIVER, 73000)]*TRANSCEIVER_NUM_MAX
        afo_thermal_spec["yellow_to_red_alarm"]   += [(TYPE_TRANSCEIVER, 75000)]*TRANSCEIVER_NUM_MAX
        afo_thermal_spec["red_alarm_to_shutdown"] += [(TYPE_TRANSCEIVER, 77000)]*TRANSCEIVER_NUM_MAX

        thermal_val = []
        max_to_mid=0
        mid_to_min=0

        # After booting, the database might not be ready for
        # connection. So, it should try to connect to the database
        # if self.transceiver_dom_sensor_table is None.
        if self.transceiver_dom_sensor_table is None:
            try:
                state_db = swsscommon.DBConnector(STATE_DB, 0, False)
                self.transceiver_dom_sensor_table = swsscommon.Table(state_db, TRANSCEIVER_DOM_SENSOR_TABLE)
            except Exception as e:
                logging.debug("{}".format(e))

        fan = self.fan
        if fan_policy_state==LEVEL_FAN_INIT:
            fan_policy_state=LEVEL_FAN_MAX #This is default state
            logging.debug("fan_policy_state=LEVEL_FAN_MAX")
            return

        count_check=count_check+1
        if count_check < CHECK_TIMES:
            return
        else:
            count_check=0

        thermal = self.thermal
        f2b_dir = 0
        b2f_dir = 0
        for i in range (fan.FAN_NUM_1_IDX, fan.FAN_NUM_ON_MAIN_BROAD+1):
            if fan.get_fan_present(i)==0:
                continue
            b2f_dir += fan.get_fan_dir(i) == 1
            f2b_dir += fan.get_fan_dir(i) == 0
        logging.debug("b2f_dir={} f2b_dir={}".format(b2f_dir, f2b_dir))
        fan_dir = int(b2f_dir >= f2b_dir)

        if fan_dir==1:   # AFI
            fan_thermal_spec = afi_thermal_spec
            fan_policy=fan_policy_b2f
            logging.debug("fan_policy = fan_policy_b2f")
        elif fan_dir==0: # AFO
            fan_thermal_spec = afo_thermal_spec
            fan_policy=fan_policy_f2b
            logging.debug("fan_policy = fan_policy_f2b")
        else:
            logging.debug( "NULL case")

        ori_duty_cycle=fan.get_fan_duty_cycle()
        new_duty_cycle=0

        if test_temp_revert==0:
            temp_test_data=temp_test_data+2000
        else:
            temp_test_data=temp_test_data-2000

        if test_temp==0:
            for i in range(thermal.THERMAL_NUM_MAX):
                thermal_val.append((TYPE_SENSOR, None, thermal._get_thermal_val(i+1)))

            for port_num in monitor_port:
                sfp = platform_chassis.get_sfp(port_num)
                thermal_val.append((TYPE_TRANSCEIVER, sfp,
                                    self.get_transceiver_temperature(sfp.get_name()) * 1000))

            logging.debug("Maximum avaliable port : %d", TRANSCEIVER_NUM_MAX)
            logging.debug(thermal_val)
        else:
            for i in range(thermal.THERMAL_NUM_MAX):
                thermal_val.append((TYPE_SENSOR, None, test_temp_list[i] + temp_test_data))
            for port_num in monitor_port:
                sfp = platform_chassis.get_sfp(port_num)
                thermal_val.append((TYPE_TRANSCEIVER, sfp, test_temp_list[i + 1] + temp_test_data))
                i = i + 1
            logging.debug(thermal_val)
            fan_fail=0

        ori_state=fan_policy_state;
        current_state=fan_policy_state;
        sfp_presence_num = 0

        if fan_dir==1: #AFI
            for i in range (0, thermal.THERMAL_NUM_MAX + TRANSCEIVER_NUM_MAX):
                (temp_type, obj, current_temp) = thermal_val[i]

                sfp = None
                if temp_type == TYPE_TRANSCEIVER:
                    sfp = obj
                    if sfp.get_presence():
                        sfp_presence_num += 1
                    else:
                        continue

                if ori_state==LEVEL_FAN_MID:
                    if current_temp >= fan_thermal_spec["mid_to_max_temp"][i][1]:
                        current_state=LEVEL_FAN_MAX
                        logging.debug("current_state=LEVEL_FAN_MAX")
                        break
                else:
                    if current_temp <= fan_thermal_spec["max_to_mid_temp"][i][1]:
                        max_to_mid=max_to_mid+1
                    if fan_policy_alarm==0:
                        if current_temp >= fan_thermal_spec["max_to_yellow_alarm"][i][1]:
                            if send_yellow_alarm==0:
                                logging.warning('Alarm-Yellow for temperature high is detected')
                                fan_policy_alarm=LEVEL_FAN_YELLOW_ALARM
                                send_yellow_alarm=1
                    elif fan_policy_alarm==LEVEL_FAN_YELLOW_ALARM:
                        if current_temp >= fan_thermal_spec["yellow_to_red_alarm"][i][1]:
                            if send_red_alarm==0:
                                logging.warning('Alarm-Red for temperature high is detected')
                                fan_policy_alarm=LEVEL_FAN_RED_ALARM
                                send_red_alarm=1
                    elif fan_policy_alarm==LEVEL_FAN_RED_ALARM:
                        if current_temp >= fan_thermal_spec["red_alarm_to_shutdown"][i][1]:
                            if fan_thermal_spec["yellow_to_red_alarm"][i][0] == TYPE_SENSOR:
                                logging.critical('Alarm-Critical for temperature high is detected, shutdown DUT')
                                fan_policy_alarm=LEVEL_FAN_SHUTDOWN
                                time.sleep(2)
                                power_off_dut()
                            elif fan_thermal_spec["yellow_to_red_alarm"][i][0] == TYPE_TRANSCEIVER:
                                if shutdown_transceiver(sfp.get_name()):
                                    logging.critical('Alarm-Critical for temperature high is detected, shutdown %s', sfp.get_name())

            if max_to_mid==(thermal.THERMAL_NUM_MAX + sfp_presence_num) and  fan_policy_state==LEVEL_FAN_MAX:
                current_state=LEVEL_FAN_MID
                if fan_policy_alarm!=0:
                    logging.warning('Alarm for temperature high is cleared')
                    fan_policy_alarm=0
                    send_yellow_alarm=0
                    send_red_alarm=0
                    test_temp_revert=0
                logging.debug("current_state=LEVEL_FAN_MID")

        else: #AFO
            psu_full_load=check_psu_loading()
            for i in range (0, thermal.THERMAL_NUM_MAX + TRANSCEIVER_NUM_MAX):
                (temp_type, obj, current_temp) = thermal_val[i]

                sfp = None
                if temp_type == TYPE_TRANSCEIVER:
                    sfp = obj
                    if sfp.get_presence():
                        sfp_presence_num += 1
                    else:
                        continue
                if ori_state==LEVEL_FAN_MID:
                    if current_temp >= fan_thermal_spec["mid_to_max_temp"][i][1]:
                        current_state=LEVEL_FAN_MAX
                        break
                    else:
                        if psu_full_load!=True and current_temp <= fan_thermal_spec["mid_to_min_temp"][i][1]:
                            mid_to_min=mid_to_min+1

                elif ori_state==LEVEL_FAN_MIN:
                    if psu_full_load==True:
                        current_state=LEVEL_FAN_MID
                        logging.debug("psu_full_load, set current_state=LEVEL_FAN_MID")
                    if current_temp >= fan_thermal_spec["min_to_mid_temp"][i][1]:
                        current_state=LEVEL_FAN_MID

                else:
                    if current_temp <= fan_thermal_spec["max_to_mid_temp"][i][1]:
                        max_to_mid=max_to_mid+1
                    if fan_policy_alarm==0:
                        if current_temp >= fan_thermal_spec["max_to_yellow_alarm"][i][1]:
                            if send_yellow_alarm==0:
                                logging.warning('Alarm-Yellow for temperature high is detected')
                                fan_policy_alarm=LEVEL_FAN_YELLOW_ALARM
                                send_yellow_alarm=1
                    elif fan_policy_alarm==LEVEL_FAN_YELLOW_ALARM:
                        if current_temp >= fan_thermal_spec["yellow_to_red_alarm"][i][1]:
                            if send_red_alarm==0:
                                logging.warning('Alarm-Red for temperature high is detected')
                                fan_policy_alarm=LEVEL_FAN_RED_ALARM
                                send_red_alarm=1
                    elif fan_policy_alarm==LEVEL_FAN_RED_ALARM:
                        if current_temp >= fan_thermal_spec["red_alarm_to_shutdown"][i][1]:
                            if fan_thermal_spec["red_alarm_to_shutdown"][i][0] == TYPE_SENSOR:
                                logging.critical('Alarm-Critical for temperature high is detected, shutdown DUT')
                                fan_policy_alarm=LEVEL_FAN_SHUTDOWN
                                time.sleep(2)
                                power_off_dut()
                            elif fan_thermal_spec["red_alarm_to_shutdown"][i][0] == TYPE_TRANSCEIVER:
                                if shutdown_transceiver(sfp.get_name()):
                                    logging.critical('Alarm-Critical for temperature high is detected, shutdown %s', sfp.get_name())

            if max_to_mid==(thermal.THERMAL_NUM_MAX + sfp_presence_num) and ori_state==LEVEL_FAN_MAX:
                current_state=LEVEL_FAN_MID
                if fan_policy_alarm!=0:
                    logging.warning('Alarm for temperature high is cleared')
                    fan_policy_alarm=0
                    send_yellow_alarm=0
                    send_red_alarm=0
                    test_temp_revert=0
                logging.debug("current_state=LEVEL_FAN_MID")

            if mid_to_min==(thermal.THERMAL_NUM_MAX + sfp_presence_num) and ori_state==LEVEL_FAN_MID:
                if psu_full_load==0:
                    current_state=LEVEL_FAN_MIN
                    logging.debug("current_state=LEVEL_FAN_MIN")

        #Check Fan fault status. True: fan not fault/present, 1: fan fault/un-present
        for i in range (fan.FAN_NUM_1_IDX, fan.FAN_NUM_ON_MAIN_BROAD+1):
            if fan.get_fan_status(i)==False:
                new_duty_cycle=100
                current_state=LEVEL_FAN_MAX
                logging.debug('fan_%d fail, set duty_cycle to 100',i)
                if test_temp==0:
                    fan_fail=1
                    fan.set_fan_duty_cycle(new_duty_cycle)
                    break
            else:
                fan_fail=0

        if current_state!=ori_state:
            fan_policy_state=current_state
            new_duty_cycle=fan_policy[current_state][0]
            logging.debug("fan_policy_state=%d, new_duty_cycle=%d", fan_policy_state, new_duty_cycle)
            if new_duty_cycle!=ori_duty_cycle and fan_fail==0:
                fan.set_fan_duty_cycle(new_duty_cycle)
                return True
            if new_duty_cycle==0 and fan_fail==0:
                fan.set_fan_duty_cycle(FAN_DUTY_CYCLE_MAX)

        return True

def signal_handler(sig, frame):
    global exit_by_sigterm
    if sig == signal.SIGTERM:
        print("Caught SIGTERM - exiting...")
        exit_by_sigterm = 1
    else:
        pass

def main(argv):
    log_file = '%s.log' % FUNCTION_NAME
    log_level = logging.INFO
    global test_temp
    global exit_by_sigterm
    signal.signal(signal.SIGTERM, signal_handler)
    if len(sys.argv) != 1:
        try:
            opts, args = getopt.getopt(argv,'hdlt:',['lfile='])
        except getopt.GetoptError:
            print('Usage: %s [-d] [-l <log_file>]' % sys.argv[0])
            return 0
        for opt, arg in opts:
            if opt == '-h':
                print('Usage: %s [-d] [-l <log_file>]' % sys.argv[0])
                return 0
            elif opt in ('-d', '--debug'):
                log_level = logging.DEBUG
            elif opt in ('-l', '--lfile'):
                log_file = arg

        if sys.argv[1]== '-t':
            if len(sys.argv)!=(2 + 7 + 16): # 7 Thermal, 16 ZR/ZR+ Thermal
                print("temp test, need input %d temp" % (7 + 16))
                return 0
            i=0
            for x in range(2, (2 + 7 + 16)): # 7 Thermal, 16 ZR/ZR+ Thermal
               test_temp_list[i]= int(sys.argv[x])*1000
               i=i+1
            test_temp = 1
            log_level = logging.DEBUG
            print(test_temp_list)

    global platform_chassis
    platform_chassis = platform.Platform().get_chassis()

    fan = FanUtil()
    fan.set_fan_duty_cycle(100)
    monitor = device_monitor(log_file, log_level)
    # Loop forever, doing something useful hopefully:
    while True:
        monitor.manage_fans()
        time.sleep(10)
        if exit_by_sigterm == 1:
            break

if __name__ == '__main__':
    main(sys.argv[1:])

