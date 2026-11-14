from .i2c import *

class LedController:
    """
    Class for controlling FPGA-connected status LEDs via I2C.
    FPGA register 0x00 contains 8 bits (2 bits per LED: green, amber/red).
    """

    # Possible status LED colors
    STATUS_LED_COLOR_GREEN = "green"
    STATUS_LED_COLOR_AMBER = "amber"
    STATUS_LED_COLOR_RED = "red"   # currently not used
    STATUS_LED_COLOR_OFF = "off"

    # LED name → (green_bit, amber_bit)
    LED_MAP = {
        "loc": (0, 1),
        "pwr": (2, 3),
        "sys": (4, 5),
        "fan": (6, 7),
    }

    def __init__(self, bus_id=1, i2c_addr=0x41, reg_addr=0x00):
        """
        bus_id  - I2C bus number (1)
        i2c_addr - FPGA I2C address (0x41)
        reg_addr - status LED register address (0x00)
        """
        self.bus = bus_id
        self.addr = i2c_addr
        self.reg = reg_addr

        # Read initial state from FPGA
        self.state = i2c_read_byte_data(self.bus, self.addr, self.reg)

    def _write_state(self):
        """Write updated LED state back into the FPGA register."""
        i2c_write_byte_data(self.bus, self.addr, self.reg, self.state)

    def set_led(self, name, color):
        """
        Set LED color.

        name  : "loc" | "pwr" | "fan" | "sys"
        color : STATUS_LED_COLOR_OFF | STATUS_LED_COLOR_GREEN | STATUS_LED_COLOR_AMBER
        """
        if name not in self.LED_MAP:
            raise ValueError("Unknown LED name")

        green_bit, amber_bit = self.LED_MAP[name]

        # Read current register state to avoid overwriting other LEDs
        self.state = i2c_read_byte_data(self.bus, self.addr, self.reg)

        # Clear both LED bits
        self.state &= ~(1 << green_bit)
        self.state &= ~(1 << amber_bit)

        # Apply new state based on class color defines
        if color == self.STATUS_LED_COLOR_GREEN:
            self.state |= (1 << green_bit)

        elif color == self.STATUS_LED_COLOR_AMBER or color == self.STATUS_LED_COLOR_RED:
            self.state |= (1 << amber_bit)

        elif color == self.STATUS_LED_COLOR_OFF:
            pass  # both bits remain cleared

        else:
            raise ValueError("Invalid LED color value")

        self._write_state()

    def get_led(self, name):
        """
        Return LED color as string:
        STATUS_LED_COLOR_OFF | STATUS_LED_COLOR_GREEN | STATUS_LED_COLOR_AMBER
        """
        if name not in self.LED_MAP:
            raise ValueError("Unknown LED name")

        green_bit, amber_bit = self.LED_MAP[name]

        green_on = bool(self.state & (1 << green_bit))
        amber_on = bool(self.state & (1 << amber_bit))

        if green_on:
            return self.STATUS_LED_COLOR_GREEN
        elif amber_on:
            return self.STATUS_LED_COLOR_AMBER

        return self.STATUS_LED_COLOR_OFF
