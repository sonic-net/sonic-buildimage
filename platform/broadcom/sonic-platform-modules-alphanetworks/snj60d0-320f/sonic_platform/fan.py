#############################################################################
# Alphanetworks
#
# Module contains an implementation of SONiC Platform Base API and
# provides the fan status which are available in the platform
#
#############################################################################

try:
    from sonic_platform_base.fan_base import FanBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Fan(FanBase):
    """Platform-specific Fan class"""

    FAN_FRONT_RPM_MAX = 21000
    FAN_REAR_RPM_MAX = 19000
    PSU_FAN_RPM_MAX = 27000
    FAN_SPEED_TOLERANCE_PERCENTAGE = 20
    NUM_FANTRAYS = 6
    FANS_PERTRAY = 2

    def __init__(self, fan_index, is_psu_fan):
        self.index = fan_index + 1
        self.is_psu_fan = is_psu_fan
        FanBase.__init__(self)
        base_path = "/sys/bus/i2c/devices/"

        if self.is_psu_fan:
            psu_bus_num = [10, 11]
            psu_pmbus_address = [58, 59]
            self.psu_pmbus_path = base_path +"{}-00{}".format(psu_bus_num[self.index-1], psu_pmbus_address[self.index-1])
            # driver attribute
            self.psu_fan_rpm = "/psu_fan1_speed_rpm"
        else:
            self.fan_path = base_path + '1-005e'
            self.fantray_index = int((fan_index)/self.FANS_PERTRAY) + 1
            self.fan_index_intray = self.index - ((self.fantray_index-1)*self.FANS_PERTRAY)
            self.fan_index_intray_str = 'front' if (self.fan_index_intray==1) else 'rear'
            # driver attribute
            self.fan_present = '/fan{}_present'.format(self.fantray_index)
            self.fan_direction = '/fan{}_direction'.format(self.fantray_index)
            self.fan_speed_rpm = '/fan{}_{}_speed_rpm'.format(self.fantray_index, self.fan_index_intray_str)
            self.fan_speed_pwm = '/fan_pwm'

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        if self.is_psu_fan:
            # each PSU has 1 Fan
            return "PSU{}-FAN{}".format(self.index, 1)
        else:
            return "Fantray{}_{}".format(self.fantray_index, self.fan_index_intray)

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        speed = self.get_speed_rpm()
        status = True if (speed != 0) else False
        return status

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        if self.is_psu_fan:
            return True
        else:
            status = 0
            node = self.fan_path + self.fan_present
            try:
                with open(node, 'r') as presence_status:
                    status = int(presence_status.read())
            except IOError as e:
                return False
            return status == 1

    def get_direction(self):
        """
        Retrieves the direction of fan

        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        direction = ""
        if self.is_psu_fan:
            direction = self.FAN_DIRECTION_EXHAUST
        else:
            node = self.fan_path + self.fan_direction
            try:
                with open(node, 'r') as fan_dir:
                    val = int(fan_dir.read())
            except IOError as e:
                return direction
            if val == 1:
                direction = self.FAN_DIRECTION_INTAKE
            else:
                direction = self.FAN_DIRECTION_EXHAUST

        return direction

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        speed = 0
        if self.is_psu_fan:
            rpm = self.get_speed_rpm()
            speed = int((rpm * 100) / self.PSU_FAN_RPM_MAX)

        else:
            frpm = 0
            node = self.fan_path + self.fan_speed_rpm
            try:
                with open(node, 'r') as speed:
                    frpm = int(speed.read())
            except IOError as e:
                return 0

            if self.fan_index_intray == 1:
                speed = int((frpm * 100) / self.FAN_FRONT_RPM_MAX)
            else:
                speed =  int((frpm * 100) / self.FAN_REAR_RPM_MAX)

            if speed > 100:
                speed = 100
        
        return speed

    def get_speed_rpm(self):
        """
        Retrieves the speed of fan in RPM

        Returns:
            An integer, representing speed of the FAN in rpm
        """
        frpm = 0
        if self.is_psu_fan:
            node = self.psu_pmbus_path + self.psu_fan_rpm
        else:
            node = self.fan_path + self.fan_speed_rpm
        try:
            with open(node, 'r') as speed:
                frpm = int(speed.read())
        except IOError as e:
            return 0
        
        return frpm

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        duty = 0
        if self.is_psu_fan:
            # Target speed not supported for PSU fans
            duty = 0
        else:
            node = self.fan_path + self.fan_speed_pwm
            try:
                with open(node, 'r') as fan_duty:
                    duty = int(fan_duty.read())
                    duty = int(duty * 100 / 255)
            except IOError:
                duty = 0
        return duty

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
            print("Setting PSU fan speed is not allowed")
            return False
        else:
            if speed < 0 or speed > 100:
                return False

            node = self.fan_path + self.fan_speed_pwm
            speed = int(round(speed * 255.0 / 100))
            try:
                with open(node, 'w') as fan_duty:
                    fan_duty.write(str(speed))
            except IOError:
                return False
            return True

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan

        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """        
        return self.FAN_SPEED_TOLERANCE_PERCENTAGE

    def set_status_led(self, color):
        """
        Sets the state of the fan module status LED

        Args:
            color: A string representing the color with which to set the
                   fan module status LED

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        if self.is_psu_fan:
            # Usually there is no led for psu_fan
            return True
        else:
            from .led import FanLed
            fanled = FanLed.get_fanLed()
            return fanled.update_status()

    def get_status_led(self):
        """
        Gets the state of the fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        if self.is_psu_fan:
            # Usually no led for psu_fan hence implement a generic scheme
            if self.get_status():
                return self.STATUS_LED_COLOR_GREEN
            else:
                return self.STATUS_LED_COLOR_OFF
        else:
            from .led import FanLed
            fanled = FanLed.get_fanLed()
            return fanled.get_status()
