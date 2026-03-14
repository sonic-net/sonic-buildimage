#
# psuutil.py
# Platform-specific PSU status interface for SONiC
#

import logging
import os.path

try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    HWMON_PATH = '/sys/class/hwmon/hwmon1/'
    PSU1_PREFIX = 'power42_'
    PSU2_PREFIX = 'power52_'
    MAX_PSUS = 2
    def __init__(self):
        PsuBase.__init__(self)

    # Get sysfs attribute
    def get_attr_value(self, attr_path):

        retval = 'ERR'
        if (not os.path.isfile(attr_path)):
            return retval

        try:
            with open(attr_path, 'r') as fd:
                retval = fd.read()
        except Exception:
            logging.error("Unable to open ", attr_path, " file !")

        retval = retval.rstrip('\r\n')
        return retval

    def get_attr_filename(self, index, attr):
        if (index == 1):
            attr_file = self.PSU1_PREFIX + attr
        elif (index == 2):
            attr_file = self.PSU2_PREFIX + attr
        else:
            logging.error("Invalid PSU number:", index)
            return ''

        return attr_file

    def get_num_psus(self):
        """
        Retrieves the number of PSUs available on the device
        :return: An integer, the number of PSUs available on the device
         """

        return self.MAX_PSUS

    def get_psu_status(self, index):
        """
        Retrieves the oprational status of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: Boolean, True if PSU is operating properly, False if PSU is\
        faulty
        """
        status = False

        attr_filename = self.get_attr_filename(index, 'input')
        if attr_filename == '':
            return status

        attr_path = self.HWMON_PATH + attr_filename
        attr_value = self.get_attr_value(attr_path)

        if (attr_value != 'ERR'):
            attr_value = float(attr_value)

            # Check PSU status
            if (attr_value != 0.0):
                status = True

        return status

    def get_psu_presence(self, index):
        """
        Retrieves the presence status of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: Boolean, True if PSU is plugged, False if not
        """
        status = False

        attr_filename = self.get_attr_filename(index, 'present')
        if attr_filename == '':
           return status

        attr_path = self.HWMON_PATH + attr_filename
        attr_value = self.get_attr_value(attr_path)

        if (attr_value != 'ERR'):
            attr_value = int(attr_value, 16)
            # Check PSU status
            if (attr_value == 1):
                status = True
        return status

    def get_powergood_status(self, index):
        status = False

        attr_filename = self.get_attr_filename(index, 'input')
        if attr_filename == '':
           return status

        attr_path = self.HWMON_PATH + attr_filename
        attr_value = self.get_attr_value(attr_path)

        if (attr_value != 'ERR'):
            attr_value = float(attr_value)

            # Check PSU status
            if (attr_value != 0.0):
                status = True

        return status
    
    def get_model(self, index):
        attr_filename = self.get_attr_filename(index, 'model')
        if attr_filename == '':
           return None

        attr_path = self.HWMON_PATH + attr_filename
        attr_value = self.get_attr_value(attr_path)

        if (attr_value != 'ERR'):
            return attr_value.rstrip()

    def get_mfr_id(self, index):
        attr_filename = self.get_attr_filename(index, 'mfrid')
        if attr_filename == '':
           return None

        attr_path = self.HWMON_PATH + attr_filename
        attr_value = self.get_attr_value(attr_path)

        if (attr_value != 'ERR'):
            return attr_value.rstrip()

    def get_serial(self, index):
        attr_filename = self.get_attr_filename(index, 'sn')
        if attr_filename == '':
           return None

        attr_path = self.HWMON_PATH + attr_filename
        attr_value = self.get_attr_value(attr_path)

        if (attr_value != 'ERR'):
            return attr_value.rstrip()

    def get_direction(self, index):
        if (index == 1):
            direction_file = 'fan40_direction'
        elif (index == 2):
            direction_file = 'fan50_direction'
        else:
            logging.error("Invalid PSU number:", index)
            return None

        direction = self.get_attr_value(self.HWMON_PATH + direction_file)
        direction = direction.rstrip()

        """
        1: FB 2: BF
        Since the fan is at rear of the switch, FB means Exhaust; BF means Intake
        """
        if direction == '2':
            return "INTAKE"
        else:
            return "EXHAUST"

    def get_output_voltage(self, index):
        if (index == 1):
            attr_file = 'in47_input'
        elif (index == 2):
            attr_file = 'in57_input'
        else:
            logging.error("Invalid PSU number:", index)
            return 0.0

        voltage = self.get_attr_value(self.HWMON_PATH + attr_file)
        voltage = voltage.rstrip()

        if (voltage != 'ERR'):
            voltage, dummy = voltage.split('.', 1)
        else:
            return 0.0

        return float(voltage)/1000

    def get_output_current(self, index):
        if (index == 1):
            attr_file = 'curr39_input'
        elif (index == 2):
            attr_file = 'curr49_input'
        else:
            logging.error("Invalid PSU number:", index)
            return 0.0

        current = self.get_attr_value(self.HWMON_PATH + attr_file)
        current = current.rstrip()

        if (current != 'ERR'):
            current, dummy = current.split('.',1)
        else:
            return 0.0

        return float(current)/1000

    def get_output_power(self, index):
        attr_filename = self.get_attr_filename(index, 'input')
        if attr_filename == '':
           return 0.0

        attr_path = self.HWMON_PATH + attr_filename
        attr_value = self.get_attr_value(attr_path)

        if (attr_value != 'ERR'):
            attr_value = float(attr_value)
        else:
            return 0.0

        return float(attr_value/1000)

    def get_fan_rpm(self, index, fan_idx):
        if (index == 1):
            rpm_file = 'fan40_input'
        elif (index == 2):
            rpm_file = 'fan50_input'
        else:
            logging.error("Invalid PSU number:", index)
            return 0

        rpm = self.get_attr_value(self.HWMON_PATH + rpm_file)
        rpm = rpm.rstrip()
        if (rpm != 'ERR'):
            rpm = float(rpm)
        else:
            return 0

        return int(rpm)
