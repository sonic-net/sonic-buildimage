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
#    07/12/2022: Michael_Shih create for as9736_64d thermal plan
#    12/12/2023: Add detect temp of xcvr, and implement shutdown function.
#    23/01/2024: Sync the log buffer to the disk before powering off the DUT.
#    11/06/2024: Enhance thermal-plan warning message.
#    01/07/2025: Roger_Ho updated the threshold value based on spec. v15.
# ------------------------------------------------------------------

try:
    import getopt
    import sys
    import logging
    import logging.config
    import logging.handlers
    import signal
    import time  # this is only being used as part of the example
    from as9736_64d.fanutil import FanUtil
    from as9736_64d.thermalutil import ThermalUtil
    from swsscommon import swsscommon
    from sonic_platform import platform
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

VERSION = '1.0'
FUNCTION_NAME = '/usr/local/bin/accton_as9736_64d_monitor'

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


# Thermal policy(AFO-Front to back):
#
# Fan Speed Level:
#     Fan Level 1 (Fan speed:  50%, RPM +/- 10%:  6800)
#     Fan Level 2 (Fan speed:  70%, RPM +/- 10%:  9520)
#     Fan Level 3 (Fan speed: 100%, RPM +/- 10%: 13600)
#
# Using Thermal sensors as below:
#    - SMB TMP75 (0x48)
#    - SMB TMP75 (0x49)
#    - SMB TMP422(0x4c)
#    - FCM TMP75 (0x48)
#    - FCM TMP75 (0x49)
#    - PDB_L TMP75 (0x48)
#    - PDB_R TMP75 (0x49)
#    - UDB TMP75 (0x48)
#    - UDB TMP422(0x4c)
#    - LDB TMP75(0x4c)
#    - LDB TMP422(0x4d)
#    - CPU core_1~8
#    - MAC Use SMB TMP422(0x4c)
#
# Raise to Fan Level 2 from Level 1 condition:
#    - SMB TMP75 (0x48) >= 59 --> Please refer to "Condition 2" in the spec. for this value.
#          or
#    - SMB TMP75 (0x49) >= 50
#          or
#    - SMB TMP422(0x4c) >= 92
#          or
#    - FCM TMP75 (0x48) >= 50
#          or
#    - FCM TMP75 (0x49) >= 50
#          or
#    - PDB_L TMP75 (0x48) >= 45
#          or
#    - PDB_R TMP75 (0x49) >= 45
#          or
#    - UDB TMP75 (0x48) >= 58
#          or
#    - UDB TMP422(0x4c) >= 51
#          or
#    - LDB TMP75 (0x4c) >= 54
#          or
#    - LDB TMP422(0x4d) >= 54
#
# Slow down to Fan Level 1 from Level 2 condition:
#    - SMB TMP75 (0x48) <= 54 --> Please refer to "Condition 1" in the spec. for this value.
#          and
#    - SMB TMP75 (0x49) <= 43
#          and
#    - SMB TMP422(0x4c) <= 83
#          and
#    - FCM TMP75 (0x48) <= 45
#          and
#    - FCM TMP75 (0x49) <= 45
#          and
#    - PDB_L TMP75 (0x48) <= 40
#          and
#    - PDB_R TMP75 (0x49) <= 40
#          and
#    - UDB TMP75 (0x48) <= 53
#          and
#    - UDB TMP422(0x4c) <= 44
#          and
#    - LDB TMP75 (0x4c) <= 47
#          and
#    - LDB TMP422(0x4d) <= 47
#
# Raise to Fan Level 3 conditions:
#    - Fan failed
#    - Fan has removed
#
# Thermal Protect Function for Shutdown condition:
#    - CPU core temp >= 99     (System shutdown except to CPU)
#          or
#    - SMB TMP75 (0x48) >= 76  --> Please refer to "Shutdown T" in the spec. for this value.
#          or
#    - SMB TMP75 (0x49) >= 60
#          or
#    - SMB TMP422(0x4c) >= 103 (MAC shutdown)
#          or
#    - FCM TMP75 (0x48) >= 67
#          or
#    - FCM TMP75 (0x49) >= 67
#          or
#    - PDB_L TMP75 (0x48) >= 62
#          or
#    - PDB_R TMP75 (0x49) >= 62
#          or
#    - UDB TMP75 (0x48) >= 70
#          or
#    - UDB TMP422(0x4c) >= 61
#          or
#    - LDB TMP75 (0x4c) >= 67
#          or
#    - LDB TMP422(0x4d) >= 67

fan_policy_state = 0
fan_fail = 0
count_check = 0

board_thermal_min_to_mid = 0
board_thermal_mid_to_min = 0
cpu_fan_policy_state = 0

exit_by_sigterm=0

send_mac_shutdown_warning = 0
send_cpu_shutdown_warning = 0

thermal_min_to_mid_waring_flag = [0]
fan_failed_msg_flag = 0

platform_chassis= None

int_port_mapping = []

def stop_syncd_service():
    cmd_str = ["systemctl", "disable", "syncd"]
    (status, output) = getstatusoutput_noshell(cmd_str)
    if status:
        logging.warning("Disable syncd.service failed")
        return False

    cmd_str = ["systemctl", "stop", "syncd"]
    (status, output) = getstatusoutput_noshell(cmd_str)
    if status:
        logging.warning("Stop syncd.service failed")
        return False

    cmd_str = ["systemctl", "mask", "syncd"]
    (status, output) = getstatusoutput_noshell(cmd_str)
    if status:
        logging.warning("Mask syncd.service failed")
        return False

    return (status == 0)

def sync_log_buffer_to_disk():
    cmd_str=["sync"]
    (status, output) = getstatusoutput_noshell(cmd_str)
    cmd_str=["/sbin/fstrim", "-av"]
    (status, output) = getstatusoutput_noshell(cmd_str)
    time.sleep(3)

    return (status == 0)

def shutdown_mac():
    cmd_str = ["i2cset", "-f", "-y", "6", "0x60", "0x7", "0x1"]
    (status, output) = getstatusoutput_noshell(cmd_str)
    if status:
        logging.warning("Shutdown MAC failed.")

    return (status == 0)

def shutdown_except_cpu():
    cmd_str = ["i2cset", "-f", "-y", "6", "0x60", "0x7", "0x2"]
    (status, output) = getstatusoutput_noshell(cmd_str)
    if status:
        logging.warning("Shutdown DUT failed.")

    return (status == 0)

# Make a class we can use to capture stdout and sterr in the log
class device_monitor(object):
    # static temp var
    init_duty_cycle = 0
    new_duty_cycle = 0
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
            formatter = logging.Formatter('%(name)-12s: %(levelname)-8s %(message)s')
            console.setFormatter(formatter)
            logging.getLogger('').addHandler(console)

        sys_handler = logging.handlers.SysLogHandler(address = '/dev/log')
        sys_handler.setLevel(logging.INFO)
        formatter = logging.Formatter('#%(module)s: %(message)s')
        sys_handler.setFormatter(formatter)
        logging.getLogger('').addHandler(sys_handler)
        #logging.debug('SET. logfile:%s / loglevel:%d', log_file, log_level)

        self.transceiver_dom_sensor_table = None

    def set_fan_duty_cycle(self, duty_cycle_percentage, old_duty_cycle_percentage):

        if duty_cycle_percentage > old_duty_cycle_percentage:
            logging.warning('Increase fan duty_cycle from %d%% to %d%%.', old_duty_cycle_percentage, duty_cycle_percentage)
        else:
            logging.info('Decrease fan duty_cycle from %d%% to %d%%.', old_duty_cycle_percentage, duty_cycle_percentage)

        self.fan.set_fan_duty_cycle(duty_cycle_percentage)

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
        global fan_fail
        global count_check
        global board_thermal_min_to_mid
        global board_thermal_mid_to_min
        global thermal_fan_policy_state
        global cpu_fan_policy_state
        global mac_fan_policy_state
        global send_mac_shutdown_warning
        global send_cpu_shutdown_warning
        global thermal_min_to_mid_waring_flag
        global fan_failed_msg_flag
        global int_port_mapping

        LEVEL_FAN_INIT=0
        FAN_LEVEL_1 = 1
        FAN_LEVEL_2 = 2
        FAN_LEVEL_3 = 3
        POLICY_NEED_SHUTDOWN  = 4

        fan_speed_policy = {
            FAN_LEVEL_1: [50],
            FAN_LEVEL_2: [70],
            FAN_LEVEL_3: [100]
        }

        fan = self.fan
        thermal = self.thermal

        TYPE_SENSOR = 1
        TYPE_TRANSCEIVER = 2

        TRANSCEIVER_NUM_MAX = 64
        TOTAL_DETECT_SENSOR_NUM = thermal.THERMAL_NUM_BD_SENSOR + TRANSCEIVER_NUM_MAX

        # thermal_spec={
        #      "min_to_mid_temp": [SMB TMP75 (0x48), SMB TMP75 (0x49),
        #                          FCM TMP75 (0x48), FCM TMP75 (0x49),
        #                          PDB_L TMP75 (0x48), PDB_R TMP75 (0x49),
        #                          UDB TMP75 (0x48), UDB TMP422(0x4c),
        #                          LDB TMP75 (0x4c), LDB TMP422(0x4d), SMB TMP422(0x4c)],
        #      "mid_to_min_temp": [SMB TMP75 (0x48), SMB TMP75 (0x49),
        #                          FCM TMP75 (0x48), FCM TMP75 (0x49),
        #                          PDB_L TMP75 (0x48), PDB_R TMP75 (0x49),
        #                          UDB TMP75 (0x48), UDB TMP422(0x4c),
        #                          LDB TMP75 (0x4c), LDB TMP422(0x4d), SMB TMP422(0x4c)],
        #      "shutdown_temp"  : [SMB TMP75 (0x48), SMB TMP75 (0x49),
        #                          FCM TMP75 (0x48), FCM TMP75 (0x49),
        #                          PDB_L TMP75 (0x48), PDB_R TMP75 (0x49),
        #                          UDB TMP75 (0x48), UDB TMP422(0x4c),
        #                          LDB TMP75 (0x4c), LDB TMP422(0x4d), SMB TMP422(0x4c)],
        #      "cpu_temp"       : [(TYPE_SENSOR, 80000), CPU],
        #      "mac_temp"       : [(TYPE_SENSOR, 85000), SMB TMP422(0x4c)]
        # }

        thermal_spec={
            "min_to_mid_temp": [(TYPE_SENSOR, 59000), (TYPE_SENSOR, 50000),
                                (TYPE_SENSOR, 50000), (TYPE_SENSOR, 50000),
                                (TYPE_SENSOR, 45000), (TYPE_SENSOR, 45000),
                                (TYPE_SENSOR, 58000), (TYPE_SENSOR, 51000),
                                (TYPE_SENSOR, 54000), (TYPE_SENSOR, 54000), (TYPE_SENSOR, 92000)],
            "mid_to_min_temp": [(TYPE_SENSOR, 54000), (TYPE_SENSOR, 43000),
                                (TYPE_SENSOR, 45000), (TYPE_SENSOR, 45000),
                                (TYPE_SENSOR, 40000), (TYPE_SENSOR, 40000),
                                (TYPE_SENSOR, 53000), (TYPE_SENSOR, 44000),
                                (TYPE_SENSOR, 47000), (TYPE_SENSOR, 47000), (TYPE_SENSOR, 83000)],
            "shutdown_temp"  : [(TYPE_SENSOR, 76000), (TYPE_SENSOR, 60000),
                                (TYPE_SENSOR, 67000), (TYPE_SENSOR, 67000),
                                (TYPE_SENSOR, 62000), (TYPE_SENSOR, 62000),
                                (TYPE_SENSOR, 70000), (TYPE_SENSOR, 61000),
                                (TYPE_SENSOR, 67000), (TYPE_SENSOR, 67000), (TYPE_SENSOR, 103000)],
            "cpu_temp"       : [(TYPE_SENSOR, 80000), (TYPE_SENSOR, 99000)],
            "mac_temp"       : [(TYPE_SENSOR, 85000), (TYPE_SENSOR, 103000)]
        }

        thermal_spec["min_to_mid_temp"] += [(TYPE_TRANSCEIVER, 70000)]
        thermal_spec["mid_to_min_temp"] += [(TYPE_TRANSCEIVER, 60000)]

        board_thermal_val   = []
        board_thermal_or_chk_min_to_mid  = [0] * (TOTAL_DETECT_SENSOR_NUM)
        board_thermal_and_chk_mid_to_min = [0] * (TOTAL_DETECT_SENSOR_NUM)
        cpucore_thermal_val = [0, 0, 0, 0, 0, 0, 0, 0]
        mactemp_thermal_val = [0]

        # After booting, the database might not be ready for
        # connection. So, it should try to connect to the database
        # if self.transceiver_dom_sensor_table is None.
        if self.transceiver_dom_sensor_table is None:
            try:
                state_db = swsscommon.DBConnector(STATE_DB, 0, False)
                self.transceiver_dom_sensor_table = swsscommon.Table(state_db, TRANSCEIVER_DOM_SENSOR_TABLE)
            except Exception as e:
                logging.debug("{}".format(e))

        # Get duty_cycle at init:
        if fan_policy_state == LEVEL_FAN_INIT:
            # Init sensors record warning flag:
            thermal_min_to_mid_waring_flag = thermal_min_to_mid_waring_flag * TOTAL_DETECT_SENSOR_NUM

            # Record port mapping to interface
            for port_num in range(TRANSCEIVER_NUM_MAX):
                sfp = platform_chassis.get_sfp(port_num+1)
                int_port_mapping.append( (port_num+1, sfp, sfp.get_name()) )

            self.init_duty_cycle = fan.get_fan_duty_cycle()
            for i in range (FAN_LEVEL_1, FAN_LEVEL_3 + 1):
                if self.init_duty_cycle == fan_speed_policy[i][0]:
                    fan_policy_state = i
                else:
                    continue
            logging.debug("- [Init]: fan_policy_state = %d, get duty_cycle = %d", fan_policy_state, self.init_duty_cycle)
            # Fan duty_cycle is not in FAN_LEVEL_1~3 case
            if fan_policy_state == LEVEL_FAN_INIT:
                if (self.init_duty_cycle > fan_speed_policy[FAN_LEVEL_2][0] and
                    self.init_duty_cycle < fan_speed_policy[FAN_LEVEL_3][0]):
                    fan_policy_state = FAN_LEVEL_2
                else:
                    fan_policy_state = FAN_LEVEL_1

                self.set_fan_duty_cycle(fan_speed_policy[fan_policy_state][0], self.init_duty_cycle)

            return True

        self.ori_duty_cycle = fan.get_fan_duty_cycle()
        self.new_duty_cycle = 0

        board_thermal_min_to_mid = 0 #use for | operation
        board_thermal_mid_to_min = 1 #use for & operation
        broad_thermal_need_shutdown = 0
        thermal_fan_policy_state = LEVEL_FAN_INIT
        cpu_fan_policy_state     = LEVEL_FAN_INIT
        mac_fan_policy_state     = LEVEL_FAN_INIT

        #1 Check fan: Unpresent or fan_fault status
        fan_fail = 0
        for i in range (fan.FAN_NUM_1_IDX, fan.FAN_NUM_ON_MAIN_BROAD+1):
            if fan.get_fan_present(i) == 0: #unpresent
                fan_fail = 1
            elif fan.get_fan_fault(i) == 1: #fan_speed is 0
                fan_fail = 1
            else:
                if fan_fail == 1:
                    continue

        ori_state     = fan_policy_state
        current_state = fan_policy_state

        #Fan protect policy: increase fan_speed need 50% --> 75% --> 100%
        if fan_fail == 1:
            if ori_state == FAN_LEVEL_2 or ori_state == FAN_LEVEL_3:
                current_state = FAN_LEVEL_3
            elif ori_state == FAN_LEVEL_1:
                current_state = FAN_LEVEL_2

            fan_policy_state = current_state
            self.new_duty_cycle = fan_speed_policy[fan_policy_state][0]

            if fan_failed_msg_flag == 0 :
                logging.warning('Monitor FAN absent/failed.')
                fan_failed_msg_flag = 1

            if self.new_duty_cycle != self.ori_duty_cycle:
                self.set_fan_duty_cycle(self.new_duty_cycle, self.ori_duty_cycle)

                return True

        # All fans are normal
        else:
            if fan_failed_msg_flag == 1:
                logging.info('All fans are normal.')
                fan_failed_msg_flag = 0

        #2-1 Board Sensors get value:
        for i in range (thermal.THERMAL_NUM_1_IDX, thermal.THERMAL_NUM_11_IDX+1):
            board_thermal_val.append((TYPE_SENSOR, None, thermal._get_thermal_val(i)))
            #index: 0~10(11 thermal sensor)
            if board_thermal_val[i-1][2] >= thermal_spec["min_to_mid_temp"][i-1][1]:
                board_thermal_or_chk_min_to_mid[i-1] = 1
                # During this fan-speed rise, each sensors can only send warning log on syslog once
                if thermal_min_to_mid_waring_flag[i-1] == 0:
                    logging.warning('Monitor %s, temperature is %d. Temperature is over the warning threshold(%d) of thermal policy.',
                                    thermal.get_thermal_name(i),
                                    board_thermal_val[i-1][2]/1000,
                                    thermal_spec["min_to_mid_temp"][i-1][1]/1000)
                    thermal_min_to_mid_waring_flag[i-1] = 1
            else:
                board_thermal_or_chk_min_to_mid[i-1] = 0

            if board_thermal_val[i-1][2] <= thermal_spec["mid_to_min_temp"][i-1][1]:
                board_thermal_and_chk_mid_to_min[i-1] = 1
            else:
                board_thermal_and_chk_mid_to_min[i-1] = 0

        for port_num in range(TRANSCEIVER_NUM_MAX):
            board_thermal_val.append((TYPE_TRANSCEIVER, int_port_mapping[port_num][1],
                                      self.get_transceiver_temperature(int_port_mapping[port_num][2]) * 1000))
            #index: 11~74(64 port)
            if board_thermal_val[thermal.THERMAL_NUM_11_IDX + port_num][2] >= thermal_spec["min_to_mid_temp"][thermal.THERMAL_NUM_BD_SENSOR][1]:
                board_thermal_or_chk_min_to_mid[thermal.THERMAL_NUM_11_IDX + port_num] = 1
                # During this fan-speed rise, each xcvr can only send warning log on syslog once
                if thermal_min_to_mid_waring_flag[thermal.THERMAL_NUM_11_IDX + port_num] == 0:
                    logging.warning('Monitor port %d, temperature is %d. Temperature is over the warning threshold(%d) of thermal policy.',
                                    port_num+1,
                                    board_thermal_val[thermal.THERMAL_NUM_11_IDX + port_num][2]/1000,
                                    thermal_spec["min_to_mid_temp"][thermal.THERMAL_NUM_BD_SENSOR][1]/1000)
                    thermal_min_to_mid_waring_flag[thermal.THERMAL_NUM_11_IDX + port_num] = 1
            else:
                board_thermal_or_chk_min_to_mid[thermal.THERMAL_NUM_11_IDX + port_num] = 0

            if board_thermal_val[thermal.THERMAL_NUM_11_IDX + port_num][2] <= thermal_spec["mid_to_min_temp"][thermal.THERMAL_NUM_BD_SENSOR][1]:
                board_thermal_and_chk_mid_to_min[thermal.THERMAL_NUM_11_IDX + port_num] = 1
            else:
                board_thermal_and_chk_mid_to_min[thermal.THERMAL_NUM_11_IDX + port_num] = 0

        for i in range (thermal.THERMAL_NUM_1_IDX, thermal.THERMAL_NUM_10_IDX+1): #Not include TH4-TMP422(0x4c)
            if board_thermal_val[i-1][2] >= thermal_spec["shutdown_temp"][i-1][1]:
                broad_thermal_need_shutdown = 1
                logging.warning('Monitor %s, temperature is %d. Temperature is over the shutdown threshold(%d) of thermal policy. Need shutdown DUT.',
                                thermal.get_thermal_name(i),
                                board_thermal_val[i-1][2]/1000,
                                thermal_spec["shutdown_temp"][i-1][1]/1000)
                break

        #2-2 CPU Sensors get value:
        for i in range (thermal.THERMAL_NUM_1_IDX, thermal.THERMAL_NUM_CPU_CORE+1):
            cpucore_thermal_val[i-1] = thermal._get_thermal_val(i + thermal.THERMAL_NUM_BD_SENSOR)

        #2-3 MAC Sensors get value:
        mactemp_thermal_val[0] = board_thermal_val[thermal.THERMAL_NUM_11_IDX-1][2]

        #3-1 Decide the board thermal policy:
        if broad_thermal_need_shutdown == 1:
            thermal_fan_policy_state = POLICY_NEED_SHUTDOWN
        else:
            for i in range (TOTAL_DETECT_SENSOR_NUM):
                board_thermal_min_to_mid |= board_thermal_or_chk_min_to_mid[i]
                board_thermal_mid_to_min &= board_thermal_and_chk_mid_to_min[i]

            if board_thermal_min_to_mid == 0 and board_thermal_mid_to_min == 1:
                thermal_fan_policy_state = FAN_LEVEL_1
            elif board_thermal_min_to_mid == 1 and board_thermal_mid_to_min == 0:
                thermal_fan_policy_state = FAN_LEVEL_2
            else:
                if ori_state == FAN_LEVEL_1:
                    thermal_fan_policy_state = FAN_LEVEL_1
                else:
                    thermal_fan_policy_state = FAN_LEVEL_2

        #3-2 Decide the CPU thermal policy:
        for i in range (thermal.THERMAL_NUM_1_IDX, thermal.THERMAL_NUM_CPU_CORE+1):

            if cpucore_thermal_val[i-1] >= thermal_spec["cpu_temp"][1][1] :  #Case of shutdown
                if send_cpu_shutdown_warning == 0:
                    logging.warning('Monitor %s, temperature is %d. Temperature is over the shutdown threshold(%d) of thermal policy.',
                                     thermal.get_thermal_name(thermal.THERMAL_NUM_BD_SENSOR+i),
                                     cpucore_thermal_val[i-1]/1000,
                                     thermal_spec["cpu_temp"][1][1]/1000)
                cpu_fan_policy_state = POLICY_NEED_SHUTDOWN

        #3-3 Decide the MAC thermal policy:
        if mactemp_thermal_val[0] >= thermal_spec["mac_temp"][1][1] :  #Case of shutdown
            if send_mac_shutdown_warning == 0:
                logging.warning('Monitor MAC, temperature is %d. Temperature is over the shutdown threshold(%d) of thermal policy.', mactemp_thermal_val[0]/1000, thermal_spec["mac_temp"][1][1]/1000)
            mac_fan_policy_state = POLICY_NEED_SHUTDOWN

        #4 Condition of change fan speed by sensors policy:
        if ori_state == FAN_LEVEL_3:
            if thermal_fan_policy_state == POLICY_NEED_SHUTDOWN or cpu_fan_policy_state == POLICY_NEED_SHUTDOWN:
                if send_cpu_shutdown_warning == 0:
                    send_cpu_shutdown_warning = 1
                    stop_syncd_service()
                    logging.critical("CPU/TMP75/TMP422 sensor for temperature high(over shutdown threshold) is detected, shutdown DUT.")
                    sync_log_buffer_to_disk()
                    shutdown_except_cpu()
                    return True

            elif mac_fan_policy_state == POLICY_NEED_SHUTDOWN:
                if send_mac_shutdown_warning == 0:
                    send_mac_shutdown_warning =1
                    stop_syncd_service()
                    logging.critical("MAC sensor for temperature high is detected(over shutdown threshold), shutdown MAC chip.")
                    shutdown_mac()  # No return, keep monitoring.

            elif fan_fail == 1:
                current_state = FAN_LEVEL_3

            else:
                current_state = FAN_LEVEL_2

        elif ori_state == FAN_LEVEL_2:
            if thermal_fan_policy_state == POLICY_NEED_SHUTDOWN or cpu_fan_policy_state == POLICY_NEED_SHUTDOWN or mac_fan_policy_state == POLICY_NEED_SHUTDOWN:
                current_state = FAN_LEVEL_3

            elif thermal_fan_policy_state == FAN_LEVEL_1:
                current_state = FAN_LEVEL_1
                logging.info('- Monitor all sensors, temperature is less than the warning threshold of thermal policy.')
                # Clear sensors send-syslog-warning record
                thermal_min_to_mid_waring_flag = [0] * TOTAL_DETECT_SENSOR_NUM
            else:
                current_state = FAN_LEVEL_2

        elif ori_state == FAN_LEVEL_1:
            if thermal_fan_policy_state == POLICY_NEED_SHUTDOWN or cpu_fan_policy_state == POLICY_NEED_SHUTDOWN or mac_fan_policy_state == POLICY_NEED_SHUTDOWN:
                current_state = FAN_LEVEL_2

            elif thermal_fan_policy_state == FAN_LEVEL_2:
                current_state = FAN_LEVEL_2
            else:
                current_state = FAN_LEVEL_1

        #4 Setting new duty-cyle:
        if current_state != ori_state:
            fan_policy_state = current_state

            self.new_duty_cycle = fan_speed_policy[fan_policy_state][0]

            if self.new_duty_cycle != self.ori_duty_cycle:
                self.set_fan_duty_cycle(fan_speed_policy[fan_policy_state][0], fan_speed_policy[ori_state][0])
                return True

            if self.new_duty_cycle == 0 :
                self.set_fan_duty_cycle(fan_speed_policy[FAN_LEVEL_3][0], fan_speed_policy[ori_state][0])

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
            opts, args = getopt.getopt(argv,'hdl:',['lfile='])
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

    monitor = device_monitor(log_file, log_level)

    global platform_chassis
    platform_chassis = platform.Platform().get_chassis()

    # Loop forever, doing something useful hopefully:
    while True:
        monitor.manage_fans()
        # HW recommends checking the temperature every 10 seconds
        time.sleep(10)
        if exit_by_sigterm == 1:
            break

if __name__ == '__main__':
    main(sys.argv[1:])

