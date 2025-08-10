try:
    from sonic_py_common.logger import Logger
    from sonic_platform_base.fan_base import FanBase
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

SYSLOG_IDENTIFIER = 'thermalctld'
logger = Logger(SYSLOG_IDENTIFIER)

FAN_POSITION_NAME = ["Front","Rear"]

class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self, fan_index, position_index, attr_path, psu_fan = False):
        # fan_index: the index of a fan module belongs to
        # position_index : position of the fan in a fan module, 0 -> front, 1 -> rear,
        # position_index : index of PSU belongs to if psu_fan is True, start from 0
        super(Fan, self).__init__()
        self.index = fan_index + 1
        self.position = position_index
        self.is_psu_fan = psu_fan
        self.attr_path = attr_path[0]

        if psu_fan is True:
            self.fan_name = "PSU{}_FAN{}".format(self.index, self.position+1)
            self.speed_file = attr_path[1]
        else:
            self.fan_name = "FAN{}-{}".format(self.index, FAN_POSITION_NAME[self.position])
        
    def __read_attr_file(self, filepath, line=0xFF):
        try:
            with open(filepath,'r') as fd:
                if line == 0xFF:
                    data = fd.read()
                    return data.rstrip('\r\n')
                else:
                    data = fd.readlines()
                    return data[line].rstrip('\r\n')
        except Exception as ex:
            logger.log_error("Unable to open {} due to {}".format(filepath, repr(ex)))
        
        return None
         
    def get_presence(self):
        if self.is_psu_fan is True:
            data = self.__read_attr_file(self.attr_path + 'psu_present')
            if "PSU {} is not".format(self.index) in data:
                return False
            else:
                return True
            
        data = self.__read_attr_file(self.attr_path + 'fan_present', self.index-1)
        if data is not None :
            search_str = "Fan {} is present".format(self.index)
            if search_str in data:
                return True
            
        return False

    def get_name(self):
        """
        Retrieves the name of the device
        Returns:
            string: The name of the device
        """
        return self.fan_name

    def get_direction(self):
        """
        Retrieves the direction of fan
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        return self.FAN_DIRECTION_NOT_APPLICABLE

    def get_status(self):
        if self.is_psu_fan is True:
            data = self.__read_attr_file(self.attr_path + 'psu_status')
            if "PSU {} is not".format(self.index) in data:
                return False
            else:
                return True


        data = self.__read_attr_file(self.attr_path + 'fan_status', self.index-1)
        if data is not None :
            search_str = "Fan {} is Good".format(self.index)
            if search_str in data:
                return True
                
        return False

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        if self.is_psu_fan is True and self.get_presence():
           if 'psu_fan_speed_1' in self.speed_file: #no bmc
               speed = self.__read_attr_file( self.speed_file, 0)
               if speed is not None:
                   return (int(speed)*100)//16000
               else:
                   return 0
           else:
               line = self.__read_attr_file( self.speed_file, 3)
               speed = line.split(' ')
               if "FAN_SPEED" in speed:
                   return (int(speed[-1])*100)//16000
               else:
                   return 0
        if self.get_presence():
            data = self.__read_attr_file(self.attr_path + 'fan_speed_rpm', (self.index-1)*2 + self.position)
            if data is not None:
                for sdata in data.split(' '):
                    if sdata.isdigit():
                        return (int(sdata)*100)//16000

        return 0

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        return self.get_speed()

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan
        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        return 10

    def set_speed(self, speed):
        """
        Sets the fan speed
        Args:
            speed: An integer, the percentage of full fan speed to set fan to,
                   in the range 0 (off) to 100 (full speed)
        Returns:
            A boolean, True if speed is set successfully, False if not
        """
        raise NotImplementedError

    def set_status_led(self, color):
        """
        Sets the state of the fan module status LED
        Args:
            color: A string representing the color with which to set the
                   fan module status LED
        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        raise NotImplementedError
