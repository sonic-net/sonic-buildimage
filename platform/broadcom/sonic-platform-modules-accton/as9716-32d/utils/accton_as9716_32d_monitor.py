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
#    8/27/2019:Jostar create for as9716_32d thermal plan
#    3/23/2023:Roger Add ZR port thermal plan
# ------------------------------------------------------------------

try:
    import subprocess
    import getopt, sys, os
    import logging
    import logging.config
    import logging.handlers
    import signal
    import time
    from as9716_32d.fanutil import FanUtil
    from as9716_32d.thermalutil import ThermalUtil
    from swsscommon import swsscommon
    from sonic_platform import platform
    from sonic_py_common.general import getstatusoutput_noshell
    from sonic_platform.helper import APIHelper
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Deafults
VERSION = '1.0'
FUNCTION_NAME = '/usr/local/bin/accton_as9716_32d_monitor'

FAN_DUTY_CYCLE_MAX = 100
STATE_DB = 'STATE_DB'
TRANSCEIVER_DOM_SENSOR_TABLE = 'TRANSCEIVER_DOM_SENSOR'
TEMPERATURE_FIELD_NAME = 'temperature'

HOST_PMONITOR_ALARM_FILE = "/usr/share/sonic/device/{}/platform_monitor_alarm"

global log_file
global log_level

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
#   LM75-1(0X48)<=45.5
#   LM75-2(0X49)<=39.5
#   LM75-3(0X4A)<=37.5
#   LM75-4(0X4C)<=38.5
#   LM75-5(0X4E)<=34.5
#   LM75-6(0X4F)<=37
#   CPU board
#   Core(1~4) <=40
#   LM75-1(0X4B)<=30.5
#   ZR<=62

#   When fan_policy_state=LEVEL_FAN_MID, meet with below case,  Fan duty_cycle will be 100%(LEVEL_FAN_DAX)
#   (MB board)
#   LM75-1(0X48)>=51.5
#   LM75-2(0X49)>=44.5
#   LM75-3(0X4A)>=43.5
#   LM75-4(0X4C)>=43.5
#   LM75-5(0X4E)>=40
#   LM75-6(0X4F)>=42.5
#   (CPU board)
#   Core-1>=45, Core-2>=45, Core-3>=46, Core-4>=46
#   LM75-1(0X4B)>=35.5
#   ZR>=65

#   Red Alarm
#   MB board
#   LM75-1(0X48)>=65
#   LM75-2(0X49)>=58
#   LM75-3(0X4A)>=57
#   LM75-4(0X4C)>=57
#   LM75-5(0X4E)>=57
#   LM75-6(0X4F)>=60
#   CPU Board
#   Core>=60
#   LM75-1(0X4B)>=50
#   ZR>=75

#   Shutdown
#   MB board
#   LM75-1(0X48)>=71
#   LM75-2(0X49)>=64
#   LM75-3(0X4A)>=63
#   LM75-4(0X4C)>=63
#   LM75-5(0X4E)>=63
#   LM75-6(0X4F)>=66
#   CPU Board
#   Core>=66
#   LM75-1(0X4B)>=56
#   ZR>=82


# 2.For AFO:
#   At default, FAN duty_cycle was 100%(LEVEL_FAN_MAX). If all below case meet with, set to 75%(LEVEL_FAN_MID).
#   (MB board)
#   LM75-1(0X48)<=47
#   LM75-2(0X49)<=47
#   LM75-3(0X4A)<=47
#   LM75-4(0X4C)<=47
#   LM75-5(0X4E)<=47
#   LM75-6(0X4F)<=47
#   (CPU board)
#   Core-(1~4)<=55
#   LM75-1(0X4B)<=40
#   ZR<=60

#   When FAN duty_cycle was 75%(LEVEL_FAN_MID). If all below case meet with, set to 50%(LEVEL_FAN_DEF).
#   (MB board)
#   LM75-1(0X48)<=40
#   LM75-2(0X49)<=40
#   LM75-3(0X4A)<=40
#   LM75-4(0X4C)<=40
#   LM75-5(0X4E)<=40
#   LM75-6(0X4F)<=40
#   (CPU board)
#   Core-(1~4)<=50
#   LM75-1(0X4B)<=33
#   ZR<=55

#   When fan_speed 50%(LEVEL_FAN_DEF).
#   Meet with below case, Fan duty_cycle will be 75%(LEVEL_FAN_MID)
#   (MB board)
#   LM75-1(0X48)>=63
#   LM75-2(0X49)>=63
#   LM75-3(0X4A)>=63
#   LM75-4(0X4C)>=63
#   LM75-5(0X4E)>=63
#   LM75-6(0X4F)>=63
#   (CPU board)
#   Core-(1~4)>=73
#   LM75-1(0X4B)>=50
#   ZR>=65

#   When FAN duty_cycle was 75%(LEVEL_FAN_MID). If all below case meet with, set to 100%(LEVEL_FAN_MAX).
#   (MB board)
#   LM75-1(0X48)>=68
#   LM75-2(0X49)>=68
#   LM75-3(0X4A)>=68
#   LM75-4(0X4C)>=68
#   LM75-5(0X4E)>=68
#   LM75-6(0X4F)>=68
#   (CPU board)
#   Core-(1~4)>=77
#   LM75-1(0X4B)>=55
#   ZR>=70

#   Red Alarm
#   MB board
#   LM75-1(0X48)>=72
#   LM75-2(0X49)>=72
#   LM75-3(0X4A)>=72
#   LM75-4(0X4C)>=72
#   LM75-5(0X4E)>=72
#   LM75-6(0X4F)>=72
#   CPU Board
#   Core>=81
#   LM75-1(0X4B)>=60
#   ZR>=75

#   Shutdown
#   MB board
#   LM75-1(0X48)>=78
#   LM75-2(0X49)>=78
#   LM75-3(0X4A)>=78
#   LM75-4(0X4C)>=78
#   LM75-5(0X4E)>=78
#   LM75-6(0X4F)>=78
#   CPU Board
#   Core>=87
#   LM75-1(0X4B)>=70
#   ZR>=82




def power_off_dut():
    # Sync log buffer to disk
    cmd_str="sync"
    status, output = subprocess.getstatusoutput(cmd_str)
    cmd_str="/sbin/fstrim -av"
    status, output = subprocess.getstatusoutput(cmd_str)
    time.sleep(3)

    # Power off dut
    cmd_str="i2cset -y -f 19 0x60 0x60 0x10"
    status, output = subprocess.getstatusoutput(cmd_str)
    return status

def shutdown_transceiver(iface_name):
    cmd_str="config interface shutdown {}".format(iface_name)
    status, output = subprocess.getstatusoutput(cmd_str)
    return (status == 0)

#If only one PSU insert(or one of PSU pwoer fail), and watt >800w. Must let DUT fan pwm >= 75% in AFO.
#Because the psu temp is high.
# Return 1: full load
# Return 0: Not full load
def check_psu_loading():
    psu_power_status=[1, 1]

    psu_power_good = {
        2: "/sys/bus/i2c/devices/10-0051/psu_power_good",
        1: "/sys/bus/i2c/devices/9-0050/psu_power_good",
    }
    psu_power_in = {
        2: "/sys/bus/i2c/devices/10-0059/psu_p_in",
        1: "/sys/bus/i2c/devices/9-0058/psu_p_in",
    }
    psu_power_out = {
        2: "/sys/bus/i2c/devices/10-0059/psu_p_out",
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
                node = psu_power_in[i]
                try:
                    with open(node, 'r') as power_status:
                        status = int(power_status.read())
                except IOError:
                    return None

                psu_p_in= int(status)
                if psu_p_in/1000 > 800:
                    return True

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
send_red_alarm=0
fan_fail=0
count_check=0

test_temp = 0
test_temp_list = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
temp_test_data=0
test_temp_revert=0

exit_by_sigterm=0

platform_chassis= None

int_port_mapping = {}

# Make a class we can use to capture stdout and sterr in the log
class device_monitor(object):
    # static temp var
    temp = 0
    new_duty_cycle = 0
    duty_cycle=0
    ori_duty_cycle = 0


    def __init__(self, log_file, log_level):
        """Needs a logger and a logger level."""

        self.fan = FanUtil()
        self.thermal = ThermalUtil()
        self._api_helper = APIHelper()
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
        sys_handler.setLevel(logging.INFO)
        formatter = logging.Formatter('#%(module)s: %(message)s')
        sys_handler.setFormatter(formatter)
        logging.getLogger('').addHandler(sys_handler)
        #logging.debug('SET. logfile:%s / loglevel:%d', log_file, log_level)

        self.transceiver_dom_sensor_table = None
        self.platform_monitor_alarm_path = None

        # Initialize the platform monitor alarm file.
        platform = self._api_helper.get_platform()
        if platform is not None:
            self.platform_monitor_alarm_path = HOST_PMONITOR_ALARM_FILE.format(platform)
            self.update_platform_monitor_alarm(fan_policy_alarm)

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

    def get_platform_monitor_alarm_path(self):
        return self.platform_monitor_alarm_path

    def update_platform_monitor_alarm(self, state):
        if self.platform_monitor_alarm_path is not None:
            return self._api_helper.write_txt_file(self.platform_monitor_alarm_path, state)

        return False

    def manage_fans(self):

        global platform_chassis
        global fan_policy_state
        global fan_policy_alarm
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
        LEVEL_FAN_MAX=3 #LEVEL_FAN_DEF
        LEVEL_FAN_RED_ALARM=4

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

        # ZR Allocation
        monitor_port = [5, 6, 11, 12, 19, 20, 31, 32]
        TRANSCEIVER_NUM_MAX = len(monitor_port)

        # afi_thermal_spec = {
        #     "...............": [0x48, 0x49,
        #                         0x4a, 0x4c,
        #                         0x4e, 0x4f,
        #                         CPU Core, 0x4b]
        # afo_thermal_spec = {
        #     "...............": [0x48, 0x49,
        #                         0x4a, 0x4c,
        #                         0x4e, 0x4f,
        #                         CPU Core, 0x4b]

        afi_thermal_spec = {
            "mid_to_max_temp": [(TYPE_SENSOR, 51500), (TYPE_SENSOR, 44500),
                                (TYPE_SENSOR, 43500), (TYPE_SENSOR, 43500),
                                (TYPE_SENSOR, 40000), (TYPE_SENSOR, 42500),
                                (TYPE_SENSOR, 45000), (TYPE_SENSOR, 35500)],
            "max_to_mid_temp": [(TYPE_SENSOR, 45500), (TYPE_SENSOR, 39500),
                                (TYPE_SENSOR, 37500), (TYPE_SENSOR, 38500),
                                (TYPE_SENSOR, 34500), (TYPE_SENSOR, 37000),
                                (TYPE_SENSOR, 40000), (TYPE_SENSOR, 30500)],
            "max_to_red_alarm": [(TYPE_SENSOR, 65000), (TYPE_SENSOR, 58000),
                                 (TYPE_SENSOR, 57000), (TYPE_SENSOR, 57000),
                                 (TYPE_SENSOR, 57000), (TYPE_SENSOR, 60000),
                                 (TYPE_SENSOR, 60000), (TYPE_SENSOR, 50000)],
            "red_alarm_to_shutdown": [(TYPE_SENSOR, 71000), (TYPE_SENSOR, 64000),
                                 (TYPE_SENSOR, 63000), (TYPE_SENSOR, 63000),
                                 (TYPE_SENSOR, 63000), (TYPE_SENSOR, 66000),
                                 (TYPE_SENSOR, 66000), (TYPE_SENSOR, 56000)],
        }
        afi_thermal_spec["mid_to_max_temp"] += [(TYPE_TRANSCEIVER, 65000)] * TRANSCEIVER_NUM_MAX
        afi_thermal_spec["max_to_mid_temp"] += [(TYPE_TRANSCEIVER, 62000)] * TRANSCEIVER_NUM_MAX
        afi_thermal_spec["max_to_red_alarm"] += [(TYPE_TRANSCEIVER, 75000)] * TRANSCEIVER_NUM_MAX
        afi_thermal_spec["red_alarm_to_shutdown"] += [(TYPE_TRANSCEIVER, 82000)] * TRANSCEIVER_NUM_MAX

        afo_thermal_spec = {
            "min_to_mid_temp": [(TYPE_SENSOR, 63000), (TYPE_SENSOR, 63000),
                                (TYPE_SENSOR, 63000), (TYPE_SENSOR, 63000),
                                (TYPE_SENSOR, 63000), (TYPE_SENSOR, 63000),
                                (TYPE_SENSOR, 73000), (TYPE_SENSOR, 50000)],
            "mid_to_max_temp": [(TYPE_SENSOR, 68000), (TYPE_SENSOR, 68000),
                                (TYPE_SENSOR, 68000), (TYPE_SENSOR, 68000),
                                (TYPE_SENSOR, 68000), (TYPE_SENSOR, 68000),
                                (TYPE_SENSOR, 77000), (TYPE_SENSOR, 55000)],
            "max_to_mid_temp": [(TYPE_SENSOR, 47000), (TYPE_SENSOR, 47000),
                                (TYPE_SENSOR, 47000), (TYPE_SENSOR, 47000),
                                (TYPE_SENSOR, 47000), (TYPE_SENSOR, 47000),
                                (TYPE_SENSOR, 55000), (TYPE_SENSOR, 40000)],
            "mid_to_min_temp": [(TYPE_SENSOR, 40000), (TYPE_SENSOR, 40000),
                                (TYPE_SENSOR, 40000), (TYPE_SENSOR, 40000),
                                (TYPE_SENSOR, 40000), (TYPE_SENSOR, 40000),
                                (TYPE_SENSOR, 50000), (TYPE_SENSOR, 33000)],
            "max_to_red_alarm": [(TYPE_SENSOR, 72000), (TYPE_SENSOR, 72000),
                                 (TYPE_SENSOR, 72000), (TYPE_SENSOR, 72000),
                                 (TYPE_SENSOR, 72000), (TYPE_SENSOR, 72000),
                                 (TYPE_SENSOR, 81000), (TYPE_SENSOR, 60000)],
            "red_alarm_to_shutdown": [(TYPE_SENSOR, 78000), (TYPE_SENSOR, 78000),
                                 (TYPE_SENSOR, 78000), (TYPE_SENSOR, 78000),
                                 (TYPE_SENSOR, 78000), (TYPE_SENSOR, 78000),
                                 (TYPE_SENSOR, 87000), (TYPE_SENSOR, 70000)],
        }
        afo_thermal_spec["min_to_mid_temp"] += [(TYPE_TRANSCEIVER, 65000)] * TRANSCEIVER_NUM_MAX
        afo_thermal_spec["mid_to_max_temp"] += [(TYPE_TRANSCEIVER, 70000)] * TRANSCEIVER_NUM_MAX
        afo_thermal_spec["max_to_mid_temp"] += [(TYPE_TRANSCEIVER, 60000)] * TRANSCEIVER_NUM_MAX
        afo_thermal_spec["mid_to_min_temp"] += [(TYPE_TRANSCEIVER, 55000)] * TRANSCEIVER_NUM_MAX
        afo_thermal_spec["max_to_red_alarm"] += [(TYPE_TRANSCEIVER, 75000)] * TRANSCEIVER_NUM_MAX
        afo_thermal_spec["red_alarm_to_shutdown"] += [(TYPE_TRANSCEIVER, 82000)] * TRANSCEIVER_NUM_MAX

        thermal_val=[]
        max_to_mid=0
        mid_to_min=0

        # After booting, the database might not be ready for
        # connection. So, it should try to connect to the database
        # if self.transceiver_dom_sensor_table is None.
        # NOTE: the main loop calls 'is_database_ready()' to ensure the redis server
        # is ready. So the exception handler here takes effect only when the redis server is down
        # suddenly after database.service is up.
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

            # Record port mapping to interface
            for port_num in monitor_port:
                list=[]
                sfp = platform_chassis.get_sfp(port_num)
                list.append(sfp)
                list.append(sfp.get_name())
                int_port_mapping[port_num] = list

            return True

        count_check=count_check+1
        if count_check < CHECK_TIMES:
            return True
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

        if fan_dir==1: # AFI
            fan_thermal_spec = afi_thermal_spec
            fan_policy=fan_policy_b2f
            logging.debug("fan_policy = fan_policy_b2f")
        elif fan_dir==0:          # AFO
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
            for i in range(thermal.THERMAL_NUM_1_IDX, thermal.THERMAL_NUM_MAX+1):
                thermal_val.append((TYPE_SENSOR, None,thermal._get_thermal_val(i)))

            for port_num in monitor_port:
                sfp = platform_chassis.get_sfp(port_num)
                thermal_val.append((TYPE_TRANSCEIVER, int_port_mapping[port_num][0],
                                    self.get_transceiver_temperature(int_port_mapping[port_num][1]) * 1000))
        else:
            for i in range (thermal.THERMAL_NUM_1_IDX, thermal.THERMAL_NUM_MAX+1):
                thermal_val.append((TYPE_SENSOR, None, test_temp_list[i-1] + temp_test_data))
            additional_test_data = 0
            for port_num in monitor_port:
                sfp = platform_chassis.get_sfp(port_num)
                thermal_val.append((TYPE_TRANSCEIVER, int_port_mapping[port_num][0],
                                    test_temp_list[i] + temp_test_data + additional_test_data))
                i = i + 1
                additional_test_data += 4000
            fan_fail=0

        logging.debug("Maximum avaliable port : %d", TRANSCEIVER_NUM_MAX)
        logging.debug("thermal_val : %s", thermal_val)

        ori_state=fan_policy_state
        current_state=fan_policy_state

        if fan_dir==1: #AFI
            sfp_presence_num = 0
            for i in range (0, thermal.THERMAL_NUM_MAX + TRANSCEIVER_NUM_MAX):
                (temp_type, obj, current_temp) = thermal_val[i]

                thermal_idx = thermal.THERMAL_NUM_1_IDX + i
                sfp = None
                if temp_type == TYPE_TRANSCEIVER:
                    sfp = obj
                    if sfp.get_presence():
                        sfp_presence_num += 1
                    else:
                        if test_temp==0:
                            continue
                        else:
                            sfp_presence_num += 1

                if ori_state==LEVEL_FAN_MID:
                    if current_temp >= fan_thermal_spec["mid_to_max_temp"][i][1]:
                        current_state=LEVEL_FAN_MAX
                        if fan_thermal_spec["mid_to_max_temp"][i][0] == TYPE_SENSOR:
                            name = thermal.get_thermal_name(thermal_idx)
                        elif fan_thermal_spec["mid_to_max_temp"][i][0] == TYPE_TRANSCEIVER:
                            name = "port {}".format(sfp.port_num)
                        logging.warning('Monitor %s, temperature is %.1f. Temperature is over the error threshold(%.1f) of thermal policy.',
                                        name,
                                        current_temp/1000,
                                        fan_thermal_spec["mid_to_max_temp"][i][1]/1000)
                        break
                else:
                    if current_temp <= fan_thermal_spec["max_to_mid_temp"][i][1]:
                        max_to_mid=max_to_mid+1
                    if fan_policy_alarm==0:
                        if current_temp >= fan_thermal_spec["max_to_red_alarm"][i][1]:
                            if send_red_alarm==0:
                                if fan_thermal_spec["max_to_red_alarm"][i][0] == TYPE_SENSOR:
                                    name = thermal.get_thermal_name(thermal_idx)
                                elif fan_thermal_spec["max_to_red_alarm"][i][0] == TYPE_TRANSCEIVER:
                                    name = "port {}".format(sfp.port_num)
                                logging.warning('Monitor %s, temperature is %.1f. Temperature is over the critical threshold(%.1f) of thermal policy.',
                                                name,
                                                current_temp/1000,
                                                fan_thermal_spec["max_to_red_alarm"][i][1]/1000)
                                fan_policy_alarm = LEVEL_FAN_RED_ALARM
                                send_red_alarm = 1
                                self.update_platform_monitor_alarm(fan_policy_alarm)
                    elif fan_policy_alarm==LEVEL_FAN_RED_ALARM:
                        if current_temp >= fan_thermal_spec["red_alarm_to_shutdown"][i][1]:
                            if fan_thermal_spec["red_alarm_to_shutdown"][i][0] == TYPE_SENSOR:
                                logging.critical('Monitor %s, temperature is %.1f. Temperature is over the shutdown threshold(%.1f) of thermal policy, shutdown DUT.',
                                                 thermal.get_thermal_name(thermal_idx),
                                                 current_temp/1000,
                                                 fan_thermal_spec["red_alarm_to_shutdown"][i][1]/1000)
                                time.sleep(2)
                                power_off_dut()

            if max_to_mid==(thermal.THERMAL_NUM_MAX + sfp_presence_num) and  fan_policy_state==LEVEL_FAN_MAX:
                current_state=LEVEL_FAN_MID
                if fan_policy_alarm!=0:
                    fan_policy_alarm=0
                    send_red_alarm=0
                    test_temp_revert=0
                    self.update_platform_monitor_alarm(fan_policy_alarm)
                logging.info('Monitor all sensors, temperature is less than the warning threshold of thermal policy.')
        else: #AFO
            sfp_presence_num = 0
            psu_full_load=check_psu_loading()
            for i in range (0, thermal.THERMAL_NUM_MAX + TRANSCEIVER_NUM_MAX):
                (temp_type, obj, current_temp) = thermal_val[i]

                thermal_idx = thermal.THERMAL_NUM_1_IDX + i
                sfp = None
                if temp_type == TYPE_TRANSCEIVER:
                    sfp = obj
                    if sfp.get_presence():
                        sfp_presence_num += 1
                    else:
                        if test_temp==0:
                            continue
                        else:
                            sfp_presence_num += 1

                if ori_state==LEVEL_FAN_MID:
                    if current_temp >= fan_thermal_spec["mid_to_max_temp"][i][1]:
                        current_state=LEVEL_FAN_MAX
                        if fan_thermal_spec["mid_to_max_temp"][i][0] == TYPE_SENSOR:
                            name = thermal.get_thermal_name(thermal_idx)
                        elif fan_thermal_spec["mid_to_max_temp"][i][0] == TYPE_TRANSCEIVER:
                            name = "port {}".format(sfp.port_num)
                        logging.warning('Monitor %s, temperature is %.1f. Temperature is over the error threshold(%.1f) of thermal policy.',
                                        name,
                                        current_temp/1000,
                                        fan_thermal_spec["mid_to_max_temp"][i][1]/1000)
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
                        if fan_thermal_spec["min_to_mid_temp"][i][0] == TYPE_SENSOR:
                            name = thermal.get_thermal_name(thermal_idx)
                        elif fan_thermal_spec["min_to_mid_temp"][i][0] == TYPE_TRANSCEIVER:
                            name = "port {}".format(sfp.port_num)
                        logging.warning('Monitor %s, temperature is %.1f. Temperature is over the warning threshold(%.1f) of thermal policy.',
                                        name,
                                        current_temp/1000,
                                        fan_thermal_spec["min_to_mid_temp"][i][1]/1000)

                else:
                    if current_temp <= fan_thermal_spec["max_to_mid_temp"][i][1] :
                        max_to_mid=max_to_mid+1
                    if fan_policy_alarm==0:
                        if current_temp >= fan_thermal_spec["max_to_red_alarm"][i][1]:
                            if send_red_alarm==0:
                                if fan_thermal_spec["max_to_red_alarm"][i][0] == TYPE_SENSOR:
                                    name = thermal.get_thermal_name(thermal_idx)
                                elif fan_thermal_spec["max_to_red_alarm"][i][0] == TYPE_TRANSCEIVER:
                                    name = "port {}".format(sfp.port_num)
                                logging.warning('Monitor %s, temperature is %.1f. Temperature is over the critical threshold(%.1f) of thermal policy.',
                                                name,
                                                current_temp/1000,
                                                fan_thermal_spec["max_to_red_alarm"][i][1]/1000)
                                fan_policy_alarm=LEVEL_FAN_RED_ALARM
                                send_red_alarm=1
                                self.update_platform_monitor_alarm(fan_policy_alarm)
                    elif fan_policy_alarm==LEVEL_FAN_RED_ALARM:
                        if current_temp >= fan_thermal_spec["red_alarm_to_shutdown"][i][1]:
                            if fan_thermal_spec["red_alarm_to_shutdown"][i][0] == TYPE_SENSOR:
                                logging.critical('Monitor %s, temperature is %.1f. Temperature is over the shutdown threshold(%.1f) of thermal policy, shutdown DUT.',
                                                 thermal.get_thermal_name(thermal_idx),
                                                 current_temp/1000,
                                                 fan_thermal_spec["red_alarm_to_shutdown"][i][1]/1000)
                                time.sleep(2)
                                power_off_dut()

            if max_to_mid==(thermal.THERMAL_NUM_MAX + sfp_presence_num) and ori_state==LEVEL_FAN_MAX:
                current_state=LEVEL_FAN_MID
                if fan_policy_alarm!=0:
                    fan_policy_alarm=0
                    send_red_alarm=0
                    test_temp_revert=0
                    self.update_platform_monitor_alarm(fan_policy_alarm)
                logging.info('Monitor all sensors, temperature is less than the error threshold of thermal policy.')

            if mid_to_min==(thermal.THERMAL_NUM_MAX + sfp_presence_num) and ori_state==LEVEL_FAN_MID:
                if psu_full_load==0:
                    current_state=LEVEL_FAN_MIN
                    logging.info('Monitor all sensors, temperature is less than the warning threshold of thermal policy.')

        #Check Fan status
        for i in range (fan.FAN_NUM_1_IDX, fan.FAN_NUM_ON_MAIN_BROAD+1):
            if fan.get_fan_status(i)==False:
                new_duty_cycle=FAN_DUTY_CYCLE_MAX
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
            if current_state > ori_state:
                logging.warning('Increase fan duty_cycle from %d%% to %d%%.', fan_policy[ori_state][0], new_duty_cycle)
            else:
                logging.info('Decrease fan duty_cycle from %d%% to %d%%.', fan_policy[ori_state][0], new_duty_cycle)
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

def is_database_ready():
    cmd_str = ["systemctl", "is-active", "database.service"]
    (status, output) = getstatusoutput_noshell(cmd_str)
    if output == "active":
        return True
    else:
        return False

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
            if len(sys.argv)!=18:
                print("temp test, need input 16 temp")
                return 0
            i=0
            for x in range(2, 18):
                test_temp_list[i]= int(sys.argv[x])*1000
                i=i+1
            test_temp = 1
            log_level = logging.DEBUG
            print(test_temp_list)

    global platform_chassis
    platform_chassis = platform.Platform().get_chassis()

    fan = FanUtil()
    fan.set_fan_duty_cycle(FAN_DUTY_CYCLE_MAX)
    monitor = device_monitor(log_file, log_level)
    try:
        # Loop forever, doing something useful hopefully:
        while True:
            monitor.manage_fans()
            time.sleep(10)
            if exit_by_sigterm == 1:
                break
    finally:
        file_path = monitor.get_platform_monitor_alarm_path()
        if file_path is not None and os.path.exists(file_path):
            os.remove(file_path)

if __name__ == '__main__':
    main(sys.argv[1:])
