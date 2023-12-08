#
# Copyright (c) 2019-2023 NVIDIA CORPORATION & AFFILIATES.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#############################################################################
# Mellanox
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs status which are available in the platform
#
#############################################################################

try:
    import os
    import time
    from sonic_platform_base.psu_base import PsuBase
    from sonic_py_common.logger import Logger
    from .device_data import DeviceDataManager
    from .led import PsuLed, SharedLed, ComponentFaultyIndicator
    from . import utils
    from .vpd_parser import VpdParser
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


# Global logger class instance
logger = Logger()

PSU_PATH = '/var/run/hw-management/'


class FixedPsu(PsuBase):
    def __init__(self, psu_index):
        super(FixedPsu, self).__init__()
        self.index = psu_index + 1
        self._name = "PSU {}".format(self.index)
        self.psu_oper_status = os.path.join(PSU_PATH, "thermal/psu{}_pwr_status".format(self.index))
        self._led = None

    def get_name(self):
        return self._name

    def get_model(self):
        return 'N/A'

    def get_serial(self):
        return 'N/A'

    def get_revision(self):
        return 'N/A'

    def get_powergood_status(self):
        """
        Retrieves the operational status of power supply unit (PSU) defined

        Returns:
            bool: True if PSU is operating properly, False if not
        """
        return utils.read_int_from_file(self.psu_oper_status) == 1

    def get_presence(self):
        """
        Retrieves the presence status of power supply unit (PSU) defined

        Returns:
            bool: True if PSU is present, False if not
        """
        return True

    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        return None

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        return None

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        return None

    @property
    def led(self):
        if not self._led:
            self._led = PsuLed(self.index)
        return self._led

    def set_status_led(self, color):
        """
        Sets the state of the PSU status LED

        Args:
            color: A string representing the color with which to set the
                   PSU status LED

        Returns:
            bool: True if status LED state is set successfully, False if not

        Notes:
            Only one led for all PSUs.
        """
        return self.led.set_status(color)

    def get_status_led(self):
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        return self.led.get_status()

    def get_power_available_status(self):
        """
        Gets the power available status

        Returns:
            True if power is present and power on.
            False and "absence of PSU" if power is not present.
            False and "absence of power" if power is present but not power on.
        """
        if not self.get_presence():
            return False, "absence of PSU"
        elif not self.get_powergood_status():
            return False, "absence of power"
        else:
            return True, ""

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device
        Returns:
            integer: The 1-based relative physical position in parent device
        """
        return self.index

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        return None

    def get_temperature_high_threshold(self):
        """
        Retrieves the high threshold temperature of PSU

        Returns:
            A float number, the high threshold temperature of PSU in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return None

    def get_input_voltage(self):
        """
        Retrieves current PSU voltage input

        Returns:
            A float number, the input voltage in volts,
            e.g. 12.1
        """
        return None

    def get_input_current(self):
        """
        Retrieves the input current draw of the power supply

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        return None

class Psu(FixedPsu):
    """Platform-specific Psu class"""
    PSU_CURRENT = "power/psu{}_curr"
    PSU_POWER = "power/psu{}_power"
    PSU_VPD = "eeprom/psu{}_vpd"
    PSU_CURRENT_IN = "power/psu{}_curr_in"
    PSU_VOLT_IN = "power/psu{}_volt_in"
    PORT_AMBIENT_TEMP = os.path.join(PSU_PATH, "thermal/port_amb")
    FAN_AMBIENT_TEMP = os.path.join(PSU_PATH, "thermal/fan_amb")
    AMBIENT_TEMP_CRITICAL_THRESHOLD = os.path.join(PSU_PATH, "config/amb_tmp_crit_limit")
    AMBIENT_TEMP_WARNING_THRESHOLD = os.path.join(PSU_PATH, "config/amb_tmp_warn_limit")
    PSU_POWER_SLOPE = os.path.join(PSU_PATH, "config/psu{}_power_slope")

    shared_led = None

    def __init__(self, psu_index):
        super(Psu, self).__init__(psu_index)

        self._psu_voltage = None
        self._psu_voltage_min = None
        self._psu_voltage_max = None
        self._psu_voltage_capability = None

        self.psu_voltage_in = os.path.join(PSU_PATH, self.PSU_VOLT_IN.format(self.index))

        self.psu_current = os.path.join(PSU_PATH, self.PSU_CURRENT.format(self.index))
        self.psu_current_in = os.path.join(PSU_PATH, self.PSU_CURRENT_IN.format(self.index))
        self.psu_power = os.path.join(PSU_PATH, self.PSU_POWER.format(self.index))
        self.psu_power_max = self.psu_power + "_max"
        self.psu_presence = os.path.join(PSU_PATH, "thermal/psu{}_status".format(self.index))

        self.psu_power_max_capacity = os.path.join(PSU_PATH, "config/psu{}_power_capacity".format(self.index))

        self.psu_temp = os.path.join(PSU_PATH, 'thermal/psu{}_temp1'.format(self.index))
        self.psu_temp_threshold = os.path.join(PSU_PATH, 'thermal/psu{}_temp1_max'.format(self.index))

        self.psu_power_slope = os.path.join(PSU_PATH, self.PSU_POWER_SLOPE.format(self.index))

        from .fan import PsuFan
        self._fan_list.append(PsuFan(psu_index, 1, self))

        self.vpd_parser = VpdParser(os.path.join(PSU_PATH, self.PSU_VPD.format(self.index)))

        # initialize thermal for PSU
        from .thermal import initialize_psu_thermal
        self._thermal_list = initialize_psu_thermal(psu_index, self.get_power_available_status)

    @property
    def psu_voltage(self):
        if not self._psu_voltage:
            psu_voltage_out = os.path.join(PSU_PATH, "power/psu{}_volt_out2".format(self.index))
            if os.path.exists(psu_voltage_out):
                self._psu_voltage = psu_voltage_out
            else:
                psu_voltage_out = os.path.join(PSU_PATH, "power/psu{}_volt".format(self.index))
                if os.path.exists(psu_voltage_out):
                    self._psu_voltage = psu_voltage_out

        return self._psu_voltage

    @property
    def psu_voltage_min(self):
        if not self._psu_voltage_min:
            psu_voltage = self.psu_voltage
            if psu_voltage:
                self._psu_voltage_min = psu_voltage + "_min"

        return self._psu_voltage_min

    @property
    def psu_voltage_max(self):
        if not self._psu_voltage_max:
            psu_voltage = self.psu_voltage
            if psu_voltage:
                self._psu_voltage_max = psu_voltage + "_max"

        return self._psu_voltage_max

    @property
    def psu_voltage_capability(self):
        if not self._psu_voltage_capability:
            psu_voltage = self.psu_voltage
            if psu_voltage:
                self._psu_voltage_capability = psu_voltage + "_capability"

        return self._psu_voltage_capability


    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        return self.vpd_parser.get_model()

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        return self.vpd_parser.get_serial()

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return self.vpd_parser.get_revision()

    def get_presence(self):
        """
        Retrieves the presence status of power supply unit (PSU) defined

        Returns:
            bool: True if PSU is present, False if not
        """
        return utils.read_int_from_file(self.psu_presence) == 1

    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        if self.get_powergood_status() and self.psu_voltage:
            voltage = utils.read_int_from_file(self.psu_voltage, log_func=logger.log_info)
            return float(voltage) / 1000
        return None

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        if self.get_powergood_status():
            amperes = utils.read_int_from_file(self.psu_current, log_func=logger.log_info)
            return float(amperes) / 1000
        return None

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        if self.get_powergood_status():
            power = utils.read_int_from_file(self.psu_power, log_func=logger.log_info)
            return float(power) / 1000000
        return None

    @classmethod
    def get_shared_led(cls):
        if not cls.shared_led:
            cls.shared_led = SharedLed(PsuLed(None))
        return cls.shared_led

    @property
    def led(self):
        if not self._led:
            self._led = ComponentFaultyIndicator(Psu.get_shared_led())
        return self._led

    def get_status_led(self):
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        return Psu.get_shared_led().get_status()

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        if self.get_powergood_status():
            temp = utils.read_int_from_file(self.psu_temp, log_func=logger.log_info)
            return float(temp) / 1000

        return None

    def get_temperature_high_threshold(self):
        """
        Retrieves the high threshold temperature of PSU

        Returns:
            A float number, the high threshold temperature of PSU in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.get_powergood_status():
            temp_threshold = utils.read_int_from_file(self.psu_temp_threshold, log_func=logger.log_info)
            return float(temp_threshold) / 1000

        return None

    @utils.default_return(None)
    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output

        Returns:
            A float number, the high threshold output voltage in volts,
            e.g. 12.1

        Notes:
            The thresholds of voltage are not supported on all platforms.
            So we have to check capability first.
        """
        if self.psu_voltage_capability and self.psu_voltage_max and self.get_powergood_status():
            capability = utils.read_str_from_file(self.psu_voltage_capability)
            if 'max' in capability:
                max_voltage = utils.read_int_from_file(self.psu_voltage_max, log_func=logger.log_info)
                max_voltage = InvalidPsuVolWA.run(self, max_voltage, self.psu_voltage_max)
                if max_voltage:
                    return float(max_voltage) / 1000

        return None

    @utils.default_return(None)
    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output

        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1

        Notes:
            The thresholds of voltage are not supported on all platforms.
            So we have to check capability first.
        """
        if self.psu_voltage_capability and self.psu_voltage_min and self.get_powergood_status():
            capability = utils.read_str_from_file(self.psu_voltage_capability)
            if 'min' in capability:
                min_voltage = utils.read_int_from_file(self.psu_voltage_min, log_func=logger.log_info)
                min_voltage = InvalidPsuVolWA.run(self, min_voltage, self.psu_voltage_min)
                if min_voltage:
                    return float(min_voltage) / 1000

        return None

    def get_maximum_supplied_power(self):
        """
        Retrieves the maximum supplied power by PSU

        Returns:
            A float number, the maximum power output in Watts.
            e.g. 1200.1
        """
        if self.psu_power_max and self.get_powergood_status():
            power_max = utils.read_int_from_file(self.psu_power_max, log_func=logger.log_info)
            return float(power_max) / 1000000
        else:
            return None

    def get_input_voltage(self):
        """
        Retrieves current PSU voltage input

        Returns:
            A float number, the input voltage in volts,
            e.g. 12.1
        """
        if self.get_powergood_status():
            voltage = utils.read_int_from_file(self.psu_voltage_in, log_func=logger.log_info)
            return float(voltage) / 1000
        return None

    def get_input_current(self):
        """
        Retrieves the input current draw of the power supply

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        if self.get_powergood_status():
            amperes = utils.read_int_from_file(self.psu_current_in, log_func=logger.log_info)
            return float(amperes) / 1000
        return None

    def get_psu_power_warning_suppress_threshold(self):
        """
        Retrieve the warning suppress threshold of the power on this PSU
        The value can be volatile, so the caller should call the API each time it is used.
        On Mellanox platform, it is translated from the `warning threshold`

        The formula to calculate power warning threshold for a PSU
            amb_temp = min(port_amb, fan_amb)
            If amb_temp < ambient_temp_warning_threshold
                threshold = max capacity - slope
            else
                threshold = max capacity - slope * (1 + amb_temp - ambient_temp_threshold)

        Returns:
            A float number, the warning suppress threshold of the PSU in watts.
        """
        if self.get_powergood_status():
            if os.path.exists(self.psu_power_max_capacity):
                power_max_capacity = utils.read_int_from_file(self.psu_power_max_capacity)
                temp_warning_threshold = utils.read_int_from_file(Psu.AMBIENT_TEMP_WARNING_THRESHOLD)
                fan_ambient_temp = utils.read_int_from_file(Psu.FAN_AMBIENT_TEMP)
                port_ambient_temp = utils.read_int_from_file(Psu.PORT_AMBIENT_TEMP)
                ambient_temp = min(fan_ambient_temp, port_ambient_temp)
                slope = utils.read_int_from_file(self.psu_power_slope) * 1000
                if ambient_temp < temp_warning_threshold:
                    power_warning_threshold = power_max_capacity - slope * 1000
                else:
                    power_warning_threshold = power_max_capacity - (1000 + ambient_temp - temp_warning_threshold) * slope
                if power_warning_threshold <= 0:
                    logger.log_warning('Got negative PSU power threshold {} for {}'.format(power_warning_threshold, self.get_name()))
                    power_warning_threshold = 0
                return float(power_warning_threshold) / 1000000

        return None

    def get_psu_power_critical_threshold(self):
        """
        Retrieve the critical threshold of the power on this PSU
        The value can be volatile, so the caller should call the API each time it is used.

        The formula to calculate power critical threshold for a PSU
            amb_temp = min(port_amb, fan_amb)
            If amb_temp < ambient_temp_critical_threshold
                threshold = max capacity
            else
                threshold = max capacity - slope*(amb_temp - ambient_temp_critical_threshold)

        Returns:
            A float number, the critical threshold of the PSU in watts.
        """
        if self.get_powergood_status():
            if os.path.exists(self.psu_power_max_capacity):
                power_max_capacity = utils.read_int_from_file(self.psu_power_max_capacity)
                temp_critical_threshold = utils.read_int_from_file(Psu.AMBIENT_TEMP_CRITICAL_THRESHOLD)
                fan_ambient_temp = utils.read_int_from_file(Psu.FAN_AMBIENT_TEMP)
                port_ambient_temp = utils.read_int_from_file(Psu.PORT_AMBIENT_TEMP)
                ambient_temp = min(fan_ambient_temp, port_ambient_temp)
                if ambient_temp < temp_critical_threshold:
                    power_critical_threshold = power_max_capacity
                else:
                    slope = utils.read_int_from_file(self.psu_power_slope) * 1000
                    power_critical_threshold = power_max_capacity - (ambient_temp - temp_critical_threshold) * slope
                if power_critical_threshold <= 0:
                    logger.log_warning('Got negative PSU power threshold {} for {}'.format(power_critical_threshold, self.get_name()))
                    power_critical_threshold = 0
                return float(power_critical_threshold) / 1000000

        return None


class InvalidPsuVolWA:
    """This class is created as a workaround for a known hardware issue that the PSU voltage threshold could be a
       invalid value 127998. Once we read a voltage threshold value equal to 127998, we should do following:
           1. Check the PSU vendor, it should be Delta
           2. Generate a temp sensor configuration file which contains a few set commands. Those set commands are the WA provided by low level team.
           3. Call "sensors -s -c <tmp_conf_file>"
           4. Wait for it to take effect

        This issue is found on 3700, 3700c, 3800, 4600c
    """

    INVALID_VOLTAGE_VALUE = 127998
    EXPECT_VENDOR_NAME = 'DELTA'
    EXPECT_CAPACITY = '1100'
    EXPECT_PLATFORMS = ['x86_64-mlnx_msn3700-r0', 'x86_64-mlnx_msn3700c-r0', 'x86_64-mlnx_msn3800-r0', 'x86_64-mlnx_msn4600c-r0']
    MFR_FIELD = 'MFR_NAME'
    CAPACITY_FIELD = 'CAPACITY'
    WAIT_TIME = 1

    @classmethod
    def run(cls, psu, threshold_value, threshold_file):
        if threshold_value != cls.INVALID_VOLTAGE_VALUE:
            # If the threshold value is not an invalid value, just return
            return threshold_value

        platform_name = DeviceDataManager.get_platform_name()
        # Apply the WA to specified platforms
        if platform_name not in cls.EXPECT_PLATFORMS:
            # It is unlikely to go to this branch, so we log a warning here
            logger.log_warning('PSU {} threshold file {} value {}, but platform is {}'.format(psu.index, threshold_file, threshold_value, platform_name))
            return threshold_value

        # Check PSU vendor, make sure it is DELTA
        vendor_name = psu.vpd_parser.get_entry_value(cls.MFR_FIELD)
        if vendor_name != 'N/A' and vendor_name != cls.EXPECT_VENDOR_NAME:
            # It is unlikely to go to this branch, so we log a warning here
            logger.log_warning('PSU {} threshold file {} value {}, but its vendor is {}'.format(psu.index, threshold_file, threshold_value, vendor_name))
            return threshold_value

        # Check PSU version, make sure it is 1100
        capacity = psu.vpd_parser.get_entry_value(cls.CAPACITY_FIELD)
        if capacity != 'N/A' and capacity != cls.EXPECT_CAPACITY:
            logger.log_warning('PSU {} threshold file {} value {}, but its capacity is {}'.format(psu.index, threshold_file, threshold_value, capacity))
            return threshold_value

        # Run a sensors -s command to triger hardware to get the real threashold value
        utils.run_command(['sensors', '-s'])

        # Wait for the threshold value change
        return cls.wait_set_done(threshold_file)

    @classmethod
    def wait_set_done(cls, threshold_file):
        wait_time = cls.WAIT_TIME
        while wait_time > 0:
            value = utils.read_int_from_file(threshold_file, log_func=logger.log_info)
            if value != cls.INVALID_VOLTAGE_VALUE:
                return value

            wait_time -= 1
            time.sleep(1)

        # It is enough to use warning here because user might power off/on the PSU which may cause threshold_file
        # does not exist
        logger.log_warning('sensors -s does not recover PSU threshold sensor after {} seconds'.format(cls.WAIT_TIME))
        return None
