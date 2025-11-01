#!/usr/bin/env python
"""
Version: 1.0

H3C B40X0
Module contains an implementation of SONiC Platform Base API and
provides the Psus' information which are available in the platform
"""

import os

try:
    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.fan import Fan
    from vendor_sonic_platform.devcfg import Devcfg
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Psu(PsuBase):
    """
    Platform-specific Psu class
    """

    def __init__(self, index, parent=None, num_fans=1):
        super(Psu, self).__init__(index, parent, num_fans)
        self.sysfs_psu_dir = Devcfg.PSU_SUB_PATH.format(self.index + 1)

    def fan_init(self, num_fans=1):
        return [Fan(index, self, num_motors=1) for index in range(num_fans)]

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU
        Returns:
            A boolean, True if PSU has stablized its output voltages and passed all
            its internal self-tests, False if not.
        """
        attr_file = 'status'
        attr_path = self.sysfs_psu_dir + attr_file
        status = 0
        if not self.get_presence():
            return False

        try:
            status = int(self.read_attr(attr_path))
        except ValueError as err:
            self.log_error(str(err))
            return False

        return status == 1

    def set_status_led(self, color):
        """
        Sets the state of the PSU status LED
        Args:
            color: A string representing the color with which to set the PSU status LED
                   Note: Only support green and off
        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        # Hardware not supported
        return False

    def get_status_led(self):
        """
        Get the state of the PSU status LED
        Returns:
            A string list, all of the predefined STATUS_LED_COLOR_* strings above
        """
        psu_led_status_list = []
        status = self.read_attr('led_status')
        if status == 0:
            psu_led_status_list.append(self.STATUS_LED_COLOR_OFF)
        elif status == 1:
            psu_led_status_list.append(self.STATUS_LED_COLOR_GREEN)
        elif status == 3:
            psu_led_status_list.append(self.STATUS_LED_COLOR_AMBER)
        else:
            psu_led_status_list.append("N/A")

        return psu_led_status_list

    def get_presence(self):
        """
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        attr_file = 'status'
        attr_path = self.sysfs_psu_dir + attr_file
        status = 0

        try:
            if os.path.exists(attr_path):
                status = int(self.read_attr(attr_path))
            else:
                return False
        except ValueError as err:
            self.log_error(str(err))
            return False

        return not status == 0

    def get_status(self):
        """
        Gets the status of the PSU
        Returns:
            A string list, all of the predefined STATUS_* strings above
            # Possible psu status
              STATUS_OK = 'ok'
              STATUS_POWER_LOS = 'power los'
              STATUS_FAN_ERR = 'fan error'
              STATUS_VOL_ERR = 'vol error'
              STATUS_CURRENT_ERR = 'current error'
              STATUS_POWER_ERR = 'power error'
              STATUS_TEMP_ERR = 'temperature error'
        """
        attr_file = 'status_word'
        attr_path = self.sysfs_psu_dir + attr_file
        psu_status_word = 0
        psu_status_list = []

        if not self.get_presence():
            psu_status_list.append(self.STATUS_REMOVED)
            return psu_status_list

        try:
            psu_status_word = int(self.read_attr(attr_path))
        except ValueError as err:
            self.log_error(str(err))
            return [self.STATUS_POWER_ERR]

        power_los_mask = (1 << 6)
        power_vol_mask = (1 << 5 | 1 << 3 | 1 << 15)
        power_current_mask = (1 << 4 | 1 << 14)
        power_temp_mask = (1 << 2)
        power_power_mask = (1 << 11)
        power_io_po_mask = (1 << 14)
        power_fan_mask = (1 << 10)

        if psu_status_word & power_fan_mask:
            psu_status_list.append(self.STATUS_FAN_ERR)
        if psu_status_word & power_los_mask:
            psu_status_list.append(self.STATUS_POWER_LOSS)
        else:
            if psu_status_word & power_vol_mask:
                psu_status_list.append(self.STATUS_VOL_ERR)
            if psu_status_word & power_current_mask:
                psu_status_list.append(self.STATUS_CURRENT_ERR)
            if psu_status_word & power_temp_mask:
                psu_status_list.append(self.STATUS_TEMP_ERR)
            if (psu_status_word & power_power_mask) or (psu_status_word & power_io_po_mask):
                psu_status_list.append(self.STATUS_POWER_ERR)

        if not psu_status_list:
            psu_status_list.append(self.STATUS_OK)

        return psu_status_list

    def get_voltage(self):
        """
        Retrieves current PSU voltage output
        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        attr_file = 'out_vol'
        attr_path = self.sysfs_psu_dir + attr_file
        psu_voltage = 0.0

        if not self.get_presence():
            return psu_voltage

        try:
            psu_voltage = float(self.read_attr(attr_path))
        except ValueError as err:
            self.log_error(str(err))
            return False

        return psu_voltage

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU
        Returns:
            A float number, electric current in amperes,
            e.g. 15.4
        """
        attr_file = 'out_curr'
        attr_path = self.sysfs_psu_dir + attr_file
        psu_current = 0.0

        if not self.get_presence():
            return psu_current

        try:
            psu_current = float(self.read_attr(attr_path))
        except ValueError as err:
            self.log_error(str(err))

        return psu_current

    def get_power(self):
        """
        Retrieves current energy supplied by PSU
        Returns:
            A float number, the power in watts,
            e.g. 302.6
        """
        attr_file = 'out_power'
        attr_path = self.sysfs_psu_dir + attr_file
        psu_power = 0.0

        if not self.get_presence():
            return psu_power

        try:
            psu_power = float(self.read_attr(attr_path))
        except ValueError as err:
            self.log_error(str(err))

        return psu_power

    def _is_ascii(self, string):
        for s in string:
            if ord(s) >= 128:
                return False
        return True

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        attr_file = 'serial_number'
        attr_path = self.sysfs_psu_dir + attr_file

        if not self.get_presence():
            return 'N/A'

        psu_sn = self.read_attr(attr_path)
        if not self._is_ascii(psu_sn) or psu_sn == '':
            return 'N/A'

        return psu_sn

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        attr_file = 'model_name'
        attr_path = self.sysfs_psu_dir + attr_file

        if not self.get_presence():
            return 'N/A'

        psu_product_name = self.read_attr(attr_path)
        if not self._is_ascii(psu_product_name) or psu_product_name == '':
            return 'N/A'

        return psu_product_name

    def get_input_voltage(self):
        """
        Get the input voltage of the PSU
        Returns:
            A float number, the input voltage in volts,
        """
        if not self.get_presence():
            return 0
        return self.read_attr(self.sysfs_psu_dir + 'in_vol')

    def get_input_current(self):
        """
        Get the input electric current of the PSU
        Returns:
            A float number, the input current in amperes, e.g 220.3
        """
        if not self.get_presence():
            return 0
        return self.read_attr(self.sysfs_psu_dir + 'in_curr')

    def get_input_power(self):
        """
        Get the input current energy of the PSU
        Returns:
            A float number, the input power in watts, e.g. 302.6
        """
        attr_file = 'in_power'
        attr_path = self.sysfs_psu_dir + attr_file
        psu_input_power = 0.0

        if not self.get_presence():
            return psu_input_power

        try:
            psu_input_power = float(self.read_attr(attr_path))
        except ValueError as err:
            self.log_error(str(err))

        return psu_input_power

    def get_temperature(self):
        """
        Get the temperature of the PSU
        Returns:
            A string, all the temperature with units of the psu. e.g. '32.5 C, 50.3 C, ...'
        """
        attr_file = 'temp0/temp_input'
        attr_path = self.sysfs_psu_dir + attr_file
        psu_temperature = 0.0

        if not self.get_presence():
            return psu_temperature

        psu_temperature = self.read_attr(attr_path)

        return psu_temperature

    def get_sw_version(self):
        """
        Get the firmware version of the PSU
        Returns:
            A string
        """
        attr_file = 'fw_version'
        attr_path = self.sysfs_psu_dir + attr_file

        if not self.get_presence():
            return 'N/A'

        psu_sw_version = self.read_attr(attr_path)
        if not self._is_ascii(psu_sw_version) or psu_sw_version == '':
            return 'N/A'

        return psu_sw_version

    def get_hw_version(self):
        """
        Get the hardware version of the PSU
        Returns:
            A string
        """
        attr_file = 'hardware_version'
        attr_path = self.sysfs_psu_dir + attr_file

        if not self.get_presence():
            return 'N/A'

        psu_hw_version = self.read_attr(attr_path)
        if not self._is_ascii(psu_hw_version) or psu_hw_version == '':
            return 'N/A'

        return psu_hw_version

    def get_vol_type(self):
        attr_file = 'type'
        attr_path = self.sysfs_psu_dir + attr_file

        if not self.get_presence():
            return 'N/A'

        psu_vol_type = self.read_attr(attr_path)
        if psu_vol_type in ('AC', 'HVDC'):
            return psu_vol_type
        else:
            return 'N/A'

    def get_vendor(self):
        """
        Retrieves the vendor name of the psu
        Returns:
            string: Vendor name of psu
        """

        attr_file = 'vendor_name_id'
        attr_path = self.sysfs_psu_dir + attr_file

        if not self.get_presence():
            return 'N/A'

        psu_vendor_name = self.read_attr(attr_path)
        if 'GRE' in psu_vendor_name:
            psu_vendor_name = 'GRE'
        elif 'FSP' in psu_vendor_name or '3Y POWER' in psu_vendor_name:
            psu_vendor_name = 'FSP'
        elif 'DELTA' in psu_vendor_name:
            psu_vendor_name = 'DELTA'
        else:
            pass

        return psu_vendor_name

    def get_inventory(self):
        """
        Retrieves the inventory info for the psu
            eeprom in fru or tlv format can read all inventory info at one time

        Returns:
            dict: inventory info for a psu
        """
        inventory_dict = {
            'hw_version': self.get_hw_version(),
            'fw_version': self.get_sw_version(),
            'manufacture': self.get_vendor(),
            'model': self.get_model(),
            'serial': self.get_serial(),
            'type': self.get_vol_type(),
            'vendor': self.get_vendor()
        }

        return inventory_dict
