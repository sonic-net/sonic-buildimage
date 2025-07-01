#############################################################################
# SuperMicro SSE-T7132S
#
# Module contains an implementation of SONiC Platform Base API and
# provides the fan status which are available in the platform
#
#############################################################################

import subprocess

try:
    from sonic_platform_base.fan_base import FanBase
    from .helper import APIHelper
    from swsscommon.swsscommon import SonicV2Connector
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


FAN_NAME_LIST = ["FAN-1", "FAN-2", "FAN-3", "FAN-4", "FAN-5", "FAN-6"]

IPMI_SENSOR_NETFN = "0x04"
IPMI_SS_READ_CMD = "0x2D {}"
IPMI_OEM_NETFN = "0x30"
IPMI_GET_FAN_SPEED_CMD = "0x70 0x66 0x00 {}"
IPMI_SET_FAN_SPEED_CMD = "0x70 0x66 0x01 {} {}"
IPMI_GET_FAN_LED_CMD = "0x89 0x03 0x00 {}"
IPMI_SET_FAN_LED_CMD = "0x89 0x03 0x01 {} {}"
IPMI_FAN_LED_OFF = 0x00
IPMI_FAN_LED_GREEN = 0x01
IPMI_FAN_LED_AMBER = 0x02
IPMI_FAN_LED_AMBER_BLINK = 0x03
IPMI_GET_PSU_FAN_SPEED_CMD = "0x89 0x04 0x{:02x} {}"

MAX_OUTLET = 29500
MAX_INLET = 25500
#MAX_PSU_FAN_OUTLET = 11200    # not a fixed value
#MAX_PSU_FAN_INLET = 11200     # not a fixed value
SPEED_TOLERANCE = 20    # based on the speed graph the slowest is about 20%

FAN_LIST = [
    #name           sensor_id led_num
    ('FAN1',        '0x41',   '0x04'),
    ('FAN2',        '0x42',   '0x05'),
    ('FAN3',        '0x43',   '0x06'),
    ('FAN4',        '0x44',   '0x07'),
    ('FAN5',        '0x45',   '0x08'),
    ('FAN6',        '0x46',   '0x09'),
]

SYSLOG_IDENTIFIER = "fan.py"
NOT_AVAILABLE = 'N/A'

class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self, fan_pwn_number=0, fan_index=0, is_psu_fan=False, psu=None):
        FanBase.__init__(self)
        #self.logger = logger.Logger(SYSLOG_IDENTIFIER)
        #self.logger.set_min_log_priority_debug()
        #self.logger.log_debug('init fan_pwn_number={} fan_index={}'.format(fan_pwn_number, fan_index))
        self.fan_pwn_number = fan_pwn_number
        self.is_psu_fan = is_psu_fan
        if self.is_psu_fan:
            self.psu = psu
            self.psu_index = self.psu.index
        self._api_helper = APIHelper()
        self.index = fan_index
        self.sensor_reading_addr = FAN_LIST[self.index][1]
        self.led_number = FAN_LIST[self.index][2]
        self.led_set = self.STATUS_LED_COLOR_OFF
        self.speed_set = None

    def get_direction(self):
        """
        Retrieves the direction of fan

        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        # read part number from eeprom
        # psu fan follows the same rule
        db = SonicV2Connector()
        db.connect(db.STATE_DB)
        eeprom_table = db.get_all(db.STATE_DB, 'EEPROM_INFO|0x22')
        if "Name" in eeprom_table and eeprom_table["Name"] == "Part Number" and "Value" in eeprom_table:
            part_number = eeprom_table["Value"]
        else:
            part_number_cmd = "sudo decode-syseeprom | grep 'Part Number' | grep -oE '[^ ]+$'"
            part_number = subprocess.Popen(part_number_cmd, shell=True, text=True, stdout=subprocess.PIPE).stdout.read()

        if "T7132SR" in part_number:
            # "SSE-T7132SR"
            direction = self.FAN_DIRECTION_INTAKE
        else:
            # "SSE-T7132S"
            direction = self.FAN_DIRECTION_EXHAUST

        return direction

    def get_speed_rpm(self):
        """
        Retrieves the speed of fan as RPM.

        Returns:
            An integer of RPM.
        """
        if self.is_psu_fan:
            status, raw_ss_read = self._api_helper.ipmi_raw(
                IPMI_OEM_NETFN, IPMI_GET_PSU_FAN_SPEED_CMD.format(self.psu_index + 1, "0x08"))
            rpm_speed = int("".join(raw_ss_read.split()[::-1]), 16) if status else 0
        else:
            status, raw_ss_read = self._api_helper.ipmi_raw(
                IPMI_SENSOR_NETFN, IPMI_SS_READ_CMD.format(self.sensor_reading_addr))
            # factor 140 should read from SDR
            rpm_speed = int(raw_ss_read.split()[0], 16) * 140 if status else 0

        return rpm_speed

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        rpm_speed = self.get_speed_rpm()

        if self.is_psu_fan:
            # psu fan do not know max speed, so return rpm
            speed = rpm_speed
            if speed <= 100:
                speed = 0       # to prevent be taken as percentage
        else:
            # when intake, the whole fan module is reversed, so still MAX_OUTLET
            max = MAX_OUTLET
            speed = int(float(rpm_speed)/max * 100)

        return speed

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        """
        status, raw_ss_read = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_GET_FAN_SPEED_CMD.format(self.fan_pwn_number))
        ss_read = raw_ss_read.split()[0]
        pwn = int(ss_read, 16)
        target = pwn
        """
        if self.is_psu_fan:
            # not support so return current speed
            target = self.get_speed()
        else:
            # set and get result are not the same with our ipmi oem command
            # This is because of the scaling between 100-based and 255-based
            # here just return the cached set value for tesstbed
            target = self.speed_set
            if target is None:
                target = self.get_speed()
        return target

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan

        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        return SPEED_TOLERANCE

    def set_speed(self, speed):
        """
        Sets the fan speed

        Args:
            speed: An integer, the percentage of full fan speed to set fan to,
                   in the range 0 (off) to 100 (full speed)

        Returns:
            A boolean, True if speed is set successfully, False if not
        """
        if self.is_psu_fan:
            # not support
            return False

        speed_hex = hex(speed)
        status, raw_ss_read = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_SET_FAN_SPEED_CMD.format(self.fan_pwn_number, speed_hex))
        set_speed = False if not status else True
        if set_speed:
            self.speed_set = speed
        return set_speed

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

        # there are only green and red led on fan
        led_color = {
            self.STATUS_LED_COLOR_GREEN: IPMI_FAN_LED_GREEN,
            self.STATUS_LED_COLOR_AMBER: IPMI_FAN_LED_AMBER,
            self.STATUS_LED_COLOR_RED: IPMI_FAN_LED_AMBER,
            self.STATUS_LED_COLOR_OFF: IPMI_FAN_LED_OFF
        }.get(color)
        status, set_led = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_SET_FAN_LED_CMD.format(self.led_number, led_color))
        if status:
            set_status_led = True
            self.led_set = color
        else:
            set_status_led = False

        return set_status_led

    def get_status_led(self):
        """
        Gets the state of the fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        if self.is_psu_fan:
            # Not support
            return NOT_AVAILABLE

        status, hx_color = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_GET_FAN_LED_CMD.format(self.led_number))
        # there are only green and red led on fan
        status_led = {
            "00": self.STATUS_LED_COLOR_OFF,
            "01": self.STATUS_LED_COLOR_GREEN,
            "02": self.STATUS_LED_COLOR_RED,
        }.get(hx_color, self.STATUS_LED_COLOR_OFF)

        # if it was set AMBER then return AMBER
        if status_led == self.STATUS_LED_COLOR_RED:
            if self.led_set == self.STATUS_LED_COLOR_AMBER:
                status_led = self.STATUS_LED_COLOR_AMBER

        return status_led

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        if self.is_psu_fan:
            fan_name = "PSU {} FAN-{}".format(self.psu_index+1, self.index+1)
        else:
            fan_name = FAN_NAME_LIST[self.index]

        return fan_name

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if FAN is present, False if not
        """
        if self.is_psu_fan:
            presence = self.psu.get_presence()
            return presence

        rpm_speed = self.get_speed_rpm()
        if rpm_speed == 0:
            presence = False
        else:
            presence = True

        return presence

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        if self.is_psu_fan:
            model = self.psu.get_model()
        else:
            model = "Unknown"

        return model

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        if self.is_psu_fan:
            serial = self.psu.get_serial()
        else:
            serial = "Unknown"

        return serial

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        if self.is_psu_fan:
            # psu status dose not include psu fan status
            # follow PWS-1K62A-1R HW P2 11122014.pdf defined slow fan (<1200rpm)
            rpm = self.get_speed_rpm()
            status = (rpm >= 1200)
            return status
        else:
            return self.get_presence() and self.get_speed() > 0

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return (self.index + 1)

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        if self.is_psu_fan:
            replaceable = self.psu.is_replaceable()
            return replaceable
        else:
            return True
