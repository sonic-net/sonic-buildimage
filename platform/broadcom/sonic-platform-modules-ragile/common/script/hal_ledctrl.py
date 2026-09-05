#!/usr/bin/env python3
import time
import syslog
import traceback
from plat_hal.interface import interface
from plat_hal.baseutil import baseutil
try:
    import abc
except ImportError as error:
    raise ImportError(str(error) + " - required module not found") from error

SWITCH_TEMP = "SWITCH_TEMP"
F2B_AIR_FLOW = "intake"
B2F_AIR_FLOW = "exhaust"
ONIE_E2_NAME = "ONIE_E2"

# status
STATUS_PRESENT = "PRESENT"
STATUS_ABSENT = "ABSENT"
STATUS_OK = "OK"
STATUS_NOT_OK = "NOT OK"
STATUS_FAILED = "FAILED"
STATUS_UNKNOWN = "UNKNOWN"

LEDCTROL_DEBUG_FILE = "/etc/.ledcontrol_debug_flag"

LEDCTROLERROR = 1
LEDCTROLDEBUG = 2

debuglevel = 0
# led status defined
COLOR_GREEN = 1
COLOR_AMBER = 2
COLOR_RED = 3
LED_STATUS_DICT = {COLOR_GREEN: "green", COLOR_AMBER: "amber", COLOR_RED: "red"}


def ledcontrol_debug(s):
    if LEDCTROLDEBUG & debuglevel:
        syslog.openlog("LEDCONTROL", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)


def ledcontrol_error(s):
    if LEDCTROLERROR & debuglevel:
        syslog.openlog("LEDCONTROL", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_ERR, s)


def air_flow_warn(s):
    syslog.openlog("AIR_FLOW_MONITOR", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_LOCAL1 | syslog.LOG_WARNING, s)


def air_flow_error(s):
    syslog.openlog("AIR_FLOW_MONITOR", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_LOCAL1 | syslog.LOG_ERR, s)


def air_flow_emerg(s):
    syslog.openlog("AIR_FLOW_MONITOR", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_LOCAL1 | syslog.LOG_EMERG, s)


def debug_init():
    global debuglevel
    try:
        with open(LEDCTROL_DEBUG_FILE, "r") as fd:
            value = fd.read()
        debuglevel = int(value)
    except Exception:
        debuglevel = 0


class DevBase(object):
    __metaclass__ = abc.ABCMeta

    def __init__(self, name, air_flow_monitor):
        self.__name = name
        self.__air_flow_monitor = air_flow_monitor
        self.present = STATUS_UNKNOWN
        self.status = STATUS_UNKNOWN
        self.status_summary = STATUS_UNKNOWN
        self.origin_name = STATUS_UNKNOWN
        self.display_name = STATUS_UNKNOWN
        self.air_flow = STATUS_UNKNOWN
        self.led_status = COLOR_GREEN

    @property
    def name(self):
        return self.__name

    @property
    def air_flow_monitor(self):
        return self.__air_flow_monitor

    @abc.abstractmethod
    def get_present(self):
        """
        Gets the present status of PSU/FAN

        Returns:
            A string, e.g. 'PRESENT, ABSENT, FAILED'
        """
        raise NotImplementedError

    @abc.abstractmethod
    def get_status(self):
        """
        Gets the status of PSU/FAN

        Returns:
            A string, e.g. 'OK, NOT OK, FAILED'
        """
        raise NotImplementedError

    @abc.abstractmethod
    def update_dev_info(self):
        """
        update status and fru info of PSU/FAN

        include present, status, status_summary, part_model_name, product_name, air_flow
        """
        raise NotImplementedError

    @abc.abstractmethod
    def set_module_led(self, color):
        """
        set PSU/FAN module LED status

        Args:
            color: A string representing the color with which to set the
                   PSU/FAN module LED status

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        raise NotImplementedError


class DevPsu(DevBase):

    def __init__(self, name, air_flow_monitor, hal_interface):
        super(DevPsu, self).__init__(name, air_flow_monitor)
        self.int_case = hal_interface

    def get_psu_presence(self):
        return self.int_case.get_psu_presence(self.name)

    def get_psu_input_output_status(self):
        return self.int_case.get_psu_input_output_status(self.name)

    def get_psu_fru_info(self):
        return self.int_case.get_psu_fru_info(self.name)

    @property
    def na_ret(self):
        return self.int_case.na_ret

    def get_present(self):
        try:
            status = self.get_psu_presence()
            if status is True:
                return STATUS_PRESENT
            if status is False:
                return STATUS_ABSENT
        except Exception as e:
            ledcontrol_error("get %s present status error, msg: %s" % (self.name, str(e)))
        return STATUS_FAILED

    def get_status(self):
        try:
            status = self.get_psu_input_output_status()
            if status is True:
                return STATUS_OK
            if status is False:
                return STATUS_NOT_OK
        except Exception as e:
            ledcontrol_error("get %s status error, msg: %s" % (self.name, str(e)))
        return STATUS_FAILED

    def update_dev_info(self):
        try:
            # update status
            self.present = self.get_present()
            if self.present != STATUS_PRESENT:
                self.status = STATUS_UNKNOWN
                self.status_summary = self.present
            else:
                self.status = self.get_status()
                self.status_summary = self.status
            # update fru info if need air flow monitor
            if self.air_flow_monitor:
                dic = self.get_psu_fru_info()
                self.origin_name = dic["PN"]
                self.air_flow = dic["AirFlow"]
                self.display_name = dic["DisplayName"]
        except Exception as e:
            ledcontrol_error("update %s info error, msg: %s" % (self.name, str(e)))
            self.present = STATUS_FAILED
            self.status = STATUS_FAILED
            self.status_summary = STATUS_FAILED
            self.origin_name = self.na_ret
            self.air_flow = self.na_ret
            self.display_name = self.na_ret

    def set_module_led(self, color):
        """
        set PSU module LED is not support, always return True
        """
        return True


class DevFan(DevBase):

    def __init__(self, name, air_flow_monitor, hal_interface):
        super(DevFan, self).__init__(name, air_flow_monitor)
        self.int_case = hal_interface

    def get_fan_rotor_number(self):
        return self.int_case.get_fan_rotor_number(self.name)

    def get_fan_presence(self):
        return self.int_case.get_fan_presence(self.name)

    def get_fan_rotor_status(self, rotor_name):
        return self.int_case.get_fan_rotor_status(self.name, rotor_name)

    def get_fan_fru_info(self):
        return self.int_case.get_fan_fru_info(self.name)

    @property
    def na_ret(self):
        return self.int_case.na_ret

    def get_present(self):
        try:
            status = self.get_fan_presence()
            if status is True:
                return STATUS_PRESENT
            if status is False:
                return STATUS_ABSENT
        except Exception as e:
            ledcontrol_error("get %s present status error, msg: %s" % (self.name, str(e)))
        return STATUS_FAILED

    def get_status(self):
        try:
            rotor_num = self.get_fan_rotor_number()
            err_motor_num = 0
            for j in range(rotor_num):
                rotor_name = "Rotor" + str(j + 1)
                roll_status = self.get_fan_rotor_status(rotor_name)
                if roll_status is not True:
                    err_motor_num += 1
                    ledcontrol_debug("%s %s error, status %s" % (self.name, rotor_name, roll_status))
                else:
                    ledcontrol_debug("%s %s ok" % (self.name, rotor_name))
            if err_motor_num > 0:
                return STATUS_NOT_OK
            return STATUS_OK
        except Exception as e:
            ledcontrol_error("get %s status error, msg: %s" % (self.name, str(e)))
        return STATUS_FAILED

    def update_dev_info(self):
        try:
            # update status
            self.present = self.get_present()
            if self.present != STATUS_PRESENT:
                self.status = STATUS_UNKNOWN
                self.status_summary = self.present
            else:
                self.status = self.get_status()
                self.status_summary = self.status
            # update fru info if need air flow monitor
            if self.air_flow_monitor:
                dic = self.get_fan_fru_info()
                self.origin_name = dic["PN"]
                self.air_flow = dic["AirFlow"]
                self.display_name = dic["DisplayName"]
        except Exception as e:
            ledcontrol_error("update %s fru info error, msg: %s" % (self.name, str(e)))
            self.present = STATUS_FAILED
            self.status = STATUS_FAILED
            self.status_summary = STATUS_FAILED
            self.origin_name = self.na_ret
            self.air_flow = self.na_ret
            self.display_name = self.na_ret

    def set_module_led(self, color):
        ret = self.int_case.set_fan_led(self.name, color)
        if ret == 0:
            return True
        return False


class ledcontrol(object):

    def __init__(self):
        self.fan_obj_list = []
        self.psu_obj_list = []
        self.board_psu_led_status = COLOR_GREEN
        self.board_fan_led_status = COLOR_GREEN
        self.__board_air_flow = ""
        self.int_case = interface()
        self.__config = baseutil.get_monitor_config()
        self.__temps_threshold_config = self.__config["temps_threshold"]
        for temp_threshold in self.__temps_threshold_config.values():
            temp_threshold['temp'] = 0
            temp_threshold['fail_num'] = 0
        self.__ledcontrol_para = self.__config["ledcontrol_para"]
        self.__interval = self.__ledcontrol_para.get("interval", 5)
        self.__checkpsu = self.__ledcontrol_para.get("checkpsu", 0)
        self.__checkfan = self.__ledcontrol_para.get("checkfan", 0)
        self.__psu_amber_num = self.__ledcontrol_para.get("psu_amber_num")
        self.__fan_amber_num = self.__ledcontrol_para.get("fan_amber_num")
        self.__psu_air_flow_amber_num = self.__ledcontrol_para.get("psu_air_flow_amber_num", 0)
        self.__fan_air_flow_amber_num = self.__ledcontrol_para.get("fan_air_flow_amber_num", 0)
        self.__board_sys_led = self.__ledcontrol_para.get("board_sys_led", [])
        self.__board_psu_led = self.__ledcontrol_para.get("board_psu_led", [])
        self.__board_fan_led = self.__ledcontrol_para.get("board_fan_led", [])
        self.__psu_air_flow_monitor = self.__ledcontrol_para.get("psu_air_flow_monitor", 0)
        self.__fan_air_flow_monitor = self.__ledcontrol_para.get("fan_air_flow_monitor", 0)
        self.__fan_mix_list = self.__ledcontrol_para.get("fan_mix_list", [])

    @property
    def na_ret(self):
        return self.int_case.na_ret

    @property
    def checkpsu(self):
        return self.__checkpsu

    @property
    def checkfan(self):
        return self.__checkfan

    @property
    def psu_amber_num(self):
        return self.__psu_amber_num

    @property
    def fan_amber_num(self):
        return self.__fan_amber_num

    @property
    def psu_air_flow_amber_num(self):
        return self.__psu_air_flow_amber_num

    @property
    def fan_air_flow_amber_num(self):
        return self.__fan_air_flow_amber_num

    @property
    def psu_air_flow_monitor(self):
        return self.__psu_air_flow_monitor

    @property
    def fan_air_flow_monitor(self):
        return self.__fan_air_flow_monitor

    @property
    def board_sys_led(self):
        return self.__board_sys_led

    @property
    def board_psu_led(self):
        return self.__board_psu_led

    @property
    def board_fan_led(self):
        return self.__board_fan_led

    @property
    def fan_mix_list(self):
        return self.__fan_mix_list

    @property
    def interval(self):
        return self.__interval

    def get_fan_total_number(self):
        return self.int_case.get_fan_total_number()

    def get_psu_total_number(self):
        return self.int_case.get_psu_total_number()

    def get_onie_e2_obj(self, name):
        return self.int_case.get_onie_e2_obj(name)

    def set_led_color(self, led_name, color):
        try:
            ret = self.int_case.set_led_color(led_name, color)
        except Exception as e:
            ledcontrol_error("set %s led %s error, msg: %s" % (led_name, color, str(e)))
            ret = False
        return ret

    def set_sys_led(self, color):
        for led in self.board_sys_led:
            led_name = led.get("led_name")
            ret = self.set_led_color(led_name, color)
            if ret is True:
                ledcontrol_debug("set %s success, color:%s," % (led_name, color))
            else:
                ledcontrol_debug("set %s failed, color:%s," % (led_name, color))

    def set_psu_led(self, color):
        for led in self.board_psu_led:
            led_name = led.get("led_name")
            ret = self.set_led_color(led_name, color)
            if ret is True:
                ledcontrol_debug("set %s success, color:%s," % (led_name, color))
            else:
                ledcontrol_debug("set %s failed, color:%s," % (led_name, color))

    def set_fan_led(self, color):
        for led in self.board_fan_led:
            led_name = led.get("led_name")
            ret = self.set_led_color(led_name, color)
            if ret is True:
                ledcontrol_debug("set %s success, color:%s," % (led_name, color))
            else:
                ledcontrol_debug("set %s failed, color:%s," % (led_name, color))

    def set_fan_module_led(self):
        for fan_obj in self.fan_obj_list:
            color = LED_STATUS_DICT.get(fan_obj.led_status)
            ret = fan_obj.set_module_led(color)
            if ret is True:
                ledcontrol_debug("set %s module led success, color: %s," % (fan_obj.name, color))
            else:
                ledcontrol_debug("set %s module led failed, color: %s," % (fan_obj.name, color))

    @property
    def board_air_flow(self):
        air_flow_tuple = (F2B_AIR_FLOW, B2F_AIR_FLOW)
        if self.__board_air_flow not in air_flow_tuple:
            self.__board_air_flow = self.int_case.get_device_airflow(ONIE_E2_NAME)
            ledcontrol_debug("board_air_flow: %s" % self.__board_air_flow)
        return self.__board_air_flow

    def update_psu_info(self):
        for psu_obj in self.psu_obj_list:
            psu_obj.update_dev_info()
            ledcontrol_debug("%s present: [%s], status: [%s] status_summary [%s]" %
                             (psu_obj.name, psu_obj.present, psu_obj.status, psu_obj.status_summary))
            if psu_obj.air_flow_monitor:
                ledcontrol_debug("%s origin name: [%s], display name: [%s] air flow [%s]" %
                                 (psu_obj.name, psu_obj.origin_name, psu_obj.display_name, psu_obj.air_flow))

    def update_fan_info(self):
        for fan_obj in self.fan_obj_list:
            fan_obj.update_dev_info()
            ledcontrol_debug("%s present: [%s], status: [%s] status_summary [%s]" %
                             (fan_obj.name, fan_obj.present, fan_obj.status, fan_obj.status_summary))
            if fan_obj.air_flow_monitor:
                ledcontrol_debug("%s origin name: [%s], display name: [%s] air flow [%s]" %
                                 (fan_obj.name, fan_obj.origin_name, fan_obj.display_name, fan_obj.air_flow))

    def get_monitor_temp(self):
        sensorlist = self.int_case.get_temp_info()

        for temp_threshold in self.__temps_threshold_config.values():
            sensor = sensorlist.get(temp_threshold['name'])
            if sensor["Value"] is None:
                temp_threshold['fail_num'] += 1
                ledcontrol_error("get %s failed, fail_num = %d" % (temp_threshold['name'], temp_threshold['fail_num']))
            else:
                temp_threshold['fail_num'] = 0
                temp_threshold.setdefault('fix', 0)
                temp_threshold['temp'] = sensor["Value"] + temp_threshold['fix']
            ledcontrol_debug("%s = %d" % (temp_threshold['name'], temp_threshold['temp']))
            ledcontrol_debug("warning = %d, critical = %d" % (temp_threshold['warning'], temp_threshold['critical']))

    def is_temp_warning(self):
        warning_flag = False
        for temp_threshold in self.__temps_threshold_config.values():
            if temp_threshold['temp'] >= temp_threshold['warning']:
                warning_flag = True
                ledcontrol_debug("%s is over warning" % temp_threshold['name'])
                ledcontrol_debug(
                    "%s = %d, warning = %d" %
                    (temp_threshold['name'],
                     temp_threshold['temp'],
                        temp_threshold['warning']))
        return warning_flag

    def checkTempWarning(self):
        try:
            if self.is_temp_warning():
                ledcontrol_debug("temp is over warning")
                return True
        except Exception as e:
            ledcontrol_error("%%policy: checkTempWarning failed")
            ledcontrol_error(str(e))
        return False

    def is_temp_critical(self):
        critical_flag = False
        for temp_threshold in self.__temps_threshold_config.values():
            temp_threshold['critical_flag'] = False
            if temp_threshold['temp'] >= temp_threshold['critical']:
                critical_flag = True
                temp_threshold['critical_flag'] = True
                ledcontrol_debug("%s is over critical" % temp_threshold['name'])
                ledcontrol_debug(
                    "%s = %d, critical = %d" %
                    (temp_threshold['name'],
                     temp_threshold['temp'],
                        temp_threshold['critical']))
        return critical_flag

    def checkTempCrit(self):
        try:
            if self.is_temp_critical():
                temp_dict = dict(self.__temps_threshold_config)
                tmp = temp_dict.get(SWITCH_TEMP)
                if tmp['critical_flag'] is True:
                    ledcontrol_debug("temp is over critical")
                    return True

                del temp_dict[SWITCH_TEMP]
                for temp_items in temp_dict.values():
                    if temp_items['critical_flag'] is False:
                        return False

                ledcontrol_debug("temp is over critical")
                return True
        except Exception as e:
            ledcontrol_error("%%policy: checkTempCrit failed")
            ledcontrol_error(str(e))
        return False

    def check_board_air_flow(self):
        board_air_flow = self.board_air_flow
        air_flow_tuple = (F2B_AIR_FLOW, B2F_AIR_FLOW)
        if board_air_flow not in air_flow_tuple:
            air_flow_error("%%AIR_FLOW_MONITOR-3-BOARD: Get board air flow failed, value: %s." % board_air_flow)
            return False
        ledcontrol_debug("board air flow check ok: %s" % board_air_flow)
        return True

    def get_monitor_fan_status(self):
        fanerrnum = 0
        for fan_obj in self.fan_obj_list:
            status = fan_obj.status_summary
            ledcontrol_debug("%s status: %s" % (fan_obj.name, status))
            if status != STATUS_OK:
                fan_obj.led_status = COLOR_RED
                fanerrnum += 1
            else:
                fan_obj.led_status = COLOR_GREEN
        ledcontrol_debug("fan error number: %d" % fanerrnum)

        if fanerrnum == 0:
            fan_led_status = COLOR_GREEN
        elif fanerrnum <= self.fan_amber_num:
            fan_led_status = COLOR_AMBER
        else:
            fan_led_status = COLOR_RED
        ledcontrol_debug("monitor fan status, set fan led: %s" % LED_STATUS_DICT.get(fan_led_status))
        return fan_led_status

    def get_monitor_psu_status(self):
        psuerrnum = 0
        for psu_obj in self.psu_obj_list:
            status = psu_obj.status_summary
            ledcontrol_debug("%s status: %s" % (psu_obj.name, status))
            if status != STATUS_OK:
                psu_obj.led_status = COLOR_RED
                psuerrnum += 1
            else:
                psu_obj.led_status = COLOR_GREEN
        ledcontrol_debug("psu error number: %d" % psuerrnum)

        if psuerrnum == 0:
            psu_led_status = COLOR_GREEN
        elif psuerrnum <= self.psu_amber_num:
            psu_led_status = COLOR_AMBER
        else:
            psu_led_status = COLOR_RED
        ledcontrol_debug("monitor psu status, set psu led: %s" % LED_STATUS_DICT.get(psu_led_status))
        return psu_led_status

    def get_monitor_fan_air_flow(self):
        if self.fan_air_flow_monitor == 0:
            ledcontrol_debug("fan air flow monitor not open, default green")
            return COLOR_GREEN

        ret = self.check_board_air_flow()
        if ret is False:
            ledcontrol_debug("check board air flow error, skip fan air flow monitor.")
            return COLOR_GREEN

        fan_led_status_list = []
        fan_air_flow_ok_obj_list = []
        fan_air_flow_ok_set = set()
        fan_module_led_list = []
        fan_air_flow_err_num = 0
        for fan_obj in self.fan_obj_list:
            if fan_obj.present != STATUS_PRESENT:
                fan_module_led_list.append(COLOR_GREEN)
                continue
            if fan_obj.air_flow == self.na_ret:
                air_flow_warn("%%AIR_FLOW_MONITOR-4-FAN: %s get air flow failed, fan model: %s, air flow: %s." %
                              (fan_obj.name, fan_obj.display_name, fan_obj.air_flow))
                led_status = COLOR_AMBER
                fan_module_led_list.append(led_status)
            elif fan_obj.air_flow != self.board_air_flow:
                air_flow_emerg("%%AIR_FLOW_MONITOR-0-FAN: %s air flow error, fan model: %s, fan air flow: %s, board air flow: %s." %
                               (fan_obj.name, fan_obj.display_name, fan_obj.air_flow, self.board_air_flow))
                led_status = COLOR_RED
                fan_air_flow_err_num += 1
            else:
                fan_air_flow_ok_obj_list.append(fan_obj)
                fan_air_flow_ok_set.add(fan_obj.origin_name)
                ledcontrol_debug("%s air flow check ok, origin name: [%s], display name: [%s], fan air flow: [%s], board air flow: [%s]" %
                                 (fan_obj.name, fan_obj.origin_name, fan_obj.display_name, fan_obj.air_flow, self.board_air_flow))
                led_status = COLOR_GREEN
                fan_module_led_list.append(led_status)
            if led_status > fan_obj.led_status:
                fan_obj.led_status = led_status
        if len(fan_module_led_list) != 0:
            fan_led_status = max(fan_module_led_list)
            fan_led_status_list.append(fan_led_status)
        # check fan mixing
        if len(fan_air_flow_ok_set) > 1 and fan_air_flow_ok_set not in self.fan_mix_list:
            for fan_obj in fan_air_flow_ok_obj_list:
                air_flow_warn("%%AIR_FLOW_MONITOR-4-FAN: %s mixing, fan model: %s, air flow: %s." %
                              (fan_obj.name, fan_obj.origin_name, fan_obj.air_flow))
            fan_led_status = COLOR_AMBER
            fan_led_status_list.append(fan_led_status)
        # check fan air flow error number
        if fan_air_flow_err_num == 0:
            fan_led_status = COLOR_GREEN
        elif fan_air_flow_err_num <= self.fan_air_flow_amber_num:
            fan_led_status = COLOR_AMBER
        else:
            fan_led_status = COLOR_RED
        fan_led_status_list.append(fan_led_status)

        fan_led_status = max(fan_led_status_list)
        ledcontrol_debug("monitor fan air flow, set fan led: %s" % LED_STATUS_DICT.get(fan_led_status))
        return fan_led_status

    def get_monitor_psu_air_flow(self):
        if self.psu_air_flow_monitor == 0:
            ledcontrol_debug("psu air flow monitor not open, default green")
            return COLOR_GREEN

        ret = self.check_board_air_flow()
        if ret is False:
            ledcontrol_debug("check board air flow error, skip psu air flow monitor.")
            return COLOR_GREEN

        psu_led_status_list = []
        psu_module_led_list = []
        psu_air_flow_err_num = 0
        for psu_obj in self.psu_obj_list:
            if psu_obj.present != STATUS_PRESENT:
                psu_module_led_list.append(COLOR_GREEN)
                continue
            if psu_obj.air_flow == self.na_ret:
                air_flow_warn("%%AIR_FLOW_MONITOR-4-PSU: %s get air flow failed, psu model: %s, air flow: %s." %
                              (psu_obj.name, psu_obj.display_name, psu_obj.air_flow))
                led_status = COLOR_AMBER
                psu_module_led_list.append(led_status)
            elif psu_obj.air_flow != self.board_air_flow:
                air_flow_emerg("%%AIR_FLOW_MONITOR-0-PSU: %s air flow error, psu model: %s, psu air flow: %s, board air flow: %s." %
                               (psu_obj.name, psu_obj.display_name, psu_obj.air_flow, self.board_air_flow))
                led_status = COLOR_RED
                psu_air_flow_err_num += 1
            else:
                ledcontrol_debug("%s psu air flow check ok, origin name: [%s], display name: [%s], psu air flow: [%s], board air flow: [%s]" %
                                 (psu_obj.name, psu_obj.origin_name, psu_obj.display_name, psu_obj.air_flow, self.board_air_flow))
                led_status = COLOR_GREEN
                psu_module_led_list.append(led_status)
            if led_status > psu_obj.led_status:
                psu_obj.led_status = led_status

        if len(psu_module_led_list) != 0:
            psu_led_status = max(psu_module_led_list)
            psu_led_status_list.append(psu_led_status)

        # check fan air flow error number
        if psu_air_flow_err_num == 0:
            psu_led_status = COLOR_GREEN
        elif psu_air_flow_err_num <= self.psu_air_flow_amber_num:
            psu_led_status = COLOR_AMBER
        else:
            psu_led_status = COLOR_RED
        psu_led_status_list.append(psu_led_status)

        psu_led_status = max(psu_led_status_list)
        ledcontrol_debug("monitor psu air flow, set psu led: %s" % LED_STATUS_DICT.get(psu_led_status))
        return psu_led_status

    def get_temp_sys_led_status(self):
        if self.checkTempCrit() is True:
            sys_led_status = COLOR_RED
        elif self.checkTempWarning() is True:
            sys_led_status = COLOR_AMBER
        else:
            sys_led_status = COLOR_GREEN
        ledcontrol_debug("monitor temperature, set sys led: %s" % LED_STATUS_DICT.get(sys_led_status))
        return sys_led_status

    def get_sys_led_follow_fan_status(self):

        if self.checkfan:
            sys_led_status = self.board_fan_led_status
            ledcontrol_debug("sys led follow fan led, set sys led: %s" % LED_STATUS_DICT.get(sys_led_status))
        else:
            sys_led_status = COLOR_GREEN
            ledcontrol_debug("sys led don't follow fan led, set default green")
        return sys_led_status

    def get_sys_led_follow_psu_status(self):
        if self.checkpsu:
            sys_led_status = self.board_psu_led_status
            ledcontrol_debug("sys led follow psu led, set sys led: %s" % LED_STATUS_DICT.get(sys_led_status))
        else:
            sys_led_status = COLOR_GREEN
            ledcontrol_debug("sys led don't follow psu led, set default green")
        return sys_led_status

    def dealSysLedStatus(self):
        sys_led_status_list = []
        # get_monitor_temp
        self.get_monitor_temp()

        # monitor temp get sys led status
        sys_led_status = self.get_temp_sys_led_status()
        sys_led_status_list.append(sys_led_status)

        # check sys led follow fan led status
        sys_led_status = self.get_sys_led_follow_fan_status()
        sys_led_status_list.append(sys_led_status)

        # check sys led follow psu led status
        sys_led_status = self.get_sys_led_follow_psu_status()
        sys_led_status_list.append(sys_led_status)

        sys_led_status = max(sys_led_status_list)
        sys_led_color = LED_STATUS_DICT.get(sys_led_status)

        # set sys led
        self.set_sys_led(sys_led_color)

    def dealFanLedStatus(self):
        fan_led_status_list = []
        # update fan info
        self.update_fan_info()

        # monitor fan status first
        fan_led_status = self.get_monitor_fan_status()
        fan_led_status_list.append(fan_led_status)

        # monitor fan air flow
        fan_led_status = self.get_monitor_fan_air_flow()
        fan_led_status_list.append(fan_led_status)

        self.board_fan_led_status = max(fan_led_status_list)
        fan_led_color = LED_STATUS_DICT.get(self.board_fan_led_status)

        # set fan led
        self.set_fan_led(fan_led_color)
        # set fan module led
        self.set_fan_module_led()

    def dealPsuLedStatus(self):
        psu_led_status_list = []
        # update psu info
        self.update_psu_info()

        # monitor psu status first
        psu_led_status = self.get_monitor_psu_status()
        psu_led_status_list.append(psu_led_status)

        # monitor psu air flow
        psu_led_status = self.get_monitor_psu_air_flow()
        psu_led_status_list.append(psu_led_status)

        self.board_psu_led_status = max(psu_led_status_list)
        psu_led_color = LED_STATUS_DICT.get(self.board_psu_led_status)

        # set psu led
        self.set_psu_led(psu_led_color)

    def do_ledcontrol(self):
        self.dealPsuLedStatus()
        self.dealFanLedStatus()
        self.dealSysLedStatus()

    def fan_obj_init(self):
        fan_num = self.get_fan_total_number()
        for i in range(fan_num):
            fan_name = "FAN" + str(i + 1)
            fan_obj = DevFan(fan_name, self.fan_air_flow_monitor, self.int_case)
            self.fan_obj_list.append(fan_obj)
        ledcontrol_debug("fan object initialize success")

    def psu_obj_init(self):
        psu_num = self.get_psu_total_number()
        for i in range(psu_num):
            psu_name = "PSU" + str(i + 1)
            psu_obj = DevPsu(psu_name, self.psu_air_flow_monitor, self.int_case)
            self.psu_obj_list.append(psu_obj)
        ledcontrol_debug("psu object initialize success")

    def run(self):
        while True:
            try:
                debug_init()
                self.do_ledcontrol()
                time.sleep(self.interval)
            except Exception as e:
                traceback.print_exc()
                ledcontrol_error(str(e))


if __name__ == '__main__':
    debug_init()
    ledcontrol_debug("enter main")
    led_control = ledcontrol()
    led_control.fan_obj_init()
    led_control.psu_obj_init()
    led_control.run()
