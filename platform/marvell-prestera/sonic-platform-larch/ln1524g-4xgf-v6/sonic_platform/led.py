from .i2c import *


class LedController:
    """
    CPLD Status LED controller via I2C.
    Blink state is encoded in the color string.
    """

    # LED colors
    STATUS_LED_COLOR_OFF = "off"

    STATUS_LED_COLOR_GREEN = "green"
    STATUS_LED_COLOR_GREEN_BLINK = "green_blink"

    STATUS_LED_COLOR_AMBER = "amber"
    STATUS_LED_COLOR_AMBER_BLINK = "amber_blink"

    STATUS_LED_COLOR_RED = "red"
    STATUS_LED_COLOR_RED_BLINK = "red_blink"

    STATUS_LED_COLOR_YELLOW = "yellow"
    STATUS_LED_COLOR_YELLOW_BLINK = "yellow_blink"

    # name -> (reg, green_bit, yellow_bit, blink_bit)
    LED_MAP = {
        "status1": (0xA3, 1, 0, 2),
        "status2": (0xA3, 5, 4, 6),
        "status3": (0xA4, 1, 0, 2),
        "status4": (0xA4, 5, 4, 6),
    }

    def __init__(self, bus_id=0, i2c_addr=0x30):
        self.bus = bus_id
        self.addr = i2c_addr

    def _read_reg(self, reg):
        return i2c_read_byte_data(self.bus, self.addr, reg)

    def _write_reg(self, reg, value):
        i2c_write_byte_data(self.bus, self.addr, reg, value)

    def set_led(self, name, color):
        """
        Set LED state using color string.

        name  : status1 | status2 | status3 | status4
        color : off | green | green_blink | amber | amber_blink | red | red_blink
        """
        if name not in self.LED_MAP:
            raise ValueError("Unknown LED name")

        reg, green_bit, yellow_bit, blink_bit = self.LED_MAP[name]

        state = self._read_reg(reg)

        # Clear color + blink bits
        state &= ~(1 << green_bit)
        state &= ~(1 << yellow_bit)
        state &= ~(1 << blink_bit)

        # Normalize color
        blink = color.endswith("_blink")
        base_color = color.replace("_blink", "")

        if base_color == self.STATUS_LED_COLOR_GREEN:
            state |= (1 << green_bit)

        elif base_color in (
            self.STATUS_LED_COLOR_AMBER,
            self.STATUS_LED_COLOR_RED,
            self.STATUS_LED_COLOR_YELLOW,
        ):
            state |= (1 << yellow_bit)

        elif base_color == self.STATUS_LED_COLOR_OFF:
            pass

        else:
            raise ValueError("Invalid LED color")

        if blink:
            state |= (1 << blink_bit)

        self._write_reg(reg, state)

    def get_led(self, name):
        """
        Return LED color string with embedded blink state.
        """
        if name not in self.LED_MAP:
            raise ValueError("Unknown LED name")

        reg, green_bit, yellow_bit, blink_bit = self.LED_MAP[name]

        state = self._read_reg(reg)

        green_on = bool(state & (1 << green_bit))
        yellow_on = bool(state & (1 << yellow_bit))
        blink_on = bool(state & (1 << blink_bit))

        if green_on:
            color = self.STATUS_LED_COLOR_GREEN
        elif yellow_on:
            color = self.STATUS_LED_COLOR_AMBER
        else:
            return self.STATUS_LED_COLOR_OFF

        if blink_on:
            color += "_blink"

        return color
