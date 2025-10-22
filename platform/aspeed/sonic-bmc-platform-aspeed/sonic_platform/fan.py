"""
SONiC Platform API - Fan class for Aspeed BMC

This module provides the Fan class for Aspeed AST2700 BMC platform.
Fans are controlled via PWM and monitored via TACH inputs.
"""

import os

try:
    from sonic_platform_base.fan_base import FanBase
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


class Fan(FanBase):
    """
    Fan class for Aspeed AST2700 BMC platform
    
    The AST2700 supports:
    - 16 TACH inputs for fan speed monitoring (hwmon0)
    - 9 PWM outputs for fan speed control (hwmon1-9)
    """
    
    # Base paths for fan devices
    TACH_HWMON_PATH = "/sys/class/hwmon/hwmon0"  # aspeed_tach
    PWM_HWMON_BASE = "/sys/class/hwmon/hwmon"    # pwm-fan devices (hwmon1-9)
    
    # PWM range (0-255)
    PWM_MAX = 255
    
    def __init__(self, fan_index, is_psu_fan=False):
        """
        Initialize a Fan object
        
        Args:
            fan_index: 0-based index of the fan (0-15 for TACH, 0-8 for PWM)
            is_psu_fan: True if this is a PSU fan, False otherwise
        """
        FanBase.__init__(self)
        
        self.index = fan_index
        self.is_psu_fan = is_psu_fan
        self.name = f"Fan{fan_index + 1}"
        
        # TACH input path (1-based in sysfs)
        self.tach_input_path = os.path.join(
            self.TACH_HWMON_PATH,
            f"fan{fan_index + 1}_input"
        )
        
        # PWM control path (for fans 0-8, maps to hwmon1-9)
        # pwm-fan0 -> hwmon8, pwm-fan1 -> hwmon9, pwm-fan2 -> hwmon1, etc.
        # We'll need to find the correct hwmon device
        self.pwm_path = None
        if fan_index < 9:
            self._find_pwm_path(fan_index)
    
    def _find_pwm_path(self, fan_index):
        """
        Find the PWM control path for this fan
        
        Args:
            fan_index: 0-based fan index
        """
        # Search for the pwm-fan device
        for hwmon_num in range(1, 10):
            hwmon_path = f"{self.PWM_HWMON_BASE}{hwmon_num}"
            device_path = os.path.join(hwmon_path, "device")
            
            try:
                # Read the device name
                device_name = os.path.basename(os.readlink(device_path))
                if device_name == f"pwm-fan{fan_index}":
                    self.pwm_path = os.path.join(hwmon_path, "pwm1")
                    return
            except (OSError, IOError):
                continue
    
    def _read_sysfs_file(self, path):
        """
        Read a value from a sysfs file
        
        Args:
            path: Path to the sysfs file
            
        Returns:
            String value from the file, or None if read fails
        """
        try:
            with open(path, 'r') as f:
                return f.read().strip()
        except (IOError, OSError):
            return None
    
    def _write_sysfs_file(self, path, value):
        """
        Write a value to a sysfs file
        
        Args:
            path: Path to the sysfs file
            value: Value to write
            
        Returns:
            True if write succeeds, False otherwise
        """
        try:
            with open(path, 'w') as f:
                f.write(str(value))
            return True
        except (IOError, OSError):
            return False
    
    def get_name(self):
        """
        Retrieves the name of the fan
        
        Returns:
            String containing the name of the fan
        """
        return self.name
    
    def get_presence(self):
        """
        Retrieves the presence of the fan
        
        Returns:
            True if fan is present, False if not
        """
        # On BMC, fans are always "present" if the TACH input exists
        return os.path.exists(self.tach_input_path)
    
    def get_model(self):
        """
        Retrieves the model of the fan
        
        Returns:
            String containing the model of the fan
        """
        return "AST2700-FAN"
    
    def get_serial(self):
        """
        Retrieves the serial number of the fan

        Returns:
            String containing the serial number of the fan
        """
        return "N/A"

    def get_status(self):
        """
        Retrieves the operational status of the fan

        Returns:
            True if fan is operating properly, False if not
        """
        # Fan is operational if it's present and speed can be read
        if not self.get_presence():
            return False

        speed_rpm = self.get_speed_rpm()
        return speed_rpm is not None

    def get_direction(self):
        """
        Retrieves the direction of fan

        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        # BMC fans don't have direction detection on EVB
        return self.FAN_DIRECTION_NOT_APPLICABLE

    def get_speed_rpm(self):
        """
        Retrieves the speed of fan in RPM

        Returns:
            An integer, the fan speed in RPM, or None if not available
        """
        value_str = self._read_sysfs_file(self.tach_input_path)
        if value_str is None:
            return None

        try:
            return int(value_str)
        except ValueError:
            return None

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
            to 100 (full speed)
        """
        # Get current PWM value
        if self.pwm_path is None or not os.path.exists(self.pwm_path):
            # If no PWM control, estimate from RPM
            # Assume max speed is 10000 RPM (platform-specific)
            rpm = self.get_speed_rpm()
            if rpm is None:
                return 0
            max_rpm = 10000
            speed_percent = min(100, int((rpm * 100) / max_rpm))
            return speed_percent

        pwm_str = self._read_sysfs_file(self.pwm_path)
        if pwm_str is None:
            return 0

        try:
            pwm_value = int(pwm_str)
            # Convert PWM (0-255) to percentage (0-100)
            return int((pwm_value * 100) / self.PWM_MAX)
        except ValueError:
            return 0

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
            to 100 (full speed)
        """
        # Target speed is the same as current speed (no separate target register)
        return self.get_speed()

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan

        Returns:
            An integer, the percentage of variance from target speed which is
            considered tolerable
        """
        # Default tolerance of 20%
        return 20

    def set_speed(self, speed):
        """
        Sets the fan speed

        Args:
            speed: An integer, the percentage of full fan speed to set fan to,
                   in the range 0 (off) to 100 (full speed)

        Returns:
            A boolean, True if speed is set successfully, False if not
        """
        if self.is_psu_fan:
            # PSU fans are controlled by the PSU itself
            return False

        if speed < 0 or speed > 100:
            return False

        if self.pwm_path is None or not os.path.exists(self.pwm_path):
            return False

        # Convert percentage (0-100) to PWM value (0-255)
        pwm_value = int((speed * self.PWM_MAX) / 100)

        return self._write_sysfs_file(self.pwm_path, pwm_value)

    def set_status_led(self, color):
        """
        Sets the state of the fan status LED

        Args:
            color: A string representing the color with which to set the
                   fan status LED

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        # BMC fans don't have individual status LEDs
        return False

    def get_status_led(self):
        """
        Gets the state of the fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings
        """
        # BMC fans don't have individual status LEDs
        return self.STATUS_LED_COLOR_OFF

