from .i2c import *

# The control PLD answers at I2C addresses 0x60-0x67 (one "register" per
# address) on one of the CP2112 USB-I2C bridges; the adapter number is
# assigned at boot, see pld_bus() below.
LED_I2CBUS = None
LED_I2CADDR = 0x60
IDLED_MASK =  0b0000_0011
SYSLED_MASK = 0b0000_1100
PWRLED_MASK = 0b0011_0000
FANLED_MASK = 0b1100_0000

# Each LED occupies a 2-bit field in the register.
# Within a field: high bit = green, low bit = red, both = amber.
SYSLED_STATES = {
    "STATUS_LED_COLOR_OFF":   0b0000_0000,
    "STATUS_LED_COLOR_GREEN": 0b0000_1000,
    "STATUS_LED_COLOR_RED":   0b0000_0100,
    "STATUS_LED_COLOR_AMBER": 0b0000_1100,
}

FANLED_STATES = {
    "STATUS_LED_COLOR_OFF":   0b0000_0000,
    "STATUS_LED_COLOR_GREEN": 0b1000_0000,
    "STATUS_LED_COLOR_RED":   0b0100_0000,
    "STATUS_LED_COLOR_AMBER": 0b1100_0000,
}

IDLED_STATES = {
    "STATUS_LED_COLOR_OFF":   0b0000_0000,
    "STATUS_LED_COLOR_GREEN": 0b0000_0010,
    "STATUS_LED_COLOR_RED":   0b0000_0001,
    "STATUS_LED_COLOR_AMBER": 0b0000_0011,
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


def pld_bus():
    """Return the i2c adapter number that carries the control PLD, or None.

    The control PLD (registers 0x60-0x67: LEDs, fan PWM, fan presence, reset
    button) is drawn on the CPU I2C in the HW block diagram but has not been
    located yet on board #31010: it is neither on cp0 i2c0/i2c1 nor behind the
    CP2112 bridges.  Do NOT probe the CP2112 bridges for it: a transfer to an
    unknown address wedges the system bridge until the board is powered off.
    """
    return LED_I2CBUS
