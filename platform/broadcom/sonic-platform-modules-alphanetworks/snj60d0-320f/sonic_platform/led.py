class Led(object):
    STATUS_LED_COLOR_GREEN = "green"
    STATUS_LED_COLOR_AMBER_BLINK = "amber_blink"
    STATUS_LED_COLOR_OFF = "off"

    value_map = {
        "on": 1,
        "off": 0,
        "blink": 4
    }
    color_map = {
        0: STATUS_LED_COLOR_OFF,
        1: STATUS_LED_COLOR_GREEN,
        4: STATUS_LED_COLOR_AMBER_BLINK
    }
    
    @staticmethod
    def get_path():
        path = "/sys/bus/i2c/devices/1-005e/"
        return path

    def set_value(self, value):
        try:
            with open(self.led_path, 'w') as led:
                led.write(str(value))
        except IOError:
            return False
            
        return True

    def get_status(self):
        status = 0
        try:
            with open(self.led_path, 'r') as led:
                status = int(led.read())
        except IOError:
            return False

        return self.color_map[status]


class SystemLed(Led):
    _systemLed = None

    @staticmethod
    def get_systemLed():
        if SystemLed._systemLed is None:
            SystemLed()
        return SystemLed._systemLed 
    
    def __init__(self):
        if SystemLed._systemLed  is not None:
            raise Exception('only one SystemLed can exist')
        else:
            self.led_path = self.get_path() + "sys_status"
            SystemLed._systemLed = self
   
    def set_status(self, color):
        if color == Led.STATUS_LED_COLOR_GREEN:
            return self.set_value(Led.value_map["on"])
        if color == Led.STATUS_LED_COLOR_AMBER_BLINK:
            return self.set_value(Led.value_map["blink"])
        if color == Led.STATUS_LED_COLOR_OFF:
            return self.set_value(Led.value_map["off"])
        return False

class PsuLed(Led):
    _psuLed = None

    @staticmethod
    def get_psuLed():
        if PsuLed._psuLed is None:
            PsuLed()
        return PsuLed._psuLed
    
    def __init__(self):
        if PsuLed._psuLed is not None:
            raise Exception('only one psuLed can exist')
        else:
            self.led_path = self.get_path() + "sys_pwr"
            PsuLed._psuLed = self

    def set_psus(self, psu_list):
        self._psu_list = psu_list
    
    def update_status(self):
        is_power_all_OK = True
        for psu in self._psu_list:
            if not psu.get_presence() or not psu.get_status():
                is_power_all_OK = False
            
        status = self.value_map["on"] if is_power_all_OK else self.value_map["blink"]
        # update led status
        return self.set_value(status)


class FanLed(Led):
    _fanLed = None

    @staticmethod
    def get_fanLed():
        if FanLed._fanLed is None:
            FanLed()
        return FanLed._fanLed
    
    def __init__(self):
        if FanLed._fanLed is not None:
            raise Exception('only one fanLed can exist')
        else:
            self.led_path = self.get_path() + "fan1_led"
            FanLed._fanLed = self

    def set_fans(self, fan_lsit):
        self._fan_list = fan_lsit
    
    def update_status(self):
        is_fan_all_OK = True
        for fan in self._fan_list:
            if not fan.get_status():
                is_fan_all_OK = False
        
        status = self.value_map["on"] if is_fan_all_OK else self.value_map["blink"]
        # update led status
        return self.set_value(status)
