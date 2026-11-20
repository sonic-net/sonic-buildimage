#!/usr/bin/env python
#
# SONiC Platform API implementation for FANs
#
try:
    import os
    from sonic_platform_base.fan_base import FanBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

PSU_FAN_MAX_RPM = 26688
SPEED_TO_PWM = {86: 35, 139: 55, 192: 75, 230: 90,255: 100}

# Fan tray to fan number, i2c address mapping
qfx5200_fans = {0: {'fan1': 1, 'fan2': 2, 'addr': '2c'}, 
                1: {'fan1': 3, 'fan2': 4, 'addr': '2c'},
                2: {'fan1': 1, 'fan2': 2, 'addr': '2e'}, 
                3: {'fan1': 3, 'fan2': 4, 'addr': '2e'},
                4: {'fan1': 1, 'fan2': 2, 'addr': '2f'}}
HWMON_DIR = '/sys/bus/i2c/devices/7-00{0}/hwmon/'

PSU_HWMON_I2C_PATH ="/sys/bus/i2c/devices/{}-00{}/"
PSU_I2C_MAPPING = {
    0: {
        "num": 3,
        "addr": "58"
    },
    1: {
        "num": 4,
        "addr": "58"
    },
}

def read_txt_file(file_path):
    try:
        with open(file_path, 'r') as fd:
            data = fd.read()
            return data.strip()
    except IOError:
        pass
    return None

def write_txt_file(file_path, value):
    try:
        with open(file_path, 'w') as fd:
            fd.write(str(value))
    except IOError:
        return False
    return True
   
class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self, fan_tray_index, fan_index=0, is_psu_fan=False, psu_index=0):
        
        FanBase.__init__(self)
        
        self.is_psu_fan = is_psu_fan

        if not self.is_psu_fan:
            self.fantrayindex = fan_tray_index
            self.fanindex = fan_index

            # Logic to calculate the overall fanindex
            if (fan_index % 2):
                index = 1
            else:
                index = 0
            self.index = (fan_tray_index * 2) + index

            self.dependency = self

            _hwmon_base = HWMON_DIR.format(qfx5200_fans[fan_tray_index]['addr'])
            _hwmon_dir = os.listdir(_hwmon_base)
            self.fan_hwmon_path = os.path.join(_hwmon_base, _hwmon_dir[0])

        else:
            self.fanindex = fan_index
            self.psu_index = psu_index
            self.psu_i2c_num = PSU_I2C_MAPPING[self.psu_index]['num']
            self.psu_i2c_addr = PSU_I2C_MAPPING[self.psu_index]['addr']
            self.psu_hwmon_path = PSU_HWMON_I2C_PATH.format(self.psu_i2c_num, self.psu_i2c_addr)

    def get_direction(self):
        """
        Retrieves the direction of fan
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        if not self.is_psu_fan:
            dir_str = "/sys/devices/pci0000:00/0000:00:1c.0/0000:0f:00.0/refpga-tmc.15/fan{}{}".format(self.fantrayindex, '_type')
        else:
            # No driver support for knowing PSU fan type. PSU fan type should be same as system fans 
            dir_str = "/sys/devices/pci0000:00/0000:00:1c.0/0000:0f:00.0/refpga-tmc.15/fan1_type"

        val = read_txt_file(dir_str)
        if val is not None:
            if val == '0':#AFI
                direction=self.FAN_DIRECTION_INTAKE
            else:
                direction=self.FAN_DIRECTION_EXHAUST
        else:
            direction=self.FAN_DIRECTION_EXHAUST

        return direction

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
                         
        """
        if self.is_psu_fan:
            speed = 0
            psu_fan_path= "{}fan{}_input".format(self.psu_hwmon_path, self.fanindex)
            fan_speed_rpm = read_txt_file(psu_fan_path)
            if fan_speed_rpm is not None:
                speed = (int(fan_speed_rpm,10))*100/26688
                return int(speed if speed <= 100 else 0) 
        else:
            if  self.get_presence():
                pwm_file = "pwm{}".format(self.fanindex)          
                pwm_file_path = os.path.join(self.fan_hwmon_path, pwm_file)
                pwm = read_txt_file(pwm_file_path)
                if pwm is not None:
                    pwm_value = int(pwm)
                    return SPEED_TO_PWM[pwm_value]
                else:
                    return 0

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        Note:
            speed_pc = pwm_target/255*100
            0   : when PWM mode is use
            pwm : when pwm mode is not use
        """
        return self.get_speed() #Not supported

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan
        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        return 0 #Not supported

    def set_speed(self, speed):
        """
        Sets the fan speed
        Args:
            speed: An integer, the percentage of full fan speed to set fan to,
                   in the range 0 (off) to 100 (full speed)
        Returns:
            A boolean, True if speed is set successfully, False if not
        """

        if not self.is_psu_fan and self.get_presence():            
            pwm_file = "pwm{}".format(self.fanindex)          
            speed_path = os.path.join(self.fan_hwmon_path, pwm_file)
            return write_txt_file(speed_path, int(speed))

        return False

    def set_status_led(self, color):
        """
        Sets the state of the fan module status LED
        Args:
            color: A string representing the color with which to set the
                   fan module status LED
        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        return True #Not supported

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if FAN is present, False if not
        """
        if not self.is_psu_fan:
            present_path = "/sys/devices/pci0000:00/0000:00:1c.0/0000:0f:00.0/refpga-tmc.15/fan{}{}".format(self.fantrayindex, '_present')
            val = read_txt_file(present_path)
            return (int(val) == 1)
        else:
            fan_speed_rpm = self.get_speed()
            if fan_speed_rpm is not None:
                return True if fan_speed_rpm > 0 else False
            else:
                return False

    def get_status(self):
        """
        Retrieves the operational status of the FAN
        Returns:
            bool: True if FAN is operating properly, False if not
        """
        status = True if (self.get_speed() > 0) else False

        return status

    def get_name(self):
        if self.is_psu_fan:
            return "PSU {} Fan".format(self.psu_index)
        else:
            return "FanTray{} Fan{}".format(self.fantrayindex, self.fanindex) 

    def get_model(self):
        return 'N/A'

    def get_serial(self):
        return 'N/A'

    def get_position_in_parent(self):
        return self.fanindex

    def is_replaceable(self):
        return True

    def get_status_led(self):
        return None
