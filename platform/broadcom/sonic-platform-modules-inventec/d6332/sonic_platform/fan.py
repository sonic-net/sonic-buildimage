#!/usr/bin/env python
#
# Name: fan.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs 
#

try:
    import math
    import os
    import logging
    from sonic_platform_base.fan_base import FanBase
    from sonic_platform.inv_const import  FanConst , PsuConst, Common
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

MAX_SPEED_OF_FAN_FRONT = 23500
MAX_SPEED_OF_FAN_BACK  = 19900
MAX_SPEED_OF_FAN_PSU   = 26000
MAX_PWM_OF_FAN         = 255

class Fan(FanBase):

    __name_of_fans = ['FAN1','FAN2','FAN3','FAN4','FAN5','PSU1_FAN1','PSU2_FAN1']
    __start_of_psu_fans = FanConst().PSU_FAN_START_INDEX

    def __init__(self, index):
        self.__index = index

        if self.__index >= self.__start_of_psu_fans:
            psu_id=self.__index- self.__start_of_psu_fans
            self.__presence_attr  = "{}/i2c-inv_cpld/psu{}".format(Common.I2C_PREFIX,psu_id+1)
            self.__rpm1_attr      = "{}/psu{}/fan1_input".format(Common.INFO_PREFIX, psu_id+1)
        else:
            self.__fan_type       = "{}/i2c-inv_cpld/fanmodule{}_type".format(Common.I2C_PREFIX, self.__index + 1)
            self.__rpm1_attr      = "{}/i2c-inv_cpld/fan{}_input".format(Common.I2C_PREFIX, 2*self.__index + 1)
            self.__rpm2_attr      = "{}/i2c-inv_cpld/fan{}_input".format(Common.I2C_PREFIX, 2*self.__index + 2)
            self.__pwm_attr       = "{}/i2c-inv_cpld/pwm{}".format(Common.I2C_PREFIX, self.__index + 1)

    def __get_attr_value(self, attr_path):

        retval = 'ERR'
        if (not os.path.isfile(attr_path)):
            return retval

        try:
            with open(attr_path, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open ", attr_path, " file !")

        retval = retval.rstrip(' \t\n\r')
        return retval

    def read_fru(self, attr_type):
        fan_addr=FanConst.FAN_VPD_ADDR_BASE+self.__index
        path="/sys/bus/i2c/devices/{}-00{}/eeprom".format(FanConst.FAN_VPD_CHANNEL, hex(fan_addr)[2:] )
        content=[]
        attr_idx=0
        attr_length=0

        if(os.path.exists(path)):
            with open(path,'rw') as f:
                content=f.read()
            target_offset=ord(content[FanConst.TLV_PRODUCT_INFO_OFFSET_IDX-1])
            target_offset*=8  #spec defined: offset are in multiples of 8 bytes

            attr_idx=target_offset+FanConst.TLV_PRODUCT_INFO_AREA_START
            for i in range(1,attr_type):
                if attr_idx > len(content):
                    raise SyntaxError
                attr_length=(ord(content[attr_idx]))&(0x3f)
                attr_idx+=(attr_length+1);

            attr_length=(ord(content[attr_idx]))&(0x3f)
            attr_idx+=1
        else:
                logging.error("[FAN] Can't find path to eeprom : %s" % path)
                return SyntaxError
        
        return content[attr_idx:attr_idx+attr_length]


##############################################
# Device methods
##############################################

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return self.__name_of_fans[self.__index]

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        presence = False

        if self.__index >= self.__start_of_psu_fans:
            #check fan of psu presence if psu presence
            attr_path = self.__presence_attr
            attr_rv = self.__get_attr_value(attr_path)
            if (attr_rv != 'ERR'):
                if (attr_rv == PsuConst.PSU_TYPE_LIST[0] or attr_rv == PsuConst.PSU_TYPE_LIST[1]):
                    presence = True
            else:
                raise SyntaxError
        else:
            attr_path = self.__fan_type
            attr_rv = self.__get_attr_value(attr_path)
            if (attr_rv != 'ERR'):
                if(attr_rv==FanConst.FAN_TYPE_LIST[0] or attr_rv==FanConst.FAN_TYPE_LIST[1]):
                    presence = True
            else:
                raise SyntaxError

        return presence

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        if self.__index >= self.__start_of_psu_fans:
            return NotImplementedError
        else:
            model=self.read_fru(FanConst.TLV_ATTR_TYPE_MODEL)
            if not model:
                return NotImplementedError

        return model

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        if self.__index >= self.__start_of_psu_fans:
            return NotImplementedError
        else:
            serial=self.read_fru(FanConst.TLV_ATTR_TYPE_SERIAL)
            if not serial:
                return NotImplementedError

        return serial

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        status = False

        if self.__index >= self.__start_of_psu_fans:
            #check fan of psu presence if psu presence
            attr_path = self.__presence_attr
            attr_rv = self.__get_attr_value(attr_path)
            if (attr_rv != 'ERR'):
                if (attr_rv == PsuConst.PSU_TYPE_LIST[1]):
                    status = True
            else:
                raise SyntaxError
        else:
            status = self.get_presence()

        return status

##############################################
# FAN methods
##############################################

    def get_direction(self):
        """
        Retrieves the direction of fan

        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        direction = 'N/A'

        if self.__index >= self.__start_of_psu_fans:
            raise NotImplementedError
        else:
            attr_path = self.__fan_type
            attr_rv = self.__get_attr_value(attr_path)
            if (attr_rv != 'ERR'):

                #"Normal Type",   //00
                #"REVERSAL Type", //01
                #"UNPLUGGED",     //10
                #"UNPLUGGED",     //11

                if(attr_rv==FanConst.FAN_TYPE_LIST[0]):
                    direction = 'FAN_DIRECTION_EXHAUST'
                elif(attr_rv==FanConst.FAN_TYPE_LIST[1]):
                    direction = 'FAN_DIRECTION_INTAKE'
            else:
                raise SyntaxError

        return direction

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        speed = 0

        if self.__index >= self.__start_of_psu_fans:
            attr_rv1 = self.__get_attr_value(self.__presence_attr)
            if( attr_rv1 == PsuConst.PSU_TYPE_LIST[0] or attr_rv1 == PsuConst.PSU_TYPE_LIST[1] ):
                attr_path1 = self.__rpm1_attr
                attr_rv1 = self.__get_attr_value(attr_path1)
                if (attr_rv1 != 'ERR' ):
                    speed = int(attr_rv1) * 100 / MAX_SPEED_OF_FAN_PSU
            elif(attr_rv1 == 'ERR' ):
                raise SyntaxError
        else:
            attr_path1 = self.__rpm1_attr
            attr_path2 = self.__rpm2_attr

            if self.get_presence() and None != attr_path1:
                attr_rv1 = self.__get_attr_value(attr_path1)
                attr_rv2 = self.__get_attr_value(attr_path2)
                if (attr_rv1 != 'ERR' and attr_rv2 != 'ERR'):
                    fan1_input = int(attr_rv1)
                    speed = math.ceil(float(fan1_input * 100 / MAX_SPEED_OF_FAN_FRONT))
                    fan2_input = int(attr_rv2)
                    speed += math.ceil(float(fan2_input * 100 / MAX_SPEED_OF_FAN_BACK))
                    speed /= 2
                elif (attr_rv1 != 'ERR'):
                    fan1_input = int(attr_rv1)
                    if self.__index >= self.__start_of_psu_fans:
                        speed = speed = math.ceil(float(fan1_input * 100 / MAX_SPEED_OF_FAN_PSU))
                    else:
                        speed = math.ceil(float(fan1_input * 100 / MAX_SPEED_OF_FAN_FRONT))
                elif (attr_rv2 != 'ERR'):
                    fan2_input = int(attr_rv2)
                    speed += math.ceil(float(fan2_input * 100 / MAX_SPEED_OF_FAN_BACK))
                else:
                    raise SyntaxError

        return speed

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        speed = 0

        if self.__index >= self.__start_of_psu_fans:
            return NotImplementedError
        else:
            attr_path = self.__pwm_attr

            if self.get_presence() and None != attr_path:
                attr_rv = self.__get_attr_value(attr_path)
                if (attr_rv != 'ERR'):
                    pwm = int(attr_rv)
                    speed = math.ceil(float(pwm * 100 / MAX_PWM_OF_FAN))
                else:
                    raise SyntaxError

        return speed

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan

        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        raise NotImplementedError

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

    def get_status_led(self):
        """
        Gets the state of the fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        raise NotImplementedError
