########################################################################
# Ruijie B6510-48VS8CQ
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Thermals' information which are available in the platform
#
########################################################################


try:
    from sonic_platform_base.thermal_base import ThermalBase
    from sonic_platform.regutil import Reg
    from sonic_platform.logger import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Thermal(ThermalBase):
    def __init__(self, index, config=None, hal_thermal=None):
        self.index = index
        if config:
            self.name = config.get("name")
            self.__reg_low_threshold = Reg(config.get("low"))
            self.__reg_high_thresnold = Reg(config.get("high"))
            self.__reg_crit_low_threshold = Reg(config.get("crit_low"))
            self.__reg_crit_high_thresnold = Reg(config.get("crit_high"))
            self.__reg_temperature = Reg(config.get("temperature"))
            self.minimum_thermal = self.get_temperature()
            self.maximum_thermal = self.get_temperature()

    def get_name(self):
        """
        Retrieves the name of the thermal

        Returns:
            string: The name of the thermal
        """
        return self.name

    def get_presence(self):
        """
        Retrieves the presence of the thermal

        Returns:
            bool: True if thermal is present, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the Thermal

        Returns:
            string: Model/part number of Thermal
        """
        return "NA"

    def get_serial(self):
        """
        Retrieves the serial number of the Thermal

        Returns:
            string: Serial number of Thermal
        """
        return "NA"

    def get_status(self):
        """
        Retrieves the operational status of the thermal

        Returns:
            A boolean value, True if thermal is operating properly,
            False if not
        """
        if self.get_temperature() == 0.0:
            return False

        return True

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125 
        """
        try:
            if isinstance(self.__reg_temperature, Reg):
                return self.__reg_temperature.decode()
        except Exception as e:
            logger.error(str(e))

        return None

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        try:
            if isinstance(self.__reg_high_thresnold, Reg):
                return float(self.__reg_high_thresnold.decode())
        except Exception as e:
            logger.error(str(e))

        return None

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal

        Returns:
            A float number, the low threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        try:
            if isinstance(self.__reg_low_threshold, Reg):
                return float(self.__reg_low_threshold.decode())
        except Exception as e:
            logger.error(str(e))

        return None

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal

        Args : 
            temperature: A float number up to nearest thousandth of one degree Celsius, 
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        try:
            if isinstance(self.__reg_high_thresnold, Reg):
                temp_val = str(int(temperature * 1000))
                return self.__reg_high_thresnold.encode(temp_val)
        except Exception as e:
            logger.error(str(e))

        return False

    def set_low_threshold(self, temperature):
        """
        Sets the low threshold temperature of thermal

        Args : 
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        # not supported
        return False

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        try:
            if isinstance(self.__reg_crit_high_thresnold, Reg):
                return float(self.__reg_crit_high_thresnold.decode())
        except Exception as e:
            logger.error(str(e))

        return None

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal

        Returns:
            A float number, the low critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        try:
            if isinstance(self.__reg_crit_low_threshold, Reg):
                return float(self.__reg_crit_low_threshold.decode())
        except Exception as e:
            logger.error(str(e))

        return None

    def get_minimum_recorded(self):
        """
        Retrieves the minimum recorded temperature of thermal
        Returns:
            A float number, the minimum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        tmp = self.get_temperature()
        if tmp < self.minimum_thermal:
            self.minimum_thermal = tmp
        raise self.minimum_thermal

    def get_maximum_recorded(self):
        """
        Retrieves the maximum recorded temperature of thermal
        Returns:
            A float number, the maximum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        tmp = self.get_temperature()
        if tmp > self.maximum_thermal:
            self.maximum_thermal = tmp
        raise self.maximum_thermal

