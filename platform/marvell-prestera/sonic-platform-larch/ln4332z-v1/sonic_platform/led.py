import os

from .i2c import *

# The control PLD answers at I2C addresses 0x60-0x67 (one "register" per
# address).  It hangs off channel 0 of the PCA9548 at 0x71 on the CP2112
# USB-I2C bridge that carries the system devices ("M_I2C2" in the board
# spec), which is why it is not visible on the bridge itself.  Neither the
# bridge's adapter number nor the mux's child adapter numbers are fixed --
# both are handed out at boot -- so the bus is discovered, see pld_bus().
LED_I2CADDR = 0x60
PLD_MUX_ADDR = 0x71
PLD_MUX_CHANNEL = 0
PLD_VERSION_ADDR = 0x67
I2C_SYSFS = "/sys/bus/i2c/devices"
IDLED_MASK =  0b0000_0011
PWRLED_MASK = 0b0000_1100
SYSLED_MASK = 0b0011_0000
FANLED_MASK = 0b1100_0000

# Each LED occupies a 2-bit field in the register (FPGA spec, reg 0x60):
#   bit0 ID green   bit1 ID red    bit2 PWR green  bit3 PWR red
#   bit4 SYS green  bit5 SYS red   bit6 FAN green  bit7 FAN red
# Within a field: low bit = green, high bit = red, both = amber.
IDLED_STATES = {
    "STATUS_LED_COLOR_OFF":   0b0000_0000,
    "STATUS_LED_COLOR_GREEN": 0b0000_0001,
    "STATUS_LED_COLOR_RED":   0b0000_0010,
    "STATUS_LED_COLOR_AMBER": 0b0000_0011,
}

PWRLED_STATES = {
    "STATUS_LED_COLOR_OFF":   0b0000_0000,
    "STATUS_LED_COLOR_GREEN": 0b0000_0100,
    "STATUS_LED_COLOR_RED":   0b0000_1000,
    "STATUS_LED_COLOR_AMBER": 0b0000_1100,
}

SYSLED_STATES = {
    "STATUS_LED_COLOR_OFF":   0b0000_0000,
    "STATUS_LED_COLOR_GREEN": 0b0001_0000,
    "STATUS_LED_COLOR_RED":   0b0010_0000,
    "STATUS_LED_COLOR_AMBER": 0b0011_0000,
}

FANLED_STATES = {
    "STATUS_LED_COLOR_OFF":   0b0000_0000,
    "STATUS_LED_COLOR_GREEN": 0b0100_0000,
    "STATUS_LED_COLOR_RED":   0b1000_0000,
    "STATUS_LED_COLOR_AMBER": 0b1100_0000,
}

class I2CRegisterBitField:
    def __init__(self, i2c_bus, i2c_addr, reg, mask, states):
        self.i2c_bus = i2c_bus
        self.i2c_addr = i2c_addr
        self.reg = reg
        self.mask = mask
        self.states = states
        self.reversed_states = {v: k for k, v in states.items()}
        
    def set_value(self, value):
        # Read the current register value to preserve the other bits
        reg_val = i2c_read_byte_data(self.i2c_bus, self.i2c_addr, self.reg)
        
        # Make sure the value is masked
        value &= self.mask
        
        # Clear the bits
        reg_val &= ~self.mask
        
        # Set the bits to the value
        reg_val |= value
        
        # Write the new value to the register
        i2c_write_byte_data(self.i2c_bus, self.i2c_addr, self.reg, reg_val)
    
    def get_value(self):
        # Read the current register value
        reg_val = i2c_read_byte_data(self.i2c_bus, self.i2c_addr, self.reg)
        
        # Mask the value
        reg_val &= self.mask
        
        return reg_val
    
    def set_to_state(self, state):   
        # Get the target state, it has to be a valid state passed by the parameter
        try :
            target_state_value = self.states[state]
        except KeyError:
            raise ValueError(f"Invalid state {state}")
        
        self.set_value(target_state_value)
        
    def get_state(self):
        value = self.get_value()
        
        try:
            state = self.reversed_states[value]
        except KeyError:
            state = "INVALID"
        
        return state


class I2CLed(object):
    def __init__(self, i2c_register_bit_field):
        self.register_bit_field = i2c_register_bit_field
    
    def set_state(self, state):
        if state not in self.register_bit_field.states:
            state = "STATUS_LED_COLOR_" + state.upper()
        self.register_bit_field.set_to_state(state)
        return True
        
    def get_state(self):
        state = self.register_bit_field.get_state()
        return state.replace("STATUS_LED_COLOR_", "").lower()


def _adapter_name(bus):
    try:
        with open(os.path.join(I2C_SYSFS, "i2c-{}".format(bus), "name")) as f:
            return f.read().strip()
    except OSError:
        return ""


def _mux_channel_bus(bus, addr, channel):
    """Return the adapter number of one channel of a mux, or None."""
    link = os.path.join(I2C_SYSFS, "{}-{:04x}".format(bus, addr),
                        "channel-{}".format(channel))
    if not os.path.islink(link):
        return None
    target = os.path.basename(os.path.realpath(link))
    if not target.startswith("i2c-"):
        return None
    try:
        return int(target[len("i2c-"):])
    except ValueError:
        return None


def _find_pld_bus():
    """Walk sysfs for the adapter the control PLD sits on, or return None.

    Do NOT go looking for it by probing addresses on the CP2112 bridges: a
    transfer to an address that does not answer wedges the bridge until the
    board is power-cycled.  The topology is fixed by the board, only the
    numbering moves, so follow the topology and read a single address at the
    end of it to confirm.
    """
    try:
        entries = os.listdir(I2C_SYSFS)
    except OSError:
        return None

    for entry in sorted(entries):
        if not entry.startswith("i2c-"):
            continue
        try:
            bus = int(entry[len("i2c-"):])
        except ValueError:
            continue
        if not _adapter_name(bus).startswith("CP2112"):
            continue
        pld = _mux_channel_bus(bus, PLD_MUX_ADDR, PLD_MUX_CHANNEL)
        if pld is None:
            continue
        if i2c_try_read_byte(pld, PLD_VERSION_ADDR) is not None:
            return pld
    return None


_pld_bus = None
_pld_bus_searched = False


def pld_bus():
    """Return the i2c adapter number that carries the control PLD, or None.

    The control PLD holds the front-panel LEDs (0x60), the software reset
    (0x61), the fan PWM (0x62) and the fan presence / reset button state
    (0x63).  Addresses 0x64-0x66 are not implemented on this board: they
    ignore writes and read back the version byte from 0x67.
    """
    global _pld_bus, _pld_bus_searched
    if not _pld_bus_searched:
        _pld_bus = _find_pld_bus()
        _pld_bus_searched = True
    return _pld_bus
