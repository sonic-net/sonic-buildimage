import time
from sonic_platform_base.fan_base import FanBase
from .i2c import *

I2C_BUS = 2
I2C_ADDR = 0x54

# Registers
REG_PAGE = 0x00
REG_FAN_ENABLE = 0x3A
REG_FAN_COMMAND = 0x3B
REG_FAN_SPEED = 0x90
REG_MFR_FAN_CONFIG = 0xF1

# Constants
MAX31785_NR_FAN_PAGES = 5
ACCESS_DELAY_SEC = 0.00025   # 250us

FANTRAY_NAME_LIST = ["FANTRAY1", "FANTRAY2",
                     "FANTRAY3", "FANTRAY4",
                     "FANTRAY5"]

class Fan(FanBase):
    """
    MAX31785 Fan class (RPM based control, no byte swap)
    """

    def __init__(self, fan_tray_index, fan_index):
        super().__init__()
        self.bus_idx = I2C_BUS
        self.addr = I2C_ADDR
        self.fan_index = fan_index
        self.fan_tray_index = fan_tray_index
        self.name = f"Fan{fan_index}"

        self._init_fans()

    # -------------------------------------------------
    # Low-level I2C helpers (NO SWAP)
    # -------------------------------------------------

    def _delay(self):
        time.sleep(ACCESS_DELAY_SEC)

    def _write_byte(self, reg, val):
        i2c_write_byte_data(self.bus_idx, self.addr, reg, val)
        self._delay()

    def _read_byte(self, reg):
        val = i2c_read_byte_data(self.bus_idx, self.addr, reg)
        self._delay()
        return val

    def _write_word(self, reg, val):
        i2c_write_word_data(self.bus_idx, self.addr, reg, val)
        self._delay()


    def _read_word(self, reg):
        val = i2c_read_word_data(self.bus_idx, self.addr, reg)
        self._delay()
        return val

    def _select_page(self, page):
        self._write_byte(REG_PAGE, page)

    # -------------------------------------------------
    # Initialization
    # -------------------------------------------------

    def _configure_fan(self):
        self._write_byte(REG_FAN_ENABLE, 0x00)

        self._write_word(REG_MFR_FAN_CONFIG, 0xE303)

        self._write_word(REG_FAN_COMMAND, 0x2EE0)  # ~12000 RPM

        self._write_byte(REG_FAN_ENABLE, 0xD0)

    def _init_fans(self):
        for page in range(MAX31785_NR_FAN_PAGES):
            self._select_page(page)
            self._configure_fan()

        self._select_page(0)

    # -------------------------------------------------
    # SONiC API
    # -------------------------------------------------

    def get_name(self):
        return FANTRAY_NAME_LIST[self.fan_tray_index] + "_" + self.name

    def get_presence(self):
        return self.get_speed_rpm() > 0

    def get_status(self):
        return self.get_speed_rpm() > 0

    def get_speed_rpm(self):
        self._select_page(self.fan_tray_index)
        return self._read_word(REG_FAN_SPEED)

    def set_speed_rpm(self, rpm):
        if rpm < 0:
            rpm = 0

        if rpm > 32767:
            rpm = 37677

        self._select_page(self.fan_tray_index)
        self._write_word(REG_FAN_COMMAND, rpm)

        return True

    def get_speed(self):
        rpm = self.get_speed_rpm()
        max_rpm = 23148
        percent = int((rpm * 100) / max_rpm)
        return min(percent, 100)
    
    def get_target_speed(self):
        rpm = self.get_speed_rpm()
        max_rpm = 23148
        percent = int((rpm * 100) / max_rpm)
        return min(percent, 100)
    
    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan
        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        return 10

    def set_speed(self, percent):
        max_rpm = 23148
        rpm = int((percent * max_rpm) / 100)
        return self.set_speed_rpm(rpm)

    def get_status_led(self):
        if not self.get_presence():
            return self.STATUS_LED_COLOR_OFF
        if self.get_status():
            return self.STATUS_LED_COLOR_GREEN
        return self.STATUS_LED_COLOR_RED

    def set_status_led(self, color):
        return False
