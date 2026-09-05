"""
    DELL S3248T

    Module contains an implementation of SONiC Platform Base API and
    provides the PSUs' information which are available in the platform
"""

try:
    import os
    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.fan import Fan
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")


class Psu(PsuBase):
    """Dell Platform-specific PSU class"""

    def __init__(self, psu_index):
        PsuBase.__init__(self)
        self.index = psu_index + 1 # PSU is 1-based in Dell platforms
        self.psu_presence_reg = f"psu{psu_index}_prs"
        self.psu_status = f"psu{psu_index}_status"
        self.eeprom = f"/sys/bus/i2c/devices/{10 + psu_index}-0056/eeprom"
        self.psu_voltage_reg = 'in3_input'
        self.psu_current_reg = 'curr2_input'
        self.psu_power_reg = 'power2_input'
        self.dps_hwmon = f"/sys/bus/i2c/devices/{10 + psu_index}-005e/hwmon/"
        self.dps_hwmon_exist = os.path.exists(self.dps_hwmon)
        self._fan_list.append(Fan(fan_index=self.index, psu_fan=True, dependency=self))
        self.psu_input_voltage_reg = "in1_input"
        self.psu_power_cap_reg = "power2_cap"
        self.psu_input_current_reg = "curr1_input"
        self.psu_input_power_reg = "power1_input"
        self.psu_temp1_reg = 'temp1_input'
        self.psu_temp1_max_reg = 'temp1_max'

    @staticmethod
    def _get_cpld_register(reg_name):
        # On successful read, returns the value read from given
        # reg name and on failure rethrns 'ERR'
        cpld_dir = "/sys/devices/platform/dell-s3248t-cpld.0/"
        cpld_reg_file = cpld_dir + '/' + reg_name
        try:
            with open(cpld_reg_file, 'r') as file_desc:
                ret_val = file_desc.read()
        except IOError:
            return 'ERR'
        return ret_val.strip('\r\n').lstrip(' ')

    def _get_dps_register(self, reg_name):
        try:
            dps_dir = self.dps_hwmon + '/' + os.listdir(self.dps_hwmon)[0]
            dps_reg_file = dps_dir + '/' + reg_name
            with open(dps_reg_file, 'r') as file_desc:
                ret_val = file_desc.read()
        except (IOError, OSError):
            return 'ERR'
        return ret_val

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return "PSU{}".format(self.index)

    def _reload_dps_module(self):
        bus_id = 10 + self.index - 1
        drv_dir = f"/sys/bus/i2c/devices/i2c-{bus_id}/{bus_id}-0056"
        dps460_drv_dir = f"/sys/bus/i2c/devices/i2c-{bus_id}/{bus_id}-005e"
        try:
            if os.path.exists(drv_dir):
                file = "/sys/bus/i2c/devices/i2c-{}/delete_device".format(bus_id)
                with open(file, 'w') as file_des:
                    file_des.write('0x56\n')
            if not os.path.exists(drv_dir):
                file = "/sys/bus/i2c/devices/i2c-{}/new_device".format(bus_id)
                with open(file, 'w') as file_des:
                    file_des.write('24c02 0x56\n')
            if os.path.exists(dps460_drv_dir):
                file = "/sys/bus/i2c/devices/i2c-{}/delete_device".format(bus_id)
                with open(file, 'w') as file_des:
                    file_des.write('0x5e\n')
            if not os.path.exists(dps460_drv_dir):
                file = "/sys/bus/i2c/devices/i2c-{}/new_device".format(bus_id)
                with open(file, 'w') as file_des:
                    file_des.write('dps460 0x5e\n')
        except (IOError, OSError):
            pass

    def get_presence(self):
        """
        Retrieves the presence of the Power Supply Unit (PSU)

        Returns:
            bool: True if PSU is present, False if not
        """
        presence = self._get_cpld_register(self.psu_presence_reg).strip()
        status = self.get_status()
        if presence == 'ERR':
            return False
        if int(presence, 0) and not status:
            if int(presence, 0) == 1:
                return True
            return False
        if not self.dps_hwmon_exist and int(presence, 0):
            self.dps_hwmon_exist = os.path.exists(self.dps_hwmon)
            if not self.dps_hwmon_exist:
                self._reload_dps_module()
        if int(presence, 0) == 1:
            return True
        return False

    def get_model(self):
        """
        Retrieves the part number of the PSU

        Returns:
            string: Part number of PSU
        """
        try:
            with open(self.eeprom, "rb") as file_desc:
                val = file_desc.read()[0x50:0x62].decode("utf-8")
        except IOError:
            val = None
        return val

    def get_serial(self):
        """
        Retrieves the serial number of the PSU

        Returns:
            string: Serial number of PSU
        """
        try:
            with open(self.eeprom, "rb") as file_desc:
                val = file_desc.read()[0xc4:0xd9].decode("utf-8")
        except IOError:
            val = None
        return val

    def get_revision(self):
        """
        Retrieves the serial number of the PSU

        Returns:
            string: Revision number of PSU
        """
        try: 
            val = open(self.eeprom, "rb").read()[0xc4:0xd9]
        except IOError:
            val = None
        if val is not None and val != "NA" and len(val) == 23:
            return val[-3:]
        return "NA"

    def get_status(self):
        """
        Retrieves the operational status of the PSU

        Returns:
            bool: True if PSU is operating properly, False if not
        """
        status = self._get_cpld_register(self.psu_status).strip()
        if status == 'ERR':
            return False
        if int(status, 0) == 1:
            return True
        return False

    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        voltage = 0.0
        volt_reading = self._get_dps_register(self.psu_voltage_reg)
        try:
            voltage = int(volt_reading)/1000
        except (ValueError, ArithmeticError):
            return None
        return float(voltage)

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, electric current in amperes,
            e.g. 15.4
        """
        current = 0.0
        curr_reading = self._get_dps_register(self.psu_current_reg)
        try:
            current = int(curr_reading)/1000
        except (ValueError, ArithmeticError):
            return None
        return float(current)

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts,
            e.g. 302.6
        """
        power = 0.0
        power_reading = self._get_dps_register(self.psu_power_reg)
        try:
            power = int(power_reading)/(1000*1000)
        except (ValueError, ArithmeticError):
            return None
        return float(power)

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU

        Returns:
            A boolean, True if PSU has stablized its output voltages and
            passed all its internal self-tests, False if not.
        """
        power_good = self._get_cpld_register(self.psu_status).strip()
        if power_good == 'ERR':
            return False
        if int(power_good, 0) == 1:
            return True
        return False

    @staticmethod
    def get_mfr_id():
        """
        Retrives the Manufacturer Id of PSU

        Returns:
            A string, the manunfacturer id.
        """
        return 'DELTA'

    def get_type(self):
        """
        Retrives the Power Type of PSU

        Returns :
            A string, PSU power type
        """
        try:
            with open(self.eeprom, "rb") as file_desc:
                val = file_desc.read()[0xe8:0xea].decode("utf-8")
        except IOError:
            return None
        return val

    def get_input_voltage(self):
        """
        Retrieves PSU input voltage

        Returns:
            returns input voltage in volts,
        """
        voltage = 0
        volt_reading = self._get_dps_register(self.psu_input_voltage_reg)
        try:
            voltage = int(volt_reading)/1000
        except (ValueError, ArithmeticError):
            return None
        return f"{voltage:.1f}"

    def get_power_rating(self):
        """
        Retrieves PSU power rating

        Returns:
            returns power rating in watts
        """
        power = 0
        power_rating = self._get_dps_register(self.psu_power_cap_reg)
        try:
            power = int(power_rating)/(1000 * 1000)
        except (ValueError, ArithmeticError):
            return None
        return f"{power:.1f}"

    def get_input_current(self):
        """
        Retrieves present electric current supplied to PSU
        Returns:
            A float number, electric current in amperes,
            e.g. 15.4
        """
        input_current = 0.0
        curr_reading = self._get_dps_register(self.psu_input_current_reg)
        try:
            input_current = int(curr_reading) / 1000
        except (ValueError, ArithmeticError):
            return None
        return float(input_current)

    def get_input_power(self):
        """
        Retrieves current energy supplied to PSU
        Returns:
            A float number, the power in watts,
            e.g. 302.6
        """
        input_power = 0.0
        power_reading = self._get_dps_register(self.psu_input_power_reg)
        try:
            input_power = int(power_reading) / (1000*1000)
        except (ValueError, ArithmeticError):
            return None
        return float(input_power)

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal
        Returns:
            A float number of current temperature in Celcius up to
            nearest thousandth of one degree celcius, e.g. 30.125
        """
        temperature = 0
        temp_reading = self._get_dps_register(self.psu_temp1_reg)
        try:
            temperature = int(temp_reading) / 1000
        except (ValueError, ArithmeticError):
            return None
        return float(temperature)

    def get_temperature_high_threshold(self):
        """
        Retrieves the high threshold temperature of PSU

        Returns:
            A float number, the high threshold temperature of PSU in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        high_threshold = 0
        temp_reading = self._get_dps_register(self.psu_temp1_max_reg)
        try:
            high_threshold = int(temp_reading) / 1000
        except (ValueError, ArithmeticError):
            return None
        return float(high_threshold)

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
