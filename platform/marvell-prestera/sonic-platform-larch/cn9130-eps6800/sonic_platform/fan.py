from sonic_platform_base.fan_base import FanBase

# PWM registers
FAN_PWM_REG = [0x57, 0x58, 0x59, 0x60]

# Speed registers
FAN_SPEED_REG = [0x61, 0x62, 0x63, 0x64]
FAN_ABS_INT_REG = 0x85

RAW_MAX = 0xB0  # value at 100% PWM

FANTRAY_NAME_LIST = [
    "FANTRAY1",
    "FANTRAY2",
    "FANTRAY3",
    "FANTRAY4"
]

class Fan(FanBase):
    """
    CPLD controlled fan
    """

    def __init__(self, fan_tray_index, fan_index, cpld):
        super().__init__()

        self.fan_tray_index = fan_tray_index
        self.fan_index = fan_index
        self.name = f"Fan{fan_index}"
        self.cpld = cpld

    # -------------------------------------------------
    # Low level I2C helpers
    # -------------------------------------------------

    def _read_byte(self, reg):
        return self.cpld.read_register(reg)

    def _write_byte(self, reg, val):
        self.cpld.write_register(reg, val)

    # -------------------------------------------------

    def _get_pwm_reg(self):
        return FAN_PWM_REG[self.fan_tray_index]

    def _get_speed_reg(self):
        return FAN_SPEED_REG[self.fan_tray_index]

    def _read_speed_raw(self):
        return self._read_byte(self._get_speed_reg())

    # -------------------------------------------------
    # SONiC API
    # -------------------------------------------------

    def get_name(self):
        return FANTRAY_NAME_LIST[self.fan_tray_index] + "_" + self.name

    def get_presence(self):
        return not self._read_byte(FAN_ABS_INT_REG) & (1 << self.fan_tray_index)

    def get_status(self):
        return self.get_presence()
    
    def get_speed(self):
        raw = self._read_speed_raw()

        if raw == 0:
            return 0

        percent = int((raw * 100) / RAW_MAX)

        if percent > 100:
            percent = 100

        return percent

    def set_speed(self, percent):

        if percent < 0:
            percent = 0
        if percent > 100:
            percent = 100

        pwm = int((percent * 255) / 100)

        self._write_byte(self._get_pwm_reg(), pwm)

        return True

    def get_status_led(self):

        if not self.get_presence():
            return self.STATUS_LED_COLOR_OFF

        return self.STATUS_LED_COLOR_GREEN

    def set_status_led(self, color):
        return False
    