#!/usr/bin/env python
"""
Version: 1.0

H3C B40X0
Module contains an implementation of SONiC Platform Base API and
provides the Fans' information which are available in the platform
"""
import fcntl
import os
import time

try:
    from sonic_platform_base.fan_base import FanBase, MotorBase
    from vendor_sonic_platform.devcfg import Devcfg
except ImportError as import_error:
    raise ImportError(str(import_error) + "- required module not found")

STATUS_LED_COLOR_GREEN = "green"
STATUS_LED_COLOR_RED = "red"
STATUS_LED_COLOR_OFF = "off"


FAN_LIST = range(0, Devcfg.FAN_NUM)

PRESENCE_CHANGE_DELAY = 30


class Motor(MotorBase):
    def __init__(self, index, parent=None):
        super(Motor, self).__init__(index, parent)
        self.parent = parent
        if parent.is_psu_fan:
            path = Devcfg.PSU_SUB_PATH.format(parent.parent.index + 1)
            self.attrs = {
                'ratio': 'N/A',
                'speed': path + 'fan_speed',
                'vendor': path + 'vendor_name_id',
                'speed_tolerance': '20',
                'presence': path + 'status',
            }
            self.psu_vendor = ''
            self.psu_vendor_temp = ''
            self.update_psufan()
        else:
            path = Devcfg.FAN_SUB_PATH.format(parent.index + 1) + 'motor{}/'.format(index)
            self.attrs = {
                'ratio': path + 'ratio',
                'speed': path + 'speed',
                'speed_tolerance': '20',
            }

    def get_max_speed(self):
        if not self.parent.get_presence():
            return 'N/A'
        vendor = self.parent.get_vendor()
        if vendor not in Devcfg.FAN_MOTOR_MAX_SPEED_DICT:
            max_speed = Devcfg.FAN_MOTOR_MAX_SPEED_DICT['DEFAULT'][self.index]
        else:
            max_speed = Devcfg.FAN_MOTOR_MAX_SPEED_DICT.get(vendor)[self.index]
        return max_speed

    def get_psu_vendor(self):
        if not self.parent.is_psu_fan:
            return 'N/A'
        if self.psu_vendor and self.psu_vendor == self.psu_vendor_temp:
            return self.psu_vendor

        status = 0
        try:
            path = self.attrs.get('presence')
            status = int(self.parent.read_attr(path))
        except ValueError as err:
            status = 0

        if not status:
            psu_vendor_name = 'N/A'
        else:
            try:
                path = self.attrs.get('vendor')
                psu_vendor_name = self.parent.read_attr(path)
            except ValueError as err:
                psu_vendor_name = 'N/A'

        if self.psu_vendor_temp and self.psu_vendor_temp == psu_vendor_name:
            self.psu_vendor = psu_vendor_name

        self.psu_vendor_temp = psu_vendor_name

        return psu_vendor_name

    def update_psufan(self):
        vendor = self.get_psu_vendor()
        if 'FSP' in vendor or 'DELTA' in vendor:
            self.psufan_max_value = 31000
        elif 'Great Wall' in vendor or 'GWPST' in vendor:
            self.psufan_max_value = 26000
        else:
            self.psufan_max_value = 28500

    def get_ratio(self):
        if not self.parent.get_presence():
            return 'N/A'
        if self.parent.is_psu_fan:
            value = self.get_speed()
            if value == 'N/A':
                return 'N/A'
            ratio = int(value * 100 / self.psufan_max_value)
            if ratio > 100:
                ratio = 100
            return ratio
        attr = self.attrs.get('ratio')
        if os.path.exists(attr):
            read_value = self.parent.read_attr(attr)
            if read_value == 'N/A':
                return read_value
            ratio = int(read_value)
            if ratio > 100:
                ratio = 100
            return ratio
        else:
            return 'N/A'

    def get_speed_tolerance(self):
        attr = self.attrs.get('speed_tolerance')
        return attr

    def get_speed(self):
        if not self.parent.get_presence():
            return 'N/A'
        attr = self.attrs.get('speed')
        if os.path.exists(attr):
            value = self.parent.read_attr(attr)
        else:
            return 'N/A'

        if self.parent.is_psu_fan:
            value = float(value)
        return int(value)

    def get_target_speed(self):
        if not self.parent.get_presence():
            return 'N/A'
        if self.parent.is_psu_fan:
            return self.get_speed()
        try:
            target_pwm = int(self.parent.get_pwm())
        except ValueError as err:
            self.log_error(str(err))
            return 'N/A'
        max_speed = self.get_max_speed()
        target_speed = target_pwm * int(max_speed) / 100 \
            if max_speed != 'N/A' else 'N/A'

        return target_speed


class Fan(FanBase):
    """
    Platform-specific Fan class
    """
    led_status_value = {
        STATUS_LED_COLOR_OFF: '0',
        STATUS_LED_COLOR_GREEN: '1',
        STATUS_LED_COLOR_RED: '3'
    }
    path = ''

    def __init__(self, index, parent=None, num_motors=2):
        FanBase.__init__(self, index, parent, num_motors)
        if self.is_psu_fan:
            self.path = Devcfg.PSU_SUB_PATH.format(index + 1)
        else:
            self.path = Devcfg.FAN_SUB_PATH.format(index + 1)
        self.old_status = self.STATUS_OK
        self.vendor = ''
        self.tmp_vendor = ''

        self.curve_pwm_max = Devcfg.PWM_REG_MAX
        self.curve_pwm_min = Devcfg.PWM_REG_MIN
        self.curve_speed_max = Devcfg.SPEED_TARGET_MAX
        self.curve_speed_min = Devcfg.SPEED_TARGET_MIN

    def motor_init(self, num_motors=2):
        """
        motors init
        """
        return [Motor(index, self) for index in range(num_motors)]

    def get_direction(self):
        """
        Retrieves the direction of fan
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        if not self.get_presence():
            return 'N/A'
        if self.is_psu_fan:
            return self.FAN_DIRECTION_R2F

        attr_file = 'motor0/direction'
        file_path = self.path + attr_file

        value = self.read_attr(file_path)
        if value != 'N/A':
            direction = int(value)
        else:
            return False

        if direction == 1:
            temp = self.FAN_DIRECTION_R2F
        elif direction == 0:
            temp = self.FAN_DIRECTION_F2R
        else:
            return False

        return temp

    def _get_file_path_status(self, main_dir, sub_dir):
        path_status_dir = os.path.join(main_dir, sub_dir)
        if os.path.exists(path_status_dir):
            fan_status = self.read_attr(path_status_dir)
            return fan_status
        else:
            return 'N/A'

    def _get_all_fan_normal(self):
        normal_list = []

        for index in FAN_LIST:
            fan_presence_dir = Devcfg.FAN_SUB_PATH.format(index + 1)
            status = self._get_file_path_status(fan_presence_dir, "status")
            if status == 'N/A':
                continue
            try:
                status_int = int(status)
            except ValueError as err:
                self.log_error(err)
                status_int = Devcfg.STATUS_ABSENT
            if status_int == Devcfg.STATUS_NORMAL:
                normal_list.append(index)

        return normal_list

    def _get_fan_normalnum(self):
        return len(self._get_all_fan_normal())

    def set_pwm(self, speed):
        """
        Sets the fan speed
        Args:
            speed: An integer, the percentage of full fan speed to set fan to,
                   in the range 0 (off) to 100 (full speed)
        Returns:
            A boolean, True if speed is set successfully, False if not

        Note:
            Depends on pwm or target mode is selected:
            1) pwm = speed_pc * 255             <-- Currently use this mode.
            2) target_pwm = speed_pc * 100 / 255
             2.1) set pwm{}_enable to 3
        """
        if not self.get_presence():
            return False

        if self.is_psu_fan:
            return False

        fan_speed_pwm = ((speed - self.curve_speed_min) *
                         (self.curve_pwm_max - self.curve_pwm_min) / (self.curve_speed_max - self.curve_speed_min)) \
            + self.curve_pwm_min

        value = self.write_attr(Devcfg.SPEED_PWM_FILE, str(fan_speed_pwm))
        time.sleep(0.01)
        return value

    def set_status_led(self, color):
        """
        Sets the state of the fan module status LED
        Args:
            color: A string representing the color with which to set the
                   fan module status LED
        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        if not self.get_presence():
            return 'N/A'

        if self.is_psu_fan:
            return False

        attr_file = 'led_status'

        if color in self.led_status_value.keys():
            if not self.write_attr(self.path + attr_file, self.led_status_value[color]):
                return False
        else:
            self.log_error("Error:Not Support Color={}!".format(color))
            return False

        return True

    def get_status_led(self):
        """
        Gets the state of the fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        if not self.get_presence():
            return 'N/A'

        if self.is_psu_fan:
            return 'N/A'

        attr_path = self.path + 'led_status'

        try:
            led_status = int(self.read_attr(attr_path))
        except (OSError, ValueError) as err:
            self.log_error(err)
            return 'N/A'

        for key, value in self.led_status_value.items():
            if str(led_status) == value:
                return key

        return 'N/A'

    def _get_speed_pwm_last_modify_time(self):

        try:
            last_modify_time = os.stat(Devcfg.SPEED_PWM_FILE).st_mtime
            return last_modify_time
        except (OSError, ValueError, IOError) as err:
            self.log_error(str(err))
            return time.time()

    def get_presence(self):
        """
        Retrieves the presence of the fan
        Returns:
            bool: True if fan is present, False if not
        """
        if self.is_psu_fan:
            value = self.parent.get_presence()
            if value == False:
                for motor in self.motor_list:
                    motor.psu_vendor = ''
                    motor.psu_vendor_temp = ''
            else:
                ##psu motor0 and motor1 always same
                if (self.motor_list[0].psu_vendor == ''):
                    for motor in self.motor_list:
                        motor.update_psufan()
            return value

        attr_file = 'status'
        attr_path = self.path + attr_file
        status = 0
        try:
            if os.path.exists(attr_path):
                status = int(self.read_attr(attr_path))
        except (OSError, ValueError) as err:
            self.log_error(err)
        if not status:
            self.vendor = ''
            self.tmp_vendor = ''

        return status != 0

    def get_status(self):
        """
        Retrieves status string of the fan
            STATUS_OK
            STATUS_SPEED_LOW
            STATUS_SPEED_HIGH
        Returns:
            eg: return self.STATUS_SPEED_LOW
        """
        if self.is_psu_fan:
            if self.parent.get_presence():
                status = self.parent.get_status()
                if self.parent.STATUS_FAN_ERR in status:
                    return self.STATUS_SPEED_LOW
                return self.STATUS_OK
            return self.STATUS_REMOVED

        if not self.get_presence():
            self.old_status = self.STATUS_REMOVED
            return self.STATUS_REMOVED

        with open(Devcfg.SPEED_PWM_FILE, 'r') as f:
            fcntl.flock(f.fileno(), fcntl.LOCK_SH)
            target_speed_list = [motor.get_target_speed() for motor in self.motor_list]

            if abs(time.time() - self._get_speed_pwm_last_modify_time()) > 5:
                for index, motor in enumerate(self.motor_list):
                    try:
                        target_speed = int(target_speed_list[index])
                        now_speed = int(motor.get_speed())
                        speed_tolerance = int(motor.get_speed_tolerance())
                        time_status = time.strftime('%Y-%m-%d %H:%M:%S %Z', time.localtime())
                    except (ValueError) as e:
                        self.log_error('{}'.format(e))
                        return self.STATUS_INSERTED

                    if abs(target_speed - now_speed) > target_speed * speed_tolerance / 100:
                        self.log_notice('{} motor{}, time:{}, target_speed:{}, now_speed:{}'.format(
                            self.get_name(), index, time_status, target_speed, now_speed))
                        if target_speed > now_speed:
                            self.old_status = self.STATUS_SPEED_LOW
                            return self.STATUS_SPEED_LOW
                        self.old_status = self.STATUS_SPEED_HIGH
                        return self.STATUS_SPEED_HIGH
                self.old_status = self.STATUS_OK
            else:
                if self.old_status == self.STATUS_REMOVED:
                    self.old_status = self.STATUS_OK

        return self.old_status

    def get_vendor(self):
        """
        Retrieves the vendor name of the fan
        Returns:
            string: Vendor name of fan
        """
        if not self.get_presence():
            return 'N/A'

        if self.is_psu_fan:
            return 'N/A'
        if self.vendor and self.vendor == self.tmp_vendor:
            return self.vendor

        attr_file = 'vendor'
        attr_path = self.path + attr_file
        fan_vendor_name = self.read_attr(attr_path)
        if self.tmp_vendor and self.tmp_vendor == fan_vendor_name:
            self.vendor = fan_vendor_name

        self.tmp_vendor = fan_vendor_name

        return fan_vendor_name

    def get_hw_version(self):
        """
        Get the hardware version of the fan
        Returns:
            A string
        """
        if not self.get_presence():
            return 'N/A'

        if self.is_psu_fan:
            return 'N/A'

        attr_file = 'hardware_version'
        attr_path = self.path + attr_file
        fan_hw_version = self.read_attr(attr_path)
        return fan_hw_version

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        if not self.get_presence():
            return 'N/A'

        if self.is_psu_fan:
            return 'N/A'

        attr_file = 'serial_number'
        attr_path = self.path + attr_file
        fan_sn = self.read_attr(attr_path)
        return fan_sn

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        if not self.get_presence():
            return 'N/A'

        if self.is_psu_fan:
            return 'N/A'

        attr_file = 'model_name'
        attr_path = self.path + attr_file
        fan_product_name = self.read_attr(attr_path)
        return fan_product_name

    def get_pwm(self):
        """
        Retrieves the pwm setting of the fan

        Returns:
            int -- (0-100) unit(%)
        """
        if not self.get_presence():
            return 'N/A'

        if self.is_psu_fan:
            return self.motor_list[0].get_ratio()

        read_pwm = self.read_attr(Devcfg.SPEED_PWM_FILE)
        try:
            target_pwm = (int(read_pwm) - self.curve_pwm_min) * \
                         (self.curve_speed_max - self.curve_speed_min) \
                / (self.curve_pwm_max - self.curve_pwm_min) + self.curve_speed_min
        except (ValueError, TypeError, OSError, IOError) as err:
            self.log_error(err)
            return 'N/A'
        return target_pwm

    def get_inventory(self):
        """
        Retrieves the inventory info for the fan
            eeprom in fru or tlv format can read all inventory info at one time

        Returns:
            dict: inventory info for a fan
        """
        inventory_dict = {
            'hw_version': self.get_hw_version(),
            'manufacture': self.get_vendor(),
            'model': self.get_model(),
            'serial': self.get_serial(),
            'vendor': self.get_vendor(),
        }

        return inventory_dict
