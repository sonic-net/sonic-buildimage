"""
    Dell S3248T

    Module contains an implementation of SONiC Platform Base API and
    provides the Fans' information which are available in the platform.

"""
try:
    import os
    import time
    from sonic_platform_base.fan_base import FanBase
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")

FAN_DIR_EEPROM_OFFSET = 0x4a
MAX_FANTRAY_FAN_SPEED = 28600
MIN_FANTRAY_FAN_SPEED = 1920
MAX_PSU_FAN_SPEED = 18000
THERMAL_LEVEL_FAN_SPEED = (5720, 11440, 17160, 22880, 28600)
MAX_FAN_ON_RAMP_TIME = 90 # in seconds

class Fan(FanBase):
    """Dell Platform-specific Fan class"""

    def __init__(self, fantray_index=0, fan_index=0, psu_fan=False, dependency=None):
        self.is_psu_fan = psu_fan

        if not self.is_psu_fan:
            # API index is starting from 0, Dell platform index is
            # starting from 1
            self.presence_reg = f"fan{fantray_index}_prs"
            self.dir_reg = f"fan{fantray_index}_dir"
            self.rpm_file = f"/sys/bus/i2c/devices/7-002c/fan{fantray_index+1}_input"
            self.rpm_target_file = f"/sys/bus/i2c/devices/7-002c/fan{fantray_index+1}_target"
            self.status_file = f"/sys/bus/i2c/devices/7-002c/fan{fantray_index+1}_fault"
            self.eeprom = f"/sys/bus/i2c/devices/{15 + fantray_index}-0050/eeprom"
            self.fantray_index = fantray_index + 1
            self.fan_index = fan_index + 1
            self.max_speed = MAX_FANTRAY_FAN_SPEED
            self.thermal_level_to_speed = THERMAL_LEVEL_FAN_SPEED
            self.last_speed_set_time = 0
        else:
            self.psu_index = fan_index - 1
            self.presence_reg = f"psu{self.psu_index}_prs"
            self.status_reg = f"psu{self.psu_index}_status"
            self.dir_reg = ""
            self.dps_hwmon = f"/sys/bus/i2c/devices/{self.psu_index + 10}-005e/hwmon/"
            self.eeprom = f"/sys/bus/i2c/devices/{self.psu_index + 10}-0056/eeprom"
            self.max_speed = MAX_PSU_FAN_SPEED
            self.fan_index = fan_index + 1
        self.eeprom_exist = os.path.exists(self.eeprom)

    @staticmethod
    def _get_cpld_register(reg_name):
        # On successful read, returns the value read from given
        # reg name and on failure rethrns 'ERR'
        cpld_dir = "/sys/devices/platform/dell-s3248t-cpld.0/"
        cpld_reg_file = cpld_dir + '/' + reg_name
        try:
            with open(cpld_reg_file, 'r') as file_desc:
                buf = file_desc.read()
        except (IOError, AttributeError):
            return 'ERR'
        return buf.strip('\r\n').lstrip(' ')

    def load_fan_module(self):
        """
        Load fan driver module
        """
        if not self.is_psu_fan:
            drv_dir = "/sys/bus/i2c/devices/i2c-{0}/{0}-0050".format(15 + self.fantray_index - 1)
            try:
                if not os.path.exists(drv_dir):
                    file = "/sys/bus/i2c/devices/i2c-{}/new_device" \
                              .format(15 + self.fantray_index - 1)
                    with open(file, 'w') as file_des:
                        file_des.write('24c02 0x50\n')
                    self.set_speed_rpm(MAX_FANTRAY_FAN_SPEED)
            except (IOError, OSError):
                pass

    def remove_fan_module(self):
        """
        Remove fan driver module
        """
        if not self.is_psu_fan:
            drv_dir = "/sys/bus/i2c/devices/i2c-{0}/{0}-0050".format(15 + self.fantray_index - 1)
            try:
                if os.path.exists(drv_dir):
                    file = "/sys/bus/i2c/devices/i2c-{}/delete_device" \
                              .format(15 + self.fantray_index - 1)
                    with open(file, 'w') as file_des:
                        file_des.write('0x50\n')
            except (IOError, OSError):
                pass

    def get_name(self):
        """
        Retrieves the name of the device
        Returns:
            String: The name of the device
        """
        if self.is_psu_fan:
            return "PSU{} Fan".format(self.psu_index+1)
        return "FanTray{}-Fan{}".format(self.fantray_index, self.fan_index)

    def get_model(self):
        """
        Retrieves the part number of the FAN
        Returns:
            String: Part number of FAN
        """
        try:
            with open(self.eeprom, "rb") as file_desc:
                val = file_desc.read()[13:19].decode("utf-8")
        except IOError:
            val = None
        return val

    def get_serial(self):
        """
        Retrieves the serial number of the FAN
        Returns:
            String: Serial number of FAN
        """
        try:
            with open(self.eeprom, "rb") as file_desc:
                val = file_desc.read()[21:41].decode("utf-8")
        except IOError:
            val = None
        return val

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if fan is present, False if not
        """
        presence = self._get_cpld_register(self.presence_reg)
        if presence == 'ERR':
            return False
        status = self.get_status()
        if int(presence, 0):
            # Fan present but status not OK then remove fan driver module
            if not status:
                self.remove_fan_module()
                self.eeprom_exist = False
                return True
            # Fan present but fan driver module not exist, load fan driver
            if not self.eeprom_exist:
                self.load_fan_module()
                self.eeprom_exist = os.path.exists(self.eeprom)
            return True

        # Fan absent but fan driver module exist, remove fan driver
        if self.eeprom_exist:
            self.remove_fan_module()
            self.eeprom_exist = False
        return False

    def get_status(self):
        """
        Retrieves the operational status of the FAN
        Returns:
            bool: True if FAN is operating properly, False if not
        """
        if not self.is_psu_fan:
            with open(self.status_file, "rb") as file_desc:
                status = file_desc.read()
            if int(status, 0) == 1:
                return False
            return True

        status = self._get_cpld_register(self.status_reg)
        if status == 'ERR':
            return False
        return bool(int(status, 0))

    def get_direction(self):
        """
        Retrieves the fan airfow direction
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction

        Notes:
            In DellEMC platforms,
            - Forward/Exhaust : Air flows from Port side to Fan side.
            - Reverse/Intake  : Air flows from Fan side to Port side.
        """
        try:
            with open(self.eeprom, "rb") as filedes:
                if not self.is_psu_fan:
                    val = filedes.read()[FAN_DIR_EEPROM_OFFSET]
                    # EEPROM is programmed with ASCII values. ASCII '0' (Decimal-48)
                    # means IO/PSU (Exhaust) and '1' means to PSU/IO (Intake)
                    direction = 'exhaust' if val == ord('0') else 'intake'
                else:
                    val = filedes.read()[0xe1:0xe8].decode("utf-8")
                    direction = 'exhaust' if val == 'FORWARD' else 'intake'

                return direction
        except IOError:
            return None

    def get_speed(self):
        """
        Retrieves the speed of the fan
        Returns:
            int: percentage of the max fan speed
        """
        fan_speed = 0
        try:
            if not self.is_psu_fan:
                rpm_file = self.rpm_file
            else:
                dps_dir = self.dps_hwmon + '/' + os.listdir(self.dps_hwmon)[0]
                rpm_file = dps_dir + '/' + 'fan1_input'
            with open(rpm_file, "rb") as file_desc:
                fan_speed = int(file_desc.read())
        except IOError:
            return 0
        speed = (100 * fan_speed)//self.max_speed
        return speed

    def get_speed_rpm(self):
        """
        Retrieves the speed of the fan
        Returns:
            int: percentage of the max fan speed
        """
        fan_speed = 0
        try:
            fan_status = self.get_status()
            if fan_status:
                if not self.is_psu_fan:
                    rpm_file = self.rpm_file
                else:
                    dps_dir = self.dps_hwmon + '/' + os.listdir(self.dps_hwmon)[0]
                    rpm_file = dps_dir + '/' + 'fan1_input'
                with open(rpm_file, "rb") as file_desc:
                    fan_speed = int(file_desc.read())
        except IOError:
            return 0
        return fan_speed

    def set_speed_rpm(self, speed):
        """
        Set the speed of the fan
        Returns:
            bool: True in case of success. False otherwise
        """
        try:
            if not self.is_psu_fan:
                rpm_file = self.rpm_target_file
                with open(rpm_file, 'w') as fan_target:
                    fan_target.write(str(speed))
                self.last_speed_set_time = time.time()

        except IOError:
            return False
        return True

    @staticmethod
    def set_status_led(color):
        """
        S3248T switches doesn't have seperate LED for fan
        """
        return False

    def get_target_speed(self):
        """
        Retrieves the target speed of the fan
        Returns:
            int: percentage of the target fan speed
        """
        fan_speed = 0
        try:
            fan_status = self.get_status()
            if fan_status:
                if not self.is_psu_fan:
                    rpm_file = self.rpm_target_file
                    with open(rpm_file, "rb") as file_desc:
                        fan_speed = int(file_desc.read())
                else:
                    fan_speed = self.get_speed_rpm()
                    if fan_speed is None:
                        return 0
            else:
                # Setting fan_speed to 0 is causing false fan high-speed alarm
                # so set fan speed to minimum.
                fan_speed = MIN_FANTRAY_FAN_SPEED
        except IOError:
            pass

        fan_speed = (100 * fan_speed)//self.max_speed
        return fan_speed


    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan

        Returns:
            An integer, the percentage of variance from target speed
            which is considered tolerable
        """
        if self.get_presence():
            tolerance = 10
        else:
            tolerance = None

        return tolerance

    def _get_speed_to_percentage(self, speed):
        """
        Convert speed value to speed percentage based on max fan speed

        Args:
            speed - Fan speed

        Returns:
            Fan speed percentage
        """
        speed_percent = (100 * speed) // self.max_speed
        return speed_percent if speed_percent <= 100 else 100

    def set_speed_for_thermal_level(self, thermal_level):
        """
        Set the fan speed based on the current system thermal level

        Args:
            thermal_level - Current system thermal level

        Returns:
            True
        """
        req_speed_rpm = self.thermal_level_to_speed[thermal_level]
        req_speed = self._get_speed_to_percentage(req_speed_rpm)
        target_speed = self.get_target_speed()

        if req_speed != target_speed:
            self.set_speed_rpm(req_speed_rpm)

        return True

    def is_on_ramp(self):
        """
        Check whether the FAN is on ramp (up/down) after a speed set

        Returns:
            bool: True if FAN is on ramp, False otherwise
        """
        if not self.is_psu_fan:
            time_diff = time.time() - self.last_speed_set_time
            if time_diff < MAX_FAN_ON_RAMP_TIME:
                return True

        return False

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.

        Returns:
            integer: The 1-based relative physical position in parent
                     device or -1 if cannot determine the position
        """
        return self.fan_index

    def is_replaceable(self):
        """
        Indicate whether Fan is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

