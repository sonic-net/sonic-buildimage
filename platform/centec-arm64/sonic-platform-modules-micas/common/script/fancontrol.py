#!/usr/bin/env python3.9
# -*- coding: UTF-8 -*-
import click
import os
import time
import json
import traceback
from interface import Interface
import logging.handlers
from platform_util import CompressedRotatingFileHandler

CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

FILE_NAME = "/var/log/fancontrol.log"
MAX_LOG_BYTES = 20*1024*1024
BACKUP_COUNT = 9

logger = logging.getLogger("fancontrol")
logger.setLevel(logging.DEBUG)
fanctrl_log = CompressedRotatingFileHandler(FILE_NAME, mode='a', maxBytes=MAX_LOG_BYTES, backupCount=BACKUP_COUNT, encoding=None, delay=0)
formatter = logging.Formatter("%(asctime)s - %(levelname)s - %(message)s")
fanctrl_log.setFormatter(formatter)
logger.addHandler(fanctrl_log)

DEBUG_FILE = "/etc/fancontrol_debug"
FAN_CTRL_CFG_FILE = "/usr/local/bin/fan_ctrl_cfg.json"
KEY_THERMAL = "Thermal"
KEY_FAN = "Fans"
KEY_PID = "PID"
KEY_OPEN_LOOP = "OpenLoop"
KEY_DEVICE = "Device"
KEY_FAN_ERROR = "FanError"

KEY_FANERROR_PWM_MAX = "Fan_Pwmmax"

KEY_INLET_TEMP = "INLET_TEMP"
KEY_OUTLET_TEMP = "OUTLET_TEMP"
KEY_SWITCH_TEMP = "SWITCH_TEMP"
KEY_TEMP = [KEY_INLET_TEMP, KEY_OUTLET_TEMP, KEY_SWITCH_TEMP]

KEY_PID_PWM_MAX = "Pwm_Max"
KEY_PID_PWM_MIN = "Pwm_Min"
KEY_PID_SETPOINT = "SetPoint"
KEY_PID_P = "P"
KEY_PID_I = "I"
KEY_PID_D = "D"
KEY_PID_TEMP_MIN = "Temp_Min"
KEY_PID_TEMP_MAX = "Temp_Max"

KEY_OPENLOOP_A = "a"
KEY_OPENLOOP_B = "b"
KEY_OPENLOOP_C = "c"
KEY_OPENLOOP_FIXUP = "fix_up"
KEY_OPENLOOP_PWM_MAX = "pwmMax"
KEY_OPENLOOP_PWM_MIN = "pwmMin"
KEY_OPENLOOP_TEMP_MIN = "tempMin"

STATUS_HIGH_CRIT = 1
STATUS_MISS_CRIT = 2
STATUS_BAD_FAN   = 4
STATUS_LOW_FAN   = 8
STATUS_MISS_ERR  = 16

class AliasedGroup(click.Group):
    def get_command(self, ctx, cmd_name):
        rv = click.Group.get_command(self, ctx, cmd_name)
        if rv is not None:
            return rv
        matches = [x for x in self.list_commands(ctx)
                   if x.startswith(cmd_name)]
        if not matches:
            return None
        elif len(matches) == 1:
            return click.Group.get_command(self, ctx, matches[0])
        ctx.fail('Too many matches: %s' % ', '.join(sorted(matches)))

def fanctrl_debug_log(s):
    # s = s.decode('utf-8').encode('gb2312')
    if os.path.isfile(DEBUG_FILE):
        logger.debug(s)

class OpenLoop():
    def __init__(self):
        self.a = 0
        self.b = 0
        self.c = 0
        self.fix_up = 0
        self.pwmMax = 0
        self.pwmMin = 0
        self.temp = 0
        self.tempMin = 0

    def calcPwm(self):
        if self.temp < self.tempMin:
            return self.pwmMin
        pwm = self.a * self.temp * self.temp + self.b * self.temp + self.c
        pwm = (pwm / 2.56)
        pwm = min(pwm, self.pwmMax)
        pwm = max(pwm, self.pwmMin)
        return pwm


class FanPid():
    def __init__(self):
        self.pwmMin = 30
        self.pwmMax = 100
        self.SetPoint = 89
        self.D = 0.196
        self.I = 0.196
        self.P = 1.176
        self.tempMin = 28.0
        self.tempMax = 45.0
        self.pwms = [30, 30]
        self.temps = [-1, -1, -1]
        self.last_temp = -1
        self.sensor = KEY_SWITCH_TEMP

    def calcPwm(self):
        temp_delta = self.temps[2] - self.last_temp
        temp_sp_delta = self.temps[2] - self.SetPoint
        temp_last_delta = ((self.temps[2] - self.temps[1]) - (self.temps[1] - self.temps[0]))

        delta_pwm = self.P * temp_delta + self.I * temp_sp_delta + self.D * temp_last_delta

        self.temps[0] = self.temps[1]
        self.temps[1] = self.temps[2]
        self.last_temp = self.temps[2]

        self.pwms[0] = self.pwms[1]
        self.pwms[1] = self.pwms[0] + delta_pwm
        self.pwms[1] = max(self.pwms[1], self.pwmMin)
        self.pwms[1] = min(self.pwms[1], self.pwmMax)

        return self.pwms[1]

class FanControl():

    def __init__(self):
        self.status = 0
        self.fan_status = 0
        self.error_time = 0
        self.low_time = 0
        self.fan_pwm = 40
        self.fanerr_pwmmax = 0
        self.interface = Interface()
        self.temps = {}
        self.tempsMax = {}
        self.tempsMin = {}
        self.tempStatus = {}
        self.tempCritTime = {}
        self.tempMissTime = {}

        self.fans = {}
        self.fanStatus = {}
        self.fanErrTime = {}
        self.fanLowTime = {}

        self.fanPid = None
        self.openloop = None

        self.isBuildin = 0
        self.isLiquid = 0
        self.eeproms = {}
        self.airflow = ""
        self.pid_switch = 1
        self.openloop_switch = 1

    def doGetAirFlow(self):
        if self.isLiquid == 1:
            return "Liquid"
        # PSU and Fan is buildin using Tlve2
        if self.isBuildin == 1:
            productName = self.interface.get_productname()
            if productName != "":
                tmp = productName.split("-")
                fanairflow = tmp[-1]
                if fanairflow == "R":
                    return "exhaust"
                else:
                    return "intake"
        elif self.isBuildin == 0:
            return self.interface.get_airflow()

        return ""

    def doFanCtrlInit(self):
        if os.path.isfile(FAN_CTRL_CFG_FILE):
            fh = open(FAN_CTRL_CFG_FILE)
            if not fh:
                logger.error("Config file %s doesn't exist" % FAN_CTRL_CFG_FILE)
                return False
            cfg_json = json.load(fh)
            if not cfg_json:
                logger.error('Load config file %s failed' % FAN_CTRL_CFG_FILE)
                fh.close()
                return False

            cfg_keys = [KEY_THERMAL, KEY_FAN, KEY_PID, KEY_OPEN_LOOP, KEY_DEVICE, KEY_FAN_ERROR]
            for key in cfg_keys:
                if key not in cfg_json:
                    logger.error('Key %s not present in cfg file' % key)
                    return False
            thermal_json = cfg_json[KEY_THERMAL]
            fan_json = cfg_json[KEY_FAN]
            pid_json = cfg_json[KEY_PID]
            openloop_json = cfg_json[KEY_OPEN_LOOP]
            device_json = cfg_json[KEY_DEVICE]
            fan_error_json = cfg_json[KEY_FAN_ERROR]

            #init fanerror
            self.fanerr_pwmmax = fan_error_json["Fan_Pwmmax"]

            # Get Airflow
            self.isBuildin = device_json["Buildin"]
            self.isLiquid = device_json["Liquid"]
            self.airflow = self.doGetAirFlow()
            if self.airflow == "":
                logger.warning("Cannot get airflow from device!")
            self.pid_switch = device_json["PID"]
            self.openloop_switch = device_json["OpenLoop"]
            if self.pid_switch == 0 and self.openloop_switch == 0:
                logger.warning("No PID and OpenLoop found!")
            # Init openloop
            self.openloop = OpenLoop()
            self.openloop.a = openloop_json[KEY_OPENLOOP_A]
            self.openloop.b = openloop_json[KEY_OPENLOOP_B]
            self.openloop.c = openloop_json[KEY_OPENLOOP_C]
            self.openloop.fix_up = openloop_json[KEY_OPENLOOP_FIXUP]
            self.openloop.pwmMax = openloop_json[KEY_OPENLOOP_PWM_MAX]
            self.openloop.pwmMin = openloop_json[KEY_OPENLOOP_PWM_MIN]
            self.openloop.tempMin = openloop_json[KEY_OPENLOOP_TEMP_MIN]
            # Init PID
            self.fanPid = FanPid()
            self.fanPid.pwmMax = pid_json[KEY_PID_PWM_MAX]
            self.fanPid.pwmMin = pid_json[KEY_PID_PWM_MIN]
            self.fanPid.SetPoint = pid_json[KEY_PID_SETPOINT]
            self.fanPid.P = pid_json[KEY_PID_P]
            self.fanPid.I = pid_json[KEY_PID_I]
            self.fanPid.D = pid_json[KEY_PID_D]
            self.fanPid.tempMin = pid_json[KEY_PID_TEMP_MIN]
            self.fanPid.tempMax = pid_json[KEY_PID_TEMP_MAX]
            # Init thermal setting
            for key, item in list(thermal_json.items()):
                fanctrl_debug_log("%s %s " % (key,item))
                if key not in KEY_TEMP:
                    logger.error('Key %s not present in cfg file' % key)
                    return False
                self.temps[item] = -1.0
                self.tempsMax[item] = self.interface.get_thermal_temp_max(item)
                self.tempsMin[item] = self.interface.get_thermal_temp_min(item)
                self.tempStatus[item] = 0
                self.tempMissTime[item] = [0, 0]
                self.tempCritTime[item] = [0, 0]
                if key == self.fanPid.sensor:
                    self.fanPid.sensor = item

            # Init fans setting
            for key, item in list(fan_json.items()):
                self.fans[key] = item
                self.fanStatus[key] = 0
                self.fanErrTime[key] = [0, 0]
                self.fanLowTime[key] = [0, 0]
            fh.close()
        else:
            logger.error('%s is not a file' % FAN_CTRL_CFG_FILE)
            return False

        fanctrl_debug_log("Device AirFlow: %s" % (self.airflow))
        self.updateThermal()
        self.fanPid.last_temp = self.temps[self.fanPid.sensor]
        for i in range(3):
            self.fanPid.temps[i] = self.temps[self.fanPid.sensor]
        return True

    def setFanSpeed(self, speed):
        return self.interface.set_fan_speed_pwm(speed)

    def updateThermal(self):
        for key in self.temps:
            self.temps[key] = self.interface.get_thermal_temp(key)
            fanctrl_debug_log("%s temps %d C" % (key, self.temps[key]))

        if self.temps[KEY_INLET_TEMP] >= self.tempsMax[KEY_INLET_TEMP] or self.temps[KEY_INLET_TEMP] <= -99999:
            self.temps[KEY_INLET_TEMP] = self.tempsMax[KEY_INLET_TEMP]
        self.openloop.temp = self.temps[KEY_INLET_TEMP]
        self.fanPid.temps[2] = self.temps[KEY_INLET_TEMP]

    def checkThermal(self):
        thermal_cnt = 0
        for key in self.temps:
            if self.temps[key] <= -9999:
                if self.tempStatus[key] & STATUS_MISS_CRIT != 0:
                    self.tempMissTime[key][0] = time.time()
                else:
                    self.tempMissTime[key][1] = time.time()
                self.tempStatus[key] = self.tempStatus[key] | STATUS_MISS_CRIT
                if self.tempMissTime[key][1] - self.tempMissTime[key][0] > 15:
                    logger.warning("%s Read Invaild Temperautre %d " % (key, self.temps[key]))
                    self.tempStatus[key] = self.tempStatus[key] | STATUS_MISS_ERR
            else:
                self.tempStatus[key] = self.tempStatus[key] & ~(STATUS_MISS_CRIT | STATUS_MISS_ERR)

            if self.temps[key] >= self.tempsMax[key]:
                self.tempCritTime[key][0] = time.time()
                self.tempStatus[key] = self.tempStatus[key] | STATUS_HIGH_CRIT
                logger.warning("%s Temperautre %d >= High Threshold %d" % (key, self.temps[key], self.tempsMax[key]))
            elif self.tempStatus[key] & (STATUS_HIGH_CRIT) != 0:
                self.tempCritTime[key][1] = time.time()
                logger.warning("%s Temperautre %d Recovery" % (key, self.temps[key]))
                if self.tempCritTime[key][1] - self.tempCritTime[key][0] > 300:
                    self.tempStatus[key] = self.tempStatus[key] & ~(STATUS_HIGH_CRIT)

            if self.tempStatus[key] & ~(STATUS_MISS_CRIT) != 0:
                thermal_cnt = thermal_cnt + 1

        self.status = thermal_cnt
        fanctrl_debug_log("Thermal error num %d" % self.status)

    def checkFanSpeed(self):
        fan_error_cnt = 0
        for key, item in list(self.fans.items()):
            speed = self.interface.get_fan_speed_rpm(item)
            rpm_max = self.interface.get_fan_rpm_max()
            if self.fan_pwm == 100 and speed < rpm_max * 0.7:
                if self.fanStatus[key] & STATUS_BAD_FAN != 0:
                    self.fanErrTime[key][0] = time.time()
                else:
                    self.fanErrTime[key][1] = time.time()

                self.fanStatus[key] = self.fanStatus[key] | STATUS_BAD_FAN
                if self.fanErrTime[key][1] - self.fanErrTime[key][0] >= 30:
                    logger.warning("%s PWM is %d but Speed %d <= %d RPM" % (item, self.fan_pwm, speed, rpm_max * 0.7))
            else:
                self.fanStatus[key] = self.fanStatus[key] & ~(STATUS_BAD_FAN)

            if speed < 1000:
                if self.fanStatus[key] & STATUS_LOW_FAN != 0:
                    self.fanLowTime[key][0] = time.time()
                else:
                    self.fanLowTime[key][1] = time.time()
                self.fanStatus[key] = self.fanStatus[key] | STATUS_LOW_FAN
                if self.fanLowTime[key][1] - self.fanLowTime[key][0] > 30:
                    logger.warning("%s Speed %d <= %d RPM" % (item, speed, 1000))
            else:
                self.fanStatus[key] = self.fanStatus[key] & ~(STATUS_LOW_FAN)

            fanctrl_debug_log("%s speed %d RPM" % (key, speed))

            if self.fanStatus[key] != 0:
                fan_error_cnt = fan_error_cnt + 1
        self.fan_status = fan_error_cnt
        fanctrl_debug_log("Fan error num %d" % fan_error_cnt)

    def doApplyPolicy(self):
        if self.isLiquid == 1:
            return

        self.fan_pwm = 0
        if self.openloop_switch == 1:
            openloop_pwm = int(self.openloop.calcPwm())
            fanctrl_debug_log("OpenLoop pwm %d" % (openloop_pwm))
            self.fan_pwm = max(self.fan_pwm, openloop_pwm)

        if self.pid_switch == 1:
            pid_pwm = int(self.fanPid.calcPwm())
            fanctrl_debug_log("PID pwm %d" % (pid_pwm))
            self.fan_pwm = max(self.fan_pwm, pid_pwm)

        # Check fan presence
        if self.interface.get_fan_presence() == False:
            logger.warning("Fan presence check false, set fan pwm to 100")
            self.fan_pwm = self.fanerr_pwmmax
        if self.fan_status != 0 or self.status != 0:
            self.fan_pwm = self.fanerr_pwmmax
        fanctrl_debug_log("Fan Speed set to %d pwm" % self.fan_pwm)
        for i in range(3):
            ret = self.setFanSpeed(self.fan_pwm)
            if ret == False:
                logger.warning("Fan speed set %d pwm failed, retry %d times" % (self.fan_pwm, i + 1))
                continue
            break

    def doBoardFanLedCtrl(self):
        if self.interface.get_fan_status() != False and self.interface.get_fan_presence() != False and self.fan_status == 0:
            fanctrl_debug_log("Fan status good setting LED to green")
            self.interface.set_fan_board_led("green")
        else:
            fanctrl_debug_log("Fan status error setting LED to red")
            self.interface.set_fan_board_led("red")

    def doBoardPsuLedCtrl(self):
        if self.interface.get_psu_status() != False:
            fanctrl_debug_log("PSU status good setting LED to green")
            self.interface.set_psu_board_led("green")
        else:
            fanctrl_debug_log("PSU status error setting LED to red")
            self.interface.set_psu_board_led("red")

    def doSysLedCtrl(self):
        self.interface.set_sysled("green")

def run(interval, fanCtrol):
    loop = 0
    # waitForDocker()
    fanCtrol.setFanSpeed(fanCtrol.fan_pwm) #init set fan speed to 50 pwm
    fanCtrol.doFanCtrlInit()
    if fanCtrol.airflow == "Liquid":
        logger.warning('Liquid device stopping fancontrol')
        return True
    while True:
        try:
            if loop % 5 == 0: # Fan speed control
                try:
                    fanCtrol.updateThermal()
                except Exception as e:
                    logger.error('Failed: Update Thermal, %s' % str(e))
                    logger.error('%s' % traceback.format_exc())
                    time.sleep(3)
                    continue

                try:
                    fanCtrol.checkThermal()
                except Exception as e:
                    logger.error('Failed: Check Thermal, %s' % str(e))
                    logger.error('%s' % traceback.format_exc())
                    time.sleep(3)
                    continue

                try:
                    fanCtrol.checkFanSpeed()
                except Exception as e:
                    logger.error('Failed: Check Fan Speed, %s' % str(e))
                    logger.error('%s' % traceback.format_exc())
                    time.sleep(3)
                    continue

                try:
                    fanCtrol.doApplyPolicy()
                except Exception as e:
                    logger.error('Failed: Apply Policy, %s' % str(e))
                    logger.error('%s' % traceback.format_exc())
                    time.sleep(3)
                    continue

                try:
                    fanCtrol.doBoardFanLedCtrl()
                except Exception as e:
                    logger.error('Failed: Led Control, %s' % str(e))
                    logger.error('%s' % traceback.format_exc())
                    time.sleep(3)
                    continue

                try:
                    fanCtrol.doBoardPsuLedCtrl()
                except Exception as e:
                    logger.error('Failed: Led Control, %s' % str(e))
                    logger.error('%s' % traceback.format_exc())
                    time.sleep(3)
                    continue

                try:
                    fanCtrol.doSysLedCtrl()
                except Exception as e:
                    logger.error('Failed: Led Control, %s' % str(e))
                    logger.error('%s' % traceback.format_exc())
                    time.sleep(3)
                    continue

            time.sleep(interval)
            loop += interval
        except Exception as e:
            traceback.print_exc()
            logger.error(str(e))

@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''
    pass

@main.command()
def start():
    '''start fan control'''
    logger.info("FANCTROL start")
    fanCtrol = FanControl()
    interval = 1
    run(interval, fanCtrol)

@main.command()
def stop():
    '''stop fan control '''
    logger.info("FANCTROL stop")

# device_i2c operation
if __name__ == '__main__':
    main()
