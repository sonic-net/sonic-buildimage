#!/usr/bin/env python3
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
#    2/20/2023:Jostar modify to add kick fan-wdt
#    3/23/2023:Roger Add ZR port thermal plan
#   11/23/2023:Roger
#              1. Sync the log buffer to the disk before
#                 powering off the DUT.
#              2. Change the decision of FAN direction
#              3. Enhance test data
# ------------------------------------------------------------------

try:
    import sys
    import getopt
    import logging
    import logging.config
    import logging.handlers
    import time
    from swsscommon import swsscommon
    from sonic_py_common.general import getstatusoutput_noshell
    from sonic_platform import platform
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Deafults
VERSION = '1.0'
FUNCTION_NAME = '/usr/local/bin/accton_as9716_32d_pddf_monitor'

FAN_DUTY_CYCLE_MAX = 100
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
        elif self.value in args:  # changed for v1.5, see below
            self.fall = True
            return True
        else:
            return False

# Read fanN_direction=1: The air flow of Fan is AFI-Back to Front
#                     0: The air flow of Fan is AFO-Front to Back
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
#   (MB board)
#   LM75-1(0X48)>=55.5
#   LM75-2(0X49)>=48.5
#   LM75-3(0X4A)>=47.5
#   LM75-4(0X4C)>=47.5
#   LM75-5(0X4E)>=47
#   LM75-6(0X4F)>=49.5
#   (CPU board)
#   Core(1~4) >=50
#   LM75-1(0X4B)>=40
#   ZR>=75

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


def as9716_32d_set_fan_speed(pwm):

    if pwm < 0 or pwm > 100:
        print(("Error: Wrong duty cycle value %d" % (pwm)))
        return -1
    platform_chassis.get_fan(0).set_speed(pwm)
    time.sleep(1)
    return 0


def power_off_dut():
    # Sync log buffer to disk
    cmd_str=["sync"]
    (status, output) = getstatusoutput_noshell(cmd_str)
    cmd_str=["/sbin/fstrim", "-av"]
    (status, output) = getstatusoutput_noshell(cmd_str)
    time.sleep(3)

    # Power off dut
    cmd_str = ["i2cset", "-y", "-f", "19", "0x60", "0x60", "0x10"]
    (status, output) = getstatusoutput_noshell(cmd_str)
    return status

def shutdown_transceiver(iface_name):
    cmd_str = ["config", "interface", "shutdown", iface_name]
    (status, output) = getstatusoutput_noshell(cmd_str)
    return (status == 0)

# If only one PSU insert(or one of PSU pwoer fail), and watt >800w. Must let DUT fan pwm >= 75% in AFO.
# Because the psu temp is high.
# Return 1: full load
# Return 0: Not full load


def check_psu_loading():
    global platform_chassis

    check_psu_watt = 0
    for i in range(2):
        if platform_chassis.get_psu(i).get_powergood_status() == 0:
            check_psu_watt = 1  # If one psu power is not good, need to do another psu checking

    if check_psu_watt:
        for i in range(2):
            if platform_chassis.get_psu(i).get_powergood_status() == 1:
                psu_p_out = platform_chassis.get_psu(i).get_power()
                if psu_p_out > 800:
                    return True
    else:
        return False

    return False


fan_policy_state = 0
fan_policy_alarm = 0
send_red_alarm = 0
fan_fail = 0
count_check = 0

test_temp = 0
test_temp_list = [0] * (8 + 8) # 8 Thermal, 8 ZR/ZR+ Thermal
temp_test_data = 0
test_temp_revert = 0
platform_chassis = None
# Make a class we can use to capture stdout and sterr in the log


class device_monitor(object):
    # static temp var
    temp = 0
    new_duty_cycle = 0
    duty_cycle = 0
    ori_duty_cycle = 0

    def __init__(self, log_file, log_level):
        """Needs a logger and a logger level."""
        # set up logging to file
        logging.basicConfig(
            filename=log_file,
            filemode='w',
            level=log_level,
            format='[%(asctime)s] {%(pathname)s:%(lineno)d} %(levelname)s - %(message)s',
            datefmt='%H:%M:%S'
        )
        # set up logging to console
        if log_level == logging.DEBUG:
            console = logging.StreamHandler()
            console.setLevel(log_level)
            formatter = logging.Formatter('[%(asctime)s] %(name)-12s: %(levelname)-8s %(message)s', datefmt='%H:%M:%S')
            console.setFormatter(formatter)
            logging.getLogger('').addHandler(console)

        sys_handler = logging.handlers.SysLogHandler(address='/dev/log')
        sys_handler.setLevel(logging.WARNING)
        logging.getLogger('').addHandler(sys_handler)

        self.transceiver_dom_sensor_table = None

    def __get_transceiver_temperature(self, iface_name):
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
        global fan_policy_state
        global fan_policy_alarm
        global send_red_alarm
        global fan_fail
        global count_check
        global test_temp
        global test_temp_list
        global temp_test_data
        global test_temp_revert
        global platform_chassis

        # 30 seonds
        CHECK_TIMES = 3

        LEVEL_FAN_INIT = 0
        LEVEL_FAN_MIN = 1
        LEVEL_FAN_MID = 2
        LEVEL_FAN_MAX = 3  # LEVEL_FAN_DEF
        LEVEL_FAN_RED_ALARM = 4
        THERMAL_NUM_MAX = 8

        FAN_NUM = 2
        FAN_TRAY_NUM = 6

        fan_policy_f2b = {  # AFO
            LEVEL_FAN_MIN: [50,  0x7],
            LEVEL_FAN_MID: [75,  0xb],
            LEVEL_FAN_MAX: [100, 0xf]
        }
        fan_policy_b2f = {  # AFI
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
        #                         0x4a, 0x4b,
        #                         0x4c, 0x4e,
        #                         0x4f, CPU Core]
        # afo_thermal_spec = {
        #     "...............": [0x48, 0x49,
        #                         0x4a, 0x4b,
        #                         0x4c, 0x4e,
        #                         0x4f, CPU Core]

        afi_thermal_spec = {
            "mid_to_max_temp": [(TYPE_SENSOR, 51500), (TYPE_SENSOR, 44500),
                                (TYPE_SENSOR, 43500), (TYPE_SENSOR, 35500),
                                (TYPE_SENSOR, 43500), (TYPE_SENSOR, 40000),
                                (TYPE_SENSOR, 42500), (TYPE_SENSOR, 45000)],
            "max_to_mid_temp": [(TYPE_SENSOR, 45500), (TYPE_SENSOR, 39500),
                                (TYPE_SENSOR, 37500), (TYPE_SENSOR, 30500),
                                (TYPE_SENSOR, 38500), (TYPE_SENSOR, 34500),
                                (TYPE_SENSOR, 37000), (TYPE_SENSOR, 40000)],
            "max_to_red_alarm": [(TYPE_SENSOR, 55500), (TYPE_SENSOR, 48500),
                                 (TYPE_SENSOR, 47500), (TYPE_SENSOR, 40000),
                                 (TYPE_SENSOR, 47500), (TYPE_SENSOR, 47000),
                                 (TYPE_SENSOR, 49500), (TYPE_SENSOR, 50000)],
        }
        afi_thermal_spec["mid_to_max_temp"] += [(TYPE_TRANSCEIVER, 65000)] * TRANSCEIVER_NUM_MAX
        afi_thermal_spec["max_to_mid_temp"] += [(TYPE_TRANSCEIVER, 62000)] * TRANSCEIVER_NUM_MAX
        afi_thermal_spec["max_to_red_alarm"] += [(TYPE_TRANSCEIVER, 75000)] * TRANSCEIVER_NUM_MAX

        afo_thermal_spec = {
            "min_to_mid_temp": [(TYPE_SENSOR, 63000), (TYPE_SENSOR, 63000),
                                (TYPE_SENSOR, 63000), (TYPE_SENSOR, 50000),
                                (TYPE_SENSOR, 63000), (TYPE_SENSOR, 63000),
                                (TYPE_SENSOR, 63000), (TYPE_SENSOR, 73000)],
            "mid_to_max_temp": [(TYPE_SENSOR, 68000), (TYPE_SENSOR, 68000),
                                (TYPE_SENSOR, 68000), (TYPE_SENSOR, 70000),
                                (TYPE_SENSOR, 68000), (TYPE_SENSOR, 68000),
                                (TYPE_SENSOR, 68000), (TYPE_SENSOR, 77000)],
            "max_to_mid_temp": [(TYPE_SENSOR, 47000), (TYPE_SENSOR, 47000),
                                (TYPE_SENSOR, 47000), (TYPE_SENSOR, 40000),
                                (TYPE_SENSOR, 47000), (TYPE_SENSOR, 47000),
                                (TYPE_SENSOR, 47000), (TYPE_SENSOR, 55000)],
            "mid_to_min_temp": [(TYPE_SENSOR, 40000), (TYPE_SENSOR, 40000),
                                (TYPE_SENSOR, 40000), (TYPE_SENSOR, 33000),
                                (TYPE_SENSOR, 40000), (TYPE_SENSOR, 40000),
                                (TYPE_SENSOR, 40000), (TYPE_SENSOR, 50000)],
            "max_to_red_alarm": [(TYPE_SENSOR, 72000), (TYPE_SENSOR, 72000),
                                 (TYPE_SENSOR, 72000), (TYPE_SENSOR, 60000),
                                 (TYPE_SENSOR, 72000), (TYPE_SENSOR, 72000),
                                 (TYPE_SENSOR, 72000), (TYPE_SENSOR, 81000)],
        }
        afo_thermal_spec["min_to_mid_temp"] += [(TYPE_TRANSCEIVER, 65000)] * TRANSCEIVER_NUM_MAX
        afo_thermal_spec["mid_to_max_temp"] += [(TYPE_TRANSCEIVER, 70000)] * TRANSCEIVER_NUM_MAX
        afo_thermal_spec["max_to_mid_temp"] += [(TYPE_TRANSCEIVER, 60000)] * TRANSCEIVER_NUM_MAX
        afo_thermal_spec["mid_to_min_temp"] += [(TYPE_TRANSCEIVER, 55000)] * TRANSCEIVER_NUM_MAX
        afo_thermal_spec["max_to_red_alarm"] += [(TYPE_TRANSCEIVER, 75000)] * TRANSCEIVER_NUM_MAX

        thermal_val = []
        max_to_mid = 0
        mid_to_min = 0

        # After booting, the database might not be ready for
        # connection. So, it should try to connect to the database
        # if self.transceiver_dom_sensor_table is None.
        if self.transceiver_dom_sensor_table is None:
            try:
                state_db = swsscommon.DBConnector(STATE_DB, 0, False)
                self.transceiver_dom_sensor_table = swsscommon.Table(state_db, TRANSCEIVER_DOM_SENSOR_TABLE)
            except Exception as e:
                logging.debug("{}".format(e))

        if fan_policy_state == LEVEL_FAN_INIT:
            fan_policy_state = LEVEL_FAN_MAX  # This is default state
            logging.debug("fan_policy_state=LEVEL_FAN_MAX")
            return

        count_check = count_check+1
        if count_check < CHECK_TIMES:
            return
        else:
            count_check = 0

        f2b_dir = 0
        b2f_dir = 0
        for i in range(FAN_TRAY_NUM * FAN_NUM):
            fan = platform_chassis.get_fan(i)
            if fan.get_presence() == False:
                continue
            res = fan.get_direction()
            if isinstance(res, str):
                b2f_dir += res.lower() == "intake"
                f2b_dir += res.lower() == "exhaust"
        logging.debug("b2f_dir={} f2b_dir={}".format(b2f_dir, f2b_dir))
        fan_dir = "intake" if b2f_dir >= f2b_dir else "exhaust"

        logging.debug("fan_dir : %s", fan_dir)

        if fan_dir == "intake":  # AFI
            fan_thermal_spec = afi_thermal_spec
            fan_policy = fan_policy_b2f
            logging.debug("fan_policy = fan_policy_b2f")
        elif fan_dir == "exhaust":          # AFO
            fan_thermal_spec = afo_thermal_spec
            fan_policy = fan_policy_f2b
            logging.debug("fan_policy = fan_policy_f2b")
        else:
            logging.debug("NULL case, not meet with fan_dir=0 or 1")

        ori_duty_cycle = platform_chassis.get_fan(0).get_speed()
        new_duty_cycle = 0

        if test_temp_revert == 0:
            temp_test_data = temp_test_data+2000
        else:
            temp_test_data = temp_test_data-2000

        if test_temp == 0:
            for i in range(THERMAL_NUM_MAX):
                thermal_val.append((TYPE_SENSOR, None,
                                    platform_chassis.get_thermal(i).get_temperature()*1000))

            for port_num in monitor_port:
                sfp = platform_chassis.get_sfp(port_num)
                thermal_val.append((TYPE_TRANSCEIVER, sfp,
                                    self.__get_transceiver_temperature(sfp.get_name()) * 1000))
        else:
            for i in range(THERMAL_NUM_MAX):
                thermal_val.append((TYPE_SENSOR, None, test_temp_list[i] + temp_test_data))
            for port_num in monitor_port:
                sfp = platform_chassis.get_sfp(port_num)
                thermal_val.append((TYPE_TRANSCEIVER, sfp, test_temp_list[i + 1] + temp_test_data))
                i = i + 1
            fan_fail = 0

        logging.debug("Maximum avaliable port : %d", TRANSCEIVER_NUM_MAX)
        logging.debug("thermal_val : %s", thermal_val)

        ori_state = fan_policy_state
        current_state = fan_policy_state

        if fan_dir == "intake":  # AFI
            sfp_presence_num = 0
            for i, (temp_type, obj, current_temp) in enumerate(thermal_val):
                sfp = None
                if temp_type == TYPE_TRANSCEIVER:
                    sfp = obj
                    if sfp.get_presence():
                        sfp_presence_num += 1
                    else:
                        continue

                if ori_state == LEVEL_FAN_MID:
                    if current_temp >= fan_thermal_spec["mid_to_max_temp"][i][1]:
                        current_state = LEVEL_FAN_MAX
                        logging.debug("current_state=LEVEL_FAN_MAX")
                        break
                else:
                    if current_temp <= fan_thermal_spec["max_to_mid_temp"][i][1]:
                        max_to_mid = max_to_mid + 1
                    if fan_policy_alarm == 0:
                        if current_temp >= fan_thermal_spec["max_to_red_alarm"][i][1]:
                            if send_red_alarm == 0:
                                fan_policy_alarm = LEVEL_FAN_RED_ALARM
                                send_red_alarm = 1
                                if fan_thermal_spec["max_to_red_alarm"][i][0] == TYPE_SENSOR:
                                    logging.warning('Alarm-Red for temperature high is detected, shutdown DUT.')
                                    time.sleep(2)
                                    power_off_dut()
                                elif fan_thermal_spec["max_to_red_alarm"][i][0] == TYPE_TRANSCEIVER:
                                    if shutdown_transceiver(sfp.get_name()):
                                        logging.warning("Alarm-Red for temperature high is detected, shutdown %s.", sfp.get_name())

            if max_to_mid == (THERMAL_NUM_MAX + sfp_presence_num) and fan_policy_state == LEVEL_FAN_MAX:
                if fan_fail == 0:
                    current_state = LEVEL_FAN_MID
                    logging.debug("current_state=LEVEL_FAN_MID")
                if fan_policy_alarm != 0:
                    logging.warning('Alarm for temperature high is cleared')
                    fan_policy_alarm = 0
                    send_red_alarm = 0
                    test_temp_revert = 0

        else:  # AFO
            sfp_presence_num = 0
            psu_full_load = check_psu_loading()
            for i, (temp_type, obj, current_temp) in enumerate(thermal_val):
                sfp = None
                if temp_type == TYPE_TRANSCEIVER:
                    sfp = obj
                    if sfp.get_presence():
                        sfp_presence_num += 1
                    else:
                        continue

                if ori_state == LEVEL_FAN_MID:
                    if current_temp >= fan_thermal_spec["mid_to_max_temp"][i][1]:
                        current_state = LEVEL_FAN_MAX
                        break
                    else:
                        if psu_full_load != True and current_temp <= fan_thermal_spec["mid_to_min_temp"][i][1]:
                            mid_to_min = mid_to_min+1

                elif ori_state == LEVEL_FAN_MIN:
                    if psu_full_load == True and fan_fail == 0:
                        current_state = LEVEL_FAN_MID
                        logging.warning("psu_full_load, set current_state=LEVEL_FAN_MID")
                        logging.debug("current_state=LEVEL_FAN_MID")
                    if current_temp >= fan_thermal_spec["min_to_mid_temp"][i][1] and fan_fail == 0:
                        current_state = LEVEL_FAN_MID
                        logging.debug("current_state=LEVEL_FAN_MID")

                else:
                    if current_temp <= fan_thermal_spec["max_to_mid_temp"][i][1]:
                        max_to_mid = max_to_mid+1
                    if fan_policy_alarm == 0:
                        if current_temp >= fan_thermal_spec["max_to_red_alarm"][i][1]:
                            if send_red_alarm == 0:
                                fan_policy_alarm = LEVEL_FAN_RED_ALARM
                                send_red_alarm = 1
                                if fan_thermal_spec["max_to_red_alarm"][i][0] == TYPE_SENSOR:
                                    logging.warning('Alarm-Red for temperature high is detected, shutdown DUT.')
                                    time.sleep(2)
                                    power_off_dut()
                                elif fan_thermal_spec["max_to_red_alarm"][i][0] == TYPE_TRANSCEIVER:
                                    if shutdown_transceiver(sfp.get_name()):
                                        logging.warning("Alarm-Red for temperature high is detected, shutdown %s.", sfp.get_name())

            if max_to_mid == (THERMAL_NUM_MAX + sfp_presence_num) and ori_state == LEVEL_FAN_MAX:
                if fan_fail == 0:
                    current_state = LEVEL_FAN_MID
                    logging.debug("current_state=LEVEL_FAN_MID")
                if fan_policy_alarm != 0:
                    logging.warning('Alarm for temperature high is cleared')
                    fan_policy_alarm = 0
                    send_red_alarm = 0
                    test_temp_revert = 0

            if mid_to_min == (THERMAL_NUM_MAX + sfp_presence_num) and ori_state == LEVEL_FAN_MID:
                if psu_full_load == 0:
                    current_state = LEVEL_FAN_MIN
                    logging.debug("current_state=LEVEL_FAN_MIN")

        # Check Fan status
        for i in range(FAN_TRAY_NUM * FAN_NUM):
            if not platform_chassis.get_fan(i).get_status() or not platform_chassis.get_fan(i).get_speed_rpm():
                new_duty_cycle = FAN_DUTY_CYCLE_MAX
                logging.warning('fan_%d fail, set duty_cycle to 100', i+1)
                if test_temp == 0:
                    fan_fail = 1
                    # 1.When insert/remove fan, fan speed/log still according to thermal policy.
                    # 2.If thermal policy state is bigger than LEVEL_FAN_MAX:
                    #  Do not change back to LEVEL_FAN_MAX, beacuse still need to deal with LOG or shutdown case.
                    # 3.If thermal policy state is smaller than LEVEL_FAN_MAX, set state=MAX.
                    # When remove and insert back fan test, policy check temp and set to correct fan_speed.
                    #
                    if current_state < LEVEL_FAN_MAX:
                        current_state = LEVEL_FAN_MAX
                        logging.debug('fan_%d fail, current_state=LEVEL_FAN_MAX', i+1)

                    as9716_32d_set_fan_speed(new_duty_cycle)

                    break
            else:
                fan_fail = 0

        if current_state != ori_state:
            fan_policy_state = current_state
            new_duty_cycle = fan_policy[current_state][0]
            logging.debug("fan_policy_state=%d, new_duty_cycle=%d", fan_policy_state, new_duty_cycle)
            if new_duty_cycle != ori_duty_cycle and fan_fail == 0:
                as9716_32d_set_fan_speed(new_duty_cycle)
                return True
            if new_duty_cycle == 0 and fan_fail == 0:
                as9716_32d_set_fan_speed(FAN_DUTY_CYCLE_MAX)

        return True


def main(argv):
    log_file = '%s.log' % FUNCTION_NAME
    log_level = logging.INFO
    global test_temp
    if len(sys.argv) != 1:
        try:
            opts, args = getopt.getopt(argv, 'hdlt:', ['lfile='])
        except getopt.GetoptError:
            print(("Usage: %s [-d] [-l <log_file>]" % sys.argv[0]))
            return 0
        for opt, arg in opts:
            if opt == '-h':
                print(("Usage: %s [-d] [-l <log_file>]" % sys.argv[0]))
                return 0
            elif opt in ('-d', '--debug'):
                log_level = logging.DEBUG
            elif opt in ('-l', '--lfile'):
                log_file = arg

        if sys.argv[1] == '-t':
            if len(sys.argv) != (2 + 8 + 8): # 8 Thermal, 8 ZR/ZR+ Thermal
                print("temp test, need input 8 temp")
                return 0
            i = 0
            for x in range(2, (2 + 8 + 8)): # 8 Thermal, 8 ZR/ZR+ Thermal
                test_temp_list[i] = int(sys.argv[x])*1000
                i = i+1
            test_temp = 1
            log_level = logging.DEBUG
            print(test_temp_list)

    global platform_chassis
    platform_chassis = platform.Platform().get_chassis()
    #status, output = subprocess.getstatusoutput('i2cset -f -y 17 0x66 0x33 0x0')
    #if status:
    #    print("Warning: Fan speed watchdog could not be disabled")
    cmd_str = ["i2cset", "-y", "-f", "17", "0x66", "0x33", "0x1"]
    status, output = getstatusoutput_noshell(cmd_str)
    if status:
        print("Warning: Fan speed watchdog could not be enabled")

    cmd_str = ["i2cset", "-y", "-f", "17", "0x66", "0x31", "0xF0"]
    status, output = getstatusoutput_noshell(cmd_str)
    if status:
        print("Warning: Fan speed watchdog timer could not be disabled")


    as9716_32d_set_fan_speed(FAN_DUTY_CYCLE_MAX)
    monitor = device_monitor(log_file, log_level)
    cmd_kick = ["i2cset", "-y", "-f", "17", "0x66", "0x31", "0xF0"] #kick WDT
    cmd_check_wdt = ["i2cget",  "-y", "-f", "17", "0x66", "0x33"]
    while True:
        monitor.manage_fans()
        getstatusoutput_noshell(cmd_kick)
        time.sleep(10)

        status, output = getstatusoutput_noshell(cmd_check_wdt)
        if status is not None:
            val= int(output,16)
            if (val & 0x1) == 0:
                logging.warning('Detect Fan-WDT disable')
                logging.warning('Try to enable Fan-WDT')
                cmd_str = ["i2cset", "-y", "-f", "17", "0x66", "0x33", "0x1"]
                getstatusoutput_noshell(cmd_str)
                cmd_str = ["i2cset", "-y", "-f", "17", "0x66", "0x31", "0xF0"]
                getstatusoutput_noshell(cmd_str)


if __name__ == '__main__':
    main(sys.argv[1:])
