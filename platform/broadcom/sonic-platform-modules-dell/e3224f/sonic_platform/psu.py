#!/usr/bin/env python

########################################################################
# DellEMC E3224F
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs' information which are available in the platform
#
########################################################################

try:
    import os
    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.fan import Fan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Psu(PsuBase):
    """DellEMC Platform-specific PSU class"""

    def __init__(self, psu_index):
        PsuBase.__init__(self)
        self.index = psu_index + 1 # PSU is 1-based in DellEMC platforms
        self.psu_presence_reg = "psu{}_prs".format(psu_index)
        self.psu_status = "psu{}_status".format(psu_index)
        self.eeprom = "/sys/bus/i2c/devices/{}-0056/eeprom".format(10+psu_index)
        self.psu_voltage_reg = 'in3_input'
        self.psu_current_reg = 'curr2_input'
        self.psu_power_reg = 'power2_input'
        self.dps_hwmon = "/sys/bus/i2c/devices/{}-005e/hwmon/".format(10 + psu_index)
        self.dps_hwmon_exist = os.path.exists(self.dps_hwmon)
        self._fan_list.append(Fan(fan_index=self.index, psu_fan=True, dependency=self))

    def _get_cpld_register(self, reg_name):
        # On successful read, returns the value read from given
        # reg name and on failure rethrns 'ERR'
        cpld_dir = "/sys/devices/platform/dell-e3224f-cpld.0/"
        cpld_reg_file = cpld_dir + '/' + reg_name
        try:
            with open(cpld_reg_file, 'r') as fd:
                rv = fd.read()
        except IOError : return 'ERR'
        return rv.strip('\r\n').lstrip(' ')

    def _get_dps_register(self, reg_name):
        try :
            dps_dir = self.dps_hwmon + '/' + os.listdir(self.dps_hwmon)[0]
            dps_reg_file = dps_dir + '/' + reg_name
            with open(dps_reg_file, 'r') as fd:
                rv = fd.read()
        except (IOError, OSError) : return 'ERR'
        return rv

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return "PSU{}".format(self.index)

    def _reload_dps_module(self):
        try:
            file = "/sys/bus/i2c/devices/i2c-{}/delete_device".format(10 + self.index - 1)
            with open(file, 'w') as f:
                f.write('0x56\n')
        except (IOError, OSError):
            pass
        try:
            file = "/sys/bus/i2c/devices/i2c-{}/delete_device".format(10 + self.index - 1)
            with open(file, 'w') as f:
                f.write('0x5e\n')
        except (IOError, OSError):
            pass
        try:
            file = "/sys/bus/i2c/devices/i2c-{}/new_device".format(10 + self.index - 1)
            with open(file, 'w') as f:
                f.write('24c02 0x56\n')
            file = "/sys/bus/i2c/devices/i2c-{}/new_device".format(10 + self.index - 1)
            with open(file, 'w') as f:
                f.write('dps460 0x5e\n')
        except (IOError, OSError):
            pass

    def get_presence(self):
        """
        Retrieves the presence of the Power Supply Unit (PSU)

        Returns:
            bool: True if PSU is present, False if not
        """
        presence = self._get_cpld_register(self.psu_presence_reg).strip()
        if presence == 'ERR':
            return False

        status = self.get_status()
        if int(presence, 0) and status == False:
            return int(presence, 0)

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
        try: val = open(self.eeprom, "rb").read()[0x50:0x62]
        except Exception:
            val = None
        return val.decode('ascii')

    def get_serial(self):
        """
        Retrieves the serial number of the PSU

        Returns:
            string: Serial number of PSU
        """
        try: val = open(self.eeprom, "rb").read()[0xc4:0xd9]
        except Exception:
            val = None
        return val.decode('ascii')

    def get_revision(self):
        """
        Retrieves the serial number of the PSU

        Returns:
            string: Serial number of PSU
        """
        try: val = open(self.eeprom, "rb").read()[0xc4:0xd9]
        except Exception:
            val = None
        if val != "NA" and len(val) == 23:
            return val[-3:]
        else:
            return "NA"

    def get_status(self):
        """
        Retrieves the operational status of the PSU

        Returns:
            bool: True if PSU is operating properly, False if not
        """
        status = self._get_cpld_register(self.psu_status).strip()
        if status == 'ERR' : return False
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
        volt_reading = self._get_dps_register(self.psu_voltage_reg)
        try:
            voltage = int(volt_reading)/1000
        except Exception:
            return None
        return float(voltage)

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, electric current in amperes,
            e.g. 15.4
        """
        curr_reading = self._get_dps_register(self.psu_current_reg)
        try:
            current = int(curr_reading)/1000
        except Exception:
            return None
        return float(current)

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts,
            e.g. 302.6
        """
        power_reading = self._get_dps_register(self.psu_power_reg)
        try:
            power = int(power_reading)/(1000*1000)
        except Exception:
            return None
        return float(power)

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU

        Returns:
            A boolean, True if PSU has stablized its output voltages and
            passed all its internal self-tests, False if not.
        """
        power_good  = self._get_cpld_register(self.psu_status).strip()
        if power_good == 'ERR' : return False
        return int(power_good, 0)

    def get_mfr_id(self):
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
        try: val = open(self.eeprom, "rb").read()[0xe8:0xea]
        except Exception:
            return None
        return val.decode()
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

