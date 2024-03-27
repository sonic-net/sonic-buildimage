#!/usr/bin/env python

#############################################################################
# Celestica
#
# Module contains an implementation of SONiC Platform Base API and
# provides the fan status which are available in the platform
#
#############################################################################

try:
    from sonic_platform_base.fan_base import FanBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

FAN_MAPPING = {
    0: {
        "name": 'FAN-1F',
        "fru_id": '5',
        "get_speed": '0x81',
        "set_speed": '0x40',
        "led_offset":'0x4',
        "presence": "0x00"
    },
    1: {
        "name": 'FAN-1R',
        "fru_id": '5',
        "get_speed": '0x80',
        "set_speed": '0x40',
        "led_offset":'0x4',
        "presence": "0x00"
    },
    2: {
        "name": 'FAN-2F',
        "fru_id": '6',
        "get_speed": '0x83',
        "set_speed": '0x44',
        "led_offset":'0x5',
        "presence": "0x01"
    },
    3: {
        "name": 'FAN-2R',
        "fru_id": '6',
        "get_speed": '0x82',
        "set_speed": '0x44',
        "led_offset":'0x5',
        "presence": "0x01"
    },
    4: {
        "name": 'FAN-3F',
        "fru_id": '7',
        "get_speed": '0x85',
        "set_speed": '0x4c',
        "led_offset":'0x6',
        "presence": "0x02"
    },
    5: {
        "name": 'FAN-3R',
        "fru_id": '7',
        "get_speed": '0x84',
        "set_speed": '0x4c',
        "led_offset":'0x6',
        "presence": "0x02"
    },
    6: {
        "name": 'FAN-4F',
        "fru_id": '8',
        "get_speed": '0x87',
        "set_speed": '0x50',
        "led_offset":'0x7',
        "presence": "0x03"
    },
    7: {
        "name": 'FAN-4R',
        "fru_id": '8',
        "get_speed": '0x86',
        "set_speed": '0x50',
        "led_offset":'0x7',
        "presence": "0x03"
    }
}

PSU_FAN_MAPPING = {
    0:{
        'speed':'0x8b'
    },
    1:{
        'speed':'0x8a'
    }
}
NULL_VAL = 'N/A'
MAX_RPM_FRONT=23000
MAX_RPM_REAR=20500

IPMI_FAN_DIR="ipmitool fru list {} | grep -E 'F2B|B2F'"
IPMI_GET_SPEED="ipmitool raw 0x04 0x2d {}"
IPMI_SET_SPEED="ipmitool raw 0x3a 0x0c 0x00 0x03 {} {}"
IPMI_GET_PRESENCE="ipmitool raw 0x3a 0x03 0x03 {}"
IPMI_GET_MODEL="ipmitool fru list {} | grep 'Board Part Number'"
IPMI_GET_SERIAL="ipmitool fru list {} | grep 'Board Serial'"
IPMI_GET_PSU_SPEED="ipmitool raw 0x04 0x2d {}"
IPMI_GET_STATUS_LED="ipmitool raw 0x3a 0x0b {}"

class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self, fan_tray_index, fan_index=0, is_psu_fan=False, psu_index=0, psu_fan_direction=NULL_VAL):
        self.fan_index = fan_index
        self.fan_tray_index = fan_tray_index
        self.is_psu_fan = is_psu_fan
        self._api_helper = APIHelper()
        if self.is_psu_fan:
            self.psu_index = psu_index
            self.psu_fan_direction = psu_fan_direction
        self.index = (self.fan_tray_index * 2) + self.fan_index

    def get_direction(self):
        """
        Retrieves the direction of fan
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        if self.is_psu_fan:
            return self.FAN_DIRECTION_INTAKE

        cmd=IPMI_FAN_DIR.format(FAN_MAPPING[self.index]['fru_id'])
        status, raw = self._api_helper.run_command(cmd)
        if status:
            return self.FAN_DIRECTION_INTAKE if 'B2F' in raw else self.FAN_DIRECTION_EXHAUST
        else:
            return NULL_VAL

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        speed = 0
        max_rpm=0
        if self.is_psu_fan:
            cmd=IPMI_GET_PSU_SPEED.format(PSU_FAN_MAPPING[self.psu_index]['speed'])
            status, raw = self._api_helper.run_command(cmd)
            if status:
                speed=int(int(raw.split()[0], 16)*100/22600*100)
        else:
            max_rpm = MAX_RPM_FRONT if 'F' in self.get_name() else MAX_RPM_REAR 
            cmd=IPMI_GET_SPEED.format(FAN_MAPPING[self.index]['get_speed'])
            status, raw = self._api_helper.run_command(cmd)
            if status:
                speed=int(int(raw.split()[0], 16)*150/max_rpm*100)

        if speed > 100:
            speed = 100
        return speed

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)

        Note:
            speed_pc = pwm_target/255*100

            0   : when PWM mode is use
            pwm : when pwm mode is not use
        """
        return self.get_speed()

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan
        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        return 10

    def set_speed(self, speed):
        """
        Sets the fan speed
        Args:
            speed: An integer, the percentage of full fan speed to set fan to,
                   in the range 0 (off) to 100 (full speed)
        Returns:
            A boolean, True if speed is set successfully, False if not
        Notes:
            pwm setting mode must set as Manual
            manual: systemctl stop fanctrl.service
            auto: systemctl start fanctrl.service
        """

        if self.is_psu_fan:
            # Not support
            return False

        speed_hex = hex(int(float(speed)/100 * 255))
        cmd=IPMI_SET_SPEED.format(FAN_MAPPING[self.index]['set_speed'], speed_hex)
        status, raw = self._api_helper.run_command(cmd)
        if status:
            return True
        return False

    def set_status_led(self, color):
        """
        Sets the state of the fan module status LED
        Args:
            color: A string representing the color with which to set the
                   fan module status LED
        Returns:
            bool: True if status LED state is set successfully, False if not
        """

        if self.is_psu_fan:
            # Not support
            return False

        return True


    def get_status_led(self):
        """
        Gets the state of the fan status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above

        Note:
            Output
            STATUS_LED_COLOR_GREEN = "green"
            STATUS_LED_COLOR_AMBER = "amber"
            STATUS_LED_COLOR_RED = "red"
            STATUS_LED_COLOR_OFF = "off"

            Input
            0x1: green
            0x2: red
            0x3: off
        """
        if self.is_psu_fan:
            # Not support
            return NULL_VAL

        cmd=IPMI_GET_STATUS_LED.format(FAN_MAPPING[self.index]["led_offset"])
        status, color = self._api_helper.run_command(cmd)
 
        if not status:
            return 'N/A'
 
        if int(color) == 0:
            return self.STATUS_LED_COLOR_OFF
        elif int(color) == 1:
            return self.STATUS_LED_COLOR_AMBER
        elif int(color) == 2:
            return self.STATUS_LED_COLOR_GREEN
 
        return 'N/A'


    ##############################################################
    ###################### Device methods ########################
    ##############################################################

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        fan_name = FAN_MAPPING[self.index]['name'] if not self.is_psu_fan else "PSU-{} FAN-{}".format(
            self.psu_index+1, self.fan_index+1)

        return fan_name

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if FAN is present, False if not
        """
        if self.is_psu_fan:
            from sonic_platform.psu import Psu
            return Psu(self.psu_index).get_presence()

        cmd=IPMI_GET_PRESENCE.format(FAN_MAPPING[self.index]['presence'])
        status, raw = self._api_helper.run_command(cmd)
        if status:
            return True if '00' in raw else False
        else:
            return False

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        if self.is_psu_fan:
            return 'N/A'

        cmd=IPMI_GET_MODEL.format(FAN_MAPPING[self.index]['fru_id'])
        status, raw = self._api_helper.run_command(cmd)
        if status:
            if raw:
                return raw.split()[-1]
            else:
                return NULL_VAL
        else:
            return NULL_VAL

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        if self.is_psu_fan:
            return 'N/A'

        cmd=IPMI_GET_SERIAL.format(FAN_MAPPING[self.index]['fru_id'])
        status, raw = self._api_helper.run_command(cmd)
        if status:
            if raw:
                return raw.split()[-1]
            else:
                return NULL_VAL
        else:
            return NULL_VAL

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence() and self.get_speed() > 0
