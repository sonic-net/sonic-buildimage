#
# fanutil.py
# Platform-specific Fan status interface for SONiC
#

import logging
import os.path

try:
    from sonic_fan.fan_base import FanBase
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

class FanUtil(FanBase):
    """Platform-specific FANutil class"""

    SYS_FAN_NUM = 6
    NUM_FANS_PERTRAY = 2
    HWMON_PATH = '/sys/class/hwmon/hwmon1/'
    FAN_INDEX_START = 21

    logger = logging.getLogger(__name__)

    def __init__(self, log_level=logging.DEBUG):
        FanBase.__init__(self)
        self.num_fans = (self.SYS_FAN_NUM * self.NUM_FANS_PERTRAY) 

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

    def check_fan_index(self, index):
        if index is None:
            return False

        if index < 1 or index > self.num_fans:
            logging.error("Invalid Fan index:", index)
            return False

        return True

    def get_num_fans(self):
        return self.num_fans

    def get_status(self, index):
        if self.check_fan_index(index) == False:
           return False

        fantray_speed_file = 'fan' + str(self.FAN_INDEX_START+(index-1)) + '_input'
        fantray_speed = self.get_attr_value(self.HWMON_PATH + fantray_speed_file)

        if fantray_speed == '0.0' :
           return False

        return True

    def get_presence(self, index):
        if self.check_fan_index(index) == False:
           return False

        fantray_present_file = 'fan' + str(self.FAN_INDEX_START+(index-1)) + '_present'
        fantray_present = self.get_attr_value(self.HWMON_PATH + fantray_present_file)

        if fantray_present == '1' :
           return True

        return False

    def get_direction(self, index):
        if self.check_fan_index(index) == False:
           return None

        fantray_direction_file = 'fan' + str(self.FAN_INDEX_START+(index-1)) + '_direction'
        fantray_direction = self.get_attr_value(self.HWMON_PATH + fantray_direction_file)

        """
        1: FB 2: BF
        Since the fan is at rear of the switch, FB means Exhaust; BF means Intake
        """
        if fantray_direction == '2':
            return "INTAKE"
        else:
            return "EXHAUST"

    def get_speed(self, index):
        if self.check_fan_index(index) == False:
           return 0

        fantray_speed_file = 'fan' + str(self.FAN_INDEX_START+(index-1)) + '_input'
        fantray_speed = self.get_attr_value(self.HWMON_PATH + fantray_speed_file)

        return int(float(fantray_speed))


    def set_speed(self, val):
        logging.error("Not allowed to set fan speed!")

        return False
