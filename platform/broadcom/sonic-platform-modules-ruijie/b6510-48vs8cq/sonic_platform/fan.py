# -*- coding: utf-8 -*-

########################################################################
# Ruijie B6510-48VS8CQ
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fans' information which are available in the platform.
#
########################################################################

try:
    from sonic_platform_base.fan_base import FanBase
    from sonic_platform.regutil import Reg
    from sonic_platform.rotor import Rotor
    from sonic_platform.logger import logger
    from math import ceil
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Fan(FanBase):
    """Ruijie Platform-specific Fan class"""

    MAX_SPEED_CODE = 255

    def __init__(self, index, config=None, hal_fan=None, is_psu_fan=False):
        self.index = index
        self.is_psu_fan = is_psu_fan   
        if config:
            if self.is_psu_fan:
                self.name = "Psu{}-{}".format(self.index, config.get("name"))
            else:
                self.name = config.get("name")
            self.__reg_sn = Reg(config.get("sn"))
            self.__reg_present = Reg(config.get("present"))
            self.__reg_status = Reg(config.get("status"))
            self.__reg_led = Reg(config.get("led"))
            self.__reg_pn = Reg(config.get("pn"))
            self.__led_colors = config.get("led_colors")

            # rotors
            rotors = config.get("rotors")
            if isinstance(rotors, list):
                self.__rotors = []
                for rotor in rotors:
                    self.__rotors.append(Rotor(rotor))

        self._hal_fan = hal_fan

    def _reg_setter(self, target, val):
        if isinstance(val, dict):
            target = Reg(val)
        elif isinstance(val, Reg):
            target = val
        else:
            raise ValueError
        return target

    @property
    def reg_sn(self):
        return self.__reg_sn

    @reg_sn.setter
    def reg_sn(self, val):
        self._reg_setter(self.__reg_sn, val)

    @property
    def reg_present(self):
        return self.__reg_present

    @reg_present.setter
    def reg_present(self, val):
        self._reg_setter(self.__reg_present, val)

    @property
    def reg_status(self):
        return self.__reg_status

    @reg_status.setter
    def reg_status(self, val):
        self._reg_setter(self.__reg_status, val)

    @property
    def reg_led(self):
        return self.__reg_led

    @reg_led.setter
    def reg_led(self, val):
        self._reg_setter(self.__reg_led, val)

    def get_name(self):
        """
        Retrieves the fan name
        Returns:
            string: The name of the device
        """
        return self.name

    def get_model(self):
        """
        Retrieves the part number of the FAN
        Returns:
            string: Part number of FAN
        """
        if self._hal_fan:
            return self._hal_fan.pn()

        try:
            if isinstance(self.__reg_pn, Reg):
                return self.__reg_pn.decode()
        except Exception as e:
            logger.error(str(e))

        return "NA"

    def get_serial(self):
        """
        Retrieves the serial number of the FAN
        Returns:
            string: Serial number of FAN
        """
        if self._hal_fan:
            return self._hal_fan.sn()

        try:
            if isinstance(self.__reg_sn, Reg):
                return self.__reg_sn.decode()
        except Exception as e:
            logger.error(str(e))

        return "NA"

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if fan is present, False if not
        """

        # print self.fan_presence_reg.decode()
        # return True if self.fan_presence_reg.decode() == 0 else False
        if self._hal_fan:
            return self._hal_fan.get_presence()

        try:
            if isinstance(self.__reg_present, Reg):
                present = self.__reg_present.decode()
                if present == 0 or present == "0":
                    return True
        except Exception as e:
            logger.error(str(e))

        return False

    def get_status(self):
        """
        Retrieves the operational status of the FAN
        Returns:
            bool: True if FAN is operating properly, False if not
        """

        # return True if self.fan_status_reg.decode() == 1 else False
        if self._hal_fan:
            return self._hal_fan.get_status()

        try:
            if isinstance(self.__reg_status, Reg):
                status = self.__reg_status.decode()
                if status == 1 or status == "1":
                    return True
        except Exception as e:
            pass

        return False

    def get_direction(self):
        """
        Retrieves the fan airflow direction
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction

        Notes:
            - Forward/Exhaust : Air flows from Port side to Fan side.
            - Reverse/Intake  : Air flows from Fan side to Port side.
        """

        # TODO
        return self.FAN_DIRECTION_EXHAUST

    def get_speed(self):
        """
        Retrieves the speed of fan
        Returns:
            int: percentage of the max fan speed
        """
        if self.get_presence():
            maxspeed = 0
            for r in self.__rotors:
                speed = r.get_speed_percentage()
                if speed > maxspeed:
                    maxspeed = speed
            return maxspeed
        else:
            return 0

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan
        Returns:
            An integer, the percentage of variance from target speed which is
        considered tolerable
        """
        # TODO
        return 0

    def set_speed(self, speed):
        """
        Set fan speed to expected value
        Args:
            speed: An integer, the percentage of full fan speed to set fan to,
                   in the range 0 (off) to 100 (full speed)
        Returns:
            bool: True if set success, False if fail.
        """
        if self.__rotors:
            speed_code = hex(int(ceil(float(self.MAX_SPEED_CODE) / 100 * speed)))
            return self.__rotors[0].set_speed(speed_code)

        return False

    def set_status_led(self, color):
        """
        Set led to expected color
        Args:
            color: A string representing the color with which to set the
                   fan module status LED
        Returns:
            bool: True if set success, False if fail.
        """
        # TODO
        if self.is_psu_fan:
            # No LED available for PSU Fan
            return False
        try:
            if color not in self.__led_colors:
                logger.error("color:%s not defined." % color)
                return False
            val = hex(self.__led_colors[color])
            if isinstance(self.__reg_led, Reg):
                return self.__reg_led.encode(val)
            return ret
        except Exception as e:
            logger.error(str(e))
        return False


    def get_status_led(self):
        """
        Gets the state of the Fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        # TODO
        if self.is_psu_fan:
            # No LED available for PSU Fan
            return None
        else:
            try:
                if isinstance(self.__reg_led, Reg) :
                    led_color = self.__reg_led.decode()
                    for color, code in self.__led_colors.items():
                        if code ^ led_color == 0:
                            return color
            except Exception as e:
                logger.error(str(e))

            return None

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        # TODO
        return 0
