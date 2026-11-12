import threading
import time
from sonic_py_common.logger import Logger

logger = Logger()

class LedController:
    """
    CPLD LED controller via register 0x77.
    Software blink support (3Hz).
    """

    STATUS_LED_COLOR_OFF = "off"

    STATUS_LED_COLOR_GREEN = "green"
    STATUS_LED_COLOR_GREEN_BLINK = "green_blink"

    STATUS_LED_COLOR_AMBER = "amber"
    STATUS_LED_COLOR_AMBER_BLINK = "amber_blink"

    STATUS_LED_COLOR_RED = "red"
    STATUS_LED_COLOR_RED_BLINK = "red_blink"

    STATUS_LED_COLOR_YELLOW = "yellow"
    STATUS_LED_COLOR_YELLOW_BLINK = "yellow_blink"

    REG = 0x77

    LED_SYSTEM = "system"
    LED_STACK = "stack"
    LED_MST = "mst"
    LED_PSU = "psu"
    LED_FAN = "fan"

    BLINK_PERIOD = 0.33  # ~3Hz



    def __init__(self, cpld):
        self.cpld = cpld

        self._blink_threads = {}
        self._blink_stop = {}
        self._blink_color = {}

    # -------------------------------------------------
    # Low-level
    # -------------------------------------------------

    def _read(self):
        return self.cpld.read_register(self.REG)

    def _write(self, val):
        self.cpld.write_register(self.REG, val)

    def _normalize_color(self, color):
        if color.endswith("_blink"):
            return color.replace("_blink", ""), True
        return color, False

    # -------------------------------------------------
    # HW SET (no blink)
    # -------------------------------------------------

    def _set_hw(self, name, base_color):
        try:
            state = self._read()

            # SYSTEM (bits 1-0)
            if name == self.LED_SYSTEM:
                state &= ~0b11

                if base_color == self.STATUS_LED_COLOR_GREEN:
                    state |= 0b00
                elif base_color in ("yellow", "amber", "red"):
                    state |= 0b01

            # STACK (bit 4, active low)
            elif name == self.LED_STACK:
                if base_color == self.STATUS_LED_COLOR_GREEN:
                    state &= ~(1 << 4)
                else:
                    state |= (1 << 4)

            # MST (bit 3)
            elif name == self.LED_MST:
                if base_color == self.STATUS_LED_COLOR_GREEN:
                    state &= ~(1 << 3)
                else:
                    state |= (1 << 3)

            # PSU (bits 6-5)
            elif name == self.LED_PSU:
                state &= ~(0b11 << 5)

                if base_color == self.STATUS_LED_COLOR_GREEN:
                    state |= (0b01 << 5)
                elif base_color in ("yellow", "amber", "red"):
                    state |= (0b10 << 5)

            elif name == self.LED_FAN:
                raise NotImplementedError("FAN LED not defined")

            else:
                raise ValueError("Unknown LED")

            self._write(state)

        except Exception as e:
            logger.log_warning(f"Failed to set LED {name} to {base_color}: {e}")

    # -------------------------------------------------
    # Blink logic
    # -------------------------------------------------

    def _stop_blink(self, name):
        if name in self._blink_stop:
            self._blink_stop[name].set()

            # wait thread to exit
            t = self._blink_threads.get(name)
            if t:
                t.join(timeout=1)

            del self._blink_stop[name]
            del self._blink_threads[name]

    def _start_blink(self, name, base_color):
        self._blink_color[name] = base_color
        self._stop_blink(name)

        stop_event = threading.Event()
        self._blink_stop[name] = stop_event

        def _blink():
            while not stop_event.is_set():
                self._set_hw(name, base_color)
                time.sleep(self.BLINK_PERIOD / 2)

                if stop_event.is_set():
                    break

                self._set_hw(name, self.STATUS_LED_COLOR_OFF)
                time.sleep(self.BLINK_PERIOD / 2)

        t = threading.Thread(target=_blink, daemon=True)
        self._blink_threads[name] = t
        t.start()

    # -------------------------------------------------
    # API
    # -------------------------------------------------

    def set_led(self, name, color):
        logger.log_info(f"SET LED: {name} -> {color}")
        base_color, blink = self._normalize_color(color)
        '''
        if blink:
            # restart blink if color changed
            if name in self._blink_stop and self._blink_color.get(name) == base_color:
                return
            self._start_blink(name, base_color)
        else:
            self._stop_blink(name)
            self._set_hw(name, base_color)
        '''
        self._set_hw(name, base_color)

    def get_led(self, name):
        state = self._read()

        # SYSTEM
        if name == self.LED_SYSTEM:
            val = state & 0b11
            color = (
                self.STATUS_LED_COLOR_GREEN
                if val == 0
                else self.STATUS_LED_COLOR_AMBER
            )

        # STACK
        elif name == self.LED_STACK:
            color = (
                self.STATUS_LED_COLOR_OFF
                if (state & (1 << 4))
                else self.STATUS_LED_COLOR_GREEN
            )

        # MST
        elif name == self.LED_MST:
            color = (
                self.STATUS_LED_COLOR_OFF
                if (state & (1 << 3))
                else self.STATUS_LED_COLOR_GREEN
            )

        # PSU
        elif name == self.LED_PSU:
            val = (state >> 5) & 0b11
            if val == 0b01:
                color = self.STATUS_LED_COLOR_GREEN
            elif val == 0b10:
                color = self.STATUS_LED_COLOR_RED
            else:
                color = self.STATUS_LED_COLOR_OFF

        else:
            raise ValueError("Unknown LED")

        # Check if blinking
        if name in self._blink_stop:
            color += "_blink"

        return color