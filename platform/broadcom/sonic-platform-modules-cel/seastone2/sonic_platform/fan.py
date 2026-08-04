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

NOT_AVAILABLE = 'N/A'

FAN_NAME_TEMPLATE = "{}Fan{}{}"
NUM_FAN_TRAYS = 4
NUM_FANS_PER_TRAY = 2
FAN_SPEED_TOLERANCE = 20
MAX_RPM_FRONT=23000
MAX_RPM_REAR=20500
MAX_RPM_PSU=22600
FAN_DIR_BASE = 0x41
FAN_LED_BASE = 0x04
FAN_FRU_BASE = 0x05
FAN_STATUS_BASE = 0x00
FAN_SID_BASE = 0x80
PSU_FAN_SID_BASE = 0x8a
PSU_FRU_BASE = 0x03


IPMI_FAN_DIR="ipmitool raw 0x3a 0x0c 0x00 0x02 {}"
IPMI_GET_SPEED="ipmitool raw 0x04 0x2d {}"
IPMI_GET_CPLD_PWM="ipmitool raw 0x3a 0x0c 0x00 0x02 {}"
IPMI_GET_PRESENCE="ipmitool raw 0x3a 0x03 0x03 {}"
IPMI_GET_MODEL="ipmitool fru list {} | grep 'Board Part Number'"
IPMI_GET_SERIAL="ipmitool fru list {} | grep 'Board Serial'"
IPMI_GET_PSU_MODEL="ipmitool fru list {} | grep 'Product Name'"
IPMI_GET_PSU_SPEED="ipmitool raw 0x04 0x2d {}"
IPMI_SET_STATUS_LED="ipmitool raw 0x3a 0x0a {} {}"
IPMI_GET_STATUS_LED="ipmitool raw 0x3a 0x0b {}"

class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self, fan_tray_index, fan_index=0, is_psu_fan=False, psu_index=0):
        self.fan_index = fan_index
        self.fan_tray_index = fan_tray_index
        self.is_psu_fan = is_psu_fan
        self._api_helper = APIHelper()
        self.index = (self.fan_tray_index * 2) + self.fan_index
        self.name = None
        if self.is_psu_fan:
            self.psu_index = psu_index
            self.max_speed = MAX_RPM_PSU
            self._fan_sid_offset = PSU_FAN_SID_BASE + self.psu_index
            self._fan_status_offset = FAN_STATUS_BASE + NUM_FAN_TRAYS + self.psu_index
        else:
            self._fan_status_offset = FAN_STATUS_BASE + self.fan_tray_index
            self._fan_fru_offset = FAN_FRU_BASE + self.fan_tray_index
            self._fan_dir_offset = FAN_DIR_BASE + (self.fan_tray_index * 4)
            if self.fan_tray_index > 1:
                # Questone CPLD firmware is used and hence FAN 3 will be missing
                # There are only 4 FAN trays in this platform
                self._fan_dir_offset = self._fan_dir_offset + 4

            self._fan_speed_offset = self._fan_dir_offset - 1
            self._fan_led_offset = FAN_LED_BASE + self.fan_tray_index

            if fan_index % 2 == 0:
                # Front FAN
                self.is_front = True
                self.max_speed = MAX_RPM_FRONT
                self._fan_sid_offset = FAN_SID_BASE + 1 + (self.fan_tray_index * NUM_FANS_PER_TRAY)
            else:
                # Rear FAN
                self.is_front = False
                self.max_speed = MAX_RPM_REAR
                self._fan_sid_offset = FAN_SID_BASE + (self.fan_tray_index * NUM_FANS_PER_TRAY)

    def get_direction(self):
        """
        Retrieves the direction of fan
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        direction = NOT_AVAILABLE

        if self.is_psu_fan:
            cmd = IPMI_GET_PSU_MODEL.format(PSU_FRU_BASE + self.psu_index)
            status, output = self._api_helper.run_command(cmd)
            if status and output:
                model = output.split(':')[-1]
                if len(model) > 0:
                    if model[-2:] == ' B':
                        direction = self.FAN_DIRECTION_INTAKE
                    else:
                        direction = self.FAN_DIRECTION_EXHAUST
        else:
            cmd = IPMI_FAN_DIR.format(self._fan_dir_offset)
            status, output = self._api_helper.run_command(cmd)
            if status:
                dir_num = int(output, 16) & 0x0C
                if dir_num == 0x0:
                    direction = self.FAN_DIRECTION_EXHAUST
                elif dir_num == 0x8:
                    direction = self.FAN_DIRECTION_INTAKE

        return direction

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        speed = 0

        if not self.is_psu_fan:
            multiplier = 150.0
        else:
            multiplier = 100.0

        cmd = IPMI_GET_PSU_SPEED.format(self._fan_sid_offset)
        status, output = self._api_helper.run_command(cmd)
        if status:
            raw_speed = output.split()[0]
            rpm_speed = int(raw_speed, 16) * multiplier
            speed = int((rpm_speed/self.max_speed) * 100)

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
        target_speed = 0

        if self.is_psu_fan:
            # Ignored for tolerance check
            return self.get_speed()

        cmd = IPMI_GET_CPLD_PWM.format(self._fan_speed_offset)
        status, output = self._api_helper.run_command(cmd)
        if status:
            fan_pwm = int(output, 16)
            target_speed = round(fan_pwm / 255 * 100)

        return target_speed

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan
        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        return FAN_SPEED_TOLERANCE

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

        # FAN speed is controlled by BCM always
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
        # There is no per Fan LED
        return True

    def drawer_set_status_led(self, color):
        status = False

        color_dict = {\
            self.STATUS_LED_COLOR_OFF: 0,\
            self.STATUS_LED_COLOR_AMBER: 1,\
            self.STATUS_LED_COLOR_RED: 1,\
            self.STATUS_LED_COLOR_GREEN: 2\
        }

        if not self.is_psu_fan:
            cmd = IPMI_SET_STATUS_LED.format(self._fan_led_offset, color_dict.get(color, 0))
            status, _ = self._api_helper.run_command(cmd) 

        return status


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
        status = NOT_AVAILABLE
        color_dict = {\
            0: self.STATUS_LED_COLOR_OFF,\
            1: self.STATUS_LED_COLOR_AMBER,\
            2: self.STATUS_LED_COLOR_GREEN\
        }

        if not self.is_psu_fan:
            cmd = IPMI_GET_STATUS_LED.format(self._fan_led_offset)
            status, output = self._api_helper.run_command(cmd) 
            if status:
                color = int(output, 16)
                status = color_dict.get(color, status)
 
        return status


    ##############################################################
    ###################### Device methods ########################
    ##############################################################

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """

        if not self.name:
            if not self.is_psu_fan:
                psu_name = ""
                fan_id = " {}".format(self.fan_tray_index + 1)
                fan_type = " Front" if self.is_front else " Rear"
            else:
                psu_name = "PSU {} ".format(self.psu_index + 1)
                fan_id = " {}".format(self.fan_tray_index + 1)
                fan_type = ""

            self.name = FAN_NAME_TEMPLATE.format(psu_name, fan_id, fan_type)

        return self.name

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if FAN is present, False if not
        """

        presence = False

        cmd = IPMI_GET_PRESENCE.format(self._fan_status_offset)
        status, output = self._api_helper.run_command(cmd)
        if status and output == "00":
            presence = True

        return presence

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        model = NOT_AVAILABLE

        if not self.is_psu_fan:
            cmd = IPMI_GET_MODEL.format(self._fan_fru_offset)
            status, output = self._api_helper.run_command(cmd)
            if status and output:
                return output.split()[-1]

        return model

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        serial = NOT_AVAILABLE

        if not self.is_psu_fan:
            cmd = IPMI_GET_SERIAL.format(self._fan_fru_offset)
            status, output = self._api_helper.run_command(cmd)
            if status and output:
                return output.split()[-1]

        return serial

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence() and self.get_speed() > 0

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        if not self.is_psu_fan:
            return True

        return False
