#!/usr/bin/env python

########################################################################
# DellEMC Z9332F
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs' information which are available in the platform
#
########################################################################

try:
    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.ipmihelper import IpmiSensor, IpmiFru, get_ipmitool_raw_output
    from sonic_platform.fan import Fan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Psu(PsuBase):
    """DellEMC Platform-specific PSU class"""

    # { PSU-ID: { Sensor-Name: Sensor-ID } }
    SENSOR_MAPPING = { 1: { "State": 0x2f, "Current": 0x37,
                            "Power": 0x38, "Voltage": 0x36,
                            "Temperature": 0x35 },
                       2: { "State": 0x39, "Current": 0x41,
                            "Power": 0x42, "Voltage": 0x40,
                            "Temperature": 0x3F } }

    # ( PSU-ID: FRU-ID }
    FRU_MAPPING = { 1: 3, 2: 4 }

    def __init__(self, psu_index):
        PsuBase.__init__(self)
        # PSU is 1-based in DellEMC platforms
        self.index = psu_index + 1
        self.state_sensor = IpmiSensor(self.SENSOR_MAPPING[self.index]["State"],
                                       is_discrete=True)
        self.voltage_sensor = IpmiSensor(self.SENSOR_MAPPING[self.index]["Voltage"])
        self.current_sensor = IpmiSensor(self.SENSOR_MAPPING[self.index]["Current"])
        self.power_sensor = IpmiSensor(self.SENSOR_MAPPING[self.index]["Power"])
        self.temp_sensor = IpmiSensor(self.SENSOR_MAPPING[self.index ]["Temperature"])
        self.fru = IpmiFru(self.FRU_MAPPING[self.index])
        self.psu_type_raw_cmd = "0x3A 0x0B {}".format(psu_index+1)

        self._fan_list.append(Fan(fan_index=self.index, psu_fan=True, dependency=self))

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return "PSU{}".format(self.index)

    def get_presence(self):
        """
        Retrieves the presence of the Power Supply Unit (PSU)

        Returns:
            bool: True if PSU is present, False if not
        """
        presence = False
        is_valid, state = self.state_sensor.get_reading()
        if is_valid:
            if state & 0b1:
                presence = True

        return presence

    def get_model(self):
        """
        Retrieves the part number of the PSU

        Returns:
            string: Part number of PSU
        """
        return self.fru.get_board_part_number()

    def get_serial(self):
        """
        Retrieves the serial number of the PSU

        Returns:
            string: Serial number of PSU
        """
        return self.fru.get_board_serial()

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        serial = self.fru.get_board_serial()
        if serial != "NA" and len(serial) == 23:
            return serial[-3:]
        else:
            return "NA"

    def get_status(self):
        """
        Retrieves the operational status of the PSU

        Returns:
            bool: True if PSU is operating properly, False if not
        """
        status = False
        is_valid, state = self.state_sensor.get_reading()
        if is_valid:
            if (state & 0b1010) == 0:
                status = True

        return status

    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        is_valid, voltage = self.voltage_sensor.get_reading()
        if not is_valid:
            return None

        return float(voltage)

    def get_voltage_low_threshold(self):
        """
        Returns PSU low threshold in Volts
        """
        is_valid, low_threshold = self.voltage_sensor.get_threshold("LowerCritical")
        if not is_valid:
            low_threshold = 11.4
        low_threshold = "{:.2f}".format(low_threshold)

        return float(low_threshold)


    def get_voltage_high_threshold(self):
        """
        Returns PSU high threshold in Volts
        """
        is_valid, high_threshold = self.voltage_sensor.get_threshold("UpperCritical")
        if not is_valid:
            high_threshold = 12.6
        high_threshold = "{:.2f}".format(high_threshold)

        return float(high_threshold)

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A float number of current temperature in Celsius up to
            nearest thousandth of one degree Celsius, e.g. 30.125
        """
        is_valid, temperature = self.temp_sensor.get_reading()
        if not is_valid:
            temperature = 0

        return float(temperature)

    def get_temperature_high_threshold(self):
        """
        Returns the high temperature threshold for PSU in Celsius
        """
        is_valid, high_threshold = self.temp_sensor.get_threshold("UpperCritical")
        if not is_valid:
            high_threshold = 113
        high_threshold = "{:.2f}".format(high_threshold)

        return float(high_threshold)


    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, electric current in amperes,
            e.g. 15.4
        """
        is_valid, current = self.current_sensor.get_reading()
        if not is_valid:
            return None

        return float(current)

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts,
            e.g. 302.6
        """
        is_valid, power = self.power_sensor.get_reading()
        if not is_valid:
            return None

        return float(power)

    def get_maximum_supplied_power(self):
        """
        Retrieves the maximum supplied power by PSU

        Returns:
            A float number, the maximum power output in Watts.
            e.g. 1200.1
        """
        is_valid, power = self.power_sensor.get_threshold("UpperCritical")
        if not is_valid:
            return None

        return float(power)

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU

        Returns:
            A boolean, True if PSU has stablized its output voltages and
            passed all its internal self-tests, False if not.
        """
        status = False
        is_valid, state = self.state_sensor.get_reading()
        if is_valid:
            if (state & 0b1010) == 0:
                status = True

        return status

    def get_mfr_id(self):
        """
        Retrives the Manufacturer Id of PSU

        Returns:
            A string, the manunfacturer id.
        """
        return self.fru.get_board_mfr_id()

    def get_type(self):
        """
        Retrives the Power Type of PSU

        Returns :
            A string, PSU power type
        """
        psu_type = [None, 'AC', 'AC', 'DC']
        type_res = get_ipmitool_raw_output(self.psu_type_raw_cmd)
        if type_res is not None and len(type_res) == 1 :
            return psu_type[type_res[0]]
        return None

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.index

    def is_replaceable(self):
        """
        Indicate whether this PSU is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True
