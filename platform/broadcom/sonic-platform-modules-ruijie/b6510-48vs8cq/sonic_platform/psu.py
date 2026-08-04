# -*- coding: utf-8 -*-

########################################################################
# Ruijie B6510-48VS8CQ
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs' information which are available in the platform
#
########################################################################


try:
    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.fan import Fan
    from sonic_platform.regutil import Reg
    from sonic_platform.logger import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Psu(PsuBase):
    """Ruijie Platform-specific PSU class"""

    # HWMON_DIR = "/sys/devices/platform/SMF.512/hwmon/"
    # HWMON_NODE = os.listdir(HWMON_DIR)[0]
    # MAILBOX_DIR = HWMON_DIR + HWMON_NODE

    def __init__(self, index, config=None, hal_psu=None):
        """
        "psus": [
            {
                "name": "psu1",
                "present": {"loc": "2-0037/psu_status", "format": DecodeFormat.ONE_BIT_HEX, "bit": 0},
                "status": {"loc": "2-0037/psu_status", "format": DecodeFormat.ONE_BIT_HEX, "bit": 1},
                "sn": {"loc": "7-0050/psu_sn"},
                "in_current": {"loc": "7-0058/hwmon/*/curr1_input", "format": DecodeFormat.THOUSANDTH},
                "in_voltage": {"loc": "7-0058/hwmon/*/in1_input", "format": DecodeFormat.THOUSANDTH},
                "out_voltage": {"loc": "7-0058/hwmon/*/in2_input", "format": DecodeFormat.THOUSANDTH},
                "out_current": {"loc": "7-0058/hwmon/*/curr2_input", "format": DecodeFormat.THOUSANDTH},
                "temperature": {"loc": "7-0058/hwmon/*/temp1_input", "format": DecodeFormat.THOUSANDTH},
                "hw_version": {"loc": "7-0050/psu_hw"},
                "psu_type": {"loc": "7-0050/psu_type"},
                "fans": [
                    {
                        "rotor": {
                            "speed": {"loc": "7-0058/hwmon/*/fan1_input"},
                            "speed_max": xx
                        }
                    }
                ],
                "in_power": {"loc": "7-0058/hwmon/*/power1_input", "format": DecodeFormat.MILLIONTH},
                "out_power": {"loc": "7-0058/hwmon/*/power2_input", "format": DecodeFormat.MILLIONTH},
            }
        ]
        """
        self._fan_list = []
        self.PSU_TEMP_MAX = 60 * 1000
        self.PSU_OUTPUT_POWER_MAX = 1300 * 1000
        self.PSU_OUTPUT_VOLTAGE_MIN = 11 * 1000
        self.PSU_OUTPUT_VOLTAGE_MAX = 14 * 1000
        self.index = index
        if config is not None:
            self.name = config.get("name")
            self.__reg_sn = Reg(config.get("sn"))
            self.__reg_present = Reg(config.get("present"))
            self.__reg_status = Reg(config.get("status"))
            self.__reg_out_vol = Reg(config.get("out_voltage"))
            self.__reg_out_cur = Reg(config.get("out_current"))
            self.__reg_out_pow = Reg(config.get("out_power"))
            self.__reg_pn = Reg(config.get("pn"))
            self.__reg_temperature = Reg(config.get("temperature"))
            self._fan_list = config.get("fans")
            self._psu_fan_parser(config.get("fans"))

        self._hal_psu = hal_psu

    def _psu_fan_parser(self, fans):
        if not isinstance(fans, list):
            raise TypeError("fan type error fans: {}".format(fans))
        for index in range(0,len(fans)):
            if not isinstance(fans[index], dict):
                raise TypeError("fan type must be a dict")
            self._fan_list.append(Fan(index, config=fans[index], is_psu_fan=True))

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
    def reg_out_vol(self):
        return self.__reg_out_vol

    @reg_out_vol.setter
    def reg_out_vol(self, val):
        self._reg_setter(self.__reg_out_vol, val)

    @property
    def reg_out_cur(self):
        return self.__reg_out_cur

    @reg_out_cur.setter
    def reg_out_cur(self, val):
        self._reg_setter(self.__reg_out_cur, val)

    @property
    def reg_out_pow(self):
        return self.__reg_out_pow

    @reg_out_pow.setter
    def reg_out_pow(self, val):
        self._reg_setter(self.__reg_out_pow, val)

    def get_all_fans(self):
        return self._fan_list

    def get_num_fans(self):
        return len(self._fan_list)
        
    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return self.name

    def get_presence(self):
        """
        Retrieves the presence of the Power Supply Unit (PSU)

        Returns:
            bool: True if PSU is present, False if not
        """
        if self._hal_psu:
            pass

        try:
            if isinstance(self.__reg_present, Reg):
                psu_presence = self.__reg_present.decode()
                if psu_presence == 0 or psu_presence == "0":
                    return True
        except Exception as e:
            logger.error(str(e))

        return False

    def get_model(self):
        """
        Retrieves the part number of the PSU

        Returns:
            string: Part number of PSU
        """

        if self._hal_psu:
            return self._hal_psu.pn()

        try:
            if isinstance(self.__reg_pn, Reg):
                return self.__reg_pn.decode()
        except Exception as e:
            logger.error(str(e))

        return "NA"

    def get_serial(self):
        """
        Retrieves the serial number of the PSU

        Returns:
            string: Serial number of PSU
        """
        if self._hal_psu:
            return self._hal_psu.sn()

        try:
            if isinstance(self.__reg_sn, Reg):
                return self.__reg_sn.decode()
        except Exception as e:
            logger.error(str(e))

        return "NA"

    def get_status(self):
        """
        Retrieves the operational status of the PSU

        Returns:
            bool: True if PSU is operating properly, False if not
        """
        # status = False
        # psu_status = self._get_pmc_register(self.psu_presence_reg)
        # if (psu_status != 'ERR'):
        #     psu_status = int(psu_status, 16)
        #     # Checking whether both bit 3 and bit 2 are not set
        #     if (~psu_status & 0b1000) and (~psu_status & 0b0100):
        #         status = True

        if self._hal_psu:
            return self._hal_psu.get_status()

        try:
            if isinstance(self.reg_status, Reg):
                psu_status = self.reg_status.decode()
                if psu_status == 1 or psu_status == "1":
                    return True
                elif psu_status == 0 or psu_status == "0":
                    return False
        except Exception as e:
            logger.error(str(e))

        return False

    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        # psu_voltage = self._get_pmc_register(self.psu_voltage_reg)
        # if (psu_voltage != 'ERR') and self.get_presence():
        #     # Converting the value returned by driver which is in
        #     # millivolts to volts
        #     psu_voltage = float(psu_voltage) / 1000
        # else:
        #     psu_voltage = 0.0

        if self._hal_psu:
            pass

        try:
            if isinstance(self.__reg_out_vol, Reg):
                return self.__reg_out_vol.decode()
        except Exception as e:
            logger.error(str(e))

        return 0.0

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, electric current in amperes,
            e.g. 15.4
        """
        # psu_current = self._get_pmc_register(self.psu_current_reg)
        # if (psu_current != 'ERR') and self.get_presence():
        #     # Converting the value returned by driver which is in
        #     # milliamperes to amperes
        #     psu_current = float(psu_current) / 1000
        # else:
        #     psu_current = 0.0

        if self._hal_psu:
            pass

        try:
            if isinstance(self.__reg_out_cur, Reg):
                return self.__reg_out_cur.decode()
        except Exception as e:
            logger.error(str(e))

        return 0.0

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts,
            e.g. 302.6
        """

        if self._hal_psu:
            pass

        try:
            if isinstance(self.__reg_out_pow, Reg):
                return self.__reg_out_pow.decode()
        except Exception as e:
            logger.error(str(e))

        return 0.0

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU

        Returns:
            A boolean, True if PSU has stablized its output voltages and
            passed all its internal self-tests, False if not.
        """
        if self._hal_psu:
            pass
        else:
            if self.get_status() and self.get_presence():
                return True

        return False

    def get_status_led(self):
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        # TODO
        if self.get_powergood_status():
            return self.STATUS_LED_COLOR_GREEN
        else:
            return self.STATUS_LED_COLOR_RED

    def set_status_led(self, color):
        """
        Sets the state of the PSU status LED
        Args:
            color: A string representing the color with which to set the
                   PSU status LED
        Returns:
            bool: True if status LED state is set successfully, False if
                  not
        """
        # not supported
        return False

    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125 
        """
        if self._hal_psu:
            pass

        try:
            if isinstance(self.__reg_temperature, Reg):
                return self.__reg_temperature.decode()
        except Exception as e:
            logger.error(str(e))

        return 0.0

    def get_temperature_high_threshold(self):
        """
        Retrieves the high threshold temperature of PSU
        Returns:
            A float number, the high threshold temperature of PSU in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return float(self.PSU_TEMP_MAX/1000)

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output
        Returns:
            A float number, the high threshold output voltage in volts, 
            e.g. 12.1 
        """
        return float(self.PSU_OUTPUT_VOLTAGE_MAX/1000)

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output
        Returns:
            A float number, the low threshold output voltage in volts, 
            e.g. 12.1 
        """
        return float(self.PSU_OUTPUT_VOLTAGE_MIN/1000)

    def get_maximum_supplied_power(self):
        """
        Retrieves the maximum supplied power by PSU
        Returns:
            A float number, the maximum power output in Watts.
            e.g. 1200.1
        """
        return float(self.PSU_OUTPUT_POWER_MAX/1000)

